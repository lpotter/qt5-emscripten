/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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

#include "qqnxnativeinterface.h"

#if !defined(QT_NO_OPENGL)
#include "qqnxglcontext.h"
#endif

#include "qqnxscreen.h"
#include "qqnxwindow.h"
#if defined(QQNX_IMF)
#include "qqnxinputcontext_imf.h"
#endif

#include "qqnxintegration.h"

#if !defined(QT_NO_OPENGL)
#include <QtGui/QOpenGLContext>
#endif

#include <QtGui/QScreen>
#include <QtGui/QWindow>

QT_BEGIN_NAMESPACE

QQnxNativeInterface::QQnxNativeInterface(QQnxIntegration *integration)
    : m_integration(integration)
{
}

void *QQnxNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (resource == "windowGroup" && window && window->screen()) {
        QQnxScreen * const screen = static_cast<QQnxScreen *>(window->screen()->handle());
        if (screen) {
            screen_window_t screenWindow = reinterpret_cast<screen_window_t>(window->winId());
            QQnxWindow *qnxWindow = screen->findWindow(screenWindow);
            // We can't just call data() instead of constData() here, since that would detach
            // and the lifetime of the char * would not be long enough. Therefore the const_cast.
            return qnxWindow ? const_cast<char *>(qnxWindow->groupName().constData()) : 0;
        }
    }

    return 0;
}

void *QQnxNativeInterface::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    if (resource == "QObject*" && screen)
        return static_cast<QObject*>(static_cast<QQnxScreen*>(screen->handle()));

    return 0;
}

void *QQnxNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
    if (resource == "screenContext")
        return m_integration->screenContext();

    return 0;
}

#if !defined(QT_NO_OPENGL)
void *QQnxNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    if (resource == "eglcontext" && context)
        return static_cast<QQnxGLContext*>(context->handle())->eglContext();

    return 0;
}
#endif

void QQnxNativeInterface::setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value)
{
    QQnxWindow *qnxWindow = static_cast<QQnxWindow*>(window);

    if (name == QLatin1String("mmRendererWindowName")) {
        qnxWindow->setMMRendererWindowName(value.toString());
    } else if (name == QLatin1String("qnxWindowGroup")) {
        if (value.isNull())
            qnxWindow->joinWindowGroup(QByteArray());
        else if (value.canConvert<QByteArray>())
            qnxWindow->joinWindowGroup(value.toByteArray());
    }
}

QPlatformNativeInterface::NativeResourceForIntegrationFunction QQnxNativeInterface::nativeResourceFunctionForIntegration(const QByteArray &resource)
{
#if defined(QQNX_IMF)
    if (resource == "blackberryIMFSetHighlightColor")
        return reinterpret_cast<NativeResourceForIntegrationFunction>(QQnxInputContext::setHighlightColor);
    if (resource == "blackberryIMFCheckSpelling")
        return reinterpret_cast<NativeResourceForIntegrationFunction>(QQnxInputContext::checkSpelling);
#else
    Q_UNUSED(resource)
#endif
    return 0;
}

QT_END_NAMESPACE
