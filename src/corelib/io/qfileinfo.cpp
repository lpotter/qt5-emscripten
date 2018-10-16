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
#include "qfileinfo.h"
#include "qglobal.h"
#include "qdir.h"
#include "qfileinfo_p.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

QString QFileInfoPrivate::getFileName(QAbstractFileEngine::FileName name) const
{
    if (cache_enabled && !fileNames[(int)name].isNull())
        return fileNames[(int)name];

    QString ret;
    if (fileEngine == 0) { // local file; use the QFileSystemEngine directly
        switch (name) {
            case QAbstractFileEngine::CanonicalName:
            case QAbstractFileEngine::CanonicalPathName: {
                QFileSystemEntry entry = QFileSystemEngine::canonicalName(fileEntry, metaData);
                if (cache_enabled) { // be smart and store both
                    fileNames[QAbstractFileEngine::CanonicalName] = entry.filePath();
                    fileNames[QAbstractFileEngine::CanonicalPathName] = entry.path();
                }
                if (name == QAbstractFileEngine::CanonicalName)
                    ret = entry.filePath();
                else
                    ret = entry.path();
                break;
            }
            case QAbstractFileEngine::LinkName:
                ret = QFileSystemEngine::getLinkTarget(fileEntry, metaData).filePath();
                break;
            case QAbstractFileEngine::BundleName:
                ret = QFileSystemEngine::bundleName(fileEntry);
                break;
            case QAbstractFileEngine::AbsoluteName:
            case QAbstractFileEngine::AbsolutePathName: {
                QFileSystemEntry entry = QFileSystemEngine::absoluteName(fileEntry);
                if (cache_enabled) { // be smart and store both
                    fileNames[QAbstractFileEngine::AbsoluteName] = entry.filePath();
                    fileNames[QAbstractFileEngine::AbsolutePathName] = entry.path();
                }
                if (name == QAbstractFileEngine::AbsoluteName)
                    ret = entry.filePath();
                else
                    ret = entry.path();
                break;
            }
            default: break;
        }
    } else {
        ret = fileEngine->fileName(name);
    }
    if (ret.isNull())
        ret = QLatin1String("");
    if (cache_enabled)
        fileNames[(int)name] = ret;
    return ret;
}

QString QFileInfoPrivate::getFileOwner(QAbstractFileEngine::FileOwner own) const
{
    if (cache_enabled && !fileOwners[(int)own].isNull())
        return fileOwners[(int)own];
    QString ret;
    if (fileEngine == 0) {
        switch (own) {
        case QAbstractFileEngine::OwnerUser:
            ret = QFileSystemEngine::resolveUserName(fileEntry, metaData);
            break;
        case QAbstractFileEngine::OwnerGroup:
            ret = QFileSystemEngine::resolveGroupName(fileEntry, metaData);
            break;
        }
     } else {
        ret = fileEngine->owner(own);
    }
    if (ret.isNull())
        ret = QLatin1String("");
    if (cache_enabled)
        fileOwners[(int)own] = ret;
    return ret;
}

uint QFileInfoPrivate::getFileFlags(QAbstractFileEngine::FileFlags request) const
{
    Q_ASSERT(fileEngine); // should never be called when using the native FS
    // We split the testing into tests for for LinkType, BundleType, PermsMask
    // and the rest.
    // Tests for file permissions on Windows can be slow, expecially on network
    // paths and NTFS drives.
    // In order to determine if a file is a symlink or not, we have to lstat().
    // If we're not interested in that information, we might as well avoid one
    // extra syscall. Bundle detecton on Mac can be slow, expecially on network
    // paths, so we separate out that as well.

    QAbstractFileEngine::FileFlags req = 0;
    uint cachedFlags = 0;

    if (request & (QAbstractFileEngine::FlagsMask | QAbstractFileEngine::TypesMask)) {
        if (!getCachedFlag(CachedFileFlags)) {
            req |= QAbstractFileEngine::FlagsMask;
            req |= QAbstractFileEngine::TypesMask;
            req &= (~QAbstractFileEngine::LinkType);
            req &= (~QAbstractFileEngine::BundleType);

            cachedFlags |= CachedFileFlags;
        }

        if (request & QAbstractFileEngine::LinkType) {
            if (!getCachedFlag(CachedLinkTypeFlag)) {
                req |= QAbstractFileEngine::LinkType;
                cachedFlags |= CachedLinkTypeFlag;
            }
        }

        if (request & QAbstractFileEngine::BundleType) {
            if (!getCachedFlag(CachedBundleTypeFlag)) {
                req |= QAbstractFileEngine::BundleType;
                cachedFlags |= CachedBundleTypeFlag;
            }
        }
    }

    if (request & QAbstractFileEngine::PermsMask) {
        if (!getCachedFlag(CachedPerms)) {
            req |= QAbstractFileEngine::PermsMask;
            cachedFlags |= CachedPerms;
        }
    }

    if (req) {
        if (cache_enabled)
            req &= (~QAbstractFileEngine::Refresh);
        else
            req |= QAbstractFileEngine::Refresh;

        QAbstractFileEngine::FileFlags flags = fileEngine->fileFlags(req);
        fileFlags |= uint(flags);
        setCachedFlag(cachedFlags);
    }

    return fileFlags & request;
}

QDateTime &QFileInfoPrivate::getFileTime(QAbstractFileEngine::FileTime request) const
{
    Q_ASSERT(fileEngine); // should never be called when using the native FS
    if (!cache_enabled)
        clearFlags();

    uint cf = 0;
    switch (request) {
    case QAbstractFileEngine::AccessTime:
        cf = CachedATime;
        break;
    case QAbstractFileEngine::BirthTime:
        cf = CachedBTime;
        break;
    case QAbstractFileEngine::MetadataChangeTime:
        cf = CachedMCTime;
        break;
    case QAbstractFileEngine::ModificationTime:
        cf = CachedMTime;
        break;
    }

    if (!getCachedFlag(cf)) {
        fileTimes[request] = fileEngine->fileTime(request);
        setCachedFlag(cf);
    }
    return fileTimes[request];
}

//************* QFileInfo

/*!
    \class QFileInfo
    \inmodule QtCore
    \reentrant
    \brief The QFileInfo class provides system-independent file information.

    \ingroup io
    \ingroup shared

    QFileInfo provides information about a file's name and position
    (path) in the file system, its access rights and whether it is a
    directory or symbolic link, etc. The file's size and last
    modified/read times are also available. QFileInfo can also be
    used to obtain information about a Qt \l{resource
    system}{resource}.

    A QFileInfo can point to a file with either a relative or an
    absolute file path. Absolute file paths begin with the directory
    separator "/" (or with a drive specification on Windows). Relative
    file names begin with a directory name or a file name and specify
    a path relative to the current working directory. An example of an
    absolute path is the string "/tmp/quartz". A relative path might
    look like "src/fatlib". You can use the function isRelative() to
    check whether a QFileInfo is using a relative or an absolute file
    path. You can call the function makeAbsolute() to convert a
    relative QFileInfo's path to an absolute path.

    The file that the QFileInfo works on is set in the constructor or
    later with setFile(). Use exists() to see if the file exists and
    size() to get its size.

    The file's type is obtained with isFile(), isDir() and
    isSymLink(). The symLinkTarget() function provides the name of the file
    the symlink points to.

    On Unix (including \macos and iOS), the symlink has the same size() has
    the file it points to, because Unix handles symlinks
    transparently; similarly, opening a symlink using QFile
    effectively opens the link's target. For example:

    \snippet code/src_corelib_io_qfileinfo.cpp 0

    On Windows, symlinks (shortcuts) are \c .lnk files. The reported
    size() is that of the symlink (not the link's target), and
    opening a symlink using QFile opens the \c .lnk file. For
    example:

    \snippet code/src_corelib_io_qfileinfo.cpp 1

    Elements of the file's name can be extracted with path() and
    fileName(). The fileName()'s parts can be extracted with
    baseName(), suffix() or completeSuffix(). QFileInfo objects to
    directories created by Qt classes will not have a trailing file
    separator. If you wish to use trailing separators in your own file
    info objects, just append one to the file name given to the constructors
    or setFile().

    The file's dates are returned by created(), lastModified(), lastRead() and
    fileTime(). Information about the file's access permissions is
    obtained with isReadable(), isWritable() and isExecutable(). The
    file's ownership is available from owner(), ownerId(), group() and
    groupId(). You can examine a file's permissions and ownership in a
    single statement using the permission() function.

    \target NTFS permissions
    \note On NTFS file systems, ownership and permissions checking is
    disabled by default for performance reasons. To enable it,
    include the following line:

    \snippet ntfsp.cpp 0

    Permission checking is then turned on and off by incrementing and
    decrementing \c qt_ntfs_permission_lookup by 1.

    \snippet ntfsp.cpp 1

    \section1 Performance Issues

    Some of QFileInfo's functions query the file system, but for
    performance reasons, some functions only operate on the
    file name itself. For example: To return the absolute path of
    a relative file name, absolutePath() has to query the file system.
    The path() function, however, can work on the file name directly,
    and so it is faster.

    \note To speed up performance, QFileInfo caches information about
    the file.

    Because files can be changed by other users or programs, or
    even by other parts of the same program, there is a function that
    refreshes the file information: refresh(). If you want to switch
    off a QFileInfo's caching and force it to access the file system
    every time you request information from it call setCaching(false).

    \sa QDir, QFile
*/

/*!
    \fn QFileInfo &QFileInfo::operator=(QFileInfo &&other)

    Move-assigns \a other to this QFileInfo instance.

    \since 5.2
*/

/*!
    \internal
*/
QFileInfo::QFileInfo(QFileInfoPrivate *p) : d_ptr(p)
{
}

/*!
    Constructs an empty QFileInfo object.

    Note that an empty QFileInfo object contain no file reference.

    \sa setFile()
*/
QFileInfo::QFileInfo() : d_ptr(new QFileInfoPrivate())
{
}

/*!
    Constructs a new QFileInfo that gives information about the given
    file. The \a file can also include an absolute or relative path.

    \sa setFile(), isRelative(), QDir::setCurrent(), QDir::isRelativePath()
*/
QFileInfo::QFileInfo(const QString &file) : d_ptr(new QFileInfoPrivate(file))
{
}

/*!
    Constructs a new QFileInfo that gives information about file \a
    file.

    If the \a file has a relative path, the QFileInfo will also have a
    relative path.

    \sa isRelative()
*/
QFileInfo::QFileInfo(const QFile &file) : d_ptr(new QFileInfoPrivate(file.fileName()))
{
}

/*!
    Constructs a new QFileInfo that gives information about the given
    \a file in the directory \a dir.

    If \a dir has a relative path, the QFileInfo will also have a
    relative path.

    If \a file is an absolute path, then the directory specified
    by \a dir will be disregarded.

    \sa isRelative()
*/
QFileInfo::QFileInfo(const QDir &dir, const QString &file)
    : d_ptr(new QFileInfoPrivate(dir.filePath(file)))
{
}

/*!
    Constructs a new QFileInfo that is a copy of the given \a fileinfo.
*/
QFileInfo::QFileInfo(const QFileInfo &fileinfo)
    : d_ptr(fileinfo.d_ptr)
{

}

/*!
    Destroys the QFileInfo and frees its resources.
*/

QFileInfo::~QFileInfo()
{
}

/*!
    \fn bool QFileInfo::operator!=(const QFileInfo &fileinfo) const

    Returns \c true if this QFileInfo object refers to a different file
    than the one specified by \a fileinfo; otherwise returns \c false.

    \sa operator==()
*/

/*!
    Returns \c true if this QFileInfo object refers to a file in the same
    location as \a fileinfo; otherwise returns \c false.

    Note that the result of comparing two empty QFileInfo objects,
    containing no file references (file paths that do not exist or
    are empty), is undefined.

    \warning This will not compare two different symbolic links
    pointing to the same file.

    \warning Long and short file names that refer to the same file on Windows
    are treated as if they referred to different files.

    \sa operator!=()
*/
bool QFileInfo::operator==(const QFileInfo &fileinfo) const
{
    Q_D(const QFileInfo);
    // ### Qt 5: understand long and short file names on Windows
    // ### (GetFullPathName()).
    if (fileinfo.d_ptr == d_ptr)
        return true;
    if (d->isDefaultConstructed || fileinfo.d_ptr->isDefaultConstructed)
        return false;

    // Assume files are the same if path is the same
    if (d->fileEntry.filePath() == fileinfo.d_ptr->fileEntry.filePath())
        return true;

    Qt::CaseSensitivity sensitive;
    if (d->fileEngine == 0 || fileinfo.d_ptr->fileEngine == 0) {
        if (d->fileEngine != fileinfo.d_ptr->fileEngine) // one is native, the other is a custom file-engine
            return false;

        sensitive = QFileSystemEngine::isCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    } else {
        if (d->fileEngine->caseSensitive() != fileinfo.d_ptr->fileEngine->caseSensitive())
            return false;
        sensitive = d->fileEngine->caseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    }

   // Fallback to expensive canonical path computation
   return canonicalFilePath().compare(fileinfo.canonicalFilePath(), sensitive) == 0;
}

/*!
    Makes a copy of the given \a fileinfo and assigns it to this QFileInfo.
*/
QFileInfo &QFileInfo::operator=(const QFileInfo &fileinfo)
{
    d_ptr = fileinfo.d_ptr;
    return *this;
}

/*!
    \fn void QFileInfo::swap(QFileInfo &other)
    \since 5.0

    Swaps this file info with \a other. This function is very fast and
    never fails.
*/

/*!
    Sets the file that the QFileInfo provides information about to \a
    file.

    The \a file can also include an absolute or relative file path.
    Absolute paths begin with the directory separator (e.g. "/" under
    Unix) or a drive specification (under Windows). Relative file
    names begin with a directory name or a file name and specify a
    path relative to the current directory.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 2

    \sa isRelative(), QDir::setCurrent(), QDir::isRelativePath()
*/
void QFileInfo::setFile(const QString &file)
{
    bool caching = d_ptr.constData()->cache_enabled;
    *this = QFileInfo(file);
    d_ptr->cache_enabled = caching;
}

/*!
    \overload

    Sets the file that the QFileInfo provides information about to \a
    file.

    If \a file includes a relative path, the QFileInfo will also have
    a relative path.

    \sa isRelative()
*/
void QFileInfo::setFile(const QFile &file)
{
    setFile(file.fileName());
}

/*!
    \overload

    Sets the file that the QFileInfo provides information about to \a
    file in directory \a dir.

    If \a file includes a relative path, the QFileInfo will also
    have a relative path.

    \sa isRelative()
*/
void QFileInfo::setFile(const QDir &dir, const QString &file)
{
    setFile(dir.filePath(file));
}

/*!
    Returns an absolute path including the file name.

    The absolute path name consists of the full path and the file
    name. On Unix this will always begin with the root, '/',
    directory. On Windows this will always begin 'D:/' where D is a
    drive letter, except for network shares that are not mapped to a
    drive letter, in which case the path will begin '//sharename/'.
    QFileInfo will uppercase drive letters. Note that QDir does not do
    this. The code snippet below shows this.

    \snippet code/src_corelib_io_qfileinfo.cpp newstuff

    This function returns the same as filePath(), unless isRelative()
    is true. In contrast to canonicalFilePath(), symbolic links or
    redundant "." or ".." elements are not necessarily removed.

    \warning If filePath() is empty the behavior of this function
            is undefined.

    \sa filePath(), canonicalFilePath(), isRelative()
*/
QString QFileInfo::absoluteFilePath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileName(QAbstractFileEngine::AbsoluteName);
}

/*!
    Returns the canonical path including the file name, i.e. an absolute
    path without symbolic links or redundant "." or ".." elements.

    If the file does not exist, canonicalFilePath() returns an empty
    string.

    \sa filePath(), absoluteFilePath(), dir()
*/
QString QFileInfo::canonicalFilePath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileName(QAbstractFileEngine::CanonicalName);
}


/*!
    Returns a file's path absolute path. This doesn't include the
    file name.

    On Unix the absolute path will always begin with the root, '/',
    directory. On Windows this will always begin 'D:/' where D is a
    drive letter, except for network shares that are not mapped to a
    drive letter, in which case the path will begin '//sharename/'.

    In contrast to canonicalPath() symbolic links or redundant "." or
    ".." elements are not necessarily removed.

    \warning If filePath() is empty the behavior of this function
             is undefined.

    \sa absoluteFilePath(), path(), canonicalPath(), fileName(), isRelative()
*/
QString QFileInfo::absolutePath() const
{
    Q_D(const QFileInfo);

    if (d->isDefaultConstructed) {
        return QLatin1String("");
    }
    return d->getFileName(QAbstractFileEngine::AbsolutePathName);
}

/*!
    Returns the file's path canonical path (excluding the file name),
    i.e. an absolute path without symbolic links or redundant "." or ".." elements.

    If the file does not exist, canonicalPath() returns an empty string.

    \sa path(), absolutePath()
*/
QString QFileInfo::canonicalPath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileName(QAbstractFileEngine::CanonicalPathName);
}

/*!
    Returns the file's path. This doesn't include the file name.

    Note that, if this QFileInfo object is given a path ending in a
    slash, the name of the file is considered empty and this function
    will return the entire path.

    \sa filePath(), absolutePath(), canonicalPath(), dir(), fileName(), isRelative()
*/
QString QFileInfo::path() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.path();
}

/*!
    \fn bool QFileInfo::isAbsolute() const

    Returns \c true if the file path name is absolute, otherwise returns
    false if the path is relative.

    \sa isRelative()
*/

/*!
    Returns \c true if the file path name is relative, otherwise returns
    false if the path is absolute (e.g. under Unix a path is absolute
    if it begins with a "/").

    \sa isAbsolute()
*/
bool QFileInfo::isRelative() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return true;
    if (d->fileEngine == 0)
        return d->fileEntry.isRelative();
    return d->fileEngine->isRelativePath();
}

/*!
    Converts the file's path to an absolute path if it is not already in that form.
    Returns \c true to indicate that the path was converted; otherwise returns \c false
    to indicate that the path was already absolute.

    \sa filePath(), isRelative()
*/
bool QFileInfo::makeAbsolute()
{
    if (d_ptr.constData()->isDefaultConstructed
            || !d_ptr.constData()->fileEntry.isRelative())
        return false;

    setFile(absoluteFilePath());
    return true;
}

/*!
    Returns \c true if the file exists; otherwise returns \c false.

    \note If the file is a symlink that points to a non-existing
    file, false is returned.
*/
bool QFileInfo::exists() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::ExistsAttribute))
            QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::ExistsAttribute);
        return d->metaData.exists();
    }
    return d->getFileFlags(QAbstractFileEngine::ExistsFlag);
}

/*!
    \since 5.2

    Returns \c true if the \a file exists; otherwise returns \c false.

    \note If \a file is a symlink that points to a non-existing
    file, false is returned.

    \note Using this function is faster than using
    \c QFileInfo(file).exists() for file system access.
*/
bool QFileInfo::exists(const QString &file)
{
    QFileSystemEntry entry(file);
    QFileSystemMetaData data;
    QAbstractFileEngine *engine =
        QFileSystemEngine::resolveEntryAndCreateLegacyEngine(entry, data);
    // Expensive fallback to non-QFileSystemEngine implementation
    if (engine)
        return QFileInfo(new QFileInfoPrivate(entry, data, engine)).exists();

    QFileSystemEngine::fillMetaData(entry, data, QFileSystemMetaData::ExistsAttribute);
    return data.exists();
}

/*!
    Refreshes the information about the file, i.e. reads in information
    from the file system the next time a cached property is fetched.
*/
void QFileInfo::refresh()
{
    Q_D(QFileInfo);
    d->clear();
}

/*!
    Returns the file name, including the path (which may be absolute
    or relative).

    \sa absoluteFilePath(), canonicalFilePath(), isRelative()
*/
QString QFileInfo::filePath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.filePath();
}

/*!
    Returns the name of the file, excluding the path.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 3

    Note that, if this QFileInfo object is given a path ending in a
    slash, the name of the file is considered empty.

    \sa isRelative(), filePath(), baseName(), suffix()
*/
QString QFileInfo::fileName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.fileName();
}

/*!
    \since 4.3
    Returns the name of the bundle.

    On \macos and iOS this returns the proper localized name for a bundle if the
    path isBundle(). On all other platforms an empty QString is returned.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 4

    \sa isBundle(), filePath(), baseName(), suffix()
*/
QString QFileInfo::bundleName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileName(QAbstractFileEngine::BundleName);
}

/*!
    Returns the base name of the file without the path.

    The base name consists of all characters in the file up to (but
    not including) the \e first '.' character.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 5


    The base name of a file is computed equally on all platforms, independent
    of file naming conventions (e.g., ".bashrc" on Unix has an empty base
    name, and the suffix is "bashrc").

    \sa fileName(), suffix(), completeSuffix(), completeBaseName()
*/
QString QFileInfo::baseName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.baseName();
}

/*!
    Returns the complete base name of the file without the path.

    The complete base name consists of all characters in the file up
    to (but not including) the \e last '.' character.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 6

    \sa fileName(), suffix(), completeSuffix(), baseName()
*/
QString QFileInfo::completeBaseName() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.completeBaseName();
}

/*!
    Returns the complete suffix (extension) of the file.

    The complete suffix consists of all characters in the file after
    (but not including) the first '.'.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 7

    \sa fileName(), suffix(), baseName(), completeBaseName()
*/
QString QFileInfo::completeSuffix() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.completeSuffix();
}

/*!
    Returns the suffix (extension) of the file.

    The suffix consists of all characters in the file after (but not
    including) the last '.'.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 8

    The suffix of a file is computed equally on all platforms, independent of
    file naming conventions (e.g., ".bashrc" on Unix has an empty base name,
    and the suffix is "bashrc").

    \sa fileName(), completeSuffix(), baseName(), completeBaseName()
*/
QString QFileInfo::suffix() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->fileEntry.suffix();
}


/*!
    Returns the path of the object's parent directory as a QDir object.

    \b{Note:} The QDir returned always corresponds to the object's
    parent directory, even if the QFileInfo represents a directory.

    For each of the following, dir() returns the QDir
    \c{"~/examples/191697"}.

    \snippet fileinfo/main.cpp 0

    For each of the following, dir() returns the QDir
    \c{"."}.

    \snippet fileinfo/main.cpp 1

    \sa absolutePath(), filePath(), fileName(), isRelative(), absoluteDir()
*/
QDir QFileInfo::dir() const
{
    Q_D(const QFileInfo);
    // ### Qt 6: Maybe rename this to parentDirectory(), considering what it actually does?
    return QDir(d->fileEntry.path());
}

/*!
    Returns the file's absolute path as a QDir object.

    \sa dir(), filePath(), fileName(), isRelative()
*/
QDir QFileInfo::absoluteDir() const
{
    return QDir(absolutePath());
}

/*!
    Returns \c true if the user can read the file; otherwise returns \c false.

    \note If the \l{NTFS permissions} check has not been enabled, the result
    on Windows will merely reflect whether the file exists.

    \sa isWritable(), isExecutable(), permission()
*/
bool QFileInfo::isReadable() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::UserReadPermission,
                [d]() { return (d->metaData.permissions() & QFile::ReadUser) != 0; },
                [d]() { return d->getFileFlags(QAbstractFileEngine::ReadUserPerm); });
}

/*!
    Returns \c true if the user can write to the file; otherwise returns \c false.

    \note If the \l{NTFS permissions} check has not been enabled, the result on
    Windows will merely reflect whether the file is marked as Read Only.

    \sa isReadable(), isExecutable(), permission()
*/
bool QFileInfo::isWritable() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::UserWritePermission,
                [d]() { return (d->metaData.permissions() & QFile::WriteUser) != 0; },
                [d]() { return d->getFileFlags(QAbstractFileEngine::WriteUserPerm); });
}

/*!
    Returns \c true if the file is executable; otherwise returns \c false.

    \sa isReadable(), isWritable(), permission()
*/
bool QFileInfo::isExecutable() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::UserExecutePermission,
                [d]() { return (d->metaData.permissions() & QFile::ExeUser) != 0; },
                [d]() { return d->getFileFlags(QAbstractFileEngine::ExeUserPerm); });
}

/*!
    Returns \c true if this is a `hidden' file; otherwise returns \c false.

    \b{Note:} This function returns \c true for the special entries
    "." and ".." on Unix, even though QDir::entryList threats them as shown.
*/
bool QFileInfo::isHidden() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::HiddenAttribute,
                [d]() { return d->metaData.isHidden(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::HiddenFlag); });
}

/*!
    \since 5.0
    Returns \c true if the file path can be used directly with native APIs.
    Returns \c false if the file is otherwise supported by a virtual file system
    inside Qt, such as \l{the Qt Resource System}.

    \b{Note:} Native paths may still require conversion of path separators
    and character encoding, depending on platform and input requirements of the
    native API.

    \sa QDir::toNativeSeparators(), QFile::encodeName(), filePath(),
    absoluteFilePath(), canonicalFilePath()
*/
bool QFileInfo::isNativePath() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0)
        return true;
    return d->getFileFlags(QAbstractFileEngine::LocalDiskFlag);
}

/*!
    Returns \c true if this object points to a file or to a symbolic
    link to a file. Returns \c false if the
    object points to something which isn't a file, such as a directory.

    \sa isDir(), isSymLink(), isBundle()
*/
bool QFileInfo::isFile() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::FileType,
                [d]() { return d->metaData.isFile(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::FileType); });
}

/*!
    Returns \c true if this object points to a directory or to a symbolic
    link to a directory; otherwise returns \c false.

    \sa isFile(), isSymLink(), isBundle()
*/
bool QFileInfo::isDir() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::DirectoryType,
                [d]() { return d->metaData.isDirectory(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::DirectoryType); });
}


/*!
    \since 4.3
    Returns \c true if this object points to a bundle or to a symbolic
    link to a bundle on \macos and iOS; otherwise returns \c false.

    \sa isDir(), isSymLink(), isFile()
*/
bool QFileInfo::isBundle() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::BundleType,
                [d]() { return d->metaData.isBundle(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::BundleType); });
}

/*!
    Returns \c true if this object points to a symbolic link;
    otherwise returns \c false.

    Symbolic links exist on Unix (including \macos and iOS) and Windows
    and are typically created by the \c{ln -s} or \c{mklink} commands,
    respectively. Opening a symbolic link effectively opens
    the \l{symLinkTarget()}{link's target}.

    In addition, true will be returned for shortcuts (\c *.lnk files) on
    Windows. Opening those will open the \c .lnk file itself.

    Example:

    \snippet code/src_corelib_io_qfileinfo.cpp 9

    \note If the symlink points to a non existing file, exists() returns
     false.

    \sa isFile(), isDir(), symLinkTarget()
*/
bool QFileInfo::isSymLink() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<bool>(
                QFileSystemMetaData::LegacyLinkType,
                [d]() { return d->metaData.isLegacyLink(); },
                [d]() { return d->getFileFlags(QAbstractFileEngine::LinkType); });
}

/*!
    Returns \c true if the object points to a directory or to a symbolic
    link to a directory, and that directory is the root directory; otherwise
    returns \c false.
*/
bool QFileInfo::isRoot() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return false;
    if (d->fileEngine == 0) {
        if (d->fileEntry.isRoot()) {
#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
            //the path is a drive root, but the drive may not exist
            //for backward compatibility, return true only if the drive exists
            if (!d->cache_enabled || !d->metaData.hasFlags(QFileSystemMetaData::ExistsAttribute))
                QFileSystemEngine::fillMetaData(d->fileEntry, d->metaData, QFileSystemMetaData::ExistsAttribute);
            return d->metaData.exists();
#else
            return true;
#endif
        }
        return false;
    }
    return d->getFileFlags(QAbstractFileEngine::RootFlag);
}

/*!
    \fn QString QFileInfo::symLinkTarget() const
    \since 4.2

    Returns the absolute path to the file or directory a symbolic link
    points to, or an empty string if the object isn't a symbolic
    link.

    This name may not represent an existing file; it is only a string.
    QFileInfo::exists() returns \c true if the symlink points to an
    existing file.

    \sa exists(), isSymLink(), isDir(), isFile()
*/

/*!
    \obsolete

    Use symLinkTarget() instead.
*/
QString QFileInfo::readLink() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileName(QAbstractFileEngine::LinkName);
}

/*!
    Returns the owner of the file. On systems where files
    do not have owners, or if an error occurs, an empty string is
    returned.

    This function can be time consuming under Unix (in the order of
    milliseconds). On Windows, it will return an empty string unless
    the \l{NTFS permissions} check has been enabled.

    \sa ownerId(), group(), groupId()
*/
QString QFileInfo::owner() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileOwner(QAbstractFileEngine::OwnerUser);
}

/*!
    Returns the id of the owner of the file.

    On Windows and on systems where files do not have owners this
    function returns ((uint) -2).

    \sa owner(), group(), groupId()
*/
uint QFileInfo::ownerId() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute(uint(-2),
                QFileSystemMetaData::UserId,
                [d]() { return d->metaData.userId(); },
                [d]() { return d->fileEngine->ownerId(QAbstractFileEngine::OwnerUser); });
}

/*!
    Returns the group of the file. On Windows, on systems where files
    do not have groups, or if an error occurs, an empty string is
    returned.

    This function can be time consuming under Unix (in the order of
    milliseconds).

    \sa groupId(), owner(), ownerId()
*/
QString QFileInfo::group() const
{
    Q_D(const QFileInfo);
    if (d->isDefaultConstructed)
        return QLatin1String("");
    return d->getFileOwner(QAbstractFileEngine::OwnerGroup);
}

/*!
    Returns the id of the group the file belongs to.

    On Windows and on systems where files do not have groups this
    function always returns (uint) -2.

    \sa group(), owner(), ownerId()
*/
uint QFileInfo::groupId() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute(uint(-2),
                QFileSystemMetaData::GroupId,
                [d]() { return d->metaData.groupId(); },
                [d]() { return d->fileEngine->ownerId(QAbstractFileEngine::OwnerGroup); });
}

/*!
    Tests for file permissions. The \a permissions argument can be
    several flags of type QFile::Permissions OR-ed together to check
    for permission combinations.

    On systems where files do not have permissions this function
    always returns \c true.

    \note The result might be inaccurate on Windows if the
    \l{NTFS permissions} check has not been enabled.

    Example:
    \snippet code/src_corelib_io_qfileinfo.cpp 10

    \sa isReadable(), isWritable(), isExecutable()
*/
bool QFileInfo::permission(QFile::Permissions permissions) const
{
    Q_D(const QFileInfo);
    // the QFileSystemMetaData::MetaDataFlag and QFile::Permissions overlap, so just cast.
    auto fseFlags = QFileSystemMetaData::MetaDataFlag(int(permissions));
    auto feFlags = QAbstractFileEngine::FileFlags(int(permissions));
    return d->checkAttribute<bool>(
                fseFlags,
                [=]() { return (d->metaData.permissions() & permissions) == permissions; },
        [=]() {
            return d->getFileFlags(feFlags) == uint(permissions);
        });
}

/*!
    Returns the complete OR-ed together combination of
    QFile::Permissions for the file.

    \note The result might be inaccurate on Windows if the
    \l{NTFS permissions} check has not been enabled.
*/
QFile::Permissions QFileInfo::permissions() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<QFile::Permissions>(
                QFileSystemMetaData::Permissions,
                [d]() { return d->metaData.permissions(); },
        [d]() {
            return QFile::Permissions(d->getFileFlags(QAbstractFileEngine::PermsMask) & QAbstractFileEngine::PermsMask);
        });
}


/*!
    Returns the file size in bytes. If the file does not exist or cannot be
    fetched, 0 is returned.

    \sa exists()
*/
qint64 QFileInfo::size() const
{
    Q_D(const QFileInfo);
    return d->checkAttribute<qint64>(
                QFileSystemMetaData::SizeAttribute,
                [d]() { return d->metaData.size(); },
        [d]() {
            if (!d->getCachedFlag(QFileInfoPrivate::CachedSize)) {
                d->setCachedFlag(QFileInfoPrivate::CachedSize);
                d->fileSize = d->fileEngine->size();
            }
            return d->fileSize;
        });
}

#if QT_DEPRECATED_SINCE(5, 10)
/*!
    \deprecated

    Returns the date and time when the file was created, the time its metadata
    was last changed or the time of last modification, whichever one of the
    three is available (in that order).

    This function is deprecated. Instead, use the birthTime() function to get
    the time the file was created, metadataChangeTime() to get the time its
    metadata was last changed, or lastModified() to get the time it was last modified.

    \sa birthTime(), metadataChangeTime(), lastModified(), lastRead()
*/
QDateTime QFileInfo::created() const
{
    QDateTime d = fileTime(QFile::FileBirthTime);
    if (d.isValid())
        return d;
    return fileTime(QFile::FileMetadataChangeTime);
}
#endif

/*!
    \since 5.10
    Returns the date and time when the file was created / born.

    If the file birth time is not available, this function returns an invalid
    QDateTime.

    \sa lastModified(), lastRead(), metadataChangeTime()
*/
QDateTime QFileInfo::birthTime() const
{
    return fileTime(QFile::FileBirthTime);
}

/*!
    \since 5.10
    Returns the date and time when the file metadata was changed. A metadata
    change occurs when the file is created, but it also occurs whenever the
    user writes or sets inode information (for example, changing the file
    permissions).

    \sa lastModified(), lastRead()
*/
QDateTime QFileInfo::metadataChangeTime() const
{
    return fileTime(QFile::FileMetadataChangeTime);
}

/*!
    Returns the date and local time when the file was last modified.

    \sa birthTime(), lastRead(), metadataChangeTime(), fileTime()
*/
QDateTime QFileInfo::lastModified() const
{
    return fileTime(QFile::FileModificationTime);
}

/*!
    Returns the date and local time when the file was last read (accessed).

    On platforms where this information is not available, returns the
    same as lastModified().

    \sa birthTime(), lastModified(), metadataChangeTime(), fileTime()
*/
QDateTime QFileInfo::lastRead() const
{
    return fileTime(QFile::FileAccessTime);
}

/*!
    \since 5.10

    Returns the file time specified by \a time. If the time cannot be
    determined, an invalid date time is returned.

    \sa QFile::FileTime, QDateTime::isValid()
*/
QDateTime QFileInfo::fileTime(QFile::FileTime time) const
{
    Q_STATIC_ASSERT(int(QFile::FileAccessTime) == int(QAbstractFileEngine::AccessTime));
    Q_STATIC_ASSERT(int(QFile::FileBirthTime) == int(QAbstractFileEngine::BirthTime));
    Q_STATIC_ASSERT(int(QFile::FileMetadataChangeTime) == int(QAbstractFileEngine::MetadataChangeTime));
    Q_STATIC_ASSERT(int(QFile::FileModificationTime) == int(QAbstractFileEngine::ModificationTime));

    Q_D(const QFileInfo);
    auto fetime = QAbstractFileEngine::FileTime(time);
    QFileSystemMetaData::MetaDataFlags flag;
    switch (time) {
    case QFile::FileAccessTime:
        flag = QFileSystemMetaData::AccessTime;
        break;
    case QFile::FileBirthTime:
        flag = QFileSystemMetaData::BirthTime;
        break;
    case QFile::FileMetadataChangeTime:
        flag = QFileSystemMetaData::MetadataChangeTime;
        break;
    case QFile::FileModificationTime:
        flag = QFileSystemMetaData::ModificationTime;
        break;
    }

    return d->checkAttribute<QDateTime>(
                flag,
                [=]() { return d->metaData.fileTime(fetime).toLocalTime(); },
                [=]() { return d->getFileTime(fetime).toLocalTime(); });
}

/*!
    \internal
*/
QFileInfoPrivate* QFileInfo::d_func()
{
    return d_ptr.data();
}

/*!
    Returns \c true if caching is enabled; otherwise returns \c false.

    \sa setCaching(), refresh()
*/
bool QFileInfo::caching() const
{
    Q_D(const QFileInfo);
    return d->cache_enabled;
}

/*!
    If \a enable is true, enables caching of file information. If \a
    enable is false caching is disabled.

    When caching is enabled, QFileInfo reads the file information from
    the file system the first time it's needed, but generally not
    later.

    Caching is enabled by default.

    \sa refresh(), caching()
*/
void QFileInfo::setCaching(bool enable)
{
    Q_D(QFileInfo);
    d->cache_enabled = enable;
}

/*!
    \typedef QFileInfoList
    \relates QFileInfo

    Synonym for QList<QFileInfo>.
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QFileInfo &fi)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg.noquote();
    dbg << "QFileInfo(" << QDir::toNativeSeparators(fi.filePath()) << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE
