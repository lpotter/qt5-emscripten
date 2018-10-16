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

#include "qfilesystemwatcher_polling_p.h"
#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

QPollingFileSystemWatcherEngine::QPollingFileSystemWatcherEngine(QObject *parent)
    : QFileSystemWatcherEngine(parent),
      timer(this)
{
    connect(&timer, SIGNAL(timeout()), SLOT(timeout()));
}

QStringList QPollingFileSystemWatcherEngine::addPaths(const QStringList &paths,
                                                      QStringList *files,
                                                      QStringList *directories)
{
    QStringList p = paths;
    QMutableListIterator<QString> it(p);
    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo fi(path);
        if (!fi.exists())
            continue;
        if (fi.isDir()) {
            if (directories->contains(path))
                continue;
            directories->append(path);
            if (!path.endsWith(QLatin1Char('/')))
                fi = QFileInfo(path + QLatin1Char('/'));
            this->directories.insert(path, fi);
        } else {
            if (files->contains(path))
                continue;
            files->append(path);
            this->files.insert(path, fi);
        }
        it.remove();
    }

    if ((!this->files.isEmpty() ||
         !this->directories.isEmpty()) &&
        !timer.isActive()) {
        timer.start(PollingInterval);
    }

    return p;
}

QStringList QPollingFileSystemWatcherEngine::removePaths(const QStringList &paths,
                                                         QStringList *files,
                                                         QStringList *directories)
{
    QStringList p = paths;
    QMutableListIterator<QString> it(p);
    while (it.hasNext()) {
        QString path = it.next();
        if (this->directories.remove(path)) {
            directories->removeAll(path);
            it.remove();
        } else if (this->files.remove(path)) {
            files->removeAll(path);
            it.remove();
        }
    }

    if (this->files.isEmpty() &&
        this->directories.isEmpty()) {
        timer.stop();
    }

    return p;
}

void QPollingFileSystemWatcherEngine::timeout()
{
    QMutableHashIterator<QString, FileInfo> fit(files);
    while (fit.hasNext()) {
        QHash<QString, FileInfo>::iterator x = fit.next();
        QString path = x.key();
        QFileInfo fi(path);
        if (!fi.exists()) {
            fit.remove();
            emit fileChanged(path, true);
        } else if (x.value() != fi) {
            x.value() = fi;
            emit fileChanged(path, false);
        }
    }
    QMutableHashIterator<QString, FileInfo> dit(directories);
    while (dit.hasNext()) {
        QHash<QString, FileInfo>::iterator x = dit.next();
        QString path = x.key();
        QFileInfo fi(path);
        if (!path.endsWith(QLatin1Char('/')))
            fi = QFileInfo(path + QLatin1Char('/'));
        if (!fi.exists()) {
            dit.remove();
            emit directoryChanged(path, true);
        } else if (x.value() != fi) {
            fi.refresh();
            if (!fi.exists()) {
                dit.remove();
                emit directoryChanged(path, true);
            } else {
                x.value() = fi;
                emit directoryChanged(path, false);
            }
        }
    }
}

QT_END_NAMESPACE

#include "moc_qfilesystemwatcher_polling_p.cpp"
