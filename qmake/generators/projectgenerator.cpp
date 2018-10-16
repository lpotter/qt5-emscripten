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

#include "projectgenerator.h"
#include "option.h"
#include <qdatetime.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>

QT_BEGIN_NAMESPACE

QString project_builtin_regx() //calculate the builtin regular expression..
{
    QString ret;
    QStringList builtin_exts;
    builtin_exts << Option::c_ext << Option::ui_ext << Option::yacc_ext << Option::lex_ext << ".ts" << ".xlf" << ".qrc";
    builtin_exts += Option::h_ext + Option::cpp_ext;
    for(int i = 0; i < builtin_exts.size(); ++i) {
        if(!ret.isEmpty())
            ret += "; ";
        ret += QString("*") + builtin_exts[i];
    }
    return ret;
}

ProjectGenerator::ProjectGenerator() : MakefileGenerator()
{
}

void
ProjectGenerator::init()
{
    int file_count = 0;
    verifyCompilers();

    project->loadSpec();
    project->evaluateFeatureFile("default_pre.prf");
    project->evaluateFeatureFile("default_post.prf");
    project->evaluateConfigFeatures();
    project->values("CONFIG").clear();
    Option::postProcessProject(project);

    ProValueMap &v = project->variables();
    QString templ = Option::globals->user_template.isEmpty() ? QString("app") : Option::globals->user_template;
    if (!Option::globals->user_template_prefix.isEmpty())
        templ.prepend(Option::globals->user_template_prefix);
    v["TEMPLATE_ASSIGN"] += templ;

    //the scary stuff
    if(project->first("TEMPLATE_ASSIGN") != "subdirs") {
        QString builtin_regex = project_builtin_regx();
        QStringList dirs = Option::projfile::project_dirs;
        if(Option::projfile::do_pwd) {
            if(!v["INCLUDEPATH"].contains("."))
                v["INCLUDEPATH"] += ".";
            dirs.prepend(qmake_getpwd());
        }

        for(int i = 0; i < dirs.count(); ++i) {
            QString dir, regex, pd = dirs.at(i);
            bool add_depend = false;
            if(exists(pd)) {
                QFileInfo fi(fileInfo(pd));
                if(fi.isDir()) {
                    dir = pd;
                    add_depend = true;
                    if(dir.right(1) != Option::dir_sep)
                        dir += Option::dir_sep;
                    if (Option::recursive) {
                        QStringList files = QDir(dir).entryList(QDir::Files);
                        for (int i = 0; i < files.count(); i++)
                            dirs.append(dir + files[i] + QDir::separator() + builtin_regex);
                    }
                    regex = builtin_regex;
                } else {
                    QString file = pd;
                    int s = file.lastIndexOf(Option::dir_sep);
                    if(s != -1)
                        dir = file.left(s+1);
                    if(addFile(file)) {
                        add_depend = true;
                        file_count++;
                    }
                }
            } else { //regexp
                regex = pd;
            }
            if(!regex.isEmpty()) {
                int s = regex.lastIndexOf(Option::dir_sep);
                if(s != -1) {
                    dir = regex.left(s+1);
                    regex = regex.right(regex.length() - (s+1));
                }
                const QDir d(dir);
                if (Option::recursive) {
                    QStringList entries = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                    for (int i = 0; i < entries.count(); i++)
                        dirs.append(dir + entries[i] + QDir::separator() + regex);
                }
                QStringList files = d.entryList(QDir::nameFiltersFromString(regex));
                for(int i = 0; i < (int)files.count(); i++) {
                    QString file = d.absoluteFilePath(files[i]);
                    if (addFile(file)) {
                        add_depend = true;
                        file_count++;
                    }
                }
            }
            if(add_depend && !dir.isEmpty() && !v["DEPENDPATH"].contains(dir, Qt::CaseInsensitive)) {
                QFileInfo fi(fileInfo(dir));
                if(fi.absoluteFilePath() != qmake_getpwd())
                    v["DEPENDPATH"] += fileFixify(dir);
            }
        }
    }
    if(!file_count) { //shall we try a subdir?
        QStringList knownDirs = Option::projfile::project_dirs;
        if(Option::projfile::do_pwd)
            knownDirs.prepend(".");
        const QString out_file = fileFixify(Option::output.fileName());
        for(int i = 0; i < knownDirs.count(); ++i) {
            QString pd = knownDirs.at(i);
            if(exists(pd)) {
                QString newdir = pd;
                QFileInfo fi(fileInfo(newdir));
                if(fi.isDir()) {
                    newdir = fileFixify(newdir, FileFixifyFromOutdir);
                    ProStringList &subdirs = v["SUBDIRS"];
                    if(exists(fi.filePath() + QDir::separator() + fi.fileName() + Option::pro_ext) &&
                       !subdirs.contains(newdir, Qt::CaseInsensitive)) {
                        subdirs.append(newdir);
                    } else {
                        QStringList profiles = QDir(newdir).entryList(QStringList("*" + Option::pro_ext), QDir::Files);
                        for(int i = 0; i < (int)profiles.count(); i++) {
                            QString nd = newdir;
                            if(nd == ".")
                                nd = "";
                            else if (!nd.isEmpty() && !nd.endsWith(QDir::separator()))
                                nd += QDir::separator();
                            nd += profiles[i];
                            fileFixify(nd);
                            if (!subdirs.contains(nd, Qt::CaseInsensitive) && !out_file.endsWith(nd))
                                subdirs.append(nd);
                        }
                    }
                    if (Option::recursive) {
                        QStringList dirs = QDir(newdir).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                        for(int i = 0; i < (int)dirs.count(); i++) {
                            QString nd = fileFixify(newdir + QDir::separator() + dirs[i]);
                            if (!knownDirs.contains(nd, Qt::CaseInsensitive))
                                knownDirs.append(nd);
                        }
                    }
                }
            } else { //regexp
                QString regx = pd, dir;
                int s = regx.lastIndexOf(Option::dir_sep);
                if(s != -1) {
                    dir = regx.left(s+1);
                    regx = regx.right(regx.length() - (s+1));
                }
                QStringList files = QDir(dir).entryList(QDir::nameFiltersFromString(regx),
                                                        QDir::Dirs | QDir::NoDotAndDotDot);
                ProStringList &subdirs = v["SUBDIRS"];
                for(int i = 0; i < (int)files.count(); i++) {
                    QString newdir(dir + files[i]);
                    QFileInfo fi(fileInfo(newdir));
                    {
                        newdir = fileFixify(newdir);
                        if(exists(fi.filePath() + QDir::separator() + fi.fileName() + Option::pro_ext) &&
                           !subdirs.contains(newdir)) {
                           subdirs.append(newdir);
                        } else {
                            QStringList profiles = QDir(newdir).entryList(QStringList("*" + Option::pro_ext), QDir::Files);
                            for(int i = 0; i < (int)profiles.count(); i++) {
                                QString nd = newdir + QDir::separator() + files[i];
                                fileFixify(nd);
                                if(files[i] != "." && files[i] != ".." && !subdirs.contains(nd, Qt::CaseInsensitive)) {
                                    if(newdir + files[i] != Option::output_dir + Option::output.fileName())
                                        subdirs.append(nd);
                                }
                            }
                        }
                        if (Option::recursive && !knownDirs.contains(newdir, Qt::CaseInsensitive))
                            knownDirs.append(newdir);
                    }
                }
            }
        }
        v["TEMPLATE_ASSIGN"] = ProStringList("subdirs");
        return;
    }

    //setup deplist
    QList<QMakeLocalFileName> deplist;
    {
        const ProStringList &d = v["DEPENDPATH"];
        for(int i = 0; i < d.size(); ++i)
            deplist.append(QMakeLocalFileName(d[i].toQString()));
    }
    setDependencyPaths(deplist);

    ProStringList &h = v["HEADERS"];
    bool no_qt_files = true;
    static const char *srcs[] = { "SOURCES", "YACCSOURCES", "LEXSOURCES", "FORMS", nullptr };
    for (int i = 0; srcs[i]; i++) {
        const ProStringList &l = v[srcs[i]];
        QMakeSourceFileInfo::SourceFileType type = QMakeSourceFileInfo::TYPE_C;
        QMakeSourceFileInfo::addSourceFiles(l, QMakeSourceFileInfo::SEEK_DEPS, type);
        for(int i = 0; i < l.size(); ++i) {
            QStringList tmp = QMakeSourceFileInfo::dependencies(l.at(i).toQString());
            if(!tmp.isEmpty()) {
                for(int dep_it = 0; dep_it < tmp.size(); ++dep_it) {
                    QString dep = tmp[dep_it];
                    dep = fixPathToQmake(dep);
                    QString file_dir = dep.section(Option::dir_sep, 0, -2),
                        file_no_path = dep.section(Option::dir_sep, -1);
                    if(!file_dir.isEmpty()) {
                        for(int inc_it = 0; inc_it < deplist.size(); ++inc_it) {
                            QMakeLocalFileName inc = deplist[inc_it];
                            if(inc.local() == file_dir && !v["INCLUDEPATH"].contains(inc.real(), Qt::CaseInsensitive))
                                v["INCLUDEPATH"] += inc.real();
                        }
                    }
                    if(no_qt_files && file_no_path.indexOf(QRegExp("^q[a-z_0-9].h$")) != -1)
                        no_qt_files = false;
                    QString h_ext;
                    for(int hit = 0; hit < Option::h_ext.size(); ++hit) {
                        if(dep.endsWith(Option::h_ext.at(hit))) {
                            h_ext = Option::h_ext.at(hit);
                            break;
                        }
                    }
                    if(!h_ext.isEmpty()) {
                        for(int cppit = 0; cppit < Option::cpp_ext.size(); ++cppit) {
                            QString src(dep.left(dep.length() - h_ext.length()) +
                                        Option::cpp_ext.at(cppit));
                            if(exists(src)) {
                                ProStringList &srcl = v["SOURCES"];
                                if(!srcl.contains(src, Qt::CaseInsensitive))
                                    srcl.append(src);
                            }
                        }
                    } else if(dep.endsWith(Option::lex_ext) &&
                              file_no_path.startsWith(Option::lex_mod)) {
                        addConfig("lex_included");
                    }
                    if(!h.contains(dep, Qt::CaseInsensitive))
                        h += dep;
                }
            }
        }
    }

    //strip out files that are actually output from internal compilers (ie temporary files)
    const ProStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
    for (ProStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
        QString tmp_out = project->first(ProKey(*it + ".output")).toQString();
        if(tmp_out.isEmpty())
            continue;

        ProStringList var_out = project->values(ProKey(*it + ".variable_out"));
        bool defaults = var_out.isEmpty();
        for(int i = 0; i < var_out.size(); ++i) {
            ProString v = var_out.at(i);
            if(v.startsWith("GENERATED_")) {
                defaults = true;
                break;
            }
        }
        if(defaults) {
            var_out << "SOURCES";
            var_out << "HEADERS";
            var_out << "FORMS";
        }
        const ProStringList &tmp = project->values(ProKey(*it + ".input"));
        for (ProStringList::ConstIterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
            ProStringList &inputs = project->values((*it2).toKey());
            for (ProStringList::Iterator input = inputs.begin(); input != inputs.end(); ++input) {
                QString path = replaceExtraCompilerVariables(tmp_out, (*input).toQString(), QString(), NoShell);
                path = fixPathToQmake(path).section('/', -1);
                for(int i = 0; i < var_out.size(); ++i) {
                    ProString v = var_out.at(i);
                    ProStringList &list = project->values(v.toKey());
                    for(int src = 0; src < list.size(); ) {
                        if(list[src] == path || list[src].endsWith("/" + path))
                            list.removeAt(src);
                        else
                            ++src;
                    }
                }
            }
        }
    }
}

bool
ProjectGenerator::writeMakefile(QTextStream &t)
{
    t << "######################################################################" << endl;
    t << "# Automatically generated by qmake (" QMAKE_VERSION_STR ") " << QDateTime::currentDateTime().toString() << endl;
    t << "######################################################################" << endl << endl;
    if (!Option::globals->extra_cmds[QMakeEvalBefore].isEmpty())
        t << Option::globals->extra_cmds[QMakeEvalBefore] << endl;
    t << getWritableVar("TEMPLATE_ASSIGN", false);
    if(project->first("TEMPLATE_ASSIGN") == "subdirs") {
        t << endl << "# Directories" << "\n"
          << getWritableVar("SUBDIRS");
    } else {
        //figure out target
        QString ofn = QFileInfo(static_cast<QFile *>(t.device())->fileName()).completeBaseName();
        if (ofn.isEmpty() || ofn == "-")
            ofn = "unknown";
        project->values("TARGET_ASSIGN") = ProStringList(ofn);

        t << getWritableVar("TARGET_ASSIGN")
          << getWritableVar("CONFIG", false)
          << getWritableVar("CONFIG_REMOVE", false)
          << getWritableVar("INCLUDEPATH") << endl;

        t << "# The following define makes your compiler warn you if you use any\n"
             "# feature of Qt which has been marked as deprecated (the exact warnings\n"
             "# depend on your compiler). Please consult the documentation of the\n"
             "# deprecated API in order to know how to port your code away from it.\n"
             "DEFINES += QT_DEPRECATED_WARNINGS\n"
             "\n"
             "# You can also make your code fail to compile if you use deprecated APIs.\n"
             "# In order to do so, uncomment the following line.\n"
             "# You can also select to disable deprecated APIs only up to a certain version of Qt.\n"
             "#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0\n\n";

        t << "# Input" << "\n";
        t << getWritableVar("HEADERS")
          << getWritableVar("FORMS")
          << getWritableVar("LEXSOURCES")
          << getWritableVar("YACCSOURCES")
          << getWritableVar("SOURCES")
          << getWritableVar("RESOURCES")
          << getWritableVar("TRANSLATIONS");
    }
    if (!Option::globals->extra_cmds[QMakeEvalAfter].isEmpty())
        t << Option::globals->extra_cmds[QMakeEvalAfter] << endl;
    return true;
}

bool
ProjectGenerator::addConfig(const QString &cfg, bool add)
{
    ProKey where = "CONFIG";
    if(!add)
        where = "CONFIG_REMOVE";
    if (!project->values(where).contains(cfg)) {
        project->values(where) += cfg;
        return true;
    }
    return false;
}

bool
ProjectGenerator::addFile(QString file)
{
    file = fileFixify(file, FileFixifyToIndir);
    QString dir;
    int s = file.lastIndexOf(Option::dir_sep);
    if(s != -1)
        dir = file.left(s+1);
    if(file.mid(dir.length(), Option::h_moc_mod.length()) == Option::h_moc_mod)
        return false;

    ProKey where;
    for(int cppit = 0; cppit < Option::cpp_ext.size(); ++cppit) {
        if(file.endsWith(Option::cpp_ext[cppit])) {
            where = "SOURCES";
            break;
        }
    }
    if(where.isEmpty()) {
        for(int hit = 0; hit < Option::h_ext.size(); ++hit)
            if(file.endsWith(Option::h_ext.at(hit))) {
                where = "HEADERS";
                break;
            }
    }
    if(where.isEmpty()) {
        for(int cit = 0; cit < Option::c_ext.size(); ++cit) {
            if(file.endsWith(Option::c_ext[cit])) {
                where = "SOURCES";
                break;
            }
        }
    }
    if(where.isEmpty()) {
        if(file.endsWith(Option::ui_ext))
            where = "FORMS";
        else if(file.endsWith(Option::lex_ext))
            where = "LEXSOURCES";
        else if(file.endsWith(Option::yacc_ext))
            where = "YACCSOURCES";
        else if(file.endsWith(".ts") || file.endsWith(".xlf"))
            where = "TRANSLATIONS";
        else if(file.endsWith(".qrc"))
            where = "RESOURCES";
    }

    QString newfile = fixPathToQmake(fileFixify(file));

    ProStringList &endList = project->values(where);
    if(!endList.contains(newfile, Qt::CaseInsensitive)) {
        endList += newfile;
        return true;
    }
    return false;
}

QString
ProjectGenerator::getWritableVar(const char *vk, bool)
{
    const ProKey v(vk);
    ProStringList &vals = project->values(v);
    if(vals.isEmpty())
        return "";

    // If values contain spaces, ensure that they are quoted
    for (ProStringList::iterator it = vals.begin(); it != vals.end(); ++it) {
        if ((*it).contains(' ') && !(*it).startsWith(' '))
            *it = "\"" + *it + "\"";
    }

    QString ret;
    if(v.endsWith("_REMOVE"))
        ret = v.left(v.length() - 7) + " -= ";
    else if(v.endsWith("_ASSIGN"))
        ret = v.left(v.length() - 7) + " = ";
    else
        ret = v + " += ";
    QString join = vals.join(' ');
    if(ret.length() + join.length() > 80) {
        QString spaces;
        for(int i = 0; i < ret.length(); i++)
            spaces += " ";
        join = vals.join(" \\\n" + spaces);
    }
    return ret + join + "\n";
}

bool
ProjectGenerator::openOutput(QFile &file, const QString &build) const
{
    ProString fileName = file.fileName();
    if (!fileName.endsWith(Option::pro_ext)) {
        if (fileName.isEmpty())
            fileName = fileInfo(Option::output_dir).fileName();
        file.setFileName(fileName + Option::pro_ext);
    }
    return MakefileGenerator::openOutput(file, build);
}


QString
ProjectGenerator::fixPathToQmake(const QString &file)
{
    QString ret = file;
    if(Option::dir_sep != QLatin1String("/"))
        ret.replace(Option::dir_sep, QLatin1String("/"));
    return ret;
}

QT_END_NAMESPACE
