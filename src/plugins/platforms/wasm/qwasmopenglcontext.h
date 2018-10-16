/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qpa/qplatformopenglcontext.h>

#include <emscripten.h>
#include <emscripten/html5.h>

QT_BEGIN_NAMESPACE

class QWasmOpenGLContext : public QPlatformOpenGLContext
{
public:
    QWasmOpenGLContext(const QSurfaceFormat &format);
    ~QWasmOpenGLContext();

    QSurfaceFormat format() const override;
    void swapBuffers(QPlatformSurface *surface) override;
    GLuint defaultFramebufferObject(QPlatformSurface *surface) const override;
    bool makeCurrent(QPlatformSurface *surface) override;
    void doneCurrent() override;
    bool isSharing() const override;
    bool isValid() const override;
    QFunctionPointer getProcAddress(const char *procName) override;

private:
    void maybeRecreateEmscriptenContext(QPlatformSurface *surface);
    static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE createEmscriptenContext(const char *canvasId, QSurfaceFormat format);

    bool m_contextLost = false;
    QSurfaceFormat m_requestedFormat;
    QPlatformSurface *m_surface = nullptr;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_context = 0;
};

QT_END_NAMESPACE

