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

#include "qgraphicslayout_p.h"
#include "qgraphicslayout.h"
#include "qgraphicswidget.h"
#include "qapplication.h"

QT_BEGIN_NAMESPACE

/*!
    \internal

    \a mw is the new parent. all items in the layout will be a child of \a mw.
 */
void QGraphicsLayoutPrivate::reparentChildItems(QGraphicsItem *newParent)
{
    Q_Q(QGraphicsLayout);
    int n =  q->count();
    //bool mwVisible = mw && mw->isVisible();
    for (int i = 0; i < n; ++i) {
        QGraphicsLayoutItem *layoutChild = q->itemAt(i);
        if (!layoutChild) {
            // Skip stretch items
            continue;
        }
        if (layoutChild->isLayout()) {
            QGraphicsLayout *l = static_cast<QGraphicsLayout*>(layoutChild);
            l->d_func()->reparentChildItems(newParent);
        } else if (QGraphicsItem *itemChild = layoutChild->graphicsItem()){
            QGraphicsItem *childParent = itemChild->parentItem();
#ifdef QT_DEBUG
            if (childParent && childParent != newParent && itemChild->isWidget() && qt_graphicsLayoutDebug()) {
                QGraphicsWidget *w = static_cast<QGraphicsWidget*>(layoutChild);
                qWarning("QGraphicsLayout::addChildLayout: widget %s \"%s\" in wrong parent; moved to correct parent",
                         w->metaObject()->className(), w->objectName().toLocal8Bit().constData());
            }
#endif
            if (childParent != newParent)
                itemChild->setParentItem(newParent);
        }
    }
}

void QGraphicsLayoutPrivate::getMargin(qreal *result, qreal userMargin, QStyle::PixelMetric pm) const
{
    if (!result)
        return;
    Q_Q(const QGraphicsLayout);

    QGraphicsLayoutItem *parent = q->parentLayoutItem();
    if (userMargin >= 0.0) {
        *result = userMargin;
    } else if (!parent) {
        *result = 0.0;
    } else if (parent->isLayout()) {    // sublayouts have 0 margin by default
        *result = 0.0;
    } else {
        *result = 0.0;
        if (QGraphicsItem *layoutParentItem = parentItem()) {
            if (layoutParentItem->isWidget())
                *result = (qreal)static_cast<QGraphicsWidget*>(layoutParentItem)->style()->pixelMetric(pm, 0);
        }
    }
}

Qt::LayoutDirection QGraphicsLayoutPrivate::visualDirection() const
{
    if (QGraphicsItem *maybeWidget = parentItem()) {
        if (maybeWidget->isWidget())
            return static_cast<QGraphicsWidget*>(maybeWidget)->layoutDirection();
    }
    return QApplication::layoutDirection();
}

static bool removeLayoutItemFromLayout(QGraphicsLayout *lay, QGraphicsLayoutItem *layoutItem)
{
    if (!lay)
        return false;

    for (int i = lay->count() - 1; i >= 0; --i) {
        QGraphicsLayoutItem *child = lay->itemAt(i);
        if (child && child->isLayout()) {
            if (removeLayoutItemFromLayout(static_cast<QGraphicsLayout*>(child), layoutItem))
                return true;
        } else if (child == layoutItem) {
            lay->removeAt(i);
            return true;
        }
    }
    return false;
}

/*!
    \internal

    This function is called from subclasses to add a layout item \a layoutItem
    to a layout.

    It takes care of automatically reparenting graphics items, if needed.

    If \a layoutItem is a  is already in a layout, it will remove it  from that layout.

*/
void QGraphicsLayoutPrivate::addChildLayoutItem(QGraphicsLayoutItem *layoutItem)
{
    Q_Q(QGraphicsLayout);
    if (QGraphicsLayoutItem *maybeLayout = layoutItem->parentLayoutItem()) {
        if (maybeLayout->isLayout())
            removeLayoutItemFromLayout(static_cast<QGraphicsLayout*>(maybeLayout), layoutItem);
    }
    layoutItem->setParentLayoutItem(q);
    if (layoutItem->isLayout()) {
        if (QGraphicsItem *parItem = parentItem()) {
            static_cast<QGraphicsLayout*>(layoutItem)->d_func()->reparentChildItems(parItem);
        }
    } else {
        if (QGraphicsItem *item = layoutItem->graphicsItem()) {
            QGraphicsItem *newParent = parentItem();
            QGraphicsItem *oldParent = item->parentItem();
            if (oldParent == newParent || !newParent)
                return;

#ifdef QT_DEBUG
            if (oldParent && item->isWidget()) {
                QGraphicsWidget *w = static_cast<QGraphicsWidget*>(item);
                qWarning("QGraphicsLayout::addChildLayoutItem: %s \"%s\" in wrong parent; moved to correct parent",
                    w->metaObject()->className(), w->objectName().toLocal8Bit().constData());
            }
#endif

            item->setParentItem(newParent);
        }
    }
}

void QGraphicsLayoutPrivate::activateRecursive(QGraphicsLayoutItem *item)
{
    if (item->isLayout()) {
        QGraphicsLayout *layout = static_cast<QGraphicsLayout *>(item);
        if (layout->d_func()->activated) {
            if (QGraphicsLayout::instantInvalidatePropagation()) {
                return;
            } else {
                layout->invalidate();   // ### LOOKS SUSPICIOUSLY WRONG!!???
            }
        }

        for (int i = layout->count() - 1; i >= 0; --i) {
            QGraphicsLayoutItem *childItem = layout->itemAt(i);
            if (childItem)
                activateRecursive(childItem);
        }
        layout->d_func()->activated = true;
    }
}

QT_END_NAMESPACE
