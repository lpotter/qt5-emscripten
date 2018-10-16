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

#include "qglobal.h"

#include "qgraphicswidget.h"
#include "qgraphicswidget_p.h"
#include "qgraphicslayout.h"
#include "qgraphicslayout_p.h"
#include "qgraphicsscene.h"
#include "qgraphicssceneevent.h"

#ifndef QT_NO_ACTION
#include <private/qaction_p.h>
#endif
#include <private/qapplication_p.h>
#include <private/qgraphicsscene_p.h>
#ifndef QT_NO_SHORTCUT
#include <private/qshortcutmap_p.h>
#endif
#include <QtCore/qmutex.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qgraphicsview.h>
#include <QtWidgets/qgraphicsproxywidget.h>
#include <QtGui/qpalette.h>
#include <QtWidgets/qstyleoption.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \class QGraphicsWidget
    \brief The QGraphicsWidget class is the base class for all widget
    items in a QGraphicsScene.
    \since 4.4
    \ingroup graphicsview-api
    \inmodule QtWidgets

    QGraphicsWidget is an extended base item that provides extra functionality
    over QGraphicsItem. It is similar to QWidget in many ways:

    \list
        \li Provides a \l palette, a \l font and a \l style().
        \li Has a defined geometry().
        \li Supports layouts with setLayout() and layout().
        \li Supports shortcuts and actions with grabShortcut() and insertAction()
    \endlist

    Unlike QGraphicsItem, QGraphicsWidget is not an abstract class; you can
    create instances of a QGraphicsWidget without having to subclass it.
    This approach is useful for widgets that only serve the purpose of
    organizing child widgets into a layout.

    QGraphicsWidget can be used as a base item for your own custom item if
    you require advanced input focus handling, e.g., tab focus and activation, or
    layouts.

    Since QGraphicsWidget resembles QWidget and has similar API, it is
    easier to port a widget from QWidget to QGraphicsWidget, instead of
    QGraphicsItem.

    \note QWidget-based widgets can be directly embedded into a
    QGraphicsScene using QGraphicsProxyWidget.

    Noticeable differences between QGraphicsWidget and QWidget are:

    \table
    \header \li QGraphicsWidget
                \li QWidget
    \row      \li Coordinates and geometry are defined with qreals (doubles or
                    floats, depending on the platform).
                \li QWidget uses integer geometry (QPoint, QRect).
    \row      \li The widget is already visible by default; you do not have to
                    call show() to display the widget.
                \li QWidget is hidden by default until you call show().
    \row      \li A subset of widget attributes are supported.
                \li All widget attributes are supported.
    \row      \li A top-level item's style defaults to QGraphicsScene::style
                \li A top-level widget's style defaults to QApplication::style
    \row      \li Graphics View provides a custom drag and drop framework, different
                    from QWidget.
                \li Standard drag and drop framework.
    \row      \li Widget items do not support modality.
                \li Full modality support.
    \endtable

    QGraphicsWidget supports a subset of Qt's widget attributes,
    (Qt::WidgetAttribute), as shown in the table below. Any attributes not
    listed in this table are unsupported, or otherwise unused.

    \table
    \header \li Widget Attribute                         \li Usage
    \row    \li Qt::WA_SetLayoutDirection
                    \li Set by setLayoutDirection(), cleared by
                        unsetLayoutDirection(). You can test this attribute to
                        check if the widget has been explicitly assigned a
                        \l{QGraphicsWidget::layoutDirection()}
                        {layoutDirection}. If the attribute is not set, the
                        \l{QGraphicsWidget::layoutDirection()}
                        {layoutDirection()} is inherited.
    \row    \li Qt::WA_RightToLeft
                    \li Toggled by setLayoutDirection(). Inherited from the
                        parent/scene. If set, the widget's layout will order
                        horizontally arranged widgets from right to left.
    \row    \li Qt::WA_SetStyle
                    \li Set and cleared by setStyle(). If this attribute is
                        set, the widget has been explicitly assigned a style.
                        If it is unset, the widget will use the scene's or the
                        application's style.
    \row    \li Qt::WA_Resized
                    \li Set by setGeometry() and resize().
    \row    \li Qt::WA_SetPalette
                    \li Set by setPalette().
    \row    \li Qt::WA_SetFont
                    \li Set by setFont().
    \row    \li Qt::WA_WindowPropagation
                    \li Enables propagation to window widgets.
    \endtable

    Although QGraphicsWidget inherits from both QObject and QGraphicsItem,
    you should use the functions provided by QGraphicsItem, \e not QObject, to
    manage the relationships between parent and child items. These functions
    control the stacking order of items as well as their ownership.

    \note The QObject::parent() should always return 0 for QGraphicsWidgets,
    but this policy is not strictly defined.

    \sa QGraphicsProxyWidget, QGraphicsItem, {Widgets and Layouts}
*/

/*!
    Constructs a QGraphicsWidget instance. The optional \a parent argument is
    passed to QGraphicsItem's constructor. The optional \a wFlags argument
    specifies the widget's window flags (e.g., whether the widget should be a
    window, a tool, a popup, etc).
*/
QGraphicsWidget::QGraphicsWidget(QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QGraphicsObject(*new QGraphicsWidgetPrivate, 0), QGraphicsLayoutItem(0, false)
{
    Q_D(QGraphicsWidget);
    d->init(parent, wFlags);
}

/*!
    \internal

    Constructs a new QGraphicsWidget, using \a dd as parent.
*/
QGraphicsWidget::QGraphicsWidget(QGraphicsWidgetPrivate &dd, QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QGraphicsObject(dd, 0), QGraphicsLayoutItem(0, false)
{
    Q_D(QGraphicsWidget);
    d->init(parent, wFlags);
}

/*
    \internal
    \class QGraphicsWidgetStyles

    We use this thread-safe class to maintain a hash of styles for widgets
    styles. Note that QApplication::style() itself isn't thread-safe, QStyle
    isn't thread-safe, and we don't have a thread-safe factory for creating
    the default style, nor cloning a style.
*/
class QGraphicsWidgetStyles
{
public:
    QStyle *styleForWidget(const QGraphicsWidget *widget) const
    {
        QMutexLocker locker(&mutex);
        return styles.value(widget, 0);
    }

    void setStyleForWidget(QGraphicsWidget *widget, QStyle *style)
    {
        QMutexLocker locker(&mutex);
        if (style)
            styles[widget] = style;
        else
            styles.remove(widget);
    }

private:
    QHash<const QGraphicsWidget *, QStyle *> styles;
    mutable QMutex mutex;
};
Q_GLOBAL_STATIC(QGraphicsWidgetStyles, widgetStyles)

/*!
    Destroys the QGraphicsWidget instance.
*/
QGraphicsWidget::~QGraphicsWidget()
{
    Q_D(QGraphicsWidget);
#ifndef QT_NO_ACTION
    // Remove all actions from this widget
    for (int i = 0; i < d->actions.size(); ++i) {
        QActionPrivate *apriv = d->actions.at(i)->d_func();
        apriv->graphicsWidgets.removeAll(this);
    }
    d->actions.clear();
#endif

    if (QGraphicsScene *scn = scene()) {
        QGraphicsScenePrivate *sceneD = scn->d_func();
        if (sceneD->tabFocusFirst == this)
            sceneD->tabFocusFirst = (d->focusNext == this ? 0 : d->focusNext);
    }
    d->focusPrev->d_func()->focusNext = d->focusNext;
    d->focusNext->d_func()->focusPrev = d->focusPrev;

    // Play it really safe
    d->focusNext = this;
    d->focusPrev = this;

    clearFocus();

    //we check if we have a layout previously
    if (d->layout) {
        QGraphicsLayout *temp = d->layout;
        const auto items = childItems();
        for (QGraphicsItem *item : items) {
            // In case of a custom layout which doesn't remove and delete items, we ensure that
            // the parent layout item does not point to the deleted layout. This code is here to
            // avoid regression from 4.4 to 4.5, because according to 4.5 docs it is not really needed.
            if (item->isWidget()) {
                QGraphicsWidget *widget = static_cast<QGraphicsWidget *>(item);
                if (widget->parentLayoutItem() == d->layout)
                    widget->setParentLayoutItem(0);
            }
        }
        d->layout = 0;
        delete temp;
    }

    // Remove this graphics widget from widgetStyles
    widgetStyles()->setStyleForWidget(this, 0);

    // Unset the parent here, when we're still a QGraphicsWidget.
    // It is otherwise done in ~QGraphicsItem() where we'd be
    // calling QGraphicsWidget members on an ex-QGraphicsWidget object
    setParentItem(nullptr);
}

/*!
    \property QGraphicsWidget::size
    \brief the size of the widget

    Calling resize() resizes the widget to a \a size bounded by minimumSize()
    and maximumSize(). This property only affects the widget's width and
    height (e.g., its right and bottom edges); the widget's position and
    top-left corner remains unaffected.

    Resizing a widget triggers the widget to immediately receive a
    \l{QEvent::GraphicsSceneResize}{GraphicsSceneResize} event with the
    widget's old and new size.  If the widget has a layout assigned when this
    event arrives, the layout will be activated and it will automatically
    update any child widgets's geometry.

    This property does not affect any layout of the parent widget. If the
    widget itself is managed by a parent layout; e.g., it has a parent widget
    with a layout assigned, that layout will not activate.

    By default, this property contains a size with zero width and height.

    \sa setGeometry(), QGraphicsSceneResizeEvent, QGraphicsLayout
*/
QSizeF QGraphicsWidget::size() const
{
    return QGraphicsLayoutItem::geometry().size();
}

void QGraphicsWidget::resize(const QSizeF &size)
{
    setGeometry(QRectF(pos(), size));
}

/*!
    \fn void QGraphicsWidget::resize(qreal w, qreal h)
    \overload

    Constructs a resize with the given \c width (\a w) and \c height (\a h).
    This convenience function is equivalent to calling resize(QSizeF(w, h)).

    \sa setGeometry(), setTransform()
*/

/*!
    \property QGraphicsWidget::sizePolicy
    \brief the size policy for the widget
    \sa sizePolicy(), setSizePolicy(), QWidget::sizePolicy()
*/

/*!
  \fn QGraphicsWidget::geometryChanged()

  This signal gets emitted whenever the geometry is changed in setGeometry().
*/

/*!
    \property QGraphicsWidget::geometry
    \brief the geometry of the widget

    Sets the item's geometry to \a rect. The item's position and size are
    modified as a result of calling this function. The item is first moved,
    then resized.

    A side effect of calling this function is that the widget will receive
    a move event and a resize event. Also, if the widget has a layout
    assigned, the layout will activate.

    \sa geometry(), resize()
*/
void QGraphicsWidget::setGeometry(const QRectF &rect)
{
    QGraphicsWidgetPrivate *wd = QGraphicsWidget::d_func();
    QGraphicsLayoutItemPrivate *d = QGraphicsLayoutItem::d_ptr.data();
    QRectF newGeom;
    QPointF oldPos = d->geom.topLeft();
    if (!wd->inSetPos) {
        setAttribute(Qt::WA_Resized);
        newGeom = rect;
        newGeom.setSize(rect.size().expandedTo(effectiveSizeHint(Qt::MinimumSize))
                                   .boundedTo(effectiveSizeHint(Qt::MaximumSize)));

        if (newGeom == d->geom) {
            goto relayoutChildrenAndReturn;
        }

        // setPos triggers ItemPositionChange, which can adjust position
        wd->inSetGeometry = 1;
        setPos(newGeom.topLeft());
        wd->inSetGeometry = 0;
        newGeom.moveTopLeft(pos());

        if (newGeom == d->geom) {
            goto relayoutChildrenAndReturn;
        }

         // Update and prepare to change the geometry (remove from index) if the size has changed.
        if (wd->scene) {
            if (rect.topLeft() == d->geom.topLeft()) {
                prepareGeometryChange();
            }
        }
    }

    // Update the layout item geometry
    {
        bool moved = oldPos != pos();
        if (moved) {
            // Send move event.
            QGraphicsSceneMoveEvent event;
            event.setOldPos(oldPos);
            event.setNewPos(pos());
            QApplication::sendEvent(this, &event);
            if (wd->inSetPos) {
                //set the new pos
                d->geom.moveTopLeft(pos());
                emit geometryChanged();
                goto relayoutChildrenAndReturn;
            }
        }
        QSizeF oldSize = size();
        QGraphicsLayoutItem::setGeometry(newGeom);
        // Send resize event
        bool resized = newGeom.size() != oldSize;
        if (resized) {
            QGraphicsSceneResizeEvent re;
            re.setOldSize(oldSize);
            re.setNewSize(newGeom.size());
            if (oldSize.width() != newGeom.size().width())
                emit widthChanged();
            if (oldSize.height() != newGeom.size().height())
                emit heightChanged();
            QGraphicsLayout *lay = wd->layout;
            if (QGraphicsLayout::instantInvalidatePropagation()) {
                if (!lay || lay->isActivated()) {
                    QApplication::sendEvent(this, &re);
                }
            } else {
                QApplication::sendEvent(this, &re);
            }
        }
    }

    emit geometryChanged();
relayoutChildrenAndReturn:
    if (QGraphicsLayout::instantInvalidatePropagation()) {
        if (QGraphicsLayout *lay = wd->layout) {
            if (!lay->isActivated()) {
                QEvent layoutRequest(QEvent::LayoutRequest);
                QApplication::sendEvent(this, &layoutRequest);
            }
        }
    }
}

/*!
    \fn QRectF QGraphicsWidget::rect() const

    Returns the item's local rect as a QRectF. This function is equivalent
    to QRectF(QPointF(), size()).

    \sa setGeometry(), resize()
*/

/*!
    \fn void QGraphicsWidget::setGeometry(qreal x, qreal y, qreal w, qreal h)

    This convenience function is equivalent to calling setGeometry(QRectF(
    \a x, \a y, \a w, \a h)).

    \sa geometry(), resize()
*/

/*!
    \property QGraphicsWidget::minimumSize
    \brief the minimum size of the widget

    \sa setMinimumSize(), minimumSize(), preferredSize, maximumSize
*/

/*!
    \property QGraphicsWidget::preferredSize
    \brief the preferred size of the widget

    \sa setPreferredSize(), preferredSize(), minimumSize, maximumSize
*/

/*!
    \property QGraphicsWidget::maximumSize
    \brief the maximum size of the widget

    \sa setMaximumSize(), maximumSize(), minimumSize, preferredSize
*/

/*!
    Sets the widget's contents margins to \a left, \a top, \a right and \a
    bottom.

    Contents margins are used by the assigned layout to define the placement
    of subwidgets and layouts. Margins are particularly useful for widgets
    that constrain subwidgets to only a section of its own geometry. For
    example, a group box with a layout will place subwidgets inside its frame,
    but below the title.

    Changing a widget's contents margins will always trigger an update(), and
    any assigned layout will be activated automatically. The widget will then
    receive a \l{QEvent::ContentsRectChange}{ContentsRectChange} event.

    \sa getContentsMargins(), setGeometry()
*/
void QGraphicsWidget::setContentsMargins(qreal left, qreal top, qreal right, qreal bottom)
{
    Q_D(QGraphicsWidget);

    if (!d->margins && left == 0 && top == 0 && right == 0 && bottom == 0)
        return;
    d->ensureMargins();
    if (left == d->margins[d->Left]
        && top == d->margins[d->Top]
        && right == d->margins[d->Right]
        && bottom == d->margins[d->Bottom])
        return;

    d->margins[d->Left] = left;
    d->margins[d->Top] = top;
    d->margins[d->Right] = right;
    d->margins[d->Bottom] = bottom;

    if (QGraphicsLayout *l = d->layout)
        l->invalidate();
    else
        updateGeometry();

    QEvent e(QEvent::ContentsRectChange);
    QApplication::sendEvent(this, &e);
}

/*!
    Gets the widget's contents margins. The margins are stored in \a left, \a
    top, \a right and \a bottom, as pointers to qreals. Each argument can
    be \e {omitted} by passing 0.

    \sa setContentsMargins()
*/
void QGraphicsWidget::getContentsMargins(qreal *left, qreal *top, qreal *right, qreal *bottom) const
{
    Q_D(const QGraphicsWidget);
    if (left || top || right || bottom)
        d->ensureMargins();
    if (left)
        *left = d->margins[d->Left];
    if (top)
        *top = d->margins[d->Top];
    if (right)
        *right = d->margins[d->Right];
    if (bottom)
        *bottom = d->margins[d->Bottom];
}

/*!
    Sets the widget's window frame margins to \a left, \a top, \a right and
    \a bottom. The default frame margins are provided by the style, and they
    depend on the current window flags.

    If you would like to draw your own window decoration, you can set your
    own frame margins to override the default margins.

    \sa unsetWindowFrameMargins(), getWindowFrameMargins(), windowFrameRect()
*/
void QGraphicsWidget::setWindowFrameMargins(qreal left, qreal top, qreal right, qreal bottom)
{
    Q_D(QGraphicsWidget);

    if (!d->windowFrameMargins && left == 0 && top == 0 && right == 0 && bottom == 0)
        return;
    d->ensureWindowFrameMargins();
    bool unchanged =
        d->windowFrameMargins[d->Left] == left
        && d->windowFrameMargins[d->Top] == top
        && d->windowFrameMargins[d->Right] == right
        && d->windowFrameMargins[d->Bottom] == bottom;
    if (d->setWindowFrameMargins && unchanged)
        return;
    if (!unchanged)
        prepareGeometryChange();
    d->windowFrameMargins[d->Left] = left;
    d->windowFrameMargins[d->Top] = top;
    d->windowFrameMargins[d->Right] = right;
    d->windowFrameMargins[d->Bottom] = bottom;
    d->setWindowFrameMargins = true;
}

/*!
    Gets the widget's window frame margins. The margins are stored in \a left,
    \a top, \a right and \a bottom as pointers to qreals. Each argument can
    be \e {omitted} by passing 0.

    \sa setWindowFrameMargins(), windowFrameRect()
*/
void QGraphicsWidget::getWindowFrameMargins(qreal *left, qreal *top, qreal *right, qreal *bottom) const
{
    Q_D(const QGraphicsWidget);
    if (left || top || right || bottom)
        d->ensureWindowFrameMargins();
    if (left)
        *left = d->windowFrameMargins[d->Left];
    if (top)
        *top = d->windowFrameMargins[d->Top];
    if (right)
        *right = d->windowFrameMargins[d->Right];
    if (bottom)
        *bottom = d->windowFrameMargins[d->Bottom];
}

/*!
    Resets the window frame margins to the default value, provided by the style.

    \sa setWindowFrameMargins(), getWindowFrameMargins(), windowFrameRect()
*/
void QGraphicsWidget::unsetWindowFrameMargins()
{
    Q_D(QGraphicsWidget);
    if ((d->windowFlags & Qt::Window) && (d->windowFlags & Qt::WindowType_Mask) != Qt::Popup &&
         (d->windowFlags & Qt::WindowType_Mask) != Qt::ToolTip && !(d->windowFlags & Qt::FramelessWindowHint)) {
        QStyleOptionTitleBar bar;
        d->initStyleOptionTitleBar(&bar);
        QStyle *style = this->style();
        qreal margin = style->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth);
        qreal titleBarHeight  = d->titleBarHeight(bar);
        setWindowFrameMargins(margin, titleBarHeight, margin, margin);
    } else {
        setWindowFrameMargins(0, 0, 0, 0);
    }
    d->setWindowFrameMargins = false;
}

/*!
    Returns the widget's geometry in parent coordinates including any window
    frame.

    \sa windowFrameRect(), getWindowFrameMargins(), setWindowFrameMargins()
*/
QRectF QGraphicsWidget::windowFrameGeometry() const
{
    Q_D(const QGraphicsWidget);
    return d->windowFrameMargins
        ? geometry().adjusted(-d->windowFrameMargins[d->Left], -d->windowFrameMargins[d->Top],
                              d->windowFrameMargins[d->Right], d->windowFrameMargins[d->Bottom])
        : geometry();
}

/*!
    Returns the widget's local rect including any window frame.

    \sa windowFrameGeometry(), getWindowFrameMargins(), setWindowFrameMargins()
*/
QRectF QGraphicsWidget::windowFrameRect() const
{
    Q_D(const QGraphicsWidget);
    return d->windowFrameMargins
        ? rect().adjusted(-d->windowFrameMargins[d->Left], -d->windowFrameMargins[d->Top],
                          d->windowFrameMargins[d->Right], d->windowFrameMargins[d->Bottom])
        : rect();
}

/*!
    Populates a style option object for this widget based on its current
    state, and stores the output in \a option. The default implementation
    populates \a option with the following properties.

    \table
      \header
        \li Style Option Property
        \li Value
      \row
        \li state & QStyle::State_Enabled
        \li Corresponds to QGraphicsItem::isEnabled().
      \row
        \li state & QStyle::State_HasFocus
        \li Corresponds to QGraphicsItem::hasFocus().
      \row
        \li state & QStyle::State_MouseOver
        \li Corresponds to QGraphicsItem::isUnderMouse().
      \row
        \li direction
        \li Corresponds to QGraphicsWidget::layoutDirection().
      \row
        \li rect
        \li Corresponds to QGraphicsWidget::rect().toRect().
      \row
        \li palette
        \li Corresponds to QGraphicsWidget::palette().
      \row
        \li fontMetrics
        \li Corresponds to QFontMetrics(QGraphicsWidget::font()).
    \endtable

    Subclasses of QGraphicsWidget should call the base implementation, and
    then test the type of \a option using qstyleoption_cast<>() or test
    QStyleOption::Type before storing widget-specific options.

    For example:

    \snippet code/src_gui_graphicsview_qgraphicswidget.cpp 0

    \sa QStyleOption::initFrom()
*/
void QGraphicsWidget::initStyleOption(QStyleOption *option) const
{
    Q_ASSERT(option);

    option->state = QStyle::State_None;
    if (isEnabled())
        option->state |= QStyle::State_Enabled;
    if (hasFocus())
        option->state |= QStyle::State_HasFocus;
    // if (window->testAttribute(Qt::WA_KeyboardFocusChange)) // ### Window
    //     option->state |= QStyle::State_KeyboardFocusChange;
    if (isUnderMouse())
        option->state |= QStyle::State_MouseOver;
    if (QGraphicsWidget *w = window()) {
        if (w->isActiveWindow())
            option->state |= QStyle::State_Active;
    }
    if (isWindow())
        option->state |= QStyle::State_Window;
    /*
      ###
#if 0 // Used to be included in Qt4 for Q_WS_MAC
    extern bool qt_mac_can_clickThrough(const QGraphicsWidget *w); //qwidget_mac.cpp
    if (!(option->state & QStyle::State_Active) && !qt_mac_can_clickThrough(widget))
        option->state &= ~QStyle::State_Enabled;

    switch (QMacStyle::widgetSizePolicy(widget)) {
    case QMacStyle::SizeSmall:
        option->state |= QStyle::State_Small;
        break;
    case QMacStyle::SizeMini:
        option->state |= QStyle::State_Mini;
        break;
    default:
        ;
    }
#endif
#ifdef QT_KEYPAD_NAVIGATION
    if (widget->hasEditFocus())
        state |= QStyle::State_HasEditFocus;
#endif
    */
    option->direction = layoutDirection();
    option->rect = rect().toRect(); // ### truncation!
    option->palette = palette();
    if (!isEnabled()) {
        option->palette.setCurrentColorGroup(QPalette::Disabled);
    } else if (isActiveWindow()) {
        option->palette.setCurrentColorGroup(QPalette::Active);
    } else {
        option->palette.setCurrentColorGroup(QPalette::Inactive);
    }
    option->fontMetrics = QFontMetrics(font());
    option->styleObject = const_cast<QGraphicsWidget *>(this);
}

/*!
    \reimp
*/
QSizeF QGraphicsWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    Q_D(const QGraphicsWidget);
    QSizeF sh;
    if (d->layout) {
        QSizeF marginSize(0,0);
        if (d->margins) {
            marginSize = QSizeF(d->margins[d->Left] + d->margins[d->Right],
                         d->margins[d->Top] + d->margins[d->Bottom]);
        }
        sh = d->layout->effectiveSizeHint(which, constraint - marginSize);
        sh += marginSize;
    } else {
        switch (which) {
            case Qt::MinimumSize:
                sh = QSizeF(0, 0);
                break;
            case Qt::PreferredSize:
                sh = QSizeF(50, 50);    //rather arbitrary
                break;
            case Qt::MaximumSize:
                sh = QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
                break;
            default:
                qWarning("QGraphicsWidget::sizeHint(): Don't know how to handle the value of 'which'");
                break;
        }
    }
    return sh;
}

/*!
    \property QGraphicsWidget::layout
    \brief The layout of the widget

    Any existing layout manager is deleted before the new layout is assigned. If
     \a layout is 0, the widget is left without a layout. Existing subwidgets'
    geometries will remain unaffected.

    QGraphicsWidget takes ownership of \a layout.

    All widgets that are currently managed by \a layout or all of its
    sublayouts, are automatically reparented to this item. The layout is then
    invalidated, and the child widget geometries are adjusted according to
    this item's geometry() and contentsMargins(). Children who are not
    explicitly managed by \a layout remain unaffected by the layout after
    it has been assigned to this widget.

    If no layout is currently managing this widget, layout() will return 0.

*/

/*!
    \fn void QGraphicsWidget::layoutChanged()
    This signal gets emitted whenever the layout of the item changes
    \internal
*/

/*!
    Returns this widget's layout, or 0 if no layout is currently managing this
    widget.

    \sa setLayout()
*/
QGraphicsLayout *QGraphicsWidget::layout() const
{
    Q_D(const QGraphicsWidget);
    return d->layout;
}

/*!
    \fn void QGraphicsWidget::setLayout(QGraphicsLayout *layout)

    Sets the layout for this widget to \a layout. Any existing layout manager
    is deleted before the new layout is assigned. If \a layout is 0, the
    widget is left without a layout. Existing subwidgets' geometries will
    remain unaffected.

    All widgets that are currently managed by \a layout or all of its
    sublayouts, are automatically reparented to this item. The layout is then
    invalidated, and the child widget geometries are adjusted according to
    this item's geometry() and contentsMargins(). Children who are not
    explicitly managed by \a layout remain unaffected by the layout after
    it has been assigned to this widget.

    QGraphicsWidget takes ownership of \a layout.

    \sa layout(), QGraphicsLinearLayout::addItem(), QGraphicsLayout::invalidate()
*/
void QGraphicsWidget::setLayout(QGraphicsLayout *l)
{
    Q_D(QGraphicsWidget);
    if (d->layout == l)
        return;
    d->setLayout_helper(l);
    if (!l)
        return;

    // Prevent assigning a layout that is already assigned to another widget.
    QGraphicsLayoutItem *oldParent = l->parentLayoutItem();
    if (oldParent && oldParent != this) {
        qWarning("QGraphicsWidget::setLayout: Attempting to set a layout on %s"
                 " \"%s\", when the layout already has a parent",
                 metaObject()->className(), qPrintable(objectName()));
        return;
    }

    // Install and activate the layout.
    l->setParentLayoutItem(this);
    l->d_func()->reparentChildItems(this);
    l->invalidate();
    emit layoutChanged();
}

/*!
    Adjusts the size of the widget to its effective preferred size hint.

    This function is called implicitly when the item is shown for the first
    time.

    \sa effectiveSizeHint(), Qt::MinimumSize
*/
void QGraphicsWidget::adjustSize()
{
    QSizeF sz = effectiveSizeHint(Qt::PreferredSize);
    // What if sz is not valid?!
    if (sz.isValid())
        resize(sz);
}

/*!
    \property QGraphicsWidget::layoutDirection
    \brief the layout direction for this widget.

    This property modifies this widget's and all of its descendants'
    Qt::WA_RightToLeft attribute. It also sets this widget's
    Qt::WA_SetLayoutDirection attribute.

    The widget's layout direction determines the order in which the layout
    manager horizontally arranges subwidgets of this widget. The default
    value depends on the language and locale of the application, and is
    typically in the same direction as words are read and written. With
    Qt::LeftToRight, the layout starts placing subwidgets from the left
    side of this widget towards the right. Qt::RightToLeft does the opposite -
    the layout will place widgets starting from the right edge moving towards
    the left.

    Subwidgets inherit their layout direction from the parent. Top-level
    widget items inherit their layout direction from
    QGraphicsScene::layoutDirection. If you change a widget's layout direction
    by calling setLayoutDirection(), the widget will send itself a
    \l{QEvent::LayoutDirectionChange}{LayoutDirectionChange} event, and then
    propagate the new layout direction to all its descendants.

    \sa QWidget::layoutDirection, QApplication::layoutDirection
*/
Qt::LayoutDirection QGraphicsWidget::layoutDirection() const
{
    return testAttribute(Qt::WA_RightToLeft) ? Qt::RightToLeft : Qt::LeftToRight;
}
void QGraphicsWidget::setLayoutDirection(Qt::LayoutDirection direction)
{
    Q_D(QGraphicsWidget);
    setAttribute(Qt::WA_SetLayoutDirection, true);
    d->setLayoutDirection_helper(direction);
}
void QGraphicsWidget::unsetLayoutDirection()
{
    Q_D(QGraphicsWidget);
    setAttribute(Qt::WA_SetLayoutDirection, false);
    d->resolveLayoutDirection();
}

/*!
    Returns a pointer to the widget's style. If this widget does not have any
    explicitly assigned style, the scene's style is returned instead. In turn,
    if the scene does not have any assigned style, this function returns
    QApplication::style().

    \sa setStyle()
*/
QStyle *QGraphicsWidget::style() const
{
    if (QStyle *style = widgetStyles()->styleForWidget(this))
        return style;
    // ### This is not thread-safe. QApplication::style() is not thread-safe.
    return scene() ? scene()->style() : QApplication::style();
}

/*!
    Sets the widget's style to \a style. QGraphicsWidget does \e not take
    ownership of \a style.

    If no style is assigned, or \a style is 0, the widget will use
    QGraphicsScene::style() (if this has been set). Otherwise the widget will
    use QApplication::style().

    This function sets the Qt::WA_SetStyle attribute if \a style is not 0;
    otherwise it clears the attribute.

    \sa style()
*/
void QGraphicsWidget::setStyle(QStyle *style)
{
    setAttribute(Qt::WA_SetStyle, style != 0);
    widgetStyles()->setStyleForWidget(this, style);

    // Deliver StyleChange to the widget itself (doesn't propagate).
    QEvent event(QEvent::StyleChange);
    QApplication::sendEvent(this, &event);
}

/*!
    \property QGraphicsWidget::font
    \brief the widgets' font

    This property provides the widget's font.

    QFont consists of font properties that have been explicitly defined and
    properties implicitly inherited from the widget's parent. Hence, font()
    can return a different font compared to the one set with setFont().
    This scheme allows you to define single entries in a font without
    affecting the font's inherited entries.

    When a widget's font changes, it resolves its entries against its
    parent widget. If the widget does not have a parent widget, it resolves
    its entries against the scene. The widget then sends itself a
    \l{QEvent::FontChange}{FontChange} event and notifies all its
    descendants so that they can resolve their fonts as well.

    By default, this property contains the application's default font.

    \sa QApplication::font(), QGraphicsScene::font, QFont::resolve()
*/
QFont QGraphicsWidget::font() const
{
    Q_D(const QGraphicsWidget);
    QFont fnt = d->font;
    fnt.resolve(fnt.resolve() | d->inheritedFontResolveMask);
    return fnt;
}
void QGraphicsWidget::setFont(const QFont &font)
{
    Q_D(QGraphicsWidget);
    setAttribute(Qt::WA_SetFont, font.resolve() != 0);

    QFont naturalFont = d->naturalWidgetFont();
    QFont resolvedFont = font.resolve(naturalFont);
    d->setFont_helper(resolvedFont);
}

/*!
    \property QGraphicsWidget::palette
    \brief the widget's palette

    This property provides the widget's palette. The palette provides colors
    and brushes for color groups (e.g., QPalette::Button) and states (e.g.,
    QPalette::Inactive), loosely defining the general look of the widget and
    its children.

    QPalette consists of color groups that have been explicitly defined, and
    groups that are implicitly inherited from the widget's parent. Because of
    this, palette() can return a different palette than what has been set with
    setPalette(). This scheme allows you to define single entries in a palette
    without affecting the palette's inherited entries.

    When a widget's palette changes, it resolves its entries against its
    parent widget, or if it doesn't have a parent widget, it resolves against
    the scene. It then sends itself a \l{QEvent::PaletteChange}{PaletteChange}
    event, and notifies all its descendants so they can resolve their palettes
    as well.

    By default, this property contains the application's default palette.

    \sa QApplication::palette(), QGraphicsScene::palette, QPalette::resolve()
*/
QPalette QGraphicsWidget::palette() const
{
    Q_D(const QGraphicsWidget);
    return d->palette;
}
void QGraphicsWidget::setPalette(const QPalette &palette)
{
    Q_D(QGraphicsWidget);
    setAttribute(Qt::WA_SetPalette, palette.resolve() != 0);

    QPalette naturalPalette = d->naturalWidgetPalette();
    QPalette resolvedPalette = palette.resolve(naturalPalette);
    d->setPalette_helper(resolvedPalette);
}

/*!
    \property QGraphicsWidget::autoFillBackground
    \brief whether the widget background is filled automatically
    \since 4.7

    If enabled, this property will cause Qt to fill the background of the
    widget before invoking the paint() method. The color used is defined by the
    QPalette::Window color role from the widget's \l{QPalette}{palette}.

    In addition, Windows are always filled with QPalette::Window, unless the
    WA_OpaquePaintEvent or WA_NoSystemBackground attributes are set.

    By default, this property is \c false.

    \sa Qt::WA_OpaquePaintEvent, Qt::WA_NoSystemBackground,
*/
bool QGraphicsWidget::autoFillBackground() const
{
    Q_D(const QGraphicsWidget);
    return d->autoFillBackground;
}
void QGraphicsWidget::setAutoFillBackground(bool enabled)
{
    Q_D(QGraphicsWidget);
    if (d->autoFillBackground != enabled) {
        d->autoFillBackground = enabled;
        update();
    }
}

/*!
    If this widget is currently managed by a layout, this function notifies
    the layout that the widget's size hints have changed and the layout
    may need to resize and reposition the widget accordingly.

    Call this function if the widget's sizeHint() has changed.

    \sa QGraphicsLayout::invalidate()
*/
void QGraphicsWidget::updateGeometry()
{
    QGraphicsLayoutItem::updateGeometry();
    QGraphicsLayoutItem *parentItem = parentLayoutItem();

    if (parentItem && parentItem->isLayout()) {
        if (QGraphicsLayout::instantInvalidatePropagation()) {
            static_cast<QGraphicsLayout *>(parentItem)->invalidate();
        } else {
            parentItem->updateGeometry();
        }
    } else {
        if (parentItem) {
            // This is for custom layouting
            QGraphicsWidget *parentWid = parentWidget();    //###
            if (parentWid->isVisible())
                QApplication::postEvent(parentWid, new QEvent(QEvent::LayoutRequest));
        } else {
            /**
             * If this is the topmost widget, post a LayoutRequest event to the widget.
             * When the event is received, it will start flowing all the way down to the leaf
             * widgets in one go. This will make a relayout flicker-free.
             */
            if (QGraphicsLayout::instantInvalidatePropagation())
                QApplication::postEvent(static_cast<QGraphicsWidget *>(this), new QEvent(QEvent::LayoutRequest));
        }
        if (!QGraphicsLayout::instantInvalidatePropagation()) {
            bool wasResized = testAttribute(Qt::WA_Resized);
            resize(size()); // this will restrict the size
            setAttribute(Qt::WA_Resized, wasResized);
        }
    }
}

/*!
    \reimp

    QGraphicsWidget uses the base implementation of this function to catch and
    deliver events related to state changes in the item. Because of this, it is
    very important that subclasses call the base implementation.

    \a change specifies the type of change, and \a value is the new value.

    For example, QGraphicsWidget uses ItemVisibleChange to deliver
    \l{QEvent::Show} {Show} and \l{QEvent::Hide}{Hide} events,
    ItemPositionHasChanged to deliver \l{QEvent::Move}{Move} events,
    and ItemParentChange both to deliver \l{QEvent::ParentChange}
    {ParentChange} events, and for managing the focus chain.

    QGraphicsWidget enables the ItemSendsGeometryChanges flag by default in
    order to track position changes.

    \sa QGraphicsItem::itemChange()
*/
QVariant QGraphicsWidget::itemChange(GraphicsItemChange change, const QVariant &value)
{
    Q_D(QGraphicsWidget);
    switch (change) {
    case ItemEnabledHasChanged: {
        // Send EnabledChange after the enabled state has changed.
        QEvent event(QEvent::EnabledChange);
        QApplication::sendEvent(this, &event);
        break;
    }
    case ItemVisibleChange:
        if (value.toBool()) {
            // Send Show event before the item has been shown.
            QShowEvent event;
            QApplication::sendEvent(this, &event);
            bool resized = testAttribute(Qt::WA_Resized);
            if (!resized) {
                adjustSize();
                setAttribute(Qt::WA_Resized, false);
            }
        }

        // layout size hint only changes if an item changes from/to explicitly hidden state
        if (value.toBool() || d->explicitlyHidden)
            updateGeometry();
        break;
    case ItemVisibleHasChanged:
        if (!value.toBool()) {
            // Send Hide event after the item has been hidden.
            QHideEvent event;
            QApplication::sendEvent(this, &event);
        }
        break;
    case ItemPositionHasChanged:
        d->setGeometryFromSetPos();
        break;
    case ItemParentChange: {
        // Deliver ParentAboutToChange.
        QEvent event(QEvent::ParentAboutToChange);
        QApplication::sendEvent(this, &event);
        break;
    }
    case ItemParentHasChanged: {
        // Deliver ParentChange.
        QEvent event(QEvent::ParentChange);
        QApplication::sendEvent(this, &event);
        break;
    }
    case ItemCursorHasChanged: {
        // Deliver CursorChange.
        QEvent event(QEvent::CursorChange);
        QApplication::sendEvent(this, &event);
        break;
    }
    case ItemToolTipHasChanged: {
        // Deliver ToolTipChange.
        QEvent event(QEvent::ToolTipChange);
        QApplication::sendEvent(this, &event);
        break;
    }
    default:
        break;
    }
    return QGraphicsItem::itemChange(change, value);
}

/*!
    \internal

    This virtual function is used to notify changes to any property (both
    dynamic properties, and registered with Q_PROPERTY) in the
    widget. Depending on the property itself, the notification can be
    delivered before or after the value has changed.

    \a propertyName is the name of the property (e.g., "size" or "font"), and
    \a value is the (proposed) new value of the property. The function returns
    the new value, which may be different from \a value if the notification
    supports adjusting the property value. The base implementation simply
    returns \a value for any \a propertyName.

    QGraphicsWidget delivers notifications for the following properties:

    \table
    \header    \li propertyName        \li Property
    \row       \li layoutDirection     \li QGraphicsWidget::layoutDirection
    \row       \li size                \li QGraphicsWidget::size
    \row       \li font                \li QGraphicsWidget::font
    \row       \li palette             \li QGraphicsWidget::palette
    \endtable

    \sa itemChange()
*/
QVariant QGraphicsWidget::propertyChange(const QString &propertyName, const QVariant &value)
{
    Q_UNUSED(propertyName);
    return value;
}

/*!
    QGraphicsWidget's implementation of sceneEvent() simply passes \a event to
    QGraphicsWidget::event(). You can handle all events for your widget in
    event() or in any of the convenience functions; you should not have to
    reimplement this function in a subclass of QGraphicsWidget.

    \sa QGraphicsItem::sceneEvent()
*/
bool QGraphicsWidget::sceneEvent(QEvent *event)
{
    return QGraphicsItem::sceneEvent(event);
}

/*!
    This event handler, for \a event, receives events for the window frame if
    this widget is a window. Its base implementation provides support for
    default window frame interaction such as moving, resizing, etc.

    You can reimplement this handler in a subclass of QGraphicsWidget to
    provide your own custom window frame interaction support.

    Returns \c true if \a event has been recognized and processed; otherwise,
    returns \c false.

    \sa event()
*/
bool QGraphicsWidget::windowFrameEvent(QEvent *event)
{
    Q_D(QGraphicsWidget);
    switch (event->type()) {
    case QEvent::GraphicsSceneMousePress:
        d->windowFrameMousePressEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
        break;
    case QEvent::GraphicsSceneMouseMove:
        d->ensureWindowData();
        if (d->windowData->grabbedSection != Qt::NoSection) {
            d->windowFrameMouseMoveEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
            event->accept();
        }
        break;
    case QEvent::GraphicsSceneMouseRelease:
        d->windowFrameMouseReleaseEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
        break;
    case QEvent::GraphicsSceneHoverMove:
        d->windowFrameHoverMoveEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
        break;
    case QEvent::GraphicsSceneHoverLeave:
        d->windowFrameHoverLeaveEvent(static_cast<QGraphicsSceneHoverEvent *>(event));
        break;
    default:
        break;
    }
    return event->isAccepted();
}

/*!
    \since 4.4

    Returns the window frame section at position \a pos, or
    Qt::NoSection if there is no window frame section at this
    position.

    This function is used in QGraphicsWidget's base implementation for window
    frame interaction.

    You can reimplement this function if you want to customize how a window
    can be interactively moved or resized.  For instance, if you only want to
    allow a window to be resized by the bottom right corner, you can
    reimplement this function to return Qt::NoSection for all sections except
    Qt::BottomRightSection.

    \sa windowFrameEvent(), paintWindowFrame(), windowFrameGeometry()
*/
Qt::WindowFrameSection QGraphicsWidget::windowFrameSectionAt(const QPointF &pos) const
{
    Q_D(const QGraphicsWidget);

    const QRectF r = windowFrameRect();
    if (!r.contains(pos))
        return Qt::NoSection;

    const qreal left = r.left();
    const qreal top = r.top();
    const qreal right = r.right();
    const qreal bottom = r.bottom();
    const qreal x = pos.x();
    const qreal y = pos.y();

    const qreal cornerMargin = 20;
    //### Not sure of this one, it should be the same value for all edges.
    const qreal windowFrameWidth = d->windowFrameMargins
        ? d->windowFrameMargins[d->Left] : 0;

    Qt::WindowFrameSection s = Qt::NoSection;
    if (x <= left + cornerMargin) {
        if (y <= top + windowFrameWidth || (x <= left + windowFrameWidth && y <= top + cornerMargin)) {
            s = Qt::TopLeftSection;
        } else if (y >= bottom - windowFrameWidth || (x <= left + windowFrameWidth && y >= bottom - cornerMargin)) {
            s = Qt::BottomLeftSection;
        } else if (x <= left + windowFrameWidth) {
            s = Qt::LeftSection;
        }
    } else if (x >= right - cornerMargin) {
        if (y <= top + windowFrameWidth || (x >= right - windowFrameWidth && y <= top + cornerMargin)) {
            s = Qt::TopRightSection;
        } else if (y >= bottom - windowFrameWidth || (x >= right - windowFrameWidth && y >= bottom - cornerMargin)) {
            s = Qt::BottomRightSection;
        } else if (x >= right - windowFrameWidth) {
            s = Qt::RightSection;
        }
    } else if (y <= top + windowFrameWidth) {
        s = Qt::TopSection;
    } else if (y >= bottom - windowFrameWidth) {
        s = Qt::BottomSection;
    }
    if (s == Qt::NoSection) {
        QRectF r1 = r;
        r1.setHeight(d->windowFrameMargins
                     ? d->windowFrameMargins[d->Top] : 0);
        if (r1.contains(pos))
            s = Qt::TitleBarArea;
    }
    return s;
}

/*!
    \reimp

    Handles the \a event.  QGraphicsWidget handles the following
    events:

    \table
    \header  \li Event                 \li Usage
    \row     \li Polish
                    \li Delivered to the widget some time after it has been
                        shown.
    \row     \li GraphicsSceneMove
                    \li Delivered to the widget after its local position has
                        changed.
    \row     \li GraphicsSceneResize
                    \li Delivered to the widget after its size has changed.
    \row     \li Show
                    \li Delivered to the widget before it has been shown.
    \row     \li Hide
                    \li Delivered to the widget after it has been hidden.
    \row     \li PaletteChange
                    \li Delivered to the widget after its palette has changed.
    \row     \li FontChange
                    \li Delivered to the widget after its font has changed.
    \row     \li EnabledChange
                    \li Delivered to the widget after its enabled state has
                        changed.
    \row     \li StyleChange
                    \li Delivered to the widget after its style has changed.
    \row     \li LayoutDirectionChange
                    \li Delivered to the widget after its layout direction has
                        changed.
    \row     \li ContentsRectChange
                    \li Delivered to the widget after its contents margins/
                        contents rect has changed.
    \endtable
*/
bool QGraphicsWidget::event(QEvent *event)
{
    Q_D(QGraphicsWidget);
    // Forward the event to the layout first.
    if (d->layout)
        d->layout->widgetEvent(event);

    // Handle the event itself.
    switch (event->type()) {
    case QEvent::GraphicsSceneMove:
        moveEvent(static_cast<QGraphicsSceneMoveEvent *>(event));
        break;
    case QEvent::GraphicsSceneResize:
        resizeEvent(static_cast<QGraphicsSceneResizeEvent *>(event));
        break;
    case QEvent::Show:
        showEvent(static_cast<QShowEvent *>(event));
        break;
    case QEvent::Hide:
        hideEvent(static_cast<QHideEvent *>(event));
        break;
    case QEvent::Polish:
        polishEvent();
        d->polished = true;
        if (!d->font.isCopyOf(QApplication::font()))
            d->updateFont(d->font);
        break;
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        update();
        break;
    case QEvent::StyleAnimationUpdate:
        if (isVisible()) {
            event->accept();
            update();
        }
        break;
        // Taken from QWidget::event
    case QEvent::ActivationChange:
    case QEvent::EnabledChange:
    case QEvent::FontChange:
    case QEvent::StyleChange:
    case QEvent::PaletteChange:
    case QEvent::ParentChange:
    case QEvent::ContentsRectChange:
    case QEvent::LayoutDirectionChange:
        changeEvent(event);
        break;
    case QEvent::Close:
        closeEvent((QCloseEvent *)event);
        break;
    case QEvent::GrabMouse:
        grabMouseEvent(event);
        break;
    case QEvent::UngrabMouse:
        ungrabMouseEvent(event);
        break;
    case QEvent::GrabKeyboard:
        grabKeyboardEvent(event);
        break;
    case QEvent::UngrabKeyboard:
        ungrabKeyboardEvent(event);
        break;
    case QEvent::GraphicsSceneMousePress:
        if (d->hasDecoration() && windowFrameEvent(event))
            return true;
        break;
    case QEvent::GraphicsSceneMouseMove:
    case QEvent::GraphicsSceneMouseRelease:
    case QEvent::GraphicsSceneMouseDoubleClick:
        d->ensureWindowData();
        if (d->hasDecoration() && d->windowData->grabbedSection != Qt::NoSection)
            return windowFrameEvent(event);
        break;
    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHoverMove:
    case QEvent::GraphicsSceneHoverLeave:
        if (d->hasDecoration()) {
            windowFrameEvent(event);
            // Filter out hover events if they were sent to us only because of the
            // decoration (special case in QGraphicsScenePrivate::dispatchHoverEvent).
            if (!acceptHoverEvents())
                return true;
        }
        break;
    default:
        break;
    }
    return QObject::event(event);
}

/*!
   This event handler can be reimplemented to handle state changes.

   The state being changed in this event can be retrieved through \a event.

   Change events include: QEvent::ActivationChange, QEvent::EnabledChange,
   QEvent::FontChange, QEvent::StyleChange, QEvent::PaletteChange,
   QEvent::ParentChange, QEvent::LayoutDirectionChange, and
   QEvent::ContentsRectChange.
*/
void QGraphicsWidget::changeEvent(QEvent *event)
{
    Q_D(QGraphicsWidget);
    switch (event->type()) {
    case QEvent::StyleChange:
        // ### Don't unset if the margins are explicitly set.
        unsetWindowFrameMargins();
        if (d->layout)
            d->layout->invalidate();
        Q_FALLTHROUGH();
    case QEvent::FontChange:
        update();
        updateGeometry();
        break;
    case QEvent::PaletteChange:
        update();
        break;
    case QEvent::ParentChange:
        d->resolveFont(d->inheritedFontResolveMask);
        d->resolvePalette(d->inheritedPaletteResolveMask);
        break;
    default:
        break;
    }
}

/*!
    This event handler, for \a event, can be reimplemented in a subclass to
    receive widget close events.  The default implementation accepts the
    event.

    \sa close(), QCloseEvent
*/
void QGraphicsWidget::closeEvent(QCloseEvent *event)
{
    event->accept();
}

/*!
    \reimp
*/
void QGraphicsWidget::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    if (focusPolicy() != Qt::NoFocus)
        update();
}

/*!
    Finds a new widget to give the keyboard focus to, as appropriate for Tab
    and Shift+Tab, and returns \c true if it can find a new widget; returns \c false
    otherwise. If \a next is true, this function searches forward; if \a next
    is false, it searches backward.

    Sometimes, you will want to reimplement this function to provide special
    focus handling for your widget and its subwidgets. For example, a web
    browser might reimplement it to move its current active link forward or
    backward, and call the base implementation only when it reaches the last
    or first link on the page.

    Child widgets call focusNextPrevChild() on their parent widgets, but only
    the window that contains the child widgets decides where to redirect
    focus. By reimplementing this function for an object, you gain control of
    focus traversal for all child widgets.

    \sa focusPolicy()
*/
bool QGraphicsWidget::focusNextPrevChild(bool next)
{
    Q_D(QGraphicsWidget);
    // Let the parent's focusNextPrevChild implementation decide what to do.
    QGraphicsWidget *parent = 0;
    if (!isWindow() && (parent = parentWidget()))
        return parent->focusNextPrevChild(next);
    if (!d->scene)
        return false;
    if (d->scene->focusNextPrevChild(next))
        return true;
    if (isWindow()) {
        setFocus(next ? Qt::TabFocusReason : Qt::BacktabFocusReason);
        if (hasFocus())
            return true;
    }
    return false;
}

/*!
    \reimp
*/
void QGraphicsWidget::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    if (focusPolicy() != Qt::NoFocus)
        update();
}

/*!
    This event handler, for \l{QEvent::Hide}{Hide} events, is delivered after
    the widget has been hidden, for example, setVisible(false) has been called
    for the widget or one of its ancestors when the widget was previously
    shown.

    You can reimplement this event handler to detect when your widget is
    hidden. Calling QEvent::accept() or QEvent::ignore() on \a event has no
    effect.

    \sa showEvent(), QWidget::hideEvent(), ItemVisibleChange
*/
void QGraphicsWidget::hideEvent(QHideEvent *event)
{
    ///### focusNextPrevChild(true), don't lose focus when the focus widget
    // is hidden.
    Q_UNUSED(event);
}

/*!
    This event handler, for \l{QEvent::GraphicsSceneMove}{GraphicsSceneMove}
    events, is delivered after the widget has moved (e.g., its local position
    has changed).

    This event is only delivered when the item is moved locally. Calling
    setTransform() or moving any of the item's ancestors does not affect the
    item's local position.

    You can reimplement this event handler to detect when your widget has
    moved. Calling QEvent::accept() or QEvent::ignore() on \a event has no
    effect.

    \sa ItemPositionChange, ItemPositionHasChanged
*/
void QGraphicsWidget::moveEvent(QGraphicsSceneMoveEvent *event)
{
    // ### Last position is always == current position
    Q_UNUSED(event);
}

/*!
    This event is delivered to the item by the scene at some point after it
    has been constructed, but before it is shown or otherwise accessed through
    the scene. You can use this event handler to do last-minute initializations
    of the widget which require the item to be fully constructed.

    The base implementation does nothing.
*/
void QGraphicsWidget::polishEvent()
{
}

/*!
    This event handler, for
    \l{QEvent::GraphicsSceneResize}{GraphicsSceneResize} events, is
    delivered after the widget has been resized (i.e., its local size has
    changed). \a event contains both the old and the new size.

    This event is only delivered when the widget is resized locally; calling
    setTransform() on the widget or any of its ancestors or view, does not
    affect the widget's local size.

    You can reimplement this event handler to detect when your widget has been
    resized. Calling QEvent::accept() or QEvent::ignore() on \a event has no
    effect.

    \sa geometry(), setGeometry()
*/
void QGraphicsWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    Q_UNUSED(event);
}

/*!
    This event handler, for \l{QEvent::Show}{Show} events, is delivered before
    the widget has been shown, for example, setVisible(true) has been called
    for the widget or one of its ancestors when the widget was previously
    hidden.

    You can reimplement this event handler to detect when your widget is
    shown. Calling QEvent::accept() or QEvent::ignore() on \a event has no
    effect.

    \sa hideEvent(), QWidget::showEvent(), ItemVisibleChange
*/
void QGraphicsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
}

/*!
    \reimp
*/
void QGraphicsWidget::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
}

/*!
    \reimp
*/
void QGraphicsWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsObject::hoverLeaveEvent(event);
}

/*!
    This event handler, for \a event, can be reimplemented in a subclass to
    receive notifications for QEvent::GrabMouse events.

    \sa grabMouse(), grabKeyboard()
*/
void QGraphicsWidget::grabMouseEvent(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
    This event handler, for \a event, can be reimplemented in a subclass to
    receive notifications for QEvent::UngrabMouse events.

    \sa ungrabMouse(), ungrabKeyboard()
*/
void QGraphicsWidget::ungrabMouseEvent(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
    This event handler, for \a event, can be reimplemented in a subclass to
    receive notifications for QEvent::GrabKeyboard events.

    \sa grabKeyboard(), grabMouse()
*/
void QGraphicsWidget::grabKeyboardEvent(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
    This event handler, for \a event, can be reimplemented in a subclass to
    receive notifications for QEvent::UngrabKeyboard events.

    \sa ungrabKeyboard(), ungrabMouse()
*/
void QGraphicsWidget::ungrabKeyboardEvent(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
    Returns the widgets window type.

    \sa windowFlags(), isWindow(), isPanel()
*/
Qt::WindowType QGraphicsWidget::windowType() const
{
    return Qt::WindowType(int(windowFlags()) & Qt::WindowType_Mask);
}

/*!
    \property QGraphicsWidget::windowFlags
    \brief the widget's window flags

    Window flags are a combination of a window type (e.g., Qt::Dialog) and
    several flags giving hints on the behavior of the window. The behavior
    is platform-dependent.

    By default, this property contains no window flags.

    Windows are panels. If you set the Qt::Window flag, the ItemIsPanel flag
    will be set automatically. If you clear the Qt::Window flag, the
    ItemIsPanel flag is also cleared. Note that the ItemIsPanel flag can be
    set independently of Qt::Window.

    \sa isWindow(), isPanel()
*/
Qt::WindowFlags QGraphicsWidget::windowFlags() const
{
    Q_D(const QGraphicsWidget);
    return d->windowFlags;
}
void QGraphicsWidget::setWindowFlags(Qt::WindowFlags wFlags)
{
    Q_D(QGraphicsWidget);
    if (d->windowFlags == wFlags)
        return;
    bool wasPopup = (d->windowFlags & Qt::WindowType_Mask) == Qt::Popup;

    d->adjustWindowFlags(&wFlags);
    d->windowFlags = wFlags;
    if (!d->setWindowFrameMargins)
        unsetWindowFrameMargins();

    setFlag(ItemIsPanel, d->windowFlags & Qt::Window);

    bool isPopup = (d->windowFlags & Qt::WindowType_Mask) == Qt::Popup;
    if (d->scene && isVisible() && wasPopup != isPopup) {
        // Popup state changed; update implicit mouse grab.
        if (!isPopup)
            d->scene->d_func()->removePopup(this);
        else
            d->scene->d_func()->addPopup(this);
    }

    if (d->scene && d->scene->d_func()->allItemsIgnoreHoverEvents && d->hasDecoration()) {
        d->scene->d_func()->allItemsIgnoreHoverEvents = false;
        d->scene->d_func()->enableMouseTrackingOnViews();
    }
}

/*!
    Returns \c true if this widget's window is in the active window, or if the
    widget does not have a window but is in an active scene (i.e., a scene
    that currently has focus).

    The active window is the window that either contains a child widget that
    currently has input focus, or that itself has input focus.

    \sa QGraphicsScene::activeWindow(), QGraphicsScene::setActiveWindow(), isActive()
*/
bool QGraphicsWidget::isActiveWindow() const
{
    return isActive();
}

/*!
    \property QGraphicsWidget::windowTitle
    \brief This property holds the window title (caption).

    This property is only used for windows.

    By default, if no title has been set, this property contains an
    empty string.
*/
void QGraphicsWidget::setWindowTitle(const QString &title)
{
    Q_D(QGraphicsWidget);
    d->ensureWindowData();
    d->windowData->windowTitle = title;
}
QString QGraphicsWidget::windowTitle() const
{
    Q_D(const QGraphicsWidget);
    return d->windowData ? d->windowData->windowTitle : QString();
}

/*!
    \property QGraphicsWidget::focusPolicy
    \brief the way the widget accepts keyboard focus

    The focus policy is Qt::TabFocus if the widget accepts keyboard focus by
    tabbing, Qt::ClickFocus if the widget accepts focus by clicking,
    Qt::StrongFocus if it accepts both, and Qt::NoFocus (the default) if it
    does not accept focus at all.

    You must enable keyboard focus for a widget if it processes keyboard
    events. This is normally done from the widget's constructor. For instance,
    the QLineEdit constructor calls setFocusPolicy(Qt::StrongFocus).

    If you enable a focus policy (i.e., not Qt::NoFocus), QGraphicsWidget will
    automatically enable the ItemIsFocusable flag.  Setting Qt::NoFocus on a
    widget will clear the ItemIsFocusable flag. If the widget currently has
    keyboard focus, the widget will automatically lose focus.

    \sa focusInEvent(), focusOutEvent(), keyPressEvent(), keyReleaseEvent(), enabled
*/
Qt::FocusPolicy QGraphicsWidget::focusPolicy() const
{
    Q_D(const QGraphicsWidget);
    return d->focusPolicy;
}
void QGraphicsWidget::setFocusPolicy(Qt::FocusPolicy policy)
{
    Q_D(QGraphicsWidget);
    if (d->focusPolicy == policy)
        return;
    d->focusPolicy = policy;
    if (hasFocus() && policy == Qt::NoFocus)
        clearFocus();
    setFlag(ItemIsFocusable, policy != Qt::NoFocus);
}

/*!
    If this widget, a child or descendant of this widget currently has input
    focus, this function will return a pointer to that widget. If
    no descendant widget has input focus, 0 is returned.

    \sa QGraphicsItem::focusItem(), QWidget::focusWidget()
*/
QGraphicsWidget *QGraphicsWidget::focusWidget() const
{
    Q_D(const QGraphicsWidget);
    if (d->subFocusItem && d->subFocusItem->d_ptr->isWidget)
        return static_cast<QGraphicsWidget *>(d->subFocusItem);
    return 0;
}

#ifndef QT_NO_SHORTCUT
/*!
    \since 4.5

    Adds a shortcut to Qt's shortcut system that watches for the given key \a
    sequence in the given \a context. If the \a context is
    Qt::ApplicationShortcut, the shortcut applies to the application as a
    whole. Otherwise, it is either local to this widget, Qt::WidgetShortcut,
    or to the window itself, Qt::WindowShortcut. For widgets that are not part
    of a window (i.e., top-level widgets and their children),
    Qt::WindowShortcut shortcuts apply to the scene.

    If the same key \a sequence has been grabbed by several widgets,
    when the key \a sequence occurs a QEvent::Shortcut event is sent
    to all the widgets to which it applies in a non-deterministic
    order, but with the ``ambiguous'' flag set to true.

    \warning You should not normally need to use this function;
    instead create \l{QAction}s with the shortcut key sequences you
    require (if you also want equivalent menu options and toolbar
    buttons), or create \l{QShortcut}s if you just need key sequences.
    Both QAction and QShortcut handle all the event filtering for you,
    and provide signals which are triggered when the user triggers the
    key sequence, so are much easier to use than this low-level
    function.

    \sa releaseShortcut(), setShortcutEnabled(), QWidget::grabShortcut()
*/
int QGraphicsWidget::grabShortcut(const QKeySequence &sequence, Qt::ShortcutContext context)
{
    Q_ASSERT(qApp);
    if (sequence.isEmpty())
        return 0;
    // ### setAttribute(Qt::WA_GrabbedShortcut);
    return qApp->d_func()->shortcutMap.addShortcut(this, sequence, context, qWidgetShortcutContextMatcher);
}

/*!
    \since 4.5

    Removes the shortcut with the given \a id from Qt's shortcut
    system. The widget will no longer receive QEvent::Shortcut events
    for the shortcut's key sequence (unless it has other shortcuts
    with the same key sequence).

    \warning You should not normally need to use this function since
    Qt's shortcut system removes shortcuts automatically when their
    parent widget is destroyed. It is best to use QAction or
    QShortcut to handle shortcuts, since they are easier to use than
    this low-level function. Note also that this is an expensive
    operation.

    \sa grabShortcut(), setShortcutEnabled(), QWidget::releaseShortcut()
*/
void QGraphicsWidget::releaseShortcut(int id)
{
    Q_ASSERT(qApp);
    if (id)
        qApp->d_func()->shortcutMap.removeShortcut(id, this, 0);
}

/*!
    \since 4.5

    If \a enabled is true, the shortcut with the given \a id is
    enabled; otherwise the shortcut is disabled.

    \warning You should not normally need to use this function since
    Qt's shortcut system enables/disables shortcuts automatically as
    widgets become hidden/visible and gain or lose focus. It is best
    to use QAction or QShortcut to handle shortcuts, since they are
    easier to use than this low-level function.

    \sa grabShortcut(), releaseShortcut(), QWidget::setShortcutEnabled()
*/
void QGraphicsWidget::setShortcutEnabled(int id, bool enabled)
{
    Q_ASSERT(qApp);
    if (id)
        qApp->d_func()->shortcutMap.setShortcutEnabled(enabled, id, this, 0);
}

/*!
    \since 4.5

    If \a enabled is true, auto repeat of the shortcut with the
    given \a id is enabled; otherwise it is disabled.

    \sa grabShortcut(), releaseShortcut(), QWidget::setShortcutAutoRepeat()
*/
void QGraphicsWidget::setShortcutAutoRepeat(int id, bool enabled)
{
    Q_ASSERT(qApp);
    if (id)
        qApp->d_func()->shortcutMap.setShortcutAutoRepeat(enabled, id, this, 0);
}
#endif

#ifndef QT_NO_ACTION
/*!
    \since 4.5

    Appends the action \a action to this widget's list of actions.

    All QGraphicsWidgets have a list of \l{QAction}s, however they can be
    represented graphically in many different ways. The default use of the
    QAction list (as returned by actions()) is to create a context QMenu.

    A QGraphicsWidget should only have one of each action and adding an action
    it already has will not cause the same action to be in the widget twice.

    \sa removeAction(), insertAction(), actions(), QWidget::addAction()
*/
void QGraphicsWidget::addAction(QAction *action)
{
    insertAction(0, action);
}

/*!
    \since 4.5

    Appends the actions \a actions to this widget's list of actions.

    \sa removeAction(), QMenu, addAction(), QWidget::addActions()
*/
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
void QGraphicsWidget::addActions(const QList<QAction *> &actions)
#else
void QGraphicsWidget::addActions(QList<QAction *> actions)
#endif
{
    for (int i = 0; i < actions.count(); ++i)
        insertAction(0, actions.at(i));
}

/*!
    \since 4.5

    Inserts the action \a action to this widget's list of actions,
    before the action \a before. It appends the action if \a before is 0 or
    \a before is not a valid action for this widget.

    A QGraphicsWidget should only have one of each action.

    \sa removeAction(), addAction(), QMenu, actions(),
    QWidget::insertActions()
*/
void QGraphicsWidget::insertAction(QAction *before, QAction *action)
{
    if (!action) {
        qWarning("QWidget::insertAction: Attempt to insert null action");
        return;
    }

    Q_D(QGraphicsWidget);
    int index = d->actions.indexOf(action);
    if (index != -1)
        d->actions.removeAt(index);

    int pos = d->actions.indexOf(before);
    if (pos < 0) {
        before = 0;
        pos = d->actions.size();
    }
    d->actions.insert(pos, action);

    if (index == -1) {
        QActionPrivate *apriv = action->d_func();
        apriv->graphicsWidgets.append(this);
    }

    QActionEvent e(QEvent::ActionAdded, action, before);
    QApplication::sendEvent(this, &e);
}

/*!
    \since 4.5

    Inserts the actions \a actions to this widget's list of actions,
    before the action \a before. It appends the action if \a before is 0 or
    \a before is not a valid action for this widget.

    A QGraphicsWidget can have at most one of each action.

    \sa removeAction(), QMenu, insertAction(), QWidget::insertActions()
*/
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
void QGraphicsWidget::insertActions(QAction *before, const QList<QAction *> &actions)
#else
void QGraphicsWidget::insertActions(QAction *before, QList<QAction *> actions)
#endif
{
    for (int i = 0; i < actions.count(); ++i)
        insertAction(before, actions.at(i));
}

/*!
    \since 4.5

    Removes the action \a action from this widget's list of actions.

    \sa insertAction(), actions(), insertAction(), QWidget::removeAction()
*/
void QGraphicsWidget::removeAction(QAction *action)
{
    if (!action)
        return;

    Q_D(QGraphicsWidget);

    QActionPrivate *apriv = action->d_func();
    apriv->graphicsWidgets.removeAll(this);

    if (d->actions.removeAll(action)) {
        QActionEvent e(QEvent::ActionRemoved, action);
        QApplication::sendEvent(this, &e);
    }
}

/*!
    \since 4.5

    Returns the (possibly empty) list of this widget's actions.

    \sa insertAction(), removeAction(), QWidget::actions(),
    QAction::associatedWidgets(), QAction::associatedGraphicsWidgets()
*/
QList<QAction *> QGraphicsWidget::actions() const
{
    Q_D(const QGraphicsWidget);
    return d->actions;
}
#endif

/*!
    Moves the \a second widget around the ring of focus widgets so that
    keyboard focus moves from the \a first widget to the \a second widget when
    the Tab key is pressed.

    Note that since the tab order of the \a second widget is changed, you
    should order a chain like this:

    \snippet code/src_gui_graphicsview_qgraphicswidget.cpp 1

    \e not like this:

    \snippet code/src_gui_graphicsview_qgraphicswidget.cpp 2

    If \a first is 0, this indicates that \a second should be the first widget
    to receive input focus should the scene gain Tab focus (i.e., the user
    hits Tab so that focus passes into the scene). If \a second is 0, this
    indicates that \a first should be the first widget to gain focus if the
    scene gained BackTab focus.

    By default, tab order is defined implicitly using widget creation order.

    \sa focusPolicy, {Keyboard Focus in Widgets}
*/
void QGraphicsWidget::setTabOrder(QGraphicsWidget *first, QGraphicsWidget *second)
{
    if (!first && !second) {
        qWarning("QGraphicsWidget::setTabOrder(0, 0) is undefined");
        return;
    }
    if ((first && second) && first->scene() != second->scene()) {
        qWarning("QGraphicsWidget::setTabOrder: scenes %p and %p are different",
                 first->scene(), second->scene());
        return;
    }
    QGraphicsScene *scene = first ? first->scene() : second->scene();
    if (!scene && (!first || !second)) {
        qWarning("QGraphicsWidget::setTabOrder: assigning tab order from/to the"
                 " scene requires the item to be in a scene.");
        return;
    }

    // If either first or second are 0, the scene's tabFocusFirst is updated
    // to point to the first item in the scene's focus chain. Then first or
    // second are set to point to tabFocusFirst.
    QGraphicsScenePrivate *sceneD = scene->d_func();
    if (!first) {
        sceneD->tabFocusFirst = second;
        return;
    }
    if (!second) {
        sceneD->tabFocusFirst = first->d_func()->focusNext;
        return;
    }

    // Both first and second are != 0.
    QGraphicsWidget *firstFocusNext = first->d_func()->focusNext;
    if (firstFocusNext == second) {
        // Nothing to do.
        return;
    }

    // Update the focus chain.
    QGraphicsWidget *secondFocusPrev = second->d_func()->focusPrev;
    QGraphicsWidget *secondFocusNext = second->d_func()->focusNext;
    firstFocusNext->d_func()->focusPrev = second;
    first->d_func()->focusNext = second;
    second->d_func()->focusNext = firstFocusNext;
    second->d_func()->focusPrev = first;
    secondFocusPrev->d_func()->focusNext = secondFocusNext;
    secondFocusNext->d_func()->focusPrev = secondFocusPrev;

    Q_ASSERT(first->d_func()->focusNext->d_func()->focusPrev == first);
    Q_ASSERT(first->d_func()->focusPrev->d_func()->focusNext == first);

    Q_ASSERT(second->d_func()->focusNext->d_func()->focusPrev == second);
    Q_ASSERT(second->d_func()->focusPrev->d_func()->focusNext == second);

}

/*!
    If \a on is true, this function enables \a attribute; otherwise
    \a attribute is disabled.

    See the class documentation for QGraphicsWidget for a complete list of
    which attributes are supported, and what they are for.

    \sa testAttribute(), QWidget::setAttribute()
*/
void QGraphicsWidget::setAttribute(Qt::WidgetAttribute attribute, bool on)
{
    Q_D(QGraphicsWidget);
    // ### most flags require some immediate action
    // ### we might want to qWarn use of unsupported attributes
    // ### we might want to not use Qt::WidgetAttribute, but roll our own instead
    d->setAttribute(attribute, on);
}

/*!
    Returns \c true if \a attribute is enabled for this widget; otherwise,
    returns \c false.

    \sa setAttribute()
*/
bool QGraphicsWidget::testAttribute(Qt::WidgetAttribute attribute) const
{
    Q_D(const QGraphicsWidget);
    return d->testAttribute(attribute);
}

/*!
  \enum QGraphicsWidget::anonymous

  The value returned by the virtual type() function.

  \value Type A graphics widget item
*/

/*!
    \reimp
*/
int QGraphicsWidget::type() const
{
    return Type;
}

/*!
    \reimp
*/
void QGraphicsWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

/*!
    This virtual function is called by QGraphicsScene to draw the window frame
    for windows using \a painter, \a option, and \a widget, in local
    coordinates. The base implementation uses the current style to render the
    frame and title bar.

    You can reimplement this function in a subclass of QGraphicsWidget to
    provide custom rendering of the widget's window frame.

    \sa QGraphicsItem::paint()
*/
void QGraphicsWidget::paintWindowFrame(QPainter *painter, const QStyleOptionGraphicsItem *option,
                                       QWidget *widget)
{
    const bool fillBackground = !testAttribute(Qt::WA_OpaquePaintEvent)
                                && !testAttribute(Qt::WA_NoSystemBackground);
    QGraphicsProxyWidget *proxy = qobject_cast<QGraphicsProxyWidget *>(this);
    const bool embeddedWidgetFillsOwnBackground = proxy && proxy->widget();

    if (rect().contains(option->exposedRect)) {
        if (fillBackground && !embeddedWidgetFillsOwnBackground)
            painter->fillRect(option->exposedRect, palette().window());
        return;
    }

    Q_D(QGraphicsWidget);

    QRect windowFrameRect = QRect(QPoint(), windowFrameGeometry().size().toSize());
    QStyleOptionTitleBar bar;
    bar.QStyleOption::operator=(*option);
    d->initStyleOptionTitleBar(&bar);   // this clear flags in bar.state
    d->ensureWindowData();
    bar.state.setFlag(QStyle::State_MouseOver, d->windowData->buttonMouseOver);
    bar.state.setFlag(QStyle::State_Sunken, d->windowData->buttonSunken);
    bar.rect = windowFrameRect;

    // translate painter to make the style happy
    const QPointF styleOrigin = this->windowFrameRect().topLeft();
    painter->translate(styleOrigin);

#ifdef Q_OS_MAC
    const QSize pixmapSize = windowFrameRect.size();
    if (pixmapSize.width() <= 0 || pixmapSize.height() <= 0)
        return;
    QPainter *realPainter = painter;
    QPixmap pm(pixmapSize);
    painter = new QPainter(&pm);
#endif

    // Fill background
    QStyleHintReturnMask mask;
    bool setMask = style()->styleHint(QStyle::SH_WindowFrame_Mask, &bar, widget, &mask) && !mask.region.isEmpty();
    bool hasBorder = !style()->styleHint(QStyle::SH_TitleBar_NoBorder, &bar, widget);
    int frameWidth = style()->pixelMetric(QStyle::PM_MDIFrameWidth, &bar, widget);
    if (setMask) {
        painter->save();
        painter->setClipRegion(mask.region, Qt::IntersectClip);
    }
    if (fillBackground) {
        if (embeddedWidgetFillsOwnBackground) {
            // Don't fill the background twice.
            QPainterPath windowFrameBackground;
            windowFrameBackground.addRect(windowFrameRect);
            // Adjust with 0.5 to avoid border artifacts between
            // widget background and frame background.
            windowFrameBackground.addRect(rect().translated(-styleOrigin).adjusted(0.5, 0.5, -0.5, -0.5));
            painter->fillPath(windowFrameBackground, palette().window());
        } else {
            painter->fillRect(windowFrameRect, palette().window());
        }
    }

    // Draw title
    int height = (int)d->titleBarHeight(bar);
    bar.rect.setHeight(height);
    if (hasBorder) // Frame is painted by PE_FrameWindow
        bar.rect.adjust(frameWidth, frameWidth, -frameWidth, 0);

    painter->save();
    painter->setFont(QApplication::font("QMdiSubWindowTitleBar"));
    style()->drawComplexControl(QStyle::CC_TitleBar, &bar, painter, widget);
    painter->restore();
    if (setMask)
        painter->restore();
    // Draw window frame
    QStyleOptionFrame frameOptions;
    frameOptions.QStyleOption::operator=(*option);
    initStyleOption(&frameOptions);
    if (!hasBorder)
        painter->setClipRect(windowFrameRect.adjusted(0, +height, 0, 0), Qt::IntersectClip);
    frameOptions.state.setFlag(QStyle::State_HasFocus, hasFocus());
    bool isActive = isActiveWindow();
    frameOptions.state.setFlag(QStyle::State_Active, isActive);

    frameOptions.palette.setCurrentColorGroup(isActive ? QPalette::Active : QPalette::Normal);
    frameOptions.rect = windowFrameRect;
    frameOptions.lineWidth = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, 0, widget);
    frameOptions.midLineWidth = 1;
    style()->drawPrimitive(QStyle::PE_FrameWindow, &frameOptions, painter, widget);

#ifdef Q_OS_MAC
    realPainter->drawPixmap(QPoint(), pm);
    delete painter;
#endif
}

/*!
    \reimp
*/
QRectF QGraphicsWidget::boundingRect() const
{
    return windowFrameRect();
}

/*!
    \reimp
*/
QPainterPath QGraphicsWidget::shape() const
{
    QPainterPath path;
    path.addRect(rect());
    return path;
}

/*!
    Call this function to close the widget.

    Returns \c true if the widget was closed; otherwise returns \c false.
    This slot will first send a QCloseEvent to the widget, which may or may
    not accept the event. If the event was ignored, nothing happens. If the
    event was accepted, it will hide() the widget.

    If the widget has the Qt::WA_DeleteOnClose attribute set it will be
    deleted.
*/
bool QGraphicsWidget::close()
{
    QCloseEvent closeEvent;
    QApplication::sendEvent(this, &closeEvent);
    if (!closeEvent.isAccepted()) {
        return false;
    }
    // hide
    if (isVisible()) {
        hide();
    }
    if (testAttribute(Qt::WA_DeleteOnClose)) {
        deleteLater();
    }
    return true;
}

#if 0
void QGraphicsWidget::dumpFocusChain()
{
    qDebug("=========== Dumping focus chain ==============");
    int i = 0;
    QGraphicsWidget *next = this;
    QSet<QGraphicsWidget*> visited;
    do {
        if (!next) {
            qWarning("Found a focus chain that is not circular, (next == 0)");
            break;
        }
        qDebug() << i++ << QString::number(uint(next), 16) << next->className() << next->data(0) << QString::fromLatin1("focusItem:%1").arg(next->hasFocus() ? '1' : '0') << QLatin1String("next:") << next->d_func()->focusNext->data(0) << QLatin1String("prev:") << next->d_func()->focusPrev->data(0);
        if (visited.contains(next)) {
            qWarning("Already visited this node. However, I expected to dump until I found myself.");
            break;
        }
        visited << next;
        next = next->d_func()->focusNext;
    } while (next != this);
}
#endif

QT_END_NAMESPACE

#include "moc_qgraphicswidget.cpp"
