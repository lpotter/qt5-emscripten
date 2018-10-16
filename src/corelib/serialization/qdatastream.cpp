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

#include "qdatastream.h"
#include "qdatastream_p.h"

#if !defined(QT_NO_DATASTREAM) || defined(QT_BOOTSTRAPPED)
#include "qbuffer.h"
#include "qfloat16.h"
#include "qstring.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "qendian.h"

QT_BEGIN_NAMESPACE

/*!
    \class QDataStream
    \inmodule QtCore
    \reentrant
    \brief The QDataStream class provides serialization of binary data
    to a QIODevice.

    \ingroup io


    A data stream is a binary stream of encoded information which is
    100% independent of the host computer's operating system, CPU or
    byte order. For example, a data stream that is written by a PC
    under Windows can be read by a Sun SPARC running Solaris.

    You can also use a data stream to read/write \l{raw}{raw
    unencoded binary data}. If you want a "parsing" input stream, see
    QTextStream.

    The QDataStream class implements the serialization of C++'s basic
    data types, like \c char, \c short, \c int, \c{char *}, etc.
    Serialization of more complex data is accomplished by breaking up
    the data into primitive units.

    A data stream cooperates closely with a QIODevice. A QIODevice
    represents an input/output medium one can read data from and write
    data to. The QFile class is an example of an I/O device.

    Example (write binary data to a stream):

    \snippet code/src_corelib_io_qdatastream.cpp 0

    Example (read binary data from a stream):

    \snippet code/src_corelib_io_qdatastream.cpp 1

    Each item written to the stream is written in a predefined binary
    format that varies depending on the item's type. Supported Qt
    types include QBrush, QColor, QDateTime, QFont, QPixmap, QString,
    QVariant and many others. For the complete list of all Qt types
    supporting data streaming see \l{Serializing Qt Data Types}.

    For integers it is best to always cast to a Qt integer type for
    writing, and to read back into the same Qt integer type. This
    ensures that you get integers of the size you want and insulates
    you from compiler and platform differences.

    To take one example, a \c{char *} string is written as a 32-bit
    integer equal to the length of the string including the '\\0' byte,
    followed by all the characters of the string including the
    '\\0' byte. When reading a \c{char *} string, 4 bytes are read to
    create the 32-bit length value, then that many characters for the
    \c {char *} string including the '\\0' terminator are read.

    The initial I/O device is usually set in the constructor, but can be
    changed with setDevice(). If you've reached the end of the data
    (or if there is no I/O device set) atEnd() will return true.

    \section1 Versioning

    QDataStream's binary format has evolved since Qt 1.0, and is
    likely to continue evolving to reflect changes done in Qt. When
    inputting or outputting complex types, it's very important to
    make sure that the same version of the stream (version()) is used
    for reading and writing. If you need both forward and backward
    compatibility, you can hardcode the version number in the
    application:

    \snippet code/src_corelib_io_qdatastream.cpp 2

    If you are producing a new binary data format, such as a file
    format for documents created by your application, you could use a
    QDataStream to write the data in a portable format. Typically, you
    would write a brief header containing a magic string and a version
    number to give yourself room for future expansion. For example:

    \snippet code/src_corelib_io_qdatastream.cpp 3

    Then read it in with:

    \snippet code/src_corelib_io_qdatastream.cpp 4

    You can select which byte order to use when serializing data. The
    default setting is big endian (MSB first). Changing it to little
    endian breaks the portability (unless the reader also changes to
    little endian). We recommend keeping this setting unless you have
    special requirements.

    \target raw
    \section1 Reading and Writing Raw Binary Data

    You may wish to read/write your own raw binary data to/from the
    data stream directly. Data may be read from the stream into a
    preallocated \c{char *} using readRawData(). Similarly data can be
    written to the stream using writeRawData(). Note that any
    encoding/decoding of the data must be done by you.

    A similar pair of functions is readBytes() and writeBytes(). These
    differ from their \e raw counterparts as follows: readBytes()
    reads a quint32 which is taken to be the length of the data to be
    read, then that number of bytes is read into the preallocated
    \c{char *}; writeBytes() writes a quint32 containing the length of the
    data, followed by the data. Note that any encoding/decoding of
    the data (apart from the length quint32) must be done by you.

    \section1 Reading and Writing Qt Collection Classes

    The Qt container classes can also be serialized to a QDataStream.
    These include QList, QLinkedList, QVector, QSet, QHash, and QMap.
    The stream operators are declared as non-members of the classes.

    \target Serializing Qt Classes
    \section1 Reading and Writing Other Qt Classes

    In addition to the overloaded stream operators documented here,
    any Qt classes that you might want to serialize to a QDataStream
    will have appropriate stream operators declared as non-member of
    the class:

    \code
    QDataStream &operator<<(QDataStream &, const QXxx &);
    QDataStream &operator>>(QDataStream &, QXxx &);
    \endcode

    For example, here are the stream operators declared as non-members
    of the QImage class:

    \code
    QDataStream & operator<< (QDataStream& stream, const QImage& image);
    QDataStream & operator>> (QDataStream& stream, QImage& image);
    \endcode

    To see if your favorite Qt class has similar stream operators
    defined, check the \b {Related Non-Members} section of the
    class's documentation page.

    \section1 Using Read Transactions

    When a data stream operates on an asynchronous device, the chunks of data
    can arrive at arbitrary points in time. The QDataStream class implements
    a transaction mechanism that provides the ability to read the data
    atomically with a series of stream operators. As an example, you can
    handle incomplete reads from a socket by using a transaction in a slot
    connected to the readyRead() signal:

    \snippet code/src_corelib_io_qdatastream.cpp 6

    If no full packet is received, this code restores the stream to the
    initial position, after which you need to wait for more data to arrive.

    \sa QTextStream, QVariant
*/

/*!
    \enum QDataStream::ByteOrder

    The byte order used for reading/writing the data.

    \value BigEndian Most significant byte first (the default)
    \value LittleEndian Least significant byte first
*/

/*!
  \enum QDataStream::FloatingPointPrecision

  The precision of floating point numbers used for reading/writing the data. This will only have
  an effect if the version of the data stream is Qt_4_6 or higher.

  \warning The floating point precision must be set to the same value on the object that writes
  and the object that reads the data stream.

  \value SinglePrecision All floating point numbers in the data stream have 32-bit precision.
  \value DoublePrecision All floating point numbers in the data stream have 64-bit precision.

  \sa setFloatingPointPrecision(), floatingPointPrecision()
*/

/*!
    \enum QDataStream::Status

    This enum describes the current status of the data stream.

    \value Ok               The data stream is operating normally.
    \value ReadPastEnd      The data stream has read past the end of the
                            data in the underlying device.
    \value ReadCorruptData  The data stream has read corrupt data.
    \value WriteFailed      The data stream cannot write to the underlying device.
*/

/*****************************************************************************
  QDataStream member functions
 *****************************************************************************/

#define Q_VOID

#undef  CHECK_STREAM_PRECOND
#ifndef QT_NO_DEBUG
#define CHECK_STREAM_PRECOND(retVal) \
    if (!dev) { \
        qWarning("QDataStream: No device"); \
        return retVal; \
    }
#else
#define CHECK_STREAM_PRECOND(retVal) \
    if (!dev) { \
        return retVal; \
    }
#endif

#define CHECK_STREAM_WRITE_PRECOND(retVal) \
    CHECK_STREAM_PRECOND(retVal) \
    if (q_status != Ok) \
        return retVal;

#define CHECK_STREAM_TRANSACTION_PRECOND(retVal) \
    if (!d || d->transactionDepth == 0) { \
        qWarning("QDataStream: No transaction in progress"); \
        return retVal; \
    }

/*!
    Constructs a data stream that has no I/O device.

    \sa setDevice()
*/

QDataStream::QDataStream()
{
    dev = 0;
    owndev = false;
    byteorder = BigEndian;
    ver = Qt_DefaultCompiledVersion;
    noswap = QSysInfo::ByteOrder == QSysInfo::BigEndian;
    q_status = Ok;
}

/*!
    Constructs a data stream that uses the I/O device \a d.

    \sa setDevice(), device()
*/

QDataStream::QDataStream(QIODevice *d)
{
    dev = d;                                // set device
    owndev = false;
    byteorder = BigEndian;                        // default byte order
    ver = Qt_DefaultCompiledVersion;
    noswap = QSysInfo::ByteOrder == QSysInfo::BigEndian;
    q_status = Ok;
}

/*!
    \fn QDataStream::QDataStream(QByteArray *a, QIODevice::OpenMode mode)

    Constructs a data stream that operates on a byte array, \a a. The
    \a mode describes how the device is to be used.

    Alternatively, you can use QDataStream(const QByteArray &) if you
    just want to read from a byte array.

    Since QByteArray is not a QIODevice subclass, internally a QBuffer
    is created to wrap the byte array.
*/

QDataStream::QDataStream(QByteArray *a, QIODevice::OpenMode flags)
{
    QBuffer *buf = new QBuffer(a);
#ifndef QT_NO_QOBJECT
    buf->blockSignals(true);
#endif
    buf->open(flags);
    dev = buf;
    owndev = true;
    byteorder = BigEndian;
    ver = Qt_DefaultCompiledVersion;
    noswap = QSysInfo::ByteOrder == QSysInfo::BigEndian;
    q_status = Ok;
}

/*!
    Constructs a read-only data stream that operates on byte array \a a.
    Use QDataStream(QByteArray*, int) if you want to write to a byte
    array.

    Since QByteArray is not a QIODevice subclass, internally a QBuffer
    is created to wrap the byte array.
*/
QDataStream::QDataStream(const QByteArray &a)
{
    QBuffer *buf = new QBuffer;
#ifndef QT_NO_QOBJECT
    buf->blockSignals(true);
#endif
    buf->setData(a);
    buf->open(QIODevice::ReadOnly);
    dev = buf;
    owndev = true;
    byteorder = BigEndian;
    ver = Qt_DefaultCompiledVersion;
    noswap = QSysInfo::ByteOrder == QSysInfo::BigEndian;
    q_status = Ok;
}

/*!
    Destroys the data stream.

    The destructor will not affect the current I/O device, unless it is
    an internal I/O device (e.g. a QBuffer) processing a QByteArray
    passed in the \e constructor, in which case the internal I/O device
    is destroyed.
*/

QDataStream::~QDataStream()
{
    if (owndev)
        delete dev;
}


/*!
    \fn QIODevice *QDataStream::device() const

    Returns the I/O device currently set, or 0 if no
    device is currently set.

    \sa setDevice()
*/

/*!
    void QDataStream::setDevice(QIODevice *d)

    Sets the I/O device to \a d, which can be 0
    to unset to current I/O device.

    \sa device()
*/

void QDataStream::setDevice(QIODevice *d)
{
    if (owndev) {
        delete dev;
        owndev = false;
    }
    dev = d;
}

/*!
    \obsolete
    Unsets the I/O device.
    Use setDevice(0) instead.
*/

void QDataStream::unsetDevice()
{
    setDevice(0);
}


/*!
    \fn bool QDataStream::atEnd() const

    Returns \c true if the I/O device has reached the end position (end of
    the stream or file) or if there is no I/O device set; otherwise
    returns \c false.

    \sa QIODevice::atEnd()
*/

bool QDataStream::atEnd() const
{
    return dev ? dev->atEnd() : true;
}

/*!
    Returns the floating point precision of the data stream.

    \since 4.6

    \sa FloatingPointPrecision, setFloatingPointPrecision()
*/
QDataStream::FloatingPointPrecision QDataStream::floatingPointPrecision() const
{
    return d == 0 ? QDataStream::DoublePrecision : d->floatingPointPrecision;
}

/*!
    Sets the floating point precision of the data stream to \a precision. If the floating point precision is
    DoublePrecision and the version of the data stream is Qt_4_6 or higher, all floating point
    numbers will be written and read with 64-bit precision. If the floating point precision is
    SinglePrecision and the version is Qt_4_6 or higher, all floating point numbers will be written
    and read with 32-bit precision.

    For versions prior to Qt_4_6, the precision of floating point numbers in the data stream depends
    on the stream operator called.

    The default is DoublePrecision.

    Note that this property does not affect the serialization or deserialization of \c qfloat16
    instances.

    \warning This property must be set to the same value on the object that writes and the object
    that reads the data stream.

    \since 4.6
*/
void QDataStream::setFloatingPointPrecision(QDataStream::FloatingPointPrecision precision)
{
    if (d == 0)
        d.reset(new QDataStreamPrivate());
    d->floatingPointPrecision = precision;
}

/*!
    Returns the status of the data stream.

    \sa Status, setStatus(), resetStatus()
*/

QDataStream::Status QDataStream::status() const
{
    return q_status;
}

/*!
    Resets the status of the data stream.

    \sa Status, status(), setStatus()
*/
void QDataStream::resetStatus()
{
    q_status = Ok;
}

/*!
    Sets the status of the data stream to the \a status given.

    Subsequent calls to setStatus() are ignored until resetStatus()
    is called.

    \sa Status, status(), resetStatus()
*/
void QDataStream::setStatus(Status status)
{
    if (q_status == Ok)
        q_status = status;
}

/*!
    \fn int QDataStream::byteOrder() const

    Returns the current byte order setting -- either BigEndian or
    LittleEndian.

    \sa setByteOrder()
*/

/*!
    Sets the serialization byte order to \a bo.

    The \a bo parameter can be QDataStream::BigEndian or
    QDataStream::LittleEndian.

    The default setting is big endian. We recommend leaving this
    setting unless you have special requirements.

    \sa byteOrder()
*/

void QDataStream::setByteOrder(ByteOrder bo)
{
    byteorder = bo;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        noswap = (byteorder == BigEndian);
    else
        noswap = (byteorder == LittleEndian);
}


/*!
    \enum QDataStream::Version

    This enum provides symbolic synonyms for the data serialization
    format version numbers.

    \value Qt_1_0 Version 1 (Qt 1.x)
    \value Qt_2_0 Version 2 (Qt 2.0)
    \value Qt_2_1 Version 3 (Qt 2.1, 2.2, 2.3)
    \value Qt_3_0 Version 4 (Qt 3.0)
    \value Qt_3_1 Version 5 (Qt 3.1, 3.2)
    \value Qt_3_3 Version 6 (Qt 3.3)
    \value Qt_4_0 Version 7 (Qt 4.0, Qt 4.1)
    \value Qt_4_1 Version 7 (Qt 4.0, Qt 4.1)
    \value Qt_4_2 Version 8 (Qt 4.2)
    \value Qt_4_3 Version 9 (Qt 4.3)
    \value Qt_4_4 Version 10 (Qt 4.4)
    \value Qt_4_5 Version 11 (Qt 4.5)
    \value Qt_4_6 Version 12 (Qt 4.6, Qt 4.7, Qt 4.8)
    \value Qt_4_7 Same as Qt_4_6.
    \value Qt_4_8 Same as Qt_4_6.
    \value Qt_4_9 Same as Qt_4_6.
    \value Qt_5_0 Version 13 (Qt 5.0)
    \value Qt_5_1 Version 14 (Qt 5.1)
    \value Qt_5_2 Version 15 (Qt 5.2)
    \value Qt_5_3 Same as Qt_5_2
    \value Qt_5_4 Version 16 (Qt 5.4)
    \value Qt_5_5 Same as Qt_5_4
    \value Qt_5_6 Version 17 (Qt 5.6)
    \value Qt_5_7 Same as Qt_5_6
    \value Qt_5_8 Same as Qt_5_6
    \value Qt_5_9 Same as Qt_5_6
    \value Qt_5_10 Same as Qt_5_6
    \value Qt_5_11 Same as Qt_5_6
    \value Qt_5_12 Version 18 (Qt 5.12)
    \omitvalue Qt_DefaultCompiledVersion

    \sa setVersion(), version()
*/

/*!
    \fn int QDataStream::version() const

    Returns the version number of the data serialization format.

    \sa setVersion(), Version
*/

/*!
    \fn void QDataStream::setVersion(int v)

    Sets the version number of the data serialization format to \a v,
    a value of the \l Version enum.

    You don't \e have to set a version if you are using the current
    version of Qt, but for your own custom binary formats we
    recommend that you do; see \l{Versioning} in the Detailed
    Description.

    To accommodate new functionality, the datastream serialization
    format of some Qt classes has changed in some versions of Qt. If
    you want to read data that was created by an earlier version of
    Qt, or write data that can be read by a program that was compiled
    with an earlier version of Qt, use this function to modify the
    serialization format used by QDataStream.

    The \l Version enum provides symbolic constants for the different
    versions of Qt. For example:

    \snippet code/src_corelib_io_qdatastream.cpp 5

    \sa version(), Version
*/

/*!
    \since 5.7

    Starts a new read transaction on the stream.

    Defines a restorable point within the sequence of read operations. For
    sequential devices, read data will be duplicated internally to allow
    recovery in case of incomplete reads. For random-access devices,
    this function saves the current position of the stream. Call
    commitTransaction(), rollbackTransaction(), or abortTransaction() to
    finish the current transaction.

    Once a transaction is started, subsequent calls to this function will make
    the transaction recursive. Inner transactions act as agents of the
    outermost transaction (i.e., report the status of read operations to the
    outermost transaction, which can restore the position of the stream).

    \note Restoring to the point of the nested startTransaction() call is not
    supported.

    When an error occurs during a transaction (including an inner transaction
    failing), reading from the data stream is suspended (all subsequent read
    operations return empty/zero values) and subsequent inner transactions are
    forced to fail. Starting a new outermost transaction recovers from this
    state. This behavior makes it unnecessary to error-check every read
    operation separately.

    \sa commitTransaction(), rollbackTransaction(), abortTransaction()
*/

void QDataStream::startTransaction()
{
    CHECK_STREAM_PRECOND(Q_VOID)

    if (d == 0)
        d.reset(new QDataStreamPrivate());

    if (++d->transactionDepth == 1) {
        dev->startTransaction();
        resetStatus();
    }
}

/*!
    \since 5.7

    Completes a read transaction. Returns \c true if no read errors have
    occurred during the transaction; otherwise returns \c false.

    If called on an inner transaction, committing will be postponed until
    the outermost commitTransaction(), rollbackTransaction(), or
    abortTransaction() call occurs.

    Otherwise, if the stream status indicates reading past the end of the
    data, this function restores the stream data to the point of the
    startTransaction() call. When this situation occurs, you need to wait for
    more data to arrive, after which you start a new transaction. If the data
    stream has read corrupt data or any of the inner transactions was aborted,
    this function aborts the transaction.

    \sa startTransaction(), rollbackTransaction(), abortTransaction()
*/

bool QDataStream::commitTransaction()
{
    CHECK_STREAM_TRANSACTION_PRECOND(false)
    if (--d->transactionDepth == 0) {
        CHECK_STREAM_PRECOND(false)

        if (q_status == ReadPastEnd) {
            dev->rollbackTransaction();
            return false;
        }
        dev->commitTransaction();
    }
    return q_status == Ok;
}

/*!
    \since 5.7

    Reverts a read transaction.

    This function is commonly used to rollback the transaction when an
    incomplete read was detected prior to committing the transaction.

    If called on an inner transaction, reverting is delegated to the outermost
    transaction, and subsequently started inner transactions are forced to
    fail.

    For the outermost transaction, restores the stream data to the point of
    the startTransaction() call. If the data stream has read corrupt data or
    any of the inner transactions was aborted, this function aborts the
    transaction.

    If the preceding stream operations were successful, sets the status of the
    data stream to \value ReadPastEnd.

    \sa startTransaction(), commitTransaction(), abortTransaction()
*/

void QDataStream::rollbackTransaction()
{
    setStatus(ReadPastEnd);

    CHECK_STREAM_TRANSACTION_PRECOND(Q_VOID)
    if (--d->transactionDepth != 0)
        return;

    CHECK_STREAM_PRECOND(Q_VOID)
    if (q_status == ReadPastEnd)
        dev->rollbackTransaction();
    else
        dev->commitTransaction();
}

/*!
    \since 5.7

    Aborts a read transaction.

    This function is commonly used to discard the transaction after
    higher-level protocol errors or loss of stream synchronization.

    If called on an inner transaction, aborting is delegated to the outermost
    transaction, and subsequently started inner transactions are forced to
    fail.

    For the outermost transaction, discards the restoration point and any
    internally duplicated data of the stream. Will not affect the current
    read position of the stream.

    Sets the status of the data stream to \value ReadCorruptData.

    \sa startTransaction(), commitTransaction(), rollbackTransaction()
*/

void QDataStream::abortTransaction()
{
    q_status = ReadCorruptData;

    CHECK_STREAM_TRANSACTION_PRECOND(Q_VOID)
    if (--d->transactionDepth != 0)
        return;

    CHECK_STREAM_PRECOND(Q_VOID)
    dev->commitTransaction();
}

/*****************************************************************************
  QDataStream read functions
 *****************************************************************************/

/*!
    \internal
*/

int QDataStream::readBlock(char *data, int len)
{
    // Disable reads on failure in transacted stream
    if (q_status != Ok && dev->isTransactionStarted())
        return -1;

    const int readResult = dev->read(data, len);
    if (readResult != len)
        setStatus(ReadPastEnd);
    return readResult;
}

/*!
    \fn QDataStream &QDataStream::operator>>(std::nullptr_t &ptr)
    \since 5.9
    \overload

    Simulates reading a \c{std::nullptr_t} from the stream into \a ptr and
    returns a reference to the stream. This function does not actually read
    anything from the stream, as \c{std::nullptr_t} values are stored as 0
    bytes.
*/

/*!
    \fn QDataStream &QDataStream::operator>>(quint8 &i)
    \overload

    Reads an unsigned byte from the stream into \a i, and returns a
    reference to the stream.
*/

/*!
    Reads a signed byte from the stream into \a i, and returns a
    reference to the stream.
*/

QDataStream &QDataStream::operator>>(qint8 &i)
{
    i = 0;
    CHECK_STREAM_PRECOND(*this)
    char c;
    if (readBlock(&c, 1) == 1)
        i = qint8(c);
    return *this;
}


/*!
    \fn QDataStream &QDataStream::operator>>(quint16 &i)
    \overload

    Reads an unsigned 16-bit integer from the stream into \a i, and
    returns a reference to the stream.
*/

/*!
    \overload

    Reads a signed 16-bit integer from the stream into \a i, and
    returns a reference to the stream.
*/

QDataStream &QDataStream::operator>>(qint16 &i)
{
    i = 0;
    CHECK_STREAM_PRECOND(*this)
    if (readBlock(reinterpret_cast<char *>(&i), 2) != 2) {
        i = 0;
    } else {
        if (!noswap) {
            i = qbswap(i);
        }
    }
    return *this;
}


/*!
    \fn QDataStream &QDataStream::operator>>(quint32 &i)
    \overload

    Reads an unsigned 32-bit integer from the stream into \a i, and
    returns a reference to the stream.
*/

/*!
    \overload

    Reads a signed 32-bit integer from the stream into \a i, and
    returns a reference to the stream.
*/

QDataStream &QDataStream::operator>>(qint32 &i)
{
    i = 0;
    CHECK_STREAM_PRECOND(*this)
    if (readBlock(reinterpret_cast<char *>(&i), 4) != 4) {
        i = 0;
    } else {
        if (!noswap) {
            i = qbswap(i);
        }
    }
    return *this;
}

/*!
    \fn QDataStream &QDataStream::operator>>(quint64 &i)
    \overload

    Reads an unsigned 64-bit integer from the stream, into \a i, and
    returns a reference to the stream.
*/

/*!
    \overload

    Reads a signed 64-bit integer from the stream into \a i, and
    returns a reference to the stream.
*/

QDataStream &QDataStream::operator>>(qint64 &i)
{
    i = qint64(0);
    CHECK_STREAM_PRECOND(*this)
    if (version() < 6) {
        quint32 i1, i2;
        *this >> i2 >> i1;
        i = ((quint64)i1 << 32) + i2;
    } else {
        if (readBlock(reinterpret_cast<char *>(&i), 8) != 8) {
            i = qint64(0);
        } else {
            if (!noswap) {
                i = qbswap(i);
            }
        }
    }
    return *this;
}

/*!
    Reads a boolean value from the stream into \a i. Returns a
    reference to the stream.
*/
QDataStream &QDataStream::operator>>(bool &i)
{
    qint8 v;
    *this >> v;
    i = !!v;
    return *this;
}

/*!
    \overload

    Reads a floating point number from the stream into \a f,
    using the standard IEEE 754 format. Returns a reference to the
    stream.

    \sa setFloatingPointPrecision()
*/

QDataStream &QDataStream::operator>>(float &f)
{
    if (version() >= QDataStream::Qt_4_6
        && floatingPointPrecision() == QDataStream::DoublePrecision) {
        double d;
        *this >> d;
        f = d;
        return *this;
    }

    f = 0.0f;
    CHECK_STREAM_PRECOND(*this)
    if (readBlock(reinterpret_cast<char *>(&f), 4) != 4) {
        f = 0.0f;
    } else {
        if (!noswap) {
            union {
                float val1;
                quint32 val2;
            } x;
            x.val2 = qbswap(*reinterpret_cast<quint32 *>(&f));
            f = x.val1;
        }
    }
    return *this;
}

/*!
    \overload

    Reads a floating point number from the stream into \a f,
    using the standard IEEE 754 format. Returns a reference to the
    stream.

    \sa setFloatingPointPrecision()
*/

QDataStream &QDataStream::operator>>(double &f)
{
    if (version() >= QDataStream::Qt_4_6
        && floatingPointPrecision() == QDataStream::SinglePrecision) {
        float d;
        *this >> d;
        f = d;
        return *this;
    }

    f = 0.0;
    CHECK_STREAM_PRECOND(*this)
    if (readBlock(reinterpret_cast<char *>(&f), 8) != 8) {
        f = 0.0;
    } else {
        if (!noswap) {
            union {
                double val1;
                quint64 val2;
            } x;
            x.val2 = qbswap(*reinterpret_cast<quint64 *>(&f));
            f = x.val1;
        }
    }
    return *this;
}


/*!
    \overload
    \since 5.9

    Reads a floating point number from the stream into \a f,
    using the standard IEEE 754 format. Returns a reference to the
    stream.
*/
QDataStream &QDataStream::operator>>(qfloat16 &f)
{
    return *this >> reinterpret_cast<qint16&>(f);
}


/*!
    \overload

    Reads the '\\0'-terminated string \a s from the stream and returns
    a reference to the stream.

    The string is deserialized using \c{readBytes()}.

    Space for the string is allocated using \c{new []} -- the caller must
    destroy it with \c{delete []}.

    \sa readBytes(), readRawData()
*/

QDataStream &QDataStream::operator>>(char *&s)
{
    uint len = 0;
    return readBytes(s, len);
}


/*!
    Reads the buffer \a s from the stream and returns a reference to
    the stream.

    The buffer \a s is allocated using \c{new []}. Destroy it with the
    \c{delete []} operator.

    The \a l parameter is set to the length of the buffer. If the
    string read is empty, \a l is set to 0 and \a s is set to
    a null pointer.

    The serialization format is a quint32 length specifier first,
    then \a l bytes of data.

    \sa readRawData(), writeBytes()
*/

QDataStream &QDataStream::readBytes(char *&s, uint &l)
{
    s = 0;
    l = 0;
    CHECK_STREAM_PRECOND(*this)

    quint32 len;
    *this >> len;
    if (len == 0)
        return *this;

    const quint32 Step = 1024 * 1024;
    quint32 allocated = 0;
    char *prevBuf = 0;
    char *curBuf = 0;

    do {
        int blockSize = qMin(Step, len - allocated);
        prevBuf = curBuf;
        curBuf = new char[allocated + blockSize + 1];
        if (prevBuf) {
            memcpy(curBuf, prevBuf, allocated);
            delete [] prevBuf;
        }
        if (readBlock(curBuf + allocated, blockSize) != blockSize) {
            delete [] curBuf;
            return *this;
        }
        allocated += blockSize;
    } while (allocated < len);

    s = curBuf;
    s[len] = '\0';
    l = (uint)len;
    return *this;
}

/*!
    Reads at most \a len bytes from the stream into \a s and returns the number of
    bytes read. If an error occurs, this function returns -1.

    The buffer \a s must be preallocated. The data is \e not decoded.

    \sa readBytes(), QIODevice::read(), writeRawData()
*/

int QDataStream::readRawData(char *s, int len)
{
    CHECK_STREAM_PRECOND(-1)
    return readBlock(s, len);
}


/*****************************************************************************
  QDataStream write functions
 *****************************************************************************/

/*!
    \fn QDataStream &QDataStream::operator<<(std::nullptr_t ptr)
    \since 5.9
    \overload

    Simulates writing a \c{std::nullptr_t}, \a ptr, to the stream and returns a
    reference to the stream. This function does not actually write anything to
    the stream, as \c{std::nullptr_t} values are stored as 0 bytes.
*/

/*!
    \fn QDataStream &QDataStream::operator<<(quint8 i)
    \overload

    Writes an unsigned byte, \a i, to the stream and returns a
    reference to the stream.
*/

/*!
    Writes a signed byte, \a i, to the stream and returns a reference
    to the stream.
*/

QDataStream &QDataStream::operator<<(qint8 i)
{
    CHECK_STREAM_WRITE_PRECOND(*this)
    if (!dev->putChar(i))
        q_status = WriteFailed;
    return *this;
}


/*!
    \fn QDataStream &QDataStream::operator<<(quint16 i)
    \overload

    Writes an unsigned 16-bit integer, \a i, to the stream and returns
    a reference to the stream.
*/

/*!
    \overload

    Writes a signed 16-bit integer, \a i, to the stream and returns a
    reference to the stream.
*/

QDataStream &QDataStream::operator<<(qint16 i)
{
    CHECK_STREAM_WRITE_PRECOND(*this)
    if (!noswap) {
        i = qbswap(i);
    }
    if (dev->write((char *)&i, sizeof(qint16)) != sizeof(qint16))
        q_status = WriteFailed;
    return *this;
}

/*!
    \overload

    Writes a signed 32-bit integer, \a i, to the stream and returns a
    reference to the stream.
*/

QDataStream &QDataStream::operator<<(qint32 i)
{
    CHECK_STREAM_WRITE_PRECOND(*this)
    if (!noswap) {
        i = qbswap(i);
    }
    if (dev->write((char *)&i, sizeof(qint32)) != sizeof(qint32))
        q_status = WriteFailed;
    return *this;
}

/*!
    \fn QDataStream &QDataStream::operator<<(quint64 i)
    \overload

    Writes an unsigned 64-bit integer, \a i, to the stream and returns a
    reference to the stream.
*/

/*!
    \overload

    Writes a signed 64-bit integer, \a i, to the stream and returns a
    reference to the stream.
*/

QDataStream &QDataStream::operator<<(qint64 i)
{
    CHECK_STREAM_WRITE_PRECOND(*this)
    if (version() < 6) {
        quint32 i1 = i & 0xffffffff;
        quint32 i2 = i >> 32;
        *this << i2 << i1;
    } else {
        if (!noswap) {
            i = qbswap(i);
        }
        if (dev->write((char *)&i, sizeof(qint64)) != sizeof(qint64))
            q_status = WriteFailed;
    }
    return *this;
}

/*!
    \fn QDataStream &QDataStream::operator<<(quint32 i)
    \overload

    Writes an unsigned integer, \a i, to the stream as a 32-bit
    unsigned integer (quint32). Returns a reference to the stream.
*/

/*!
    Writes a boolean value, \a i, to the stream. Returns a reference
    to the stream.
*/

QDataStream &QDataStream::operator<<(bool i)
{
    CHECK_STREAM_WRITE_PRECOND(*this)
    if (!dev->putChar(qint8(i)))
        q_status = WriteFailed;
    return *this;
}

/*!
    \overload

    Writes a floating point number, \a f, to the stream using
    the standard IEEE 754 format. Returns a reference to the stream.

    \sa setFloatingPointPrecision()
*/

QDataStream &QDataStream::operator<<(float f)
{
    if (version() >= QDataStream::Qt_4_6
        && floatingPointPrecision() == QDataStream::DoublePrecision) {
        *this << double(f);
        return *this;
    }

    CHECK_STREAM_WRITE_PRECOND(*this)
    float g = f;                                // fixes float-on-stack problem
    if (!noswap) {
        union {
            float val1;
            quint32 val2;
        } x;
        x.val1 = g;
        x.val2 = qbswap(x.val2);

        if (dev->write((char *)&x.val2, sizeof(float)) != sizeof(float))
            q_status = WriteFailed;
        return *this;
    }

    if (dev->write((char *)&g, sizeof(float)) != sizeof(float))
        q_status = WriteFailed;
    return *this;
}


/*!
    \overload

    Writes a floating point number, \a f, to the stream using
    the standard IEEE 754 format. Returns a reference to the stream.

    \sa setFloatingPointPrecision()
*/

QDataStream &QDataStream::operator<<(double f)
{
    if (version() >= QDataStream::Qt_4_6
        && floatingPointPrecision() == QDataStream::SinglePrecision) {
        *this << float(f);
        return *this;
    }

    CHECK_STREAM_WRITE_PRECOND(*this)
    if (noswap) {
        if (dev->write((char *)&f, sizeof(double)) != sizeof(double))
            q_status = WriteFailed;
    } else {
        union {
            double val1;
            quint64 val2;
        } x;
        x.val1 = f;
        x.val2 = qbswap(x.val2);
        if (dev->write((char *)&x.val2, sizeof(double)) != sizeof(double))
            q_status = WriteFailed;
    }
    return *this;
}


/*!
    \fn QDataStream &QDataStream::operator<<(qfloat16 f)
    \overload
    \since 5.9

    Writes a floating point number, \a f, to the stream using
    the standard IEEE 754 format. Returns a reference to the stream.
*/
QDataStream &QDataStream::operator<<(qfloat16 f)
{
    return *this << reinterpret_cast<qint16&>(f);
}

/*!
    \overload

    Writes the '\\0'-terminated string \a s to the stream and returns a
    reference to the stream.

    The string is serialized using \c{writeBytes()}.

    \sa writeBytes(), writeRawData()
*/

QDataStream &QDataStream::operator<<(const char *s)
{
    if (!s) {
        *this << (quint32)0;
        return *this;
    }
    uint len = qstrlen(s) + 1;                        // also write null terminator
    *this << (quint32)len;                        // write length specifier
    writeRawData(s, len);
    return *this;
}

/*!
    Writes the length specifier \a len and the buffer \a s to the
    stream and returns a reference to the stream.

    The \a len is serialized as a quint32, followed by \a len bytes
    from \a s. Note that the data is \e not encoded.

    \sa writeRawData(), readBytes()
*/

QDataStream &QDataStream::writeBytes(const char *s, uint len)
{
    CHECK_STREAM_WRITE_PRECOND(*this)
    *this << (quint32)len;                        // write length specifier
    if (len)
        writeRawData(s, len);
    return *this;
}


/*!
    Writes \a len bytes from \a s to the stream. Returns the
    number of bytes actually written, or -1 on error.
    The data is \e not encoded.

    \sa writeBytes(), QIODevice::write(), readRawData()
*/

int QDataStream::writeRawData(const char *s, int len)
{
    CHECK_STREAM_WRITE_PRECOND(-1)
    int ret = dev->write(s, len);
    if (ret != len)
        q_status = WriteFailed;
    return ret;
}

/*!
    \since 4.1

    Skips \a len bytes from the device. Returns the number of bytes
    actually skipped, or -1 on error.

    This is equivalent to calling readRawData() on a buffer of length
    \a len and ignoring the buffer.

    \sa QIODevice::seek()
*/
int QDataStream::skipRawData(int len)
{
    CHECK_STREAM_PRECOND(-1)
    if (q_status != Ok && dev->isTransactionStarted())
        return -1;

    const int skipResult = dev->skip(len);
    if (skipResult != len)
        setStatus(ReadPastEnd);
    return skipResult;
}

QT_END_NAMESPACE

#endif // QT_NO_DATASTREAM
