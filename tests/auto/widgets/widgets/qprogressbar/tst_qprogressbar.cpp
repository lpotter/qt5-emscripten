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
#include "qprogressbar.h"
#include <qlocale.h>
#include <qapplication.h>
#include <qstyleoption.h>
#include <qdebug.h>
#include <qtimer.h>
#include <QStyleFactory>

class tst_QProgressBar : public QObject
{
Q_OBJECT
private slots:
    void getSetCheck();
    void minMaxSameValue();
    void destroyIndeterminate();
    void text();
    void format();
    void setValueRepaint_data();
    void setValueRepaint();
#ifndef Q_OS_MAC
    void setMinMaxRepaint();
#endif
    void sizeHint();
    void formatedText_data();
    void formatedText();
    void localizedFormattedText();

    void task245201_testChangeStyleAndDelete_data();
    void task245201_testChangeStyleAndDelete();
};

// Testing get/set functions
void tst_QProgressBar::getSetCheck()
{
    QProgressBar obj1;
    // bool QProgressBar::invertedAppearance()
    // void QProgressBar::setInvertedAppearance(bool)
    obj1.setInvertedAppearance(false);
    QCOMPARE(false, obj1.invertedAppearance());
    obj1.setInvertedAppearance(true);
    QCOMPARE(true, obj1.invertedAppearance());

    // int QProgressBar::minimum()
    // void QProgressBar::setMinimum(int)
    obj1.setMinimum(0);
    QCOMPARE(0, obj1.minimum());
    obj1.setMinimum(INT_MAX);
    QCOMPARE(INT_MAX, obj1.minimum());
    obj1.setMinimum(INT_MIN);
    QCOMPARE(INT_MIN, obj1.minimum());

    // int QProgressBar::maximum()
    // void QProgressBar::setMaximum(int)
    obj1.setMaximum(0);
    QCOMPARE(0, obj1.maximum());
    obj1.setMaximum(INT_MIN);
    QCOMPARE(INT_MIN, obj1.maximum());
    obj1.setMaximum(INT_MAX);
    QCOMPARE(INT_MAX, obj1.maximum());

    // int QProgressBar::value()
    // void QProgressBar::setValue(int)
    obj1.setValue(0);
    QCOMPARE(0, obj1.value());
    obj1.setValue(INT_MIN);
    QCOMPARE(INT_MIN, obj1.value());
    obj1.setValue(INT_MAX);
    QCOMPARE(INT_MAX, obj1.value());
}

void tst_QProgressBar::minMaxSameValue()
{
    QProgressBar bar;
    bar.setRange(10, 10);
    bar.setValue(10);
    bar.move(300, 300);
    bar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&bar));
}

void tst_QProgressBar::destroyIndeterminate()
{
    // This test crashes on styles that animate indeterminate / busy
    // progressbars, and forget to remove the bars from internal logics when
    // it's deleted.
    QPointer<QProgressBar> bar = new QProgressBar;
    bar->setMaximum(0);
    bar->move(300, 300);
    bar->show();
    QVERIFY(QTest::qWaitForWindowExposed(bar.data()));

    QEventLoop loop;
    QTimer::singleShot(500, bar, SLOT(deleteLater()));
    QTimer::singleShot(3000, &loop, SLOT(quit()));
    loop.exec();

    QVERIFY(!bar);
}

void tst_QProgressBar::text()
{
    QProgressBar bar;
    bar.setRange(10, 10);
    bar.setValue(10);
    QCOMPARE(bar.text(), QString("100%"));
    bar.setRange(0, 10);
    QCOMPARE(bar.text(), QString("100%"));
    bar.setValue(5);
    QCOMPARE(bar.text(), QString("50%"));
    bar.setRange(0, 5);
    bar.setValue(0);
    bar.setRange(5, 5);
    QCOMPARE(bar.text(), QString());
}

class ProgressBar : public QProgressBar
{
    void paintEvent(QPaintEvent *event)
    {
        repainted = true;
        QProgressBar::paintEvent(event);
    }
public:
    bool repainted;
    using QProgressBar::initStyleOption;
};

void tst_QProgressBar::format()
{
    ProgressBar bar;
    bar.setRange(0, 10);
    bar.setValue(1);
    bar.move(300, 300);
    bar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&bar));

    bar.repainted = false;
    bar.setFormat("%v of %m (%p%)");
    QTRY_VERIFY(bar.repainted);
    bar.repainted = false;
    bar.setFormat("%v of %m (%p%)");
    qApp->processEvents();

#ifndef Q_OS_MAC
    // Animated scroll bars get paint events all the time
#ifdef Q_OS_WIN
    if (QSysInfo::WindowsVersion < QSysInfo::WV_VISTA)
#endif
    QVERIFY(!bar.repainted);
#endif

    QCOMPARE(bar.text(), QString("1 of 10 (10%)"));
    bar.setRange(5, 5);
    bar.setValue(5);
    QCOMPARE(bar.text(), QString("5 of 0 (100%)"));
    bar.setRange(0, 5);
    bar.setValue(0);
    bar.setRange(5, 5);
    QCOMPARE(bar.text(), QString());
}

void tst_QProgressBar::setValueRepaint_data()
{
    QTest::addColumn<int>("min");
    QTest::addColumn<int>("max");
    QTest::addColumn<int>("increment");

    auto add = [](int min, int max, int increment) {
        QTest::addRow("%d-%d@%d", min, max, increment) << min << max << increment;
    };

    add(0, 10, 1);

    auto add_set = [=](int min, int max, int increment) {
        // check corner cases, and adjacent values (to circumvent explicit checks for corner cases)
        add(min,     max,     increment);
        add(min + 1, max,     increment);
        add(min,     max - 1, increment);
        add(min + 1, max - 1, increment);
    };

    add_set(INT_MIN, INT_MAX, INT_MAX / 50 + 1);
    add_set(0, INT_MAX, INT_MAX / 100 + 1);
    add_set(INT_MIN, 0, INT_MAX / 100 + 1);
}

void tst_QProgressBar::setValueRepaint()
{
    QFETCH(int, min);
    QFETCH(int, max);
    QFETCH(int, increment);

    ProgressBar pbar;
    pbar.setMinimum(min);
    pbar.setMaximum(max);
    pbar.setFormat("%v");
    pbar.move(300, 300);
    pbar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&pbar));

    QApplication::processEvents();
    for (qint64 i = min; i < max; i += increment) {
        pbar.repainted = false;
        pbar.setValue(int(i));
        QTRY_VERIFY(pbar.repainted);
    }
}

// This test is invalid on Mac, since progressbars
// are animated there

#ifndef Q_OS_MAC
void tst_QProgressBar::setMinMaxRepaint()
{
    ProgressBar pbar;
    pbar.setMinimum(0);
    pbar.setMaximum(10);
    pbar.setFormat("%v");
    pbar.move(300, 300);
    pbar.show();
    qApp->setActiveWindow(&pbar);
    QVERIFY(QTest::qWaitForWindowActive(&pbar));

    // No repaint when setting minimum to the current minimum
    pbar.repainted = false;
    pbar.setMinimum(0);
    QTest::qWait(50);
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Broken on WinRT - QTBUG-68297", Abort);
#endif
    QTRY_VERIFY(!pbar.repainted);

    // No repaint when setting maximum to the current maximum
    pbar.repainted = false;
    pbar.setMaximum(10);
    QTest::qWait(50);
    QTRY_VERIFY(!pbar.repainted);

    // Repaint when setting minimum
    for (int i = 9; i >= 0; i--) {
        pbar.repainted = false;
        pbar.setMinimum(i);
        QTRY_VERIFY(pbar.repainted);
    }

    // Repaint when setting maximum
    for (int i = 0; i < 10; ++i) {
        pbar.repainted = false;
        pbar.setMaximum(i);
        QTRY_VERIFY(pbar.repainted);
    }
}
#endif //Q_OS_MAC

void tst_QProgressBar::sizeHint()
{
    ProgressBar bar;
    bar.setMinimum(0);
    bar.setMaximum(10);
    bar.setValue(5);

    //test if the sizeHint is big enough
    QFontMetrics fm = bar.fontMetrics();
    QStyleOptionProgressBar opt;
    bar.initStyleOption(&opt);
    QSize size = QSize(9 * 7 + fm.horizontalAdvance(QLatin1Char('0')) * 4, fm.height() + 8);
    size= bar.style()->sizeFromContents(QStyle::CT_ProgressBar, &opt, size, &bar);
    QSize barSize = bar.sizeHint();
    QVERIFY(barSize.width() >= size.width());
    QCOMPARE(barSize.height(), size.height());
}

void tst_QProgressBar::formatedText_data()
{
    QTest::addColumn<int>("minimum");
    QTest::addColumn<int>("maximum");
    QTest::addColumn<int>("value");
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("text");

    QTest::newRow("1") <<  -100 << 100 << 0 << QString::fromLatin1(" %p - %v - %m ") << QString::fromLatin1(" 50 - 0 - 200 ");
    QTest::newRow("2") <<  -100 << 0 << -25 << QString::fromLatin1(" %p - %v - %m ") << QString::fromLatin1(" 75 - -25 - 100 ");
    QTest::newRow("3") <<  10 << 10 << 10 << QString::fromLatin1(" %p - %v - %m ") << QString::fromLatin1(" 100 - 10 - 0 ");
    QTest::newRow("task152227") <<  INT_MIN << INT_MAX << 42 << QString::fromLatin1(" %p - %v - %m ") << QString::fromLatin1(" 50 - 42 - 4294967295 ");
}

void tst_QProgressBar::formatedText()
{
    QFETCH(int, minimum);
    QFETCH(int, maximum);
    QFETCH(int, value);
    QFETCH(QString, format);
    QFETCH(QString, text);
    QProgressBar bar;
    bar.setRange(minimum, maximum);
    bar.setValue(value);
    bar.setFormat(format);
    QCOMPARE(bar.text(), text);
}

void tst_QProgressBar::localizedFormattedText() // QTBUG-28751
{
    QProgressBar bar;
    const int value = 42;
    bar.setValue(value);
    const QString defaultExpectedNumber = QString::number(value);
    const QString defaultExpectedValue = defaultExpectedNumber + QLatin1Char('%');
    QCOMPARE(bar.text(), defaultExpectedValue);

    // Temporarily switch to Egyptian, which has a different percent sign and number formatting
    QLocale egypt(QLocale::Arabic, QLocale::Egypt);
    bar.setLocale(egypt);
    const QString egyptianExpectedNumber = egypt.toString(value);
    const QString egyptianExpectedValue = egyptianExpectedNumber + egypt.percent();
    if (egyptianExpectedValue == defaultExpectedValue)
        QSKIP("Egyptian locale does not work on this system.");
    QCOMPARE(bar.text(), egyptianExpectedValue);

    bar.setLocale(QLocale());
    QCOMPARE(bar.text(), defaultExpectedValue);

    // Set a custom format containing only the number
    bar.setFormat(QStringLiteral("%p"));
    QCOMPARE(bar.text(), defaultExpectedNumber);
    bar.setLocale(egypt);
    QCOMPARE(bar.text(), egyptianExpectedNumber);

    // Clear the format
    bar.resetFormat();
    QCOMPARE(bar.text(), egyptianExpectedValue);
    bar.setLocale(QLocale());
    QCOMPARE(bar.text(), defaultExpectedValue);
}

void tst_QProgressBar::task245201_testChangeStyleAndDelete_data()
{
    QTest::addColumn<QString>("style1_str");
    QTest::addColumn<QString>("style2_str");

    QTest::newRow("fusion-windows") << QString::fromLatin1("fusion") << QString::fromLatin1("windows");
    QTest::newRow("gtk-fusion") << QString::fromLatin1("gtk") << QString::fromLatin1("fusion");
}

void tst_QProgressBar::task245201_testChangeStyleAndDelete()
{
    QFETCH(QString, style1_str);
    QFETCH(QString, style2_str);

    QProgressBar *bar = new QProgressBar;

    QStyle *style = QStyleFactory::create(style1_str);
    bar->setStyle(style);
    bar->move(300, 300);
    bar->show();
    QVERIFY(QTest::qWaitForWindowExposed(bar));
    QStyle *style2 = QStyleFactory::create(style2_str);
    bar->setStyle(style2);
    QTest::qWait(10);

    delete bar;
    QTest::qWait(100); //should not crash
    delete style;
    delete style2;
}

QTEST_MAIN(tst_QProgressBar)
#include "tst_qprogressbar.moc"
