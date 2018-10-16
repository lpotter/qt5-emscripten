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

#include "option.h"
#include "cachekeys.h"
#include <ioutils.h>
#include <qdir.h>
#include <qregexp.h>
#include <qhash.h>
#include <qdebug.h>
#include <qlibraryinfo.h>
#include <stdlib.h>
#include <stdarg.h>

QT_BEGIN_NAMESPACE

using namespace QMakeInternal;

EvalHandler Option::evalHandler;
QMakeGlobals *Option::globals;
ProFileCache *Option::proFileCache;
QMakeVfs *Option::vfs;
QMakeParser *Option::parser;

//convenience
QString Option::prf_ext;
QString Option::prl_ext;
QString Option::libtool_ext;
QString Option::pkgcfg_ext;
QString Option::ui_ext;
QStringList Option::h_ext;
QString Option::cpp_moc_ext;
QStringList Option::cpp_ext;
QStringList Option::c_ext;
QString Option::objc_ext;
QString Option::objcpp_ext;
QString Option::obj_ext;
QString Option::lex_ext;
QString Option::yacc_ext;
QString Option::pro_ext;
QString Option::dir_sep;
QString Option::h_moc_mod;
QString Option::yacc_mod;
QString Option::lex_mod;
QString Option::res_ext;
char Option::field_sep;

//mode
Option::QMAKE_MODE Option::qmake_mode = Option::QMAKE_GENERATE_NOTHING;

//all modes
int Option::warn_level = WarnLogic | WarnDeprecated;
int Option::debug_level = 0;
QFile Option::output;
QString Option::output_dir;
bool Option::recursive = false;

//QMAKE_*_PROPERTY stuff
QStringList Option::prop::properties;

//QMAKE_GENERATE_PROJECT stuff
bool Option::projfile::do_pwd = true;
QStringList Option::projfile::project_dirs;

//QMAKE_GENERATE_MAKEFILE stuff
int Option::mkfile::cachefile_depth = -1;
bool Option::mkfile::do_deps = true;
bool Option::mkfile::do_mocs = true;
bool Option::mkfile::do_dep_heuristics = true;
bool Option::mkfile::do_preprocess = false;
bool Option::mkfile::do_stub_makefile = false;
QStringList Option::mkfile::project_files;

static Option::QMAKE_MODE default_mode(QString progname)
{
    int s = progname.lastIndexOf(QDir::separator());
    if(s != -1)
        progname = progname.right(progname.length() - (s + 1));
    if(progname == "qmakegen")
        return Option::QMAKE_GENERATE_PROJECT;
    else if(progname == "qt-config")
        return Option::QMAKE_QUERY_PROPERTY;
    return Option::QMAKE_GENERATE_MAKEFILE;
}

static QString detectProjectFile(const QString &path)
{
    QString ret;
    QDir dir(path);
    if(dir.exists(dir.dirName() + Option::pro_ext)) {
        ret = dir.filePath(dir.dirName()) + Option::pro_ext;
    } else { //last try..
        QStringList profiles = dir.entryList(QStringList("*" + Option::pro_ext));
        if(profiles.count() == 1)
            ret = dir.filePath(profiles.at(0));
    }
    return ret;
}

QString project_builtin_regx();
bool usage(const char *a0)
{
    fprintf(stdout, "Usage: %s [mode] [options] [files]\n"
            "\n"
            "QMake has two modes, one mode for generating project files based on\n"
            "some heuristics, and the other for generating makefiles. Normally you\n"
            "shouldn't need to specify a mode, as makefile generation is the default\n"
            "mode for qmake, but you may use this to test qmake on an existing project\n"
            "\n"
            "Mode:\n"
            "  -project       Put qmake into project file generation mode%s\n"
            "                 In this mode qmake interprets files as files to\n"
            "                 be built,\n"
            "                 defaults to %s\n"
            "                 Note: The created .pro file probably will \n"
            "                 need to be edited. For example add the QT variable to \n"
            "                 specify what modules are required.\n"
            "  -makefile      Put qmake into makefile generation mode%s\n"
            "                 In this mode qmake interprets files as project files to\n"
            "                 be processed, if skipped qmake will try to find a project\n"
            "                 file in your current working directory\n"
            "\n"
            "Warnings Options:\n"
            "  -Wnone         Turn off all warnings; specific ones may be re-enabled by\n"
            "                 later -W options\n"
            "  -Wall          Turn on all warnings\n"
            "  -Wparser       Turn on parser warnings\n"
            "  -Wlogic        Turn on logic warnings (on by default)\n"
            "  -Wdeprecated   Turn on deprecation warnings (on by default)\n"
            "\n"
            "Options:\n"
            "   * You can place any variable assignment in options and it will be *\n"
            "   * processed as if it was in [files]. These assignments will be    *\n"
            "   * processed before [files] by default.                            *\n"
            "  -o file        Write output to file\n"
            "  -d             Increase debug level\n"
            "  -t templ       Overrides TEMPLATE as templ\n"
            "  -tp prefix     Overrides TEMPLATE so that prefix is prefixed into the value\n"
            "  -help          This help\n"
            "  -v             Version information\n"
            "  -early         All subsequent variable assignments will be\n"
            "                 parsed right before default_pre.prf\n"
            "  -before        All subsequent variable assignments will be\n"
            "                 parsed right before [files] (the default)\n"
            "  -after         All subsequent variable assignments will be\n"
            "                 parsed after [files]\n"
            "  -late          All subsequent variable assignments will be\n"
            "                 parsed right after default_post.prf\n"
            "  -norecursive   Don't do a recursive search\n"
            "  -recursive     Do a recursive search\n"
            "  -set <prop> <value> Set persistent property\n"
            "  -unset <prop>  Unset persistent property\n"
            "  -query <prop>  Query persistent property. Show all if <prop> is empty.\n"
            "  -qtconf file   Use file instead of looking for qt.conf\n"
            "  -cache file    Use file as cache           [makefile mode only]\n"
            "  -spec spec     Use spec as QMAKESPEC       [makefile mode only]\n"
            "  -nocache       Don't use a cache file      [makefile mode only]\n"
            "  -nodepend      Don't generate dependencies [makefile mode only]\n"
            "  -nomoc         Don't generate moc targets  [makefile mode only]\n"
            "  -nopwd         Don't look for files in pwd [project mode only]\n"
            ,a0,
            default_mode(a0) == Option::QMAKE_GENERATE_PROJECT  ? " (default)" : "", project_builtin_regx().toLatin1().constData(),
            default_mode(a0) == Option::QMAKE_GENERATE_MAKEFILE ? " (default)" : ""
        );
    return false;
}

int
Option::parseCommandLine(QStringList &args, QMakeCmdLineParserState &state)
{
    enum { ArgNone, ArgOutput } argState = ArgNone;
    int x = 0;
    while (x < args.count()) {
        switch (argState) {
        case ArgOutput:
            Option::output.setFileName(args.at(x--));
            args.erase(args.begin() + x, args.begin() + x + 2);
            argState = ArgNone;
            continue;
        default:
            QMakeGlobals::ArgumentReturn cmdRet = globals->addCommandLineArguments(state, args, &x);
            if (cmdRet == QMakeGlobals::ArgumentsOk)
                break;
            if (cmdRet == QMakeGlobals::ArgumentMalformed) {
                fprintf(stderr, "***Option %s requires a parameter\n", qPrintable(args.at(x - 1)));
                return Option::QMAKE_CMDLINE_SHOW_USAGE | Option::QMAKE_CMDLINE_ERROR;
            }
            Q_ASSERT(cmdRet == QMakeGlobals::ArgumentUnknown);
            QString arg = args.at(x);
            if (arg.startsWith(QLatin1Char('-'))) {
                if (arg == "-d") {
                    Option::debug_level++;
                } else if (arg == "-v" || arg == "-version" || arg == "--version") {
                    fprintf(stdout,
                            "QMake version %s\n"
                            "Using Qt version %s in %s\n",
                            QMAKE_VERSION_STR, QT_VERSION_STR,
                            QLibraryInfo::location(QLibraryInfo::LibrariesPath).toLatin1().constData());
#ifdef QMAKE_OPENSOURCE_VERSION
                    fprintf(stdout, "QMake is Open Source software from The Qt Company Ltd and/or its subsidiary(-ies).\n");
#endif
                    return Option::QMAKE_CMDLINE_BAIL;
                } else if (arg == "-h" || arg == "-help" || arg == "--help") {
                    return Option::QMAKE_CMDLINE_SHOW_USAGE;
                } else if (arg == "-Wall") {
                    Option::warn_level |= WarnAll;
                } else if (arg == "-Wparser") {
                    Option::warn_level |= WarnParser;
                } else if (arg == "-Wlogic") {
                    Option::warn_level |= WarnLogic;
                } else if (arg == "-Wdeprecated") {
                    Option::warn_level |= WarnDeprecated;
                } else if (arg == "-Wnone") {
                    Option::warn_level = WarnNone;
                } else if (arg == "-r" || arg == "-recursive") {
                    Option::recursive = true;
                    args.removeAt(x);
                    continue;
                } else if (arg == "-nr" || arg == "-norecursive") {
                    Option::recursive = false;
                    args.removeAt(x);
                    continue;
                } else if (arg == "-o" || arg == "-output") {
                    argState = ArgOutput;
                } else {
                    if (Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
                        Option::qmake_mode == Option::QMAKE_GENERATE_PRL) {
                        if (arg == "-nodepend" || arg == "-nodepends") {
                            Option::mkfile::do_deps = false;
                        } else if (arg == "-nomoc") {
                            Option::mkfile::do_mocs = false;
                        } else if (arg == "-createstub") {
                            Option::mkfile::do_stub_makefile = true;
                        } else if (arg == "-nodependheuristics") {
                            Option::mkfile::do_dep_heuristics = false;
                        } else if (arg == "-E") {
                            Option::mkfile::do_preprocess = true;
                        } else {
                            fprintf(stderr, "***Unknown option %s\n", arg.toLatin1().constData());
                            return Option::QMAKE_CMDLINE_SHOW_USAGE | Option::QMAKE_CMDLINE_ERROR;
                        }
                    } else if (Option::qmake_mode == Option::QMAKE_GENERATE_PROJECT) {
                        if (arg == "-nopwd") {
                            Option::projfile::do_pwd = false;
                        } else {
                            fprintf(stderr, "***Unknown option %s\n", arg.toLatin1().constData());
                            return Option::QMAKE_CMDLINE_SHOW_USAGE | Option::QMAKE_CMDLINE_ERROR;
                        }
                    }
                }
            } else {
                bool handled = true;
                if(Option::qmake_mode == Option::QMAKE_QUERY_PROPERTY ||
                    Option::qmake_mode == Option::QMAKE_SET_PROPERTY ||
                    Option::qmake_mode == Option::QMAKE_UNSET_PROPERTY) {
                    Option::prop::properties.append(arg);
                } else {
                    QFileInfo fi(arg);
                    if(!fi.makeAbsolute()) //strange
                        arg = fi.filePath();
                    if(Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
                       Option::qmake_mode == Option::QMAKE_GENERATE_PRL) {
                        if(fi.isDir()) {
                            QString proj = detectProjectFile(arg);
                            if (!proj.isNull())
                                arg = proj;
                        }
                        Option::mkfile::project_files.append(arg);
                    } else if(Option::qmake_mode == Option::QMAKE_GENERATE_PROJECT) {
                        Option::projfile::project_dirs.append(arg);
                    } else {
                        handled = false;
                    }
                }
                if(!handled) {
                    return Option::QMAKE_CMDLINE_SHOW_USAGE | Option::QMAKE_CMDLINE_ERROR;
                }
                args.removeAt(x);
                continue;
            }
        }
        x++;
    }
    if (argState != ArgNone) {
        fprintf(stderr, "***Option %s requires a parameter\n", qPrintable(args.at(x - 1)));
        return Option::QMAKE_CMDLINE_SHOW_USAGE | Option::QMAKE_CMDLINE_ERROR;
    }
    return Option::QMAKE_CMDLINE_SUCCESS;
}

int
Option::init(int argc, char **argv)
{
    Option::prf_ext = ".prf";
    Option::pro_ext = ".pro";
    Option::field_sep = ' ';

    if(argc && argv) {
        QString argv0 = argv[0];
#ifdef Q_OS_WIN
        if (!argv0.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
            argv0 += QLatin1String(".exe");
#endif
        if(Option::qmake_mode == Option::QMAKE_GENERATE_NOTHING)
            Option::qmake_mode = default_mode(argv0);
        if (!argv0.isEmpty() && IoUtils::isAbsolutePath(argv0)) {
            globals->qmake_abslocation = argv0;
        } else if (argv0.contains(QLatin1Char('/'))
#ifdef Q_OS_WIN
                   || argv0.contains(QLatin1Char('\\'))
#endif
            ) { //relative PWD
            globals->qmake_abslocation = QDir::current().absoluteFilePath(argv0);
        } else { //in the PATH
            QByteArray pEnv = qgetenv("PATH");
            QDir currentDir = QDir::current();
#ifdef Q_OS_WIN
            QStringList paths = QString::fromLocal8Bit(pEnv).split(QLatin1String(";"));
            paths.prepend(QLatin1String("."));
#else
            QStringList paths = QString::fromLocal8Bit(pEnv).split(QLatin1String(":"));
#endif
            for (QStringList::const_iterator p = paths.constBegin(); p != paths.constEnd(); ++p) {
                if ((*p).isEmpty())
                    continue;
                QString candidate = currentDir.absoluteFilePath(*p + QLatin1Char('/') + argv0);
                if (QFile::exists(candidate)) {
                    globals->qmake_abslocation = candidate;
                    break;
                }
            }
        }
        if (Q_UNLIKELY(globals->qmake_abslocation.isNull())) {
            // This is rather unlikely to ever happen on a modern system ...
            globals->qmake_abslocation = QLibraryInfo::rawLocation(
                                                QLibraryInfo::HostBinariesPath,
                                                QLibraryInfo::EffectivePaths)
#ifdef Q_OS_WIN
                                         + "/qmake.exe";
#else
                                         + "/qmake";
#endif
        } else {
            globals->qmake_abslocation = QDir::cleanPath(globals->qmake_abslocation);
        }
    } else {
        Option::qmake_mode = Option::QMAKE_GENERATE_MAKEFILE;
    }

    QMakeCmdLineParserState cmdstate(QDir::currentPath());
    const QByteArray envflags = qgetenv("QMAKEFLAGS");
    if (!envflags.isNull()) {
        QStringList args;
        QByteArray buf = "";
        char quote = 0;
        bool hasWord = false;
        for (int i = 0; i < envflags.size(); ++i) {
            char c = envflags.at(i);
            if (!quote && (c == '\'' || c == '"')) {
                quote = c;
            } else if (c == quote) {
                quote = 0;
            } else if (!quote && c == ' ') {
                if (hasWord) {
                    args << QString::fromLocal8Bit(buf);
                    hasWord = false;
                    buf = "";
                }
            } else {
                buf += c;
                hasWord = true;
            }
        }
        if (hasWord)
            args << QString::fromLocal8Bit(buf);
        parseCommandLine(args, cmdstate);
        cmdstate.flush();
    }
    if(argc && argv) {
        QStringList args;
        args.reserve(argc - 1);
        for (int i = 1; i < argc; i++)
            args << QString::fromLocal8Bit(argv[i]);

        while (!args.isEmpty()) {
            QString opt = args.at(0);
            if (opt == "-project") {
                Option::recursive = true;
                Option::qmake_mode = Option::QMAKE_GENERATE_PROJECT;
            } else if (opt == "-prl") {
                Option::mkfile::do_deps = false;
                Option::mkfile::do_mocs = false;
                Option::qmake_mode = Option::QMAKE_GENERATE_PRL;
            } else if (opt == "-set") {
                Option::qmake_mode = Option::QMAKE_SET_PROPERTY;
            } else if (opt == "-unset") {
                Option::qmake_mode = Option::QMAKE_UNSET_PROPERTY;
            } else if (opt == "-query") {
                Option::qmake_mode = Option::QMAKE_QUERY_PROPERTY;
            } else if (opt == "-makefile") {
                Option::qmake_mode = Option::QMAKE_GENERATE_MAKEFILE;
            } else {
                break;
            }
            args.takeFirst();
            break;
        }

        int ret = parseCommandLine(args, cmdstate);
        if(ret != Option::QMAKE_CMDLINE_SUCCESS) {
            if ((ret & Option::QMAKE_CMDLINE_SHOW_USAGE) != 0)
                usage(argv[0]);
            return ret;
            //return ret == QMAKE_CMDLINE_SHOW_USAGE ? usage(argv[0]) : false;
        }
        globals->qmake_args = args;
        globals->qmake_extra_args = cmdstate.extraargs;
    }
    globals->commitCommandLineArguments(cmdstate);
    globals->debugLevel = Option::debug_level;

    //last chance for defaults
    if(Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
        Option::qmake_mode == Option::QMAKE_GENERATE_PRL) {
        globals->useEnvironment();

        //try REALLY hard to do it for them, lazy..
        if(Option::mkfile::project_files.isEmpty()) {
            QString proj = detectProjectFile(qmake_getpwd());
            if(!proj.isNull())
                Option::mkfile::project_files.append(proj);
#ifndef QT_BUILD_QMAKE_LIBRARY
            if(Option::mkfile::project_files.isEmpty()) {
                usage(argv[0]);
                return Option::QMAKE_CMDLINE_ERROR;
            }
#endif
        }
    }

    return QMAKE_CMDLINE_SUCCESS;
}

void Option::prepareProject(const QString &pfile)
{
    // Canonicalize only the directory, otherwise things will go haywire
    // if the file itself is a symbolic link.
    const QString srcpath = QFileInfo(QFileInfo(pfile).absolutePath()).canonicalFilePath();
    globals->setDirectories(srcpath, output_dir);
}

bool Option::postProcessProject(QMakeProject *project)
{
    Option::cpp_ext = project->values("QMAKE_EXT_CPP").toQStringList();
    Option::h_ext = project->values("QMAKE_EXT_H").toQStringList();
    Option::c_ext = project->values("QMAKE_EXT_C").toQStringList();
    Option::objc_ext = project->first("QMAKE_EXT_OBJC").toQString();
    Option::objcpp_ext = project->first("QMAKE_EXT_OBJCXX").toQString();
    Option::res_ext = project->first("QMAKE_EXT_RES").toQString();
    Option::pkgcfg_ext = project->first("QMAKE_EXT_PKGCONFIG").toQString();
    Option::libtool_ext = project->first("QMAKE_EXT_LIBTOOL").toQString();
    Option::prl_ext = project->first("QMAKE_EXT_PRL").toQString();
    Option::ui_ext = project->first("QMAKE_EXT_UI").toQString();
    Option::cpp_moc_ext = project->first("QMAKE_EXT_CPP_MOC").toQString();
    Option::lex_ext = project->first("QMAKE_EXT_LEX").toQString();
    Option::yacc_ext = project->first("QMAKE_EXT_YACC").toQString();
    Option::obj_ext = project->first("QMAKE_EXT_OBJ").toQString();
    Option::h_moc_mod = project->first("QMAKE_H_MOD_MOC").toQString();
    Option::lex_mod = project->first("QMAKE_MOD_LEX").toQString();
    Option::yacc_mod = project->first("QMAKE_MOD_YACC").toQString();

    Option::dir_sep = project->dirSep().toQString();

    if (!project->buildRoot().isEmpty() && Option::output_dir.startsWith(project->buildRoot()))
        Option::mkfile::cachefile_depth =
                Option::output_dir.mid(project->buildRoot().length()).count('/');

    return true;
}

QString
Option::fixString(QString string, uchar flags)
{
    //const QString orig_string = string;
    static QHash<FixStringCacheKey, QString> *cache = nullptr;
    if(!cache) {
        cache = new QHash<FixStringCacheKey, QString>;
        qmakeAddCacheClear(qmakeDeleteCacheClear<QHash<FixStringCacheKey, QString> >, (void**)&cache);
    }
    FixStringCacheKey cacheKey(string, flags);

    QHash<FixStringCacheKey, QString>::const_iterator it = cache->constFind(cacheKey);

    if (it != cache->constEnd()) {
        //qDebug() << "Fix (cached) " << orig_string << "->" << it.value();
        return it.value();
    }

    //fix the environment variables
    if(flags & Option::FixEnvVars) {
        int rep;
        static QRegExp reg_var("\\$\\(.*\\)");
        reg_var.setMinimal(true);
        while((rep = reg_var.indexIn(string)) != -1)
            string.replace(rep, reg_var.matchedLength(),
                           QString::fromLocal8Bit(qgetenv(string.mid(rep + 2, reg_var.matchedLength() - 3).toLatin1().constData()).constData()));
    }

    //canonicalize it (and treat as a path)
    if(flags & Option::FixPathCanonicalize) {
#if 0
        string = QFileInfo(string).canonicalFilePath();
#endif
        string = QDir::cleanPath(string);
    }

    // either none or only one active flag
    Q_ASSERT(((flags & Option::FixPathToLocalSeparators) != 0) +
             ((flags & Option::FixPathToTargetSeparators) != 0) +
             ((flags & Option::FixPathToNormalSeparators) != 0) <= 1);

    //fix separators
    if (flags & Option::FixPathToNormalSeparators) {
        string.replace('\\', '/');
    } else if (flags & Option::FixPathToLocalSeparators) {
#if defined(Q_OS_WIN32)
        string.replace('/', '\\');
#else
        string.replace('\\', '/');
#endif
    } else if(flags & Option::FixPathToTargetSeparators) {
        string.replace('/', Option::dir_sep).replace('\\', Option::dir_sep);
    }

    if ((string.startsWith("\"") && string.endsWith("\"")) ||
        (string.startsWith("\'") && string.endsWith("\'")))
        string = string.mid(1, string.length()-2);

    //cache
    //qDebug() << "Fix" << orig_string << "->" << string;
    cache->insert(cacheKey, string);
    return string;
}

void debug_msg_internal(int level, const char *fmt, ...)
{
    if(Option::debug_level < level)
        return;
    fprintf(stderr, "DEBUG %d: ", level);
    {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    fprintf(stderr, "\n");
}

void warn_msg(QMakeWarn type, const char *fmt, ...)
{
    if(!(Option::warn_level & type))
        return;
    fprintf(stderr, "WARNING: ");
    {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    fprintf(stderr, "\n");
}

void EvalHandler::message(int type, const QString &msg, const QString &fileName, int lineNo)
{
    QString pfx;
    if ((type & QMakeHandler::CategoryMask) == QMakeHandler::WarningMessage) {
        int code = (type & QMakeHandler::CodeMask);
        if ((code == QMakeHandler::WarnLanguage && !(Option::warn_level & WarnParser))
            || (code == QMakeHandler::WarnDeprecated && !(Option::warn_level & WarnDeprecated)))
            return;
        pfx = QString::fromLatin1("WARNING: ");
    }
    if (lineNo > 0)
        fprintf(stderr, "%s%s:%d: %s\n", qPrintable(pfx), qPrintable(fileName), lineNo, qPrintable(msg));
    else if (lineNo)
        fprintf(stderr, "%s%s: %s\n", qPrintable(pfx), qPrintable(fileName), qPrintable(msg));
    else
        fprintf(stderr, "%s%s\n", qPrintable(pfx), qPrintable(msg));
}

void EvalHandler::fileMessage(int type, const QString &msg)
{
    Q_UNUSED(type)
    fprintf(stderr, "%s\n", qPrintable(msg));
}

void EvalHandler::aboutToEval(ProFile *, ProFile *, EvalFileType)
{
}

void EvalHandler::doneWithEval(ProFile *)
{
}

class QMakeCacheClearItem {
private:
    qmakeCacheClearFunc func;
    void **data;
public:
    QMakeCacheClearItem(qmakeCacheClearFunc f, void **d) : func(f), data(d) { }
    ~QMakeCacheClearItem() {
        (*func)(*data);
        *data = nullptr;
    }
};
static QList<QMakeCacheClearItem*> cache_items;

void
qmakeClearCaches()
{
    qDeleteAll(cache_items);
    cache_items.clear();
}

void
qmakeAddCacheClear(qmakeCacheClearFunc func, void **data)
{
    cache_items.append(new QMakeCacheClearItem(func, data));
}

QString qmake_libraryInfoFile()
{
    if (!Option::globals->qtconf.isEmpty())
        return Option::globals->qtconf;
    if (!Option::globals->qmake_abslocation.isEmpty())
        return QDir(QFileInfo(Option::globals->qmake_abslocation).absolutePath()).filePath("qt.conf");
    return QString();
}

QT_END_NAMESPACE
