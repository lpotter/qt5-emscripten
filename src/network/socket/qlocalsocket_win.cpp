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

#include "qlocalsocket_p.h"

QT_BEGIN_NAMESPACE

void QLocalSocketPrivate::init()
{
    Q_Q(QLocalSocket);
    pipeReader = new QWindowsPipeReader(q);
    q->connect(pipeReader, SIGNAL(readyRead()), SIGNAL(readyRead()));
    q->connect(pipeReader, SIGNAL(pipeClosed()), SLOT(_q_pipeClosed()), Qt::QueuedConnection);
    q->connect(pipeReader, SIGNAL(winError(ulong,QString)), SLOT(_q_winError(ulong,QString)));
}

void QLocalSocketPrivate::setErrorString(const QString &function)
{
    DWORD windowsError = GetLastError();
    _q_winError(windowsError, function);
}

void QLocalSocketPrivate::_q_winError(ulong windowsError, const QString &function)
{
    Q_Q(QLocalSocket);
    QLocalSocket::LocalSocketState currentState = state;

    // If the connectToServer fails due to WaitNamedPipe() time-out, assume ConnectionError
    if (state == QLocalSocket::ConnectingState && windowsError == ERROR_SEM_TIMEOUT)
        windowsError = ERROR_NO_DATA;

    switch (windowsError) {
    case ERROR_PIPE_NOT_CONNECTED:
    case ERROR_BROKEN_PIPE:
    case ERROR_NO_DATA:
        error = QLocalSocket::ConnectionError;
        errorString = QLocalSocket::tr("%1: Connection error").arg(function);
        state = QLocalSocket::UnconnectedState;
        break;
    case ERROR_FILE_NOT_FOUND:
        error = QLocalSocket::ServerNotFoundError;
        errorString = QLocalSocket::tr("%1: Invalid name").arg(function);
        state = QLocalSocket::UnconnectedState;
        break;
    case ERROR_ACCESS_DENIED:
        error = QLocalSocket::SocketAccessError;
        errorString = QLocalSocket::tr("%1: Access denied").arg(function);
        state = QLocalSocket::UnconnectedState;
        break;
    default:
        error = QLocalSocket::UnknownSocketError;
        errorString = QLocalSocket::tr("%1: Unknown error %2").arg(function).arg(windowsError);
#if defined QLOCALSOCKET_DEBUG
        qWarning() << "QLocalSocket error not handled:" << errorString;
#endif
        state = QLocalSocket::UnconnectedState;
    }

    if (currentState != state) {
        q->emit stateChanged(state);
        if (state == QLocalSocket::UnconnectedState && currentState != QLocalSocket::ConnectingState)
            q->emit disconnected();
    }
    emit q->error(error);
}

QLocalSocketPrivate::QLocalSocketPrivate() : QIODevicePrivate(),
       handle(INVALID_HANDLE_VALUE),
       pipeWriter(0),
       pipeReader(0),
       error(QLocalSocket::UnknownSocketError),
       state(QLocalSocket::UnconnectedState)
{
    writeBufferChunkSize = QIODEVICE_BUFFERSIZE;
}

QLocalSocketPrivate::~QLocalSocketPrivate()
{
    destroyPipeHandles();
}

void QLocalSocketPrivate::destroyPipeHandles()
{
    if (handle != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(handle);
        CloseHandle(handle);
    }
}

void QLocalSocket::connectToServer(OpenMode openMode)
{
    Q_D(QLocalSocket);
    if (state() == ConnectedState || state() == ConnectingState) {
        setErrorString(tr("Trying to connect while connection is in progress"));
        emit error(QLocalSocket::OperationError);
        return;
    }

    d->error = QLocalSocket::UnknownSocketError;
    d->errorString = QString();
    d->state = ConnectingState;
    emit stateChanged(d->state);
    if (d->serverName.isEmpty()) {
        d->error = QLocalSocket::ServerNotFoundError;
        setErrorString(QLocalSocket::tr("%1: Invalid name").arg(QLatin1String("QLocalSocket::connectToServer")));
        d->state = UnconnectedState;
        emit error(d->error);
        emit stateChanged(d->state);
        return;
    }

    const QLatin1String pipePath("\\\\.\\pipe\\");
    if (d->serverName.startsWith(pipePath))
        d->fullServerName = d->serverName;
    else
        d->fullServerName = pipePath + d->serverName;
    // Try to open a named pipe
    HANDLE localSocket;
    forever {
        DWORD permissions = (openMode & QIODevice::ReadOnly) ? GENERIC_READ : 0;
        permissions |= (openMode & QIODevice::WriteOnly) ? GENERIC_WRITE : 0;
        localSocket = CreateFile(reinterpret_cast<const wchar_t *>(d->fullServerName.utf16()), // pipe name
                                 permissions,
                                 0,              // no sharing
                                 NULL,           // default security attributes
                                 OPEN_EXISTING,  // opens existing pipe
                                 FILE_FLAG_OVERLAPPED,
                                 NULL);          // no template file

        if (localSocket != INVALID_HANDLE_VALUE)
            break;
        DWORD error = GetLastError();
        // It is really an error only if it is not ERROR_PIPE_BUSY
        if (ERROR_PIPE_BUSY != error) {
            break;
        }

        // All pipe instances are busy, so wait until connected or up to 5 seconds.
        if (!WaitNamedPipe((const wchar_t *)d->fullServerName.utf16(), 5000))
            break;
    }

    if (localSocket == INVALID_HANDLE_VALUE) {
        d->setErrorString(QLatin1String("QLocalSocket::connectToServer"));
        d->fullServerName = QString();
        return;
    }

    // we have a valid handle
    if (setSocketDescriptor(reinterpret_cast<qintptr>(localSocket), ConnectedState, openMode))
        emit connected();
}

// This is reading from the buffer
qint64 QLocalSocket::readData(char *data, qint64 maxSize)
{
    Q_D(QLocalSocket);

    if (!maxSize)
        return 0;

    qint64 ret = d->pipeReader->read(data, maxSize);

    // QWindowsPipeReader::read() returns error codes that don't match what we need
    switch (ret) {
    case 0:     // EOF -> transform to error
        return -1;
    case -2:    // EWOULDBLOCK -> no error, just no bytes
        return 0;
    default:
        return ret;
    }
}

qint64 QLocalSocket::writeData(const char *data, qint64 len)
{
    Q_D(QLocalSocket);
    if (len == 0)
        return 0;
    d->writeBuffer.append(data, len);
    if (!d->pipeWriter) {
        d->pipeWriter = new QWindowsPipeWriter(d->handle, this);
        connect(d->pipeWriter, &QWindowsPipeWriter::bytesWritten,
                this, &QLocalSocket::bytesWritten);
        QObjectPrivate::connect(d->pipeWriter, &QWindowsPipeWriter::canWrite,
                                d, &QLocalSocketPrivate::_q_canWrite);
    }
    d->_q_canWrite();
    return len;
}

void QLocalSocket::abort()
{
    Q_D(QLocalSocket);
    if (d->pipeWriter) {
        delete d->pipeWriter;
        d->pipeWriter = 0;
    }
    close();
}

void QLocalSocketPrivate::_q_pipeClosed()
{
    Q_Q(QLocalSocket);
    if (state == QLocalSocket::UnconnectedState)
        return;

    emit q->readChannelFinished();
    if (state != QLocalSocket::ClosingState) {
        state = QLocalSocket::ClosingState;
        emit q->stateChanged(state);
        if (state != QLocalSocket::ClosingState)
            return;
    }
    state = QLocalSocket::UnconnectedState;
    emit q->stateChanged(state);
    emit q->disconnected();

    pipeReader->stop();
    destroyPipeHandles();
    handle = INVALID_HANDLE_VALUE;

    if (pipeWriter) {
        delete pipeWriter;
        pipeWriter = 0;
    }
}

qint64 QLocalSocket::bytesAvailable() const
{
    Q_D(const QLocalSocket);
    qint64 available = QIODevice::bytesAvailable();
    available += d->pipeReader->bytesAvailable();
    return available;
}

qint64 QLocalSocket::bytesToWrite() const
{
    Q_D(const QLocalSocket);
    return d->writeBuffer.size() + (d->pipeWriter ? d->pipeWriter->bytesToWrite() : 0);
}

bool QLocalSocket::canReadLine() const
{
    Q_D(const QLocalSocket);
    return QIODevice::canReadLine() || d->pipeReader->canReadLine();
}

void QLocalSocket::close()
{
    Q_D(QLocalSocket);
    if (openMode() == NotOpen)
        return;

    d->setWriteChannelCount(0);
    QIODevice::close();
    d->serverName = QString();
    d->fullServerName = QString();

    if (state() != UnconnectedState) {
        if (bytesToWrite() > 0) {
            disconnectFromServer();
            return;
        }

        d->_q_pipeClosed();
    }
}

bool QLocalSocket::flush()
{
    Q_D(QLocalSocket);
    bool written = false;
    while (d->pipeWriter && d->pipeWriter->waitForWrite(0))
        written = true;
    return written;
}

void QLocalSocket::disconnectFromServer()
{
    Q_D(QLocalSocket);

    // Are we still connected?
    if (!isValid()) {
        // If we have unwritten data, the pipeWriter is still present.
        // It must be destroyed before close() to prevent an infinite loop.
        delete d->pipeWriter;
        d->pipeWriter = 0;
        d->writeBuffer.clear();
    }

    flush();
    if (bytesToWrite() != 0) {
        d->state = QLocalSocket::ClosingState;
        emit stateChanged(d->state);
    } else {
        close();
    }
}

QLocalSocket::LocalSocketError QLocalSocket::error() const
{
    Q_D(const QLocalSocket);
    return d->error;
}

bool QLocalSocket::setSocketDescriptor(qintptr socketDescriptor,
              LocalSocketState socketState, OpenMode openMode)
{
    Q_D(QLocalSocket);
    d->pipeReader->stop();
    d->handle = reinterpret_cast<HANDLE>(socketDescriptor);
    d->state = socketState;
    d->pipeReader->setHandle(d->handle);
    QIODevice::open(openMode);
    emit stateChanged(d->state);
    if (d->state == ConnectedState && openMode.testFlag(QIODevice::ReadOnly))
        d->pipeReader->startAsyncRead();
    return true;
}

void QLocalSocketPrivate::_q_canWrite()
{
    Q_Q(QLocalSocket);
    if (writeBuffer.isEmpty()) {
        if (state == QLocalSocket::ClosingState)
            q->close();
    } else {
        Q_ASSERT(pipeWriter);
        if (!pipeWriter->isWriteOperationActive())
            pipeWriter->write(writeBuffer.read());
    }
}

qintptr QLocalSocket::socketDescriptor() const
{
    Q_D(const QLocalSocket);
    return reinterpret_cast<qintptr>(d->handle);
}

qint64 QLocalSocket::readBufferSize() const
{
    Q_D(const QLocalSocket);
    return d->pipeReader->maxReadBufferSize();
}

void QLocalSocket::setReadBufferSize(qint64 size)
{
    Q_D(QLocalSocket);
    d->pipeReader->setMaxReadBufferSize(size);
}

bool QLocalSocket::waitForConnected(int msecs)
{
    Q_UNUSED(msecs);
    return (state() == ConnectedState);
}

bool QLocalSocket::waitForDisconnected(int msecs)
{
    Q_D(QLocalSocket);
    if (state() == UnconnectedState) {
        qWarning("QLocalSocket::waitForDisconnected() is not allowed in UnconnectedState");
        return false;
    }
    if (!openMode().testFlag(QIODevice::ReadOnly)) {
        qWarning("QLocalSocket::waitForDisconnected isn't supported for write only pipes.");
        return false;
    }
    if (d->pipeReader->waitForPipeClosed(msecs)) {
        d->_q_pipeClosed();
        return true;
    }
    return false;
}

bool QLocalSocket::isValid() const
{
    Q_D(const QLocalSocket);
    return d->handle != INVALID_HANDLE_VALUE;
}

bool QLocalSocket::waitForReadyRead(int msecs)
{
    Q_D(QLocalSocket);

    if (d->state != QLocalSocket::ConnectedState)
        return false;

    // We already know that the pipe is gone, but did not enter the event loop yet.
    if (d->pipeReader->isPipeClosed()) {
        d->_q_pipeClosed();
        return false;
    }

    bool result = d->pipeReader->waitForReadyRead(msecs);

    // We just noticed that the pipe is gone.
    if (d->pipeReader->isPipeClosed())
        d->_q_pipeClosed();

    return result;
}

bool QLocalSocket::waitForBytesWritten(int msecs)
{
    Q_D(const QLocalSocket);
    if (!d->pipeWriter)
        return false;

    // Wait for the pipe writer to acknowledge that it has
    // written. This will succeed if either the pipe writer has
    // already written the data, or if it manages to write data
    // within the given timeout.
    return d->pipeWriter->waitForWrite(msecs);
}

QT_END_NAMESPACE
