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

#if defined(WINVER) && WINVER < 0x0601
#  undef WINVER
#endif
#if !defined(WINVER)
#  define WINVER 0x0601 // Enable touch functions for MinGW
#endif

#include "qwindowswindow.h"
#include "qwindowscontext.h"
#if QT_CONFIG(draganddrop)
#  include "qwindowsdrag.h"
#endif
#include "qwindowsscreen.h"
#include "qwindowsintegration.h"
#include "qwindowsmenu.h"
#include "qwindowsnativeinterface.h"
#if QT_CONFIG(dynamicgl)
#  include "qwindowsglcontext.h"
#else
#  include "qwindowsopenglcontext.h"
#endif
#include "qwindowsopengltester.h"
#ifdef QT_NO_CURSOR
#  include "qwindowscursor.h"
#endif

#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <QtGui/qregion.h>
#include <QtGui/qopenglcontext.h>
#include <private/qsystemlibrary_p.h>
#include <private/qwindow_p.h> // QWINDOWSIZE_MAX
#include <private/qguiapplication_p.h>
#include <private/qhighdpiscaling_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtCore/qdebug.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qoperatingsystemversion.h>

#include <dwmapi.h>

#if QT_CONFIG(vulkan)
#include "qwindowsvulkaninstance.h"
#endif

QT_BEGIN_NAMESPACE

typedef QSharedPointer<QWindowCreationContext> QWindowCreationContextPtr;

enum {
    defaultWindowWidth = 160,
    defaultWindowHeight = 160
};

Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &);

static QByteArray debugWinStyle(DWORD style)
{
    QByteArray rc = "0x";
    rc += QByteArray::number(qulonglong(style), 16);
    if (style & WS_POPUP)
        rc += " WS_POPUP";
    if (style & WS_CHILD)
        rc += " WS_CHILD";
    if (style & WS_OVERLAPPED)
        rc += " WS_OVERLAPPED";
    if (style & WS_CLIPSIBLINGS)
        rc += " WS_CLIPSIBLINGS";
    if (style & WS_CLIPCHILDREN)
        rc += " WS_CLIPCHILDREN";
    if (style & WS_THICKFRAME)
        rc += " WS_THICKFRAME";
    if (style & WS_DLGFRAME)
        rc += " WS_DLGFRAME";
    if (style & WS_SYSMENU)
        rc += " WS_SYSMENU";
    if (style & WS_MINIMIZEBOX)
        rc += " WS_MINIMIZEBOX";
    if (style & WS_MAXIMIZEBOX)
        rc += " WS_MAXIMIZEBOX";
    return rc;
}

static QByteArray debugWinExStyle(DWORD exStyle)
{
    QByteArray rc = "0x";
    rc += QByteArray::number(qulonglong(exStyle), 16);
    if (exStyle & WS_EX_TOOLWINDOW)
        rc += " WS_EX_TOOLWINDOW";
    if (exStyle & WS_EX_CONTEXTHELP)
        rc += " WS_EX_CONTEXTHELP";
    if (exStyle & WS_EX_LAYERED)
        rc += " WS_EX_LAYERED";
    if (exStyle & WS_EX_DLGMODALFRAME)
        rc += " WS_EX_DLGMODALFRAME";
    return rc;
}

static QByteArray debugWinSwpPos(UINT flags)
{
    QByteArray rc = "0x";
    rc += QByteArray::number(flags, 16);
    if (flags & SWP_FRAMECHANGED)
        rc += " SWP_FRAMECHANGED";
    if (flags & SWP_HIDEWINDOW)
        rc += " SWP_HIDEWINDOW";
    if (flags & SWP_NOACTIVATE)
        rc += " SWP_NOACTIVATE";
    if (flags & SWP_NOCOPYBITS)
        rc += " SWP_NOCOPYBITS";
    if (flags & SWP_NOMOVE)
        rc += " SWP_NOMOVE";
    if (flags & SWP_NOOWNERZORDER)
        rc += " SWP_NOOWNERZORDER";
    if (flags & SWP_NOREDRAW)
        rc += " SWP_NOREDRAW";
    if (flags & SWP_NOSENDCHANGING)
        rc += " SWP_NOSENDCHANGING";
    if (flags & SWP_NOSIZE)
        rc += " SWP_NOSIZE";
    if (flags & SWP_NOZORDER)
        rc += " SWP_NOZORDER";
    if (flags & SWP_SHOWWINDOW)
        rc += " SWP_SHOWWINDOW";
    return rc;
}

static inline QSize qSizeOfRect(const RECT &rect)
{
    return QSize(rect.right -rect.left, rect.bottom - rect.top);
}

static inline QRect qrectFromRECT(const RECT &rect)
{
    return QRect(QPoint(rect.left, rect.top), qSizeOfRect(rect));
}

static inline RECT RECTfromQRect(const QRect &rect)
{
    const int x = rect.left();
    const int y = rect.top();
    RECT result = { x, y, x + rect.width(), y + rect.height() };
    return result;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const RECT &r)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "RECT(left=" << r.left << ", top=" << r.top
        << ", right=" << r.right << ", bottom=" << r.bottom
        << " (" << r.right - r.left << 'x' << r.bottom - r.top << "))";
    return d;
}

QDebug operator<<(QDebug d, const POINT &p)
{
    d << p.x << ',' << p.y;
    return d;
}

QDebug operator<<(QDebug d, const WINDOWPOS &wp)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d <<  "WINDOWPOS(flags=" << debugWinSwpPos(wp.flags) << ", hwnd="
       << wp.hwnd << ", hwndInsertAfter=" << wp.hwndInsertAfter << ", x=" << wp.x
       <<  ", y=" << wp.y << ", cx=" << wp.cx <<  ", cy=" << wp.cy << ')';
    return d;
}

QDebug operator<<(QDebug d, const NCCALCSIZE_PARAMS &p)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "NCCALCSIZE_PARAMS(rgrc=[" << p.rgrc[0] << ' ' << p.rgrc[1] << ' '
        << p.rgrc[2] << "], lppos=" << *p.lppos << ')';
    return d;
}

QDebug operator<<(QDebug d, const MINMAXINFO &i)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "MINMAXINFO maxSize=" << i.ptMaxSize.x << ','
        << i.ptMaxSize.y << " maxpos=" << i.ptMaxPosition.x
        << ',' << i.ptMaxPosition.y << " mintrack="
        << i.ptMinTrackSize.x << ',' << i.ptMinTrackSize.y
        << " maxtrack=" << i.ptMaxTrackSize.x << ',' << i.ptMaxTrackSize.y;
    return d;
}

QDebug operator<<(QDebug d, const WINDOWPLACEMENT &wp)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d <<  "WINDOWPLACEMENT(flags=0x" << hex << wp.flags << dec << ", showCmd="
        << wp.showCmd << ", ptMinPosition=" << wp.ptMinPosition << ", ptMaxPosition=" << wp.ptMaxPosition
        << ", rcNormalPosition=" << wp.rcNormalPosition;
    return d;
}

QDebug operator<<(QDebug d, const GUID &guid)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << '{' << hex << uppercasedigits << qSetPadChar(QLatin1Char('0'))
      << qSetFieldWidth(8) << guid.Data1
      << qSetFieldWidth(0) << '-' << qSetFieldWidth(4)
      << guid.Data2 << qSetFieldWidth(0) << '-' << qSetFieldWidth(4)
      << guid.Data3 << qSetFieldWidth(0) << '-' << qSetFieldWidth(4)
      << qSetFieldWidth(2) << guid.Data4[0] << guid.Data4[1]
      << qSetFieldWidth(0) << '-' << qSetFieldWidth(2);
    for (int i = 2; i < 8; ++i)
        d << guid.Data4[i];
    d << qSetFieldWidth(0) << '}';
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

// QTBUG-43872, for windows that do not have WS_EX_TOOLWINDOW set, WINDOWPLACEMENT
// is in workspace/available area coordinates.
static QPoint windowPlacementOffset(HWND hwnd, const QPoint &point)
{
    if (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
        return QPoint(0, 0);
    const QWindowsScreenManager &screenManager = QWindowsContext::instance()->screenManager();
    const QWindowsScreen *screen = screenManager.screens().size() == 1
        ? screenManager.screens().constFirst() : screenManager.screenAtDp(point);
    if (screen)
        return screen->availableGeometry().topLeft() - screen->geometry().topLeft();
    return QPoint(0, 0);
}

// Return the frame geometry relative to the parent
// if there is one.
static inline QRect frameGeometry(HWND hwnd, bool topLevel)
{
    RECT rect = { 0, 0, 0, 0 };
    if (topLevel) {
        WINDOWPLACEMENT windowPlacement;
        windowPlacement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd, &windowPlacement);
        if (windowPlacement.showCmd == SW_SHOWMINIMIZED) {
            const QRect result = qrectFromRECT(windowPlacement.rcNormalPosition);
            return result.translated(windowPlacementOffset(hwnd, result.topLeft()));
        }
    }
    GetWindowRect(hwnd, &rect); // Screen coordinates.
    const HWND parent = GetParent(hwnd);
    if (parent && !topLevel) {
        const int width = rect.right - rect.left;
        const int height = rect.bottom - rect.top;
        POINT leftTop = { rect.left, rect.top };
        ScreenToClient(parent, &leftTop);
        rect.left = leftTop.x;
        rect.top = leftTop.y;
        rect.right = leftTop.x + width;
        rect.bottom = leftTop.y + height;
    }
    return qrectFromRECT(rect);
}

// Return the visibility of the Window (except full screen since it is not a window state).
static QWindow::Visibility windowVisibility_sys(HWND hwnd)
{
    if (!IsWindowVisible(hwnd))
        return QWindow::Hidden;
    WINDOWPLACEMENT windowPlacement;
    windowPlacement.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &windowPlacement)) {
        switch (windowPlacement.showCmd) {
        case SW_SHOWMINIMIZED:
        case SW_MINIMIZE:
        case SW_FORCEMINIMIZE:
            return QWindow::Minimized;
        case SW_SHOWMAXIMIZED:
            return QWindow::Maximized;
        default:
            break;
        }
    }
    return QWindow::Windowed;
}

static inline bool windowIsAccelerated(const QWindow *w)
{
    switch (w->surfaceType()) {
    case QSurface::OpenGLSurface:
        return true;
    case QSurface::RasterGLSurface:
        return qt_window_private(const_cast<QWindow *>(w))->compositing;
    case QSurface::VulkanSurface:
        return true;
    default:
        return false;
    }
}

static bool applyBlurBehindWindow(HWND hwnd)
{
    BOOL compositionEnabled;
    if (DwmIsCompositionEnabled(&compositionEnabled) != S_OK)
        return false;

    DWM_BLURBEHIND blurBehind = {0, 0, 0, 0};

    if (compositionEnabled) {
        blurBehind.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
        blurBehind.fEnable = TRUE;
        blurBehind.hRgnBlur = CreateRectRgn(0, 0, -1, -1);
    } else {
        blurBehind.dwFlags = DWM_BB_ENABLE;
        blurBehind.fEnable = FALSE;
    }

    const bool result = DwmEnableBlurBehindWindow(hwnd, &blurBehind) == S_OK;

    if (blurBehind.hRgnBlur)
        DeleteObject(blurBehind.hRgnBlur);

    return result;
}

// from qwidget_win.cpp, pass flags separately in case they have been "autofixed".
static bool shouldShowMaximizeButton(const QWindow *w, Qt::WindowFlags flags)
{
    if ((flags & Qt::MSWindowsFixedSizeDialogHint) || !(flags & Qt::WindowMaximizeButtonHint))
        return false;
    // if the user explicitly asked for the maximize button, we try to add
    // it even if the window has fixed size.
    return (flags & Qt::CustomizeWindowHint) ||
        w->maximumSize() == QSize(QWINDOWSIZE_MAX, QWINDOWSIZE_MAX);
}

// Set the WS_EX_LAYERED flag on a HWND if required. This is required for
// translucent backgrounds, not fully opaque windows and for
// Qt::WindowTransparentForInput (in combination with WS_EX_TRANSPARENT).
bool QWindowsWindow::setWindowLayered(HWND hwnd, Qt::WindowFlags flags, bool hasAlpha, qreal opacity)
{
    const LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    const bool needsLayered = (flags & Qt::WindowTransparentForInput)
        || (hasAlpha && (flags & Qt::FramelessWindowHint)) || opacity < 1.0;
    const bool isLayered = (exStyle & WS_EX_LAYERED);
    if (needsLayered != isLayered) {
        if (needsLayered) {
            SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
        } else {
            SetWindowLong(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
        }
    }
    return needsLayered;
}

static void setWindowOpacity(HWND hwnd, Qt::WindowFlags flags, bool hasAlpha, bool accelerated, qreal level)
{
    if (QWindowsWindow::setWindowLayered(hwnd, flags, hasAlpha, level)) {
        const BYTE alpha = BYTE(qRound(255.0 * level));
        if (hasAlpha && !accelerated && (flags & Qt::FramelessWindowHint)) {
            // Non-GL windows with alpha: Use blend function to update.
            BLENDFUNCTION blend = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
            UpdateLayeredWindow(hwnd, NULL, NULL, NULL, NULL, NULL, 0, &blend, ULW_ALPHA);
        } else {
            SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
        }
    } else if (IsWindowVisible(hwnd)) { // Repaint when switching from layered.
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

static inline void updateGLWindowSettings(const QWindow *w, HWND hwnd, Qt::WindowFlags flags, qreal opacity)
{
    const bool isAccelerated = windowIsAccelerated(w);
    const bool hasAlpha = w->format().hasAlpha();

    if (isAccelerated && hasAlpha)
        applyBlurBehindWindow(hwnd);

    setWindowOpacity(hwnd, flags, hasAlpha, isAccelerated, opacity);
}

/*!
    Calculates the dimensions of the invisible borders within the
    window frames in Windows 10, using an empirical expression that
    reproduces the measured values for standard DPI settings.
*/

static QMargins invisibleMargins(QPoint screenPoint)
{
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10) {
        POINT pt = {screenPoint.x(), screenPoint.y()};
        if (HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL)) {
            if (QWindowsContext::shcoredll.isValid()) {
                UINT dpiX;
                UINT dpiY;
                if (SUCCEEDED(QWindowsContext::shcoredll.getDpiForMonitor(hMonitor, 0, &dpiX, &dpiY))) {
                    const qreal sc = (dpiX - 96) / 96.0;
                    const int gap = 7 + qRound(5*sc) - int(sc);
                    return QMargins(gap, 0, gap, gap);
                }
            }
        }
    }
    return QMargins();
}

/*!
    \class WindowCreationData
    \brief Window creation code.

    This struct gathers all information required to create a window.
    Window creation is split in 3 steps:

    \list
    \li fromWindow() Gather all required information
    \li create() Create the system handle.
    \li initialize() Post creation initialization steps.
    \endlist

    The reason for this split is to also enable changing the QWindowFlags
    by calling:

    \list
    \li fromWindow() Gather information and determine new system styles
    \li applyWindowFlags() to apply the new window system styles.
    \li initialize() Post creation initialization steps.
    \endlist

    Contains the window creation code formerly in qwidget_win.cpp.

    \sa QWindowCreationContext
    \internal
    \ingroup qt-lighthouse-win
*/

struct WindowCreationData
{
    typedef QWindowsWindowData WindowData;
    enum Flags { ForceChild = 0x1, ForceTopLevel = 0x2 };

    WindowCreationData() : parentHandle(0), type(Qt::Widget), style(0), exStyle(0),
        topLevel(false), popup(false), dialog(false),
        tool(false), embedded(false), hasAlpha(false) {}

    void fromWindow(const QWindow *w, const Qt::WindowFlags flags, unsigned creationFlags = 0);
    inline WindowData create(const QWindow *w, const WindowData &data, QString title) const;
    inline void applyWindowFlags(HWND hwnd) const;
    void initialize(const QWindow *w, HWND h, bool frameChange, qreal opacityLevel) const;

    Qt::WindowFlags flags;
    HWND parentHandle;
    Qt::WindowType type;
    unsigned style;
    unsigned exStyle;
    bool topLevel;
    bool popup;
    bool dialog;
    bool tool;
    bool embedded;
    bool hasAlpha;
};

QDebug operator<<(QDebug debug, const WindowCreationData &d)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();
    debug << "WindowCreationData: " << d.flags
        << "\n  topLevel=" << d.topLevel;
     if (d.parentHandle)
         debug << " parent=" << d.parentHandle;
     debug << " popup=" << d.popup << " dialog=" << d.dialog
        << " embedded=" << d.embedded << " tool=" << d.tool
        << "\n  style=" << debugWinStyle(d.style);
    if (d.exStyle)
        debug << "\n  exStyle=" << debugWinExStyle(d.exStyle);
    return debug;
}

// Fix top level window flags in case only the type flags are passed.
static inline void fixTopLevelWindowFlags(Qt::WindowFlags &flags)
{
    // Not supported on Windows, also do correction when it is set.
    flags &= ~Qt::WindowFullscreenButtonHint;
    switch (flags) {
    case Qt::Window:
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint
              |Qt::WindowMaximizeButtonHint|Qt::WindowCloseButtonHint;
        break;
    case Qt::Dialog:
    case Qt::Tool:
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;
        break;
    default:
        break;
    }
    if ((flags & Qt::WindowType_Mask) == Qt::SplashScreen)
        flags |= Qt::FramelessWindowHint;
}

static QScreen *screenForName(const QWindow *w, const QString &name)
{
    QScreen *winScreen = w ? w->screen() : QGuiApplication::primaryScreen();
    if (winScreen && winScreen->name() != name) {
        const auto screens = winScreen->virtualSiblings();
        for (QScreen *screen : screens) {
            if (screen->name() == name)
                return screen;
        }
    }
    return winScreen;
}

static QScreen *forcedScreenForGLWindow(const QWindow *w)
{
    const QString forceToScreen = GpuDescription::detect().gpuSuitableScreen;
    return forceToScreen.isEmpty() ? nullptr : screenForName(w, forceToScreen);
}

static QPoint calcPosition(const QWindow *w, const QWindowCreationContextPtr &context, const QMargins &invMargins)
{
    const QPoint orgPos(context->frameX - invMargins.left(), context->frameY - invMargins.top());

    if (!w || (!w->isTopLevel() && w->surfaceType() != QWindow::OpenGLSurface))
        return orgPos;

    // Workaround for QTBUG-50371
    const QScreen *screenForGL = forcedScreenForGLWindow(w);
    if (!screenForGL)
        return orgPos;

    const QPoint posFrame(context->frameX, context->frameY);
    const QMargins margins = context->margins;
    const QRect scrGeo = screenForGL->handle()->availableGeometry();

    // Point is already in the required screen.
    if (scrGeo.contains(orgPos))
        return orgPos;

    // If the visible part of the window is already in the
    // required screen, just ignore the invisible offset.
    if (scrGeo.contains(posFrame))
        return posFrame;

    // Find the original screen containing the coordinates.
    const QList<QScreen *> screens = screenForGL->virtualSiblings();
    const QScreen *orgScreen = nullptr;
    for (QScreen *screen : screens) {
        if (screen->handle()->availableGeometry().contains(posFrame)) {
            orgScreen = screen;
            break;
        }
    }
    const QPoint ctPos = QPoint(qMax(scrGeo.left(), scrGeo.center().x()
                                     + (margins.right() - margins.left() - context->frameWidth)/2),
                                qMax(scrGeo.top(), scrGeo.center().y()
                                     + (margins.bottom() - margins.top() - context->frameHeight)/2));

    // If initial coordinates were outside all screens, center the window on the required screen.
    if (!orgScreen)
        return ctPos;

    const QRect orgGeo = orgScreen->handle()->availableGeometry();
    const QRect orgFrame(QPoint(context->frameX, context->frameY),
                         QSize(context->frameWidth, context->frameHeight));

    // Window would be centered on orgScreen. Center it on the required screen.
    if (orgGeo.center() == (orgFrame - margins).center())
        return ctPos;

    // Transform the coordinates to map them into the required screen.
    const QPoint newPos(scrGeo.left() + ((posFrame.x() - orgGeo.left()) * scrGeo.width()) / orgGeo.width(),
                        scrGeo.top() + ((posFrame.y() - orgGeo.top()) * scrGeo.height()) / orgGeo.height());
    const QPoint newPosNoMargin(newPos.x() - invMargins.left(), newPos.y() - invMargins.top());

    return scrGeo.contains(newPosNoMargin) ? newPosNoMargin : newPos;
}

void WindowCreationData::fromWindow(const QWindow *w, const Qt::WindowFlags flagsIn,
                                    unsigned creationFlags)
{
    flags = flagsIn;

    // Sometimes QWindow doesn't have a QWindow parent but does have a native parent window,
    // e.g. in case of embedded ActiveQt servers. They should not be considered a top-level
    // windows in such cases.
    QVariant prop = w->property(QWindowsWindow::embeddedNativeParentHandleProperty);
    if (prop.isValid()) {
        embedded = true;
        parentHandle = reinterpret_cast<HWND>(prop.value<WId>());
    }

    if (creationFlags & ForceChild) {
        topLevel = false;
    } else if (embedded) {
        // Embedded native windows (for example Active X server windows) are by
        // definition never toplevel, even though they do not have QWindow parents.
        topLevel = false;
    } else {
        topLevel = (creationFlags & ForceTopLevel) ? true : w->isTopLevel();
    }

    if (topLevel)
        fixTopLevelWindowFlags(flags);

    type = static_cast<Qt::WindowType>(int(flags) & Qt::WindowType_Mask);
    switch (type) {
    case Qt::Dialog:
    case Qt::Sheet:
        dialog = true;
        break;
    case Qt::Drawer:
    case Qt::Tool:
        tool = true;
        break;
    case Qt::Popup:
        popup = true;
        break;
    default:
        break;
    }
    if ((flags & Qt::MSWindowsFixedSizeDialogHint))
        dialog = true;

    // Parent: Use transient parent for top levels.
    if (popup) {
        flags |= Qt::WindowStaysOnTopHint; // a popup stays on top, no parent.
    } else if (!embedded) {
        if (const QWindow *parentWindow = topLevel ? w->transientParent() : w->parent())
            parentHandle = QWindowsWindow::handleOf(parentWindow);
    }

    if (popup || (type == Qt::ToolTip) || (type == Qt::SplashScreen)) {
        style = WS_POPUP;
    } else if (topLevel) {
        if (flags & Qt::FramelessWindowHint)
            style = WS_POPUP;                // no border
        else if (flags & Qt::WindowTitleHint)
            style = WS_OVERLAPPED;
        else
            style = 0;
    } else {
        style = WS_CHILD;
    }

        // if (!testAttribute(Qt::WA_PaintUnclipped))
        // ### Commented out for now as it causes some problems, but
        // this should be correct anyway, so dig some more into this
#ifdef Q_FLATTEN_EXPOSE
        if (windowIsOpenGL(w)) // a bit incorrect since the is-opengl status may change from false to true at any time later on
            style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN; // see SetPixelFormat
#else
        style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN ;
#endif
        if (topLevel) {
            if ((type == Qt::Window || dialog || tool)) {
                if (!(flags & Qt::FramelessWindowHint)) {
                    style |= WS_POPUP;
                    if (flags & Qt::MSWindowsFixedSizeDialogHint) {
                        style |= WS_DLGFRAME;
                    } else {
                        style |= WS_THICKFRAME;
                    }
                    if (flags & Qt::WindowTitleHint)
                        style |= WS_CAPTION; // Contains WS_DLGFRAME
                }
                if (flags & Qt::WindowSystemMenuHint)
                    style |= WS_SYSMENU;
                else if (dialog && (flags & Qt::WindowCloseButtonHint) && !(flags & Qt::FramelessWindowHint)) {
                    style |= WS_SYSMENU | WS_BORDER; // QTBUG-2027, dialogs without system menu.
                    exStyle |= WS_EX_DLGMODALFRAME;
                }
                if (flags & Qt::WindowMinimizeButtonHint)
                    style |= WS_MINIMIZEBOX;
                if (shouldShowMaximizeButton(w, flags))
                    style |= WS_MAXIMIZEBOX;
                if (tool)
                    exStyle |= WS_EX_TOOLWINDOW;
                if (flags & Qt::WindowContextHelpButtonHint)
                    exStyle |= WS_EX_CONTEXTHELP;
            } else {
                 exStyle |= WS_EX_TOOLWINDOW;
            }

            // make mouse events fall through this window
            // NOTE: WS_EX_TRANSPARENT flag can make mouse inputs fall through a layered window
            if (flagsIn & Qt::WindowTransparentForInput)
                exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
    }
}

QWindowsWindowData
    WindowCreationData::create(const QWindow *w, const WindowData &data, QString title) const
{
    WindowData result;
    result.flags = flags;

    const auto appinst = reinterpret_cast<HINSTANCE>(GetModuleHandle(nullptr));

    const QString windowClassName = QWindowsContext::instance()->registerWindowClass(w);

    const QRect rect = QPlatformWindow::initialGeometry(w, data.geometry, defaultWindowWidth, defaultWindowHeight);

    if (title.isEmpty() && (result.flags & Qt::WindowTitleHint))
        title = topLevel ? qAppName() : w->objectName();

    const wchar_t *titleUtf16 = reinterpret_cast<const wchar_t *>(title.utf16());
    const wchar_t *classNameUtf16 = reinterpret_cast<const wchar_t *>(windowClassName.utf16());

    // Capture events before CreateWindowEx() returns. The context is cleared in
    // the QWindowsWindow constructor.
    const QWindowCreationContextPtr context(new QWindowCreationContext(w, data.geometry, rect, data.customMargins, style, exStyle));
    QWindowsContext::instance()->setWindowCreationContext(context);

    QMargins invMargins = topLevel && !(result.flags & Qt::FramelessWindowHint) && QWindowsGeometryHint::positionIncludesFrame(w)
            ? invisibleMargins(QPoint(context->frameX, context->frameY)) : QMargins();

    qCDebug(lcQpaWindows).nospace()
        << "CreateWindowEx: " << w << " class=" << windowClassName << " title=" << title
        << '\n' << *this << "\nrequested: " << rect << ": "
        << context->frameWidth << 'x' <<  context->frameHeight
        << '+' << context->frameX << '+' << context->frameY
        << " custom margins: " << context->customMargins
        << " invisible margins: " << invMargins;


    QPoint pos = calcPosition(w, context, invMargins);

    result.hwnd = CreateWindowEx(exStyle, classNameUtf16, titleUtf16,
                                 style,
                                 pos.x(), pos.y(),
                                 context->frameWidth, context->frameHeight,
                                 parentHandle, NULL, appinst, NULL);
    qCDebug(lcQpaWindows).nospace()
        << "CreateWindowEx: returns " << w << ' ' << result.hwnd << " obtained geometry: "
        << context->obtainedGeometry << ' ' << context->margins;

    if (!result.hwnd) {
        qErrnoWarning("%s: CreateWindowEx failed", __FUNCTION__);
        return result;
    }

    result.geometry = context->obtainedGeometry;
    result.fullFrameMargins = context->margins;
    result.embedded = embedded;
    result.customMargins = context->customMargins;

    return result;
}

void WindowCreationData::applyWindowFlags(HWND hwnd) const
{
    // Keep enabled and visible from the current style.
    const LONG_PTR oldStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
    const LONG_PTR oldExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    const LONG_PTR newStyle = style | (oldStyle & (WS_DISABLED|WS_VISIBLE));
    if (oldStyle != newStyle)
        SetWindowLongPtr(hwnd, GWL_STYLE, newStyle);
    const LONG_PTR newExStyle = exStyle;
    if (newExStyle != oldExStyle)
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, newExStyle);
    qCDebug(lcQpaWindows).nospace() << __FUNCTION__ << hwnd << *this
        << "\n    Style from " << debugWinStyle(DWORD(oldStyle)) << "\n    to "
        << debugWinStyle(DWORD(newStyle)) << "\n    ExStyle from "
        << debugWinExStyle(DWORD(oldExStyle)) << " to "
        << debugWinExStyle(DWORD(newExStyle));
}

void WindowCreationData::initialize(const QWindow *w, HWND hwnd, bool frameChange, qreal opacityLevel) const
{
    if (!hwnd)
        return;
    UINT swpFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER;
    if (frameChange)
        swpFlags |= SWP_FRAMECHANGED;
    if (topLevel) {
        swpFlags |= SWP_NOACTIVATE;
        if ((flags & Qt::WindowStaysOnTopHint) || (type == Qt::ToolTip)) {
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, swpFlags);
            if (flags & Qt::WindowStaysOnBottomHint)
                qWarning("QWidget: Incompatible window flags: the window can't be on top and on bottom at the same time");
        } else if (flags & Qt::WindowStaysOnBottomHint) {
            SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, swpFlags);
        } else if (frameChange) { // Force WM_NCCALCSIZE with wParam=1 in case of custom margins.
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, swpFlags);
        }
        if (flags & (Qt::CustomizeWindowHint|Qt::WindowTitleHint)) {
            HMENU systemMenu = GetSystemMenu(hwnd, FALSE);
            if (flags & Qt::WindowCloseButtonHint)
                EnableMenuItem(systemMenu, SC_CLOSE, MF_BYCOMMAND|MF_ENABLED);
            else
                EnableMenuItem(systemMenu, SC_CLOSE, MF_BYCOMMAND|MF_GRAYED);
        }
        updateGLWindowSettings(w, hwnd, flags, opacityLevel);
    } else { // child.
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, swpFlags);
    }
}


// Scaling helpers for size constraints.
static QSize toNativeSizeConstrained(QSize dip, const QWindow *w)
{
    if (QHighDpiScaling::isActive()) {
        const qreal factor = QHighDpiScaling::factor(w);
        if (dip.width() > 0 && dip.width() < QWINDOWSIZE_MAX)
            dip.rwidth() *= factor;
        if (dip.height() > 0 && dip.height() < QWINDOWSIZE_MAX)
            dip.rheight() *= factor;
    }
    return dip;
}

/*!
    \class QWindowsGeometryHint
    \brief Stores geometry constraints and provides utility functions.

    Geometry constraints ready to apply to a MINMAXINFO taking frame
    into account.

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsGeometryHint::QWindowsGeometryHint(const QWindow *w, const QMargins &cm) :
     minimumSize(toNativeSizeConstrained(w->minimumSize(), w)),
     maximumSize(toNativeSizeConstrained(w->maximumSize(), w)),
     customMargins(cm)
{
}

bool QWindowsGeometryHint::validSize(const QSize &s) const
{
    const int width = s.width();
    const int height = s.height();
    return width >= minimumSize.width() && width <= maximumSize.width()
           && height >= minimumSize.height() && height <= maximumSize.height();
}

QMargins QWindowsGeometryHint::frame(DWORD style, DWORD exStyle)
{
    RECT rect = {0,0,0,0};
    style &= ~(WS_OVERLAPPED); // Not permitted, see docs.
    if (!AdjustWindowRectEx(&rect, style, FALSE, exStyle))
        qErrnoWarning("%s: AdjustWindowRectEx failed", __FUNCTION__);
    const QMargins result(qAbs(rect.left), qAbs(rect.top),
                          qAbs(rect.right), qAbs(rect.bottom));
    qCDebug(lcQpaWindows).nospace() << __FUNCTION__ << " style="
        << showbase << hex << style << " exStyle=" << exStyle << dec << noshowbase
        << ' ' << rect << ' ' << result;
    return result;
}

bool QWindowsGeometryHint::handleCalculateSize(const QMargins &customMargins, const MSG &msg, LRESULT *result)
{
    // NCCALCSIZE_PARAMS structure if wParam==TRUE
    if (!msg.wParam || customMargins.isNull())
        return false;
    *result = DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
    NCCALCSIZE_PARAMS *ncp = reinterpret_cast<NCCALCSIZE_PARAMS *>(msg.lParam);
    const RECT oldClientArea = ncp->rgrc[0];
    ncp->rgrc[0].left += customMargins.left();
    ncp->rgrc[0].top += customMargins.top();
    ncp->rgrc[0].right -= customMargins.right();
    ncp->rgrc[0].bottom -= customMargins.bottom();
    result = 0;
    qCDebug(lcQpaWindows).nospace() << __FUNCTION__ << oldClientArea << '+' << customMargins << "-->"
        << ncp->rgrc[0] << ' ' << ncp->rgrc[1] << ' ' << ncp->rgrc[2]
        << ' ' << ncp->lppos->cx << ',' << ncp->lppos->cy;
    return true;
}

void QWindowsGeometryHint::applyToMinMaxInfo(HWND hwnd, MINMAXINFO *mmi) const
{
    return applyToMinMaxInfo(DWORD(GetWindowLong(hwnd, GWL_STYLE)),
                             DWORD(GetWindowLong(hwnd, GWL_EXSTYLE)), mmi);
}

void QWindowsGeometryHint::applyToMinMaxInfo(DWORD style, DWORD exStyle, MINMAXINFO *mmi) const
{
    qCDebug(lcQpaWindows).nospace() << '>' << __FUNCTION__ << '<' << " min="
        << minimumSize.width() << ',' << minimumSize.height()
        << " max=" << maximumSize.width() << ',' << maximumSize.height()
        << " in " << *mmi;

    const QMargins margins = QWindowsGeometryHint::frame(style, exStyle);
    const int frameWidth = margins.left() + margins.right() + customMargins.left() + customMargins.right();
    const int frameHeight = margins.top() + margins.bottom() + customMargins.top() + customMargins.bottom();
    if (minimumSize.width() > 0)
        mmi->ptMinTrackSize.x = minimumSize.width() + frameWidth;
    if (minimumSize.height() > 0)
        mmi->ptMinTrackSize.y = minimumSize.height() + frameHeight;

    const int maximumWidth = qMax(maximumSize.width(), minimumSize.width());
    const int maximumHeight = qMax(maximumSize.height(), minimumSize.height());
    if (maximumWidth < QWINDOWSIZE_MAX)
        mmi->ptMaxTrackSize.x = maximumWidth + frameWidth;
    if (maximumHeight < QWINDOWSIZE_MAX)
        mmi->ptMaxTrackSize.y = maximumHeight + frameHeight;
    qCDebug(lcQpaWindows).nospace() << '<' << __FUNCTION__
        << " frame=" << margins << ' ' << frameWidth << ',' << frameHeight
        << " out " << *mmi;
}

bool QWindowsGeometryHint::positionIncludesFrame(const QWindow *w)
{
    return qt_window_private(const_cast<QWindow *>(w))->positionPolicy
           == QWindowPrivate::WindowFrameInclusive;
}

/*!
    \class QWindowsBaseWindow
    \brief Base class for QWindowsForeignWindow, QWindowsWindow

    The class provides some _sys() getters for querying window
    data from a HWND and some _sys() setters.

    Derived classes wrapping foreign windows may use them directly
    to calculate geometry, margins, etc.

    Derived classes representing windows created by Qt may defer
    expensive calculations until change notifications are received.

    \since 5.6
    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsBaseWindow *QWindowsBaseWindow::baseWindowOf(const QWindow *w)
{
    if (w) {
        if (QPlatformWindow *pw = w->handle())
            return static_cast<QWindowsBaseWindow *>(pw);
    }
    return nullptr;
}

HWND QWindowsBaseWindow::handleOf(const QWindow *w)
{
    const QWindowsBaseWindow *bw = QWindowsBaseWindow::baseWindowOf(w);
    return bw ? bw->handle() : HWND(0);
}

bool QWindowsBaseWindow::isTopLevel_sys() const
{
    const HWND parent = parentHwnd();
    return !parent || parent == GetDesktopWindow();
}

QRect QWindowsBaseWindow::frameGeometry_sys() const
{
    return frameGeometry(handle(), isTopLevel());
}

QRect QWindowsBaseWindow::geometry_sys() const
{
    return frameGeometry_sys().marginsRemoved(fullFrameMargins());
}

QMargins QWindowsBaseWindow::frameMargins_sys() const
{
    return QWindowsGeometryHint::frame(style(), exStyle());
}

void QWindowsBaseWindow::hide_sys() // Normal hide, do not activate other windows.
{
    SetWindowPos(handle(),0 , 0, 0, 0, 0,
                 SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void QWindowsBaseWindow::raise_sys()
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << this << window();
    const Qt::WindowType type = window()->type();
    if (type == Qt::Popup
        || type == Qt::SubWindow // Special case for QTBUG-63121: MDI subwindows with WindowStaysOnTopHint
        || !(window()->flags() & Qt::WindowStaysOnBottomHint)) {
        SetWindowPos(handle(), HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    }
}

void QWindowsBaseWindow::lower_sys()
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << this << window();
    if (!(window()->flags() & Qt::WindowStaysOnTopHint))
        SetWindowPos(handle(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

void QWindowsBaseWindow::setWindowTitle_sys(const QString &title)
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << this << window() << title;
    SetWindowText(handle(), reinterpret_cast<const wchar_t *>(title.utf16()));
}

QPoint QWindowsBaseWindow::mapToGlobal(const QPoint &pos) const
{
    return QWindowsGeometryHint::mapToGlobal(handle(), pos);
}

QPoint QWindowsBaseWindow::mapFromGlobal(const QPoint &pos) const
{
    return QWindowsGeometryHint::mapFromGlobal(handle(), pos);
}

/*!
    \class QWindowsDesktopWindow
    \brief Window wrapping GetDesktopWindow not allowing any manipulation.
    \since 5.6
    \internal
    \ingroup qt-lighthouse-win
*/

/*!
    \class QWindowsForeignWindow
    \brief Window wrapping a foreign native window.

    QWindowsForeignWindow stores a native HWND and implements getters for
    geometry, margins, etc. reparenting and geometry manipulation for use as a
    child window in Qt.

    \since 5.6
    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsForeignWindow::QWindowsForeignWindow(QWindow *window, HWND hwnd)
    : QWindowsBaseWindow(window)
    , m_hwnd(hwnd)
    , m_topLevelStyle(0)
{
}

void QWindowsForeignWindow::setParent(const QPlatformWindow *newParentWindow)
{
    const bool wasTopLevel = isTopLevel_sys();
    const HWND newParent = newParentWindow ? reinterpret_cast<HWND>(newParentWindow->winId()) : HWND(0);
    const bool isTopLevel = !newParent;
    const DWORD oldStyle = style();
    qCDebug(lcQpaWindows) << __FUNCTION__ << window() << "newParent="
        << newParentWindow << newParent << "oldStyle=" << debugWinStyle(oldStyle);
    SetParent(m_hwnd, newParent);
    if (wasTopLevel != isTopLevel) { // Top level window flags need to be set/cleared manually.
        DWORD newStyle = oldStyle;
        if (isTopLevel) {
            newStyle = m_topLevelStyle;
        } else {
            m_topLevelStyle = oldStyle;
            newStyle &= ~(WS_OVERLAPPEDWINDOW | WS_POPUPWINDOW);
            newStyle |= WS_CHILD;
        }
        SetWindowLongPtr(m_hwnd, GWL_STYLE, newStyle);
    }
}

void QWindowsForeignWindow::setVisible(bool visible)
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << window() << visible;
    if (visible)
        ShowWindow(handle(), SW_SHOWNOACTIVATE);
    else
        hide_sys();
}

/*!
    \class QWindowCreationContext
    \brief Active Context for creating windows.

    There is a phase in window creation (WindowCreationData::create())
    in which events are sent before the system API CreateWindowEx() returns
    the handle. These cannot be handled by the platform window as the association
    of the unknown handle value to the window does not exist yet and as not
    to trigger recursive handle creation, etc.

    In that phase, an instance of  QWindowCreationContext is set on
    QWindowsContext.

    QWindowCreationContext stores the information to answer the initial
    WM_GETMINMAXINFO and obtains the corrected size/position.

    \sa WindowCreationData, QWindowsContext
    \internal
    \ingroup qt-lighthouse-win
*/

QWindowCreationContext::QWindowCreationContext(const QWindow *w,
                                               const QRect &geometryIn, const QRect &geometry,
                                               const QMargins &cm,
                                               DWORD style_, DWORD exStyle_) :
    geometryHint(w, cm), window(w), style(style_), exStyle(exStyle_),
    requestedGeometryIn(geometryIn),
    requestedGeometry(geometry), obtainedGeometry(geometry),
    margins(QWindowsGeometryHint::frame(style, exStyle)), customMargins(cm)
{
    // Geometry of toplevels does not consider window frames.
    // TODO: No concept of WA_wasMoved yet that would indicate a
    // CW_USEDEFAULT unless set. For now, assume that 0,0 means 'default'
    // for toplevels.
    if (geometry.isValid()) {
        frameX = geometry.x();
        frameY = geometry.y();
        const QMargins effectiveMargins = margins + customMargins;
        frameWidth = effectiveMargins.left() + geometry.width() + effectiveMargins.right();
        frameHeight = effectiveMargins.top() + geometry.height() + effectiveMargins.bottom();
        if (QWindowsMenuBar::menuBarOf(w) != nullptr) {
            menuHeight = GetSystemMetrics(SM_CYMENU);
            frameHeight += menuHeight;
        }
        const bool isDefaultPosition = !frameX && !frameY && w->isTopLevel();
        if (!QWindowsGeometryHint::positionIncludesFrame(w) && !isDefaultPosition) {
            frameX -= effectiveMargins.left();
            frameY -= effectiveMargins.top();
        }
    }

    qCDebug(lcQpaWindows).nospace()
        << __FUNCTION__ << ' ' << w << ' ' << geometry
        << " pos incl. frame=" << QWindowsGeometryHint::positionIncludesFrame(w)
        << " frame=" << frameWidth << 'x' << frameHeight << '+'
        << frameX << '+' << frameY
        << " min=" << geometryHint.minimumSize << " max=" << geometryHint.maximumSize
        << " custom margins=" << customMargins;
}

/*!
    \class QWindowsWindow
    \brief Raster or OpenGL Window.

    \list
    \li Raster type: handleWmPaint() is implemented to
       to bitblt the image. The DC can be accessed
       via getDC/Relase DC, which has a special handling
       when within a paint event (in that case, the DC obtained
       from BeginPaint() is returned).

    \li Open GL: The first time QWindowsGLContext accesses
       the handle, it sets up the pixelformat on the DC
       which in turn sets it on the window (see flag
       PixelFormatInitialized).
       handleWmPaint() is empty (although required).
    \endlist

    \internal
    \ingroup qt-lighthouse-win
*/

const char *QWindowsWindow::embeddedNativeParentHandleProperty = "_q_embedded_native_parent_handle";
const char *QWindowsWindow::hasBorderInFullScreenProperty = "_q_has_border_in_fullscreen";

QWindowsWindow::QWindowsWindow(QWindow *aWindow, const QWindowsWindowData &data) :
    QWindowsBaseWindow(aWindow),
    m_data(data),
    m_cursor(new CursorHandle),
    m_format(aWindow->requestedFormat())
#if QT_CONFIG(vulkan)
  , m_vkSurface(0)
#endif
{
    QWindowsContext::instance()->addWindow(m_data.hwnd, this);
    const Qt::WindowType type = aWindow->type();
    if (type == Qt::Desktop)
        return; // No further handling for Qt::Desktop
#ifndef QT_NO_OPENGL
    if (aWindow->surfaceType() == QWindow::OpenGLSurface) {
        if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL)
            setFlag(OpenGLSurface);
        else
            setFlag(OpenGL_ES2);
    }
#endif // QT_NO_OPENGL
#if QT_CONFIG(vulkan)
    if (aWindow->surfaceType() == QSurface::VulkanSurface)
        setFlag(VulkanSurface);
#endif
    updateDropSite(window()->isTopLevel());

    registerTouchWindow();
    const qreal opacity = qt_window_private(aWindow)->opacity;
    if (!qFuzzyCompare(opacity, qreal(1.0)))
        setOpacity(opacity);

    setMask(QHighDpi::toNativeLocalRegion(window()->mask(), window()));

    if (aWindow->isTopLevel())
        setWindowIcon(aWindow->icon());
    if (aWindow->property(hasBorderInFullScreenProperty).toBool())
        setFlag(HasBorderInFullScreen);
    clearFlag(WithinCreate);
}

QWindowsWindow::~QWindowsWindow()
{
    setFlag(WithinDestroy);
    if (testFlag(TouchRegistered))
        UnregisterTouchWindow(m_data.hwnd);
    destroyWindow();
    destroyIcon();
}

void QWindowsWindow::initialize()
{
    // Clear the creation context as the window can be found in QWindowsContext's map.
    QWindowCreationContextPtr creationContext =
        QWindowsContext::instance()->setWindowCreationContext(QWindowCreationContextPtr());

    QWindow *w = window();
    setWindowState(w->windowStates());

    // Trigger geometry change (unless it has a special state in which case setWindowState()
    // will send the message) and screen change signals of QWindow.
    if (w->type() != Qt::Desktop) {
        const Qt::WindowState state = w->windowState();
        if (state != Qt::WindowMaximized && state != Qt::WindowFullScreen
            && creationContext->requestedGeometryIn != creationContext->obtainedGeometry) {
            QWindowSystemInterface::handleGeometryChange<QWindowSystemInterface::SynchronousDelivery>(w, creationContext->obtainedGeometry);
        }
        QPlatformScreen *obtainedScreen = screenForGeometry(creationContext->obtainedGeometry);
        if (obtainedScreen && screen() != obtainedScreen)
            QWindowSystemInterface::handleWindowScreenChanged<QWindowSystemInterface::SynchronousDelivery>(w, obtainedScreen->screen());
    }
}

void QWindowsWindow::fireExpose(const QRegion &region, bool force)
{
    if (region.isEmpty() && !force)
        clearFlag(Exposed);
    else
        setFlag(Exposed);
    QWindowSystemInterface::handleExposeEvent(window(), region);
}

void QWindowsWindow::destroyWindow()
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << this << window() << m_data.hwnd;
    if (m_data.hwnd) { // Stop event dispatching before Window is destroyed.
        setFlag(WithinDestroy);
        // Clear any transient child relationships as Windows will otherwise destroy them (QTBUG-35499, QTBUG-36666)
        const auto tlw = QGuiApplication::topLevelWindows();
        for (QWindow *w : tlw) {
            if (w->transientParent() == window()) {
                if (QWindowsWindow *tw = QWindowsWindow::windowsWindowOf(w))
                    tw->updateTransientParent();
            }
        }
        QWindowsContext *context = QWindowsContext::instance();
        if (context->windowUnderMouse() == window())
            context->clearWindowUnderMouse();
        if (hasMouseCapture())
            setMouseGrabEnabled(false);
        setDropSiteEnabled(false);
#if QT_CONFIG(vulkan)
        if (m_vkSurface) {
            QVulkanInstance *inst = window()->vulkanInstance();
            if (inst)
                static_cast<QWindowsVulkanInstance *>(inst->handle())->destroySurface(m_vkSurface);
            m_vkSurface = 0;
        }
#endif
#ifndef QT_NO_OPENGL
        if (m_surface) {
            if (QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext())
                staticOpenGLContext->destroyWindowSurface(m_surface);
            m_surface = 0;
        }
#endif
        DestroyWindow(m_data.hwnd);
        context->removeWindow(m_data.hwnd);
        m_data.hwnd = 0;
    }
}

void QWindowsWindow::updateDropSite(bool topLevel)
{
    bool enabled = false;
    bool parentIsEmbedded = false;

    if (!topLevel) {
        // if the parent window is a foreign window wrapped via QWindow::fromWinId, we need to enable the drop site
        // on the first child window
        const QWindow *parent = window()->parent();
        if (parent && parent->handle() && parent->handle()->isForeignWindow())
            parentIsEmbedded = true;
    }

    if (topLevel || parentIsEmbedded) {
        switch (window()->type()) {
        case Qt::Window:
        case Qt::Dialog:
        case Qt::Sheet:
        case Qt::Drawer:
        case Qt::Popup:
        case Qt::Tool:
            enabled = true;
            break;
        default:
            break;
        }
    }
    setDropSiteEnabled(enabled);
}

void QWindowsWindow::setDropSiteEnabled(bool dropEnabled)
{
    if (isDropSiteEnabled() == dropEnabled)
        return;
    qCDebug(lcQpaMime) << __FUNCTION__ << window() << dropEnabled;
#if QT_CONFIG(clipboard) && QT_CONFIG(draganddrop)
    if (dropEnabled) {
        Q_ASSERT(m_data.hwnd);
        m_dropTarget = new QWindowsOleDropTarget(window());
        RegisterDragDrop(m_data.hwnd, m_dropTarget);
        CoLockObjectExternal(m_dropTarget, true, true);
    } else {
        CoLockObjectExternal(m_dropTarget, false, true);
        m_dropTarget->Release();
        RevokeDragDrop(m_data.hwnd);
        m_dropTarget = 0;
    }
#endif // QT_CONFIG(clipboard) && QT_CONFIG(draganddrop)
}

// Returns topmost QWindowsWindow ancestor even if there are embedded windows in the chain.
// Returns this window if it is the topmost ancestor.
QWindow *QWindowsWindow::topLevelOf(QWindow *w)
{
    while (QWindow *parent = w->parent())
        w = parent;

    if (const QPlatformWindow *handle = w->handle()) {
        const QWindowsWindow *ww = static_cast<const QWindowsWindow *>(handle);
        if (ww->isEmbedded()) {
            HWND parentHWND = GetAncestor(ww->handle(), GA_PARENT);
            const HWND desktopHwnd = GetDesktopWindow();
            const QWindowsContext *ctx = QWindowsContext::instance();
            while (parentHWND && parentHWND != desktopHwnd) {
                if (QWindowsWindow *ancestor = ctx->findPlatformWindow(parentHWND))
                    return topLevelOf(ancestor->window());
                parentHWND = GetAncestor(parentHWND, GA_PARENT);
            }
        }
    }
    return w;
}

QWindowsWindowData
    QWindowsWindowData::create(const QWindow *w,
                                       const QWindowsWindowData &parameters,
                                       const QString &title)
{
    WindowCreationData creationData;
    creationData.fromWindow(w, parameters.flags);
    QWindowsWindowData result = creationData.create(w, parameters, title);
    // Force WM_NCCALCSIZE (with wParam=1) via SWP_FRAMECHANGED for custom margin.
    creationData.initialize(w, result.hwnd, !parameters.customMargins.isNull(), 1);
    return result;
}

void QWindowsWindow::setVisible(bool visible)
{
    const QWindow *win = window();
    qCDebug(lcQpaWindows) << __FUNCTION__ << this << win << m_data.hwnd << visible;
    if (m_data.hwnd) {
        if (visible) {
            show_sys();

            // When the window is layered, we won't get WM_PAINT, and "we" are in control
            // over the rendering of the window
            // There is nobody waiting for this, so we don't need to flush afterwards.
            if (isLayered())
                fireExpose(QRect(0, 0, win->width(), win->height()));
            // QTBUG-44928, QTBUG-7386: This is to resolve the problem where popups are
            // opened from the system tray and not being implicitly activated

            if (win->type() == Qt::Popup && !win->parent() && !QGuiApplication::focusWindow())
                SetForegroundWindow(m_data.hwnd);
        } else {
            if (hasMouseCapture())
                setMouseGrabEnabled(false);
            if (window()->flags() & Qt::Popup) // from QWidgetPrivate::hide_sys(), activate other
                ShowWindow(m_data.hwnd, SW_HIDE);
            else
                hide_sys();
            fireExpose(QRegion());
        }
    }
}

bool QWindowsWindow::isVisible() const
{
    return m_data.hwnd && IsWindowVisible(m_data.hwnd);
}

bool QWindowsWindow::isActive() const
{
    // Check for native windows or children of the active native window.
    if (const HWND activeHwnd = GetForegroundWindow())
        if (m_data.hwnd == activeHwnd || IsChild(activeHwnd, m_data.hwnd))
            return true;
    return false;
}

bool QWindowsWindow::isAncestorOf(const QPlatformWindow *child) const
{
    const QWindowsWindow *childWindow = static_cast<const QWindowsWindow *>(child);
    return IsChild(m_data.hwnd, childWindow->handle());
}

bool QWindowsWindow::isEmbedded() const
{
    return m_data.embedded;
}

QPoint QWindowsWindow::mapToGlobal(const QPoint &pos) const
{
    return m_data.hwnd ? QWindowsGeometryHint::mapToGlobal(m_data.hwnd, pos) : pos;
}

QPoint QWindowsWindow::mapFromGlobal(const QPoint &pos) const
{
    return m_data.hwnd ? QWindowsGeometryHint::mapFromGlobal(m_data.hwnd, pos) : pos;
}

// Update the transient parent for a toplevel window. The concept does not
// really exist on Windows, the relationship is set by passing a parent along with !WS_CHILD
// to window creation or by setting the parent using  GWL_HWNDPARENT (as opposed to
// SetParent, which would make it a real child).

#ifndef GWL_HWNDPARENT
#    define GWL_HWNDPARENT (-8)
#endif

void QWindowsWindow::updateTransientParent() const
{
    if (window()->type() == Qt::Popup)
        return; // QTBUG-34503, // a popup stays on top, no parent, see also WindowCreationData::fromWindow().
    // Update transient parent.
    const HWND oldTransientParent = GetWindow(m_data.hwnd, GW_OWNER);
    HWND newTransientParent = 0;
    if (const QWindow *tp = window()->transientParent())
        if (const QWindowsWindow *tw = QWindowsWindow::windowsWindowOf(tp))
            if (!tw->testFlag(WithinDestroy)) // Prevent destruction by parent window (QTBUG-35499, QTBUG-36666)
                newTransientParent = tw->handle();

    // QTSOLBUG-71: When using the MFC/winmigrate solution, it is possible that a child
    // window is found, which can cause issues with modality. Loop up to top level.
    while (newTransientParent && (GetWindowLongPtr(newTransientParent, GWL_STYLE) & WS_CHILD) != 0)
        newTransientParent = GetParent(newTransientParent);

    if (newTransientParent != oldTransientParent)
        SetWindowLongPtr(m_data.hwnd, GWL_HWNDPARENT, LONG_PTR(newTransientParent));
}

static inline bool testShowWithoutActivating(const QWindow *window)
{
    // QWidget-attribute Qt::WA_ShowWithoutActivating .
    const QVariant showWithoutActivating = window->property("_q_showWithoutActivating");
    return showWithoutActivating.isValid() && showWithoutActivating.toBool();
}

static void setMinimizedGeometry(HWND hwnd, const QRect &r)
{
    WINDOWPLACEMENT windowPlacement;
    windowPlacement.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &windowPlacement)) {
        windowPlacement.showCmd = SW_SHOWMINIMIZED;
        windowPlacement.rcNormalPosition = RECTfromQRect(r);
        SetWindowPlacement(hwnd, &windowPlacement);
    }
}

static void setRestoreMaximizedFlag(HWND hwnd, bool set = true)
{
    // Let Windows know that we need to restore as maximized
    WINDOWPLACEMENT windowPlacement;
    windowPlacement.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &windowPlacement)) {
        if (set)
            windowPlacement.flags |= WPF_RESTORETOMAXIMIZED;
        else
            windowPlacement.flags &= ~WPF_RESTORETOMAXIMIZED;
        SetWindowPlacement(hwnd, &windowPlacement);
    }
}

// partially from QWidgetPrivate::show_sys()
void QWindowsWindow::show_sys() const
{
    int sm = SW_SHOWNORMAL;
    bool fakedMaximize = false;
    bool restoreMaximize = false;
    const QWindow *w = window();
    const Qt::WindowFlags flags = w->flags();
    const Qt::WindowType type = w->type();
    if (w->isTopLevel()) {
        const Qt::WindowStates state = w->windowStates();
        if (state & Qt::WindowMinimized) {
            sm = SW_SHOWMINIMIZED;
            if (!isVisible())
                sm = SW_SHOWMINNOACTIVE;
            if (state & Qt::WindowMaximized)
                restoreMaximize = true;
        } else {
            updateTransientParent();
            if (state & Qt::WindowMaximized) {
                sm = SW_SHOWMAXIMIZED;
                // Windows will not behave correctly when we try to maximize a window which does not
                // have minimize nor maximize buttons in the window frame. Windows would then ignore
                // non-available geometry, and rather maximize the widget to the full screen, minus the
                // window frame (caption). So, we do a trick here, by adding a maximize button before
                // maximizing the widget, and then remove the maximize button afterwards.
                if (flags & Qt::WindowTitleHint &&
                        !(flags & (Qt::WindowMinMaxButtonsHint | Qt::FramelessWindowHint))) {
                    fakedMaximize = TRUE;
                    setStyle(style() | WS_MAXIMIZEBOX);
                }
            } // Qt::WindowMaximized
        } // !Qt::WindowMinimized
    }
    if (type == Qt::Popup || type == Qt::ToolTip || type == Qt::Tool || testShowWithoutActivating(w))
        sm = SW_SHOWNOACTIVATE;

    if (w->windowStates() & Qt::WindowMaximized)
        setFlag(WithinMaximize); // QTBUG-8361

    ShowWindow(m_data.hwnd, sm);

    clearFlag(WithinMaximize);

    if (fakedMaximize) {
        setStyle(style() & ~WS_MAXIMIZEBOX);
        SetWindowPos(m_data.hwnd, 0, 0, 0, 0, 0,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER
                     | SWP_FRAMECHANGED);
    }
    if (restoreMaximize)
        setRestoreMaximizedFlag(m_data.hwnd);
}

void QWindowsWindow::setParent(const QPlatformWindow *newParent)
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << window() << newParent;

    if (m_data.hwnd)
        setParent_sys(newParent);
}

void QWindowsWindow::setParent_sys(const QPlatformWindow *parent)
{
    // Use GetAncestor instead of GetParent, as GetParent can return owner window for toplevels
    HWND oldParentHWND = parentHwnd();
    HWND newParentHWND = 0;
    if (parent) {
        const QWindowsWindow *parentW = static_cast<const QWindowsWindow *>(parent);
        newParentHWND = parentW->handle();

    }

    // NULL handle means desktop window, which also has its proper handle -> disambiguate
    HWND desktopHwnd = GetDesktopWindow();
    if (oldParentHWND == desktopHwnd)
        oldParentHWND = 0;
    if (newParentHWND == desktopHwnd)
        newParentHWND = 0;

    if (newParentHWND != oldParentHWND) {
        const bool wasTopLevel = oldParentHWND == 0;
        const bool isTopLevel = newParentHWND == 0;

        setFlag(WithinSetParent);
        SetParent(m_data.hwnd, newParentHWND);
        clearFlag(WithinSetParent);

        // WS_CHILD/WS_POPUP must be manually set/cleared in addition
        // to dialog frames, etc (see  SetParent() ) if the top level state changes.
        // Force toplevel state as QWindow::isTopLevel cannot be relied upon here.
        if (wasTopLevel != isTopLevel) {
            setDropSiteEnabled(false);
            setWindowFlags_sys(window()->flags(), unsigned(isTopLevel ? WindowCreationData::ForceTopLevel : WindowCreationData::ForceChild));
            updateDropSite(isTopLevel);
        }
    }
}

void QWindowsWindow::handleHidden()
{
    fireExpose(QRegion());
}

void QWindowsWindow::handleCompositionSettingsChanged()
{
    const QWindow *w = window();
    if ((w->surfaceType() == QWindow::OpenGLSurface || w->surfaceType() == QWindow::VulkanSurface)
            && w->format().hasAlpha()) {
        applyBlurBehindWindow(handle());
    }
}

static QRect normalFrameGeometry(HWND hwnd)
{
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &wp)) {
        const QRect result = qrectFromRECT(wp.rcNormalPosition);
        return result.translated(windowPlacementOffset(hwnd, result.topLeft()));
    }
    return QRect();
}

QRect QWindowsWindow::normalGeometry() const
{
    // Check for fake 'fullscreen' mode.
    const bool fakeFullScreen =
        m_savedFrameGeometry.isValid() && (window()->windowStates() & Qt::WindowFullScreen);
    const QRect frame = fakeFullScreen ? m_savedFrameGeometry : normalFrameGeometry(m_data.hwnd);
    const QMargins margins = fakeFullScreen ? QWindowsGeometryHint::frame(m_savedStyle, 0) : fullFrameMargins();
    return frame.isValid() ? frame.marginsRemoved(margins) : frame;
}

void QWindowsWindow::setGeometry(const QRect &rectIn)
{
    QRect rect = rectIn;
    // This means it is a call from QWindow::setFramePosition() and
    // the coordinates include the frame (size is still the contents rectangle).
    if (QWindowsGeometryHint::positionIncludesFrame(window())) {
        const QMargins margins = frameMargins();
        rect.moveTopLeft(rect.topLeft() + QPoint(margins.left(), margins.top()));
    }
    if (m_windowState & Qt::WindowMinimized)
        m_data.geometry = rect; // Otherwise set by handleGeometryChange() triggered by event.
    if (m_data.hwnd) {
        // A ResizeEvent with resulting geometry will be sent. If we cannot
        // achieve that size (for example, window title minimal constraint),
        // notify and warn.
        setFlag(WithinSetGeometry);
        setGeometry_sys(rect);
        clearFlag(WithinSetGeometry);
        if (m_data.geometry != rect && (isVisible() || QLibraryInfo::isDebugBuild())) {
            qWarning("%s: Unable to set geometry %dx%d+%d+%d on %s/'%s'."
                     " Resulting geometry:  %dx%d+%d+%d "
                     "(frame: %d, %d, %d, %d, custom margin: %d, %d, %d, %d"
                     ", minimum size: %dx%d, maximum size: %dx%d).",
                     __FUNCTION__,
                     rect.width(), rect.height(), rect.x(), rect.y(),
                     window()->metaObject()->className(), qPrintable(window()->objectName()),
                     m_data.geometry.width(), m_data.geometry.height(),
                     m_data.geometry.x(), m_data.geometry.y(),
                     m_data.fullFrameMargins.left(), m_data.fullFrameMargins.top(),
                     m_data.fullFrameMargins.right(), m_data.fullFrameMargins.bottom(),
                     m_data.customMargins.left(), m_data.customMargins.top(),
                     m_data.customMargins.right(), m_data.customMargins.bottom(),
                     window()->minimumWidth(), window()->minimumHeight(),
                     window()->maximumWidth(), window()->maximumHeight());
        }
    } else {
        QPlatformWindow::setGeometry(rect);
    }
}

void QWindowsWindow::handleMoved()
{
    // Minimize/Set parent can send nonsensical move events.
    if (!IsIconic(m_data.hwnd) && !testFlag(WithinSetParent))
        handleGeometryChange();
}

void QWindowsWindow::handleResized(int wParam)
{
    switch (wParam) {
    case SIZE_MAXHIDE: // Some other window affected.
    case SIZE_MAXSHOW:
        return;
    case SIZE_MINIMIZED: // QTBUG-53577, prevent state change events during programmatic state change
        if (!testFlag(WithinSetStyle) && !testFlag(WithinSetGeometry))
            handleWindowStateChange(m_windowState | Qt::WindowMinimized);
        return;
    case SIZE_MAXIMIZED:
        if (!testFlag(WithinSetStyle) && !testFlag(WithinSetGeometry))
            handleWindowStateChange(Qt::WindowMaximized | (isFullScreen_sys() ? Qt::WindowFullScreen
                                                                              : Qt::WindowNoState));
        handleGeometryChange();
        break;
    case SIZE_RESTORED:
        if (!testFlag(WithinSetStyle) && !testFlag(WithinSetGeometry)) {
            if (isFullScreen_sys())
                handleWindowStateChange(
                    Qt::WindowFullScreen
                    | (testFlag(MaximizeToFullScreen) ? Qt::WindowMaximized : Qt::WindowNoState));
            else if (m_windowState != Qt::WindowNoState && !testFlag(MaximizeToFullScreen))
                handleWindowStateChange(Qt::WindowNoState);
        }
        handleGeometryChange();
        break;
    }
}

void QWindowsWindow::checkForScreenChanged()
{
    if (parent())
        return;

    QPlatformScreen *currentScreen = screen();
    const auto &screenManager = QWindowsContext::instance()->screenManager();
    // QTBUG-62971: When dragging a window by its border, detect by mouse position
    // to prevent it from oscillating between screens when it resizes
    const QWindowsScreen *newScreen = testFlag(ResizeMoveActive)
        ? screenManager.screenAtDp(QWindowsCursor::mousePosition())
        : screenManager.screenForHwnd(m_data.hwnd);
    if (newScreen != nullptr && newScreen != currentScreen) {
        qCDebug(lcQpaWindows).noquote().nospace() << __FUNCTION__
            << ' ' << window() << " \"" << currentScreen->name()
            << "\"->\"" << newScreen->name() << '"';
        QWindowSystemInterface::handleWindowScreenChanged(window(), newScreen->screen());
    }
}

void QWindowsWindow::handleGeometryChange()
{
    const QRect previousGeometry = m_data.geometry;
    m_data.geometry = geometry_sys();
    QWindowSystemInterface::handleGeometryChange(window(), m_data.geometry);
    // QTBUG-32121: OpenGL/normal windows (with exception of ANGLE) do not receive
    // expose events when shrinking, synthesize.
    if (!testFlag(OpenGL_ES2) && isExposed()
        && m_data.geometry.size() != previousGeometry.size() // Exclude plain move
        // One dimension grew -> Windows will send expose, no need to synthesize.
        && !(m_data.geometry.width() > previousGeometry.width() || m_data.geometry.height() > previousGeometry.height())) {
        fireExpose(QRect(QPoint(0, 0), m_data.geometry.size()), true);
    }

    checkForScreenChanged();

    if (testFlag(SynchronousGeometryChangeEvent))
        QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);

    qCDebug(lcQpaEvents) << __FUNCTION__ << this << window() << m_data.geometry;
}

void QWindowsBaseWindow::setGeometry_sys(const QRect &rect) const
{
    const QMargins margins = fullFrameMargins();
    const QRect frameGeometry = rect + margins;

    qCDebug(lcQpaWindows) << '>' << __FUNCTION__ << window()
        << "\n from " << geometry_sys() << " frame: "
        << margins << " to " <<rect
        << " new frame: " << frameGeometry;

    bool result = false;
    const HWND hwnd = handle();
    WINDOWPLACEMENT windowPlacement;
    windowPlacement.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &windowPlacement);
    // If the window is hidden and in maximized state or minimized, instead of moving the
    // window, set the normal position of the window.
    if ((windowPlacement.showCmd == SW_MAXIMIZE && !IsWindowVisible(hwnd))
        || windowPlacement.showCmd == SW_SHOWMINIMIZED) {
        windowPlacement.rcNormalPosition =
            RECTfromQRect(frameGeometry.translated(-windowPlacementOffset(hwnd, frameGeometry.topLeft())));
        windowPlacement.showCmd = windowPlacement.showCmd == SW_SHOWMINIMIZED ? SW_SHOWMINIMIZED : SW_HIDE;
        result = SetWindowPlacement(hwnd, &windowPlacement);
    } else {
        result = MoveWindow(hwnd, frameGeometry.x(), frameGeometry.y(),
                            frameGeometry.width(), frameGeometry.height(), true);
    }
    qCDebug(lcQpaWindows) << '<' << __FUNCTION__ << window()
        << "\n resulting " << result << geometry_sys();
}

/*!
    Allocates a HDC for the window or returns the temporary one
    obtained from WinAPI BeginPaint within a WM_PAINT event.

    \sa releaseDC()
*/

HDC QWindowsWindow::getDC()
{
    if (!m_hdc)
        m_hdc = GetDC(handle());
    return m_hdc;
}

/*!
    Relases the HDC for the window or does nothing in
    case it was obtained from WinAPI BeginPaint within a WM_PAINT event.

    \sa getDC()
*/

void QWindowsWindow::releaseDC()
{
    if (m_hdc) {
        ReleaseDC(handle(), m_hdc);
        m_hdc = 0;
    }
}

static inline bool dwmIsCompositionEnabled()
{
    BOOL dWmCompositionEnabled = FALSE;
    return SUCCEEDED(DwmIsCompositionEnabled(&dWmCompositionEnabled)) && dWmCompositionEnabled == TRUE;
}

static inline bool isSoftwareGl()
{
#if QT_CONFIG(dynamicgl)
    return QOpenGLStaticContext::opengl32.moduleIsNotOpengl32()
        && QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL;
#else
    return false;
#endif // dynamicgl
}

bool QWindowsWindow::handleWmPaint(HWND hwnd, UINT message,
                                         WPARAM, LPARAM)
{
    if (message == WM_ERASEBKGND) // Backing store - ignored.
        return true;
    // Ignore invalid update bounding rectangles
    RECT updateRect;
    if (!GetUpdateRect(m_data.hwnd, &updateRect, FALSE))
        return false;
    PAINTSTRUCT ps;

    // GL software rendering (QTBUG-58178) and Windows 7/Aero off with some AMD cards
    // (QTBUG-60527) need InvalidateRect() to suppress artifacts while resizing.
    if (testFlag(OpenGLSurface) && (isSoftwareGl() || !dwmIsCompositionEnabled()))
        InvalidateRect(hwnd, 0, false);

    BeginPaint(hwnd, &ps);

    // Observed painting problems with Aero style disabled (QTBUG-7865).
    if (Q_UNLIKELY(!dwmIsCompositionEnabled())
            && ((testFlag(OpenGLSurface) && testFlag(OpenGLDoubleBuffered)) || testFlag(VulkanSurface)))
    {
        SelectClipRgn(ps.hdc, NULL);
    }

    // If the a window is obscured by another window (such as a child window)
    // we still need to send isExposed=true, for compatibility.
    // Our tests depend on it.
    fireExpose(QRegion(qrectFromRECT(ps.rcPaint)), true);
    if (qSizeOfRect(updateRect) == m_data.geometry.size() && !QWindowsContext::instance()->asyncExpose())
        QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);

    EndPaint(hwnd, &ps);
    return true;
}

void QWindowsWindow::setWindowTitle(const QString &title)
{
    setWindowTitle_sys(QWindowsWindow::formatWindowTitle(title));
}

void QWindowsWindow::setWindowFlags(Qt::WindowFlags flags)
{
    qCDebug(lcQpaWindows) << '>' << __FUNCTION__ << this << window() << "\n    from: "
        << m_data.flags << "\n    to: " << flags;
    const QRect oldGeometry = geometry();
    if (m_data.flags != flags) {
        m_data.flags = flags;
        if (m_data.hwnd) {
            m_data = setWindowFlags_sys(flags);
            updateDropSite(window()->isTopLevel());
        }
    }
    // When switching to a frameless window, geometry
    // may change without a WM_MOVE. Report change manually.
    // Do not send synchronously as not to clobber the widget
    // geometry in a sequence of setting flags and geometry.
    const QRect newGeometry = geometry_sys();
    if (oldGeometry != newGeometry)
        handleGeometryChange();

    qCDebug(lcQpaWindows) << '<' << __FUNCTION__ << "\n    returns: "
        << m_data.flags << " geometry " << oldGeometry << "->" << newGeometry;
}

QWindowsWindowData QWindowsWindow::setWindowFlags_sys(Qt::WindowFlags wt,
                                                              unsigned flags) const
{
    WindowCreationData creationData;
    creationData.fromWindow(window(), wt, flags);
    creationData.applyWindowFlags(m_data.hwnd);
    creationData.initialize(window(), m_data.hwnd, true, m_opacity);

    QWindowsWindowData result = m_data;
    result.flags = creationData.flags;
    result.embedded = creationData.embedded;
    return result;
}

void QWindowsWindow::handleWindowStateChange(Qt::WindowStates state)
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << this << window()
                 << "\n    from " << m_windowState << " to " << state;
    m_windowState = state;
    QWindowSystemInterface::handleWindowStateChanged(window(), state);
    if (state & Qt::WindowMinimized) {
        handleHidden();
        QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents); // Tell QQuickWindow to stop rendering now.
    } else {
        // QTBUG-17548: We send expose events when receiving WM_Paint, but for
        // layered windows and transient children, we won't receive any WM_Paint.
        QWindow *w = window();
        bool exposeEventsSent = false;
        if (isLayered()) {
            fireExpose(QRegion(0, 0, w->width(), w->height()));
            exposeEventsSent = true;
        }
        const QWindowList allWindows = QGuiApplication::allWindows();
        for (QWindow *child : allWindows) {
            if (child != w && child->isVisible() && child->transientParent() == w) {
                QWindowsWindow *platformWindow = QWindowsWindow::windowsWindowOf(child);
                if (platformWindow && platformWindow->isLayered()) {
                    platformWindow->fireExpose(QRegion(0, 0, child->width(), child->height()));
                    exposeEventsSent = true;
                }
            }
        }
        if (exposeEventsSent && !QWindowsContext::instance()->asyncExpose())
            QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
    }
}

void QWindowsWindow::setWindowState(Qt::WindowStates state)
{
    if (m_data.hwnd) {
        setWindowState_sys(state);
        m_windowState = state;
    }
}

bool QWindowsWindow::isFullScreen_sys() const
{
    const QWindow *w = window();
    if (!w->isTopLevel())
        return false;
    QRect geometry = geometry_sys();
    if (testFlag(HasBorderInFullScreen))
        geometry += QMargins(1, 1, 1, 1);
    QPlatformScreen *screen = screenForGeometry(geometry);
    return screen && geometry == screen->geometry();
}

/*!
    \brief Change the window state.

    \note Window frames change when maximized;
    the top margin shrinks somewhat but that cannot be obtained using
    AdjustWindowRectEx().

    \note Some calls to SetWindowLong require a subsequent call
    to ShowWindow.
*/

void QWindowsWindow::setWindowState_sys(Qt::WindowStates newState)
{
    const Qt::WindowStates oldState = m_windowState;
    if (oldState == newState)
        return;
    qCDebug(lcQpaWindows) << '>' << __FUNCTION__ << this << window()
        << " from " << oldState << " to " << newState;

    const bool visible = isVisible();
    auto stateChange = oldState ^ newState;

    if (stateChange & Qt::WindowFullScreen) {
        if (newState & Qt::WindowFullScreen) {
#ifndef Q_FLATTEN_EXPOSE
            UINT newStyle = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;
#else
            UINT newStyle = WS_POPUP;
#endif
            // Save geometry and style to be restored when fullscreen
            // is turned off again, since on Windows, it is not a real
            // Window state but emulated by changing geometry and style.
            if (!m_savedStyle) {
                m_savedStyle = style();
                if ((oldState & Qt::WindowMinimized) || (oldState & Qt::WindowMaximized)) {
                    const QRect nf = normalFrameGeometry(m_data.hwnd);
                    if (nf.isValid())
                        m_savedFrameGeometry = nf;
                } else {
                    m_savedFrameGeometry = frameGeometry_sys();
                }
            }
            if (newState & Qt::WindowMaximized)
                setFlag(MaximizeToFullScreen);
            if (m_savedStyle & WS_SYSMENU)
                newStyle |= WS_SYSMENU;
            if (visible)
                newStyle |= WS_VISIBLE;
            if (testFlag(HasBorderInFullScreen))
                newStyle |= WS_BORDER;
            setStyle(newStyle);
            // Use geometry of QWindow::screen() within creation or the virtual screen the
            // window is in (QTBUG-31166, QTBUG-30724).
            const QScreen *screen = window()->screen();
            if (!screen)
                screen = QGuiApplication::primaryScreen();
            const QRect r = screen ? QHighDpi::toNativePixels(screen->geometry(), window()) : m_savedFrameGeometry;

            if (newState & Qt::WindowMinimized) {
                setMinimizedGeometry(m_data.hwnd, r);
                if (stateChange & Qt::WindowMaximized)
                    setRestoreMaximizedFlag(m_data.hwnd, newState & Qt::WindowMaximized);
            } else {
                const UINT swpf = SWP_FRAMECHANGED | SWP_NOACTIVATE;
                const bool wasSync = testFlag(SynchronousGeometryChangeEvent);
                setFlag(SynchronousGeometryChangeEvent);
                SetWindowPos(m_data.hwnd, HWND_TOP, r.left(), r.top(), r.width(), r.height(), swpf);
                if (!wasSync)
                    clearFlag(SynchronousGeometryChangeEvent);
                clearFlag(MaximizeToFullScreen);
                QWindowSystemInterface::handleGeometryChange(window(), r);
                QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
            }
        } else {
            // Restore saved state.
            unsigned newStyle = m_savedStyle ? m_savedStyle : style();
            if (visible)
                newStyle |= WS_VISIBLE;
            setStyle(newStyle);

            const QScreen *screen = window()->screen();
            if (!screen)
                screen = QGuiApplication::primaryScreen();
            // That area of the virtual desktop might not be covered by a screen anymore.
            if (!screen->geometry().intersects(m_savedFrameGeometry))
                m_savedFrameGeometry.moveTo(screen->geometry().topLeft());

            if (newState & Qt::WindowMinimized) {
                setMinimizedGeometry(m_data.hwnd, m_savedFrameGeometry);
                if (stateChange & Qt::WindowMaximized)
                    setRestoreMaximizedFlag(m_data.hwnd, newState & Qt::WindowMaximized);
            } else {
                UINT swpf = SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE;
                if (!m_savedFrameGeometry.isValid())
                    swpf |= SWP_NOSIZE | SWP_NOMOVE;
                const bool wasSync = testFlag(SynchronousGeometryChangeEvent);
                setFlag(SynchronousGeometryChangeEvent);
                // After maximized/fullscreen; the window can be in a maximized state. Clear
                // it before applying the normal geometry.
                if (windowVisibility_sys(m_data.hwnd) == QWindow::Maximized)
                    ShowWindow(m_data.hwnd, SW_SHOWNOACTIVATE);
                SetWindowPos(m_data.hwnd, 0, m_savedFrameGeometry.x(), m_savedFrameGeometry.y(),
                             m_savedFrameGeometry.width(), m_savedFrameGeometry.height(), swpf);
                if (!wasSync)
                    clearFlag(SynchronousGeometryChangeEvent);
                // preserve maximized state
                if (visible) {
                    setFlag(WithinMaximize);
                    ShowWindow(m_data.hwnd,
                               (newState & Qt::WindowMaximized) ? SW_MAXIMIZE : SW_SHOWNA);
                    clearFlag(WithinMaximize);
                }
            }
            m_savedStyle = 0;
            m_savedFrameGeometry = QRect();
        }
    } else if ((oldState & Qt::WindowMaximized) != (newState & Qt::WindowMaximized)) {
        if (visible && !(newState & Qt::WindowMinimized)) {
            setFlag(WithinMaximize);
            if (newState & Qt::WindowFullScreen)
                setFlag(MaximizeToFullScreen);
            ShowWindow(m_data.hwnd,
                       (newState & Qt::WindowMaximized) ? SW_MAXIMIZE : SW_SHOWNOACTIVATE);
            clearFlag(WithinMaximize);
            clearFlag(MaximizeToFullScreen);
        } else if (visible && (oldState & newState & Qt::WindowMinimized)) {
            // change of the maximized state while keeping minimized
            setRestoreMaximizedFlag(m_data.hwnd, newState & Qt::WindowMaximized);
        }
    }

    if (stateChange & Qt::WindowMinimized) {
        if (visible) {
            ShowWindow(m_data.hwnd,
                       (newState & Qt::WindowMinimized) ? SW_MINIMIZE :
                       (newState & Qt::WindowMaximized) ? SW_MAXIMIZE : SW_SHOWNORMAL);
            if ((newState & Qt::WindowMinimized) && (stateChange & Qt::WindowMaximized))
                setRestoreMaximizedFlag(m_data.hwnd, newState & Qt::WindowMaximized);
        }
    }
    qCDebug(lcQpaWindows) << '<' << __FUNCTION__ << this << window() << newState;
}

void QWindowsWindow::setStyle(unsigned s) const
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << this << window() << debugWinStyle(s);
    setFlag(WithinSetStyle);
    SetWindowLongPtr(m_data.hwnd, GWL_STYLE, s);
    clearFlag(WithinSetStyle);
}

void QWindowsWindow::setExStyle(unsigned s) const
{
    qCDebug(lcQpaWindows).nospace() << __FUNCTION__ << ' ' << this << ' ' << window()
        << " 0x" << QByteArray::number(s, 16);
    SetWindowLongPtr(m_data.hwnd, GWL_EXSTYLE, s);
}

bool QWindowsWindow::windowEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowBlocked: // Blocked by another modal window.
        setEnabled(false);
        setFlag(BlockedByModal);
        if (hasMouseCapture())
            ReleaseCapture();
        break;
    case QEvent::WindowUnblocked:
        setEnabled(true);
        clearFlag(BlockedByModal);
        break;
    default:
        break;
    }

    return QPlatformWindow::windowEvent(event);
}

void QWindowsWindow::propagateSizeHints()
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << this << window();
}

bool QWindowsWindow::handleGeometryChangingMessage(MSG *message, const QWindow *qWindow, const QMargins &margins)
{
    WINDOWPOS *windowPos = reinterpret_cast<WINDOWPOS *>(message->lParam);
    if ((windowPos->flags & SWP_NOZORDER) == 0) {
        if (QWindowsWindow *platformWindow = QWindowsWindow::windowsWindowOf(qWindow)) {
            QWindow *parentWindow = qWindow->parent();
            HWND parentHWND = GetAncestor(windowPos->hwnd, GA_PARENT);
            HWND desktopHWND = GetDesktopWindow();
            platformWindow->m_data.embedded = !parentWindow && parentHWND && (parentHWND != desktopHWND);
        }
        if (qWindow->flags().testFlag(Qt::WindowStaysOnBottomHint))
            windowPos->hwndInsertAfter = HWND_BOTTOM;
    }
    if (!qWindow->isTopLevel()) // Implement hasHeightForWidth().
        return false;
    if ((windowPos->flags & (SWP_NOCOPYBITS | SWP_NOSIZE)))
        return false;
    const QRect suggestedFrameGeometry(windowPos->x, windowPos->y,
                                       windowPos->cx, windowPos->cy);
    const QRect suggestedGeometry = suggestedFrameGeometry - margins;
    const QRectF correctedGeometryF = QPlatformWindow::closestAcceptableGeometry(qWindow, suggestedGeometry);
    if (!correctedGeometryF.isValid())
        return false;
    const QRect correctedFrameGeometry = correctedGeometryF.toRect() + margins;
    if (correctedFrameGeometry == suggestedFrameGeometry)
        return false;
    windowPos->x = correctedFrameGeometry.left();
    windowPos->y = correctedFrameGeometry.top();
    windowPos->cx = correctedFrameGeometry.width();
    windowPos->cy = correctedFrameGeometry.height();
    return true;
}

bool QWindowsWindow::handleGeometryChanging(MSG *message) const
{
    const QMargins margins = window()->isTopLevel() ? fullFrameMargins() : QMargins();
    return QWindowsWindow::handleGeometryChangingMessage(message, window(), margins);
}

void QWindowsWindow::setFullFrameMargins(const QMargins &newMargins)
{
    if (m_data.fullFrameMargins != newMargins) {
        qCDebug(lcQpaWindows) << __FUNCTION__ << window() <<  m_data.fullFrameMargins  << "->" << newMargins;
        m_data.fullFrameMargins = newMargins;
    }
}

QMargins QWindowsWindow::frameMargins() const
{
    QMargins result = fullFrameMargins();
    if (isTopLevel() && !(m_data.flags & Qt::FramelessWindowHint))
        result -= invisibleMargins(geometry().topLeft());
    return result;
}

QMargins QWindowsWindow::fullFrameMargins() const
{
    return m_data.fullFrameMargins;
}

void QWindowsWindow::setOpacity(qreal level)
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << level;
    if (!qFuzzyCompare(m_opacity, level)) {
        m_opacity = level;
        if (m_data.hwnd)
            setWindowOpacity(m_data.hwnd, m_data.flags,
                             window()->format().hasAlpha(), testFlag(OpenGLSurface) || testFlag(VulkanSurface),
                             level);
    }
}

static inline HRGN createRectRegion(const QRect &r)
{
    return CreateRectRgn(r.left(), r.top(), r.x() + r.width(), r.y() + r.height());
}

static inline void addRectToWinRegion(const QRect &rect, HRGN *winRegion)
{
    if (const HRGN rectRegion = createRectRegion(rect)) {
        HRGN result = CreateRectRgn(0, 0, 0, 0);
        if (CombineRgn(result, *winRegion, rectRegion, RGN_OR)) {
            DeleteObject(*winRegion);
            *winRegion = result;
        }
        DeleteObject(rectRegion);
    }
}

static HRGN qRegionToWinRegion(const QRegion &region)
{
    auto it = region.begin();
    const auto end = region.end();
    if (it == end)
        return nullptr;
    HRGN hRegion = createRectRegion(*it);
    while (++it != end)
        addRectToWinRegion(*it, &hRegion);
    return hRegion;
}

void QWindowsWindow::setMask(const QRegion &region)
{
    if (region.isEmpty()) {
         SetWindowRgn(m_data.hwnd, 0, true);
         return;
    }
    const HRGN winRegion = qRegionToWinRegion(region);

    // Mask is in client area coordinates, so offset it in case we have a frame
    if (window()->isTopLevel()) {
        const QMargins margins = fullFrameMargins();
        OffsetRgn(winRegion, margins.left(), margins.top());
    }

    // SetWindowRgn takes ownership.
    if (!SetWindowRgn(m_data.hwnd, winRegion, true))
        DeleteObject(winRegion);
}

void QWindowsWindow::requestActivateWindow()
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << this << window();
    // 'Active' state handling is based in focus since it needs to work for
    // child windows as well.
    if (m_data.hwnd) {
        const DWORD currentThread = GetCurrentThreadId();
        bool attached = false;
        DWORD foregroundThread = 0;

        // QTBUG-14062, QTBUG-37435: Windows normally only flashes the taskbar entry
        // when activating windows of inactive applications. Attach to the input of the
        // currently active window while setting the foreground window to always activate
        // the window when desired.
        if (QGuiApplication::applicationState() != Qt::ApplicationActive
            && QWindowsNativeInterface::windowActivationBehavior() == QWindowsWindowFunctions::AlwaysActivateWindow) {
            if (const HWND foregroundWindow = GetForegroundWindow()) {
                foregroundThread = GetWindowThreadProcessId(foregroundWindow, NULL);
                if (foregroundThread && foregroundThread != currentThread)
                    attached = AttachThreadInput(foregroundThread, currentThread, TRUE) == TRUE;
                if (attached) {
                    if (!window()->flags().testFlag(Qt::WindowStaysOnBottomHint)
                        && !window()->flags().testFlag(Qt::WindowStaysOnTopHint)
                        && window()->type() != Qt::ToolTip) {
                        const UINT swpFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER;
                        SetWindowPos(m_data.hwnd, HWND_TOPMOST, 0, 0, 0, 0, swpFlags);
                        SetWindowPos(m_data.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, swpFlags);
                    }
                }
            }
        }
        SetForegroundWindow(m_data.hwnd);
        SetFocus(m_data.hwnd);
        if (attached)
            AttachThreadInput(foregroundThread, currentThread, FALSE);
    }
}

bool QWindowsWindow::setKeyboardGrabEnabled(bool grab)
{
    if (!m_data.hwnd) {
        qWarning("%s: No handle", __FUNCTION__);
        return false;
    }
    qCDebug(lcQpaWindows) << __FUNCTION__ << this << window() << grab;

    QWindowsContext *context = QWindowsContext::instance();
    if (grab) {
        context->setKeyGrabber(window());
    } else {
        if (context->keyGrabber() == window())
            context->setKeyGrabber(0);
    }
    return true;
}

bool QWindowsWindow::setMouseGrabEnabled(bool grab)
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << window() << grab;
    if (!m_data.hwnd) {
        qWarning("%s: No handle", __FUNCTION__);
        return false;
    }
    if (!isVisible() && grab) {
        qWarning("%s: Not setting mouse grab for invisible window %s/'%s'",
                 __FUNCTION__, window()->metaObject()->className(),
                 qPrintable(window()->objectName()));
        return false;
    }
    // release grab or an explicit grab overriding autocapture: Clear flag.
    clearFlag(QWindowsWindow::AutoMouseCapture);
    if (hasMouseCapture() != grab) {
        if (grab) {
            SetCapture(m_data.hwnd);
        } else {
            ReleaseCapture();
        }
    }
    return grab;
}

static inline DWORD cornerToWinOrientation(Qt::Corner corner)
{
    switch (corner) {
    case Qt::TopLeftCorner:
        return 0xf004; // SZ_SIZETOPLEFT;
    case Qt::TopRightCorner:
        return 0xf005; // SZ_SIZETOPRIGHT
    case Qt::BottomLeftCorner:
        return 0xf007; // SZ_SIZEBOTTOMLEFT
    case Qt::BottomRightCorner:
        return 0xf008; // SZ_SIZEBOTTOMRIGHT
    }
    return 0;
}

bool QWindowsWindow::startSystemResize(const QPoint &, Qt::Corner corner)
{
    if (!GetSystemMenu(m_data.hwnd, FALSE))
        return false;

    ReleaseCapture();
    PostMessage(m_data.hwnd, WM_SYSCOMMAND, cornerToWinOrientation(corner), 0);
    setFlag(SizeGripOperation);
    return true;
}

bool QWindowsWindow::startSystemMove(const QPoint &)
{
    if (!GetSystemMenu(m_data.hwnd, FALSE))
        return false;

    ReleaseCapture();
    PostMessage(m_data.hwnd, WM_SYSCOMMAND, 0xF012 /*SC_DRAGMOVE*/, 0);
    return true;
}

void QWindowsWindow::setFrameStrutEventsEnabled(bool enabled)
{
    if (enabled) {
        setFlag(FrameStrutEventsEnabled);
    } else {
        clearFlag(FrameStrutEventsEnabled);
    }
}

void QWindowsWindow::getSizeHints(MINMAXINFO *mmi) const
{
    // We don't apply the min/max size hint as we change the dpi, because we did not adjust the
    // QScreen of the window yet so we don't have the min/max with the right ratio
    if (!testFlag(QWindowsWindow::WithinDpiChanged)) {
        const QWindowsGeometryHint hint(window(), m_data.customMargins);
        hint.applyToMinMaxInfo(m_data.hwnd, mmi);
    }

    if ((testFlag(WithinMaximize) || (window()->windowStates() & Qt::WindowMinimized))
            && (m_data.flags & Qt::FramelessWindowHint)) {
        // This block fixes QTBUG-8361: Frameless windows shouldn't cover the
        // taskbar when maximized
        const QScreen *screen = window()->screen();

        // Documentation of MINMAXINFO states that it will only work for the primary screen
        if (screen && screen == QGuiApplication::primaryScreen()) {
            const QRect availableGeometry = QHighDpi::toNativePixels(screen->availableGeometry(), screen);
            mmi->ptMaxSize.y = availableGeometry.height();

            // Width, because you can have the taskbar on the sides too.
            mmi->ptMaxSize.x = availableGeometry.width();

            // If you have the taskbar on top, or on the left you don't want it at (0,0):
            mmi->ptMaxPosition.x = availableGeometry.x();
            mmi->ptMaxPosition.y = availableGeometry.y();
        } else if (!screen){
            qWarning("window()->screen() returned a null screen");
        }
    }

    qCDebug(lcQpaWindows) << __FUNCTION__ << window() << *mmi;
}

bool QWindowsWindow::handleNonClientHitTest(const QPoint &globalPos, LRESULT *result) const
{
    // QTBUG-32663, suppress resize cursor for fixed size windows.
    const QWindow *w = window();
    if (!w->isTopLevel() // Task 105852, minimized windows need to respond to user input.
        || !(m_windowState & ~Qt::WindowActive)
        || (m_data.flags & Qt::FramelessWindowHint)) {
        return false;
    }
    const QSize minimumSize = w->minimumSize();
    if (minimumSize.isEmpty())
        return false;
    const QSize maximumSize = w->maximumSize();
    const bool fixedWidth = minimumSize.width() == maximumSize.width();
    const bool fixedHeight = minimumSize.height() == maximumSize.height();
    if (!fixedWidth && !fixedHeight)
        return false;
    const QPoint localPos = w->mapFromGlobal(QHighDpi::fromNativePixels(globalPos, w));
    const QSize size = w->size();
    if (fixedHeight) {
        if (localPos.y() >= size.height()) {
            *result = HTBORDER; // Unspecified border, no resize cursor.
            return true;
        }
        if (localPos.y() < 0) {
            const QMargins margins = frameMargins();
            const int topResizeBarPos = margins.left() - margins.top();
            if (localPos.y() < topResizeBarPos) {
                *result = HTCAPTION; // Extend caption over top resize bar, let's user move the window.
                return true;
            }
        }
    }
    if (fixedWidth && (localPos.x() < 0 || localPos.x() >= size.width())) {
        *result = HTBORDER; // Unspecified border, no resize cursor.
        return true;
    }
    return false;
}

#ifndef QT_NO_CURSOR
// Return the default cursor (Arrow) from QWindowsCursor's cache.
static inline CursorHandlePtr defaultCursor(const QWindow *w)
{
    if (QScreen *screen = w->screen())
        if (const QPlatformScreen *platformScreen = screen->handle())
            if (QPlatformCursor *cursor = platformScreen->cursor())
                return static_cast<QWindowsCursor *>(cursor)->standardWindowCursor(Qt::ArrowCursor);
    return CursorHandlePtr(new CursorHandle(QWindowsCursor::createCursorFromShape(Qt::ArrowCursor)));
}

// Check whether to apply a new cursor. Either the window in question is
// currently under mouse, or it is the parent of the window under mouse and
// there is no other window with an explicitly set cursor in-between.
static inline bool applyNewCursor(const QWindow *w)
{
    const QWindow *underMouse = QWindowsContext::instance()->windowUnderMouse();
    if (underMouse == w)
        return true;
    for (const QWindow *p = underMouse; p ; p = p->parent()) {
        if (p == w)
            return true;
        const QWindowsWindow *platformWindow = QWindowsWindow::windowsWindowOf(p);
        if (platformWindow && !platformWindow->cursor()->isNull())
            return false;
    }
    return false;
}
#endif // !QT_NO_CURSOR

/*!
    \brief Applies to cursor property set on the window to the global cursor.

    \sa QWindowsCursor
*/

void QWindowsWindow::applyCursor()
{
    if (QWindowsCursor::hasOverrideCursor()) {
        if (isTopLevel())
            QWindowsCursor::enforceOverrideCursor();
        return;
    }
#ifndef QT_NO_CURSOR
    if (m_cursor->isNull()) { // Recurse up to parent with non-null cursor. Set default for toplevel.
        if (const QWindow *p = window()->parent()) {
            if (QWindowsWindow *platformWindow = QWindowsWindow::windowsWindowOf(p))
                platformWindow->applyCursor();
        } else {
            SetCursor(defaultCursor(window())->handle());
        }
    } else {
        SetCursor(m_cursor->handle());
    }
#endif
}

void QWindowsWindow::setCursor(const CursorHandlePtr &c)
{
#ifndef QT_NO_CURSOR
    if (c->handle() != m_cursor->handle()) {
        const bool apply = applyNewCursor(window());
        qCDebug(lcQpaWindows) << window() << __FUNCTION__
            << c->handle() << " doApply=" << apply;
        m_cursor = c;
        if (apply)
            applyCursor();
    }
#endif
}

void QWindowsWindow::setAlertState(bool enabled)
{
    if (isAlertState() == enabled)
        return;
    if (enabled) {
        alertWindow(0);
        setFlag(AlertState);
    } else {
        stopAlertWindow();
        clearFlag(AlertState);
    }
}

void QWindowsWindow::alertWindow(int durationMs)
{
    UINT timeOutMs = GetCaretBlinkTime();
    if (!timeOutMs || timeOutMs == INFINITE)
        timeOutMs = 250;

    FLASHWINFO info;
    info.cbSize = sizeof(info);
    info.hwnd = m_data.hwnd;
    info.dwFlags = FLASHW_TRAY;
    info.dwTimeout = timeOutMs;
    info.uCount = durationMs == 0 ? 10 : UINT(durationMs) / timeOutMs;
    FlashWindowEx(&info);
}

void QWindowsWindow::stopAlertWindow()
{
    FLASHWINFO info;
    info.cbSize = sizeof(info);
    info.hwnd = m_data.hwnd;
    info.dwFlags = FLASHW_STOP;
    info.dwTimeout = 0;
    info.uCount = 0;
    FlashWindowEx(&info);
}

bool QWindowsWindow::isEnabled() const
{
    return (style() & WS_DISABLED) == 0;
}

void QWindowsWindow::setEnabled(bool enabled)
{
    const unsigned oldStyle = style();
    unsigned newStyle = oldStyle;
    if (enabled) {
        newStyle &= ~WS_DISABLED;
    } else {
        newStyle |= WS_DISABLED;
    }
    if (newStyle != oldStyle)
        setStyle(newStyle);
}

static HICON createHIcon(const QIcon &icon, int xSize, int ySize)
{
    if (!icon.isNull()) {
        const QPixmap pm = icon.pixmap(icon.actualSize(QSize(xSize, ySize)));
        if (!pm.isNull())
            return qt_pixmapToWinHICON(pm);
    }
    return 0;
}

void QWindowsWindow::setWindowIcon(const QIcon &icon)
{
    if (m_data.hwnd) {
        destroyIcon();

        m_iconSmall = createHIcon(icon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
        m_iconBig = createHIcon(icon, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));

        if (m_iconBig) {
            SendMessage(m_data.hwnd, WM_SETICON, 0 /* ICON_SMALL */, LPARAM(m_iconSmall));
            SendMessage(m_data.hwnd, WM_SETICON, 1 /* ICON_BIG */, LPARAM(m_iconBig));
        } else {
            SendMessage(m_data.hwnd, WM_SETICON, 0 /* ICON_SMALL */, LPARAM(m_iconSmall));
            SendMessage(m_data.hwnd, WM_SETICON, 1 /* ICON_BIG */, LPARAM(m_iconSmall));
        }
    }
}

bool QWindowsWindow::isTopLevel() const
{
    return window()->isTopLevel() && !m_data.embedded;
}

QWindowsMenuBar *QWindowsWindow::menuBar() const
{
    return m_menuBar.data();
}

void QWindowsWindow::setMenuBar(QWindowsMenuBar *mb)
{
    m_menuBar = mb;
}

/*!
    \brief Sets custom margins to be added to the default margins determined by
    the windows style in the handling of the WM_NCCALCSIZE message.

    This is currently used to give the Aero-style QWizard a smaller top margin.
    The property can be set using QPlatformNativeInterface::setWindowProperty() or,
    before platform window creation, by setting a dynamic property
    on the QWindow (see QWindowsIntegration::createPlatformWindow()).
*/

void QWindowsWindow::setCustomMargins(const QMargins &newCustomMargins)
{
    if (newCustomMargins != m_data.customMargins) {
        const QMargins oldCustomMargins = m_data.customMargins;
        m_data.customMargins = newCustomMargins;
         // Re-trigger WM_NCALCSIZE with wParam=1 by passing SWP_FRAMECHANGED
        const QRect currentFrameGeometry = frameGeometry_sys();
        const QPoint topLeft = currentFrameGeometry.topLeft();
        QRect newFrame = currentFrameGeometry.marginsRemoved(oldCustomMargins) + m_data.customMargins;
        newFrame.moveTo(topLeft);
        qCDebug(lcQpaWindows) << __FUNCTION__ << oldCustomMargins << "->" << newCustomMargins
            << currentFrameGeometry << "->" << newFrame;
        SetWindowPos(m_data.hwnd, 0, newFrame.x(), newFrame.y(), newFrame.width(), newFrame.height(), SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    }
}

void *QWindowsWindow::surface(void *nativeConfig, int *err)
{
#if QT_CONFIG(vulkan)
    Q_UNUSED(nativeConfig);
    Q_UNUSED(err);
    if (window()->surfaceType() == QSurface::VulkanSurface) {
        if (!m_vkSurface) {
            QVulkanInstance *inst = window()->vulkanInstance();
            if (inst)
                m_vkSurface = static_cast<QWindowsVulkanInstance *>(inst->handle())->createSurface(handle());
            else
                qWarning("Attempted to create Vulkan surface without an instance; was QWindow::setVulkanInstance() called?");
        }
        // Different semantics for VkSurfaces: the return value is the address,
        // not the value, given that this is a 64-bit handle even on x86.
        return &m_vkSurface;
    }
#elif defined(QT_NO_OPENGL)
    Q_UNUSED(err)
    Q_UNUSED(nativeConfig)
    return 0;
#endif
#ifndef QT_NO_OPENGL
    if (!m_surface) {
        if (QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext())
            m_surface = staticOpenGLContext->createWindowSurface(m_data.hwnd, nativeConfig, err);
    }

    return m_surface;
#endif
}

void QWindowsWindow::invalidateSurface()
{
#if QT_CONFIG(vulkan)
    if (m_vkSurface) {
        QVulkanInstance *inst = window()->vulkanInstance();
        if (inst)
            static_cast<QWindowsVulkanInstance *>(inst->handle())->destroySurface(m_vkSurface);
        m_vkSurface = 0;
    }
#endif
#ifndef QT_NO_OPENGL
    if (m_surface) {
        if (QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext())
            staticOpenGLContext->destroyWindowSurface(m_surface);
        m_surface = 0;
    }
#endif // QT_NO_OPENGL
}

void QWindowsWindow::setTouchWindowTouchTypeStatic(QWindow *window, QWindowsWindowFunctions::TouchWindowTouchTypes touchTypes)
{
    if (!window->handle())
        return;
    static_cast<QWindowsWindow *>(window->handle())->registerTouchWindow(touchTypes);
}

void QWindowsWindow::registerTouchWindow(QWindowsWindowFunctions::TouchWindowTouchTypes touchTypes)
{
    if ((QWindowsContext::instance()->systemInfo() & QWindowsContext::SI_SupportsTouch)
        && !testFlag(TouchRegistered)) {
        ULONG touchFlags = 0;
        const bool ret = IsTouchWindow(m_data.hwnd, &touchFlags);
        // Return if it is not a touch window or the flags are already set by a hook
        // such as HCBT_CREATEWND
        if (ret || touchFlags != 0)
            return;
        if (RegisterTouchWindow(m_data.hwnd, ULONG(touchTypes)))
            setFlag(TouchRegistered);
        else
            qErrnoWarning("RegisterTouchWindow() failed for window '%s'.", qPrintable(window()->objectName()));
    }
}

void QWindowsWindow::aboutToMakeCurrent()
{
#ifndef QT_NO_OPENGL
    // For RasterGLSurface windows, that become OpenGL windows dynamically, it might be
    // time to set up some GL specifics.  This is particularly important for layered
    // windows (WS_EX_LAYERED due to alpha > 0).
    const bool isCompositing = qt_window_private(window())->compositing;
    if (isCompositing != testFlag(Compositing)) {
        if (isCompositing)
            setFlag(Compositing);
        else
            clearFlag(Compositing);

        updateGLWindowSettings(window(), m_data.hwnd, m_data.flags, m_opacity);
    }
#endif
}

void QWindowsWindow::setHasBorderInFullScreenStatic(QWindow *window, bool border)
{
    if (QPlatformWindow *handle = window->handle())
        static_cast<QWindowsWindow *>(handle)->setHasBorderInFullScreen(border);
    else
        window->setProperty(hasBorderInFullScreenProperty, QVariant(border));
}

void QWindowsWindow::setHasBorderInFullScreen(bool border)
{
    if (testFlag(HasBorderInFullScreen) == border)
        return;
    if (border)
        setFlag(HasBorderInFullScreen);
    else
        clearFlag(HasBorderInFullScreen);
    // Directly apply the flag in case we are fullscreen.
    if (m_windowState == Qt::WindowFullScreen) {
        LONG_PTR style = GetWindowLongPtr(handle(), GWL_STYLE);
        if (border)
            style |= WS_BORDER;
        else
            style &= ~WS_BORDER;
        SetWindowLongPtr(handle(), GWL_STYLE, style);
    }
}

QString QWindowsWindow::formatWindowTitle(const QString &title)
{
    return QPlatformWindow::formatWindowTitle(title, QStringLiteral(" - "));
}

QT_END_NAMESPACE
