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

#include "qlayout.h"

#include "qapplication.h"
#include "qlayoutengine_p.h"
#if QT_CONFIG(menubar)
#include "qmenubar.h"
#endif
#if QT_CONFIG(toolbar)
#include "qtoolbar.h"
#endif
#if QT_CONFIG(sizegrip)
#include "qsizegrip.h"
#endif
#include "qevent.h"
#include "qstyle.h"
#include "qvariant.h"
#include "qwidget_p.h"
#include "qlayout_p.h"
#if QT_CONFIG(formlayout)
#include "qformlayout.h"
#endif

QT_BEGIN_NAMESPACE

static int menuBarHeightForWidth(QWidget *menubar, int w)
{
    if (menubar && !menubar->isHidden() && !menubar->isWindow()) {
        int result = menubar->heightForWidth(qMax(w, menubar->minimumWidth()));
        if (result == -1)
            result = menubar->sizeHint().height();
        const int min = qSmartMinSize(menubar).height();
        result = qBound(min, result, menubar->maximumSize().height());
        if (result != -1)
            return result;
    }
    return 0;
}

/*!
    \class QLayout
    \brief The QLayout class is the base class of geometry managers.

    \ingroup geomanagement
    \inmodule QtWidgets

    This is an abstract base class inherited by the concrete classes
    QBoxLayout, QGridLayout, QFormLayout, and QStackedLayout.

    For users of QLayout subclasses or of QMainWindow there is seldom
    any need to use the basic functions provided by QLayout, such as
    setSizeConstraint() or setMenuBar(). See \l{Layout Management}
    for more information.

    To make your own layout manager, implement the functions
    addItem(), sizeHint(), setGeometry(), itemAt() and takeAt(). You
    should also implement minimumSize() to ensure your layout isn't
    resized to zero size if there is too little space. To support
    children whose heights depend on their widths, implement
    hasHeightForWidth() and heightForWidth(). See the
    \l{layouts/borderlayout}{Border Layout} and
    \l{layouts/flowlayout}{Flow Layout} examples for
    more information about implementing custom layout managers.

    Geometry management stops when the layout manager is deleted.

    \sa QLayoutItem, {Layout Management}, {Basic Layouts Example},
        {Border Layout Example}, {Flow Layout Example}
*/


/*!
    Constructs a new top-level QLayout, with parent \a parent.
    \a parent may not be a \c nullptr.

    The layout is set directly as the top-level layout for
    \a parent. There can be only one top-level layout for a
    widget. It is returned by QWidget::layout().
*/
QLayout::QLayout(QWidget *parent)
    : QObject(*new QLayoutPrivate, parent)
{
    if (!parent)
        return;
    parent->setLayout(this);
}

/*!
    Constructs a new child QLayout.

    This layout has to be inserted into another layout before geometry
    management will work.
*/
QLayout::QLayout()
    : QObject(*new QLayoutPrivate, 0)
{
}


/*! \internal
 */
QLayout::QLayout(QLayoutPrivate &dd, QLayout *lay, QWidget *w)
    : QObject(dd, lay ? static_cast<QObject*>(lay) : static_cast<QObject*>(w))
{
    Q_D(QLayout);
    if (lay) {
        lay->addItem(this);
    } else if (w) {
        if (Q_UNLIKELY(w->layout())) {
            qWarning("QLayout: Attempting to add QLayout \"%ls\" to %s \"%ls\", which"
                     " already has a layout",
                     qUtf16Printable(QObject::objectName()), w->metaObject()->className(),
                     qUtf16Printable(w->objectName()));
            setParent(0);
        } else {
            d->topLevel = true;
            w->d_func()->layout = this;
            QT_TRY {
                invalidate();
            } QT_CATCH(...) {
                w->d_func()->layout = 0;
                QT_RETHROW;
            }
        }
    }
}

QLayoutPrivate::QLayoutPrivate()
    : QObjectPrivate(), insideSpacing(-1), userLeftMargin(-1), userTopMargin(-1), userRightMargin(-1),
      userBottomMargin(-1), topLevel(false), enabled(true), activated(true), autoNewChild(false),
      constraint(QLayout::SetDefaultConstraint), menubar(0)
{
}

void QLayoutPrivate::getMargin(int *result, int userMargin, QStyle::PixelMetric pm) const
{
    if (!result)
        return;

    Q_Q(const QLayout);
    if (userMargin >= 0) {
        *result = userMargin;
    } else if (!topLevel) {
        *result = 0;
    } else if (QWidget *pw = q->parentWidget()) {
        *result = pw->style()->pixelMetric(pm, 0, pw);
    } else {
        *result = 0;
    }
}

// Static item factory functions that allow for hooking things in Designer

QLayoutPrivate::QWidgetItemFactoryMethod QLayoutPrivate::widgetItemFactoryMethod = 0;
QLayoutPrivate::QSpacerItemFactoryMethod QLayoutPrivate::spacerItemFactoryMethod = 0;

QWidgetItem *QLayoutPrivate::createWidgetItem(const QLayout *layout, QWidget *widget)
{
    if (widgetItemFactoryMethod)
        if (QWidgetItem *wi = (*widgetItemFactoryMethod)(layout, widget))
            return wi;
    return new QWidgetItemV2(widget);
}

QSpacerItem *QLayoutPrivate::createSpacerItem(const QLayout *layout, int w, int h, QSizePolicy::Policy hPolicy, QSizePolicy::Policy vPolicy)
{
    if (spacerItemFactoryMethod)
        if (QSpacerItem *si = (*spacerItemFactoryMethod)(layout, w, h, hPolicy, vPolicy))
            return si;
    return new QSpacerItem(w, h,  hPolicy, vPolicy);
}



/*!
    \fn void QLayout::addItem(QLayoutItem *item)

    Implemented in subclasses to add an \a item. How it is added is
    specific to each subclass.

    This function is not usually called in application code. To add a widget
    to a layout, use the addWidget() function; to add a child layout, use the
    addLayout() function provided by the relevant QLayout subclass.

    \b{Note:} The ownership of \a item is transferred to the layout, and it's
    the layout's responsibility to delete it.

    \sa addWidget(), QBoxLayout::addLayout(), QGridLayout::addLayout()
*/

/*!
    Adds widget \a w to this layout in a manner specific to the
    layout. This function uses addItem().
*/
void QLayout::addWidget(QWidget *w)
{
    addChildWidget(w);
    addItem(QLayoutPrivate::createWidgetItem(this, w));
}



/*!
    Sets the alignment for widget \a w to \a alignment and returns
    true if \a w is found in this layout (not including child
    layouts); otherwise returns \c false.
*/
bool QLayout::setAlignment(QWidget *w, Qt::Alignment alignment)
{
    int i = 0;
    QLayoutItem *item = itemAt(i);
    while (item) {
        if (item->widget() == w) {
            item->setAlignment(alignment);
            invalidate();
            return true;
        }
        ++i;
        item = itemAt(i);
    }
    return false;
}

/*!
  \overload

  Sets the alignment for the layout \a l to \a alignment and
  returns \c true if \a l is found in this layout (not including child
  layouts); otherwise returns \c false.
*/
bool QLayout::setAlignment(QLayout *l, Qt::Alignment alignment)
{
    int i = 0;
    QLayoutItem *item = itemAt(i);
    while (item) {
        if (item->layout() == l) {
            item->setAlignment(alignment);
            invalidate();
            return true;
        }
        ++i;
        item = itemAt(i);
    }
    return false;
}

/*!
    \property QLayout::margin
    \brief the width of the outside border of the layout
    \obsolete

    Use setContentsMargins() and getContentsMargins() instead.

    \sa contentsRect(), spacing
*/

/*!
    \obsolete
*/
int QLayout::margin() const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    if (left == top && top == right && right == bottom) {
        return left;
    } else {
        return -1;
    }
}

/*!
    \property QLayout::spacing
    \brief the spacing between widgets inside the layout

    If no value is explicitly set, the layout's spacing is inherited
    from the parent layout, or from the style settings for the parent
    widget.

    For QGridLayout and QFormLayout, it is possible to set different horizontal and
    vertical spacings using \l{QGridLayout::}{setHorizontalSpacing()}
    and \l{QGridLayout::}{setVerticalSpacing()}. In that case,
    spacing() returns -1.

    \sa contentsRect(), getContentsMargins(), QStyle::layoutSpacing(),
        QStyle::pixelMetric()
*/

int QLayout::spacing() const
{
    if (const QBoxLayout* boxlayout = qobject_cast<const QBoxLayout*>(this)) {
        return boxlayout->spacing();
    } else if (const QGridLayout* gridlayout = qobject_cast<const QGridLayout*>(this)) {
        return gridlayout->spacing();
#if QT_CONFIG(formlayout)
    } else if (const QFormLayout* formlayout = qobject_cast<const QFormLayout*>(this)) {
        return formlayout->spacing();
#endif
    } else {
        Q_D(const QLayout);
        if (d->insideSpacing >=0) {
            return d->insideSpacing;
        } else {
            // arbitrarily prefer horizontal spacing to vertical spacing
            return qSmartSpacing(this, QStyle::PM_LayoutHorizontalSpacing);
        }
    }
}

/*!
    \obsolete
*/
void QLayout::setMargin(int margin)
{
    setContentsMargins(margin, margin, margin, margin);
}

void QLayout::setSpacing(int spacing)
{
    if (QBoxLayout* boxlayout = qobject_cast<QBoxLayout*>(this)) {
        boxlayout->setSpacing(spacing);
    } else if (QGridLayout* gridlayout = qobject_cast<QGridLayout*>(this)) {
        gridlayout->setSpacing(spacing);
#if QT_CONFIG(formlayout)
    } else if (QFormLayout* formlayout = qobject_cast<QFormLayout*>(this)) {
        formlayout->setSpacing(spacing);
#endif
    } else {
        Q_D(QLayout);
        d->insideSpacing = spacing;
        invalidate();
    }
}

/*!
    \since 4.3

    Sets the \a left, \a top, \a right, and \a bottom margins to use
    around the layout.

    By default, QLayout uses the values provided by the style. On
    most platforms, the margin is 11 pixels in all directions.

    \sa getContentsMargins(), QStyle::pixelMetric(),
        {QStyle::}{PM_LayoutLeftMargin},
        {QStyle::}{PM_LayoutTopMargin},
        {QStyle::}{PM_LayoutRightMargin},
        {QStyle::}{PM_LayoutBottomMargin}
*/
void QLayout::setContentsMargins(int left, int top, int right, int bottom)
{
    Q_D(QLayout);

    if (d->userLeftMargin == left && d->userTopMargin == top &&
        d->userRightMargin == right && d->userBottomMargin == bottom)
        return;

    d->userLeftMargin = left;
    d->userTopMargin = top;
    d->userRightMargin = right;
    d->userBottomMargin = bottom;
    invalidate();
}

/*!
    \since 4.6

    Sets the \a margins to use around the layout.

    By default, QLayout uses the values provided by the style. On
    most platforms, the margin is 11 pixels in all directions.

    \sa contentsMargins()
*/
void QLayout::setContentsMargins(const QMargins &margins)
{
    setContentsMargins(margins.left(), margins.top(), margins.right(), margins.bottom());
}

/*!
    \since 4.3

    Extracts the left, top, right, and bottom margins used around the
    layout, and assigns them to *\a left, *\a top, *\a right, and *\a
    bottom (unless they are null pointers).

    By default, QLayout uses the values provided by the style. On
    most platforms, the margin is 11 pixels in all directions.

    \sa setContentsMargins(), QStyle::pixelMetric(),
        {QStyle::}{PM_LayoutLeftMargin},
        {QStyle::}{PM_LayoutTopMargin},
        {QStyle::}{PM_LayoutRightMargin},
        {QStyle::}{PM_LayoutBottomMargin}
*/
void QLayout::getContentsMargins(int *left, int *top, int *right, int *bottom) const
{
    Q_D(const QLayout);
    d->getMargin(left, d->userLeftMargin, QStyle::PM_LayoutLeftMargin);
    d->getMargin(top, d->userTopMargin, QStyle::PM_LayoutTopMargin);
    d->getMargin(right, d->userRightMargin, QStyle::PM_LayoutRightMargin);
    d->getMargin(bottom, d->userBottomMargin, QStyle::PM_LayoutBottomMargin);
}

/*!
    \since 4.6

    Returns the margins used around the layout.

    By default, QLayout uses the values provided by the style. On
    most platforms, the margin is 11 pixels in all directions.

    \sa setContentsMargins()
*/
QMargins QLayout::contentsMargins() const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    return QMargins(left, top, right, bottom);
}

/*!
    \since 4.3

    Returns the layout's geometry() rectangle, but taking into account the
    contents margins.

    \sa setContentsMargins(), getContentsMargins()
*/
QRect QLayout::contentsRect() const
{
    Q_D(const QLayout);
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    return d->rect.adjusted(+left, +top, -right, -bottom);
}


/*!
    Returns the parent widget of this layout, or 0 if this layout is
    not installed on any widget.

    If the layout is a sub-layout, this function returns the parent
    widget of the parent layout.

    \sa parent()
*/
QWidget *QLayout::parentWidget() const
{
    Q_D(const QLayout);
    if (!d->topLevel) {
        if (parent()) {
            QLayout *parentLayout = qobject_cast<QLayout*>(parent());
            if (Q_UNLIKELY(!parentLayout)) {
                qWarning("QLayout::parentWidget: A layout can only have another layout as a parent.");
                return 0;
            }
            return parentLayout->parentWidget();
        } else {
            return 0;
        }
    } else {
        Q_ASSERT(parent() && parent()->isWidgetType());
        return static_cast<QWidget *>(parent());
    }
}

/*!
    \reimp
*/
bool QLayout::isEmpty() const
{
    int i = 0;
    QLayoutItem *item = itemAt(i);
    while (item) {
        if (!item->isEmpty())
            return false;
        ++i;
        item = itemAt(i);
    }
    return true;
}

/*!
    \reimp
*/
QSizePolicy::ControlTypes QLayout::controlTypes() const
{
    if (count() == 0)
        return QSizePolicy::DefaultType;
    QSizePolicy::ControlTypes types;
    for (int i = count() - 1; i >= 0; --i)
        types |= itemAt(i)->controlTypes();
    return types;
}

/*!
    \reimp
*/
void QLayout::setGeometry(const QRect &r)
{
    Q_D(QLayout);
    d->rect = r;
}

/*!
    \reimp
*/
QRect QLayout::geometry() const
{
    Q_D(const QLayout);
    return d->rect;
}

/*!
    \reimp
*/
void QLayout::invalidate()
{
    Q_D(QLayout);
    d->rect = QRect();
    update();
}

static bool removeWidgetRecursively(QLayoutItem *li, QObject *w)
{
    QLayout *lay = li->layout();
    if (!lay)
        return false;
    int i = 0;
    QLayoutItem *child;
    while ((child = lay->itemAt(i))) {
        if (child->widget() == w) {
            delete lay->takeAt(i);
            lay->invalidate();
            return true;
        } else if (removeWidgetRecursively(child, w)) {
            return true;
        } else {
            ++i;
        }
    }
    return false;
}


void QLayoutPrivate::doResize(const QSize &r)
{
    Q_Q(QLayout);
    int mbh = menuBarHeightForWidth(menubar, r.width());
    QWidget *mw = q->parentWidget();
    QRect rect = mw->testAttribute(Qt::WA_LayoutOnEntireRect) ? mw->rect() : mw->contentsRect();
    const int mbTop = rect.top();
    rect.setTop(mbTop + mbh);
    q->setGeometry(rect);
#if QT_CONFIG(menubar)
    if (menubar)
        menubar->setGeometry(rect.left(), mbTop, r.width(), mbh);
#endif
}


/*!
    \internal
    Performs child widget layout when the parent widget is
    resized.  Also handles removal of widgets. \a e is the
    event
*/
void QLayout::widgetEvent(QEvent *e)
{
    Q_D(QLayout);
    if (!d->enabled)
        return;

    switch (e->type()) {
    case QEvent::Resize:
        if (d->activated) {
            QResizeEvent *r = (QResizeEvent *)e;
            d->doResize(r->size());
        } else {
            activate();
        }
        break;
    case QEvent::ChildRemoved:
        {
            QChildEvent *c = (QChildEvent *)e;
            if (c->child()->isWidgetType()) {
#if QT_CONFIG(menubar)
                if (c->child() == d->menubar)
                    d->menubar = 0;
#endif
                removeWidgetRecursively(this, c->child());
            }
        }
        break;
    case QEvent::LayoutRequest:
        if (static_cast<QWidget *>(parent())->isVisible())
            activate();
        break;
    default:
        break;
    }
}

/*!
    \reimp
*/
void QLayout::childEvent(QChildEvent *e)
{
    Q_D(QLayout);
    if (!d->enabled)
        return;

    if (e->type() != QEvent::ChildRemoved)
        return;

    if (QLayout *childLayout = qobject_cast<QLayout *>(e->child()))
        removeItem(childLayout);
}

/*!
  \internal
  Also takes contentsMargins and menu bar into account.
*/
int QLayout::totalHeightForWidth(int w) const
{
    Q_D(const QLayout);
    int side=0, top=0;
    if (d->topLevel) {
        QWidget *parent = parentWidget();
        parent->ensurePolished();
        QWidgetPrivate *wd = parent->d_func();
        side += wd->leftmargin + wd->rightmargin;
        top += wd->topmargin + wd->bottommargin;
    }
    int h = heightForWidth(w - side) + top;
#if QT_CONFIG(menubar)
    h += menuBarHeightForWidth(d->menubar, w);
#endif
    return h;
}

/*!
  \internal
  Also takes contentsMargins and menu bar into account.
*/
QSize QLayout::totalMinimumSize() const
{
    Q_D(const QLayout);
    int side=0, top=0;
    if (d->topLevel) {
        QWidget *pw = parentWidget();
        pw->ensurePolished();
        QWidgetPrivate *wd = pw->d_func();
        side += wd->leftmargin + wd->rightmargin;
        top += wd->topmargin + wd->bottommargin;
    }

    QSize s = minimumSize();
#if QT_CONFIG(menubar)
    top += menuBarHeightForWidth(d->menubar, s.width() + side);
#endif
    return s + QSize(side, top);
}

/*!
  \internal
  Also takes contentsMargins and menu bar into account.
*/
QSize QLayout::totalSizeHint() const
{
    Q_D(const QLayout);
    int side=0, top=0;
    if (d->topLevel) {
        QWidget *pw = parentWidget();
        pw->ensurePolished();
        QWidgetPrivate *wd = pw->d_func();
        side += wd->leftmargin + wd->rightmargin;
        top += wd->topmargin + wd->bottommargin;
    }

    QSize s = sizeHint();
    if (hasHeightForWidth())
        s.setHeight(heightForWidth(s.width() + side));
#if QT_CONFIG(menubar)
    top += menuBarHeightForWidth(d->menubar, s.width());
#endif
    return s + QSize(side, top);
}

/*!
  \internal
  Also takes contentsMargins and menu bar into account.
*/
QSize QLayout::totalMaximumSize() const
{
    Q_D(const QLayout);
    int side=0, top=0;
    if (d->topLevel) {
        QWidget *pw = parentWidget();
        pw->ensurePolished();
        QWidgetPrivate *wd = pw->d_func();
        side += wd->leftmargin + wd->rightmargin;
        top += wd->topmargin + wd->bottommargin;
    }

    QSize s = maximumSize();
#if QT_CONFIG(menubar)
    top += menuBarHeightForWidth(d->menubar, s.width());
#endif

    if (d->topLevel)
        s = QSize(qMin(s.width() + side, QLAYOUTSIZE_MAX),
                   qMin(s.height() + top, QLAYOUTSIZE_MAX));
    return s;
}

/*!
  \internal
  Destroys the layout, deleting all child layouts.
  Geometry management stops when a top-level layout is deleted.

  The layout classes will probably be fatally confused if you delete
  a sublayout.
*/
QLayout::~QLayout()
{
    Q_D(QLayout);
    if (d->topLevel && parent() && parent()->isWidgetType() && parentWidget()->layout() == this)
        parentWidget()->d_func()->layout = 0;
    else if (QLayout *parentLayout = qobject_cast<QLayout *>(parent()))
        parentLayout->removeItem(this);
}


/*!
    This function is called from \c addLayout() or \c insertLayout() functions in
    subclasses to add layout \a l as a sub-layout.

    The only scenario in which you need to call it directly is if you
    implement a custom layout that supports nested layouts.

    \sa QBoxLayout::addLayout(), QBoxLayout::insertLayout(), QGridLayout::addLayout()
*/
void QLayout::addChildLayout(QLayout *l)
{
    if (Q_UNLIKELY(l->parent())) {
        qWarning("QLayout::addChildLayout: layout \"%ls\" already has a parent",
                 qUtf16Printable(l->objectName()));
        return;
    }
    l->setParent(this);

    if (QWidget *mw = parentWidget()) {
        l->d_func()->reparentChildWidgets(mw);
    }

}

/*!
   \internal
 */
bool QLayout::adoptLayout(QLayout *layout)
{
    const bool ok = !layout->parent();
    addChildLayout(layout);
    return ok;
}

#ifdef QT_DEBUG
static bool layoutDebug()
{
    static int checked_env = -1;
    if(checked_env == -1)
        checked_env = !!qEnvironmentVariableIntValue("QT_LAYOUT_DEBUG");

    return checked_env;
}
#endif

void QLayoutPrivate::reparentChildWidgets(QWidget *mw)
{
    Q_Q(QLayout);
    int n =  q->count();

#if QT_CONFIG(menubar)
    if (menubar && menubar->parentWidget() != mw) {
        menubar->setParent(mw);
    }
#endif
    bool mwVisible = mw && mw->isVisible();
    for (int i = 0; i < n; ++i) {
        QLayoutItem *item = q->itemAt(i);
        if (QWidget *w = item->widget()) {
            QWidget *pw = w->parentWidget();
#ifdef QT_DEBUG
            if (Q_UNLIKELY(pw && pw != mw && layoutDebug())) {
                qWarning("QLayout::addChildLayout: widget %s \"%ls\" in wrong parent; moved to correct parent",
                         w->metaObject()->className(), qUtf16Printable(w->objectName()));
            }
#endif
            bool needShow = mwVisible && !(w->isHidden() && w->testAttribute(Qt::WA_WState_ExplicitShowHide));
            if (pw != mw)
                w->setParent(mw);
            if (needShow)
                QMetaObject::invokeMethod(w, "_q_showIfNotHidden", Qt::QueuedConnection); //show later
        } else if (QLayout *l = item->layout()) {
            l->d_func()->reparentChildWidgets(mw);
        }
    }
}

/*!
    Returns \c true if the \a widget can be added to the \a layout;
    otherwise returns \c false.
*/
bool QLayoutPrivate::checkWidget(QWidget *widget) const
{
    Q_Q(const QLayout);
    if (Q_UNLIKELY(!widget)) {
        qWarning("QLayout: Cannot add a null widget to %s/%ls", q->metaObject()->className(),
                  qUtf16Printable(q->objectName()));
        return false;
    }
    if (Q_UNLIKELY(widget == q->parentWidget())) {
        qWarning("QLayout: Cannot add parent widget %s/%ls to its child layout %s/%ls",
                  widget->metaObject()->className(), qUtf16Printable(widget->objectName()),
                  q->metaObject()->className(), qUtf16Printable(q->objectName()));
        return false;
    }
    return true;
}

/*!
    Returns \c true if the \a otherLayout can be added to the \a layout;
    otherwise returns \c false.
*/
bool QLayoutPrivate::checkLayout(QLayout *otherLayout) const
{
    Q_Q(const QLayout);
    if (Q_UNLIKELY(!otherLayout)) {
        qWarning("QLayout: Cannot add a null layout to %s/%ls",
                 q->metaObject()->className(), qUtf16Printable(q->objectName()));
        return false;
    }
    if (Q_UNLIKELY(otherLayout == q)) {
        qWarning("QLayout: Cannot add layout %s/%ls to itself",
                 q->metaObject()->className(), qUtf16Printable(q->objectName()));
        return false;
    }
    return true;
}

/*!
    This function is called from \c addWidget() functions in
    subclasses to add \a w as a managed widget of a layout.

    If \a w is already managed by a layout, this function will give a warning
    and remove \a w from that layout. This function must therefore be
    called before adding \a w to the layout's data structure.
*/
void QLayout::addChildWidget(QWidget *w)
{
    QWidget *mw = parentWidget();
    QWidget *pw = w->parentWidget();

    //Qt::WA_LaidOut is never reset. It only means that the widget at some point has
    //been in a layout.
    if (pw && w->testAttribute(Qt::WA_LaidOut)) {
        QLayout *l = pw->layout();
        if (l && removeWidgetRecursively(l, w)) {
#ifdef QT_DEBUG
            if (Q_UNLIKELY(layoutDebug()))
                qWarning("QLayout::addChildWidget: %s \"%ls\" is already in a layout; moved to new layout",
                         w->metaObject()->className(), qUtf16Printable(w->objectName()));
#endif
        }
    }
    if (pw && mw && pw != mw) {
#ifdef QT_DEBUG
            if (Q_UNLIKELY(layoutDebug()))
                qWarning("QLayout::addChildWidget: %s \"%ls\" in wrong parent; moved to correct parent",
                         w->metaObject()->className(), qUtf16Printable(w->objectName()));
#endif
        pw = 0;
    }
    bool needShow = mw && mw->isVisible() && !(w->isHidden() && w->testAttribute(Qt::WA_WState_ExplicitShowHide));
    if (!pw && mw)
        w->setParent(mw);
    w->setAttribute(Qt::WA_LaidOut);
    if (needShow)
        QMetaObject::invokeMethod(w, "_q_showIfNotHidden", Qt::QueuedConnection); //show later
}








/*!
    Tells the geometry manager to place the menu bar \a widget at the
    top of parentWidget(), outside QWidget::contentsMargins(). All
    child widgets are placed below the bottom edge of the menu bar.
*/
void QLayout::setMenuBar(QWidget *widget)
{
    Q_D(QLayout);
        if (widget)
            addChildWidget(widget);
    d->menubar = widget;
}

/*!
    Returns the menu bar set for this layout, or 0 if no menu bar is
    set.
*/

QWidget *QLayout::menuBar() const
{
    Q_D(const QLayout);
    return d->menubar;
}


/*!
    Returns the minimum size of this layout. This is the smallest
    size that the layout can have while still respecting the
    specifications.

    The returned value doesn't include the space required by
    QWidget::setContentsMargins() or menuBar().

    The default implementation allows unlimited resizing.
*/
QSize QLayout::minimumSize() const
{
    return QSize(0, 0);
}

/*!
    Returns the maximum size of this layout. This is the largest size
    that the layout can have while still respecting the
    specifications.

    The returned value doesn't include the space required by
    QWidget::setContentsMargins() or menuBar().

    The default implementation allows unlimited resizing.
*/
QSize QLayout::maximumSize() const
{
    return QSize(QLAYOUTSIZE_MAX, QLAYOUTSIZE_MAX);
}

/*!
    Returns whether this layout can make use of more space than
    sizeHint(). A value of Qt::Vertical or Qt::Horizontal means that
    it wants to grow in only one dimension, whereas Qt::Vertical |
    Qt::Horizontal means that it wants to grow in both dimensions.

    The default implementation returns Qt::Horizontal | Qt::Vertical.
    Subclasses reimplement it to return a meaningful value based on
    their child widgets's \l{QSizePolicy}{size policies}.

    \sa sizeHint()
*/
Qt::Orientations QLayout::expandingDirections() const
{
    return Qt::Horizontal | Qt::Vertical;
}

void QLayout::activateRecursiveHelper(QLayoutItem *item)
{
    item->invalidate();
    QLayout *layout = item->layout();
    if (layout) {
        QLayoutItem *child;
        int i=0;
        while ((child = layout->itemAt(i++)))
            activateRecursiveHelper(child);
        layout->d_func()->activated = true;
    }
}

/*!
  Updates the layout for parentWidget().

  You should generally not need to call this because it is
  automatically called at the most appropriate times.

  \sa activate(), invalidate()
*/

void QLayout::update()
{
    QLayout *layout = this;
    while (layout && layout->d_func()->activated) {
        layout->d_func()->activated = false;
        if (layout->d_func()->topLevel) {
            Q_ASSERT(layout->parent()->isWidgetType());
            QWidget *mw = static_cast<QWidget*>(layout->parent());
            QApplication::postEvent(mw, new QEvent(QEvent::LayoutRequest));
            break;
        }
        layout = static_cast<QLayout*>(layout->parent());
    }
}

/*!
    Redoes the layout for parentWidget() if necessary.

    You should generally not need to call this because it is
    automatically called at the most appropriate times. It returns
    true if the layout was redone.

    \sa update(), QWidget::updateGeometry()
*/
bool QLayout::activate()
{
    Q_D(QLayout);
    if (!d->enabled || !parent())
        return false;
    if (!d->topLevel)
        return static_cast<QLayout*>(parent())->activate();
    if (d->activated)
        return false;
    QWidget *mw = static_cast<QWidget*>(parent());
    if (Q_UNLIKELY(!mw)) {
        qWarning("QLayout::activate: %s \"%ls\" does not have a main widget",
                 metaObject()->className(), qUtf16Printable(objectName()));
        return false;
    }
    activateRecursiveHelper(this);

    QWidgetPrivate *md = mw->d_func();
    uint explMin = md->extra ? md->extra->explicitMinSize : 0;
    uint explMax = md->extra ? md->extra->explicitMaxSize : 0;

    switch (d->constraint) {
    case SetFixedSize:
        // will trigger resize
        mw->setFixedSize(totalSizeHint());
        break;
    case SetMinimumSize:
        mw->setMinimumSize(totalMinimumSize());
        break;
    case SetMaximumSize:
        mw->setMaximumSize(totalMaximumSize());
        break;
    case SetMinAndMaxSize:
        mw->setMinimumSize(totalMinimumSize());
        mw->setMaximumSize(totalMaximumSize());
        break;
    case SetDefaultConstraint: {
        bool widthSet = explMin & Qt::Horizontal;
        bool heightSet = explMin & Qt::Vertical;
        if (mw->isWindow()) {
            QSize ms = totalMinimumSize();
            if (widthSet)
                ms.setWidth(mw->minimumSize().width());
            if (heightSet)
                ms.setHeight(mw->minimumSize().height());
            mw->setMinimumSize(ms);
        } else if (!widthSet || !heightSet) {
            QSize ms = mw->minimumSize();
            if (!widthSet)
                ms.setWidth(0);
            if (!heightSet)
                ms.setHeight(0);
            mw->setMinimumSize(ms);
        }
        break;
    }
    case SetNoConstraint:
        break;
    }

    d->doResize(mw->size());

    if (md->extra) {
        md->extra->explicitMinSize = explMin;
        md->extra->explicitMaxSize = explMax;
    }
    // ideally only if sizeHint() or sizePolicy() has changed
    mw->updateGeometry();
    return true;
}

/*!
    \since 5.2

    Searches for widget \a from and replaces it with widget \a to if found.
    Returns the layout item that contains the widget \a from on success.
    Otherwise \c 0 is returned. If \a options contains \c Qt::FindChildrenRecursively
    (the default), sub-layouts are searched for doing the replacement.
    Any other flag in \a options is ignored.

    Notice that the returned item therefore might not belong to this layout,
    but to a sub-layout.

    The returned layout item is no longer owned by the layout and should be
    either deleted or inserted to another layout. The widget \a from is no
    longer managed by the layout and may need to be deleted or hidden. The
    parent of widget \a from is left unchanged.

    This function works for the built-in Qt layouts, but might not work for
    custom layouts.

    \sa indexOf()
*/

QLayoutItem *QLayout::replaceWidget(QWidget *from, QWidget *to, Qt::FindChildOptions options)
{
    Q_D(QLayout);
    if (!from || !to)
        return 0;

    int index = -1;
    QLayoutItem *item = 0;
    for (int u = 0; u < count(); ++u) {
        item = itemAt(u);
        if (!item)
            continue;

        if (item->widget() == from) {
            index = u;
            break;
        }

        if (item->layout() && (options & Qt::FindChildrenRecursively)) {
            QLayoutItem *r = item->layout()->replaceWidget(from, to, options);
            if (r)
                return r;
        }
    }
    if (index == -1)
        return 0;

    addChildWidget(to);
    QLayoutItem *newitem = new QWidgetItem(to);
    newitem->setAlignment(item->alignment());
    QLayoutItem *r = d->replaceAt(index, newitem);
    if (!r)
        delete newitem;
    return r;
}

/*!
    \fn QLayoutItem *QLayout::itemAt(int index) const

    Must be implemented in subclasses to return the layout item at \a
    index. If there is no such item, the function must return 0.
    Items are numbered consecutively from 0. If an item is deleted, other items will be renumbered.

    This function can be used to iterate over a layout. The following
    code will draw a rectangle for each layout item in the layout structure of the widget.

    \snippet code/src_gui_kernel_qlayout.cpp 0

    \sa count(), takeAt()
*/

/*!
    \fn QLayoutItem *QLayout::takeAt(int index)

    Must be implemented in subclasses to remove the layout item at \a
    index from the layout, and return the item. If there is no such
    item, the function must do nothing and return 0.  Items are numbered
    consecutively from 0. If an item is removed, other items will be
    renumbered.

    The following code fragment shows a safe way to remove all items
    from a layout:

    \snippet code/src_gui_kernel_qlayout.cpp 1

    \sa itemAt(), count()
*/

/*!
    \fn int *QLayout::count() const

    Must be implemented in subclasses to return the number of items
    in the layout.

    \sa itemAt()
*/

/*!
    Searches for widget \a widget in this layout (not including child
    layouts).

    Returns the index of \a widget, or -1 if \a widget is not found.

    The default implementation iterates over all items using itemAt()
*/
int QLayout::indexOf(QWidget *widget) const
{
    int i = 0;
    QLayoutItem *item = itemAt(i);
    while (item) {
        if (item->widget() == widget)
            return i;
        ++i;
        item = itemAt(i);
    }
    return -1;
}

/*!
    \since 5.12
    Searches for layout item \a layoutItem in this layout (not including child
    layouts).

    Returns the index of \a layoutItem, or -1 if \a layoutItem is not found.
*/
int QLayout::indexOf(QLayoutItem *layoutItem) const
{
    int i = 0;
    QLayoutItem *item = itemAt(i);
    while (item) {
        if (item == layoutItem)
            return i;
        ++i;
        item = itemAt(i);
    }
    return -1;
}

/*!
    \enum QLayout::SizeConstraint

    The possible values are:

    \value SetDefaultConstraint The main widget's minimum size is set
                    to minimumSize(), unless the widget already has
                    a minimum size.

    \value SetFixedSize The main widget's size is set to sizeHint(); it
                    cannot be resized at all.
    \value SetMinimumSize  The main widget's minimum size is set to
                    minimumSize(); it cannot be smaller.

    \value SetMaximumSize  The main widget's maximum size is set to
                    maximumSize(); it cannot be larger.

    \value SetMinAndMaxSize  The main widget's minimum size is set to
                    minimumSize() and its maximum size is set to
                    maximumSize().

    \value SetNoConstraint  The widget is not constrained.

    \sa setSizeConstraint()
*/

/*!
    \property QLayout::sizeConstraint
    \brief the resize mode of the layout

    The default mode is \l {QLayout::SetDefaultConstraint}
    {SetDefaultConstraint}.
*/
void QLayout::setSizeConstraint(SizeConstraint constraint)
{
    Q_D(QLayout);
    if (constraint == d->constraint)
        return;

    d->constraint = constraint;
    invalidate();
}

QLayout::SizeConstraint QLayout::sizeConstraint() const
{
    Q_D(const QLayout);
    return d->constraint;
}

/*!
    Returns the rectangle that should be covered when the geometry of
    this layout is set to \a r, provided that this layout supports
    setAlignment().

    The result is derived from sizeHint() and expanding(). It is never
    larger than \a r.
*/
QRect QLayout::alignmentRect(const QRect &r) const
{
    QSize s = sizeHint();
    Qt::Alignment a = alignment();

    /*
      This is a hack to obtain the real maximum size, not
      QSize(QLAYOUTSIZE_MAX, QLAYOUTSIZE_MAX), the value consistently
      returned by QLayoutItems that have an alignment.
    */
    QLayout *that = const_cast<QLayout *>(this);
    that->setAlignment(0);
    QSize ms = that->maximumSize();
    that->setAlignment(a);

    if ((expandingDirections() & Qt::Horizontal) ||
         !(a & Qt::AlignHorizontal_Mask)) {
        s.setWidth(qMin(r.width(), ms.width()));
    }
    if ((expandingDirections() & Qt::Vertical) ||
         !(a & Qt::AlignVertical_Mask)) {
        s.setHeight(qMin(r.height(), ms.height()));
    } else if (hasHeightForWidth()) {
        int hfw = heightForWidth(s.width());
        if (hfw < s.height())
            s.setHeight(qMin(hfw, ms.height()));
    }

    s = s.boundedTo(r.size());
    int x = r.x();
    int y = r.y();

    if (a & Qt::AlignBottom)
        y += (r.height() - s.height());
    else if (!(a & Qt::AlignTop))
        y += (r.height() - s.height()) / 2;

    QWidget *parent = parentWidget();
    a = QStyle::visualAlignment(parent ? parent->layoutDirection() : QApplication::layoutDirection(), a);
    if (a & Qt::AlignRight)
        x += (r.width() - s.width());
    else if (!(a & Qt::AlignLeft))
        x += (r.width() - s.width()) / 2;

    return QRect(x, y, s.width(), s.height());
}

/*!
    Removes the widget \a widget from the layout. After this call, it
    is the caller's responsibility to give the widget a reasonable
    geometry or to put the widget back into a layout or to explicitly
    hide it if necessary.

    \b{Note:} The ownership of \a widget remains the same as
    when it was added.

    \sa removeItem(), QWidget::setGeometry(), addWidget()
*/
void QLayout::removeWidget(QWidget *widget)
{
    int i = 0;
    QLayoutItem *child;
    while ((child = itemAt(i))) {
        if (child->widget() == widget) {
            delete takeAt(i);
            invalidate();
        } else {
            ++i;
        }
    }
}

/*!
    Removes the layout item \a item from the layout. It is the
    caller's responsibility to delete the item.

    Notice that \a item can be a layout (since QLayout inherits
    QLayoutItem).

    \sa removeWidget(), addItem()
*/
void QLayout::removeItem(QLayoutItem *item)
{
    int i = 0;
    QLayoutItem *child;
    while ((child = itemAt(i))) {
        if (child == item) {
            takeAt(i);
            invalidate();
        } else {
            ++i;
        }
    }
}

/*!
    Enables this layout if \a enable is true, otherwise disables it.

    An enabled layout adjusts dynamically to changes; a disabled
    layout acts as if it did not exist.

    By default all layouts are enabled.

    \sa isEnabled()
*/
void QLayout::setEnabled(bool enable)
{
    Q_D(QLayout);
    d->enabled = enable;
}

/*!
    Returns \c true if the layout is enabled; otherwise returns \c false.

    \sa setEnabled()
*/
bool QLayout::isEnabled() const
{
    Q_D(const QLayout);
    return d->enabled;
}

/*!
    Returns a size that satisfies all size constraints on \a widget,
    including heightForWidth() and that is as close as possible to \a
    size.
*/

QSize QLayout::closestAcceptableSize(const QWidget *widget, const QSize &size)
{
    QSize result = size.boundedTo(qSmartMaxSize(widget));
    result = result.expandedTo(qSmartMinSize(widget));
    QLayout *l = widget->layout();
    if (l && l->hasHeightForWidth() && result.height() < l->minimumHeightForWidth(result.width()) ) {
        QSize current = widget->size();
        int currentHfw =  l->minimumHeightForWidth(current.width());
        int newHfw = l->minimumHeightForWidth(result.width());
        if (current.height() < currentHfw || currentHfw == newHfw) {
            //handle the constant hfw case and the vertical-only case, as well as the
            // current-size-is-not-correct case
            result.setHeight(newHfw);
        } else {
            // binary search; assume hfw is decreasing ###

            int maxw = qMax(widget->width(),result.width());
            int maxh = qMax(widget->height(), result.height());
            int minw = qMin(widget->width(),result.width());
            int minh = qMin(widget->height(), result.height());

            int minhfw = l->minimumHeightForWidth(minw);
            int maxhfw = l->minimumHeightForWidth(maxw);
            while (minw < maxw) {
                if (minhfw > maxh) { //assume decreasing
                    minw = maxw - (maxw-minw)/2;
                    minhfw = l->minimumHeightForWidth(minw);
                } else if (maxhfw < minh ) { //assume decreasing
                    maxw = minw + (maxw-minw)/2;
                    maxhfw = l->minimumHeightForWidth(maxw);
                } else  {
                    break;
                }
            }
            result = result.expandedTo(QSize(minw, minhfw));
        }
    }
    return result;
}

QT_END_NAMESPACE

#include "moc_qlayout.cpp"
