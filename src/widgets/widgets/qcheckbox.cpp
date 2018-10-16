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

#include "qcheckbox.h"
#include "qapplication.h"
#include "qbitmap.h"
#include "qicon.h"
#include "qstylepainter.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qevent.h"
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif

#include "private/qabstractbutton_p.h"

QT_BEGIN_NAMESPACE

class QCheckBoxPrivate : public QAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QCheckBox)
public:
    QCheckBoxPrivate()
        : QAbstractButtonPrivate(QSizePolicy::CheckBox), tristate(false), noChange(false),
          hovering(true), publishedState(Qt::Unchecked) {}

    uint tristate : 1;
    uint noChange : 1;
    uint hovering : 1;
    uint publishedState : 2;

    void init();
};

/*!
    \class QCheckBox
    \brief The QCheckBox widget provides a checkbox with a text label.

    \ingroup basicwidgets
    \inmodule QtWidgets

    \image windows-checkbox.png

    A QCheckBox is an option button that can be switched on (checked) or off
    (unchecked). Checkboxes are typically used to represent features in an
    application that can be enabled or disabled without affecting others.
    Different types of behavior can be implemented. For example, a
    QButtonGroup can be used to group check buttons logically, allowing
    exclusive checkboxes. However, QButtonGroup does not provide any visual
    representation.

    The image below further illustrates the differences between exclusive and
    non-exclusive checkboxes.

    \table
    \row \li \inlineimage checkboxes-exclusive.png
         \li \inlineimage checkboxes-non-exclusive.png
    \endtable

    Whenever a checkbox is checked or cleared, it emits the signal
    stateChanged(). Connect to this signal if you want to trigger an action
    each time the checkbox changes state. You can use isChecked() to query
    whether or not a checkbox is checked.

    In addition to the usual checked and unchecked states, QCheckBox optionally
    provides a third state to indicate "no change". This is useful whenever you
    need to give the user the option of neither checking nor unchecking a
    checkbox. If you need this third state, enable it with setTristate(), and
    use checkState() to query the current toggle state.

    Just like QPushButton, a checkbox displays text, and optionally a small
    icon. The icon is set with setIcon(). The text can be set in the
    constructor or with setText(). A shortcut key can be specified by preceding
    the preferred character with an ampersand. For example:

    \snippet code/src_gui_widgets_qcheckbox.cpp 0

    In this example, the shortcut is \e{Alt+A}. See the \l{QShortcut#mnemonic}
    {QShortcut} documentation for details. To display an actual ampersand,
    use '&&'.

    Important inherited functions: text(), setText(), text(), pixmap(),
    setPixmap(), accel(), setAccel(), isToggleButton(), setDown(), isDown(),
    isOn(), checkState(), autoRepeat(), isExclusiveToggle(), group(),
    setAutoRepeat(), toggle(), pressed(), released(), clicked(), toggled(),
    checkState(), and stateChanged().

    \sa QAbstractButton, QRadioButton, {fowler}{GUI Design Handbook: Check Box}
*/

/*!
    \fn void QCheckBox::stateChanged(int state)

    This signal is emitted whenever the checkbox's state changes, i.e.,
    whenever the user checks or unchecks it.

    \a state contains the checkbox's new Qt::CheckState.
*/

/*!
    \property QCheckBox::tristate
    \brief whether the checkbox is a tri-state checkbox

    The default is false, i.e., the checkbox has only two states.
*/

void QCheckBoxPrivate::init()
{
    Q_Q(QCheckBox);
    q->setCheckable(true);
    q->setMouseTracking(true);
    q->setForegroundRole(QPalette::WindowText);
    q->setAttribute(Qt::WA_MacShowFocusRect);
    setLayoutItemMargins(QStyle::SE_CheckBoxLayoutItem);
}

/*!
    Initializes \a option with the values from this QCheckBox. This method is
    useful for subclasses that require a QStyleOptionButton, but do not want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QCheckBox::initStyleOption(QStyleOptionButton *option) const
{
    if (!option)
        return;
    Q_D(const QCheckBox);
    option->initFrom(this);
    if (d->down)
        option->state |= QStyle::State_Sunken;
    if (d->tristate && d->noChange)
        option->state |= QStyle::State_NoChange;
    else
        option->state |= d->checked ? QStyle::State_On : QStyle::State_Off;
    if (testAttribute(Qt::WA_Hover) && underMouse()) {
        option->state.setFlag(QStyle::State_MouseOver, d->hovering);
    }
    option->text = d->text;
    option->icon = d->icon;
    option->iconSize = iconSize();
}

/*!
    Constructs a checkbox with the given \a parent, but with no text.

    \a parent is passed on to the QAbstractButton constructor.
*/

QCheckBox::QCheckBox(QWidget *parent)
    : QAbstractButton (*new QCheckBoxPrivate, parent)
{
    Q_D(QCheckBox);
    d->init();
}

/*!
    Constructs a checkbox with the given \a parent and \a text.

    \a parent is passed on to the QAbstractButton constructor.
*/

QCheckBox::QCheckBox(const QString &text, QWidget *parent)
    : QCheckBox(parent)
{
    setText(text);
}

/*!
    Destructor.
*/
QCheckBox::~QCheckBox()
{
}

void QCheckBox::setTristate(bool y)
{
    Q_D(QCheckBox);
    d->tristate = y;
}

bool QCheckBox::isTristate() const
{
    Q_D(const QCheckBox);
    return d->tristate;
}


/*!
    Returns the checkbox's check state. If you do not need tristate support,
    you can also use \l QAbstractButton::isChecked(), which returns a boolean.

    \sa setCheckState(), Qt::CheckState
*/
Qt::CheckState QCheckBox::checkState() const
{
    Q_D(const QCheckBox);
    if (d->tristate &&  d->noChange)
        return Qt::PartiallyChecked;
    return d->checked ? Qt::Checked : Qt::Unchecked;
}

/*!
    Sets the checkbox's check state to \a state. If you do not need tristate
    support, you can also use \l QAbstractButton::setChecked(), which takes a
    boolean.

    \sa checkState(), Qt::CheckState
*/
void QCheckBox::setCheckState(Qt::CheckState state)
{
    Q_D(QCheckBox);
#ifndef QT_NO_ACCESSIBILITY
    bool noChange = d->noChange;
#endif
    if (state == Qt::PartiallyChecked) {
        d->tristate = true;
        d->noChange = true;
    } else {
        d->noChange = false;
    }
    d->blockRefresh = true;
    setChecked(state != Qt::Unchecked);
    d->blockRefresh = false;
    d->refresh();
    if ((uint)state != d->publishedState) {
        d->publishedState = state;
        emit stateChanged(state);
    }

#ifndef QT_NO_ACCESSIBILITY
    if (noChange != d->noChange) {
        QAccessible::State s;
        s.checkStateMixed = true;
        QAccessibleStateChangeEvent event(this, s);
        QAccessible::updateAccessibility(&event);
    }
#endif
}


/*!
    \reimp
*/
QSize QCheckBox::sizeHint() const
{
    Q_D(const QCheckBox);
    if (d->sizeHint.isValid())
        return d->sizeHint;
    ensurePolished();
    QFontMetrics fm = fontMetrics();
    QStyleOptionButton opt;
    initStyleOption(&opt);
    QSize sz = style()->itemTextRect(fm, QRect(), Qt::TextShowMnemonic, false,
                                     text()).size();
    if (!opt.icon.isNull())
        sz = QSize(sz.width() + opt.iconSize.width() + 4, qMax(sz.height(), opt.iconSize.height()));
    d->sizeHint = (style()->sizeFromContents(QStyle::CT_CheckBox, &opt, sz, this)
                  .expandedTo(QApplication::globalStrut()));
    return d->sizeHint;
}


/*!
    \reimp
*/
QSize QCheckBox::minimumSizeHint() const
{
    return sizeHint();
}

/*!
    \reimp
*/
void QCheckBox::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionButton opt;
    initStyleOption(&opt);
    p.drawControl(QStyle::CE_CheckBox, opt);
}

/*!
    \reimp
*/
void QCheckBox::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QCheckBox);
    if (testAttribute(Qt::WA_Hover)) {
        bool hit = false;
        if (underMouse())
            hit = hitButton(e->pos());

        if (hit != d->hovering) {
            update(rect());
            d->hovering = hit;
        }
    }

    QAbstractButton::mouseMoveEvent(e);
}


/*!
    \reimp
*/
bool QCheckBox::hitButton(const QPoint &pos) const
{
    QStyleOptionButton opt;
    initStyleOption(&opt);
    return style()->subElementRect(QStyle::SE_CheckBoxClickRect, &opt, this).contains(pos);
}

/*!
    \reimp
*/
void QCheckBox::checkStateSet()
{
    Q_D(QCheckBox);
    d->noChange = false;
    Qt::CheckState state = checkState();
    if ((uint)state != d->publishedState) {
        d->publishedState = state;
        emit stateChanged(state);
    }
}

/*!
    \reimp
*/
void QCheckBox::nextCheckState()
{
    Q_D(QCheckBox);
    if (d->tristate)
        setCheckState((Qt::CheckState)((checkState() + 1) % 3));
    else {
        QAbstractButton::nextCheckState();
        QCheckBox::checkStateSet();
    }
}

/*!
    \reimp
*/
bool QCheckBox::event(QEvent *e)
{
    Q_D(QCheckBox);
    if (e->type() == QEvent::StyleChange
#ifdef Q_OS_MAC
            || e->type() == QEvent::MacSizeChange
#endif
            )
        d->setLayoutItemMargins(QStyle::SE_CheckBoxLayoutItem);
    return QAbstractButton::event(e);
}



QT_END_NAMESPACE

#include "moc_qcheckbox.cpp"
