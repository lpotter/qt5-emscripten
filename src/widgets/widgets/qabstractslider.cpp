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

#include <qapplication.h>
#include "qabstractslider.h"
#include "qevent.h"
#include "qabstractslider_p.h"
#include "qdebug.h"
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif
#include <limits.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractSlider
    \brief The QAbstractSlider class provides an integer value within a range.

    \ingroup abstractwidgets
    \inmodule QtWidgets

    The class is designed as a common super class for widgets like
    QScrollBar, QSlider and QDial.

    Here are the main properties of the class:

    \list 1

    \li \l value: The bounded integer that QAbstractSlider maintains.

    \li \l minimum: The lowest possible value.

    \li \l maximum: The highest possible value.

    \li \l singleStep: The smaller of two natural steps that an
    abstract sliders provides and typically corresponds to the user
    pressing an arrow key.

    \li \l pageStep: The larger of two natural steps that an abstract
    slider provides and typically corresponds to the user pressing
    PageUp or PageDown.

    \li \l tracking: Whether slider tracking is enabled.

    \li \l sliderPosition: The current position of the slider. If \l
    tracking is enabled (the default), this is identical to \l value.

    \endlist

    Unity (1) may be viewed as a third step size. setValue() lets you
    set the current value to any integer in the allowed range, not
    just minimum() + \e n * singleStep() for integer values of \e n.
    Some widgets may allow the user to set any value at all; others
    may just provide multiples of singleStep() or pageStep().

    QAbstractSlider emits a comprehensive set of signals:

    \table
    \header \li Signal \li Emitted when
    \row \li \l valueChanged()
         \li the value has changed. The \l tracking
            determines whether this signal is emitted during user
            interaction.
    \row \li \l sliderPressed()
         \li the user starts to drag the slider.
    \row \li \l sliderMoved()
         \li the user drags the slider.
    \row \li \l sliderReleased()
         \li the user releases the slider.
    \row \li \l actionTriggered()
         \li a slider action was triggerd.
    \row \li \l rangeChanged()
         \li a the range has changed.
    \endtable

    QAbstractSlider provides a virtual sliderChange() function that is
    well suited for updating the on-screen representation of
    sliders. By calling triggerAction(), subclasses trigger slider
    actions. Two helper functions QStyle::sliderPositionFromValue() and
    QStyle::sliderValueFromPosition() help subclasses and styles to map
    screen coordinates to logical range values.

    \sa QAbstractSpinBox, QSlider, QDial, QScrollBar, {Sliders Example}
*/

/*!
    \enum QAbstractSlider::SliderAction

    \value SliderNoAction
    \value SliderSingleStepAdd
    \value SliderSingleStepSub
    \value SliderPageStepAdd
    \value SliderPageStepSub
    \value SliderToMinimum
    \value SliderToMaximum
    \value SliderMove

*/

/*!
    \fn void QAbstractSlider::valueChanged(int value)

    This signal is emitted when the slider value has changed, with the
    new slider \a value as argument.
*/

/*!
    \fn void QAbstractSlider::sliderPressed()

    This signal is emitted when the user presses the slider with the
    mouse, or programmatically when setSliderDown(true) is called.

    \sa sliderReleased(), sliderMoved(), isSliderDown()
*/

/*!
    \fn void QAbstractSlider::sliderMoved(int value)

    This signal is emitted when sliderDown is true and the slider moves. This
    usually happens when the user is dragging the slider. The \a value
    is the new slider position.

    This signal is emitted even when tracking is turned off.

    \sa setTracking(), valueChanged(), isSliderDown(),
    sliderPressed(), sliderReleased()
*/

/*!
    \fn void QAbstractSlider::sliderReleased()

    This signal is emitted when the user releases the slider with the
    mouse, or programmatically when setSliderDown(false) is called.

    \sa sliderPressed(), sliderMoved(), sliderDown
*/

/*!
    \fn void QAbstractSlider::rangeChanged(int min, int max)

    This signal is emitted when the slider range has changed, with \a
    min being the new minimum, and \a max being the new maximum.

    \sa minimum, maximum
*/

/*!
    \fn void QAbstractSlider::actionTriggered(int action)

    This signal is emitted when the slider action \a action is
    triggered. Actions are \l SliderSingleStepAdd, \l
    SliderSingleStepSub, \l SliderPageStepAdd, \l SliderPageStepSub,
    \l SliderToMinimum, \l SliderToMaximum, and \l SliderMove.

    When the signal is emitted, the \l sliderPosition has been
    adjusted according to the action, but the \l value has not yet
    been propagated (meaning the valueChanged() signal was not yet
    emitted), and the visual display has not been updated. In slots
    connected to this signal you can thus safely adjust any action by
    calling setSliderPosition() yourself, based on both the action and
    the slider's value.

    \sa triggerAction()
*/

/*!
    \enum QAbstractSlider::SliderChange

    \value SliderRangeChange
    \value SliderOrientationChange
    \value SliderStepsChange
    \value SliderValueChange
*/

QAbstractSliderPrivate::QAbstractSliderPrivate()
    : minimum(0), maximum(99), pageStep(10), value(0), position(0), pressValue(-1),
      singleStep(1), singleStepFromItemView(-1), viewMayChangeSingleStep(true), offset_accumulated(0), tracking(true),
      blocktracking(false), pressed(false),
      invertedAppearance(false), invertedControls(false),
      orientation(Qt::Horizontal), repeatAction(QAbstractSlider::SliderNoAction)
#ifdef QT_KEYPAD_NAVIGATION
      , isAutoRepeating(false)
      , repeatMultiplier(1)
{
    firstRepeat.invalidate();
#else
{
#endif

}

QAbstractSliderPrivate::~QAbstractSliderPrivate()
{
}

/*!
    Sets the slider's minimum to \a min and its maximum to \a max.

    If \a max is smaller than \a min, \a min becomes the only legal
    value.

    \sa minimum, maximum
*/
void QAbstractSlider::setRange(int min, int max)
{
    Q_D(QAbstractSlider);
    int oldMin = d->minimum;
    int oldMax = d->maximum;
    d->minimum = min;
    d->maximum = qMax(min, max);
    if (oldMin != d->minimum || oldMax != d->maximum) {
        sliderChange(SliderRangeChange);
        emit rangeChanged(d->minimum, d->maximum);
        setValue(d->value); // re-bound
    }
}


void QAbstractSliderPrivate::setSteps(int single, int page)
{
    Q_Q(QAbstractSlider);
    singleStep = qAbs(single);
    pageStep = qAbs(page);
    q->sliderChange(QAbstractSlider::SliderStepsChange);
}

/*!
    Constructs an abstract slider.

    The \a parent argument is sent to the QWidget constructor.

    The \l minimum defaults to 0, the \l maximum to 99, with a \l
    singleStep size of 1 and a \l pageStep size of 10, and an initial
    \l value of 0.
*/
QAbstractSlider::QAbstractSlider(QWidget *parent)
    :QWidget(*new QAbstractSliderPrivate, parent, 0)
{
}

/*! \internal */
QAbstractSlider::QAbstractSlider(QAbstractSliderPrivate &dd, QWidget *parent)
    :QWidget(dd, parent, 0)
{
}

/*!
    Destroys the slider.
*/
QAbstractSlider::~QAbstractSlider()
{
}

/*!
    \property QAbstractSlider::orientation
    \brief the orientation of the slider

    The orientation must be \l Qt::Vertical (the default) or \l
    Qt::Horizontal.
*/
void QAbstractSlider::setOrientation(Qt::Orientation orientation)
{
    Q_D(QAbstractSlider);
    if (d->orientation == orientation)
        return;

    d->orientation = orientation;
    if (!testAttribute(Qt::WA_WState_OwnSizePolicy)) {
        setSizePolicy(sizePolicy().transposed());
        setAttribute(Qt::WA_WState_OwnSizePolicy, false);
    }
    update();
    updateGeometry();
}

Qt::Orientation QAbstractSlider::orientation() const
{
    Q_D(const QAbstractSlider);
    return d->orientation;
}


/*!
    \property QAbstractSlider::minimum
    \brief the sliders's minimum value

    When setting this property, the \l maximum is adjusted if
    necessary to ensure that the range remains valid. Also the
    slider's current value is adjusted to be within the new range.

*/

void QAbstractSlider::setMinimum(int min)
{
    Q_D(QAbstractSlider);
    setRange(min, qMax(d->maximum, min));
}

int QAbstractSlider::minimum() const
{
    Q_D(const QAbstractSlider);
    return d->minimum;
}


/*!
    \property QAbstractSlider::maximum
    \brief the slider's maximum value

    When setting this property, the \l minimum is adjusted if
    necessary to ensure that the range remains valid.  Also the
    slider's current value is adjusted to be within the new range.


*/

void QAbstractSlider::setMaximum(int max)
{
    Q_D(QAbstractSlider);
    setRange(qMin(d->minimum, max), max);
}

int QAbstractSlider::maximum() const
{
    Q_D(const QAbstractSlider);
    return d->maximum;
}



/*!
    \property QAbstractSlider::singleStep
    \brief the single step.

    The smaller of two natural steps that an
    abstract sliders provides and typically corresponds to the user
    pressing an arrow key.

    If the property is modified during an auto repeating key event, behavior
    is undefined.

    \sa pageStep
*/

void QAbstractSlider::setSingleStep(int step)
{
    Q_D(QAbstractSlider);

    d->viewMayChangeSingleStep = (step < 0);
    if (step < 0 && d->singleStepFromItemView > 0)
        step = d->singleStepFromItemView;

    if (step != d->singleStep)
        d->setSteps(step, d->pageStep);
}

int QAbstractSlider::singleStep() const
{
    Q_D(const QAbstractSlider);
    return d->singleStep;
}


/*!
    \property QAbstractSlider::pageStep
    \brief the page step.

    The larger of two natural steps that an abstract slider provides
    and typically corresponds to the user pressing PageUp or PageDown.

    \sa singleStep
*/

void QAbstractSlider::setPageStep(int step)
{
    Q_D(QAbstractSlider);
    if (step != d->pageStep)
        d->setSteps(d->singleStep, step);
}

int QAbstractSlider::pageStep() const
{
    Q_D(const QAbstractSlider);
    return d->pageStep;
}

/*!
    \property QAbstractSlider::tracking
    \brief whether slider tracking is enabled

    If tracking is enabled (the default), the slider emits the
    valueChanged() signal while the slider is being dragged. If
    tracking is disabled, the slider emits the valueChanged() signal
    only when the user releases the slider.

    \sa sliderDown
*/
void QAbstractSlider::setTracking(bool enable)
{
    Q_D(QAbstractSlider);
    d->tracking = enable;
}

bool QAbstractSlider::hasTracking() const
{
    Q_D(const QAbstractSlider);
    return d->tracking;
}


/*!
    \property QAbstractSlider::sliderDown
    \brief whether the slider is pressed down.

    The property is set by subclasses in order to let the abstract
    slider know whether or not \l tracking has any effect.

    Changing the slider down property emits the sliderPressed() and
    sliderReleased() signals.

*/
void QAbstractSlider::setSliderDown(bool down)
{
    Q_D(QAbstractSlider);
    bool doEmit = d->pressed != down;

    d->pressed = down;

    if (doEmit) {
        if (down)
            emit sliderPressed();
        else
            emit sliderReleased();
    }

    if (!down && d->position != d->value)
        triggerAction(SliderMove);
}

bool QAbstractSlider::isSliderDown() const
{
    Q_D(const QAbstractSlider);
    return d->pressed;
}


/*!
    \property QAbstractSlider::sliderPosition
    \brief the current slider position

    If \l tracking is enabled (the default), this is identical to \l value.
*/
void QAbstractSlider::setSliderPosition(int position)
{
    Q_D(QAbstractSlider);
    position = d->bound(position);
    if (position == d->position)
        return;
    d->position = position;
    if (!d->tracking)
        update();
    if (d->pressed)
        emit sliderMoved(position);
    if (d->tracking && !d->blocktracking)
        triggerAction(SliderMove);
}

int QAbstractSlider::sliderPosition() const
{
    Q_D(const QAbstractSlider);
    return d->position;
}


/*!
    \property QAbstractSlider::value
    \brief the slider's current value

    The slider forces the value to be within the legal range: \l
    minimum <= \c value <= \l maximum.

    Changing the value also changes the \l sliderPosition.
*/


int QAbstractSlider::value() const
{
    Q_D(const QAbstractSlider);
    return d->value;
}

void QAbstractSlider::setValue(int value)
{
    Q_D(QAbstractSlider);
    value = d->bound(value);
    if (d->value == value && d->position == value)
        return;
    d->value = value;
    if (d->position != value) {
        d->position = value;
        if (d->pressed)
            emit sliderMoved((d->position = value));
    }
#ifndef QT_NO_ACCESSIBILITY
    QAccessibleValueChangeEvent event(this, d->value);
    QAccessible::updateAccessibility(&event);
#endif
    sliderChange(SliderValueChange);
    emit valueChanged(value);
}

/*!
    \property QAbstractSlider::invertedAppearance
    \brief whether or not a slider shows its values inverted.

    If this property is \c false (the default), the minimum and maximum will
    be shown in its classic position for the inherited widget. If the
    value is true, the minimum and maximum appear at their opposite location.

    Note: This property makes most sense for sliders and dials. For
    scroll bars, the visual effect of the scroll bar subcontrols depends on
    whether or not the styles understand inverted appearance; most styles
    ignore this property for scroll bars.
*/

bool QAbstractSlider::invertedAppearance() const
{
    Q_D(const QAbstractSlider);
    return d->invertedAppearance;
}

void QAbstractSlider::setInvertedAppearance(bool invert)
{
    Q_D(QAbstractSlider);
    d->invertedAppearance = invert;
    update();
}


/*!
    \property QAbstractSlider::invertedControls
    \brief whether or not the slider inverts its wheel and key events.

    If this property is \c false, scrolling the mouse wheel "up" and using keys
    like page up will increase the slider's value towards its maximum. Otherwise
    pressing page up will move value towards the slider's minimum.
*/


bool QAbstractSlider::invertedControls() const
{
    Q_D(const QAbstractSlider);
    return d->invertedControls;
}

void QAbstractSlider::setInvertedControls(bool invert)
{
    Q_D(QAbstractSlider);
    d->invertedControls = invert;
}

/*!  Triggers a slider \a action.  Possible actions are \l
  SliderSingleStepAdd, \l SliderSingleStepSub, \l SliderPageStepAdd,
  \l SliderPageStepSub, \l SliderToMinimum, \l SliderToMaximum, and \l
  SliderMove.

  \sa actionTriggered()
 */
void QAbstractSlider::triggerAction(SliderAction action)
{
    Q_D(QAbstractSlider);
    d->blocktracking = true;
    switch (action) {
    case SliderSingleStepAdd:
        setSliderPosition(d->overflowSafeAdd(d->effectiveSingleStep()));
        break;
    case SliderSingleStepSub:
        setSliderPosition(d->overflowSafeAdd(-d->effectiveSingleStep()));
        break;
    case SliderPageStepAdd:
        setSliderPosition(d->overflowSafeAdd(d->pageStep));
        break;
    case SliderPageStepSub:
        setSliderPosition(d->overflowSafeAdd(-d->pageStep));
        break;
    case SliderToMinimum:
        setSliderPosition(d->minimum);
        break;
    case SliderToMaximum:
        setSliderPosition(d->maximum);
        break;
    case SliderMove:
    case SliderNoAction:
        break;
    };
    emit actionTriggered(action);
    d->blocktracking = false;
    setValue(d->position);
}

/*!  Sets action \a action to be triggered repetitively in intervals
of \a repeatTime, after an initial delay of \a thresholdTime.

\sa triggerAction(), repeatAction()
 */
void QAbstractSlider::setRepeatAction(SliderAction action, int thresholdTime, int repeatTime)
{
    Q_D(QAbstractSlider);
    if ((d->repeatAction = action) == SliderNoAction) {
        d->repeatActionTimer.stop();
    } else {
        d->repeatActionTime = repeatTime;
        d->repeatActionTimer.start(thresholdTime, this);
    }
}

/*!
  Returns the current repeat action.
  \sa setRepeatAction()
 */
QAbstractSlider::SliderAction QAbstractSlider::repeatAction() const
{
    Q_D(const QAbstractSlider);
    return d->repeatAction;
}

/*!\reimp
 */
void QAbstractSlider::timerEvent(QTimerEvent *e)
{
    Q_D(QAbstractSlider);
    if (e->timerId() == d->repeatActionTimer.timerId()) {
        if (d->repeatActionTime) { // was threshold time, use repeat time next time
            d->repeatActionTimer.start(d->repeatActionTime, this);
            d->repeatActionTime = 0;
        }
        if (d->repeatAction == SliderPageStepAdd)
            d->setAdjustedSliderPosition(d->overflowSafeAdd(d->pageStep));
        else if (d->repeatAction == SliderPageStepSub)
            d->setAdjustedSliderPosition(d->overflowSafeAdd(-d->pageStep));
        else
            triggerAction(d->repeatAction);
    }
}

/*!
    Reimplement this virtual function to track slider changes such as
    \l SliderRangeChange, \l SliderOrientationChange, \l
    SliderStepsChange, or \l SliderValueChange. The default
    implementation only updates the display and ignores the \a change
    parameter.
 */
void QAbstractSlider::sliderChange(SliderChange)
{
    update();
}

bool QAbstractSliderPrivate::scrollByDelta(Qt::Orientation orientation, Qt::KeyboardModifiers modifiers, int delta)
{
    Q_Q(QAbstractSlider);
    int stepsToScroll = 0;
    // in Qt scrolling to the right gives negative values.
    if (orientation == Qt::Horizontal)
        delta = -delta;
    qreal offset = qreal(delta) / 120;

    if ((modifiers & Qt::ControlModifier) || (modifiers & Qt::ShiftModifier)) {
        // Scroll one page regardless of delta:
        stepsToScroll = qBound(-pageStep, int(offset * pageStep), pageStep);
        offset_accumulated = 0;
    } else {
        // Calculate how many lines to scroll. Depending on what delta is (and
        // offset), we might end up with a fraction (e.g. scroll 1.3 lines). We can
        // only scroll whole lines, so we keep the reminder until next event.
        qreal stepsToScrollF =
#if QT_CONFIG(wheelevent)
                QApplication::wheelScrollLines() *
#endif
                offset * effectiveSingleStep();
        // Check if wheel changed direction since last event:
        if (offset_accumulated != 0 && (offset / offset_accumulated) < 0)
            offset_accumulated = 0;

        offset_accumulated += stepsToScrollF;
#if 1 // Used to be excluded in Qt4 for Q_WS_MAC
        // Don't scroll more than one page in any case:
        stepsToScroll = qBound(-pageStep, int(offset_accumulated), pageStep);
#else
        // Native UI-elements on Mac can scroll hundreds of lines at a time as
        // a result of acceleration. So keep the same behaviour in Qt, and
        // don't restrict stepsToScroll to certain maximum (pageStep):
        stepsToScroll = int(offset_accumulated);
#endif
        offset_accumulated -= int(offset_accumulated);
        if (stepsToScroll == 0) {
            // We moved less than a line, but might still have accumulated partial scroll,
            // unless we already are at one of the ends.
            const float effective_offset = invertedControls ? -offset_accumulated : offset_accumulated;
            if (effective_offset > 0.f && value < maximum)
                return true;
            if (effective_offset < 0.f && value > minimum)
                return true;
            offset_accumulated = 0;
            return false;
        }
    }

    if (invertedControls)
        stepsToScroll = -stepsToScroll;

    int prevValue = value;
    position = bound(overflowSafeAdd(stepsToScroll)); // value will be updated by triggerAction()
    q->triggerAction(QAbstractSlider::SliderMove);

    if (prevValue == value) {
        offset_accumulated = 0;
        return false;
    }
    return true;
}

/*!
    \reimp
*/
#if QT_CONFIG(wheelevent)
void QAbstractSlider::wheelEvent(QWheelEvent * e)
{
    Q_D(QAbstractSlider);
    e->ignore();
    int delta = e->delta();
    if (e->inverted())
        delta = -delta;
    if (d->scrollByDelta(e->orientation(), e->modifiers(), delta))
        e->accept();
}

#endif

/*!
    \reimp
*/
void QAbstractSlider::keyPressEvent(QKeyEvent *ev)
{
    Q_D(QAbstractSlider);
    SliderAction action = SliderNoAction;
#ifdef QT_KEYPAD_NAVIGATION
    if (ev->isAutoRepeat()) {
        if (!d->firstRepeat.isValid())
            d->firstRepeat.start();
        else if (1 == d->repeatMultiplier) {
            // This is the interval in milli seconds which one key repetition
            // takes.
            const int repeatMSecs = d->firstRepeat.elapsed();

            /**
             * The time it takes to currently navigate the whole slider.
             */
            const qreal currentTimeElapse = (qreal(maximum()) / singleStep()) * repeatMSecs;

            /**
             * This is an arbitrarily determined constant in msecs that
             * specifies how long time it should take to navigate from the
             * start to the end(excluding starting key auto repeat).
             */
            const int SliderRepeatElapse = 2500;

            d->repeatMultiplier = currentTimeElapse / SliderRepeatElapse;
        }

    }
    else if (d->firstRepeat.isValid()) {
        d->firstRepeat.invalidate();
        d->repeatMultiplier = 1;
    }

#endif

    switch (ev->key()) {
#ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Select:
            if (QApplication::keypadNavigationEnabled())
                setEditFocus(!hasEditFocus());
            else
                ev->ignore();
            break;
        case Qt::Key_Back:
            if (QApplication::keypadNavigationEnabled() && hasEditFocus()) {
                setValue(d->origValue);
                setEditFocus(false);
            } else
                ev->ignore();
            break;
#endif

        // It seems we need to use invertedAppearance for Left and right, otherwise, things look weird.
        case Qt::Key_Left:
#ifdef QT_KEYPAD_NAVIGATION
            // In QApplication::KeypadNavigationDirectional, we want to change the slider
            // value if there is no left/right navigation possible and if this slider is not
            // inside a tab widget.
            if (QApplication::keypadNavigationEnabled()
                    && (!hasEditFocus() && QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                    || d->orientation == Qt::Vertical
                    || !hasEditFocus()
                    && (QWidgetPrivate::canKeypadNavigate(Qt::Horizontal) || QWidgetPrivate::inTabWidget(this)))) {
                ev->ignore();
                return;
            }
            if (QApplication::keypadNavigationEnabled() && d->orientation == Qt::Vertical)
                action = d->invertedControls ? SliderSingleStepSub : SliderSingleStepAdd;
            else
#endif
            if (isRightToLeft())
                action = d->invertedAppearance ? SliderSingleStepSub : SliderSingleStepAdd;
            else
                action = !d->invertedAppearance ? SliderSingleStepSub : SliderSingleStepAdd;
            break;
        case Qt::Key_Right:
#ifdef QT_KEYPAD_NAVIGATION
            // Same logic as in Qt::Key_Left
            if (QApplication::keypadNavigationEnabled()
                    && (!hasEditFocus() && QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                    || d->orientation == Qt::Vertical
                    || !hasEditFocus()
                    && (QWidgetPrivate::canKeypadNavigate(Qt::Horizontal) || QWidgetPrivate::inTabWidget(this)))) {
                ev->ignore();
                return;
            }
            if (QApplication::keypadNavigationEnabled() && d->orientation == Qt::Vertical)
                action = d->invertedControls ? SliderSingleStepAdd : SliderSingleStepSub;
            else
#endif
            if (isRightToLeft())
                action = d->invertedAppearance ? SliderSingleStepAdd : SliderSingleStepSub;
            else
                action = !d->invertedAppearance ? SliderSingleStepAdd : SliderSingleStepSub;
            break;
        case Qt::Key_Up:
#ifdef QT_KEYPAD_NAVIGATION
            // In QApplication::KeypadNavigationDirectional, we want to change the slider
            // value if there is no up/down navigation possible.
            if (QApplication::keypadNavigationEnabled()
                    && (QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                    || d->orientation == Qt::Horizontal
                    || !hasEditFocus() && QWidgetPrivate::canKeypadNavigate(Qt::Vertical))) {
                ev->ignore();
                break;
            }
#endif
            action = d->invertedControls ? SliderSingleStepSub : SliderSingleStepAdd;
            break;
        case Qt::Key_Down:
#ifdef QT_KEYPAD_NAVIGATION
            // Same logic as in Qt::Key_Up
            if (QApplication::keypadNavigationEnabled()
                    && (QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder
                    || d->orientation == Qt::Horizontal
                    || !hasEditFocus() && QWidgetPrivate::canKeypadNavigate(Qt::Vertical))) {
                ev->ignore();
                break;
            }
#endif
            action = d->invertedControls ? SliderSingleStepAdd : SliderSingleStepSub;
            break;
        case Qt::Key_PageUp:
            action = d->invertedControls ? SliderPageStepSub : SliderPageStepAdd;
            break;
        case Qt::Key_PageDown:
            action = d->invertedControls ? SliderPageStepAdd : SliderPageStepSub;
            break;
        case Qt::Key_Home:
            action = SliderToMinimum;
            break;
        case Qt::Key_End:
            action = SliderToMaximum;
            break;
        default:
            ev->ignore();
            break;
    }
    if (action)
        triggerAction(action);
}

/*!
    \reimp
*/
void QAbstractSlider::changeEvent(QEvent *ev)
{
    Q_D(QAbstractSlider);
    switch (ev->type()) {
    case QEvent::EnabledChange:
        if (!isEnabled()) {
            d->repeatActionTimer.stop();
            setSliderDown(false);
        }
        Q_FALLTHROUGH();
    default:
        QWidget::changeEvent(ev);
    }
}

/*!
    \reimp
*/
bool QAbstractSlider::event(QEvent *e)
{
#ifdef QT_KEYPAD_NAVIGATION
    Q_D(QAbstractSlider);
    switch (e->type()) {
    case QEvent::FocusIn:
        d->origValue = d->value;
        break;
    default:
        break;
    }
#endif

    return QWidget::event(e);
}

// This function is called from itemviews when doing scroll per pixel (on updateGeometries())
// It will not have any effect if there has been a call to setSingleStep with
// a 'reasonable' value (since viewMayChangeSingleStep will be set to false).
// (If setSingleStep is called with -1 it will however allow the views to change singleStep.)

void QAbstractSliderPrivate::itemviewChangeSingleStep(int step)
{
    singleStepFromItemView = step;
    if (viewMayChangeSingleStep && singleStep != step)
        setSteps(step, pageStep);
}

QT_END_NAMESPACE

#include "moc_qabstractslider.cpp"
