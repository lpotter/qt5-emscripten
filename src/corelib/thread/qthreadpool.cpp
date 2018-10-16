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

#include "qthreadpool.h"
#include "qthreadpool_p.h"
#include "qelapsedtimer.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QThreadPool, theInstance)

/*
    QThread wrapper, provides synchronization against a ThreadPool
*/
class QThreadPoolThread : public QThread
{
public:
    QThreadPoolThread(QThreadPoolPrivate *manager);
    void run() override;
    void registerThreadInactive();

    QWaitCondition runnableReady;
    QThreadPoolPrivate *manager;
    QRunnable *runnable;
};

/*
    QThreadPool private class.
*/


/*!
    \internal
*/
QThreadPoolThread::QThreadPoolThread(QThreadPoolPrivate *manager)
    :manager(manager), runnable(nullptr)
{
    setStackSize(manager->stackSize);
}

/*
    \internal
*/
void QThreadPoolThread::run()
{
    QMutexLocker locker(&manager->mutex);
    for(;;) {
        QRunnable *r = runnable;
        runnable = nullptr;

        do {
            if (r) {
                const bool autoDelete = r->autoDelete();


                // run the task
                locker.unlock();
#ifndef QT_NO_EXCEPTIONS
                try {
#endif
                    r->run();
#ifndef QT_NO_EXCEPTIONS
                } catch (...) {
                    qWarning("Qt Concurrent has caught an exception thrown from a worker thread.\n"
                             "This is not supported, exceptions thrown in worker threads must be\n"
                             "caught before control returns to Qt Concurrent.");
                    registerThreadInactive();
                    throw;
                }
#endif
                locker.relock();

                if (autoDelete && !--r->ref)
                    delete r;
            }

            // if too many threads are active, expire this thread
            if (manager->tooManyThreadsActive())
                break;

            if (manager->queue.isEmpty()) {
                r = nullptr;
                break;
            }

            QueuePage *page = manager->queue.first();
            r = page->pop();

            if (page->isFinished()) {
                manager->queue.removeFirst();
                delete page;
            }
        } while (true);

        if (manager->isExiting) {
            registerThreadInactive();
            break;
        }

        // if too many threads are active, expire this thread
        bool expired = manager->tooManyThreadsActive();
        if (!expired) {
            manager->waitingThreads.enqueue(this);
            registerThreadInactive();
            // wait for work, exiting after the expiry timeout is reached
            runnableReady.wait(locker.mutex(), manager->expiryTimeout);
            ++manager->activeThreads;
            if (manager->waitingThreads.removeOne(this))
                expired = true;
        }
        if (expired) {
            manager->expiredThreads.enqueue(this);
            registerThreadInactive();
            break;
        }
    }
}

void QThreadPoolThread::registerThreadInactive()
{
    if (--manager->activeThreads == 0)
        manager->noActiveThreads.wakeAll();
}


/*
    \internal
*/
QThreadPoolPrivate:: QThreadPoolPrivate()
{ }

bool QThreadPoolPrivate::tryStart(QRunnable *task)
{
    Q_ASSERT(task != nullptr);
    if (allThreads.isEmpty()) {
        // always create at least one thread
        startThread(task);
        return true;
    }

    // can't do anything if we're over the limit
    if (activeThreadCount() >= maxThreadCount)
        return false;

    if (waitingThreads.count() > 0) {
        // recycle an available thread
        enqueueTask(task);
        waitingThreads.takeFirst()->runnableReady.wakeOne();
        return true;
    }

    if (!expiredThreads.isEmpty()) {
        // restart an expired thread
        QThreadPoolThread *thread = expiredThreads.dequeue();
        Q_ASSERT(thread->runnable == nullptr);

        ++activeThreads;

        if (task->autoDelete())
            ++task->ref;
        thread->runnable = task;
        thread->start();
        return true;
    }

    // start a new thread
    startThread(task);
    return true;
}

inline bool comparePriority(int priority, const QueuePage *p)
{
    return p->priority() < priority;
}

void QThreadPoolPrivate::enqueueTask(QRunnable *runnable, int priority)
{
    Q_ASSERT(runnable != nullptr);
    if (runnable->autoDelete())
        ++runnable->ref;

    for (QueuePage *page : qAsConst(queue)) {
        if (page->priority() == priority && !page->isFull()) {
            page->push(runnable);
            return;
        }
    }
    auto it = std::upper_bound(queue.constBegin(), queue.constEnd(), priority, comparePriority);
    queue.insert(std::distance(queue.constBegin(), it), new QueuePage(runnable, priority));
}

int QThreadPoolPrivate::activeThreadCount() const
{
    return (allThreads.count()
            - expiredThreads.count()
            - waitingThreads.count()
            + reservedThreads);
}

void QThreadPoolPrivate::tryToStartMoreThreads()
{
    // try to push tasks on the queue to any available threads
    while (!queue.isEmpty()) {
        QueuePage *page = queue.first();
        if (!tryStart(page->first()))
            break;

        page->pop();

        if (page->isFinished()) {
            queue.removeFirst();
            delete page;
        }
    }
}

bool QThreadPoolPrivate::tooManyThreadsActive() const
{
    const int activeThreadCount = this->activeThreadCount();
    return activeThreadCount > maxThreadCount && (activeThreadCount - reservedThreads) > 1;
}

/*!
    \internal
*/
void QThreadPoolPrivate::startThread(QRunnable *runnable)
{
    Q_ASSERT(runnable != nullptr);
    QScopedPointer <QThreadPoolThread> thread(new QThreadPoolThread(this));
    thread->setObjectName(QLatin1String("Thread (pooled)"));
    Q_ASSERT(!allThreads.contains(thread.data())); // if this assert hits, we have an ABA problem (deleted threads don't get removed here)
    allThreads.append(thread.data());
    ++activeThreads;

    if (runnable->autoDelete())
        ++runnable->ref;
    thread->runnable = runnable;
    thread.take()->start();
}

/*!
    \internal
    Makes all threads exit, waits for each thread to exit and deletes it.
*/
void QThreadPoolPrivate::reset()
{
    QMutexLocker locker(&mutex);
    isExiting = true;

    while (!allThreads.empty()) {
        // move the contents of the set out so that we can iterate without the lock
        QList<QThreadPoolThread *> allThreadsCopy;
        allThreadsCopy.swap(allThreads);
        locker.unlock();

        for (QThreadPoolThread *thread : qAsConst(allThreadsCopy)) {
            thread->runnableReady.wakeAll();
            thread->wait();
            delete thread;
        }

        locker.relock();
        // repeat until all newly arrived threads have also completed
    }

    waitingThreads.clear();
    expiredThreads.clear();

    isExiting = false;
}

bool QThreadPoolPrivate::waitForDone(int msecs)
{
    QMutexLocker locker(&mutex);
    if (msecs < 0) {
        while (!(queue.isEmpty() && activeThreads == 0))
            noActiveThreads.wait(locker.mutex());
    } else {
        QElapsedTimer timer;
        timer.start();
        int t;
        while (!(queue.isEmpty() && activeThreads == 0) &&
               ((t = msecs - timer.elapsed()) > 0))
            noActiveThreads.wait(locker.mutex(), t);
    }
    return queue.isEmpty() && activeThreads == 0;
}

void QThreadPoolPrivate::clear()
{
    QMutexLocker locker(&mutex);
    for (QueuePage *page : qAsConst(queue)) {
        while (!page->isFinished()) {
            QRunnable *r = page->pop();
            if (r && r->autoDelete() && !--r->ref)
                delete r;
        }
    }
    qDeleteAll(queue);
    queue.clear();
}

/*!
    \since 5.9

    Attempts to remove the specified \a runnable from the queue if it is not yet started.
    If the runnable had not been started, returns \c true, and ownership of \a runnable
    is transferred to the caller (even when \c{runnable->autoDelete() == true}).
    Otherwise returns \c false.

    \note If \c{runnable->autoDelete() == true}, this function may remove the wrong
    runnable. This is known as the \l{https://en.wikipedia.org/wiki/ABA_problem}{ABA problem}:
    the original \a runnable may already have executed and has since been deleted.
    The memory is re-used for another runnable, which then gets removed instead of
    the intended one. For this reason, we recommend calling this function only for
    runnables that are not auto-deleting.

    \sa start(), QRunnable::autoDelete()
*/
bool QThreadPool::tryTake(QRunnable *runnable)
{
    Q_D(QThreadPool);

    if (runnable == nullptr)
        return false;
    {
        QMutexLocker locker(&d->mutex);

        for (QueuePage *page : qAsConst(d->queue)) {
            if (page->tryTake(runnable)) {
                if (page->isFinished()) {
                    d->queue.removeOne(page);
                    delete page;
                }
                if (runnable->autoDelete())
                    --runnable->ref; // undo ++ref in start()
                return true;
            }
        }
    }

    return false;
}

    /*!
     \internal
     Searches for \a runnable in the queue, removes it from the queue and
     runs it if found. This function does not return until the runnable
     has completed.
     */
void QThreadPoolPrivate::stealAndRunRunnable(QRunnable *runnable)
{
    Q_Q(QThreadPool);
    if (!q->tryTake(runnable))
        return;
    const bool del = runnable->autoDelete() && !runnable->ref; // tryTake already deref'ed

    runnable->run();

    if (del) {
        delete runnable;
    }
}

/*!
    \class QThreadPool
    \inmodule QtCore
    \brief The QThreadPool class manages a collection of QThreads.
    \since 4.4
    \threadsafe

    \ingroup thread

    QThreadPool manages and recyles individual QThread objects to help reduce
    thread creation costs in programs that use threads. Each Qt application
    has one global QThreadPool object, which can be accessed by calling
    globalInstance().

    To use one of the QThreadPool threads, subclass QRunnable and implement
    the run() virtual function. Then create an object of that class and pass
    it to QThreadPool::start().

    \snippet code/src_corelib_concurrent_qthreadpool.cpp 0

    QThreadPool deletes the QRunnable automatically by default. Use
    QRunnable::setAutoDelete() to change the auto-deletion flag.

    QThreadPool supports executing the same QRunnable more than once
    by calling tryStart(this) from within QRunnable::run().
    If autoDelete is enabled the QRunnable will be deleted when
    the last thread exits the run function. Calling start()
    multiple times with the same QRunnable when autoDelete is enabled
    creates a race condition and is not recommended.

    Threads that are unused for a certain amount of time will expire. The
    default expiry timeout is 30000 milliseconds (30 seconds). This can be
    changed using setExpiryTimeout(). Setting a negative expiry timeout
    disables the expiry mechanism.

    Call maxThreadCount() to query the maximum number of threads to be used.
    If needed, you can change the limit with setMaxThreadCount(). The default
    maxThreadCount() is QThread::idealThreadCount(). The activeThreadCount()
    function returns the number of threads currently doing work.

    The reserveThread() function reserves a thread for external
    use. Use releaseThread() when your are done with the thread, so
    that it may be reused.  Essentially, these functions temporarily
    increase or reduce the active thread count and are useful when
    implementing time-consuming operations that are not visible to the
    QThreadPool.

    Note that QThreadPool is a low-level class for managing threads, see
    the Qt Concurrent module for higher level alternatives.

    \sa QRunnable
*/

/*!
    Constructs a thread pool with the given \a parent.
*/
QThreadPool::QThreadPool(QObject *parent)
    : QObject(*new QThreadPoolPrivate, parent)
{ }

/*!
    Destroys the QThreadPool.
    This function will block until all runnables have been completed.
*/
QThreadPool::~QThreadPool()
{
    waitForDone();
}

/*!
    Returns the global QThreadPool instance.
*/
QThreadPool *QThreadPool::globalInstance()
{
    return theInstance();
}

/*!
    Reserves a thread and uses it to run \a runnable, unless this thread will
    make the current thread count exceed maxThreadCount().  In that case,
    \a runnable is added to a run queue instead. The \a priority argument can
    be used to control the run queue's order of execution.

    Note that the thread pool takes ownership of the \a runnable if
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns \c true,
    and the \a runnable will be deleted automatically by the thread
    pool after the \l{QRunnable::run()}{runnable->run()} returns. If
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns \c false,
    ownership of \a runnable remains with the caller. Note that
    changing the auto-deletion on \a runnable after calling this
    functions results in undefined behavior.
*/
void QThreadPool::start(QRunnable *runnable, int priority)
{
    if (!runnable)
        return;

    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    if (!d->tryStart(runnable)) {
        d->enqueueTask(runnable, priority);

        if (!d->waitingThreads.isEmpty())
            d->waitingThreads.takeFirst()->runnableReady.wakeOne();
    }
}

/*!
    Attempts to reserve a thread to run \a runnable.

    If no threads are available at the time of calling, then this function
    does nothing and returns \c false.  Otherwise, \a runnable is run immediately
    using one available thread and this function returns \c true.

    Note that the thread pool takes ownership of the \a runnable if
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns \c true,
    and the \a runnable will be deleted automatically by the thread
    pool after the \l{QRunnable::run()}{runnable->run()} returns. If
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns \c false,
    ownership of \a runnable remains with the caller. Note that
    changing the auto-deletion on \a runnable after calling this
    function results in undefined behavior.
*/
bool QThreadPool::tryStart(QRunnable *runnable)
{
    if (!runnable)
        return false;

    Q_D(QThreadPool);

    QMutexLocker locker(&d->mutex);

    if (d->allThreads.isEmpty() == false && d->activeThreadCount() >= d->maxThreadCount)
        return false;

    return d->tryStart(runnable);
}

/*! \property QThreadPool::expiryTimeout

    Threads that are unused for \a expiryTimeout milliseconds are considered
    to have expired and will exit. Such threads will be restarted as needed.
    The default \a expiryTimeout is 30000 milliseconds (30 seconds). If
    \a expiryTimeout is negative, newly created threads will not expire, e.g.,
    they will not exit until the thread pool is destroyed.

    Note that setting \a expiryTimeout has no effect on already running
    threads. Only newly created threads will use the new \a expiryTimeout.
    We recommend setting the \a expiryTimeout immediately after creating the
    thread pool, but before calling start().
*/

int QThreadPool::expiryTimeout() const
{
    Q_D(const QThreadPool);
    return d->expiryTimeout;
}

void QThreadPool::setExpiryTimeout(int expiryTimeout)
{
    Q_D(QThreadPool);
    if (d->expiryTimeout == expiryTimeout)
        return;
    d->expiryTimeout = expiryTimeout;
}

/*! \property QThreadPool::maxThreadCount

    This property represents the maximum number of threads used by the thread
    pool.

    \note The thread pool will always use at least 1 thread, even if
    \a maxThreadCount limit is zero or negative.

    The default \a maxThreadCount is QThread::idealThreadCount().
*/

int QThreadPool::maxThreadCount() const
{
    Q_D(const QThreadPool);
    return d->maxThreadCount;
}

void QThreadPool::setMaxThreadCount(int maxThreadCount)
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);

    if (maxThreadCount == d->maxThreadCount)
        return;

    d->maxThreadCount = maxThreadCount;
    d->tryToStartMoreThreads();
}

/*! \property QThreadPool::activeThreadCount

    This property represents the number of active threads in the thread pool.

    \note It is possible for this function to return a value that is greater
    than maxThreadCount(). See reserveThread() for more details.

    \sa reserveThread(), releaseThread()
*/

int QThreadPool::activeThreadCount() const
{
    Q_D(const QThreadPool);
    QMutexLocker locker(&d->mutex);
    return d->activeThreadCount();
}

/*!
    Reserves one thread, disregarding activeThreadCount() and maxThreadCount().

    Once you are done with the thread, call releaseThread() to allow it to be
    reused.

    \note This function will always increase the number of active threads.
    This means that by using this function, it is possible for
    activeThreadCount() to return a value greater than maxThreadCount() .

    \sa releaseThread()
 */
void QThreadPool::reserveThread()
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    ++d->reservedThreads;
}

/*! \property QThreadPool::stackSize

    This property contains the stack size for the thread pool worker
    threads.

    The value of the property is only used when the thread pool creates
    new threads. Changing it has no effect for already created
    or running threads.

    The default value is 0, which makes QThread use the operating
    system default stack size.

    \since 5.10
*/
void QThreadPool::setStackSize(uint stackSize)
{
    Q_D(QThreadPool);
    d->stackSize = stackSize;
}

uint QThreadPool::stackSize() const
{
    Q_D(const QThreadPool);
    return d->stackSize;
}

/*!
    Releases a thread previously reserved by a call to reserveThread().

    \note Calling this function without previously reserving a thread
    temporarily increases maxThreadCount(). This is useful when a
    thread goes to sleep waiting for more work, allowing other threads
    to continue. Be sure to call reserveThread() when done waiting, so
    that the thread pool can correctly maintain the
    activeThreadCount().

    \sa reserveThread()
*/
void QThreadPool::releaseThread()
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    --d->reservedThreads;
    d->tryToStartMoreThreads();
}

/*!
    Waits up to \a msecs milliseconds for all threads to exit and removes all
    threads from the thread pool. Returns \c true if all threads were removed;
    otherwise it returns \c false. If \a msecs is -1 (the default), the timeout
    is ignored (waits for the last thread to exit).
*/
bool QThreadPool::waitForDone(int msecs)
{
    Q_D(QThreadPool);
    bool rc = d->waitForDone(msecs);
    if (rc)
      d->reset();
    return rc;
}

/*!
    \since 5.2

    Removes the runnables that are not yet started from the queue.
    The runnables for which \l{QRunnable::autoDelete()}{runnable->autoDelete()}
    returns \c true are deleted.

    \sa start()
*/
void QThreadPool::clear()
{
    Q_D(QThreadPool);
    d->clear();
}

#if QT_DEPRECATED_SINCE(5, 9)
/*!
    \since 5.5
    \obsolete use tryTake() instead, but note the different deletion rules.

    Removes the specified \a runnable from the queue if it is not yet started.
    The runnables for which \l{QRunnable::autoDelete()}{runnable->autoDelete()}
    returns \c true are deleted.

    \sa start(), tryTake()
*/
void QThreadPool::cancel(QRunnable *runnable)
{
    if (tryTake(runnable) && runnable->autoDelete() && !runnable->ref) // tryTake already deref'ed
        delete runnable;
}
#endif

QT_END_NAMESPACE

#include "moc_qthreadpool.cpp"
