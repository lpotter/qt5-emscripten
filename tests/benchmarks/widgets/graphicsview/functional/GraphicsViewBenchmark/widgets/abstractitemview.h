/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#ifndef ABSTRACTITEMVIEW_H
#define ABSTRACTITEMVIEW_H

#include <QAbstractItemModel>
#include <QGraphicsSceneResizeEvent>
#include <QPersistentModelIndex>
#include <QItemSelection>

#include "listitemcontainer.h"
#include "abstractscrollarea.h"
#include "scroller.h"

class QItemSelectionModel;

class AbstractItemView : public AbstractScrollArea
{
    Q_OBJECT
public:
    AbstractItemView(QGraphicsWidget *parent = 0);
    virtual ~AbstractItemView();
    virtual void setContainer(AbstractItemContainer *container);
    virtual void setModel(QAbstractItemModel *model, AbstractViewItem *prototype);
    virtual QAbstractItemModel* model() const;
    virtual void setItemPrototype(AbstractViewItem* prototype);

    void setSelectionModel(QItemSelectionModel *smodel);

    virtual QModelIndex nextIndex(const QModelIndex &index) const;
    virtual QModelIndex previousIndex(const QModelIndex &index) const;

    virtual int indexCount() const;

    void refreshContainerGeometry(); // TODO Can this be moved to scroll area?

    void updateViewContent();
    virtual void scrollContentsBy(qreal dx, qreal dy);

    virtual bool listItemCaching() const = 0;
    virtual void setListItemCaching(bool enabled) = 0;

protected:
    virtual bool event(QEvent *e);
    void changeTheme();

public slots:
    virtual void setRootIndex(const QModelIndex &index);
    void setCurrentIndex(const QModelIndex &index,
                         QItemSelectionModel::SelectionFlags selectionFlag = QItemSelectionModel::NoUpdate);
protected slots:
    virtual void currentIndexChanged(const QModelIndex &current, const QModelIndex &previous);
    virtual void currentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    virtual void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    virtual void rowsAboutToBeInserted(const QModelIndex &index, int start, int end);
    virtual void rowsInserted(const QModelIndex &parent, int start, int end);
    virtual void rowsAboutToBeRemoved(const QModelIndex &index,int start, int end);
    virtual void rowsRemoved(const QModelIndex &parent,int start, int end);
    virtual void modelDestroyed();
    virtual void layoutChanged();
    virtual void reset();

protected:

    QAbstractItemModel *m_model;
    QPersistentModelIndex m_rootIndex;
    AbstractItemContainer *m_container;
    QItemSelectionModel *m_selectionModel;
    QPersistentModelIndex m_currentIndex;

private:
    Q_DISABLE_COPY(AbstractItemView)
    Scroller m_scroller;
};


#endif // ABSTRACTITEMVIEW_H
