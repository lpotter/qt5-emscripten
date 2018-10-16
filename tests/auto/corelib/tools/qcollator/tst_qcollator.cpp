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

#include <qlocale.h>
#include <qcollator.h>
#include <private/qglobal_p.h>

#include <cstring>

class tst_QCollator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void moveSemantics();

    void compare_data();
    void compare();

    void state();
};

#ifdef Q_COMPILER_RVALUE_REFS
static bool dpointer_is_null(QCollator &c)
{
    char mem[sizeof c];
    using namespace std;
    memcpy(mem, &c, sizeof c);
    for (size_t i = 0; i < sizeof c; ++i)
        if (mem[i])
            return false;
    return true;
}
#endif

void tst_QCollator::moveSemantics()
{
#ifdef Q_COMPILER_RVALUE_REFS
    const QLocale de_AT(QLocale::German, QLocale::Austria);

    QCollator c1(de_AT);
    QCOMPARE(c1.locale(), de_AT);

    QCollator c2(std::move(c1));
    QCOMPARE(c2.locale(), de_AT);
    QVERIFY(dpointer_is_null(c1));

    QCollator c3(c1);
    QVERIFY(dpointer_is_null(c3));

    c1 = std::move(c2);
    QCOMPARE(c1.locale(), de_AT);
    QVERIFY(dpointer_is_null(c2));
#else
    QSKIP("The compiler is not in C++11 mode or does not support move semantics.");
#endif
}


void tst_QCollator::compare_data()
{
    QTest::addColumn<QString>("locale");
    QTest::addColumn<QString>("s1");
    QTest::addColumn<QString>("s2");
    QTest::addColumn<int>("result");
    QTest::addColumn<int>("caseInsensitiveResult");
    QTest::addColumn<bool>("numericMode");
    QTest::addColumn<bool>("ignorePunctuation");
    QTest::addColumn<int>("punctuationResult");

    /*
        A few tests below are commented out on the mac. It's unclear why they fail,
        as it looks like the collator for the locale is created correctly.
    */

    /*
        It's hard to test English, because it's treated differently
        on different platforms. For example, on Linux, it uses the
        iso14651_t1 template file, which happens to provide good
        defaults for Swedish. OS X seems to do a pure bytewise
        comparison of Latin-1 values, although I'm not sure. So I
        just test digits to make sure that it's not totally broken.
    */
    QTest::newRow("english1") << QString("en_US") << QString("5") << QString("4") << 1 << 1 << false << false << 1;
    QTest::newRow("english2") << QString("en_US") << QString("4") << QString("6") << -1 << -1 << false << false << -1;
    QTest::newRow("english3") << QString("en_US") << QString("5") << QString("6") << -1 << -1 << false << false << -1;
    QTest::newRow("english4") << QString("en_US") << QString("a") << QString("b") << -1 << -1 << false << false << -1;
    QTest::newRow("english5") << QString("en_US") << QString("test 9") << QString("test 19") << -1 << -1 << true << false << -1;
    QTest::newRow("english6") << QString("en_US") << QString("test 9") << QString("test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("english7") << QString("en_US") << QString("test_19") << QString("test 19") << 1 << 1 << true << false << 1;
    QTest::newRow("english8") << QString("en_US") << QString("test.19") << QString("test,19") << 1 << 1 << true << true << 0;

    /*
        In Swedish, a with ring above (E5) comes before a with
        diaresis (E4), which comes before o diaresis (F6), which
        all come after z.
    */
    QTest::newRow("swedish1") << QString("sv_SE") << QString::fromLatin1("\xe5") << QString::fromLatin1("\xe4") << -1 << -1 << false << false << -1;
    QTest::newRow("swedish2") << QString("sv_SE") << QString::fromLatin1("\xe4") << QString::fromLatin1("\xf6") << -1 << -1 << false << false << -1;
    QTest::newRow("swedish3") << QString("sv_SE") << QString::fromLatin1("\xe5") << QString::fromLatin1("\xf6") << -1 << -1 << false << false << -1;
    QTest::newRow("swedish4") << QString("sv_SE") << QString::fromLatin1("z") << QString::fromLatin1("\xe5") << -1 << -1 << false << false << -1;
    QTest::newRow("swedish5") << QString("sv_SE") << QString("9") << QString("19") << -1 << -1 << true << false << -1;
    QTest::newRow("swedish6") << QString("sv_SE") << QString("Test 9") << QString("Test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("swedish7") << QString("sv_SE") << QString("test_19") << QString("test 19") << 1 << 1 << true << false << 1;
    QTest::newRow("swedish8") << QString("sv_SE") << QString("test.19") << QString("test,19") << 1 << 1 << true << true << 0;


    /*
        In Norwegian, ae (E6) comes before o with stroke (D8), which
        comes before a with ring above (E5).
    */
    QTest::newRow("norwegian1") << QString("no_NO") << QString::fromLatin1("\xe6") << QString::fromLatin1("\xd8") << -1 << -1 << false << false << -1;
    QTest::newRow("norwegian2") << QString("no_NO") << QString::fromLatin1("\xd8") << QString::fromLatin1("\xe5") << -1 << -1 << false << false << -1;
    QTest::newRow("norwegian3") << QString("no_NO") << QString::fromLatin1("\xe6") << QString::fromLatin1("\xe5") << -1 << -1 << false << false << -1;
    QTest::newRow("norwegian4") << QString("no_NO") << QString("9") << QString("19") << -1 << -1 << true << false << -1;
    QTest::newRow("norwegian5") << QString("no_NO") << QString("Test 9") << QString("Test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("norwegian6") << QString("no_NO") << QString("Test 9") << QString("Test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("norwegian7") << QString("no_NO") << QString("test_19") << QString("test 19") << 1 << 1 << true << false << 1;
    QTest::newRow("norwegian8") << QString("no_NO") << QString("test.19") << QString("test,19") << 1 << 1 << true << true << 0;

    /*
        In German, z comes *after* a with diaresis (E4),
        which comes before o diaresis (F6).
    */
    QTest::newRow("german1") << QString("de_DE") << QString::fromLatin1("a") << QString::fromLatin1("\xe4") << -1 << -1 << false << false << -1;
    QTest::newRow("german2") << QString("de_DE") << QString::fromLatin1("b") << QString::fromLatin1("\xe4") << 1 << 1 << false << false << 1;
    QTest::newRow("german3") << QString("de_DE") << QString::fromLatin1("z") << QString::fromLatin1("\xe4") << 1 << 1 << false << false << 1;
    QTest::newRow("german4") << QString("de_DE") << QString::fromLatin1("\xe4") << QString::fromLatin1("\xf6") << -1 << -1 << false << false << -1;
    QTest::newRow("german5") << QString("de_DE") << QString::fromLatin1("z") << QString::fromLatin1("\xf6") << 1 << 1 << false << false << 1;
    QTest::newRow("german6") << QString("de_DE") << QString::fromLatin1("\xc0") << QString::fromLatin1("\xe0") << 1 << 0 << false << false << 0;
    QTest::newRow("german7") << QString("de_DE") << QString::fromLatin1("\xd6") << QString::fromLatin1("\xf6") << 1 << 0 << false << false << 0;
    QTest::newRow("german8") << QString("de_DE") << QString::fromLatin1("oe") << QString::fromLatin1("\xf6") << 1 << 1 << false << false << 1;
    QTest::newRow("german9") << QString("de_DE") << QString("A") << QString("a") << 1 << 0 << false << false << 0;
    QTest::newRow("german10") << QString("de_DE") << QString("9") << QString("19") << -1 << -1 << true << false << -1;
    QTest::newRow("german11") << QString("de_DE") << QString("Test 9") << QString("Test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("german12") << QString("de_DE") << QString("test_19") << QString("test 19") << 1 << 1 << true << false << 1;
    QTest::newRow("german13") << QString("de_DE") << QString("test.19") << QString("test,19") << 1 << 1 << true << true << 0;

    /*
        French sorting of e and e with accent
    */
    QTest::newRow("french1") << QString("fr_FR") << QString::fromLatin1("\xe9") << QString::fromLatin1("e") << 1 << 1 << false << false << 1;
    QTest::newRow("french2") << QString("fr_FR") << QString::fromLatin1("\xe9t") << QString::fromLatin1("et") << 1 << 1 << false << false << 1;
    QTest::newRow("french3") << QString("fr_FR") << QString::fromLatin1("\xe9") << QString::fromLatin1("d") << 1 << 1 << false << false << 1;
    QTest::newRow("french4") << QString("fr_FR") << QString::fromLatin1("\xe9") << QString::fromLatin1("f") << -1 << -1 << false << false << -1;
    QTest::newRow("french5") << QString("fr_FR") << QString("9") << QString("19") << -1 << -1 << true << false << -1;
    QTest::newRow("french6") << QString("fr_FR") << QString("Test 9") << QString("Test_19") << -1 << -1 << true << true << -1;
    QTest::newRow("french7") << QString("fr_FR") << QString("test_19") << QString("test 19") << 1 << 1 << true << false << 1;
    QTest::newRow("french8") << QString("fr_FR") << QString("test.19") << QString("test,19") << 1 << 1 << true << true << 0;
}


void tst_QCollator::compare()
{
    QFETCH(QString, locale);
    QFETCH(QString, s1);
    QFETCH(QString, s2);
    QFETCH(int, result);
    QFETCH(int, caseInsensitiveResult);
    QFETCH(bool, numericMode);
    QFETCH(bool, ignorePunctuation);
    QFETCH(int, punctuationResult);

    QCollator collator(locale);

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
    if (collator.locale() != QLocale())
        QSKIP("Posix implementation of collation only supports default locale");
#endif

    if (numericMode)
        collator.setNumericMode(true);

    QCOMPARE(collator.compare(s1, s2), result);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    QCOMPARE(collator.compare(s1, s2), caseInsensitiveResult);
#if !QT_CONFIG(iconv)
    collator.setIgnorePunctuation(ignorePunctuation);
    QCOMPARE(collator.compare(s1, s2), punctuationResult);
#endif
}


void tst_QCollator::state()
{
    QCollator c;
    c.setCaseSensitivity(Qt::CaseInsensitive);
    c.setLocale(QLocale::German);

    c.compare(QString("a"), QString("b"));

    QCOMPARE(c.caseSensitivity(), Qt::CaseInsensitive);
    QCOMPARE(c.locale(), QLocale(QLocale::German));

    c.setLocale(QLocale::French);
    c.setNumericMode(true);
    c.setIgnorePunctuation(true);
    c.setLocale(QLocale::Norwegian);

    QCOMPARE(c.caseSensitivity(), Qt::CaseInsensitive);
    QCOMPARE(c.numericMode(), true);
    QCOMPARE(c.ignorePunctuation(), true);
    QCOMPARE(c.locale(), QLocale(QLocale::Norwegian));

}

QTEST_APPLESS_MAIN(tst_QCollator)

#include "tst_qcollator.moc"
