/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
** Copyright (C) 2018 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "generator.h"
#include "cbordevice.h"
#include "outputrevision.h"
#include "utils.h"
#include <QtCore/qmetatype.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qplugin.h>
#include <QtCore/qstringview.h>

#include <math.h>
#include <stdio.h>

#include <private/qmetaobject_p.h> //for the flags.
#include <private/qplugin_p.h> //for the flags.

QT_BEGIN_NAMESPACE

uint nameToBuiltinType(const QByteArray &name)
{
    if (name.isEmpty())
        return 0;

    uint tp = QMetaType::type(name.constData());
    return tp < uint(QMetaType::User) ? tp : uint(QMetaType::UnknownType);
}

/*
  Returns \c true if the type is a built-in type.
*/
bool isBuiltinType(const QByteArray &type)
 {
    int id = QMetaType::type(type.constData());
    if (id == QMetaType::UnknownType)
        return false;
    return (id < QMetaType::User);
}

static const char *metaTypeEnumValueString(int type)
 {
#define RETURN_METATYPENAME_STRING(MetaTypeName, MetaTypeId, RealType) \
    case QMetaType::MetaTypeName: return #MetaTypeName;

    switch (type) {
QT_FOR_EACH_STATIC_TYPE(RETURN_METATYPENAME_STRING)
    }
#undef RETURN_METATYPENAME_STRING
    return 0;
 }

Generator::Generator(ClassDef *classDef, const QVector<QByteArray> &metaTypes, const QHash<QByteArray, QByteArray> &knownQObjectClasses, const QHash<QByteArray, QByteArray> &knownGadgets, FILE *outfile)
    : out(outfile), cdef(classDef), metaTypes(metaTypes), knownQObjectClasses(knownQObjectClasses)
    , knownGadgets(knownGadgets)
{
    if (cdef->superclassList.size())
        purestSuperClass = cdef->superclassList.constFirst().first;
}

static inline int lengthOfEscapeSequence(const QByteArray &s, int i)
{
    if (s.at(i) != '\\' || i >= s.length() - 1)
        return 1;
    const int startPos = i;
    ++i;
    char ch = s.at(i);
    if (ch == 'x') {
        ++i;
        while (i < s.length() && is_hex_char(s.at(i)))
            ++i;
    } else if (is_octal_char(ch)) {
        while (i < startPos + 4
               && i < s.length()
               && is_octal_char(s.at(i))) {
            ++i;
        }
    } else { // single character escape sequence
        i = qMin(i + 1, s.length());
    }
    return i - startPos;
}

void Generator::strreg(const QByteArray &s)
{
    if (!strings.contains(s))
        strings.append(s);
}

int Generator::stridx(const QByteArray &s)
{
    int i = strings.indexOf(s);
    Q_ASSERT_X(i != -1, Q_FUNC_INFO, "We forgot to register some strings");
    return i;
}

// Returns the sum of all parameters (including return type) for the given
// \a list of methods. This is needed for calculating the size of the methods'
// parameter type/name meta-data.
static int aggregateParameterCount(const QVector<FunctionDef> &list)
{
    int sum = 0;
    for (int i = 0; i < list.count(); ++i)
        sum += list.at(i).arguments.count() + 1; // +1 for return type
    return sum;
}

bool Generator::registerableMetaType(const QByteArray &propertyType)
{
    if (metaTypes.contains(propertyType))
        return true;

    if (propertyType.endsWith('*')) {
        QByteArray objectPointerType = propertyType;
        // The objects container stores class names, such as 'QState', 'QLabel' etc,
        // not 'QState*', 'QLabel*'. The propertyType does contain the '*', so we need
        // to chop it to find the class type in the known QObjects list.
        objectPointerType.chop(1);
        if (knownQObjectClasses.contains(objectPointerType))
            return true;
    }

    static const QVector<QByteArray> smartPointers = QVector<QByteArray>()
#define STREAM_SMART_POINTER(SMART_POINTER) << #SMART_POINTER
        QT_FOR_EACH_AUTOMATIC_TEMPLATE_SMART_POINTER(STREAM_SMART_POINTER)
#undef STREAM_SMART_POINTER
        ;

    for (const QByteArray &smartPointer : smartPointers) {
        if (propertyType.startsWith(smartPointer + "<") && !propertyType.endsWith("&"))
            return knownQObjectClasses.contains(propertyType.mid(smartPointer.size() + 1, propertyType.size() - smartPointer.size() - 1 - 1));
    }

    static const QVector<QByteArray> oneArgTemplates = QVector<QByteArray>()
#define STREAM_1ARG_TEMPLATE(TEMPLATENAME) << #TEMPLATENAME
      QT_FOR_EACH_AUTOMATIC_TEMPLATE_1ARG(STREAM_1ARG_TEMPLATE)
#undef STREAM_1ARG_TEMPLATE
    ;
    for (const QByteArray &oneArgTemplateType : oneArgTemplates) {
        if (propertyType.startsWith(oneArgTemplateType + "<") && propertyType.endsWith(">")) {
            const int argumentSize = propertyType.size() - oneArgTemplateType.size() - 1
                                     // The closing '>'
                                     - 1
                                     // templates inside templates have an extra whitespace char to strip.
                                     - (propertyType.at(propertyType.size() - 2) == ' ' ? 1 : 0 );
            const QByteArray templateArg = propertyType.mid(oneArgTemplateType.size() + 1, argumentSize);
            return isBuiltinType(templateArg) || registerableMetaType(templateArg);
        }
    }
    return false;
}

/* returns \c true if name and qualifiedName refers to the same name.
 * If qualified name is "A::B::C", it returns \c true for "C", "B::C" or "A::B::C" */
static bool qualifiedNameEquals(const QByteArray &qualifiedName, const QByteArray &name)
{
    if (qualifiedName == name)
        return true;
    int index = qualifiedName.indexOf("::");
    if (index == -1)
        return false;
    return qualifiedNameEquals(qualifiedName.mid(index+2), name);
}

void Generator::generateCode()
{
    bool isQt = (cdef->classname == "Qt");
    bool isQObject = (cdef->classname == "QObject");
    bool isConstructible = !cdef->constructorList.isEmpty();

    // filter out undeclared enumerators and sets
    {
        QVector<EnumDef> enumList;
        for (int i = 0; i < cdef->enumList.count(); ++i) {
            EnumDef def = cdef->enumList.at(i);
            if (cdef->enumDeclarations.contains(def.name)) {
                enumList += def;
            }
            def.enumName = def.name;
            QByteArray alias = cdef->flagAliases.value(def.name);
            if (cdef->enumDeclarations.contains(alias)) {
                def.name = alias;
                enumList += def;
            }
        }
        cdef->enumList = enumList;
    }

//
// Register all strings used in data section
//
    strreg(cdef->qualified);
    registerClassInfoStrings();
    registerFunctionStrings(cdef->signalList);
    registerFunctionStrings(cdef->slotList);
    registerFunctionStrings(cdef->methodList);
    registerFunctionStrings(cdef->constructorList);
    registerByteArrayVector(cdef->nonClassSignalList);
    registerPropertyStrings();
    registerEnumStrings();

    QByteArray qualifiedClassNameIdentifier = cdef->qualified;
    qualifiedClassNameIdentifier.replace(':', '_');

//
// Build stringdata struct
//
    const int constCharArraySizeLimit = 65535;
    fprintf(out, "struct qt_meta_stringdata_%s_t {\n", qualifiedClassNameIdentifier.constData());
    fprintf(out, "    QByteArrayData data[%d];\n", strings.size());
    {
        int stringDataLength = 0;
        int stringDataCounter = 0;
        for (int i = 0; i < strings.size(); ++i) {
            int thisLength = strings.at(i).length() + 1;
            stringDataLength += thisLength;
            if (stringDataLength / constCharArraySizeLimit) {
                // save previous stringdata and start computing the next one.
                fprintf(out, "    char stringdata%d[%d];\n", stringDataCounter++, stringDataLength - thisLength);
                stringDataLength = thisLength;
            }
        }
        fprintf(out, "    char stringdata%d[%d];\n", stringDataCounter, stringDataLength);

    }
    fprintf(out, "};\n");

    // Macro that expands into a QByteArrayData. The offset member is
    // calculated from 1) the offset of the actual characters in the
    // stringdata.stringdata member, and 2) the stringdata.data index of the
    // QByteArrayData being defined. This calculation relies on the
    // QByteArrayData::data() implementation returning simply "this + offset".
    fprintf(out, "#define QT_MOC_LITERAL(idx, ofs, len) \\\n"
            "    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \\\n"
            "    qptrdiff(offsetof(qt_meta_stringdata_%s_t, stringdata0) + ofs \\\n"
            "        - idx * sizeof(QByteArrayData)) \\\n"
            "    )\n",
            qualifiedClassNameIdentifier.constData());

    fprintf(out, "static const qt_meta_stringdata_%s_t qt_meta_stringdata_%s = {\n",
            qualifiedClassNameIdentifier.constData(), qualifiedClassNameIdentifier.constData());
    fprintf(out, "    {\n");
    {
        int idx = 0;
        for (int i = 0; i < strings.size(); ++i) {
            const QByteArray &str = strings.at(i);
            fprintf(out, "QT_MOC_LITERAL(%d, %d, %d)", i, idx, str.length());
            if (i != strings.size() - 1)
                fputc(',', out);
            const QByteArray comment = str.length() > 32 ? str.left(29) + "..." : str;
            fprintf(out, " // \"%s\"\n", comment.constData());
            idx += str.length() + 1;
            for (int j = 0; j < str.length(); ++j) {
                if (str.at(j) == '\\') {
                    int cnt = lengthOfEscapeSequence(str, j) - 1;
                    idx -= cnt;
                    j += cnt;
                }
            }
        }
        fprintf(out, "\n    },\n");
    }

//
// Build stringdata array
//
    fprintf(out, "    \"");
    int col = 0;
    int len = 0;
    int stringDataLength = 0;
    for (int i = 0; i < strings.size(); ++i) {
        QByteArray s = strings.at(i);
        len = s.length();
        stringDataLength += len + 1;
        if (stringDataLength >= constCharArraySizeLimit) {
            fprintf(out, "\",\n    \"");
            stringDataLength = len + 1;
            col = 0;
        } else if (i)
            fputs("\\0", out); // add \0 at the end of each string

        if (col && col + len >= 72) {
            fprintf(out, "\"\n    \"");
            col = 0;
        } else if (len && s.at(0) >= '0' && s.at(0) <= '9') {
            fprintf(out, "\"\"");
            len += 2;
        }
        int idx = 0;
        while (idx < s.length()) {
            if (idx > 0) {
                col = 0;
                fprintf(out, "\"\n    \"");
            }
            int spanLen = qMin(70, s.length() - idx);
            // don't cut escape sequences at the end of a line
            int backSlashPos = s.lastIndexOf('\\', idx + spanLen - 1);
            if (backSlashPos >= idx) {
                int escapeLen = lengthOfEscapeSequence(s, backSlashPos);
                spanLen = qBound(spanLen, backSlashPos + escapeLen - idx, s.length() - idx);
            }
            fprintf(out, "%.*s", spanLen, s.constData() + idx);
            idx += spanLen;
            col += spanLen;
        }
        col += len + 2;
    }

// Terminate stringdata struct
    fprintf(out, "\"\n};\n");
    fprintf(out, "#undef QT_MOC_LITERAL\n\n");

//
// build the data array
//

    int index = MetaObjectPrivateFieldCount;
    fprintf(out, "static const uint qt_meta_data_%s[] = {\n", qualifiedClassNameIdentifier.constData());
    fprintf(out, "\n // content:\n");
    fprintf(out, "    %4d,       // revision\n", int(QMetaObjectPrivate::OutputRevision));
    fprintf(out, "    %4d,       // classname\n", stridx(cdef->qualified));
    fprintf(out, "    %4d, %4d, // classinfo\n", cdef->classInfoList.count(), cdef->classInfoList.count() ? index : 0);
    index += cdef->classInfoList.count() * 2;

    int methodCount = cdef->signalList.count() + cdef->slotList.count() + cdef->methodList.count();
    fprintf(out, "    %4d, %4d, // methods\n", methodCount, methodCount ? index : 0);
    index += methodCount * 5;
    if (cdef->revisionedMethods)
        index += methodCount;
    int paramsIndex = index;
    int totalParameterCount = aggregateParameterCount(cdef->signalList)
            + aggregateParameterCount(cdef->slotList)
            + aggregateParameterCount(cdef->methodList)
            + aggregateParameterCount(cdef->constructorList);
    index += totalParameterCount * 2 // types and parameter names
            - methodCount // return "parameters" don't have names
            - cdef->constructorList.count(); // "this" parameters don't have names

    fprintf(out, "    %4d, %4d, // properties\n", cdef->propertyList.count(), cdef->propertyList.count() ? index : 0);
    index += cdef->propertyList.count() * 3;
    if(cdef->notifyableProperties)
        index += cdef->propertyList.count();
    if (cdef->revisionedProperties)
        index += cdef->propertyList.count();
    fprintf(out, "    %4d, %4d, // enums/sets\n", cdef->enumList.count(), cdef->enumList.count() ? index : 0);

    int enumsIndex = index;
    for (int i = 0; i < cdef->enumList.count(); ++i)
        index += 5 + (cdef->enumList.at(i).values.count() * 2);
    fprintf(out, "    %4d, %4d, // constructors\n", isConstructible ? cdef->constructorList.count() : 0,
            isConstructible ? index : 0);

    int flags = 0;
    if (cdef->hasQGadget) {
        // Ideally, all the classes could have that flag. But this broke classes generated
        // by qdbusxml2cpp which generate code that require that we call qt_metacall for properties
        flags |= PropertyAccessInStaticMetaCall;
    }
    fprintf(out, "    %4d,       // flags\n", flags);
    fprintf(out, "    %4d,       // signalCount\n", cdef->signalList.count());


//
// Build classinfo array
//
    generateClassInfos();

//
// Build signals array first, otherwise the signal indices would be wrong
//
    generateFunctions(cdef->signalList, "signal", MethodSignal, paramsIndex);

//
// Build slots array
//
    generateFunctions(cdef->slotList, "slot", MethodSlot, paramsIndex);

//
// Build method array
//
    generateFunctions(cdef->methodList, "method", MethodMethod, paramsIndex);

//
// Build method version arrays
//
    if (cdef->revisionedMethods) {
        generateFunctionRevisions(cdef->signalList, "signal");
        generateFunctionRevisions(cdef->slotList, "slot");
        generateFunctionRevisions(cdef->methodList, "method");
    }

//
// Build method parameters array
//
    generateFunctionParameters(cdef->signalList, "signal");
    generateFunctionParameters(cdef->slotList, "slot");
    generateFunctionParameters(cdef->methodList, "method");
    if (isConstructible)
        generateFunctionParameters(cdef->constructorList, "constructor");

//
// Build property array
//
    generateProperties();

//
// Build enums array
//
    generateEnums(enumsIndex);

//
// Build constructors array
//
    if (isConstructible)
        generateFunctions(cdef->constructorList, "constructor", MethodConstructor, paramsIndex);

//
// Terminate data array
//
    fprintf(out, "\n       0        // eod\n};\n\n");

//
// Generate internal qt_static_metacall() function
//
    const bool hasStaticMetaCall = !isQt &&
            (cdef->hasQObject || !cdef->methodList.isEmpty()
             || !cdef->propertyList.isEmpty() || !cdef->constructorList.isEmpty());
    if (hasStaticMetaCall)
        generateStaticMetacall();

//
// Build extra array
//
    QVector<QByteArray> extraList;
    QHash<QByteArray, QByteArray> knownExtraMetaObject = knownGadgets;
    knownExtraMetaObject.unite(knownQObjectClasses);

    for (int i = 0; i < cdef->propertyList.count(); ++i) {
        const PropertyDef &p = cdef->propertyList.at(i);
        if (isBuiltinType(p.type))
            continue;

        if (p.type.contains('*') || p.type.contains('<') || p.type.contains('>'))
            continue;

        int s = p.type.lastIndexOf("::");
        if (s <= 0)
            continue;

        QByteArray unqualifiedScope = p.type.left(s);

        // The scope may be a namespace for example, so it's only safe to include scopes that are known QObjects (QTBUG-2151)
        QHash<QByteArray, QByteArray>::ConstIterator scopeIt;

        QByteArray thisScope = cdef->qualified;
        do {
            int s = thisScope.lastIndexOf("::");
            thisScope = thisScope.left(s);
            QByteArray currentScope = thisScope.isEmpty() ? unqualifiedScope : thisScope + "::" + unqualifiedScope;
            scopeIt = knownExtraMetaObject.constFind(currentScope);
        } while (!thisScope.isEmpty() && scopeIt == knownExtraMetaObject.constEnd());

        if (scopeIt == knownExtraMetaObject.constEnd())
            continue;

        const QByteArray &scope = *scopeIt;

        if (scope == "Qt")
            continue;
        if (qualifiedNameEquals(cdef->qualified, scope))
            continue;

        if (!extraList.contains(scope))
            extraList += scope;
    }

    // QTBUG-20639 - Accept non-local enums for QML signal/slot parameters.
    // Look for any scoped enum declarations, and add those to the list
    // of extra/related metaobjects for this object.
    for (auto it = cdef->enumDeclarations.keyBegin(),
         end = cdef->enumDeclarations.keyEnd(); it != end; ++it) {
        const QByteArray &enumKey = *it;
        int s = enumKey.lastIndexOf("::");
        if (s > 0) {
            QByteArray scope = enumKey.left(s);
            if (scope != "Qt" && !qualifiedNameEquals(cdef->qualified, scope) && !extraList.contains(scope))
                extraList += scope;
        }
    }

    if (!extraList.isEmpty()) {
        fprintf(out, "static const QMetaObject * const qt_meta_extradata_%s[] = {\n    ", qualifiedClassNameIdentifier.constData());
        for (int i = 0; i < extraList.count(); ++i) {
            fprintf(out, "    &%s::staticMetaObject,\n", extraList.at(i).constData());
        }
        fprintf(out, "    nullptr\n};\n\n");
    }

//
// Finally create and initialize the static meta object
//
    if (isQt)
        fprintf(out, "QT_INIT_METAOBJECT const QMetaObject QObject::staticQtMetaObject = { {\n");
    else
        fprintf(out, "QT_INIT_METAOBJECT const QMetaObject %s::staticMetaObject = { {\n", cdef->qualified.constData());

    if (isQObject)
        fprintf(out, "    nullptr,\n");
    else if (cdef->superclassList.size() && (!cdef->hasQGadget || knownGadgets.contains(purestSuperClass)))
        fprintf(out, "    &%s::staticMetaObject,\n", purestSuperClass.constData());
    else
        fprintf(out, "    nullptr,\n");
    fprintf(out, "    qt_meta_stringdata_%s.data,\n"
            "    qt_meta_data_%s,\n", qualifiedClassNameIdentifier.constData(),
            qualifiedClassNameIdentifier.constData());
    if (hasStaticMetaCall)
        fprintf(out, "    qt_static_metacall,\n");
    else
        fprintf(out, "    nullptr,\n");

    if (extraList.isEmpty())
        fprintf(out, "    nullptr,\n");
    else
        fprintf(out, "    qt_meta_extradata_%s,\n", qualifiedClassNameIdentifier.constData());
    fprintf(out, "    nullptr\n} };\n\n");

    if(isQt)
        return;

    if (!cdef->hasQObject)
        return;

    fprintf(out, "\nconst QMetaObject *%s::metaObject() const\n{\n    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;\n}\n",
            cdef->qualified.constData());

//
// Generate smart cast function
//
    fprintf(out, "\nvoid *%s::qt_metacast(const char *_clname)\n{\n", cdef->qualified.constData());
    fprintf(out, "    if (!_clname) return nullptr;\n");
    fprintf(out, "    if (!strcmp(_clname, qt_meta_stringdata_%s.stringdata0))\n"
                  "        return static_cast<void*>(this);\n",
            qualifiedClassNameIdentifier.constData());
    for (int i = 1; i < cdef->superclassList.size(); ++i) { // for all superclasses but the first one
        if (cdef->superclassList.at(i).second == FunctionDef::Private)
            continue;
        const char *cname = cdef->superclassList.at(i).first.constData();
        fprintf(out, "    if (!strcmp(_clname, \"%s\"))\n        return static_cast< %s*>(this);\n",
                cname, cname);
    }
    for (int i = 0; i < cdef->interfaceList.size(); ++i) {
        const QVector<ClassDef::Interface> &iface = cdef->interfaceList.at(i);
        for (int j = 0; j < iface.size(); ++j) {
            fprintf(out, "    if (!strcmp(_clname, %s))\n        return ", iface.at(j).interfaceId.constData());
            for (int k = j; k >= 0; --k)
                fprintf(out, "static_cast< %s*>(", iface.at(k).className.constData());
            fprintf(out, "this%s;\n", QByteArray(j + 1, ')').constData());
        }
    }
    if (!purestSuperClass.isEmpty() && !isQObject) {
        QByteArray superClass = purestSuperClass;
        fprintf(out, "    return %s::qt_metacast(_clname);\n", superClass.constData());
    } else {
        fprintf(out, "    return nullptr;\n");
    }
    fprintf(out, "}\n");

//
// Generate internal qt_metacall()  function
//
    generateMetacall();

//
// Generate internal signal functions
//
    for (int signalindex = 0; signalindex < cdef->signalList.size(); ++signalindex)
        generateSignal(&cdef->signalList[signalindex], signalindex);

//
// Generate plugin meta data
//
    generatePluginMetaData();

//
// Generate function to make sure the non-class signals exist in the parent classes
//
    if (!cdef->nonClassSignalList.isEmpty()) {
        fprintf(out, "// If you get a compile error in this function it can be because either\n");
        fprintf(out, "//     a) You are using a NOTIFY signal that does not exist. Fix it.\n");
        fprintf(out, "//     b) You are using a NOTIFY signal that does exist (in a parent class) but has a non-empty parameter list. This is a moc limitation.\n");
        fprintf(out, "Q_DECL_UNUSED static void checkNotifySignalValidity_%s(%s *t) {\n", qualifiedClassNameIdentifier.constData(), cdef->qualified.constData());
        for (const QByteArray &nonClassSignal : cdef->nonClassSignalList)
            fprintf(out, "    t->%s();\n", nonClassSignal.constData());
        fprintf(out, "}\n");
    }
}


void Generator::registerClassInfoStrings()
{
    for (int i = 0; i < cdef->classInfoList.size(); ++i) {
        const ClassInfoDef &c = cdef->classInfoList.at(i);
        strreg(c.name);
        strreg(c.value);
    }
}

void Generator::generateClassInfos()
{
    if (cdef->classInfoList.isEmpty())
        return;

    fprintf(out, "\n // classinfo: key, value\n");

    for (int i = 0; i < cdef->classInfoList.size(); ++i) {
        const ClassInfoDef &c = cdef->classInfoList.at(i);
        fprintf(out, "    %4d, %4d,\n", stridx(c.name), stridx(c.value));
    }
}

void Generator::registerFunctionStrings(const QVector<FunctionDef>& list)
{
    for (int i = 0; i < list.count(); ++i) {
        const FunctionDef &f = list.at(i);

        strreg(f.name);
        if (!isBuiltinType(f.normalizedType))
            strreg(f.normalizedType);
        strreg(f.tag);

        int argsCount = f.arguments.count();
        for (int j = 0; j < argsCount; ++j) {
            const ArgumentDef &a = f.arguments.at(j);
            if (!isBuiltinType(a.normalizedType))
                strreg(a.normalizedType);
            strreg(a.name);
        }
    }
}

void Generator::registerByteArrayVector(const QVector<QByteArray> &list)
{
    for (const QByteArray &ba : list)
        strreg(ba);
}

void Generator::generateFunctions(const QVector<FunctionDef>& list, const char *functype, int type, int &paramsIndex)
{
    if (list.isEmpty())
        return;
    fprintf(out, "\n // %ss: name, argc, parameters, tag, flags\n", functype);

    for (int i = 0; i < list.count(); ++i) {
        const FunctionDef &f = list.at(i);

        QByteArray comment;
        unsigned char flags = type;
        if (f.access == FunctionDef::Private) {
            flags |= AccessPrivate;
            comment.append("Private");
        } else if (f.access == FunctionDef::Public) {
            flags |= AccessPublic;
            comment.append("Public");
        } else if (f.access == FunctionDef::Protected) {
            flags |= AccessProtected;
            comment.append("Protected");
        }
        if (f.isCompat) {
            flags |= MethodCompatibility;
            comment.append(" | MethodCompatibility");
        }
        if (f.wasCloned) {
            flags |= MethodCloned;
            comment.append(" | MethodCloned");
        }
        if (f.isScriptable) {
            flags |= MethodScriptable;
            comment.append(" | isScriptable");
        }
        if (f.revision > 0) {
            flags |= MethodRevisioned;
            comment.append(" | MethodRevisioned");
        }

        int argc = f.arguments.count();
        fprintf(out, "    %4d, %4d, %4d, %4d, 0x%02x /* %s */,\n",
            stridx(f.name), argc, paramsIndex, stridx(f.tag), flags, comment.constData());

        paramsIndex += 1 + argc * 2;
    }
}

void Generator::generateFunctionRevisions(const QVector<FunctionDef>& list, const char *functype)
{
    if (list.count())
        fprintf(out, "\n // %ss: revision\n", functype);
    for (int i = 0; i < list.count(); ++i) {
        const FunctionDef &f = list.at(i);
        fprintf(out, "    %4d,\n", f.revision);
    }
}

void Generator::generateFunctionParameters(const QVector<FunctionDef>& list, const char *functype)
{
    if (list.isEmpty())
        return;
    fprintf(out, "\n // %ss: parameters\n", functype);
    for (int i = 0; i < list.count(); ++i) {
        const FunctionDef &f = list.at(i);
        fprintf(out, "    ");

        // Types
        int argsCount = f.arguments.count();
        for (int j = -1; j < argsCount; ++j) {
            if (j > -1)
                fputc(' ', out);
            const QByteArray &typeName = (j < 0) ? f.normalizedType : f.arguments.at(j).normalizedType;
            generateTypeInfo(typeName, /*allowEmptyName=*/f.isConstructor);
            fputc(',', out);
        }

        // Parameter names
        for (int j = 0; j < argsCount; ++j) {
            const ArgumentDef &arg = f.arguments.at(j);
            fprintf(out, " %4d,", stridx(arg.name));
        }

        fprintf(out, "\n");
    }
}

void Generator::generateTypeInfo(const QByteArray &typeName, bool allowEmptyName)
{
    Q_UNUSED(allowEmptyName);
    if (isBuiltinType(typeName)) {
        int type;
        const char *valueString;
        if (typeName == "qreal") {
            type = QMetaType::UnknownType;
            valueString = "QReal";
        } else {
            type = nameToBuiltinType(typeName);
            valueString = metaTypeEnumValueString(type);
        }
        if (valueString) {
            fprintf(out, "QMetaType::%s", valueString);
        } else {
            Q_ASSERT(type != QMetaType::UnknownType);
            fprintf(out, "%4d", type);
        }
    } else {
        Q_ASSERT(!typeName.isEmpty() || allowEmptyName);
        fprintf(out, "0x%.8x | %d", IsUnresolvedType, stridx(typeName));
    }
}

void Generator::registerPropertyStrings()
{
    for (int i = 0; i < cdef->propertyList.count(); ++i) {
        const PropertyDef &p = cdef->propertyList.at(i);
        strreg(p.name);
        if (!isBuiltinType(p.type))
            strreg(p.type);
    }
}

void Generator::generateProperties()
{
    //
    // Create meta data
    //

    if (cdef->propertyList.count())
        fprintf(out, "\n // properties: name, type, flags\n");
    for (int i = 0; i < cdef->propertyList.count(); ++i) {
        const PropertyDef &p = cdef->propertyList.at(i);
        uint flags = Invalid;
        if (!isBuiltinType(p.type))
            flags |= EnumOrFlag;
        if (!p.member.isEmpty() && !p.constant)
            flags |= Writable;
        if (!p.read.isEmpty() || !p.member.isEmpty())
            flags |= Readable;
        if (!p.write.isEmpty()) {
            flags |= Writable;
            if (p.stdCppSet())
                flags |= StdCppSet;
        }
        if (!p.reset.isEmpty())
            flags |= Resettable;

//         if (p.override)
//             flags |= Override;

        if (p.designable.isEmpty())
            flags |= ResolveDesignable;
        else if (p.designable != "false")
            flags |= Designable;

        if (p.scriptable.isEmpty())
            flags |= ResolveScriptable;
        else if (p.scriptable != "false")
            flags |= Scriptable;

        if (p.stored.isEmpty())
            flags |= ResolveStored;
        else if (p.stored != "false")
            flags |= Stored;

        if (p.editable.isEmpty())
            flags |= ResolveEditable;
        else if (p.editable != "false")
            flags |= Editable;

        if (p.user.isEmpty())
            flags |= ResolveUser;
        else if (p.user != "false")
            flags |= User;

        if (p.notifyId != -1)
            flags |= Notify;

        if (p.revision > 0)
            flags |= Revisioned;

        if (p.constant)
            flags |= Constant;
        if (p.final)
            flags |= Final;

        fprintf(out, "    %4d, ", stridx(p.name));
        generateTypeInfo(p.type);
        fprintf(out, ", 0x%.8x,\n", flags);
    }

    if(cdef->notifyableProperties) {
        fprintf(out, "\n // properties: notify_signal_id\n");
        for (int i = 0; i < cdef->propertyList.count(); ++i) {
            const PropertyDef &p = cdef->propertyList.at(i);
            if (p.notifyId == -1) {
                fprintf(out, "    %4d,\n",
                        0);
            } else if (p.notifyId > -1) {
                fprintf(out, "    %4d,\n",
                        p.notifyId);
            } else {
                const int indexInStrings = strings.indexOf(p.notify);
                fprintf(out, "    %4d,\n",
                        indexInStrings | IsUnresolvedSignal);
            }
        }
    }
    if (cdef->revisionedProperties) {
        fprintf(out, "\n // properties: revision\n");
        for (int i = 0; i < cdef->propertyList.count(); ++i) {
            const PropertyDef &p = cdef->propertyList.at(i);
            fprintf(out, "    %4d,\n", p.revision);
        }
    }
}

void Generator::registerEnumStrings()
{
    for (int i = 0; i < cdef->enumList.count(); ++i) {
        const EnumDef &e = cdef->enumList.at(i);
        strreg(e.name);
        if (!e.enumName.isNull())
            strreg(e.enumName);
        for (int j = 0; j < e.values.count(); ++j)
            strreg(e.values.at(j));
    }
}

void Generator::generateEnums(int index)
{
    if (cdef->enumDeclarations.isEmpty())
        return;

    fprintf(out, "\n // enums: name, alias, flags, count, data\n");
    index += 5 * cdef->enumList.count();
    int i;
    for (i = 0; i < cdef->enumList.count(); ++i) {
        const EnumDef &e = cdef->enumList.at(i);
        int flags = 0;
        if (cdef->enumDeclarations.value(e.name))
            flags |= EnumIsFlag;
        if (e.isEnumClass)
            flags |= EnumIsScoped;
        fprintf(out, "    %4d, %4d, 0x%.1x, %4d, %4d,\n",
                 stridx(e.name),
                 e.enumName.isNull() ? stridx(e.name) : stridx(e.enumName),
                 flags,
                 e.values.count(),
                 index);
        index += e.values.count() * 2;
    }

    fprintf(out, "\n // enum data: key, value\n");
    for (i = 0; i < cdef->enumList.count(); ++i) {
        const EnumDef &e = cdef->enumList.at(i);
        for (int j = 0; j < e.values.count(); ++j) {
            const QByteArray &val = e.values.at(j);
            QByteArray code = cdef->qualified.constData();
            if (e.isEnumClass)
                code += "::" + (e.enumName.isNull() ? e.name : e.enumName);
            code += "::" + val;
            fprintf(out, "    %4d, uint(%s),\n",
                    stridx(val), code.constData());
        }
    }
}

void Generator::generateMetacall()
{
    bool isQObject = (cdef->classname == "QObject");

    fprintf(out, "\nint %s::qt_metacall(QMetaObject::Call _c, int _id, void **_a)\n{\n",
             cdef->qualified.constData());

    if (!purestSuperClass.isEmpty() && !isQObject) {
        QByteArray superClass = purestSuperClass;
        fprintf(out, "    _id = %s::qt_metacall(_c, _id, _a);\n", superClass.constData());
    }


    bool needElse = false;
    QVector<FunctionDef> methodList;
    methodList += cdef->signalList;
    methodList += cdef->slotList;
    methodList += cdef->methodList;

    // If there are no methods or properties, we will return _id anyway, so
    // don't emit this comparison -- it is unnecessary, and it makes coverity
    // unhappy.
    if (methodList.size() || cdef->propertyList.size()) {
        fprintf(out, "    if (_id < 0)\n        return _id;\n");
    }

    fprintf(out, "    ");

    if (methodList.size()) {
        needElse = true;
        fprintf(out, "if (_c == QMetaObject::InvokeMetaMethod) {\n");
        fprintf(out, "        if (_id < %d)\n", methodList.size());
        fprintf(out, "            qt_static_metacall(this, _c, _id, _a);\n");
        fprintf(out, "        _id -= %d;\n    }", methodList.size());

        fprintf(out, " else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {\n");
        fprintf(out, "        if (_id < %d)\n", methodList.size());

        if (methodsWithAutomaticTypesHelper(methodList).isEmpty())
            fprintf(out, "            *reinterpret_cast<int*>(_a[0]) = -1;\n");
        else
            fprintf(out, "            qt_static_metacall(this, _c, _id, _a);\n");
        fprintf(out, "        _id -= %d;\n    }", methodList.size());

    }

    if (cdef->propertyList.size()) {
        bool needDesignable = false;
        bool needScriptable = false;
        bool needStored = false;
        bool needEditable = false;
        bool needUser = false;
        for (int i = 0; i < cdef->propertyList.size(); ++i) {
            const PropertyDef &p = cdef->propertyList.at(i);
            needDesignable |= p.designable.endsWith(')');
            needScriptable |= p.scriptable.endsWith(')');
            needStored |= p.stored.endsWith(')');
            needEditable |= p.editable.endsWith(')');
            needUser |= p.user.endsWith(')');
        }

        fprintf(out, "\n#ifndef QT_NO_PROPERTIES\n   ");
        if (needElse)
            fprintf(out, "else ");
        fprintf(out,
            "if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty\n"
            "            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {\n"
            "        qt_static_metacall(this, _c, _id, _a);\n"
            "        _id -= %d;\n    }", cdef->propertyList.count());

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::QueryPropertyDesignable) {\n");
        if (needDesignable) {
            fprintf(out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.designable.endsWith(')'))
                    continue;
                fprintf(out, "        case %d: *_b = %s; break;\n",
                         propindex, p.designable.constData());
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
        }
        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::QueryPropertyScriptable) {\n");
        if (needScriptable) {
            fprintf(out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.scriptable.endsWith(')'))
                    continue;
                fprintf(out, "        case %d: *_b = %s; break;\n",
                         propindex, p.scriptable.constData());
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
        }
        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::QueryPropertyStored) {\n");
        if (needStored) {
            fprintf(out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.stored.endsWith(')'))
                    continue;
                fprintf(out, "        case %d: *_b = %s; break;\n",
                         propindex, p.stored.constData());
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
        }
        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::QueryPropertyEditable) {\n");
        if (needEditable) {
            fprintf(out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.editable.endsWith(')'))
                    continue;
                fprintf(out, "        case %d: *_b = %s; break;\n",
                         propindex, p.editable.constData());
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
        }
        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());


        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::QueryPropertyUser) {\n");
        if (needUser) {
            fprintf(out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.user.endsWith(')'))
                    continue;
                fprintf(out, "        case %d: *_b = %s; break;\n",
                         propindex, p.user.constData());
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
        }
        fprintf(out,
                "        _id -= %d;\n"
                "    }", cdef->propertyList.count());

        fprintf(out, "\n#endif // QT_NO_PROPERTIES");
    }
    if (methodList.size() || cdef->propertyList.size())
        fprintf(out, "\n    ");
    fprintf(out,"return _id;\n}\n");
}


QMultiMap<QByteArray, int> Generator::automaticPropertyMetaTypesHelper()
{
    QMultiMap<QByteArray, int> automaticPropertyMetaTypes;
    for (int i = 0; i < cdef->propertyList.size(); ++i) {
        const QByteArray propertyType = cdef->propertyList.at(i).type;
        if (registerableMetaType(propertyType) && !isBuiltinType(propertyType))
            automaticPropertyMetaTypes.insert(propertyType, i);
    }
    return automaticPropertyMetaTypes;
}

QMap<int, QMultiMap<QByteArray, int> > Generator::methodsWithAutomaticTypesHelper(const QVector<FunctionDef> &methodList)
{
    QMap<int, QMultiMap<QByteArray, int> > methodsWithAutomaticTypes;
    for (int i = 0; i < methodList.size(); ++i) {
        const FunctionDef &f = methodList.at(i);
        for (int j = 0; j < f.arguments.count(); ++j) {
            const QByteArray argType = f.arguments.at(j).normalizedType;
            if (registerableMetaType(argType) && !isBuiltinType(argType))
                methodsWithAutomaticTypes[i].insert(argType, j);
        }
    }
    return methodsWithAutomaticTypes;
}

void Generator::generateStaticMetacall()
{
    fprintf(out, "void %s::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)\n{\n",
            cdef->qualified.constData());

    bool needElse = false;
    bool isUsed_a = false;

    if (!cdef->constructorList.isEmpty()) {
        fprintf(out, "    if (_c == QMetaObject::CreateInstance) {\n");
        fprintf(out, "        switch (_id) {\n");
        for (int ctorindex = 0; ctorindex < cdef->constructorList.count(); ++ctorindex) {
            fprintf(out, "        case %d: { %s *_r = new %s(", ctorindex,
                    cdef->classname.constData(), cdef->classname.constData());
            const FunctionDef &f = cdef->constructorList.at(ctorindex);
            int offset = 1;

            int argsCount = f.arguments.count();
            for (int j = 0; j < argsCount; ++j) {
                const ArgumentDef &a = f.arguments.at(j);
                if (j)
                    fprintf(out, ",");
                fprintf(out, "(*reinterpret_cast< %s>(_a[%d]))", a.typeNameForCast.constData(), offset++);
            }
            if (f.isPrivateSignal) {
                if (argsCount > 0)
                    fprintf(out, ", ");
                fprintf(out, "%s", QByteArray("QPrivateSignal()").constData());
            }
            fprintf(out, ");\n");
            fprintf(out, "            if (_a[0]) *reinterpret_cast<%s**>(_a[0]) = _r; } break;\n",
                    cdef->hasQGadget ? "void" : "QObject");
        }
        fprintf(out, "        default: break;\n");
        fprintf(out, "        }\n");
        fprintf(out, "    }");
        needElse = true;
        isUsed_a = true;
    }

    QVector<FunctionDef> methodList;
    methodList += cdef->signalList;
    methodList += cdef->slotList;
    methodList += cdef->methodList;

    if (!methodList.isEmpty()) {
        if (needElse)
            fprintf(out, " else ");
        else
            fprintf(out, "    ");
        fprintf(out, "if (_c == QMetaObject::InvokeMetaMethod) {\n");
        if (cdef->hasQObject) {
#ifndef QT_NO_DEBUG
            fprintf(out, "        Q_ASSERT(staticMetaObject.cast(_o));\n");
#endif
            fprintf(out, "        %s *_t = static_cast<%s *>(_o);\n", cdef->classname.constData(), cdef->classname.constData());
        } else {
            fprintf(out, "        %s *_t = reinterpret_cast<%s *>(_o);\n", cdef->classname.constData(), cdef->classname.constData());
        }
        fprintf(out, "        Q_UNUSED(_t)\n");
        fprintf(out, "        switch (_id) {\n");
        for (int methodindex = 0; methodindex < methodList.size(); ++methodindex) {
            const FunctionDef &f = methodList.at(methodindex);
            Q_ASSERT(!f.normalizedType.isEmpty());
            fprintf(out, "        case %d: ", methodindex);
            if (f.normalizedType != "void")
                fprintf(out, "{ %s _r = ", noRef(f.normalizedType).constData());
            fprintf(out, "_t->");
            if (f.inPrivateClass.size())
                fprintf(out, "%s->", f.inPrivateClass.constData());
            fprintf(out, "%s(", f.name.constData());
            int offset = 1;

            int argsCount = f.arguments.count();
            for (int j = 0; j < argsCount; ++j) {
                const ArgumentDef &a = f.arguments.at(j);
                if (j)
                    fprintf(out, ",");
                fprintf(out, "(*reinterpret_cast< %s>(_a[%d]))",a.typeNameForCast.constData(), offset++);
                isUsed_a = true;
            }
            if (f.isPrivateSignal) {
                if (argsCount > 0)
                    fprintf(out, ", ");
                fprintf(out, "%s", "QPrivateSignal()");
            }
            fprintf(out, ");");
            if (f.normalizedType != "void") {
                fprintf(out, "\n            if (_a[0]) *reinterpret_cast< %s*>(_a[0]) = std::move(_r); } ",
                        noRef(f.normalizedType).constData());
                isUsed_a = true;
            }
            fprintf(out, " break;\n");
        }
        fprintf(out, "        default: ;\n");
        fprintf(out, "        }\n");
        fprintf(out, "    }");
        needElse = true;

        QMap<int, QMultiMap<QByteArray, int> > methodsWithAutomaticTypes = methodsWithAutomaticTypesHelper(methodList);

        if (!methodsWithAutomaticTypes.isEmpty()) {
            fprintf(out, " else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {\n");
            fprintf(out, "        switch (_id) {\n");
            fprintf(out, "        default: *reinterpret_cast<int*>(_a[0]) = -1; break;\n");
            QMap<int, QMultiMap<QByteArray, int> >::const_iterator it = methodsWithAutomaticTypes.constBegin();
            const QMap<int, QMultiMap<QByteArray, int> >::const_iterator end = methodsWithAutomaticTypes.constEnd();
            for ( ; it != end; ++it) {
                fprintf(out, "        case %d:\n", it.key());
                fprintf(out, "            switch (*reinterpret_cast<int*>(_a[1])) {\n");
                fprintf(out, "            default: *reinterpret_cast<int*>(_a[0]) = -1; break;\n");
                auto jt = it->begin();
                const auto jend = it->end();
                while (jt != jend) {
                    fprintf(out, "            case %d:\n", jt.value());
                    const QByteArray &lastKey = jt.key();
                    ++jt;
                    if (jt == jend || jt.key() != lastKey)
                        fprintf(out, "                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< %s >(); break;\n", lastKey.constData());
                }
                fprintf(out, "            }\n");
                fprintf(out, "            break;\n");
            }
            fprintf(out, "        }\n");
            fprintf(out, "    }");
            isUsed_a = true;
        }

    }
    if (!cdef->signalList.isEmpty()) {
        Q_ASSERT(needElse); // if there is signal, there was method.
        fprintf(out, " else if (_c == QMetaObject::IndexOfMethod) {\n");
        fprintf(out, "        int *result = reinterpret_cast<int *>(_a[0]);\n");
        bool anythingUsed = false;
        for (int methodindex = 0; methodindex < cdef->signalList.size(); ++methodindex) {
            const FunctionDef &f = cdef->signalList.at(methodindex);
            if (f.wasCloned || !f.inPrivateClass.isEmpty() || f.isStatic)
                continue;
            anythingUsed = true;
            fprintf(out, "        {\n");
            fprintf(out, "            using _t = %s (%s::*)(",f.type.rawName.constData() , cdef->classname.constData());

            int argsCount = f.arguments.count();
            for (int j = 0; j < argsCount; ++j) {
                const ArgumentDef &a = f.arguments.at(j);
                if (j)
                    fprintf(out, ", ");
                fprintf(out, "%s", QByteArray(a.type.name + ' ' + a.rightType).constData());
            }
            if (f.isPrivateSignal) {
                if (argsCount > 0)
                    fprintf(out, ", ");
                fprintf(out, "%s", "QPrivateSignal");
            }
            if (f.isConst)
                fprintf(out, ") const;\n");
            else
                fprintf(out, ");\n");
            fprintf(out, "            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&%s::%s)) {\n",
                    cdef->classname.constData(), f.name.constData());
            fprintf(out, "                *result = %d;\n", methodindex);
            fprintf(out, "                return;\n");
            fprintf(out, "            }\n        }\n");
        }
        if (!anythingUsed)
            fprintf(out, "        Q_UNUSED(result);\n");
        fprintf(out, "    }");
        needElse = true;
    }

    const QMultiMap<QByteArray, int> automaticPropertyMetaTypes = automaticPropertyMetaTypesHelper();

    if (!automaticPropertyMetaTypes.isEmpty()) {
        if (needElse)
            fprintf(out, " else ");
        else
            fprintf(out, "    ");
        fprintf(out, "if (_c == QMetaObject::RegisterPropertyMetaType) {\n");
        fprintf(out, "        switch (_id) {\n");
        fprintf(out, "        default: *reinterpret_cast<int*>(_a[0]) = -1; break;\n");
        auto it = automaticPropertyMetaTypes.begin();
        const auto end = automaticPropertyMetaTypes.end();
        while (it != end) {
            fprintf(out, "        case %d:\n", it.value());
            const QByteArray &lastKey = it.key();
            ++it;
            if (it == end || it.key() != lastKey)
                fprintf(out, "            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< %s >(); break;\n", lastKey.constData());
        }
        fprintf(out, "        }\n");
        fprintf(out, "    }\n");
        isUsed_a = true;
        needElse = true;
    }

    if (!cdef->propertyList.empty()) {
        bool needGet = false;
        bool needTempVarForGet = false;
        bool needSet = false;
        bool needReset = false;
        for (int i = 0; i < cdef->propertyList.size(); ++i) {
            const PropertyDef &p = cdef->propertyList.at(i);
            needGet |= !p.read.isEmpty() || !p.member.isEmpty();
            if (!p.read.isEmpty() || !p.member.isEmpty())
                needTempVarForGet |= (p.gspec != PropertyDef::PointerSpec
                                      && p.gspec != PropertyDef::ReferenceSpec);

            needSet |= !p.write.isEmpty() || (!p.member.isEmpty() && !p.constant);
            needReset |= !p.reset.isEmpty();
        }
        fprintf(out, "\n#ifndef QT_NO_PROPERTIES\n    ");

        if (needElse)
            fprintf(out, "else ");
        fprintf(out, "if (_c == QMetaObject::ReadProperty) {\n");
        if (needGet) {
            if (cdef->hasQObject) {
#ifndef QT_NO_DEBUG
                fprintf(out, "        Q_ASSERT(staticMetaObject.cast(_o));\n");
#endif
                fprintf(out, "        %s *_t = static_cast<%s *>(_o);\n", cdef->classname.constData(), cdef->classname.constData());
            } else {
                fprintf(out, "        %s *_t = reinterpret_cast<%s *>(_o);\n", cdef->classname.constData(), cdef->classname.constData());
            }
            fprintf(out, "        Q_UNUSED(_t)\n");
            if (needTempVarForGet)
                fprintf(out, "        void *_v = _a[0];\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (p.read.isEmpty() && p.member.isEmpty())
                    continue;
                QByteArray prefix = "_t->";
                if (p.inPrivateClass.size()) {
                    prefix += p.inPrivateClass + "->";
                }
                if (p.gspec == PropertyDef::PointerSpec)
                    fprintf(out, "        case %d: _a[0] = const_cast<void*>(reinterpret_cast<const void*>(%s%s())); break;\n",
                            propindex, prefix.constData(), p.read.constData());
                else if (p.gspec == PropertyDef::ReferenceSpec)
                    fprintf(out, "        case %d: _a[0] = const_cast<void*>(reinterpret_cast<const void*>(&%s%s())); break;\n",
                            propindex, prefix.constData(), p.read.constData());
                else if (cdef->enumDeclarations.value(p.type, false))
                    fprintf(out, "        case %d: *reinterpret_cast<int*>(_v) = QFlag(%s%s()); break;\n",
                            propindex, prefix.constData(), p.read.constData());
                else if (!p.read.isEmpty())
                    fprintf(out, "        case %d: *reinterpret_cast< %s*>(_v) = %s%s(); break;\n",
                            propindex, p.type.constData(), prefix.constData(), p.read.constData());
                else
                    fprintf(out, "        case %d: *reinterpret_cast< %s*>(_v) = %s%s; break;\n",
                            propindex, p.type.constData(), prefix.constData(), p.member.constData());
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
        }

        fprintf(out, "    }");

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::WriteProperty) {\n");

        if (needSet) {
            if (cdef->hasQObject) {
#ifndef QT_NO_DEBUG
                fprintf(out, "        Q_ASSERT(staticMetaObject.cast(_o));\n");
#endif
                fprintf(out, "        %s *_t = static_cast<%s *>(_o);\n", cdef->classname.constData(), cdef->classname.constData());
            } else {
                fprintf(out, "        %s *_t = reinterpret_cast<%s *>(_o);\n", cdef->classname.constData(), cdef->classname.constData());
            }
            fprintf(out, "        Q_UNUSED(_t)\n");
            fprintf(out, "        void *_v = _a[0];\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (p.constant)
                    continue;
                if (p.write.isEmpty() && p.member.isEmpty())
                    continue;
                QByteArray prefix = "_t->";
                if (p.inPrivateClass.size()) {
                    prefix += p.inPrivateClass + "->";
                }
                if (cdef->enumDeclarations.value(p.type, false)) {
                    fprintf(out, "        case %d: %s%s(QFlag(*reinterpret_cast<int*>(_v))); break;\n",
                            propindex, prefix.constData(), p.write.constData());
                } else if (!p.write.isEmpty()) {
                    fprintf(out, "        case %d: %s%s(*reinterpret_cast< %s*>(_v)); break;\n",
                            propindex, prefix.constData(), p.write.constData(), p.type.constData());
                } else {
                    fprintf(out, "        case %d:\n", propindex);
                    fprintf(out, "            if (%s%s != *reinterpret_cast< %s*>(_v)) {\n",
                            prefix.constData(), p.member.constData(), p.type.constData());
                    fprintf(out, "                %s%s = *reinterpret_cast< %s*>(_v);\n",
                            prefix.constData(), p.member.constData(), p.type.constData());
                    if (!p.notify.isEmpty() && p.notifyId > -1) {
                        const FunctionDef &f = cdef->signalList.at(p.notifyId);
                        if (f.arguments.size() == 0)
                            fprintf(out, "                Q_EMIT _t->%s();\n", p.notify.constData());
                        else if (f.arguments.size() == 1 && f.arguments.at(0).normalizedType == p.type)
                            fprintf(out, "                Q_EMIT _t->%s(%s%s);\n",
                                    p.notify.constData(), prefix.constData(), p.member.constData());
                    } else if (!p.notify.isEmpty() && p.notifyId < -1) {
                        fprintf(out, "                Q_EMIT _t->%s();\n", p.notify.constData());
                    }
                    fprintf(out, "            }\n");
                    fprintf(out, "            break;\n");
                }
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
        }

        fprintf(out, "    }");

        fprintf(out, " else ");
        fprintf(out, "if (_c == QMetaObject::ResetProperty) {\n");
        if (needReset) {
            if (cdef->hasQObject) {
#ifndef QT_NO_DEBUG
                fprintf(out, "        Q_ASSERT(staticMetaObject.cast(_o));\n");
#endif
                fprintf(out, "        %s *_t = static_cast<%s *>(_o);\n", cdef->classname.constData(), cdef->classname.constData());
            } else {
                fprintf(out, "        %s *_t = reinterpret_cast<%s *>(_o);\n", cdef->classname.constData(), cdef->classname.constData());
            }
            fprintf(out, "        Q_UNUSED(_t)\n");
            fprintf(out, "        switch (_id) {\n");
            for (int propindex = 0; propindex < cdef->propertyList.size(); ++propindex) {
                const PropertyDef &p = cdef->propertyList.at(propindex);
                if (!p.reset.endsWith(')'))
                    continue;
                QByteArray prefix = "_t->";
                if (p.inPrivateClass.size()) {
                    prefix += p.inPrivateClass + "->";
                }
                fprintf(out, "        case %d: %s%s; break;\n",
                        propindex, prefix.constData(), p.reset.constData());
            }
            fprintf(out, "        default: break;\n");
            fprintf(out, "        }\n");
        }
        fprintf(out, "    }");
        fprintf(out, "\n#endif // QT_NO_PROPERTIES");
        needElse = true;
    }

    if (needElse)
        fprintf(out, "\n");

    if (methodList.isEmpty()) {
        fprintf(out, "    Q_UNUSED(_o);\n");
        if (cdef->constructorList.isEmpty() && automaticPropertyMetaTypes.isEmpty() && methodsWithAutomaticTypesHelper(methodList).isEmpty()) {
            fprintf(out, "    Q_UNUSED(_id);\n");
            fprintf(out, "    Q_UNUSED(_c);\n");
        }
    }
    if (!isUsed_a)
        fprintf(out, "    Q_UNUSED(_a);\n");

    fprintf(out, "}\n\n");
}

void Generator::generateSignal(FunctionDef *def,int index)
{
    if (def->wasCloned || def->isAbstract)
        return;
    fprintf(out, "\n// SIGNAL %d\n%s %s::%s(",
            index, def->type.name.constData(), cdef->qualified.constData(), def->name.constData());

    QByteArray thisPtr = "this";
    const char *constQualifier = "";

    if (def->isConst) {
        thisPtr = "const_cast< " + cdef->qualified + " *>(this)";
        constQualifier = "const";
    }

    Q_ASSERT(!def->normalizedType.isEmpty());
    if (def->arguments.isEmpty() && def->normalizedType == "void" && !def->isPrivateSignal) {
        fprintf(out, ")%s\n{\n"
                "    QMetaObject::activate(%s, &staticMetaObject, %d, nullptr);\n"
                "}\n", constQualifier, thisPtr.constData(), index);
        return;
    }

    int offset = 1;
    for (int j = 0; j < def->arguments.count(); ++j) {
        const ArgumentDef &a = def->arguments.at(j);
        if (j)
            fprintf(out, ", ");
        fprintf(out, "%s _t%d%s", a.type.name.constData(), offset++, a.rightType.constData());
    }
    if (def->isPrivateSignal) {
        if (!def->arguments.isEmpty())
            fprintf(out, ", ");
        fprintf(out, "QPrivateSignal _t%d", offset++);
    }

    fprintf(out, ")%s\n{\n", constQualifier);
    if (def->type.name.size() && def->normalizedType != "void") {
        QByteArray returnType = noRef(def->normalizedType);
        fprintf(out, "    %s _t0{};\n", returnType.constData());
    }

    fprintf(out, "    void *_a[] = { ");
    if (def->normalizedType == "void") {
        fprintf(out, "nullptr");
    } else {
        if (def->returnTypeIsVolatile)
             fprintf(out, "const_cast<void*>(reinterpret_cast<const volatile void*>(&_t0))");
        else
             fprintf(out, "const_cast<void*>(reinterpret_cast<const void*>(&_t0))");
    }
    int i;
    for (i = 1; i < offset; ++i)
        if (i <= def->arguments.count() && def->arguments.at(i - 1).type.isVolatile)
            fprintf(out, ", const_cast<void*>(reinterpret_cast<const volatile void*>(&_t%d))", i);
        else
            fprintf(out, ", const_cast<void*>(reinterpret_cast<const void*>(&_t%d))", i);
    fprintf(out, " };\n");
    fprintf(out, "    QMetaObject::activate(%s, &staticMetaObject, %d, _a);\n", thisPtr.constData(), index);
    if (def->normalizedType != "void")
        fprintf(out, "    return _t0;\n");
    fprintf(out, "}\n");
}

static CborError jsonValueToCbor(CborEncoder *parent, const QJsonValue &v);
static CborError jsonObjectToCbor(CborEncoder *parent, const QJsonObject &o)
{
    auto it = o.constBegin();
    auto end = o.constEnd();
    CborEncoder map;
    cbor_encoder_create_map(parent, &map, o.size());

    for ( ; it != end; ++it) {
        QByteArray key = it.key().toUtf8();
        cbor_encode_text_string(&map, key.constData(), key.size());
        jsonValueToCbor(&map, it.value());
    }
    return cbor_encoder_close_container(parent, &map);
}

static CborError jsonArrayToCbor(CborEncoder *parent, const QJsonArray &a)
{
    CborEncoder array;
    cbor_encoder_create_array(parent, &array, a.size());
    for (const QJsonValue &v : a)
        jsonValueToCbor(&array, v);
    return cbor_encoder_close_container(parent, &array);
}

static CborError jsonValueToCbor(CborEncoder *parent, const QJsonValue &v)
{
    switch (v.type()) {
    case QJsonValue::Null:
    case QJsonValue::Undefined:
        return cbor_encode_null(parent);
    case QJsonValue::Bool:
        return cbor_encode_boolean(parent, v.toBool());
    case QJsonValue::Array:
        return jsonArrayToCbor(parent, v.toArray());
    case QJsonValue::Object:
        return jsonObjectToCbor(parent, v.toObject());
    case QJsonValue::String: {
        QByteArray s = v.toString().toUtf8();
        return cbor_encode_text_string(parent, s.constData(), s.size());
    }
    case QJsonValue::Double: {
        double d = v.toDouble();
        if (d == floor(d) && fabs(d) <= (Q_INT64_C(1) << std::numeric_limits<double>::digits))
            return cbor_encode_int(parent, qint64(d));
        return cbor_encode_double(parent, d);
    }
    }
    Q_UNREACHABLE();
    return CborUnknownError;
}

void Generator::generatePluginMetaData()
{
    if (cdef->pluginData.iid.isEmpty())
        return;

    fputs("\nQT_PLUGIN_METADATA_SECTION\n"
          "static constexpr unsigned char qt_pluginMetaData[] = {\n"
          "    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', '!',\n"
          "    // metadata version, Qt version, architectural requirements\n"
          "    0, QT_VERSION_MAJOR, QT_VERSION_MINOR, qPluginArchRequirements(),", out);


    CborDevice dev(out);
    CborEncoder enc;
    cbor_encoder_init_writer(&enc, CborDevice::callback, &dev);

    CborEncoder map;
    cbor_encoder_create_map(&enc, &map, CborIndefiniteLength);

    dev.nextItem("\"IID\"");
    cbor_encode_int(&map, int(QtPluginMetaDataKeys::IID));
    cbor_encode_text_string(&map, cdef->pluginData.iid.constData(), cdef->pluginData.iid.size());

    dev.nextItem("\"className\"");
    cbor_encode_int(&map, int(QtPluginMetaDataKeys::ClassName));
    cbor_encode_text_string(&map, cdef->classname.constData(), cdef->classname.size());

    QJsonObject o = cdef->pluginData.metaData.object();
    if (!o.isEmpty()) {
        dev.nextItem("\"MetaData\"");
        cbor_encode_int(&map, int(QtPluginMetaDataKeys::MetaData));
        jsonObjectToCbor(&map, o);
    }

    // Add -M args from the command line:
    for (auto it = cdef->pluginData.metaArgs.cbegin(), end = cdef->pluginData.metaArgs.cend(); it != end; ++it) {
        const QJsonArray &a = it.value();
        QByteArray key = it.key().toUtf8();
        dev.nextItem(QByteArray("command-line \"" + key + "\"").constData());
        cbor_encode_text_string(&map, key.constData(), key.size());
        jsonArrayToCbor(&map, a);
    }

    // Close the CBOR map manually
    dev.nextItem();
    cbor_encoder_close_container(&enc, &map);
    fputs("\n};\n", out);

    // 'Use' all namespaces.
    int pos = cdef->qualified.indexOf("::");
    for ( ; pos != -1 ; pos = cdef->qualified.indexOf("::", pos + 2) )
        fprintf(out, "using namespace %s;\n", cdef->qualified.left(pos).constData());
    fprintf(out, "QT_MOC_EXPORT_PLUGIN(%s, %s)\n\n",
            cdef->qualified.constData(), cdef->classname.constData());
}

QT_WARNING_DISABLE_GCC("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wundefined-internal")
QT_WARNING_DISABLE_MSVC(4334) // '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)

#define CBOR_ENCODER_WRITER_CONTROL     1
#define CBOR_ENCODER_WRITE_FUNCTION     CborDevice::callback

QT_END_NAMESPACE

#include "cborencoder.c"
