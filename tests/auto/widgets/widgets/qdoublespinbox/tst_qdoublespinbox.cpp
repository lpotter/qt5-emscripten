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
#include <qapplication.h>
#include <limits.h>

#include <cmath>
#include <float.h>

#include <qspinbox.h>
#include <qlocale.h>
#include <qlayout.h>

#include <qlineedit.h>
#include <qdebug.h>

#include <QStyleOptionSpinBox>
#include <QStyle>
#include <QProxyStyle>

class DoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    DoubleSpinBox(QWidget *parent = 0)
        : QDoubleSpinBox(parent)
    { /*connect(this, SIGNAL(valueChanged(double)), this, SLOT(foo(double)));*/ }
    QString textFromValue(double v) const
    {
        return QDoubleSpinBox::textFromValue(v);
    }
    QValidator::State validate(QString &text, int &pos) const
    {
        return QDoubleSpinBox::validate(text, pos);
    }
    double valueFromText(const QString &text) const
    {
        return QDoubleSpinBox::valueFromText(text);
    }
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event)
    {
        QDoubleSpinBox::wheelEvent(event);
    }
#endif
    void initStyleOption(QStyleOptionSpinBox *option) const
    {
        QDoubleSpinBox::initStyleOption(option);
    }

    QLineEdit* lineEdit() const { return QDoubleSpinBox::lineEdit(); }
public slots:
    void foo(double vla)
    {
        qDebug() << vla;
    }
};

class PressAndHoldStyle : public QProxyStyle
{
    Q_OBJECT
public:
    using QProxyStyle::QProxyStyle;

    int styleHint(QStyle::StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override
    {
        switch (hint) {
        case QStyle::SH_SpinBox_ClickAutoRepeatRate:
            return 5;
        case QStyle::SH_SpinBox_ClickAutoRepeatThreshold:
            return 10;
        default:
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    }
};

class StepModifierStyle : public QProxyStyle
{
    Q_OBJECT
public:
    using QProxyStyle::QProxyStyle;

    int styleHint(QStyle::StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override
    {
        switch (hint) {
        case QStyle::SH_SpinBox_StepModifier:
            return stepModifier;
        default:
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    }

    Qt::KeyboardModifier stepModifier = Qt::ControlModifier;
};


class tst_QDoubleSpinBox : public QObject
{
    Q_OBJECT
public:
    tst_QDoubleSpinBox();
    virtual ~tst_QDoubleSpinBox();
public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

private slots:
    void germanTest();

    void task54433();

    void setValue_data();
    void setValue();

    void setPrefixSuffix_data();
    void setPrefixSuffix();

    void setTracking_data();
    void setTracking();

    void setWrapping_data();
    void setWrapping();

    void setSpecialValueText_data();
    void setSpecialValueText();

    void setSingleStep_data();
    void setSingleStep();

    void setMinMax_data();
    void setMinMax();

    void setDecimals_data();
    void setDecimals();

    void doubleDot();

    void undoRedo();

    void valueFromTextAndValidate_data();
    void valueFromTextAndValidate();

    void setReadOnly();

    void editingFinished();

    void removeAll();

    void task199226_stateAfterEnter();
    void task224497_fltMax();

    void task221221();
    void task255471_decimalsValidation();

    void taskQTBUG_5008_textFromValueAndValidate();
    void taskQTBUG_6670_selectAllWithPrefix();
    void taskQTBUG_6496_fiddlingWithPrecision();

    void setGroupSeparatorShown_data();
    void setGroupSeparatorShown();

    void adaptiveDecimalStep();

    void wheelEvents_data();
    void wheelEvents();

    void stepModifierKeys_data();
    void stepModifierKeys();

    void stepModifierButtons_data();
    void stepModifierButtons();

    void stepModifierPressAndHold_data();
    void stepModifierPressAndHold();
public slots:
    void valueChangedHelper(const QString &);
    void valueChangedHelper(double);
private:
    QStringList actualTexts;
    QList<double> actualValues;
    QWidget *testFocusWidget;
};

typedef QList<double> DoubleList;

Q_DECLARE_METATYPE(QLocale::Language)
Q_DECLARE_METATYPE(QLocale::Country)

static QLatin1String modifierToName(Qt::KeyboardModifier modifier)
{
    switch (modifier) {
    case Qt::NoModifier:
        return QLatin1Literal("No");
        break;
    case Qt::ControlModifier:
        return QLatin1Literal("Ctrl");
        break;
    case Qt::ShiftModifier:
        return QLatin1Literal("Shift");
        break;
    case Qt::AltModifier:
        return QLatin1Literal("Alt");
        break;
    case Qt::MetaModifier:
        return QLatin1Literal("Meta");
        break;
    default:
        qFatal("Unexpected keyboard modifier");
        return QLatin1String();
    }
}

tst_QDoubleSpinBox::tst_QDoubleSpinBox()

{
}

tst_QDoubleSpinBox::~tst_QDoubleSpinBox()
{

}

void tst_QDoubleSpinBox::initTestCase()
{
    testFocusWidget = new QWidget(0);
    testFocusWidget->resize(200, 100);
    testFocusWidget->show();
    QVERIFY(QTest::qWaitForWindowActive(testFocusWidget));
}

void tst_QDoubleSpinBox::cleanupTestCase()
{
    delete testFocusWidget;
    testFocusWidget = 0;
}

void tst_QDoubleSpinBox::init()
{
    QLocale::setDefault(QLocale(QLocale::C));
}

void tst_QDoubleSpinBox::setValue_data()
{
    QTest::addColumn<double>("val");

    QTest::newRow("data0") << 0.0;
    QTest::newRow("data1") << 100.5;
    QTest::newRow("data2") << -100.5;
    QTest::newRow("data3") << -DBL_MAX;
    QTest::newRow("data4") << DBL_MAX;
}

void tst_QDoubleSpinBox::setValue()
{
    QFETCH(double, val);
    QDoubleSpinBox spin(0);
    spin.setRange(-DBL_MAX, DBL_MAX);
    spin.setValue(val);
    QCOMPARE(spin.value(), val);
}

void tst_QDoubleSpinBox::setPrefixSuffix_data()
{
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<double>("value");
    QTest::addColumn<int>("decimals");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<QString>("expectedCleanText");
    QTest::addColumn<bool>("show");

    QTest::newRow("data0") << QString() << QString() << 10.5 << 1 << "10.5" << "10.5" << false;
    QTest::newRow("data1") << QString() << "cm" << 10.5 << 2 << "10.50cm" << "10.50" << false;

    // 10.5 is rounded correctly to 11 when using libdouble-conversion, or incorrectly to 10 when
    // using snprintf. This is not the point of this test, though.
    QTest::newRow("data2") << "cm: " << QString() << 10.4 << 0 << "cm: 10" << "10" << false;
    QTest::newRow("data3") << "length: " << "cm" << 10.5 << 3 << "length: 10.500cm" << "10.500" << false;

    QTest::newRow("data4") << QString() << QString() << 10.5 << 1 << "10.5" << "10.5" << true;
    QTest::newRow("data5") << QString() << "cm" << 10.5 << 2 << "10.50cm" << "10.50" << true;
    QTest::newRow("data6") << "cm: " << QString() << 10.4 << 0 << "cm: 10" << "10" << true;
    QTest::newRow("data7") << "length: " << "cm" << 10.5 << 3 << "length: 10.500cm" << "10.500" << true;
}

void tst_QDoubleSpinBox::setPrefixSuffix()
{
    QFETCH(QString, prefix);
    QFETCH(QString, suffix);
    QFETCH(double, value);
    QFETCH(int, decimals);
    QFETCH(QString, expectedText);
    QFETCH(QString, expectedCleanText);
    QFETCH(bool, show);

    QDoubleSpinBox spin(0);
    spin.setDecimals(decimals);
    spin.setPrefix(prefix);
    spin.setSuffix(suffix);
    spin.setValue(value);
    if (show)
        spin.show();

    QCOMPARE(spin.text(), expectedText);
    QCOMPARE(spin.cleanText(), expectedCleanText);
}

void tst_QDoubleSpinBox::valueChangedHelper(const QString &text)
{
    actualTexts << text;
}

void tst_QDoubleSpinBox::valueChangedHelper(double value)
{
    actualValues << value;
}

void tst_QDoubleSpinBox::setTracking_data()
{
    QTest::addColumn<int>("decimals");
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<QStringList>("texts");
    QTest::addColumn<bool>("tracking");

    QTestEventList keys;
    QStringList texts1;
    QStringList texts2;
#ifdef Q_OS_MAC
    keys.addKeyClick(Qt::Key_Right, Qt::ControlModifier);
#else
    keys.addKeyClick(Qt::Key_End);
#endif
    keys.addKeyClick(Qt::Key_Backspace);
    keys.addKeyClick(Qt::Key_Backspace);
    keys.addKeyClick(Qt::Key_Backspace);
    keys.addKeyClick(Qt::Key_Backspace);
    keys.addKeyClick(Qt::Key_Backspace);
    keys.addKeyClick(Qt::Key_Backspace);
    keys.addKeyClick(Qt::Key_Backspace);
    keys.addKeyClick(Qt::Key_Backspace);
    keys.addKeyClick('7');
    keys.addKeyClick('.');
    keys.addKeyClick('9');
    keys.addKeyClick(Qt::Key_Backspace);
    keys.addKeyClick('2');
    keys.addKeyClick(Qt::Key_Enter);
    keys.addKeyClick(Qt::Key_Enter);
    keys.addKeyClick(Qt::Key_Enter);
    texts1 << "7" << "7.9" << "7." << "7.2" << "7.200" << "7.200" << "7.200";
    texts2 << "7.200";
    QTest::newRow("data1") << 3 << keys << texts1 << true;
    QTest::newRow("data2") << 3 << keys << texts2 << false;

}

void tst_QDoubleSpinBox::setTracking()
{
    QLocale::setDefault(QLocale(QLocale::C));

    actualTexts.clear();
    QFETCH(int, decimals);
    QFETCH(QTestEventList, keys);
    QFETCH(QStringList, texts);
    QFETCH(bool, tracking);

    QDoubleSpinBox spin(0);
    spin.setKeyboardTracking(tracking);
    spin.setDecimals(decimals);
    spin.show();

    connect(&spin, SIGNAL(valueChanged(QString)), this, SLOT(valueChangedHelper(QString)));

    keys.simulate(&spin);
    QCOMPARE(actualTexts, texts);
}

void tst_QDoubleSpinBox::setWrapping_data()
{
    QTest::addColumn<bool>("wrapping");
    QTest::addColumn<double>("minimum");
    QTest::addColumn<double>("maximum");
    QTest::addColumn<double>("startValue");
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<DoubleList>("expected");

    QTestEventList keys;
    DoubleList values;
    keys.addKeyClick(Qt::Key_Up);
    values << 10;
    keys.addKeyClick(Qt::Key_Up);
    QTest::newRow("data0") << false << 0.0 << 10.0 << 9.0 << keys << values;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_Up);
    values << 10;
    keys.addKeyClick(Qt::Key_Up);
    values << 0;
    QTest::newRow("data1") << true << 0.0 << 10.0 << 9.0 << keys << values;

    keys.clear();
    values.clear();
#ifdef Q_OS_MAC
    keys.addKeyClick(Qt::Key_Right, Qt::ControlModifier);
#else
    keys.addKeyClick(Qt::Key_End);
#endif
    keys.addKeyClick(Qt::Key_Backspace);
    keys.addKeyClick('1');
    keys.addKeyClick(Qt::Key_Down);
    keys.addKeyClick(Qt::Key_Down);
    keys.addKeyClick(Qt::Key_PageDown);
    values << 9.01 << 8.01 << 7.01 << 0.0;
    QTest::newRow("data2") << false << 0.0 << 10.0 << 9.0 << keys << values;

    keys.clear();
    values.clear();
#ifdef Q_OS_MAC
    keys.addKeyClick(Qt::Key_Left, Qt::ControlModifier);
#else
    keys.addKeyClick(Qt::Key_Home);
#endif
    keys.addKeyClick(Qt::Key_Delete);
    keys.addKeyClick(Qt::Key_Delete);
    keys.addKeyClick(Qt::Key_Delete);
    keys.addKeyClick(Qt::Key_Delete);
    keys.addKeyClick(Qt::Key_Delete);
    keys.addKeyClick(Qt::Key_Delete);
    keys.addKeyClick(Qt::Key_Delete);
    keys.addKeyClick('1');
    keys.addKeyClick(Qt::Key_Down);
    keys.addKeyClick(Qt::Key_Down);
    values << 0 << 1 << 0 << 10;
    QTest::newRow("data3") << true << 0.0 << 10.0 << 9.0 << keys << values;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_PageDown);
    keys.addKeyClick(Qt::Key_Down);
    values << 0;
    QTest::newRow("data4") << false << 0.0 << 10.0 << 6.0 << keys << values;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_PageDown);
    keys.addKeyClick(Qt::Key_Down);
    values << 0 << 10;
    QTest::newRow("data5") << true << 0.0 << 10.0 << 6.0 << keys << values;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_PageUp);
    keys.addKeyClick(Qt::Key_PageDown);
    keys.addKeyClick(Qt::Key_Down);
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_PageDown);
    keys.addKeyClick(Qt::Key_PageDown);
    values << 10 << 0 << 10 << 0 << 10 << 0;
    QTest::newRow("data6") << true << 0.0 << 10.0 << 6.0 << keys << values;

}


void tst_QDoubleSpinBox::setWrapping()
{
    QLocale::setDefault(QLocale(QLocale::C));
    QFETCH(bool, wrapping);
    QFETCH(double, minimum);
    QFETCH(double, maximum);
    QFETCH(double, startValue);
    QFETCH(QTestEventList, keys);
    QFETCH(DoubleList, expected);

    QDoubleSpinBox spin(0);
    spin.setMinimum(minimum);
    spin.setMaximum(maximum);
    spin.setValue(startValue);
    spin.setWrapping(wrapping);
    spin.show();
    actualValues.clear();
    connect(&spin, SIGNAL(valueChanged(double)), this, SLOT(valueChangedHelper(double)));

    keys.simulate(&spin);

    QCOMPARE(actualValues.size(), expected.size());
    for (int i=0; i<qMin(actualValues.size(), expected.size()); ++i) {
        QCOMPARE(actualValues.at(i), expected.at(i));
    }
}

void tst_QDoubleSpinBox::setSpecialValueText_data()
{
    QTest::addColumn<QString>("specialValueText");
    QTest::addColumn<double>("minimum");
    QTest::addColumn<double>("maximum");
    QTest::addColumn<double>("value");
    QTest::addColumn<int>("decimals");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("show");

    QTest::newRow("data0") << QString() << 0.0 << 10.0 << 1.0 << 4 << "1.0000" << false;
    QTest::newRow("data1") << QString() << 0.0 << 10.0 << 1.0 << 4 << "1.0000" << true;
    QTest::newRow("data2") << "foo" << 0.0 << 10.0 << 0.0 << 2 << "foo" << false;
    QTest::newRow("data3") << "foo" << 0.0 << 10.0 << 0.0 << 2 << "foo" << true;
}

void tst_QDoubleSpinBox::setSpecialValueText()
{
    QFETCH(QString, specialValueText);
    QFETCH(double, minimum);
    QFETCH(double, maximum);
    QFETCH(double, value);
    QFETCH(int, decimals);
    QFETCH(QString, expected);
    QFETCH(bool, show);

    QDoubleSpinBox spin(0);
    spin.setSpecialValueText(specialValueText);
    spin.setMinimum(minimum);
    spin.setMaximum(maximum);
    spin.setValue(value);
    spin.setDecimals(decimals);
    if (show)
        spin.show();

    QCOMPARE(spin.text(), expected);
}

void tst_QDoubleSpinBox::setSingleStep_data()
{
    QTest::addColumn<double>("singleStep");
    QTest::addColumn<double>("startValue");
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<DoubleList>("expected");
    QTest::addColumn<bool>("show");

    QTestEventList keys;
    DoubleList values;
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Down);
    keys.addKeyClick(Qt::Key_Up);
    values << 11 << 10 << 11;
    QTest::newRow("data0") << 1.0 << 10.0 << keys << values << false;
    QTest::newRow("data1") << 1.0 << 10.0 << keys << values << true;

    keys.clear();
    values.clear();
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Down);
    keys.addKeyClick(Qt::Key_Up);
    values << 12.5 << 10.0 << 12.5;
    QTest::newRow("data2") << 2.5 << 10.0 << keys << values << false;
    QTest::newRow("data3") << 2.5 << 10.0 << keys << values << true;
}

void tst_QDoubleSpinBox::setSingleStep()
{
    QFETCH(double, singleStep);
    QFETCH(double, startValue);
    QFETCH(QTestEventList, keys);
    QFETCH(DoubleList, expected);
    QFETCH(bool, show);

    QDoubleSpinBox spin(0);
    actualValues.clear();
    spin.setSingleStep(singleStep);
    spin.setValue(startValue);
    if (show)
        spin.show();
    connect(&spin, SIGNAL(valueChanged(double)), this, SLOT(valueChangedHelper(double)));

    QCOMPARE(actualValues.size(), 0);
    keys.simulate(&spin);
    QCOMPARE(actualValues.size(), expected.size());
    for (int i=0; i<qMin(actualValues.size(), expected.size()); ++i) {
        QCOMPARE(actualValues.at(i), expected.at(i));
    }
}

void tst_QDoubleSpinBox::setMinMax_data()
{
    QTest::addColumn<double>("startValue");
    QTest::addColumn<double>("minimum");
    QTest::addColumn<double>("maximum");
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<double>("expected");
    QTest::addColumn<bool>("show");

    QTestEventList keys;
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Up);
    keys.addKeyClick(Qt::Key_Up);
    QTest::newRow("data0") << 1.0 << -DBL_MAX << 2.0 << keys << 2.0 << false;
    QTest::newRow("data1") << 1.0 << -DBL_MAX << 2.0 << keys << 2.0 << true;
    QTest::newRow("data2") << -20000.0 << -15000.0 << -13000.0 << keys << -14995.0 << false;
    QTest::newRow("data3") << -20000.0 << -15000.0 << -13000.0 << keys << -14995.0 << true;
    QTest::newRow("data4") << 20.0 << -101.2 << -102.0 << QTestEventList() << -102.0 << false;
    QTest::newRow("data5") << 20.0 << -101.2 << -102.0 << QTestEventList() << -102.0 << true;
}

void tst_QDoubleSpinBox::setMinMax()
{
    QFETCH(double, startValue);
    QFETCH(double, minimum);
    QFETCH(double, maximum);
    QFETCH(QTestEventList, keys);
    QFETCH(double, expected);
    QFETCH(bool, show);

    {
        QDoubleSpinBox spin(0);
        spin.setMinimum(minimum);
        spin.setMaximum(maximum);
        spin.setValue(startValue);

        if (show)
            spin.show();
        keys.simulate(&spin);
        QCOMPARE(spin.value(), expected);
    }

    {
        QDoubleSpinBox spin(0);
        spin.setMaximum(maximum);
        spin.setMinimum(minimum);
        spin.setValue(startValue);

        if (show)
            spin.show();
        keys.simulate(&spin);
    }

    {
        QDoubleSpinBox spin(0);
        spin.setRange(minimum, maximum);
        spin.setValue(startValue);

        if (show)
            spin.show();
        keys.simulate(&spin);
    }


}

void tst_QDoubleSpinBox::setDecimals_data()
{
    QTest::addColumn<int>("decimals");
    QTest::addColumn<int>("expectedDecimals");
    QTest::addColumn<double>("startValue");
    QTest::addColumn<QString>("expected");

    QTest::newRow("data0") << 4 << 4 << 1.0 << "1.0000";
    QTest::newRow("data1") << 1 << 1 << 1.243443 << "1.2";
    QTest::newRow("data2") << 6 << 6 << 1.29 << "1.290000";
    QTest::newRow("data3") << 8 << 8 << 9.1234567809 << "9.12345678";
    QTest::newRow("data4") << 13 << 13 << 0.12345678901237 << "0.1234567890124";
    QTest::newRow("data5") << 13 << 13 <<  -0.12345678901237 << "-0.1234567890124";
    QTest::newRow("data6") << 13 << 13 << -0.12345678901237 << "-0.1234567890124";
    QTest::newRow("data7") << -1 << 0 << 0.1 << "0";
    QTest::newRow("data8") << 120 << 120 << -0.12345678901237 << "-0.123456789012370005131913330842508003115653991699218750000000000000000000000000000000000000000000000000000000000000000000";

}

void tst_QDoubleSpinBox::setDecimals()
{
    QFETCH(int, decimals);
    QFETCH(int, expectedDecimals);
    QFETCH(double, startValue);
    QFETCH(QString, expected);

    QDoubleSpinBox spin(0);
    spin.setRange(-DBL_MAX, DBL_MAX);
    spin.setDecimals(decimals);
    spin.setValue(startValue);
    QCOMPARE(spin.decimals(), expectedDecimals);
    if (sizeof(qreal) == sizeof(float))
        QCOMPARE(spin.text().left(17), expected.left(17));
    else
        QCOMPARE(spin.text(), expected);

    if (spin.decimals() > 0) {
#ifdef Q_OS_MAC
        QTest::keyClick(&spin, Qt::Key_Right, Qt::ControlModifier);
#else
        QTest::keyClick(&spin, Qt::Key_End);
#endif
        QTest::keyClick(&spin, Qt::Key_1); // just make sure we can't input more decimals than what is specified
        QTest::keyClick(&spin, Qt::Key_1);
        QTest::keyClick(&spin, Qt::Key_1);
        QTest::keyClick(&spin, Qt::Key_1);
        QTest::keyClick(&spin, Qt::Key_1);
        if (sizeof(qreal) == sizeof(float))
            QCOMPARE(spin.text().left(17), expected.left(17));
        else
            QCOMPARE(spin.text(), expected);
    }
}

static QString stateName(int state)
{
    switch (state) {
    case QValidator::Acceptable: return QString("Acceptable");
    case QValidator::Intermediate: return QString("Intermediate");
    case QValidator::Invalid: return QString("Invalid");
    default: break;
    }
    qWarning("%s %d: this should never happen", __FILE__, __LINE__);
    return QString();
}

void tst_QDoubleSpinBox::valueFromTextAndValidate_data()
{
    const int Intermediate = QValidator::Intermediate;
    const int Invalid = QValidator::Invalid;
    const int Acceptable = QValidator::Acceptable;

    QTest::addColumn<QString>("txt");
    QTest::addColumn<int>("state");
    QTest::addColumn<double>("mini");
    QTest::addColumn<double>("maxi");
    QTest::addColumn<int>("language");
    QTest::addColumn<QString>("expectedText"); // if empty we don't check

    QTest::newRow("data0") << QString("2.2") << Intermediate << 3.0 << 5.0 << (int)QLocale::C << QString();
    QTest::newRow("data1") << QString() << Intermediate << 0.0 << 100.0 << (int)QLocale::C << QString();
    QTest::newRow("data2") << QString("asd") << Invalid << 0.0 << 100.0 << (int)QLocale::C << QString();
    QTest::newRow("data3") << QString("2.2") << Acceptable << 0.0 << 100.0 << (int)QLocale::C << QString();
    QTest::newRow("data4") << QString(" ") << Intermediate << 0.0 << 100.0 << (int)QLocale::Norwegian << QString();
    QTest::newRow("data5") << QString(" ") << Intermediate << 0.0 << 100.0 << (int)QLocale::C << QString();
    QTest::newRow("data6") << QString(",") << Intermediate << 0.0 << 100.0 << (int)QLocale::Norwegian << QString();
    QTest::newRow("data7") << QString(",") << Invalid << 0.0 << 100.0 << (int)QLocale::C << QString();
    QTest::newRow("data8") << QString("1 ") << Acceptable << 0.0 << 1000.0 << (int)QLocale::Norwegian << QString("1");
    QTest::newRow("data9") << QString("1 ") << Acceptable << 0.0 << 100.0 << (int)QLocale::C << QString("1");
    QTest::newRow("data10") << QString(" 1") << Acceptable << 0.0 << 100.0 << (int)QLocale::Norwegian << QString("1");
    QTest::newRow("data11") << QString(" 1") << Acceptable << 0.0 << 100.0 << (int)QLocale::C << QString("1");
    QTest::newRow("data12") << QString("1,") << Acceptable << 0.0 << 100.0 << (int)QLocale::Norwegian << QString();
    QTest::newRow("data13") << QString("1,") << Acceptable << 0.0 << 1000.0 << (int)QLocale::C << QString();
    QTest::newRow("data14") << QString("1, ") << Acceptable << 0.0 << 100.0 << (int)QLocale::Norwegian << QString("1,");
    QTest::newRow("data15") << QString("1, ") << Invalid << 0.0 << 100.0 << (int)QLocale::C << QString();
    QTest::newRow("data16") << QString("2") << Intermediate << 100.0 << 102.0 << (int)QLocale::C << QString();
    QTest::newRow("data17") << QString("22.0") << Intermediate << 100.0 << 102.0 << (int)QLocale::C << QString();
    QTest::newRow("data18") << QString("12.0") << Intermediate << 100.0 << 102.0 << (int)QLocale::C << QString();
    QTest::newRow("data19") << QString("12.2") << Intermediate << 100. << 102.0 << (int)QLocale::C << QString();
    QTest::newRow("data20") << QString("21.") << Intermediate << 100.0 << 102.0 << (int)QLocale::C << QString();
    QTest::newRow("data21") << QString("-21.") << Intermediate << -102.0 << -100.0 << (int)QLocale::C << QString();
    QTest::newRow("data22") << QString("-12.") << Intermediate << -102.0 << -100.0 << (int)QLocale::C << QString();
    QTest::newRow("data23") << QString("-11.11") << Intermediate << -102.0 << -101.2 << (int)QLocale::C << QString();
    QTest::newRow("data24") << QString("-11.4") << Intermediate << -102.0 << -101.3 << (int)QLocale::C << QString();
    QTest::newRow("data25") << QString("11.400") << Invalid << 0.0 << 100.0 << (int)QLocale::C << QString();
    QTest::newRow("data26") << QString(".4") << Intermediate << 0.45 << 0.5 << (int)QLocale::C << QString();
    QTest::newRow("data27") << QString("+.4") << Intermediate << 0.45 << 0.5 << (int)QLocale::C << QString();
    QTest::newRow("data28") << QString("-.4") << Intermediate << -0.5 << -0.45 << (int)QLocale::C << QString();
    QTest::newRow("data29") << QString(".4") << Intermediate << 1.0 << 2.4 << (int)QLocale::C << QString();
    QTest::newRow("data30") << QString("-.4") << Intermediate << -2.3 << -1.9 << (int)QLocale::C << QString();
    QTest::newRow("data31") << QString("-42") << Invalid << -2.43 << -1.0 << (int)QLocale::C << QString();
    QTest::newRow("data32") << QString("-4") << Invalid << -1.4 << -1.0 << (int)QLocale::C << QString();
    QTest::newRow("data33") << QString("-42") << Invalid << -1.4 << -1.0 << (int)QLocale::C << QString();
    QTest::newRow("data34") << QString("1000000000000") << Invalid << -140.0 << -120.2 << (int)QLocale::C << QString();
    QTest::newRow("data35") << QString("+.12") << Invalid << -5.0 << -3.2 << (int)QLocale::C << QString();
    QTest::newRow("data36") << QString("-.12") << Invalid << 5.0 << 33.2 << (int)QLocale::C << QString();
    QTest::newRow("data37") << QString("12.2") << Intermediate << 100. << 103.0 << (int)QLocale::C << QString();
    QTest::newRow("data38") << QString("12.2") << Intermediate << 100. << 102.3 << (int)QLocale::C << QString();
    QTest::newRow("data39") << QString("-12.") << Acceptable << -102.0 << 102.0 << (int)QLocale::C << QString();
    QTest::newRow("data40") << QString("12.") << Invalid << -102.0 << 11.0 << (int)QLocale::C << QString();
    QTest::newRow("data41") << QString("103.") << Invalid << -102.0 << 11.0 << (int)QLocale::C << QString();
    QTest::newRow("data42") << QString("122") << Invalid << 10.0 << 12.2 << (int)QLocale::C << QString();
    QTest::newRow("data43") << QString("-2.2") << Intermediate << -12.2 << -3.2 << (int)QLocale::C << QString();
    QTest::newRow("data44") << QString("-2.20") << Intermediate << -12.1 << -3.2 << (int)QLocale::C << QString();
    QTest::newRow("data45") << QString("200,2") << Invalid << 0.0 << 1000.0 << (int)QLocale::C << QString();
    QTest::newRow("data46") << QString("200,2") << Acceptable << 0.0 << 1000.0 << (int)QLocale::German << QString();
    QTest::newRow("data47") << QString("2.2") << Acceptable << 0.0 << 1000.0 << (int)QLocale::C << QString();
    QTest::newRow("data48") << QString("2.2") << Acceptable << 0.0 << 1000.0 << (int)QLocale::German << QString();
    QTest::newRow("data49") << QString("2.2,00") << Acceptable << 0.0 << 1000.0 << (int)QLocale::German << QString();
    QTest::newRow("data50") << QString("2.2") << Acceptable << 0.0 << 1000.0 << (int)QLocale::C << QString();
    QTest::newRow("data51") << QString("2.2,00") << Invalid << 0.0 << 1000.0 << (int)QLocale::C << QString();
    QTest::newRow("data52") << QString("2..2,00") << Invalid << 0.0 << 1000.0 << (int)QLocale::German << QString();
    QTest::newRow("data53") << QString("2.2") << Invalid << 0.0 << 1000.0 << (int)QLocale::Norwegian << QString();
    QTest::newRow("data54") << QString("  2.2") << Acceptable << 0.0 << 1000.0 << (int)QLocale::C << QString();
    QTest::newRow("data55") << QString("2.2  ") << Acceptable << 0.0 << 1000.0 << (int)QLocale::C << QString("2.2");
    QTest::newRow("data56") << QString("  2.2  ") << Acceptable << 0.0 << 1000.0 << (int)QLocale::C << QString("2.2");
    QTest::newRow("data57") << QString("2 2") << Invalid << 0.0 << 1000.0 << (int)QLocale::C << QString();
}

void tst_QDoubleSpinBox::valueFromTextAndValidate()
{
    QFETCH(QString, txt);
    QFETCH(int, state);
    QFETCH(double, mini);
    QFETCH(double, maxi);
    QFETCH(int, language);
    QFETCH(QString, expectedText);
    QLocale::setDefault(QLocale((QLocale::Language)language));

    DoubleSpinBox sb(0);
    sb.show();
    sb.setRange(mini, maxi);

    int unused = 0;
    QCOMPARE(stateName(sb.validate(txt, unused)), stateName(state));
    if (!expectedText.isEmpty())
        QCOMPARE(txt, expectedText);
}

void tst_QDoubleSpinBox::setReadOnly()
{
    QDoubleSpinBox spin(0);
    spin.setValue(0.2);
    spin.show();
    QVERIFY(QTest::qWaitForWindowActive(&spin));
    QCOMPARE(spin.value(), 0.2);
    QTest::keyClick(&spin, Qt::Key_Up);
    QCOMPARE(spin.value(), 1.2);
    spin.setReadOnly(true);
    QTest::keyClick(&spin, Qt::Key_Up);
    QCOMPARE(spin.value(), 1.2);
    spin.stepBy(1);
    QCOMPARE(spin.value(), 2.2);
    spin.setReadOnly(false);
    QTest::keyClick(&spin, Qt::Key_Up);
    QCOMPARE(spin.value(), 3.2);
}

void tst_QDoubleSpinBox::editingFinished()
{
    QVBoxLayout *layout = new QVBoxLayout(testFocusWidget);
    QDoubleSpinBox *box = new QDoubleSpinBox(testFocusWidget);
    layout->addWidget(box);
    QDoubleSpinBox *box2 = new QDoubleSpinBox(testFocusWidget);
    layout->addWidget(box2);

    testFocusWidget->show();
    testFocusWidget->activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(testFocusWidget));
    box->setFocus();
    QTRY_VERIFY(box->hasFocus());

    QSignalSpy editingFinishedSpy1(box, SIGNAL(editingFinished()));
    QSignalSpy editingFinishedSpy2(box2, SIGNAL(editingFinished()));

    QTest::keyClick(box, Qt::Key_Up);
    QTest::keyClick(box, Qt::Key_Up);


    QCOMPARE(editingFinishedSpy1.count(), 0);
    QCOMPARE(editingFinishedSpy2.count(), 0);

    QTest::keyClick(box2, Qt::Key_Up);
    QTest::keyClick(box2, Qt::Key_Up);
    box2->setFocus();
    QCOMPARE(editingFinishedSpy1.count(), 1);
    box->setFocus();
    QCOMPARE(editingFinishedSpy1.count(), 1);
    QCOMPARE(editingFinishedSpy2.count(), 1);
    QTest::keyClick(box, Qt::Key_Up);
    QCOMPARE(editingFinishedSpy1.count(), 1);
    QCOMPARE(editingFinishedSpy2.count(), 1);
    QTest::keyClick(box, Qt::Key_Enter);
    QCOMPARE(editingFinishedSpy1.count(), 2);
    QCOMPARE(editingFinishedSpy2.count(), 1);
    QTest::keyClick(box, Qt::Key_Return);
    QCOMPARE(editingFinishedSpy1.count(), 3);
    QCOMPARE(editingFinishedSpy2.count(), 1);
    box2->setFocus();
    QCOMPARE(editingFinishedSpy1.count(), 4);
    QCOMPARE(editingFinishedSpy2.count(), 1);
    QTest::keyClick(box2, Qt::Key_Enter);
    QCOMPARE(editingFinishedSpy1.count(), 4);
    QCOMPARE(editingFinishedSpy2.count(), 2);
    QTest::keyClick(box2, Qt::Key_Return);
    QCOMPARE(editingFinishedSpy1.count(), 4);
    QCOMPARE(editingFinishedSpy2.count(), 3);
    testFocusWidget->hide();
    QCOMPARE(editingFinishedSpy1.count(), 4);
    QCOMPARE(editingFinishedSpy2.count(), 4);

    // On some platforms this is our root window
    // we need to show it again otherwise subsequent
    // tests will fail
    testFocusWidget->show();
}

void tst_QDoubleSpinBox::removeAll()
{
    DoubleSpinBox spin(0);
    spin.setPrefix("foo");
    spin.setSuffix("bar");
    spin.setValue(0.2);
    spin.setDecimals(1);
    spin.show();
#ifdef Q_OS_MAC
    QTest::keyClick(&spin, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(&spin, Qt::Key_Home);
#endif

#ifdef Q_OS_MAC
    QTest::keyClick(&spin, Qt::Key_Right, Qt::ControlModifier|Qt::ShiftModifier);
#else
    QTest::keyClick(&spin, Qt::Key_End, Qt::ShiftModifier);
#endif

    QCOMPARE(spin.lineEdit()->selectedText(), QString("foo0.2bar"));
    QTest::keyClick(&spin, Qt::Key_1);
    QCOMPARE(spin.text(), QString("foo1bar"));
}

void tst_QDoubleSpinBox::task54433()
{
    DoubleSpinBox priceSpinBox;
    QCOMPARE(priceSpinBox.decimals(), 2);
    priceSpinBox.show();
    priceSpinBox.setRange(0.0, 999.99);
    priceSpinBox.setDecimals(2);
    priceSpinBox.setValue(999.99);
    QCOMPARE(priceSpinBox.text(), QString("999.99"));
    QCOMPARE(priceSpinBox.value(), 999.99);
    QCOMPARE(priceSpinBox.maximum(), 999.99);
    priceSpinBox.setDecimals(1);
    QCOMPARE(priceSpinBox.value(), 1000.0);
    QCOMPARE(priceSpinBox.maximum(), 1000.0);
    QCOMPARE(priceSpinBox.text(), QString("1000.0"));

    priceSpinBox.setDecimals(2);
    priceSpinBox.setRange(-999.99, 0.0);
    priceSpinBox.setValue(-999.99);
    QCOMPARE(priceSpinBox.text(), QString("-999.99"));
    QCOMPARE(priceSpinBox.value(), -999.99);
    QCOMPARE(priceSpinBox.minimum(), -999.99);
    priceSpinBox.setDecimals(1);
    QCOMPARE(priceSpinBox.value(), -1000.0);
    QCOMPARE(priceSpinBox.minimum(), -1000.0);
    QCOMPARE(priceSpinBox.text(), QString("-1000.0"));
}



void tst_QDoubleSpinBox::germanTest()
{
    QLocale::setDefault(QLocale(QLocale::German));
    DoubleSpinBox spin;
    spin.show();
    spin.setValue(2.12);
#ifdef Q_OS_MAC
    QTest::keyClick(&spin, Qt::Key_Right, Qt::ControlModifier);
#else
    QTest::keyClick(&spin, Qt::Key_End);
#endif
    QTest::keyClick(&spin, Qt::Key_Backspace);
    QCOMPARE(spin.text(), QString("2,1"));
    QTest::keyClick(&spin, Qt::Key_Enter);
    QCOMPARE(spin.text(), QString("2,10"));
}

void tst_QDoubleSpinBox::doubleDot()
{
    DoubleSpinBox spin;
    spin.show();
    spin.setValue(2.12);
    QTest::keyClick(&spin, Qt::Key_Backspace);
    QCOMPARE(spin.text(), QString("2.12"));
#ifdef Q_OS_MAC
    QTest::keyClick(&spin, Qt::Key_Left, Qt::ControlModifier);
#else
    QTest::keyClick(&spin, Qt::Key_Home);
#endif
    QTest::keyClick(&spin, Qt::Key_Right, Qt::ShiftModifier);
    QCOMPARE(spin.lineEdit()->selectedText(), QString("2"));
    QTest::keyClick(&spin, Qt::Key_1);
    QCOMPARE(spin.text(), QString("1.12"));
    QCOMPARE(spin.lineEdit()->cursorPosition(), 1);
    QTest::keyClick(&spin, Qt::Key_Period);
    QCOMPARE(spin.text(), QString("1.12"));
    QCOMPARE(spin.lineEdit()->cursorPosition(), 2);
}

void tst_QDoubleSpinBox::undoRedo()
{
    //test undo/redo feature (in conjunction with the "undoRedoEnabled" property)
    DoubleSpinBox spin(0);
    spin.show();

    //the undo/redo is disabled by default

    QCOMPARE(spin.value(), 0.0); //this is the default value
    QVERIFY(!spin.lineEdit()->isUndoAvailable());
    QVERIFY(!spin.lineEdit()->isRedoAvailable());

    spin.lineEdit()->selectAll(); //ensures everything is selected and will be cleared by typing a key
    QTest::keyClick(&spin, Qt::Key_1); //we put 1 into the spinbox
    QCOMPARE(spin.value(), 1.0);
    QVERIFY(spin.lineEdit()->isUndoAvailable());

    //testing CTRL+Z (undo)
    int val = QKeySequence(QKeySequence::Undo)[0];
    if (val != 0) {
        Qt::KeyboardModifiers mods = (Qt::KeyboardModifiers)(val & Qt::KeyboardModifierMask);
        QTest::keyClick(&spin, val & ~mods, mods);
        QCOMPARE(spin.value(), 0.0);
        QVERIFY(!spin.lineEdit()->isUndoAvailable());
        QVERIFY(spin.lineEdit()->isRedoAvailable());
    } else {
        QWARN("Undo not tested because no key sequence associated to QKeySequence::Redo");
    }


    //testing CTRL+Y (redo)
    val = QKeySequence(QKeySequence::Redo)[0];
    if (val != 0) {
        Qt::KeyboardModifiers mods = (Qt::KeyboardModifiers)(val & Qt::KeyboardModifierMask);
        QTest::keyClick(&spin, val & ~mods, mods);
        QCOMPARE(spin.value(), 1.0);
        QVERIFY(!spin.lineEdit()->isRedoAvailable());
        QVERIFY(spin.lineEdit()->isUndoAvailable());
    } else {
        QWARN("Redo not tested because no key sequence associated to QKeySequence::Redo");
    }


    spin.setValue(55.0);
    QVERIFY(!spin.lineEdit()->isUndoAvailable());
    QVERIFY(!spin.lineEdit()->isRedoAvailable());
}

struct task199226_DoubleSpinBox : public QDoubleSpinBox
{
    task199226_DoubleSpinBox(QWidget *parent = 0) : QDoubleSpinBox(parent) {}
    QLineEdit *lineEdit() { return QAbstractSpinBox::lineEdit(); }
};

void tst_QDoubleSpinBox::task199226_stateAfterEnter()
{
    task199226_DoubleSpinBox spin;
    spin.setMinimum(0);
    spin.setMaximum(10);
    spin.setDecimals(0);
    spin.show();
    QTest::mouseDClick(spin.lineEdit(), Qt::LeftButton);
    QTest::keyClick(spin.lineEdit(), Qt::Key_3);
    QVERIFY(spin.lineEdit()->isModified());
    QVERIFY(spin.lineEdit()->isUndoAvailable());
    QTest::keyClick(spin.lineEdit(), Qt::Key_Enter);
    QVERIFY(!spin.lineEdit()->isModified());
    QVERIFY(!spin.lineEdit()->isUndoAvailable());
}

class task224497_fltMax_DoubleSpinBox : public QDoubleSpinBox
{
public:
    QLineEdit * lineEdit () const { return QDoubleSpinBox::lineEdit(); }
};

void tst_QDoubleSpinBox::task224497_fltMax()
{
    task224497_fltMax_DoubleSpinBox *dspin = new task224497_fltMax_DoubleSpinBox;
    dspin->setMinimum(3);
    dspin->setMaximum(FLT_MAX);
    dspin->show();
    QVERIFY(QTest::qWaitForWindowActive(dspin));
    dspin->lineEdit()->selectAll();
    QTest::keyClick(dspin->lineEdit(), Qt::Key_Delete);
    QTest::keyClick(dspin->lineEdit(), Qt::Key_1);
    QCOMPARE(dspin->cleanText(), QLatin1String("1"));
}

void tst_QDoubleSpinBox::task221221()
{
    QDoubleSpinBox spin;
    QTest::keyClick(&spin, Qt::Key_1);
    spin.show();
    QVERIFY(QTest::qWaitForWindowExposed(&spin));
    QCOMPARE(spin.text(), QLatin1String("1"));
}

void tst_QDoubleSpinBox::task255471_decimalsValidation()
{
    // QDoubleSpinBox shouldn't crash with large numbers of decimals. Even if
    // the results are useless ;-)
    for (int i = 0; i < 32; ++i)
    {
        QDoubleSpinBox spinBox;
        spinBox.setDecimals(i);
        spinBox.setMinimum(0.3);
        spinBox.setMaximum(12);

        spinBox.show();
        QVERIFY(QTest::qWaitForWindowExposed(&spinBox));
        spinBox.setFocus();
        QTRY_VERIFY(spinBox.hasFocus());

        QTest::keyPress(&spinBox, Qt::Key_Right);
        QTest::keyPress(&spinBox, Qt::Key_Right);
        QTest::keyPress(&spinBox, Qt::Key_Delete);

        // Don't crash!
        QTest::keyPress(&spinBox, Qt::Key_2);
    }
}

void tst_QDoubleSpinBox::taskQTBUG_5008_textFromValueAndValidate()
{
    class DecoratedSpinBox : public QDoubleSpinBox
    {
    public:
        DecoratedSpinBox()
        {
            setLocale(QLocale::French);
            setMaximum(100000000);
            setValue(1000);
        }

        QLineEdit *lineEdit() const
        {
            return QDoubleSpinBox::lineEdit();
        }

        //we use the French delimiters here
        QString textFromValue (double value) const
        {
            return locale().toString(value);
        }
    } spinbox;
    spinbox.show();
    spinbox.activateWindow();
    spinbox.setFocus();
    QApplication::setActiveWindow(&spinbox);
    QVERIFY(QTest::qWaitForWindowActive(&spinbox));
    QCOMPARE(static_cast<QWidget *>(&spinbox), QApplication::activeWindow());
    QTRY_VERIFY(spinbox.hasFocus());
    QCOMPARE(spinbox.text(), spinbox.locale().toString(spinbox.value()));
    spinbox.lineEdit()->setCursorPosition(2); //just after the first thousand separator
    QTest::keyClick(static_cast<QWidget *>(0), Qt::Key_0); // let's insert a 0
    QCOMPARE(spinbox.value(), 10000.);
    spinbox.clearFocus(); //make sure the value is correctly formatted
    QCOMPARE(spinbox.text(), spinbox.locale().toString(spinbox.value()));
}

void tst_QDoubleSpinBox::taskQTBUG_6670_selectAllWithPrefix()
{
    DoubleSpinBox spin;
    spin.setPrefix("$ ");
    spin.lineEdit()->selectAll();
    QTest::keyClick(spin.lineEdit(), Qt::Key_1);
    QCOMPARE(spin.value(), 1.);
    QTest::keyClick(spin.lineEdit(), Qt::Key_2);
    QCOMPARE(spin.value(), 12.);
}

void tst_QDoubleSpinBox::taskQTBUG_6496_fiddlingWithPrecision()
{
    QDoubleSpinBox dsb;
    dsb.setRange(0, 0.991);
    dsb.setDecimals(1);
    QCOMPARE(dsb.maximum(), 1.0);
    dsb.setDecimals(2);
    QCOMPARE(dsb.maximum(), 0.99);
    dsb.setDecimals(3);
    QCOMPARE(dsb.maximum(), 0.991);
}

void tst_QDoubleSpinBox::setGroupSeparatorShown_data()
{
    QTest::addColumn<QLocale::Language>("lang");
    QTest::addColumn<QLocale::Country>("country");

    QTest::newRow("data0") << QLocale::English << QLocale::UnitedStates;
    QTest::newRow("data1") << QLocale::Swedish << QLocale::Sweden;
    QTest::newRow("data2") << QLocale::German << QLocale::Germany;
    QTest::newRow("data3") << QLocale::Georgian << QLocale::Georgia;
    QTest::newRow("data3") << QLocale::Macedonian << QLocale::Macedonia;
}

void tst_QDoubleSpinBox::setGroupSeparatorShown()
{
    QFETCH(QLocale::Language, lang);
    QFETCH(QLocale::Country, country);

    QLocale loc(lang, country);
    QLocale::setDefault(loc);
    DoubleSpinBox spinBox;
    spinBox.setMaximum(99999999);
    spinBox.setValue(1300000.00);
    spinBox.setGroupSeparatorShown(true);
    QCOMPARE(spinBox.lineEdit()->text(), spinBox.locale().toString(1300000.00, 'f', 2));
    QCOMPARE(spinBox.isGroupSeparatorShown(), true);
    QCOMPARE(spinBox.textFromValue(23421),spinBox.locale().toString(23421.00, 'f', 2));

    spinBox.setGroupSeparatorShown(false);
    QCOMPARE(spinBox.lineEdit()->text(), spinBox.locale().toString(1300000.00, 'f', 2).remove(
                 spinBox.locale().groupSeparator()));
    QCOMPARE(spinBox.isGroupSeparatorShown(), false);

    spinBox.setMaximum(72000);
    spinBox.lineEdit()->setText(spinBox.locale().toString(32000.64, 'f', 2));
    QCOMPARE(spinBox.value()+1000, 33000.64);

    spinBox.lineEdit()->setText(spinBox.locale().toString(32000.44, 'f', 2));
    QCOMPARE(spinBox.value()+1000, 33000.44);
}

void tst_QDoubleSpinBox::adaptiveDecimalStep()
{
    DoubleSpinBox spinBox;
    spinBox.setRange(-100000, 100000);
    spinBox.setDecimals(4);
    spinBox.setStepType(DoubleSpinBox::StepType::AdaptiveDecimalStepType);

    // Positive values

    spinBox.setValue(0);

    // Go from 0 to 0.01
    for (double i = 0; i < 0.00999; i += 0.0001) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(1);
    }

    // Go from 0.01 to 0.1
    for (double i = 0.01; i < 0.0999; i += 0.001) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(1);
    }

    // Go from 0.1 to 1
    for (double i = 0.1; i < 0.999; i += 0.01) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(1);
    }

    // Go from 1 to 10
    for (double i = 1; i < 9.99; i += 0.1) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(1);
    }

    // Go from 10 to 100
    for (int i = 10; i < 100; i++) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(1);
    }

    // Go from 100 to 1000
    for (int i = 100; i < 1000; i += 10) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(1);
    }

    // Go from 1000 to 10000
    for (int i = 1000; i < 10000; i += 100) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(1);
    }

    // Test decreasing the values now

    // Go from 10000 down to 1000
    for (int i = 10000; i > 1000; i -= 100) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(-1);
    }

    // Go from 1000 down to 100
    for (int i = 1000; i > 100; i -= 10) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(-1);
    }

    // Negative values

    spinBox.setValue(0);

    // Go from 0 to -0.01
    for (double i = 0; i > -0.00999; i -= 0.0001) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(-1);
    }

    // Go from -0.01 to -0.1
    for (double i = -0.01; i > -0.0999; i -= 0.001) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(-1);
    }

    // Go from -0.1 to -1
    for (double i = -0.1; i > -0.999; i -= 0.01) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(-1);
    }

    // Go from -1 to -10
    for (double i = -1; i > -9.99; i -= 0.1) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(-1);
    }

    // Go from -10 to -100
    for (int i = -10; i > -100; i--) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(-1);
    }

    // Go from -100 to -1000
    for (int i = -100; i > -1000; i -= 10) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(-1);
    }

    // Go from 1000 to 10000
    for (int i = -1000; i > -10000; i -= 100) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(-1);
    }

    // Test increasing the values now

    // Go from -10000 up to -1000
    for (int i = -10000; i < -1000; i += 100) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(1);
    }

    // Go from -1000 up to -100
    for (int i = -1000; i < -100; i += 10) {
        QVERIFY(qFuzzyCompare(spinBox.value(), i));
        spinBox.stepBy(1);
    }
}

void tst_QDoubleSpinBox::wheelEvents_data()
{
#if QT_CONFIG(wheelevent)
    QTest::addColumn<QPoint>("angleDelta");
    QTest::addColumn<int>("qt4Delta");
    QTest::addColumn<int>("stepModifier");
    QTest::addColumn<Qt::KeyboardModifiers>("modifier");
    QTest::addColumn<Qt::MouseEventSource>("source");
    QTest::addColumn<double>("start");
    QTest::addColumn<DoubleList>("expectedValues");

    const auto fractions = {false, true};

    const auto directions = {true, false};

    const auto modifierList = {Qt::NoModifier,
                               Qt::ShiftModifier,
                               Qt::ControlModifier,
                               Qt::AltModifier,
                               Qt::MetaModifier};

    const auto validStepModifierList = {Qt::NoModifier,
                                        Qt::ControlModifier,
                                        Qt::ShiftModifier};

    const auto sources = {Qt::MouseEventNotSynthesized,
                          Qt::MouseEventSynthesizedBySystem,
                          Qt::MouseEventSynthesizedByQt,
                          Qt::MouseEventSynthesizedByApplication};

    const double startValue = 0.0;

    for (auto fraction : fractions) {
        for (auto up : directions) {

            const int units = (fraction ? 60 : 120) * (up ? 1 : -1);

            for (auto modifier : modifierList) {

                const Qt::KeyboardModifiers modifiers(modifier);

                const auto modifierName = modifierToName(modifier);
                if (modifierName.isEmpty())
                    continue;

                for (auto stepModifier : validStepModifierList) {

                    const auto stepModifierName = modifierToName(stepModifier);
                    if (stepModifierName.isEmpty())
                        continue;

                    const int steps = (modifier & stepModifier ? 10 : 1)
                            * (up ? 1 : -1);

                    for (auto source : sources) {

#ifdef Q_OS_MACOS
                        QPoint angleDelta;
                        if ((modifier & Qt::ShiftModifier) &&
                                source == Qt::MouseEventNotSynthesized) {
                            // On macOS the Shift modifier converts vertical
                            // mouse wheel events to horizontal.
                            angleDelta = { units, 0 };
                        } else {
                            // However, this is not the case for trackpad scroll
                            // events.
                            angleDelta = { 0, units };
                        }
#else
                        const QPoint angleDelta(0, units);
#endif

                        QLatin1String sourceName;
                        switch (source) {
                        case Qt::MouseEventNotSynthesized:
                            sourceName = QLatin1Literal("NotSynthesized");
                            break;
                        case Qt::MouseEventSynthesizedBySystem:
                            sourceName = QLatin1Literal("SynthesizedBySystem");
                            break;
                        case Qt::MouseEventSynthesizedByQt:
                            sourceName = QLatin1Literal("SynthesizedByQt");
                            break;
                        case Qt::MouseEventSynthesizedByApplication:
                            sourceName = QLatin1Literal("SynthesizedByApplication");
                            break;
                        default:
                            qFatal("Unexpected wheel event source");
                            continue;
                        }

                        DoubleList expectedValues;
                        if (fraction)
                            expectedValues << startValue;
                        expectedValues << startValue + steps;

                        QTest::addRow("%s%s%sWith%sKeyboardModifier%s",
                                      fraction ? "half" : "full",
                                      up ? "Up" : "Down",
                                      stepModifierName.latin1(),
                                      modifierName.latin1(),
                                      sourceName.latin1())
                                << angleDelta
                                << units
                                << static_cast<int>(stepModifier)
                                << modifiers
                                << source
                                << startValue
                                << expectedValues;
                    }
                }
            }
        }
    }
#else
    QSKIP("Built with --no-feature-wheelevent");
#endif
}

void tst_QDoubleSpinBox::wheelEvents()
{
#if QT_CONFIG(wheelevent)
    QFETCH(QPoint, angleDelta);
    QFETCH(int, qt4Delta);
    QFETCH(int, stepModifier);
    QFETCH(Qt::KeyboardModifiers, modifier);
    QFETCH(Qt::MouseEventSource, source);
    QFETCH(double, start);
    QFETCH(DoubleList, expectedValues);

    DoubleSpinBox spinBox;
    spinBox.setRange(-20, 20);
    spinBox.setValue(start);

    QScopedPointer<StepModifierStyle, QScopedPointerDeleteLater> style(
                new StepModifierStyle);
    style->stepModifier = static_cast<Qt::KeyboardModifier>(stepModifier);
    spinBox.setStyle(style.data());

    QWheelEvent event(QPointF(), QPointF(), QPoint(), angleDelta, qt4Delta,
                      Qt::Vertical, Qt::NoButton, modifier, Qt::NoScrollPhase,
                      source);
    for (int expected : expectedValues) {
        qApp->sendEvent(&spinBox, &event);
        QCOMPARE(spinBox.value(), expected);
    }
#else
    QSKIP("Built with --no-feature-wheelevent");
#endif
}

void tst_QDoubleSpinBox::stepModifierKeys_data()
{
    QTest::addColumn<double>("startValue");
    QTest::addColumn<int>("stepModifier");
    QTest::addColumn<QTestEventList>("keys");
    QTest::addColumn<double>("expectedValue");

    const auto keyList = {Qt::Key_Up, Qt::Key_Down};

    const auto modifierList = {Qt::NoModifier,
                               Qt::ShiftModifier,
                               Qt::ControlModifier,
                               Qt::AltModifier,
                               Qt::MetaModifier};

    const auto validStepModifierList = {Qt::NoModifier,
                                        Qt::ControlModifier,
                                        Qt::ShiftModifier};


    for (auto key : keyList) {

        const bool up = key == Qt::Key_Up;
        Q_ASSERT(up || key == Qt::Key_Down);

        const double startValue = up ? 0.0 : 10.0;

        for (auto modifier : modifierList) {

            QTestEventList keys;
            keys.addKeyClick(key, modifier);

            const auto modifierName = modifierToName(modifier);
            if (modifierName.isEmpty())
                continue;

            for (auto stepModifier : validStepModifierList) {

                const auto stepModifierName = modifierToName(stepModifier);
                if (stepModifierName.isEmpty())
                    continue;

                const int steps = (modifier & stepModifier ? 10 : 1)
                        * (up ? 1 : -1);

                const double expectedValue = startValue + steps;

                QTest::addRow("%s%sWith%sKeyboardModifier",
                              up ? "up" : "down",
                              stepModifierName.latin1(),
                              modifierName.latin1())
                        << startValue
                        << static_cast<int>(stepModifier)
                        << keys
                        << expectedValue;
            }
        }
    }
}

void tst_QDoubleSpinBox::stepModifierKeys()
{
    QFETCH(double, startValue);
    QFETCH(int, stepModifier);
    QFETCH(QTestEventList, keys);
    QFETCH(double, expectedValue);

    QDoubleSpinBox spin(0);
    spin.setValue(startValue);

    QScopedPointer<StepModifierStyle, QScopedPointerDeleteLater> style(
                new StepModifierStyle);
    style->stepModifier = static_cast<Qt::KeyboardModifier>(stepModifier);
    spin.setStyle(style.data());

    spin.show();
    QVERIFY(QTest::qWaitForWindowActive(&spin));

    QCOMPARE(spin.value(), startValue);
    keys.simulate(&spin);
    QCOMPARE(spin.value(), expectedValue);
}

void tst_QDoubleSpinBox::stepModifierButtons_data()
{
    QTest::addColumn<QStyle::SubControl>("subControl");
    QTest::addColumn<int>("stepModifier");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<double>("startValue");
    QTest::addColumn<double>("expectedValue");

    const auto subControls = {QStyle::SC_SpinBoxUp, QStyle::SC_SpinBoxDown};

    const auto modifierList = {Qt::NoModifier,
                                   Qt::ShiftModifier,
                                   Qt::ControlModifier,
                                   Qt::AltModifier,
                                   Qt::MetaModifier};

    const auto validStepModifierList = {Qt::NoModifier,
                                        Qt::ControlModifier,
                                        Qt::ShiftModifier};

    for (auto subControl : subControls) {

        const bool up = subControl == QStyle::SC_SpinBoxUp;
        Q_ASSERT(up || subControl == QStyle::SC_SpinBoxDown);

        const double startValue = up ? 0 : 10;

        for (auto modifier : modifierList) {

            const Qt::KeyboardModifiers modifiers(modifier);

            const auto modifierName = modifierToName(modifier);
            if (modifierName.isEmpty())
                continue;

            for (auto stepModifier : validStepModifierList) {

                const auto stepModifierName = modifierToName(stepModifier);
                if (stepModifierName.isEmpty())
                    continue;

                const int steps = (modifier & stepModifier ? 10 : 1)
                        * (up ? 1 : -1);

                const double expectedValue = startValue + steps;

                QTest::addRow("%s%sWith%sKeyboardModifier",
                              up ? "up" : "down",
                              stepModifierName.latin1(),
                              modifierName.latin1())
                        << subControl
                        << static_cast<int>(stepModifier)
                        << modifiers
                        << startValue
                        << expectedValue;
            }
        }
    }
}

void tst_QDoubleSpinBox::stepModifierButtons()
{
    QFETCH(QStyle::SubControl, subControl);
    QFETCH(int, stepModifier);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(double, startValue);
    QFETCH(double, expectedValue);

    DoubleSpinBox spin(0);
    spin.setRange(-20, 20);
    spin.setValue(startValue);

    QScopedPointer<StepModifierStyle, QScopedPointerDeleteLater> style(
                new StepModifierStyle);
    style->stepModifier = static_cast<Qt::KeyboardModifier>(stepModifier);
    spin.setStyle(style.data());

    spin.show();
    QVERIFY(QTest::qWaitForWindowActive(&spin));

    QStyleOptionSpinBox spinBoxStyleOption;
    spin.initStyleOption(&spinBoxStyleOption);

    const QRect buttonRect = spin.style()->subControlRect(
                QStyle::CC_SpinBox, &spinBoxStyleOption, subControl, &spin);

    QCOMPARE(spin.value(), startValue);
    QTest::mouseClick(&spin, Qt::LeftButton, modifiers, buttonRect.center());
    QCOMPARE(spin.value(), expectedValue);
}

void tst_QDoubleSpinBox::stepModifierPressAndHold_data()
{
    QTest::addColumn<QStyle::SubControl>("subControl");
    QTest::addColumn<int>("stepModifier");
    QTest::addColumn<Qt::KeyboardModifiers>("modifiers");
    QTest::addColumn<int>("expectedStepModifier");

    const auto subControls = {QStyle::SC_SpinBoxUp, QStyle::SC_SpinBoxDown};

    const auto modifierList = {Qt::NoModifier,
                               Qt::ShiftModifier,
                               Qt::ControlModifier,
                               Qt::AltModifier,
                               Qt::MetaModifier};

    const auto validStepModifierList = {Qt::NoModifier,
                                        Qt::ControlModifier,
                                        Qt::ShiftModifier};

    for (auto subControl : subControls) {

        const bool up = subControl == QStyle::SC_SpinBoxUp;
        Q_ASSERT(up || subControl == QStyle::SC_SpinBoxDown);

        for (auto modifier : modifierList) {

            const Qt::KeyboardModifiers modifiers(modifier);

            const auto modifierName = modifierToName(modifier);
            if (modifierName.isEmpty())
                continue;

            for (auto stepModifier : validStepModifierList) {

                const auto stepModifierName = modifierToName(stepModifier);
                if (stepModifierName.isEmpty())
                    continue;

                const int steps = (modifier & stepModifier ? 10 : 1)
                        * (up ? 1 : -1);

                QTest::addRow("%s%sWith%sKeyboardModifier",
                              up ? "up" : "down",
                              stepModifierName.latin1(),
                              modifierName.latin1())
                        << subControl
                        << static_cast<int>(stepModifier)
                        << modifiers
                        << steps;
            }
        }
    }
}

void tst_QDoubleSpinBox::stepModifierPressAndHold()
{
    QFETCH(QStyle::SubControl, subControl);
    QFETCH(int, stepModifier);
    QFETCH(Qt::KeyboardModifiers, modifiers);
    QFETCH(int, expectedStepModifier);

    DoubleSpinBox spin(0);
    spin.setRange(-100.0, 100.0);
    spin.setValue(0.0);

    QScopedPointer<StepModifierStyle, QScopedPointerDeleteLater> stepModifierStyle(
                new StepModifierStyle(new PressAndHoldStyle));
    stepModifierStyle->stepModifier = static_cast<Qt::KeyboardModifier>(stepModifier);
    spin.setStyle(stepModifierStyle.data());

    QSignalSpy spy(&spin, QOverload<double>::of(&DoubleSpinBox::valueChanged));

    spin.show();
    QVERIFY(QTest::qWaitForWindowExposed(&spin));

    QStyleOptionSpinBox spinBoxStyleOption;
    spin.initStyleOption(&spinBoxStyleOption);

    const QRect buttonRect = spin.style()->subControlRect(
                QStyle::CC_SpinBox, &spinBoxStyleOption, subControl, &spin);

    QTest::mousePress(&spin, Qt::LeftButton, modifiers, buttonRect.center());
    QTRY_VERIFY(spy.length() >= 3);
    QTest::mouseRelease(&spin, Qt::LeftButton, modifiers, buttonRect.center());

    const auto value = spy.last().at(0);
    QVERIFY(value.type() == QVariant::Double);
    QCOMPARE(value.toDouble(), spy.length() * expectedStepModifier);
}

QTEST_MAIN(tst_QDoubleSpinBox)
#include "tst_qdoublespinbox.moc"
