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

#include "qplatformdefs.h"
#include "qfiledevice.h"
#include "qfiledevice_p.h"
#include "qfsfileengine_p.h"

#ifdef QT_NO_QOBJECT
#define tr(X) QString::fromLatin1(X)
#endif

QT_BEGIN_NAMESPACE

#ifndef QFILE_WRITEBUFFER_SIZE
#define QFILE_WRITEBUFFER_SIZE 16384
#endif

QFileDevicePrivate::QFileDevicePrivate()
    : fileEngine(0),
      cachedSize(0),
      error(QFile::NoError), lastWasWrite(false)
{
    writeBufferChunkSize = QFILE_WRITEBUFFER_SIZE;
}

QFileDevicePrivate::~QFileDevicePrivate()
{
    delete fileEngine;
    fileEngine = 0;
}

QAbstractFileEngine * QFileDevicePrivate::engine() const
{
    if (!fileEngine)
        fileEngine = new QFSFileEngine;
    return fileEngine;
}

void QFileDevicePrivate::setError(QFileDevice::FileError err)
{
    error = err;
    errorString.clear();
}

void QFileDevicePrivate::setError(QFileDevice::FileError err, const QString &errStr)
{
    error = err;
    errorString = errStr;
}

void QFileDevicePrivate::setError(QFileDevice::FileError err, int errNum)
{
    error = err;
    errorString = qt_error_string(errNum);
}

/*!
    \enum QFileDevice::FileError

    This enum describes the errors that may be returned by the error()
    function.

    \value NoError          No error occurred.
    \value ReadError        An error occurred when reading from the file.
    \value WriteError       An error occurred when writing to the file.
    \value FatalError       A fatal error occurred.
    \value ResourceError    Out of resources (e.g., too many open files, out of memory, etc.)
    \value OpenError        The file could not be opened.
    \value AbortError       The operation was aborted.
    \value TimeOutError     A timeout occurred.
    \value UnspecifiedError An unspecified error occurred.
    \value RemoveError      The file could not be removed.
    \value RenameError      The file could not be renamed.
    \value PositionError    The position in the file could not be changed.
    \value ResizeError      The file could not be resized.
    \value PermissionsError The file could not be accessed.
    \value CopyError        The file could not be copied.
*/

/*!
    \enum QFileDevice::Permission

    This enum is used by the permission() function to report the
    permissions and ownership of a file. The values may be OR-ed
    together to test multiple permissions and ownership values.

    \value ReadOwner The file is readable by the owner of the file.
    \value WriteOwner The file is writable by the owner of the file.
    \value ExeOwner The file is executable by the owner of the file.
    \value ReadUser The file is readable by the user.
    \value WriteUser The file is writable by the user.
    \value ExeUser The file is executable by the user.
    \value ReadGroup The file is readable by the group.
    \value WriteGroup The file is writable by the group.
    \value ExeGroup The file is executable by the group.
    \value ReadOther The file is readable by anyone.
    \value WriteOther The file is writable by anyone.
    \value ExeOther The file is executable by anyone.

    \warning Because of differences in the platforms supported by Qt,
    the semantics of ReadUser, WriteUser and ExeUser are
    platform-dependent: On Unix, the rights of the owner of the file
    are returned and on Windows the rights of the current user are
    returned. This behavior might change in a future Qt version.

    \note On NTFS file systems, ownership and permissions checking is
    disabled by default for performance reasons. To enable it,
    include the following line:

    \snippet ntfsp.cpp 0

    Permission checking is then turned on and off by incrementing and
    decrementing \c qt_ntfs_permission_lookup by 1.

    \snippet ntfsp.cpp 1
*/

//************* QFileDevice

/*!
    \class QFileDevice
    \inmodule QtCore
    \since 5.0

    \brief The QFileDevice class provides an interface for reading from and writing to open files.

    \ingroup io

    \reentrant

    QFileDevice is the base class for I/O devices that can read and write text and binary files
    and \l{The Qt Resource System}{resources}. QFile offers the main functionality,
    QFileDevice serves as a base class for sharing functionality with other file devices such
    as QTemporaryFile, by providing all the operations that can be done on files that have
    been opened by QFile or QTemporaryFile.

    \sa QFile, QTemporaryFile
*/

/*!
    \enum QFileDevice::FileHandleFlag

    This enum is used when opening a file to specify additional
    options which only apply to files and not to a generic
    QIODevice.

    \value AutoCloseHandle The file handle passed into open() should be
    closed by close(), the default behavior is that close just flushes
    the file and the application is responsible for closing the file handle.
    When opening a file by name, this flag is ignored as Qt always owns the
    file handle and must close it.
    \value DontCloseHandle If not explicitly closed, the underlying file
    handle is left open when the QFile object is destroyed.
 */

#ifdef QT_NO_QOBJECT
QFileDevice::QFileDevice()
    : QIODevice(*new QFileDevicePrivate)
{
}
QFileDevice::QFileDevice(QFileDevicePrivate &dd)
    : QIODevice(dd)
{
}
#else
/*!
    \internal
*/
QFileDevice::QFileDevice()
    : QIODevice(*new QFileDevicePrivate, 0)
{
}
/*!
    \internal
*/
QFileDevice::QFileDevice(QObject *parent)
    : QIODevice(*new QFileDevicePrivate, parent)
{
}
/*!
    \internal
*/
QFileDevice::QFileDevice(QFileDevicePrivate &dd, QObject *parent)
    : QIODevice(dd, parent)
{
}
#endif

/*!
    Destroys the file device, closing it if necessary.
*/
QFileDevice::~QFileDevice()
{
    close();
}

/*!
    Returns \c true if the file can only be manipulated sequentially;
    otherwise returns \c false.

    Most files support random-access, but some special files may not.

    \sa QIODevice::isSequential()
*/
bool QFileDevice::isSequential() const
{
    Q_D(const QFileDevice);
    return d->fileEngine && d->fileEngine->isSequential();
}

/*!
  Returns the file handle of the file.

  This is a small positive integer, suitable for use with C library
  functions such as \c fdopen() and \c fcntl(). On systems that use file
  descriptors for sockets (i.e. Unix systems, but not Windows) the handle
  can be used with QSocketNotifier as well.

  If the file is not open, or there is an error, handle() returns -1.

  \sa QSocketNotifier
*/
int QFileDevice::handle() const
{
    Q_D(const QFileDevice);
    if (!isOpen() || !d->fileEngine)
        return -1;

    return d->fileEngine->handle();
}

/*!
    Returns the name of the file.
    The default implementation in QFileDevice returns a null string.
*/
QString QFileDevice::fileName() const
{
    return QString();
}

/*!
    Flushes any buffered data to the file. Returns \c true if successful;
    otherwise returns \c false.
*/
bool QFileDevice::flush()
{
    Q_D(QFileDevice);
    if (!d->fileEngine) {
        qWarning("QFileDevice::flush: No file engine. Is IODevice open?");
        return false;
    }

    if (!d->writeBuffer.isEmpty()) {
        qint64 size = d->writeBuffer.nextDataBlockSize();
        qint64 written = d->fileEngine->write(d->writeBuffer.readPointer(), size);
        if (written > 0)
            d->writeBuffer.free(written);
        if (written != size) {
            QFileDevice::FileError err = d->fileEngine->error();
            if (err == QFileDevice::UnspecifiedError)
                err = QFileDevice::WriteError;
            d->setError(err, d->fileEngine->errorString());
            return false;
        }
    }

    if (!d->fileEngine->flush()) {
        QFileDevice::FileError err = d->fileEngine->error();
        if (err == QFileDevice::UnspecifiedError)
            err = QFileDevice::WriteError;
        d->setError(err, d->fileEngine->errorString());
        return false;
    }
    return true;
}

/*!
  Calls QFileDevice::flush() and closes the file. Errors from flush are ignored.

  \sa QIODevice::close()
*/
void QFileDevice::close()
{
    Q_D(QFileDevice);
    if (!isOpen())
        return;
    bool flushed = flush();
    QIODevice::close();

    // reset write buffer
    d->lastWasWrite = false;
    d->writeBuffer.clear();

    // reset cached size
    d->cachedSize = 0;

    // keep earlier error from flush
    if (d->fileEngine->close() && flushed)
        unsetError();
    else if (flushed)
        d->setError(d->fileEngine->error(), d->fileEngine->errorString());
}

/*!
  \reimp
*/
qint64 QFileDevice::pos() const
{
    return QIODevice::pos();
}

/*!
  Returns \c true if the end of the file has been reached; otherwise returns
  false.

  For regular empty files on Unix (e.g. those in \c /proc), this function
  returns \c true, since the file system reports that the size of such a file is
  0. Therefore, you should not depend on atEnd() when reading data from such a
  file, but rather call read() until no more data can be read.
*/
bool QFileDevice::atEnd() const
{
    Q_D(const QFileDevice);

    // If there's buffered data left, we're not at the end.
    if (!d->isBufferEmpty())
        return false;

    if (!isOpen())
        return true;

    if (!d->ensureFlushed())
        return false;

    // If the file engine knows best, say what it says.
    if (d->fileEngine->supportsExtension(QAbstractFileEngine::AtEndExtension)) {
        // Check if the file engine supports AtEndExtension, and if it does,
        // check if the file engine claims to be at the end.
        return d->fileEngine->atEnd();
    }

    // if it looks like we are at the end, or if size is not cached,
    // fall through to bytesAvailable() to make sure.
    if (pos() < d->cachedSize)
        return false;

    // Fall back to checking how much is available (will stat files).
    return bytesAvailable() == 0;
}

/*!
    \fn bool QFileDevice::seek(qint64 pos)

    For random-access devices, this function sets the current position
    to \a pos, returning true on success, or false if an error occurred.
    For sequential devices, the default behavior is to do nothing and
    return false.

    Seeking beyond the end of a file:
    If the position is beyond the end of a file, then seek() will not
    immediately extend the file. If a write is performed at this position,
    then the file will be extended. The content of the file between the
    previous end of file and the newly written data is UNDEFINED and
    varies between platforms and file systems.
*/
bool QFileDevice::seek(qint64 off)
{
    Q_D(QFileDevice);
    if (!isOpen()) {
        qWarning("QFileDevice::seek: IODevice is not open");
        return false;
    }

    if (!d->ensureFlushed())
        return false;

    if (!d->fileEngine->seek(off) || !QIODevice::seek(off)) {
        QFileDevice::FileError err = d->fileEngine->error();
        if (err == QFileDevice::UnspecifiedError)
            err = QFileDevice::PositionError;
        d->setError(err, d->fileEngine->errorString());
        return false;
    }
    unsetError();
    return true;
}

/*!
  \reimp
*/
qint64 QFileDevice::readLineData(char *data, qint64 maxlen)
{
    Q_D(QFileDevice);
    if (!d->ensureFlushed())
        return -1;

    qint64 read;
    if (d->fileEngine->supportsExtension(QAbstractFileEngine::FastReadLineExtension)) {
        read = d->fileEngine->readLine(data, maxlen);
    } else {
        // Fall back to QIODevice's readLine implementation if the engine
        // cannot do it faster.
        read = QIODevice::readLineData(data, maxlen);
    }

    if (read < maxlen) {
        // failed to read all requested, may be at the end of file, stop caching size so that it's rechecked
        d->cachedSize = 0;
    }

    return read;
}

/*!
  \reimp
*/
qint64 QFileDevice::readData(char *data, qint64 len)
{
    Q_D(QFileDevice);
    if (!len)
        return 0;
    unsetError();
    if (!d->ensureFlushed())
        return -1;

    const qint64 read = d->fileEngine->read(data, len);
    if (read < 0) {
        QFileDevice::FileError err = d->fileEngine->error();
        if (err == QFileDevice::UnspecifiedError)
            err = QFileDevice::ReadError;
        d->setError(err, d->fileEngine->errorString());
    }

    if (read < len) {
        // failed to read all requested, may be at the end of file, stop caching size so that it's rechecked
        d->cachedSize = 0;
    }

    return read;
}

/*!
    \internal
*/
bool QFileDevicePrivate::putCharHelper(char c)
{
#ifdef QT_NO_QOBJECT
    return QIODevicePrivate::putCharHelper(c);
#else

    // Cutoff for code that doesn't only touch the buffer.
    qint64 writeBufferSize = writeBuffer.size();
    if ((openMode & QIODevice::Unbuffered) || writeBufferSize + 1 >= writeBufferChunkSize
#ifdef Q_OS_WIN
        || ((openMode & QIODevice::Text) && c == '\n'
            && writeBufferSize + 2 >= writeBufferChunkSize)
#endif
        ) {
        return QIODevicePrivate::putCharHelper(c);
    }

    if (!(openMode & QIODevice::WriteOnly)) {
        if (openMode == QIODevice::NotOpen)
            qWarning("QIODevice::putChar: Closed device");
        else
            qWarning("QIODevice::putChar: ReadOnly device");
        return false;
    }

    // Make sure the device is positioned correctly.
    const bool sequential = isSequential();
    if (pos != devicePos && !sequential && !q_func()->seek(pos))
        return false;

    lastWasWrite = true;

    int len = 1;
#ifdef Q_OS_WIN
    if ((openMode & QIODevice::Text) && c == '\n') {
        ++len;
        *writeBuffer.reserve(1) = '\r';
    }
#endif

    // Write to buffer.
    *writeBuffer.reserve(1) = c;

    if (!sequential) {
        pos += len;
        devicePos += len;
        if (!buffer.isEmpty())
            buffer.skip(len);
    }

    return true;
#endif
}

/*!
  \reimp
*/
qint64 QFileDevice::writeData(const char *data, qint64 len)
{
    Q_D(QFileDevice);
    unsetError();
    d->lastWasWrite = true;
    bool buffered = !(d->openMode & Unbuffered);

    // Flush buffered data if this read will overflow.
    if (buffered && (d->writeBuffer.size() + len) > d->writeBufferChunkSize) {
        if (!flush())
            return -1;
    }

    // Write directly to the engine if the block size is larger than
    // the write buffer size.
    if (!buffered || len > d->writeBufferChunkSize) {
        const qint64 ret = d->fileEngine->write(data, len);
        if (ret < 0) {
            QFileDevice::FileError err = d->fileEngine->error();
            if (err == QFileDevice::UnspecifiedError)
                err = QFileDevice::WriteError;
            d->setError(err, d->fileEngine->errorString());
        }
        return ret;
    }

    // Write to the buffer.
    d->writeBuffer.append(data, len);
    return len;
}

/*!
    Returns the file error status.

    The I/O device status returns an error code. For example, if open()
    returns \c false, or a read/write operation returns -1, this function can
    be called to find out the reason why the operation failed.

    \sa unsetError()
*/
QFileDevice::FileError QFileDevice::error() const
{
    Q_D(const QFileDevice);
    return d->error;
}

/*!
    Sets the file's error to QFileDevice::NoError.

    \sa error()
*/
void QFileDevice::unsetError()
{
    Q_D(QFileDevice);
    d->setError(QFileDevice::NoError);
}

/*!
  Returns the size of the file.

  For regular empty files on Unix (e.g. those in \c /proc), this function
  returns 0; the contents of such a file are generated on demand in response
  to you calling read().
*/
qint64 QFileDevice::size() const
{
    Q_D(const QFileDevice);
    if (!d->ensureFlushed())
        return 0;
    d->cachedSize = d->engine()->size();
    return d->cachedSize;
}

/*!
    Sets the file size (in bytes) \a sz. Returns \c true if the
    resize succeeds; false otherwise. If \a sz is larger than the file
    currently is, the new bytes will be set to 0; if \a sz is smaller, the
    file is simply truncated.

    \warning This function can fail if the file doesn't exist.

    \sa size()
*/
bool QFileDevice::resize(qint64 sz)
{
    Q_D(QFileDevice);
    if (!d->ensureFlushed())
        return false;
    d->engine();
    if (isOpen() && d->fileEngine->pos() > sz)
        seek(sz);
    if (d->fileEngine->setSize(sz)) {
        unsetError();
        d->cachedSize = sz;
        return true;
    }
    d->cachedSize = 0;
    d->setError(QFile::ResizeError, d->fileEngine->errorString());
    return false;
}

/*!
    Returns the complete OR-ed together combination of
    QFile::Permission for the file.

    \sa setPermissions()
*/
QFile::Permissions QFileDevice::permissions() const
{
    Q_D(const QFileDevice);
    QAbstractFileEngine::FileFlags perms = d->engine()->fileFlags(QAbstractFileEngine::PermsMask) & QAbstractFileEngine::PermsMask;
    return QFile::Permissions((int)perms); //ewww
}

/*!
    Sets the permissions for the file to the \a permissions specified.
    Returns \c true if successful, or \c false if the permissions cannot be
    modified.

    \warning This function does not manipulate ACLs, which may limit its
    effectiveness.

    \sa permissions()
*/
bool QFileDevice::setPermissions(Permissions permissions)
{
    Q_D(QFileDevice);
    if (d->engine()->setPermissions(permissions)) {
        unsetError();
        return true;
    }
    d->setError(QFile::PermissionsError, d->fileEngine->errorString());
    return false;
}

/*!
    \enum QFileDevice::MemoryMapFlags
    \since 4.4

    This enum describes special options that may be used by the map()
    function.

    \value NoOptions        No options.
    \value MapPrivateOption The mapped memory will be private, so any
    modifications will not be visible to other processes and will not
    be written to disk.  Any such modifications will be lost when the
    memory is unmapped.  It is unspecified whether modifications made
    to the file made after the mapping is created will be visible through
    the mapped memory. This enum value was introduced in Qt 5.4.
*/

/*!
    Maps \a size bytes of the file into memory starting at \a offset.  A file
    should be open for a map to succeed but the file does not need to stay
    open after the memory has been mapped.  When the QFile is destroyed
    or a new file is opened with this object, any maps that have not been
    unmapped will automatically be unmapped.

    The mapping will have the same open mode as the file (read and/or write),
    except when using MapPrivateOption, in which case it is always possible
    to write to the mapped memory.

    Any mapping options can be passed through \a flags.

    Returns a pointer to the memory or 0 if there is an error.

    \sa unmap()
 */
uchar *QFileDevice::map(qint64 offset, qint64 size, MemoryMapFlags flags)
{
    Q_D(QFileDevice);
    if (d->engine()
            && d->fileEngine->supportsExtension(QAbstractFileEngine::MapExtension)) {
        unsetError();
        uchar *address = d->fileEngine->map(offset, size, flags);
        if (address == 0)
            d->setError(d->fileEngine->error(), d->fileEngine->errorString());
        return address;
    }
    return 0;
}

/*!
    Unmaps the memory \a address.

    Returns \c true if the unmap succeeds; false otherwise.

    \sa map()
 */
bool QFileDevice::unmap(uchar *address)
{
    Q_D(QFileDevice);
    if (d->engine()
        && d->fileEngine->supportsExtension(QAbstractFileEngine::UnMapExtension)) {
        unsetError();
        bool success = d->fileEngine->unmap(address);
        if (!success)
            d->setError(d->fileEngine->error(), d->fileEngine->errorString());
        return success;
    }
    d->setError(PermissionsError, tr("No file engine available or engine does not support UnMapExtension"));
    return false;
}

/*!
    \enum QFileDevice::FileTime
    \since 5.10

    This enum is used by the fileTime() and setFileTime() functions.

    \value FileAccessTime           When the file was most recently accessed
                                    (e.g. read or written to).
    \value FileBirthTime            When the file was created (may not be not
                                    supported on UNIX).
    \value FileMetadataChangeTime   When the file's metadata was last changed.
    \value FileModificationTime     When the file was most recently modified.

    \sa setFileTime(), fileTime(), QFileInfo::fileTime()
*/

static inline QAbstractFileEngine::FileTime FileDeviceTimeToAbstractFileEngineTime(QFileDevice::FileTime time)
{
    Q_STATIC_ASSERT(int(QFileDevice::FileAccessTime) == int(QAbstractFileEngine::AccessTime));
    Q_STATIC_ASSERT(int(QFileDevice::FileBirthTime) == int(QAbstractFileEngine::BirthTime));
    Q_STATIC_ASSERT(int(QFileDevice::FileMetadataChangeTime) == int(QAbstractFileEngine::MetadataChangeTime));
    Q_STATIC_ASSERT(int(QFileDevice::FileModificationTime) == int(QAbstractFileEngine::ModificationTime));
    return QAbstractFileEngine::FileTime(time);
}

/*!
    \since 5.10
    Returns the file time specified by \a time.
    If the time cannot be determined return QDateTime() (an invalid
    date time).

    \sa setFileTime(), FileTime, QDateTime::isValid()
*/
QDateTime QFileDevice::fileTime(QFileDevice::FileTime time) const
{
    Q_D(const QFileDevice);

    if (d->engine())
        return d->engine()->fileTime(FileDeviceTimeToAbstractFileEngineTime(time));

    return QDateTime();
}

/*!
    \since 5.10
    Sets the file time specified by \a fileTime to \a newDate, returning true
    if successful; otherwise returns false.

    \note The file must be open to use this function.

    \sa fileTime(), FileTime
*/
bool QFileDevice::setFileTime(const QDateTime &newDate, QFileDevice::FileTime fileTime)
{
    Q_D(QFileDevice);

    if (!d->engine()) {
        d->setError(QFileDevice::UnspecifiedError, tr("No file engine available"));
        return false;
    }

    if (!d->fileEngine->setFileTime(newDate, FileDeviceTimeToAbstractFileEngineTime(fileTime))) {
        d->setError(d->fileEngine->error(), d->fileEngine->errorString());
        return false;
    }

    unsetError();
    return true;
}

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qfiledevice.cpp"
#endif
