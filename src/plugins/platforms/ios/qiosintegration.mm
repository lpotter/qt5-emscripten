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

#include "qiosintegration.h"
#include "qioseventdispatcher.h"
#include "qiosglobal.h"
#include "qioswindow.h"
#include "qiosbackingstore.h"
#include "qiosscreen.h"
#include "qiosplatformaccessibility.h"
#include "qioscontext.h"
#ifndef Q_OS_TVOS
#include "qiosclipboard.h"
#endif
#include "qiosinputcontext.h"
#include "qiostheme.h"
#include "qiosservices.h"
#include "qiosoptionalplugininterface.h"

#include <QtGui/private/qguiapplication_p.h>

#include <qoffscreensurface.h>
#include <qpa/qplatformoffscreensurface.h>

#include <QtFontDatabaseSupport/private/qcoretextfontdatabase_p.h>
#include <QtClipboardSupport/private/qmacmime_p.h>
#include <QDir>
#include <QOperatingSystemVersion>

#import <AudioToolbox/AudioServices.h>

#include <QtDebug>

QT_BEGIN_NAMESPACE

class QCoreTextFontEngine;

QIOSIntegration *QIOSIntegration::instance()
{
    return static_cast<QIOSIntegration *>(QGuiApplicationPrivate::platformIntegration());
}

QIOSIntegration::QIOSIntegration()
    : m_fontDatabase(new QCoreTextFontDatabaseEngineFactory<QCoreTextFontEngine>)
#if !defined(Q_OS_TVOS) && !defined(QT_NO_CLIPBOARD)
    , m_clipboard(new QIOSClipboard)
#endif
    , m_inputContext(0)
    , m_platformServices(new QIOSServices)
    , m_accessibility(0)
    , m_optionalPlugins(new QFactoryLoader(QIosOptionalPluginInterface_iid, QLatin1String("/platforms/darwin")))
{
    if (Q_UNLIKELY(!qt_apple_isApplicationExtension() && !qt_apple_sharedApplication())) {
        qFatal("Error: You are creating QApplication before calling UIApplicationMain.\n" \
               "If you are writing a native iOS application, and only want to use Qt for\n" \
               "parts of the application, a good place to create QApplication is from within\n" \
               "'applicationDidFinishLaunching' inside your UIApplication delegate.\n");
    }

    // Set current directory to app bundle folder
    QDir::setCurrent(QString::fromUtf8([[[NSBundle mainBundle] bundlePath] UTF8String]));
}

void QIOSIntegration::initialize()
{
    UIScreen *mainScreen = [UIScreen mainScreen];
    NSMutableArray<UIScreen *> *screens = [[[UIScreen screens] mutableCopy] autorelease];
    if (![screens containsObject:mainScreen]) {
        // Fallback for iOS 7.1 (QTBUG-42345)
        [screens insertObject:mainScreen atIndex:0];
    }

    for (UIScreen *screen in screens)
        addScreen(new QIOSScreen(screen));

    // Depends on a primary screen being present
    m_inputContext = new QIOSInputContext;

    m_touchDevice = new QTouchDevice;
    m_touchDevice->setType(QTouchDevice::TouchScreen);
    QTouchDevice::Capabilities touchCapabilities = QTouchDevice::Position | QTouchDevice::NormalizedPosition;
    if (mainScreen.traitCollection.forceTouchCapability == UIForceTouchCapabilityAvailable)
        touchCapabilities |= QTouchDevice::Pressure;
    m_touchDevice->setCapabilities(touchCapabilities);
    QWindowSystemInterface::registerTouchDevice(m_touchDevice);
#if QT_CONFIG(tabletevent)
    QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);
#endif
    QMacInternalPasteboardMime::initializeMimeTypes();

    for (int i = 0; i < m_optionalPlugins->metaData().size(); ++i)
        qobject_cast<QIosOptionalPluginInterface *>(m_optionalPlugins->instance(i))->initPlugin();
}

QIOSIntegration::~QIOSIntegration()
{
    delete m_fontDatabase;
    m_fontDatabase = 0;

#if !defined(Q_OS_TVOS) && !defined(QT_NO_CLIPBOARD)
    delete m_clipboard;
    m_clipboard = 0;
#endif
    QMacInternalPasteboardMime::destroyMimeTypes();

    delete m_inputContext;
    m_inputContext = 0;

    foreach (QScreen *screen, QGuiApplication::screens())
        destroyScreen(screen->handle());

    delete m_platformServices;
    m_platformServices = 0;

    delete m_accessibility;
    m_accessibility = 0;

    delete m_optionalPlugins;
    m_optionalPlugins = 0;
}

bool QIOSIntegration::hasCapability(Capability cap) const
{
    switch (cap) {
    case BufferQueueingOpenGL:
        return true;
    case OpenGL:
    case ThreadedOpenGL:
        return true;
    case ThreadedPixmaps:
        return true;
    case MultipleWindows:
        return true;
    case WindowManagement:
        return false;
    case ApplicationState:
        return true;
    case RasterGLSurface:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QIOSIntegration::createPlatformWindow(QWindow *window) const
{
    return new QIOSWindow(window);
}

// Used when the QWindow's surface type is set by the client to QSurface::RasterSurface
QPlatformBackingStore *QIOSIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QIOSBackingStore(window);
}

// Used when the QWindow's surface type is set by the client to QSurface::OpenGLSurface
QPlatformOpenGLContext *QIOSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QIOSContext(context);
}

class QIOSOffscreenSurface : public QPlatformOffscreenSurface
{
public:
    QIOSOffscreenSurface(QOffscreenSurface *offscreenSurface) : QPlatformOffscreenSurface(offscreenSurface) {}

    QSurfaceFormat format() const override
    {
        Q_ASSERT(offscreenSurface());
        return offscreenSurface()->requestedFormat();
    }
    bool isValid() const override { return true; }
};

QPlatformOffscreenSurface *QIOSIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    return new QIOSOffscreenSurface(surface);
}

QAbstractEventDispatcher *QIOSIntegration::createEventDispatcher() const
{
    return QIOSEventDispatcher::create();
}

QPlatformFontDatabase * QIOSIntegration::fontDatabase() const
{
    return m_fontDatabase;
}

#ifndef QT_NO_CLIPBOARD
QPlatformClipboard *QIOSIntegration::clipboard() const
{
#ifndef Q_OS_TVOS
    return m_clipboard;
#else
    return QPlatformIntegration::clipboard();
#endif
}
#endif

QPlatformInputContext *QIOSIntegration::inputContext() const
{
    return m_inputContext;
}

QPlatformServices *QIOSIntegration::services() const
{
    return m_platformServices;
}

QVariant QIOSIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case StartDragTime:
        return 300;
    case PasswordMaskDelay:
        // this number is based on timing the native delay
        // since there is no API to get it
        return 2000;
    case ShowIsMaximized:
        return true;
    case SetFocusOnTouchRelease:
        return true;
    default:
        return QPlatformIntegration::styleHint(hint);
    }
}

QStringList QIOSIntegration::themeNames() const
{
    return QStringList(QLatin1String(QIOSTheme::name));
}

QPlatformTheme *QIOSIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1String(QIOSTheme::name))
        return new QIOSTheme;

    return QPlatformIntegration::createPlatformTheme(name);
}

QTouchDevice *QIOSIntegration::touchDevice()
{
    return m_touchDevice;
}

#ifndef QT_NO_ACCESSIBILITY
QPlatformAccessibility *QIOSIntegration::accessibility() const
{
    if (!m_accessibility)
        m_accessibility = new QIOSPlatformAccessibility;
    return m_accessibility;
}
#endif

QPlatformNativeInterface *QIOSIntegration::nativeInterface() const
{
    return const_cast<QIOSIntegration *>(this);
}

void QIOSIntegration::beep() const
{
#if !TARGET_IPHONE_SIMULATOR
    AudioServicesPlayAlertSound(kSystemSoundID_Vibrate);
#endif
}

// ---------------------------------------------------------

void *QIOSIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (!window || !window->handle())
        return 0;

    QByteArray lowerCaseResource = resource.toLower();

    QIOSWindow *platformWindow = static_cast<QIOSWindow *>(window->handle());

    if (lowerCaseResource == "uiview")
        return reinterpret_cast<void *>(platformWindow->winId());

    return 0;
}

// ---------------------------------------------------------

#include "moc_qiosintegration.cpp"

QT_END_NAMESPACE
