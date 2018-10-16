/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qdebug.h>
#include "qplatformdefs.h"
#include "qsettings.h"

#ifndef QT_NO_SETTINGS

#include "qsettings_p.h"
#include "qcache.h"
#include "qfile.h"
#include "qdir.h"
#include "qfileinfo.h"
#include "qmutex.h"
#include "qlibraryinfo.h"
#include "qtemporaryfile.h"
#include "qstandardpaths.h"
#include <qdatastream.h>

#ifndef QT_NO_TEXTCODEC
#  include "qtextcodec.h"
#endif

#ifndef QT_NO_GEOM_VARIANT
#include "qsize.h"
#include "qpoint.h"
#include "qrect.h"
#endif // !QT_NO_GEOM_VARIANT

#ifndef QT_NO_QOBJECT
#include "qcoreapplication.h"
#endif

#ifndef QT_BOOTSTRAPPED
#include "qsavefile.h"
#include "qlockfile.h"
#endif

#ifdef Q_OS_VXWORKS
#  include <ioLib.h>
#endif

#ifdef Q_OS_WASM
#include <emscripten.h>
#endif

#include <algorithm>
#include <stdlib.h>

#ifdef Q_OS_WIN // for homedirpath reading from registry
#  include <qt_windows.h>
#  ifndef Q_OS_WINRT
#    include <shlobj.h>
#  endif
#endif

#ifdef Q_OS_WINRT
#include <wrl.h>
#include <windows.foundation.h>
#include <windows.storage.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_ANDROID)
#define Q_XDG_PLATFORM
#endif

#if !defined(QT_NO_STANDARDPATHS) && (defined(Q_XDG_PLATFORM) || defined(QT_PLATFORM_UIKIT))
#define QSETTINGS_USE_QSTANDARDPATHS
#endif

// ************************************************************************
// QConfFile

/*
    QConfFile objects are explicitly shared within the application.
    This ensures that modification to the settings done through one
    QSettings object are immediately reflected in other setting
    objects of the same application.
*/

QT_BEGIN_NAMESPACE

struct QConfFileCustomFormat
{
    QString extension;
    QSettings::ReadFunc readFunc;
    QSettings::WriteFunc writeFunc;
    Qt::CaseSensitivity caseSensitivity;
};
Q_DECLARE_TYPEINFO(QConfFileCustomFormat, Q_MOVABLE_TYPE);

typedef QHash<QString, QConfFile *> ConfFileHash;
typedef QCache<QString, QConfFile> ConfFileCache;
namespace {
    struct Path
    {
        // Note: Defining constructors explicitly because of buggy C++11
        // implementation in MSVC (uniform initialization).
        Path() {}
        Path(const QString & p, bool ud) : path(p), userDefined(ud) {}
        QString path;
        bool userDefined; //!< true - user defined, overridden by setPath
    };
}
typedef QHash<int, Path> PathHash;
typedef QVector<QConfFileCustomFormat> CustomFormatVector;

Q_GLOBAL_STATIC(ConfFileHash, usedHashFunc)
Q_GLOBAL_STATIC(ConfFileCache, unusedCacheFunc)
Q_GLOBAL_STATIC(PathHash, pathHashFunc)
Q_GLOBAL_STATIC(CustomFormatVector, customFormatVectorFunc)

static QBasicMutex settingsGlobalMutex;

static QSettings::Format globalDefaultFormat = QSettings::NativeFormat;

QConfFile::QConfFile(const QString &fileName, bool _userPerms)
    : name(fileName), size(0), ref(1), userPerms(_userPerms)
{
    usedHashFunc()->insert(name, this);
}

QConfFile::~QConfFile()
{
    if (usedHashFunc())
        usedHashFunc()->remove(name);
}

ParsedSettingsMap QConfFile::mergedKeyMap() const
{
    ParsedSettingsMap result = originalKeys;
    ParsedSettingsMap::const_iterator i;

    for (i = removedKeys.begin(); i != removedKeys.end(); ++i)
        result.remove(i.key());
    for (i = addedKeys.begin(); i != addedKeys.end(); ++i)
        result.insert(i.key(), i.value());
    return result;
}

bool QConfFile::isWritable() const
{
    QFileInfo fileInfo(name);

#ifndef QT_NO_TEMPORARYFILE
    if (fileInfo.exists()) {
#endif
        QFile file(name);
        return file.open(QFile::ReadWrite);
#ifndef QT_NO_TEMPORARYFILE
    } else {
        // Create the directories to the file.
        QDir dir(fileInfo.absolutePath());
        if (!dir.exists()) {
            if (!dir.mkpath(dir.absolutePath()))
                return false;
        }

        // we use a temporary file to avoid race conditions
        QTemporaryFile file(name);
        return file.open();
    }
#endif
}

QConfFile *QConfFile::fromName(const QString &fileName, bool _userPerms)
{
    QString absPath = QFileInfo(fileName).absoluteFilePath();

    ConfFileHash *usedHash = usedHashFunc();
    ConfFileCache *unusedCache = unusedCacheFunc();

    QConfFile *confFile = 0;
    QMutexLocker locker(&settingsGlobalMutex);

    if (!(confFile = usedHash->value(absPath))) {
        if ((confFile = unusedCache->take(absPath)))
            usedHash->insert(absPath, confFile);
    }
    if (confFile) {
        confFile->ref.ref();
        return confFile;
    }
    return new QConfFile(absPath, _userPerms);
}

void QConfFile::clearCache()
{
    QMutexLocker locker(&settingsGlobalMutex);
    unusedCacheFunc()->clear();
}

// ************************************************************************
// QSettingsPrivate

QSettingsPrivate::QSettingsPrivate(QSettings::Format format)
    : format(format), scope(QSettings::UserScope /* nothing better to put */), iniCodec(0), fallbacks(true),
      pendingChanges(false), status(QSettings::NoError)
{
}

QSettingsPrivate::QSettingsPrivate(QSettings::Format format, QSettings::Scope scope,
                                   const QString &organization, const QString &application)
    : format(format), scope(scope), organizationName(organization), applicationName(application),
      iniCodec(0), fallbacks(true), pendingChanges(false), status(QSettings::NoError)
{
}

QSettingsPrivate::~QSettingsPrivate()
{
}

QString QSettingsPrivate::actualKey(const QString &key) const
{
    QString n = normalizedKey(key);
    Q_ASSERT_X(!n.isEmpty(), "QSettings", "empty key");
    return groupPrefix + n;
}

/*
    Returns a string that never starts nor ends with a slash (or an
    empty string). Examples:

            "foo"            becomes   "foo"
            "/foo//bar///"   becomes   "foo/bar"
            "///"            becomes   ""

    This function is optimized to avoid a QString deep copy in the
    common case where the key is already normalized.
*/
QString QSettingsPrivate::normalizedKey(const QString &key)
{
    QString result = key;

    int i = 0;
    while (i < result.size()) {
        while (result.at(i) == QLatin1Char('/')) {
            result.remove(i, 1);
            if (i == result.size())
                goto after_loop;
        }
        while (result.at(i) != QLatin1Char('/')) {
            ++i;
            if (i == result.size())
                return result;
        }
        ++i; // leave the slash alone
    }

after_loop:
    if (!result.isEmpty())
        result.truncate(i - 1); // remove the trailing slash
    return result;
}

// see also qsettings_win.cpp, qsettings_winrt.cpp and qsettings_mac.cpp

#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
QSettingsPrivate *QSettingsPrivate::create(QSettings::Format format, QSettings::Scope scope,
                                           const QString &organization, const QString &application)
{
    return new QConfFileSettingsPrivate(format, scope, organization, application);
}
#endif

#if !defined(Q_OS_WIN)
QSettingsPrivate *QSettingsPrivate::create(const QString &fileName, QSettings::Format format)
{
    return new QConfFileSettingsPrivate(fileName, format);
}
#endif

void QSettingsPrivate::processChild(QStringRef key, ChildSpec spec, QStringList &result)
{
    if (spec != AllKeys) {
        int slashPos = key.indexOf(QLatin1Char('/'));
        if (slashPos == -1) {
            if (spec != ChildKeys)
                return;
        } else {
            if (spec != ChildGroups)
                return;
            key.truncate(slashPos);
        }
    }
    result.append(key.toString());
}

void QSettingsPrivate::beginGroupOrArray(const QSettingsGroup &group)
{
    groupStack.push(group);
    const QString name = group.name();
    if (!name.isEmpty())
        groupPrefix += name + QLatin1Char('/');
}

/*
    We only set an error if there isn't one set already. This way the user always gets the
    first error that occurred. We always allow clearing errors.
*/

void QSettingsPrivate::setStatus(QSettings::Status status) const
{
    if (status == QSettings::NoError || this->status == QSettings::NoError)
        this->status = status;
}

void QSettingsPrivate::update()
{
    flush();
    pendingChanges = false;
}

void QSettingsPrivate::requestUpdate()
{
    if (!pendingChanges) {
        pendingChanges = true;
#ifndef QT_NO_QOBJECT
        Q_Q(QSettings);
        QCoreApplication::postEvent(q, new QEvent(QEvent::UpdateRequest));
#else
        update();
#endif
    }
}

QStringList QSettingsPrivate::variantListToStringList(const QVariantList &l)
{
    QStringList result;
    result.reserve(l.count());
    QVariantList::const_iterator it = l.constBegin();
    for (; it != l.constEnd(); ++it)
        result.append(variantToString(*it));
    return result;
}

QVariant QSettingsPrivate::stringListToVariantList(const QStringList &l)
{
    QStringList outStringList = l;
    for (int i = 0; i < outStringList.count(); ++i) {
        const QString &str = outStringList.at(i);

        if (str.startsWith(QLatin1Char('@'))) {
            if (str.length() >= 2 && str.at(1) == QLatin1Char('@')) {
                outStringList[i].remove(0, 1);
            } else {
                QVariantList variantList;
                const int stringCount = l.count();
                variantList.reserve(stringCount);
                for (int j = 0; j < stringCount; ++j)
                    variantList.append(stringToVariant(l.at(j)));
                return variantList;
            }
        }
    }
    return outStringList;
}

QString QSettingsPrivate::variantToString(const QVariant &v)
{
    QString result;

    switch (v.type()) {
        case QVariant::Invalid:
            result = QLatin1String("@Invalid()");
            break;

        case QVariant::ByteArray: {
            QByteArray a = v.toByteArray();
            result = QLatin1String("@ByteArray(")
                     + QLatin1String(a.constData(), a.size())
                     + QLatin1Char(')');
            break;
        }

        case QVariant::String:
        case QVariant::LongLong:
        case QVariant::ULongLong:
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::Bool:
        case QVariant::Double:
        case QVariant::KeySequence: {
            result = v.toString();
            if (result.contains(QChar::Null))
                result = QLatin1String("@String(") + result + QLatin1Char(')');
            else if (result.startsWith(QLatin1Char('@')))
                result.prepend(QLatin1Char('@'));
            break;
        }
#ifndef QT_NO_GEOM_VARIANT
        case QVariant::Rect: {
            QRect r = qvariant_cast<QRect>(v);
            result = QString::asprintf("@Rect(%d %d %d %d)", r.x(), r.y(), r.width(), r.height());
            break;
        }
        case QVariant::Size: {
            QSize s = qvariant_cast<QSize>(v);
            result = QString::asprintf("@Size(%d %d)", s.width(), s.height());
            break;
        }
        case QVariant::Point: {
            QPoint p = qvariant_cast<QPoint>(v);
            result = QString::asprintf("@Point(%d %d)", p.x(), p.y());
            break;
        }
#endif // !QT_NO_GEOM_VARIANT

        default: {
#ifndef QT_NO_DATASTREAM
            QDataStream::Version version;
            const char *typeSpec;
            if (v.type() == QVariant::DateTime) {
                version = QDataStream::Qt_5_6;
                typeSpec = "@DateTime(";
            } else {
                version = QDataStream::Qt_4_0;
                typeSpec = "@Variant(";
            }
            QByteArray a;
            {
                QDataStream s(&a, QIODevice::WriteOnly);
                s.setVersion(version);
                s << v;
            }

            result = QLatin1String(typeSpec)
                     + QLatin1String(a.constData(), a.size())
                     + QLatin1Char(')');
#else
            Q_ASSERT(!"QSettings: Cannot save custom types without QDataStream support");
#endif
            break;
        }
    }

    return result;
}


QVariant QSettingsPrivate::stringToVariant(const QString &s)
{
    if (s.startsWith(QLatin1Char('@'))) {
        if (s.endsWith(QLatin1Char(')'))) {
            if (s.startsWith(QLatin1String("@ByteArray("))) {
                return QVariant(s.midRef(11, s.size() - 12).toLatin1());
            } else if (s.startsWith(QLatin1String("@String("))) {
                return QVariant(s.midRef(8, s.size() - 9).toString());
            } else if (s.startsWith(QLatin1String("@Variant("))
                       || s.startsWith(QLatin1String("@DateTime("))) {
#ifndef QT_NO_DATASTREAM
                QDataStream::Version version;
                int offset;
                if (s.at(1) == QLatin1Char('D')) {
                    version = QDataStream::Qt_5_6;
                    offset = 10;
                } else {
                    version = QDataStream::Qt_4_0;
                    offset = 9;
                }
                QByteArray a = s.midRef(offset).toLatin1();
                QDataStream stream(&a, QIODevice::ReadOnly);
                stream.setVersion(version);
                QVariant result;
                stream >> result;
                return result;
#else
                Q_ASSERT(!"QSettings: Cannot load custom types without QDataStream support");
#endif
#ifndef QT_NO_GEOM_VARIANT
            } else if (s.startsWith(QLatin1String("@Rect("))) {
                QStringList args = QSettingsPrivate::splitArgs(s, 5);
                if (args.size() == 4)
                    return QVariant(QRect(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt()));
            } else if (s.startsWith(QLatin1String("@Size("))) {
                QStringList args = QSettingsPrivate::splitArgs(s, 5);
                if (args.size() == 2)
                    return QVariant(QSize(args[0].toInt(), args[1].toInt()));
            } else if (s.startsWith(QLatin1String("@Point("))) {
                QStringList args = QSettingsPrivate::splitArgs(s, 6);
                if (args.size() == 2)
                    return QVariant(QPoint(args[0].toInt(), args[1].toInt()));
#endif
            } else if (s == QLatin1String("@Invalid()")) {
                return QVariant();
            }

        }
        if (s.startsWith(QLatin1String("@@")))
            return QVariant(s.mid(1));
    }

    return QVariant(s);
}

static const char hexDigits[] = "0123456789ABCDEF";

void QSettingsPrivate::iniEscapedKey(const QString &key, QByteArray &result)
{
    result.reserve(result.length() + key.length() * 3 / 2);
    for (int i = 0; i < key.size(); ++i) {
        uint ch = key.at(i).unicode();

        if (ch == '/') {
            result += '\\';
        } else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')
                || ch == '_' || ch == '-' || ch == '.') {
            result += (char)ch;
        } else if (ch <= 0xFF) {
            result += '%';
            result += hexDigits[ch / 16];
            result += hexDigits[ch % 16];
        } else {
            result += "%U";
            QByteArray hexCode;
            for (int i = 0; i < 4; ++i) {
                hexCode.prepend(hexDigits[ch % 16]);
                ch >>= 4;
            }
            result += hexCode;
        }
    }
}

bool QSettingsPrivate::iniUnescapedKey(const QByteArray &key, int from, int to, QString &result)
{
    bool lowercaseOnly = true;
    int i = from;
    result.reserve(result.length() + (to - from));
    while (i < to) {
        int ch = (uchar)key.at(i);

        if (ch == '\\') {
            result += QLatin1Char('/');
            ++i;
            continue;
        }

        if (ch != '%' || i == to - 1) {
            if (uint(ch - 'A') <= 'Z' - 'A') // only for ASCII
                lowercaseOnly = false;
            result += QLatin1Char(ch);
            ++i;
            continue;
        }

        int numDigits = 2;
        int firstDigitPos = i + 1;

        ch = key.at(i + 1);
        if (ch == 'U') {
            ++firstDigitPos;
            numDigits = 4;
        }

        if (firstDigitPos + numDigits > to) {
            result += QLatin1Char('%');
            // ### missing U
            ++i;
            continue;
        }

        bool ok;
        ch = key.mid(firstDigitPos, numDigits).toInt(&ok, 16);
        if (!ok) {
            result += QLatin1Char('%');
            // ### missing U
            ++i;
            continue;
        }

        QChar qch(ch);
        if (qch.isUpper())
            lowercaseOnly = false;
        result += qch;
        i = firstDigitPos + numDigits;
    }
    return lowercaseOnly;
}

void QSettingsPrivate::iniEscapedString(const QString &str, QByteArray &result, QTextCodec *codec)
{
    bool needsQuotes = false;
    bool escapeNextIfDigit = false;
    bool useCodec = codec && !str.startsWith(QLatin1String("@ByteArray("))
                    && !str.startsWith(QLatin1String("@Variant("));

    int i;
    int startPos = result.size();

    result.reserve(startPos + str.size() * 3 / 2);
    const QChar *unicode = str.unicode();
    for (i = 0; i < str.size(); ++i) {
        uint ch = unicode[i].unicode();
        if (ch == ';' || ch == ',' || ch == '=')
            needsQuotes = true;

        if (escapeNextIfDigit
                && ((ch >= '0' && ch <= '9')
                    || (ch >= 'a' && ch <= 'f')
                    || (ch >= 'A' && ch <= 'F'))) {
            result += "\\x" + QByteArray::number(ch, 16);
            continue;
        }

        escapeNextIfDigit = false;

        switch (ch) {
        case '\0':
            result += "\\0";
            escapeNextIfDigit = true;
            break;
        case '\a':
            result += "\\a";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        case '\v':
            result += "\\v";
            break;
        case '"':
        case '\\':
            result += '\\';
            result += (char)ch;
            break;
        default:
            if (ch <= 0x1F || (ch >= 0x7F && !useCodec)) {
                result += "\\x" + QByteArray::number(ch, 16);
                escapeNextIfDigit = true;
#ifndef QT_NO_TEXTCODEC
            } else if (useCodec) {
                // slow
                result += codec->fromUnicode(&unicode[i], 1);
#endif
            } else {
                result += (char)ch;
            }
        }
    }

    if (needsQuotes
            || (startPos < result.size() && (result.at(startPos) == ' '
                                                || result.at(result.size() - 1) == ' '))) {
        result.insert(startPos, '"');
        result += '"';
    }
}

inline static void iniChopTrailingSpaces(QString &str, int limit)
{
    int n = str.size() - 1;
    QChar ch;
    while (n >= limit && ((ch = str.at(n)) == QLatin1Char(' ') || ch == QLatin1Char('\t')))
        str.truncate(n--);
}

void QSettingsPrivate::iniEscapedStringList(const QStringList &strs, QByteArray &result, QTextCodec *codec)
{
    if (strs.isEmpty()) {
        /*
            We need to distinguish between empty lists and one-item
            lists that contain an empty string. Ideally, we'd have a
            @EmptyList() symbol but that would break compatibility
            with Qt 4.0. @Invalid() stands for QVariant(), and
            QVariant().toStringList() returns an empty QStringList,
            so we're in good shape.
        */
        result += "@Invalid()";
    } else {
        for (int i = 0; i < strs.size(); ++i) {
            if (i != 0)
                result += ", ";
            iniEscapedString(strs.at(i), result, codec);
        }
    }
}

bool QSettingsPrivate::iniUnescapedStringList(const QByteArray &str, int from, int to,
                                              QString &stringResult, QStringList &stringListResult,
                                              QTextCodec *codec)
{
    static const char escapeCodes[][2] =
    {
        { 'a', '\a' },
        { 'b', '\b' },
        { 'f', '\f' },
        { 'n', '\n' },
        { 'r', '\r' },
        { 't', '\t' },
        { 'v', '\v' },
        { '"', '"' },
        { '?', '?' },
        { '\'', '\'' },
        { '\\', '\\' }
    };
    static const int numEscapeCodes = sizeof(escapeCodes) / sizeof(escapeCodes[0]);

    bool isStringList = false;
    bool inQuotedString = false;
    bool currentValueIsQuoted = false;
    int escapeVal = 0;
    int i = from;
    char ch;

StSkipSpaces:
    while (i < to && ((ch = str.at(i)) == ' ' || ch == '\t'))
        ++i;
    // fallthrough

StNormal:
    int chopLimit = stringResult.length();
    while (i < to) {
        switch (str.at(i)) {
        case '\\':
            ++i;
            if (i >= to)
                goto end;

            ch = str.at(i++);
            for (int j = 0; j < numEscapeCodes; ++j) {
                if (ch == escapeCodes[j][0]) {
                    stringResult += QLatin1Char(escapeCodes[j][1]);
                    goto StNormal;
                }
            }

            if (ch == 'x') {
                escapeVal = 0;

                if (i >= to)
                    goto end;

                ch = str.at(i);
                if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
                    goto StHexEscape;
            } else if (ch >= '0' && ch <= '7') {
                escapeVal = ch - '0';
                goto StOctEscape;
            } else if (ch == '\n' || ch == '\r') {
                if (i < to) {
                    char ch2 = str.at(i);
                    // \n, \r, \r\n, and \n\r are legitimate line terminators in INI files
                    if ((ch2 == '\n' || ch2 == '\r') && ch2 != ch)
                        ++i;
                }
            } else {
                // the character is skipped
            }
            chopLimit = stringResult.length();
            break;
        case '"':
            ++i;
            currentValueIsQuoted = true;
            inQuotedString = !inQuotedString;
            if (!inQuotedString)
                goto StSkipSpaces;
            break;
        case ',':
            if (!inQuotedString) {
                if (!currentValueIsQuoted)
                    iniChopTrailingSpaces(stringResult, chopLimit);
                if (!isStringList) {
                    isStringList = true;
                    stringListResult.clear();
                    stringResult.squeeze();
                }
                stringListResult.append(stringResult);
                stringResult.clear();
                currentValueIsQuoted = false;
                ++i;
                goto StSkipSpaces;
            }
            Q_FALLTHROUGH();
        default: {
            int j = i + 1;
            while (j < to) {
                ch = str.at(j);
                if (ch == '\\' || ch == '"' || ch == ',')
                    break;
                ++j;
            }

#ifdef QT_NO_TEXTCODEC
            Q_UNUSED(codec)
#else
            if (codec) {
                stringResult += codec->toUnicode(str.constData() + i, j - i);
            } else
#endif
            {
                int n = stringResult.size();
                stringResult.resize(n + (j - i));
                QChar *resultData = stringResult.data() + n;
                for (int k = i; k < j; ++k)
                    *resultData++ = QLatin1Char(str.at(k));
            }
            i = j;
        }
        }
    }
    if (!currentValueIsQuoted)
        iniChopTrailingSpaces(stringResult, chopLimit);
    goto end;

StHexEscape:
    if (i >= to) {
        stringResult += QChar(escapeVal);
        goto end;
    }

    ch = str.at(i);
    if (ch >= 'a')
        ch -= 'a' - 'A';
    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')) {
        escapeVal <<= 4;
        escapeVal += strchr(hexDigits, ch) - hexDigits;
        ++i;
        goto StHexEscape;
    } else {
        stringResult += QChar(escapeVal);
        goto StNormal;
    }

StOctEscape:
    if (i >= to) {
        stringResult += QChar(escapeVal);
        goto end;
    }

    ch = str.at(i);
    if (ch >= '0' && ch <= '7') {
        escapeVal <<= 3;
        escapeVal += ch - '0';
        ++i;
        goto StOctEscape;
    } else {
        stringResult += QChar(escapeVal);
        goto StNormal;
    }

end:
    if (isStringList)
        stringListResult.append(stringResult);
    return isStringList;
}

QStringList QSettingsPrivate::splitArgs(const QString &s, int idx)
{
    int l = s.length();
    Q_ASSERT(l > 0);
    Q_ASSERT(s.at(idx) == QLatin1Char('('));
    Q_ASSERT(s.at(l - 1) == QLatin1Char(')'));

    QStringList result;
    QString item;

    for (++idx; idx < l; ++idx) {
        QChar c = s.at(idx);
        if (c == QLatin1Char(')')) {
            Q_ASSERT(idx == l - 1);
            result.append(item);
        } else if (c == QLatin1Char(' ')) {
            result.append(item);
            item.clear();
        } else {
            item.append(c);
        }
    }

    return result;
}

// ************************************************************************
// QConfFileSettingsPrivate

void QConfFileSettingsPrivate::initFormat()
{
    extension = (format == QSettings::NativeFormat) ? QLatin1String(".conf") : QLatin1String(".ini");
    readFunc = 0;
    writeFunc = 0;
#if defined(Q_OS_MAC)
    caseSensitivity = (format == QSettings::NativeFormat) ? Qt::CaseSensitive : IniCaseSensitivity;
#else
    caseSensitivity = IniCaseSensitivity;
#endif

    if (format > QSettings::IniFormat) {
        QMutexLocker locker(&settingsGlobalMutex);
        const CustomFormatVector *customFormatVector = customFormatVectorFunc();

        int i = (int)format - (int)QSettings::CustomFormat1;
        if (i >= 0 && i < customFormatVector->size()) {
            QConfFileCustomFormat info = customFormatVector->at(i);
            extension = info.extension;
            readFunc = info.readFunc;
            writeFunc = info.writeFunc;
            caseSensitivity = info.caseSensitivity;
        }
    }
}

void QConfFileSettingsPrivate::initAccess()
{
    if (!confFiles.isEmpty()) {
        if (format > QSettings::IniFormat) {
            if (!readFunc)
                setStatus(QSettings::AccessError);
        }
    }

    sync();       // loads the files the first time
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
static QString windowsConfigPath(const KNOWNFOLDERID &type)
{
    QString result;

    PWSTR path = nullptr;
    if (SHGetKnownFolderPath(type, KF_FLAG_DONT_VERIFY, NULL, &path) == S_OK) {
        result = QString::fromWCharArray(path);
        CoTaskMemFree(path);
    }

    if (result.isEmpty()) {
        if (type == FOLDERID_ProgramData) {
            result = QLatin1String("C:\\temp\\qt-common");
        } else if (type == FOLDERID_RoamingAppData) {
            result = QLatin1String("C:\\temp\\qt-user");
        }
    }

    return result;
}
#elif defined(Q_OS_WINRT) // Q_OS_WIN && !Q_OS_WINRT

enum ConfigPathType {
    ConfigPath_CommonAppData,
    ConfigPath_UserAppData
};

static QString windowsConfigPath(ConfigPathType type)
{
    static QString result;
    while (result.isEmpty()) {
        ComPtr<IApplicationDataStatics> applicationDataStatics;
        if (FAILED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), &applicationDataStatics)))
            return result;
        ComPtr<IApplicationData> applicationData;
        if (FAILED(applicationDataStatics->get_Current(&applicationData)))
            return result;
        ComPtr<IStorageFolder> localFolder;
        if (FAILED(applicationData->get_LocalFolder(&localFolder)))
            return result;
        ComPtr<IStorageItem> localFolderItem;
        if (FAILED(localFolder.As(&localFolderItem)))
            return result;
        HString path;
        if (FAILED(localFolderItem->get_Path(path.GetAddressOf())))
            return result;
        result = QString::fromWCharArray(path.GetRawBuffer(nullptr));
    }

    switch (type) {
    case ConfigPath_CommonAppData:
        return result + QLatin1String("\\qt-common");
    case ConfigPath_UserAppData:
        return result + QLatin1String("\\qt-user");
    }
    return result;
}
#endif // Q_OS_WINRT

static inline int pathHashKey(QSettings::Format format, QSettings::Scope scope)
{
    return int((uint(format) << 1) | uint(scope == QSettings::SystemScope));
}

#ifndef Q_OS_WIN
static QString make_user_path()
{
    static Q_CONSTEXPR QChar sep = QLatin1Char('/');
#ifndef QSETTINGS_USE_QSTANDARDPATHS
    // Non XDG platforms (OS X, iOS, Android...) have used this code path erroneously
    // for some time now. Moving away from that would require migrating existing settings.
    QByteArray env = qgetenv("XDG_CONFIG_HOME");
    if (env.isEmpty()) {
        return QDir::homePath() + QLatin1String("/.config/");
    } else if (env.startsWith('/')) {
        return QFile::decodeName(env) + sep;
    } else {
        return QDir::homePath() + sep + QFile::decodeName(env) + sep;
    }
#else
    // When using a proper XDG platform, use QStandardPaths rather than the above hand-written code;
    // it makes the use of test mode from unit tests possible.
    // Ideally all platforms should use this, but see above for the migration issue.
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + sep;
#endif
}
#endif // !Q_OS_WIN

static void initDefaultPaths(QMutexLocker *locker)
{
    PathHash *pathHash = pathHashFunc();

    locker->unlock();

    /*
       QLibraryInfo::location() uses QSettings, so in order to
       avoid a dead-lock, we can't hold the global mutex while
       calling it.
    */
    QString systemPath = QLibraryInfo::location(QLibraryInfo::SettingsPath) + QLatin1Char('/');

    locker->relock();
    if (pathHash->isEmpty()) {
        /*
           Lazy initialization of pathHash. We initialize the
           IniFormat paths and (on Unix) the NativeFormat paths.
           (The NativeFormat paths are not configurable for the
           Windows registry and the Mac CFPreferences.)
       */
#ifdef Q_OS_WIN

#  ifdef Q_OS_WINRT
        const QString roamingAppDataFolder = windowsConfigPath(ConfigPath_UserAppData);
        const QString programDataFolder = windowsConfigPath(ConfigPath_CommonAppData);
#  else
        const QString roamingAppDataFolder = windowsConfigPath(FOLDERID_RoamingAppData);
        const QString programDataFolder = windowsConfigPath(FOLDERID_ProgramData);
#  endif
        pathHash->insert(pathHashKey(QSettings::IniFormat, QSettings::UserScope),
                         Path(roamingAppDataFolder + QDir::separator(), false));
        pathHash->insert(pathHashKey(QSettings::IniFormat, QSettings::SystemScope),
                         Path(programDataFolder + QDir::separator(), false));
#else
        const QString userPath = make_user_path();
        pathHash->insert(pathHashKey(QSettings::IniFormat, QSettings::UserScope), Path(userPath, false));
        pathHash->insert(pathHashKey(QSettings::IniFormat, QSettings::SystemScope), Path(systemPath, false));
#ifndef Q_OS_MAC
        pathHash->insert(pathHashKey(QSettings::NativeFormat, QSettings::UserScope), Path(userPath, false));
        pathHash->insert(pathHashKey(QSettings::NativeFormat, QSettings::SystemScope), Path(systemPath, false));
#endif
#endif // Q_OS_WIN
    }
}

static Path getPath(QSettings::Format format, QSettings::Scope scope)
{
    Q_ASSERT((int)QSettings::NativeFormat == 0);
    Q_ASSERT((int)QSettings::IniFormat == 1);

    QMutexLocker locker(&settingsGlobalMutex);
    PathHash *pathHash = pathHashFunc();
    if (pathHash->isEmpty())
        initDefaultPaths(&locker);

    Path result = pathHash->value(pathHashKey(format, scope));
    if (!result.path.isEmpty())
        return result;

    // fall back on INI path
    return pathHash->value(pathHashKey(QSettings::IniFormat, scope));
}

#if defined(QT_BUILD_INTERNAL) && defined(Q_XDG_PLATFORM) && !defined(QT_NO_STANDARDPATHS)
// Note: Suitable only for autotests.
void Q_AUTOTEST_EXPORT clearDefaultPaths()
{
    QMutexLocker locker(&settingsGlobalMutex);
    pathHashFunc()->clear();
}
#endif // QT_BUILD_INTERNAL && Q_XDG_PLATFORM && !QT_NO_STANDARDPATHS

QConfFileSettingsPrivate::QConfFileSettingsPrivate(QSettings::Format format,
                                                   QSettings::Scope scope,
                                                   const QString &organization,
                                                   const QString &application)
    : QSettingsPrivate(format, scope, organization, application),
      nextPosition(0x40000000) // big positive number
{
    initFormat();

    QString org = organization;
    if (org.isEmpty()) {
        setStatus(QSettings::AccessError);
        org = QLatin1String("Unknown Organization");
    }

    QString appFile = org + QDir::separator() + application + extension;
    QString orgFile = org + extension;

    if (scope == QSettings::UserScope) {
        Path userPath = getPath(format, QSettings::UserScope);
        if (!application.isEmpty())
            confFiles.append(QConfFile::fromName(userPath.path + appFile, true));
        confFiles.append(QConfFile::fromName(userPath.path + orgFile, true));
    }

    Path systemPath = getPath(format, QSettings::SystemScope);
#if defined(Q_XDG_PLATFORM) && !defined(QT_NO_STANDARDPATHS)
    // check if the systemPath wasn't overridden by QSettings::setPath()
    if (!systemPath.userDefined) {
        // Note: We can't use QStandardPaths::locateAll() as we need all the
        // possible files (not just the existing ones) and there is no way
        // to exclude user specific (XDG_CONFIG_HOME) directory from the search.
        QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation);
        // remove the QStandardLocation::writableLocation() (XDG_CONFIG_HOME)
        if (!dirs.isEmpty())
            dirs.takeFirst();
        QStringList paths;
        if (!application.isEmpty()) {
            paths.reserve(dirs.size() * 2);
            for (const auto &dir : qAsConst(dirs))
                paths.append(dir + QLatin1Char('/') + appFile);
        } else {
            paths.reserve(dirs.size());
        }
        for (const auto &dir : qAsConst(dirs))
            paths.append(dir + QLatin1Char('/') + orgFile);

        // Note: No check for existence of files is done intentionaly.
        for (const auto &path : qAsConst(paths))
            confFiles.append(QConfFile::fromName(path, false));
    } else
#endif // Q_XDG_PLATFORM && !QT_NO_STANDARDPATHS
    {
        if (!application.isEmpty())
            confFiles.append(QConfFile::fromName(systemPath.path + appFile, false));
        confFiles.append(QConfFile::fromName(systemPath.path + orgFile, false));
    }

    initAccess();
}

QConfFileSettingsPrivate::QConfFileSettingsPrivate(const QString &fileName,
                                                   QSettings::Format format)
    : QSettingsPrivate(format),
      nextPosition(0x40000000) // big positive number
{
    initFormat();

    confFiles.append(QConfFile::fromName(fileName, true));

    initAccess();
}

QConfFileSettingsPrivate::~QConfFileSettingsPrivate()
{
    QMutexLocker locker(&settingsGlobalMutex);
    ConfFileHash *usedHash = usedHashFunc();
    ConfFileCache *unusedCache = unusedCacheFunc();

    for (auto conf_file : qAsConst(confFiles)) {
        if (!conf_file->ref.deref()) {
            if (conf_file->size == 0) {
                delete conf_file;
            } else {
                if (usedHash)
                    usedHash->remove(conf_file->name);
                if (unusedCache) {
                    QT_TRY {
                        // compute a better size?
                        unusedCache->insert(conf_file->name, conf_file,
                                            10 + (conf_file->originalKeys.size() / 4));
                    } QT_CATCH(...) {
                        // out of memory. Do not cache the file.
                        delete conf_file;
                    }
                } else {
                    // unusedCache is gone - delete the entry to prevent a memory leak
                    delete conf_file;
                }
            }
        }
    }
}

void QConfFileSettingsPrivate::remove(const QString &key)
{
    if (confFiles.isEmpty())
        return;

    // Note: First config file is always the most specific.
    QConfFile *confFile = confFiles.at(0);

    QSettingsKey theKey(key, caseSensitivity);
    QSettingsKey prefix(key + QLatin1Char('/'), caseSensitivity);
    QMutexLocker locker(&confFile->mutex);

    ensureSectionParsed(confFile, theKey);
    ensureSectionParsed(confFile, prefix);

    ParsedSettingsMap::iterator i = confFile->addedKeys.lowerBound(prefix);
    while (i != confFile->addedKeys.end() && i.key().startsWith(prefix))
        i = confFile->addedKeys.erase(i);
    confFile->addedKeys.remove(theKey);

    ParsedSettingsMap::const_iterator j = const_cast<const ParsedSettingsMap *>(&confFile->originalKeys)->lowerBound(prefix);
    while (j != confFile->originalKeys.constEnd() && j.key().startsWith(prefix)) {
        confFile->removedKeys.insert(j.key(), QVariant());
        ++j;
    }
    if (confFile->originalKeys.contains(theKey))
        confFile->removedKeys.insert(theKey, QVariant());
}

void QConfFileSettingsPrivate::set(const QString &key, const QVariant &value)
{
    if (confFiles.isEmpty())
        return;

    // Note: First config file is always the most specific.
    QConfFile *confFile = confFiles.at(0);

    QSettingsKey theKey(key, caseSensitivity, nextPosition++);
    QMutexLocker locker(&confFile->mutex);
    confFile->removedKeys.remove(theKey);
    confFile->addedKeys.insert(theKey, value);
}

bool QConfFileSettingsPrivate::get(const QString &key, QVariant *value) const
{
    QSettingsKey theKey(key, caseSensitivity);
    ParsedSettingsMap::const_iterator j;
    bool found = false;

    for (auto confFile : qAsConst(confFiles)) {
        QMutexLocker locker(&confFile->mutex);

        if (!confFile->addedKeys.isEmpty()) {
            j = confFile->addedKeys.constFind(theKey);
            found = (j != confFile->addedKeys.constEnd());
        }
        if (!found) {
            ensureSectionParsed(confFile, theKey);
            j = confFile->originalKeys.constFind(theKey);
            found = (j != confFile->originalKeys.constEnd()
                     && !confFile->removedKeys.contains(theKey));
        }

        if (found && value)
            *value = *j;

        if (found)
            return true;
        if (!fallbacks)
            break;
    }
    return false;
}

QStringList QConfFileSettingsPrivate::children(const QString &prefix, ChildSpec spec) const
{
    QStringList result;
    ParsedSettingsMap::const_iterator j;

    QSettingsKey thePrefix(prefix, caseSensitivity);
    int startPos = prefix.size();

    for (auto confFile : qAsConst(confFiles)) {
        QMutexLocker locker(&confFile->mutex);

        if (thePrefix.isEmpty())
            ensureAllSectionsParsed(confFile);
        else
            ensureSectionParsed(confFile, thePrefix);

        j = const_cast<const ParsedSettingsMap *>(
                &confFile->originalKeys)->lowerBound( thePrefix);
        while (j != confFile->originalKeys.constEnd() && j.key().startsWith(thePrefix)) {
            if (!confFile->removedKeys.contains(j.key()))
                processChild(j.key().originalCaseKey().midRef(startPos), spec, result);
            ++j;
        }

        j = const_cast<const ParsedSettingsMap *>(
                &confFile->addedKeys)->lowerBound(thePrefix);
        while (j != confFile->addedKeys.constEnd() && j.key().startsWith(thePrefix)) {
            processChild(j.key().originalCaseKey().midRef(startPos), spec, result);
            ++j;
        }

        if (!fallbacks)
            break;
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()),
                 result.end());
    return result;
}

void QConfFileSettingsPrivate::clear()
{
    if (confFiles.isEmpty())
        return;

    // Note: First config file is always the most specific.
    QConfFile *confFile = confFiles.at(0);

    QMutexLocker locker(&confFile->mutex);
    ensureAllSectionsParsed(confFile);
    confFile->addedKeys.clear();
    confFile->removedKeys = confFile->originalKeys;
}

void QConfFileSettingsPrivate::sync()
{
    // people probably won't be checking the status a whole lot, so in case of
    // error we just try to go on and make the best of it

    for (auto confFile : qAsConst(confFiles)) {
        QMutexLocker locker(&confFile->mutex);
        syncConfFile(confFile);
    }
}

void QConfFileSettingsPrivate::flush()
{
    sync();
}

QString QConfFileSettingsPrivate::fileName() const
{
    if (confFiles.isEmpty())
        return QString();

    // Note: First config file is always the most specific.
    return confFiles.at(0)->name;
}

bool QConfFileSettingsPrivate::isWritable() const
{
    if (format > QSettings::IniFormat && !writeFunc)
        return false;

    if (confFiles.isEmpty())
        return false;

    return confFiles.at(0)->isWritable();
}

void QConfFileSettingsPrivate::syncConfFile(QConfFile *confFile)
{
    bool readOnly = confFile->addedKeys.isEmpty() && confFile->removedKeys.isEmpty();

    /*
        We can often optimize the read-only case, if the file on disk
        hasn't changed.
    */
    if (readOnly && confFile->size > 0) {
        QFileInfo fileInfo(confFile->name);
        if (confFile->size == fileInfo.size() && confFile->timeStamp == fileInfo.lastModified())
            return;
    }

    if (!readOnly && !confFile->isWritable()) {
        setStatus(QSettings::AccessError);
        return;
    }

#ifndef QT_BOOTSTRAPPED
    /*
        Use a lockfile in order to protect us against other QSettings instances
        trying to write the same settings at the same time.

        We only need to lock if we are actually writing as only concurrent writes are a problem.
        Concurrent read and write are not a problem because the writing operation is atomic.
    */
    QLockFile lockFile(confFile->name + QLatin1String(".lock"));
    if (!readOnly && !lockFile.lock() && atomicSyncOnly) {
        setStatus(QSettings::AccessError);
        return;
    }
#endif

    /*
        We hold the lock. Let's reread the file if it has changed
        since last time we read it.
    */
    QFileInfo fileInfo(confFile->name);
    bool mustReadFile = true;
    bool createFile = !fileInfo.exists();

    if (!readOnly)
        mustReadFile = (confFile->size != fileInfo.size()
                        || (confFile->size != 0 && confFile->timeStamp != fileInfo.lastModified()));

    if (mustReadFile) {
        confFile->unparsedIniSections.clear();
        confFile->originalKeys.clear();

        QFile file(confFile->name);
        if (!createFile && !file.open(QFile::ReadOnly)) {
            setStatus(QSettings::AccessError);
            return;
        }

        /*
            Files that we can't read (because of permissions or
            because they don't exist) are treated as empty files.
        */
        if (file.isReadable() && file.size() != 0) {
            bool ok = false;
#ifdef Q_OS_MAC
            if (format == QSettings::NativeFormat) {
                QByteArray data = file.readAll();
                ok = readPlistFile(data, &confFile->originalKeys);
            } else
#endif
            if (format <= QSettings::IniFormat) {
                QByteArray data = file.readAll();
                ok = readIniFile(data, &confFile->unparsedIniSections);
            } else if (readFunc) {
                QSettings::SettingsMap tempNewKeys;
                ok = readFunc(file, tempNewKeys);

                if (ok) {
                    QSettings::SettingsMap::const_iterator i = tempNewKeys.constBegin();
                    while (i != tempNewKeys.constEnd()) {
                        confFile->originalKeys.insert(QSettingsKey(i.key(), caseSensitivity),
                                                      i.value());
                        ++i;
                    }
                }
            }

            if (!ok)
                setStatus(QSettings::FormatError);
        }

        confFile->size = fileInfo.size();
        confFile->timeStamp = fileInfo.lastModified();
    }

    /*
        We also need to save the file. We still hold the file lock,
        so everything is under control.
    */
    if (!readOnly) {
        bool ok = false;
        ensureAllSectionsParsed(confFile);
        ParsedSettingsMap mergedKeys = confFile->mergedKeyMap();

#if !defined(QT_BOOTSTRAPPED) && QT_CONFIG(temporaryfile)
        QSaveFile sf(confFile->name);
        sf.setDirectWriteFallback(!atomicSyncOnly);
#else
        QFile sf(confFile->name);
#endif
        if (!sf.open(QIODevice::WriteOnly)) {
            setStatus(QSettings::AccessError);
            return;
        }

#ifdef Q_OS_MAC
        if (format == QSettings::NativeFormat) {
            ok = writePlistFile(sf, mergedKeys);
        } else
#endif
        if (format <= QSettings::IniFormat) {
            ok = writeIniFile(sf, mergedKeys);
        } else if (writeFunc) {
            QSettings::SettingsMap tempOriginalKeys;

            ParsedSettingsMap::const_iterator i = mergedKeys.constBegin();
            while (i != mergedKeys.constEnd()) {
                tempOriginalKeys.insert(i.key(), i.value());
                ++i;
            }
            ok = writeFunc(sf, tempOriginalKeys);
        }

#if !defined(QT_BOOTSTRAPPED) && QT_CONFIG(temporaryfile)
        if (ok)
            ok = sf.commit();
#endif

        if (ok) {
            confFile->unparsedIniSections.clear();
            confFile->originalKeys = mergedKeys;
            confFile->addedKeys.clear();
            confFile->removedKeys.clear();

            QFileInfo fileInfo(confFile->name);
            confFile->size = fileInfo.size();
            confFile->timeStamp = fileInfo.lastModified();

            // If we have created the file, apply the file perms
            if (createFile) {
                QFile::Permissions perms = fileInfo.permissions() | QFile::ReadOwner | QFile::WriteOwner;
                if (!confFile->userPerms)
                    perms |= QFile::ReadGroup | QFile::ReadOther;
                QFile(confFile->name).setPermissions(perms);
            }
#ifdef Q_OS_WASM
        EM_ASM(
            // Sync sandbox filesystem to persistent database filesystem. See QTBUG-70002
            FS.syncfs(false, function(err) {
            });
        );
#endif
        } else {
            setStatus(QSettings::AccessError);
        }
    }
}

enum { Space = 0x1, Special = 0x2 };

static const char charTraits[256] =
{
    // Space: '\t', '\n', '\r', ' '
    // Special: '\n', '\r', '"', ';', '=', '\\'

    0, 0, 0, 0, 0, 0, 0, 0, 0, Space, Space | Special, 0, 0, Space | Special, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    Space, 0, Special, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Special, 0, Special, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Special, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

bool QConfFileSettingsPrivate::readIniLine(const QByteArray &data, int &dataPos,
                                           int &lineStart, int &lineLen, int &equalsPos)
{
    int dataLen = data.length();
    bool inQuotes = false;

    equalsPos = -1;

    lineStart = dataPos;
    while (lineStart < dataLen && (charTraits[uint(uchar(data.at(lineStart)))] & Space))
        ++lineStart;

    int i = lineStart;
    while (i < dataLen) {
        while (!(charTraits[uint(uchar(data.at(i)))] & Special)) {
            if (++i == dataLen)
                goto break_out_of_outer_loop;
        }

        char ch = data.at(i++);
        if (ch == '=') {
            if (!inQuotes && equalsPos == -1)
                equalsPos = i - 1;
        } else if (ch == '\n' || ch == '\r') {
            if (i == lineStart + 1) {
                ++lineStart;
            } else if (!inQuotes) {
                --i;
                goto break_out_of_outer_loop;
            }
        } else if (ch == '\\') {
            if (i < dataLen) {
                char ch = data.at(i++);
                if (i < dataLen) {
                    char ch2 = data.at(i);
                    // \n, \r, \r\n, and \n\r are legitimate line terminators in INI files
                    if ((ch == '\n' && ch2 == '\r') || (ch == '\r' && ch2 == '\n'))
                        ++i;
                }
            }
        } else if (ch == '"') {
            inQuotes = !inQuotes;
        } else {
            Q_ASSERT(ch == ';');

            if (i == lineStart + 1) {
                char ch;
                while (i < dataLen && (((ch = data.at(i)) != '\n') && ch != '\r'))
                    ++i;
                lineStart = i;
            } else if (!inQuotes) {
                --i;
                goto break_out_of_outer_loop;
            }
        }
    }

break_out_of_outer_loop:
    dataPos = i;
    lineLen = i - lineStart;
    return lineLen > 0;
}

/*
    Returns \c false on parse error. However, as many keys are read as
    possible, so if the user doesn't check the status he will get the
    most out of the file anyway.
*/
bool QConfFileSettingsPrivate::readIniFile(const QByteArray &data,
                                           UnparsedSettingsMap *unparsedIniSections)
{
#define FLUSH_CURRENT_SECTION() \
    { \
        QByteArray &sectionData = (*unparsedIniSections)[QSettingsKey(currentSection, \
                                                                      IniCaseSensitivity, \
                                                                      sectionPosition)]; \
        if (!sectionData.isEmpty()) \
            sectionData.append('\n'); \
        sectionData += data.mid(currentSectionStart, lineStart - currentSectionStart); \
        sectionPosition = ++position; \
    }

    QString currentSection;
    int currentSectionStart = 0;
    int dataPos = 0;
    int lineStart;
    int lineLen;
    int equalsPos;
    int position = 0;
    int sectionPosition = 0;
    bool ok = true;

#ifndef QT_NO_TEXTCODEC
    // detect utf8 BOM
    const uchar *dd = (const uchar *)data.constData();
    if (data.size() >= 3 && dd[0] == 0xef && dd[1] == 0xbb && dd[2] == 0xbf) {
        iniCodec = QTextCodec::codecForName("UTF-8");
        dataPos = 3;
    }
#endif

    while (readIniLine(data, dataPos, lineStart, lineLen, equalsPos)) {
        char ch = data.at(lineStart);
        if (ch == '[') {
            FLUSH_CURRENT_SECTION();

            // this is a section
            QByteArray iniSection;
            int idx = data.indexOf(']', lineStart);
            if (idx == -1 || idx >= lineStart + lineLen) {
                ok = false;
                iniSection = data.mid(lineStart + 1, lineLen - 1);
            } else {
                iniSection = data.mid(lineStart + 1, idx - lineStart - 1);
            }

            iniSection = iniSection.trimmed();

            if (iniSection.compare("general", Qt::CaseInsensitive) == 0) {
                currentSection.clear();
            } else {
                if (iniSection.compare("%general", Qt::CaseInsensitive) == 0) {
                    currentSection = QLatin1String(iniSection.constData() + 1);
                } else {
                    currentSection.clear();
                    iniUnescapedKey(iniSection, 0, iniSection.size(), currentSection);
                }
                currentSection += QLatin1Char('/');
            }
            currentSectionStart = dataPos;
        }
        ++position;
    }

    Q_ASSERT(lineStart == data.length());
    FLUSH_CURRENT_SECTION();

    return ok;

#undef FLUSH_CURRENT_SECTION
}

bool QConfFileSettingsPrivate::readIniSection(const QSettingsKey &section, const QByteArray &data,
                                              ParsedSettingsMap *settingsMap, QTextCodec *codec)
{
    QStringList strListValue;
    bool sectionIsLowercase = (section == section.originalCaseKey());
    int equalsPos;

    bool ok = true;
    int dataPos = 0;
    int lineStart;
    int lineLen;
    int position = section.originalKeyPosition();

    while (readIniLine(data, dataPos, lineStart, lineLen, equalsPos)) {
        char ch = data.at(lineStart);
        Q_ASSERT(ch != '[');

        if (equalsPos == -1) {
            if (ch != ';')
                ok = false;
            continue;
        }

        int keyEnd = equalsPos;
        while (keyEnd > lineStart && ((ch = data.at(keyEnd - 1)) == ' ' || ch == '\t'))
            --keyEnd;
        int valueStart = equalsPos + 1;

        QString key = section.originalCaseKey();
        bool keyIsLowercase = (iniUnescapedKey(data, lineStart, keyEnd, key) && sectionIsLowercase);

        QString strValue;
        strValue.reserve(lineLen - (valueStart - lineStart));
        bool isStringList = iniUnescapedStringList(data, valueStart, lineStart + lineLen,
                                                   strValue, strListValue, codec);
        QVariant variant;
        if (isStringList) {
            variant = stringListToVariantList(strListValue);
        } else {
            variant = stringToVariant(strValue);
        }

        /*
            We try to avoid the expensive toLower() call in
            QSettingsKey by passing Qt::CaseSensitive when the
            key is already in lowercase.
        */
        settingsMap->insert(QSettingsKey(key, keyIsLowercase ? Qt::CaseSensitive
                                                             : IniCaseSensitivity,
                                         position),
                            variant);
        ++position;
    }

    return ok;
}

class QSettingsIniKey : public QString
{
public:
    inline QSettingsIniKey() : position(-1) {}
    inline QSettingsIniKey(const QString &str, int pos = -1) : QString(str), position(pos) {}

    int position;
};
Q_DECLARE_TYPEINFO(QSettingsIniKey, Q_MOVABLE_TYPE);

static bool operator<(const QSettingsIniKey &k1, const QSettingsIniKey &k2)
{
    if (k1.position != k2.position)
        return k1.position < k2.position;
    return static_cast<const QString &>(k1) < static_cast<const QString &>(k2);
}

typedef QMap<QSettingsIniKey, QVariant> IniKeyMap;

struct QSettingsIniSection
{
    int position;
    IniKeyMap keyMap;

    inline QSettingsIniSection() : position(-1) {}
};

Q_DECLARE_TYPEINFO(QSettingsIniSection, Q_MOVABLE_TYPE);

typedef QMap<QString, QSettingsIniSection> IniMap;

/*
    This would be more straightforward if we didn't try to remember the original
    key order in the .ini file, but we do.
*/
bool QConfFileSettingsPrivate::writeIniFile(QIODevice &device, const ParsedSettingsMap &map)
{
    IniMap iniMap;
    IniMap::const_iterator i;

#ifdef Q_OS_WIN
    const char * const eol = "\r\n";
#else
    const char eol = '\n';
#endif

    for (ParsedSettingsMap::const_iterator j = map.constBegin(); j != map.constEnd(); ++j) {
        QString section;
        QSettingsIniKey key(j.key().originalCaseKey(), j.key().originalKeyPosition());
        int slashPos;

        if ((slashPos = key.indexOf(QLatin1Char('/'))) != -1) {
            section = key.left(slashPos);
            key.remove(0, slashPos + 1);
        }

        QSettingsIniSection &iniSection = iniMap[section];

        // -1 means infinity
        if (uint(key.position) < uint(iniSection.position))
            iniSection.position = key.position;
        iniSection.keyMap[key] = j.value();
    }

    const int sectionCount = iniMap.size();
    QVector<QSettingsIniKey> sections;
    sections.reserve(sectionCount);
    for (i = iniMap.constBegin(); i != iniMap.constEnd(); ++i)
        sections.append(QSettingsIniKey(i.key(), i.value().position));
    std::sort(sections.begin(), sections.end());

    bool writeError = false;
    for (int j = 0; !writeError && j < sectionCount; ++j) {
        i = iniMap.constFind(sections.at(j));
        Q_ASSERT(i != iniMap.constEnd());

        QByteArray realSection;

        iniEscapedKey(i.key(), realSection);

        if (realSection.isEmpty()) {
            realSection = "[General]";
        } else if (realSection.compare("general", Qt::CaseInsensitive) == 0) {
            realSection = "[%General]";
        } else {
            realSection.prepend('[');
            realSection.append(']');
        }

        if (j != 0)
            realSection.prepend(eol);
        realSection += eol;

        device.write(realSection);

        const IniKeyMap &ents = i.value().keyMap;
        for (IniKeyMap::const_iterator j = ents.constBegin(); j != ents.constEnd(); ++j) {
            QByteArray block;
            iniEscapedKey(j.key(), block);
            block += '=';

            const QVariant &value = j.value();

            /*
                The size() != 1 trick is necessary because
                QVariant(QString("foo")).toList() returns an empty
                list, not a list containing "foo".
            */
            if (value.type() == QVariant::StringList
                    || (value.type() == QVariant::List && value.toList().size() != 1)) {
                iniEscapedStringList(variantListToStringList(value.toList()), block, iniCodec);
            } else {
                iniEscapedString(variantToString(value), block, iniCodec);
            }
            block += eol;
            if (device.write(block) == -1) {
                writeError = true;
                break;
            }
        }
    }
    return !writeError;
}

void QConfFileSettingsPrivate::ensureAllSectionsParsed(QConfFile *confFile) const
{
    UnparsedSettingsMap::const_iterator i = confFile->unparsedIniSections.constBegin();
    const UnparsedSettingsMap::const_iterator end = confFile->unparsedIniSections.constEnd();

    for (; i != end; ++i) {
        if (!QConfFileSettingsPrivate::readIniSection(i.key(), i.value(), &confFile->originalKeys, iniCodec))
            setStatus(QSettings::FormatError);
    }
    confFile->unparsedIniSections.clear();
}

void QConfFileSettingsPrivate::ensureSectionParsed(QConfFile *confFile,
                                                   const QSettingsKey &key) const
{
    if (confFile->unparsedIniSections.isEmpty())
        return;

    UnparsedSettingsMap::iterator i;

    int indexOfSlash = key.indexOf(QLatin1Char('/'));
    if (indexOfSlash != -1) {
        i = confFile->unparsedIniSections.upperBound(key);
        if (i == confFile->unparsedIniSections.begin())
            return;
        --i;
        if (i.key().isEmpty() || !key.startsWith(i.key()))
            return;
    } else {
        i = confFile->unparsedIniSections.begin();
        if (i == confFile->unparsedIniSections.end() || !i.key().isEmpty())
            return;
    }

    if (!QConfFileSettingsPrivate::readIniSection(i.key(), i.value(), &confFile->originalKeys, iniCodec))
        setStatus(QSettings::FormatError);
    confFile->unparsedIniSections.erase(i);
}

/*!
    \class QSettings
    \inmodule QtCore
    \brief The QSettings class provides persistent platform-independent application settings.

    \ingroup io

    \reentrant

    Users normally expect an application to remember its settings
    (window sizes and positions, options, etc.) across sessions. This
    information is often stored in the system registry on Windows,
    and in property list files on \macos and iOS. On Unix systems, in the
    absence of a standard, many applications (including the KDE
    applications) use INI text files.

    QSettings is an abstraction around these technologies, enabling
    you to save and restore application settings in a portable
    manner. It also supports \l{registerFormat()}{custom storage
    formats}.

    QSettings's API is based on QVariant, allowing you to save
    most value-based types, such as QString, QRect, and QImage,
    with the minimum of effort.

    If all you need is a non-persistent memory-based structure,
    consider using QMap<QString, QVariant> instead.

    \tableofcontents section1

    \section1 Basic Usage

    When creating a QSettings object, you must pass the name of your
    company or organization as well as the name of your application.
    For example, if your product is called Star Runner and your
    company is called MySoft, you would construct the QSettings
    object as follows:

    \snippet settings/settings.cpp 0

    QSettings objects can be created either on the stack or on
    the heap (i.e. using \c new). Constructing and destroying a
    QSettings object is very fast.

    If you use QSettings from many places in your application, you
    might want to specify the organization name and the application
    name using QCoreApplication::setOrganizationName() and
    QCoreApplication::setApplicationName(), and then use the default
    QSettings constructor:

    \snippet settings/settings.cpp 1
    \snippet settings/settings.cpp 2
    \snippet settings/settings.cpp 3
    \dots
    \snippet settings/settings.cpp 4

    (Here, we also specify the organization's Internet domain. When
    the Internet domain is set, it is used on \macos and iOS instead of the
    organization name, since \macos and iOS applications conventionally use
    Internet domains to identify themselves. If no domain is set, a
    fake domain is derived from the organization name. See the
    \l{Platform-Specific Notes} below for details.)

    QSettings stores settings. Each setting consists of a QString
    that specifies the setting's name (the \e key) and a QVariant
    that stores the data associated with the key. To write a setting,
    use setValue(). For example:

    \snippet settings/settings.cpp 5

    If there already exists a setting with the same key, the existing
    value is overwritten by the new value. For efficiency, the
    changes may not be saved to permanent storage immediately. (You
    can always call sync() to commit your changes.)

    You can get a setting's value back using value():

    \snippet settings/settings.cpp 6

    If there is no setting with the specified name, QSettings
    returns a null QVariant (which can be converted to the integer 0).
    You can specify another default value by passing a second
    argument to value():

    \snippet settings/settings.cpp 7

    To test whether a given key exists, call contains(). To remove
    the setting associated with a key, call remove(). To obtain the
    list of all keys, call allKeys(). To remove all keys, call
    clear().

    \section1 QVariant and GUI Types

    Because QVariant is part of the Qt Core module, it cannot provide
    conversion functions to data types such as QColor, QImage, and
    QPixmap, which are part of Qt GUI. In other words, there is no
    \c toColor(), \c toImage(), or \c toPixmap() functions in QVariant.

    Instead, you can use the QVariant::value() or the qVariantValue()
    template function. For example:

    \snippet code/src_corelib_io_qsettings.cpp 0

    The inverse conversion (e.g., from QColor to QVariant) is
    automatic for all data types supported by QVariant, including
    GUI-related types:

    \snippet code/src_corelib_io_qsettings.cpp 1

    Custom types registered using qRegisterMetaType() and
    qRegisterMetaTypeStreamOperators() can be stored using QSettings.

    \section1 Section and Key Syntax

    Setting keys can contain any Unicode characters. The Windows
    registry and INI files use case-insensitive keys, whereas the
    CFPreferences API on \macos and iOS uses case-sensitive keys. To
    avoid portability problems, follow these simple rules:

    \list 1
    \li Always refer to the same key using the same case. For example,
       if you refer to a key as "text fonts" in one place in your
       code, don't refer to it as "Text Fonts" somewhere else.

    \li Avoid key names that are identical except for the case. For
       example, if you have a key called "MainWindow", don't try to
       save another key as "mainwindow".

    \li Do not use slashes ('/' and '\\') in section or key names; the
       backslash character is used to separate sub keys (see below). On
       windows '\\' are converted by QSettings to '/', which makes
       them identical.
    \endlist

    You can form hierarchical keys using the '/' character as a
    separator, similar to Unix file paths. For example:

    \snippet settings/settings.cpp 8
    \snippet settings/settings.cpp 9
    \snippet settings/settings.cpp 10

    If you want to save or restore many settings with the same
    prefix, you can specify the prefix using beginGroup() and call
    endGroup() at the end. Here's the same example again, but this
    time using the group mechanism:

    \snippet settings/settings.cpp 11
    \codeline
    \snippet settings/settings.cpp 12

    If a group is set using beginGroup(), the behavior of most
    functions changes consequently. Groups can be set recursively.

    In addition to groups, QSettings also supports an "array"
    concept. See beginReadArray() and beginWriteArray() for details.

    \section1 Fallback Mechanism

    Let's assume that you have created a QSettings object with the
    organization name MySoft and the application name Star Runner.
    When you look up a value, up to four locations are searched in
    that order:

    \list 1
    \li a user-specific location for the Star Runner application
    \li a user-specific location for all applications by MySoft
    \li a system-wide location for the Star Runner application
    \li a system-wide location for all applications by MySoft
    \endlist

    (See \l{Platform-Specific Notes} below for information on what
    these locations are on the different platforms supported by Qt.)

    If a key cannot be found in the first location, the search goes
    on in the second location, and so on. This enables you to store
    system-wide or organization-wide settings and to override them on
    a per-user or per-application basis. To turn off this mechanism,
    call setFallbacksEnabled(false).

    Although keys from all four locations are available for reading,
    only the first file (the user-specific location for the
    application at hand) is accessible for writing. To write to any
    of the other files, omit the application name and/or specify
    QSettings::SystemScope (as opposed to QSettings::UserScope, the
    default).

    Let's see with an example:

    \snippet settings/settings.cpp 13
    \snippet settings/settings.cpp 14

    The table below summarizes which QSettings objects access
    which location. "\b{X}" means that the location is the main
    location associated to the QSettings object and is used both
    for reading and for writing; "o" means that the location is used
    as a fallback when reading.

    \table
    \header \li Locations               \li \c{obj1} \li \c{obj2} \li \c{obj3} \li \c{obj4}
    \row    \li 1. User, Application    \li \b{X} \li          \li          \li
    \row    \li 2. User, Organization   \li o        \li \b{X} \li          \li
    \row    \li 3. System, Application  \li o        \li          \li \b{X} \li
    \row    \li 4. System, Organization \li o        \li o        \li o        \li \b{X}
    \endtable

    The beauty of this mechanism is that it works on all platforms
    supported by Qt and that it still gives you a lot of flexibility,
    without requiring you to specify any file names or registry
    paths.

    If you want to use INI files on all platforms instead of the
    native API, you can pass QSettings::IniFormat as the first
    argument to the QSettings constructor, followed by the scope, the
    organization name, and the application name:

    \snippet settings/settings.cpp 15

    The \l{tools/settingseditor}{Settings Editor} example lets you
    experiment with different settings location and with fallbacks
    turned on or off.

    \section1 Restoring the State of a GUI Application

    QSettings is often used to store the state of a GUI
    application. The following example illustrates how to use QSettings
    to save and restore the geometry of an application's main window.

    \snippet settings/settings.cpp 16
    \codeline
    \snippet settings/settings.cpp 17

    See \l{Window Geometry} for a discussion on why it is better to
    call QWidget::resize() and QWidget::move() rather than QWidget::setGeometry()
    to restore a window's geometry.

    The \c readSettings() and \c writeSettings() functions must be
    called from the main window's constructor and close event handler
    as follows:

    \snippet settings/settings.cpp 18
    \dots
    \snippet settings/settings.cpp 19
    \snippet settings/settings.cpp 20
    \codeline
    \snippet settings/settings.cpp 21

    See the \l{mainwindows/application}{Application} example for a
    self-contained example that uses QSettings.

    \section1 Accessing Settings from Multiple Threads or Processes Simultaneously

    QSettings is \l{reentrant}. This means that you can use
    distinct QSettings object in different threads
    simultaneously. This guarantee stands even when the QSettings
    objects refer to the same files on disk (or to the same entries
    in the system registry). If a setting is modified through one
    QSettings object, the change will immediately be visible in
    any other QSettings objects that operate on the same location
    and that live in the same process.

    QSettings can safely be used from different processes (which can
    be different instances of your application running at the same
    time or different applications altogether) to read and write to
    the same system locations, provided certain conditions are met. For
    QSettings::IniFormat, it uses advisory file locking and a smart merging
    algorithm to ensure data integrity. The condition for that to work is that
    the writeable configuration file must be a regular file and must reside in
    a directory that the current user can create new, temporary files in. If
    that is not the case, then one must use setAtomicSyncRequired() to turn the
    safety off.

    Note that sync() imports changes made by other processes (in addition to
    writing the changes from this QSettings).

    \section1 Platform-Specific Notes

    \section2 Locations Where Application Settings Are Stored

    As mentioned in the \l{Fallback Mechanism} section, QSettings
    stores settings for an application in up to four locations,
    depending on whether the settings are user-specific or
    system-wide and whether the settings are application-specific
    or organization-wide. For simplicity, we're assuming the
    organization is called MySoft and the application is called Star
    Runner.

    On Unix systems, if the file format is NativeFormat, the
    following files are used by default:

    \list 1
    \li \c{$HOME/.config/MySoft/Star Runner.conf} (Qt for Embedded Linux: \c{$HOME/Settings/MySoft/Star Runner.conf})
    \li \c{$HOME/.config/MySoft.conf} (Qt for Embedded Linux: \c{$HOME/Settings/MySoft.conf})
    \li for each directory <dir> in $XDG_CONFIG_DIRS: \c{<dir>/MySoft/Star Runner.conf}
    \li for each directory <dir> in $XDG_CONFIG_DIRS: \c{<dir>/MySoft.conf}
    \endlist
    \note If XDG_CONFIG_DIRS is unset, the default value of \c{/etc/xdg} is used.

    On \macos versions 10.2 and 10.3, these files are used by
    default:

    \list 1
    \li \c{$HOME/Library/Preferences/com.MySoft.Star Runner.plist}
    \li \c{$HOME/Library/Preferences/com.MySoft.plist}
    \li \c{/Library/Preferences/com.MySoft.Star Runner.plist}
    \li \c{/Library/Preferences/com.MySoft.plist}
    \endlist

    On Windows, NativeFormat settings are stored in the following
    registry paths:

    \list 1
    \li \c{HKEY_CURRENT_USER\Software\MySoft\Star Runner}
    \li \c{HKEY_CURRENT_USER\Software\MySoft\OrganizationDefaults}
    \li \c{HKEY_LOCAL_MACHINE\Software\MySoft\Star Runner}
    \li \c{HKEY_LOCAL_MACHINE\Software\MySoft\OrganizationDefaults}
    \endlist

    \note On Windows, for 32-bit programs running in WOW64 mode, settings are
    stored in the following registry path:
    \c{HKEY_LOCAL_MACHINE\Software\WOW6432node}.

    If the file format is NativeFormat, this is "Settings/MySoft/Star Runner.conf"
    in the application's home directory.

    If the file format is IniFormat, the following files are
    used on Unix, \macos, and iOS:

    \list 1
    \li \c{$HOME/.config/MySoft/Star Runner.ini} (Qt for Embedded Linux: \c{$HOME/Settings/MySoft/Star Runner.ini})
    \li \c{$HOME/.config/MySoft.ini} (Qt for Embedded Linux: \c{$HOME/Settings/MySoft.ini})
    \li for each directory <dir> in $XDG_CONFIG_DIRS: \c{<dir>/MySoft/Star Runner.ini}
    \li for each directory <dir> in $XDG_CONFIG_DIRS: \c{<dir>/MySoft.ini}
    \endlist
    \note If XDG_CONFIG_DIRS is unset, the default value of \c{/etc/xdg} is used.

    On Windows, the following files are used:

    \list 1
    \li \c{FOLDERID_RoamingAppData\MySoft\Star Runner.ini}
    \li \c{FOLDERID_RoamingAppData\MySoft.ini}
    \li \c{FOLDERID_ProgramData\MySoft\Star Runner.ini}
    \li \c{FOLDERID_ProgramData\MySoft.ini}
    \endlist

    The identifiers prefixed by \c{FOLDERID_} are special item ID lists to be passed
    to the Win32 API function \c{SHGetKnownFolderPath()} to obtain the
    corresponding path.

    \c{FOLDERID_RoamingAppData} usually points to \tt{C:\\Users\\\e{User Name}\\AppData\\Roaming},
    also shown by the environment variable \c{%APPDATA%}.

    \c{FOLDERID_ProgramData} usually points to \tt{C:\\ProgramData}.

    If the file format is IniFormat, this is "Settings/MySoft/Star Runner.ini"
    in the application's home directory.

    The paths for the \c .ini and \c .conf files can be changed using
    setPath(). On Unix, \macos, and iOS the user can override them by
    setting the \c XDG_CONFIG_HOME environment variable; see
    setPath() for details.

    \section2 Accessing INI and .plist Files Directly

    Sometimes you do want to access settings stored in a specific
    file or registry path. On all platforms, if you want to read an
    INI file directly, you can use the QSettings constructor that
    takes a file name as first argument and pass QSettings::IniFormat
    as second argument. For example:

    \snippet code/src_corelib_io_qsettings.cpp 2

    You can then use the QSettings object to read and write settings
    in the file.

    On \macos and iOS, you can access property list \c .plist files by passing
    QSettings::NativeFormat as second argument. For example:

    \snippet code/src_corelib_io_qsettings.cpp 3

    \section2 Accessing the Windows Registry Directly

    On Windows, QSettings lets you access settings that have been
    written with QSettings (or settings in a supported format, e.g., string
    data) in the system registry. This is done by constructing a QSettings
    object with a path in the registry and QSettings::NativeFormat.

    For example:

    \snippet code/src_corelib_io_qsettings.cpp 4

    All the registry entries that appear under the specified path can
    be read or written through the QSettings object as usual (using
    forward slashes instead of backslashes). For example:

    \snippet code/src_corelib_io_qsettings.cpp 5

    Note that the backslash character is, as mentioned, used by
    QSettings to separate subkeys. As a result, you cannot read or
    write windows registry entries that contain slashes or
    backslashes; you should use a native windows API if you need to do
    so.

    \section2 Accessing Common Registry Settings on Windows

    On Windows, it is possible for a key to have both a value and subkeys.
    Its default value is accessed by using "Default" or "." in
    place of a subkey:

    \snippet code/src_corelib_io_qsettings.cpp 6

    On other platforms than Windows, "Default" and "." would be
    treated as regular subkeys.

    \section2 Platform Limitations

    While QSettings attempts to smooth over the differences between
    the different supported platforms, there are still a few
    differences that you should be aware of when porting your
    application:

    \list
    \li  The Windows system registry has the following limitations: A
        subkey may not exceed 255 characters, an entry's value may
        not exceed 16,383 characters, and all the values of a key may
        not exceed 65,535 characters. One way to work around these
        limitations is to store the settings using the IniFormat
        instead of the NativeFormat.

    \li  On Windows, when the Windows system registry is used, QSettings
         does not preserve the original type of the value. Therefore,
         the type of the value might change when a new value is set. For
         example, a value with type \c REG_EXPAND_SZ will change to \c REG_SZ.

    \li  On \macos and iOS, allKeys() will return some extra keys for global
        settings that apply to all applications. These keys can be
        read using value() but cannot be changed, only shadowed.
        Calling setFallbacksEnabled(false) will hide these global
        settings.

    \li  On \macos and iOS, the CFPreferences API used by QSettings expects
        Internet domain names rather than organization names. To
        provide a uniform API, QSettings derives a fake domain name
        from the organization name (unless the organization name
        already is a domain name, e.g. OpenOffice.org). The algorithm
        appends ".com" to the company name and replaces spaces and
        other illegal characters with hyphens. If you want to specify
        a different domain name, call
        QCoreApplication::setOrganizationDomain(),
        QCoreApplication::setOrganizationName(), and
        QCoreApplication::setApplicationName() in your \c main()
        function and then use the default QSettings constructor.
        Another solution is to use preprocessor directives, for
        example:

        \snippet code/src_corelib_io_qsettings.cpp 7

    \li On \macos, permissions to access settings not belonging to the
       current user (i.e. SystemScope) have changed with 10.7 (Lion). Prior to
       that version, users having admin rights could access these. For 10.7 and
       10.8 (Mountain Lion), only root can. However, 10.9 (Mavericks) changes
       that rule again but only for the native format (plist files).

    \endlist

    \sa QVariant, QSessionManager, {Settings Editor Example}, {Application Example}
*/

/*! \enum QSettings::Status

    The following status values are possible:

    \value NoError  No error occurred.
    \value AccessError  An access error occurred (e.g. trying to write to a read-only file).
    \value FormatError  A format error occurred (e.g. loading a malformed INI file).

    \sa status()
*/

/*! \enum QSettings::Format

    This enum type specifies the storage format used by QSettings.

    \value NativeFormat     Store the settings using the most
                            appropriate storage format for the platform.
                            On Windows, this means the system registry;
                            on \macos and iOS, this means the CFPreferences
                            API; on Unix, this means textual
                            configuration files in INI format.
    \value Registry32Format Windows only: Explicitly access the 32-bit system registry
                            from a 64-bit application running on 64-bit Windows.
                            On 32-bit Windows or from a 32-bit application on 64-bit Windows,
                            this works the same as specifying NativeFormat.
                            This enum value was added in Qt 5.7.
    \value Registry64Format Windows only: Explicitly access the 64-bit system registry
                            from a 32-bit application running on 64-bit Windows.
                            On 32-bit Windows or from a 64-bit application on 64-bit Windows,
                            this works the same as specifying NativeFormat.
                            This enum value was added in Qt 5.7.
    \value IniFormat        Store the settings in INI files.
    \value InvalidFormat    Special value returned by registerFormat().
    \omitvalue CustomFormat1
    \omitvalue CustomFormat2
    \omitvalue CustomFormat3
    \omitvalue CustomFormat4
    \omitvalue CustomFormat5
    \omitvalue CustomFormat6
    \omitvalue CustomFormat7
    \omitvalue CustomFormat8
    \omitvalue CustomFormat9
    \omitvalue CustomFormat10
    \omitvalue CustomFormat11
    \omitvalue CustomFormat12
    \omitvalue CustomFormat13
    \omitvalue CustomFormat14
    \omitvalue CustomFormat15
    \omitvalue CustomFormat16

    On Unix, NativeFormat and IniFormat mean the same thing, except
    that the file extension is different (\c .conf for NativeFormat,
    \c .ini for IniFormat).

    The INI file format is a Windows file format that Qt supports on
    all platforms. In the absence of an INI standard, we try to
    follow what Microsoft does, with the following exceptions:

    \list
    \li  If you store types that QVariant can't convert to QString
        (e.g., QPoint, QRect, and QSize), Qt uses an \c{@}-based
        syntax to encode the type. For example:

        \snippet code/src_corelib_io_qsettings.cpp 8

        To minimize compatibility issues, any \c @ that doesn't
        appear at the first position in the value or that isn't
        followed by a Qt type (\c Point, \c Rect, \c Size, etc.) is
        treated as a normal character.

    \li  Although backslash is a special character in INI files, most
        Windows applications don't escape backslashes (\c{\}) in file
        paths:

        \snippet code/src_corelib_io_qsettings.cpp 9

        QSettings always treats backslash as a special character and
        provides no API for reading or writing such entries.

    \li  The INI file format has severe restrictions on the syntax of
        a key. Qt works around this by using \c % as an escape
        character in keys. In addition, if you save a top-level
        setting (a key with no slashes in it, e.g., "someKey"), it
        will appear in the INI file's "General" section. To avoid
        overwriting other keys, if you save something using a key
        such as "General/someKey", the key will be located in the
        "%General" section, \e not in the "General" section.

    \li  Following the philosophy that we should be liberal in what
        we accept and conservative in what we generate, QSettings
        will accept Latin-1 encoded INI files, but generate pure
        ASCII files, where non-ASCII values are encoded using standard
        INI escape sequences. To make the INI files more readable (but
        potentially less compatible), call setIniCodec().
    \endlist

    \sa registerFormat(), setPath()
*/

/*! \enum QSettings::Scope

    This enum specifies whether settings are user-specific or shared
    by all users of the same system.

    \value UserScope  Store settings in a location specific to the
                      current user (e.g., in the user's home
                      directory).
    \value SystemScope  Store settings in a global location, so that
                        all users on the same machine access the same
                        set of settings.

    \sa setPath()
*/

#ifndef QT_NO_QOBJECT
/*!
    Constructs a QSettings object for accessing settings of the
    application called \a application from the organization called \a
    organization, and with parent \a parent.

    Example:
    \snippet code/src_corelib_io_qsettings.cpp 10

    The scope is set to QSettings::UserScope, and the format is
    set to QSettings::NativeFormat (i.e. calling setDefaultFormat()
    before calling this constructor has no effect).

    \sa setDefaultFormat(), {Fallback Mechanism}
*/
QSettings::QSettings(const QString &organization, const QString &application, QObject *parent)
    : QObject(*QSettingsPrivate::create(NativeFormat, UserScope, organization, application),
              parent)
{
}

/*!
    Constructs a QSettings object for accessing settings of the
    application called \a application from the organization called \a
    organization, and with parent \a parent.

    If \a scope is QSettings::UserScope, the QSettings object searches
    user-specific settings first, before it searches system-wide
    settings as a fallback. If \a scope is QSettings::SystemScope, the
    QSettings object ignores user-specific settings and provides
    access to system-wide settings.

    The storage format is set to QSettings::NativeFormat (i.e. calling
    setDefaultFormat() before calling this constructor has no effect).

    If no application name is given, the QSettings object will
    only access the organization-wide \l{Fallback Mechanism}{locations}.

    \sa setDefaultFormat()
*/
QSettings::QSettings(Scope scope, const QString &organization, const QString &application,
                     QObject *parent)
    : QObject(*QSettingsPrivate::create(NativeFormat, scope, organization, application), parent)
{
}

/*!
    Constructs a QSettings object for accessing settings of the
    application called \a application from the organization called
    \a organization, and with parent \a parent.

    If \a scope is QSettings::UserScope, the QSettings object searches
    user-specific settings first, before it searches system-wide
    settings as a fallback. If \a scope is
    QSettings::SystemScope, the QSettings object ignores user-specific
    settings and provides access to system-wide settings.

    If \a format is QSettings::NativeFormat, the native API is used for
    storing settings. If \a format is QSettings::IniFormat, the INI format
    is used.

    If no application name is given, the QSettings object will
    only access the organization-wide \l{Fallback Mechanism}{locations}.
*/
QSettings::QSettings(Format format, Scope scope, const QString &organization,
                     const QString &application, QObject *parent)
    : QObject(*QSettingsPrivate::create(format, scope, organization, application), parent)
{
}

/*!
    Constructs a QSettings object for accessing the settings
    stored in the file called \a fileName, with parent \a parent. If
    the file doesn't already exist, it is created.

    If \a format is QSettings::NativeFormat, the meaning of \a
    fileName depends on the platform. On Unix, \a fileName is the
    name of an INI file. On \macos and iOS, \a fileName is the name of a
    \c .plist file. On Windows, \a fileName is a path in the system
    registry.

    If \a format is QSettings::IniFormat, \a fileName is the name of an INI
    file.

    \warning This function is provided for convenience. It works well for
    accessing INI or \c .plist files generated by Qt, but might fail on some
    syntaxes found in such files originated by other programs. In particular,
    be aware of the following limitations:

    \list
    \li QSettings provides no way of reading INI "path" entries, i.e., entries
       with unescaped slash characters. (This is because these entries are
       ambiguous and cannot be resolved automatically.)
    \li In INI files, QSettings uses the \c @ character as a metacharacter in some
       contexts, to encode Qt-specific data types (e.g., \c @Rect), and might
       therefore misinterpret it when it occurs in pure INI files.
    \endlist

    \sa fileName()
*/
QSettings::QSettings(const QString &fileName, Format format, QObject *parent)
    : QObject(*QSettingsPrivate::create(fileName, format), parent)
{
}

/*!
    Constructs a QSettings object for accessing settings of the
    application and organization set previously with a call to
    QCoreApplication::setOrganizationName(),
    QCoreApplication::setOrganizationDomain(), and
    QCoreApplication::setApplicationName().

    The scope is QSettings::UserScope and the format is
    defaultFormat() (QSettings::NativeFormat by default).
    Use setDefaultFormat() before calling this constructor
    to change the default format used by this constructor.

    The code

    \snippet code/src_corelib_io_qsettings.cpp 11

    is equivalent to

    \snippet code/src_corelib_io_qsettings.cpp 12

    If QCoreApplication::setOrganizationName() and
    QCoreApplication::setApplicationName() has not been previously
    called, the QSettings object will not be able to read or write
    any settings, and status() will return AccessError.

    On \macos and iOS, if both a name and an Internet domain are specified
    for the organization, the domain is preferred over the name. On
    other platforms, the name is preferred over the domain.

    \sa QCoreApplication::setOrganizationName(),
        QCoreApplication::setOrganizationDomain(),
        QCoreApplication::setApplicationName(),
        setDefaultFormat()
*/
QSettings::QSettings(QObject *parent)
    : QObject(*QSettingsPrivate::create(globalDefaultFormat, UserScope,
#ifdef Q_OS_MAC
                                        QCoreApplication::organizationDomain().isEmpty()
                                            ? QCoreApplication::organizationName()
                                            : QCoreApplication::organizationDomain()
#else
                                        QCoreApplication::organizationName().isEmpty()
                                            ? QCoreApplication::organizationDomain()
                                            : QCoreApplication::organizationName()
#endif
                                        , QCoreApplication::applicationName()),
              parent)
{
}

#else
QSettings::QSettings(const QString &organization, const QString &application)
    : d_ptr(QSettingsPrivate::create(globalDefaultFormat, QSettings::UserScope, organization, application))
{
    d_ptr->q_ptr = this;
}

QSettings::QSettings(Scope scope, const QString &organization, const QString &application)
    : d_ptr(QSettingsPrivate::create(globalDefaultFormat, scope, organization, application))
{
    d_ptr->q_ptr = this;
}

QSettings::QSettings(Format format, Scope scope, const QString &organization,
                     const QString &application)
    : d_ptr(QSettingsPrivate::create(format, scope, organization, application))
{
    d_ptr->q_ptr = this;
}

QSettings::QSettings(const QString &fileName, Format format)
    : d_ptr(QSettingsPrivate::create(fileName, format))
{
    d_ptr->q_ptr = this;
}
#endif

/*!
    Destroys the QSettings object.

    Any unsaved changes will eventually be written to permanent
    storage.

    \sa sync()
*/
QSettings::~QSettings()
{
    Q_D(QSettings);
    if (d->pendingChanges) {
        QT_TRY {
            d->flush();
        } QT_CATCH(...) {
            ; // ok. then don't flush but at least don't throw in the destructor
        }
    }
}

/*!
    Removes all entries in the primary location associated to this
    QSettings object.

    Entries in fallback locations are not removed.

    If you only want to remove the entries in the current group(),
    use remove("") instead.

    \sa remove(), setFallbacksEnabled()
*/
void QSettings::clear()
{
    Q_D(QSettings);
    d->clear();
    d->requestUpdate();
}

/*!
    Writes any unsaved changes to permanent storage, and reloads any
    settings that have been changed in the meantime by another
    application.

    This function is called automatically from QSettings's destructor and
    by the event loop at regular intervals, so you normally don't need to
    call it yourself.

    \sa status()
*/
void QSettings::sync()
{
    Q_D(QSettings);
    d->sync();
    d->pendingChanges = false;
}

/*!
    Returns the path where settings written using this QSettings
    object are stored.

    On Windows, if the format is QSettings::NativeFormat, the return value
    is a system registry path, not a file path.

    \sa isWritable(), format()
*/
QString QSettings::fileName() const
{
    Q_D(const QSettings);
    return d->fileName();
}

/*!
    \since 4.4

    Returns the format used for storing the settings.

    \sa defaultFormat(), fileName(), scope(), organizationName(), applicationName()
*/
QSettings::Format QSettings::format() const
{
    Q_D(const QSettings);
    return d->format;
}

/*!
    \since 4.4

    Returns the scope used for storing the settings.

    \sa format(), organizationName(), applicationName()
*/
QSettings::Scope QSettings::scope() const
{
    Q_D(const QSettings);
    return d->scope;
}

/*!
    \since 4.4

    Returns the organization name used for storing the settings.

    \sa QCoreApplication::organizationName(), format(), scope(), applicationName()
*/
QString QSettings::organizationName() const
{
    Q_D(const QSettings);
    return d->organizationName;
}

/*!
    \since 4.4

    Returns the application name used for storing the settings.

    \sa QCoreApplication::applicationName(), format(), scope(), organizationName()
*/
QString QSettings::applicationName() const
{
    Q_D(const QSettings);
    return d->applicationName;
}

#ifndef QT_NO_TEXTCODEC

/*!
    \since 4.5

    Sets the codec for accessing INI files (including \c .conf files on Unix)
    to \a codec. The codec is used for decoding any data that is read from
    the INI file, and for encoding any data that is written to the file. By
    default, no codec is used, and non-ASCII characters are encoded using
    standard INI escape sequences.

    \warning The codec must be set immediately after creating the QSettings
    object, before accessing any data.

    \sa iniCodec()
*/
void QSettings::setIniCodec(QTextCodec *codec)
{
    Q_D(QSettings);
    d->iniCodec = codec;
}

/*!
    \since 4.5
    \overload

    Sets the codec for accessing INI files (including \c .conf files on Unix)
    to the QTextCodec for the encoding specified by \a codecName. Common
    values for \c codecName include "ISO 8859-1", "UTF-8", and "UTF-16".
    If the encoding isn't recognized, nothing happens.

    \sa QTextCodec::codecForName()
*/
void QSettings::setIniCodec(const char *codecName)
{
    Q_D(QSettings);
    if (QTextCodec *codec = QTextCodec::codecForName(codecName))
        d->iniCodec = codec;
}

/*!
    \since 4.5

    Returns the codec that is used for accessing INI files. By default,
    no codec is used, so a null pointer is returned.
*/

QTextCodec *QSettings::iniCodec() const
{
    Q_D(const QSettings);
    return d->iniCodec;
}

#endif // QT_NO_TEXTCODEC

/*!
    Returns a status code indicating the first error that was met by
    QSettings, or QSettings::NoError if no error occurred.

    Be aware that QSettings delays performing some operations. For this
    reason, you might want to call sync() to ensure that the data stored
    in QSettings is written to disk before calling status().

    \sa sync()
*/
QSettings::Status QSettings::status() const
{
    Q_D(const QSettings);
    return d->status;
}

/*!
    \since 5.10

    Returns \c true if QSettings is only allowed to perform atomic saving and
    reloading (synchronization) of the settings. Returns \c false if it is
    allowed to save the settings contents directly to the configuration file.

    The default is \c true.

    \sa setAtomicSyncRequired(), QSaveFile
*/
bool QSettings::isAtomicSyncRequired() const
{
    Q_D(const QSettings);
    return d->atomicSyncOnly;
}

/*!
    \since 5.10

    Configures whether QSettings is required to perform atomic saving and
    reloading (synchronization) of the settings. If the \a enable argument is
    \c true (the default), sync() will only perform synchronization operations
    that are atomic. If this is not possible, sync() will fail and status()
    will be an error condition.

    Setting this property to \c false will allow QSettings to write directly to
    the configuration file and ignore any errors trying to lock it against
    other processes trying to write at the same time. Because of the potential
    for corruption, this option should be used with care, but is required in
    certain conditions, like a QSettings::IniFormat configuration file that
    exists in an otherwise non-writeable directory or NTFS Alternate Data
    Streams.

    See \l QSaveFile for more information on the feature.

    \sa isAtomicSyncRequired(), QSaveFile
*/
void QSettings::setAtomicSyncRequired(bool enable)
{
    Q_D(QSettings);
    d->atomicSyncOnly = enable;
}

/*!
    Appends \a prefix to the current group.

    The current group is automatically prepended to all keys
    specified to QSettings. In addition, query functions such as
    childGroups(), childKeys(), and allKeys() are based on the group.
    By default, no group is set.

    Groups are useful to avoid typing in the same setting paths over
    and over. For example:

    \snippet code/src_corelib_io_qsettings.cpp 13

    This will set the value of three settings:

    \list
    \li \c mainwindow/size
    \li \c mainwindow/fullScreen
    \li \c outputpanel/visible
    \endlist

    Call endGroup() to reset the current group to what it was before
    the corresponding beginGroup() call. Groups can be nested.

    \sa endGroup(), group()
*/
void QSettings::beginGroup(const QString &prefix)
{
    Q_D(QSettings);
    d->beginGroupOrArray(QSettingsGroup(d->normalizedKey(prefix)));
}

/*!
    Resets the group to what it was before the corresponding
    beginGroup() call.

    Example:

    \snippet code/src_corelib_io_qsettings.cpp 14

    \sa beginGroup(), group()
*/
void QSettings::endGroup()
{
    Q_D(QSettings);
    if (d->groupStack.isEmpty()) {
        qWarning("QSettings::endGroup: No matching beginGroup()");
        return;
    }

    QSettingsGroup group = d->groupStack.pop();
    int len = group.toString().size();
    if (len > 0)
        d->groupPrefix.truncate(d->groupPrefix.size() - (len + 1));

    if (group.isArray())
        qWarning("QSettings::endGroup: Expected endArray() instead");
}

/*!
    Returns the current group.

    \sa beginGroup(), endGroup()
*/
QString QSettings::group() const
{
    Q_D(const QSettings);
    return d->groupPrefix.left(d->groupPrefix.size() - 1);
}

/*!
    Adds \a prefix to the current group and starts reading from an
    array. Returns the size of the array.

    Example:

    \snippet code/src_corelib_io_qsettings.cpp 15

    Use beginWriteArray() to write the array in the first place.

    \sa beginWriteArray(), endArray(), setArrayIndex()
*/
int QSettings::beginReadArray(const QString &prefix)
{
    Q_D(QSettings);
    d->beginGroupOrArray(QSettingsGroup(d->normalizedKey(prefix), false));
    return value(QLatin1String("size")).toInt();
}

/*!
    Adds \a prefix to the current group and starts writing an array
    of size \a size. If \a size is -1 (the default), it is automatically
    determined based on the indexes of the entries written.

    If you have many occurrences of a certain set of keys, you can
    use arrays to make your life easier. For example, let's suppose
    that you want to save a variable-length list of user names and
    passwords. You could then write:

    \snippet code/src_corelib_io_qsettings.cpp 16

    The generated keys will have the form

    \list
    \li \c logins/size
    \li \c logins/1/userName
    \li \c logins/1/password
    \li \c logins/2/userName
    \li \c logins/2/password
    \li \c logins/3/userName
    \li \c logins/3/password
    \li ...
    \endlist

    To read back an array, use beginReadArray().

    \sa beginReadArray(), endArray(), setArrayIndex()
*/
void QSettings::beginWriteArray(const QString &prefix, int size)
{
    Q_D(QSettings);
    d->beginGroupOrArray(QSettingsGroup(d->normalizedKey(prefix), size < 0));

    if (size < 0)
        remove(QLatin1String("size"));
    else
        setValue(QLatin1String("size"), size);
}

/*!
    Closes the array that was started using beginReadArray() or
    beginWriteArray().

    \sa beginReadArray(), beginWriteArray()
*/
void QSettings::endArray()
{
    Q_D(QSettings);
    if (d->groupStack.isEmpty()) {
        qWarning("QSettings::endArray: No matching beginArray()");
        return;
    }

    QSettingsGroup group = d->groupStack.top();
    int len = group.toString().size();
    d->groupStack.pop();
    if (len > 0)
        d->groupPrefix.truncate(d->groupPrefix.size() - (len + 1));

    if (group.arraySizeGuess() != -1)
        setValue(group.name() + QLatin1String("/size"), group.arraySizeGuess());

    if (!group.isArray())
        qWarning("QSettings::endArray: Expected endGroup() instead");
}

/*!
    Sets the current array index to \a i. Calls to functions such as
    setValue(), value(), remove(), and contains() will operate on the
    array entry at that index.

    You must call beginReadArray() or beginWriteArray() before you
    can call this function.
*/
void QSettings::setArrayIndex(int i)
{
    Q_D(QSettings);
    if (d->groupStack.isEmpty() || !d->groupStack.top().isArray()) {
        qWarning("QSettings::setArrayIndex: Missing beginArray()");
        return;
    }

    QSettingsGroup &top = d->groupStack.top();
    int len = top.toString().size();
    top.setArrayIndex(qMax(i, 0));
    d->groupPrefix.replace(d->groupPrefix.size() - len - 1, len, top.toString());
}

/*!
    Returns a list of all keys, including subkeys, that can be read
    using the QSettings object.

    Example:

    \snippet code/src_corelib_io_qsettings.cpp 17

    If a group is set using beginGroup(), only the keys in the group
    are returned, without the group prefix:

    \snippet code/src_corelib_io_qsettings.cpp 18

    \sa childGroups(), childKeys()
*/
QStringList QSettings::allKeys() const
{
    Q_D(const QSettings);
    return d->children(d->groupPrefix, QSettingsPrivate::AllKeys);
}

/*!
    Returns a list of all top-level keys that can be read using the
    QSettings object.

    Example:

    \snippet code/src_corelib_io_qsettings.cpp 19

    If a group is set using beginGroup(), the top-level keys in that
    group are returned, without the group prefix:

    \snippet code/src_corelib_io_qsettings.cpp 20

    You can navigate through the entire setting hierarchy using
    childKeys() and childGroups() recursively.

    \sa childGroups(), allKeys()
*/
QStringList QSettings::childKeys() const
{
    Q_D(const QSettings);
    return d->children(d->groupPrefix, QSettingsPrivate::ChildKeys);
}

/*!
    Returns a list of all key top-level groups that contain keys that
    can be read using the QSettings object.

    Example:

    \snippet code/src_corelib_io_qsettings.cpp 21

    If a group is set using beginGroup(), the first-level keys in
    that group are returned, without the group prefix.

    \snippet code/src_corelib_io_qsettings.cpp 22

    You can navigate through the entire setting hierarchy using
    childKeys() and childGroups() recursively.

    \sa childKeys(), allKeys()
*/
QStringList QSettings::childGroups() const
{
    Q_D(const QSettings);
    return d->children(d->groupPrefix, QSettingsPrivate::ChildGroups);
}

/*!
    Returns \c true if settings can be written using this QSettings
    object; returns \c false otherwise.

    One reason why isWritable() might return false is if
    QSettings operates on a read-only file.

    \warning This function is not perfectly reliable, because the
    file permissions can change at any time.

    \sa fileName(), status(), sync()
*/
bool QSettings::isWritable() const
{
    Q_D(const QSettings);
    return d->isWritable();
}

/*!

  Sets the value of setting \a key to \a value. If the \a key already
  exists, the previous value is overwritten.

  Note that the Windows registry and INI files use case-insensitive
  keys, whereas the CFPreferences API on \macos and iOS uses
  case-sensitive keys. To avoid portability problems, see the
  \l{Section and Key Syntax} rules.

  Example:

  \snippet code/src_corelib_io_qsettings.cpp 23

  \sa value(), remove(), contains()
*/
void QSettings::setValue(const QString &key, const QVariant &value)
{
    Q_D(QSettings);
    if (key.isEmpty()) {
        qWarning("QSettings::setValue: Empty key passed");
        return;
    }
    QString k = d->actualKey(key);
    d->set(k, value);
    d->requestUpdate();
}

/*!
    Removes the setting \a key and any sub-settings of \a key.

    Example:

    \snippet code/src_corelib_io_qsettings.cpp 24

    Be aware that if one of the fallback locations contains a setting
    with the same key, that setting will be visible after calling
    remove().

    If \a key is an empty string, all keys in the current group() are
    removed. For example:

    \snippet code/src_corelib_io_qsettings.cpp 25

    Note that the Windows registry and INI files use case-insensitive
    keys, whereas the CFPreferences API on \macos and iOS uses
    case-sensitive keys. To avoid portability problems, see the
    \l{Section and Key Syntax} rules.

    \sa setValue(), value(), contains()
*/
void QSettings::remove(const QString &key)
{
    Q_D(QSettings);
    /*
        We cannot use actualKey(), because remove() supports empty
        keys. The code is also tricky because of slash handling.
    */
    QString theKey = d->normalizedKey(key);
    if (theKey.isEmpty())
        theKey = group();
    else
        theKey.prepend(d->groupPrefix);

    if (theKey.isEmpty()) {
        d->clear();
    } else {
        d->remove(theKey);
    }
    d->requestUpdate();
}

/*!
    Returns \c true if there exists a setting called \a key; returns
    false otherwise.

    If a group is set using beginGroup(), \a key is taken to be
    relative to that group.

    Note that the Windows registry and INI files use case-insensitive
    keys, whereas the CFPreferences API on \macos and iOS uses
    case-sensitive keys. To avoid portability problems, see the
    \l{Section and Key Syntax} rules.

    \sa value(), setValue()
*/
bool QSettings::contains(const QString &key) const
{
    Q_D(const QSettings);
    QString k = d->actualKey(key);
    return d->get(k, 0);
}

/*!
    Sets whether fallbacks are enabled to \a b.

    By default, fallbacks are enabled.

    \sa fallbacksEnabled()
*/
void QSettings::setFallbacksEnabled(bool b)
{
    Q_D(QSettings);
    d->fallbacks = !!b;
}

/*!
    Returns \c true if fallbacks are enabled; returns \c false otherwise.

    By default, fallbacks are enabled.

    \sa setFallbacksEnabled()
*/
bool QSettings::fallbacksEnabled() const
{
    Q_D(const QSettings);
    return d->fallbacks;
}

#ifndef QT_NO_QOBJECT
/*!
    \reimp
*/
bool QSettings::event(QEvent *event)
{
    Q_D(QSettings);
    if (event->type() == QEvent::UpdateRequest) {
        d->update();
        return true;
    }
    return QObject::event(event);
}
#endif

/*!
    Returns the value for setting \a key. If the setting doesn't
    exist, returns \a defaultValue.

    If no default value is specified, a default QVariant is
    returned.

    Note that the Windows registry and INI files use case-insensitive
    keys, whereas the CFPreferences API on \macos and iOS uses
    case-sensitive keys. To avoid portability problems, see the
    \l{Section and Key Syntax} rules.

    Example:

    \snippet code/src_corelib_io_qsettings.cpp 26

    \sa setValue(), contains(), remove()
*/
QVariant QSettings::value(const QString &key, const QVariant &defaultValue) const
{
    Q_D(const QSettings);
    if (key.isEmpty()) {
        qWarning("QSettings::value: Empty key passed");
        return QVariant();
    }
    QVariant result = defaultValue;
    QString k = d->actualKey(key);
    d->get(k, &result);
    return result;
}

/*!
    \since 4.4

    Sets the default file format to the given \a format, which is used
    for storing settings for the QSettings(QObject *) constructor.

    If no default format is set, QSettings::NativeFormat is used. See
    the documentation for the QSettings constructor you are using to
    see if that constructor will ignore this function.

    \sa format()
*/
void QSettings::setDefaultFormat(Format format)
{
    globalDefaultFormat = format;
}

/*!
    \since 4.4

    Returns default file format used for storing settings for the QSettings(QObject *) constructor.
    If no default format is set, QSettings::NativeFormat is used.

    \sa format()
*/
QSettings::Format QSettings::defaultFormat()
{
    return globalDefaultFormat;
}

/*!
    \obsolete

    Use setPath() instead.

    \oldcode
        setSystemIniPath(path);
    \newcode
        setPath(QSettings::NativeFormat, QSettings::SystemScope, path);
        setPath(QSettings::IniFormat, QSettings::SystemScope, path);
    \endcode
*/
void QSettings::setSystemIniPath(const QString &dir)
{
    setPath(IniFormat, SystemScope, dir);
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
    setPath(NativeFormat, SystemScope, dir);
#endif
}

/*!
    \obsolete

    Use setPath() instead.
*/

void QSettings::setUserIniPath(const QString &dir)
{
    setPath(IniFormat, UserScope, dir);
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
    setPath(NativeFormat, UserScope, dir);
#endif
}

/*!
    \since 4.1

    Sets the path used for storing settings for the given \a format
    and \a scope, to \a path. The \a format can be a custom format.

    The table below summarizes the default values:

    \table
    \header \li Platform         \li Format                       \li Scope       \li Path
    \row    \li{1,2} Windows     \li{1,2} IniFormat               \li UserScope   \li \c FOLDERID_RoamingAppData
    \row                                                        \li SystemScope \li \c FOLDERID_ProgramData
    \row    \li{1,2} Unix        \li{1,2} NativeFormat, IniFormat \li UserScope   \li \c $HOME/.config
    \row                                                        \li SystemScope \li \c /etc/xdg
    \row    \li{1,2} Qt for Embedded Linux \li{1,2} NativeFormat, IniFormat \li UserScope   \li \c $HOME/Settings
    \row                                                        \li SystemScope \li \c /etc/xdg
    \row    \li{1,2} \macos and iOS   \li{1,2} IniFormat               \li UserScope   \li \c $HOME/.config
    \row                                                        \li SystemScope \li \c /etc/xdg
    \endtable

    The default UserScope paths on Unix, \macos, and iOS (\c
    $HOME/.config or $HOME/Settings) can be overridden by the user by setting the
    \c XDG_CONFIG_HOME environment variable. The default SystemScope
    paths on Unix, \macos, and iOS (\c /etc/xdg) can be overridden when
    building the Qt library using the \c configure script's \c
    -sysconfdir flag (see QLibraryInfo for details).

    Setting the NativeFormat paths on Windows, \macos, and iOS has no
    effect.

    \warning This function doesn't affect existing QSettings objects.

    \sa registerFormat()
*/
void QSettings::setPath(Format format, Scope scope, const QString &path)
{
    QMutexLocker locker(&settingsGlobalMutex);
    PathHash *pathHash = pathHashFunc();
    if (pathHash->isEmpty())
        initDefaultPaths(&locker);
    pathHash->insert(pathHashKey(format, scope), Path(path + QDir::separator(), true));
}

/*!
    \typedef QSettings::SettingsMap

    Typedef for QMap<QString, QVariant>.

    \sa registerFormat()
*/

/*!
    \typedef QSettings::ReadFunc

    Typedef for a pointer to a function with the following signature:

    \snippet code/src_corelib_io_qsettings.cpp 27

    \c ReadFunc is used in \c registerFormat() as a pointer to a function
    that reads a set of key/value pairs. \c ReadFunc should read all the
    options in one pass, and return all the settings in the \c SettingsMap
    container, which is initially empty.

    \sa WriteFunc, registerFormat()
*/

/*!
    \typedef QSettings::WriteFunc

    Typedef for a pointer to a function with the following signature:

    \snippet code/src_corelib_io_qsettings.cpp 28

    \c WriteFunc is used in \c registerFormat() as a pointer to a function
    that writes a set of key/value pairs. \c WriteFunc is only called once,
    so you need to output the settings in one go.

    \sa ReadFunc, registerFormat()
*/

/*!
    \since 4.1
    \threadsafe

    Registers a custom storage format. On success, returns a special
    Format value that can then be passed to the QSettings constructor.
    On failure, returns InvalidFormat.

    The \a extension is the file
    extension associated to the format (without the '.').

    The \a readFunc and \a writeFunc parameters are pointers to
    functions that read and write a set of key/value pairs. The
    QIODevice parameter to the read and write functions is always
    opened in binary mode (i.e., without the QIODevice::Text flag).

    The \a caseSensitivity parameter specifies whether keys are case
    sensitive or not. This makes a difference when looking up values
    using QSettings. The default is case sensitive.

    By default, if you use one of the constructors that work in terms
    of an organization name and an application name, the file system
    locations used are the same as for IniFormat. Use setPath() to
    specify other locations.

    Example:

    \snippet code/src_corelib_io_qsettings.cpp 29

    \sa setPath()
*/
QSettings::Format QSettings::registerFormat(const QString &extension, ReadFunc readFunc,
                                            WriteFunc writeFunc,
                                            Qt::CaseSensitivity caseSensitivity)
{
#ifdef QT_QSETTINGS_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER
    Q_ASSERT(caseSensitivity == Qt::CaseSensitive);
#endif

    QMutexLocker locker(&settingsGlobalMutex);
    CustomFormatVector *customFormatVector = customFormatVectorFunc();
    int index = customFormatVector->size();
    if (index == 16) // the QSettings::Format enum has room for 16 custom formats
        return QSettings::InvalidFormat;

    QConfFileCustomFormat info;
    info.extension = QLatin1Char('.') + extension;
    info.readFunc = readFunc;
    info.writeFunc = writeFunc;
    info.caseSensitivity = caseSensitivity;
    customFormatVector->append(info);

    return QSettings::Format((int)QSettings::CustomFormat1 + index);
}

QT_END_NAMESPACE

#ifndef QT_BOOTSTRAPPED
#include "moc_qsettings.cpp"
#endif

#endif // QT_NO_SETTINGS
