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
#include <QTextCodec>
#include <QFile>
#include <qtest.h>

Q_DECLARE_METATYPE(QTextCodec *)

class tst_QTextCodec: public QObject
{
    Q_OBJECT
private slots:
    void codecForName() const;
    void codecForName_data() const;
    void codecForMib() const;
    void fromUnicode_data() const;
    void fromUnicode() const;
    void toUnicode_data() const;
    void toUnicode() const;
};

void tst_QTextCodec::codecForName() const
{
    QFETCH(QList<QByteArray>, codecs);

    QBENCHMARK {
        foreach(const QByteArray& c, codecs) {
            QVERIFY(QTextCodec::codecForName(c));
            QVERIFY(QTextCodec::codecForName(c + '-'));
        }
        foreach(const QByteArray& c, codecs) {
            QVERIFY(QTextCodec::codecForName(c + '+'));
            QVERIFY(QTextCodec::codecForName(c + '*'));
        }
    }
}

void tst_QTextCodec::codecForName_data() const
{
    QTest::addColumn<QList<QByteArray> >("codecs");

    QTest::newRow("all") << QTextCodec::availableCodecs();
    QTest::newRow("many utf-8") << (QList<QByteArray>()
            << "utf-8" << "utf-8" << "utf-8" << "utf-8" << "utf-8"
            << "utf-8" << "utf-8" << "utf-8" << "utf-8" << "utf-8"
            << "utf-8" << "utf-8" << "utf-8" << "utf-8" << "utf-8"
            << "utf-8" << "utf-8" << "utf-8" << "utf-8" << "utf-8"
            << "utf-8" << "utf-8" << "utf-8" << "utf-8" << "utf-8"
            << "utf-8" << "utf-8" << "utf-8" << "utf-8" << "utf-8"
            << "utf-8" << "utf-8" << "utf-8" << "utf-8" << "utf-8"
            << "utf-8" << "utf-8" << "utf-8" << "utf-8" << "utf-8"
            << "utf-8" << "utf-8" << "utf-8" << "utf-8" << "utf-8"   );
}

void tst_QTextCodec::codecForMib() const
{
    QBENCHMARK {
        QTextCodec::codecForMib(106);
        QTextCodec::codecForMib(111);
        QTextCodec::codecForMib(106);
        QTextCodec::codecForMib(2254);
        QTextCodec::codecForMib(2255);
        QTextCodec::codecForMib(2256);
        QTextCodec::codecForMib(2257);
        QTextCodec::codecForMib(2258);
        QTextCodec::codecForMib(111);
        QTextCodec::codecForMib(2250);
        QTextCodec::codecForMib(2251);
        QTextCodec::codecForMib(2252);
        QTextCodec::codecForMib(106);
        QTextCodec::codecForMib(106);
        QTextCodec::codecForMib(106);
        QTextCodec::codecForMib(106);
    }
}

void tst_QTextCodec::fromUnicode_data() const
{
    QTest::addColumn<QTextCodec*>("codec");

    QTest::newRow("utf-8") << QTextCodec::codecForName("utf-8");
    QTest::newRow("latin 1") << QTextCodec::codecForName("latin 1");
    QTest::newRow("utf-16") << QTextCodec::codecForName("utf16"); ;
    QTest::newRow("utf-32") << QTextCodec::codecForName("utf32");
    QTest::newRow("latin15") << QTextCodec::codecForName("iso-8859-15");
    QTest::newRow("eucKr") << QTextCodec::codecForName("eucKr");
}


void tst_QTextCodec::fromUnicode() const
{
    QFETCH(QTextCodec*, codec);
    QString testFile = QFINDTESTDATA("utf-8.txt");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file utf-8.txt!");
    QFile file(testFile);
    if (!file.open(QFile::ReadOnly)) {
        qFatal("Cannot open input file");
        return;
    }
    QByteArray data = file.readAll();
    const char *d = data.constData();
    int size = data.size();
    QString s = QString::fromUtf8(d, size);
    s = s + s + s;
    s = s + s + s;
    QBENCHMARK {
        for (int i = 0; i < 10; i ++)
            codec->fromUnicode(s);
    }
}


void tst_QTextCodec::toUnicode_data() const
{
    fromUnicode_data();
}


void tst_QTextCodec::toUnicode() const
{
    QFETCH(QTextCodec*, codec);
    QString testFile = QFINDTESTDATA("utf-8.txt");
    QVERIFY2(!testFile.isEmpty(), "cannot find test file utf-8.txt!");
    QFile file(testFile);
    QVERIFY(file.open(QFile::ReadOnly));
    QByteArray data = file.readAll();
    const char *d = data.constData();
    int size = data.size();
    QString s = QString::fromUtf8(d, size);
    s = s + s + s;
    s = s + s + s;
    QByteArray orig = codec->fromUnicode(s);
    QBENCHMARK {
        for (int i = 0; i < 10; i ++)
            codec->toUnicode(orig);
    }
}




QTEST_MAIN(tst_QTextCodec)

#include "main.moc"
