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

#include <QApplication>
#include <QPushButton>
#include <QFileInfo>
#include <QDir>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

//! [0]
    QFileInfo fileInfo1("~/examples/191697/.");
    QFileInfo fileInfo2("~/examples/191697/..");
    QFileInfo fileInfo3("~/examples/191697/main.cpp");
//! [0]
//! [1]
    QFileInfo fileInfo4(".");
    QFileInfo fileInfo5("..");
    QFileInfo fileInfo6("main.cpp");
//! [1]

    qDebug() << fileInfo1.fileName();
    qDebug() << fileInfo2.fileName();
    qDebug() << fileInfo3.fileName();
    qDebug() << fileInfo4.fileName();
    qDebug() << fileInfo5.fileName();
    qDebug() << fileInfo6.fileName();

    QPushButton* button1 = new QPushButton(fileInfo1.dir().path());
    QPushButton* button2 = new QPushButton(fileInfo2.dir().path());
    QPushButton* button3 = new QPushButton(fileInfo3.dir().path());
    QPushButton* button4 = new QPushButton(fileInfo4.dir().path());
    QPushButton* button5 = new QPushButton(fileInfo5.dir().path());
    QPushButton* button6 = new QPushButton(fileInfo6.dir().path());

    QVBoxLayout* vbox = new QVBoxLayout;
    vbox->addWidget(button1);
    vbox->addWidget(button2);
    vbox->addWidget(button3);
    vbox->addWidget(button4);
    vbox->addWidget(button5);
    vbox->addWidget(button6);
    vbox->addStretch(1);

    QGroupBox *groupBox = new QGroupBox("QFileInfo::dir() test");
    groupBox->setLayout(vbox);
    groupBox->show();

    return app.exec();
}
