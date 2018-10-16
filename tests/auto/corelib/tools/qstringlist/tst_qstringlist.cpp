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

#include <QtTest/QtTest>
#include <qregexp.h>
#include <qregularexpression.h>
#include <qstringlist.h>

#include <locale.h>

class tst_QStringList : public QObject
{
    Q_OBJECT
private slots:
    void sort();
    void filter();
    void replaceInStrings();
    void removeDuplicates();
    void removeDuplicates_data();
    void contains();
    void indexOf();
    void lastIndexOf();

    void indexOf_regExp();
    void lastIndexOf_regExp();

    void streamingOperator();
    void assignmentOperator();
    void join() const;
    void join_data() const;
    void joinEmptiness() const;
    void joinChar() const;
    void joinChar_data() const;

#ifdef Q_COMPILER_INITIALIZER_LISTS
    void initializeList() const;
#endif
};

extern const char email[];

void tst_QStringList::indexOf_regExp()
{
    QStringList list;
    list << "harald" << "trond" << "vohi" << "harald";
    {
        QRegExp re(".*o.*");

        QCOMPARE(list.indexOf(re), 1);
        QCOMPARE(list.indexOf(re, 2), 2);
        QCOMPARE(list.indexOf(re, 3), -1);

        QCOMPARE(list.indexOf(QRegExp(".*x.*")), -1);
        QCOMPARE(list.indexOf(re, -1), -1);
        QCOMPARE(list.indexOf(re, -3), 1);
        QCOMPARE(list.indexOf(re, -9999), 1);
        QCOMPARE(list.indexOf(re, 9999), -1);

        QCOMPARE(list.indexOf(QRegExp("[aeiou]")), -1);
    }

    {
        QRegularExpression re(".*o.*");

        QCOMPARE(list.indexOf(re), 1);
        QCOMPARE(list.indexOf(re, 2), 2);
        QCOMPARE(list.indexOf(re, 3), -1);

        QCOMPARE(list.indexOf(QRegularExpression(".*x.*")), -1);
        QCOMPARE(list.indexOf(re, -1), -1);
        QCOMPARE(list.indexOf(re, -3), 1);
        QCOMPARE(list.indexOf(re, -9999), 1);
        QCOMPARE(list.indexOf(re, 9999), -1);

        QCOMPARE(list.indexOf(QRegularExpression("[aeiou]")), -1);
    }
}

void tst_QStringList::lastIndexOf_regExp()
{
    QStringList list;
    list << "harald" << "trond" << "vohi" << "harald";

    {
        QRegExp re(".*o.*");

        QCOMPARE(list.lastIndexOf(re), 2);
        QCOMPARE(list.lastIndexOf(re, 2), 2);
        QCOMPARE(list.lastIndexOf(re, 1), 1);

        QCOMPARE(list.lastIndexOf(QRegExp(".*x.*")), -1);
        QCOMPARE(list.lastIndexOf(re, -1), 2);
        QCOMPARE(list.lastIndexOf(re, -3), 1);
        QCOMPARE(list.lastIndexOf(re, -9999), -1);
        QCOMPARE(list.lastIndexOf(re, 9999), 2);

        QCOMPARE(list.lastIndexOf(QRegExp("[aeiou]")), -1);
    }

    {
        QRegularExpression re(".*o.*");

        QCOMPARE(list.lastIndexOf(re), 2);
        QCOMPARE(list.lastIndexOf(re, 2), 2);
        QCOMPARE(list.lastIndexOf(re, 1), 1);

        QCOMPARE(list.lastIndexOf(QRegularExpression(".*x.*")), -1);
        QCOMPARE(list.lastIndexOf(re, -1), 2);
        QCOMPARE(list.lastIndexOf(re, -3), 1);
        QCOMPARE(list.lastIndexOf(re, -9999), -1);
        QCOMPARE(list.lastIndexOf(re, 9999), 2);

        QCOMPARE(list.lastIndexOf(QRegularExpression("[aeiou]")), -1);
    }


}

void tst_QStringList::indexOf()
{
    QStringList list;
    list << "harald" << "trond" << "vohi" << "harald";

    QCOMPARE(list.indexOf("harald"), 0);
    QCOMPARE(list.indexOf("trond"), 1);
    QCOMPARE(list.indexOf("vohi"), 2);
    QCOMPARE(list.indexOf("harald", 1), 3);

    QCOMPARE(list.indexOf("hans"), -1);
    QCOMPARE(list.indexOf("trond", 2), -1);
    QCOMPARE(list.indexOf("harald", -1), 3);
    QCOMPARE(list.indexOf("vohi", -3), 2);
}

void tst_QStringList::lastIndexOf()
{
    QStringList list;
    list << "harald" << "trond" << "vohi" << "harald";

    QCOMPARE(list.lastIndexOf("harald"), 3);
    QCOMPARE(list.lastIndexOf("trond"), 1);
    QCOMPARE(list.lastIndexOf("vohi"), 2);
    QCOMPARE(list.lastIndexOf("harald", 2), 0);

    QCOMPARE(list.lastIndexOf("hans"), -1);
    QCOMPARE(list.lastIndexOf("vohi", 1), -1);
    QCOMPARE(list.lastIndexOf("vohi", -1), 2);
    QCOMPARE(list.lastIndexOf("vohi", -3), -1);
}

void tst_QStringList::filter()
{
    QStringList list1, list2;
    list1 << "Bill Gates" << "Joe Blow" << "Bill Clinton";
    list1 = list1.filter( "Bill" );
    list2 << "Bill Gates" << "Bill Clinton";
    QCOMPARE( list1, list2 );

    QStringList list3, list4;
    list3 << "Bill Gates" << "Joe Blow" << "Bill Clinton";
    list3 = list3.filter( QRegExp("[i]ll") );
    list4 << "Bill Gates" << "Bill Clinton";
    QCOMPARE( list3, list4 );

    QStringList list5, list6;
    list5 << "Bill Gates" << "Joe Blow" << "Bill Clinton";
    list5 = list5.filter( QRegularExpression("[i]ll") );
    list6 << "Bill Gates" << "Bill Clinton";
    QCOMPARE( list5, list6 );
}

void tst_QStringList::sort()
{
    QStringList list1, list2;
    list1 << "alpha" << "beta" << "BETA" << "gamma" << "Gamma" << "gAmma" << "epsilon";
    list1.sort();
    list2 << "BETA" << "Gamma" << "alpha" << "beta" << "epsilon" << "gAmma" << "gamma";
    QCOMPARE( list1, list2 );

    char *current_locale = setlocale(LC_ALL, "C");
    QStringList list3, list4;
    list3 << "alpha" << "beta" << "BETA" << "gamma" << "Gamma" << "gAmma" << "epsilon";
    list3.sort(Qt::CaseInsensitive);
    list4 << "alpha" << "beta" << "BETA" << "epsilon" << "Gamma" << "gAmma" << "gamma";
    // with this list, case insensitive sorting can give more than one permutation for "equivalent"
    // elements; so we check that the sort gave the formally correct result (list[i] <= list[i+1])
    for (int i = 0; i < list4.count() - 1; ++i)
        QVERIFY2(QString::compare(list4.at(i), list4.at(i + 1), Qt::CaseInsensitive) <= 0, qPrintable(QString("index %1 failed").arg(i)));
    // additional checks
    QCOMPARE(list4.at(0), QString("alpha"));
    QVERIFY(list4.indexOf("epsilon") > 0);
    QVERIFY(list4.indexOf("epsilon") < (list4.count() - 1));
    setlocale(LC_ALL, current_locale);
}

void tst_QStringList::replaceInStrings()
{
    QStringList list1, list2;
    list1 << "alpha" << "beta" << "gamma" << "epsilon";
    list1.replaceInStrings( "a", "o" );
    list2 << "olpho" << "beto" << "gommo" << "epsilon";
    QCOMPARE( list1, list2 );

    QStringList list3, list4;
    list3 << "alpha" << "beta" << "gamma" << "epsilon";
    list3.replaceInStrings( QRegExp("^a"), "o" );
    list4 << "olpha" << "beta" << "gamma" << "epsilon";
    QCOMPARE( list3, list4 );

    QStringList list5, list6;
    list5 << "Bill Clinton" << "Gates, Bill";
    list6 << "Bill Clinton" << "Bill Gates";
    list5.replaceInStrings( QRegExp("^(.*), (.*)$"), "\\2 \\1" );
    QCOMPARE( list5, list6 );

    QStringList list7, list8;
    list7 << "alpha" << "beta" << "gamma" << "epsilon";
    list7.replaceInStrings( QRegularExpression("^a"), "o" );
    list8 << "olpha" << "beta" << "gamma" << "epsilon";
    QCOMPARE( list7, list8 );

    QStringList list9, list10;
    list9 << "Bill Clinton" << "Gates, Bill";
    list10 << "Bill Clinton" << "Bill Gates";
    list9.replaceInStrings( QRegularExpression("^(.*), (.*)$"), "\\2 \\1" );
    QCOMPARE( list9, list10 );
}

void tst_QStringList::contains()
{
    QStringList list;
    list << "arthur" << "Arthur" << "arthuR" << "ARTHUR" << "Dent" << "Hans Dent";

    QVERIFY(list.contains("arthur"));
    QVERIFY(!list.contains("ArthuR"));
    QVERIFY(!list.contains("Hans"));
    QVERIFY(list.contains("arthur", Qt::CaseInsensitive));
    QVERIFY(list.contains("ArthuR", Qt::CaseInsensitive));
    QVERIFY(list.contains("ARTHUR", Qt::CaseInsensitive));
    QVERIFY(list.contains("dent", Qt::CaseInsensitive));
    QVERIFY(!list.contains("hans", Qt::CaseInsensitive));

    QVERIFY(list.contains(QLatin1String("arthur")));
    QVERIFY(!list.contains(QLatin1String("ArthuR")));
    QVERIFY(!list.contains(QLatin1String("Hans")));
    QVERIFY(list.contains(QLatin1String("arthur"), Qt::CaseInsensitive));
    QVERIFY(list.contains(QLatin1String("ArthuR"), Qt::CaseInsensitive));
    QVERIFY(list.contains(QLatin1String("ARTHUR"), Qt::CaseInsensitive));
    QVERIFY(list.contains(QLatin1String("dent"), Qt::CaseInsensitive));
    QVERIFY(!list.contains(QLatin1String("hans"), Qt::CaseInsensitive));

    QVERIFY(list.contains(QStringView(QString("arthur"))));
    QVERIFY(!list.contains(QStringView(QString("ArthuR"))));
    QVERIFY(!list.contains(QStringView(QString("Hans"))));
    QVERIFY(list.contains(QStringView(QString("arthur")), Qt::CaseInsensitive));
    QVERIFY(list.contains(QStringView(QString("ArthuR")), Qt::CaseInsensitive));
    QVERIFY(list.contains(QStringView(QString("ARTHUR")), Qt::CaseInsensitive));
    QVERIFY(list.contains(QStringView(QString("dent")), Qt::CaseInsensitive));
    QVERIFY(!list.contains(QStringView(QString("hans")), Qt::CaseInsensitive));
}

void tst_QStringList::removeDuplicates_data()
{
    QTest::addColumn<QString>("before");
    QTest::addColumn<QString>("after");
    QTest::addColumn<int>("count");
    QTest::addColumn<bool>("detached");

    QTest::newRow("empty-1") << "Hello,Hello" << "Hello" << 1 << true;
    QTest::newRow("empty-2") << "Hello,World" << "Hello,World" << 0 << false;
    QTest::newRow("middle")  << "Hello,World,Hello" << "Hello,World" << 1 << true;
}

void tst_QStringList::removeDuplicates()
{
    QFETCH(QString, before);
    QFETCH(QString, after);
    QFETCH(int, count);
    QFETCH(bool, detached);

    QStringList lbefore = before.split(',');
    const QStringList oldlbefore = lbefore;
    QStringList lafter = after.split(',');
    int removed = lbefore.removeDuplicates();

    QCOMPARE(removed, count);
    QCOMPARE(lbefore, lafter);
    QCOMPARE(detached, !oldlbefore.isSharedWith(lbefore));
}

void tst_QStringList::streamingOperator()
{
    QStringList list;
    list << "hei";
    list << list << "hopp" << list;

    QList<QString> slist = list;
    list << slist;

    QCOMPARE(list, QStringList()
            << "hei" << "hei" << "hopp"
            << "hei" << "hei" << "hopp"
            << "hei" << "hei" << "hopp"
            << "hei" << "hei" << "hopp");

    QStringList list2;
    list2 << "adam";

    QStringList list3;
    list3 << "eva";

    QCOMPARE(list2 << list3, QStringList() << "adam" << "eva");
}

void tst_QStringList::assignmentOperator()
{
    // compile-only test

    QStringList adam;
    adam << "adam";
    QList<QString> eva;
    eva << "eva";
    QStringList result;
    QStringList &ref1 = (result = adam);
    QStringList &ref2 = (result = eva);
    Q_UNUSED(ref1);
    Q_UNUSED(ref2);
}

void tst_QStringList::join() const
{
    QFETCH(QStringList, input);
    QFETCH(QString, separator);
    QFETCH(QString, expectedResult);

    QCOMPARE(input.join(separator), expectedResult);
    QCOMPARE(input.join(QLatin1String(separator.toLatin1())), expectedResult);
}

void tst_QStringList::join_data() const
{
    QTest::addColumn<QStringList>("input");
    QTest::addColumn<QString>("separator");
    QTest::addColumn<QString>("expectedResult");

    QTest::newRow("data1")
                << QStringList()
                << QString()
                << QString();

    QTest::newRow("data2")
                << QStringList()
                << QString(QLatin1String("separator"))
                << QString();

    QTest::newRow("data3")
                << QStringList("one")
                << QString(QLatin1String("separator"))
                << QString("one");

    QTest::newRow("data4")
                << QStringList("one")
                << QString(QLatin1String("separator"))
                << QString("one");


    QTest::newRow("data5")
                << (QStringList()
                        << QLatin1String("a")
                        << QLatin1String("b"))
                << QString(QLatin1String(" "))
                << QString("a b");

    QTest::newRow("data6")
                << (QStringList()
                        << QLatin1String("a")
                        << QLatin1String("b")
                        << QLatin1String("c"))
                << QString(QLatin1String(" "))
                << QString("a b c");
}

void tst_QStringList::joinChar() const
{
    QFETCH(QStringList, input);
    QFETCH(QChar, separator);
    QFETCH(QString, expectedResult);

    QCOMPARE(input.join(separator), expectedResult);
}

void tst_QStringList::joinChar_data() const
{
    QTest::addColumn<QStringList>("input");
    QTest::addColumn<QChar>("separator");
    QTest::addColumn<QString>("expectedResult");

    QTest::newRow("data1")
                << QStringList()
                << QChar(QLatin1Char(' '))
                << QString();

    QTest::newRow("data5")
                << (QStringList()
                        << QLatin1String("a")
                        << QLatin1String("b"))
                << QChar(QLatin1Char(' '))
                << QString("a b");

    QTest::newRow("data6")
                << (QStringList()
                        << QLatin1String("a")
                        << QLatin1String("b")
                        << QLatin1String("c"))
                << QChar(QLatin1Char(' '))
                << QString("a b c");
}

void tst_QStringList::joinEmptiness() const
{
    QStringList list;
    QString string = list.join(QString());

    QVERIFY(string.isEmpty());
    QVERIFY(string.isNull());
}

#ifdef Q_COMPILER_INITIALIZER_LISTS
// C++0x support is required
void tst_QStringList::initializeList() const
{

    QStringList v1{QLatin1String("hello"),"world",QString::fromLatin1("plop")};
    QCOMPARE(v1, (QStringList() << "hello" << "world" << "plop"));
    QCOMPARE(v1, (QStringList{"hello","world","plop"}));
}
#endif

QTEST_APPLESS_MAIN(tst_QStringList)
#include "tst_qstringlist.moc"
