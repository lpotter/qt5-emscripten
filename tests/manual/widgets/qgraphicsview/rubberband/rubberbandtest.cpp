/****************************************************************************
**
** Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
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

#include <QtWidgets>

class MyGraphicsItem : public QGraphicsRectItem
{
public:
    MyGraphicsItem() : QGraphicsRectItem()
    {
        setFlags(QGraphicsItem::ItemIsSelectable);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem * /* option*/, QWidget * /*widget*/) override
    {
        if (isSelected())
            painter->fillRect(rect(), QColor(255, 0, 0));
        else
            painter->fillRect(rect(), QColor(0, 255, 0));
    }
};

class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    MyGraphicsView(QWidget *w, QLabel *l) : QGraphicsView(w), rubberbandLabel(l)
    {
        setDragMode(QGraphicsView::RubberBandDrag);
        connect(this, SIGNAL(rubberBandChanged(QRect, QPointF, QPointF)), this, SLOT(updateRubberbandInfo(QRect, QPointF, QPointF)));
    }
protected:
    void mouseMoveEvent(QMouseEvent *event) override
    {
        QGraphicsView::mouseMoveEvent(event);

        int rightmostInView = viewport()->mapToGlobal(viewport()->geometry().topRight()).x();
        int xglobal = event->globalX();
        if (xglobal > rightmostInView)
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + 10);

        int bottomPos = viewport()->mapToGlobal(viewport()->geometry().bottomRight()).y();
        int yglobal = event->globalY();
        if (yglobal > bottomPos)
            verticalScrollBar()->setValue(verticalScrollBar()->value() + 10);
    }

protected slots:
    void updateRubberbandInfo(QRect r, QPointF from, QPointF to)
    {
        QString textToShow;
        QDebug s(&textToShow);
        s << r << from << to;
        rubberbandLabel->setText(textToShow);
    }
protected:
    QLabel *rubberbandLabel;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget w;
    w.setLayout(new QVBoxLayout);
    QLabel *l = new QLabel(&w);
    MyGraphicsView *v = new MyGraphicsView(&w, l);

    w.layout()->addWidget(v);
    w.layout()->addWidget(l);

    QGraphicsScene s(0.0, 0.0, 5000.0, 5000.0);
    v->setScene(&s);
    v->setInteractive(true);
    v->setRubberBandSelectionMode(Qt::IntersectsItemBoundingRect);

    for (int u = 0; u < 100; ++u)
        for (int n = 0; n < 100; ++n) {
            MyGraphicsItem *item = new MyGraphicsItem();
            item->setRect(QRectF(n * 80.0, u * 80.0, 50.0, 20.0));
            s.addItem(item);
        }

    w.show();
    app.exec();
    return 0;
}

#include "rubberbandtest.moc"
