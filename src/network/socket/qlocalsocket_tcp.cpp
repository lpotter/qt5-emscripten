/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlocalsocket.h"
#include "qlocalsocket_p.h"
#include "qlocalserver.h"

#include <qhostaddress.h>
#include <qsettings.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

QLocalSocketPrivate::QLocalSocketPrivate() : QIODevicePrivate(),
        tcpSocket(0),
        ownsTcpSocket(true),
        state(QLocalSocket::UnconnectedState)
{
}

void QLocalSocketPrivate::init()
{
    setSocket(new QLocalUnixSocket);
}

void QLocalSocketPrivate::setSocket(QLocalUnixSocket* socket)
{
    if (ownsTcpSocket)
        delete tcpSocket;
    ownsTcpSocket = false;
    tcpSocket = socket;

    Q_Q(QLocalSocket);
    // QIODevice signals
    q->connect(tcpSocket, SIGNAL(aboutToClose()), q, SIGNAL(aboutToClose()));
    q->connect(tcpSocket, SIGNAL(bytesWritten(qint64)),
               q, SIGNAL(bytesWritten(qint64)));
    q->connect(tcpSocket, SIGNAL(readyRead()), q, SIGNAL(readyRead()));
    // QAbstractSocket signals
    q->connect(tcpSocket, SIGNAL(connected()), q, SIGNAL(connected()));
    q->connect(tcpSocket, SIGNAL(disconnected()), q, SIGNAL(disconnected()));
    q->connect(tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
               q, SLOT(_q_stateChanged(QAbstractSocket::SocketState)));
    q->connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
               q, SLOT(_q_error(QAbstractSocket::SocketError)));
    q->connect(tcpSocket, SIGNAL(readChannelFinished()), q, SIGNAL(readChannelFinished()));
    tcpSocket->setParent(q);
}

qint64 QLocalSocketPrivate::skip(qint64 maxSize)
{
    return tcpSocket->skip(maxSize);
}

void QLocalSocketPrivate::_q_error(QAbstractSocket::SocketError socketError)
{
    Q_Q(QLocalSocket);
    QString function = QLatin1String("QLocalSocket");
    QLocalSocket::LocalSocketError error = (QLocalSocket::LocalSocketError)socketError;
    QString errorString = generateErrorString(error, function);
    q->setErrorString(errorString);
    emit q->error(error);
}

void QLocalSocketPrivate::_q_stateChanged(QAbstractSocket::SocketState newState)
{
    Q_Q(QLocalSocket);
    QLocalSocket::LocalSocketState currentState = state;
    switch(newState) {
    case QAbstractSocket::UnconnectedState:
        state = QLocalSocket::UnconnectedState;
        serverName.clear();
        fullServerName.clear();
        break;
    case QAbstractSocket::ConnectingState:
        state = QLocalSocket::ConnectingState;
        break;
    case QAbstractSocket::ConnectedState:
        state = QLocalSocket::ConnectedState;
        break;
    case QAbstractSocket::ClosingState:
        state = QLocalSocket::ClosingState;
        break;
    default:
#if defined QLOCALSOCKET_DEBUG
        qWarning() << "QLocalSocket::Unhandled socket state change:" << newState;
#endif
        return;
    }
    if (currentState != state)
        emit q->stateChanged(state);
}

QString QLocalSocketPrivate::generateErrorString(QLocalSocket::LocalSocketError error, const QString &function) const
{
    QString errorString;
    switch (error) {
    case QLocalSocket::ConnectionRefusedError:
        errorString = QLocalSocket::tr("%1: Connection refused").arg(function);
        break;
    case QLocalSocket::PeerClosedError:
        errorString = QLocalSocket::tr("%1: Remote closed").arg(function);
        break;
    case QLocalSocket::ServerNotFoundError:
        errorString = QLocalSocket::tr("%1: Invalid name").arg(function);
        break;
    case QLocalSocket::SocketAccessError:
        errorString = QLocalSocket::tr("%1: Socket access error").arg(function);
        break;
    case QLocalSocket::SocketResourceError:
        errorString = QLocalSocket::tr("%1: Socket resource error").arg(function);
        break;
    case QLocalSocket::SocketTimeoutError:
        errorString = QLocalSocket::tr("%1: Socket operation timed out").arg(function);
        break;
    case QLocalSocket::DatagramTooLargeError:
        errorString = QLocalSocket::tr("%1: Datagram too large").arg(function);
        break;
    case QLocalSocket::ConnectionError:
        errorString = QLocalSocket::tr("%1: Connection error").arg(function);
        break;
    case QLocalSocket::UnsupportedSocketOperationError:
        errorString = QLocalSocket::tr("%1: The socket operation is not supported").arg(function);
        break;
    case QLocalSocket::OperationError:
        errorString = QLocalSocket::tr("%1: Operation not permitted when socket is in this state").arg(function);
        break;
    case QLocalSocket::UnknownSocketError:
    default:
        errorString = QLocalSocket::tr("%1: Unknown error").arg(function);
    }
    return errorString;
}

void QLocalSocketPrivate::errorOccurred(QLocalSocket::LocalSocketError error, const QString &function)
{
    Q_Q(QLocalSocket);
    switch (error) {
    case QLocalSocket::ConnectionRefusedError:
        tcpSocket->setSocketError(QAbstractSocket::ConnectionRefusedError);
        break;
    case QLocalSocket::PeerClosedError:
        tcpSocket->setSocketError(QAbstractSocket::RemoteHostClosedError);
        break;
    case QLocalSocket::ServerNotFoundError:
        tcpSocket->setSocketError(QAbstractSocket::HostNotFoundError);
        break;
    case QLocalSocket::SocketAccessError:
        tcpSocket->setSocketError(QAbstractSocket::SocketAccessError);
        break;
    case QLocalSocket::SocketResourceError:
        tcpSocket->setSocketError(QAbstractSocket::SocketResourceError);
        break;
    case QLocalSocket::SocketTimeoutError:
        tcpSocket->setSocketError(QAbstractSocket::SocketTimeoutError);
        break;
    case QLocalSocket::DatagramTooLargeError:
        tcpSocket->setSocketError(QAbstractSocket::DatagramTooLargeError);
        break;
    case QLocalSocket::ConnectionError:
        tcpSocket->setSocketError(QAbstractSocket::NetworkError);
        break;
    case QLocalSocket::UnsupportedSocketOperationError:
        tcpSocket->setSocketError(QAbstractSocket::UnsupportedSocketOperationError);
        break;
    case QLocalSocket::UnknownSocketError:
    default:
        tcpSocket->setSocketError(QAbstractSocket::UnknownSocketError);
    }

    QString errorString = generateErrorString(error, function);
    q->setErrorString(errorString);
    emit q->error(error);

    // errors cause a disconnect
    tcpSocket->setSocketState(QAbstractSocket::UnconnectedState);
    bool stateChanged = (state != QLocalSocket::UnconnectedState);
    state = QLocalSocket::UnconnectedState;
    q->close();
    if (stateChanged)
        q->emit stateChanged(state);
}

void QLocalSocket::connectToServer(OpenMode openMode)
{
    Q_D(QLocalSocket);
    if (state() == ConnectedState || state() == ConnectingState) {
        setErrorString(tr("Trying to connect while connection is in progress"));
        emit error(QLocalSocket::OperationError);
        return;
    }

    d->errorString.clear();
    d->state = ConnectingState;
    emit stateChanged(d->state);

    if (d->serverName.isEmpty()) {
        d->errorOccurred(ServerNotFoundError,
                         QLatin1String("QLocalSocket::connectToServer"));
        return;
    }

    const QLatin1String prefix("QLocalServer/");
    if (d->serverName.startsWith(prefix))
        d->fullServerName = d->serverName;
    else
        d->fullServerName = prefix + d->serverName;

    QSettings settings(QLatin1String("QtProject"), QLatin1String("Qt"));
    bool ok;
    const quint16 port = settings.value(d->fullServerName).toUInt(&ok);
    if (!ok) {
        d->errorOccurred(ServerNotFoundError,
                         QLatin1String("QLocalSocket::connectToServer"));
        return;
    }
    QIODevice::open(openMode);
    d->tcpSocket->connectToHost(QHostAddress::LocalHost, port, openMode);
}

bool QLocalSocket::setSocketDescriptor(qintptr socketDescriptor,
        LocalSocketState socketState, OpenMode openMode)
{
    Q_D(QLocalSocket);
    QAbstractSocket::SocketState newSocketState = QAbstractSocket::UnconnectedState;
    switch (socketState) {
    case ConnectingState:
        newSocketState = QAbstractSocket::ConnectingState;
        break;
    case ConnectedState:
        newSocketState = QAbstractSocket::ConnectedState;
        break;
    case ClosingState:
        newSocketState = QAbstractSocket::ClosingState;
        break;
    case UnconnectedState:
        newSocketState = QAbstractSocket::UnconnectedState;
        break;
    }
    QIODevice::open(openMode);
    d->state = socketState;

    // Is our parent a localServer? Then it wants us to use its remote socket.
    QLocalServer* localServer = qobject_cast<QLocalServer*>( parent() );
    if (localServer) {
        for (QObject* child : localServer->children()) {
            QTcpSocket* childTcpSocket = qobject_cast<QTcpSocket*>(child);
            if (childTcpSocket && childTcpSocket->socketDescriptor() == socketDescriptor) {
                d->setSocket( static_cast<QLocalUnixSocket*>(childTcpSocket) );
                return true;
            }
        }
    }

    // We couldn't find the socket in the children list of our server.
    // So it might be that the user wants to set a socket descriptor.
    return d->tcpSocket->setSocketDescriptor(socketDescriptor,
                                             newSocketState, openMode);
}

qintptr QLocalSocket::socketDescriptor() const
{
    Q_D(const QLocalSocket);
    return d->tcpSocket->socketDescriptor();
}

qint64 QLocalSocket::readData(char *data, qint64 c)
{
    Q_D(QLocalSocket);
    return d->tcpSocket->read(data, c);
}

qint64 QLocalSocket::writeData(const char *data, qint64 c)
{
    Q_D(QLocalSocket);
    return d->tcpSocket->writeData(data, c);
}

void QLocalSocket::abort()
{
    Q_D(QLocalSocket);
    d->tcpSocket->abort();
}

qint64 QLocalSocket::bytesAvailable() const
{
    Q_D(const QLocalSocket);
    return QIODevice::bytesAvailable() + d->tcpSocket->bytesAvailable();
}

qint64 QLocalSocket::bytesToWrite() const
{
    Q_D(const QLocalSocket);
    return d->tcpSocket->bytesToWrite();
}

bool QLocalSocket::canReadLine() const
{
    Q_D(const QLocalSocket);
    return QIODevice::canReadLine() || d->tcpSocket->canReadLine();
}

void QLocalSocket::close()
{
    Q_D(QLocalSocket);
    d->tcpSocket->close();
    d->serverName.clear();
    d->fullServerName.clear();
    QIODevice::close();
}

bool QLocalSocket::waitForBytesWritten(int msecs)
{
    Q_D(QLocalSocket);
    return d->tcpSocket->waitForBytesWritten(msecs);
}

bool QLocalSocket::flush()
{
    Q_D(QLocalSocket);
    return d->tcpSocket->flush();
}

void QLocalSocket::disconnectFromServer()
{
    Q_D(QLocalSocket);
    d->tcpSocket->disconnectFromHost();
}

QLocalSocket::LocalSocketError QLocalSocket::error() const
{
    Q_D(const QLocalSocket);
    switch (d->tcpSocket->error()) {
    case QAbstractSocket::ConnectionRefusedError:
        return QLocalSocket::ConnectionRefusedError;
    case QAbstractSocket::RemoteHostClosedError:
        return QLocalSocket::PeerClosedError;
    case QAbstractSocket::HostNotFoundError:
        return QLocalSocket::ServerNotFoundError;
    case QAbstractSocket::SocketAccessError:
        return QLocalSocket::SocketAccessError;
    case QAbstractSocket::SocketResourceError:
        return QLocalSocket::SocketResourceError;
    case QAbstractSocket::SocketTimeoutError:
        return QLocalSocket::SocketTimeoutError;
    case QAbstractSocket::DatagramTooLargeError:
        return QLocalSocket::DatagramTooLargeError;
    case QAbstractSocket::NetworkError:
        return QLocalSocket::ConnectionError;
    case QAbstractSocket::UnsupportedSocketOperationError:
        return QLocalSocket::UnsupportedSocketOperationError;
    case QAbstractSocket::UnknownSocketError:
        return QLocalSocket::UnknownSocketError;
    default:
#if defined QLOCALSOCKET_DEBUG
        qWarning() << "QLocalSocket error not handled:" << d->tcpSocket->error();
#endif
        break;
    }
    return UnknownSocketError;
}

bool QLocalSocket::isValid() const
{
    Q_D(const QLocalSocket);
    return d->tcpSocket->isValid();
}

qint64 QLocalSocket::readBufferSize() const
{
    Q_D(const QLocalSocket);
    return d->tcpSocket->readBufferSize();
}

void QLocalSocket::setReadBufferSize(qint64 size)
{
    Q_D(QLocalSocket);
    d->tcpSocket->setReadBufferSize(size);
}

bool QLocalSocket::waitForConnected(int msec)
{
    Q_D(QLocalSocket);
    if (state() != ConnectingState)
        return (state() == ConnectedState);

    return d->tcpSocket->waitForConnected(msec);
}

bool QLocalSocket::waitForDisconnected(int msecs)
{
    Q_D(QLocalSocket);
    if (state() == UnconnectedState) {
        qWarning("QLocalSocket::waitForDisconnected() is not allowed in UnconnectedState");
        return false;
    }
    return (d->tcpSocket->waitForDisconnected(msecs));
}

bool QLocalSocket::waitForReadyRead(int msecs)
{
    Q_D(QLocalSocket);
    if (state() == QLocalSocket::UnconnectedState)
        return false;
    return (d->tcpSocket->waitForReadyRead(msecs));
}

QT_END_NAMESPACE
