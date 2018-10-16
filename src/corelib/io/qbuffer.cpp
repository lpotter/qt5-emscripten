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

#include "qbuffer.h"
#include <QtCore/qmetaobject.h>
#include "private/qiodevice_p.h"

QT_BEGIN_NAMESPACE

/** QBufferPrivate **/
class QBufferPrivate : public QIODevicePrivate
{
    Q_DECLARE_PUBLIC(QBuffer)

public:
    QBufferPrivate()
    : buf(0)
#ifndef QT_NO_QOBJECT
        , writtenSinceLastEmit(0), signalConnectionCount(0), signalsEmitted(false)
#endif
    { }
    ~QBufferPrivate() { }

    QByteArray *buf;
    QByteArray defaultBuf;

    virtual qint64 peek(char *data, qint64 maxSize) override;
    virtual QByteArray peek(qint64 maxSize) override;

#ifndef QT_NO_QOBJECT
    // private slots
    void _q_emitSignals();

    qint64 writtenSinceLastEmit;
    int signalConnectionCount;
    bool signalsEmitted;
#endif
};

#ifndef QT_NO_QOBJECT
void QBufferPrivate::_q_emitSignals()
{
    Q_Q(QBuffer);
    emit q->bytesWritten(writtenSinceLastEmit);
    writtenSinceLastEmit = 0;
    emit q->readyRead();
    signalsEmitted = false;
}
#endif

qint64 QBufferPrivate::peek(char *data, qint64 maxSize)
{
    qint64 readBytes = qMin(maxSize, static_cast<qint64>(buf->size()) - pos);
    memcpy(data, buf->constData() + pos, readBytes);
    return readBytes;
}

QByteArray QBufferPrivate::peek(qint64 maxSize)
{
    qint64 readBytes = qMin(maxSize, static_cast<qint64>(buf->size()) - pos);
    if (pos == 0 && maxSize >= buf->size())
        return *buf;
    return QByteArray(buf->constData() + pos, readBytes);
}

/*!
    \class QBuffer
    \inmodule QtCore
    \reentrant
    \brief The QBuffer class provides a QIODevice interface for a QByteArray.

    \ingroup io

    QBuffer allows you to access a QByteArray using the QIODevice
    interface. The QByteArray is treated just as a standard random-accessed
    file. Example:

    \snippet buffer/buffer.cpp 0

    By default, an internal QByteArray buffer is created for you when
    you create a QBuffer. You can access this buffer directly by
    calling buffer(). You can also use QBuffer with an existing
    QByteArray by calling setBuffer(), or by passing your array to
    QBuffer's constructor.

    Call open() to open the buffer. Then call write() or
    putChar() to write to the buffer, and read(), readLine(),
    readAll(), or getChar() to read from it. size() returns the
    current size of the buffer, and you can seek to arbitrary
    positions in the buffer by calling seek(). When you are done with
    accessing the buffer, call close().

    The following code snippet shows how to write data to a
    QByteArray using QDataStream and QBuffer:

    \snippet buffer/buffer.cpp 1

    Effectively, we convert the application's QPalette into a byte
    array. Here's how to read the data from the QByteArray:

    \snippet buffer/buffer.cpp 2

    QTextStream and QDataStream also provide convenience constructors
    that take a QByteArray and that create a QBuffer behind the
    scenes.

    QBuffer emits readyRead() when new data has arrived in the
    buffer. By connecting to this signal, you can use QBuffer to
    store temporary data before processing it. QBuffer also emits
    bytesWritten() every time new data has been written to the buffer.

    \sa QFile, QDataStream, QTextStream, QByteArray
*/

#ifdef QT_NO_QOBJECT
QBuffer::QBuffer()
    : QIODevice(*new QBufferPrivate)
{
    Q_D(QBuffer);
    d->buf = &d->defaultBuf;
}
QBuffer::QBuffer(QByteArray *buf)
    : QIODevice(*new QBufferPrivate)
{
    Q_D(QBuffer);
    d->buf = buf ? buf : &d->defaultBuf;
    d->defaultBuf.clear();
}
#else
/*!
    Constructs an empty buffer with the given \a parent. You can call
    setData() to fill the buffer with data, or you can open it in
    write mode and use write().

    \sa open()
*/
QBuffer::QBuffer(QObject *parent)
    : QIODevice(*new QBufferPrivate, parent)
{
    Q_D(QBuffer);
    d->buf = &d->defaultBuf;
}

/*!
    Constructs a QBuffer that uses the QByteArray pointed to by \a
    byteArray as its internal buffer, and with the given \a parent.
    The caller is responsible for ensuring that \a byteArray remains
    valid until the QBuffer is destroyed, or until setBuffer() is
    called to change the buffer. QBuffer doesn't take ownership of
    the QByteArray.

    If you open the buffer in write-only mode or read-write mode and
    write something into the QBuffer, \a byteArray will be modified.

    Example:

    \snippet buffer/buffer.cpp 3

    \sa open(), setBuffer(), setData()
*/
QBuffer::QBuffer(QByteArray *byteArray, QObject *parent)
    : QIODevice(*new QBufferPrivate, parent)
{
    Q_D(QBuffer);
    d->buf = byteArray ? byteArray : &d->defaultBuf;
    d->defaultBuf.clear();
}
#endif

/*!
    Destroys the buffer.
*/

QBuffer::~QBuffer()
{
}

/*!
    Makes QBuffer uses the QByteArray pointed to by \a
    byteArray as its internal buffer. The caller is responsible for
    ensuring that \a byteArray remains valid until the QBuffer is
    destroyed, or until setBuffer() is called to change the buffer.
    QBuffer doesn't take ownership of the QByteArray.

    Does nothing if isOpen() is true.

    If you open the buffer in write-only mode or read-write mode and
    write something into the QBuffer, \a byteArray will be modified.

    Example:

    \snippet buffer/buffer.cpp 4

    If \a byteArray is 0, the buffer creates its own internal
    QByteArray to work on. This byte array is initially empty.

    \sa buffer(), setData(), open()
*/

void QBuffer::setBuffer(QByteArray *byteArray)
{
    Q_D(QBuffer);
    if (isOpen()) {
        qWarning("QBuffer::setBuffer: Buffer is open");
        return;
    }
    if (byteArray) {
        d->buf = byteArray;
    } else {
        d->buf = &d->defaultBuf;
    }
    d->defaultBuf.clear();
}

/*!
    Returns a reference to the QBuffer's internal buffer. You can use
    it to modify the QByteArray behind the QBuffer's back.

    \sa setBuffer(), data()
*/

QByteArray &QBuffer::buffer()
{
    Q_D(QBuffer);
    return *d->buf;
}

/*!
    \overload

    This is the same as data().
*/

const QByteArray &QBuffer::buffer() const
{
    Q_D(const QBuffer);
    return *d->buf;
}


/*!
    Returns the data contained in the buffer.

    This is the same as buffer().

    \sa setData(), setBuffer()
*/

const QByteArray &QBuffer::data() const
{
    Q_D(const QBuffer);
    return *d->buf;
}

/*!
    Sets the contents of the internal buffer to be \a data. This is
    the same as assigning \a data to buffer().

    Does nothing if isOpen() is true.

    \sa setBuffer()
*/
void QBuffer::setData(const QByteArray &data)
{
    Q_D(QBuffer);
    if (isOpen()) {
        qWarning("QBuffer::setData: Buffer is open");
        return;
    }
    *d->buf = data;
}

/*!
    \fn void QBuffer::setData(const char *data, int size)

    \overload

    Sets the contents of the internal buffer to be the first \a size
    bytes of \a data.
*/

/*!
   \reimp
*/
bool QBuffer::open(OpenMode flags)
{
    Q_D(QBuffer);

    if ((flags & (Append | Truncate)) != 0)
        flags |= WriteOnly;
    if ((flags & (ReadOnly | WriteOnly)) == 0) {
        qWarning("QBuffer::open: Buffer access not specified");
        return false;
    }

    if ((flags & Truncate) == Truncate)
        d->buf->resize(0);

    return QIODevice::open(flags | QIODevice::Unbuffered);
}

/*!
    \reimp
*/
void QBuffer::close()
{
    QIODevice::close();
}

/*!
    \reimp
*/
qint64 QBuffer::pos() const
{
    return QIODevice::pos();
}

/*!
    \reimp
*/
qint64 QBuffer::size() const
{
    Q_D(const QBuffer);
    return qint64(d->buf->size());
}

/*!
    \reimp
*/
bool QBuffer::seek(qint64 pos)
{
    Q_D(QBuffer);
    if (pos > d->buf->size() && isWritable()) {
        if (seek(d->buf->size())) {
            const qint64 gapSize = pos - d->buf->size();
            if (write(QByteArray(gapSize, 0)) != gapSize) {
                qWarning("QBuffer::seek: Unable to fill gap");
                return false;
            }
        } else {
            return false;
        }
    } else if (pos > d->buf->size() || pos < 0) {
        qWarning("QBuffer::seek: Invalid pos: %d", int(pos));
        return false;
    }
    return QIODevice::seek(pos);
}

/*!
    \reimp
*/
bool QBuffer::atEnd() const
{
    return QIODevice::atEnd();
}

/*!
   \reimp
*/
bool QBuffer::canReadLine() const
{
    Q_D(const QBuffer);
    if (!isOpen())
        return false;

    return d->buf->indexOf('\n', int(pos())) != -1 || QIODevice::canReadLine();
}

/*!
    \reimp
*/
qint64 QBuffer::readData(char *data, qint64 len)
{
    Q_D(QBuffer);
    if ((len = qMin(len, qint64(d->buf->size()) - pos())) <= 0)
        return qint64(0);
    memcpy(data, d->buf->constData() + pos(), len);
    return len;
}

/*!
    \reimp
*/
qint64 QBuffer::writeData(const char *data, qint64 len)
{
    Q_D(QBuffer);
    int extraBytes = pos() + len - d->buf->size();
    if (extraBytes > 0) { // overflow
        int newSize = d->buf->size() + extraBytes;
        d->buf->resize(newSize);
        if (d->buf->size() != newSize) { // could not resize
            qWarning("QBuffer::writeData: Memory allocation error");
            return -1;
        }
    }

    memcpy(d->buf->data() + pos(), data, int(len));

#ifndef QT_NO_QOBJECT
    d->writtenSinceLastEmit += len;
    if (d->signalConnectionCount && !d->signalsEmitted && !signalsBlocked()) {
        d->signalsEmitted = true;
        QMetaObject::invokeMethod(this, "_q_emitSignals", Qt::QueuedConnection);
    }
#endif
    return len;
}

#ifndef QT_NO_QOBJECT
/*!
    \reimp
    \internal
*/
void QBuffer::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod readyReadSignal = QMetaMethod::fromSignal(&QBuffer::readyRead);
    static const QMetaMethod bytesWrittenSignal = QMetaMethod::fromSignal(&QBuffer::bytesWritten);
    if (signal == readyReadSignal || signal == bytesWrittenSignal)
        d_func()->signalConnectionCount++;
}

/*!
    \reimp
    \internal
*/
void QBuffer::disconnectNotify(const QMetaMethod &signal)
{
    if (signal.isValid()) {
        static const QMetaMethod readyReadSignal = QMetaMethod::fromSignal(&QBuffer::readyRead);
        static const QMetaMethod bytesWrittenSignal = QMetaMethod::fromSignal(&QBuffer::bytesWritten);
        if (signal == readyReadSignal || signal == bytesWrittenSignal)
            d_func()->signalConnectionCount--;
    } else {
        d_func()->signalConnectionCount = 0;
    }
}
#endif

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
# include "moc_qbuffer.cpp"
#endif

