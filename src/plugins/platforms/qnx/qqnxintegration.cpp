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

#include "qqnxglobal.h"

#include "qqnxintegration.h"
#include "qqnxscreeneventthread.h"
#include "qqnxnativeinterface.h"
#include "qqnxrasterbackingstore.h"
#include "qqnxscreen.h"
#include "qqnxscreeneventhandler.h"
#include "qqnxwindow.h"
#include "qqnxnavigatoreventhandler.h"
#include "qqnxabstractnavigator.h"
#include "qqnxabstractvirtualkeyboard.h"
#include "qqnxservices.h"

#include "qqnxrasterwindow.h"
#if !defined(QT_NO_OPENGL)
#include "qqnxeglwindow.h"
#endif

#if QT_CONFIG(qqnx_pps)
#include "qqnxnavigatorpps.h"
#include "qqnxnavigatoreventnotifier.h"
#include "qqnxvirtualkeyboardpps.h"
#endif

#if QT_CONFIG(qqnx_pps)
#  include "qqnxbuttoneventnotifier.h"
#  include "qqnxclipboard.h"
#  if QT_CONFIG(qqnx_imf)
#    include "qqnxinputcontext_imf.h"
#  else
#    include "qqnxinputcontext_noimf.h"
#  endif
#endif

#include "private/qgenericunixfontdatabase_p.h"
#include "private/qgenericunixeventdispatcher_p.h"

#include <qpa/qplatformwindow.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtGui/private/qguiapplication_p.h>

#if !defined(QT_NO_OPENGL)
#include "qqnxglcontext.h"
#include <QtGui/QOpenGLContext>
#endif

#include <private/qsimpledrag_p.h>

#include <QtCore/QDebug>

#include <errno.h>

#if defined(QQNXINTEGRATION_DEBUG)
#define qIntegrationDebug qDebug
#else
#define qIntegrationDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxIntegration *QQnxIntegration::ms_instance;

static inline QQnxIntegration::Options parseOptions(const QStringList &paramList)
{
    QQnxIntegration::Options options = QQnxIntegration::NoOptions;
    if (!paramList.contains(QLatin1String("no-fullscreen"))) {
        options |= QQnxIntegration::FullScreenApplication;
    }

    if (paramList.contains(QLatin1String("flush-screen-context"))) {
        options |= QQnxIntegration::AlwaysFlushScreenContext;
    }

    if (paramList.contains(QLatin1String("rootwindow"))) {
        options |= QQnxIntegration::RootWindow;
    }

    if (!paramList.contains(QLatin1String("disable-EGL_KHR_surfaceless_context"))) {
        options |= QQnxIntegration::SurfacelessEGLContext;
    }

    return options;
}

static inline int getContextCapabilities(const QStringList &paramList)
{
    QString contextCapabilitiesPrefix = QStringLiteral("screen-context-capabilities=");
    int contextCapabilities = SCREEN_APPLICATION_CONTEXT;
    for (const QString &param : paramList) {
        if (param.startsWith(contextCapabilitiesPrefix)) {
            QStringRef value = param.midRef(contextCapabilitiesPrefix.length());
            bool ok = false;
            contextCapabilities = value.toInt(&ok, 0);
            if (!ok)
                contextCapabilities = SCREEN_APPLICATION_CONTEXT;
        }
    }
    return contextCapabilities;
}

QQnxIntegration::QQnxIntegration(const QStringList &paramList)
    : QPlatformIntegration()
    , m_screenEventThread(0)
    , m_navigatorEventHandler(new QQnxNavigatorEventHandler())
    , m_virtualKeyboard(0)
#if QT_CONFIG(qqnx_pps)
    , m_navigatorEventNotifier(0)
    , m_inputContext(0)
    , m_buttonsNotifier(new QQnxButtonEventNotifier())
#endif
    , m_services(0)
    , m_fontDatabase(new QGenericUnixFontDatabase())
    , m_eventDispatcher(createUnixEventDispatcher())
    , m_nativeInterface(new QQnxNativeInterface(this))
    , m_screenEventHandler(new QQnxScreenEventHandler(this))
#if !defined(QT_NO_CLIPBOARD)
    , m_clipboard(0)
#endif
    , m_navigator(0)
#if QT_CONFIG(draganddrop)
    , m_drag(new QSimpleDrag())
#endif
{
    ms_instance = this;
    m_options = parseOptions(paramList);
    qIntegrationDebug();

    // Open connection to QNX composition manager
    if (screen_create_context(&m_screenContext, getContextCapabilities(paramList))) {
        qFatal("%s - Screen: Failed to create screen context - Error: %s (%i)",
               Q_FUNC_INFO, strerror(errno), errno);
    }

#if QT_CONFIG(qqnx_pps)
    // Create/start navigator event notifier
    m_navigatorEventNotifier = new QQnxNavigatorEventNotifier(m_navigatorEventHandler);

    // delay invocation of start() to the time the event loop is up and running
    // needed to have the QThread internals of the main thread properly initialized
    QMetaObject::invokeMethod(m_navigatorEventNotifier, "start", Qt::QueuedConnection);
#endif

#if !defined(QT_NO_OPENGL)
    // Initialize global OpenGL resources
    QQnxGLContext::initializeContext();
#endif

    // Create/start event thread
    m_screenEventThread = new QQnxScreenEventThread(m_screenContext);
    m_screenEventHandler->setScreenEventThread(m_screenEventThread);
    m_screenEventThread->start();

#if QT_CONFIG(qqnx_pps)
    // Create/start the keyboard class.
    m_virtualKeyboard = new QQnxVirtualKeyboardPps();

    // delay invocation of start() to the time the event loop is up and running
    // needed to have the QThread internals of the main thread properly initialized
    QMetaObject::invokeMethod(m_virtualKeyboard, "start", Qt::QueuedConnection);
#endif

#if QT_CONFIG(qqnx_pps)
    m_navigator = new QQnxNavigatorPps();
#endif

    // Create services handling class
    if (m_navigator)
        m_services = new QQnxServices(m_navigator);

    createDisplays();

    if (m_virtualKeyboard) {
        // TODO check if we need to do this for all screens or only the primary one
        QObject::connect(m_virtualKeyboard, SIGNAL(heightChanged(int)),
                         primaryDisplay(), SLOT(keyboardHeightChanged(int)));

#if QT_CONFIG(qqnx_pps)
        // Set up the input context
        m_inputContext = new QQnxInputContext(this, *m_virtualKeyboard);
#if QT_CONFIG(qqnx_imf)
        m_screenEventHandler->addScreenEventFilter(m_inputContext);
#endif
#endif
    }

#if QT_CONFIG(qqnx_pps)
    // delay invocation of start() to the time the event loop is up and running
    // needed to have the QThread internals of the main thread properly initialized
    QMetaObject::invokeMethod(m_buttonsNotifier, "start", Qt::QueuedConnection);
#endif
}

QQnxIntegration::~QQnxIntegration()
{
    qIntegrationDebug("platform plugin shutdown begin");
    delete m_nativeInterface;

#if QT_CONFIG(draganddrop)
    // Destroy the drag object
    delete m_drag;
#endif

#if !defined(QT_NO_CLIPBOARD)
    // Delete the clipboard
    delete m_clipboard;
#endif

    // Stop/destroy navigator event notifier
#if QT_CONFIG(qqnx_pps)
    delete m_navigatorEventNotifier;
#endif
    delete m_navigatorEventHandler;

    // Stop/destroy screen event thread
    delete m_screenEventThread;

    // In case the event-dispatcher was never transferred to QCoreApplication
    delete m_eventDispatcher;

    delete m_screenEventHandler;

    // Destroy all displays
    destroyDisplays();

    // Close connection to QNX composition manager
    screen_destroy_context(m_screenContext);

#if !defined(QT_NO_OPENGL)
    // Cleanup global OpenGL resources
    QQnxGLContext::shutdownContext();
#endif

#if QT_CONFIG(qqnx_pps)
    // Destroy the hardware button notifier
    delete m_buttonsNotifier;

    // Destroy input context
    delete m_inputContext;
#endif

    // Destroy the keyboard class.
    delete m_virtualKeyboard;

    // Destroy services class
    delete m_services;

    // Destroy navigator interface
    delete m_navigator;

    ms_instance = nullptr;

    qIntegrationDebug("platform plugin shutdown end");
}

bool QQnxIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    qIntegrationDebug();
    switch (cap) {
    case MultipleWindows:
    case ThreadedPixmaps:
        return true;
#if !defined(QT_NO_OPENGL)
    case OpenGL:
    case ThreadedOpenGL:
    case BufferQueueingOpenGL:
        return true;
#endif
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QQnxIntegration::createPlatformWindow(QWindow *window) const
{
    qIntegrationDebug();
    QSurface::SurfaceType surfaceType = window->surfaceType();
    const bool needRootWindow = options() & RootWindow;
    switch (surfaceType) {
    case QSurface::RasterSurface:
        return new QQnxRasterWindow(window, m_screenContext, needRootWindow);
#if !defined(QT_NO_OPENGL)
    case QSurface::OpenGLSurface:
        return new QQnxEglWindow(window, m_screenContext, needRootWindow);
#endif
    default:
        qFatal("QQnxWindow: unsupported window API");
    }
    return 0;
}

QPlatformBackingStore *QQnxIntegration::createPlatformBackingStore(QWindow *window) const
{
    qIntegrationDebug();
    return new QQnxRasterBackingStore(window);
}

#if !defined(QT_NO_OPENGL)
QPlatformOpenGLContext *QQnxIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    qIntegrationDebug();

    // Get color channel sizes from window format
    QSurfaceFormat format = context->format();
    int alphaSize = format.alphaBufferSize();
    int redSize = format.redBufferSize();
    int greenSize = format.greenBufferSize();
    int blueSize = format.blueBufferSize();

    // Check if all channels are don't care
    if (alphaSize == -1 && redSize == -1 && greenSize == -1 && blueSize == -1) {
        // Set color channels based on depth of window's screen
        QQnxScreen *screen = static_cast<QQnxScreen*>(context->screen()->handle());
        int depth = screen->depth();
        if (depth == 32) {
            // SCREEN_FORMAT_RGBA8888
            alphaSize = 8;
            redSize = 8;
            greenSize = 8;
            blueSize = 8;
        } else {
            // SCREEN_FORMAT_RGB565
            alphaSize = 0;
            redSize = 5;
            greenSize = 6;
            blueSize = 5;
        }
    } else {
        // Choose best match based on supported pixel formats
        if (alphaSize <= 0 && redSize <= 5 && greenSize <= 6 && blueSize <= 5) {
            // SCREEN_FORMAT_RGB565
            alphaSize = 0;
            redSize = 5;
            greenSize = 6;
            blueSize = 5;
        } else {
            // SCREEN_FORMAT_RGBA8888
            alphaSize = 8;
            redSize = 8;
            greenSize = 8;
            blueSize = 8;
        }
    }

    // Update color channel sizes in window format
    format.setAlphaBufferSize(alphaSize);
    format.setRedBufferSize(redSize);
    format.setGreenBufferSize(greenSize);
    format.setBlueBufferSize(blueSize);
    context->setFormat(format);

    QQnxGLContext *ctx = new QQnxGLContext(context->format(), context->shareHandle());
    return ctx;
}
#endif

#if QT_CONFIG(qqnx_pps)
QPlatformInputContext *QQnxIntegration::inputContext() const
{
    qIntegrationDebug();
    return m_inputContext;
}
#endif

void QQnxIntegration::moveToScreen(QWindow *window, int screen)
{
    qIntegrationDebug() << "w =" << window << ", s =" << screen;

    // get platform window used by widget
    QQnxWindow *platformWindow = static_cast<QQnxWindow *>(window->handle());

    // lookup platform screen by index
    QQnxScreen *platformScreen = m_screens.at(screen);

    // move the platform window to the platform screen
    platformWindow->setScreen(platformScreen);
}

QAbstractEventDispatcher *QQnxIntegration::createEventDispatcher() const
{
    qIntegrationDebug();

    // We transfer ownersip of the event-dispatcher to QtCoreApplication
    QAbstractEventDispatcher *eventDispatcher = m_eventDispatcher;
    m_eventDispatcher = 0;

    return eventDispatcher;
}

QPlatformNativeInterface *QQnxIntegration::nativeInterface() const
{
    return m_nativeInterface;
}

#if !defined(QT_NO_CLIPBOARD)
QPlatformClipboard *QQnxIntegration::clipboard() const
{
    qIntegrationDebug();

#if QT_CONFIG(qqnx_pps)
    if (!m_clipboard)
        m_clipboard = new QQnxClipboard;
#endif
    return m_clipboard;
}
#endif

#if QT_CONFIG(draganddrop)
QPlatformDrag *QQnxIntegration::drag() const
{
    return m_drag;
}
#endif

QVariant QQnxIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    qIntegrationDebug();
    if ((hint == ShowIsFullScreen) && (m_options & FullScreenApplication))
        return true;

    return QPlatformIntegration::styleHint(hint);
}

QPlatformServices * QQnxIntegration::services() const
{
    return m_services;
}

QWindow *QQnxIntegration::window(screen_window_t qnxWindow)
{
    qIntegrationDebug();
    QMutexLocker locker(&m_windowMapperMutex);
    Q_UNUSED(locker);
    return m_windowMapper.value(qnxWindow, 0);
}

void QQnxIntegration::addWindow(screen_window_t qnxWindow, QWindow *window)
{
    qIntegrationDebug();
    QMutexLocker locker(&m_windowMapperMutex);
    Q_UNUSED(locker);
    m_windowMapper.insert(qnxWindow, window);
}

void QQnxIntegration::removeWindow(screen_window_t qnxWindow)
{
    qIntegrationDebug();
    QMutexLocker locker(&m_windowMapperMutex);
    Q_UNUSED(locker);
    m_windowMapper.remove(qnxWindow);
}

void QQnxIntegration::createDisplays()
{
    qIntegrationDebug();
    // Query number of displays
    int displayCount = 0;
    int result = screen_get_context_property_iv(m_screenContext, SCREEN_PROPERTY_DISPLAY_COUNT,
                                                &displayCount);
    Q_SCREEN_CRITICALERROR(result, "Failed to query display count");

    if (Q_UNLIKELY(displayCount < 1)) {
        // Never happens, even if there's no display, libscreen returns 1
        qFatal("QQnxIntegration: displayCount=%d", displayCount);
    }

    // Get all displays
    screen_display_t *displays = (screen_display_t *)alloca(sizeof(screen_display_t) * displayCount);
    result = screen_get_context_property_pv(m_screenContext, SCREEN_PROPERTY_DISPLAYS,
                                            (void **)displays);
    Q_SCREEN_CRITICALERROR(result, "Failed to query displays");

    // If it's primary, we create a QScreen for it even if it's not attached
    // since Qt will dereference QGuiApplication::primaryScreen()
    createDisplay(displays[0], /*isPrimary=*/true);

    for (int i=1; i<displayCount; i++) {
        int isAttached = 1;
        result = screen_get_display_property_iv(displays[i], SCREEN_PROPERTY_ATTACHED,
                                                &isAttached);
        Q_SCREEN_CHECKERROR(result, "Failed to query display attachment");

        if (!isAttached) {
            qIntegrationDebug("Skipping non-attached display %d", i);
            continue;
        }

        qIntegrationDebug("Creating screen for display %d", i);
        createDisplay(displays[i], /*isPrimary=*/false);
    } // of displays iteration
}

void QQnxIntegration::createDisplay(screen_display_t display, bool isPrimary)
{
    QQnxScreen *screen = new QQnxScreen(m_screenContext, display, isPrimary);
    m_screens.append(screen);
    screenAdded(screen);
    screen->adjustOrientation();

    QObject::connect(m_screenEventHandler, SIGNAL(newWindowCreated(void*)),
                     screen, SLOT(newWindowCreated(void*)));
    QObject::connect(m_screenEventHandler, SIGNAL(windowClosed(void*)),
                     screen, SLOT(windowClosed(void*)));

    QObject::connect(m_navigatorEventHandler, SIGNAL(rotationChanged(int)), screen, SLOT(setRotation(int)));
    QObject::connect(m_navigatorEventHandler, SIGNAL(windowGroupActivated(QByteArray)), screen, SLOT(activateWindowGroup(QByteArray)));
    QObject::connect(m_navigatorEventHandler, SIGNAL(windowGroupDeactivated(QByteArray)), screen, SLOT(deactivateWindowGroup(QByteArray)));
    QObject::connect(m_navigatorEventHandler, SIGNAL(windowGroupStateChanged(QByteArray,Qt::WindowState)),
            screen, SLOT(windowGroupStateChanged(QByteArray,Qt::WindowState)));
}

void QQnxIntegration::removeDisplay(QQnxScreen *screen)
{
    Q_CHECK_PTR(screen);
    Q_ASSERT(m_screens.contains(screen));
    m_screens.removeAll(screen);
    destroyScreen(screen);
}

void QQnxIntegration::destroyDisplays()
{
    qIntegrationDebug();
    Q_FOREACH (QQnxScreen *screen, m_screens) {
        QPlatformIntegration::destroyScreen(screen);
    }
    m_screens.clear();
}

QQnxScreen *QQnxIntegration::screenForNative(screen_display_t qnxScreen) const
{
    Q_FOREACH (QQnxScreen *screen, m_screens) {
        if (screen->nativeDisplay() == qnxScreen)
            return screen;
    }

    return 0;
}

QQnxScreen *QQnxIntegration::primaryDisplay() const
{
    return m_screens.first();
}

QQnxIntegration::Options QQnxIntegration::options() const
{
    return m_options;
}

screen_context_t QQnxIntegration::screenContext()
{
    return m_screenContext;
}

QQnxNavigatorEventHandler *QQnxIntegration::navigatorEventHandler()
{
    return m_navigatorEventHandler;
}

bool QQnxIntegration::supportsNavigatorEvents() const
{
    // If QQNX_PPS is defined then we have navigator
    return m_navigator != 0;
}

QT_END_NAMESPACE
