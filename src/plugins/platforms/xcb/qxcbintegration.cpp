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

#include "qxcbintegration.h"
#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qxcbcursor.h"
#include "qxcbkeyboard.h"
#include "qxcbbackingstore.h"
#include "qxcbnativeinterface.h"
#include "qxcbclipboard.h"
#if QT_CONFIG(draganddrop)
#include "qxcbdrag.h"
#endif
#include "qxcbglintegration.h"

#ifndef QT_NO_SESSIONMANAGER
#include "qxcbsessionmanager.h"
#endif

#include <xcb/xcb.h>

#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>
#include <QtServiceSupport/private/qgenericunixservices_p.h>

#include <stdio.h>

#include <QtGui/private/qguiapplication_p.h>

#if QT_CONFIG(xcb_xlib)
#define register        /* C++17 deprecated register */
#include <X11/Xlib.h>
#undef register
#endif
#if QT_CONFIG(xcb_native_painting)
#include "qxcbnativepainting.h"
#include "qpixmap_x11_p.h"
#include "qbackingstore_x11_p.h"
#endif

#include <qpa/qplatforminputcontextfactory_p.h>
#include <private/qgenericunixthemes_p.h>
#include <qpa/qplatforminputcontext.h>

#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <QtGui/QOffscreenSurface>
#ifndef QT_NO_ACCESSIBILITY
#include <qpa/qplatformaccessibility.h>
#ifndef QT_NO_ACCESSIBILITY_ATSPI_BRIDGE
#include <QtLinuxAccessibilitySupport/private/bridge_p.h>
#endif
#endif

#include <QtCore/QFileInfo>

#if QT_CONFIG(vulkan)
#include "qxcbvulkaninstance.h"
#include "qxcbvulkanwindow.h"
#endif

QT_BEGIN_NAMESPACE

// Find out if our parent process is gdb by looking at the 'exe' symlink under /proc,.
// or, for older Linuxes, read out 'cmdline'.
static bool runningUnderDebugger()
{
#if defined(QT_DEBUG) && defined(Q_OS_LINUX)
    const QString parentProc = QLatin1String("/proc/") + QString::number(getppid());
    const QFileInfo parentProcExe(parentProc + QLatin1String("/exe"));
    if (parentProcExe.isSymLink())
        return parentProcExe.symLinkTarget().endsWith(QLatin1String("/gdb"));
    QFile f(parentProc + QLatin1String("/cmdline"));
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QByteArray s;
    char c;
    while (f.getChar(&c) && c) {
        if (c == '/')
            s.clear();
        else
            s += c;
    }
    return s == "gdb";
#else
    return false;
#endif
}

QXcbIntegration *QXcbIntegration::m_instance = nullptr;

QXcbIntegration::QXcbIntegration(const QStringList &parameters, int &argc, char **argv)
    : m_services(new QGenericUnixServices)
    , m_instanceName(0)
    , m_canGrab(true)
    , m_defaultVisualId(UINT_MAX)
{
    m_instance = this;
    qApp->setAttribute(Qt::AA_CompressHighFrequencyEvents, true);

    qRegisterMetaType<QXcbWindow*>();
#if QT_CONFIG(xcb_xlib)
    XInitThreads();
#endif
    m_nativeInterface.reset(new QXcbNativeInterface);

    // Parse arguments
    const char *displayName = 0;
    bool noGrabArg = false;
    bool doGrabArg = false;
    if (argc) {
        int j = 1;
        for (int i = 1; i < argc; i++) {
            QByteArray arg(argv[i]);
            if (arg.startsWith("--"))
                arg.remove(0, 1);
            if (arg == "-display" && i < argc - 1)
                displayName = argv[++i];
            else if (arg == "-name" && i < argc - 1)
                m_instanceName = argv[++i];
            else if (arg == "-nograb")
                noGrabArg = true;
            else if (arg == "-dograb")
                doGrabArg = true;
            else if (arg == "-visual" && i < argc - 1) {
                bool ok = false;
                m_defaultVisualId = QByteArray(argv[++i]).toUInt(&ok, 0);
                if (!ok)
                    m_defaultVisualId = UINT_MAX;
            }
            else
                argv[j++] = argv[i];
        }
        argc = j;
    } // argc

    bool underDebugger = runningUnderDebugger();
    if (noGrabArg && doGrabArg && underDebugger) {
        qWarning("Both -nograb and -dograb command line arguments specified. Please pick one. -nograb takes prcedence");
        doGrabArg = false;
    }

#if defined(QT_DEBUG)
    if (!noGrabArg && !doGrabArg && underDebugger) {
        qCDebug(lcQpaXcb, "Qt: gdb: -nograb added to command-line options.\n"
                "\t Use the -dograb option to enforce grabbing.");
    }
#endif
    m_canGrab = (!underDebugger && !noGrabArg) || (underDebugger && doGrabArg);

    static bool canNotGrabEnv = qEnvironmentVariableIsSet("QT_XCB_NO_GRAB_SERVER");
    if (canNotGrabEnv)
        m_canGrab = false;

    const int numParameters = parameters.size();
    m_connections.reserve(1 + numParameters / 2);

    if (QXcbConnection *defaultConnection = QXcbConnection::create(m_nativeInterface.data(), m_canGrab, m_defaultVisualId, displayName)) {
        m_connections.append(defaultConnection);
        for (int i = 0; i < numParameters - 1; i += 2) {
            qCDebug(lcQpaScreen) << "connecting to additional display: " << parameters.at(i) << parameters.at(i+1);
            QString display = parameters.at(i) + QLatin1Char(':') + parameters.at(i+1);
            if (QXcbConnection *connection = QXcbConnection::create(m_nativeInterface.data(), m_canGrab, m_defaultVisualId, display.toLatin1().constData()))
                m_connections.append(connection);
        }
    }

    m_fontDatabase.reset(new QGenericUnixFontDatabase());

#if QT_CONFIG(xcb_native_painting)
    if (nativePaintingEnabled()) {
        qCDebug(lcQpaXcb, "QXCB USING NATIVE PAINTING");
        qt_xcb_native_x11_info_init(defaultConnection());
    }
#endif
}

QXcbIntegration::~QXcbIntegration()
{
    qDeleteAll(m_connections);
    m_instance = nullptr;
}

QPlatformPixmap *QXcbIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
#if QT_CONFIG(xcb_native_painting)
    if (nativePaintingEnabled())
        return new QX11PlatformPixmap(type);
#endif

    return QPlatformIntegration::createPlatformPixmap(type);
}

QPlatformWindow *QXcbIntegration::createPlatformWindow(QWindow *window) const
{
    QXcbGlIntegration *glIntegration = nullptr;
    const bool isTrayIconWindow = QXcbWindow::isTrayIconWindow(window);;
    if (window->type() != Qt::Desktop && !isTrayIconWindow) {
        if (window->supportsOpenGL()) {
            glIntegration = defaultConnection()->glIntegration();
            if (glIntegration) {
                QXcbWindow *xcbWindow = glIntegration->createWindow(window);
                xcbWindow->create();
                return xcbWindow;
            }
#if QT_CONFIG(vulkan)
        } else if (window->surfaceType() == QSurface::VulkanSurface) {
            QXcbWindow *xcbWindow = new QXcbVulkanWindow(window);
            xcbWindow->create();
            return xcbWindow;
#endif
        }
    }

    Q_ASSERT(window->type() == Qt::Desktop || isTrayIconWindow || !window->supportsOpenGL()
             || (!glIntegration && window->surfaceType() == QSurface::RasterGLSurface)); // for VNC
    QXcbWindow *xcbWindow = new QXcbWindow(window);
    xcbWindow->create();
    return xcbWindow;
}

QPlatformWindow *QXcbIntegration::createForeignWindow(QWindow *window, WId nativeHandle) const
{
    return new QXcbForeignWindow(window, nativeHandle);
}

#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QXcbIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(context->screen()->handle());
    QXcbGlIntegration *glIntegration = screen->connection()->glIntegration();
    if (!glIntegration) {
        qWarning("QXcbIntegration: Cannot create platform OpenGL context, neither GLX nor EGL are enabled");
        return nullptr;
    }
    return glIntegration->createPlatformOpenGLContext(context);
}
#endif

QPlatformBackingStore *QXcbIntegration::createPlatformBackingStore(QWindow *window) const
{
    const bool isTrayIconWindow = QXcbWindow::isTrayIconWindow(window);
    if (isTrayIconWindow)
        return new QXcbSystemTrayBackingStore(window);

#if QT_CONFIG(xcb_native_painting)
    if (nativePaintingEnabled())
        return new QXcbNativeBackingStore(window);
#endif

    return new QXcbBackingStore(window);
}

QPlatformOffscreenSurface *QXcbIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(surface->screen()->handle());
    QXcbGlIntegration *glIntegration = screen->connection()->glIntegration();
    if (!glIntegration) {
        qWarning("QXcbIntegration: Cannot create platform offscreen surface, neither GLX nor EGL are enabled");
        return nullptr;
    }
    return glIntegration->createPlatformOffscreenSurface(surface);
}

bool QXcbIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case OpenGL:
    case ThreadedOpenGL:
    {
        const auto *connection = qAsConst(m_connections).first();
        if (const auto *integration = connection->glIntegration())
            return cap != ThreadedOpenGL
                || (connection->threadedEventHandling() && integration->supportsThreadedOpenGL());
        return false;
    }

    case ThreadedPixmaps:
    case WindowMasks:
    case MultipleWindows:
    case ForeignWindows:
    case SyncState:
    case RasterGLSurface:
        return true;

    case SwitchableWidgetComposition:
    {
        return m_connections.at(0)->glIntegration()
            && m_connections.at(0)->glIntegration()->supportsSwitchableWidgetComposition();
    }

    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QAbstractEventDispatcher *QXcbIntegration::createEventDispatcher() const
{
    QAbstractEventDispatcher *dispatcher = createUnixEventDispatcher();
    for (int i = 0; i < m_connections.size(); i++)
        m_connections[i]->eventReader()->registerEventDispatcher(dispatcher);
    return dispatcher;
}

void QXcbIntegration::initialize()
{
    const QLatin1String defaultInputContext("compose");
    // Perform everything that may potentially need the event dispatcher (timers, socket
    // notifiers) here instead of the constructor.
    QString icStr = QPlatformInputContextFactory::requested();
    if (icStr.isNull())
        icStr = defaultInputContext;
    m_inputContext.reset(QPlatformInputContextFactory::create(icStr));
    if (!m_inputContext && icStr != defaultInputContext && icStr != QLatin1String("none"))
        m_inputContext.reset(QPlatformInputContextFactory::create(defaultInputContext));
}

void QXcbIntegration::moveToScreen(QWindow *window, int screen)
{
    Q_UNUSED(window);
    Q_UNUSED(screen);
}

QPlatformFontDatabase *QXcbIntegration::fontDatabase() const
{
    return m_fontDatabase.data();
}

QPlatformNativeInterface * QXcbIntegration::nativeInterface() const
{
    return m_nativeInterface.data();
}

#ifndef QT_NO_CLIPBOARD
QPlatformClipboard *QXcbIntegration::clipboard() const
{
    return m_connections.at(0)->clipboard();
}
#endif

#if QT_CONFIG(draganddrop)
#include <private/qsimpledrag_p.h>
QPlatformDrag *QXcbIntegration::drag() const
{
    static const bool useSimpleDrag = qEnvironmentVariableIsSet("QT_XCB_USE_SIMPLE_DRAG");
    if (Q_UNLIKELY(useSimpleDrag)) { // This is useful for testing purposes
        static QSimpleDrag *simpleDrag = nullptr;
        if (!simpleDrag)
            simpleDrag = new QSimpleDrag();
        return simpleDrag;
    }

    return m_connections.at(0)->drag();
}
#endif

QPlatformInputContext *QXcbIntegration::inputContext() const
{
    return m_inputContext.data();
}

#ifndef QT_NO_ACCESSIBILITY
QPlatformAccessibility *QXcbIntegration::accessibility() const
{
#if !defined(QT_NO_ACCESSIBILITY_ATSPI_BRIDGE)
    if (!m_accessibility) {
        Q_ASSERT_X(QCoreApplication::eventDispatcher(), "QXcbIntegration",
            "Initializing accessibility without event-dispatcher!");
        m_accessibility.reset(new QSpiAccessibleBridge());
    }
#endif

    return m_accessibility.data();
}
#endif

QPlatformServices *QXcbIntegration::services() const
{
    return m_services.data();
}

Qt::KeyboardModifiers QXcbIntegration::queryKeyboardModifiers() const
{
    return m_connections.at(0)->queryKeyboardModifiers();
}

QList<int> QXcbIntegration::possibleKeys(const QKeyEvent *e) const
{
    return m_connections.at(0)->keyboard()->possibleKeys(e);
}

QStringList QXcbIntegration::themeNames() const
{
    return QGenericUnixTheme::themeNames();
}

QPlatformTheme *QXcbIntegration::createPlatformTheme(const QString &name) const
{
    return QGenericUnixTheme::createUnixTheme(name);
}

QVariant QXcbIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    switch (hint) {
    case QPlatformIntegration::CursorFlashTime:
    case QPlatformIntegration::KeyboardInputInterval:
    case QPlatformIntegration::MouseDoubleClickInterval:
    case QPlatformIntegration::StartDragTime:
    case QPlatformIntegration::KeyboardAutoRepeatRate:
    case QPlatformIntegration::PasswordMaskDelay:
    case QPlatformIntegration::StartDragVelocity:
    case QPlatformIntegration::UseRtlExtensions:
    case QPlatformIntegration::PasswordMaskCharacter:
        // TODO using various xcb, gnome or KDE settings
        break; // Not implemented, use defaults
    case QPlatformIntegration::StartDragDistance: {
        // The default (in QPlatformTheme::defaultThemeHint) is 10 pixels, but
        // on a high-resolution screen it makes sense to increase it.
        qreal dpi = 100.0;
        if (const QXcbScreen *screen = defaultConnection()->primaryScreen()) {
            if (screen->logicalDpi().first > dpi)
                dpi = screen->logicalDpi().first;
            if (screen->logicalDpi().second > dpi)
                dpi = screen->logicalDpi().second;
        }
        return 10.0 * dpi / 100.0;
    }
    case QPlatformIntegration::ShowIsFullScreen:
        // X11 always has support for windows, but the
        // window manager could prevent it (e.g. matchbox)
        return false;
    case QPlatformIntegration::ReplayMousePressOutsidePopup:
        return false;
    default:
        break;
    }
    return QPlatformIntegration::styleHint(hint);
}

static QString argv0BaseName()
{
    QString result;
    const QStringList arguments = QCoreApplication::arguments();
    if (!arguments.isEmpty() && !arguments.front().isEmpty()) {
        result = arguments.front();
        const int lastSlashPos = result.lastIndexOf(QLatin1Char('/'));
        if (lastSlashPos != -1)
            result.remove(0, lastSlashPos + 1);
    }
    return result;
}

static const char resourceNameVar[] = "RESOURCE_NAME";

QByteArray QXcbIntegration::wmClass() const
{
    if (m_wmClass.isEmpty()) {
        // Instance name according to ICCCM 4.1.2.5
        QString name;
        if (m_instanceName)
            name = QString::fromLocal8Bit(m_instanceName);
        if (name.isEmpty() && qEnvironmentVariableIsSet(resourceNameVar))
            name = QString::fromLocal8Bit(qgetenv(resourceNameVar));
        if (name.isEmpty())
            name = argv0BaseName();

        // Note: QCoreApplication::applicationName() cannot be called from the QGuiApplication constructor,
        // hence this delayed initialization.
        QString className = QCoreApplication::applicationName();
        if (className.isEmpty()) {
            className = argv0BaseName();
            if (!className.isEmpty() && className.at(0).isLower())
                className[0] = className.at(0).toUpper();
        }

        if (!name.isEmpty() && !className.isEmpty())
            m_wmClass = std::move(name).toLocal8Bit() + '\0' + std::move(className).toLocal8Bit() + '\0';
    }
    return m_wmClass;
}

#if QT_CONFIG(xcb_sm)
QPlatformSessionManager *QXcbIntegration::createPlatformSessionManager(const QString &id, const QString &key) const
{
    return new QXcbSessionManager(id, key);
}
#endif

void QXcbIntegration::sync()
{
    for (int i = 0; i < m_connections.size(); i++) {
        m_connections.at(i)->sync();
    }
}

// For QApplication::beep()
void QXcbIntegration::beep() const
{
    QScreen *priScreen = QGuiApplication::primaryScreen();
    if (!priScreen)
        return;
    QPlatformScreen *screen = priScreen->handle();
    if (!screen)
        return;
    xcb_connection_t *connection = static_cast<QXcbScreen *>(screen)->xcb_connection();
    xcb_bell(connection, 0);
}

bool QXcbIntegration::nativePaintingEnabled() const
{
#if QT_CONFIG(xcb_native_painting)
    static bool enabled = qEnvironmentVariableIsSet("QT_XCB_NATIVE_PAINTING");
    return enabled;
#else
    return false;
#endif
}

#if QT_CONFIG(vulkan)
QPlatformVulkanInstance *QXcbIntegration::createPlatformVulkanInstance(QVulkanInstance *instance) const
{
    return new QXcbVulkanInstance(instance);
}
#endif

QT_END_NAMESPACE
