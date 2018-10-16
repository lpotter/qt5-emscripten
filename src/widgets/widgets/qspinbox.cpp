/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qabstractspinbox_p.h>
#include <qspinbox.h>

#include <qlineedit.h>
#include <qlocale.h>
#include <qvalidator.h>
#include <qdebug.h>

#include <algorithm>
#include <cmath>
#include <float.h>

QT_BEGIN_NAMESPACE

//#define QSPINBOX_QSBDEBUG
#ifdef QSPINBOX_QSBDEBUG
#  define QSBDEBUG qDebug
#else
#  define QSBDEBUG if (false) qDebug
#endif

class QSpinBoxPrivate : public QAbstractSpinBoxPrivate
{
    Q_DECLARE_PUBLIC(QSpinBox)
public:
    QSpinBoxPrivate();
    void emitSignals(EmitPolicy ep, const QVariant &) override;

    virtual QVariant valueFromText(const QString &n) const override;
    virtual QString textFromValue(const QVariant &n) const override;
    QVariant validateAndInterpret(QString &input, int &pos,
                                  QValidator::State &state) const;

    inline void init() {
        Q_Q(QSpinBox);
        q->setInputMethodHints(Qt::ImhDigitsOnly);
        setLayoutItemMargins(QStyle::SE_SpinBoxLayoutItem);
    }

    int displayIntegerBase;

    QVariant calculateAdaptiveDecimalStep(int steps) const override;
};

class QDoubleSpinBoxPrivate : public QAbstractSpinBoxPrivate
{
    Q_DECLARE_PUBLIC(QDoubleSpinBox)
public:
    QDoubleSpinBoxPrivate();
    void emitSignals(EmitPolicy ep, const QVariant &) override;

    virtual QVariant valueFromText(const QString &n) const override;
    virtual QString textFromValue(const QVariant &n) const override;
    QVariant validateAndInterpret(QString &input, int &pos,
                                  QValidator::State &state) const;
    double round(double input) const;
    // variables
    int decimals;

    inline void init() {
        Q_Q(QDoubleSpinBox);
        q->setInputMethodHints(Qt::ImhFormattedNumbersOnly);
    }

    // When fiddling with the decimals property, we may lose precision in these properties.
    double actualMin;
    double actualMax;

    QVariant calculateAdaptiveDecimalStep(int steps) const override;
};


/*!
    \class QSpinBox
    \brief The QSpinBox class provides a spin box widget.

    \ingroup basicwidgets
    \inmodule QtWidgets

    \image windows-spinbox.png

    QSpinBox is designed to handle integers and discrete sets of
    values (e.g., month names); use QDoubleSpinBox for floating point
    values.

    QSpinBox allows the user to choose a value by clicking the up/down
    buttons or pressing up/down on the keyboard to increase/decrease
    the value currently displayed. The user can also type the value in
    manually. The spin box supports integer values but can be extended to
    use different strings with validate(), textFromValue() and valueFromText().

    Every time the value changes QSpinBox emits two valueChanged() signals,
    one providing an int and the other a QString. The QString overload
    provides the value with both prefix() and suffix().
    The current value can be fetched with value() and set with setValue().

    Clicking the up/down buttons or using the keyboard accelerator's
    up and down arrows will increase or decrease the current value in
    steps of size singleStep(). If you want to change this behaviour you
    can reimplement the virtual function stepBy(). The minimum and
    maximum value and the step size can be set using one of the
    constructors, and can be changed later with setMinimum(),
    setMaximum() and setSingleStep().

    Most spin boxes are directional, but QSpinBox can also operate as
    a circular spin box, i.e. if the range is 0-99 and the current
    value is 99, clicking "up" will give 0 if wrapping() is set to
    true. Use setWrapping() if you want circular behavior.

    The displayed value can be prepended and appended with arbitrary
    strings indicating, for example, currency or the unit of
    measurement. See setPrefix() and setSuffix(). The text in the spin
    box is retrieved with text() (which includes any prefix() and
    suffix()), or with cleanText() (which has no prefix(), no suffix()
    and no leading or trailing whitespace).

    It is often desirable to give the user a special (often default)
    choice in addition to the range of numeric values. See
    setSpecialValueText() for how to do this with QSpinBox.

    \section1 Subclassing QSpinBox

    If using prefix(), suffix(), and specialValueText() don't provide
    enough control, you subclass QSpinBox and reimplement
    valueFromText() and textFromValue(). For example, here's the code
    for a custom spin box that allows the user to enter icon sizes
    (e.g., "32 x 32"):

    \snippet widgets/icons/iconsizespinbox.cpp 1
    \codeline
    \snippet widgets/icons/iconsizespinbox.cpp 2

    See the \l{widgets/icons}{Icons} example for the full source
    code.

    \sa QDoubleSpinBox, QDateTimeEdit, QSlider, {Spin Boxes Example}
*/

/*!
    \fn void QSpinBox::valueChanged(int i)

    This signal is emitted whenever the spin box's value is changed.
    The new value's integer value is passed in \a i.
*/

/*!
    \fn void QSpinBox::valueChanged(const QString &text)

    \overload

    The new value is passed in \a text with prefix() and suffix().
*/

/*!
    Constructs a spin box with 0 as minimum value and 99 as maximum value, a
    step value of 1. The value is initially set to 0. It is parented to \a
    parent.

    \sa setMinimum(), setMaximum(), setSingleStep()
*/

QSpinBox::QSpinBox(QWidget *parent)
    : QAbstractSpinBox(*new QSpinBoxPrivate, parent)
{
    Q_D(QSpinBox);
    d->init();
}

/*!
    Destructor.
*/
QSpinBox::~QSpinBox() {}

/*!
    \property QSpinBox::value
    \brief the value of the spin box

    setValue() will emit valueChanged() if the new value is different
    from the old one. The value property has a second notifier
    signal which includes the spin box's prefix and suffix.
*/

int QSpinBox::value() const
{
    Q_D(const QSpinBox);
    return d->value.toInt();
}

void QSpinBox::setValue(int value)
{
    Q_D(QSpinBox);
    d->setValue(QVariant(value), EmitIfChanged);
}

/*!
    \property QSpinBox::prefix
    \brief the spin box's prefix

    The prefix is prepended to the start of the displayed value.
    Typical use is to display a unit of measurement or a currency
    symbol. For example:

    \snippet code/src_gui_widgets_qspinbox.cpp 0

    To turn off the prefix display, set this property to an empty
    string. The default is no prefix. The prefix is not displayed when
    value() == minimum() and specialValueText() is set.

    If no prefix is set, prefix() returns an empty string.

    \sa suffix(), setSuffix(), specialValueText(), setSpecialValueText()
*/

QString QSpinBox::prefix() const
{
    Q_D(const QSpinBox);
    return d->prefix;
}

void QSpinBox::setPrefix(const QString &prefix)
{
    Q_D(QSpinBox);

    d->prefix = prefix;
    d->updateEdit();

    d->cachedSizeHint = QSize();
    d->cachedMinimumSizeHint = QSize(); // minimumSizeHint cares about the prefix
    updateGeometry();
}

/*!
    \property QSpinBox::suffix
    \brief the suffix of the spin box

    The suffix is appended to the end of the displayed value. Typical
    use is to display a unit of measurement or a currency symbol. For
    example:

    \snippet code/src_gui_widgets_qspinbox.cpp 1

    To turn off the suffix display, set this property to an empty
    string. The default is no suffix. The suffix is not displayed for
    the minimum() if specialValueText() is set.

    If no suffix is set, suffix() returns an empty string.

    \sa prefix(), setPrefix(), specialValueText(), setSpecialValueText()
*/

QString QSpinBox::suffix() const
{
    Q_D(const QSpinBox);

    return d->suffix;
}

void QSpinBox::setSuffix(const QString &suffix)
{
    Q_D(QSpinBox);

    d->suffix = suffix;
    d->updateEdit();

    d->cachedSizeHint = QSize();
    updateGeometry();
}

/*!
    \property QSpinBox::cleanText

    \brief the text of the spin box excluding any prefix, suffix,
    or leading or trailing whitespace.

    \sa text, QSpinBox::prefix, QSpinBox::suffix
*/

QString QSpinBox::cleanText() const
{
    Q_D(const QSpinBox);

    return d->stripped(d->edit->displayText());
}


/*!
    \property QSpinBox::singleStep
    \brief the step value

    When the user uses the arrows to change the spin box's value the
    value will be incremented/decremented by the amount of the
    singleStep. The default value is 1. Setting a singleStep value of
    less than 0 does nothing.
*/

int QSpinBox::singleStep() const
{
    Q_D(const QSpinBox);

    return d->singleStep.toInt();
}

void QSpinBox::setSingleStep(int value)
{
    Q_D(QSpinBox);
    if (value >= 0) {
        d->singleStep = QVariant(value);
        d->updateEdit();
    }
}

/*!
    \property QSpinBox::minimum

    \brief the minimum value of the spin box

    When setting this property the \l maximum is adjusted
    if necessary to ensure that the range remains valid.

    The default minimum value is 0.

    \sa setRange(), specialValueText
*/

int QSpinBox::minimum() const
{
    Q_D(const QSpinBox);

    return d->minimum.toInt();
}

void QSpinBox::setMinimum(int minimum)
{
    Q_D(QSpinBox);
    const QVariant m(minimum);
    d->setRange(m, (d->variantCompare(d->maximum, m) > 0 ? d->maximum : m));
}

/*!
    \property QSpinBox::maximum

    \brief the maximum value of the spin box

    When setting this property the minimum is adjusted
    if necessary, to ensure that the range remains valid.

    The default maximum value is 99.

    \sa setRange(), specialValueText

*/

int QSpinBox::maximum() const
{
    Q_D(const QSpinBox);

    return d->maximum.toInt();
}

void QSpinBox::setMaximum(int maximum)
{
    Q_D(QSpinBox);
    const QVariant m(maximum);
    d->setRange((d->variantCompare(d->minimum, m) < 0 ? d->minimum : m), m);
}

/*!
    Convenience function to set the \a minimum, and \a maximum values
    with a single function call.

    \snippet code/src_gui_widgets_qspinbox.cpp 2
    is equivalent to:
    \snippet code/src_gui_widgets_qspinbox.cpp 3

    \sa minimum, maximum
*/

void QSpinBox::setRange(int minimum, int maximum)
{
    Q_D(QSpinBox);
    d->setRange(QVariant(minimum), QVariant(maximum));
}

/*!
    Sets the step type for the spin box to \a stepType, which is single
    step or adaptive decimal step.

    Adaptive decimal step means that the step size will continuously be
    adjusted to one power of ten below the current \l value. So when
    the value is 1100, the step is set to 100, so stepping up once
    increases it to 1200. For 1200 stepping up takes it to 1300. For
    negative values, stepping down from -1100 goes to -1200.

    Step direction is taken into account to handle edges cases, so
    that stepping down from 100 takes the value to 99 instead of 90.
    Thus a step up followed by a step down -- or vice versa -- always
    lands on the starting value; 99 -> 100 -> 99.

    Setting this will cause the spin box to disregard the value of
    \l singleStep, although it is preserved so that \l singleStep
    comes into effect if adaptive decimal step is later turned off.

    \since 5.12
*/

void QSpinBox::setStepType(QAbstractSpinBox::StepType stepType)
{
    Q_D(QSpinBox);
    d->stepType = stepType;
}

/*!
    \property QSpinBox::stepType
    \brief The step type.

    The step type can be single step or adaptive decimal step.
*/

QAbstractSpinBox::StepType QSpinBox::stepType() const
{
    Q_D(const QSpinBox);
    return d->stepType;
}

/*!
    \property QSpinBox::displayIntegerBase

    \brief the base used to display the value of the spin box

    The default displayIntegerBase value is 10.

    \sa textFromValue(), valueFromText()
    \since 5.2
*/

int QSpinBox::displayIntegerBase() const
{
    Q_D(const QSpinBox);
    return d->displayIntegerBase;
}

void QSpinBox::setDisplayIntegerBase(int base)
{
    Q_D(QSpinBox);
    // Falls back to base 10 on invalid bases (like QString)
    if (Q_UNLIKELY(base < 2 || base > 36)) {
        qWarning("QSpinBox::setDisplayIntegerBase: Invalid base (%d)", base);
        base = 10;
    }

    if (base != d->displayIntegerBase) {
        d->displayIntegerBase = base;
        d->updateEdit();
    }
}

/*!
    This virtual function is used by the spin box whenever it needs to
    display the given \a value. The default implementation returns a
    string containing \a value printed in the standard way using
    QWidget::locale().toString(), but with the thousand separator
    removed unless setGroupSeparatorShown() is set. Reimplementations may
    return anything. (See the example in the detailed description.)

    Note: QSpinBox does not call this function for specialValueText()
    and that neither prefix() nor suffix() should be included in the
    return value.

    If you reimplement this, you may also need to reimplement
    valueFromText() and validate()

    \sa valueFromText(), validate(), QLocale::groupSeparator()
*/

QString QSpinBox::textFromValue(int value) const
{
    Q_D(const QSpinBox);
    QString str;

    if (d->displayIntegerBase != 10) {
        const QLatin1String prefix = value < 0 ? QLatin1String("-") : QLatin1String();
        str = prefix + QString::number(qAbs(value), d->displayIntegerBase);
    } else {
        str = locale().toString(value);
        if (!d->showGroupSeparator && (qAbs(value) >= 1000 || value == INT_MIN)) {
            str.remove(locale().groupSeparator());
        }
    }

    return str;
}

/*!
    \fn int QSpinBox::valueFromText(const QString &text) const

    This virtual function is used by the spin box whenever it needs to
    interpret \a text entered by the user as a value.

    Subclasses that need to display spin box values in a non-numeric
    way need to reimplement this function.

    Note: QSpinBox handles specialValueText() separately; this
    function is only concerned with the other values.

    \sa textFromValue(), validate()
*/

int QSpinBox::valueFromText(const QString &text) const
{
    Q_D(const QSpinBox);

    QString copy = text;
    int pos = d->edit->cursorPosition();
    QValidator::State state = QValidator::Acceptable;
    return d->validateAndInterpret(copy, pos, state).toInt();
}

/*!
  \reimp
*/
QValidator::State QSpinBox::validate(QString &text, int &pos) const
{
    Q_D(const QSpinBox);

    QValidator::State state;
    d->validateAndInterpret(text, pos, state);
    return state;
}


/*!
  \reimp
*/
void QSpinBox::fixup(QString &input) const
{
    if (!isGroupSeparatorShown())
        input.remove(locale().groupSeparator());
}


// --- QDoubleSpinBox ---

/*!
    \class QDoubleSpinBox
    \brief The QDoubleSpinBox class provides a spin box widget that
    takes doubles.

    \ingroup basicwidgets
    \inmodule QtWidgets

    QDoubleSpinBox allows the user to choose a value by clicking the
    up and down buttons or by pressing Up or Down on the keyboard to
    increase or decrease the value currently displayed. The user can
    also type the value in manually. The spin box supports double
    values but can be extended to use different strings with
    validate(), textFromValue() and valueFromText().

    Every time the value changes QDoubleSpinBox emits two
    valueChanged() signals, one taking providing a double and the other
    a QString. The QString overload provides the value with both
    prefix() and suffix(). The current value can be fetched with
    value() and set with setValue().

    Note: QDoubleSpinBox will round numbers so they can be displayed
    with the current precision. In a QDoubleSpinBox with decimals set
    to 2, calling setValue(2.555) will cause value() to return 2.56.

    Clicking the up and down buttons or using the keyboard accelerator's
    Up and Down arrows will increase or decrease the current value in
    steps of size singleStep(). If you want to change this behavior you
    can reimplement the virtual function stepBy(). The minimum and
    maximum value and the step size can be set using one of the
    constructors, and can be changed later with setMinimum(),
    setMaximum() and setSingleStep(). The spinbox has a default
    precision of 2 decimal places but this can be changed using
    setDecimals().

    Most spin boxes are directional, but QDoubleSpinBox can also
    operate as a circular spin box, i.e. if the range is 0.0-99.9 and
    the current value is 99.9, clicking "up" will give 0 if wrapping()
    is set to true. Use setWrapping() if you want circular behavior.

    The displayed value can be prepended and appended with arbitrary
    strings indicating, for example, currency or the unit of
    measurement. See setPrefix() and setSuffix(). The text in the spin
    box is retrieved with text() (which includes any prefix() and
    suffix()), or with cleanText() (which has no prefix(), no suffix()
    and no leading or trailing whitespace).

    It is often desirable to give the user a special (often default)
    choice in addition to the range of numeric values. See
    setSpecialValueText() for how to do this with QDoubleSpinBox.

    \note The displayed value of the QDoubleSpinBox is limited to 18 characters
    in addition to eventual prefix and suffix content. This limitation is used
    to keep the double spin box usable even with extremely large values.
    \sa QSpinBox, QDateTimeEdit, QSlider, {Spin Boxes Example}
*/

/*!
    \fn void QDoubleSpinBox::valueChanged(double d);

    This signal is emitted whenever the spin box's value is changed.
    The new value is passed in \a d.
*/

/*!
    \fn void QDoubleSpinBox::valueChanged(const QString &text);

    \overload

    The new value is passed in \a text with prefix() and suffix().
*/

/*!
    Constructs a spin box with 0.0 as minimum value and 99.99 as maximum value,
    a step value of 1.0 and a precision of 2 decimal places. The value is
    initially set to 0.00. The spin box has the given \a parent.

    \sa setMinimum(), setMaximum(), setSingleStep()
*/
QDoubleSpinBox::QDoubleSpinBox(QWidget *parent)
    : QAbstractSpinBox(*new QDoubleSpinBoxPrivate, parent)
{
    Q_D(QDoubleSpinBox);
    d->init();
}

/*!
    Destructor.
*/
QDoubleSpinBox::~QDoubleSpinBox() {}

/*!
    \property QDoubleSpinBox::value
    \brief the value of the spin box

    setValue() will emit valueChanged() if the new value is different
    from the old one. The value property has a second notifier
    signal which includes the spin box's prefix and suffix.

    Note: The value will be rounded so it can be displayed with the
    current setting of decimals.

    \sa decimals
*/
double QDoubleSpinBox::value() const
{
    Q_D(const QDoubleSpinBox);

    return d->value.toDouble();
}

void QDoubleSpinBox::setValue(double value)
{
    Q_D(QDoubleSpinBox);
    QVariant v(d->round(value));
    d->setValue(v, EmitIfChanged);
}
/*!
    \property QDoubleSpinBox::prefix
    \brief the spin box's prefix

    The prefix is prepended to the start of the displayed value.
    Typical use is to display a unit of measurement or a currency
    symbol. For example:

    \snippet code/src_gui_widgets_qspinbox.cpp 4

    To turn off the prefix display, set this property to an empty
    string. The default is no prefix. The prefix is not displayed when
    value() == minimum() and specialValueText() is set.

    If no prefix is set, prefix() returns an empty string.

    \sa suffix(), setSuffix(), specialValueText(), setSpecialValueText()
*/

QString QDoubleSpinBox::prefix() const
{
    Q_D(const QDoubleSpinBox);

    return d->prefix;
}

void QDoubleSpinBox::setPrefix(const QString &prefix)
{
    Q_D(QDoubleSpinBox);

    d->prefix = prefix;
    d->updateEdit();
}

/*!
    \property QDoubleSpinBox::suffix
    \brief the suffix of the spin box

    The suffix is appended to the end of the displayed value. Typical
    use is to display a unit of measurement or a currency symbol. For
    example:

    \snippet code/src_gui_widgets_qspinbox.cpp 5

    To turn off the suffix display, set this property to an empty
    string. The default is no suffix. The suffix is not displayed for
    the minimum() if specialValueText() is set.

    If no suffix is set, suffix() returns an empty string.

    \sa prefix(), setPrefix(), specialValueText(), setSpecialValueText()
*/

QString QDoubleSpinBox::suffix() const
{
    Q_D(const QDoubleSpinBox);

    return d->suffix;
}

void QDoubleSpinBox::setSuffix(const QString &suffix)
{
    Q_D(QDoubleSpinBox);

    d->suffix = suffix;
    d->updateEdit();

    d->cachedSizeHint = QSize();
    updateGeometry();
}

/*!
    \property QDoubleSpinBox::cleanText

    \brief the text of the spin box excluding any prefix, suffix,
    or leading or trailing whitespace.

    \sa text, QDoubleSpinBox::prefix, QDoubleSpinBox::suffix
*/

QString QDoubleSpinBox::cleanText() const
{
    Q_D(const QDoubleSpinBox);

    return d->stripped(d->edit->displayText());
}

/*!
    \property QDoubleSpinBox::singleStep
    \brief the step value

    When the user uses the arrows to change the spin box's value the
    value will be incremented/decremented by the amount of the
    singleStep. The default value is 1.0. Setting a singleStep value
    of less than 0 does nothing.
*/
double QDoubleSpinBox::singleStep() const
{
    Q_D(const QDoubleSpinBox);

    return d->singleStep.toDouble();
}

void QDoubleSpinBox::setSingleStep(double value)
{
    Q_D(QDoubleSpinBox);

    if (value >= 0) {
        d->singleStep = value;
        d->updateEdit();
    }
}

/*!
    \property QDoubleSpinBox::minimum

    \brief the minimum value of the spin box

    When setting this property the \l maximum is adjusted
    if necessary to ensure that the range remains valid.

    The default minimum value is 0.0.

    Note: The minimum value will be rounded to match the decimals
    property.

    \sa decimals, setRange(), specialValueText
*/

double QDoubleSpinBox::minimum() const
{
    Q_D(const QDoubleSpinBox);

    return d->minimum.toDouble();
}

void QDoubleSpinBox::setMinimum(double minimum)
{
    Q_D(QDoubleSpinBox);
    d->actualMin = minimum;
    const QVariant m(d->round(minimum));
    d->setRange(m, (d->variantCompare(d->maximum, m) > 0 ? d->maximum : m));
}

/*!
    \property QDoubleSpinBox::maximum

    \brief the maximum value of the spin box

    When setting this property the \l minimum is adjusted
    if necessary, to ensure that the range remains valid.

    The default maximum value is 99.99.

    Note: The maximum value will be rounded to match the decimals
    property.

    \sa decimals, setRange()
*/

double QDoubleSpinBox::maximum() const
{
    Q_D(const QDoubleSpinBox);

    return d->maximum.toDouble();
}

void QDoubleSpinBox::setMaximum(double maximum)
{
    Q_D(QDoubleSpinBox);
    d->actualMax = maximum;
    const QVariant m(d->round(maximum));
    d->setRange((d->variantCompare(d->minimum, m) < 0 ? d->minimum : m), m);
}

/*!
    Convenience function to set the \a minimum and \a maximum values
    with a single function call.

    Note: The maximum and minimum values will be rounded to match the
    decimals property.

    \snippet code/src_gui_widgets_qspinbox.cpp 6
    is equivalent to:
    \snippet code/src_gui_widgets_qspinbox.cpp 7

    \sa minimum, maximum
*/

void QDoubleSpinBox::setRange(double minimum, double maximum)
{
    Q_D(QDoubleSpinBox);
    d->actualMin = minimum;
    d->actualMax = maximum;
    d->setRange(QVariant(d->round(minimum)), QVariant(d->round(maximum)));
}

/*!
    Sets the step type for the spin box to \a stepType, which is single
    step or adaptive decimal step.

    Adaptive decimal step means that the step size will continuously be
    adjusted to one power of ten below the current \l value. So when
    the value is 1100, the step is set to 100, so stepping up once
    increases it to 1200. For 1200 stepping up takes it to 1300. For
    negative values, stepping down from -1100 goes to -1200.

    It also works for any decimal values, 0.041 is increased to 0.042
    by stepping once.

    Step direction is taken into account to handle edges cases, so
    that stepping down from 100 takes the value to 99 instead of 90.
    Thus a step up followed by a step down -- or vice versa -- always
    lands on the starting value; 99 -> 100 -> 99.

    Setting this will cause the spin box to disregard the value of
    \l singleStep, although it is preserved so that \l singleStep
    comes into effect if adaptive decimal step is later turned off.

    \since 5.12
*/

void QDoubleSpinBox::setStepType(StepType stepType)
{
    Q_D(QDoubleSpinBox);
    d->stepType = stepType;
}

/*!
    \property QDoubleSpinBox::stepType
    \brief The step type.

    The step type can be single step or adaptive decimal step.
*/

QAbstractSpinBox::StepType QDoubleSpinBox::stepType() const
{
    Q_D(const QDoubleSpinBox);
    return d->stepType;
}

/*!
     \property QDoubleSpinBox::decimals

     \brief the precision of the spin box, in decimals

     Sets how many decimals the spinbox will use for displaying and
     interpreting doubles.

     \warning The maximum value for \a decimals is DBL_MAX_10_EXP +
     DBL_DIG (ie. 323) because of the limitations of the double type.

     Note: The maximum, minimum and value might change as a result of
     changing this property.
*/

int QDoubleSpinBox::decimals() const
{
    Q_D(const QDoubleSpinBox);

    return d->decimals;
}

void QDoubleSpinBox::setDecimals(int decimals)
{
    Q_D(QDoubleSpinBox);
    d->decimals = qBound(0, decimals, DBL_MAX_10_EXP + DBL_DIG);

    setRange(d->actualMin, d->actualMax); // make sure values are rounded
    setValue(value());
}

/*!
    This virtual function is used by the spin box whenever it needs to
    display the given \a value. The default implementation returns a string
    containing \a value printed using QWidget::locale().toString(\a value,
    QLatin1Char('f'), decimals()) and will remove the thousand
    separator unless setGroupSeparatorShown() is set. Reimplementations may
    return anything.

    Note: QDoubleSpinBox does not call this function for
    specialValueText() and that neither prefix() nor suffix() should
    be included in the return value.

    If you reimplement this, you may also need to reimplement
    valueFromText().

    \sa valueFromText(), QLocale::groupSeparator()
*/


QString QDoubleSpinBox::textFromValue(double value) const
{
    Q_D(const QDoubleSpinBox);
    QString str = locale().toString(value, 'f', d->decimals);
    if (!d->showGroupSeparator && qAbs(value) >= 1000.0)
        str.remove(locale().groupSeparator());

    return str;
}

/*!
    This virtual function is used by the spin box whenever it needs to
    interpret \a text entered by the user as a value.

    Subclasses that need to display spin box values in a non-numeric
    way need to reimplement this function.

    Note: QDoubleSpinBox handles specialValueText() separately; this
    function is only concerned with the other values.

    \sa textFromValue(), validate()
*/
double QDoubleSpinBox::valueFromText(const QString &text) const
{
    Q_D(const QDoubleSpinBox);

    QString copy = text;
    int pos = d->edit->cursorPosition();
    QValidator::State state = QValidator::Acceptable;
    return d->validateAndInterpret(copy, pos, state).toDouble();
}

/*!
  \reimp
*/
QValidator::State QDoubleSpinBox::validate(QString &text, int &pos) const
{
    Q_D(const QDoubleSpinBox);

    QValidator::State state;
    d->validateAndInterpret(text, pos, state);
    return state;
}


/*!
  \reimp
*/
void QDoubleSpinBox::fixup(QString &input) const
{
    input.remove(locale().groupSeparator());
}

// --- QSpinBoxPrivate ---

/*!
    \internal
    Constructs a QSpinBoxPrivate object
*/

QSpinBoxPrivate::QSpinBoxPrivate()
{
    minimum = QVariant((int)0);
    maximum = QVariant((int)99);
    value = minimum;
    displayIntegerBase = 10;
    singleStep = QVariant((int)1);
    type = QVariant::Int;
}

/*!
    \internal
    \reimp
*/

void QSpinBoxPrivate::emitSignals(EmitPolicy ep, const QVariant &old)
{
    Q_Q(QSpinBox);
    if (ep != NeverEmit) {
        pendingEmit = false;
        if (ep == AlwaysEmit || value != old) {
            emit q->valueChanged(edit->displayText());
            emit q->valueChanged(value.toInt());
        }
    }
}

/*!
    \internal
    \reimp
*/

QString QSpinBoxPrivate::textFromValue(const QVariant &value) const
{
    Q_Q(const QSpinBox);
    return q->textFromValue(value.toInt());
}
/*!
    \internal
    \reimp
*/

QVariant QSpinBoxPrivate::valueFromText(const QString &text) const
{
    Q_Q(const QSpinBox);

    return QVariant(q->valueFromText(text));
}


/*!
    \internal Multi purpose function that parses input, sets state to
    the appropriate state and returns the value it will be interpreted
    as.
*/

QVariant QSpinBoxPrivate::validateAndInterpret(QString &input, int &pos,
                                               QValidator::State &state) const
{
    if (cachedText == input && !input.isEmpty()) {
        state = cachedState;
        QSBDEBUG() << "cachedText was '" << cachedText << "' state was "
                   << state << " and value was " << cachedValue;

        return cachedValue;
    }
    const int max = maximum.toInt();
    const int min = minimum.toInt();

    QString copy = stripped(input, &pos);
    QSBDEBUG() << "input" << input << "copy" << copy;
    state = QValidator::Acceptable;
    int num = min;

    if (max != min && (copy.isEmpty()
                       || (min < 0 && copy == QLatin1String("-"))
                       || (max >= 0 && copy == QLatin1String("+")))) {
        state = QValidator::Intermediate;
        QSBDEBUG() << __FILE__ << __LINE__<< "num is set to" << num;
    } else if (copy.startsWith(QLatin1Char('-')) && min >= 0) {
        state = QValidator::Invalid; // special-case -0 will be interpreted as 0 and thus not be invalid with a range from 0-100
    } else {
        bool ok = false;
        if (displayIntegerBase != 10) {
            num = copy.toInt(&ok, displayIntegerBase);
        } else {
            num = locale.toInt(copy, &ok);
            if (!ok && copy.contains(locale.groupSeparator()) && (max >= 1000 || min <= -1000)) {
                QString copy2 = copy;
                copy2.remove(locale.groupSeparator());
                num = locale.toInt(copy2, &ok);
            }
        }
        QSBDEBUG() << __FILE__ << __LINE__<< "num is set to" << num;
        if (!ok) {
            state = QValidator::Invalid;
        } else if (num >= min && num <= max) {
            state = QValidator::Acceptable;
        } else if (max == min) {
            state = QValidator::Invalid;
        } else {
            if ((num >= 0 && num > max) || (num < 0 && num < min)) {
                state = QValidator::Invalid;
                QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
            } else {
                state = QValidator::Intermediate;
                QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Intermediate";
            }
        }
    }
    if (state != QValidator::Acceptable)
        num = max > 0 ? min : max;
    input = prefix + copy + suffix;
    cachedText = input;
    cachedState = state;
    cachedValue = QVariant((int)num);

    QSBDEBUG() << "cachedText is set to '" << cachedText << "' state is set to "
               << state << " and value is set to " << cachedValue;
    return cachedValue;
}

QVariant QSpinBoxPrivate::calculateAdaptiveDecimalStep(int steps) const
{
    const int intValue = value.toInt();
    const int absValue = qAbs(intValue);

    if (absValue < 100)
        return 1;

    const bool valueNegative = intValue < 0;
    const bool stepsNegative = steps < 0;
    const int signCompensation = (valueNegative == stepsNegative) ? 0 : 1;

    const int log = static_cast<int>(std::log10(absValue - signCompensation)) - 1;
    return static_cast<int>(std::pow(10, log));
}

// --- QDoubleSpinBoxPrivate ---

/*!
    \internal
    Constructs a QSpinBoxPrivate object
*/

QDoubleSpinBoxPrivate::QDoubleSpinBoxPrivate()
{
    actualMin = 0.0;
    actualMax = 99.99;
    minimum = QVariant(actualMin);
    maximum = QVariant(actualMax);
    value = minimum;
    singleStep = QVariant(1.0);
    decimals = 2;
    type = QVariant::Double;
}

/*!
    \internal
    \reimp
*/

void QDoubleSpinBoxPrivate::emitSignals(EmitPolicy ep, const QVariant &old)
{
    Q_Q(QDoubleSpinBox);
    if (ep != NeverEmit) {
        pendingEmit = false;
        if (ep == AlwaysEmit || value != old) {
            emit q->valueChanged(edit->displayText());
            emit q->valueChanged(value.toDouble());
        }
    }
}


/*!
    \internal
    \reimp
*/
QVariant QDoubleSpinBoxPrivate::valueFromText(const QString &f) const
{
    Q_Q(const QDoubleSpinBox);
    return QVariant(q->valueFromText(f));
}

/*!
    \internal
    Rounds to a double value that is restricted to decimals.
    E.g. // decimals = 2

    round(5.555) => 5.56
    */

double QDoubleSpinBoxPrivate::round(double value) const
{
    return QString::number(value, 'f', decimals).toDouble();
}


/*!
    \internal Multi purpose function that parses input, sets state to
    the appropriate state and returns the value it will be interpreted
    as.
*/

QVariant QDoubleSpinBoxPrivate::validateAndInterpret(QString &input, int &pos,
                                                     QValidator::State &state) const
{
    if (cachedText == input && !input.isEmpty()) {
        state = cachedState;
        QSBDEBUG() << "cachedText was '" << cachedText << "' state was "
                   << state << " and value was " << cachedValue;
        return cachedValue;
    }
    const double max = maximum.toDouble();
    const double min = minimum.toDouble();

    QString copy = stripped(input, &pos);
    QSBDEBUG() << "input" << input << "copy" << copy;
    int len = copy.size();
    double num = min;
    const bool plus = max >= 0;
    const bool minus = min <= 0;

    switch (len) {
    case 0:
        state = max != min ? QValidator::Intermediate : QValidator::Invalid;
        goto end;
    case 1:
        if (copy.at(0) == locale.decimalPoint()
            || (plus && copy.at(0) == QLatin1Char('+'))
            || (minus && copy.at(0) == QLatin1Char('-'))) {
            state = QValidator::Intermediate;
            goto end;
        }
        break;
    case 2:
        if (copy.at(1) == locale.decimalPoint()
            && ((plus && copy.at(0) == QLatin1Char('+')) || (minus && copy.at(0) == QLatin1Char('-')))) {
            state = QValidator::Intermediate;
            goto end;
        }
        break;
    default: break;
    }

    if (copy.at(0) == locale.groupSeparator()) {
        QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
        state = QValidator::Invalid;
        goto end;
    } else if (len > 1) {
        const int dec = copy.indexOf(locale.decimalPoint());
        if (dec != -1) {
            if (dec + 1 < copy.size() && copy.at(dec + 1) == locale.decimalPoint() && pos == dec + 1) {
                copy.remove(dec + 1, 1); // typing a delimiter when you are on the delimiter
            } // should be treated as typing right arrow

            if (copy.size() - dec > decimals + 1) {
                QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
                state = QValidator::Invalid;
                goto end;
            }
            for (int i=dec + 1; i<copy.size(); ++i) {
                if (copy.at(i).isSpace() || copy.at(i) == locale.groupSeparator()) {
                    QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
                    state = QValidator::Invalid;
                    goto end;
                }
            }
        } else {
            const QChar last = copy.at(len - 1);
            const QChar secondLast = copy.at(len - 2);
            if ((last == locale.groupSeparator() || last.isSpace())
                && (secondLast == locale.groupSeparator() || secondLast.isSpace())) {
                state = QValidator::Invalid;
                QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
                goto end;
            } else if (last.isSpace() && (!locale.groupSeparator().isSpace() || secondLast.isSpace())) {
                state = QValidator::Invalid;
                QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
                goto end;
            }
        }
    }

    {
        bool ok = false;
        num = locale.toDouble(copy, &ok);
        QSBDEBUG() << __FILE__ << __LINE__ << locale << copy << num << ok;

        if (!ok) {
            if (locale.groupSeparator().isPrint()) {
                if (max < 1000 && min > -1000 && copy.contains(locale.groupSeparator())) {
                    state = QValidator::Invalid;
                    QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
                    goto end;
                }

                const int len = copy.size();
                for (int i=0; i<len- 1; ++i) {
                    if (copy.at(i) == locale.groupSeparator() && copy.at(i + 1) == locale.groupSeparator()) {
                        QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
                        state = QValidator::Invalid;
                        goto end;
                    }
                }

                QString copy2 = copy;
                copy2.remove(locale.groupSeparator());
                num = locale.toDouble(copy2, &ok);
                QSBDEBUG() << locale.groupSeparator() << num << copy2 << ok;

                if (!ok) {
                    state = QValidator::Invalid;
                    QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
                    goto end;
                }
            }
        }

        if (!ok) {
            state = QValidator::Invalid;
            QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
        } else if (num >= min && num <= max) {
            state = QValidator::Acceptable;
            QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Acceptable";
        } else if (max == min) { // when max and min is the same the only non-Invalid input is max (or min)
            state = QValidator::Invalid;
            QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
        } else {
            if ((num >= 0 && num > max) || (num < 0 && num < min)) {
                state = QValidator::Invalid;
                QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Invalid";
            } else {
                state = QValidator::Intermediate;
                QSBDEBUG() << __FILE__ << __LINE__<< "state is set to Intermediate";
            }
        }
    }

end:
    if (state != QValidator::Acceptable) {
        num = max > 0 ? min : max;
    }

    input = prefix + copy + suffix;
    cachedText = input;
    cachedState = state;
    cachedValue = QVariant(num);
    return QVariant(num);
}

/*
    \internal
    \reimp
*/

QString QDoubleSpinBoxPrivate::textFromValue(const QVariant &f) const
{
    Q_Q(const QDoubleSpinBox);
    return q->textFromValue(f.toDouble());
}

QVariant QDoubleSpinBoxPrivate::calculateAdaptiveDecimalStep(int steps) const
{
    const double doubleValue = value.toDouble();
    const double minStep = std::pow(10, -decimals);
    double absValue = qAbs(doubleValue);

    if (absValue < minStep)
        return minStep;

    const bool valueNegative = doubleValue < 0;
    const bool stepsNegative = steps < 0;
    if (valueNegative != stepsNegative)
        absValue /= 1.01;

    const double shift = std::pow(10, 1 - std::floor(std::log10(absValue)));
    const double absRounded = round(absValue * shift) / shift;
    const double log = floorf(std::log10(absRounded)) - 1;

    return std::max(minStep, std::pow(10, log));
}

/*! \reimp */
bool QSpinBox::event(QEvent *event)
{
    Q_D(QSpinBox);
    if (event->type() == QEvent::StyleChange
#ifdef Q_OS_MAC
            || event->type() == QEvent::MacSizeChange
#endif
            )
        d->setLayoutItemMargins(QStyle::SE_SpinBoxLayoutItem);
    return QAbstractSpinBox::event(event);
}

QT_END_NAMESPACE

#include "moc_qspinbox.cpp"
