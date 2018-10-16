/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite module of the Qt Toolkit.
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

#include <QtWidgets>
#include <qtest.h>

#include "dummyobject.h"
#include "dummyanimation.h"
#include "rectanimation.h"

#define ITERATION_COUNT 10e3

class tst_qanimation : public QObject
{
    Q_OBJECT
private slots:
    void itemPropertyAnimation();
    void itemPropertyAnimation_data() { data();}
    void dummyAnimation();
    void dummyAnimation_data() { data();}
    void dummyPropertyAnimation();
    void dummyPropertyAnimation_data() { data();}
    void rectAnimation();
    void rectAnimation_data() { data();}

    void floatAnimation_data() { data(); }
    void floatAnimation();

private:
    void data();
};


void tst_qanimation::data()
{
    QTest::addColumn<bool>("started");
    QTest::newRow("NotRunning") << false;
    QTest::newRow("Running") << true;
}

void tst_qanimation::itemPropertyAnimation()
{
    QFETCH(bool, started);
    QGraphicsWidget item;

    //then the property animation
    {
        QPropertyAnimation anim(&item, "pos");
        anim.setDuration(ITERATION_COUNT);
        anim.setStartValue(QPointF(0,0));
        anim.setEndValue(QPointF(ITERATION_COUNT,ITERATION_COUNT));
        if (started)
            anim.start();
        QBENCHMARK {
            for(int i = 0; i < ITERATION_COUNT; ++i) {
                anim.setCurrentTime(i);
            }
        }
    }

}

void tst_qanimation::dummyAnimation()
{
    QFETCH(bool, started);
    DummyObject dummy;

    //first the dummy animation
    {
        DummyAnimation anim(&dummy);
        anim.setDuration(ITERATION_COUNT);
        anim.setStartValue(QRect(0, 0, 0, 0));
        anim.setEndValue(QRect(0, 0, ITERATION_COUNT,ITERATION_COUNT));
        if (started)
            anim.start();
        QBENCHMARK {
            for(int i = 0; i < anim.duration(); ++i) {
                anim.setCurrentTime(i);
            }
        }
    }
}

void tst_qanimation::dummyPropertyAnimation()
{
    QFETCH(bool, started);
    DummyObject dummy;

    //then the property animation
    {
        QPropertyAnimation anim(&dummy, "rect");
        anim.setDuration(ITERATION_COUNT);
        anim.setStartValue(QRect(0, 0, 0, 0));
        anim.setEndValue(QRect(0, 0, ITERATION_COUNT,ITERATION_COUNT));
        if (started)
            anim.start();
        QBENCHMARK {
            for(int i = 0; i < ITERATION_COUNT; ++i) {
                anim.setCurrentTime(i);
            }
        }
    }
}

void tst_qanimation::rectAnimation()
{
    //this is the simplest animation you can do
    QFETCH(bool, started);
    DummyObject dummy;

    //then the property animation
    {
        RectAnimation anim(&dummy);
        anim.setDuration(ITERATION_COUNT);
        anim.setStartValue(QRect(0, 0, 0, 0));
        anim.setEndValue(QRect(0, 0, ITERATION_COUNT,ITERATION_COUNT));
        if (started)
            anim.start();
        QBENCHMARK {
            for(int i = 0; i < ITERATION_COUNT; ++i) {
                anim.setCurrentTime(i);
            }
        }
    }
}

void tst_qanimation::floatAnimation()
{
    //this is the simplest animation you can do
    QFETCH(bool, started);
    DummyObject dummy;

    //then the property animation
    {
        QPropertyAnimation anim(&dummy, "opacity");
        anim.setDuration(ITERATION_COUNT);
        anim.setStartValue(0.f);
        anim.setEndValue(1.f);
        if (started)
            anim.start();
        QBENCHMARK {
            for(int i = 0; i < ITERATION_COUNT; ++i) {
                anim.setCurrentTime(i);
            }
        }
    }
}



QTEST_MAIN(tst_qanimation)

#include "main.moc"
