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

#include "qgroupbox.h"

#include "qapplication.h"
#include "qbitmap.h"
#include "qdrawutil.h"
#include "qevent.h"
#include "qlayout.h"
#if QT_CONFIG(radiobutton)
#include "qradiobutton.h"
#endif
#include "qstyle.h"
#include "qstyleoption.h"
#include "qstylepainter.h"
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif
#include <private/qwidget_p.h>

#include "qdebug.h"

QT_BEGIN_NAMESPACE

class QGroupBoxPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QGroupBox)

public:
    void skip();
    void init();
    void calculateFrame();
    QString title;
    int align;
#ifndef QT_NO_SHORTCUT
    int shortcutId;
#endif

    void _q_fixFocus(Qt::FocusReason reason);
    void _q_setChildrenEnabled(bool b);
    void click();
    bool flat;
    bool checkable;
    bool checked;
    bool hover;
    bool overCheckBox;
    QStyle::SubControl pressedControl;
};

/*!
    Initialize \a option with the values from this QGroupBox. This method
    is useful for subclasses when they need a QStyleOptionGroupBox, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QGroupBox::initStyleOption(QStyleOptionGroupBox *option) const
{
    if (!option)
        return;

    Q_D(const QGroupBox);
    option->initFrom(this);
    option->text = d->title;
    option->lineWidth = 1;
    option->midLineWidth = 0;
    option->textAlignment = Qt::Alignment(d->align);
    option->activeSubControls |= d->pressedControl;
    option->subControls = QStyle::SC_GroupBoxFrame;

    option->state.setFlag(QStyle::State_MouseOver, d->hover);
    if (d->flat)
        option->features |= QStyleOptionFrame::Flat;

    if (d->checkable) {
        option->subControls |= QStyle::SC_GroupBoxCheckBox;
        option->state |= (d->checked ? QStyle::State_On : QStyle::State_Off);
        if ((d->pressedControl == QStyle::SC_GroupBoxCheckBox
            || d->pressedControl == QStyle::SC_GroupBoxLabel) && (d->hover || d->overCheckBox))
            option->state |= QStyle::State_Sunken;
    }

    if (!option->palette.isBrushSet(isEnabled() ? QPalette::Active :
                                    QPalette::Disabled, QPalette::WindowText))
        option->textColor = QColor(style()->styleHint(QStyle::SH_GroupBox_TextLabelColor,
                                   option, this));

    if (!d->title.isEmpty())
        option->subControls |= QStyle::SC_GroupBoxLabel;
}

void QGroupBoxPrivate::click()
{
    Q_Q(QGroupBox);

    QPointer<QGroupBox> guard(q);
    q->setChecked(!checked);
    if (!guard)
        return;
    emit q->clicked(checked);
}

/*!
    \class QGroupBox
    \brief The QGroupBox widget provides a group box frame with a title.

    \ingroup organizers
    \ingroup geomanagement
    \inmodule QtWidgets

    \image windows-groupbox.png

    A group box provides a frame, a title on top, a keyboard shortcut, and
    displays various other widgets inside itself. The keyboard shortcut moves
    keyboard focus to one of the group box's child widgets.

    QGroupBox also lets you set the \l title (normally set in the
    constructor) and the title's \l alignment. Group boxes can be
    \l checkable. Child widgets in checkable group boxes are enabled or
    disabled depending on whether or not the group box is \l checked.

    You can minimize the space consumption of a group box by enabling
    the \l flat property. In most \l{QStyle}{styles}, enabling this
    property results in the removal of the left, right and bottom
    edges of the frame.

    QGroupBox doesn't automatically lay out the child widgets (which
    are often \l{QCheckBox}es or \l{QRadioButton}s but can be any
    widgets). The following example shows how we can set up a
    QGroupBox with a layout:

    \snippet widgets/groupbox/window.cpp 2

    \sa QButtonGroup, {Group Box Example}
*/



/*!
    Constructs a group box widget with the given \a parent but with no title.
*/

QGroupBox::QGroupBox(QWidget *parent)
    : QWidget(*new QGroupBoxPrivate, parent, 0)
{
    Q_D(QGroupBox);
    d->init();
}

/*!
    Constructs a group box with the given \a title and \a parent.
*/

QGroupBox::QGroupBox(const QString &title, QWidget *parent)
    : QGroupBox(parent)
{
    setTitle(title);
}


/*!
    Destroys the group box.
*/
QGroupBox::~QGroupBox()
{
}

void QGroupBoxPrivate::init()
{
    Q_Q(QGroupBox);
    align = Qt::AlignLeft;
#ifndef QT_NO_SHORTCUT
    shortcutId = 0;
#endif
    flat = false;
    checkable = false;
    checked = true;
    hover = false;
    overCheckBox = false;
    pressedControl = QStyle::SC_None;
    calculateFrame();
    q->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred,
                     QSizePolicy::GroupBox));
}

void QGroupBox::setTitle(const QString &title)
{
    Q_D(QGroupBox);
    if (d->title == title)                                // no change
        return;
    d->title = title;
#ifndef QT_NO_SHORTCUT
    releaseShortcut(d->shortcutId);
    d->shortcutId = grabShortcut(QKeySequence::mnemonic(title));
#endif
    d->calculateFrame();

    update();
    updateGeometry();
#ifndef QT_NO_ACCESSIBILITY
    QAccessibleEvent event(this, QAccessible::NameChanged);
    QAccessible::updateAccessibility(&event);
#endif
}

/*!
    \property QGroupBox::title
    \brief the group box title text

    The group box title text will have a keyboard shortcut if the title
    contains an ampersand ('&') followed by a letter.

    \snippet code/src_gui_widgets_qgroupbox.cpp 0

    In the example above, \uicontrol Alt+U moves the keyboard focus to the
    group box. See the \l {QShortcut#mnemonic}{QShortcut}
    documentation for details (to display an actual ampersand, use
    '&&').

    There is no default title text.

    \sa alignment
*/

QString QGroupBox::title() const
{
    Q_D(const QGroupBox);
    return d->title;
}

/*!
    \property QGroupBox::alignment
    \brief the alignment of the group box title.

    Most styles place the title at the top of the frame. The horizontal
    alignment of the title can be specified using single values from
    the following list:

    \list
    \li Qt::AlignLeft aligns the title text with the left-hand side of the group box.
    \li Qt::AlignRight aligns the title text with the right-hand side of the group box.
    \li Qt::AlignHCenter aligns the title text with the horizontal center of the group box.
    \endlist

    The default alignment is Qt::AlignLeft.

    \sa Qt::Alignment
*/
Qt::Alignment QGroupBox::alignment() const
{
    Q_D(const QGroupBox);
    return QFlag(d->align);
}

void QGroupBox::setAlignment(int alignment)
{
    Q_D(QGroupBox);
    d->align = alignment;
    updateGeometry();
    update();
}

/*! \reimp
*/
void QGroupBox::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
}

/*! \reimp
*/

void QGroupBox::paintEvent(QPaintEvent *)
{
    QStylePainter paint(this);
    QStyleOptionGroupBox option;
    initStyleOption(&option);
    paint.drawComplexControl(QStyle::CC_GroupBox, option);
}

/*! \reimp  */
bool QGroupBox::event(QEvent *e)
{
    Q_D(QGroupBox);
#ifndef QT_NO_SHORTCUT
    if (e->type() == QEvent::Shortcut) {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        if (se->shortcutId() == d->shortcutId) {
            if (!isCheckable()) {
                d->_q_fixFocus(Qt::ShortcutFocusReason);
            } else {
                d->click();
                setFocus(Qt::ShortcutFocusReason);
            }
            return true;
        }
    }
#endif
    QStyleOptionGroupBox box;
    initStyleOption(&box);
    switch (e->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverMove: {
        QStyle::SubControl control = style()->hitTestComplexControl(QStyle::CC_GroupBox, &box,
                                                                    static_cast<QHoverEvent *>(e)->pos(),
                                                                    this);
        bool oldHover = d->hover;
        d->hover = d->checkable && (control == QStyle::SC_GroupBoxLabel || control == QStyle::SC_GroupBoxCheckBox);
        if (oldHover != d->hover) {
            QRect rect = style()->subControlRect(QStyle::CC_GroupBox, &box, QStyle::SC_GroupBoxCheckBox, this)
                         | style()->subControlRect(QStyle::CC_GroupBox, &box, QStyle::SC_GroupBoxLabel, this);
            update(rect);
        }
        return true;
    }
    case QEvent::HoverLeave:
        d->hover = false;
        if (d->checkable) {
            QRect rect = style()->subControlRect(QStyle::CC_GroupBox, &box, QStyle::SC_GroupBoxCheckBox, this)
                         | style()->subControlRect(QStyle::CC_GroupBox, &box, QStyle::SC_GroupBoxLabel, this);
            update(rect);
        }
        return true;
    case QEvent::KeyPress: {
        QKeyEvent *k = static_cast<QKeyEvent*>(e);
        if (!k->isAutoRepeat() && (k->key() == Qt::Key_Select || k->key() == Qt::Key_Space)) {
            d->pressedControl = QStyle::SC_GroupBoxCheckBox;
            update(style()->subControlRect(QStyle::CC_GroupBox, &box, QStyle::SC_GroupBoxCheckBox, this));
            return true;
        }
        break;
    }
    case QEvent::KeyRelease: {
        QKeyEvent *k = static_cast<QKeyEvent*>(e);
        if (!k->isAutoRepeat() && (k->key() == Qt::Key_Select || k->key() == Qt::Key_Space)) {
            bool toggle = (d->pressedControl == QStyle::SC_GroupBoxLabel
                           || d->pressedControl == QStyle::SC_GroupBoxCheckBox);
            d->pressedControl = QStyle::SC_None;
            if (toggle)
                d->click();
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QWidget::event(e);
}

/*!\reimp */
void QGroupBox::childEvent(QChildEvent *c)
{
    Q_D(QGroupBox);
    if (c->type() != QEvent::ChildAdded || !c->child()->isWidgetType())
        return;
    QWidget *w = (QWidget*)c->child();
    if (w->isWindow())
        return;
    if (d->checkable) {
        if (d->checked) {
            if (!w->testAttribute(Qt::WA_ForceDisabled))
                w->setEnabled(true);
        } else {
            if (w->isEnabled()) {
                w->setEnabled(false);
                w->setAttribute(Qt::WA_ForceDisabled, false);
            }
        }
    }
}


/*!
    \internal

    This private slot finds a widget in this group box that can accept
    focus, and gives the focus to that widget.
*/

void QGroupBoxPrivate::_q_fixFocus(Qt::FocusReason reason)
{
    Q_Q(QGroupBox);
    QWidget *fw = q->focusWidget();
    if (!fw || fw == q) {
        QWidget * best = 0;
        QWidget * candidate = 0;
        QWidget * w = q;
        while ((w = w->nextInFocusChain()) != q) {
            if (q->isAncestorOf(w) && (w->focusPolicy() & Qt::TabFocus) == Qt::TabFocus && w->isVisibleTo(q)) {
#if QT_CONFIG(radiobutton)
                if (!best && qobject_cast<QRadioButton*>(w) && ((QRadioButton*)w)->isChecked())
                    // we prefer a checked radio button or a widget that
                    // already has focus, if there is one
                    best = w;
                else
#endif
                    if (!candidate)
                        // but we'll accept anything that takes focus
                        candidate = w;
            }
        }
        if (best)
            fw = best;
        else if (candidate)
                fw = candidate;
    }
    if (fw)
        fw->setFocus(reason);
}

/*
    Sets the right frame rect depending on the title.
*/
void QGroupBoxPrivate::calculateFrame()
{
    Q_Q(QGroupBox);
    QStyleOptionGroupBox box;
    q->initStyleOption(&box);
    QRect contentsRect = q->style()->subControlRect(QStyle::CC_GroupBox, &box, QStyle::SC_GroupBoxContents, q);
    q->setContentsMargins(contentsRect.left() - box.rect.left(), contentsRect.top() - box.rect.top(),
                          box.rect.right() - contentsRect.right(), box.rect.bottom() - contentsRect.bottom());
    setLayoutItemMargins(QStyle::SE_GroupBoxLayoutItem, &box);
}

/*! \reimp
 */
void QGroupBox::focusInEvent(QFocusEvent *fe)
{ // note no call to super
    Q_D(QGroupBox);
    if (focusPolicy() == Qt::NoFocus) {
        d->_q_fixFocus(fe->reason());
    } else {
        QWidget::focusInEvent(fe);
    }
}


/*!
  \reimp
*/
QSize QGroupBox::minimumSizeHint() const
{
    Q_D(const QGroupBox);
    QStyleOptionGroupBox option;
    initStyleOption(&option);

    QFontMetrics metrics(fontMetrics());

    int baseWidth = metrics.horizontalAdvance(d->title) + metrics.horizontalAdvance(QLatin1Char(' '));
    int baseHeight = metrics.height();
    if (d->checkable) {
        baseWidth += style()->pixelMetric(QStyle::PM_IndicatorWidth);
        baseWidth += style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing);
        baseHeight = qMax(baseHeight, style()->pixelMetric(QStyle::PM_IndicatorHeight));
    }

    QSize size = style()->sizeFromContents(QStyle::CT_GroupBox, &option, QSize(baseWidth, baseHeight), this);
    return size.expandedTo(QWidget::minimumSizeHint());
}

/*!
    \property QGroupBox::flat
    \brief whether the group box is painted flat or has a frame

    A group box usually consists of a surrounding frame with a title
    at the top. If this property is enabled, only the top part of the frame is
    drawn in most styles; otherwise, the whole frame is drawn.

    By default, this property is disabled, i.e., group boxes are not flat unless
    explicitly specified.

    \b{Note:} In some styles, flat and non-flat group boxes have similar
    representations and may not be as distinguishable as they are in other
    styles.

    \sa title
*/
bool QGroupBox::isFlat() const
{
    Q_D(const QGroupBox);
    return d->flat;
}

void QGroupBox::setFlat(bool b)
{
    Q_D(QGroupBox);
    if (d->flat == b)
        return;
    d->flat = b;
    updateGeometry();
    update();
}


/*!
    \property QGroupBox::checkable
    \brief whether the group box has a checkbox in its title

    If this property is \c true, the group box displays its title using
    a checkbox in place of an ordinary label. If the checkbox is checked,
    the group box's children are enabled; otherwise, they are disabled and
    inaccessible.

    By default, group boxes are not checkable.

    If this property is enabled for a group box, it will also be initially
    checked to ensure that its contents are enabled.

    \sa checked
*/
void QGroupBox::setCheckable(bool checkable)
{
    Q_D(QGroupBox);

    bool wasCheckable = d->checkable;
    d->checkable = checkable;

    if (checkable) {
        setChecked(true);
        if (!wasCheckable) {
            setFocusPolicy(Qt::StrongFocus);
            d->_q_setChildrenEnabled(true);
            updateGeometry();
        }
    } else {
        if (wasCheckable) {
            setFocusPolicy(Qt::NoFocus);
            d->_q_setChildrenEnabled(true);
            updateGeometry();
        }
        d->_q_setChildrenEnabled(true);
    }

    if (wasCheckable != checkable) {
        d->calculateFrame();
        update();
    }
}

bool QGroupBox::isCheckable() const
{
    Q_D(const QGroupBox);
    return d->checkable;
}


bool QGroupBox::isChecked() const
{
    Q_D(const QGroupBox);
    return d->checkable && d->checked;
}


/*!
    \fn void QGroupBox::toggled(bool on)

    If the group box is checkable, this signal is emitted when the check box
    is toggled. \a on is true if the check box is checked; otherwise, it is false.

    \sa checkable
*/


/*!
    \fn void QGroupBox::clicked(bool checked)
    \since 4.2

    This signal is emitted when the check box is activated (i.e., pressed down
    then released while the mouse cursor is inside the button), or when the
    shortcut key is typed. Notably, this signal is \e not emitted if you call
    setChecked().

    If the check box is checked, \a checked is true; it is false if the check
    box is unchecked.

    \sa checkable, toggled(), checked
*/

/*!
    \property QGroupBox::checked
    \brief whether the group box is checked

    If the group box is checkable, it is displayed with a check box.
    If the check box is checked, the group box's children are enabled;
    otherwise, the children are disabled and are inaccessible to the user.

    By default, checkable group boxes are also checked.

    \sa checkable
*/
void QGroupBox::setChecked(bool b)
{
    Q_D(QGroupBox);
    if (d->checkable && b != d->checked) {
        update();
        d->checked = b;
        d->_q_setChildrenEnabled(b);
#ifndef QT_NO_ACCESSIBILITY
        QAccessible::State st;
        st.checked = true;
        QAccessibleStateChangeEvent e(this, st);
        QAccessible::updateAccessibility(&e);
#endif
        emit toggled(b);
    }
}

/*
  sets all children of the group box except the qt_groupbox_checkbox
  to either disabled/enabled
*/
void QGroupBoxPrivate::_q_setChildrenEnabled(bool b)
{
    Q_Q(QGroupBox);
    for (QObject *o : q->children()) {
        if (o->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(o);
            if (b) {
                if (!w->testAttribute(Qt::WA_ForceDisabled))
                    w->setEnabled(true);
            } else {
                if (w->isEnabled()) {
                    w->setEnabled(false);
                    w->setAttribute(Qt::WA_ForceDisabled, false);
                }
            }
        }
    }
}

/*! \reimp */
void QGroupBox::changeEvent(QEvent *ev)
{
    Q_D(QGroupBox);
    if (ev->type() == QEvent::EnabledChange) {
        if (d->checkable && isEnabled()) {
            // we are being enabled - disable children
            if (!d->checked)
                d->_q_setChildrenEnabled(false);
        }
    } else if (ev->type() == QEvent::FontChange
#ifdef Q_OS_MAC
               || ev->type() == QEvent::MacSizeChange
#endif
               || ev->type() == QEvent::StyleChange) {
        d->calculateFrame();
    }
    QWidget::changeEvent(ev);
}

/*! \reimp */
void QGroupBox::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    Q_D(QGroupBox);
    QStyleOptionGroupBox box;
    initStyleOption(&box);
    d->pressedControl = style()->hitTestComplexControl(QStyle::CC_GroupBox, &box,
                                                       event->pos(), this);
    if (d->checkable && (d->pressedControl & (QStyle::SC_GroupBoxCheckBox | QStyle::SC_GroupBoxLabel))) {
        d->overCheckBox = true;
        update(style()->subControlRect(QStyle::CC_GroupBox, &box, QStyle::SC_GroupBoxCheckBox, this));
    } else {
        event->ignore();
    }
}

/*! \reimp */
void QGroupBox::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QGroupBox);
    QStyleOptionGroupBox box;
    initStyleOption(&box);
    QStyle::SubControl pressed = style()->hitTestComplexControl(QStyle::CC_GroupBox, &box,
                                                                event->pos(), this);
    bool oldOverCheckBox = d->overCheckBox;
    d->overCheckBox = (pressed == QStyle::SC_GroupBoxCheckBox || pressed == QStyle::SC_GroupBoxLabel);
    if (d->checkable && (d->pressedControl == QStyle::SC_GroupBoxCheckBox || d->pressedControl == QStyle::SC_GroupBoxLabel)
        && (d->overCheckBox != oldOverCheckBox))
        update(style()->subControlRect(QStyle::CC_GroupBox, &box, QStyle::SC_GroupBoxCheckBox, this));

    event->ignore();
}

/*! \reimp */
void QGroupBox::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    Q_D(QGroupBox);
    if (!d->overCheckBox) {
        event->ignore();
        return;
    }
    QStyleOptionGroupBox box;
    initStyleOption(&box);
    QStyle::SubControl released = style()->hitTestComplexControl(QStyle::CC_GroupBox, &box,
                                                                 event->pos(), this);
    bool toggle = d->checkable && (released == QStyle::SC_GroupBoxLabel
                                   || released == QStyle::SC_GroupBoxCheckBox);
    d->pressedControl = QStyle::SC_None;
    d->overCheckBox = false;
    if (toggle)
        d->click();
    else if (d->checkable)
        update(style()->subControlRect(QStyle::CC_GroupBox, &box, QStyle::SC_GroupBoxCheckBox, this));
}

QT_END_NAMESPACE

#include "moc_qgroupbox.cpp"
