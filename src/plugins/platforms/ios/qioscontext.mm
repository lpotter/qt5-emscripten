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

#include "qioscontext.h"

#include "qiosintegration.h"
#include "qioswindow.h"

#include <dlfcn.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QOpenGLContext>

#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/glext.h>
#import <QuartzCore/CAEAGLLayer.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaGLContext, "qt.qpa.glcontext");

QIOSContext::QIOSContext(QOpenGLContext *context)
    : QPlatformOpenGLContext()
    , m_sharedContext(static_cast<QIOSContext *>(context->shareHandle()))
    , m_eaglContext(0)
    , m_format(context->format())
{
    m_format.setRenderableType(QSurfaceFormat::OpenGLES);

    EAGLSharegroup *shareGroup = m_sharedContext ? [m_sharedContext->m_eaglContext sharegroup] : nil;
    const int preferredVersion = m_format.majorVersion() == 1 ? kEAGLRenderingAPIOpenGLES1 : kEAGLRenderingAPIOpenGLES3;
    for (int version = preferredVersion; !m_eaglContext && version >= m_format.majorVersion(); --version)
        m_eaglContext = [[EAGLContext alloc] initWithAPI:EAGLRenderingAPI(version) sharegroup:shareGroup];

    if (m_eaglContext != nil) {
        EAGLContext *originalContext = [EAGLContext currentContext];
        [EAGLContext setCurrentContext:m_eaglContext];
        const GLubyte *s = glGetString(GL_VERSION);
        if (s) {
            QByteArray version = QByteArray(reinterpret_cast<const char *>(s));
            int major, minor;
            if (QPlatformOpenGLContext::parseOpenGLVersion(version, major, minor)) {
                m_format.setMajorVersion(major);
                m_format.setMinorVersion(minor);
            }
        }
        [EAGLContext setCurrentContext:originalContext];
    }

    // iOS internally double-buffers its rendering using copy instead of flipping,
    // so technically we could report that we are single-buffered so that clients
    // could take advantage of the unchanged buffer, but this means clients (and Qt)
    // will also assume that swapBufferes() is not needed, which is _not_ the case.
    m_format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

    qCDebug(lcQpaGLContext) << "created context with format" << m_format << "shared with" << m_sharedContext;
}

QIOSContext::~QIOSContext()
{
    [EAGLContext setCurrentContext:m_eaglContext];

    foreach (const FramebufferObject &framebufferObject, m_framebufferObjects)
        deleteBuffers(framebufferObject);

    [EAGLContext setCurrentContext:nil];
    [m_eaglContext release];
}

void QIOSContext::deleteBuffers(const FramebufferObject &framebufferObject)
{
    if (framebufferObject.handle)
        glDeleteFramebuffers(1, &framebufferObject.handle);
    if (framebufferObject.colorRenderbuffer)
        glDeleteRenderbuffers(1, &framebufferObject.colorRenderbuffer);
    if (framebufferObject.depthRenderbuffer)
        glDeleteRenderbuffers(1, &framebufferObject.depthRenderbuffer);
}

QSurfaceFormat QIOSContext::format() const
{
    return m_format;
}

#define QT_IOS_GL_STATUS_CASE(val) case val: return QLatin1Literal(#val)

static QString fboStatusString(GLenum status)
{
    switch (status) {
        QT_IOS_GL_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
        QT_IOS_GL_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS);
        QT_IOS_GL_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
        QT_IOS_GL_STATUS_CASE(GL_FRAMEBUFFER_UNSUPPORTED);
    default:
        return QString(QStringLiteral("unknown status: %x")).arg(status);
    }
}

#define Q_ASSERT_IS_GL_SURFACE(surface) \
    Q_ASSERT(surface && (surface->surface()->surfaceType() & (QSurface::OpenGLSurface | QSurface::RasterGLSurface)))

bool QIOSContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT_IS_GL_SURFACE(surface);

    if (!verifyGraphicsHardwareAvailability())
        return false;

    [EAGLContext setCurrentContext:m_eaglContext];

    // For offscreen surfaces we don't prepare a default FBO
    if (surface->surface()->surfaceClass() == QSurface::Offscreen)
        return true;

    Q_ASSERT(surface->surface()->surfaceClass() == QSurface::Window);
    FramebufferObject &framebufferObject = backingFramebufferObjectFor(surface);

    if (!framebufferObject.handle) {
        // Set up an FBO for the window if it hasn't been created yet
        glGenFramebuffers(1, &framebufferObject.handle);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject.handle);

        glGenRenderbuffers(1, &framebufferObject.colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.colorRenderbuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
            framebufferObject.colorRenderbuffer);

        if (m_format.depthBufferSize() > 0 || m_format.stencilBufferSize() > 0) {
            glGenRenderbuffers(1, &framebufferObject.depthRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.depthRenderbuffer);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                framebufferObject.depthRenderbuffer);

            if (m_format.stencilBufferSize() > 0)
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                    framebufferObject.depthRenderbuffer);
        }
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject.handle);
    }

    if (needsRenderbufferResize(surface)) {
        // Ensure that the FBO's buffers match the size of the layer
        CAEAGLLayer *layer = static_cast<QIOSWindow *>(surface)->eaglLayer();
        qCDebug(lcQpaGLContext, "Reallocating renderbuffer storage - current: %dx%d, layer: %gx%g",
            framebufferObject.renderbufferWidth, framebufferObject.renderbufferHeight,
            layer.frame.size.width * layer.contentsScale, layer.frame.size.height * layer.contentsScale);

        glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.colorRenderbuffer);
        [m_eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];

        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &framebufferObject.renderbufferWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &framebufferObject.renderbufferHeight);

        if (framebufferObject.depthRenderbuffer) {
            glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.depthRenderbuffer);

            // FIXME: Support more fine grained control over depth/stencil buffer sizes
            if (m_format.stencilBufferSize() > 0)
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES,
                    framebufferObject.renderbufferWidth, framebufferObject.renderbufferHeight);
            else
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                    framebufferObject.renderbufferWidth, framebufferObject.renderbufferHeight);
        }

        framebufferObject.isComplete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

        if (!framebufferObject.isComplete) {
            qCWarning(lcQpaGLContext, "QIOSContext failed to make complete framebuffer object (%s)",
                qPrintable(fboStatusString(glCheckFramebufferStatus(GL_FRAMEBUFFER))));
        }
    }

    return framebufferObject.isComplete;
}

void QIOSContext::doneCurrent()
{
    [EAGLContext setCurrentContext:nil];
}

void QIOSContext::swapBuffers(QPlatformSurface *surface)
{
    Q_ASSERT_IS_GL_SURFACE(surface);

    if (!verifyGraphicsHardwareAvailability())
        return;

    if (surface->surface()->surfaceClass() == QSurface::Offscreen)
        return; // Nothing to do

    FramebufferObject &framebufferObject = backingFramebufferObjectFor(surface);
    Q_ASSERT_X(framebufferObject.isComplete, "QIOSContext", "swapBuffers on incomplete FBO");

    if (needsRenderbufferResize(surface)) {
        qCWarning(lcQpaGLContext, "CAEAGLLayer was resized between makeCurrent and swapBuffers, skipping flush");
        return;
    }

    [EAGLContext setCurrentContext:m_eaglContext];
    glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.colorRenderbuffer);
    [m_eaglContext presentRenderbuffer:GL_RENDERBUFFER];
}

QIOSContext::FramebufferObject &QIOSContext::backingFramebufferObjectFor(QPlatformSurface *surface) const
{
    // We keep track of default-FBOs in the root context of a share-group. This assumes
    // that the contexts form a tree, where leaf nodes are always destroyed before their
    // parents. If that assumption (based on the current implementation) doesn't hold we
    // should probably use QOpenGLMultiGroupSharedResource to track the shared default-FBOs.
    if (m_sharedContext)
        return m_sharedContext->backingFramebufferObjectFor(surface);

    if (!m_framebufferObjects.contains(surface)) {
        // We're about to create a new FBO, make sure it's cleaned up as well
        connect(static_cast<QIOSWindow *>(surface), SIGNAL(destroyed(QObject*)), this, SLOT(windowDestroyed(QObject*)));
    }

    return m_framebufferObjects[surface];
}

GLuint QIOSContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    if (surface->surface()->surfaceClass() == QSurface::Offscreen) {
        // Binding and rendering to the zero-FBO on iOS seems to be
        // no-ops, so we can safely return 0 here, even if it's not
        // really a valid FBO on iOS.
        return 0;
    }

    FramebufferObject &framebufferObject = backingFramebufferObjectFor(surface);
    Q_ASSERT_X(framebufferObject.handle, "QIOSContext", "can't resolve default FBO before makeCurrent");

    return framebufferObject.handle;
}

bool QIOSContext::needsRenderbufferResize(QPlatformSurface *surface) const
{
    Q_ASSERT(surface->surface()->surfaceClass() == QSurface::Window);

    FramebufferObject &framebufferObject = backingFramebufferObjectFor(surface);
    CAEAGLLayer *layer = static_cast<QIOSWindow *>(surface)->eaglLayer();

    if (framebufferObject.renderbufferWidth != (layer.frame.size.width * layer.contentsScale))
        return true;

    if (framebufferObject.renderbufferHeight != (layer.frame.size.height * layer.contentsScale))
        return true;

    return false;
}

bool QIOSContext::verifyGraphicsHardwareAvailability()
{
    // Per the iOS OpenGL ES Programming Guide, background apps may not execute commands on the
    // graphics hardware. Specifically: "In your app delegate’s applicationDidEnterBackground:
    // method, your app may want to delete some of its OpenGL ES objects to make memory and
    // resources available to the foreground app. Call the glFinish function to ensure that
    // the resources are removed immediately. After your app exits its applicationDidEnterBackground:
    // method, it must not make any new OpenGL ES calls. If it makes an OpenGL ES call, it is
    // terminated by iOS.".
    static bool applicationBackgrounded = QGuiApplication::applicationState() == Qt::ApplicationSuspended;

    static dispatch_once_t onceToken = 0;
    dispatch_once(&onceToken, ^{
        QIOSApplicationState *applicationState = &QIOSIntegration::instance()->applicationState;
        connect(applicationState, &QIOSApplicationState::applicationStateWillChange,
            [](Qt::ApplicationState oldState, Qt::ApplicationState newState) {
                Q_UNUSED(oldState);
                if (applicationBackgrounded && newState != Qt::ApplicationSuspended) {
                    qCDebug(lcQpaGLContext) << "app no longer backgrounded, rendering enabled";
                    applicationBackgrounded = true;
                }
            }
        );
        connect(applicationState, &QIOSApplicationState::applicationStateDidChange,
            [](Qt::ApplicationState oldState, Qt::ApplicationState newState) {
                Q_UNUSED(oldState);
                if (newState != Qt::ApplicationSuspended)
                    return;

                qCDebug(lcQpaGLContext) << "app backgrounded, rendering disabled";

                // By the time we receive this signal the application has moved into
                // Qt::ApplactionStateSuspended, and all windows have been obscured,
                // which should stop all rendering. If there's still an active GL context,
                // we follow Apple's advice and call glFinish before making it inactive.
                if (QOpenGLContext *currentContext = QOpenGLContext::currentContext()) {
                    qCWarning(lcQpaGLContext) << "explicitly glFinishing and deactivating" << currentContext;
                    glFinish();
                    currentContext->doneCurrent();
                }
            }
        );
    });

    if (applicationBackgrounded) {
        static const char warning[] = "OpenGL ES calls are not allowed while an application is backgrounded";
        Q_ASSERT_X(!applicationBackgrounded, "QIOSContext", warning);
        qCWarning(lcQpaGLContext, warning);
    }

    return !applicationBackgrounded;
}

void QIOSContext::windowDestroyed(QObject *object)
{
    QIOSWindow *window = static_cast<QIOSWindow *>(object);
    if (!m_framebufferObjects.contains(window))
        return;

    qCDebug(lcQpaGLContext) << object << "destroyed, deleting corresponding FBO";

    EAGLContext *originalContext = [EAGLContext currentContext];
    [EAGLContext setCurrentContext:m_eaglContext];
    deleteBuffers(m_framebufferObjects[window]);
    m_framebufferObjects.remove(window);
    [EAGLContext setCurrentContext:originalContext];
}

QFunctionPointer QIOSContext::getProcAddress(const char *functionName)
{
    return QFunctionPointer(dlsym(RTLD_DEFAULT, functionName));
}

bool QIOSContext::isValid() const
{
    return m_eaglContext;
}

bool QIOSContext::isSharing() const
{
    return m_sharedContext;
}

#include "moc_qioscontext.cpp"

QT_END_NAMESPACE
