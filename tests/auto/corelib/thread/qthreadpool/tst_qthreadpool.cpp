/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QtTest>
#include <qdatetime.h>
#include <qthreadpool.h>
#include <qstring.h>
#include <qmutex.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

typedef void (*FunctionPointer)();

class FunctionPointerTask : public QRunnable
{
public:
    FunctionPointerTask(FunctionPointer function)
    :function(function) {}
    void run() { function(); }
private:
    FunctionPointer function;
};

QRunnable *createTask(FunctionPointer pointer)
{
    return new FunctionPointerTask(pointer);
}

class tst_QThreadPool : public QObject
{
    Q_OBJECT
public:
    tst_QThreadPool();
    ~tst_QThreadPool();

    static QMutex *functionTestMutex;

private slots:
    void runFunction();
    void createThreadRunFunction();
    void runMultiple();
    void waitcomplete();
    void runTask();
    void singleton();
    void destruction();
    void threadRecycling();
    void expiryTimeout();
    void expiryTimeoutRace();
#ifndef QT_NO_EXCEPTIONS
    void exceptions();
#endif
    void setMaxThreadCount_data();
    void setMaxThreadCount();
    void setMaxThreadCountStartsAndStopsThreads();
    void reserveThread_data();
    void reserveThread();
    void releaseThread_data();
    void releaseThread();
    void reserveAndStart();
    void start();
    void tryStart();
    void tryStartPeakThreadCount();
    void tryStartCount();
    void priorityStart_data();
    void priorityStart();
    void waitForDone();
    void clear();
    void cancel();
    void tryTake();
    void waitForDoneTimeout();
    void destroyingWaitsForTasksToFinish();
    void stackSize();
    void stressTest();
    void takeAllAndIncreaseMaxThreadCount();
    void waitForDoneAfterTake();

private:
    QMutex m_functionTestMutex;
};


QMutex *tst_QThreadPool::functionTestMutex = 0;

tst_QThreadPool::tst_QThreadPool()
{
    tst_QThreadPool::functionTestMutex = &m_functionTestMutex;
}

tst_QThreadPool::~tst_QThreadPool()
{
    tst_QThreadPool::functionTestMutex = 0;
}

int testFunctionCount;

void sleepTestFunction()
{
    QTest::qSleep(1000);
    ++testFunctionCount;
}

void emptyFunct()
{

}

void noSleepTestFunction()
{
    ++testFunctionCount;
}

void sleepTestFunctionMutex()
{
    Q_ASSERT(tst_QThreadPool::functionTestMutex);
    QTest::qSleep(1000);
    tst_QThreadPool::functionTestMutex->lock();
    ++testFunctionCount;
    tst_QThreadPool::functionTestMutex->unlock();
}

void noSleepTestFunctionMutex()
{
    Q_ASSERT(tst_QThreadPool::functionTestMutex);
    tst_QThreadPool::functionTestMutex->lock();
    ++testFunctionCount;
    tst_QThreadPool::functionTestMutex->unlock();
}

void tst_QThreadPool::runFunction()
{
    {
        QThreadPool manager;
        testFunctionCount = 0;
        manager.start(createTask(noSleepTestFunction));
    }
    QCOMPARE(testFunctionCount, 1);
}

void tst_QThreadPool::createThreadRunFunction()
{
    {
        QThreadPool manager;
        testFunctionCount = 0;
        manager.start(createTask(noSleepTestFunction));
    }

    QCOMPARE(testFunctionCount, 1);
}

void tst_QThreadPool::runMultiple()
{
    const int runs = 10;

    {
        QThreadPool manager;
        testFunctionCount = 0;
        for (int i = 0; i < runs; ++i) {
            manager.start(createTask(sleepTestFunctionMutex));
        }
    }
    QCOMPARE(testFunctionCount, runs);

    {
        QThreadPool manager;
        testFunctionCount = 0;
        for (int i = 0; i < runs; ++i) {
            manager.start(createTask(noSleepTestFunctionMutex));
        }
    }
    QCOMPARE(testFunctionCount, runs);

    {
        QThreadPool manager;
        for (int i = 0; i < 500; ++i)
            manager.start(createTask(emptyFunct));
    }
}

void tst_QThreadPool::waitcomplete()
{
    testFunctionCount = 0;
    const int runs = 500;
    for (int i = 0; i < 500; ++i) {
        QThreadPool pool;
        pool.start(createTask(noSleepTestFunction));
    }
    QCOMPARE(testFunctionCount, runs);
}

QAtomicInt ran; // bool
class TestTask : public QRunnable
{
public:
    void run()
    {
        ran.store(true);
    }
};

void tst_QThreadPool::runTask()
{
    QThreadPool manager;
    ran.store(false);
    manager.start(new TestTask());
    QTRY_VERIFY(ran.load());
}

/*
    Test running via QThreadPool::globalInstance()
*/
void tst_QThreadPool::singleton()
{
    ran.store(false);
    QThreadPool::globalInstance()->start(new TestTask());
    QTRY_VERIFY(ran.load());
}

QAtomicInt *value = 0;
class IntAccessor : public QRunnable
{
public:
    void run()
    {
        for (int i = 0; i < 100; ++i) {
            value->ref();
            QTest::qSleep(1);
        }
    }
};

/*
    Test that the ThreadManager destructor waits until
    all threads have completed.
*/
void tst_QThreadPool::destruction()
{
    value = new QAtomicInt;
    QThreadPool *threadManager = new QThreadPool();
    threadManager->start(new IntAccessor());
    threadManager->start(new IntAccessor());
    delete threadManager;
    delete value;
    value = 0;
}

QSemaphore threadRecyclingSemaphore;
QThread *recycledThread = 0;

class ThreadRecorderTask : public QRunnable
{
public:
    void run()
    {
        recycledThread = QThread::currentThread();
        threadRecyclingSemaphore.release();
    }
};

/*
    Test that the thread pool really reuses threads.
*/
void tst_QThreadPool::threadRecycling()
{
    QThreadPool threadPool;

    threadPool.start(new ThreadRecorderTask());
    threadRecyclingSemaphore.acquire();
    QThread *thread1 = recycledThread;

    QTest::qSleep(100);

    threadPool.start(new ThreadRecorderTask());
    threadRecyclingSemaphore.acquire();
    QThread *thread2 = recycledThread;
    QCOMPARE(thread1, thread2);

    QTest::qSleep(100);

    threadPool.start(new ThreadRecorderTask());
    threadRecyclingSemaphore.acquire();
    QThread *thread3 = recycledThread;
    QCOMPARE(thread2, thread3);
}

class ExpiryTimeoutTask : public QRunnable
{
public:
    QThread *thread;
    QAtomicInt runCount;
    QSemaphore semaphore;

    ExpiryTimeoutTask()
        : thread(0), runCount(0)
    {
        setAutoDelete(false);
    }

    void run()
    {
        thread = QThread::currentThread();
        runCount.ref();
        semaphore.release();
    }
};

void tst_QThreadPool::expiryTimeout()
{
    ExpiryTimeoutTask task;

    QThreadPool threadPool;
    threadPool.setMaxThreadCount(1);

    int expiryTimeout = threadPool.expiryTimeout();
    threadPool.setExpiryTimeout(1000);
    QCOMPARE(threadPool.expiryTimeout(), 1000);

    // run the task
    threadPool.start(&task);
    QVERIFY(task.semaphore.tryAcquire(1, 10000));
    QCOMPARE(task.runCount.load(), 1);
    QVERIFY(!task.thread->wait(100));
    // thread should expire
    QThread *firstThread = task.thread;
    QVERIFY(task.thread->wait(10000));

    // run task again, thread should be restarted
    threadPool.start(&task);
    QVERIFY(task.semaphore.tryAcquire(1, 10000));
    QCOMPARE(task.runCount.load(), 2);
    QVERIFY(!task.thread->wait(100));
    // thread should expire again
    QVERIFY(task.thread->wait(10000));

    // thread pool should have reused the expired thread (instead of
    // starting a new one)
    QCOMPARE(firstThread, task.thread);

    threadPool.setExpiryTimeout(expiryTimeout);
    QCOMPARE(threadPool.expiryTimeout(), expiryTimeout);
}

void tst_QThreadPool::expiryTimeoutRace() // QTBUG-3786
{
#ifdef Q_OS_WIN
    QSKIP("This test is unstable on Windows. See QTBUG-3786.");
#endif
    ExpiryTimeoutTask task;

    QThreadPool threadPool;
    threadPool.setMaxThreadCount(1);
    threadPool.setExpiryTimeout(50);
    const int numTasks = 20;
    for (int i = 0; i < numTasks; ++i) {
        threadPool.start(&task);
        QThread::msleep(50); // exactly the same as the expiry timeout
    }
    QVERIFY(task.semaphore.tryAcquire(numTasks, 10000));
    QCOMPARE(task.runCount.load(), numTasks);
    QVERIFY(threadPool.waitForDone(2000));
}

#ifndef QT_NO_EXCEPTIONS
class ExceptionTask : public QRunnable
{
public:
    void run()
    {
        throw new int;
    }
};

void tst_QThreadPool::exceptions()
{
    ExceptionTask task;
    {
        QThreadPool threadPool;
//  Uncomment this for a nice crash.
//        threadPool.start(&task);
    }
}
#endif

void tst_QThreadPool::setMaxThreadCount_data()
{
    QTest::addColumn<int>("limit");

    QTest::newRow("1") << 1;
    QTest::newRow("-1") << -1;
    QTest::newRow("2") << 2;
    QTest::newRow("-2") << -2;
    QTest::newRow("4") << 4;
    QTest::newRow("-4") << -4;
    QTest::newRow("0") << 0;
    QTest::newRow("12345") << 12345;
    QTest::newRow("-6789") << -6789;
    QTest::newRow("42") << 42;
    QTest::newRow("-666") << -666;
}

void tst_QThreadPool::setMaxThreadCount()
{
    QFETCH(int, limit);
    QThreadPool *threadPool = QThreadPool::globalInstance();
    int savedLimit = threadPool->maxThreadCount();

    // maxThreadCount() should always return the previous argument to
    // setMaxThreadCount(), regardless of input
    threadPool->setMaxThreadCount(limit);
    QCOMPARE(threadPool->maxThreadCount(), limit);

    // the value returned from maxThreadCount() should always be valid input for setMaxThreadCount()
    threadPool->setMaxThreadCount(savedLimit);
    QCOMPARE(threadPool->maxThreadCount(), savedLimit);

    // setting the limit on children should have no effect on the parent
    {
        QThreadPool threadPool2(threadPool);
        savedLimit = threadPool2.maxThreadCount();

        // maxThreadCount() should always return the previous argument to
        // setMaxThreadCount(), regardless of input
        threadPool2.setMaxThreadCount(limit);
        QCOMPARE(threadPool2.maxThreadCount(), limit);

        // the value returned from maxThreadCount() should always be valid input for setMaxThreadCount()
        threadPool2.setMaxThreadCount(savedLimit);
        QCOMPARE(threadPool2.maxThreadCount(), savedLimit);
    }
}

void tst_QThreadPool::setMaxThreadCountStartsAndStopsThreads()
{
    class WaitingTask : public QRunnable
    {
    public:
        QSemaphore waitForStarted, waitToFinish;

        WaitingTask() { setAutoDelete(false); }

        void run()
        {
            waitForStarted.release();
            waitToFinish.acquire();
        }
    };

    QThreadPool threadPool;
    threadPool.setMaxThreadCount(1);

    WaitingTask *task = new WaitingTask;
    threadPool.start(task);
    QVERIFY(task->waitForStarted.tryAcquire(1, 1000));

    // thread limit is 1, cannot start more tasks
    threadPool.start(task);
    QVERIFY(!task->waitForStarted.tryAcquire(1, 1000));

    // increasing the limit by 1 should start the task immediately
    threadPool.setMaxThreadCount(2);
    QVERIFY(task->waitForStarted.tryAcquire(1, 1000));

    // ... but we still cannot start more tasks
    threadPool.start(task);
    QVERIFY(!task->waitForStarted.tryAcquire(1, 1000));

    // increasing the limit should be able to start more than one at a time
    threadPool.start(task);
    threadPool.setMaxThreadCount(4);
    QVERIFY(task->waitForStarted.tryAcquire(2, 1000));

    // ... but we still cannot start more tasks
    threadPool.start(task);
    threadPool.start(task);
    QVERIFY(!task->waitForStarted.tryAcquire(2, 1000));

    // decreasing the thread limit should cause the active thread count to go down
    threadPool.setMaxThreadCount(2);
    QCOMPARE(threadPool.activeThreadCount(), 4);
    task->waitToFinish.release(2);
    QTest::qWait(1000);
    QCOMPARE(threadPool.activeThreadCount(), 2);

    // ... and we still cannot start more tasks
    threadPool.start(task);
    threadPool.start(task);
    QVERIFY(!task->waitForStarted.tryAcquire(2, 1000));

    // start all remaining tasks
    threadPool.start(task);
    threadPool.start(task);
    threadPool.start(task);
    threadPool.start(task);
    threadPool.setMaxThreadCount(8);
    QVERIFY(task->waitForStarted.tryAcquire(6, 1000));

    task->waitToFinish.release(10);
    threadPool.waitForDone();
    delete task;
}

void tst_QThreadPool::reserveThread_data()
{
    setMaxThreadCount_data();
}

void tst_QThreadPool::reserveThread()
{
    QFETCH(int, limit);
    QThreadPool *threadpool = QThreadPool::globalInstance();
    int savedLimit = threadpool->maxThreadCount();
    threadpool->setMaxThreadCount(limit);

    // reserve up to the limit
    for (int i = 0; i < limit; ++i)
        threadpool->reserveThread();

    // reserveThread() should always reserve a thread, regardless of
    // how many have been previously reserved
    threadpool->reserveThread();
    QCOMPARE(threadpool->activeThreadCount(), (limit > 0 ? limit : 0) + 1);
    threadpool->reserveThread();
    QCOMPARE(threadpool->activeThreadCount(), (limit > 0 ? limit : 0) + 2);

    // cleanup
    threadpool->releaseThread();
    threadpool->releaseThread();
    for (int i = 0; i < limit; ++i)
        threadpool->releaseThread();

    // reserving threads in children should not effect the parent
    {
        QThreadPool threadpool2(threadpool);
        threadpool2.setMaxThreadCount(limit);

        // reserve up to the limit
        for (int i = 0; i < limit; ++i)
            threadpool2.reserveThread();

        // reserveThread() should always reserve a thread, regardless
        // of how many have been previously reserved
        threadpool2.reserveThread();
        QCOMPARE(threadpool2.activeThreadCount(), (limit > 0 ? limit : 0) + 1);
        threadpool2.reserveThread();
        QCOMPARE(threadpool2.activeThreadCount(), (limit > 0 ? limit : 0) + 2);

        threadpool->reserveThread();
        QCOMPARE(threadpool->activeThreadCount(), 1);
        threadpool->reserveThread();
        QCOMPARE(threadpool->activeThreadCount(), 2);

        // cleanup
        threadpool2.releaseThread();
        threadpool2.releaseThread();
        threadpool->releaseThread();
        threadpool->releaseThread();
        while (threadpool2.activeThreadCount() > 0)
            threadpool2.releaseThread();
    }

    // reset limit on global QThreadPool
    threadpool->setMaxThreadCount(savedLimit);
}

void tst_QThreadPool::releaseThread_data()
{
    setMaxThreadCount_data();
}

void tst_QThreadPool::releaseThread()
{
    QFETCH(int, limit);
    QThreadPool *threadpool = QThreadPool::globalInstance();
    int savedLimit = threadpool->maxThreadCount();
    threadpool->setMaxThreadCount(limit);

    // reserve up to the limit
    for (int i = 0; i < limit; ++i)
        threadpool->reserveThread();

    // release should decrease the number of reserved threads
    int reserved = threadpool->activeThreadCount();
    while (reserved-- > 0) {
        threadpool->releaseThread();
        QCOMPARE(threadpool->activeThreadCount(), reserved);
    }
    QCOMPARE(threadpool->activeThreadCount(), 0);

    // releaseThread() can release more than have been reserved
    threadpool->releaseThread();
    QCOMPARE(threadpool->activeThreadCount(), -1);
    threadpool->reserveThread();
    QCOMPARE(threadpool->activeThreadCount(), 0);

    // releasing threads in children should not effect the parent
    {
        QThreadPool threadpool2(threadpool);
        threadpool2.setMaxThreadCount(limit);

        // reserve up to the limit
        for (int i = 0; i < limit; ++i)
            threadpool2.reserveThread();

        // release should decrease the number of reserved threads
        int reserved = threadpool2.activeThreadCount();
        while (reserved-- > 0) {
            threadpool2.releaseThread();
            QCOMPARE(threadpool2.activeThreadCount(), reserved);
            QCOMPARE(threadpool->activeThreadCount(), 0);
        }
        QCOMPARE(threadpool2.activeThreadCount(), 0);
        QCOMPARE(threadpool->activeThreadCount(), 0);

        // releaseThread() can release more than have been reserved
        threadpool2.releaseThread();
        QCOMPARE(threadpool2.activeThreadCount(), -1);
        QCOMPARE(threadpool->activeThreadCount(), 0);
        threadpool2.reserveThread();
        QCOMPARE(threadpool2.activeThreadCount(), 0);
        QCOMPARE(threadpool->activeThreadCount(), 0);
    }

    // reset limit on global QThreadPool
    threadpool->setMaxThreadCount(savedLimit);
}

void tst_QThreadPool::reserveAndStart() // QTBUG-21051
{
    class WaitingTask : public QRunnable
    {
    public:
        QAtomicInt count;
        QSemaphore waitForStarted;
        QSemaphore waitBeforeDone;

        WaitingTask() { setAutoDelete(false); }

        void run()
        {
            count.ref();
            waitForStarted.release();
            waitBeforeDone.acquire();
        }
    };

    // Set up
    QThreadPool *threadpool = QThreadPool::globalInstance();
    int savedLimit = threadpool->maxThreadCount();
    threadpool->setMaxThreadCount(1);
    QCOMPARE(threadpool->activeThreadCount(), 0);

    // reserve
    threadpool->reserveThread();
    QCOMPARE(threadpool->activeThreadCount(), 1);

    // start a task, to get a running thread
    WaitingTask *task = new WaitingTask;
    threadpool->start(task);
    QCOMPARE(threadpool->activeThreadCount(), 2);
    task->waitForStarted.acquire();
    task->waitBeforeDone.release();
    QTRY_COMPARE(task->count.load(), 1);
    QTRY_COMPARE(threadpool->activeThreadCount(), 1);

    // now the thread is waiting, but tryStart() will fail since activeThreadCount() >= maxThreadCount()
    QVERIFY(!threadpool->tryStart(task));
    QTRY_COMPARE(threadpool->activeThreadCount(), 1);

    // start() will therefore do a failing tryStart(), followed by enqueueTask()
    // which will actually wake up the waiting thread.
    threadpool->start(task);
    QTRY_COMPARE(threadpool->activeThreadCount(), 2);
    task->waitForStarted.acquire();
    task->waitBeforeDone.release();
    QTRY_COMPARE(task->count.load(), 2);
    QTRY_COMPARE(threadpool->activeThreadCount(), 1);

    threadpool->releaseThread();
    QTRY_COMPARE(threadpool->activeThreadCount(), 0);

    delete task;

    threadpool->setMaxThreadCount(savedLimit);
}

QAtomicInt count;
class CountingRunnable : public QRunnable
{
    public: void run()
    {
        count.ref();
    }
};

void tst_QThreadPool::start()
{
    const int runs = 1000;
    count.store(0);
    {
        QThreadPool threadPool;
        for (int i = 0; i< runs; ++i) {
            threadPool.start(new CountingRunnable());
        }
    }
    QCOMPARE(count.load(), runs);
}

void tst_QThreadPool::tryStart()
{
    class WaitingTask : public QRunnable
    {
    public:
        QSemaphore semaphore;

        WaitingTask() { setAutoDelete(false); }

        void run()
        {
            semaphore.acquire();
            count.ref();
        }
    };

    count.store(0);

    WaitingTask task;
    QThreadPool threadPool;
    for (int i = 0; i < threadPool.maxThreadCount(); ++i) {
        threadPool.start(&task);
    }
    QVERIFY(!threadPool.tryStart(&task));
    task.semaphore.release(threadPool.maxThreadCount());
    threadPool.waitForDone();
    QCOMPARE(count.load(), threadPool.maxThreadCount());
}

QMutex mutex;
QAtomicInt activeThreads;
QAtomicInt peakActiveThreads;
void tst_QThreadPool::tryStartPeakThreadCount()
{
    class CounterTask : public QRunnable
    {
    public:
        CounterTask() { setAutoDelete(false); }

        void run()
        {
            {
                QMutexLocker lock(&mutex);
                activeThreads.ref();
                peakActiveThreads.store(qMax(peakActiveThreads.load(), activeThreads.load()));
            }

            QTest::qWait(100);
            {
                QMutexLocker lock(&mutex);
                activeThreads.deref();
            }
        }
    };

    CounterTask task;
    QThreadPool threadPool;

    for (int i = 0; i < 20; ++i) {
        if (threadPool.tryStart(&task) == false)
            QTest::qWait(10);
    }
    QCOMPARE(peakActiveThreads.load(), QThread::idealThreadCount());

    for (int i = 0; i < 20; ++i) {
        if (threadPool.tryStart(&task) == false)
            QTest::qWait(10);
    }
    QCOMPARE(peakActiveThreads.load(), QThread::idealThreadCount());
}

void tst_QThreadPool::tryStartCount()
{
    class SleeperTask : public QRunnable
    {
    public:
        SleeperTask() { setAutoDelete(false); }

        void run()
        {
            QTest::qWait(50);
        }
    };

    SleeperTask task;
    QThreadPool threadPool;
    const int runs = 5;

    for (int i = 0; i < runs; ++i) {
        int count = 0;
        while (threadPool.tryStart(&task))
            ++count;
        QCOMPARE(count, QThread::idealThreadCount());

        QTRY_COMPARE(threadPool.activeThreadCount(), 0);
    }
}

void tst_QThreadPool::priorityStart_data()
{
    QTest::addColumn<int>("otherCount");
    QTest::newRow("0") << 0;
    QTest::newRow("1") << 1;
    QTest::newRow("2") << 2;
}

void tst_QThreadPool::priorityStart()
{
    class Holder : public QRunnable
    {
    public:
        QSemaphore &sem;
        Holder(QSemaphore &sem) : sem(sem) {}
        void run()
        {
            sem.acquire();
        }
    };
    class Runner : public QRunnable
    {
    public:
        QAtomicPointer<QRunnable> &ptr;
        Runner(QAtomicPointer<QRunnable> &ptr) : ptr(ptr) {}
        void run()
        {
            ptr.testAndSetRelaxed(0, this);
        }
    };

    QFETCH(int, otherCount);
    QSemaphore sem;
    QAtomicPointer<QRunnable> firstStarted;
    QRunnable *expected;
    QThreadPool threadPool;
    threadPool.setMaxThreadCount(1); // start only one thread at a time

    // queue the holder first
    // We need to be sure that all threads are active when we
    // queue the two Runners
    threadPool.start(new Holder(sem));
    while (otherCount--)
        threadPool.start(new Runner(firstStarted), 0); // priority 0
    threadPool.start(expected = new Runner(firstStarted), 1); // priority 1

    sem.release();
    QVERIFY(threadPool.waitForDone());
    QCOMPARE(firstStarted.load(), expected);
}

void tst_QThreadPool::waitForDone()
{
    QTime total, pass;
    total.start();

    QThreadPool threadPool;
    while (total.elapsed() < 10000) {
        int runs;
        count.store(runs = 0);
        pass.restart();
        while (pass.elapsed() < 100) {
            threadPool.start(new CountingRunnable());
            ++runs;
        }
        threadPool.waitForDone();
        QCOMPARE(count.load(), runs);

        count.store(runs = 0);
        pass.restart();
        while (pass.elapsed() < 100) {
            threadPool.start(new CountingRunnable());
            threadPool.start(new CountingRunnable());
            runs += 2;
        }
        threadPool.waitForDone();
        QCOMPARE(count.load(), runs);
    }
}

void tst_QThreadPool::waitForDoneTimeout()
{
    QMutex mutex;
    class BlockedTask : public QRunnable
    {
    public:
      QMutex &mutex;
      explicit BlockedTask(QMutex &m) : mutex(m) {}

      void run()
        {
          mutex.lock();
          mutex.unlock();
          QTest::qSleep(50);
        }
    };

    QThreadPool threadPool;

    mutex.lock();
    threadPool.start(new BlockedTask(mutex));
    QVERIFY(!threadPool.waitForDone(100));
    mutex.unlock();
    QVERIFY(threadPool.waitForDone(400));
}

void tst_QThreadPool::clear()
{
    QSemaphore sem(0);
    class BlockingRunnable : public QRunnable
    {
        public:
            QSemaphore & sem;
            BlockingRunnable(QSemaphore & sem) : sem(sem){}
            void run()
            {
                sem.acquire();
                count.ref();
            }
    };

    QThreadPool threadPool;
    threadPool.setMaxThreadCount(10);
    int runs = 2 * threadPool.maxThreadCount();
    count.store(0);
    for (int i = 0; i <= runs; i++) {
        threadPool.start(new BlockingRunnable(sem));
    }
    threadPool.clear();
    sem.release(threadPool.maxThreadCount());
    threadPool.waitForDone();
    QCOMPARE(count.load(), threadPool.maxThreadCount());
}

void tst_QThreadPool::cancel()
{
    QSemaphore sem(0);
    QSemaphore startedThreads(0);

    class BlockingRunnable : public QRunnable
    {
    public:
        QSemaphore & sem;
        QSemaphore &startedThreads;
        QAtomicInt &dtorCounter;
        QAtomicInt &runCounter;
        int dummy;

        explicit BlockingRunnable(QSemaphore &s, QSemaphore &started, QAtomicInt &c, QAtomicInt &r)
            : sem(s), startedThreads(started), dtorCounter(c), runCounter(r){}

        ~BlockingRunnable()
        {
            dtorCounter.fetchAndAddRelaxed(1);
        }

        void run()
        {
            startedThreads.release();
            runCounter.fetchAndAddRelaxed(1);
            sem.acquire();
            count.ref();
        }
    };

    enum {
        MaxThreadCount = 3,
        OverProvisioning = 2,
        runs = MaxThreadCount * OverProvisioning
    };

    QThreadPool threadPool;
    threadPool.setMaxThreadCount(MaxThreadCount);
    BlockingRunnable *runnables[runs];

    // ensure that the QThreadPool doesn't deadlock if any of the checks fail
    // and cause an early return:
    const QSemaphoreReleaser semReleaser(sem, runs);

    count.store(0);
    QAtomicInt dtorCounter = 0;
    QAtomicInt runCounter = 0;
    for (int i = 0; i < runs; i++) {
        runnables[i] = new BlockingRunnable(sem, startedThreads, dtorCounter, runCounter);
        runnables[i]->setAutoDelete(i != 0 && i != (runs-1)); //one which will run and one which will not
        threadPool.cancel(runnables[i]); //verify NOOP for jobs not in the queue
        threadPool.start(runnables[i]);
    }
    // wait for all worker threads to have started up:
    QVERIFY(startedThreads.tryAcquire(MaxThreadCount, 60*1000 /* 1min */));

    for (int i = 0; i < runs; i++) {
        threadPool.cancel(runnables[i]);
    }
    runnables[0]->dummy = 0; //valgrind will catch this if cancel() is crazy enough to delete currently running jobs
    runnables[runs-1]->dummy = 0;
    QCOMPARE(dtorCounter.load(), runs - threadPool.maxThreadCount() - 1);
    sem.release(threadPool.maxThreadCount());
    threadPool.waitForDone();
    QCOMPARE(runCounter.load(), threadPool.maxThreadCount());
    QCOMPARE(count.load(), threadPool.maxThreadCount());
    QCOMPARE(dtorCounter.load(), runs - 2);
    delete runnables[0]; //if the pool deletes them then we'll get double-free crash
    delete runnables[runs-1];
}

void tst_QThreadPool::tryTake()
{
    QSemaphore sem(0);
    QSemaphore startedThreads(0);

    class BlockingRunnable : public QRunnable
    {
    public:
        QSemaphore &sem;
        QSemaphore &startedThreads;
        QAtomicInt &dtorCounter;
        QAtomicInt &runCounter;
        int dummy;

        explicit BlockingRunnable(QSemaphore &s, QSemaphore &started, QAtomicInt &c, QAtomicInt &r)
            : sem(s), startedThreads(started), dtorCounter(c), runCounter(r) {}

        ~BlockingRunnable()
        {
            dtorCounter.fetchAndAddRelaxed(1);
        }

        void run() override
        {
            startedThreads.release();
            runCounter.fetchAndAddRelaxed(1);
            sem.acquire();
            count.ref();
        }
    };

    enum {
        MaxThreadCount = 3,
        OverProvisioning = 2,
        Runs = MaxThreadCount * OverProvisioning
    };

    QThreadPool threadPool;
    threadPool.setMaxThreadCount(MaxThreadCount);
    BlockingRunnable *runnables[Runs];

    // ensure that the QThreadPool doesn't deadlock if any of the checks fail
    // and cause an early return:
    const QSemaphoreReleaser semReleaser(sem, Runs);

    count.store(0);
    QAtomicInt dtorCounter = 0;
    QAtomicInt runCounter = 0;
    for (int i = 0; i < Runs; i++) {
        runnables[i] = new BlockingRunnable(sem, startedThreads, dtorCounter, runCounter);
        runnables[i]->setAutoDelete(i != 0 && i != Runs - 1); // one which will run and one which will not
        QVERIFY(!threadPool.tryTake(runnables[i])); // verify NOOP for jobs not in the queue
        threadPool.start(runnables[i]);
    }
    // wait for all worker threads to have started up:
    QVERIFY(startedThreads.tryAcquire(MaxThreadCount, 60*1000 /* 1min */));

    for (int i = 0; i < MaxThreadCount; ++i) {
        // check taking runnables doesn't work once they were started:
        QVERIFY(!threadPool.tryTake(runnables[i]));
    }
    for (int i = MaxThreadCount; i < Runs ; ++i) {
        QVERIFY(threadPool.tryTake(runnables[i]));
        delete runnables[i];
    }

    runnables[0]->dummy = 0; // valgrind will catch this if tryTake() is crazy enough to delete currently running jobs
    QCOMPARE(dtorCounter.load(), int(Runs - MaxThreadCount));
    sem.release(MaxThreadCount);
    threadPool.waitForDone();
    QCOMPARE(runCounter.load(), int(MaxThreadCount));
    QCOMPARE(count.load(), int(MaxThreadCount));
    QCOMPARE(dtorCounter.load(), int(Runs - 1));
    delete runnables[0]; // if the pool deletes them then we'll get double-free crash
}

void tst_QThreadPool::destroyingWaitsForTasksToFinish()
{
    QTime total, pass;
    total.start();

    while (total.elapsed() < 10000) {
        int runs;
        count.store(runs = 0);
        {
            QThreadPool threadPool;
            pass.restart();
            while (pass.elapsed() < 100) {
                threadPool.start(new CountingRunnable());
                ++runs;
            }
        }
        QCOMPARE(count.load(), runs);

        count.store(runs = 0);
        {
            QThreadPool threadPool;
            pass.restart();
            while (pass.elapsed() < 100) {
                threadPool.start(new CountingRunnable());
                threadPool.start(new CountingRunnable());
                runs += 2;
            }
        }
        QCOMPARE(count.load(), runs);
    }
}

// Verify that QThreadPool::stackSize is used when creating
// new threads. Note that this tests the Qt property only
// since QThread::stackSize() does not reflect the actual
// stack size used by the native thread.
void tst_QThreadPool::stackSize()
{
#if defined(Q_OS_UNIX) && !(defined(_POSIX_THREAD_ATTR_STACKSIZE) && (_POSIX_THREAD_ATTR_STACKSIZE-0 > 0))
    QSKIP("Setting stack size is unsupported on this platform.");
#endif

    uint targetStackSize = 512 * 1024;
    uint threadStackSize = 1; // impossible value

    class StackSizeChecker : public QRunnable
    {
        public:
        uint *stackSize;

        StackSizeChecker(uint *stackSize)
        :stackSize(stackSize)
        {

        }

        void run()
        {
            *stackSize = QThread::currentThread()->stackSize();
        }
    };

    QThreadPool threadPool;
    threadPool.setStackSize(targetStackSize);
    threadPool.start(new StackSizeChecker(&threadStackSize));
    QVERIFY(threadPool.waitForDone(30000)); // 30s timeout
    QCOMPARE(threadStackSize, targetStackSize);
}

void tst_QThreadPool::stressTest()
{
    class Task : public QRunnable
    {
        QSemaphore semaphore;
    public:
        Task() { setAutoDelete(false); }

        void start()
        {
            QThreadPool::globalInstance()->start(this);
        }

        void wait()
        {
            semaphore.acquire();
        }

        void run()
        {
            semaphore.release();
        }
    };

    QTime total;
    total.start();
    while (total.elapsed() < 30000) {
        Task t;
        t.start();
        t.wait();
    }
}

void tst_QThreadPool::takeAllAndIncreaseMaxThreadCount() {
    class Task : public QRunnable
    {
    public:
        Task(QSemaphore *mainBarrier, QSemaphore *threadBarrier)
            : m_mainBarrier(mainBarrier)
            , m_threadBarrier(threadBarrier)
        {
            setAutoDelete(false);
        }

        void run() {
            m_mainBarrier->release();
            m_threadBarrier->acquire();
        }
    private:
        QSemaphore *m_mainBarrier;
        QSemaphore *m_threadBarrier;
    };

    QSemaphore mainBarrier;
    QSemaphore taskBarrier;

    QThreadPool threadPool;
    threadPool.setMaxThreadCount(1);

    Task *task1 = new Task(&mainBarrier, &taskBarrier);
    Task *task2 = new Task(&mainBarrier, &taskBarrier);
    Task *task3 = new Task(&mainBarrier, &taskBarrier);

    threadPool.start(task1);
    threadPool.start(task2);
    threadPool.start(task3);

    mainBarrier.acquire(1);

    QCOMPARE(threadPool.activeThreadCount(), 1);

    QVERIFY(!threadPool.tryTake(task1));
    QVERIFY(threadPool.tryTake(task2));
    QVERIFY(threadPool.tryTake(task3));

    // A bad queue implementation can segfault here because two consecutive items in the queue
    // have been taken
    threadPool.setMaxThreadCount(4);

    // Even though we increase the max thread count, there should only be one job to run
    QCOMPARE(threadPool.activeThreadCount(), 1);

    // Make sure jobs 2 and 3 never started
    QCOMPARE(mainBarrier.available(), 0);

    taskBarrier.release(1);

    threadPool.waitForDone();

    QCOMPARE(threadPool.activeThreadCount(), 0);

    delete task1;
    delete task2;
    delete task3;
}

void tst_QThreadPool::waitForDoneAfterTake()
{
    class Task : public QRunnable
    {
    public:
        Task(QSemaphore *mainBarrier, QSemaphore *threadBarrier)
            : m_mainBarrier(mainBarrier)
            , m_threadBarrier(threadBarrier)
        {}

        void run()
        {
            m_mainBarrier->release();
            m_threadBarrier->acquire();
        }

    private:
        QSemaphore *m_mainBarrier = nullptr;
        QSemaphore *m_threadBarrier = nullptr;
    };

    int threadCount = 4;

    // Blocks the main thread from releasing the threadBarrier before all run() functions have started
    QSemaphore mainBarrier;
    // Blocks the tasks from completing their run function
    QSemaphore threadBarrier;

    QThreadPool manager;
    manager.setMaxThreadCount(threadCount);

    // Fill all the threads with runnables that wait for the threadBarrier
    for (int i = 0; i < threadCount; i++) {
        auto *task = new Task(&mainBarrier, &threadBarrier);
        manager.start(task);
    }

    QVERIFY(manager.activeThreadCount() == manager.maxThreadCount());

    // Add runnables that are immediately removed from the pool queue.
    // This sets the queue elements to nullptr in QThreadPool and we want to test that
    // the threads keep going through the queue after encountering a nullptr.
    for (int i = 0; i < threadCount; i++) {
        QRunnable *runnable = createTask(emptyFunct);
        manager.start(runnable);
        QVERIFY(manager.tryTake(runnable));
    }

    // Add another runnable that will not be removed
    manager.start(createTask(emptyFunct));

    // Wait for the first runnables to start
    mainBarrier.acquire(threadCount);

    QVERIFY(mainBarrier.available() == 0);
    QVERIFY(threadBarrier.available() == 0);

    // Release runnables that are waiting and expect all runnables to complete
    threadBarrier.release(threadCount);

    // Using qFatal instead of QVERIFY to force exit if threads are still running after timeout.
    // Otherwise, QCoreApplication will still wait for the stale threads and never exit the test.
    if (!manager.waitForDone(5 * 60 * 1000))
        qFatal("waitForDone returned false. Aborting to stop background threads.");

}

QTEST_MAIN(tst_QThreadPool);
#include "tst_qthreadpool.moc"
