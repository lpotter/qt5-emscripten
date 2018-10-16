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

#include "qoffscreenwindow.h"
#include "qoffscreencommon.h"

#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include <private/qwindow_p.h>

QT_BEGIN_NAMESPACE

QOffscreenWindow::QOffscreenWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_positionIncludesFrame(false)
    , m_visible(false)
    , m_pendingGeometryChangeOnShow(true)
{
    if (window->windowState() == Qt::WindowNoState)
        setGeometry(window->geometry());
    else
        setWindowState(window->windowStates());

    QWindowSystemInterface::flushWindowSystemEvents();

    static WId counter = 0;
    m_winId = ++counter;

    m_windowForWinIdHash[m_winId] = this;
}

QOffscreenWindow::~QOffscreenWindow()
{
    if (QOffscreenScreen::windowContainingCursor == this)
        QOffscreenScreen::windowContainingCursor = 0;
    m_windowForWinIdHash.remove(m_winId);
}

void QOffscreenWindow::setGeometry(const QRect &rect)
{
    if (window()->windowState() != Qt::WindowNoState)
        return;

    m_positionIncludesFrame = qt_window_private(window())->positionPolicy == QWindowPrivate::WindowFrameInclusive;

    setFrameMarginsEnabled(true);
    setGeometryImpl(rect);

    m_normalGeometry = geometry();
}

void QOffscreenWindow::setGeometryImpl(const QRect &rect)
{
    QRect adjusted = rect;
    if (adjusted.width() <= 0)
        adjusted.setWidth(1);
    if (adjusted.height() <= 0)
        adjusted.setHeight(1);

    if (m_positionIncludesFrame) {
        adjusted.translate(m_margins.left(), m_margins.top());
    } else {
        // make sure we're not placed off-screen
        if (adjusted.left() < m_margins.left())
            adjusted.translate(m_margins.left(), 0);
        if (adjusted.top() < m_margins.top())
            adjusted.translate(0, m_margins.top());
    }

    QPlatformWindow::setGeometry(adjusted);

    if (m_visible) {
        QWindowSystemInterface::handleGeometryChange(window(), adjusted);
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), adjusted.size()));
    } else {
        m_pendingGeometryChangeOnShow = true;
    }
}

void QOffscreenWindow::setVisible(bool visible)
{
    if (visible == m_visible)
        return;

    if (visible) {
        if (window()->type() != Qt::ToolTip)
            QWindowSystemInterface::handleWindowActivated(window());

        if (m_pendingGeometryChangeOnShow) {
            m_pendingGeometryChangeOnShow = false;
            QWindowSystemInterface::handleGeometryChange(window(), geometry());
        }
    }

    if (visible) {
        QRect rect(QPoint(), geometry().size());
        QWindowSystemInterface::handleExposeEvent(window(), rect);
    } else {
        QWindowSystemInterface::handleExposeEvent(window(), QRegion());
    }

    m_visible = visible;
}

void QOffscreenWindow::requestActivateWindow()
{
    if (m_visible)
        QWindowSystemInterface::handleWindowActivated(window());
}

WId QOffscreenWindow::winId() const
{
    return m_winId;
}

QMargins QOffscreenWindow::frameMargins() const
{
    return m_margins;
}

void QOffscreenWindow::setFrameMarginsEnabled(bool enabled)
{
    if (enabled
        && !(window()->flags() & Qt::FramelessWindowHint)
        && (parent() == nullptr)) {
        m_margins = QMargins(2, 2, 2, 2);
    } else {
        m_margins = QMargins(0, 0, 0, 0);
    }
}

void QOffscreenWindow::setWindowState(Qt::WindowStates state)
{
    setFrameMarginsEnabled(!(state & Qt::WindowFullScreen));
    m_positionIncludesFrame = false;

    if (state & Qt::WindowMinimized)
        ; // nothing to do
    else if (state & Qt::WindowFullScreen)
        setGeometryImpl(screen()->geometry());
    else if (state & Qt::WindowMaximized)
        setGeometryImpl(screen()->availableGeometry().adjusted(m_margins.left(), m_margins.top(), -m_margins.right(), -m_margins.bottom()));
    else
        setGeometryImpl(m_normalGeometry);

    QWindowSystemInterface::handleWindowStateChanged(window(), state);
}

QOffscreenWindow *QOffscreenWindow::windowForWinId(WId id)
{
    return m_windowForWinIdHash.value(id, 0);
}

QHash<WId, QOffscreenWindow *> QOffscreenWindow::m_windowForWinIdHash;

QT_END_NAMESPACE
