/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qwindowspipewriter_p.h"
#include "qiodevice_p.h"

QT_BEGIN_NAMESPACE

QWindowsPipeWriter::Overlapped::Overlapped(QWindowsPipeWriter *pipeWriter)
    : pipeWriter(pipeWriter)
{
}

void QWindowsPipeWriter::Overlapped::clear()
{
    ZeroMemory(this, sizeof(OVERLAPPED));
}


QWindowsPipeWriter::QWindowsPipeWriter(HANDLE pipeWriteEnd, QObject *parent)
    : QObject(parent),
      handle(pipeWriteEnd),
      overlapped(nullptr),
      pendingBytesWrittenValue(0),
      stopped(true),
      writeSequenceStarted(false),
      notifiedCalled(false),
      bytesWrittenPending(false),
      inBytesWritten(false)
{
    connect(this, &QWindowsPipeWriter::_q_queueBytesWritten,
            this, &QWindowsPipeWriter::emitPendingBytesWrittenValue, Qt::QueuedConnection);
}

QWindowsPipeWriter::~QWindowsPipeWriter()
{
    stop();
    delete overlapped;
}

bool QWindowsPipeWriter::waitForWrite(int msecs)
{
    if (bytesWrittenPending) {
        emitPendingBytesWrittenValue();
        return true;
    }

    if (!writeSequenceStarted)
        return false;

    if (!waitForNotification(msecs))
        return false;

    if (bytesWrittenPending) {
        emitPendingBytesWrittenValue();
        return true;
    }

    return false;
}

qint64 QWindowsPipeWriter::bytesToWrite() const
{
    return buffer.size() + pendingBytesWrittenValue;
}

void QWindowsPipeWriter::emitPendingBytesWrittenValue()
{
    if (bytesWrittenPending) {
        // Reset the state even if we don't emit bytesWritten().
        // It's a defined behavior to not re-emit this signal recursively.
        bytesWrittenPending = false;
        const qint64 bytes = pendingBytesWrittenValue;
        pendingBytesWrittenValue = 0;

        emit canWrite();
        if (!inBytesWritten) {
            inBytesWritten = true;
            emit bytesWritten(bytes);
            inBytesWritten = false;
        }
    }
}

void QWindowsPipeWriter::writeFileCompleted(DWORD errorCode, DWORD numberOfBytesTransfered,
                                            OVERLAPPED *overlappedBase)
{
    Overlapped *overlapped = static_cast<Overlapped *>(overlappedBase);
    if (overlapped->pipeWriter)
        overlapped->pipeWriter->notified(errorCode, numberOfBytesTransfered);
    else
        delete overlapped;
}

/*!
    \internal
    Will be called whenever the write operation completes.
 */
void QWindowsPipeWriter::notified(DWORD errorCode, DWORD numberOfBytesWritten)
{
    notifiedCalled = true;
    writeSequenceStarted = false;
    Q_ASSERT(errorCode != ERROR_SUCCESS || numberOfBytesWritten == DWORD(buffer.size()));
    buffer.clear();

    switch (errorCode) {
    case ERROR_SUCCESS:
        break;
    case ERROR_OPERATION_ABORTED:
        if (stopped)
            break;
        Q_FALLTHROUGH();
    default:
        qErrnoWarning(errorCode, "QWindowsPipeWriter: asynchronous write failed.");
        break;
    }

    // After the writer was stopped, the only reason why this function can be called is the
    // completion of a cancellation. No signals should be emitted, and no new write sequence should
    // be started in this case.
    if (stopped)
        return;

    pendingBytesWrittenValue += qint64(numberOfBytesWritten);
    if (!bytesWrittenPending) {
        bytesWrittenPending = true;
        emit _q_queueBytesWritten(QWindowsPipeWriter::QPrivateSignal());
    }
}

bool QWindowsPipeWriter::waitForNotification(int timeout)
{
    QElapsedTimer t;
    t.start();
    notifiedCalled = false;
    int msecs = timeout;
    while (SleepEx(msecs == -1 ? INFINITE : msecs, TRUE) == WAIT_IO_COMPLETION) {
        if (notifiedCalled)
            return true;

        // Some other I/O completion routine was called. Wait some more.
        msecs = qt_subtract_from_timeout(timeout, t.elapsed());
        if (!msecs)
            break;
    }
    return notifiedCalled;
}

bool QWindowsPipeWriter::write(const QByteArray &ba)
{
    if (writeSequenceStarted)
        return false;

    if (!overlapped)
        overlapped = new Overlapped(this);
    overlapped->clear();
    buffer = ba;
    stopped = false;
    writeSequenceStarted = true;
    if (!WriteFileEx(handle, buffer.constData(), buffer.size(),
                     overlapped, &writeFileCompleted)) {
        writeSequenceStarted = false;
        buffer.clear();

        const DWORD errorCode = GetLastError();
        switch (errorCode) {
        case ERROR_NO_DATA:     // "The pipe is being closed."
            // The other end has closed the pipe. This can happen in QLocalSocket. Do not warn.
            break;
        default:
            qErrnoWarning(errorCode, "QWindowsPipeWriter::write failed.");
        }
        return false;
    }

    return true;
}

void QWindowsPipeWriter::stop()
{
    stopped = true;
    bytesWrittenPending = false;
    pendingBytesWrittenValue = 0;
    if (writeSequenceStarted) {
        overlapped->pipeWriter = nullptr;
        if (!CancelIoEx(handle, overlapped)) {
            const DWORD dwError = GetLastError();
            if (dwError != ERROR_NOT_FOUND) {
                qErrnoWarning(dwError, "QWindowsPipeWriter: CancelIoEx on handle %p failed.",
                              handle);
            }
        }
        overlapped = nullptr;       // The object will be deleted in the I/O callback.
        writeSequenceStarted = false;
    }
}

QT_END_NAMESPACE
