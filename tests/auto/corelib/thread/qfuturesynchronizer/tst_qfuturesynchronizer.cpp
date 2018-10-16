/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QtTest>

#include <QtCore/qfuturesynchronizer.h>
#include <QtCore/qfuture.h>

class tst_QFutureSynchronizer : public QObject
{
    Q_OBJECT


private Q_SLOTS:
    void construction();
    void addFuture();
    void cancelOnWait();
    void clearFutures();
    void futures();
    void setFuture();
    void waitForFinished();
};


void tst_QFutureSynchronizer::construction()
{

    QFuture<void> future;
    QFutureSynchronizer<void> synchronizer;
    QFutureSynchronizer<void> synchronizerWithFuture(future);

    QCOMPARE(synchronizer.futures().size(), 0);
    QCOMPARE(synchronizerWithFuture.futures().size(), 1);
}

void tst_QFutureSynchronizer::addFuture()
{
    QFutureSynchronizer<void> synchronizer;

    synchronizer.addFuture(QFuture<void>());
    QFuture<void> future;
    synchronizer.addFuture(future);
    synchronizer.addFuture(future);

    QCOMPARE(synchronizer.futures().size(), 3);
}

void tst_QFutureSynchronizer::cancelOnWait()
{
    QFutureSynchronizer<void> synchronizer;
    QVERIFY(!synchronizer.cancelOnWait());
    synchronizer.setCancelOnWait(true);
    QVERIFY(synchronizer.cancelOnWait());
    synchronizer.setCancelOnWait(false);
    QVERIFY(!synchronizer.cancelOnWait());
    synchronizer.setCancelOnWait(true);
    QVERIFY(synchronizer.cancelOnWait());
}

void tst_QFutureSynchronizer::clearFutures()
{
    QFutureSynchronizer<void> synchronizer;
    synchronizer.clearFutures();
    QVERIFY(synchronizer.futures().isEmpty());

    synchronizer.addFuture(QFuture<void>());
    QFuture<void> future;
    synchronizer.addFuture(future);
    synchronizer.addFuture(future);
    synchronizer.clearFutures();
    QVERIFY(synchronizer.futures().isEmpty());
}

void tst_QFutureSynchronizer::futures()
{
    QFutureSynchronizer<void> synchronizer;

    QList<QFuture<void> > futures;
    for (int i=0; i<100; i++) {
        QFuture<void> future;
        futures.append(future);
        synchronizer.addFuture(future);
    }

    QCOMPARE(futures, synchronizer.futures());
}

void tst_QFutureSynchronizer::setFuture()
{
    QFutureSynchronizer<void> synchronizer;

    for (int i=0; i<100; i++) {
        synchronizer.addFuture(QFuture<void>());
    }
    QCOMPARE(synchronizer.futures().size(), 100);

    QFuture<void> future;
    synchronizer.setFuture(future);
    QCOMPARE(synchronizer.futures().size(), 1);
    QCOMPARE(synchronizer.futures().first(), future);
}

void tst_QFutureSynchronizer::waitForFinished()
{
    QFutureSynchronizer<void> synchronizer;

    for (int i=0; i<100; i++) {
        synchronizer.addFuture(QFuture<void>());
    }
    synchronizer.waitForFinished();
    const QList<QFuture<void> > futures = synchronizer.futures();

    for (int i=0; i<100; i++) {
        QVERIFY(futures.at(i).isFinished());
    }
}

QTEST_MAIN(tst_QFutureSynchronizer)

#include "tst_qfuturesynchronizer.moc"
