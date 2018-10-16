/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#ifndef QWINDOWSPOINTERHANDLER_H
#define QWINDOWSPOINTERHANDLER_H

#include "qtwindowsglobal.h"
#include <QtCore/qt_windows.h>

#include <QtCore/qpointer.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

class QWindow;
class QTouchDevice;

class QWindowsPointerHandler
{
    Q_DISABLE_COPY(QWindowsPointerHandler)
public:
    QWindowsPointerHandler() = default;
    bool translatePointerEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et, MSG msg, LRESULT *result);
    bool translateMouseEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et, MSG msg, LRESULT *result);
    QTouchDevice *touchDevice() const { return m_touchDevice; }
    QTouchDevice *ensureTouchDevice();
    Qt::MouseButtons queryMouseButtons();
    QWindow *windowUnderMouse() const { return m_windowUnderPointer.data(); }
    void clearWindowUnderMouse() { m_windowUnderPointer = nullptr; }

private:
    bool translateMouseTouchPadEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et, MSG msg, PVOID vPointerInfo);
    bool translateTouchEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et, MSG msg, PVOID vTouchInfo, unsigned int count);
    bool translatePenEvent(QWindow *window, HWND hwnd, QtWindows::WindowsEventType et, MSG msg, PVOID vPenInfo);

    QTouchDevice *m_touchDevice = nullptr;
    QHash<int, QPointF> m_lastTouchPositions;
    QPointer<QWindow> m_windowUnderPointer;
    QPointer<QWindow> m_currentWindow;
    bool m_needsEnterOnPointerUpdate = false;
};

QT_END_NAMESPACE

#endif // QWINDOWSPOINTERHANDLER_H
