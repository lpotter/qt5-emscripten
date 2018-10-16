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

#include <qurl.h>
#include <qtest.h>

class tst_qurl: public QObject
{
    Q_OBJECT

private slots:
    void emptyUrl();
    void relativeUrl();
    void absoluteUrl();
    void isRelative_data();
    void isRelative();
    void toLocalFile_data();
    void toLocalFile();
    void toString_data();
    void toString();
    void resolved_data();
    void resolved();
    void equality_data();
    void equality();
    void qmlPropertyWriteUseCase();

private:
    void generateFirstRunData();
};

void tst_qurl::emptyUrl()
{
    QBENCHMARK {
        QUrl url;
    }
}

void tst_qurl::relativeUrl()
{
    QBENCHMARK {
        QUrl url("pics/avatar.png");
    }
}

void tst_qurl::absoluteUrl()
{
    QBENCHMARK {
        QUrl url("/tmp/avatar.png");
    }
}

void tst_qurl::generateFirstRunData()
{
    QTest::addColumn<bool>("firstRun");

    QTest::newRow("construction + first run") << true;
    QTest::newRow("subsequent runs") << false;
}

void tst_qurl::isRelative_data()
{
    generateFirstRunData();
}

void tst_qurl::isRelative()
{
    QFETCH(bool, firstRun);
    if (firstRun) {
        QBENCHMARK {
            QUrl url("pics/avatar.png");
            url.isRelative();
        }
    } else {
        QUrl url("pics/avatar.png");
        QBENCHMARK {
            url.isRelative();
        }
    }
}

void tst_qurl::toLocalFile_data()
{
    generateFirstRunData();
}

void tst_qurl::toLocalFile()
{
    QFETCH(bool, firstRun);
    if (firstRun) {
        QBENCHMARK {
            QUrl url("/tmp/avatar.png");
            url.toLocalFile();
        }
    } else {
        QUrl url("/tmp/avatar.png");
        QBENCHMARK {
            url.toLocalFile();
        }
    }
}

void tst_qurl::toString_data()
{
    generateFirstRunData();
}

void tst_qurl::toString()
{
    QFETCH(bool, firstRun);
    if(firstRun) {
        QBENCHMARK {
            QUrl url("pics/avatar.png");
            url.toString();
        }
    } else {
        QUrl url("pics/avatar.png");
        QBENCHMARK {
            url.toString();
        }
    }
}

void tst_qurl::resolved_data()
{
   generateFirstRunData();
}

void tst_qurl::resolved()
{
    QFETCH(bool, firstRun);
    if(firstRun) {
        QBENCHMARK {
            QUrl baseUrl("/home/user/");
            QUrl url("pics/avatar.png");
            baseUrl.resolved(url);
        }
    } else {
        QUrl baseUrl("/home/user/");
        QUrl url("pics/avatar.png");
        QBENCHMARK {
            baseUrl.resolved(url);
        }
    }
}

void tst_qurl::equality_data()
{
   generateFirstRunData();
}

void tst_qurl::equality()
{
    QFETCH(bool, firstRun);
    if(firstRun) {
        QBENCHMARK {
            QUrl url("pics/avatar.png");
            QUrl url2("pics/avatar2.png");
            //url == url2;
        }
    } else {
        QUrl url("pics/avatar.png");
        QUrl url2("pics/avatar2.png");
        QBENCHMARK {
            url == url2;
        }
    }
}

void tst_qurl::qmlPropertyWriteUseCase()
{
    QUrl base("file:///home/user/qt/examples/declarative/samegame/SamegameCore/");
    QString str("pics/redStar.png");

    QBENCHMARK {
        QUrl u = QUrl(str);
        if (!u.isEmpty() && u.isRelative())
            u = base.resolved(u);
    }
}

QTEST_MAIN(tst_qurl)

#include "main.moc"
