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

#include "pbuilder_pbx.h"
#include "option.h"
#include "meta.h"
#include <qdir.h>
#include <qregexp.h>
#include <qcryptographichash.h>
#include <qdebug.h>
#include <qsettings.h>
#include <qstring.h>
#include <stdlib.h>
#include <time.h>
#ifdef Q_OS_UNIX
#  include <sys/types.h>
#  include <sys/stat.h>
#endif
#ifdef Q_OS_DARWIN
#include <ApplicationServices/ApplicationServices.h>
#include <private/qcore_mac_p.h>
#endif

QT_BEGIN_NAMESPACE

//#define GENERATE_AGGREGRATE_SUBDIR

// Note: this is fairly hacky, but it does the job...

using namespace QMakeInternal;

static QString qtSha1(const QByteArray &src)
{
    QByteArray digest = QCryptographicHash::hash(src, QCryptographicHash::Sha1);
    return QString::fromLatin1(digest.toHex());
}

ProjectBuilderMakefileGenerator::ProjectBuilderMakefileGenerator() : UnixMakefileGenerator()
{

}

bool
ProjectBuilderMakefileGenerator::writeMakefile(QTextStream &t)
{
    writingUnixMakefileGenerator = false;
    if(!project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()) {
        /* for now just dump, I need to generated an empty xml or something.. */
        fprintf(stderr, "Project file not generated because all requirements not met:\n\t%s\n",
                var("QMAKE_FAILED_REQUIREMENTS").toLatin1().constData());
        return true;
    }

    project->values("MAKEFILE").clear();
    project->values("MAKEFILE").append("Makefile");
    if(project->first("TEMPLATE") == "app" || project->first("TEMPLATE") == "lib")
        return writeMakeParts(t);
    else if(project->first("TEMPLATE") == "subdirs")
        return writeSubDirs(t);
    return false;
}

struct ProjectBuilderSubDirs {
    QMakeProject *project;
    QString subdir;
    bool autoDelete;
    ProjectBuilderSubDirs(QMakeProject *p, QString s, bool a=true) : project(p), subdir(s), autoDelete(a) { }
    ~ProjectBuilderSubDirs() {
        if(autoDelete)
            delete project;
    }
};

bool
ProjectBuilderMakefileGenerator::writeSubDirs(QTextStream &t)
{
    //HEADER
    const int pbVersion = pbuilderVersion();
    t << "// !$*UTF8*$!\n"
      << "{\n"
      << "\t" << writeSettings("archiveVersion", "1", SettingsNoQuote) << ";\n"
      << "\tclasses = {\n\t};\n"
      << "\t" << writeSettings("objectVersion", QString::number(pbVersion), SettingsNoQuote) << ";\n"
      << "\tobjects = {\n";

    //SUBDIRS
    QList<ProjectBuilderSubDirs*> pb_subdirs;
    pb_subdirs.append(new ProjectBuilderSubDirs(project, QString(), false));
    QString oldpwd = qmake_getpwd();
    QString oldoutpwd = Option::output_dir;
    QMap<QString, ProStringList> groups;
    for(int pb_subdir = 0; pb_subdir < pb_subdirs.size(); ++pb_subdir) {
        ProjectBuilderSubDirs *pb = pb_subdirs[pb_subdir];
        const ProStringList &subdirs = pb->project->values("SUBDIRS");
        for(int subdir = 0; subdir < subdirs.count(); subdir++) {
            ProString tmpk = subdirs[subdir];
            const ProKey fkey(tmpk + ".file");
            if (!pb->project->isEmpty(fkey)) {
                tmpk = pb->project->first(fkey);
            } else {
                const ProKey skey(tmpk + ".subdir");
                if (!pb->project->isEmpty(skey))
                    tmpk = pb->project->first(skey);
            }
            QString tmp = tmpk.toQString();
            if(fileInfo(tmp).isRelative() && !pb->subdir.isEmpty()) {
                QString subdir = pb->subdir;
                if(!subdir.endsWith(Option::dir_sep))
                    subdir += Option::dir_sep;
                tmp = subdir + tmp;
            }
            QFileInfo fi(fileInfo(Option::normalizePath(tmp)));
            if(fi.exists()) {
                if(fi.isDir()) {
                    QString profile = tmp;
                    if(!profile.endsWith(Option::dir_sep))
                        profile += Option::dir_sep;
                    profile += fi.baseName() + Option::pro_ext;
                    fi = QFileInfo(profile);
                }
                QMakeProject tmp_proj;
                QString dir = fi.path(), fn = fi.fileName();
                if(!dir.isEmpty()) {
                    if(!qmake_setpwd(dir))
                        fprintf(stderr, "Cannot find directory: %s\n", dir.toLatin1().constData());
                }
                Option::output_dir = Option::globals->shadowedPath(qmake_getpwd());
                if(tmp_proj.read(fn)) {
                    if(tmp_proj.first("TEMPLATE") == "subdirs") {
                        QMakeProject *pp = new QMakeProject(&tmp_proj);
                        pb_subdirs += new ProjectBuilderSubDirs(pp, dir);
                    } else if(tmp_proj.first("TEMPLATE") == "app" || tmp_proj.first("TEMPLATE") == "lib") {
                        QString pbxproj = Option::output_dir + Option::dir_sep + tmp_proj.first("TARGET") + projectSuffix();
                        if(!exists(pbxproj)) {
                            warn_msg(WarnLogic, "Ignored (not found) '%s'", pbxproj.toLatin1().constData());
                            goto nextfile; // # Dirty!
                        }
                        const QString project_key = keyFor(pbxproj + "_PROJECTREF");
                        project->values("QMAKE_PBX_SUBDIRS") += pbxproj;
                        //PROJECTREF
                        {
                            bool in_root = true;
                            QString name = qmake_getpwd();
                            if(project->isActiveConfig("flat")) {
                                QString flat_file = fileFixify(name, FileFixifyBackwards | FileFixifyRelative);
                                if(flat_file.indexOf(Option::dir_sep) != -1) {
                                    QStringList dirs = flat_file.split(Option::dir_sep);
                                    name = dirs.back();
                                }
                            } else {
                                QString flat_file = fileFixify(name, FileFixifyBackwards | FileFixifyRelative);
                                if(QDir::isRelativePath(flat_file) && flat_file.indexOf(Option::dir_sep) != -1) {
                                    QString last_grp("QMAKE_SUBDIR_PBX_HEIR_GROUP");
                                    QStringList dirs = flat_file.split(Option::dir_sep);
                                    name = dirs.back();
                                    for(QStringList::Iterator dir_it = dirs.begin(); dir_it != dirs.end(); ++dir_it) {
                                        QString new_grp(last_grp + Option::dir_sep + (*dir_it)), new_grp_key(keyFor(new_grp));
                                        if(dir_it == dirs.begin()) {
                                            if(!groups.contains(new_grp))
                                                project->values("QMAKE_SUBDIR_PBX_GROUPS").append(new_grp_key);
                                        } else {
                                            if(!groups[last_grp].contains(new_grp_key))
                                                groups[last_grp] += new_grp_key;
                                        }
                                        last_grp = new_grp;
                                    }
                                    groups[last_grp] += project_key;
                                    in_root = false;
                                }
                            }
                            if(in_root)
                                project->values("QMAKE_SUBDIR_PBX_GROUPS") += project_key;
                            t << "\t\t" << project_key << " = {\n"
                              << "\t\t\t" << writeSettings("isa", "PBXFileReference", SettingsNoQuote) << ";\n"
                              << "\t\t\t" << writeSettings("lastKnownFileType", "wrapper.pb-project") << ";\n"
                              << "\t\t\t" << writeSettings("name", tmp_proj.first("TARGET") + projectSuffix()) << ";\n"
                              << "\t\t\t" << writeSettings("path", pbxproj) << ";\n"
                              << "\t\t\t" << writeSettings("sourceTree", "<absolute>") << ";\n"
                              << "\t\t};\n";
                            //WRAPPER
                            t << "\t\t" << keyFor(pbxproj + "_WRAPPER") << " = {\n"
                              << "\t\t\t" << writeSettings("isa", "PBXReferenceProxy", SettingsNoQuote) << ";\n";
                            if(tmp_proj.first("TEMPLATE") == "app") {
                                t << "\t\t\t" << writeSettings("fileType", "wrapper.application") << ";\n"
                                  << "\t\t\t" << writeSettings("path", tmp_proj.first("TARGET") + ".app") << ";\n";
                            } else {
                                t << "\t\t\t" << writeSettings("fileType", "compiled.mach-o.dylib") << ";\n"
                                  << "\t\t\t" << writeSettings("path", tmp_proj.first("TARGET") + ".dylib") << ";\n";
                            }
                            t << "\t\t\t" << writeSettings("remoteRef", keyFor(pbxproj + "_WRAPPERREF")) << ";\n"
                              << "\t\t\t" << writeSettings("sourceTree", "BUILT_PRODUCTS_DIR", SettingsNoQuote) << ";\n"
                              << "\t\t};\n";
                            t << "\t\t" << keyFor(pbxproj + "_WRAPPERREF") << " = {\n"
                              << "\t\t\t" << writeSettings("containerPortal", project_key) << ";\n"
                              << "\t\t\t" << writeSettings("isa", "PBXContainerItemProxy", SettingsNoQuote) << ";\n"
                              << "\t\t\t" << writeSettings("proxyType", "2") << ";\n"
//                              << "\t\t\t" << writeSettings("remoteGlobalIDString", keyFor(pbxproj + "QMAKE_PBX_REFERENCE")) << ";\n"
                              << "\t\t\t" << writeSettings("remoteGlobalIDString", keyFor(pbxproj + "QMAKE_PBX_REFERENCE!!!")) << ";\n"
                              << "\t\t\t" << writeSettings("remoteInfo", tmp_proj.first("TARGET")) << ";\n"
                              << "\t\t};\n";
                            //PRODUCTGROUP
                            t << "\t\t" << keyFor(pbxproj + "_PRODUCTGROUP") << " = {\n"
                              << "\t\t\t" << writeSettings("children", project->values(ProKey(pbxproj + "_WRAPPER")), SettingsAsList, 4) << ";\n"
                              << "\t\t\t" << writeSettings("isa", "PBXGroup", SettingsNoQuote) << ";\n"
                              << "\t\t\t" << writeSettings("name", "Products") << ";\n"
                              << "\t\t\t" << writeSettings("sourceTree", "<group>") << ";\n"
                              << "\t\t};\n";
                        }
#ifdef GENERATE_AGGREGRATE_SUBDIR
                        //TARGET (for aggregate)
                        {
                            //container
                            const QString container_proxy = keyFor(pbxproj + "_CONTAINERPROXY");
                            t << "\t\t" << container_proxy << " = {\n"
                              << "\t\t\t" << writeSettings("containerPortal", project_key) << ";\n"
                              << "\t\t\t" << writeSettings("isa", "PBXContainerItemProxy", SettingsNoQuote) << ";\n"
                              << "\t\t\t" << writeSettings("proxyType", "1") << ";\n"
                              << "\t\t\t" << writeSettings("remoteGlobalIDString", keyFor(pbxproj + "QMAKE_PBX_TARGET")) << ";\n"
                              << "\t\t\t" << writeSettings("remoteInfo", tmp_proj.first("TARGET")) << ";\n"
                              << "\t\t};\n";
                            //targetref
                            t << "\t\t" << keyFor(pbxproj + "_TARGETREF") << " = {\n"
                              << "\t\t\t" << writeSettings("isa", "PBXTargetDependency", SettingsNoQuote) << ";\n"
                              << "\t\t\t" << writeSettings("name", fixForOutput(tmp_proj.first("TARGET") +" (from " + tmp_proj.first("TARGET") + projectSuffix() + ")")) << ";\n"
                              << "\t\t\t" << writeSettings("targetProxy", container_proxy) << ";\n"
                              << "\t\t};\n";
                        }
#endif
                    }
                }
            nextfile:
                qmake_setpwd(oldpwd);
                Option::output_dir = oldoutpwd;
            }
        }
    }
    qDeleteAll(pb_subdirs);
    pb_subdirs.clear();

    for (QMap<QString, ProStringList>::Iterator grp_it = groups.begin(); grp_it != groups.end(); ++grp_it) {
        t << "\t\t" << keyFor(grp_it.key()) << " = {\n"
          << "\t\t\t" << writeSettings("isa", "PBXGroup", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("children", grp_it.value(), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("name", grp_it.key().section(Option::dir_sep, -1)) << ";\n"
          << "\t\t\t" << writeSettings("sourceTree", "<group>") << ";\n"
          << "\t\t};\n";
    }

    //DUMP EVERYTHING THAT TIES THE ABOVE TOGETHER
    //BUILDCONFIGURATIONS
    QString defaultConfig;
    for(int as_release = 0; as_release < 2; as_release++)
    {
        QString configName = (as_release ? "Release" : "Debug");

        QMap<QString, QString> settings;
        if(project->isActiveConfig("sdk") && !project->isEmpty("QMAKE_MAC_SDK"))
            settings.insert("SDKROOT", project->first("QMAKE_MAC_SDK").toQString());
        {
            const ProStringList &l = project->values("QMAKE_MAC_XCODE_SETTINGS");
            for(int i = 0; i < l.size(); ++i) {
                ProString name = l.at(i);
                const ProKey buildKey(name + ".build");
                if (!project->isEmpty(buildKey)) {
                    const QString build = project->first(buildKey).toQString();
                    if (build.toLower() != configName.toLower())
                        continue;
                }
                const ProKey nkey(name + ".name");
                if (!project->isEmpty(nkey))
                    name = project->first(nkey);
                const QString value = project->values(ProKey(name + ".value")).join(QString(Option::field_sep));
                settings.insert(name.toQString(), value);
            }
        }

        if (project->isActiveConfig("debug") != (bool)as_release)
            defaultConfig = configName;
        QString key = keyFor("QMAKE_SUBDIR_PBX_BUILDCONFIG_" + configName);
        project->values("QMAKE_SUBDIR_PBX_BUILDCONFIGS").append(key);
        t << "\t\t" << key << " = {\n"
        << "\t\t\t" << writeSettings("isa", "XCBuildConfiguration", SettingsNoQuote) << ";\n"
        << "\t\t\tbuildSettings = {\n";
        for (QMap<QString, QString>::Iterator set_it = settings.begin(); set_it != settings.end(); ++set_it)
            t << "\t\t\t\t" << writeSettings(set_it.key(), set_it.value()) << ";\n";
        t << "\t\t\t};\n"
          << "\t\t\t" << writeSettings("name", configName) << ";\n"
          << "\t\t};\n";
    }
    t << "\t\t" << keyFor("QMAKE_SUBDIR_PBX_BUILDCONFIG_LIST") << " = {\n"
      << "\t\t\t" << writeSettings("isa", "XCConfigurationList", SettingsNoQuote) << ";\n"
      << "\t\t\t" << writeSettings("buildConfigurations", project->values("QMAKE_SUBDIR_PBX_BUILDCONFIGS"), SettingsAsList, 4) << ";\n"
      << "\t\t\t" << writeSettings("defaultConfigurationIsVisible", "0", SettingsNoQuote) << ";\n"
      << "\t\t\t" << writeSettings("defaultConfigurationName", defaultConfig, SettingsNoQuote) << ";\n"
      << "\t\t};\n";

#ifdef GENERATE_AGGREGRATE_SUBDIR
    //target
    t << "\t\t" << keyFor("QMAKE_SUBDIR_PBX_AGGREGATE_TARGET") << " = {\n"
      << "\t\t\t" << writeSettings("buildPhases", ProStringList(), SettingsAsList, 4) << ";\n"
      << "\t\t\tbuildSettings = {\n"
      << "\t\t\t\t" << writeSettings("PRODUCT_NAME", project->first("TARGET")) << ";\n"
      << "\t\t\t};\n";
    {
        ProStringList dependencies;
        const ProStringList &qmake_subdirs = project->values("QMAKE_PBX_SUBDIRS");
        for(int i = 0; i < qmake_subdirs.count(); i++)
            dependencies += keyFor(qmake_subdirs[i] + "_TARGETREF");
        t << "\t\t\t" << writeSettings("dependencies", dependencies, SettingsAsList, 4) << ";\n"
    }
    t << "\t\t\t" << writeSettings("isa", "PBXAggregateTarget", SettingsNoQuote) << ";\n"
      << "\t\t\t" << writeSettings("name", project->first("TARGET")) << ";\n"
      << "\t\t\t" << writeSettings("productName", project->first("TARGET")) << ";\n"
      << "\t\t};\n";
#endif

    //ROOT_GROUP
    t << "\t\t" << keyFor("QMAKE_SUBDIR_PBX_ROOT_GROUP") << " = {\n"
      << "\t\t\t" << writeSettings("children", project->values("QMAKE_SUBDIR_PBX_GROUPS"), SettingsAsList, 4) << ";\n"
      << "\t\t\t" << writeSettings("isa", "PBXGroup", SettingsNoQuote) << ";\n"
      << "\t\t\t" << writeSettings("sourceTree", "<group>") << ";\n"
      << "\t\t};\n";


    //ROOT
    t << "\t\t" << keyFor("QMAKE_SUBDIR_PBX_ROOT") << " = {\n"
      << "\t\t\tbuildSettings = {\n"
      << "\t\t\t};\n"
      << "\t\t\t" << writeSettings("buildStyles", project->values("QMAKE_SUBDIR_PBX_BUILDSTYLES"), SettingsAsList, 4) << ";\n"
      << "\t\t\t" << writeSettings("isa", "PBXProject", SettingsNoQuote) << ";\n"
      << "\t\t\t" << writeSettings("mainGroup", keyFor("QMAKE_SUBDIR_PBX_ROOT_GROUP")) << ";\n"
      << "\t\t\t" << writeSettings("projectDirPath", ProStringList()) << ";\n";
    t << "\t\t\t" << writeSettings("buildConfigurationList", keyFor("QMAKE_SUBDIR_PBX_BUILDCONFIG_LIST")) << ";\n";
    t << "\t\t\tprojectReferences = (\n";
    {
        const ProStringList &qmake_subdirs = project->values("QMAKE_PBX_SUBDIRS");
        for(int i = 0; i < qmake_subdirs.count(); i++) {
            const ProString &subdir = qmake_subdirs[i];
            t << "\t\t\t\t{\n"
              << "\t\t\t\t\t" << writeSettings("ProductGroup", keyFor(subdir + "_PRODUCTGROUP")) << ";\n"
              << "\t\t\t\t\t" << writeSettings("ProjectRef", keyFor(subdir + "_PROJECTREF")) << ";\n"
              << "\t\t\t\t},\n";
        }
    }
    t << "\t\t\t);\n"
      << "\t\t\t" << writeSettings("targets",
#ifdef GENERATE_AGGREGRATE_SUBDIR
                                 project->values("QMAKE_SUBDIR_AGGREGATE_TARGET"),
#else
                                 ProStringList(),
#endif
                                   SettingsAsList, 4) << ";\n"
      << "\t\t};\n";

    //FOOTER
    t << "\t};\n"
      << "\t" << writeSettings("rootObject", keyFor("QMAKE_SUBDIR_PBX_ROOT")) << ";\n"
      << "}\n";

    return true;
}

class ProjectBuilderSources
{
    bool buildable, object_output;
    QString key, group, compiler;
public:
    ProjectBuilderSources(const QString &key, bool buildable = false, const QString &compiler = QString(), bool producesObject = false);
    QStringList files(QMakeProject *project) const;
    inline bool isBuildable() const { return buildable; }
    inline QString keyName() const { return key; }
    inline QString groupName() const { return group; }
    inline QString compilerName() const { return compiler; }
    inline bool isObjectOutput(const QString &file) const {
        if (object_output)
            return true;

        if (file.endsWith(Option::objc_ext))
            return true;
        if (file.endsWith(Option::objcpp_ext))
            return true;

        for (int i = 0; i < Option::c_ext.size(); ++i) {
            if (file.endsWith(Option::c_ext.at(i)))
                return true;
        }
        for (int i = 0; i < Option::cpp_ext.size(); ++i) {
            if (file.endsWith(Option::cpp_ext.at(i)))
                return true;
        }

        return false;
    }
};

ProjectBuilderSources::ProjectBuilderSources(const QString &k, bool b, const QString &c, bool o) :
    buildable(b), object_output(o), key(k), compiler(c)
{
    // Override group name for a few common keys
    if (k == "SOURCES" || k == "OBJECTIVE_SOURCES" || k == "HEADERS")
        group = "Sources";
    else if (k == "QMAKE_INTERNAL_INCLUDED_FILES")
        group = "Supporting Files";
    else if (k == "GENERATED_SOURCES" || k == "GENERATED_FILES")
        group = "Generated Sources";
    else if (k == "RESOURCES")
        group = "Resources";
    else if (group.isNull())
        group = QString("Sources [") + c + "]";
}

QStringList
ProjectBuilderSources::files(QMakeProject *project) const
{
    QStringList ret = project->values(ProKey(key)).toQStringList();
    if(key == "QMAKE_INTERNAL_INCLUDED_FILES") {
        QString qtPrefix(project->propertyValue(ProKey("QT_INSTALL_PREFIX/get")).toQString() + '/');
        QString qtSrcPrefix(project->propertyValue(ProKey("QT_INSTALL_PREFIX/src")).toQString() + '/');

        QStringList newret;
        for(int i = 0; i < ret.size(); ++i) {
            // Don't show files "internal" to Qt in Xcode
            if (ret.at(i).startsWith(qtPrefix) || ret.at(i).startsWith(qtSrcPrefix))
                continue;

            newret.append(ret.at(i));
        }
        ret = newret;
    }
    if(key == "SOURCES" && project->first("TEMPLATE") == "app" && !project->isEmpty("ICON"))
        ret.append(project->first("ICON").toQString());
    return ret;
}

static QString xcodeFiletypeForFilename(const QString &filename)
{
    for (const QString &ext : qAsConst(Option::cpp_ext)) {
        if (filename.endsWith(ext))
            return QStringLiteral("sourcecode.cpp.cpp");
    }

    for (const QString &ext : qAsConst(Option::c_ext)) {
        if (filename.endsWith(ext))
            return QStringLiteral("sourcecode.c.c");
    }

    for (const QString &ext : qAsConst(Option::h_ext)) {
        if (filename.endsWith(ext))
            return "sourcecode.c.h";
    }

    if (filename.endsWith(Option::objcpp_ext))
        return QStringLiteral("sourcecode.cpp.objcpp");
    if (filename.endsWith(Option::objc_ext))
        return QStringLiteral("sourcecode.c.objc");
    if (filename.endsWith(QLatin1String(".framework")))
        return QStringLiteral("wrapper.framework");
    if (filename.endsWith(QLatin1String(".a")))
        return QStringLiteral("archive.ar");
    if (filename.endsWith(QLatin1String(".pro")) || filename.endsWith(QLatin1String(".qrc")))
        return QStringLiteral("text");

    return QString();
}

static bool compareProvisioningTeams(const QVariantMap &a, const QVariantMap &b)
{
    int aFree = a.value(QLatin1String("isFreeProvisioningTeam")).toBool() ? 1 : 0;
    int bFree = b.value(QLatin1String("isFreeProvisioningTeam")).toBool() ? 1 : 0;
    return aFree < bFree;
}

static QList<QVariantMap> provisioningTeams()
{
    const QSettings xcodeSettings(
        QDir::homePath() + QLatin1String("/Library/Preferences/com.apple.dt.Xcode.plist"),
        QSettings::NativeFormat);
    const QVariantMap teamMap = xcodeSettings.value(QLatin1String("IDEProvisioningTeams")).toMap();
    QList<QVariantMap> flatTeams;
    for (QVariantMap::const_iterator it = teamMap.begin(), end = teamMap.end(); it != end; ++it) {
        const QString emailAddress = it.key();
        QVariantMap team = it.value().toMap();
        team[QLatin1String("emailAddress")] = emailAddress;
        flatTeams.append(team);
    }

    // Sort teams so that Free Provisioning teams come last
    std::sort(flatTeams.begin(), flatTeams.end(), ::compareProvisioningTeams);
    return flatTeams;
}

bool
ProjectBuilderMakefileGenerator::writeMakeParts(QTextStream &t)
{
    ProStringList tmp;
    bool did_preprocess = false;

    //HEADER
    const int pbVersion = pbuilderVersion();
    ProStringList buildConfigGroups;
    buildConfigGroups << "PROJECT" << "TARGET";

    t << "// !$*UTF8*$!\n"
      << "{\n"
      << "\t" << writeSettings("archiveVersion", "1", SettingsNoQuote) << ";\n"
      << "\tclasses = {\n\t};\n"
      << "\t" << writeSettings("objectVersion", QString::number(pbVersion), SettingsNoQuote) << ";\n"
      << "\tobjects = {\n";

    //MAKE QMAKE equivelant
    if (!project->isActiveConfig("no_autoqmake")) {
        QString mkfile = pbx_dir + Option::dir_sep + "qt_makeqmake.mak";
        QFile mkf(mkfile);
        if(mkf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            writingUnixMakefileGenerator = true;
            debug_msg(1, "pbuilder: Creating file: %s", mkfile.toLatin1().constData());
            QTextStream mkt(&mkf);
            writeHeader(mkt);
            mkt << "QMAKE    = " << var("QMAKE_QMAKE") << endl;
            project->values("QMAKE_MAKE_QMAKE_EXTRA_COMMANDS")
                << "@echo 'warning: Xcode project has been regenerated, custom settings have been lost. " \
                   "Use CONFIG+=no_autoqmake to prevent this behavior in the future, " \
                   "at the cost of requiring manual project change tracking.'";
            writeMakeQmake(mkt);
            mkt.flush();
            mkf.close();
            writingUnixMakefileGenerator = false;
        }
        QString phase_key = keyFor("QMAKE_PBX_MAKEQMAKE_BUILDPHASE");
        mkfile = fileFixify(mkfile);
        project->values("QMAKE_PBX_PRESCRIPT_BUILDPHASES").append(phase_key);
        t << "\t\t" << phase_key << " = {\n"
          << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("files", ProStringList(), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXShellScriptBuildPhase", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", "Qt Qmake") << ";\n"
          << "\t\t\t" << writeSettings("shellPath", "/bin/sh") << ";\n"
          << "\t\t\t" << writeSettings("shellScript", "make -C " + IoUtils::shellQuoteUnix(Option::output_dir)
                                                      + " -f " + IoUtils::shellQuoteUnix(mkfile)) << ";\n"
          << "\t\t\t" << writeSettings("showEnvVarsInLog", "0") << ";\n"
          << "\t\t};\n";
    }

    // FIXME: Move all file resolving logic out of ProjectBuilderSources::files(), as it
    // doesn't have access to any of the information it needs to resolve relative paths.
    project->values("QMAKE_INTERNAL_INCLUDED_FILES").prepend(project->projectFile());

    // Since we can't fileFixify inside ProjectBuilderSources::files(), we resolve the absolute paths here
    project->values("QMAKE_INTERNAL_INCLUDED_FILES") = ProStringList(
                fileFixify(project->values("QMAKE_INTERNAL_INCLUDED_FILES").toQStringList(),
                           FileFixifyFromOutdir | FileFixifyAbsolute));

    //DUMP SOURCES
    QSet<QString> processedSources;
    QMap<QString, ProStringList> groups;
    QList<ProjectBuilderSources> sources;
    sources.append(ProjectBuilderSources("SOURCES", true));
    sources.append(ProjectBuilderSources("GENERATED_SOURCES", true));
    sources.append(ProjectBuilderSources("GENERATED_FILES"));
    sources.append(ProjectBuilderSources("HEADERS"));
    if(!project->isEmpty("QMAKE_EXTRA_COMPILERS")) {
        const ProStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
        for (ProStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
            if (project->isEmpty(ProKey(*it + ".output")))
                continue;
            ProStringList &inputs = project->values(ProKey(*it + ".input"));
            int input = 0;
            while (input < inputs.size()) {
                if (project->isEmpty(inputs.at(input).toKey())) {
                    ++input;
                    continue;
                }
                bool duplicate = false;
                bool isObj = project->values(ProKey(*it + ".CONFIG")).indexOf("no_link") == -1;
                if (!isObj) {
                    for (int i = 0; i < sources.size(); ++i) {
                        if (sources.at(i).keyName() == inputs.at(input)) {
                            duplicate = true;
                            break;
                        }
                    }
                }
                if (!duplicate) {
                    const ProStringList &outputs = project->values(ProKey(*it + ".variable_out"));
                    for(int output = 0; output < outputs.size(); ++output) {
                        if(outputs.at(output) != "OBJECT") {
                            isObj = false;
                            break;
                        }
                    }
                    sources.append(ProjectBuilderSources(inputs.at(input).toQString(), true,
                                                         (*it).toQString(), isObj));

                    if (isObj) {
                        inputs.removeAt(input);
                        continue;
                    }
                }

                ++input;
            }
        }
    }
    sources.append(ProjectBuilderSources("QMAKE_INTERNAL_INCLUDED_FILES"));
    for(int source = 0; source < sources.size(); ++source) {
        ProStringList &src_list = project->values(ProKey("QMAKE_PBX_" + sources.at(source).keyName()));
        ProStringList &root_group_list = project->values("QMAKE_PBX_GROUPS");

        const QStringList &files = fileFixify(sources.at(source).files(project),
                                              FileFixifyFromOutdir | FileFixifyAbsolute);
        for(int f = 0; f < files.count(); ++f) {
            QString file = files[f];
            if(!sources.at(source).compilerName().isNull() &&
               !verifyExtraCompiler(sources.at(source).compilerName(), file))
                continue;
            if(file.endsWith(Option::prl_ext))
                continue;
            if (processedSources.contains(file))
                continue;
            processedSources.insert(file);

            bool in_root = true;
            QString src_key = keyFor(file);

            QString name = file.split(Option::dir_sep).back();

            if (!project->isActiveConfig("flat")) {
                // Build group hierarchy for file references that match the source our build dir
                QString relativePath = fileFixify(file, FileFixifyToIndir | FileFixifyRelative);
                if (QDir::isRelativePath(relativePath) && relativePath.startsWith(QLatin1String("../")))
                    relativePath = fileFixify(file, FileFixifyRelative); // Try build dir

                if (QDir::isRelativePath(relativePath)
                        && !relativePath.startsWith(QLatin1String("../"))
                        && relativePath.indexOf(Option::dir_sep) != -1) {
                    QString last_grp("QMAKE_PBX_" + sources.at(source).groupName() + "_HEIR_GROUP");
                    QStringList dirs = relativePath.split(Option::dir_sep);
                    name = dirs.back();
                    dirs.pop_back(); //remove the file portion as it will be added via src_key
                    for(QStringList::Iterator dir_it = dirs.begin(); dir_it != dirs.end(); ++dir_it) {
                        QString new_grp(last_grp + Option::dir_sep + (*dir_it)), new_grp_key(keyFor(new_grp));
                        if(dir_it == dirs.begin()) {
                            if(!src_list.contains(new_grp_key))
                                src_list.append(new_grp_key);
                        } else {
                            if(!groups[last_grp].contains(new_grp_key))
                                groups[last_grp] += new_grp_key;
                        }
                        last_grp = new_grp;
                    }
                    if (groups[last_grp].contains(src_key))
                        continue;
                    groups[last_grp] += src_key;
                    in_root = false;
                }
            }
            if (in_root) {
                if (src_list.contains(src_key))
                    continue;
                src_list.append(src_key);
            }
            //source reference
            t << "\t\t" << src_key << " = {\n"
              << "\t\t\t" << writeSettings("isa", "PBXFileReference", SettingsNoQuote) << ";\n"
              << "\t\t\t" << writeSettings("path", file) << ";\n";
            if (name != file)
                t << "\t\t\t" << writeSettings("name", name) << ";\n";
            t << "\t\t\t" << writeSettings("sourceTree", "<absolute>") << ";\n";
            QString filetype = xcodeFiletypeForFilename(file);
            if (!filetype.isNull())
                t << "\t\t\t" << writeSettings("lastKnownFileType", filetype) << ";\n";
            t << "\t\t};\n";
            if (sources.at(source).isBuildable()) { //build reference
                QString build_key = keyFor(file + ".BUILDABLE");
                t << "\t\t" << build_key << " = {\n"
                  << "\t\t\t" << writeSettings("fileRef", src_key) << ";\n"
                  << "\t\t\t" << writeSettings("isa", "PBXBuildFile", SettingsNoQuote) << ";\n"
                  << "\t\t\tsettings = {\n"
                  << "\t\t\t\t" << writeSettings("ATTRIBUTES", ProStringList(), SettingsAsList, 5) << ";\n"
                  << "\t\t\t};\n"
                  << "\t\t};\n";
                if (sources.at(source).isObjectOutput(file))
                    project->values("QMAKE_PBX_OBJ").append(build_key);
            }
        }
        if(!src_list.isEmpty()) {
            QString group_key = keyFor(sources.at(source).groupName());
            if(root_group_list.indexOf(group_key) == -1)
                root_group_list += group_key;

            ProStringList &group = groups[sources.at(source).groupName()];
            for(int src = 0; src < src_list.size(); ++src) {
                if(group.indexOf(src_list.at(src)) == -1)
                    group += src_list.at(src);
            }
        }
    }
    for (QMap<QString, ProStringList>::Iterator grp_it = groups.begin(); grp_it != groups.end(); ++grp_it) {
        t << "\t\t" << keyFor(grp_it.key()) << " = {\n"
          << "\t\t\t" << writeSettings("isa", "PBXGroup", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("children", grp_it.value(), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("name", grp_it.key().section(Option::dir_sep, -1)) << ";\n"
          << "\t\t\t" << writeSettings("sourceTree", "<group>") << ";\n"
          << "\t\t};\n";
    }

    //PREPROCESS BUILDPHASE (just a makefile)
    {
        QString mkfile = pbx_dir + Option::dir_sep + "qt_preprocess.mak";
        QFile mkf(mkfile);
        if(mkf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            writingUnixMakefileGenerator = true;
            did_preprocess = true;
            debug_msg(1, "pbuilder: Creating file: %s", mkfile.toLatin1().constData());
            QTextStream mkt(&mkf);
            writeHeader(mkt);
            mkt << "MOC       = " << var("QMAKE_MOC") << endl;
            mkt << "UIC       = " << var("QMAKE_UIC") << endl;
            mkt << "LEX       = " << var("QMAKE_LEX") << endl;
            mkt << "LEXFLAGS  = " << var("QMAKE_LEXFLAGS") << endl;
            mkt << "YACC      = " << var("QMAKE_YACC") << endl;
            mkt << "YACCFLAGS = " << var("QMAKE_YACCFLAGS") << endl;
            mkt << "DEFINES       = "
                << varGlue("PRL_EXPORT_DEFINES","-D"," -D"," ")
                << varGlue("DEFINES","-D"," -D","") << endl;
            mkt << "INCPATH       =";
            {
                const ProStringList &incs = project->values("INCLUDEPATH");
                for (ProStringList::ConstIterator incit = incs.begin(); incit != incs.end(); ++incit)
                    mkt << " -I" << escapeFilePath((*incit));
            }
            if(!project->isEmpty("QMAKE_FRAMEWORKPATH_FLAGS"))
               mkt << " " << var("QMAKE_FRAMEWORKPATH_FLAGS");
            mkt << endl;
            mkt << "DEL_FILE  = " << var("QMAKE_DEL_FILE") << endl;
            mkt << "MOVE      = " << var("QMAKE_MOVE") << endl << endl;
            mkt << "preprocess: compilers\n";
            mkt << "clean preprocess_clean: compiler_clean\n\n";
            writeExtraTargets(mkt);
            if(!project->isEmpty("QMAKE_EXTRA_COMPILERS")) {
                mkt << "compilers:";
                const ProStringList &compilers = project->values("QMAKE_EXTRA_COMPILERS");
                for(int compiler = 0; compiler < compilers.size(); ++compiler) {
                    const ProStringList &tmp_out = project->values(ProKey(compilers.at(compiler) + ".output"));
                    if (tmp_out.isEmpty())
                        continue;
                    const ProStringList &inputs = project->values(ProKey(compilers.at(compiler) + ".input"));
                    for(int input = 0; input < inputs.size(); ++input) {
                        const ProStringList &files = project->values(inputs.at(input).toKey());
                        if (files.isEmpty())
                            continue;
                        for(int file = 0, added = 0; file < files.size(); ++file) {
                            QString fn = files.at(file).toQString();
                            if (!verifyExtraCompiler(compilers.at(compiler), fn))
                                continue;
                            if(added && !(added % 3))
                                mkt << "\\\n\t";
                            ++added;
                            const QString file_name = fileFixify(fn, FileFixifyFromOutdir);
                            const QString tmpOut = fileFixify(tmp_out.first().toQString(), FileFixifyFromOutdir);
                            mkt << ' ' << escapeDependencyPath(Option::fixPathToTargetOS(
                                    replaceExtraCompilerVariables(tmpOut, file_name, QString(), NoShell)));
                        }
                    }
                }
                mkt << endl;
                writeExtraCompilerTargets(mkt);
                writingUnixMakefileGenerator = false;
            }
            mkt.flush();
            mkf.close();
        }
        mkfile = fileFixify(mkfile);
        QString phase_key = keyFor("QMAKE_PBX_PREPROCESS_TARGET");
//        project->values("QMAKE_PBX_BUILDPHASES").append(phase_key);
        project->values("QMAKE_PBX_PRESCRIPT_BUILDPHASES").append(phase_key);
        t << "\t\t" << phase_key << " = {\n"
          << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("files", ProStringList(), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXShellScriptBuildPhase", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", "Qt Preprocessors") << ";\n"
          << "\t\t\t" << writeSettings("shellPath", "/bin/sh") << ";\n"
          << "\t\t\t" << writeSettings("shellScript", "make -C " + IoUtils::shellQuoteUnix(Option::output_dir)
                                                      + " -f " + IoUtils::shellQuoteUnix(mkfile)) << ";\n"
          << "\t\t\t" << writeSettings("showEnvVarsInLog", "0") << ";\n"
          << "\t\t};\n";
   }

    //SOURCE BUILDPHASE
    if(!project->isEmpty("QMAKE_PBX_OBJ")) {
        QString grp = "Compile Sources", key = keyFor(grp);
        project->values("QMAKE_PBX_BUILDPHASES").append(key);
        t << "\t\t" << key << " = {\n"
          << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("files", fixListForOutput("QMAKE_PBX_OBJ"), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXSourcesBuildPhase", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", grp) << ";\n"
          << "\t\t};\n";
    }

    if(!project->isActiveConfig("staticlib")) { //DUMP LIBRARIES
        ProStringList &libdirs = project->values("QMAKE_PBX_LIBPATHS"),
              &frameworkdirs = project->values("QMAKE_FRAMEWORKPATH");
        static const char * const libs[] = { "QMAKE_LIBS", "QMAKE_LIBS_PRIVATE", nullptr };
        for (int i = 0; libs[i]; i++) {
            tmp = project->values(libs[i]);
            for(int x = 0; x < tmp.count();) {
                bool remove = false;
                QString library, name;
                ProString opt = tmp[x];
                if(opt.startsWith("-L")) {
                    QString r = opt.mid(2).toQString();
                    fixForOutput(r);
                    libdirs.append(r);
                } else if(opt.startsWith("-l")) {
                    name = opt.mid(2).toQString();
                    QString lib("lib" + name);
                    for (ProStringList::Iterator lit = libdirs.begin(); lit != libdirs.end(); ++lit) {
                        if(project->isActiveConfig("link_prl")) {
                            /* This isn't real nice, but it is real useful. This looks in a prl
                               for what the library will ultimately be called so we can stick it
                               in the ProjectFile. If the prl format ever changes (not likely) then
                               this will not really work. However, more concerning is that it will
                               encode the version number in the Project file which might be a bad
                               things in days to come? --Sam
                            */
                            QString lib_file = QMakeMetaInfo::checkLib(Option::normalizePath(
                                        (*lit) + Option::dir_sep + lib + Option::prl_ext));
                            if (!lib_file.isEmpty()) {
                                QMakeMetaInfo libinfo(project);
                                if(libinfo.readLib(lib_file)) {
                                    if(!libinfo.isEmpty("QMAKE_PRL_TARGET")) {
                                        library = (*lit) + Option::dir_sep + libinfo.first("QMAKE_PRL_TARGET");
                                        debug_msg(1, "pbuilder: Found library (%s) via PRL %s (%s)",
                                                  opt.toLatin1().constData(), lib_file.toLatin1().constData(), library.toLatin1().constData());
                                        remove = true;

                                        if (project->isActiveConfig("xcode_dynamic_library_suffix")) {
                                            QString suffixSetting = project->first("QMAKE_XCODE_LIBRARY_SUFFIX_SETTING").toQString();
                                            if (!suffixSetting.isEmpty()) {
                                                QString librarySuffix = project->first("QMAKE_XCODE_LIBRARY_SUFFIX").toQString();
                                                suffixSetting = "$(" + suffixSetting + ")";
                                                if (!librarySuffix.isEmpty()) {
                                                    int pos = library.lastIndexOf(librarySuffix + '.');
                                                    if (pos == -1) {
                                                        warn_msg(WarnLogic, "Failed to find expected suffix '%s' for library '%s'.",
                                                                            qPrintable(librarySuffix), qPrintable(library));
                                                    } else {
                                                        library.replace(pos, librarySuffix.length(), suffixSetting);
                                                        if (name.endsWith(librarySuffix))
                                                            name.chop(librarySuffix.length());
                                                    }
                                                } else {
                                                    int pos = library.lastIndexOf(name);
                                                    if (pos != -1)
                                                        library.insert(pos + name.length(), suffixSetting);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if(!remove) {
                            QString extns[] = { ".dylib", ".so", ".a", QString() };
                            for(int n = 0; !remove && !extns[n].isNull(); n++) {
                                QString tmp =  (*lit) + Option::dir_sep + lib + extns[n];
                                if(exists(tmp)) {
                                    library = tmp;
                                    debug_msg(1, "pbuilder: Found library (%s) via %s",
                                              opt.toLatin1().constData(), library.toLatin1().constData());
                                    remove = true;
                                }
                            }
                        }
                    }
                } else if(opt.startsWith("-F")) {
                    QString r;
                    if(opt.size() > 2) {
                        r = opt.mid(2).toQString();
                    } else {
                        if(x == tmp.count()-1)
                            break;
                        r = tmp[++x].toQString();
                    }
                    if(!r.isEmpty()) {
                        fixForOutput(r);
                        frameworkdirs.append(r);
                    }
                } else if(opt == "-framework") {
                    if(x == tmp.count()-1)
                        break;
                    const ProString &framework = tmp[x+1];
                    ProStringList fdirs = frameworkdirs;
                    fdirs << "/System/Library/Frameworks/" << "/Library/Frameworks/";
                    for(int fdir = 0; fdir < fdirs.count(); fdir++) {
                        if(exists(fdirs[fdir] + QDir::separator() + framework + ".framework")) {
                            remove = true;
                            library = fdirs[fdir] + Option::dir_sep + framework + ".framework";
                            tmp.removeAt(x);
                            break;
                        }
                    }
                } else if (!opt.startsWith('-')) {
                    QString fn = opt.toQString();
                    if (exists(fn)) {
                        remove = true;
                        library = fn;
                    }
                }
                if(!library.isEmpty()) {
                    const int slsh = library.lastIndexOf(Option::dir_sep);
                    if(name.isEmpty()) {
                        if(slsh != -1)
                            name = library.right(library.length() - slsh - 1);
                    }
                    if(slsh != -1) {
                        const QString path = QFileInfo(library.left(slsh)).absoluteFilePath();
                        if(!path.isEmpty() && !libdirs.contains(path))
                            libdirs += path;
                    }
                    library = fileFixify(library, FileFixifyFromOutdir | FileFixifyAbsolute);
                    QString key = keyFor(library);
                    if (!project->values("QMAKE_PBX_LIBRARIES").contains(key)) {
                        bool is_frmwrk = (library.endsWith(".framework"));
                        t << "\t\t" << key << " = {\n"
                          << "\t\t\t" << writeSettings("isa", "PBXFileReference", SettingsNoQuote) << ";\n"
                          << "\t\t\t" << writeSettings("name", name) << ";\n"
                          << "\t\t\t" << writeSettings("path", library) << ";\n"
                          << "\t\t\t" << writeSettings("refType", QString::number(reftypeForFile(library)), SettingsNoQuote) << ";\n"
                          << "\t\t\t" << writeSettings("sourceTree", "<absolute>") << ";\n";
                        if (is_frmwrk)
                            t << "\t\t\t" << writeSettings("lastKnownFileType", "wrapper.framework") << ";\n";
                        t << "\t\t};\n";
                        project->values("QMAKE_PBX_LIBRARIES").append(key);
                        QString build_key = keyFor(library + ".BUILDABLE");
                        t << "\t\t" << build_key << " = {\n"
                          << "\t\t\t" << writeSettings("fileRef", key) << ";\n"
                          << "\t\t\t" << writeSettings("isa", "PBXBuildFile", SettingsNoQuote) << ";\n"
                          << "\t\t\tsettings = {\n"
                          << "\t\t\t};\n"
                          << "\t\t};\n";
                        project->values("QMAKE_PBX_BUILD_LIBRARIES").append(build_key);
                    }
                }
                if(remove)
                    tmp.removeAt(x);
                else
                    x++;
            }
            project->values(libs[i]) = tmp;
        }
    }
    //SUBLIBS BUILDPHASE (just another makefile)
    if(!project->isEmpty("SUBLIBS")) {
        QString mkfile = pbx_dir + Option::dir_sep + "qt_sublibs.mak";
        QFile mkf(mkfile);
        if(mkf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            writingUnixMakefileGenerator = true;
            debug_msg(1, "pbuilder: Creating file: %s", mkfile.toLatin1().constData());
            QTextStream mkt(&mkf);
            writeHeader(mkt);
            mkt << "SUBLIBS= ";
            // ### This is missing the parametrization found in unixmake2.cpp
            tmp = project->values("SUBLIBS");
            for(int i = 0; i < tmp.count(); i++)
                t << escapeFilePath("tmp/lib" + tmp[i] + ".a") << ' ';
            t << endl << endl;
            mkt << "sublibs: $(SUBLIBS)\n\n";
            tmp = project->values("SUBLIBS");
            for(int i = 0; i < tmp.count(); i++)
                t << escapeFilePath("tmp/lib" + tmp[i] + ".a") + ":\n\t"
                  << var(ProKey("MAKELIB" + tmp[i])) << endl << endl;
            mkt.flush();
            mkf.close();
            writingUnixMakefileGenerator = false;
        }
        QString phase_key = keyFor("QMAKE_PBX_SUBLIBS_BUILDPHASE");
        mkfile = fileFixify(mkfile);
        project->values("QMAKE_PBX_PRESCRIPT_BUILDPHASES").append(phase_key);
        t << "\t\t" << phase_key << " = {\n"
          << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("files", ProStringList(), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXShellScriptBuildPhase", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", "Qt Sublibs") << ";\n"
          << "\t\t\t" << writeSettings("shellPath", "/bin/sh") << "\n"
          << "\t\t\t" << writeSettings("shellScript", "make -C " + IoUtils::shellQuoteUnix(Option::output_dir)
                                                      + " -f " + IoUtils::shellQuoteUnix(mkfile)) << ";\n"
          << "\t\t\t" << writeSettings("showEnvVarsInLog", "0") << ";\n"
          << "\t\t};\n";
    }

    if (!project->isEmpty("QMAKE_PRE_LINK")) {
        QString phase_key = keyFor("QMAKE_PBX_PRELINK_BUILDPHASE");
        project->values("QMAKE_PBX_BUILDPHASES").append(phase_key);

        ProStringList inputPaths;
        ProStringList outputPaths;
        const ProStringList &archs = project->values("QMAKE_XCODE_ARCHS");
        if (!archs.isEmpty()) {
            const int size = archs.size();
            inputPaths.reserve(size);
            outputPaths.reserve(size);
            for (int i = 0; i < size; ++i) {
                const ProString &arch = archs.at(i);
                inputPaths << "$(OBJECT_FILE_DIR_$(CURRENT_VARIANT))/" + arch + "/";
                outputPaths << "$(LINK_FILE_LIST_$(CURRENT_VARIANT)_" + arch + ")";
            }
        } else {
            inputPaths << "$(OBJECT_FILE_DIR_$(CURRENT_VARIANT))/$(CURRENT_ARCH)/";
            outputPaths << "$(LINK_FILE_LIST_$(CURRENT_VARIANT)_$(CURRENT_ARCH))";
        }

        t << "\t\t" << phase_key << " = {\n"
          << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("files", ProStringList(), SettingsAsList, 4) << ";\n"
          // The build phases are not executed in the order they are defined, but by their
          // resolved dependenices, so we have to ensure that this phase is run after the
          // compilation phase, and before the link phase. Making the phase depend on the
          // object file directory, and "write" to the list of files to link achieves that.
          << "\t\t\t" << writeSettings("inputPaths", inputPaths, SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("outputPaths", outputPaths, SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXShellScriptBuildPhase", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", "Qt Prelink") << ";\n"
          << "\t\t\t" << writeSettings("shellPath", "/bin/sh") << ";\n"
          << "\t\t\t" << writeSettings("shellScript", project->values("QMAKE_PRE_LINK")) << ";\n"
          << "\t\t\t" << writeSettings("showEnvVarsInLog", "0") << ";\n"
          << "\t\t};\n";
    }

    //LIBRARY BUILDPHASE
    if(!project->isEmpty("QMAKE_PBX_LIBRARIES")) {
        tmp = project->values("QMAKE_PBX_LIBRARIES");
        if(!tmp.isEmpty()) {
            QString grp("Frameworks"), key = keyFor(grp);
            project->values("QMAKE_PBX_GROUPS").append(key);
            t << "\t\t" << key << " = {\n"
              << "\t\t\t" << writeSettings("children", project->values("QMAKE_PBX_LIBRARIES"), SettingsAsList, 4) << ";\n"
              << "\t\t\t" << writeSettings("isa", "PBXGroup", SettingsNoQuote) << ";\n"
              << "\t\t\t" << writeSettings("name", grp) << ";\n"
              << "\t\t\t" << writeSettings("sourceTree", "<group>") << ";\n"
              << "\t\t};\n";
        }
    }
    {
        QString grp("Link Binary With Libraries"), key = keyFor(grp);
        project->values("QMAKE_PBX_BUILDPHASES").append(key);
        t << "\t\t" << key << " = {\n"
          << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("files", project->values("QMAKE_PBX_BUILD_LIBRARIES"), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXFrameworksBuildPhase", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", grp) << ";\n"
          << "\t\t};\n";
    }

    if (!project->isEmpty("QMAKE_POST_LINK")) {
        QString phase_key = keyFor("QMAKE_PBX_POSTLINK_BUILDPHASE");
        project->values("QMAKE_PBX_BUILDPHASES").append(phase_key);
        t << "\t\t" << phase_key << " = {\n"
          << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("files", ProStringList(), SettingsAsList, 4) << ";\n"
          // The build phases are not executed in the order they are defined, but by their
          // resolved dependenices, so we have to ensure the phase is run after linking.
          << "\t\t\t" << writeSettings("inputPaths", ProStringList("$(TARGET_BUILD_DIR)/$(EXECUTABLE_PATH)"), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXShellScriptBuildPhase", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", "Qt Postlink") << ";\n"
          << "\t\t\t" << writeSettings("shellPath", "/bin/sh") << ";\n"
          << "\t\t\t" << writeSettings("shellScript", project->values("QMAKE_POST_LINK")) << ";\n"
          << "\t\t\t" << writeSettings("showEnvVarsInLog", "0") << ";\n"
          << "\t\t};\n";
    }

    if (!project->isEmpty("DESTDIR")) {
        QString phase_key = keyFor("QMAKE_PBX_TARGET_COPY_PHASE");
        QString destDir = fileFixify(project->first("DESTDIR").toQString(),
                                     FileFixifyFromOutdir | FileFixifyAbsolute);
        project->values("QMAKE_PBX_BUILDPHASES").append(phase_key);
        t << "\t\t" << phase_key << " = {\n"
          << "\t\t\t" << writeSettings("isa", "PBXShellScriptBuildPhase", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", "Project Copy") << ";\n"
          << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("files", ProStringList(), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("inputPaths", ProStringList(), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("outputPaths", ProStringList(), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("shellPath", "/bin/sh") << ";\n"
          << "\t\t\t" << writeSettings("shellScript", fixForOutput("cp -r $BUILT_PRODUCTS_DIR/$FULL_PRODUCT_NAME " + IoUtils::shellQuoteUnix(destDir))) << ";\n"
          << "\t\t\t" << writeSettings("showEnvVarsInLog", "0") << ";\n"
          << "\t\t};\n";
    }
    bool copyBundleResources = project->isActiveConfig("app_bundle") && project->first("TEMPLATE") == "app";
    ProStringList bundle_resources_files;
    ProStringList embedded_frameworks;
    QMap<ProString, ProStringList> embedded_plugins;
    // Copy Bundle Data
    if (!project->isEmpty("QMAKE_BUNDLE_DATA")) {
        ProStringList bundle_file_refs;
        bool osx = project->isActiveConfig("osx");

        //all bundle data
        const ProStringList &bundle_data = project->values("QMAKE_BUNDLE_DATA");
        for(int i = 0; i < bundle_data.count(); i++) {
            ProStringList bundle_files;
            ProString path = project->first(ProKey(bundle_data[i] + ".path"));
            const bool isEmbeddedFramework = ((!osx && path == QLatin1String("Frameworks"))
                || (osx && path == QLatin1String("Contents/Frameworks")));
            const ProString pluginsPrefix = ProString(osx ? QLatin1String("Contents/PlugIns") : QLatin1String("PlugIns"));
            const bool isEmbeddedPlugin = (path == pluginsPrefix) || path.startsWith(pluginsPrefix + "/");

            //all files
            const ProStringList &files = project->values(ProKey(bundle_data[i] + ".files"));
            for(int file = 0; file < files.count(); file++) {
                QString fn = fileFixify(files[file].toQString(), FileFixifyAbsolute);
                QString name = fn.split(Option::dir_sep).back();
                QString file_ref_key = keyFor("QMAKE_PBX_BUNDLE_DATA_FILE_REF." + bundle_data[i] + "-" + fn);
                bundle_file_refs += file_ref_key;
                t << "\t\t" << file_ref_key << " = {\n"
                  << "\t\t\t" << writeSettings("isa", "PBXFileReference", SettingsNoQuote) << ";\n"
                  << "\t\t\t" << writeSettings("path", fn) << ";\n"
                  << "\t\t\t" << writeSettings("name", name) << ";\n"
                  << "\t\t\t" << writeSettings("sourceTree", "<absolute>") << ";\n"
                  << "\t\t};\n";
                QString file_key = keyFor("QMAKE_PBX_BUNDLE_DATA_FILE." + bundle_data[i] + "-" + fn);
                bundle_files += file_key;
                t << "\t\t" <<  file_key << " = {\n"
                  << "\t\t\t" << writeSettings("fileRef", file_ref_key) << ";\n"
                  << "\t\t\t" << writeSettings("isa", "PBXBuildFile", SettingsNoQuote) << ";\n";
                if (isEmbeddedFramework || isEmbeddedPlugin || name.endsWith(".dylib") || name.endsWith(".framework"))
                    t << "\t\t\t" << writeSettings("settings", "{ATTRIBUTES = (CodeSignOnCopy, RemoveHeadersOnCopy, ); }", SettingsNoQuote) << ";\n";
                t << "\t\t};\n";
            }

            if (copyBundleResources && ((!osx && path.isEmpty())
                                        || (osx && path == QLatin1String("Contents/Resources")))) {
                for (const ProString &s : qAsConst(bundle_files))
                    bundle_resources_files << s;
            } else if (copyBundleResources && isEmbeddedFramework) {
                for (const ProString &s : qAsConst(bundle_files))
                    embedded_frameworks << s;
            } else if (copyBundleResources && isEmbeddedPlugin) {
                for (const ProString &s : qAsConst(bundle_files)) {
                    ProString subpath = (path == pluginsPrefix) ? ProString() : path.mid(pluginsPrefix.size() + 1);
                    embedded_plugins[subpath] << s;
                }
            } else {
                QString phase_key = keyFor("QMAKE_PBX_BUNDLE_COPY." + bundle_data[i]);
                //if (!project->isActiveConfig("shallow_bundle")
                //    && !project->isEmpty(ProKey(bundle_data[i] + ".version"))) {
                //}

                project->values("QMAKE_PBX_BUILDPHASES").append(phase_key);
                t << "\t\t" << phase_key << " = {\n"
                  << "\t\t\t" << writeSettings("name", "Copy '" + bundle_data[i] + "' Files to Bundle") << ";\n"
                  << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
                  << "\t\t\t" << writeSettings("dstPath", path) << ";\n"
                  << "\t\t\t" << writeSettings("dstSubfolderSpec", "1", SettingsNoQuote) << ";\n"
                  << "\t\t\t" << writeSettings("isa", "PBXCopyFilesBuildPhase", SettingsNoQuote) << ";\n"
                  << "\t\t\t" << writeSettings("files", bundle_files, SettingsAsList, 4) << ";\n"
                  << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
                  << "\t\t};\n";
            }
        }

        QString bundle_data_key = keyFor("QMAKE_PBX_BUNDLE_DATA");
        project->values("QMAKE_PBX_GROUPS").append(bundle_data_key);
        t << "\t\t" << bundle_data_key << " = {\n"
          << "\t\t\t" << writeSettings("children", bundle_file_refs, SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXGroup", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", "Bundle Data") << ";\n"
          << "\t\t\t" << writeSettings("sourceTree", "<group>") << ";\n"
          << "\t\t};\n";
    }

    // Copy bundle resources. Optimizes resources, and puts them into the Resources
    // subdirectory on OSX, but doesn't support paths.
    if (copyBundleResources) {
        if (!project->isEmpty("ICON")) {
            ProString icon = project->first("ICON");
            bundle_resources_files += keyFor(icon + ".BUILDABLE");
        }

        // Always add "Copy Bundle Resources" phase, even when we have no bundle
        // resources, since Xcode depends on it being there for e.g asset catalogs.
        QString grp("Copy Bundle Resources"), key = keyFor(grp);
        project->values("QMAKE_PBX_BUILDPHASES").append(key);
        t << "\t\t" << key << " = {\n"
          << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("files", bundle_resources_files, SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXResourcesBuildPhase", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", grp) << ";\n"
          << "\t\t};\n";

        QString grp2("Embed Frameworks"), key2 = keyFor(grp2);
        project->values("QMAKE_PBX_BUILDPHASES").append(key2);
        t << "\t\t" << key2 << " = {\n"
          << "\t\t\t" << writeSettings("isa", "PBXCopyFilesBuildPhase", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("dstPath", "") << ";\n"
          << "\t\t\t" << writeSettings("dstSubfolderSpec", "10", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("files", embedded_frameworks, SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("name", grp2) << ";\n"
          << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
          << "\t\t};\n";

        QMapIterator<ProString, ProStringList> it(embedded_plugins);
        while (it.hasNext()) {
            it.next();
            QString suffix = !it.key().isEmpty() ? (" (" + it.key() + ")") : QString();
            QString grp3("Embed PlugIns" + suffix), key3 = keyFor(grp3);
            project->values("QMAKE_PBX_BUILDPHASES").append(key3);
            t << "\t\t" << key3 << " = {\n"
              << "\t\t\t" << writeSettings("isa", "PBXCopyFilesBuildPhase", SettingsNoQuote) << ";\n"
              << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
              << "\t\t\t" << writeSettings("dstPath", it.key()) << ";\n"
              << "\t\t\t" << writeSettings("dstSubfolderSpec", "13", SettingsNoQuote) << ";\n"
              << "\t\t\t" << writeSettings("files", it.value(), SettingsAsList, 4) << ";\n"
              << "\t\t\t" << writeSettings("name", grp3) << ";\n"
              << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
              << "\t\t};\n";
        }
    }

    //REFERENCE
    project->values("QMAKE_PBX_PRODUCTS").append(keyFor(pbx_dir + "QMAKE_PBX_REFERENCE"));
    t << "\t\t" << keyFor(pbx_dir + "QMAKE_PBX_REFERENCE") << " = {\n"
      << "\t\t\t" << writeSettings("isa",  "PBXFileReference", SettingsNoQuote) << ";\n"
      << "\t\t\t" << writeSettings("includeInIndex",  "0", SettingsNoQuote) << ";\n";
    if(project->first("TEMPLATE") == "app") {
        ProString targ = project->first("QMAKE_ORIG_TARGET");
        if(project->isActiveConfig("bundle") && !project->isEmpty("QMAKE_BUNDLE_EXTENSION")) {
            if(!project->isEmpty("QMAKE_BUNDLE_NAME"))
                targ = project->first("QMAKE_BUNDLE_NAME");
            targ += project->first("QMAKE_BUNDLE_EXTENSION");
            if(!project->isEmpty("QMAKE_PBX_BUNDLE_TYPE"))
                t << "\t\t\t" << writeSettings("explicitFileType", project->first("QMAKE_PBX_BUNDLE_TYPE")) + ";\n";
        } else if(project->isActiveConfig("app_bundle")) {
            if(!project->isEmpty("QMAKE_APPLICATION_BUNDLE_NAME"))
                targ = project->first("QMAKE_APPLICATION_BUNDLE_NAME");
            targ += ".app";
            t << "\t\t\t" << writeSettings("explicitFileType", "wrapper.application") << ";\n";
        } else {
            t << "\t\t\t" << writeSettings("explicitFileType", "compiled.mach-o.executable") << ";\n";
        }
        t << "\t\t\t" << writeSettings("path", targ) << ";\n";
    } else {
        ProString lib = project->first("QMAKE_ORIG_TARGET");
        if(project->isActiveConfig("staticlib")) {
            lib = project->first("TARGET");
        } else if(!project->isActiveConfig("lib_bundle")) {
            if(project->isActiveConfig("plugin"))
                lib = project->first("TARGET");
            else
                lib = project->first("TARGET_");
        }
        int slsh = lib.lastIndexOf(Option::dir_sep);
        if(slsh != -1)
            lib = lib.right(lib.length() - slsh - 1);
        if(project->isActiveConfig("bundle") && !project->isEmpty("QMAKE_BUNDLE_EXTENSION")) {
            if(!project->isEmpty("QMAKE_BUNDLE_NAME"))
                lib = project->first("QMAKE_BUNDLE_NAME");
            lib += project->first("QMAKE_BUNDLE_EXTENSION");
            if(!project->isEmpty("QMAKE_PBX_BUNDLE_TYPE"))
                t << "\t\t\t" << writeSettings("explicitFileType", project->first("QMAKE_PBX_BUNDLE_TYPE")) << ";\n";
        } else if(!project->isActiveConfig("staticlib") && project->isActiveConfig("lib_bundle")) {
            if(!project->isEmpty("QMAKE_FRAMEWORK_BUNDLE_NAME"))
                lib = project->first("QMAKE_FRAMEWORK_BUNDLE_NAME");
            lib += ".framework";
            t << "\t\t\t" << writeSettings("explicitFileType", "wrapper.framework") << ";\n";
        } else {
            t << "\t\t\t" << writeSettings("explicitFileType", "compiled.mach-o.dylib") << ";\n";
        }
        t << "\t\t\t" << writeSettings("path", lib) << ";\n";
    }
    t  << "\t\t\t" << writeSettings("sourceTree", "BUILT_PRODUCTS_DIR", SettingsNoQuote) << ";\n"
      << "\t\t};\n";
    { //Products group
        QString grp("Products"), key = keyFor(grp);
        project->values("QMAKE_PBX_GROUPS").append(key);
        t << "\t\t" << key << " = {\n"
          << "\t\t\t" << writeSettings("children", project->values("QMAKE_PBX_PRODUCTS"), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXGroup", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", "Products") << ";\n"
          << "\t\t\t" << writeSettings("sourceTree", "<group>") << ";\n"
          << "\t\t};\n";
    }

    //DUMP EVERYTHING THAT TIES THE ABOVE TOGETHER
    //ROOT_GROUP
    t << "\t\t" << keyFor("QMAKE_PBX_ROOT_GROUP") << " = {\n"
      << "\t\t\t" << writeSettings("children", project->values("QMAKE_PBX_GROUPS"), SettingsAsList, 4) << ";\n"
      << "\t\t\t" << writeSettings("isa", "PBXGroup", SettingsNoQuote) << ";\n"
      << "\t\t\t" << writeSettings("name", project->first("QMAKE_ORIG_TARGET")) << ";\n"
      << "\t\t\t" << writeSettings("sourceTree", "<group>") << ";\n"
      << "\t\t};\n";

    {
        QString aggregate_target_key = keyFor(pbx_dir + "QMAKE_PBX_AGGREGATE_TARGET");
        project->values("QMAKE_PBX_TARGETS").append(aggregate_target_key);
        t << "\t\t" << aggregate_target_key << " = {\n"
          << "\t\t\t" << writeSettings("buildPhases", project->values("QMAKE_PBX_PRESCRIPT_BUILDPHASES"), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("dependencies", ProStringList(), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("buildConfigurationList", keyFor("QMAKE_PBX_BUILDCONFIG_LIST_TARGET"), SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXAggregateTarget", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("buildRules", ProStringList(), SettingsAsList) << ";\n"
          << "\t\t\t" << writeSettings("productName", "Qt Preprocess") << ";\n"
          << "\t\t\t" << writeSettings("name", "Qt Preprocess") << ";\n"
          << "\t\t};\n";

        QString aggregate_target_dep_key = keyFor(pbx_dir + "QMAKE_PBX_AGGREGATE_TARGET_DEP");
        t << "\t\t" << aggregate_target_dep_key  << " = {\n"
          << "\t\t\t" << writeSettings("isa", "PBXTargetDependency", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("target", aggregate_target_key) << ";\n"
          << "\t\t};\n";

        project->values("QMAKE_PBX_TARGET_DEPENDS").append(aggregate_target_dep_key);
    }

    //TARGET
    QString target_key = keyFor(pbx_dir + "QMAKE_PBX_TARGET");
    project->values("QMAKE_PBX_TARGETS").prepend(target_key);
    t << "\t\t" << target_key << " = {\n"
      << "\t\t\t" << writeSettings("buildPhases", project->values("QMAKE_PBX_BUILDPHASES"),
                                   SettingsAsList, 4) << ";\n";
    t << "\t\t\t" << writeSettings("dependencies", project->values("QMAKE_PBX_TARGET_DEPENDS"), SettingsAsList, 4) << ";\n"
      << "\t\t\t" << writeSettings("productReference", keyFor(pbx_dir + "QMAKE_PBX_REFERENCE")) << ";\n";
    t << "\t\t\t" << writeSettings("buildConfigurationList", keyFor("QMAKE_PBX_BUILDCONFIG_LIST_TARGET"), SettingsNoQuote) << ";\n";
    t << "\t\t\t" << writeSettings("isa", "PBXNativeTarget", SettingsNoQuote) << ";\n";
    t << "\t\t\t" << writeSettings("buildRules", ProStringList(), SettingsAsList) << ";\n";
    if(project->first("TEMPLATE") == "app") {
        if (!project->isEmpty("QMAKE_PBX_PRODUCT_TYPE")) {
            t << "\t\t\t" << writeSettings("productType", project->first("QMAKE_PBX_PRODUCT_TYPE")) << ";\n";
        } else {
            if (project->isActiveConfig("app_bundle"))
                t << "\t\t\t" << writeSettings("productType",  "com.apple.product-type.application") << ";\n";
            else
                t << "\t\t\t" << writeSettings("productType", "com.apple.product-type.tool") << ";\n";
        }
        t << "\t\t\t" << writeSettings("name", project->first("QMAKE_ORIG_TARGET")) << ";\n"
          << "\t\t\t" << writeSettings("productName", project->first("QMAKE_ORIG_TARGET")) << ";\n";
    } else {
        ProString lib = project->first("QMAKE_ORIG_TARGET");
        if(!project->isActiveConfig("lib_bundle") && !project->isActiveConfig("staticlib"))
           lib.prepend("lib");
        t << "\t\t\t" << writeSettings("name", lib) << ";\n"
          << "\t\t\t" << writeSettings("productName", lib) << ";\n";
        if (!project->isEmpty("QMAKE_PBX_PRODUCT_TYPE"))
            t << "\t\t\t" << writeSettings("productType", project->first("QMAKE_PBX_PRODUCT_TYPE")) << ";\n";
        else if (project->isActiveConfig("staticlib"))
            t << "\t\t\t" << writeSettings("productType", "com.apple.product-type.library.static") << ";\n";
        else if (project->isActiveConfig("lib_bundle"))
            t << "\t\t\t" << writeSettings("productType", "com.apple.product-type.framework") << ";\n";
        else
            t << "\t\t\t" << writeSettings("productType", "com.apple.product-type.library.dynamic") << ";\n";
    }
    if(!project->isEmpty("DESTDIR"))
        t << "\t\t\t" << writeSettings("productInstallPath", project->first("DESTDIR")) << ";\n";
    t << "\t\t};\n";

    // Test target for running Qt unit tests under XCTest
    if (project->isActiveConfig("testcase") && project->isActiveConfig("app_bundle")) {
        QString devNullFileReferenceKey = keyFor(pbx_dir + "QMAKE_PBX_DEV_NULL_FILE_REFERENCE");
        t << "\t\t" << devNullFileReferenceKey << " = {\n"
          << "\t\t\t" << writeSettings("isa", "PBXFileReference", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("name", "/dev/null") << ";\n"
          << "\t\t\t" << writeSettings("path", "/dev/null") << ";\n"
          << "\t\t\t" << writeSettings("lastKnownFileType", "sourcecode.c.c") << ";\n"
          << "\t\t\t" << writeSettings("sourceTree", "<absolute>") << ";\n"
          << "\t\t};\n";

        QString devNullBuildFileKey = keyFor(pbx_dir + "QMAKE_PBX_DEV_NULL_BUILD_FILE");
        t << "\t\t" << devNullBuildFileKey << " = {\n"
          << "\t\t\t" << writeSettings("fileRef", devNullFileReferenceKey) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXBuildFile", SettingsNoQuote) << ";\n"
          << "\t\t};\n";

        QString dummySourceBuildPhaseKey = keyFor(pbx_dir + "QMAKE_PBX_DUMMY_SOURCE_BUILD_PHASE");
        t << "\t\t" << dummySourceBuildPhaseKey << " = {\n"
          << "\t\t\t" << writeSettings("buildActionMask", "2147483647", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("files", devNullBuildFileKey, SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXSourcesBuildPhase", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("runOnlyForDeploymentPostprocessing", "0", SettingsNoQuote) << ";\n"
          << "\t\t};\n";

        ProStringList testBundleBuildConfigs;

        ProString targetName = project->first("QMAKE_ORIG_TARGET");
        ProString testHost = "$(BUILT_PRODUCTS_DIR)/" + targetName + ".app/";
        if (project->isActiveConfig("osx"))
            testHost.append("Contents/MacOS/");
        testHost.append(targetName);

        static const char * const configs[] = { "Debug", "Release", nullptr };
        for (int i = 0; configs[i]; i++) {
            QString testBundleBuildConfig = keyFor(pbx_dir + "QMAKE_PBX_TEST_BUNDLE_BUILDCONFIG_" + configs[i]);
            t << "\t\t" << testBundleBuildConfig << " = {\n"
              << "\t\t\t" << writeSettings("isa", "XCBuildConfiguration", SettingsNoQuote) << ";\n"
              << "\t\t\tbuildSettings = {\n"
              << "\t\t\t\t" << writeSettings("INFOPLIST_FILE", project->first("QMAKE_XCODE_SPECDIR") + "/QtTest.plist") << ";\n"
              << "\t\t\t\t" << writeSettings("OTHER_LDFLAGS", "") << ";\n"
              << "\t\t\t\t" << writeSettings("TEST_HOST", testHost) << ";\n"
              << "\t\t\t\t" << writeSettings("DEBUG_INFORMATION_FORMAT", "dwarf-with-dsym") << ";\n"
              << "\t\t\t};\n"
              << "\t\t\t" << writeSettings("name", configs[i], SettingsNoQuote) << ";\n"
              << "\t\t};\n";

              testBundleBuildConfigs.append(testBundleBuildConfig);
        }

        QString testBundleBuildConfigurationListKey = keyFor(pbx_dir + "QMAKE_PBX_TEST_BUNDLE_BUILDCONFIG_LIST");
        t << "\t\t" << testBundleBuildConfigurationListKey << " = {\n"
          << "\t\t\t" << writeSettings("isa", "XCConfigurationList", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("buildConfigurations", testBundleBuildConfigs, SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("defaultConfigurationIsVisible", "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("defaultConfigurationName", "Debug", SettingsNoQuote) << ";\n"
          << "\t\t};\n";

        QString primaryTargetDependencyKey = keyFor(pbx_dir + "QMAKE_PBX_PRIMARY_TARGET_DEP");
        t << "\t\t" << primaryTargetDependencyKey  << " = {\n"
          << "\t\t\t" << writeSettings("isa", "PBXTargetDependency", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("target", keyFor(pbx_dir + "QMAKE_PBX_TARGET")) << ";\n"
          << "\t\t};\n";

        QString testBundleReferenceKey = keyFor("QMAKE_TEST_BUNDLE_REFERENCE");
        t << "\t\t" << testBundleReferenceKey << " = {\n"
          << "\t\t\t" << writeSettings("isa",  "PBXFileReference", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("explicitFileType", "wrapper.cfbundle") << ";\n"
          << "\t\t\t" << writeSettings("includeInIndex",  "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("sourceTree", "BUILT_PRODUCTS_DIR", SettingsNoQuote) << ";\n"
          << "\t\t};\n";

        QString testTargetKey = keyFor(pbx_dir + "QMAKE_PBX_TEST_TARGET");
        project->values("QMAKE_PBX_TARGETS").append(testTargetKey);
        t << "\t\t" << testTargetKey << " = {\n"
          << "\t\t\t" << writeSettings("buildPhases", dummySourceBuildPhaseKey, SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("dependencies", primaryTargetDependencyKey, SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("buildConfigurationList", testBundleBuildConfigurationListKey) << ";\n"
          << "\t\t\t" << writeSettings("productType", "com.apple.product-type.bundle.unit-test") << ";\n"
          << "\t\t\t" << writeSettings("isa", "PBXNativeTarget", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("productReference", testBundleReferenceKey) << ";\n"
          << "\t\t\t" << writeSettings("name", "Qt Test") << ";\n"
          << "\t\t};\n";

        QLatin1Literal testTargetID("TestTargetID");
        project->values(ProKey("QMAKE_PBX_TARGET_ATTRIBUTES_" + testTargetKey + "_" + testTargetID)).append(keyFor(pbx_dir + "QMAKE_PBX_TARGET"));
        project->values(ProKey("QMAKE_PBX_TARGET_ATTRIBUTES_" + testTargetKey)).append(ProKey(testTargetID));
    }

    //DEBUG/RELEASE
    QString defaultConfig;
    for(int as_release = 0; as_release < 2; as_release++)
    {
        QString configName = (as_release ? "Release" : "Debug");

        QMap<QString, QString> settings;
        if (!project->isActiveConfig("no_xcode_development_team")) {
            QString teamId;
            if (!project->isEmpty("QMAKE_DEVELOPMENT_TEAM")) {
                teamId = project->first("QMAKE_DEVELOPMENT_TEAM").toQString();
            } else {
                const QList<QVariantMap> teams = provisioningTeams();
                if (!teams.isEmpty()) // first suitable team we find is the one we'll use by default
                    teamId = teams.first().value(QLatin1String("teamID")).toString();
            }
            if (!teamId.isEmpty())
                settings.insert("DEVELOPMENT_TEAM", teamId);
            if (!project->isEmpty("QMAKE_PROVISIONING_PROFILE"))
                settings.insert("PROVISIONING_PROFILE_SPECIFIER", project->first("QMAKE_PROVISIONING_PROFILE").toQString());
        }

        settings.insert("APPLICATION_EXTENSION_API_ONLY", project->isActiveConfig("app_extension_api_only") ? "YES" : "NO");
        // required for tvOS (and watchos), optional on iOS (deployment target >= iOS 6.0)
        settings.insert("ENABLE_BITCODE", project->isActiveConfig("bitcode") ? "YES" : "NO");
        if(!as_release)
            settings.insert("GCC_OPTIMIZATION_LEVEL", "0");
        if(project->isActiveConfig("sdk") && !project->isEmpty("QMAKE_MAC_SDK"))
                settings.insert("SDKROOT", project->first("QMAKE_MAC_SDK").toQString());
        {
            const ProStringList &l = project->values("QMAKE_MAC_XCODE_SETTINGS");
            for(int i = 0; i < l.size(); ++i) {
                ProString name = l.at(i);
                const ProKey buildKey(name + ".build");
                if (!project->isEmpty(buildKey)) {
                    const QString build = project->first(buildKey).toQString();
                    if (build.toLower() != configName.toLower())
                        continue;
                }
                const QString value = project->values(ProKey(name + ".value")).join(QString(Option::field_sep));
                const ProKey nkey(name + ".name");
                if (!project->isEmpty(nkey))
                    name = project->first(nkey);
                settings.insert(name.toQString(), value);
            }
        }
        if (project->first("TEMPLATE") == "app") {
            settings.insert("PRODUCT_NAME", fixForOutput(project->first("QMAKE_ORIG_TARGET").toQString()));
        } else {
            ProString lib = project->first("QMAKE_ORIG_TARGET");
            if (!project->isActiveConfig("lib_bundle") && !project->isActiveConfig("staticlib"))
                lib.prepend("lib");
            settings.insert("PRODUCT_NAME", lib.toQString());
        }

        if (project->isActiveConfig("debug") != (bool)as_release)
            defaultConfig = configName;
        for (int i = 0; i < buildConfigGroups.size(); i++) {
            QString key = keyFor("QMAKE_PBX_BUILDCONFIG_" + configName + buildConfigGroups.at(i));
            project->values(ProKey("QMAKE_PBX_BUILDCONFIGS_" + buildConfigGroups.at(i))).append(key);
            t << "\t\t" << key << " = {\n"
              << "\t\t\t" << writeSettings("isa", "XCBuildConfiguration", SettingsNoQuote) << ";\n"
              << "\t\t\tbuildSettings = {\n";
            for (QMap<QString, QString>::Iterator set_it = settings.begin(); set_it != settings.end(); ++set_it)
                t << "\t\t\t\t" << writeSettings(set_it.key(), set_it.value()) << ";\n";
            if (buildConfigGroups.at(i) == QLatin1String("PROJECT")) {
                if (!project->isEmpty("QMAKE_XCODE_GCC_VERSION"))
                    t << "\t\t\t\t" << writeSettings("GCC_VERSION", project->first("QMAKE_XCODE_GCC_VERSION"), SettingsNoQuote) << ";\n";
                ProString program = project->first("QMAKE_CC");
                if (!program.isEmpty())
                    t << "\t\t\t\t" << writeSettings("CC", fixForOutput(findProgram(program))) << ";\n";
                program = project->first("QMAKE_CXX");
                // Xcode will automatically take care of using CC with the right -x option,
                // and will actually break if we pass CPLUSPLUS, by adding an additional set of "++"
                if (!program.isEmpty() && !program.contains("clang++"))
                    t << "\t\t\t\t" << writeSettings("CPLUSPLUS", fixForOutput(findProgram(program))) << ";\n";
                program = project->first("QMAKE_LINK");
                if (!program.isEmpty())
                    t << "\t\t\t\t" << writeSettings("LDPLUSPLUS", fixForOutput(findProgram(program))) << ";\n";

                if ((project->first("TEMPLATE") == "app" && project->isActiveConfig("app_bundle")) ||
                   (project->first("TEMPLATE") == "lib" && !project->isActiveConfig("staticlib") &&
                    project->isActiveConfig("lib_bundle"))) {
                    QString plist = fileFixify(project->first("QMAKE_INFO_PLIST").toQString(), FileFixifyToIndir);
                    if (!plist.isEmpty()) {
                        if (exists(plist))
                            t << "\t\t\t\t" << writeSettings("INFOPLIST_FILE", fileFixify(plist)) << ";\n";
                        else
                            warn_msg(WarnLogic, "Could not resolve Info.plist: '%s'. Check if QMAKE_INFO_PLIST points to a valid file.", plist.toLatin1().constData());
                    } else {
                        plist = fileFixify(specdir() + QDir::separator() + "Info.plist." + project->first("TEMPLATE"), FileFixifyBackwards);
                        QFile plist_in_file(plist);
                        if (plist_in_file.open(QIODevice::ReadOnly)) {
                            QTextStream plist_in(&plist_in_file);
                            QString plist_in_text = plist_in.readAll();
                            plist_in_text.replace(QLatin1String("@ICON@"),
                              (project->isEmpty("ICON") ? QString("") : project->first("ICON").toQString().section(Option::dir_sep, -1)));
                            if (project->first("TEMPLATE") == "app") {
                                ProString app_bundle_name = project->first("QMAKE_APPLICATION_BUNDLE_NAME");
                                if (app_bundle_name.isEmpty())
                                    app_bundle_name = project->first("QMAKE_ORIG_TARGET");
                                plist_in_text.replace(QLatin1String("@EXECUTABLE@"), app_bundle_name.toQString());
                            } else {
                                ProString lib_bundle_name = project->first("QMAKE_FRAMEWORK_BUNDLE_NAME");
                                if (lib_bundle_name.isEmpty())
                                    lib_bundle_name = project->first("QMAKE_ORIG_TARGET");
                                plist_in_text.replace(QLatin1String("@LIBRARY@"), lib_bundle_name.toQString());
                            }
                            QString bundlePrefix = project->first("QMAKE_TARGET_BUNDLE_PREFIX").toQString();
                            if (bundlePrefix.isEmpty())
                                bundlePrefix = "com.yourcompany";
                            plist_in_text.replace(QLatin1String("@BUNDLEIDENTIFIER@"), bundlePrefix + '.' + QLatin1String("${PRODUCT_NAME:rfc1034identifier}"));
                            if (!project->values("VERSION").isEmpty()) {
                                plist_in_text.replace(QLatin1String("@SHORT_VERSION@"), project->first("VER_MAJ") + "." + project->first("VER_MIN"));
                            }
                            plist_in_text.replace(QLatin1String("@TYPEINFO@"),
                                (project->isEmpty("QMAKE_PKGINFO_TYPEINFO")
                                    ? QString::fromLatin1("????") : project->first("QMAKE_PKGINFO_TYPEINFO").left(4).toQString()));
                            QFile plist_out_file(Option::output_dir + "/Info.plist");
                            if (plist_out_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                                QTextStream plist_out(&plist_out_file);
                                plist_out << plist_in_text;
                                t << "\t\t\t\t" << writeSettings("INFOPLIST_FILE", "Info.plist") << ";\n";
                            } else {
                                warn_msg(WarnLogic, "Could not write Info.plist: '%s'.", plist_out_file.fileName().toLatin1().constData());
                            }
                        } else {
                            warn_msg(WarnLogic, "Could not open Info.plist: '%s'.", plist.toLatin1().constData());
                        }
                    }
                }

                // The symroot is marked by xcodebuild as excluded from Time Machine
                // backups, as it's a temporary build dir, so we don't want it to be
                // the same as the possibe in-source dir, as that would leave out
                // sources from being backed up.
                t << "\t\t\t\t" << writeSettings("SYMROOT",
                    Option::output_dir + Option::dir_sep + ".xcode") << ";\n";

                // The configuration build dir however is not treated as excluded,
                // so we can safely point it to the root output dir.
                t << "\t\t\t\t" << writeSettings("CONFIGURATION_BUILD_DIR",
                    Option::output_dir + Option::dir_sep + "$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)") << ";\n";

                if (!project->isEmpty("DESTDIR")) {
                    ProString dir = project->first("DESTDIR");
                    if (QDir::isRelativePath(dir.toQString()))
                        dir.prepend(Option::output_dir + Option::dir_sep);
                    t << "\t\t\t\t" << writeSettings("INSTALL_DIR", dir) << ";\n";
                }

                if (project->first("TEMPLATE") == "lib")
                    t << "\t\t\t\t" << writeSettings("INSTALL_PATH", ProStringList()) << ";\n";

                if (!project->isEmpty("VERSION") && project->first("VERSION") != "0.0.0") {
                    t << "\t\t\t\t" << writeSettings("DYLIB_CURRENT_VERSION",  project->first("VER_MAJ")+"."+project->first("VER_MIN")+"."+project->first("VER_PAT")) << ";\n";
                    if (project->isEmpty("COMPAT_VERSION"))
                        t << "\t\t\t\t" << writeSettings("DYLIB_COMPATIBILITY_VERSION", project->first("VER_MAJ")+"."+project->first("VER_MIN")) << ";\n";
                    if (project->first("TEMPLATE") == "lib" && !project->isActiveConfig("staticlib") &&
                       project->isActiveConfig("lib_bundle"))
                        t << "\t\t\t\t" << writeSettings("FRAMEWORK_VERSION", project->first("QMAKE_FRAMEWORK_VERSION")) << ";\n";
                }
                if (!project->isEmpty("COMPAT_VERSION"))
                    t << "\t\t\t\t" << writeSettings("DYLIB_COMPATIBILITY_VERSION", project->first("COMPAT_VERSION")) << ";\n";

                if (!project->isEmpty("QMAKE_MACOSX_DEPLOYMENT_TARGET"))
                    t << "\t\t\t\t" << writeSettings("MACOSX_DEPLOYMENT_TARGET", project->first("QMAKE_MACOSX_DEPLOYMENT_TARGET")) << ";\n";
                if (!project->isEmpty("QMAKE_IOS_DEPLOYMENT_TARGET"))
                    t << "\t\t\t\t" << writeSettings("IPHONEOS_DEPLOYMENT_TARGET", project->first("QMAKE_IOS_DEPLOYMENT_TARGET")) << ";\n";
                if (!project->isEmpty("QMAKE_TVOS_DEPLOYMENT_TARGET"))
                    t << "\t\t\t\t" << writeSettings("APPLETVOS_DEPLOYMENT_TARGET", project->first("QMAKE_TVOS_DEPLOYMENT_TARGET")) << ";\n";
                if (!project->isEmpty("QMAKE_WATCHOS_DEPLOYMENT_TARGET"))
                    t << "\t\t\t\t" << writeSettings("WATCHOS_DEPLOYMENT_TARGET", project->first("QMAKE_WATCHOS_DEPLOYMENT_TARGET")) << ";\n";

                if (!project->isEmpty("QMAKE_XCODE_CODE_SIGN_IDENTITY"))
                    t << "\t\t\t\t" << writeSettings("CODE_SIGN_IDENTITY", project->first("QMAKE_XCODE_CODE_SIGN_IDENTITY")) << ";\n";

                tmp = project->values("QMAKE_PBX_VARS");
                for (int i = 0; i < tmp.count(); i++) {
                    QString var = tmp[i].toQString(), val = QString::fromLocal8Bit(qgetenv(var.toLatin1().constData()));
                    if (val.isEmpty() && var == "TB")
                        val = "/usr/bin/";
                    t << "\t\t\t\t" << writeSettings(var, val) << ";\n";
                }
                if (!project->isEmpty("PRECOMPILED_HEADER")) {
                    t << "\t\t\t\t" << writeSettings("GCC_PRECOMPILE_PREFIX_HEADER", "YES") << ";\n"
                    << "\t\t\t\t" << writeSettings("GCC_PREFIX_HEADER", project->first("PRECOMPILED_HEADER")) << ";\n";
                }
                t << "\t\t\t\t" << writeSettings("HEADER_SEARCH_PATHS", fixListForOutput("INCLUDEPATH"), SettingsAsList, 5) << ";\n"
                  << "\t\t\t\t" << writeSettings("LIBRARY_SEARCH_PATHS", fixListForOutput("QMAKE_PBX_LIBPATHS"), SettingsAsList, 5) << ";\n"
                  << "\t\t\t\t" << writeSettings("FRAMEWORK_SEARCH_PATHS", fixListForOutput("QMAKE_FRAMEWORKPATH"),
                        !project->values("QMAKE_FRAMEWORKPATH").isEmpty() ? SettingsAsList : 0, 5) << ";\n";

                {
                    ProStringList cflags = project->values("QMAKE_CFLAGS");
                    const ProStringList &prl_defines = project->values("PRL_EXPORT_DEFINES");
                    for (int i = 0; i < prl_defines.size(); ++i)
                        cflags += "-D" + prl_defines.at(i);
                    const ProStringList &defines = project->values("DEFINES");
                    for (int i = 0; i < defines.size(); ++i)
                        cflags += "-D" + defines.at(i);
                    t << "\t\t\t\t" << writeSettings("OTHER_CFLAGS", fixListForOutput(cflags), SettingsAsList, 5) << ";\n";
                }
                {
                    ProStringList cxxflags = project->values("QMAKE_CXXFLAGS");
                    const ProStringList &prl_defines = project->values("PRL_EXPORT_DEFINES");
                    for (int i = 0; i < prl_defines.size(); ++i)
                        cxxflags += "-D" + prl_defines.at(i);
                    const ProStringList &defines = project->values("DEFINES");
                    for (int i = 0; i < defines.size(); ++i)
                        cxxflags += "-D" + defines.at(i);
                    t << "\t\t\t\t" << writeSettings("OTHER_CPLUSPLUSFLAGS", fixListForOutput(cxxflags), SettingsAsList, 5) << ";\n";
                }
                if (!project->isActiveConfig("staticlib")) {
                    t << "\t\t\t\t" << writeSettings("OTHER_LDFLAGS",
                                                     fixListForOutput("SUBLIBS")
                                                     + fixListForOutput("QMAKE_LFLAGS")
                                                     + fixListForOutput(fixLibFlags("QMAKE_LIBS"))
                                                     + fixListForOutput(fixLibFlags("QMAKE_LIBS_PRIVATE")),
                                                     SettingsAsList, 6) << ";\n";
                }
                const ProStringList &archs = !project->values("QMAKE_XCODE_ARCHS").isEmpty() ?
                                                project->values("QMAKE_XCODE_ARCHS") : project->values("QT_ARCH");
                if (!archs.isEmpty())
                    t << "\t\t\t\t" << writeSettings("ARCHS", archs) << ";\n";
                if (!project->isEmpty("OBJECTS_DIR"))
                    t << "\t\t\t\t" << writeSettings("OBJROOT", project->first("OBJECTS_DIR")) << ";\n";
            } else {
                if (project->first("TEMPLATE") == "app") {
                    t << "\t\t\t\t" << writeSettings("PRODUCT_NAME", fixForOutput(project->first("QMAKE_ORIG_TARGET").toQString())) << ";\n";
                } else {
                    if (!project->isActiveConfig("plugin") && project->isActiveConfig("staticlib"))
                        t << "\t\t\t\t" << writeSettings("LIBRARY_STYLE", "STATIC") << ";\n";
                    else
                        t << "\t\t\t\t" << writeSettings("LIBRARY_STYLE", "DYNAMIC") << ";\n";
                    ProString lib = project->first("QMAKE_ORIG_TARGET");
                    if (!project->isActiveConfig("lib_bundle") && !project->isActiveConfig("staticlib"))
                        lib.prepend("lib");
                    t << "\t\t\t\t" << writeSettings("PRODUCT_NAME", lib) << ";\n";
                }
            }
            t << "\t\t\t};\n"
              << "\t\t\t" << writeSettings("name", configName) << ";\n"
              << "\t\t};\n";
        }
    }
    for (int i = 0; i < buildConfigGroups.size(); i++) {
        t << "\t\t" << keyFor("QMAKE_PBX_BUILDCONFIG_LIST_" + buildConfigGroups.at(i)) << " = {\n"
          << "\t\t\t" << writeSettings("isa", "XCConfigurationList", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("buildConfigurations", project->values(ProKey("QMAKE_PBX_BUILDCONFIGS_" + buildConfigGroups.at(i))), SettingsAsList, 4) << ";\n"
          << "\t\t\t" << writeSettings("defaultConfigurationIsVisible", "0", SettingsNoQuote) << ";\n"
          << "\t\t\t" << writeSettings("defaultConfigurationName", defaultConfig) << ";\n"
          << "\t\t};\n";
    }
    //ROOT
    t << "\t\t" << keyFor("QMAKE_PBX_ROOT") << " = {\n"
      << "\t\t\t" << writeSettings("hasScannedForEncodings", "1", SettingsNoQuote) << ";\n"
      << "\t\t\t" << writeSettings("compatibilityVersion", "Xcode 3.2") << ";\n"
      << "\t\t\t" << writeSettings("isa", "PBXProject", SettingsNoQuote) << ";\n"
      << "\t\t\t" << writeSettings("mainGroup", keyFor("QMAKE_PBX_ROOT_GROUP")) << ";\n"
      << "\t\t\t" << writeSettings("productRefGroup", keyFor("Products")) << ";\n";
    t << "\t\t\t" << writeSettings("buildConfigurationList", keyFor("QMAKE_PBX_BUILDCONFIG_LIST_PROJECT")) << ";\n";
    t << "\t\t\t" << writeSettings("projectDirPath", ProStringList()) << ";\n"
      << "\t\t\t" << writeSettings("projectRoot", "") << ";\n"
      << "\t\t\t" << writeSettings("targets", project->values("QMAKE_PBX_TARGETS"), SettingsAsList, 4) << ";\n"
      << "\t\t\t" << "attributes = {\n"
      << "\t\t\t\tTargetAttributes = {\n";
    for (const ProString &target : project->values("QMAKE_PBX_TARGETS")) {
        const ProStringList &attributes = project->values(ProKey("QMAKE_PBX_TARGET_ATTRIBUTES_" + target));
        if (attributes.isEmpty())
            continue;
        t << "\t\t\t\t\t" << target << " = {\n";
        for (const ProString &attribute : attributes)
            t << "\t\t\t\t\t\t" << writeSettings(attribute.toQString(), project->first(ProKey("QMAKE_PBX_TARGET_ATTRIBUTES_" + target + "_" + attribute))) << ";\n";
        t << "\t\t\t\t\t};\n";
    }
    t << "\t\t\t\t};\n"
      << "\t\t\t};\n"
      << "\t\t};\n";

    // FIXME: Deal with developmentRegion and knownRegions for QMAKE_PBX_ROOT

    //FOOTER
    t << "\t};\n"
      << "\t" << writeSettings("rootObject", keyFor("QMAKE_PBX_ROOT")) << ";\n"
      << "}\n";

    // Scheme
    {
        QString xcodeSpecDir = project->first("QMAKE_XCODE_SPECDIR").toQString();

        bool wroteCustomScheme = false;

        QString projectSharedSchemesPath = pbx_dir + "/xcshareddata/xcschemes";
        if (mkdir(projectSharedSchemesPath)) {
            QString target = project->first("QMAKE_ORIG_TARGET").toQString();

            QFile defaultSchemeFile(xcodeSpecDir + "/default.xcscheme");
            QFile outputSchemeFile(projectSharedSchemesPath + Option::dir_sep + target + ".xcscheme");

            if (defaultSchemeFile.open(QIODevice::ReadOnly)
                && outputSchemeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {

                QTextStream defaultSchemeStream(&defaultSchemeFile);
                QString schemeData = defaultSchemeStream.readAll();

                schemeData.replace(QLatin1String("@QMAKE_ORIG_TARGET@"), target);
                schemeData.replace(QLatin1String("@TARGET_PBX_KEY@"), keyFor(pbx_dir + "QMAKE_PBX_TARGET"));
                schemeData.replace(QLatin1String("@TEST_BUNDLE_PBX_KEY@"), keyFor("QMAKE_TEST_BUNDLE_REFERENCE"));
                schemeData.replace(QLatin1String("@QMAKE_RELATIVE_PBX_DIR@"), fileFixify(pbx_dir));

                QTextStream outputSchemeStream(&outputSchemeFile);
                outputSchemeStream << schemeData;

                wroteCustomScheme = true;
            }
        }

        if (wroteCustomScheme) {
             // Prevent Xcode from auto-generating schemes
            QString workspaceSettingsFilename("WorkspaceSettings.xcsettings");
            QString workspaceSharedDataPath = pbx_dir + "/project.xcworkspace/xcshareddata";
            if (mkdir(workspaceSharedDataPath)) {
                QFile::copy(xcodeSpecDir + Option::dir_sep + workspaceSettingsFilename,
                            workspaceSharedDataPath + Option::dir_sep + workspaceSettingsFilename);
            } else {
                wroteCustomScheme = false;
            }
        }

        if (!wroteCustomScheme)
            warn_msg(WarnLogic, "Failed to generate schemes in '%s', " \
                "falling back to Xcode auto-generated schemes", qPrintable(projectSharedSchemesPath));
    }

    return true;
}

QString
ProjectBuilderMakefileGenerator::findProgram(const ProString &prog)
{
    QString ret = prog.toQString();
    if(QDir::isRelativePath(ret)) {
        QStringList paths = QString(qgetenv("PATH")).split(':');
        for(int i = 0; i < paths.size(); ++i) {
            QString path = paths.at(i) + "/" + prog;
            if(exists(path)) {
                ret = path;
                break;
            }
        }
    }
    return ret;
}

QString
ProjectBuilderMakefileGenerator::fixForOutput(const QString &values)
{
    //get the environment variables references
    QRegExp reg_var("\\$\\((.*)\\)");
    for(int rep = 0; (rep = reg_var.indexIn(values, rep)) != -1;) {
        if(project->values("QMAKE_PBX_VARS").indexOf(reg_var.cap(1)) == -1)
            project->values("QMAKE_PBX_VARS").append(reg_var.cap(1));
        rep += reg_var.matchedLength();
    }

    return values;
}

ProStringList
ProjectBuilderMakefileGenerator::fixListForOutput(const char *where)
{
    return fixListForOutput(project->values(where));
}

ProStringList
ProjectBuilderMakefileGenerator::fixListForOutput(const ProStringList &l)
{
    ProStringList ret;
    for(int i = 0; i < l.count(); i++)
        ret += fixForOutput(l[i].toQString());
    return ret;
}

QString
ProjectBuilderMakefileGenerator::keyFor(const QString &block)
{
#if 1 //This make this code much easier to debug..
    if(project->isActiveConfig("no_pb_munge_key"))
       return block;
#endif
    QString ret;
    if(!keys.contains(block)) {
        ret = qtSha1(block.toUtf8()).left(24).toUpper();
        keys.insert(block, ret);
    } else {
        ret = keys[block];
    }
    return ret;
}

bool
ProjectBuilderMakefileGenerator::openOutput(QFile &file, const QString &build) const
{
    Q_ASSERT_X(QDir::isRelativePath(file.fileName()), "ProjectBuilderMakefileGenerator",
        "runQMake() should have normalized the filename and made it relative");

    QFileInfo fi(fileInfo(file.fileName()));
    if (fi.suffix() != "pbxproj") {
        QString output = file.fileName();
        if (!output.endsWith(projectSuffix())) {
            if (fi.fileName().isEmpty()) {
                if (project->first("TEMPLATE") == "subdirs" || project->isEmpty("QMAKE_ORIG_TARGET"))
                    output += fileInfo(project->projectFile()).baseName();
                else
                    output += project->first("QMAKE_ORIG_TARGET").toQString();
            }
            output += projectSuffix() + QDir::separator();
        } else {
            output += QDir::separator();
        }
        output += QString("project.pbxproj");
        file.setFileName(output);
    }

    pbx_dir = Option::output_dir + Option::dir_sep + file.fileName().section(Option::dir_sep, 0, 0);
    return UnixMakefileGenerator::openOutput(file, build);
}

int
ProjectBuilderMakefileGenerator::pbuilderVersion() const
{
    if (!project->isEmpty("QMAKE_PBUILDER_VERSION"))
        return project->first("QMAKE_PBUILDER_VERSION").toInt();
    return 46; // Xcode 3.2-compatible; default format since that version
}

int
ProjectBuilderMakefileGenerator::reftypeForFile(const QString &where)
{
    int ret = 0; //absolute is the default..
    if (QDir::isRelativePath(where))
        ret = 4; //relative
    return ret;
}

QString
ProjectBuilderMakefileGenerator::projectSuffix() const
{
    return ".xcodeproj";
}

QString
ProjectBuilderMakefileGenerator::pbxbuild()
{
    return "xcodebuild";
}

static QString quotedStringLiteral(const QString &value)
{
    QString result;
    const int len = value.length();
    result.reserve(int(len * 1.1) + 2);

    result += QLatin1Char('"');

    // Escape
    for (int i = 0; i < len; ++i) {
        QChar character = value.at(i);;
        ushort code = character.unicode();
        switch (code) {
        case '\\':
            result += QLatin1String("\\\\");
            break;
        case '"':
            result += QLatin1String("\\\"");
            break;
        case '\b':
            result += QLatin1String("\\b");
            break;
        case '\n':
            result += QLatin1String("\\n");
            break;
        case '\r':
            result += QLatin1String("\\r");
            break;
        case '\t':
            result += QLatin1String("\\t");
            break;
        default:
            if (code >= 32 && code <= 127)
                result += character;
            else
                result += QLatin1String("\\u") + QString::number(code, 16).rightJustified(4, '0');
        }
    }

    result += QLatin1Char('"');

    result.squeeze();
    return result;
}

QString
ProjectBuilderMakefileGenerator::writeSettings(const QString &var, const ProStringList &vals, int flags, int indent_level)
{
    QString ret;
    bool shouldQuote = !((flags & SettingsNoQuote));

    QString newline = "\n";
    for(int i = 0; i < indent_level; ++i)
        newline += "\t";

    static QRegExp allowedVariableCharacters("^[a-zA-Z0-9_]*$");
    ret += var.contains(allowedVariableCharacters) ? var : quotedStringLiteral(var);

    ret += " = ";

    if(flags & SettingsAsList) {
        ret += "(" + newline;
        for(int i = 0, count = 0; i < vals.size(); ++i) {
            QString val = vals.at(i).toQString();
            if(!val.isEmpty()) {
                if(count++ > 0)
                    ret += "," + newline;
                if (shouldQuote)
                    val = quotedStringLiteral(val);
                ret += val;
            }
        }
        ret += ")";
    } else {
        QString val = vals.join(QLatin1Char(' '));
        if (shouldQuote)
            val = quotedStringLiteral(val);
        ret += val;
    }
    return ret;
}

QT_END_NAMESPACE
