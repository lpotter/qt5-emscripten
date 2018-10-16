/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QWINDOWSTABLETSUPPORT_H
#define QWINDOWSTABLETSUPPORT_H

#include "qtwindowsglobal.h"
#include <QtGui/qtguiglobal.h>

#include <QtCore/qvector.h>
#include <QtCore/qpoint.h>

#include <wintab.h>

QT_REQUIRE_CONFIG(tabletevent);

QT_BEGIN_NAMESPACE

class QDebug;
class QWindow;
class QRect;

struct QWindowsWinTab32DLL
{
    bool init();

    typedef HCTX (API *PtrWTOpen)(HWND, LPLOGCONTEXT, BOOL);
    typedef BOOL (API *PtrWTClose)(HCTX);
    typedef UINT (API *PtrWTInfo)(UINT, UINT, LPVOID);
    typedef BOOL (API *PtrWTEnable)(HCTX, BOOL);
    typedef BOOL (API *PtrWTOverlap)(HCTX, BOOL);
    typedef int  (API *PtrWTPacketsGet)(HCTX, int, LPVOID);
    typedef BOOL (API *PtrWTGet)(HCTX, LPLOGCONTEXT);
    typedef int  (API *PtrWTQueueSizeGet)(HCTX);
    typedef BOOL (API *PtrWTQueueSizeSet)(HCTX, int);

    PtrWTOpen wTOpen = nullptr;
    PtrWTClose wTClose = nullptr;
    PtrWTInfo wTInfo = nullptr;
    PtrWTEnable wTEnable = nullptr;
    PtrWTOverlap wTOverlap = nullptr;
    PtrWTPacketsGet wTPacketsGet = nullptr;
    PtrWTGet wTGet = nullptr;
    PtrWTQueueSizeGet wTQueueSizeGet = nullptr;
    PtrWTQueueSizeSet wTQueueSizeSet = nullptr;
};

struct QWindowsTabletDeviceData
{
    QPointF scaleCoordinates(int coordX, int coordY,const QRect &targetArea) const;
    qreal scalePressure(qreal p) const { return p / qreal(maxPressure - minPressure); }
    qreal scaleTangentialPressure(qreal p) const { return p / qreal(maxTanPressure - minTanPressure); }

    int minPressure = 0;
    int maxPressure = 0;
    int minTanPressure = 0;
    int maxTanPressure = 0;
    int minX = 0;
    int maxX = 0;
    int minY = 0;
    int maxY = 0;
    int minZ = 0;
    int maxZ = 0;
    qint64 uniqueId = 0;
    int currentDevice = 0;
    int currentPointerType = 0;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QWindowsTabletDeviceData &t);
#endif

class QWindowsTabletSupport
{
    Q_DISABLE_COPY(QWindowsTabletSupport)

    explicit QWindowsTabletSupport(HWND window, HCTX context);

public:
    enum Mode
    {
        PenMode,
        MouseMode
    };

    enum State
    {
        PenUp,
        PenProximity,
        PenDown
    };

    ~QWindowsTabletSupport();

    static QWindowsTabletSupport *create();

    void notifyActivate();
    QString description() const;

    bool translateTabletProximityEvent(WPARAM wParam, LPARAM lParam);
    bool translateTabletPacketEvent();

    int absoluteRange() const { return m_absoluteRange; }
    void setAbsoluteRange(int a) { m_absoluteRange = a; }

private:
    unsigned options() const;
    QWindowsTabletDeviceData tabletInit(qint64 uniqueId, UINT cursorType) const;

    static QWindowsWinTab32DLL m_winTab32DLL;
    const HWND m_window;
    const HCTX m_context;
    int m_absoluteRange = 20;
    bool m_tiltSupport = false;
    QVector<QWindowsTabletDeviceData> m_devices;
    int m_currentDevice = -1;
    Mode m_mode = PenMode;
    State m_state = PenUp;
};

QT_END_NAMESPACE

#endif // QWINDOWSTABLETSUPPORT_H
