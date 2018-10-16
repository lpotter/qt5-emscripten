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

#include "qtconcurrentthreadengine.h"

#if !defined(QT_NO_CONCURRENT) || defined(Q_CLANG_QDOC)

QT_BEGIN_NAMESPACE

namespace QtConcurrent {

/*!
  \class QtConcurrent::ThreadEngineBarrier
  \inmodule QtConcurrent
  \internal
*/

/*!
  \enum QtConcurrent::ThreadFunctionResult
  \internal
*/

/*!
  \class QtConcurrent::ThreadEngineBase
  \inmodule QtConcurrent
  \internal
*/

/*!
  \class QtConcurrent::ThreadEngine
  \inmodule QtConcurrent
  \internal
*/

/*!
  \class QtConcurrent::ThreadEngineStarterBase
  \inmodule QtConcurrent
  \internal
*/

/*!
  \class QtConcurrent::ThreadEngineStarter
  \inmodule QtConcurrent
  \internal
*/

/*!
  \fn [qtconcurrentthreadengine-1] template <typename ThreadEngine> ThreadEngineStarter<typename ThreadEngine::ResultType> QtConcurrent::startThreadEngine(ThreadEngine *threadEngine)
  \internal
*/

ThreadEngineBarrier::ThreadEngineBarrier()
:count(0) { }

void ThreadEngineBarrier::acquire()
{
    forever {
        int localCount = count.load();
        if (localCount < 0) {
            if (count.testAndSetOrdered(localCount, localCount -1))
                return;
        } else {
            if (count.testAndSetOrdered(localCount, localCount + 1))
                return;
        }
    }
}

int ThreadEngineBarrier::release()
{
    forever {
        int localCount = count.load();
        if (localCount == -1) {
            if (count.testAndSetOrdered(-1, 0)) {
                semaphore.release();
                return 0;
            }
        } else if (localCount < 0) {
            if (count.testAndSetOrdered(localCount, localCount + 1))
                return qAbs(localCount + 1);
        } else {
            if (count.testAndSetOrdered(localCount, localCount - 1))
                return localCount - 1;
        }
    }
}

// Wait until all threads have been released
void ThreadEngineBarrier::wait()
{
    forever {
        int localCount = count.load();
        if (localCount == 0)
            return;

        Q_ASSERT(localCount > 0); // multiple waiters are not allowed.
        if (count.testAndSetOrdered(localCount, -localCount)) {
            semaphore.acquire();
            return;
        }
    }
}

int ThreadEngineBarrier::currentCount()
{
    return count.load();
}

// releases a thread, unless this is the last thread.
// returns true if the thread was released.
bool ThreadEngineBarrier::releaseUnlessLast()
{
    forever {
        int localCount = count.load();
        if (qAbs(localCount) == 1) {
            return false;
        } else if (localCount < 0) {
            if (count.testAndSetOrdered(localCount, localCount + 1))
                return true;
        } else {
            if (count.testAndSetOrdered(localCount, localCount - 1))
                return true;
        }
    }
}

ThreadEngineBase::ThreadEngineBase()
:futureInterface(0), threadPool(QThreadPool::globalInstance())
{
    setAutoDelete(false);
}

ThreadEngineBase::~ThreadEngineBase() {}

void ThreadEngineBase::startSingleThreaded()
{
    start();
    while (threadFunction() != ThreadFinished)
        ;
    finish();
}

void ThreadEngineBase::startBlocking()
{
    start();
    barrier.acquire();
    startThreads();

    bool throttled = false;
#ifndef QT_NO_EXCEPTIONS
    try {
#endif
        while (threadFunction() == ThrottleThread) {
            if (threadThrottleExit()) {
                throttled = true;
                break;
            }
        }
#ifndef QT_NO_EXCEPTIONS
    } catch (QException &e) {
        handleException(e);
    } catch (...) {
        handleException(QUnhandledException());
    }
#endif

    if (throttled == false) {
        barrier.release();
    }

    barrier.wait();
    finish();
    exceptionStore.throwPossibleException();
}

void ThreadEngineBase::startThread()
{
    startThreadInternal();
}

void ThreadEngineBase::acquireBarrierSemaphore()
{
    barrier.acquire();
}

bool ThreadEngineBase::isCanceled()
{
    if (futureInterface)
        return futureInterface->isCanceled();
    else
        return false;
}

void ThreadEngineBase::waitForResume()
{
    if (futureInterface)
        futureInterface->waitForResume();
}

bool ThreadEngineBase::isProgressReportingEnabled()
{
    // If we don't have a QFuture, there is no-one to report the progress to.
    return (futureInterface != 0);
}

void ThreadEngineBase::setProgressValue(int progress)
{
    if (futureInterface)
        futureInterface->setProgressValue(progress);
}

void ThreadEngineBase::setProgressRange(int minimum, int maximum)
{
    if (futureInterface)
        futureInterface->setProgressRange(minimum, maximum);
}

bool ThreadEngineBase::startThreadInternal()
{
    if (this->isCanceled())
        return false;

    barrier.acquire();
    if (!threadPool->tryStart(this)) {
        barrier.release();
        return false;
    }
    return true;
}

void ThreadEngineBase::startThreads()
{
    while (shouldStartThread() && startThreadInternal())
        ;
}

void ThreadEngineBase::threadExit()
{
    const bool asynchronous = futureInterface != 0;
    const int lastThread = (barrier.release() == 0);

    if (lastThread && asynchronous)
        this->asynchronousFinish();
}

// Called by a worker thread that wants to be throttled. If the current number
// of running threads is larger than one the thread is allowed to exit and
// this function returns one.
bool ThreadEngineBase::threadThrottleExit()
{
    return barrier.releaseUnlessLast();
}

void ThreadEngineBase::run() // implements QRunnable.
{
    if (this->isCanceled()) {
        threadExit();
        return;
    }

    startThreads();

#ifndef QT_NO_EXCEPTIONS
    try {
#endif
        while (threadFunction() == ThrottleThread) {
            // threadFunction returning ThrottleThread means it that the user
            // struct wants to be throttled by making a worker thread exit.
            // Respect that request unless this is the only worker thread left
            // running, in which case it has to keep going.
            if (threadThrottleExit())
                return;
        }

#ifndef QT_NO_EXCEPTIONS
    } catch (QException &e) {
        handleException(e);
    } catch (...) {
        handleException(QUnhandledException());
    }
#endif
    threadExit();
}

#ifndef QT_NO_EXCEPTIONS

void ThreadEngineBase::handleException(const QException &exception)
{
    if (futureInterface)
        futureInterface->reportException(exception);
    else
        exceptionStore.setException(exception);
}
#endif


} // namepsace QtConcurrent

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT
