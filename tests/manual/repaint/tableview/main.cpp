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

#include "../shared/shared.h"

#include <QApplication>
#include <QMainWindow>
#include <QTableWidget>
#include <QPaintEvent>

class CellWidget : public QWidget
{
public:
    CellWidget(QWidget *parent = 0) : QWidget(parent) {}
    void paintEvent(QPaintEvent * event)
    {
        static int value = 200;
        value = (value + 41) % 205 + 50;
        QPainter p(this);
        p.fillRect(event->rect(), QColor::fromHsv(100, 255, value));
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTableWidget tableWidget;
//    tableWidget.setAttribute(Qt::WA_StaticContents);
    tableWidget.viewport()->setAttribute(Qt::WA_StaticContents);
    tableWidget.setRowCount(15);
    tableWidget.setColumnCount(4);
    for (int row = 0; row  < 15; ++row)
    for (int col = 0; col  < 4; ++col)
//        tableWidget.setCellWidget(row, col, new StaticWidget());
       tableWidget.setCellWidget(row, col, new CellWidget());
    tableWidget.resize(400, 600);
    tableWidget.show();

    return app.exec();
}


