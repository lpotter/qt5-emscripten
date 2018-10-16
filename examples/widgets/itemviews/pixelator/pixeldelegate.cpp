/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "pixeldelegate.h"

#include <QPainter>

//! [0]
PixelDelegate::PixelDelegate(QObject *parent)
    : QAbstractItemDelegate(parent)
{
    pixelSize = 12;
}
//! [0]

//! [1]
void PixelDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
//! [2]
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());
//! [1]

//! [3]
    int size = qMin(option.rect.width(), option.rect.height());
//! [3] //! [4]
    int brightness = index.model()->data(index, Qt::DisplayRole).toInt();
    double radius = (size / 2.0) - (brightness / 255.0 * size / 2.0);
    if (radius == 0.0)
        return;
//! [4]

//! [5]
    painter->save();
//! [5] //! [6]
    painter->setRenderHint(QPainter::Antialiasing, true);
//! [6] //! [7]
    painter->setPen(Qt::NoPen);
//! [7] //! [8]
    if (option.state & QStyle::State_Selected)
//! [8] //! [9]
        painter->setBrush(option.palette.highlightedText());
    else
//! [2]
        painter->setBrush(option.palette.text());
//! [9]

//! [10]
    painter->drawEllipse(QRectF(option.rect.x() + option.rect.width() / 2 - radius,
                                option.rect.y() + option.rect.height() / 2 - radius,
                                2 * radius, 2 * radius));
    painter->restore();
}
//! [10]

//! [11]
QSize PixelDelegate::sizeHint(const QStyleOptionViewItem & /* option */,
                              const QModelIndex & /* index */) const
{
    return QSize(pixelSize, pixelSize);
}
//! [11]

//! [12]
void PixelDelegate::setPixelSize(int size)
{
    pixelSize = size;
}
//! [12]
