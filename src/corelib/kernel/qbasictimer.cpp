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

#include "qbasictimer.h"
#include "qabstracteventdispatcher.h"
#include "qabstracteventdispatcher_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QBasicTimer
    \inmodule QtCore
    \brief The QBasicTimer class provides timer events for objects.

    \ingroup events

    This is a fast, lightweight, and low-level class used by Qt
    internally. We recommend using the higher-level QTimer class
    rather than this class if you want to use timers in your
    applications. Note that this timer is a repeating timer that
    will send subsequent timer events unless the stop() function is called.

    To use this class, create a QBasicTimer, and call its start()
    function with a timeout interval and with a pointer to a QObject
    subclass. When the timer times out it will send a timer event to
    the QObject subclass. The timer can be stopped at any time using
    stop(). isActive() returns \c true for a timer that is running;
    i.e. it has been started, has not reached the timeout time, and
    has not been stopped. The timer's ID can be retrieved using
    timerId().

    The \l{widgets/wiggly}{Wiggly} example uses QBasicTimer to repaint
    a widget at regular intervals.

    \sa QTimer, QTimerEvent, QObject::timerEvent(), Timers, {Wiggly Example}
*/


/*!
    \fn QBasicTimer::QBasicTimer()

    Contructs a basic timer.

    \sa start()
*/
/*!
    \fn QBasicTimer::~QBasicTimer()

    Destroys the basic timer.
*/

/*!
    \fn bool QBasicTimer::isActive() const

    Returns \c true if the timer is running and has not been stopped; otherwise
    returns \c false.

    \sa start(), stop()
*/

/*!
    \fn int QBasicTimer::timerId() const

    Returns the timer's ID.

    \sa QTimerEvent::timerId()
*/

/*!
    \fn void QBasicTimer::start(int msec, QObject *object)

    Starts (or restarts) the timer with a \a msec milliseconds timeout. The
    timer will be a Qt::CoarseTimer. See Qt::TimerType for information on the
    different timer types.

    The given \a object will receive timer events.

    \sa stop(), isActive(), QObject::timerEvent(), Qt::CoarseTimer
 */
void QBasicTimer::start(int msec, QObject *obj)
{
    QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
    if (Q_UNLIKELY(!eventDispatcher)) {
        qWarning("QBasicTimer::start: QBasicTimer can only be used with threads started with QThread");
        return;
    }
    if (Q_UNLIKELY(obj && obj->thread() != eventDispatcher->thread())) {
        qWarning("QBasicTimer::start: Timers cannot be started from another thread");
        return;
    }
    if (id) {
        if (Q_LIKELY(eventDispatcher->unregisterTimer(id)))
            QAbstractEventDispatcherPrivate::releaseTimerId(id);
        else
            qWarning("QBasicTimer::start: Stopping previous timer failed. Possibly trying to stop from a different thread");
    }
    id = 0;
    if (obj)
        id = eventDispatcher->registerTimer(msec, Qt::CoarseTimer, obj);
}

/*!
    \overload

    Starts (or restarts) the timer with a \a msec milliseconds timeout and the
    given \a timerType. See Qt::TimerType for information on the different
    timer types.

    \a obj will receive timer events.

    \sa stop(), isActive(), QObject::timerEvent(), Qt::TimerType
 */
void QBasicTimer::start(int msec, Qt::TimerType timerType, QObject *obj)
{
    QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
    if (Q_UNLIKELY(msec < 0)) {
        qWarning("QBasicTimer::start: Timers cannot have negative timeouts");
        return;
    }
    if (Q_UNLIKELY(!eventDispatcher)) {
        qWarning("QBasicTimer::start: QBasicTimer can only be used with threads started with QThread");
        return;
    }
    if (Q_UNLIKELY(obj && obj->thread() != eventDispatcher->thread())) {
        qWarning("QBasicTimer::start: Timers cannot be started from another thread");
        return;
    }
    if (id) {
        if (Q_LIKELY(eventDispatcher->unregisterTimer(id)))
            QAbstractEventDispatcherPrivate::releaseTimerId(id);
        else
            qWarning("QBasicTimer::start: Stopping previous timer failed. Possibly trying to stop from a different thread");
    }
    id = 0;
    if (obj)
        id = eventDispatcher->registerTimer(msec, timerType, obj);
}

/*!
    Stops the timer.

    \sa start(), isActive()
*/
void QBasicTimer::stop()
{
    if (id) {
        QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
        if (eventDispatcher) {
            if (Q_UNLIKELY(!eventDispatcher->unregisterTimer(id))) {
                qWarning("QBasicTimer::stop: Failed. Possibly trying to stop from a different thread");
                return;
            }
            QAbstractEventDispatcherPrivate::releaseTimerId(id);
        }
    }
    id = 0;
}

QT_END_NAMESPACE
