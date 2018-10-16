/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "certificateinfo.h"
#include "sslclient.h"

#include "ui_sslclient.h"
#include "ui_sslerrors.h"

#include <QtCore>

SslClient::SslClient(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupSecureSocket();
}

SslClient::~SslClient()
{
    delete form;
}

void SslClient::updateEnabledState()
{
    const bool unconnected = socket->state() == QAbstractSocket::UnconnectedState;
    form->hostNameEdit->setReadOnly(!unconnected);
    form->hostNameEdit->setFocusPolicy(unconnected ? Qt::StrongFocus : Qt::NoFocus);
    form->hostNameLabel->setEnabled(unconnected);
    form->portBox->setEnabled(unconnected);
    form->portLabel->setEnabled(unconnected);
    form->connectButton->setEnabled(unconnected && !form->hostNameEdit->text().isEmpty());

    const bool connected = socket->state() == QAbstractSocket::ConnectedState;
    form->sessionOutput->setEnabled(connected);
    form->sessionInput->setEnabled(connected);
    form->sessionInputLabel->setEnabled(connected);
    form->sendButton->setEnabled(connected);
}

void SslClient::secureConnect()
{
    socket->connectToHostEncrypted(form->hostNameEdit->text(), form->portBox->value());
    updateEnabledState();
}

void SslClient::socketStateChanged(QAbstractSocket::SocketState state)
{
    if (executingDialog)
        return;

    updateEnabledState();

    if (state == QAbstractSocket::UnconnectedState) {
        form->sessionInput->clear();
        form->hostNameEdit->setPalette(QPalette());
        form->hostNameEdit->setFocus();
        form->cipherLabel->setText(tr("<none>"));
        padLock->hide();
    }
}

void SslClient::socketEncrypted()
{
    form->sessionOutput->clear();
    form->sessionInput->setFocus();

    QPalette palette;
    palette.setColor(QPalette::Base, QColor(255, 255, 192));
    form->hostNameEdit->setPalette(palette);

    const QSslCipher cipher = socket->sessionCipher();
    const QString cipherInfo = QString("%1, %2 (%3/%4)").arg(cipher.authenticationMethod())
                                       .arg(cipher.name()).arg(cipher.usedBits())
                                       .arg(cipher.supportedBits());;
    form->cipherLabel->setText(cipherInfo);
    padLock->show();
}

void SslClient::socketReadyRead()
{
    appendString(QString::fromUtf8(socket->readAll()));
}

void SslClient::sendData()
{
    const QString input = form->sessionInput->text();
    appendString(input + '\n');
    socket->write(input.toUtf8() + "\r\n");
    form->sessionInput->clear();
}

void SslClient::socketError(QAbstractSocket::SocketError)
{
    if (handlingSocketError)
        return;

    handlingSocketError = true;
    QMessageBox::critical(this, tr("Connection error"), socket->errorString());
    handlingSocketError = false;
}

void SslClient::sslErrors(const QList<QSslError> &errors)
{
    QDialog errorDialog(this);
    Ui_SslErrors ui;
    ui.setupUi(&errorDialog);
    connect(ui.certificateChainButton, &QPushButton::clicked,
            this, &SslClient::displayCertificateInfo);

    for (const auto &error : errors)
        ui.sslErrorList->addItem(error.errorString());

    executingDialog = true;
    if (errorDialog.exec() == QDialog::Accepted)
        socket->ignoreSslErrors();
    executingDialog = false;

    // did the socket state change?
    if (socket->state() != QAbstractSocket::ConnectedState)
        socketStateChanged(socket->state());
}

void SslClient::displayCertificateInfo()
{
    CertificateInfo info;
    info.setCertificateChain(socket->peerCertificateChain());
    info.exec();
}

void SslClient::setupUi()
{
    if (form)
        return;

    form = new Ui_Form;
    form->setupUi(this);
    form->hostNameEdit->setSelection(0, form->hostNameEdit->text().size());
    form->sessionOutput->setHtml(tr("&lt;not connected&gt;"));

    connect(form->hostNameEdit, SIGNAL(textChanged(QString)),
            this, SLOT(updateEnabledState()));
    connect(form->connectButton, SIGNAL(clicked()),
            this, SLOT(secureConnect()));
    connect(form->sendButton, SIGNAL(clicked()),
            this, SLOT(sendData()));

    padLock = new QToolButton;
    padLock->setIcon(QIcon(":/encrypted.png"));
    connect(padLock, SIGNAL(clicked()), this, SLOT(displayCertificateInfo()));

#if QT_CONFIG(cursor)
    padLock->setCursor(Qt::ArrowCursor);
#endif
    padLock->setToolTip(tr("Display encryption details."));

    const int extent = form->hostNameEdit->height() - 2;
    padLock->resize(extent, extent);
    padLock->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);

    QHBoxLayout *layout = new QHBoxLayout(form->hostNameEdit);
    layout->setMargin(form->hostNameEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth));
    layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(padLock);

    form->hostNameEdit->setLayout(layout);
    padLock->hide();
}

void SslClient::setupSecureSocket()
{
    if (socket)
        return;

    socket = new QSslSocket(this);

    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(encrypted()),
            this, SLOT(socketEncrypted()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(sslErrors(QList<QSslError>)));
    connect(socket, SIGNAL(readyRead()),
            this, SLOT(socketReadyRead()));

}

void SslClient::appendString(const QString &line)
{
    QTextCursor cursor(form->sessionOutput->textCursor());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(line);
    form->sessionOutput->verticalScrollBar()->setValue(form->sessionOutput->verticalScrollBar()->maximum());
}
