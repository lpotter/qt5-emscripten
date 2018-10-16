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


#ifndef QTEST_EXTERNAL_TESTS_H
#define QTEST_EXTERNAL_TESTS_H

#include <QList>
#include <QByteArray>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace QTest {
    class QExternalTestPrivate;
    class QExternalTest
    {
    public:
        QExternalTest();
        ~QExternalTest();

        enum Stage {
            FileStage,
            QmakeStage,
            CompilationStage,
            LinkStage,
            RunStage
        };

        enum QtModule {
            QtCore      = 0x0001,
            QtGui       = 0x0002,
            QtNetwork   = 0x0004,
            QtXml       = 0x0008,
            QtXmlPatterns=0x0010,
            QtOpenGL    = 0x0020,
            QtSql       = 0x0040,
            QtSvg       = 0x0080,
            QtScript    = 0x0100,
            QtTest      = 0x0200,
            QtDBus      = 0x0400,
            QtWebKit    = 0x0800,
            QtWidgets   = 0x1000,
            Phonon      = 0x2000 // odd man out
        };
        Q_DECLARE_FLAGS(QtModules, QtModule)

        enum ApplicationType {
            AutoApplication,
            Applicationless,
            QCoreApplication,
            QGuiApplication,
            QApplication
        };

        QList<QByteArray> qmakeSettings() const;
        void setQmakeSettings(const QList<QByteArray> &settings);

        QtModules qtModules() const;
        void setQtModules(QtModules modules);

        ApplicationType applicationType() const;
        void setApplicationType(ApplicationType type);

        QStringList extraProgramSources() const;
        void setExtraProgramSources(const QStringList &list);

        QByteArray programHeader() const;
        void setProgramHeader(const QByteArray &header);

        // execution:
        bool tryCompile(const QByteArray &body);
        bool tryLink(const QByteArray &body);
        bool tryRun(const QByteArray &body);
        bool tryCompileFail(const QByteArray &body);
        bool tryLinkFail(const QByteArray &body);
        bool tryRunFail(const QByteArray &body);

        Stage failedStage() const;
        int exitCode() const;
        QByteArray fullProgramSource() const;
        QByteArray standardOutput() const;
        QByteArray standardError() const;

        QString errorReport() const;

    private:
        QExternalTestPrivate * const d;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(QExternalTest::QtModules)
}
QT_END_NAMESPACE

#endif
