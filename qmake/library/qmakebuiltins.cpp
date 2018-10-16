/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmake application of the Qt Toolkit.
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

#include "qmakeevaluator.h"

#include "qmakeevaluator_p.h"
#include "qmakeglobals.h"
#include "qmakeparser.h"
#include "qmakevfs.h"
#include "ioutils.h"

#include <qbytearray.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qregexp.h>
#include <qset.h>
#include <qstringlist.h>
#include <qtextstream.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
# include <qjsondocument.h>
# include <qjsonobject.h>
# include <qjsonarray.h>
#endif
#ifdef PROEVALUATOR_THREAD_SAFE
# include <qthreadpool.h>
#endif
#include <qversionnumber.h>

#include <algorithm>

#ifdef Q_OS_UNIX
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#else
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef Q_OS_WIN32
#define QT_POPEN _popen
#define QT_POPEN_READ "rb"
#define QT_PCLOSE _pclose
#else
#define QT_POPEN popen
#define QT_POPEN_READ "r"
#define QT_PCLOSE pclose
#endif

using namespace QMakeInternal;

QT_BEGIN_NAMESPACE

#define fL1S(s) QString::fromLatin1(s)

enum ExpandFunc {
    E_INVALID = 0, E_MEMBER, E_STR_MEMBER, E_FIRST, E_TAKE_FIRST, E_LAST, E_TAKE_LAST,
    E_SIZE, E_STR_SIZE, E_CAT, E_FROMFILE, E_EVAL, E_LIST, E_SPRINTF, E_FORMAT_NUMBER,
    E_NUM_ADD, E_JOIN, E_SPLIT, E_BASENAME, E_DIRNAME, E_SECTION,
    E_FIND, E_SYSTEM, E_UNIQUE, E_SORTED, E_REVERSE, E_QUOTE, E_ESCAPE_EXPAND,
    E_UPPER, E_LOWER, E_TITLE, E_FILES, E_PROMPT, E_RE_ESCAPE, E_VAL_ESCAPE,
    E_REPLACE, E_SORT_DEPENDS, E_RESOLVE_DEPENDS, E_ENUMERATE_VARS,
    E_SHADOWED, E_ABSOLUTE_PATH, E_RELATIVE_PATH, E_CLEAN_PATH,
    E_SYSTEM_PATH, E_SHELL_PATH, E_SYSTEM_QUOTE, E_SHELL_QUOTE, E_GETENV
};

enum TestFunc {
    T_INVALID = 0, T_REQUIRES, T_GREATERTHAN, T_LESSTHAN, T_EQUALS,
    T_VERSION_AT_LEAST, T_VERSION_AT_MOST,
    T_EXISTS, T_EXPORT, T_CLEAR, T_UNSET, T_EVAL, T_CONFIG, T_SYSTEM,
    T_DEFINED, T_DISCARD_FROM, T_CONTAINS, T_INFILE,
    T_COUNT, T_ISEMPTY, T_PARSE_JSON, T_INCLUDE, T_LOAD, T_DEBUG, T_LOG, T_MESSAGE, T_WARNING, T_ERROR, T_IF,
    T_MKPATH, T_WRITE_FILE, T_TOUCH, T_CACHE, T_RELOAD_PROPERTIES
};

QMakeBuiltin::QMakeBuiltin(const QMakeBuiltinInit &d)
    : index(d.func), minArgs(qMax(0, d.min_args)), maxArgs(d.max_args)
{
    static const char * const nstr[6] = { "no", "one", "two", "three", "four", "five" };
    // For legacy reasons, there is actually no such thing as "no arguments"
    // - there is only "empty first argument", which needs to be mapped back.
    // -1 means "one, which may be empty", which is effectively zero, except
    // for the error message if there are too many arguments.
    int dmin = qAbs(d.min_args);
    int dmax = d.max_args;
    if (dmax == QMakeBuiltinInit::VarArgs) {
        Q_ASSERT_X(dmin < 2, "init", d.name);
        if (dmin == 1) {
            Q_ASSERT_X(d.args != nullptr, "init", d.name);
            usage = fL1S("%1(%2) requires at least one argument.")
                    .arg(fL1S(d.name), fL1S(d.args));
        }
        return;
    }
    int arange = dmax - dmin;
    Q_ASSERT_X(arange >= 0, "init", d.name);
    Q_ASSERT_X(d.args != nullptr, "init", d.name);
    usage = arange > 1
               ? fL1S("%1(%2) requires %3 to %4 arguments.")
                 .arg(fL1S(d.name), fL1S(d.args), fL1S(nstr[dmin]), fL1S(nstr[dmax]))
               : arange > 0
                   ? fL1S("%1(%2) requires %3 or %4 arguments.")
                     .arg(fL1S(d.name), fL1S(d.args), fL1S(nstr[dmin]), fL1S(nstr[dmax]))
                   : dmax != 1
                       ? fL1S("%1(%2) requires %3 arguments.")
                         .arg(fL1S(d.name), fL1S(d.args), fL1S(nstr[dmax]))
                       : fL1S("%1(%2) requires one argument.")
                         .arg(fL1S(d.name), fL1S(d.args));
}

void QMakeEvaluator::initFunctionStatics()
{
    static const QMakeBuiltinInit expandInits[] = {
        { "member", E_MEMBER, 1, 3, "var, [start, [end]]" },
        { "str_member", E_STR_MEMBER, -1, 3, "str, [start, [end]]" },
        { "first", E_FIRST, 1, 1, "var" },
        { "take_first", E_TAKE_FIRST, 1, 1, "var" },
        { "last", E_LAST, 1, 1, "var" },
        { "take_last", E_TAKE_LAST, 1, 1, "var" },
        { "size", E_SIZE, 1, 1, "var" },
        { "str_size", E_STR_SIZE, -1, 1, "str" },
        { "cat", E_CAT, 1, 2, "file, [mode=true|blob|lines]" },
        { "fromfile", E_FROMFILE, 2, 2, "file, var" },
        { "eval", E_EVAL, 1, 1, "var" },
        { "list", E_LIST, 0, QMakeBuiltinInit::VarArgs, nullptr },
        { "sprintf", E_SPRINTF, 1, QMakeBuiltinInit::VarArgs, "format, ..." },
        { "format_number", E_FORMAT_NUMBER, 1, 2, "number, [options...]" },
        { "num_add", E_NUM_ADD, 1, QMakeBuiltinInit::VarArgs, "num, ..." },
        { "join", E_JOIN, 1, 4, "var, [glue, [before, [after]]]" },
        { "split", E_SPLIT, 1, 2, "var, sep" },
        { "basename", E_BASENAME, 1, 1, "var" },
        { "dirname", E_DIRNAME, 1, 1, "var" },
        { "section", E_SECTION, 3, 4, "var, sep, begin, [end]" },
        { "find", E_FIND, 2, 2, "var, str" },
        { "system", E_SYSTEM, 1, 3, "command, [mode], [stsvar]" },
        { "unique", E_UNIQUE, 1, 1, "var" },
        { "sorted", E_SORTED, 1, 1, "var" },
        { "reverse", E_REVERSE, 1, 1, "var" },
        { "quote", E_QUOTE, 0, QMakeBuiltinInit::VarArgs, nullptr },
        { "escape_expand", E_ESCAPE_EXPAND, 0, QMakeBuiltinInit::VarArgs, nullptr },
        { "upper", E_UPPER, 0, QMakeBuiltinInit::VarArgs, nullptr },
        { "lower", E_LOWER, 0, QMakeBuiltinInit::VarArgs, nullptr },
        { "title", E_TITLE, 0, QMakeBuiltinInit::VarArgs, nullptr },
        { "re_escape", E_RE_ESCAPE, 0, QMakeBuiltinInit::VarArgs, nullptr },
        { "val_escape", E_VAL_ESCAPE, 1, 1, "var" },
        { "files", E_FILES, 1, 2, "pattern, [recursive=false]" },
        { "prompt", E_PROMPT, 1, 2, "question, [decorate=true]" },
        { "replace", E_REPLACE, 3, 3, "var, before, after" },
        { "sort_depends", E_SORT_DEPENDS, 1, 4, "var, [prefix, [suffixes, [prio-suffix]]]" },
        { "resolve_depends", E_RESOLVE_DEPENDS, 1, 4, "var, [prefix, [suffixes, [prio-suffix]]]" },
        { "enumerate_vars", E_ENUMERATE_VARS, 0, 0, "" },
        { "shadowed", E_SHADOWED, 1, 1, "path" },
        { "absolute_path", E_ABSOLUTE_PATH, -1, 2, "path, [base]" },
        { "relative_path", E_RELATIVE_PATH, -1, 2, "path, [base]" },
        { "clean_path", E_CLEAN_PATH, -1, 1, "path" },
        { "system_path", E_SYSTEM_PATH, -1, 1, "path" },
        { "shell_path", E_SHELL_PATH, -1, 1, "path" },
        { "system_quote", E_SYSTEM_QUOTE, -1, 1, "arg" },
        { "shell_quote", E_SHELL_QUOTE, -1, 1, "arg" },
        { "getenv", E_GETENV, 1, 1, "arg" },
    };
    statics.expands.reserve((int)(sizeof(expandInits)/sizeof(expandInits[0])));
    for (unsigned i = 0; i < sizeof(expandInits)/sizeof(expandInits[0]); ++i)
        statics.expands.insert(ProKey(expandInits[i].name), QMakeBuiltin(expandInits[i]));

    static const QMakeBuiltinInit testInits[] = {
        { "requires", T_REQUIRES, 0, QMakeBuiltinInit::VarArgs, nullptr },
        { "greaterThan", T_GREATERTHAN, 2, 2, "var, val" },
        { "lessThan", T_LESSTHAN, 2, 2, "var, val" },
        { "equals", T_EQUALS, 2, 2, "var, val" },
        { "isEqual", T_EQUALS, 2, 2, "var, val" },
        { "versionAtLeast", T_VERSION_AT_LEAST, 2, 2, "var, version" },
        { "versionAtMost", T_VERSION_AT_MOST, 2, 2, "var, version" },
        { "exists", T_EXISTS, 1, 1, "file" },
        { "export", T_EXPORT, 1, 1, "var" },
        { "clear", T_CLEAR, 1, 1, "var" },
        { "unset", T_UNSET, 1, 1, "var" },
        { "eval", T_EVAL, 0, QMakeBuiltinInit::VarArgs, nullptr },
        { "CONFIG", T_CONFIG, 1, 2, "config, [mutuals]" },
        { "if", T_IF, 1, 1, "condition" },
        { "isActiveConfig", T_CONFIG, 1, 2, "config, [mutuals]" },
        { "system", T_SYSTEM, 1, 1, "exec" },
        { "discard_from", T_DISCARD_FROM, 1, 1, "file" },
        { "defined", T_DEFINED, 1, 2, "object, [\"test\"|\"replace\"|\"var\"]" },
        { "contains", T_CONTAINS, 2, 3, "var, val, [mutuals]" },
        { "infile", T_INFILE, 2, 3, "file, var, [values]" },
        { "count", T_COUNT, 2, 3, "var, count, [op=operator]" },
        { "isEmpty", T_ISEMPTY, 1, 1, "var" },
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        { "parseJson", T_PARSE_JSON, 2, 2, "var, into" },
#endif
        { "load", T_LOAD, 1, 2, "feature, [ignore_errors=false]" },
        { "include", T_INCLUDE, 1, 3, "file, [into, [silent]]" },
        { "debug", T_DEBUG, 2, 2, "level, message" },
        { "log", T_LOG, 1, 1, "message" },
        { "message", T_MESSAGE, 1, 1, "message" },
        { "warning", T_WARNING, 1, 1, "message" },
        { "error", T_ERROR, 0, 1, "message" },
        { "mkpath", T_MKPATH, 1, 1, "path" },
        { "write_file", T_WRITE_FILE, 1, 3, "name, [content var, [append] [exe]]" },
        { "touch", T_TOUCH, 2, 2, "file, reffile" },
        { "cache", T_CACHE, 0, 3, "[var], [set|add|sub] [transient] [super|stash], [srcvar]" },
        { "reload_properties", T_RELOAD_PROPERTIES, 0, 0, "" },
    };
    statics.functions.reserve((int)(sizeof(testInits)/sizeof(testInits[0])));
    for (unsigned i = 0; i < sizeof(testInits)/sizeof(testInits[0]); ++i)
        statics.functions.insert(ProKey(testInits[i].name), QMakeBuiltin(testInits[i]));
}

static bool isTrue(const ProString &str)
{
    return !str.compare(statics.strtrue, Qt::CaseInsensitive) || str.toInt();
}

bool
QMakeEvaluator::getMemberArgs(const ProKey &func, int srclen, const ProStringList &args,
                              int *start, int *end)
{
    *start = 0, *end = 0;
    if (args.count() >= 2) {
        bool ok = true;
        const ProString &start_str = args.at(1);
        *start = start_str.toInt(&ok);
        if (!ok) {
            if (args.count() == 2) {
                int dotdot = start_str.indexOf(statics.strDotDot);
                if (dotdot != -1) {
                    *start = start_str.left(dotdot).toInt(&ok);
                    if (ok)
                        *end = start_str.mid(dotdot+2).toInt(&ok);
                }
            }
            if (!ok) {
                ProStringRoUser u1(func, m_tmp1);
                ProStringRoUser u2(start_str, m_tmp2);
                evalError(fL1S("%1() argument 2 (start) '%2' invalid.").arg(u1.str(), u2.str()));
                return false;
            }
        } else {
            *end = *start;
            if (args.count() == 3)
                *end = args.at(2).toInt(&ok);
            if (!ok) {
                ProStringRoUser u1(func, m_tmp1);
                ProStringRoUser u2(args.at(2), m_tmp2);
                evalError(fL1S("%1() argument 3 (end) '%2' invalid.").arg(u1.str(), u2.str()));
                return false;
            }
        }
    }
    if (*start < 0)
        *start += srclen;
    if (*end < 0)
        *end += srclen;
    if (*start < 0 || *start >= srclen || *end < 0 || *end >= srclen)
        return false;
    return true;
}

QString
QMakeEvaluator::quoteValue(const ProString &val)
{
    QString ret;
    ret.reserve(val.size());
    const QChar *chars = val.constData();
    bool quote = val.isEmpty();
    bool escaping = false;
    for (int i = 0, l = val.size(); i < l; i++) {
        QChar c = chars[i];
        ushort uc = c.unicode();
        if (uc < 32) {
            if (!escaping) {
                escaping = true;
                ret += QLatin1String("$$escape_expand(");
            }
            switch (uc) {
            case '\r':
                ret += QLatin1String("\\\\r");
                break;
            case '\n':
                ret += QLatin1String("\\\\n");
                break;
            case '\t':
                ret += QLatin1String("\\\\t");
                break;
            default:
                ret += QString::fromLatin1("\\\\x%1").arg(uc, 2, 16, QLatin1Char('0'));
                break;
            }
        } else {
            if (escaping) {
                escaping = false;
                ret += QLatin1Char(')');
            }
            switch (uc) {
            case '\\':
                ret += QLatin1String("\\\\");
                break;
            case '"':
                ret += QLatin1String("\\\"");
                break;
            case '\'':
                ret += QLatin1String("\\'");
                break;
            case '$':
                ret += QLatin1String("\\$");
                break;
            case '#':
                ret += QLatin1String("$${LITERAL_HASH}");
                break;
            case 32:
                quote = true;
                Q_FALLTHROUGH();
            default:
                ret += c;
                break;
            }
        }
    }
    if (escaping)
        ret += QLatin1Char(')');
    if (quote) {
        ret.prepend(QLatin1Char('"'));
        ret.append(QLatin1Char('"'));
    }
    return ret;
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
static void addJsonValue(const QJsonValue &value, const QString &keyPrefix, ProValueMap *map);

static void insertJsonKeyValue(const QString &key, const QStringList &values, ProValueMap *map)
{
    map->insert(ProKey(key), ProStringList(values));
}

static void addJsonArray(const QJsonArray &array, const QString &keyPrefix, ProValueMap *map)
{
    QStringList keys;
    const int size = array.count();
    keys.reserve(size);
    for (int i = 0; i < size; ++i) {
        const QString number = QString::number(i);
        keys.append(number);
        addJsonValue(array.at(i), keyPrefix + number, map);
    }
    insertJsonKeyValue(keyPrefix + QLatin1String("_KEYS_"), keys, map);
}

static void addJsonObject(const QJsonObject &object, const QString &keyPrefix, ProValueMap *map)
{
    QStringList keys;
    keys.reserve(object.size());
    for (auto it = object.begin(), end = object.end(); it != end; ++it) {
        const QString key = it.key();
        keys.append(key);
        addJsonValue(it.value(), keyPrefix + key, map);
    }
    insertJsonKeyValue(keyPrefix + QLatin1String("_KEYS_"), keys, map);
}

static void addJsonValue(const QJsonValue &value, const QString &keyPrefix, ProValueMap *map)
{
    switch (value.type()) {
    case QJsonValue::Bool:
        insertJsonKeyValue(keyPrefix, QStringList() << (value.toBool() ? QLatin1String("true") : QLatin1String("false")), map);
        break;
    case QJsonValue::Double:
        insertJsonKeyValue(keyPrefix, QStringList() << QString::number(value.toDouble()), map);
        break;
    case QJsonValue::String:
        insertJsonKeyValue(keyPrefix, QStringList() << value.toString(), map);
        break;
    case QJsonValue::Array:
        addJsonArray(value.toArray(), keyPrefix + QLatin1Char('.'), map);
        break;
    case QJsonValue::Object:
        addJsonObject(value.toObject(), keyPrefix + QLatin1Char('.'), map);
        break;
    default:
        break;
    }
}

struct ErrorPosition {
    int line;
    int column;
};

static ErrorPosition calculateErrorPosition(const QByteArray &json, int offset)
{
    ErrorPosition pos = { 0, 0 };
    offset--; // offset is 1-based, switching to 0-based
    for (int i = 0; i < offset; ++i) {
        switch (json.at(i)) {
        case '\n':
            pos.line++;
            pos.column = 0;
            break;
        case '\r':
            break;
        case '\t':
            pos.column = (pos.column + 8) & ~7;
            break;
        default:
            pos.column++;
            break;
        }
    }
    // Lines and columns in text editors are 1-based:
    pos.line++;
    pos.column++;
    return pos;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::parseJsonInto(const QByteArray &json, const QString &into, ProValueMap *value)
{
    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if (document.isNull()) {
        if (error.error != QJsonParseError::NoError) {
            ErrorPosition errorPos = calculateErrorPosition(json, error.offset);
            evalError(fL1S("Error parsing JSON at %1:%2: %3")
                      .arg(errorPos.line).arg(errorPos.column).arg(error.errorString()));
        }
        return QMakeEvaluator::ReturnFalse;
    }

    QString currentKey = into + QLatin1Char('.');

    // top-level item is either an array or object
    if (document.isArray())
        addJsonArray(document.array(), currentKey, value);
    else if (document.isObject())
        addJsonObject(document.object(), currentKey, value);
    else
        return QMakeEvaluator::ReturnFalse;

    return QMakeEvaluator::ReturnTrue;
}
#endif

QMakeEvaluator::VisitReturn
QMakeEvaluator::writeFile(const QString &ctx, const QString &fn, QIODevice::OpenMode mode,
                          QMakeVfs::VfsFlags flags, const QString &contents)
{
    int oldId = m_vfs->idForFileName(fn, flags | QMakeVfs::VfsAccessedOnly);
    int id = m_vfs->idForFileName(fn, flags | QMakeVfs::VfsCreate);
    QString errStr;
    if (!m_vfs->writeFile(id, mode, flags, contents, &errStr)) {
        evalError(fL1S("Cannot write %1file %2: %3")
                  .arg(ctx, QDir::toNativeSeparators(fn), errStr));
        return ReturnFalse;
    }
    if (oldId)
        m_parser->discardFileFromCache(oldId);
    return ReturnTrue;
}

#if QT_CONFIG(process)
void QMakeEvaluator::runProcess(QProcess *proc, const QString &command) const
{
    proc->setWorkingDirectory(currentDirectory());
# ifdef PROEVALUATOR_SETENV
    if (!m_option->environment.isEmpty())
        proc->setProcessEnvironment(m_option->environment);
# endif
# ifdef Q_OS_WIN
    proc->setNativeArguments(QLatin1String("/v:off /s /c \"") + command + QLatin1Char('"'));
    proc->start(m_option->getEnv(QLatin1String("COMSPEC")), QStringList());
# else
    proc->start(QLatin1String("/bin/sh"), QStringList() << QLatin1String("-c") << command);
# endif
    proc->waitForFinished(-1);
}
#endif

QByteArray QMakeEvaluator::getCommandOutput(const QString &args, int *exitCode) const
{
    QByteArray out;
#if QT_CONFIG(process)
    QProcess proc;
    runProcess(&proc, args);
    *exitCode = (proc.exitStatus() == QProcess::NormalExit) ? proc.exitCode() : -1;
    QByteArray errout = proc.readAllStandardError();
# ifdef PROEVALUATOR_FULL
    // FIXME: Qt really should have the option to set forwarding per channel
    fputs(errout.constData(), stderr);
# else
    if (!errout.isEmpty()) {
        if (errout.endsWith('\n'))
            errout.chop(1);
        m_handler->message(
            QMakeHandler::EvalError | (m_cumulative ? QMakeHandler::CumulativeEvalMessage : 0),
            QString::fromLocal8Bit(errout));
    }
# endif
    out = proc.readAllStandardOutput();
# ifdef Q_OS_WIN
    // FIXME: Qt's line end conversion on sequential files should really be fixed
    out.replace("\r\n", "\n");
# endif
#else
    if (FILE *proc = QT_POPEN(QString(QLatin1String("cd ")
                               + IoUtils::shellQuote(QDir::toNativeSeparators(currentDirectory()))
                               + QLatin1String(" && ") + args).toLocal8Bit().constData(), QT_POPEN_READ)) {
        while (!feof(proc)) {
            char buff[10 * 1024];
            int read_in = int(fread(buff, 1, sizeof(buff), proc));
            if (!read_in)
                break;
            out += QByteArray(buff, read_in);
        }
        int ec = QT_PCLOSE(proc);
# ifdef Q_OS_WIN
        *exitCode = ec >= 0 ? ec : -1;
# else
        *exitCode = WIFEXITED(ec) ? WEXITSTATUS(ec) : -1;
# endif
    }
# ifdef Q_OS_WIN
    out.replace("\r\n", "\n");
# endif
#endif
    return out;
}

void QMakeEvaluator::populateDeps(
        const ProStringList &deps, const ProString &prefix, const ProStringList &suffixes,
        const ProString &priosfx,
        QHash<ProKey, QSet<ProKey> > &dependencies, ProValueMap &dependees,
        QMultiMap<int, ProString> &rootSet) const
{
    for (const ProString &item : deps)
        if (!dependencies.contains(item.toKey())) {
            QSet<ProKey> &dset = dependencies[item.toKey()]; // Always create entry
            ProStringList depends;
            for (const ProString &suffix : suffixes)
                depends += values(ProKey(prefix + item + suffix));
            if (depends.isEmpty()) {
                rootSet.insert(first(ProKey(prefix + item + priosfx)).toInt(), item);
            } else {
                for (const ProString &dep : qAsConst(depends)) {
                    dset.insert(dep.toKey());
                    dependees[dep.toKey()] << item;
                }
                populateDeps(depends, prefix, suffixes, priosfx, dependencies, dependees, rootSet);
            }
        }
}

QString QMakeEvaluator::filePathArg0(const ProStringList &args)
{
    ProStringRoUser u1(args.at(0), m_tmp1);
    QString fn = resolvePath(u1.str());
    fn.detach();
    return fn;
}

QString QMakeEvaluator::filePathEnvArg0(const ProStringList &args)
{
    ProStringRoUser u1(args.at(0), m_tmp1);
    QString fn = resolvePath(m_option->expandEnvVars(u1.str()));
    fn.detach();
    return fn;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateBuiltinExpand(
        const QMakeBuiltin &adef, const ProKey &func, const ProStringList &args, ProStringList &ret)
{
    traceMsg("calling built-in $$%s(%s)", dbgKey(func), dbgSepStrList(args));
    int asz = args.size() > 1 ? args.size() : args.at(0).isEmpty() ? 0 : 1;
    if (asz < adef.minArgs || asz > adef.maxArgs) {
        evalError(adef.usage);
        return ReturnTrue;
    }

    int func_t = adef.index;
    switch (func_t) {
    case E_BASENAME:
    case E_DIRNAME:
    case E_SECTION: {
        bool regexp = false;
        QString sep;
        ProString var;
        int beg = 0;
        int end = -1;
        if (func_t == E_SECTION) {
            var = args[0];
            sep = args.at(1).toQString();
            beg = args.at(2).toInt();
            if (args.count() == 4)
                end = args.at(3).toInt();
        } else {
            var = args[0];
            regexp = true;
            sep = QLatin1String("[\\\\/]");
            if (func_t == E_DIRNAME)
                end = -2;
            else
                beg = -1;
        }
        if (!var.isEmpty()) {
            const auto strings = values(map(var));
            if (regexp) {
                QRegExp sepRx(sep);
                for (const ProString &str : strings) {
                    ProStringRwUser u1(str, m_tmp[m_toggle ^= 1]);
                    ret << u1.extract(u1.str().section(sepRx, beg, end));
                }
            } else {
                for (const ProString &str : strings) {
                    ProStringRwUser u1(str, m_tmp1);
                    ret << u1.extract(u1.str().section(sep, beg, end));
                }
            }
        }
        break;
    }
    case E_SPRINTF: {
        ProStringRwUser u1(args.at(0), m_tmp1);
        QString tmp = u1.str();
        for (int i = 1; i < args.count(); ++i)
            tmp = tmp.arg(args.at(i).toQStringView());
        ret << u1.extract(tmp);
        break;
    }
    case E_FORMAT_NUMBER: {
        int ibase = 10;
        int obase = 10;
        int width = 0;
        bool zeropad = false;
        bool leftalign = false;
        enum { DefaultSign, PadSign, AlwaysSign } sign = DefaultSign;
        if (args.count() >= 2) {
            const auto opts = split_value_list(args.at(1).toQStringRef());
            for (const ProString &opt : opts) {
                if (opt.startsWith(QLatin1String("ibase="))) {
                    ibase = opt.mid(6).toInt();
                } else if (opt.startsWith(QLatin1String("obase="))) {
                    obase = opt.mid(6).toInt();
                } else if (opt.startsWith(QLatin1String("width="))) {
                    width = opt.mid(6).toInt();
                } else if (opt == QLatin1String("zeropad")) {
                    zeropad = true;
                } else if (opt == QLatin1String("padsign")) {
                    sign = PadSign;
                } else if (opt == QLatin1String("alwayssign")) {
                    sign = AlwaysSign;
                } else if (opt == QLatin1String("leftalign")) {
                    leftalign = true;
                } else {
                    evalError(fL1S("format_number(): invalid format option %1.")
                              .arg(opt.toQStringView()));
                    goto allfail;
                }
            }
        }
        if (args.at(0).contains(QLatin1Char('.'))) {
            evalError(fL1S("format_number(): floats are currently not supported."));
            break;
        }
        bool ok;
        qlonglong num = args.at(0).toLongLong(&ok, ibase);
        if (!ok) {
            evalError(fL1S("format_number(): malformed number %2 for base %1.")
                      .arg(ibase).arg(args.at(0).toQStringView()));
            break;
        }
        QString outstr;
        if (num < 0) {
            num = -num;
            outstr = QLatin1Char('-');
        } else if (sign == AlwaysSign) {
            outstr = QLatin1Char('+');
        } else if (sign == PadSign) {
            outstr = QLatin1Char(' ');
        }
        QString numstr = QString::number(num, obase);
        int space = width - outstr.length() - numstr.length();
        if (space <= 0) {
            outstr += numstr;
        } else if (leftalign) {
            outstr += numstr + QString(space, QLatin1Char(' '));
        } else if (zeropad) {
            outstr += QString(space, QLatin1Char('0')) + numstr;
        } else {
            outstr.prepend(QString(space, QLatin1Char(' ')));
            outstr += numstr;
        }
        ret += ProString(outstr);
        break;
    }
    case E_NUM_ADD: {
        qlonglong sum = 0;
        for (const ProString &arg : qAsConst(args)) {
            if (arg.contains(QLatin1Char('.'))) {
                evalError(fL1S("num_add(): floats are currently not supported."));
                goto allfail;
            }
            bool ok;
            qlonglong num = arg.toLongLong(&ok);
            if (!ok) {
                evalError(fL1S("num_add(): malformed number %1.")
                          .arg(arg.toQStringView()));
                goto allfail;
            }
            sum += num;
        }
        ret += ProString(QString::number(sum));
        break;
    }
    case E_JOIN: {
        ProString glue, before, after;
        if (args.count() >= 2)
            glue = args.at(1);
        if (args.count() >= 3)
            before = args[2];
        if (args.count() == 4)
            after = args[3];
        const ProStringList &var = values(map(args.at(0)));
        if (!var.isEmpty()) {
            int src = currentFileId();
            for (const ProString &v : var)
                if (int s = v.sourceFile()) {
                    src = s;
                    break;
                }
            ret << ProString(before + var.join(glue) + after).setSource(src);
        }
        break;
    }
    case E_SPLIT: {
        ProStringRoUser u1(m_tmp1);
        const QString &sep = (args.count() == 2) ? u1.set(args.at(1)) : statics.field_sep;
        const auto vars = values(map(args.at(0)));
        for (const ProString &var : vars) {
            // FIXME: this is inconsistent with the "there are no empty strings" dogma.
            const auto splits = var.toQStringRef().split(sep, QString::KeepEmptyParts);
            for (const auto &splt : splits)
                ret << ProString(splt).setSource(var);
        }
        break;
    }
    case E_MEMBER: {
        const ProStringList &src = values(map(args.at(0)));
        int start, end;
        if (getMemberArgs(func, src.size(), args, &start, &end)) {
            ret.reserve(qAbs(end - start) + 1);
            if (start < end) {
                for (int i = start; i <= end && src.size() >= i; i++)
                    ret += src.at(i);
            } else {
                for (int i = start; i >= end && src.size() >= i && i >= 0; i--)
                    ret += src.at(i);
            }
        }
        break;
    }
    case E_STR_MEMBER: {
        const ProString &src = args.at(0);
        int start, end;
        if (getMemberArgs(func, src.size(), args, &start, &end)) {
            QString res;
            res.reserve(qAbs(end - start) + 1);
            if (start < end) {
                for (int i = start; i <= end && src.size() >= i; i++)
                    res += src.at(i);
            } else {
                for (int i = start; i >= end && src.size() >= i && i >= 0; i--)
                    res += src.at(i);
            }
            ret += ProString(res);
        }
        break;
    }
    case E_FIRST:
    case E_LAST: {
        const ProStringList &var = values(map(args.at(0)));
        if (!var.isEmpty()) {
            if (func_t == E_FIRST)
                ret.append(var[0]);
            else
                ret.append(var.last());
        }
        break;
    }
    case E_TAKE_FIRST:
    case E_TAKE_LAST: {
        ProStringList &var = valuesRef(map(args.at(0)));
        if (!var.isEmpty()) {
            if (func_t == E_TAKE_FIRST)
                ret.append(var.takeFirst());
            else
                ret.append(var.takeLast());
        }
        break;
    }
    case E_SIZE:
        ret.append(ProString(QString::number(values(map(args.at(0))).size())));
        break;
    case E_STR_SIZE:
        ret.append(ProString(QString::number(args.at(0).size())));
        break;
    case E_CAT: {
        bool blob = false;
        bool lines = false;
        bool singleLine = true;
        if (args.count() > 1) {
            if (!args.at(1).compare(QLatin1String("false"), Qt::CaseInsensitive))
                singleLine = false;
            else if (!args.at(1).compare(QLatin1String("blob"), Qt::CaseInsensitive))
                blob = true;
            else if (!args.at(1).compare(QLatin1String("lines"), Qt::CaseInsensitive))
                lines = true;
        }
        QString fn = filePathEnvArg0(args);
        QFile qfile(fn);
        if (qfile.open(QIODevice::ReadOnly)) {
            QTextStream stream(&qfile);
            if (blob) {
                ret += ProString(stream.readAll());
            } else {
                while (!stream.atEnd()) {
                    if (lines) {
                        ret += ProString(stream.readLine());
                    } else {
                        const QString &line = stream.readLine();
                        ret += split_value_list(QStringRef(&line).trimmed());
                        if (!singleLine)
                            ret += ProString("\n");
                    }
                }
            }
        }
        break;
    }
    case E_FROMFILE: {
        ProValueMap vars;
        QString fn = filePathEnvArg0(args);
        if (evaluateFileInto(fn, &vars, LoadProOnly) == ReturnTrue)
            ret = vars.value(map(args.at(1)));
        break;
    }
    case E_EVAL:
        ret += values(map(args.at(0)));
        break;
    case E_LIST: {
        QString tmp;
        tmp.sprintf(".QMAKE_INTERNAL_TMP_variableName_%d", m_listCount++);
        ret = ProStringList(ProString(tmp));
        ProStringList lst;
        for (const ProString &arg : args)
            lst += split_value_list(arg.toQStringRef(), arg.sourceFile()); // Relies on deep copy
        m_valuemapStack.top()[ret.at(0).toKey()] = lst;
        break; }
    case E_FIND: {
        QRegExp regx(args.at(1).toQString());
        const auto vals = values(map(args.at(0)));
        for (const ProString &val : vals) {
            ProStringRoUser u1(val, m_tmp[m_toggle ^= 1]);
            if (regx.indexIn(u1.str()) != -1)
                ret += val;
        }
        break;
    }
    case E_SYSTEM: {
        if (m_skipLevel)
            break;
        bool blob = false;
        bool lines = false;
        bool singleLine = true;
        if (args.count() > 1) {
            if (!args.at(1).compare(QLatin1String("false"), Qt::CaseInsensitive))
                singleLine = false;
            else if (!args.at(1).compare(QLatin1String("blob"), Qt::CaseInsensitive))
                blob = true;
            else if (!args.at(1).compare(QLatin1String("lines"), Qt::CaseInsensitive))
                lines = true;
        }
        int exitCode;
        QByteArray bytes = getCommandOutput(args.at(0).toQString(), &exitCode);
        if (args.count() > 2 && !args.at(2).isEmpty()) {
            m_valuemapStack.top()[args.at(2).toKey()] =
                    ProStringList(ProString(QString::number(exitCode)));
        }
        if (lines) {
            QTextStream stream(bytes);
            while (!stream.atEnd())
                ret += ProString(stream.readLine());
        } else {
            QString output = QString::fromLocal8Bit(bytes);
            if (blob) {
                ret += ProString(output);
            } else {
                output.replace(QLatin1Char('\t'), QLatin1Char(' '));
                if (singleLine)
                    output.replace(QLatin1Char('\n'), QLatin1Char(' '));
                ret += split_value_list(QStringRef(&output));
            }
        }
        break;
    }
    case E_UNIQUE:
        ret = values(map(args.at(0)));
        ret.removeDuplicates();
        break;
    case E_SORTED:
        ret = values(map(args.at(0)));
        std::sort(ret.begin(), ret.end());
        break;
    case E_REVERSE: {
        ProStringList var = values(args.at(0).toKey());
        for (int i = 0; i < var.size() / 2; i++)
            qSwap(var[i], var[var.size() - i - 1]);
        ret += var;
        break;
    }
    case E_QUOTE:
        ret += args;
        break;
    case E_ESCAPE_EXPAND:
        for (int i = 0; i < args.size(); ++i) {
            QString str = args.at(i).toQString();
            QChar *i_data = str.data();
            int i_len = str.length();
            for (int x = 0; x < i_len; ++x) {
                if (*(i_data+x) == QLatin1Char('\\') && x < i_len-1) {
                    if (*(i_data+x+1) == QLatin1Char('\\')) {
                        ++x;
                    } else {
                        struct {
                            char in, out;
                        } mapped_quotes[] = {
                            { 'n', '\n' },
                            { 't', '\t' },
                            { 'r', '\r' },
                            { 0, 0 }
                        };
                        for (int i = 0; mapped_quotes[i].in; ++i) {
                            if (*(i_data+x+1) == QLatin1Char(mapped_quotes[i].in)) {
                                *(i_data+x) = QLatin1Char(mapped_quotes[i].out);
                                if (x < i_len-2)
                                    memmove(i_data+x+1, i_data+x+2, (i_len-x-2)*sizeof(QChar));
                                --i_len;
                                break;
                            }
                        }
                    }
                }
            }
            ret.append(ProString(QString(i_data, i_len)).setSource(args.at(i)));
        }
        break;
    case E_RE_ESCAPE:
        for (int i = 0; i < args.size(); ++i) {
            ProStringRwUser u1(args.at(i), m_tmp1);
            ret << u1.extract(QRegExp::escape(u1.str()));
        }
        break;
    case E_VAL_ESCAPE: {
        const ProStringList &vals = values(args.at(0).toKey());
        ret.reserve(vals.size());
        for (const ProString &str : vals)
            ret += ProString(quoteValue(str));
        break;
    }
    case E_UPPER:
    case E_LOWER:
    case E_TITLE:
        for (int i = 0; i < args.count(); ++i) {
            ProStringRwUser u1(args.at(i), m_tmp1);
            QString rstr = u1.str();
            if (func_t == E_UPPER) {
                rstr = rstr.toUpper();
            } else {
                rstr = rstr.toLower();
                if (func_t == E_TITLE && rstr.length() > 0)
                    rstr[0] = rstr.at(0).toTitleCase();
            }
            ret << u1.extract(rstr);
        }
        break;
    case E_FILES: {
        bool recursive = false;
        if (args.count() == 2)
            recursive = isTrue(args.at(1));
        QStringList dirs;
        ProStringRoUser u1(args.at(0), m_tmp1);
        QString r = m_option->expandEnvVars(u1.str())
                    .replace(QLatin1Char('\\'), QLatin1Char('/'));
        QString pfx;
        if (IoUtils::isRelativePath(r)) {
            pfx = currentDirectory();
            if (!pfx.endsWith(QLatin1Char('/')))
                pfx += QLatin1Char('/');
        }
        int slash = r.lastIndexOf(QLatin1Char('/'));
        if (slash != -1) {
            dirs.append(r.left(slash+1));
            r = r.mid(slash+1);
        } else {
            dirs.append(QString());
        }

        r.detach(); // Keep m_tmp out of QRegExp's cache
        QRegExp regex(r, Qt::CaseSensitive, QRegExp::Wildcard);
        for (int d = 0; d < dirs.count(); d++) {
            QString dir = dirs[d];
            QDir qdir(pfx + dir);
            for (int i = 0; i < (int)qdir.count(); ++i) {
                if (qdir[i] == statics.strDot || qdir[i] == statics.strDotDot)
                    continue;
                QString fname = dir + qdir[i];
                if (IoUtils::fileType(pfx + fname) == IoUtils::FileIsDir) {
                    if (recursive)
                        dirs.append(fname + QLatin1Char('/'));
                }
                if (regex.exactMatch(qdir[i]))
                      ret += ProString(fname).setSource(currentFileId());
            }
        }
        break;
    }
#ifdef PROEVALUATOR_FULL
    case E_PROMPT: {
        ProStringRoUser u1(args.at(0), m_tmp1);
        QString msg = m_option->expandEnvVars(u1.str());
        bool decorate = true;
        if (args.count() == 2)
            decorate = isTrue(args.at(1));
        if (decorate) {
            if (!msg.endsWith(QLatin1Char('?')))
                msg += QLatin1Char('?');
            fprintf(stderr, "Project PROMPT: %s ", qPrintable(msg));
        } else {
            fputs(qPrintable(msg), stderr);
        }
        QFile qfile;
        if (qfile.open(stdin, QIODevice::ReadOnly)) {
            QTextStream t(&qfile);
            const QString &line = t.readLine();
            if (t.atEnd()) {
                fputs("\n", stderr);
                evalError(fL1S("Unexpected EOF."));
                return ReturnError;
            }
            ret = split_value_list(QStringRef(&line));
        }
        break;
    }
#endif
    case E_REPLACE: {
        const QRegExp before(args.at(1).toQString());
        ProStringRwUser u2(args.at(2), m_tmp2);
        const QString &after = u2.str();
        const auto vals = values(map(args.at(0)));
        for (const ProString &val : vals) {
            ProStringRwUser u1(val, m_tmp1);
            QString rstr = u1.str();
            QString copy = rstr; // Force a detach on modify
            rstr.replace(before, after);
            ret << u1.extract(rstr, u2);
        }
        break;
    }
    case E_SORT_DEPENDS:
    case E_RESOLVE_DEPENDS: {
        QHash<ProKey, QSet<ProKey> > dependencies;
        ProValueMap dependees;
        QMultiMap<int, ProString> rootSet;
        ProStringList orgList = values(args.at(0).toKey());
        ProString prefix = args.count() < 2 ? ProString() : args.at(1);
        ProString priosfx = args.count() < 4 ? ProString(".priority") : args.at(3);
        populateDeps(orgList, prefix,
                     args.count() < 3 ? ProStringList(ProString(".depends"))
                                      : split_value_list(args.at(2).toQStringRef()),
                     priosfx, dependencies, dependees, rootSet);
        while (!rootSet.isEmpty()) {
            QMultiMap<int, ProString>::iterator it = rootSet.begin();
            const ProString item = *it;
            rootSet.erase(it);
            if ((func_t == E_RESOLVE_DEPENDS) || orgList.contains(item))
                ret.prepend(item);
            for (const ProString &dep : qAsConst(dependees[item.toKey()])) {
                QSet<ProKey> &dset = dependencies[dep.toKey()];
                dset.remove(item.toKey());
                if (dset.isEmpty())
                    rootSet.insert(first(ProKey(prefix + dep + priosfx)).toInt(), dep);
            }
        }
        break;
    }
    case E_ENUMERATE_VARS: {
        QSet<ProString> keys;
        for (const ProValueMap &vmap : qAsConst(m_valuemapStack))
            for (ProValueMap::ConstIterator it = vmap.constBegin(); it != vmap.constEnd(); ++it)
                keys.insert(it.key());
        ret.reserve(keys.size());
        for (const ProString &key : qAsConst(keys))
            ret << key;
        break; }
    case E_SHADOWED: {
        ProStringRwUser u1(args.at(0), m_tmp1);
        QString rstr = m_option->shadowedPath(resolvePath(u1.str()));
        if (!rstr.isEmpty())
            ret << u1.extract(rstr);
        break;
    }
    case E_ABSOLUTE_PATH: {
        ProStringRwUser u1(args.at(0), m_tmp1);
        ProStringRwUser u2(m_tmp2);
        QString baseDir = args.count() > 1
                ? IoUtils::resolvePath(currentDirectory(), u2.set(args.at(1)))
                : currentDirectory();
        QString rstr = u1.str().isEmpty() ? baseDir : IoUtils::resolvePath(baseDir, u1.str());
        ret << u1.extract(rstr, u2);
        break;
    }
    case E_RELATIVE_PATH: {
        ProStringRwUser u1(args.at(0), m_tmp1);
        ProStringRoUser u2(m_tmp2);
        QString baseDir = args.count() > 1
                ? IoUtils::resolvePath(currentDirectory(), u2.set(args.at(1)))
                : currentDirectory();
        QString absArg = u1.str().isEmpty() ? baseDir : IoUtils::resolvePath(baseDir, u1.str());
        QString rstr = QDir(baseDir).relativeFilePath(absArg);
        ret << u1.extract(rstr);
        break;
    }
    case E_CLEAN_PATH: {
        ProStringRwUser u1(args.at(0), m_tmp1);
        ret << u1.extract(QDir::cleanPath(u1.str()));
        break;
    }
    case E_SYSTEM_PATH: {
        ProStringRwUser u1(args.at(0), m_tmp1);
        QString rstr = u1.str();
#ifdef Q_OS_WIN
        rstr.replace(QLatin1Char('/'), QLatin1Char('\\'));
#else
        rstr.replace(QLatin1Char('\\'), QLatin1Char('/'));
#endif
        ret << u1.extract(rstr);
        break;
    }
    case E_SHELL_PATH: {
        ProStringRwUser u1(args.at(0), m_tmp1);
        QString rstr = u1.str();
        if (m_dirSep.startsWith(QLatin1Char('\\'))) {
            rstr.replace(QLatin1Char('/'), QLatin1Char('\\'));
        } else {
            rstr.replace(QLatin1Char('\\'), QLatin1Char('/'));
#ifdef Q_OS_WIN
            // Convert d:/foo/bar to msys-style /d/foo/bar.
            if (rstr.length() > 2 && rstr.at(1) == QLatin1Char(':') && rstr.at(2) == QLatin1Char('/')) {
                rstr[1] = rstr.at(0);
                rstr[0] = QLatin1Char('/');
            }
#endif
        }
        ret << u1.extract(rstr);
        break;
    }
    case E_SYSTEM_QUOTE: {
        ProStringRwUser u1(args.at(0), m_tmp1);
        ret << u1.extract(IoUtils::shellQuote(u1.str()));
        break;
    }
    case E_SHELL_QUOTE: {
        ProStringRwUser u1(args.at(0), m_tmp1);
        QString rstr = u1.str();
        if (m_dirSep.startsWith(QLatin1Char('\\')))
            rstr = IoUtils::shellQuoteWin(rstr);
        else
            rstr = IoUtils::shellQuoteUnix(rstr);
        ret << u1.extract(rstr);
        break;
    }
    case E_GETENV: {
        ProStringRoUser u1(args.at(0), m_tmp1);
        ret << ProString(m_option->getEnv(u1.str()));
        break;
    }
    default:
        evalError(fL1S("Function '%1' is not implemented.").arg(func.toQStringView()));
        break;
    }

  allfail:
    return ReturnTrue;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::testFunc_cache(const ProStringList &args)
{
    bool persist = true;
    enum { TargetStash, TargetCache, TargetSuper } target = TargetCache;
    enum { CacheSet, CacheAdd, CacheSub } mode = CacheSet;
    ProKey srcvar;
    if (args.count() >= 2) {
        const auto opts = split_value_list(args.at(1).toQStringRef());
        for (const ProString &opt : opts) {
            if (opt == QLatin1String("transient")) {
                persist = false;
            } else if (opt == QLatin1String("super")) {
                target = TargetSuper;
            } else if (opt == QLatin1String("stash")) {
                target = TargetStash;
            } else if (opt == QLatin1String("set")) {
                mode = CacheSet;
            } else if (opt == QLatin1String("add")) {
                mode = CacheAdd;
            } else if (opt == QLatin1String("sub")) {
                mode = CacheSub;
            } else {
                evalError(fL1S("cache(): invalid flag %1.").arg(opt.toQStringView()));
                return ReturnFalse;
            }
        }
        if (args.count() >= 3) {
            srcvar = args.at(2).toKey();
        } else if (mode != CacheSet) {
            evalError(fL1S("cache(): modes other than 'set' require a source variable."));
            return ReturnFalse;
        }
    }
    QString varstr;
    ProKey dstvar = args.at(0).toKey();
    if (!dstvar.isEmpty()) {
        if (srcvar.isEmpty())
            srcvar = dstvar;
        ProValueMap::Iterator srcvarIt;
        if (!findValues(srcvar, &srcvarIt)) {
            evalError(fL1S("Variable %1 is not defined.").arg(srcvar.toQStringView()));
            return ReturnFalse;
        }
        // The caches for the host and target may differ (e.g., when we are manipulating
        // CONFIG), so we cannot compute a common new value for both.
        const ProStringList &diffval = *srcvarIt;
        ProStringList newval;
        bool changed = false;
        for (bool hostBuild = false; ; hostBuild = true) {
#ifdef PROEVALUATOR_THREAD_SAFE
            m_option->mutex.lock();
#endif
            QMakeBaseEnv *baseEnv =
                m_option->baseEnvs.value(QMakeBaseKey(m_buildRoot, m_stashfile, hostBuild));
#ifdef PROEVALUATOR_THREAD_SAFE
            // It's ok to unlock this before locking baseEnv,
            // as we have no intention to initialize the env.
            m_option->mutex.unlock();
#endif
            do {
                if (!baseEnv)
                    break;
#ifdef PROEVALUATOR_THREAD_SAFE
                QMutexLocker locker(&baseEnv->mutex);
                if (baseEnv->inProgress && baseEnv->evaluator != this) {
                    // The env is still in the works, but it may be already past the cache
                    // loading. So we need to wait for completion and amend it as usual.
                    QThreadPool::globalInstance()->releaseThread();
                    baseEnv->cond.wait(&baseEnv->mutex);
                    QThreadPool::globalInstance()->reserveThread();
                }
                if (!baseEnv->isOk)
                    break;
#endif
                QMakeEvaluator *baseEval = baseEnv->evaluator;
                const ProStringList &oldval = baseEval->values(dstvar);
                if (mode == CacheSet) {
                    newval = diffval;
                } else {
                    newval = oldval;
                    if (mode == CacheAdd)
                        newval += diffval;
                    else
                        newval.removeEach(diffval);
                }
                if (oldval != newval) {
                    if (target != TargetStash || !m_stashfile.isEmpty()) {
                        baseEval->valuesRef(dstvar) = newval;
                        if (target == TargetSuper) {
                            do {
                                if (dstvar == QLatin1String("QMAKEPATH")) {
                                    baseEval->m_qmakepath = newval.toQStringList();
                                    baseEval->updateMkspecPaths();
                                } else if (dstvar == QLatin1String("QMAKEFEATURES")) {
                                    baseEval->m_qmakefeatures = newval.toQStringList();
                                } else {
                                    break;
                                }
                                baseEval->updateFeaturePaths();
                                if (hostBuild == m_hostBuild)
                                    m_featureRoots = baseEval->m_featureRoots;
                            } while (false);
                        }
                    }
                    changed = true;
                }
            } while (false);
            if (hostBuild)
                break;
        }
        // We assume that whatever got the cached value to be what it is now will do so
        // the next time as well, so we just skip the persisting if nothing changed.
        if (!persist || !changed)
            return ReturnTrue;
        varstr = dstvar.toQString();
        if (mode == CacheAdd)
            varstr += QLatin1String(" +=");
        else if (mode == CacheSub)
            varstr += QLatin1String(" -=");
        else
            varstr += QLatin1String(" =");
        if (diffval.count() == 1) {
            varstr += QLatin1Char(' ');
            varstr += quoteValue(diffval.at(0));
        } else if (!diffval.isEmpty()) {
            for (const ProString &vval : diffval) {
                varstr += QLatin1String(" \\\n    ");
                varstr += quoteValue(vval);
            }
        }
        varstr += QLatin1Char('\n');
    }
    QString fn;
    QMakeVfs::VfsFlags flags = (m_cumulative ? QMakeVfs::VfsCumulative : QMakeVfs::VfsExact);
    if (target == TargetSuper) {
        if (m_superfile.isEmpty()) {
            m_superfile = QDir::cleanPath(m_outputDir + QLatin1String("/.qmake.super"));
            printf("Info: creating super cache file %s\n", qPrintable(QDir::toNativeSeparators(m_superfile)));
            valuesRef(ProKey("_QMAKE_SUPER_CACHE_")) << ProString(m_superfile);
        }
        fn = m_superfile;
    } else if (target == TargetCache) {
        if (m_cachefile.isEmpty()) {
            m_cachefile = QDir::cleanPath(m_outputDir + QLatin1String("/.qmake.cache"));
            printf("Info: creating cache file %s\n", qPrintable(QDir::toNativeSeparators(m_cachefile)));
            valuesRef(ProKey("_QMAKE_CACHE_")) << ProString(m_cachefile);
            // We could update m_{source,build}Root and m_featureRoots here, or even
            // "re-home" our rootEnv, but this doesn't sound too useful - if somebody
            // wanted qmake to find something in the build directory, he could have
            // done so "from the outside".
            // The sub-projects will find the new cache all by themselves.
        }
        fn = m_cachefile;
    } else {
        fn = m_stashfile;
        if (fn.isEmpty())
            fn = QDir::cleanPath(m_outputDir + QLatin1String("/.qmake.stash"));
        if (!m_vfs->exists(fn, flags)) {
            printf("Info: creating stash file %s\n", qPrintable(QDir::toNativeSeparators(fn)));
            valuesRef(ProKey("_QMAKE_STASH_")) << ProString(fn);
        }
    }
    return writeFile(fL1S("cache "), fn, QIODevice::Append, flags, varstr);
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateBuiltinConditional(
        const QMakeInternal::QMakeBuiltin &adef, const ProKey &function, const ProStringList &args)
{
    traceMsg("calling built-in %s(%s)", dbgKey(function), dbgSepStrList(args));
    int asz = args.size() > 1 ? args.size() : args.at(0).isEmpty() ? 0 : 1;
    if (asz < adef.minArgs || asz > adef.maxArgs) {
        evalError(adef.usage);
        return ReturnFalse;
    }

    int func_t = adef.index;
    switch (func_t) {
    case T_DEFINED: {
        const ProKey &var = args.at(0).toKey();
        if (args.count() > 1) {
            if (args[1] == QLatin1String("test")) {
                return returnBool(m_functionDefs.testFunctions.contains(var));
            } else if (args[1] == QLatin1String("replace")) {
                return returnBool(m_functionDefs.replaceFunctions.contains(var));
            } else if (args[1] == QLatin1String("var")) {
                ProValueMap::Iterator it;
                return returnBool(findValues(var, &it));
            }
            evalError(fL1S("defined(function, type): unexpected type [%1].")
                      .arg(args.at(1).toQStringView()));
            return ReturnFalse;
        }
        return returnBool(m_functionDefs.replaceFunctions.contains(var)
                          || m_functionDefs.testFunctions.contains(var));
    }
    case T_EXPORT: {
        const ProKey &var = map(args.at(0));
        for (ProValueMapStack::Iterator vmi = m_valuemapStack.end();
             --vmi != m_valuemapStack.begin(); ) {
            ProValueMap::Iterator it = (*vmi).find(var);
            if (it != (*vmi).end()) {
                if (it->constBegin() == statics.fakeValue.constBegin()) {
                    // This is stupid, but qmake doesn't propagate deletions
                    m_valuemapStack.first()[var] = ProStringList();
                } else {
                    m_valuemapStack.first()[var] = *it;
                }
                (*vmi).erase(it);
                while (--vmi != m_valuemapStack.begin())
                    (*vmi).remove(var);
                break;
            }
        }
        return ReturnTrue;
    }
    case T_DISCARD_FROM: {
        if (m_valuemapStack.count() != 1) {
            evalError(fL1S("discard_from() cannot be called from functions."));
            return ReturnFalse;
        }
        ProStringRoUser u1(args.at(0), m_tmp1);
        QString fn = resolvePath(u1.str());
        QMakeVfs::VfsFlags flags = (m_cumulative ? QMakeVfs::VfsCumulative : QMakeVfs::VfsExact);
        int pro = m_vfs->idForFileName(fn, flags | QMakeVfs::VfsAccessedOnly);
        if (!pro)
            return ReturnFalse;
        ProValueMap &vmap = m_valuemapStack.first();
        for (auto vit = vmap.begin(); vit != vmap.end(); ) {
            if (!vit->isEmpty()) {
                auto isFrom = [pro](const ProString &s) {
                    return s.sourceFile() == pro;
                };
                vit->erase(std::remove_if(vit->begin(), vit->end(), isFrom), vit->end());
                if (vit->isEmpty()) {
                    // When an initially non-empty variable becomes entirely empty,
                    // undefine it altogether.
                    vit = vmap.erase(vit);
                    continue;
                }
            }
            ++vit;
        }
        for (auto fit = m_functionDefs.testFunctions.begin(); fit != m_functionDefs.testFunctions.end(); ) {
            if (fit->pro()->id() == pro)
                fit = m_functionDefs.testFunctions.erase(fit);
            else
                ++fit;
        }
        for (auto fit = m_functionDefs.replaceFunctions.begin(); fit != m_functionDefs.replaceFunctions.end(); ) {
            if (fit->pro()->id() == pro)
                fit = m_functionDefs.replaceFunctions.erase(fit);
            else
                ++fit;
        }
        ProStringList &iif = m_valuemapStack.first()[ProKey("QMAKE_INTERNAL_INCLUDED_FILES")];
        int idx = iif.indexOf(ProString(fn));
        if (idx >= 0)
            iif.removeAt(idx);
        return ReturnTrue;
    }
    case T_INFILE: {
        ProValueMap vars;
        QString fn = filePathEnvArg0(args);
        VisitReturn ok = evaluateFileInto(fn, &vars, LoadProOnly);
        if (ok != ReturnTrue)
            return ok;
        if (args.count() == 2)
            return returnBool(vars.contains(map(args.at(1))));
        QRegExp regx;
        ProStringRoUser u1(args.at(2), m_tmp1);
        const QString &qry = u1.str();
        if (qry != QRegExp::escape(qry)) {
            QString copy = qry;
            copy.detach();
            regx.setPattern(copy);
        }
        const auto strings = vars.value(map(args.at(1)));
        for (const ProString &s : strings) {
            if (s == qry)
                return ReturnTrue;
            if (!regx.isEmpty()) {
                ProStringRoUser u2(s, m_tmp[m_toggle ^= 1]);
                if (regx.exactMatch(u2.str()))
                    return ReturnTrue;
            }
        }
        return ReturnFalse;
    }
    case T_REQUIRES:
#ifdef PROEVALUATOR_FULL
        if (checkRequirements(args) == ReturnError)
            return ReturnError;
#endif
        return ReturnFalse; // Another qmake breakage
    case T_EVAL: {
        VisitReturn ret = ReturnFalse;
        QString contents = args.join(statics.field_sep);
        ProFile *pro = m_parser->parsedProBlock(QStringRef(&contents),
                                                0, m_current.pro->fileName(), m_current.line);
        if (m_cumulative || pro->isOk()) {
            m_locationStack.push(m_current);
            visitProBlock(pro, pro->tokPtr());
            ret = ReturnTrue; // This return value is not too useful, but that's qmake
            m_current = m_locationStack.pop();
        }
        pro->deref();
        return ret;
    }
    case T_IF: {
        return evaluateConditional(args.at(0).toQStringRef(),
                                   m_current.pro->fileName(), m_current.line);
    }
    case T_CONFIG: {
        if (args.count() == 1)
            return returnBool(isActiveConfig(args.at(0).toQStringRef()));
        const auto mutuals = args.at(1).toQStringRef().split(QLatin1Char('|'),
                                                             QString::SkipEmptyParts);
        const ProStringList &configs = values(statics.strCONFIG);

        for (int i = configs.size() - 1; i >= 0; i--) {
            for (int mut = 0; mut < mutuals.count(); mut++) {
                if (configs[i].toQStringRef() == mutuals[mut].trimmed())
                    return returnBool(configs[i] == args[0]);
            }
        }
        return ReturnFalse;
    }
    case T_CONTAINS: {
        ProStringRoUser u1(args.at(1), m_tmp1);
        const QString &qry = u1.str();
        QRegExp regx;
        if (qry != QRegExp::escape(qry)) {
            QString copy = qry;
            copy.detach();
            regx.setPattern(copy);
        }
        const ProStringList &l = values(map(args.at(0)));
        if (args.count() == 2) {
            for (int i = 0; i < l.size(); ++i) {
                const ProString &val = l[i];
                if (val == qry)
                    return ReturnTrue;
                if (!regx.isEmpty()) {
                    ProStringRoUser u2(val, m_tmp[m_toggle ^= 1]);
                    if (regx.exactMatch(u2.str()))
                        return ReturnTrue;
                }
            }
        } else {
            const auto mutuals = args.at(2).toQStringRef().split(QLatin1Char('|'),
                                                                 QString::SkipEmptyParts);
            for (int i = l.size() - 1; i >= 0; i--) {
                const ProString &val = l[i];
                for (int mut = 0; mut < mutuals.count(); mut++) {
                    if (val.toQStringRef() == mutuals[mut].trimmed()) {
                        if (val == qry)
                            return ReturnTrue;
                        if (!regx.isEmpty()) {
                            ProStringRoUser u2(val, m_tmp[m_toggle ^= 1]);
                            if (regx.exactMatch(u2.str()))
                                return ReturnTrue;
                        }
                        return ReturnFalse;
                    }
                }
            }
        }
        return ReturnFalse;
    }
    case T_COUNT: {
        int cnt = values(map(args.at(0))).count();
        int val = args.at(1).toInt();
        if (args.count() == 3) {
            const ProString &comp = args.at(2);
            if (comp == QLatin1String(">") || comp == QLatin1String("greaterThan")) {
                return returnBool(cnt > val);
            } else if (comp == QLatin1String(">=")) {
                return returnBool(cnt >= val);
            } else if (comp == QLatin1String("<") || comp == QLatin1String("lessThan")) {
                return returnBool(cnt < val);
            } else if (comp == QLatin1String("<=")) {
                return returnBool(cnt <= val);
            } else if (comp == QLatin1String("equals") || comp == QLatin1String("isEqual")
                       || comp == QLatin1String("=") || comp == QLatin1String("==")) {
                // fallthrough
            } else {
                evalError(fL1S("Unexpected modifier to count(%2).").arg(comp.toQStringView()));
                return ReturnFalse;
            }
        }
        return returnBool(cnt == val);
    }
    case T_GREATERTHAN:
    case T_LESSTHAN: {
        const ProString &rhs = args.at(1);
        const QString &lhs = values(map(args.at(0))).join(statics.field_sep);
        bool ok;
        int rhs_int = rhs.toInt(&ok);
        if (ok) { // do integer compare
            int lhs_int = lhs.toInt(&ok);
            if (ok) {
                if (func_t == T_GREATERTHAN)
                    return returnBool(lhs_int > rhs_int);
                return returnBool(lhs_int < rhs_int);
            }
        }
        if (func_t == T_GREATERTHAN)
            return returnBool(lhs > rhs.toQStringRef());
        return returnBool(lhs < rhs.toQStringRef());
    }
    case T_EQUALS:
        return returnBool(values(map(args.at(0))).join(statics.field_sep)
                          == args.at(1).toQStringView());
    case T_VERSION_AT_LEAST:
    case T_VERSION_AT_MOST: {
        const QVersionNumber lvn = QVersionNumber::fromString(values(args.at(0).toKey()).join('.'));
        const QVersionNumber rvn = QVersionNumber::fromString(args.at(1).toQStringView());
        if (func_t == T_VERSION_AT_LEAST)
            return returnBool(lvn >= rvn);
        return returnBool(lvn <= rvn);
    }
    case T_CLEAR: {
        ProValueMap *hsh;
        ProValueMap::Iterator it;
        const ProKey &var = map(args.at(0));
        if (!(hsh = findValues(var, &it)))
            return ReturnFalse;
        if (hsh == &m_valuemapStack.top())
            it->clear();
        else
            m_valuemapStack.top()[var].clear();
        return ReturnTrue;
    }
    case T_UNSET: {
        ProValueMap *hsh;
        ProValueMap::Iterator it;
        const ProKey &var = map(args.at(0));
        if (!(hsh = findValues(var, &it)))
            return ReturnFalse;
        if (m_valuemapStack.size() == 1)
            hsh->erase(it);
        else if (hsh == &m_valuemapStack.top())
            *it = statics.fakeValue;
        else
            m_valuemapStack.top()[var] = statics.fakeValue;
        return ReturnTrue;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    case T_PARSE_JSON: {
        QByteArray json = values(args.at(0).toKey()).join(QLatin1Char(' ')).toUtf8();
        ProStringRoUser u1(args.at(1), m_tmp2);
        QString parseInto = u1.str();
        return parseJsonInto(json, parseInto, &m_valuemapStack.top());
    }
#endif
    case T_INCLUDE: {
        QString parseInto;
        LoadFlags flags;
        if (m_cumulative)
            flags = LoadSilent;
        if (args.count() >= 2) {
            if (!args.at(1).isEmpty())
                parseInto = args.at(1) + QLatin1Char('.');
            if (args.count() >= 3 && isTrue(args.at(2)))
                flags = LoadSilent;
        }
        QString fn = filePathEnvArg0(args);
        VisitReturn ok;
        if (parseInto.isEmpty()) {
            ok = evaluateFileChecked(fn, QMakeHandler::EvalIncludeFile, LoadProOnly | flags);
        } else {
            ProValueMap symbols;
            if ((ok = evaluateFileInto(fn, &symbols, LoadAll | flags)) == ReturnTrue) {
                ProValueMap newMap;
                for (ProValueMap::ConstIterator
                        it = m_valuemapStack.top().constBegin(),
                        end = m_valuemapStack.top().constEnd();
                        it != end; ++it) {
                    const ProString &ky = it.key();
                    if (!ky.startsWith(parseInto))
                        newMap[it.key()] = it.value();
                }
                for (ProValueMap::ConstIterator it = symbols.constBegin();
                     it != symbols.constEnd(); ++it) {
                    if (!it.key().startsWith(QLatin1Char('.')))
                        newMap.insert(ProKey(parseInto + it.key()), it.value());
                }
                m_valuemapStack.top() = newMap;
            }
        }
        if (ok == ReturnFalse && (flags & LoadSilent))
            ok = ReturnTrue;
        return ok;
    }
    case T_LOAD: {
        bool ignore_error = (args.count() == 2 && isTrue(args.at(1)));
        VisitReturn ok = evaluateFeatureFile(m_option->expandEnvVars(args.at(0).toQString()),
                                             ignore_error);
        if (ok == ReturnFalse && ignore_error)
            ok = ReturnTrue;
        return ok;
    }
    case T_DEBUG: {
#ifdef PROEVALUATOR_DEBUG
        int level = args.at(0).toInt();
        if (level <= m_debugLevel) {
            ProStringRoUser u1(args.at(1), m_tmp1);
            const QString &msg = m_option->expandEnvVars(u1.str());
            debugMsg(level, "Project DEBUG: %s", qPrintable(msg));
        }
#endif
        return ReturnTrue;
    }
    case T_LOG:
    case T_ERROR:
    case T_WARNING:
    case T_MESSAGE: {
        ProStringRoUser u1(args.at(0), m_tmp1);
        const QString &msg = m_option->expandEnvVars(u1.str());
        if (!m_skipLevel) {
            if (func_t == T_LOG) {
#ifdef PROEVALUATOR_FULL
                fputs(msg.toLatin1().constData(), stderr);
#endif
            } else if (!msg.isEmpty() || func_t != T_ERROR) {
                ProStringRoUser u2(function, m_tmp2);
                m_handler->fileMessage(
                        (func_t == T_ERROR   ? QMakeHandler::ErrorMessage :
                         func_t == T_WARNING ? QMakeHandler::WarningMessage :
                                               QMakeHandler::InfoMessage)
                        | (m_cumulative ? QMakeHandler::CumulativeEvalMessage : 0),
                        fL1S("Project %1: %2").arg(u2.str().toUpper(), msg));
            }
        }
        return (func_t == T_ERROR && !m_cumulative) ? ReturnError : ReturnTrue;
    }
    case T_SYSTEM: {
#ifdef PROEVALUATOR_FULL
        if (m_cumulative) // Anything else would be insanity
            return ReturnFalse;
#if QT_CONFIG(process)
        QProcess proc;
        proc.setProcessChannelMode(QProcess::ForwardedChannels);
        runProcess(&proc, args.at(0).toQString());
        return returnBool(proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0);
#else
        int ec = system((QLatin1String("cd ")
                         + IoUtils::shellQuote(QDir::toNativeSeparators(currentDirectory()))
                         + QLatin1String(" && ") + args.at(0)).toLocal8Bit().constData());
#  ifdef Q_OS_UNIX
        if (ec != -1 && WIFSIGNALED(ec) && (WTERMSIG(ec) == SIGQUIT || WTERMSIG(ec) == SIGINT))
            raise(WTERMSIG(ec));
#  endif
        return returnBool(ec == 0);
#endif
#else
        return ReturnTrue;
#endif
    }
    case T_ISEMPTY: {
        return returnBool(values(map(args.at(0))).isEmpty());
    }
    case T_EXISTS: {
        QString file = filePathEnvArg0(args);
        // Don't use VFS here:
        // - it supports neither listing nor even directories
        // - it's unlikely that somebody would test for files they created themselves
        if (IoUtils::exists(file))
            return ReturnTrue;
        int slsh = file.lastIndexOf(QLatin1Char('/'));
        QString fn = file.mid(slsh+1);
        if (fn.contains(QLatin1Char('*')) || fn.contains(QLatin1Char('?'))) {
            QString dirstr = file.left(slsh+1);
            dirstr.detach();
            if (!QDir(dirstr).entryList(QStringList(fn)).isEmpty())
                return ReturnTrue;
        }

        return ReturnFalse;
    }
    case T_MKPATH: {
#ifdef PROEVALUATOR_FULL
        QString fn = filePathArg0(args);
        if (!QDir::current().mkpath(fn)) {
            evalError(fL1S("Cannot create directory %1.").arg(QDir::toNativeSeparators(fn)));
            return ReturnFalse;
        }
#endif
        return ReturnTrue;
    }
    case T_WRITE_FILE: {
        QIODevice::OpenMode mode = QIODevice::Truncate;
        QMakeVfs::VfsFlags flags = (m_cumulative ? QMakeVfs::VfsCumulative : QMakeVfs::VfsExact);
        QString contents;
        if (args.count() >= 2) {
            const ProStringList &vals = values(args.at(1).toKey());
            if (!vals.isEmpty())
                contents = vals.join(QLatin1Char('\n')) + QLatin1Char('\n');
            if (args.count() >= 3) {
                const auto opts = split_value_list(args.at(2).toQStringRef());
                for (const ProString &opt : opts) {
                    if (opt == QLatin1String("append")) {
                        mode = QIODevice::Append;
                    } else if (opt == QLatin1String("exe")) {
                        flags |= QMakeVfs::VfsExecutable;
                    } else {
                        evalError(fL1S("write_file(): invalid flag %1.").arg(opt.toQStringView()));
                        return ReturnFalse;
                    }
                }
            }
        }
        QString path = filePathArg0(args);
        return writeFile(QString(), path, mode, flags, contents);
    }
    case T_TOUCH: {
#ifdef PROEVALUATOR_FULL
        ProStringRoUser u1(args.at(0), m_tmp1);
        ProStringRoUser u2(args.at(1), m_tmp2);
        const QString &tfn = resolvePath(u1.str());
        const QString &rfn = resolvePath(u2.str());
        QString error;
        if (!IoUtils::touchFile(tfn, rfn, &error)) {
            evalError(error);
            return ReturnFalse;
        }
#endif
        return ReturnTrue;
    }
    case T_CACHE:
        return testFunc_cache(args);
    case T_RELOAD_PROPERTIES:
#ifdef QT_BUILD_QMAKE
        m_option->reloadProperties();
#endif
        return ReturnTrue;
    default:
        evalError(fL1S("Function '%1' is not implemented.").arg(function.toQStringView()));
        return ReturnFalse;
    }
}

QT_END_NAMESPACE
