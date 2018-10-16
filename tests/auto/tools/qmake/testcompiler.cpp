/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "testcompiler.h"

#include <QProcess>
#include <QDir>

static QString targetName( BuildType buildMode, const QString& target, const QString& version )
{
    Q_UNUSED(version);
    QString targetName = target;

#if defined (Q_OS_WIN32)
    switch (buildMode)
    {
    case Exe: // app
        targetName.append(".exe");
        break;
    case Dll: // dll
        if (!version.isEmpty())
            targetName.append(version.section(".", 0, 0));
        targetName.append(".dll");
        break;
    case Lib: // lib
#ifdef Q_CC_GNU
        targetName.prepend("lib");
        targetName.append(".a");
#else
        targetName.append(".lib");
#endif
        break;
    case Plain:
        // no conversion
        break;
    }
#elif defined( Q_OS_MAC )
    switch (buildMode)
    {
    case Exe: // app
        targetName += ".app/Contents/MacOS/" + target.section('/', -1);
        break;
    case Dll: // dll
        targetName.prepend("lib");
        targetName.append("." + version + ".dylib");
        break;
    case Lib: // lib
        targetName.prepend("lib");
        targetName.append(".a");
        break;
    case Plain:
        // no conversion
        break;
    }
#else
    switch (buildMode)
    {
    case Exe: // app
        break;
    case Dll: // dll
        targetName.prepend("lib");
#if defined (Q_OS_HPUX) && !defined (__ia64)
        targetName.append(".sl");
#elif defined (Q_OS_AIX)
        targetName.append(".a");
#else
        targetName.append(".so");
#endif
        break;
    case Lib: // lib
        targetName.prepend("lib");
        targetName.append(".a");
        break;
    case Plain:
        // no conversion
        break;
    }
#endif
    return targetName;
}

TestCompiler::TestCompiler()
{
    setBaseCommands( "", "" );
}

TestCompiler::~TestCompiler()
{
}

bool TestCompiler::errorOut()
{
    qDebug("%s", qPrintable(testOutput_.join(QStringLiteral("\n"))));
    return false;
}

// Return the system environment, remove MAKEFLAGS variable in
// case the CI uses jom passing flags incompatible to nmake
// or vice versa.
static inline QStringList systemEnvironment()
{
#ifdef Q_OS_WIN
    static QStringList result;
    if (result.isEmpty()) {
        foreach (const QString &variable, QProcess::systemEnvironment()) {
            if (variable.startsWith(QStringLiteral("MAKEFLAGS="), Qt::CaseInsensitive)) {
                qWarning("Removing environment setting '%s'", qPrintable(variable));
            } else {
                result.push_back(variable);
            }
        }
    }
#else
    static const QStringList result = QProcess::systemEnvironment();
#endif // ifdef Q_OS_WIN
    return result;
}

bool TestCompiler::runCommand(const QString &cmd, const QStringList &args, bool expectFail)
{
    QString dbg = cmd;
    if (dbg.contains(' '))
        dbg.prepend('"').append('"');
    foreach (QString arg, args) {
        if (arg.contains(' '))
            arg.prepend('"').append('"');
        dbg.append(' ').append(arg);
    }
    testOutput_.append("Running command: " + dbg);

    QProcess child;
    child.setEnvironment(systemEnvironment() + environment_);

    child.start(cmd, args);
    if (!child.waitForStarted(-1)) {
        testOutput_.append( "Unable to start child process." );
        return errorOut();
    }

    child.setReadChannel(QProcess::StandardError);
    child.waitForFinished(-1);
    bool ok = child.exitStatus() == QProcess::NormalExit && (expectFail ^ (child.exitCode() == 0));

    foreach (const QByteArray &output, child.readAllStandardError().split('\n')) {
        testOutput_.append(QString::fromLocal8Bit(output));

        if (output.startsWith("Project MESSAGE: FAILED"))
            ok = false;
    }

    return ok ? true : errorOut();
}

void TestCompiler::setBaseCommands( QString makeCmd, QString qmakeCmd )
{
    makeCmd_ = makeCmd;
    qmakeCmd_ = qmakeCmd;
}

void TestCompiler::resetArguments()
{
    makeArgs_.clear();
    qmakeArgs_.clear();
}

void TestCompiler::setArguments(const QStringList &makeArgs, const QStringList &qmakeArgs)
{
    makeArgs_ = makeArgs;
    qmakeArgs_ = qmakeArgs;
}

void TestCompiler::resetEnvironment()
{
    environment_.clear();
}

void TestCompiler::addToEnvironment( QString varAssignment )
{
    environment_.push_back(varAssignment);
}

bool TestCompiler::makeClean( const QString &workPath )
{
    QDir D;
    if (!D.exists(workPath)) {
        testOutput_.append( "Directory '" + workPath + "' doesn't exist" );
        return errorOut();
    }

    D.setCurrent(workPath);
    QFileInfo Fi( workPath + "/Makefile");
    if (Fi.exists())
        // Run make clean
        return runCommand(makeCmd_, QStringList() << "clean");

    return true;
}

bool TestCompiler::makeDistClean( const QString &workPath )
{
    QDir D;
    if (!D.exists(workPath)) {
        testOutput_.append( "Directory '" + workPath + "' doesn't exist" );
        return errorOut();
    }

    D.setCurrent(workPath);
    QFileInfo Fi( workPath + "/Makefile");
    if (Fi.exists())
        // Run make distclean
        return runCommand(makeCmd_, QStringList() << "distclean");

    return true;

}

bool TestCompiler::qmakeProject( const QString &workDir, const QString &proName )
{
    QDir D;
    if (!D.exists(workDir)) {
        testOutput_.append( "Directory '" + workDir + "' doesn't exist" );
        return errorOut();
    }
    D.setCurrent(workDir);

    QString projectFile = proName;
    if (!projectFile.endsWith(".pro"))
        projectFile += ".pro";

    return runCommand(qmakeCmd_, QStringList() << "-project" << "-o" << projectFile << "DESTDIR=./");
}

bool TestCompiler::qmake( const QString &workDir, const QString &proName, const QString &buildDir )
{
    QDir D;
    D.setCurrent( workDir );

    if (D.exists("Makefile"))
        D.remove("Makefile");

    QString projectFile = proName;
    QString makeFile = buildDir;
    if (!projectFile.endsWith(".pro"))
        projectFile += ".pro";
    if (!makeFile.isEmpty() && !makeFile.endsWith('/'))
        makeFile += '/';
    makeFile += "Makefile";

    // Now start qmake and generate the makefile
    return runCommand(qmakeCmd_, QStringList(qmakeArgs_) << projectFile << "-o" << makeFile);
}

bool TestCompiler::make( const QString &workPath, const QString &target, bool expectFail )
{
    QDir D;
    D.setCurrent( workPath );

    QStringList args = makeArgs_;
    if (makeCmd_.contains("nmake", Qt::CaseInsensitive))
        args << "/NOLOGO";
    if (!target.isEmpty())
        args << target;

    return runCommand(makeCmd_, args, expectFail);
}

bool TestCompiler::exists( const QString &destDir, const QString &exeName, BuildType buildType, const QString &version )
{
    QFileInfo f(destDir + QLatin1Char('/') + targetName(buildType, exeName, version));
    return f.exists();
}

bool TestCompiler::removeMakefile( const QString &workPath )
{
    return removeFile( workPath, "Makefile" );
}

bool TestCompiler::removeProject( const QString &workPath, const QString &project )
{
    QString projectFile = project;
    if (!projectFile.endsWith(".pro"))
        projectFile += ".pro";

    return removeFile( workPath, projectFile );
}

bool TestCompiler::removeFile( const QString &workPath, const QString &fileName )
{
    QDir D;
    D.setCurrent( workPath );

    return ( D.exists( fileName ) ) ? D.remove( fileName ) : true;
}

QString TestCompiler::commandOutput() const
{
#ifndef Q_OS_WIN
    return testOutput_.join("\n");
#else
    return testOutput_.join("\r\n");
#endif
}

void TestCompiler::clearCommandOutput()
{
   testOutput_.clear();
}
