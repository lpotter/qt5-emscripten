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
#include "private/qabstractfileengine_p.h"
#include "private/qfsfileengine_p.h"
#include "qfilesystemengine_p.h"
#include <qdebug.h>

#include "qfile.h"
#include "qdir.h"
#include "qvarlengtharray.h"
#include "qdatetime.h"
#include "qt_windows.h"

#include <sys/types.h>
#include <direct.h>
#include <winioctl.h>
#include <objbase.h>
#ifndef Q_OS_WINRT
#  include <shlobj.h>
#  include <accctrl.h>
#endif
#include <initguid.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#ifndef Q_OS_WINRT
#  define SECURITY_WIN32
#  include <security.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX FILENAME_MAX
#endif

QT_BEGIN_NAMESPACE

static inline bool isUncPath(const QString &path)
{
    // Starts with \\, but not \\.
    return (path.startsWith(QLatin1String("\\\\"))
            && path.size() > 2 && path.at(2) != QLatin1Char('.'));
}

/*!
    \internal
*/
QString QFSFileEnginePrivate::longFileName(const QString &path)
{
    if (path.startsWith(QLatin1String("\\\\.\\")))
        return path;

    QString absPath = QFileSystemEngine::nativeAbsoluteFilePath(path);
#if !defined(Q_OS_WINRT)
    QString prefix = QLatin1String("\\\\?\\");
    if (isUncPath(absPath)) {
        prefix.append(QLatin1String("UNC\\")); // "\\\\?\\UNC\\"
        absPath.remove(0, 2);
    }
    return prefix + absPath;
#else
    return absPath;
#endif
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeOpen(QIODevice::OpenMode openMode)
{
    Q_Q(QFSFileEngine);

    // All files are opened in share mode (both read and write).
    DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    int accessRights = 0;
    if (openMode & QIODevice::ReadOnly)
        accessRights |= GENERIC_READ;
    if (openMode & QIODevice::WriteOnly)
        accessRights |= GENERIC_WRITE;

    // WriteOnly can create files, ReadOnly cannot.
    DWORD creationDisp = (openMode & QIODevice::NewOnly)
                            ? CREATE_NEW
                            : openModeCanCreate(openMode)
                                ? OPEN_ALWAYS
                                : OPEN_EXISTING;
    // Create the file handle.
#ifndef Q_OS_WINRT
    SECURITY_ATTRIBUTES securityAtts = { sizeof(SECURITY_ATTRIBUTES), NULL, FALSE };
    fileHandle = CreateFile((const wchar_t*)fileEntry.nativeFilePath().utf16(),
                            accessRights,
                            shareMode,
                            &securityAtts,
                            creationDisp,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
#else // !Q_OS_WINRT
    fileHandle = CreateFile2((const wchar_t*)fileEntry.nativeFilePath().utf16(),
                             accessRights,
                             shareMode,
                             creationDisp,
                             NULL);
#endif // Q_OS_WINRT

    // Bail out on error.
    if (fileHandle == INVALID_HANDLE_VALUE) {
        q->setError(QFile::OpenError, qt_error_string());
        return false;
    }

    // Truncate the file after successfully opening it if Truncate is passed.
    if (openMode & QIODevice::Truncate)
        q->setSize(0);

    return true;
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeClose()
{
    Q_Q(QFSFileEngine);
    if (fh || fd != -1) {
        // stdlib / stdio mode.
        return closeFdFh();
    }

    // Windows native mode.
    bool ok = true;

    if (cachedFd != -1) {
        if (::_close(cachedFd) && !::CloseHandle(fileHandle)) {
            q->setError(QFile::UnspecifiedError, qt_error_string());
            ok = false;
        }

        // System handle is closed with associated file descriptor.
        fileHandle = INVALID_HANDLE_VALUE;
        cachedFd = -1;

        return ok;
    }

    if ((fileHandle == INVALID_HANDLE_VALUE || !::CloseHandle(fileHandle))) {
        q->setError(QFile::UnspecifiedError, qt_error_string());
        ok = false;
    }
    fileHandle = INVALID_HANDLE_VALUE;
    return ok;
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeFlush()
{
    if (fh) {
        // Buffered stdlib mode.
        return flushFh();
    }
    if (fd != -1) {
        // Unbuffered stdio mode; always succeeds (no buffer).
        return true;
    }

    // Windows native mode; flushing is unnecessary.
    return true;
}

/*
    \internal
    \since 5.1
*/
bool QFSFileEnginePrivate::nativeSyncToDisk()
{
    if (fh || fd != -1) {
        // stdlib / stdio mode. No API available.
        return false;
    }
    return FlushFileBuffers(fileHandle);
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeSize() const
{
    Q_Q(const QFSFileEngine);
    QFSFileEngine *thatQ = const_cast<QFSFileEngine *>(q);

    // ### Don't flush; for buffered files, we should get away with ftell.
    thatQ->flush();

    // Always retrive the current information
    metaData.clearFlags(QFileSystemMetaData::SizeAttribute);
    bool filled = false;
    if (fileHandle != INVALID_HANDLE_VALUE && openMode != QIODevice::NotOpen )
        filled = QFileSystemEngine::fillMetaData(fileHandle, metaData,
                                                 QFileSystemMetaData::SizeAttribute);
    else
        filled = doStat(QFileSystemMetaData::SizeAttribute);

    if (!filled) {
        thatQ->setError(QFile::UnspecifiedError, QSystemError::stdString());
        return 0;
    }
    return metaData.size();
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativePos() const
{
    Q_Q(const QFSFileEngine);
    QFSFileEngine *thatQ = const_cast<QFSFileEngine *>(q);

    if (fh || fd != -1) {
        // stdlib / stido mode.
        return posFdFh();
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return 0;

    LARGE_INTEGER currentFilePos;
    LARGE_INTEGER offset;
    offset.QuadPart = 0;
    if (!::SetFilePointerEx(fileHandle, offset, &currentFilePos, FILE_CURRENT)) {
        thatQ->setError(QFile::UnspecifiedError, qt_error_string());
        return 0;
    }

    return qint64(currentFilePos.QuadPart);
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeSeek(qint64 pos)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdlib / stdio mode.
        return seekFdFh(pos);
    }

    LARGE_INTEGER currentFilePos;
    LARGE_INTEGER offset;
    offset.QuadPart = pos;
    if (!::SetFilePointerEx(fileHandle, offset, &currentFilePos, FILE_BEGIN)) {
        q->setError(QFile::UnspecifiedError, qt_error_string());
        return false;
    }

    return true;
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeRead(char *data, qint64 maxlen)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdio / stdlib mode.
        if (fh && nativeIsSequential() && feof(fh)) {
            q->setError(QFile::ReadError, QSystemError::stdString());
            return -1;
        }

        return readFdFh(data, maxlen);
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return -1;

    qint64 bytesToRead = maxlen;

    // Reading on Windows fails with ERROR_NO_SYSTEM_RESOURCES when
    // the chunks are too large, so we limit the block size to 32MB.
    static const qint64 maxBlockSize = 32 * 1024 * 1024;

    qint64 totalRead = 0;
    do {
        DWORD blockSize = DWORD(qMin(bytesToRead, maxBlockSize));
        DWORD bytesRead;
        if (!ReadFile(fileHandle, data + totalRead, blockSize, &bytesRead, NULL)) {
            if (totalRead == 0) {
                // Note: only return failure if the first ReadFile fails.
                q->setError(QFile::ReadError, qt_error_string());
                return -1;
            }
            break;
        }
        if (bytesRead == 0)
            break;
        totalRead += bytesRead;
        bytesToRead -= bytesRead;
    } while (totalRead < maxlen);
    return totalRead;
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeReadLine(char *data, qint64 maxlen)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdio / stdlib mode.
        return readLineFdFh(data, maxlen);
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return -1;

    // ### No equivalent in Win32?
    return q->QAbstractFileEngine::readLine(data, maxlen);
}

/*
    \internal
*/
qint64 QFSFileEnginePrivate::nativeWrite(const char *data, qint64 len)
{
    Q_Q(QFSFileEngine);

    if (fh || fd != -1) {
        // stdio / stdlib mode.
        return writeFdFh(data, len);
    }

    // Windows native mode.
    if (fileHandle == INVALID_HANDLE_VALUE)
        return -1;

    qint64 bytesToWrite = len;

    // Writing on Windows fails with ERROR_NO_SYSTEM_RESOURCES when
    // the chunks are too large, so we limit the block size to 32MB.
    qint64 totalWritten = 0;
    do {
        const DWORD currentBlockSize = DWORD(qMin(bytesToWrite, qint64(32 * 1024 * 1024)));
        DWORD bytesWritten;
        if (!WriteFile(fileHandle, data + totalWritten, currentBlockSize, &bytesWritten, NULL)) {
            if (totalWritten == 0) {
                // Note: Only return error if the first WriteFile failed.
                q->setError(QFile::WriteError, qt_error_string());
                return -1;
            }
            break;
        }
        if (bytesWritten == 0)
            break;
        totalWritten += bytesWritten;
        bytesToWrite -= bytesWritten;
    } while (totalWritten < len);
    return qint64(totalWritten);
}

/*
    \internal
*/
int QFSFileEnginePrivate::nativeHandle() const
{
    if (fh || fd != -1)
        return fh ? QT_FILENO(fh) : fd;
    if (cachedFd != -1)
        return cachedFd;

    int flags = 0;
    if (openMode & QIODevice::Append)
        flags |= _O_APPEND;
    if (!(openMode & QIODevice::WriteOnly))
        flags |= _O_RDONLY;
    cachedFd = _open_osfhandle((intptr_t) fileHandle, flags);
    return cachedFd;
}

/*
    \internal
*/
bool QFSFileEnginePrivate::nativeIsSequential() const
{
#if !defined(Q_OS_WINRT)
    HANDLE handle = fileHandle;
    if (fh || fd != -1)
        handle = (HANDLE)_get_osfhandle(fh ? QT_FILENO(fh) : fd);
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    DWORD fileType = GetFileType(handle);
    return (fileType == FILE_TYPE_CHAR)
            || (fileType == FILE_TYPE_PIPE);
#else
    return false;
#endif
}

bool QFSFileEngine::remove()
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::removeFile(d->fileEntry, error);
    if (!ret)
        setError(QFile::RemoveError, error.toString());
    return ret;
}

bool QFSFileEngine::copy(const QString &copyName)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::copyFile(d->fileEntry, QFileSystemEntry(copyName), error);
    if (!ret)
        setError(QFile::CopyError, error.toString());
    return ret;
}

bool QFSFileEngine::rename(const QString &newName)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::renameFile(d->fileEntry, QFileSystemEntry(newName), error);
    if (!ret)
        setError(QFile::RenameError, error.toString());
    return ret;
}

bool QFSFileEngine::renameOverwrite(const QString &newName)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::renameOverwriteFile(d->fileEntry, QFileSystemEntry(newName), error);
    if (!ret)
        setError(QFile::RenameError, error.toString());
    return ret;
}

bool QFSFileEngine::mkdir(const QString &name, bool createParentDirectories) const
{
    return QFileSystemEngine::createDirectory(QFileSystemEntry(name), createParentDirectories);
}

bool QFSFileEngine::rmdir(const QString &name, bool recurseParentDirectories) const
{
    return QFileSystemEngine::removeDirectory(QFileSystemEntry(name), recurseParentDirectories);
}

bool QFSFileEngine::caseSensitive() const
{
    return false;
}

bool QFSFileEngine::setCurrentPath(const QString &path)
{
    return QFileSystemEngine::setCurrentPath(QFileSystemEntry(path));
}

QString QFSFileEngine::currentPath(const QString &fileName)
{
#if !defined(Q_OS_WINRT)
    QString ret;
    //if filename is a drive: then get the pwd of that drive
    if (fileName.length() >= 2 &&
        fileName.at(0).isLetter() && fileName.at(1) == QLatin1Char(':')) {
        int drv = fileName.toUpper().at(0).toLatin1() - 'A' + 1;
        if (_getdrive() != drv) {
            wchar_t buf[PATH_MAX];
            ::_wgetdcwd(drv, buf, PATH_MAX);
            ret = QString::fromWCharArray(buf);
        }
    }
    if (ret.isEmpty()) {
        //just the pwd
        ret = QFileSystemEngine::currentPath().filePath();
    }
    if (ret.length() >= 2 && ret[1] == QLatin1Char(':'))
        ret[0] = ret.at(0).toUpper(); // Force uppercase drive letters.
    return ret;
#else // !Q_OS_WINRT
    Q_UNUSED(fileName);
    return QFileSystemEngine::currentPath().filePath();
#endif // Q_OS_WINRT
}

QString QFSFileEngine::homePath()
{
    return QFileSystemEngine::homePath();
}

QString QFSFileEngine::rootPath()
{
    return QFileSystemEngine::rootPath();
}

QString QFSFileEngine::tempPath()
{
    return QFileSystemEngine::tempPath();
}

#if !defined(Q_OS_WINRT)
// cf QStorageInfo::isReady
static inline bool isDriveReady(const wchar_t *path)
{
    DWORD fileSystemFlags;
    const UINT driveType = GetDriveType(path);
    return (driveType != DRIVE_REMOVABLE && driveType != DRIVE_CDROM)
        || GetVolumeInformation(path, nullptr, 0, nullptr, nullptr,
                                &fileSystemFlags, nullptr, 0) == TRUE;
}
#endif // !Q_OS_WINRT

QFileInfoList QFSFileEngine::drives()
{
    QFileInfoList ret;
#if !defined(Q_OS_WINRT)
    const UINT oldErrorMode = ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    quint32 driveBits = (quint32) GetLogicalDrives() & 0x3ffffff;
    wchar_t driveName[] = L"A:\\";

    while (driveBits) {
        if ((driveBits & 1) && isDriveReady(driveName))
            ret.append(QFileInfo(QString::fromWCharArray(driveName)));
        driveName[0]++;
        driveBits = driveBits >> 1;
    }
    ::SetErrorMode(oldErrorMode);
    return ret;
#else // !Q_OS_WINRT
    ret.append(QFileInfo(QLatin1String("/")));
    return ret;
#endif // Q_OS_WINRT
}

bool QFSFileEnginePrivate::doStat(QFileSystemMetaData::MetaDataFlags flags) const
{
    if (!tried_stat || !metaData.hasFlags(flags)) {
        tried_stat = true;

        int localFd = fd;
        if (fh && fileEntry.isEmpty())
            localFd = QT_FILENO(fh);
        if (localFd != -1)
            QFileSystemEngine::fillMetaData(localFd, metaData, flags);
        if (metaData.missingFlags(flags) && !fileEntry.isEmpty())
            QFileSystemEngine::fillMetaData(fileEntry, metaData, metaData.missingFlags(flags));
    }

    return metaData.exists();
}


bool QFSFileEngine::link(const QString &newName)
{
#if !defined(Q_OS_WINRT)
    bool ret = false;

    QString linkName = newName;
    //### assume that they add .lnk

    IShellLink *psl;
    bool neededCoInit = false;

    HRESULT hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink,
                                    reinterpret_cast<void **>(&psl));

    if (hres == CO_E_NOTINITIALIZED) { // COM was not initialized
        neededCoInit = true;
        CoInitialize(nullptr);
        hres = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink,
                                reinterpret_cast<void **>(&psl));
    }

    if (SUCCEEDED(hres)) {
        const QString nativeAbsoluteName = fileName(AbsoluteName).replace(QLatin1Char('/'), QLatin1Char('\\'));
        hres = psl->SetPath(reinterpret_cast<const wchar_t *>(nativeAbsoluteName.utf16()));
        if (SUCCEEDED(hres)) {
            const QString nativeAbsolutePathName = fileName(AbsolutePathName).replace(QLatin1Char('/'), QLatin1Char('\\'));
            hres = psl->SetWorkingDirectory(reinterpret_cast<const wchar_t *>(nativeAbsolutePathName.utf16()));
            if (SUCCEEDED(hres)) {
                IPersistFile *ppf;
                hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void **>(&ppf));
                if (SUCCEEDED(hres)) {
                    hres = ppf->Save(reinterpret_cast<const wchar_t *>(linkName.utf16()), TRUE);
                    if (SUCCEEDED(hres))
                         ret = true;
                    ppf->Release();
                }
            }
        }
        psl->Release();
    }
    if (!ret)
        setError(QFile::RenameError, qt_error_string());

    if (neededCoInit)
        CoUninitialize();

    return ret;
#else // !Q_OS_WINRT
    Q_UNUSED(newName);
    Q_UNIMPLEMENTED();
    return false;
#endif // Q_OS_WINRT
}

/*!
    \reimp
*/
QAbstractFileEngine::FileFlags QFSFileEngine::fileFlags(QAbstractFileEngine::FileFlags type) const
{
    Q_D(const QFSFileEngine);

    if (type & Refresh)
        d->metaData.clear();

    QAbstractFileEngine::FileFlags ret = 0;

    if (type & FlagsMask)
        ret |= LocalDiskFlag;

    bool exists;
    {
        QFileSystemMetaData::MetaDataFlags queryFlags = 0;

        queryFlags |= QFileSystemMetaData::MetaDataFlags(uint(type))
                & QFileSystemMetaData::Permissions;

        // AliasType and BundleType are 0x0
        if (type & TypesMask)
            queryFlags |= QFileSystemMetaData::AliasType
                    | QFileSystemMetaData::LinkType
                    | QFileSystemMetaData::FileType
                    | QFileSystemMetaData::DirectoryType
                    | QFileSystemMetaData::BundleType;

        if (type & FlagsMask)
            queryFlags |= QFileSystemMetaData::HiddenAttribute
                    | QFileSystemMetaData::ExistsAttribute;

        queryFlags |= QFileSystemMetaData::LinkType;

        exists = d->doStat(queryFlags);
    }

    if (exists && (type & PermsMask))
        ret |= FileFlags(uint(d->metaData.permissions()));

    if (type & TypesMask) {
        if ((type & LinkType) && d->metaData.isLegacyLink())
            ret |= LinkType;
        if (d->metaData.isDirectory()) {
            ret |= DirectoryType;
        } else {
            ret |= FileType;
        }
    }
    if (type & FlagsMask) {
        if (d->metaData.exists()) {
            // if we succeeded in querying, then the file exists: a file on
            // Windows cannot be deleted if we have an open handle to it
            ret |= ExistsFlag;
            if (d->fileEntry.isRoot())
                ret |= RootFlag;
            else if (d->metaData.isHidden())
                ret |= HiddenFlag;
        }
    }
    return ret;
}

QByteArray QFSFileEngine::id() const
{
    Q_D(const QFSFileEngine);
    HANDLE h = d->fileHandle;
    if (h == INVALID_HANDLE_VALUE) {
        int localFd = d->fd;
        if (d->fh && d->fileEntry.isEmpty())
            localFd = QT_FILENO(d->fh);
        if (localFd != -1)
            h = HANDLE(_get_osfhandle(localFd));
    }
    if (h != INVALID_HANDLE_VALUE)
        return QFileSystemEngine::id(h);

    // file is not open, try by path
    return QFileSystemEngine::id(d->fileEntry);
}

QString QFSFileEngine::fileName(FileName file) const
{
    Q_D(const QFSFileEngine);
    if (file == BaseName) {
        return d->fileEntry.fileName();
    } else if (file == PathName) {
        return d->fileEntry.path();
    } else if (file == AbsoluteName || file == AbsolutePathName) {
        QString ret;

        if (!isRelativePath()) {
            if (d->fileEntry.filePath().startsWith(QLatin1Char('/')) || // It's a absolute path to the current drive, so \a.txt -> Z:\a.txt
                d->fileEntry.filePath().size() == 2 ||                  // It's a drive letter that needs to get a working dir appended
                (d->fileEntry.filePath().size() > 2 && d->fileEntry.filePath().at(2) != QLatin1Char('/')) || // It's a drive-relative path, so Z:a.txt -> Z:\currentpath\a.txt
                d->fileEntry.filePath().contains(QLatin1String("/../")) || d->fileEntry.filePath().contains(QLatin1String("/./")) ||
                d->fileEntry.filePath().endsWith(QLatin1String("/..")) || d->fileEntry.filePath().endsWith(QLatin1String("/.")))
            {
                ret = QDir::fromNativeSeparators(QFileSystemEngine::nativeAbsoluteFilePath(d->fileEntry.filePath()));
            } else {
                ret = d->fileEntry.filePath();
            }
        } else {
            ret = QDir::cleanPath(QDir::currentPath() + QLatin1Char('/') + d->fileEntry.filePath());
        }

        // The path should be absolute at this point.
        // From the docs :
        // Absolute paths begin with the directory separator "/"
        // (optionally preceded by a drive specification under Windows).
        if (ret.at(0) != QLatin1Char('/')) {
            Q_ASSERT(ret.length() >= 2);
            Q_ASSERT(ret.at(0).isLetter());
            Q_ASSERT(ret.at(1) == QLatin1Char(':'));

            // Force uppercase drive letters.
            ret[0] = ret.at(0).toUpper();
        }

        if (file == AbsolutePathName) {
            int slash = ret.lastIndexOf(QLatin1Char('/'));
            if (slash < 0)
                return ret;
            if (ret.at(0) != QLatin1Char('/') && slash == 2)
                return ret.left(3);      // include the slash
            return ret.left(slash > 0 ? slash : 1);
        }
        return ret;
    } else if (file == CanonicalName || file == CanonicalPathName) {
        if (!(fileFlags(ExistsFlag) & ExistsFlag))
            return QString();
        QFileSystemEntry entry(QFileSystemEngine::canonicalName(QFileSystemEntry(fileName(AbsoluteName)), d->metaData));

        if (file == CanonicalPathName)
            return entry.path();
        return entry.filePath();
    } else if (file == LinkName) {
        return QFileSystemEngine::getLinkTarget(d->fileEntry, d->metaData).filePath();
    } else if (file == BundleName) {
        return QString();
    }
    return d->fileEntry.filePath();
}

bool QFSFileEngine::isRelativePath() const
{
    Q_D(const QFSFileEngine);
    // drive, e.g. "a:", or UNC root, e.q. "//"
    return d->fileEntry.isRelative();
}

uint QFSFileEngine::ownerId(FileOwner /*own*/) const
{
    static const uint nobodyID = (uint) -2;
    return nobodyID;
}

QString QFSFileEngine::owner(FileOwner own) const
{
    Q_D(const QFSFileEngine);
    return QFileSystemEngine::owner(d->fileEntry, own);
}

bool QFSFileEngine::setPermissions(uint perms)
{
    Q_D(QFSFileEngine);
    QSystemError error;
    bool ret = QFileSystemEngine::setPermissions(d->fileEntry, QFile::Permissions(perms), error);
    if (!ret)
        setError(QFile::PermissionsError, error.toString());
    return ret;
}

bool QFSFileEngine::setSize(qint64 size)
{
    Q_D(QFSFileEngine);

    if (d->fileHandle != INVALID_HANDLE_VALUE || d->fd != -1 || d->fh) {
        // resize open file
        HANDLE fh = d->fileHandle;
        if (fh == INVALID_HANDLE_VALUE) {
            if (d->fh)
                fh = (HANDLE)_get_osfhandle(QT_FILENO(d->fh));
            else
                fh = (HANDLE)_get_osfhandle(d->fd);
        }
        if (fh == INVALID_HANDLE_VALUE)
            return false;
        qint64 currentPos = pos();

        if (seek(size) && SetEndOfFile(fh)) {
            seek(qMin(currentPos, size));
            return true;
        }

        seek(currentPos);
        return false;
    }

    if (!d->fileEntry.isEmpty()) {
        // resize file on disk
        QFile file(d->fileEntry.filePath());
        if (file.open(QFile::ReadWrite)) {
            bool ret = file.resize(size);
            if (!ret)
                setError(QFile::ResizeError, file.errorString());
            return ret;
        }
    }
    return false;
}

bool QFSFileEngine::setFileTime(const QDateTime &newDate, FileTime time)
{
    Q_D(QFSFileEngine);

    if (d->openMode == QFile::NotOpen) {
        setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
        return false;
    }

    if (!newDate.isValid() || time == QAbstractFileEngine::MetadataChangeTime) {
        setError(QFile::UnspecifiedError, qt_error_string(ERROR_INVALID_PARAMETER));
        return false;
    }

    HANDLE handle = d->fileHandle;
    if (handle == INVALID_HANDLE_VALUE) {
        if (d->fh)
            handle = reinterpret_cast<HANDLE>(::_get_osfhandle(QT_FILENO(d->fh)));
        else if (d->fd != -1)
            handle = reinterpret_cast<HANDLE>(::_get_osfhandle(d->fd));
    }

    if (handle == INVALID_HANDLE_VALUE) {
        setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
        return false;
    }

    QSystemError error;
    if (!QFileSystemEngine::setFileTime(handle, newDate, time, error)) {
        setError(QFile::PermissionsError, error.toString());
        return false;
    }

    d->metaData.clearFlags(QFileSystemMetaData::Times);
    return true;
}

uchar *QFSFileEnginePrivate::map(qint64 offset, qint64 size,
                                 QFile::MemoryMapFlags flags)
{
    Q_Q(QFSFileEngine);
    Q_UNUSED(flags);
    if (openMode == QFile::NotOpen) {
        q->setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
        return 0;
    }
    if (offset == 0 && size == 0) {
        q->setError(QFile::UnspecifiedError, qt_error_string(ERROR_INVALID_PARAMETER));
        return 0;
    }

    // check/setup args to map
    DWORD access = 0;
    if (flags & QFileDevice::MapPrivateOption) {
#ifdef FILE_MAP_COPY
        access = FILE_MAP_COPY;
#else
        q->setError(QFile::UnspecifiedError, "MapPrivateOption unsupported");
        return 0;
#endif
    } else if (openMode & QIODevice::WriteOnly) {
        access = FILE_MAP_WRITE;
    } else if (openMode & QIODevice::ReadOnly) {
        access = FILE_MAP_READ;
    }

    if (mapHandle == NULL) {
        // get handle to the file
        HANDLE handle = fileHandle;

        if (handle == INVALID_HANDLE_VALUE && fh)
            handle = (HANDLE)::_get_osfhandle(QT_FILENO(fh));

#ifdef Q_USE_DEPRECATED_MAP_API
        nativeClose();
        // handle automatically closed by kernel with mapHandle (below).
        handle = ::CreateFileForMapping((const wchar_t*)fileEntry.nativeFilePath().utf16(),
                GENERIC_READ | (openMode & QIODevice::WriteOnly ? GENERIC_WRITE : 0),
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
        // Since this is a special case, we check if the return value was NULL and if so
        // we change it to INVALID_HANDLE_VALUE to follow the logic inside this function.
        if(0 == handle)
            handle = INVALID_HANDLE_VALUE;
#endif

        if (handle == INVALID_HANDLE_VALUE) {
            q->setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
            return 0;
        }

        // first create the file mapping handle
        DWORD protection = (openMode & QIODevice::WriteOnly) ? PAGE_READWRITE : PAGE_READONLY;
#ifndef Q_OS_WINRT
        mapHandle = ::CreateFileMapping(handle, 0, protection, 0, 0, 0);
#else
        mapHandle = ::CreateFileMappingFromApp(handle, 0, protection, 0, 0);
#endif
        if (mapHandle == NULL) {
            q->setError(QFile::PermissionsError, qt_error_string());
#ifdef Q_USE_DEPRECATED_MAP_API
            ::CloseHandle(handle);
#endif
            return 0;
        }
    }

    DWORD offsetHi = offset >> 32;
    DWORD offsetLo = offset & Q_UINT64_C(0xffffffff);
    SYSTEM_INFO sysinfo;
#ifndef Q_OS_WINRT
    ::GetSystemInfo(&sysinfo);
#else
    ::GetNativeSystemInfo(&sysinfo);
#endif
    DWORD mask = sysinfo.dwAllocationGranularity - 1;
    DWORD extra = offset & mask;
    if (extra)
        offsetLo &= ~mask;

    // attempt to create the map
#ifndef Q_OS_WINRT
    LPVOID mapAddress = ::MapViewOfFile(mapHandle, access,
                                      offsetHi, offsetLo, size + extra);
#else
    LPVOID mapAddress = ::MapViewOfFileFromApp(mapHandle, access,
                                               (ULONG64(offsetHi) << 32) + offsetLo, size + extra);
#endif
    if (mapAddress) {
        uchar *address = extra + static_cast<uchar*>(mapAddress);
        maps[address] = extra;
        return address;
    }

    switch(GetLastError()) {
    case ERROR_ACCESS_DENIED:
        q->setError(QFile::PermissionsError, qt_error_string());
        break;
    case ERROR_INVALID_PARAMETER:
        // size are out of bounds
    default:
        q->setError(QFile::UnspecifiedError, qt_error_string());
    }

    ::CloseHandle(mapHandle);
    mapHandle = NULL;
    return 0;
}

bool QFSFileEnginePrivate::unmap(uchar *ptr)
{
    Q_Q(QFSFileEngine);
    if (!maps.contains(ptr)) {
        q->setError(QFile::PermissionsError, qt_error_string(ERROR_ACCESS_DENIED));
        return false;
    }
    uchar *start = ptr - maps[ptr];
    if (!UnmapViewOfFile(start)) {
        q->setError(QFile::PermissionsError, qt_error_string());
        return false;
    }

    maps.remove(ptr);
    if (maps.isEmpty()) {
        ::CloseHandle(mapHandle);
        mapHandle = NULL;
    }

    return true;
}

/*!
    \reimp
*/
bool QFSFileEngine::cloneTo(QAbstractFileEngine *target)
{
    // There's some Windows Server 2016 API, but we won't
    // bother with it.
    Q_UNUSED(target);
    return false;
}

QT_END_NAMESPACE
