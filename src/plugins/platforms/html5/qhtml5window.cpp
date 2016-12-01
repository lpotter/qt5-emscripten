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

#include <qpa/qwindowsysteminterface.h>
#include <private/qguiapplication_p.h>
# include <QtGui/private/qopenglcontext_p.h>
# include <QtGui/QOpenGLContext>

#include "qhtml5window.h"
#include "qhtml5screen.h"

#include <QDebug>

//#include "qhtml5compositor.h"

QT_BEGIN_NAMESPACE

static QHTML5Window *globalHtml5Window;
QHTML5Window *QHTML5Window::get() { return globalHtml5Window; }

QHTML5Window::QHTML5Window(QWindow *w)
    : QPlatformWindow(w),
      firstRun(true),
      m_raster(false)
{
    globalHtml5Window = this;
    static int serialNo = 0;
    m_winid  = ++serialNo;
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglWindow %p: %p 0x%x\n", this, w, uint(m_winid));
#endif

//    m_raster = (w->surfaceType() == QSurface::RasterSurface);
//    if (m_raster)
        w->setSurfaceType(QSurface::OpenGLSurface);
}

QHTML5Window::~QHTML5Window()
{
}

QHTML5Screen *QHTML5Window::platformScreen() const
{
    return static_cast<QHTML5Screen *>(window()->screen()->handle());
}

void QHTML5Window::setGeometry(const QRect &rect)
{
    mOldGeometry = geometry();

    QRect screenRect = rect;
    if ((rect.width() != 0 && rect.height() != 0) && firstRun) {
        //we want to show full screen at first, but resize to browser window for now
        screenRect = screen()->availableGeometry();
         firstRun = false;
    }
    QWindowSystemInterface::handleGeometryChange(window(), screenRect);
    QPlatformWindow::setGeometry(screenRect);

    if (mOldGeometry != screenRect)
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), geometry().size()));

    QWindowSystemInterface::flushWindowSystemEvents();
}

void QHTML5Window::setVisible(bool visible)
{
    QRect newGeom;
    QHTML5Screen *html5Screen = platformScreen();

    if (visible) {
        bool convOk = false;
        static bool envDisableForceFullScreen = qEnvironmentVariableIntValue("QT_QPA_HTML5_FORCE_FULLSCREEN", &convOk) == 0 && convOk;

        const bool forceFullScreen = !envDisableForceFullScreen && html5Screen->windowCount() == 0;

        if (forceFullScreen || (mWindowState & Qt::WindowFullScreen))
            newGeom = platformScreen()->geometry();
        else if (mWindowState & Qt::WindowMaximized)
            newGeom = platformScreen()->availableGeometry();
    }

    QPlatformWindow::setVisible(visible);

    if (visible)
        html5Screen->addWindow(this);
    else
        html5Screen->removeWindow(this);

    if (!newGeom.isEmpty())
        setGeometry(newGeom); // may or may not generate an expose

//    if (newGeom.isEmpty() || newGeom == mOldGeometry) {
//        // QWindow::isExposed() maps to QWindow::visible() by default so simply
//        // generating an expose event regardless of this being a show or hide is
//        // just what is needed here.
    QWindowSystemInterface::handleExposeEvent(window(), visible ? QRect(QPoint(), geometry().size()) : QRect());
    QWindowSystemInterface::flushWindowSystemEvents();
}

void QHTML5Window::raise()
{
    platformScreen()->raise(this);
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), geometry().size()));
}

void QHTML5Window::lower()
{
    platformScreen()->lower(this);
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), geometry().size()));
}

WId QHTML5Window::winId() const
{
    return m_winid;
}

void QHTML5Window::propagateSizeHints()
{
// get rid of base class warning
}

QT_END_NAMESPACE
