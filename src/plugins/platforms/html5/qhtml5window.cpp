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
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/QOpenGLContext>

#include "qhtml5window.h"
#include "qhtml5screen.h"
#include "qhtml5compositor.h"

#include <QDebug>

#include <iostream>

//#include "qhtml5compositor.h"

QT_BEGIN_NAMESPACE

//static QHtml5Window *globalHtml5Window;
//QHtml5Window *QHtml5Window::get() { return globalHtml5Window; }

QHtml5Window::QHtml5Window(QWindow *w, QHtml5Compositor* compositor)
    : QPlatformWindow(w),
      mWindow(w),
      mWindowState(Qt::WindowNoState),
      firstRun(true),
      mCompositor(compositor),
      m_raster(false)
{
    //globalHtml5Window = this;
    static int serialNo = 0;
    m_winid  = ++serialNo;
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglWindow %p: %p 0x%x\n", this, w, uint(m_winid));
#endif

//    m_raster = (w->surfaceType() == QSurface::RasterSurface);
//    if (m_raster)
    w->setSurfaceType(QSurface::OpenGLSurface);

    mCompositor->addWindow(this);
}

QHtml5Window::~QHtml5Window()
{
    mCompositor->removeWindow(this);
}

QHTML5Screen *QHtml5Window::platformScreen() const
{
    return static_cast<QHTML5Screen *>(window()->screen()->handle());
}

void QHtml5Window::setGeometry(const QRect &rect)
{
    //auto rect = rect2;
    //rect.setWidth(500);
    //rect.setHeight(500);
    mOldGeometry = geometry();

    QRect screenRect = rect;
    if ((rect.width() != 0 && rect.height() != 0) && firstRun) {
        //we want to show full screen at first, but resize to browser window for now
        //screenRect = screen()->availableGeometry();
        screenRect.setWidth(500);
        screenRect.setHeight(500);
        firstRun = false;
    }
    QWindowSystemInterface::handleGeometryChange(window(), screenRect);
    QPlatformWindow::setGeometry(screenRect);

    if (mOldGeometry != screenRect)
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(100, 100), geometry().size()));

    QWindowSystemInterface::flushWindowSystemEvents();
    invalidate();
}

void QHtml5Window::setVisible(bool visible)
{
    QRect newGeom;
    //QHTML5Screen *html5Screen = platformScreen();

    if (visible) {
        bool convOk = false;
        static bool envDisableForceFullScreen = qEnvironmentVariableIntValue("QT_QPA_HTML5_FORCE_FULLSCREEN", &convOk) == 0 && convOk;

        const bool forceFullScreen = !envDisableForceFullScreen && mCompositor->windowCount() == 0;

        if (forceFullScreen || (mWindowState & Qt::WindowFullScreen))
            newGeom = platformScreen()->geometry();
        else if (mWindowState & Qt::WindowMaximized)
            newGeom = platformScreen()->availableGeometry();
    }

    QPlatformWindow::setVisible(visible);

    mCompositor->setVisible(this, visible);

    /*
    if (visible)
        mCompositor->addWindow(this);
    else
        mCompositor->removeWindow(this);
    */

    if (!newGeom.isEmpty())
        setGeometry(newGeom); // may or may not generate an expose

//    if (newGeom.isEmpty() || newGeom == mOldGeometry) {
//        // QWindow::isExposed() maps to QWindow::visible() by default so simply
//        // generating an expose event regardless of this being a show or hide is
//        // just what is needed here.
    QWindowSystemInterface::handleExposeEvent(window(), visible ? QRect(QPoint(), geometry().size()) : QRect());
    QWindowSystemInterface::flushWindowSystemEvents();
    invalidate();
}

QMargins QHtml5Window::frameMargins() const
{
    QApplication *app = static_cast<QApplication*>(QApplication::instance());
    QStyle *style = app->style();
    int border = style->pixelMetric(QStyle::PM_MDIFrameWidth);
    int titleHeight = style->pixelMetric(QStyle::PM_TitleBarHeight, nullptr, nullptr);

    QMargins margins;
    margins.setLeft(border);
    margins.setRight(border);
    margins.setTop(2*border + titleHeight);
    margins.setBottom(border);

    return margins;
}

void QHtml5Window::raise()
{
    mCompositor->raise(this);
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), geometry().size()));
    invalidate();
}

void QHtml5Window::lower()
{
    mCompositor->lower(this);
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), geometry().size()));
    invalidate();
}

WId QHtml5Window::winId() const
{
    return m_winid;
}

void QHtml5Window::propagateSizeHints()
{
// get rid of base class warning
}

void QHtml5Window::invalidate()
{
    mCompositor->requestRedraw();
}

QT_END_NAMESPACE
