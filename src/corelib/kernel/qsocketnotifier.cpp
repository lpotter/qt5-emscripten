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

#include "qsocketnotifier.h"

#include "qplatformdefs.h"

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"

#include "qobject_p.h"
#include <private/qthread_p.h>

QT_BEGIN_NAMESPACE

class QSocketNotifierPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSocketNotifier)
public:
    qintptr sockfd;
    QSocketNotifier::Type sntype;
    bool snenabled;
};

/*!
    \class QSocketNotifier
    \inmodule QtCore
    \brief The QSocketNotifier class provides support for monitoring
    activity on a file descriptor.

    \ingroup network
    \ingroup io

    The QSocketNotifier makes it possible to integrate Qt's event
    loop with other event loops based on file descriptors. File
    descriptor action is detected in Qt's main event
    loop (QCoreApplication::exec()).

    \target write notifiers

    Once you have opened a device using a low-level (usually
    platform-specific) API, you can create a socket notifier to
    monitor the file descriptor. The socket notifier is enabled by
    default, i.e. it emits the activated() signal whenever a socket
    event corresponding to its type occurs. Connect the activated()
    signal to the slot you want to be called when an event
    corresponding to your socket notifier's type occurs.

    There are three types of socket notifiers: read, write, and
    exception. The type is described by the \l Type enum, and must be
    specified when constructing the socket notifier. After
    construction it can be determined using the type() function. Note
    that if you need to monitor both reads and writes for the same
    file descriptor, you must create two socket notifiers. Note also
    that it is not possible to install two socket notifiers of the
    same type (\l Read, \l Write, \l Exception) on the same socket.

    The setEnabled() function allows you to disable as well as enable
    the socket notifier. It is generally advisable to explicitly
    enable or disable the socket notifier, especially for write
    notifiers. A disabled notifier ignores socket events (the same
    effect as not creating the socket notifier). Use the isEnabled()
    function to determine the notifier's current status.

    Finally, you can use the socket() function to retrieve the
    socket identifier.  Although the class is called QSocketNotifier,
    it is normally used for other types of devices than sockets.
    QTcpSocket and QUdpSocket provide notification through signals, so
    there is normally no need to use a QSocketNotifier on them.

    \sa QFile, QProcess, QTcpSocket, QUdpSocket
*/

/*!
    \enum QSocketNotifier::Type

    This enum describes the various types of events that a socket
    notifier can recognize. The type must be specified when
    constructing the socket notifier.

    Note that if you need to monitor both reads and writes for the
    same file descriptor, you must create two socket notifiers. Note
    also that it is not possible to install two socket notifiers of
    the same type (Read, Write, Exception) on the same socket.

    \value Read      There is data to be read.
    \value Write      Data can be written.
    \value Exception  An exception has occurred. We recommend against using this.

    \sa QSocketNotifier(), type()
*/

/*!
    Constructs a socket notifier with the given \a parent. It enables
    the \a socket, and watches for events of the given \a type.

    It is generally advisable to explicitly enable or disable the
    socket notifier, especially for write notifiers.

    \b{Note for Windows users:} The socket passed to QSocketNotifier
    will become non-blocking, even if it was created as a blocking socket.

    \sa setEnabled(), isEnabled()
*/

QSocketNotifier::QSocketNotifier(qintptr socket, Type type, QObject *parent)
    : QObject(*new QSocketNotifierPrivate, parent)
{
    Q_D(QSocketNotifier);
    d->sockfd = socket;
    d->sntype = type;
    d->snenabled = true;

    if (socket < 0)
        qWarning("QSocketNotifier: Invalid socket specified");
    else if (!d->threadData->hasEventDispatcher())
        qWarning("QSocketNotifier: Can only be used with threads started with QThread");
    else
        d->threadData->eventDispatcher.load()->registerSocketNotifier(this);
}

/*!
    Destroys this socket notifier.
*/

QSocketNotifier::~QSocketNotifier()
{
    setEnabled(false);
}


/*!
    \fn void QSocketNotifier::activated(int socket)

    This signal is emitted whenever the socket notifier is enabled and
    a socket event corresponding to its \l {Type}{type} occurs.

    The socket identifier is passed in the \a socket parameter.

    \sa type(), socket()
*/


/*!
    Returns the socket identifier specified to the constructor.

    \sa type()
*/
qintptr QSocketNotifier::socket() const
{
    Q_D(const QSocketNotifier);
    return d->sockfd;
}

/*!
    Returns the socket event type specified to the constructor.

    \sa socket()
*/
QSocketNotifier::Type QSocketNotifier::type() const
{
    Q_D(const QSocketNotifier);
    return d->sntype;
}

/*!
    Returns \c true if the notifier is enabled; otherwise returns \c false.

    \sa setEnabled()
*/
bool QSocketNotifier::isEnabled() const
{
    Q_D(const QSocketNotifier);
    return d->snenabled;
}

/*!
    If \a enable is true, the notifier is enabled; otherwise the notifier
    is disabled.

    The notifier is enabled by default, i.e. it emits the activated()
    signal whenever a socket event corresponding to its
    \l{type()}{type} occurs. If it is disabled, it ignores socket
    events (the same effect as not creating the socket notifier).

    Write notifiers should normally be disabled immediately after the
    activated() signal has been emitted

    \sa isEnabled(), activated()
*/

void QSocketNotifier::setEnabled(bool enable)
{
    Q_D(QSocketNotifier);
    if (d->sockfd < 0)
        return;
    if (d->snenabled == enable)                        // no change
        return;
    d->snenabled = enable;

    if (!d->threadData->hasEventDispatcher()) // perhaps application/thread is shutting down
        return;
    if (Q_UNLIKELY(thread() != QThread::currentThread())) {
        qWarning("QSocketNotifier: Socket notifiers cannot be enabled or disabled from another thread");
        return;
    }
    if (d->snenabled)
        d->threadData->eventDispatcher.load()->registerSocketNotifier(this);
    else
        d->threadData->eventDispatcher.load()->unregisterSocketNotifier(this);
}


/*!\reimp
*/
bool QSocketNotifier::event(QEvent *e)
{
    Q_D(QSocketNotifier);
    // Emits the activated() signal when a QEvent::SockAct or QEvent::SockClose is
    // received.
    if (e->type() == QEvent::ThreadChange) {
        if (d->snenabled) {
            QMetaObject::invokeMethod(this, "setEnabled", Qt::QueuedConnection,
                                      Q_ARG(bool, d->snenabled));
            setEnabled(false);
        }
    }
    QObject::event(e);                        // will activate filters
    if ((e->type() == QEvent::SockAct) || (e->type() == QEvent::SockClose)) {
        emit activated(d->sockfd, QPrivateSignal());
        return true;
    }
    return false;
}

QT_END_NAMESPACE

#include "moc_qsocketnotifier.cpp"
