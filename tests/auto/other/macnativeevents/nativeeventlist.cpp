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

#include "nativeeventlist.h"

NativeEventList::NativeEventList(int defaultWaitMs)
    : playbackMultiplier(1.0)
    , currIndex(-1)
    , wait(false)
    , defaultWaitMs(defaultWaitMs)
{
    debug = qgetenv("NATIVEDEBUG").toInt();
    QString multiplier = qgetenv("NATIVEDEBUGSPEED");
    if (!multiplier.isEmpty())
        setTimeMultiplier(multiplier.toFloat());
}

NativeEventList::~NativeEventList()
{
    for (int i=0; i<eventList.size(); i++)
        delete eventList.takeAt(i).second;
}

void NativeEventList::sendNextEvent()
{
    QNativeEvent *e = eventList.at(currIndex).second;
    if (e) {
        if (debug > 0)
            qDebug() << "Sending:" << *e;
        QNativeInput::sendNativeEvent(*e);
    }
    waitNextEvent();
}

void NativeEventList::waitNextEvent()
{
    if (++currIndex >= eventList.size()){
        emit done();
        stop();
        return;
    }

    int interval = eventList.at(currIndex).first;
    QTimer::singleShot(interval * playbackMultiplier, this, SLOT(sendNextEvent()));
}

void NativeEventList::append(QNativeEvent *event)
{
    eventList.append(QPair<int, QNativeEvent *>(defaultWaitMs, event));
}

void NativeEventList::append(int waitMs, QNativeEvent *event)
{
    eventList.append(QPair<int, QNativeEvent *>(waitMs, event));
}

void NativeEventList::play(Playback playback)
{
    waitNextEvent();

    wait = (playback == WaitUntilFinished);
    while (wait)
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
}

void NativeEventList::stop()
{
    wait = false;
    QAbstractEventDispatcher::instance()->interrupt();
}

void NativeEventList::setTimeMultiplier(float multiplier)
{
    playbackMultiplier = multiplier;
}

