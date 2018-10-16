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

#include "qgraphicslayoutstyleinfo_p.h"

#include "qgraphicslayout_p.h"
#include "qgraphicswidget.h"
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qapplication.h>

QT_BEGIN_NAMESPACE

QGraphicsLayoutStyleInfo::QGraphicsLayoutStyleInfo(const QGraphicsLayoutPrivate *layout)
    : m_layout(layout), m_style(0)
{
    m_widget = new QWidget; // pixelMetric might need a widget ptr
    if (m_widget)
        m_styleOption.initFrom(m_widget);
    m_isWindow = m_styleOption.state & QStyle::State_Window;
}

QGraphicsLayoutStyleInfo::~QGraphicsLayoutStyleInfo()
{
    delete m_widget;
}

qreal QGraphicsLayoutStyleInfo::combinedLayoutSpacing(QLayoutPolicy::ControlTypes controls1,
                                                      QLayoutPolicy::ControlTypes controls2,
                                                      Qt::Orientation orientation) const
{
    Q_ASSERT(style());
    return style()->combinedLayoutSpacing(QSizePolicy::ControlTypes(int(controls1)), QSizePolicy::ControlTypes(int(controls2)),
                                          orientation, const_cast<QStyleOption*>(&m_styleOption), widget());
}

qreal QGraphicsLayoutStyleInfo::perItemSpacing(QLayoutPolicy::ControlType control1,
                                               QLayoutPolicy::ControlType control2,
                                               Qt::Orientation orientation) const
{
    Q_ASSERT(style());
    return style()->layoutSpacing(QSizePolicy::ControlType(control1), QSizePolicy::ControlType(control2),
                                  orientation, const_cast<QStyleOption*>(&m_styleOption), widget());
}

qreal QGraphicsLayoutStyleInfo::spacing(Qt::Orientation orientation) const
{
    Q_ASSERT(style());
    return style()->pixelMetric(orientation == Qt::Horizontal ? QStyle::PM_LayoutHorizontalSpacing : QStyle::PM_LayoutVerticalSpacing);
}

qreal QGraphicsLayoutStyleInfo::windowMargin(Qt::Orientation orientation) const
{
    return style()->pixelMetric(orientation == Qt::Vertical
                                ? QStyle::PM_LayoutBottomMargin
                                : QStyle::PM_LayoutRightMargin,
                                const_cast<QStyleOption*>(&m_styleOption), widget());
}

QWidget *QGraphicsLayoutStyleInfo::widget() const { return m_widget; }

QStyle *QGraphicsLayoutStyleInfo::style() const
{
    if (!m_style) {
        Q_ASSERT(m_layout);
        QGraphicsItem *item = m_layout->parentItem();
        m_style = (item && item->isWidget()) ? static_cast<QGraphicsWidget*>(item)->style() : QApplication::style();
    }
    return m_style;
}

QT_END_NAMESPACE
