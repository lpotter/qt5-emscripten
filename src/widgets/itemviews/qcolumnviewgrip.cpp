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

#include "qcolumnviewgrip_p.h"
#include <qstyleoption.h>
#include <qpainter.h>
#include <qbrush.h>
#include <qevent.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*
    \internal
    class QColumnViewGrip

    QColumnViewGrip is created to go inside QAbstractScrollArea's corner.
    When the mouse it moved it will resize the scroll area and emit's a signal.
 */

/*
    \internal
    \fn void QColumnViewGrip::gripMoved()
    Signal that is emitted when the grip moves the parent widget.
 */

/*!
    Creates a new QColumnViewGrip with the given \a parent to view a model.
    Use setModel() to set the model.
*/
QColumnViewGrip::QColumnViewGrip(QWidget *parent)
:  QWidget(*new QColumnViewGripPrivate, parent, 0)
{
#ifndef QT_NO_CURSOR
    setCursor(Qt::SplitHCursor);
#endif
}

/*!
  \internal
*/
QColumnViewGrip::QColumnViewGrip(QColumnViewGripPrivate & dd, QWidget *parent, Qt::WindowFlags f)
:  QWidget(dd, parent, f)
{
}

/*!
  Destroys the view.
*/
QColumnViewGrip::~QColumnViewGrip()
{
}

/*!
    Attempt to resize the parent object by \a offset
    returns the amount of offset that it was actually able to resized
*/
int QColumnViewGrip::moveGrip(int offset)
{
    QWidget *parentWidget = (QWidget*)parent();

    // first resize the parent
    int oldWidth = parentWidget->width();
    int newWidth = oldWidth;
    if (isRightToLeft())
       newWidth -= offset;
    else
       newWidth += offset;
    newWidth = qMax(parentWidget->minimumWidth(), newWidth);
    parentWidget->resize(newWidth, parentWidget->height());

    // Then have the view move the widget
    int realOffset = parentWidget->width() - oldWidth;
    int oldX = parentWidget->x();
    if (realOffset != 0)
        emit gripMoved(realOffset);
    if (isRightToLeft())
        realOffset = -1 * (oldX - parentWidget->x());
    return realOffset;
}

/*!
    \reimp
*/
void QColumnViewGrip::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawControl(QStyle::CE_ColumnViewGrip, &opt, &painter, this);
    event->accept();
}

/*!
    \reimp
    Resize the parent window to the sizeHint
*/
void QColumnViewGrip::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    QWidget *parentWidget = (QWidget*)parent();
    int offset = parentWidget->sizeHint().width() - parentWidget->width();
    if (isRightToLeft())
        offset *= -1;
    moveGrip(offset);
    event->accept();
}

/*!
    \reimp
    Begin watching for mouse movements
*/
void QColumnViewGrip::mousePressEvent(QMouseEvent *event)
{
    Q_D(QColumnViewGrip);
    d->originalXLocation = event->globalX();
    event->accept();
}

/*!
    \reimp
    Calculate the movement of the grip and moveGrip() and emit gripMoved
*/
void QColumnViewGrip::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QColumnViewGrip);
    int offset = event->globalX() - d->originalXLocation;
    d->originalXLocation = moveGrip(offset) + d->originalXLocation;
    event->accept();
}

/*!
    \reimp
    Stop watching for mouse movements
*/
void QColumnViewGrip::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QColumnViewGrip);
    d->originalXLocation = -1;
    event->accept();
}

/*
 * private object implementation
 */
QColumnViewGripPrivate::QColumnViewGripPrivate()
:  QWidgetPrivate(),
originalXLocation(-1)
{
}

QT_END_NAMESPACE

#include "moc_qcolumnviewgrip_p.cpp"
