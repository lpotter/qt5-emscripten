/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

class StaticWidget : public QWidget
{
Q_OBJECT
public:
    int hue;
    bool pressed;
    StaticWidget(QWidget *parent = 0)
    :QWidget(parent)
    {
        setAttribute(Qt::WA_StaticContents);
        setAttribute(Qt::WA_OpaquePaintEvent);
           hue = 200;
        pressed = false;
    }

    // Update 4 rects in a checkerboard pattern, using either
    // a QRegion or separate rects (see the useRegion switch)
    void updatePattern(QPoint pos)
    {
        const int rectSize = 10;
        QRect rect(pos.x() - rectSize, pos.y() - rectSize, rectSize *2, rectSize * 2);

        QVector<QRect> updateRects;
        updateRects.append(rect.translated(rectSize * 2, rectSize * 2));
        updateRects.append(rect.translated(rectSize * 2, -rectSize * 2));
        updateRects.append(rect.translated(-rectSize * 2, rectSize * 2));
        updateRects.append(rect.translated(-rectSize * 2, -rectSize * 2));


        bool useRegion = false;
        if (useRegion) {
            QRegion region;
            region.setRects(updateRects.data(), 4);
            update(region);
        } else {
            foreach (QRect rect, updateRects)
                update(rect);
        }
    }


    void resizeEvent(QResizeEvent *)
    {
  //      qDebug() << "static widget resize from" << e->oldSize() << "to" << e->size();
    }

    void mousePressEvent(QMouseEvent *event)
    {
//        qDebug() << "mousePress at" << event->pos();
        pressed = true;
        updatePattern(event->pos());
    }

    void mouseReleaseEvent(QMouseEvent *)
    {
        pressed = false;
    }

    void mouseMoveEvent(QMouseEvent *event)
    {
        if (pressed)
            updatePattern(event->pos());
    }

    void paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        static int color = 200;
        color = (color + 41) % 205 + 50;
//        color = ((color + 45) %150) + 100;
        qDebug() << "static widget repaint" << e->rect();
        if (pressed)
            p.fillRect(e->rect(), QColor::fromHsv(100, 255, color));
        else
            p.fillRect(e->rect(), QColor::fromHsv(hue, 255, color));
        p.setPen(QPen(QColor(Qt::white)));

        for (int y = e->rect().top(); y <= e->rect().bottom() + 1; ++y) {
            if (y % 20 == 0)
                p.drawLine(e->rect().left(), y, e->rect().right(), y);
        }

        for (int x = e->rect().left(); x <= e->rect().right() +1 ; ++x) {
            if (x % 20 == 0)
                p.drawLine(x, e->rect().top(), x, e->rect().bottom());
        }
    }
};
