/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Keith Gardner <kreios4004@gmail.com>
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

#include <QVersionNumber>

class Object
{
public:
    static void genericExample();
    static void equalityExample();
    static void isPrefixOf();
    static void parse();
    static void equivalent();
};

void Object::genericExample()
{
    //! [0]
    QVersionNumber version(1, 2, 3);  // 1.2.3
    //! [0]
}

void Object::equalityExample()
{
    //! [1]
    QVersionNumber v1(1, 2);
    QVersionNumber v2(1, 2, 0);
    int compare = QVersionNumber::compare(v1, v2); // compare == -1
    //! [1]
}

void Object::isPrefixOf()
{
    //! [2]
    QVersionNumber v1(5, 3);
    QVersionNumber v2(5, 3, 1);
    bool value = v1.isPrefixOf(v2); // true
    //! [2]
}

void QObject::parse()
{
    //! [3]
    QString string("5.4.0-alpha");
    int suffixIndex;
    QVersionNumber version = QVersionNumber::fromString(string, &suffixIndex);
    // version is 5.4.0
    // suffixIndex is 5
    //! [3]

    //! [3-latin1-1]
    QLatin1String string("5.4.0-alpha");
    int suffixIndex;
    auto version = QVersionNumber::fromString(string, &suffixIndex);
    // version is 5.4.0
    // suffixIndex is 5
    //! [3-latin1-1]
}

void Object::equivalent()
{
    //! [4]
    QVersionNumber v1(5, 4);
    QVersionNumber v2(5, 4, 0);
    bool equivalent = v1.normalized() == v2.normalized();
    bool equal = v1 == v2;
    // equivalent is true
    // equal is false
    //! [4]
}

int main()
{
    Object::genericExample();
    Object::equalityExample();
    Object::isPrefixOf();
    Object::parse();
    Object::equivalent();
}
