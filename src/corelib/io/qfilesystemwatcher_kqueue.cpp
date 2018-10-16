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

#include <qplatformdefs.h>

#include "qfilesystemwatcher.h"
#include "qfilesystemwatcher_kqueue_p.h"
#include "private/qcore_unix_p.h"

#include <qdebug.h>
#include <qfile.h>
#include <qsocketnotifier.h>
#include <qvarlengtharray.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

QT_BEGIN_NAMESPACE

// #define KEVENT_DEBUG
#ifdef KEVENT_DEBUG
#  define DEBUG qDebug
#else
#  define DEBUG if(false)qDebug
#endif

QKqueueFileSystemWatcherEngine *QKqueueFileSystemWatcherEngine::create(QObject *parent)
{
    int kqfd = kqueue();
    if (kqfd == -1)
        return 0;
    return new QKqueueFileSystemWatcherEngine(kqfd, parent);
}

QKqueueFileSystemWatcherEngine::QKqueueFileSystemWatcherEngine(int kqfd, QObject *parent)
    : QFileSystemWatcherEngine(parent),
      kqfd(kqfd),
      notifier(kqfd, QSocketNotifier::Read, this)
{
    connect(&notifier, SIGNAL(activated(int)), SLOT(readFromKqueue()));

    fcntl(kqfd, F_SETFD, FD_CLOEXEC);
}

QKqueueFileSystemWatcherEngine::~QKqueueFileSystemWatcherEngine()
{
    notifier.setEnabled(false);
    close(kqfd);

    for (int id : qAsConst(pathToID))
        ::close(id < 0 ? -id : id);
}

QStringList QKqueueFileSystemWatcherEngine::addPaths(const QStringList &paths,
                                                     QStringList *files,
                                                     QStringList *directories)
{
    QStringList p = paths;
    QMutableListIterator<QString> it(p);
    while (it.hasNext()) {
        QString path = it.next();
        int fd;
#if defined(O_EVTONLY)
        fd = qt_safe_open(QFile::encodeName(path), O_EVTONLY);
#else
        fd = qt_safe_open(QFile::encodeName(path), O_RDONLY);
#endif
        if (fd == -1) {
            perror("QKqueueFileSystemWatcherEngine::addPaths: open");
            continue;
        }
        if (fd >= (int)FD_SETSIZE / 2 && fd < (int)FD_SETSIZE) {
            int fddup = qt_safe_dup(fd, FD_SETSIZE);
            if (fddup != -1) {
                ::close(fd);
                fd = fddup;
            }
        }

        QT_STATBUF st;
        if (QT_FSTAT(fd, &st) == -1) {
            perror("QKqueueFileSystemWatcherEngine::addPaths: fstat");
            ::close(fd);
            continue;
        }
        int id = (S_ISDIR(st.st_mode)) ? -fd : fd;
        if (id < 0) {
            if (directories->contains(path)) {
                ::close(fd);
                continue;
            }
        } else {
            if (files->contains(path)) {
                ::close(fd);
                continue;
            }
        }

        struct kevent kev;
        EV_SET(&kev,
               fd,
               EVFILT_VNODE,
               EV_ADD | EV_ENABLE | EV_CLEAR,
               NOTE_DELETE | NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_RENAME | NOTE_REVOKE,
               0,
               0);
        if (kevent(kqfd, &kev, 1, 0, 0, 0) == -1) {
            perror("QKqueueFileSystemWatcherEngine::addPaths: kevent");
            ::close(fd);
            continue;
        }

        it.remove();
        if (id < 0) {
            DEBUG() << "QKqueueFileSystemWatcherEngine: added directory path" << path;
            directories->append(path);
        } else {
            DEBUG() << "QKqueueFileSystemWatcherEngine: added file path" << path;
            files->append(path);
        }

        pathToID.insert(path, id);
        idToPath.insert(id, path);
    }

    return p;
}

QStringList QKqueueFileSystemWatcherEngine::removePaths(const QStringList &paths,
                                                        QStringList *files,
                                                        QStringList *directories)
{
    QStringList p = paths;
    if (pathToID.isEmpty())
        return p;

    QMutableListIterator<QString> it(p);
    while (it.hasNext()) {
        QString path = it.next();
        int id = pathToID.take(path);
        QString x = idToPath.take(id);
        if (x.isEmpty() || x != path)
            continue;

        ::close(id < 0 ? -id : id);

        it.remove();
        if (id < 0)
            directories->removeAll(path);
        else
            files->removeAll(path);
    }

    return p;
}

void QKqueueFileSystemWatcherEngine::readFromKqueue()
{
    forever {
        DEBUG() << "QKqueueFileSystemWatcherEngine: polling for changes";
        int r;
        struct kevent kev;
        struct timespec ts = { 0, 0 }; // 0 ts, because we want to poll
        EINTR_LOOP(r, kevent(kqfd, 0, 0, &kev, 1, &ts));
        if (r < 0) {
            perror("QKqueueFileSystemWatcherEngine: error during kevent wait");
            return;
        } else if (r == 0) {
            // polling returned no events, so stop
            break;
        } else {
            int fd = kev.ident;

            DEBUG() << "QKqueueFileSystemWatcherEngine: processing kevent" << kev.ident << kev.filter;

            int id = fd;
            QString path = idToPath.value(id);
            if (path.isEmpty()) {
                // perhaps a directory?
                id = -id;
                path = idToPath.value(id);
                if (path.isEmpty()) {
                    DEBUG() << "QKqueueFileSystemWatcherEngine: received a kevent for a file we're not watching";
                    continue;
                }
            }
            if (kev.filter != EVFILT_VNODE) {
                DEBUG() << "QKqueueFileSystemWatcherEngine: received a kevent with the wrong filter";
                continue;
            }

            if ((kev.fflags & (NOTE_DELETE | NOTE_REVOKE | NOTE_RENAME)) != 0) {
                DEBUG() << path << "removed, removing watch also";

                pathToID.remove(path);
                idToPath.remove(id);
                ::close(fd);

                if (id < 0)
                    emit directoryChanged(path, true);
                else
                    emit fileChanged(path, true);
            } else {
                DEBUG() << path << "changed";

                if (id < 0)
                    emit directoryChanged(path, false);
                else
                    emit fileChanged(path, false);
            }
        }

    }
}

QT_END_NAMESPACE
