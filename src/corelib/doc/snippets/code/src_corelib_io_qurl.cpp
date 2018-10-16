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

//! [0]
QUrl url("http://www.example.com/List of holidays.xml");
// url.toEncoded() == "http://www.example.com/List%20of%20holidays.xml"
//! [0]


//! [1]
QUrl url = QUrl::fromEncoded("http://qt-project.org/List%20of%20holidays.xml");
//! [1]


//! [2]
bool checkUrl(const QUrl &url) {
    if (!url.isValid()) {
        qDebug("Invalid URL: %s", qUtf8Printable(url.toString()));
        return false;
    }

    return true;
}
//! [2]


//! [3]
QTcpSocket sock;
sock.connectToHost(url.host(), url.port(80));
//! [3]


//! [4]
http://www.example.com/cgi-bin/drawgraph.cgi?type(pie)color(green)
//! [4]


//! [5]
QUrl baseUrl("http://qt.digia.com/Support/");
QUrl relativeUrl("../Product/Library/");
qDebug(baseUrl.resolved(relativeUrl).toString());
// prints "http://qt.digia.com/Product/Library/"
//! [5]


//! [6]
QByteArray ba = QUrl::toPercentEncoding("{a fishy string?}", "{}", "s");
qDebug(ba.constData());
// prints "{a fi%73hy %73tring%3F}"
//! [6]

//! [7]
QUrl url("http://qt-project.org/support/file.html");
// url.adjusted(RemoveFilename) == "http://qt-project.org/support/"
// url.fileName() == "file.html"
//! [7]
