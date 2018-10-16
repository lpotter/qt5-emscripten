/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#ifndef QQNXEGLWINDOW_H
#define QQNXEGLWINDOW_H

#include "qqnxwindow.h"
#include <QtCore/QMutex>

QT_BEGIN_NAMESPACE

class QQnxGLContext;

class QQnxEglWindow : public QQnxWindow
{
public:
    QQnxEglWindow(QWindow *window, screen_context_t context, bool needRootWindow);
    ~QQnxEglWindow();

    EGLSurface surface() const;

    bool isInitialized() const;
    void ensureInitialized(QQnxGLContext *context);

    void setGeometry(const QRect &rect) override;

    QSurfaceFormat format() const override { return m_format; }

protected:
    int pixelFormat() const override;
    void resetBuffers() override;

private:
    void createEGLSurface(QQnxGLContext *context);
    void destroyEGLSurface();

    QSize m_requestedBufferSize;

    // This mutex is used to protect access to the m_requestedBufferSize
    // member. This member is used in conjunction with QQnxGLContext::requestNewSurface()
    // to coordinate recreating the EGL surface which involves destroying any
    // existing EGL surface; resizing the native window buffers; and creating a new
    // EGL surface. All of this has to be done from the thread that is calling
    // QQnxGLContext::makeCurrent()
    mutable QMutex m_mutex;

    QAtomicInt m_newSurfaceRequested;
    EGLDisplay m_eglDisplay;
    EGLConfig m_eglConfig;
    EGLSurface m_eglSurface;
    QSurfaceFormat m_format;
};

QT_END_NAMESPACE

#endif // QQNXEGLWINDOW_H
