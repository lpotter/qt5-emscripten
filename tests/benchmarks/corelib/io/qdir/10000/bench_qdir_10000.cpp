/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite module of the Qt Toolkit.
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

#include <QtTest/QtTest>

#ifdef Q_OS_WIN
#   include <windows.h>
#else
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <dirent.h>
#   include <unistd.h>
#endif

class bench_QDir_10000 : public QObject{
  Q_OBJECT
public slots:
    void initTestCase() {
        QDir testdir = QDir::tempPath();

        const QString subfolder_name = QLatin1String("test_speed");
        QVERIFY(testdir.mkdir(subfolder_name));
        QVERIFY(testdir.cd(subfolder_name));

        for (uint i=0; i<10000; ++i) {
            QFile file(testdir.absolutePath() + "/testfile_" + QString::number(i));
            file.open(QIODevice::WriteOnly);
        }
    }
    void cleanupTestCase() {
        {
            QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
            testdir.setSorting(QDir::Unsorted);
            testdir.setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);
            foreach (const QString &filename, testdir.entryList()) {
                testdir.remove(filename);
            }
        }
        const QDir temp = QDir(QDir::tempPath());
        temp.rmdir(QLatin1String("test_speed"));
    }
private slots:
    void baseline() {}

    void sizeSpeed() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        QBENCHMARK {
            QFileInfoList fileInfoList = testdir.entryInfoList(QDir::Files, QDir::Unsorted);
            foreach (const QFileInfo &fileInfo, fileInfoList) {
                fileInfo.isDir();
                fileInfo.size();
            }
        }
    }
    void sizeSpeedIterator() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        QBENCHMARK {
            QDirIterator dit(testdir.path(), QDir::Files);
            while (dit.hasNext()) {
                dit.next();
                dit.fileInfo().isDir();
                dit.fileInfo().size();
            }
        }
    }

    void sizeSpeedWithoutFilter() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        QBENCHMARK {
            QFileInfoList fileInfoList = testdir.entryInfoList(QDir::NoFilter, QDir::Unsorted);
            foreach (const QFileInfo &fileInfo, fileInfoList) {
                fileInfo.size();
            }
        }
    }
    void sizeSpeedWithoutFilterIterator() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        QBENCHMARK {
            QDirIterator dit(testdir.path());
            while (dit.hasNext()) {
                dit.next();
                dit.fileInfo().isDir();
                dit.fileInfo().size();
            }
        }
    }

    void sizeSpeedWithoutFileInfoList() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        testdir.setSorting(QDir::Unsorted);
        QBENCHMARK {
            QStringList fileList = testdir.entryList(QDir::NoFilter, QDir::Unsorted);
            foreach (const QString &filename, fileList) {
                QFileInfo fileInfo(filename);
                fileInfo.size();
            }
        }
    }

    void iDontWantAnyStat() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        testdir.setSorting(QDir::Unsorted);
        testdir.setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);
        QBENCHMARK {
            QStringList fileList = testdir.entryList(QDir::NoFilter, QDir::Unsorted);
            foreach (const QString &filename, fileList) {

            }
        }
    }
    void iDontWantAnyStatIterator() {
        QBENCHMARK {
            QDirIterator dit(QDir::tempPath() + QLatin1String("/test_speed"));
            while (dit.hasNext()) {
                dit.next();
            }
        }
    }

    void sorted_byTime() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
        testdir.setSorting(QDir::Time);
        testdir.setFilter(QDir::AllEntries | QDir::System | QDir::Hidden);
        QBENCHMARK {
            QStringList fileList = testdir.entryList(QDir::NoFilter, QDir::Time);
            foreach (const QString &filename, fileList) {

            }
        }
    }

    void sizeSpeedWithoutFilterLowLevel() {
        QDir testdir(QDir::tempPath() + QLatin1String("/test_speed"));
#ifdef Q_OS_WIN
        const wchar_t *dirpath = (wchar_t*)testdir.absolutePath().utf16();
        wchar_t appendedPath[MAX_PATH];
        wcscpy(appendedPath, dirpath);
        wcscat(appendedPath, L"\\*");

        WIN32_FIND_DATA fd;
#ifndef Q_OS_WINRT
        HANDLE hSearch = FindFirstFileW(appendedPath, &fd);
#else
        HANDLE hSearch = FindFirstFileEx(appendedPath, FindExInfoStandard, &fd,
                                         FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
#endif
        QVERIFY(hSearch != INVALID_HANDLE_VALUE);

        QBENCHMARK {
            do {

            } while (FindNextFile(hSearch, &fd));
        }
        FindClose(hSearch);
#else
        DIR *dir = opendir(qPrintable(testdir.absolutePath()));
        QVERIFY(dir);

        QVERIFY(!chdir(qPrintable(testdir.absolutePath())));
        QBENCHMARK {
            struct dirent *item = readdir(dir);
            while (item) {
                char *fileName = item->d_name;

                struct stat fileStat;
                QVERIFY(!stat(fileName, &fileStat));

                item = readdir(dir);
            }
        }
        closedir(dir);
#endif
    }
};

QTEST_MAIN(bench_QDir_10000)
#include "bench_qdir_10000.moc"
