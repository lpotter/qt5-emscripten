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
/*!
    \class QGraphicsSceneIndex
    \brief The QGraphicsSceneIndex class provides a base class to implement
    a custom indexing algorithm for discovering items in QGraphicsScene.
    \since 4.6
    \ingroup graphicsview-api

    \internal

    The QGraphicsSceneIndex class provides a base class to implement
    a custom indexing algorithm for discovering items in QGraphicsScene. You
    need to subclass it and reimplement addItem, removeItem, estimateItems
    and items in order to have an functional indexing.

    \sa QGraphicsScene, QGraphicsView
*/

#include "qdebug.h"
#include "qgraphicsscene.h"
#include "qgraphicsitem_p.h"
#include "qgraphicsscene_p.h"
#include "qgraphicswidget.h"
#include "qgraphicssceneindex_p.h"
#include "qgraphicsscenebsptreeindex_p.h"

QT_BEGIN_NAMESPACE

namespace QtPrivate { // just to keep indentation of the following functions at the same level

    static bool intersect_rect(const QGraphicsItem *item, const QRectF &exposeRect, Qt::ItemSelectionMode mode,
                               const QTransform &deviceTransform, const void *intersectData)
    {
        const QRectF sceneRect = *static_cast<const QRectF *>(intersectData);

        QRectF brect = item->boundingRect();
        _q_adjustRect(&brect);

        // ### Add test for this (without making things slower?)
        Q_UNUSED(exposeRect);

        bool keep = true;
        const QGraphicsItemPrivate *itemd = QGraphicsItemPrivate::get(item);
        if (itemd->itemIsUntransformable()) {
            // Untransformable items; map the scene rect to item coordinates.
            const QTransform transform = item->deviceTransform(deviceTransform);
            QRectF itemRect = (deviceTransform * transform.inverted()).mapRect(sceneRect);
            if (mode == Qt::ContainsItemShape || mode == Qt::ContainsItemBoundingRect)
                keep = itemRect.contains(brect) && itemRect != brect;
            else
                keep = itemRect.intersects(brect);
            if (keep && (mode == Qt::ContainsItemShape || mode == Qt::IntersectsItemShape)) {
                QPainterPath itemPath;
                itemPath.addRect(itemRect);
                keep = QGraphicsSceneIndexPrivate::itemCollidesWithPath(item, itemPath, mode);
            }
        } else {
            Q_ASSERT(!itemd->dirtySceneTransform);
            const QRectF itemSceneBoundingRect = itemd->sceneTransformTranslateOnly
                                               ? brect.translated(itemd->sceneTransform.dx(),
                                                                  itemd->sceneTransform.dy())
                                               : itemd->sceneTransform.mapRect(brect);
            if (mode == Qt::ContainsItemShape || mode == Qt::ContainsItemBoundingRect)
                keep = sceneRect != brect && sceneRect.contains(itemSceneBoundingRect);
            else
                keep = sceneRect.intersects(itemSceneBoundingRect);
            if (keep && (mode == Qt::ContainsItemShape || mode == Qt::IntersectsItemShape)) {
                QPainterPath rectPath;
                rectPath.addRect(sceneRect);
                if (itemd->sceneTransformTranslateOnly)
                    rectPath.translate(-itemd->sceneTransform.dx(), -itemd->sceneTransform.dy());
                else
                    rectPath = itemd->sceneTransform.inverted().map(rectPath);
                keep = QGraphicsSceneIndexPrivate::itemCollidesWithPath(item, rectPath, mode);
            }
        }
        return keep;
    }

    static bool intersect_point(const QGraphicsItem *item, const QRectF &exposeRect, Qt::ItemSelectionMode mode,
                                const QTransform &deviceTransform, const void *intersectData)
    {
        const QPointF scenePoint = *static_cast<const QPointF *>(intersectData);

        QRectF brect = item->boundingRect();
        _q_adjustRect(&brect);

        // ### Add test for this (without making things slower?)
        Q_UNUSED(exposeRect);

        bool keep = false;
        const QGraphicsItemPrivate *itemd = QGraphicsItemPrivate::get(item);
        if (itemd->itemIsUntransformable()) {
            // Untransformable items; map the scene point to item coordinates.
            const QTransform transform = item->deviceTransform(deviceTransform);
            QPointF itemPoint = (deviceTransform * transform.inverted()).map(scenePoint);
            keep = brect.contains(itemPoint);
            if (keep && (mode == Qt::ContainsItemShape || mode == Qt::IntersectsItemShape)) {
                QPainterPath pointPath;
                pointPath.addRect(QRectF(itemPoint, QSizeF(1, 1)));
                keep = QGraphicsSceneIndexPrivate::itemCollidesWithPath(item, pointPath, mode);
            }
        } else {
            Q_ASSERT(!itemd->dirtySceneTransform);
            QRectF sceneBoundingRect = itemd->sceneTransformTranslateOnly
                                     ? brect.translated(itemd->sceneTransform.dx(),
                                                        itemd->sceneTransform.dy())
                                     : itemd->sceneTransform.mapRect(brect);
            keep = sceneBoundingRect.intersects(QRectF(scenePoint, QSizeF(1, 1)));
            if (keep && (mode == Qt::ContainsItemShape || mode == Qt::IntersectsItemShape)) {
                QPointF p = itemd->sceneTransformTranslateOnly
                          ? QPointF(scenePoint.x() - itemd->sceneTransform.dx(),
                                    scenePoint.y() - itemd->sceneTransform.dy())
                          : itemd->sceneTransform.inverted().map(scenePoint);
                keep = item->contains(p);
            }
        }

        return keep;
    }

    static bool intersect_path(const QGraphicsItem *item, const QRectF &exposeRect, Qt::ItemSelectionMode mode,
                               const QTransform &deviceTransform, const void *intersectData)
    {
        const QPainterPath scenePath = *static_cast<const QPainterPath *>(intersectData);

        QRectF brect = item->boundingRect();
        _q_adjustRect(&brect);

        // ### Add test for this (without making things slower?)
        Q_UNUSED(exposeRect);

        bool keep = true;
        const QGraphicsItemPrivate *itemd = QGraphicsItemPrivate::get(item);
        if (itemd->itemIsUntransformable()) {
            // Untransformable items; map the scene rect to item coordinates.
            const QTransform transform = item->deviceTransform(deviceTransform);
            QPainterPath itemPath = (deviceTransform * transform.inverted()).map(scenePath);
            if (mode == Qt::ContainsItemShape || mode == Qt::ContainsItemBoundingRect)
                keep = itemPath.contains(brect);
            else
                keep = itemPath.intersects(brect);
            if (keep && (mode == Qt::ContainsItemShape || mode == Qt::IntersectsItemShape))
                keep = QGraphicsSceneIndexPrivate::itemCollidesWithPath(item, itemPath, mode);
        } else {
            Q_ASSERT(!itemd->dirtySceneTransform);
            const QRectF itemSceneBoundingRect = itemd->sceneTransformTranslateOnly
                                               ? brect.translated(itemd->sceneTransform.dx(),
                                                                  itemd->sceneTransform.dy())
                                               : itemd->sceneTransform.mapRect(brect);
            if (mode == Qt::ContainsItemShape || mode == Qt::ContainsItemBoundingRect)
                keep = scenePath.contains(itemSceneBoundingRect);
            else
                keep = scenePath.intersects(itemSceneBoundingRect);
            if (keep && (mode == Qt::ContainsItemShape || mode == Qt::IntersectsItemShape)) {
                QPainterPath itemPath = itemd->sceneTransformTranslateOnly
                                      ? scenePath.translated(-itemd->sceneTransform.dx(),
                                                             -itemd->sceneTransform.dy())
                                      : itemd->sceneTransform.inverted().map(scenePath);
                keep = QGraphicsSceneIndexPrivate::itemCollidesWithPath(item, itemPath, mode);
            }
        }
        return keep;
    }

} // namespace QtPrivate

/*!
    Constructs a private scene index.
*/
QGraphicsSceneIndexPrivate::QGraphicsSceneIndexPrivate(QGraphicsScene *scene) : scene(scene)
{
}

/*!
    Destructor of private scene index.
*/
QGraphicsSceneIndexPrivate::~QGraphicsSceneIndexPrivate()
{
}

/*!
    \internal

    Checks if item collides with the path and mode, but also checks that if it
    doesn't collide, maybe its frame rect will.
*/
bool QGraphicsSceneIndexPrivate::itemCollidesWithPath(const QGraphicsItem *item,
                                                      const QPainterPath &path,
                                                      Qt::ItemSelectionMode mode)
{
    if (item->collidesWithPath(path, mode))
        return true;
    if (item->isWidget()) {
        // Check if this is a window, and if its frame rect collides.
        const QGraphicsWidget *widget = static_cast<const QGraphicsWidget *>(item);
        if (widget->isWindow()) {
            QRectF frameRect = widget->windowFrameRect();
            QPainterPath framePath;
            framePath.addRect(frameRect);
            bool intersects = path.intersects(frameRect);
            if (mode == Qt::IntersectsItemShape || mode == Qt::IntersectsItemBoundingRect)
                return intersects || path.contains(frameRect.topLeft())
                    || framePath.contains(path.elementAt(0));
            return !intersects && path.contains(frameRect.topLeft());
        }
    }
    return false;
}

/*!
    \internal
    This function returns the items in ascending order.
*/
void QGraphicsSceneIndexPrivate::recursive_items_helper(QGraphicsItem *item, QRectF exposeRect,
                                                        QGraphicsSceneIndexIntersector intersect,
                                                        QList<QGraphicsItem *> *items,
                                                        const QTransform &viewTransform,
                                                        Qt::ItemSelectionMode mode,
                                                        qreal parentOpacity, const void *intersectData) const
{
    Q_ASSERT(item);
    if (!item->d_ptr->visible)
        return;

    const qreal opacity = item->d_ptr->combineOpacityFromParent(parentOpacity);
    const bool itemIsFullyTransparent = QGraphicsItemPrivate::isOpacityNull(opacity);
    const bool itemHasChildren = !item->d_ptr->children.isEmpty();
    if (itemIsFullyTransparent && (!itemHasChildren || item->d_ptr->childrenCombineOpacity()))
        return;

    // Update the item's scene transform if dirty.
    const bool itemIsUntransformable = item->d_ptr->itemIsUntransformable();
    const bool wasDirtyParentSceneTransform = item->d_ptr->dirtySceneTransform && !itemIsUntransformable;
    if (wasDirtyParentSceneTransform) {
        item->d_ptr->updateSceneTransformFromParent();
        Q_ASSERT(!item->d_ptr->dirtySceneTransform);
    }

    const bool itemClipsChildrenToShape = (item->d_ptr->flags & QGraphicsItem::ItemClipsChildrenToShape
                                           || item->d_ptr->flags & QGraphicsItem::ItemContainsChildrenInShape);
    bool processItem = !itemIsFullyTransparent;
    if (processItem) {
        processItem = intersect(item, exposeRect, mode, viewTransform, intersectData);
        if (!processItem && (!itemHasChildren || itemClipsChildrenToShape)) {
            if (wasDirtyParentSceneTransform)
                item->d_ptr->invalidateChildrenSceneTransform();
            return;
        }
    } // else we know for sure this item has children we must process.

    int i = 0;
    if (itemHasChildren) {
        // Sort children.
        item->d_ptr->ensureSortedChildren();

        // Clip to shape.
        if (itemClipsChildrenToShape && !itemIsUntransformable) {
            QPainterPath mappedShape = item->d_ptr->sceneTransformTranslateOnly
                                     ? item->shape().translated(item->d_ptr->sceneTransform.dx(),
                                                                item->d_ptr->sceneTransform.dy())
                                     : item->d_ptr->sceneTransform.map(item->shape());
            exposeRect &= mappedShape.controlPointRect();
        }

        // Process children behind
        for (i = 0; i < item->d_ptr->children.size(); ++i) {
            QGraphicsItem *child = item->d_ptr->children.at(i);
            if (wasDirtyParentSceneTransform)
                child->d_ptr->dirtySceneTransform = 1;
            if (!(child->d_ptr->flags & QGraphicsItem::ItemStacksBehindParent))
                break;
            if (itemIsFullyTransparent && !(child->d_ptr->flags & QGraphicsItem::ItemIgnoresParentOpacity))
                continue;
            recursive_items_helper(child, exposeRect, intersect, items, viewTransform,
                                   mode, opacity, intersectData);
        }
    }

    // Process item
    if (processItem)
        items->append(item);

    // Process children in front
    if (itemHasChildren) {
        for (; i < item->d_ptr->children.size(); ++i) {
            QGraphicsItem *child = item->d_ptr->children.at(i);
            if (wasDirtyParentSceneTransform)
                child->d_ptr->dirtySceneTransform = 1;
            if (itemIsFullyTransparent && !(child->d_ptr->flags & QGraphicsItem::ItemIgnoresParentOpacity))
                continue;
            recursive_items_helper(child, exposeRect, intersect, items, viewTransform,
                                   mode, opacity, intersectData);
        }
    }
}

void QGraphicsSceneIndexPrivate::init()
{
    if (!scene)
        return;

    QObject::connect(scene, SIGNAL(sceneRectChanged(QRectF)),
                     q_func(), SLOT(updateSceneRect(QRectF)));
}

/*!
    Constructs an abstract scene index for a given \a scene.
*/
QGraphicsSceneIndex::QGraphicsSceneIndex(QGraphicsScene *scene)
: QObject(*new QGraphicsSceneIndexPrivate(scene), scene)
{
    d_func()->init();
}

/*!
    \internal
*/
QGraphicsSceneIndex::QGraphicsSceneIndex(QGraphicsSceneIndexPrivate &dd, QGraphicsScene *scene)
    : QObject(dd, scene)
{
    d_func()->init();
}

/*!
    Destroys the scene index.
*/
QGraphicsSceneIndex::~QGraphicsSceneIndex()
{

}

/*!
    Returns the scene of this index.
*/
QGraphicsScene* QGraphicsSceneIndex::scene() const
{
    Q_D(const QGraphicsSceneIndex);
    return d->scene;
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsSceneIndex::items(const QPointF &pos,
    Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform
    &deviceTransform) const

    Returns all visible items that, depending on \a mode, are at the specified
    \a pos and return a list sorted using \a order.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with \a pos are returned.

    \a deviceTransform is the transformation apply to the view.

    This method use the estimation of the index (estimateItems) and refine the
    list to get an exact result. If you want to implement your own refinement
    algorithm you can reimplement this method.

    \sa estimateItems()

*/
QList<QGraphicsItem *> QGraphicsSceneIndex::items(const QPointF &pos, Qt::ItemSelectionMode mode,
                                                  Qt::SortOrder order, const QTransform &deviceTransform) const
{

    Q_D(const QGraphicsSceneIndex);
    QList<QGraphicsItem *> itemList;
    d->items_helper(QRectF(pos, QSizeF(1, 1)), &QtPrivate::intersect_point, &itemList, deviceTransform, mode, order, &pos);
    return itemList;
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsSceneIndex::items(const QRectF &rect,
    Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform
    &deviceTransform) const

    \overload

    Returns all visible items that, depending on \a mode, are either inside or
    intersect with the specified \a rect and return a list sorted using \a order.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a rect are returned.

    \a deviceTransform is the transformation apply to the view.

    This method use the estimation of the index (estimateItems) and refine
    the list to get an exact result. If you want to implement your own
    refinement algorithm you can reimplement this method.

    \sa estimateItems()

*/
QList<QGraphicsItem *> QGraphicsSceneIndex::items(const QRectF &rect, Qt::ItemSelectionMode mode,
                                                  Qt::SortOrder order, const QTransform &deviceTransform) const
{
    Q_D(const QGraphicsSceneIndex);
    QRectF exposeRect = rect;
    _q_adjustRect(&exposeRect);
    QList<QGraphicsItem *> itemList;
    d->items_helper(exposeRect, &QtPrivate::intersect_rect, &itemList, deviceTransform, mode, order, &rect);
    return itemList;
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsSceneIndex::items(const QPolygonF
    &polygon, Qt::ItemSelectionMode mode, Qt::SortOrder order, const
    QTransform &deviceTransform) const

    \overload

    Returns all visible items that, depending on \a mode, are either inside or
    intersect with the specified \a polygon and return a list sorted using \a order.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a polygon are returned.

    \a deviceTransform is the transformation apply to the view.

    This method use the estimation of the index (estimateItems) and refine
    the list to get an exact result. If you want to implement your own
    refinement algorithm you can reimplement this method.

    \sa estimateItems()

*/
QList<QGraphicsItem *> QGraphicsSceneIndex::items(const QPolygonF &polygon, Qt::ItemSelectionMode mode,
                                                  Qt::SortOrder order, const QTransform &deviceTransform) const
{
    Q_D(const QGraphicsSceneIndex);
    QList<QGraphicsItem *> itemList;
    QRectF exposeRect = polygon.boundingRect();
    _q_adjustRect(&exposeRect);
    QPainterPath path;
    path.addPolygon(polygon);
    d->items_helper(exposeRect, &QtPrivate::intersect_path, &itemList, deviceTransform, mode, order, &path);
    return itemList;
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsSceneIndex::items(const QPainterPath
    &path, Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform
    &deviceTransform) const

    \overload

    Returns all visible items that, depending on \a mode, are either inside or
    intersect with the specified \a path and return a list sorted using \a order.

    The default value for \a mode is Qt::IntersectsItemShape; all items whose
    exact shape intersects with or is contained by \a path are returned.

    \a deviceTransform is the transformation apply to the view.

    This method use the estimation of the index (estimateItems) and refine
    the list to get an exact result. If you want to implement your own
    refinement algorithm you can reimplement this method.

    \sa estimateItems()

*/
QList<QGraphicsItem *> QGraphicsSceneIndex::items(const QPainterPath &path, Qt::ItemSelectionMode mode,
                                                  Qt::SortOrder order, const QTransform &deviceTransform) const
{
    Q_D(const QGraphicsSceneIndex);
    QList<QGraphicsItem *> itemList;
    QRectF exposeRect = path.controlPointRect();
    _q_adjustRect(&exposeRect);
    d->items_helper(exposeRect, &QtPrivate::intersect_path, &itemList, deviceTransform, mode, order, &path);
    return itemList;
}

/*!
    This virtual function return an estimation of items at position \a point.
    This method return a list sorted using \a order.
*/
QList<QGraphicsItem *> QGraphicsSceneIndex::estimateItems(const QPointF &point, Qt::SortOrder order) const
{
    return estimateItems(QRectF(point, QSize(1, 1)), order);
}

QList<QGraphicsItem *> QGraphicsSceneIndex::estimateTopLevelItems(const QRectF &rect, Qt::SortOrder order) const
{
    Q_D(const QGraphicsSceneIndex);
    Q_UNUSED(rect);
    QGraphicsScenePrivate *scened = d->scene->d_func();
    scened->ensureSortedTopLevelItems();
    if (order == Qt::DescendingOrder) {
        QList<QGraphicsItem *> sorted;
        const int numTopLevelItems = scened->topLevelItems.size();
        sorted.reserve(numTopLevelItems);
        for (int i = numTopLevelItems - 1; i >= 0; --i)
            sorted << scened->topLevelItems.at(i);
        return sorted;
    }
    return scened->topLevelItems;
}

/*!
    \fn QList<QGraphicsItem *> QGraphicsSceneIndex::items(Qt::SortOrder order = Qt::DescendingOrder) const

    This pure virtual function all items in the index and sort them using
    \a order.
*/


/*!
    Notifies the index that the scene's scene rect has changed. \a rect
    is thew new scene rect.

    \sa QGraphicsScene::sceneRect()
*/
void QGraphicsSceneIndex::updateSceneRect(const QRectF &rect)
{
    Q_UNUSED(rect);
}

/*!
    This virtual function removes all items in the scene index.
*/
void QGraphicsSceneIndex::clear()
{
    const QList<QGraphicsItem *> allItems = items();
    for (int i = 0 ; i < allItems.size(); ++i)
        removeItem(allItems.at(i));
}

/*!
    \fn virtual void QGraphicsSceneIndex::addItem(QGraphicsItem *item) = 0

    This pure virtual function inserts an \a item to the scene index.

    \sa removeItem(), deleteItem()
*/

/*!
    \fn virtual void QGraphicsSceneIndex::removeItem(QGraphicsItem *item) = 0

    This pure virtual function removes an \a item to the scene index.

    \sa addItem(), deleteItem()
*/

/*!
    This method is called when an \a item has been deleted.
    The default implementation call removeItem. Be carefull,
    if your implementation of removeItem use pure virtual method
    of QGraphicsItem like boundingRect(), then you should reimplement
    this method.

    \sa addItem(), removeItem()
*/
void QGraphicsSceneIndex::deleteItem(QGraphicsItem *item)
{
    removeItem(item);
}

/*!
    This virtual function is called by QGraphicsItem to notify the index
    that some part of the \a item 's state changes. By reimplementing this
    function, your can react to a change, and in some cases, (depending on \a
    change,) adjustments in the index can be made.

    \a change is the parameter of the item that is changing. \a value is the
    value that changed; the type of the value depends on \a change.

    The default implementation does nothing.

    \sa QGraphicsItem::GraphicsItemChange
*/
void QGraphicsSceneIndex::itemChange(const QGraphicsItem *item, QGraphicsItem::GraphicsItemChange change, const void *const value)
{
    Q_UNUSED(item);
    Q_UNUSED(change);
    Q_UNUSED(value);
}

/*!
    Notify the index for a geometry change of an \a item.

    \sa QGraphicsItem::prepareGeometryChange()
*/
void QGraphicsSceneIndex::prepareBoundingRectChange(const QGraphicsItem *item)
{
    Q_UNUSED(item);
}

QT_END_NAMESPACE

#include "moc_qgraphicssceneindex_p.cpp"
