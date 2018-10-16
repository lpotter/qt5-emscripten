/***************************************************************************
**
** Copyright (C) 2011 - 2013 BlackBerry Limited. All rights reserved.
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

#include "qqnxglcontext.h"
#include "qqnxintegration.h"
#include "qqnxscreen.h"
#include "qqnxeglwindow.h"

#include "private/qeglconvenience_p.h"

#include <QtCore/QDebug>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>

#include <dlfcn.h>

#if defined(QQNXGLCONTEXT_DEBUG)
#define qGLContextDebug qDebug
#else
#define qGLContextDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

EGLDisplay QQnxGLContext::ms_eglDisplay = EGL_NO_DISPLAY;

static QEGLPlatformContext::Flags makeFlags()
{
    QEGLPlatformContext::Flags result = 0;

    if (!QQnxIntegration::instance()->options().testFlag(QQnxIntegration::SurfacelessEGLContext))
        result |= QEGLPlatformContext::NoSurfaceless;

    return result;
}

QQnxGLContext::QQnxGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share)
    : QEGLPlatformContext(format, share, ms_eglDisplay, 0, QVariant(), makeFlags())
{
}

QQnxGLContext::~QQnxGLContext()
{
}

void QQnxGLContext::initializeContext()
{
    qGLContextDebug();

    // Initialize connection to EGL
    ms_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (Q_UNLIKELY(ms_eglDisplay == EGL_NO_DISPLAY))
        qFatal("QQnxGLContext: failed to obtain EGL display: %x", eglGetError());

    EGLBoolean eglResult = eglInitialize(ms_eglDisplay, 0, 0);
    if (Q_UNLIKELY(eglResult != EGL_TRUE))
        qFatal("QQnxGLContext: failed to initialize EGL display, err=%d", eglGetError());
}

void QQnxGLContext::shutdownContext()
{
    qGLContextDebug();

    // Close connection to EGL
    eglTerminate(ms_eglDisplay);
}

EGLSurface QQnxGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    QQnxEglWindow *window = static_cast<QQnxEglWindow *>(surface);
    window->ensureInitialized(this);
    return window->surface();
}

bool QQnxGLContext::makeCurrent(QPlatformSurface *surface)
{
    qGLContextDebug();
    return QEGLPlatformContext::makeCurrent(surface);
}

void QQnxGLContext::swapBuffers(QPlatformSurface *surface)
{
    qGLContextDebug();

    QEGLPlatformContext::swapBuffers(surface);

    QQnxEglWindow *platformWindow = static_cast<QQnxEglWindow*>(surface);
    platformWindow->windowPosted();
}

void QQnxGLContext::doneCurrent()
{
    QEGLPlatformContext::doneCurrent();
}

QT_END_NAMESPACE
