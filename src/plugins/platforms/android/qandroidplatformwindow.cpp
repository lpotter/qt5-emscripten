/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
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

#include "qandroidplatformwindow.h"
#include "qandroidplatformopenglcontext.h"
#include "qandroidplatformscreen.h"

#include "androidjnimain.h"

#include <qguiapplication.h>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformWindow::QAndroidPlatformWindow(QWindow *window)
    : QPlatformWindow(window)
{
    m_windowFlags = Qt::Widget;
    m_windowState = Qt::WindowNoState;
    static QAtomicInt winIdGenerator(1);
    m_windowId = winIdGenerator.fetchAndAddRelaxed(1);
    setWindowState(window->windowStates());
}

void QAndroidPlatformWindow::lower()
{
    platformScreen()->lower(this);
}

void QAndroidPlatformWindow::raise()
{
    updateStatusBarVisibility();
    platformScreen()->raise(this);
}

void QAndroidPlatformWindow::setGeometry(const QRect &rect)
{
    QWindowSystemInterface::handleGeometryChange(window(), rect);
}

void QAndroidPlatformWindow::setVisible(bool visible)
{
    if (visible)
        updateStatusBarVisibility();

    if (visible) {
        if (m_windowState & Qt::WindowFullScreen)
            setGeometry(platformScreen()->geometry());
        else if (m_windowState & Qt::WindowMaximized)
            setGeometry(platformScreen()->availableGeometry());
    }

    if (visible)
        platformScreen()->addWindow(this);
    else
        platformScreen()->removeWindow(this);

    QRect availableGeometry = screen()->availableGeometry();
    if (geometry().width() > 0 && geometry().height() > 0 && availableGeometry.width() > 0 && availableGeometry.height() > 0)
        QPlatformWindow::setVisible(visible);
}

void QAndroidPlatformWindow::setWindowState(Qt::WindowStates state)
{
    if (m_windowState == state)
        return;

    QPlatformWindow::setWindowState(state);
    m_windowState = state;

    if (window()->isVisible())
        updateStatusBarVisibility();
}

void QAndroidPlatformWindow::setWindowFlags(Qt::WindowFlags flags)
{
    if (m_windowFlags == flags)
        return;

    m_windowFlags = flags;
}

Qt::WindowFlags QAndroidPlatformWindow::windowFlags() const
{
    return m_windowFlags;
}

void QAndroidPlatformWindow::setParent(const QPlatformWindow *window)
{
    Q_UNUSED(window);
}

QAndroidPlatformScreen *QAndroidPlatformWindow::platformScreen() const
{
    return static_cast<QAndroidPlatformScreen *>(window()->screen()->handle());
}

void QAndroidPlatformWindow::propagateSizeHints()
{
    //shut up warning from default implementation
}

void QAndroidPlatformWindow::requestActivateWindow()
{
    platformScreen()->topWindowChanged(window());
}

void QAndroidPlatformWindow::updateStatusBarVisibility()
{
    Qt::WindowFlags flags = window()->flags();
    bool isNonRegularWindow = flags & (Qt::Popup | Qt::Dialog | Qt::Sheet) & ~Qt::Window;
    if (!isNonRegularWindow) {
        if (m_windowState & Qt::WindowFullScreen)
            QtAndroid::hideStatusBar();
        else
            QtAndroid::showStatusBar();
    }
}

bool QAndroidPlatformWindow::isExposed() const
{
    return qApp->applicationState() > Qt::ApplicationHidden
            && window()->isVisible()
            && !window()->geometry().isEmpty();
}

void QAndroidPlatformWindow::applicationStateChanged(Qt::ApplicationState)
{
    QRegion region;
    if (isExposed())
        region = QRect(QPoint(), geometry().size());

    QWindowSystemInterface::handleExposeEvent(window(), region);
    QWindowSystemInterface::flushWindowSystemEvents();
}

QT_END_NAMESPACE
