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

#include "qelapsedtimer.h"
#include "qdeadlinetimer.h"
#include "qdeadlinetimer_p.h"
#if defined(Q_OS_VXWORKS)
#include "qfunctions_vxworks.h"
#else
#include <sys/time.h>
#include <time.h>
#endif
#include <unistd.h>

#include <qatomic.h>
#include "private/qcore_unix_p.h"

#if defined(QT_NO_CLOCK_MONOTONIC) || defined(QT_BOOTSTRAPPED)
// turn off the monotonic clock
# ifdef _POSIX_MONOTONIC_CLOCK
#  undef _POSIX_MONOTONIC_CLOCK
# endif
# define _POSIX_MONOTONIC_CLOCK -1
#endif

QT_BEGIN_NAMESPACE

/*
 * Design:
 *
 * POSIX offers a facility to select the system's monotonic clock when getting
 * the current timestamp. Whereas the functions are mandatory in POSIX.1-2008,
 * the presence of a monotonic clock is a POSIX Option (see the document
 *  http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap02.html#tag_02_01_06 )
 *
 * The macro _POSIX_MONOTONIC_CLOCK can therefore assume the following values:
 *  -1          monotonic clock is never supported on this system
 *   0          monotonic clock might be supported, runtime check is needed
 *  >1          (such as 200809L) monotonic clock is always supported
 *
 * The unixCheckClockType() function will return the clock to use: either
 * CLOCK_MONOTONIC or CLOCK_REALTIME. In the case the POSIX option has a value
 * of zero, then this function stores a static that contains the clock to be
 * used.
 *
 * There's one extra case, which is when CLOCK_REALTIME isn't defined. When
 * that's the case, we'll emulate the clock_gettime function with gettimeofday.
 *
 * Conforming to:
 *  POSIX.1b-1993 section "Clocks and Timers"
 *  included in UNIX98 (Single Unix Specification v2)
 *  included in POSIX.1-2001
 *  see http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_getres.html
 */

#if !defined(CLOCK_REALTIME)
#  define CLOCK_REALTIME 0
static inline void qt_clock_gettime(int, struct timespec *ts)
{
    // support clock_gettime with gettimeofday
    struct timeval tv;
    gettimeofday(&tv, 0);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
}

#  ifdef _POSIX_MONOTONIC_CLOCK
#    undef _POSIX_MONOTONIC_CLOCK
#    define _POSIX_MONOTONIC_CLOCK -1
#  endif
#else
static inline void qt_clock_gettime(clockid_t clock, struct timespec *ts)
{
    clock_gettime(clock, ts);
}
#endif

static int unixCheckClockType()
{
#ifdef Q_OS_LINUX
    // Despite glibc claiming that we should check at runtime, the Linux kernel
    // always supports the monotonic clock
    return CLOCK_MONOTONIC;
#elif (_POSIX_MONOTONIC_CLOCK-0 == 0) && defined(_SC_MONOTONIC_CLOCK)
    // we need a value we can store in a clockid_t that isn't a valid clock
    // check if the valid ones are both non-negative or both non-positive
#  if CLOCK_MONOTONIC >= 0 && CLOCK_REALTIME >= 0
#    define IS_VALID_CLOCK(clock)    (clock >= 0)
#    define INVALID_CLOCK            -1
#  elif CLOCK_MONOTONIC <= 0 && CLOCK_REALTIME <= 0
#    define IS_VALID_CLOCK(clock)    (clock <= 0)
#    define INVALID_CLOCK            1
#  else
#    error "Sorry, your system has weird values for CLOCK_MONOTONIC and CLOCK_REALTIME"
#  endif

    static QBasicAtomicInt clockToUse = Q_BASIC_ATOMIC_INITIALIZER(INVALID_CLOCK);
    int clock = clockToUse.loadAcquire();
    if (Q_LIKELY(IS_VALID_CLOCK(clock)))
        return clock;

    // detect if the system supports monotonic timers
    clock = sysconf(_SC_MONOTONIC_CLOCK) > 0 ? CLOCK_MONOTONIC : CLOCK_REALTIME;
    clockToUse.storeRelease(clock);
    return clock;

#  undef INVALID_CLOCK
#  undef IS_VALID_CLOCK
#elif (_POSIX_MONOTONIC_CLOCK-0) > 0
    return CLOCK_MONOTONIC;
#else
    return CLOCK_REALTIME;
#endif
}

bool QElapsedTimer::isMonotonic() Q_DECL_NOTHROW
{
    return clockType() == MonotonicClock;
}

QElapsedTimer::ClockType QElapsedTimer::clockType() Q_DECL_NOTHROW
{
    return unixCheckClockType() == CLOCK_REALTIME ? SystemTime : MonotonicClock;
}

static inline void do_gettime(qint64 *sec, qint64 *frac)
{
    timespec ts;
    qt_clock_gettime(unixCheckClockType(), &ts);
    *sec = ts.tv_sec;
    *frac = ts.tv_nsec;
}

// used in qcore_unix.cpp and qeventdispatcher_unix.cpp
struct timespec qt_gettime() Q_DECL_NOTHROW
{
    qint64 sec, frac;
    do_gettime(&sec, &frac);

    timespec tv;
    tv.tv_sec = sec;
    tv.tv_nsec = frac;

    return tv;
}

void qt_nanosleep(timespec amount)
{
    // We'd like to use clock_nanosleep.
    //
    // But clock_nanosleep is from POSIX.1-2001 and both are *not*
    // affected by clock changes when using relative sleeps, even for
    // CLOCK_REALTIME.
    //
    // nanosleep is POSIX.1-1993

    int r;
    EINTR_LOOP(r, nanosleep(&amount, &amount));
}

static qint64 elapsedAndRestart(qint64 sec, qint64 frac,
                                qint64 *nowsec, qint64 *nowfrac)
{
    do_gettime(nowsec, nowfrac);
    sec = *nowsec - sec;
    frac = *nowfrac - frac;
    return (sec * Q_INT64_C(1000000000) + frac) / Q_INT64_C(1000000);
}

void QElapsedTimer::start() Q_DECL_NOTHROW
{
    do_gettime(&t1, &t2);
}

qint64 QElapsedTimer::restart() Q_DECL_NOTHROW
{
    return elapsedAndRestart(t1, t2, &t1, &t2);
}

qint64 QElapsedTimer::nsecsElapsed() const Q_DECL_NOTHROW
{
    qint64 sec, frac;
    do_gettime(&sec, &frac);
    sec = sec - t1;
    frac = frac - t2;
    return sec * Q_INT64_C(1000000000) + frac;
}

qint64 QElapsedTimer::elapsed() const Q_DECL_NOTHROW
{
    return nsecsElapsed() / Q_INT64_C(1000000);
}

qint64 QElapsedTimer::msecsSinceReference() const Q_DECL_NOTHROW
{
    return t1 * Q_INT64_C(1000) + t2 / Q_INT64_C(1000000);
}

qint64 QElapsedTimer::msecsTo(const QElapsedTimer &other) const Q_DECL_NOTHROW
{
    qint64 secs = other.t1 - t1;
    qint64 fraction = other.t2 - t2;
    return (secs * Q_INT64_C(1000000000) + fraction) / Q_INT64_C(1000000);
}

qint64 QElapsedTimer::secsTo(const QElapsedTimer &other) const Q_DECL_NOTHROW
{
    return other.t1 - t1;
}

bool operator<(const QElapsedTimer &v1, const QElapsedTimer &v2) Q_DECL_NOTHROW
{
    return v1.t1 < v2.t1 || (v1.t1 == v2.t1 && v1.t2 < v2.t2);
}

QDeadlineTimer QDeadlineTimer::current(Qt::TimerType timerType) Q_DECL_NOTHROW
{
    Q_STATIC_ASSERT(QDeadlineTimerNanosecondsInT2);
    QDeadlineTimer result;
    qint64 cursec, curnsec;
    do_gettime(&cursec, &curnsec);
    result.t1 = cursec;
    result.t2 = curnsec;
    result.type = timerType;
    return result;
}

QT_END_NAMESPACE
