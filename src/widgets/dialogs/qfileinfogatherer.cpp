/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qfileinfogatherer_p.h"
#include <qdebug.h>
#include <qdiriterator.h>
#ifndef Q_OS_WIN
#  include <unistd.h>
#  include <sys/types.h>
#endif
#if defined(Q_OS_VXWORKS)
#  include "qplatformdefs.h"
#endif

QT_BEGIN_NAMESPACE

#ifdef QT_BUILD_INTERNAL
static QBasicAtomicInt fetchedRoot = Q_BASIC_ATOMIC_INITIALIZER(false);
Q_AUTOTEST_EXPORT void qt_test_resetFetchedRoot()
{
    fetchedRoot.store(false);
}

Q_AUTOTEST_EXPORT bool qt_test_isFetchedRoot()
{
    return fetchedRoot.load();
}
#endif

static QString translateDriveName(const QFileInfo &drive)
{
    QString driveName = drive.absoluteFilePath();
#ifdef Q_OS_WIN
    if (driveName.startsWith(QLatin1Char('/'))) // UNC host
        return drive.fileName();
    if (driveName.endsWith(QLatin1Char('/')))
        driveName.chop(1);
#endif // Q_OS_WIN
    return driveName;
}

/*!
    Creates thread
*/
QFileInfoGatherer::QFileInfoGatherer(QObject *parent)
    : QThread(parent), abort(false),
#if QT_CONFIG(filesystemwatcher)
      watcher(0),
#endif
#ifdef Q_OS_WIN
      m_resolveSymlinks(true),
#endif
      m_iconProvider(&defaultProvider)
{
#if QT_CONFIG(filesystemwatcher)
    watcher = new QFileSystemWatcher(this);
    connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(list(QString)));
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(updateFile(QString)));

#  if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    const QVariant listener = watcher->property("_q_driveListener");
    if (listener.canConvert<QObject *>()) {
        if (QObject *driveListener = listener.value<QObject *>()) {
            connect(driveListener, SIGNAL(driveAdded()), this, SLOT(driveAdded()));
            connect(driveListener, SIGNAL(driveRemoved()), this, SLOT(driveRemoved()));
        }
    }
#  endif // Q_OS_WIN && !Q_OS_WINRT
#endif
    start(LowPriority);
}

/*!
    Destroys thread
*/
QFileInfoGatherer::~QFileInfoGatherer()
{
    abort.store(true);
    QMutexLocker locker(&mutex);
    condition.wakeAll();
    locker.unlock();
    wait();
}

void QFileInfoGatherer::setResolveSymlinks(bool enable)
{
    Q_UNUSED(enable);
#ifdef Q_OS_WIN
    m_resolveSymlinks = enable;
#endif
}

void QFileInfoGatherer::driveAdded()
{
    fetchExtendedInformation(QString(), QStringList());
}

void QFileInfoGatherer::driveRemoved()
{
    QStringList drives;
    const QFileInfoList driveInfoList = QDir::drives();
    for (const QFileInfo &fi : driveInfoList)
        drives.append(translateDriveName(fi));
    newListOfFiles(QString(), drives);
}

bool QFileInfoGatherer::resolveSymlinks() const
{
#ifdef Q_OS_WIN
    return m_resolveSymlinks;
#else
    return false;
#endif
}

void QFileInfoGatherer::setIconProvider(QFileIconProvider *provider)
{
    m_iconProvider = provider;
}

QFileIconProvider *QFileInfoGatherer::iconProvider() const
{
    return m_iconProvider;
}

/*!
    Fetch extended information for all \a files in \a path

    \sa updateFile(), update(), resolvedName()
*/
void QFileInfoGatherer::fetchExtendedInformation(const QString &path, const QStringList &files)
{
    QMutexLocker locker(&mutex);
    // See if we already have this dir/file in our queue
    int loc = this->path.lastIndexOf(path);
    while (loc > 0)  {
        if (this->files.at(loc) == files) {
            return;
        }
        loc = this->path.lastIndexOf(path, loc - 1);
    }
    this->path.push(path);
    this->files.push(files);
    condition.wakeAll();

#if QT_CONFIG(filesystemwatcher)
    if (files.isEmpty()
        && !path.isEmpty()
        && !path.startsWith(QLatin1String("//")) /*don't watch UNC path*/) {
        if (!watcher->directories().contains(path))
            watcher->addPath(path);
    }
#endif
}

/*!
    Fetch extended information for all \a filePath

    \sa fetchExtendedInformation()
*/
void QFileInfoGatherer::updateFile(const QString &filePath)
{
    QString dir = filePath.mid(0, filePath.lastIndexOf(QLatin1Char('/')));
    QString fileName = filePath.mid(dir.length() + 1);
    fetchExtendedInformation(dir, QStringList(fileName));
}

/*
    List all files in \a directoryPath

    \sa listed()
*/
void QFileInfoGatherer::clear()
{
#if QT_CONFIG(filesystemwatcher)
    QMutexLocker locker(&mutex);
    watcher->removePaths(watcher->files());
    watcher->removePaths(watcher->directories());
#endif
}

/*
    Remove a \a path from the watcher

    \sa listed()
*/
void QFileInfoGatherer::removePath(const QString &path)
{
#if QT_CONFIG(filesystemwatcher)
    QMutexLocker locker(&mutex);
    watcher->removePath(path);
#else
    Q_UNUSED(path);
#endif
}

/*
    List all files in \a directoryPath

    \sa listed()
*/
void QFileInfoGatherer::list(const QString &directoryPath)
{
    fetchExtendedInformation(directoryPath, QStringList());
}

/*
    Until aborted wait to fetch a directory or files
*/
void QFileInfoGatherer::run()
{
    forever {
        QMutexLocker locker(&mutex);
        while (!abort.load() && path.isEmpty())
            condition.wait(&mutex);
        if (abort.load())
            return;
        const QString thisPath = qAsConst(path).front();
        path.pop_front();
        const QStringList thisList = qAsConst(files).front();
        files.pop_front();
        locker.unlock();

        getFileInfos(thisPath, thisList);
    }
}

QExtendedInformation QFileInfoGatherer::getInfo(const QFileInfo &fileInfo) const
{
    QExtendedInformation info(fileInfo);
    info.icon = m_iconProvider->icon(fileInfo);
    info.displayType = m_iconProvider->type(fileInfo);
#if QT_CONFIG(filesystemwatcher)
    // ### Not ready to listen all modifications by default
    static const bool watchFiles = qEnvironmentVariableIsSet("QT_FILESYSTEMMODEL_WATCH_FILES");
    if (watchFiles) {
        if (!fileInfo.exists() && !fileInfo.isSymLink()) {
            watcher->removePath(fileInfo.absoluteFilePath());
        } else {
            const QString path = fileInfo.absoluteFilePath();
            if (!path.isEmpty() && fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable()
                && !watcher->files().contains(path)) {
                watcher->addPath(path);
            }
        }
    }
#endif // filesystemwatcher

#ifdef Q_OS_WIN
    if (m_resolveSymlinks && info.isSymLink(/* ignoreNtfsSymLinks = */ true)) {
        QFileInfo resolvedInfo(fileInfo.symLinkTarget());
        resolvedInfo = resolvedInfo.canonicalFilePath();
        if (resolvedInfo.exists()) {
            emit nameResolved(fileInfo.filePath(), resolvedInfo.fileName());
        }
    }
#endif
    return info;
}

/*
    Get specific file info's, batch the files so update when we have 100
    items and every 200ms after that
 */
void QFileInfoGatherer::getFileInfos(const QString &path, const QStringList &files)
{
    // List drives
    if (path.isEmpty()) {
#ifdef QT_BUILD_INTERNAL
        fetchedRoot.store(true);
#endif
        QFileInfoList infoList;
        if (files.isEmpty()) {
            infoList = QDir::drives();
        } else {
            infoList.reserve(files.count());
            for (const auto &file : files)
                infoList << QFileInfo(file);
        }
        for (int i = infoList.count() - 1; i >= 0; --i) {
            QString driveName = translateDriveName(infoList.at(i));
            QVector<QPair<QString,QFileInfo> > updatedFiles;
            updatedFiles.append(QPair<QString,QFileInfo>(driveName, infoList.at(i)));
            emit updates(path, updatedFiles);
        }
        return;
    }

    QElapsedTimer base;
    base.start();
    QFileInfo fileInfo;
    bool firstTime = true;
    QVector<QPair<QString, QFileInfo> > updatedFiles;
    QStringList filesToCheck = files;

    QStringList allFiles;
    if (files.isEmpty()) {
        QDirIterator dirIt(path, QDir::AllEntries | QDir::System | QDir::Hidden);
        while (!abort.load() && dirIt.hasNext()) {
            dirIt.next();
            fileInfo = dirIt.fileInfo();
            allFiles.append(fileInfo.fileName());
            fetch(fileInfo, base, firstTime, updatedFiles, path);
        }
    }
    if (!allFiles.isEmpty())
        emit newListOfFiles(path, allFiles);

    QStringList::const_iterator filesIt = filesToCheck.constBegin();
    while (!abort.load() && filesIt != filesToCheck.constEnd()) {
        fileInfo.setFile(path + QDir::separator() + *filesIt);
        ++filesIt;
        fetch(fileInfo, base, firstTime, updatedFiles, path);
    }
    if (!updatedFiles.isEmpty())
        emit updates(path, updatedFiles);
    emit directoryLoaded(path);
}

void QFileInfoGatherer::fetch(const QFileInfo &fileInfo, QElapsedTimer &base, bool &firstTime, QVector<QPair<QString, QFileInfo> > &updatedFiles, const QString &path) {
    updatedFiles.append(QPair<QString, QFileInfo>(fileInfo.fileName(), fileInfo));
    QElapsedTimer current;
    current.start();
    if ((firstTime && updatedFiles.count() > 100) || base.msecsTo(current) > 1000) {
        emit updates(path, updatedFiles);
        updatedFiles.clear();
        base = current;
        firstTime = false;
    }
}

QT_END_NAMESPACE

#include "moc_qfileinfogatherer_p.cpp"
