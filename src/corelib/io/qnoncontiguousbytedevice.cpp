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

#include "qnoncontiguousbytedevice_p.h"
#include <qbuffer.h>
#include <qdebug.h>
#include <qfile.h>

QT_BEGIN_NAMESPACE

/*!
    \class QNonContiguousByteDevice
    \inmodule QtCore
    \brief A QNonContiguousByteDevice is a representation of a
    file, array or buffer that allows access with a read pointer.
    \since 4.6

    The goal of this class is to have a data representation that
    allows us to avoid doing a memcpy as we have to do with QIODevice.

    \sa QNonContiguousByteDeviceFactory

    \internal
*/
/*!
    \fn virtual const char* QNonContiguousByteDevice::readPointer(qint64 maximumLength, qint64 &len)

    Return a byte pointer for at most \a maximumLength bytes of that device.
    if \a maximumLength is -1, the caller does not care about the length and
    the device may return what it desires to.
    The actual number of bytes the pointer is valid for is returned in
    the \a len variable.
    \a len will be -1 if EOF or an error occurs.
    If it was really EOF can then afterwards be checked with atEnd()
    Returns 0 if it is not possible to read at that position.

    \sa atEnd()

    \internal
*/
/*!
    \fn virtual bool QNonContiguousByteDevice::advanceReadPointer(qint64 amount)

     will advance the internal read pointer by \a amount bytes.
     The old readPointer is invalid after this call.

    \sa readPointer()

    \internal
*/
/*!
    \fn virtual bool QNonContiguousByteDevice::atEnd() const

     Returns \c true if everything has been read and the read
     pointer cannot be advanced anymore.

    \sa readPointer(), advanceReadPointer(), reset()

    \internal
*/
/*!
    \fn virtual bool QNonContiguousByteDevice::reset()

    Moves the internal read pointer back to the beginning.
    Returns \c false if this was not possible.

    \sa atEnd()

    \internal
*/
/*!
    \fn virtual qint64 QNonContiguousByteDevice::size() const

    Returns the size of the complete device or -1 if unknown.
    May also return less/more than what can be actually read with readPointer()

    \internal
*/
/*!
    \fn void QNonContiguousByteDevice::readyRead()

    Emitted when there is data available

    \internal
*/
/*!
    \fn void QNonContiguousByteDevice::readProgress(qint64 current, qint64 total)

    Emitted when data has been "read" by advancing the read pointer

    \internal
*/

QNonContiguousByteDevice::QNonContiguousByteDevice() : QObject((QObject*)0)
{
}

QNonContiguousByteDevice::~QNonContiguousByteDevice()
{
}

// FIXME we should scrap this whole implementation and instead change the ByteArrayImpl to be able to cope with sub-arrays?
QNonContiguousByteDeviceBufferImpl::QNonContiguousByteDeviceBufferImpl(QBuffer *b) : QNonContiguousByteDevice()
{
    buffer = b;
    byteArray = QByteArray::fromRawData(buffer->buffer().constData() + buffer->pos(), buffer->size() - buffer->pos());
    arrayImpl = new QNonContiguousByteDeviceByteArrayImpl(&byteArray);
    arrayImpl->setParent(this);
    connect(arrayImpl, SIGNAL(readyRead()), SIGNAL(readyRead()));
    connect(arrayImpl, SIGNAL(readProgress(qint64,qint64)), SIGNAL(readProgress(qint64,qint64)));
}

QNonContiguousByteDeviceBufferImpl::~QNonContiguousByteDeviceBufferImpl()
{
}

const char* QNonContiguousByteDeviceBufferImpl::readPointer(qint64 maximumLength, qint64 &len)
{
    return arrayImpl->readPointer(maximumLength, len);
}

bool QNonContiguousByteDeviceBufferImpl::advanceReadPointer(qint64 amount)
{
    return arrayImpl->advanceReadPointer(amount);
}

bool QNonContiguousByteDeviceBufferImpl::atEnd() const
{
    return arrayImpl->atEnd();
}

bool QNonContiguousByteDeviceBufferImpl::reset()
{
    return arrayImpl->reset();
}

qint64 QNonContiguousByteDeviceBufferImpl::size() const
{
    return arrayImpl->size();
}

QNonContiguousByteDeviceByteArrayImpl::QNonContiguousByteDeviceByteArrayImpl(QByteArray *ba) : QNonContiguousByteDevice(), currentPosition(0)
{
    byteArray = ba;
}

QNonContiguousByteDeviceByteArrayImpl::~QNonContiguousByteDeviceByteArrayImpl()
{
}

const char* QNonContiguousByteDeviceByteArrayImpl::readPointer(qint64 maximumLength, qint64 &len)
{
    if (atEnd()) {
        len = -1;
        return 0;
    }

    if (maximumLength != -1)
        len = qMin(maximumLength, size() - currentPosition);
    else
        len = size() - currentPosition;

    return byteArray->constData() + currentPosition;
}

bool QNonContiguousByteDeviceByteArrayImpl::advanceReadPointer(qint64 amount)
{
    currentPosition += amount;
    emit readProgress(currentPosition, size());
    return true;
}

bool QNonContiguousByteDeviceByteArrayImpl::atEnd() const
{
    return currentPosition >= size();
}

bool QNonContiguousByteDeviceByteArrayImpl::reset()
{
    currentPosition = 0;
    return true;
}

qint64 QNonContiguousByteDeviceByteArrayImpl::size() const
{
    return byteArray->size();
}

qint64 QNonContiguousByteDeviceByteArrayImpl::pos() const
{
    return currentPosition;
}

QNonContiguousByteDeviceRingBufferImpl::QNonContiguousByteDeviceRingBufferImpl(QSharedPointer<QRingBuffer> rb)
    : QNonContiguousByteDevice(), currentPosition(0)
{
    ringBuffer = rb;
}

QNonContiguousByteDeviceRingBufferImpl::~QNonContiguousByteDeviceRingBufferImpl()
{
}

const char* QNonContiguousByteDeviceRingBufferImpl::readPointer(qint64 maximumLength, qint64 &len)
{
    if (atEnd()) {
        len = -1;
        return 0;
    }

    const char *returnValue = ringBuffer->readPointerAtPosition(currentPosition, len);

    if (maximumLength != -1)
        len = qMin(len, maximumLength);

    return returnValue;
}

bool QNonContiguousByteDeviceRingBufferImpl::advanceReadPointer(qint64 amount)
{
    currentPosition += amount;
    emit readProgress(currentPosition, size());
    return true;
}

bool QNonContiguousByteDeviceRingBufferImpl::atEnd() const
{
    return currentPosition >= size();
}

qint64 QNonContiguousByteDeviceRingBufferImpl::pos() const
{
    return currentPosition;
}

bool QNonContiguousByteDeviceRingBufferImpl::reset()
{
    currentPosition = 0;
    return true;
}

qint64 QNonContiguousByteDeviceRingBufferImpl::size() const
{
    return ringBuffer->size();
}

QNonContiguousByteDeviceIoDeviceImpl::QNonContiguousByteDeviceIoDeviceImpl(QIODevice *d)
    : QNonContiguousByteDevice(),
    currentReadBuffer(0), currentReadBufferSize(16*1024),
    currentReadBufferAmount(0), currentReadBufferPosition(0), totalAdvancements(0),
    eof(false)
{
    device = d;
    initialPosition = d->pos();
    connect(device, SIGNAL(readyRead()), this, SIGNAL(readyRead()), Qt::QueuedConnection);
    connect(device, SIGNAL(readChannelFinished()), this, SIGNAL(readyRead()), Qt::QueuedConnection);
}

QNonContiguousByteDeviceIoDeviceImpl::~QNonContiguousByteDeviceIoDeviceImpl()
{
    delete currentReadBuffer;
}

const char* QNonContiguousByteDeviceIoDeviceImpl::readPointer(qint64 maximumLength, qint64 &len)
{
    if (eof == true) {
        len = -1;
        return 0;
    }

    if (currentReadBuffer == 0)
        currentReadBuffer = new QByteArray(currentReadBufferSize, '\0'); // lazy alloc

    if (maximumLength == -1)
        maximumLength = currentReadBufferSize;

    if (currentReadBufferAmount - currentReadBufferPosition > 0) {
        len = currentReadBufferAmount - currentReadBufferPosition;
        return currentReadBuffer->data() + currentReadBufferPosition;
    }

    qint64 haveRead = device->read(currentReadBuffer->data(), qMin(maximumLength, currentReadBufferSize));

    if ((haveRead == -1) || (haveRead == 0 && device->atEnd() && !device->isSequential())) {
        eof = true;
        len = -1;
        // size was unknown before, emit a readProgress with the final size
        if (size() == -1)
            emit readProgress(totalAdvancements, totalAdvancements);
        return 0;
    }

    currentReadBufferAmount = haveRead;
    currentReadBufferPosition = 0;

    len = haveRead;
    return currentReadBuffer->data();
}

bool QNonContiguousByteDeviceIoDeviceImpl::advanceReadPointer(qint64 amount)
{
    totalAdvancements += amount;

    // normal advancement
    currentReadBufferPosition += amount;

    if (size() == -1)
        emit readProgress(totalAdvancements, totalAdvancements);
    else
        emit readProgress(totalAdvancements, size());

    // advancing over that what has actually been read before
    if (currentReadBufferPosition > currentReadBufferAmount) {
        qint64 i = currentReadBufferPosition - currentReadBufferAmount;
        while (i > 0) {
            if (device->getChar(0) == false) {
                emit readProgress(totalAdvancements - i, size());
                return false; // ### FIXME handle eof
            }
            i--;
        }

        currentReadBufferPosition = 0;
        currentReadBufferAmount = 0;
    }


    return true;
}

bool QNonContiguousByteDeviceIoDeviceImpl::atEnd() const
{
    return eof == true;
}

bool QNonContiguousByteDeviceIoDeviceImpl::reset()
{
    bool reset = (initialPosition == 0) ? device->reset() : device->seek(initialPosition);
    if (reset) {
        eof = false; // assume eof is false, it will be true after a read has been attempted
        totalAdvancements = 0; //reset the progress counter
        if (currentReadBuffer) {
            delete currentReadBuffer;
            currentReadBuffer = 0;
        }
        currentReadBufferAmount = 0;
        currentReadBufferPosition = 0;
        return true;
    }

    return false;
}

qint64 QNonContiguousByteDeviceIoDeviceImpl::size() const
{
    // note that this is different from the size() implementation of QIODevice!

    if (device->isSequential())
        return -1;

    return device->size() - initialPosition;
}

qint64 QNonContiguousByteDeviceIoDeviceImpl::pos() const
{
    if (device->isSequential())
        return -1;

    return device->pos();
}

QByteDeviceWrappingIoDevice::QByteDeviceWrappingIoDevice(QNonContiguousByteDevice *bd) : QIODevice((QObject*)0)
{
    byteDevice = bd;
    connect(bd, SIGNAL(readyRead()), SIGNAL(readyRead()));

    open(ReadOnly);
}

QByteDeviceWrappingIoDevice::~QByteDeviceWrappingIoDevice()
{

}

bool QByteDeviceWrappingIoDevice::isSequential() const
{
    return (byteDevice->size() == -1);
}

bool QByteDeviceWrappingIoDevice::atEnd() const
{
    return byteDevice->atEnd();
}

bool QByteDeviceWrappingIoDevice::reset()
{
    return byteDevice->reset();
}

qint64 QByteDeviceWrappingIoDevice::size() const
{
    if (isSequential())
        return 0;

    return byteDevice->size();
}


qint64 QByteDeviceWrappingIoDevice::readData( char * data, qint64 maxSize)
{
    qint64 len;
    const char *readPointer = byteDevice->readPointer(maxSize, len);
    if (len == -1)
        return -1;

    memcpy(data, readPointer, len);
    byteDevice->advanceReadPointer(len);
    return len;
}

qint64 QByteDeviceWrappingIoDevice::writeData( const char* data, qint64 maxSize)
{
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
}

/*!
    \class QNonContiguousByteDeviceFactory
    \inmodule QtCore
    \since 4.6

    Creates a QNonContiguousByteDevice out of a QIODevice,
    QByteArray etc.

    \sa QNonContiguousByteDevice

    \internal
*/

/*!
    \fn static QNonContiguousByteDevice* QNonContiguousByteDeviceFactory::create(QIODevice *device)

    Create a QNonContiguousByteDevice out of a QIODevice.
    For QFile, QBuffer and all other QIoDevice, sequential or not.

    \internal
*/
QNonContiguousByteDevice* QNonContiguousByteDeviceFactory::create(QIODevice *device)
{
    // shortcut if it is a QBuffer
    if (QBuffer* buffer = qobject_cast<QBuffer*>(device)) {
        return new QNonContiguousByteDeviceBufferImpl(buffer);
    }

    // ### FIXME special case if device is a QFile that supports map()
    // then we can actually deal with the file without using read/peek

    // generic QIODevice
    return new QNonContiguousByteDeviceIoDeviceImpl(device); // FIXME
}

/*!
    Create a QNonContiguousByteDevice out of a QIODevice, return it in a QSharedPointer.
    For QFile, QBuffer and all other QIODevice, sequential or not.

    \internal
*/
QSharedPointer<QNonContiguousByteDevice> QNonContiguousByteDeviceFactory::createShared(QIODevice *device)
{
    // shortcut if it is a QBuffer
    if (QBuffer *buffer = qobject_cast<QBuffer*>(device))
        return QSharedPointer<QNonContiguousByteDeviceBufferImpl>::create(buffer);

    // ### FIXME special case if device is a QFile that supports map()
    // then we can actually deal with the file without using read/peek

    // generic QIODevice
    return QSharedPointer<QNonContiguousByteDeviceIoDeviceImpl>::create(device); // FIXME
}

/*!
    \fn static QNonContiguousByteDevice* QNonContiguousByteDeviceFactory::create(QSharedPointer<QRingBuffer> ringBuffer)

    Create a QNonContiguousByteDevice out of a QRingBuffer.

    \internal
*/
QNonContiguousByteDevice* QNonContiguousByteDeviceFactory::create(QSharedPointer<QRingBuffer> ringBuffer)
{
    return new QNonContiguousByteDeviceRingBufferImpl(ringBuffer);
}

/*!
    Create a QNonContiguousByteDevice out of a QRingBuffer, return it in a QSharedPointer.

    \internal
*/
QSharedPointer<QNonContiguousByteDevice> QNonContiguousByteDeviceFactory::createShared(QSharedPointer<QRingBuffer> ringBuffer)
{
    return QSharedPointer<QNonContiguousByteDeviceRingBufferImpl>::create(qMove(ringBuffer));
}

/*!
    \fn static QNonContiguousByteDevice* QNonContiguousByteDeviceFactory::create(QByteArray *byteArray)

    Create a QNonContiguousByteDevice out of a QByteArray.

    \internal
*/
QNonContiguousByteDevice* QNonContiguousByteDeviceFactory::create(QByteArray *byteArray)
{
    return new QNonContiguousByteDeviceByteArrayImpl(byteArray);
}

/*!
    Create a QNonContiguousByteDevice out of a QByteArray.

    \internal
*/
QSharedPointer<QNonContiguousByteDevice> QNonContiguousByteDeviceFactory::createShared(QByteArray *byteArray)
{
    return QSharedPointer<QNonContiguousByteDeviceByteArrayImpl>::create(byteArray);
}

/*!
    \fn static QIODevice* QNonContiguousByteDeviceFactory::wrap(QNonContiguousByteDevice* byteDevice)

    Wrap the \a byteDevice (possibly again) into a QIODevice.

    \internal
*/
QIODevice* QNonContiguousByteDeviceFactory::wrap(QNonContiguousByteDevice* byteDevice)
{
    // ### FIXME if it already has been based on QIoDevice, we could that one out again
    // and save some calling

    // needed for FTP backend

    return new QByteDeviceWrappingIoDevice(byteDevice);
}

QT_END_NAMESPACE

#include "moc_qnoncontiguousbytedevice_p.cpp"
