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

#ifndef QWINDOWSSCREEN_H
#define QWINDOWSSCREEN_H

#include "qtwindowsglobal.h"

#include <QtCore/qlist.h>
#include <QtCore/qvector.h>
#include <QtCore/qpair.h>
#include <QtCore/qscopedpointer.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

struct QWindowsScreenData
{
    enum Flags
    {
        PrimaryScreen = 0x1,
        VirtualDesktop = 0x2,
        LockScreen = 0x4 // Temporary screen existing during user change, etc.
    };

    QRect geometry;
    QRect availableGeometry;
    QDpi dpi{96, 96};
    QSizeF physicalSizeMM;
    int depth = 32;
    QImage::Format format = QImage::Format_ARGB32_Premultiplied;
    unsigned flags = VirtualDesktop;
    QString name;
    Qt::ScreenOrientation orientation = Qt::LandscapeOrientation;
    qreal refreshRateHz = 60;
    HMONITOR hMonitor = nullptr;
};

class QWindowsScreen : public QPlatformScreen
{
public:
#ifndef QT_NO_CURSOR
    typedef QScopedPointer<QPlatformCursor> CursorPtr;
#endif

    explicit QWindowsScreen(const QWindowsScreenData &data);

    QRect geometry() const override { return m_data.geometry; }
    QRect availableGeometry() const override { return m_data.availableGeometry; }
    int depth() const override { return m_data.depth; }
    QImage::Format format() const override { return m_data.format; }
    QSizeF physicalSize() const override { return m_data.physicalSizeMM; }
    QDpi logicalDpi() const override { return m_data.dpi; }
    qreal pixelDensity() const override;
    qreal devicePixelRatio() const override { return 1.0; }
    qreal refreshRate() const override { return m_data.refreshRateHz; }
    QString name() const override { return m_data.name; }
    Qt::ScreenOrientation orientation() const override { return m_data.orientation; }
    QList<QPlatformScreen *> virtualSiblings() const override;
    QWindow *topLevelAt(const QPoint &point) const override;
    static QWindow *windowAt(const QPoint &point, unsigned flags);

    QPixmap grabWindow(WId window, int qX, int qY, int qWidth, int qHeight) const override;
    QPlatformScreen::SubpixelAntialiasingType subpixelAntialiasingTypeHint() const override;

    static Qt::ScreenOrientation orientationPreference();
    static bool setOrientationPreference(Qt::ScreenOrientation o);

    inline void handleChanges(const QWindowsScreenData &newData);

#ifndef QT_NO_CURSOR
    QPlatformCursor *cursor() const override { return m_cursor.data(); }
    const CursorPtr &cursorPtr() const { return m_cursor; }
#else
    QPlatformCursor *cursor() const               { return 0; }
#endif // !QT_NO_CURSOR

    const QWindowsScreenData &data() const  { return m_data; }

    static QRect virtualGeometry(const QPlatformScreen *screen);

private:
    QWindowsScreenData m_data;
#ifndef QT_NO_CURSOR
    const CursorPtr m_cursor;
#endif
};

class QWindowsScreenManager
{
public:
    typedef QList<QWindowsScreen *> WindowsScreenList;

    QWindowsScreenManager();

    void clearScreens();

    bool handleScreenChanges();
    bool handleDisplayChange(WPARAM wParam, LPARAM lParam);
    const WindowsScreenList &screens() const { return m_screens; }

    const QWindowsScreen *screenAtDp(const QPoint &p) const;
    const QWindowsScreen *screenForHwnd(HWND hwnd) const;

private:
    void removeScreen(int index);

    WindowsScreenList m_screens;
    int m_lastDepth = -1;
    WORD m_lastHorizontalResolution = 0;
    WORD m_lastVerticalResolution = 0;
};

QT_END_NAMESPACE

#endif // QWINDOWSSCREEN_H
