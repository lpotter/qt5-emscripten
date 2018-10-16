/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2017 Intel Corporation.
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

#include "qtemporarydir.h"

#ifndef QT_NO_TEMPORARYFILE

#include "qdebug.h"
#include "qdiriterator.h"
#include "qpair.h"
#include "qplatformdefs.h"
#include "qrandom.h"
#include "private/qtemporaryfile_p.h"

#if defined(QT_BUILD_CORE_LIB)
#include "qcoreapplication.h"
#endif

#if !defined(Q_OS_WIN)
#include <errno.h>
#endif

QT_BEGIN_NAMESPACE

//************* QTemporaryDirPrivate
class QTemporaryDirPrivate
{
public:
    QTemporaryDirPrivate();
    ~QTemporaryDirPrivate();

    void create(const QString &templateName);

    QString pathOrError;
    bool autoRemove;
    bool success;
};

QTemporaryDirPrivate::QTemporaryDirPrivate()
    : autoRemove(true),
      success(false)
{
}

QTemporaryDirPrivate::~QTemporaryDirPrivate()
{
}

static QString defaultTemplateName()
{
    QString baseName;
#if defined(QT_BUILD_CORE_LIB)
    baseName = QCoreApplication::applicationName();
    if (baseName.isEmpty())
#endif
        baseName = QLatin1String("qt_temp");

    return QDir::tempPath() + QLatin1Char('/') + baseName + QLatin1String("-XXXXXX");
}

void QTemporaryDirPrivate::create(const QString &templateName)
{
    QTemporaryFileName tfn(templateName);
    for (int i = 0; i < 256; ++i) {
        tfn.generateNext();
        QFileSystemEntry fileSystemEntry(tfn.path, QFileSystemEntry::FromNativePath());
        if (QFileSystemEngine::createDirectory(fileSystemEntry, false)) {
            QSystemError error;
            QFileSystemEngine::setPermissions(fileSystemEntry,
                                              QFile::ReadOwner |
                                              QFile::WriteOwner |
                                              QFile::ExeOwner, error);
            if (error.error() != 0) {
                if (!QFileSystemEngine::removeDirectory(fileSystemEntry, false))
                    qWarning() << "Unable to remove unused directory" << templateName;
                continue;
            }
            success = true;
            pathOrError = fileSystemEntry.filePath();
            return;
        }
#  ifdef Q_OS_WIN
        const int exists = ERROR_ALREADY_EXISTS;
        int code = GetLastError();
#  else
        const int exists = EEXIST;
        int code = errno;
#  endif
        if (code != exists)
            break;
    }
    pathOrError = qt_error_string();
    success = false;
}

//************* QTemporaryDir

/*!
    \class QTemporaryDir
    \inmodule QtCore
    \reentrant
    \brief The QTemporaryDir class creates a unique directory for temporary use.

    \ingroup io


    QTemporaryDir is used to create unique temporary directories safely.
    The directory itself is created by the constructor. The name of the
    temporary directory is guaranteed to be unique (i.e., you are
    guaranteed to not overwrite an existing directory), and the directory will
    subsequently be removed upon destruction of the QTemporaryDir
    object. The directory name is either auto-generated, or created based
    on a template, which is passed to QTemporaryDir's constructor.

    Example:

    \snippet code/src_corelib_io_qtemporarydir.cpp 0

    It is very important to test that the temporary directory could be
    created, using isValid(). Do not use \l {QDir::exists()}{exists()}, since a default-constructed
    QDir represents the current directory, which exists.

    The path to the temporary directory can be found by calling path().

    A temporary directory will have some static part of the name and some
    part that is calculated to be unique. The default path will be
    determined from QCoreApplication::applicationName() (otherwise \c qt_temp) and will
    be placed into the temporary path as returned by QDir::tempPath().
    If you specify your own path, a relative path will not be placed in the
    temporary directory by default, but be relative to the current working directory.
    In all cases, a random string will be appended to the path in order to make it unique.

    \sa QDir::tempPath(), QDir, QTemporaryFile
*/

/*!
    Constructs a QTemporaryDir using as template the application name
    returned by QCoreApplication::applicationName() (otherwise \c qt_temp).
    The directory is stored in the system's temporary directory, QDir::tempPath().

    \sa QDir::tempPath()
*/
QTemporaryDir::QTemporaryDir()
    : d_ptr(new QTemporaryDirPrivate)
{
    d_ptr->create(defaultTemplateName());
}

/*!
    Constructs a QTemporaryDir with a template of \a templatePath.

    If \a templatePath is a relative path, the path will be relative to the
    current working directory. You can use QDir::tempPath() to construct \a
    templatePath if you want use the system's temporary directory.

    If the \a templatePath ends with XXXXXX it will be used as the dynamic portion
    of the directory name, otherwise it will be appended.
    Unlike QTemporaryFile, XXXXXX in the middle of the template string is not supported.

    \sa QDir::tempPath()
*/
QTemporaryDir::QTemporaryDir(const QString &templatePath)
    : d_ptr(new QTemporaryDirPrivate)
{
    if (templatePath.isEmpty())
        d_ptr->create(defaultTemplateName());
    else
        d_ptr->create(templatePath);
}

/*!
    Destroys the temporary directory object.
    If auto remove mode was set, it will automatically delete the directory
    including all its contents.

    \sa autoRemove()
*/
QTemporaryDir::~QTemporaryDir()
{
    if (d_ptr->autoRemove)
        remove();
}

/*!
   Returns \c true if the QTemporaryDir was created successfully.
*/
bool QTemporaryDir::isValid() const
{
    return d_ptr->success;
}

/*!
   \since 5.6

   If isValid() returns \c false, this function returns the error string that
   explains why the creation of the temporary directory failed. Otherwise, this
   function return an empty string.
*/
QString QTemporaryDir::errorString() const
{
    return d_ptr->success ? QString() : d_ptr->pathOrError;
}

/*!
   Returns the path to the temporary directory.
   Empty if the QTemporaryDir could not be created.
*/
QString QTemporaryDir::path() const
{
    return d_ptr->success ? d_ptr->pathOrError : QString();
}

/*!
    \since 5.9

    Returns the path name of a file in the temporary directory.
    Does \e not check if the file actually exists in the directory.
    Redundant multiple separators or "." and ".." directories in
    \a fileName are not removed (see QDir::cleanPath()). Absolute
    paths are not allowed.
*/
QString QTemporaryDir::filePath(const QString &fileName) const
{
    if (QDir::isAbsolutePath(fileName)) {
        qWarning("QTemporaryDir::filePath: Absolute paths are not allowed: %s", qUtf8Printable(fileName));
        return QString();
    }

    if (!d_ptr->success)
        return QString();

    QString ret = d_ptr->pathOrError;
    if (!fileName.isEmpty()) {
        ret += QLatin1Char('/');
        ret += fileName;
    }
    return ret;
}

/*!
   Returns \c true if the QTemporaryDir is in auto remove
   mode. Auto-remove mode will automatically delete the directory from
   disk upon destruction. This makes it very easy to create your
   QTemporaryDir object on the stack, fill it with files, do something with
   the files, and finally on function return it will automatically clean up
   after itself.

   Auto-remove is on by default.

   \sa setAutoRemove(), remove()
*/
bool QTemporaryDir::autoRemove() const
{
    return d_ptr->autoRemove;
}

/*!
    Sets the QTemporaryDir into auto-remove mode if \a b is true.

    Auto-remove is on by default.

    \sa autoRemove(), remove()
*/
void QTemporaryDir::setAutoRemove(bool b)
{
    d_ptr->autoRemove = b;
}

/*!
    Removes the temporary directory, including all its contents.

    Returns \c true if removing was successful.
*/
bool QTemporaryDir::remove()
{
    if (!d_ptr->success)
        return false;
    Q_ASSERT(!path().isEmpty());
    Q_ASSERT(path() != QLatin1String("."));

    const bool result = QDir(path()).removeRecursively();
    if (!result) {
        qWarning() << "QTemporaryDir: Unable to remove"
                   << QDir::toNativeSeparators(path())
                   << "most likely due to the presence of read-only files.";
    }
    return result;
}

QT_END_NAMESPACE

#endif // QT_NO_TEMPORARYFILE
