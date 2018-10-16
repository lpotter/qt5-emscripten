/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include "qstandardpaths.h"

#include <qdir.h>
#include <qfileinfo.h>

#ifndef QT_BOOTSTRAPPED
#include <qobject.h>
#include <qcoreapplication.h>
#endif

#if QT_HAS_INCLUDE(<paths.h>)
#include <paths.h>
#endif

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

#ifndef QT_NO_STANDARDPATHS

QT_BEGIN_NAMESPACE

/*!
    \class QStandardPaths
    \inmodule QtCore
    \brief The QStandardPaths class provides methods for accessing standard paths.
    \since 5.0

    This class contains functions to query standard locations on the local
    filesystem, for common tasks such as user-specific directories or system-wide
    configuration directories.
*/

/*!
    \enum QStandardPaths::StandardLocation

    This enum describes the different locations that can be queried using
    methods such as QStandardPaths::writableLocation, QStandardPaths::standardLocations,
    and QStandardPaths::displayName.

    Some of the values in this enum represent a user configuration. Such enum
    values will return the same paths in different applications, so they could
    be used to share data with other applications. Other values are specific to
    this application. Each enum value in the table below describes whether it's
    application-specific or generic.

    Application-specific directories should be assumed to be unreachable by
    other applications. Therefore, files placed there might not be readable by
    other applications, even if run by the same user. On the other hand, generic
    directories should be assumed to be accessible by all applications run by
    this user, but should still be assumed to be unreachable by applications by
    other users.

    Data interchange with other users is out of the scope of QStandardPaths.

    \value DesktopLocation Returns the user's desktop directory. This is a generic value.
           On systems with no concept of a desktop.
    \value DocumentsLocation Returns the directory containing user document files.
           This is a generic value. The returned path is never empty.
    \value FontsLocation Returns the directory containing user's fonts. This is a generic value.
           Note that installing fonts may require additional, platform-specific operations.
    \value ApplicationsLocation Returns the directory containing the user applications
           (either executables, application bundles, or shortcuts to them). This is a generic value.
           Note that installing applications may require additional, platform-specific operations.
           Files, folders or shortcuts in this directory are platform-specific.
    \value MusicLocation Returns the directory containing the user's music or other audio files.
           This is a generic value. If no directory specific for music files exists, a sensible
           fallback for storing user documents is returned.
    \value MoviesLocation Returns the directory containing the user's movies and videos.
           This is a generic value. If no directory specific for movie files exists, a sensible
           fallback for storing user documents is returned.
    \value PicturesLocation Returns the directory containing the user's pictures or photos.
           This is a generic value. If no directory specific for picture files exists, a sensible
           fallback for storing user documents is returned.
    \value TempLocation Returns a directory where temporary files can be stored. The returned value
           might be application-specific, shared among other applications for this user, or even
           system-wide. The returned path is never empty.
    \value HomeLocation Returns the user's home directory (the same as QDir::homePath()). On Unix
           systems, this is equal to the HOME environment variable. This value might be
           generic or application-specific, but the returned path is never empty.
    \value DataLocation Returns the same value as AppLocalDataLocation. This enumeration value
           is deprecated. Using AppDataLocation is preferable since on Windows, the roaming path is
           recommended.
    \value CacheLocation Returns a directory location where user-specific
           non-essential (cached) data should be written. This is an application-specific directory.
           The returned path is never empty.
    \value GenericCacheLocation Returns a directory location where user-specific non-essential
           (cached) data, shared across applications, should be written. This is a generic value.
           Note that the returned path may be empty if the system has no concept of shared cache.
    \value GenericDataLocation Returns a directory location where persistent
           data shared across applications can be stored. This is a generic value. The returned
           path is never empty.
    \value RuntimeLocation Returns a directory location where runtime communication
           files should be written, like Unix local sockets. This is a generic value.
           The returned path may be empty on some systems.
    \value ConfigLocation Returns a directory location where user-specific
           configuration files should be written. This may be either a generic value
           or application-specific, and the returned path is never empty.
    \value DownloadLocation Returns a directory for user's downloaded files. This is a generic value.
           If no directory specific for downloads exists, a sensible fallback for storing user
           documents is returned.
    \value GenericConfigLocation Returns a directory location where user-specific
           configuration files shared between multiple applications should be written.
           This is a generic value and the returned path is never empty.
    \value AppDataLocation Returns a directory location where persistent
           application data can be stored. This is an application-specific directory.
           To obtain a path to store data to be shared with other applications, use
           QStandardPaths::GenericDataLocation. The returned path is never empty.
           On the Windows operating system, this returns the roaming path.
           This enum value was added in Qt 5.4.
    \value AppLocalDataLocation Returns the local settings path on the Windows operating
           system. On all other platforms, it returns the same value as AppDataLocation.
           This enum value was added in Qt 5.4.
    \value AppConfigLocation Returns a directory location where user-specific
           configuration files should be written. This is an application-specific directory,
           and the returned path is never empty.
           This enum value was added in Qt 5.5.

    The following table gives examples of paths on different operating systems.
    The first path is the writable path (unless noted). Other, additional
    paths, if any, represent non-writable locations.

    \table
    \header \li Path type \li \macos  \li Windows
    \row \li DesktopLocation
         \li "~/Desktop"
         \li "C:/Users/<USER>/Desktop"
    \row \li DocumentsLocation
         \li "~/Documents"
         \li "C:/Users/<USER>/Documents"
    \row \li FontsLocation
         \li "/System/Library/Fonts" (not writable)
         \li "C:/Windows/Fonts" (not writable)
    \row \li ApplicationsLocation
         \li "/Applications" (not writable)
         \li "C:/Users/<USER>/AppData/Roaming/Microsoft/Windows/Start Menu/Programs"
    \row \li MusicLocation
         \li "~/Music"
         \li "C:/Users/<USER>/Music"
    \row \li MoviesLocation
         \li "~/Movies"
         \li "C:/Users/<USER>/Videos"
    \row \li PicturesLocation
         \li "~/Pictures"
         \li "C:/Users/<USER>/Pictures"
    \row \li TempLocation
         \li randomly generated by the OS
         \li "C:/Users/<USER>/AppData/Local/Temp"
    \row \li HomeLocation
         \li "~"
         \li "C:/Users/<USER>"
    \row \li DataLocation
         \li "~/Library/Application Support/<APPNAME>", "/Library/Application Support/<APPNAME>". "<APPDIR>/../Resources"
         \li "C:/Users/<USER>/AppData/Local/<APPNAME>", "C:/ProgramData/<APPNAME>", "<APPDIR>", "<APPDIR>/data", "<APPDIR>/data/<APPNAME>"
    \row \li CacheLocation
         \li "~/Library/Caches/<APPNAME>", "/Library/Caches/<APPNAME>"
         \li "C:/Users/<USER>/AppData/Local/<APPNAME>/cache"
    \row \li GenericDataLocation
         \li "~/Library/Application Support", "/Library/Application Support"
         \li "C:/Users/<USER>/AppData/Local", "C:/ProgramData", "<APPDIR>", "<APPDIR>/data"
    \row \li RuntimeLocation
         \li "~/Library/Application Support"
         \li "C:/Users/<USER>"
    \row \li ConfigLocation
         \li "~/Library/Preferences"
         \li "C:/Users/<USER>/AppData/Local/<APPNAME>", "C:/ProgramData/<APPNAME>"
    \row \li GenericConfigLocation
         \li "~/Library/Preferences"
         \li "C:/Users/<USER>/AppData/Local", "C:/ProgramData"
    \row \li DownloadLocation
         \li "~/Downloads"
         \li "C:/Users/<USER>/Documents"
    \row \li GenericCacheLocation
         \li "~/Library/Caches", "/Library/Caches"
         \li "C:/Users/<USER>/AppData/Local/cache"
    \row \li AppDataLocation
         \li "~/Library/Application Support/<APPNAME>", "/Library/Application Support/<APPNAME>". "<APPDIR>/../Resources"
         \li "C:/Users/<USER>/AppData/Roaming/<APPNAME>", "C:/ProgramData/<APPNAME>", "<APPDIR>", "<APPDIR>/data", "<APPDIR>/data/<APPNAME>"
    \row \li AppLocalDataLocation
         \li "~/Library/Application Support/<APPNAME>", "/Library/Application Support/<APPNAME>". "<APPDIR>/../Resources"
         \li "C:/Users/<USER>/AppData/Local/<APPNAME>", "C:/ProgramData/<APPNAME>", "<APPDIR>", "<APPDIR>/data", "<APPDIR>/data/<APPNAME>"
    \row \li AppConfigLocation
         \li "~/Library/Preferences/<APPNAME>"
         \li "C:/Users/<USER>/AppData/Local/<APPNAME>", "C:/ProgramData/<APPNAME>"
    \endtable

    \table
    \header \li Path type \li Linux
    \row \li DesktopLocation
         \li "~/Desktop"
    \row \li DocumentsLocation
         \li "~/Documents"
    \row \li FontsLocation
         \li "~/.fonts"
    \row \li ApplicationsLocation
         \li "~/.local/share/applications", "/usr/local/share/applications", "/usr/share/applications"
    \row \li MusicLocation
         \li "~/Music"
    \row \li MoviesLocation
         \li "~/Videos"
    \row \li PicturesLocation
         \li "~/Pictures"
    \row \li TempLocation
         \li "/tmp"
    \row \li HomeLocation
         \li "~"
    \row \li DataLocation
         \li "~/.local/share/<APPNAME>", "/usr/local/share/<APPNAME>", "/usr/share/<APPNAME>"
    \row \li CacheLocation
         \li "~/.cache/<APPNAME>"
    \row \li GenericDataLocation
         \li "~/.local/share", "/usr/local/share", "/usr/share"
    \row \li RuntimeLocation
         \li "/run/user/<USER>"
    \row \li ConfigLocation
         \li "~/.config", "/etc/xdg"
    \row \li GenericConfigLocation
         \li "~/.config", "/etc/xdg"
    \row \li DownloadLocation
         \li "~/Downloads"
    \row \li GenericCacheLocation
         \li "~/.cache"
    \row \li AppDataLocation
         \li "~/.local/share/<APPNAME>", "/usr/local/share/<APPNAME>", "/usr/share/<APPNAME>"
    \row \li AppLocalDataLocation
         \li "~/.local/share/<APPNAME>", "/usr/local/share/<APPNAME>", "/usr/share/<APPNAME>"
    \row \li AppConfigLocation
         \li "~/.config/<APPNAME>", "/etc/xdg/<APPNAME>"
    \endtable

    \table
    \header \li Path type \li Android \li iOS
    \row \li DesktopLocation
         \li "<APPROOT>/files"
         \li "<APPROOT>/Documents/Desktop"
    \row \li DocumentsLocation
         \li "<USER>/Documents", "<USER>/<APPNAME>/Documents"
         \li "<APPROOT>/Documents"
    \row \li FontsLocation
         \li "/system/fonts" (not writable)
         \li "<APPROOT>/Library/Fonts"
    \row \li ApplicationsLocation
         \li not supported (directory not readable)
         \li not supported
    \row \li MusicLocation
         \li "<USER>/Music", "<USER>/<APPNAME>/Music"
         \li "<APPROOT>/Documents/Music"
    \row \li MoviesLocation
         \li "<USER>/Movies", "<USER>/<APPNAME>/Movies"
         \li "<APPROOT>/Documents/Movies"
    \row \li PicturesLocation
         \li "<USER>/Pictures", "<USER>/<APPNAME>/Pictures"
         \li "<APPROOT>/Documents/Pictures", "assets-library://"
    \row \li TempLocation
         \li "<APPROOT>/cache"
         \li "<APPROOT>/tmp"
    \row \li HomeLocation
         \li "<APPROOT>/files"
         \li "<APPROOT>" (not writable)
    \row \li DataLocation
         \li "<APPROOT>/files", "<USER>/<APPNAME>/files"
         \li "<APPROOT>/Library/Application Support"
    \row \li CacheLocation
         \li "<APPROOT>/cache", "<USER>/<APPNAME>/cache"
         \li "<APPROOT>/Library/Caches"
    \row \li GenericDataLocation
         \li "<USER>"
         \li "<APPROOT>/Documents"
    \row \li RuntimeLocation
         \li "<APPROOT>/cache"
         \li not supported
    \row \li ConfigLocation
         \li "<APPROOT>/files/settings"
         \li "<APPROOT>/Library/Preferences"
    \row \li GenericConfigLocation
         \li "<APPROOT>/files/settings" (there is no shared settings)
         \li "<APPROOT>/Library/Preferences"
    \row \li DownloadLocation
         \li "<USER>/Downloads", "<USER>/<APPNAME>/Downloads"
         \li "<APPROOT>/Documents/Downloads"
    \row \li GenericCacheLocation
         \li "<APPROOT>/cache" (there is no shared cache)
         \li "<APPROOT>/Library/Caches"
    \row \li AppDataLocation
         \li "<APPROOT>/files", "<USER>/<APPNAME>/files"
         \li "<APPROOT>/Library/Application Support"
    \row \li AppConfigLocation
         \li "<APPROOT>/files/settings"
         \li "<APPROOT>/Library/Preferences/<APPNAME>"
    \row \li AppLocalDataLocation
         \li "<APPROOT>/files", "<USER>/<APPNAME>/files"
         \li "<APPROOT>/Library/Application Support"
    \endtable

    In the table above, \c <APPNAME> is usually the organization name, the
    application name, or both, or a unique name generated at packaging.
    Similarly, <APPROOT> is the location where this application is installed
    (often a sandbox). <APPDIR> is the directory containing the application
    executable.

    The paths above should not be relied upon, as they may change according to
    OS configuration, locale, or they may change in future Qt versions.

    \note On Android, applications with open files on the external storage (<USER> locations),
          will be killed if the external storage is unmounted.

    \note On iOS, if you do pass \c {QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).last()}
        as argument to \l{QFileDialog::setDirectory()},
        a native image picker dialog will be used for accessing the user's photo album.
        The filename returned can be loaded using QFile and related APIs.
        This feature was added in Qt 5.5.

    \sa writableLocation(), standardLocations(), displayName(), locate(), locateAll()
*/

/*!
    \fn QString QStandardPaths::writableLocation(StandardLocation type)

    Returns the directory where files of \a type should be written to, or an empty string
    if the location cannot be determined.

    \note The storage location returned can be a directory that does not exist; i.e., it
    may need to be created by the system or the user.
*/


/*!
   \fn QStringList QStandardPaths::standardLocations(StandardLocation type)

   Returns all the directories where files of \a type belong.

   The list of directories is sorted from high to low priority, starting with
   writableLocation() if it can be determined. This list is empty if no locations
   for \a type are defined.

   \sa writableLocation()
 */

/*!
    \enum QStandardPaths::LocateOption

    This enum describes the different flags that can be used for
    controlling the behavior of QStandardPaths::locate and
    QStandardPaths::locateAll.

    \value LocateFile return only files
    \value LocateDirectory return only directories
*/

static bool existsAsSpecified(const QString &path, QStandardPaths::LocateOptions options)
{
    if (options & QStandardPaths::LocateDirectory)
        return QDir(path).exists();
    return QFileInfo(path).isFile();
}

/*!
   Tries to find a file or directory called \a fileName in the standard locations
   for \a type.

   The full path to the first file or directory (depending on \a options) found is returned.
   If no such file or directory can be found, an empty string is returned.
 */
QString QStandardPaths::locate(StandardLocation type, const QString &fileName, LocateOptions options)
{
    const QStringList &dirs = standardLocations(type);
    for (QStringList::const_iterator dir = dirs.constBegin(); dir != dirs.constEnd(); ++dir) {
        const QString path = *dir + QLatin1Char('/') + fileName;
        if (existsAsSpecified(path, options))
            return path;
    }
    return QString();
}

/*!
   Tries to find all files or directories called \a fileName in the standard locations
   for \a type.

   The \a options flag allows to specify whether to look for files or directories.

   Returns the list of all the files that were found.
 */
QStringList QStandardPaths::locateAll(StandardLocation type, const QString &fileName, LocateOptions options)
{
    const QStringList &dirs = standardLocations(type);
    QStringList result;
    for (QStringList::const_iterator dir = dirs.constBegin(); dir != dirs.constEnd(); ++dir) {
        const QString path = *dir + QLatin1Char('/') + fileName;
        if (existsAsSpecified(path, options))
            result.append(path);
    }
    return result;
}

#ifdef Q_OS_WIN
static QStringList executableExtensions()
{
    // If %PATHEXT% does not contain .exe, it is either empty, malformed, or distorted in ways that we cannot support, anyway.
    const QStringList pathExt = QString::fromLocal8Bit(qgetenv("PATHEXT")).toLower().split(QLatin1Char(';'));
    return pathExt.contains(QLatin1String(".exe"), Qt::CaseInsensitive) ?
           pathExt :
           QStringList() << QLatin1String(".exe") << QLatin1String(".com")
                         << QLatin1String(".bat") << QLatin1String(".cmd");
}
#endif

static QString checkExecutable(const QString &path)
{
    const QFileInfo info(path);
    if (info.isBundle())
        return info.bundleName();
    if (info.isFile() && info.isExecutable())
        return QDir::cleanPath(path);
    return QString();
}

static inline QString searchExecutable(const QStringList &searchPaths,
                                       const QString &executableName)
{
    const QDir currentDir = QDir::current();
    for (const QString &searchPath : searchPaths) {
        const QString candidate = currentDir.absoluteFilePath(searchPath + QLatin1Char('/') + executableName);
        const QString absPath = checkExecutable(candidate);
        if (!absPath.isEmpty())
            return absPath;
    }
    return QString();
}

#ifdef Q_OS_WIN

// Find executable appending candidate suffixes, used for suffix-less executables
// on Windows.
static inline QString
    searchExecutableAppendSuffix(const QStringList &searchPaths,
                                 const QString &executableName,
                                 const QStringList &suffixes)
{
    const QDir currentDir = QDir::current();
    for (const QString &searchPath : searchPaths) {
        const QString candidateRoot = currentDir.absoluteFilePath(searchPath + QLatin1Char('/') + executableName);
        for (const QString &suffix : suffixes) {
            const QString absPath = checkExecutable(candidateRoot + suffix);
            if (!absPath.isEmpty())
                return absPath;
        }
    }
    return QString();
}

#endif // Q_OS_WIN

/*!
  Finds the executable named \a executableName in the paths specified by \a paths,
  or the system paths if \a paths is empty.

  On most operating systems the system path is determined by the PATH environment variable.

  The directories where to search for the executable can be set in the \a paths argument.
  To search in both your own paths and the system paths, call findExecutable twice, once with
  \a paths set and once with \a paths empty.

  Symlinks are not resolved, in order to preserve behavior for the case of executables
  whose behavior depends on the name they are invoked with.

  \note On Windows, the usual executable extensions (from the PATHEXT environment variable)
  are automatically appended, so that for instance findExecutable("foo") will find foo.exe
  or foo.bat if present.

  Returns the absolute file path to the executable, or an empty string if not found.
 */
QString QStandardPaths::findExecutable(const QString &executableName, const QStringList &paths)
{
    if (QFileInfo(executableName).isAbsolute())
        return checkExecutable(executableName);

    QStringList searchPaths = paths;
    if (paths.isEmpty()) {
        QByteArray pEnv = qgetenv("PATH");
        if (Q_UNLIKELY(pEnv.isNull())) {
            // Get a default path. POSIX.1 does not actually require this, but
            // most Unix libc fall back to confstr(_CS_PATH) if the PATH
            // environment variable isn't set. Let's try to do the same.
#if defined(_PATH_DEFPATH)
            // BSD API.
            pEnv = _PATH_DEFPATH;
#elif defined(_CS_PATH)
            // POSIX API.
            size_t n = confstr(_CS_PATH, nullptr, 0);
            if (n) {
                pEnv.resize(n);
                // size()+1 is ok because QByteArray always has an extra NUL-terminator
                confstr(_CS_PATH, pEnv.data(), pEnv.size() + 1);
            }
#else
            // Windows SDK's execvpe() does not have a fallback, so we won't
            // apply one either.
#endif
        }

        // Remove trailing slashes, which occur on Windows.
        const QStringList rawPaths = QString::fromLocal8Bit(pEnv.constData()).split(QDir::listSeparator(), QString::SkipEmptyParts);
        searchPaths.reserve(rawPaths.size());
        for (const QString &rawPath : rawPaths) {
            QString cleanPath = QDir::cleanPath(rawPath);
            if (cleanPath.size() > 1 && cleanPath.endsWith(QLatin1Char('/')))
                cleanPath.truncate(cleanPath.size() - 1);
            searchPaths.push_back(cleanPath);
        }
    }

#ifdef Q_OS_WIN
    // On Windows, if the name does not have a suffix or a suffix not
    // in PATHEXT ("xx.foo"), append suffixes from PATHEXT.
    static const QStringList executable_extensions = executableExtensions();
    if (executableName.contains(QLatin1Char('.'))) {
        const QString suffix = QFileInfo(executableName).suffix();
        if (suffix.isEmpty() || !executable_extensions.contains(QLatin1Char('.') + suffix, Qt::CaseInsensitive))
            return searchExecutableAppendSuffix(searchPaths, executableName, executable_extensions);
    } else {
        return searchExecutableAppendSuffix(searchPaths, executableName, executable_extensions);
    }
#endif
    return searchExecutable(searchPaths, executableName);
}

/*!
    \fn QString QStandardPaths::displayName(StandardLocation type)

    Returns a localized display name for the given location \a type or
    an empty QString if no relevant location can be found.
*/

#if !defined(Q_OS_MAC) && !defined(QT_BOOTSTRAPPED)
QString QStandardPaths::displayName(StandardLocation type)
{
    switch (type) {
    case DesktopLocation:
        return QCoreApplication::translate("QStandardPaths", "Desktop");
    case DocumentsLocation:
        return QCoreApplication::translate("QStandardPaths", "Documents");
    case FontsLocation:
        return QCoreApplication::translate("QStandardPaths", "Fonts");
    case ApplicationsLocation:
        return QCoreApplication::translate("QStandardPaths", "Applications");
    case MusicLocation:
        return QCoreApplication::translate("QStandardPaths", "Music");
    case MoviesLocation:
        return QCoreApplication::translate("QStandardPaths", "Movies");
    case PicturesLocation:
        return QCoreApplication::translate("QStandardPaths", "Pictures");
    case TempLocation:
        return QCoreApplication::translate("QStandardPaths", "Temporary Directory");
    case HomeLocation:
        return QCoreApplication::translate("QStandardPaths", "Home");
    case CacheLocation:
        return QCoreApplication::translate("QStandardPaths", "Cache");
    case GenericDataLocation:
        return QCoreApplication::translate("QStandardPaths", "Shared Data");
    case RuntimeLocation:
        return QCoreApplication::translate("QStandardPaths", "Runtime");
    case ConfigLocation:
        return QCoreApplication::translate("QStandardPaths", "Configuration");
    case GenericConfigLocation:
        return QCoreApplication::translate("QStandardPaths", "Shared Configuration");
    case GenericCacheLocation:
        return QCoreApplication::translate("QStandardPaths", "Shared Cache");
    case DownloadLocation:
        return QCoreApplication::translate("QStandardPaths", "Download");
    case AppDataLocation:
    case AppLocalDataLocation:
        return QCoreApplication::translate("QStandardPaths", "Application Data");
    case AppConfigLocation:
        return QCoreApplication::translate("QStandardPaths", "Application Configuration");
    }
    // not reached
    return QString();
}
#endif

/*!
  \fn void QStandardPaths::enableTestMode(bool testMode)
  \obsolete Use QStandardPaths::setTestModeEnabled
 */
/*!
  \fn void QStandardPaths::setTestModeEnabled(bool testMode)

  If \a testMode is true, this enables a special "test mode" in
  QStandardPaths, which changes writable locations
  to point to test directories, in order to prevent auto tests from reading from
  or writing to the current user's configuration.

  This affects the locations into which test programs might write files:
  GenericDataLocation, DataLocation, ConfigLocation, GenericConfigLocation,
  AppConfigLocation, GenericCacheLocation, CacheLocation.
  Other locations are not affected.

  On Unix, \c XDG_DATA_HOME is set to \e ~/.qttest/share, \c XDG_CONFIG_HOME is
  set to \e ~/.qttest/config, and \c XDG_CACHE_HOME is set to \e ~/.qttest/cache.

  On \macos, data goes to \e ~/.qttest/Application Support, cache goes to
  \e ~/.qttest/Cache, and config goes to \e ~/.qttest/Preferences.

  On Windows, everything goes to a "qttest" directory under Application Data.
*/

static bool qsp_testMode = false;

#if QT_DEPRECATED_SINCE(5, 2)
void QStandardPaths::enableTestMode(bool testMode)
{
    qsp_testMode = testMode;
}
#endif

void QStandardPaths::setTestModeEnabled(bool testMode)
{
    qsp_testMode = testMode;
}

/*!
  \fn void QStandardPaths::isTestModeEnabled()

  \internal

  Returns \c true if test mode is enabled in QStandardPaths; otherwise returns \c false.
*/

bool QStandardPaths::isTestModeEnabled()
{
    return qsp_testMode;
}


QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qstandardpaths.cpp"
#endif

#endif // QT_NO_STANDARDPATHS
