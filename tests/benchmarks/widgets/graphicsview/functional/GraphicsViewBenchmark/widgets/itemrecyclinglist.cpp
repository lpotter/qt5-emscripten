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

#include <QDebug>
#include <QTime>

#include "itemrecyclinglist.h"
#include "listitemcontainer.h"
#include "abstractviewitem.h"
#include "recycledlistitem.h"
#include "theme.h"
#include "scrollbar.h"

ItemRecyclingList::ItemRecyclingList(const int itemBuffer, QGraphicsWidget * parent)
    : ItemRecyclingListView(parent),
      m_listModel(new ListModel(this))
{
    ListItemContainer *container = new ListItemContainer(itemBuffer, this, this);
    container->setParentItem(this);
    ItemRecyclingListView::setContainer(container);
    ItemRecyclingListView::setModel(m_listModel, new RecycledListItem(this));
    setObjectName("ItemRecyclingList");
    connect(Theme::p(), SIGNAL(themeChanged()), this, SLOT(themeChange()));

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

/* virtual */
ItemRecyclingList::~ItemRecyclingList()
{
}

/* virtual */
void ItemRecyclingList::insertItem(int index, RecycledListItem *item)
{
    if (index<0)
        index = 0;
    if (index > m_listModel->rowCount())
        index = m_listModel->rowCount();
    if (m_listModel && item)
        m_listModel->insert(index,item);

    updateListItemBackgrounds(index);
}

/* virtual */
void ItemRecyclingList::addItem(RecycledListItem *item)
{
    if (item)
        m_listModel->appendRow(item);

    const int index = m_listModel->rowCount()-1;
    updateListItemBackgrounds(index);
}

/* virtual */
void ItemRecyclingList::clear()
{
    m_listModel->clear();
}

/* virtual */
AbstractViewItem *ItemRecyclingList::takeItem(const int row)
{
    if (row < 0 || row >= m_listModel->rowCount() || !m_listModel)
        return 0;
    return m_listModel->takeItem(row);
}

/*virtual*/
void ItemRecyclingList::setItemPrototype(AbstractViewItem* prototype)
{
    ItemRecyclingListView::setItemPrototype(prototype);
}

void ItemRecyclingList::themeChange()
{
    const bool caching = listItemCaching();
    setListItemCaching(false);

    const QString iconName = Theme::p()->pixmapPath()+"contact_default_icon.svg";
    const int count = m_listModel->rowCount();

    for (int i=0; i<count; ++i)
    {
        RecycledListItem *ritem = m_listModel->item(i);
        if (ritem) {
            ListItem *item = ritem->item();

            // Update default icons
            const QString filename = item->icon(ListItem::LeftIcon)->fileName();
            if (filename.contains("contact_default_icon")) {
                item->icon(ListItem::LeftIcon)->setFileName(iconName);
            }

            // Update status icons
            QString statusIcon = item->icon(ListItem::RightIcon)->fileName();
            const int index = statusIcon.indexOf("contact_status");
            if (index != -1) {
                statusIcon.remove(0, index);
                item->icon(ListItem::RightIcon)->setFileName(Theme::p()->pixmapPath()+statusIcon);
            }

            // Update fonts
            item->setFont(Theme::p()->font(Theme::ContactName), ListItem::FirstPos);
            item->setFont(Theme::p()->font(Theme::ContactNumber), ListItem::SecondPos);
            item->setFont(Theme::p()->font(Theme::ContactEmail), ListItem::ThirdPos);

            // Update list dividers
            if (i%2) {
                item->setBackgroundBrush(Theme::p()->listItemBackgroundBrushOdd());
                item->setBackgroundOpacity(Theme::p()->listItemBackgroundOpacityOdd());
            }
            else {
                item->setBackgroundBrush(Theme::p()->listItemBackgroundBrushEven());
                item->setBackgroundOpacity(Theme::p()->listItemBackgroundOpacityEven());
            }

            // Update borders
            item->setBorderPen(Theme::p()->listItemBorderPen());
            item->setRounding(Theme::p()->listItemRounding());

            // Update icons
            item->icon(ListItem::LeftIcon)->setRotation(Theme::p()->iconRotation(ListItem::LeftIcon));
            item->icon(ListItem::RightIcon)->setRotation(Theme::p()->iconRotation(ListItem::RightIcon));
            item->icon(ListItem::LeftIcon)->setOpacityEffectEnabled(Theme::p()->isIconOpacityEffectEnabled(ListItem::LeftIcon));
            item->icon(ListItem::RightIcon)->setOpacityEffectEnabled(Theme::p()->isIconOpacityEffectEnabled(ListItem::RightIcon));
            item->icon(ListItem::LeftIcon)->setSmoothTransformationEnabled(Theme::p()->isIconSmoothTransformationEnabled(ListItem::LeftIcon));
            item->icon(ListItem::RightIcon)->setSmoothTransformationEnabled(Theme::p()->isIconSmoothTransformationEnabled(ListItem::RightIcon));
        }
    }
    updateViewContent();
    setListItemCaching(caching);
}

void ItemRecyclingList::keyPressEvent(QKeyEvent *event)
{
    static QTime keyPressInterval = QTime::currentTime();
    static qreal step = 0.0;
    static bool repeat = false;
    int interval = keyPressInterval.elapsed();

    ScrollBar* sb = verticalScrollBar();
    qreal currentValue = sb->sliderPosition();

    if(interval < 250 ) {
        if(!repeat) step = 0.0;
        step = step + 2.0;
        if(step > 100) step = 100;
        repeat = true;
    }
    else {
        step = 1.0;
        if(m_listModel->item(0)) m_listModel->item(0)->size().height();
            step = m_listModel->item(0)->size().height();
        repeat = false;
    }

    if(event->key() == Qt::Key_Up ) { //Up Arrow
        sb->setSliderPosition(currentValue - step);
    }

    if(event->key() == Qt::Key_Down ) { //Down Arrow
        sb->setSliderPosition(currentValue + step);
    }
    keyPressInterval.start();
}

bool ItemRecyclingList::listItemCaching() const
{
    ListItemContainer *container =
        static_cast<ListItemContainer *>(m_container);

    return container->listItemCaching();
}

void ItemRecyclingList::setListItemCaching(bool enabled)
{
    ListItemContainer *container =
        static_cast<ListItemContainer *>(m_container);
    container->setListItemCaching(enabled);
}

void ItemRecyclingList::updateListItemBackgrounds(int index)
{
    const int itemCount = m_listModel->rowCount();

    for (int i=index; i<itemCount; ++i)
    {
        RecycledListItem *ritem = m_listModel->item(i);
        if (ritem) {
            ListItem *item = ritem->item();
            if (i%2) {
                item->setBackgroundBrush(Theme::p()->listItemBackgroundBrushOdd());
                item->setBackgroundOpacity(Theme::p()->listItemBackgroundOpacityOdd());
            }
            else {
                item->setBackgroundBrush(Theme::p()->listItemBackgroundBrushEven());
                item->setBackgroundOpacity(Theme::p()->listItemBackgroundOpacityEven());
            }
        }
    }
}

void ItemRecyclingList::setTwoColumns(const bool enabled)
{
    if (twoColumns() == enabled)
        return;

    const bool caching = listItemCaching();
    setListItemCaching(false);

    m_container->setTwoColumns(enabled);
    refreshContainerGeometry();

    setListItemCaching(caching);
}

bool ItemRecyclingList::twoColumns()
{
    return m_container->twoColumns();
}

