/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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

#include <QtGui>

#include "mainwindow.h"

MainWindow::MainWindow()
{
    QMenu *fileMenu = new QMenu(tr("&File"));

    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcut(tr("Ctrl+Q"));

    QMenu *tableMenu = new QMenu(tr("&Table"));

    QAction *tableWidthAction = tableMenu->addAction(tr("Change Table &Width"));
    QAction *tableHeightAction = tableMenu->addAction(tr("Change Table &Height"));

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(tableMenu);

//! [0]
    tableWidget = new QTableWidget(this);
//! [0]
    tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
    connect(tableWidthAction, SIGNAL(triggered()), this, SLOT(changeWidth()));
    connect(tableHeightAction, SIGNAL(triggered()), this, SLOT(changeHeight()));

    setupTableItems();

    setCentralWidget(tableWidget);
    setWindowTitle(tr("Table Widget Resizing"));
}

void MainWindow::setupTableItems()
{
//! [1]
    tableWidget->setRowCount(10);
    tableWidget->setColumnCount(5);
//! [1]

    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        for (int column = 0; column < tableWidget->columnCount(); ++column) {
//! [2]
    QTableWidgetItem *newItem = new QTableWidgetItem(tr("%1").arg(
        (row+1)*(column+1)));
    tableWidget->setItem(row, column, newItem);
//! [2]
        }
    }
}

void MainWindow::changeWidth()
{
    bool ok;

    int newWidth = QInputDialog::getInteger(this, tr("Change table width"),
        tr("Input the number of columns required (1-20):"),
        tableWidget->columnCount(), 1, 20, 1, &ok);

    if (ok)
        tableWidget->setColumnCount(newWidth);
}

void MainWindow::changeHeight()
{
    bool ok;

    int newHeight = QInputDialog::getInteger(this, tr("Change table height"),
        tr("Input the number of rows required (1-20):"),
        tableWidget->rowCount(), 1, 20, 1, &ok);

    if (ok)
        tableWidget->setRowCount(newHeight);
}
