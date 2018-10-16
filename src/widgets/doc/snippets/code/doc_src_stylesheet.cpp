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

//! [21]
qApp->setStyleSheet("QPushButton { color: white }");
//! [21]


//! [22]
myPushButton->setStyleSheet("* { color: blue }");
//! [22]


//! [23]
myPushButton->setStyleSheet("color: blue");
//! [23]


//! [24]
qApp->setStyleSheet("QGroupBox { color: red; } ");
//! [24]

//! [25]
qApp->setStyleSheet("QGroupBox, QGroupBox * { color: red; }");
//! [25]


//! [26]
class MyPushButton : public QPushButton {
    // ...
}

// ...
qApp->setStyleSheet("MyPushButton { background: yellow; }");
//! [26]


//! [27]
namespace ns {
    class MyPushButton : public QPushButton {
        // ...
    }
}

// ...
qApp->setStyleSheet("ns--MyPushButton { background: yellow; }");
//! [27]


//! [32]
void CustomWidget::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
//! [32]


//! [88]
qApp->setStyleSheet("QLineEdit { background-color: yellow }");
//! [88]


//! [89]
myDialog->setStyleSheet("QLineEdit { background-color: yellow }");
//! [89]


//! [90]
myDialog->setStyleSheet("QLineEdit#nameEdit { background-color: yellow }");
//! [90]


//! [91]
nameEdit->setStyleSheet("background-color: yellow");
//! [91]


//! [92]
nameEdit->setStyleSheet("color: blue; background-color: yellow");
//! [92]


//! [93]
nameEdit->setStyleSheet("color: blue;"
                        "background-color: yellow;"
                        "selection-color: yellow;"
                        "selection-background-color: blue;");
//! [93]


//! [95]
QLineEdit *nameEdit = new QLineEdit(this);
nameEdit->setProperty("mandatoryField", true);

QLineEdit *emailEdit = new QLineEdit(this);
emailEdit->setProperty("mandatoryField", true);

QSpinBox *ageSpinBox = new QSpinBox(this);
ageSpinBox->setProperty("mandatoryField", true);
//! [95]

//! [96]
QCoreApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles, true);
//! [96]
