/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
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

#include "qwindowsintegration.h"
#include "qwindowswindow.h"
#include "qwindowscontext.h"
#include "qwin10helpers.h"
#include "qwindowsmenu.h"
#include "qwindowsopenglcontext.h"

#include "qwindowsscreen.h"
#include "qwindowstheme.h"
#include "qwindowsservices.h"
#ifndef QT_NO_FREETYPE
#  include <QtFontDatabaseSupport/private/qwindowsfontdatabase_ft_p.h>
#endif
#if QT_CONFIG(clipboard)
#  include "qwindowsclipboard.h"
#  if QT_CONFIG(draganddrop)
#    include "qwindowsdrag.h"
#  endif
#endif
#include "qwindowsinputcontext.h"
#include "qwindowskeymapper.h"
#if QT_CONFIG(accessibility)
#  include "uiautomation/qwindowsuiaaccessibility.h"
#endif

#include <qpa/qplatformnativeinterface.h>
#include <qpa/qwindowsysteminterface.h>
#if QT_CONFIG(sessionmanager)
#  include "qwindowssessionmanager.h"
#endif
#include <QtGui/qtouchdevice.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qpa/qplatforminputcontextfactory_p.h>
#include <QtGui/qpa/qplatformcursor.h>

#include <QtEventDispatcherSupport/private/qwindowsguieventdispatcher_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>

#include <limits.h>

#if defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_DYNAMIC)
#  include "qwindowseglcontext.h"
#endif
#if !defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)
#  include "qwindowsglcontext.h"
#endif

#include "qwindowsopengltester.h"

static inline void initOpenGlBlacklistResources()
{
    Q_INIT_RESOURCE(openglblacklists);
}

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsIntegration
    \brief QPlatformIntegration implementation for Windows.
    \internal

    \section1 Programming Considerations

    The platform plugin should run on Desktop Windows from Windows XP onwards
    and Windows Embedded.

    It should compile with:
    \list
    \li Microsoft Visual Studio 2013 or later (using the Microsoft Windows SDK,
        (\c Q_CC_MSVC).
    \li Stock \l{http://mingw.org/}{MinGW} (\c Q_CC_MINGW).
        This version ships with headers that are missing a lot of WinAPI.
    \li MinGW distributions using GCC 4.7 or higher and a recent MinGW-w64 runtime API,
        such as \l{http://tdm-gcc.tdragon.net/}{TDM-GCC}, or
        \l{http://mingwbuilds.sourceforge.net/}{MinGW-builds}
        (\c Q_CC_MINGW and \c __MINGW64_VERSION_MAJOR indicating the version).
        MinGW-w64 provides more complete headers (compared to stock MinGW from mingw.org),
        including a considerable part of the Windows SDK.
    \endlist

    When using a function from the WinAPI, the minimum supported Windows version
    and Windows Embedded support should be checked. If the function is not supported
    on Windows XP or is not present in the MinGW-headers, it should be dynamically
    resolved. For this purpose, QWindowsContext has static structs like
    QWindowsUser32DLL and QWindowsShell32DLL. All function pointers should go to
    these structs to avoid lookups in several places.

    \ingroup qt-lighthouse-win
*/

struct QWindowsIntegrationPrivate
{
    Q_DISABLE_COPY(QWindowsIntegrationPrivate)
    explicit QWindowsIntegrationPrivate(const QStringList &paramList);
    ~QWindowsIntegrationPrivate();

    unsigned m_options = 0;
    QWindowsContext m_context;
    QPlatformFontDatabase *m_fontDatabase = nullptr;
#if QT_CONFIG(clipboard)
    QWindowsClipboard m_clipboard;
#  if QT_CONFIG(draganddrop)
    QWindowsDrag m_drag;
#  endif
#endif
#ifndef QT_NO_OPENGL
    QMutex m_staticContextLock;
    QScopedPointer<QWindowsStaticOpenGLContext> m_staticOpenGLContext;
#endif // QT_NO_OPENGL
    QScopedPointer<QPlatformInputContext> m_inputContext;
#if QT_CONFIG(accessibility)
   QWindowsUiaAccessibility m_accessibility;
#endif
    QWindowsServices m_services;
};

template <typename IntType>
bool parseIntOption(const QString &parameter,const QLatin1String &option,
                    IntType minimumValue, IntType maximumValue, IntType *target)
{
    const int valueLength = parameter.size() - option.size() - 1;
    if (valueLength < 1 || !parameter.startsWith(option) || parameter.at(option.size()) != QLatin1Char('='))
        return false;
    bool ok;
    const QStringRef valueRef = parameter.rightRef(valueLength);
    const int value = valueRef.toInt(&ok);
    if (ok) {
        if (value >= minimumValue && value <= maximumValue)
            *target = static_cast<IntType>(value);
        else {
            qWarning() << "Value" << value << "for option" << option << "out of range"
                << minimumValue << ".." << maximumValue;
        }
    } else {
        qWarning() << "Invalid value" << valueRef << "for option" << option;
    }
    return true;
}

static inline unsigned parseOptions(const QStringList &paramList,
                                    int *tabletAbsoluteRange,
                                    QtWindows::ProcessDpiAwareness *dpiAwareness)
{
    unsigned options = 0;
    for (const QString &param : paramList) {
        if (param.startsWith(QLatin1String("fontengine="))) {
            if (param.endsWith(QLatin1String("freetype"))) {
                options |= QWindowsIntegration::FontDatabaseFreeType;
            } else if (param.endsWith(QLatin1String("native"))) {
                options |= QWindowsIntegration::FontDatabaseNative;
            }
        } else if (param.startsWith(QLatin1String("dialogs="))) {
            if (param.endsWith(QLatin1String("xp"))) {
                options |= QWindowsIntegration::XpNativeDialogs;
            } else if (param.endsWith(QLatin1String("none"))) {
                options |= QWindowsIntegration::NoNativeDialogs;
            }
        } else if (param == QLatin1String("altgr")) {
            options |= QWindowsIntegration::DetectAltGrModifier;
        } else if (param == QLatin1String("gl=gdi")) {
            options |= QWindowsIntegration::DisableArb;
        } else if (param == QLatin1String("nodirectwrite")) {
            options |= QWindowsIntegration::DontUseDirectWriteFonts;
        } else if (param == QLatin1String("nocolorfonts")) {
            options |= QWindowsIntegration::DontUseColorFonts;
        } else if (param == QLatin1String("nomousefromtouch")) {
            options |= QWindowsIntegration::DontPassOsMouseEventsSynthesizedFromTouch;
        } else if (parseIntOption(param, QLatin1String("verbose"), 0, INT_MAX, &QWindowsContext::verbose)
            || parseIntOption(param, QLatin1String("tabletabsoluterange"), 0, INT_MAX, tabletAbsoluteRange)
            || parseIntOption(param, QLatin1String("dpiawareness"), QtWindows::ProcessDpiUnaware, QtWindows::ProcessPerMonitorDpiAware, dpiAwareness)) {
        } else if (param == QLatin1String("menus=native")) {
            options |= QWindowsIntegration::AlwaysUseNativeMenus;
        } else if (param == QLatin1String("menus=none")) {
            options |= QWindowsIntegration::NoNativeMenus;
        } else if (param == QLatin1String("nowmpointer")) {
            options |= QWindowsIntegration::DontUseWMPointer;
        } else {
            qWarning() << "Unknown option" << param;
        }
    }
    return options;
}

QWindowsIntegrationPrivate::QWindowsIntegrationPrivate(const QStringList &paramList)
{
    initOpenGlBlacklistResources();

    static bool dpiAwarenessSet = false;
    int tabletAbsoluteRange = -1;
    // Default to per-monitor awareness to avoid being scaled when monitors with different DPI
    // are connected to Windows 8.1
    QtWindows::ProcessDpiAwareness dpiAwareness = QtWindows::ProcessPerMonitorDpiAware;
    m_options = parseOptions(paramList, &tabletAbsoluteRange, &dpiAwareness);
    QWindowsFontDatabase::setFontOptions(m_options);

    if (m_context.initPointer(m_options)) {
        QCoreApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);
    } else {
        m_context.initTablet(m_options);
        if (tabletAbsoluteRange >= 0)
            m_context.setTabletAbsoluteRange(tabletAbsoluteRange);
    }

    if (!dpiAwarenessSet) { // Set only once in case of repeated instantiations of QGuiApplication.
        if (!QCoreApplication::testAttribute(Qt::AA_PluginApplication)) {
            m_context.setProcessDpiAwareness(dpiAwareness);
            qCDebug(lcQpaWindows)
                << __FUNCTION__ << "DpiAwareness=" << dpiAwareness
                << "effective process DPI awareness=" << QWindowsContext::processDpiAwareness();
        }
        dpiAwarenessSet = true;
    }

    m_context.initTouch(m_options);
    QPlatformCursor::setCapability(QPlatformCursor::OverrideCursor);
}

QWindowsIntegrationPrivate::~QWindowsIntegrationPrivate()
{
    delete m_fontDatabase;
}

QWindowsIntegration *QWindowsIntegration::m_instance = nullptr;

QWindowsIntegration::QWindowsIntegration(const QStringList &paramList) :
    d(new QWindowsIntegrationPrivate(paramList))
{
    m_instance = this;
#if QT_CONFIG(clipboard)
    d->m_clipboard.registerViewer();
#endif
    d->m_context.screenManager().handleScreenChanges();
    d->m_context.setDetectAltGrModifier((d->m_options & DetectAltGrModifier) != 0);
}

QWindowsIntegration::~QWindowsIntegration()
{
    m_instance = nullptr;
}

void QWindowsIntegration::initialize()
{
    QString icStr = QPlatformInputContextFactory::requested();
    icStr.isNull() ? d->m_inputContext.reset(new QWindowsInputContext)
                   : d->m_inputContext.reset(QPlatformInputContextFactory::create(icStr));
}

bool QWindowsIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
        return true;
#ifndef QT_NO_OPENGL
    case OpenGL:
        return true;
    case ThreadedOpenGL:
        if (const QWindowsStaticOpenGLContext *glContext = QWindowsIntegration::staticOpenGLContext())
            return glContext->supportsThreadedOpenGL();
        return false;
#endif // !QT_NO_OPENGL
    case WindowMasks:
        return true;
    case MultipleWindows:
        return true;
    case ForeignWindows:
        return true;
    case RasterGLSurface:
        return true;
    case AllGLFunctionsQueryable:
        return true;
    case SwitchableWidgetComposition:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
    return false;
}

QPlatformWindow *QWindowsIntegration::createPlatformWindow(QWindow *window) const
{
    if (window->type() == Qt::Desktop) {
        QWindowsDesktopWindow *result = new QWindowsDesktopWindow(window);
        qCDebug(lcQpaWindows) << "Desktop window:" << window
            << showbase << hex << result->winId() << noshowbase << dec << result->geometry();
        return result;
    }

    QWindowsWindowData requested;
    requested.flags = window->flags();
    requested.geometry = QHighDpi::toNativePixels(window->geometry(), window);
    // Apply custom margins (see  QWindowsWindow::setCustomMargins())).
    const QVariant customMarginsV = window->property("_q_windowsCustomMargins");
    if (customMarginsV.isValid())
        requested.customMargins = qvariant_cast<QMargins>(customMarginsV);

    QWindowsWindowData obtained =
        QWindowsWindowData::create(window, requested,
                                   QWindowsWindow::formatWindowTitle(window->title()));
    qCDebug(lcQpaWindows).nospace()
        << __FUNCTION__ << ' ' << window
        << "\n    Requested: " << requested.geometry << " frame incl.="
        << QWindowsGeometryHint::positionIncludesFrame(window)
        << ' ' << requested.flags
        << "\n    Obtained : " << obtained.geometry << " margins=" << obtained.fullFrameMargins
        << " handle=" << obtained.hwnd << ' ' << obtained.flags << '\n';

    if (Q_UNLIKELY(!obtained.hwnd))
        return nullptr;

    QWindowsWindow *result = createPlatformWindowHelper(window, obtained);
    Q_ASSERT(result);

    if (QWindowsMenuBar *menuBarToBeInstalled = QWindowsMenuBar::menuBarOf(window))
        menuBarToBeInstalled->install(result);

    return result;
}

QPlatformWindow *QWindowsIntegration::createForeignWindow(QWindow *window, WId nativeHandle) const
{
    const HWND hwnd = reinterpret_cast<HWND>(nativeHandle);
    if (!IsWindow(hwnd)) {
       qWarning("Windows QPA: Invalid foreign window ID %p.", hwnd);
       return nullptr;
    }
    QWindowsForeignWindow *result = new QWindowsForeignWindow(window, hwnd);
    const QRect obtainedGeometry = result->geometry();
    QScreen *screen = nullptr;
    if (const QPlatformScreen *pScreen = result->screenForGeometry(obtainedGeometry))
        screen = pScreen->screen();
    if (screen && screen != window->screen())
        window->setScreen(screen);
    qCDebug(lcQpaWindows) << "Foreign window:" << window << showbase << hex
        << result->winId() << noshowbase << dec << obtainedGeometry << screen;
    return result;
}

// Overridden to return a QWindowsDirect2DWindow in Direct2D plugin.
QWindowsWindow *QWindowsIntegration::createPlatformWindowHelper(QWindow *window, const QWindowsWindowData &data) const
{
    return new QWindowsWindow(window, data);
}

#ifndef QT_NO_OPENGL

QWindowsStaticOpenGLContext *QWindowsStaticOpenGLContext::doCreate()
{
#if defined(QT_OPENGL_DYNAMIC)
    QWindowsOpenGLTester::Renderer requestedRenderer = QWindowsOpenGLTester::requestedRenderer();
    switch (requestedRenderer) {
    case QWindowsOpenGLTester::DesktopGl:
        if (QWindowsStaticOpenGLContext *glCtx = QOpenGLStaticContext::create()) {
            if ((QWindowsOpenGLTester::supportedRenderers(requestedRenderer) & QWindowsOpenGLTester::DisableRotationFlag)
                && !QWindowsScreen::setOrientationPreference(Qt::LandscapeOrientation)) {
                qCWarning(lcQpaGl, "Unable to disable rotation.");
            }
            return glCtx;
        }
        qCWarning(lcQpaGl, "System OpenGL failed. Falling back to Software OpenGL.");
        return QOpenGLStaticContext::create(true);
    // If ANGLE is requested, use it, don't try anything else.
    case QWindowsOpenGLTester::AngleRendererD3d9:
    case QWindowsOpenGLTester::AngleRendererD3d11:
    case QWindowsOpenGLTester::AngleRendererD3d11Warp:
        return QWindowsEGLStaticContext::create(requestedRenderer);
    case QWindowsOpenGLTester::Gles:
        return QWindowsEGLStaticContext::create(requestedRenderer);
    case QWindowsOpenGLTester::SoftwareRasterizer:
        if (QWindowsStaticOpenGLContext *swCtx = QOpenGLStaticContext::create(true))
            return swCtx;
        qCWarning(lcQpaGl, "Software OpenGL failed. Falling back to system OpenGL.");
        if (QWindowsOpenGLTester::supportedRenderers(requestedRenderer) & QWindowsOpenGLTester::DesktopGl)
            return QOpenGLStaticContext::create();
        return nullptr;
    default:
        break;
    }

    const QWindowsOpenGLTester::Renderers supportedRenderers = QWindowsOpenGLTester::supportedRenderers(requestedRenderer);
    if (supportedRenderers.testFlag(QWindowsOpenGLTester::DisableProgramCacheFlag)
        && !QCoreApplication::testAttribute(Qt::AA_DisableShaderDiskCache)) {
        QCoreApplication::setAttribute(Qt::AA_DisableShaderDiskCache);
    }
    if (supportedRenderers & QWindowsOpenGLTester::DesktopGl) {
        if (QWindowsStaticOpenGLContext *glCtx = QOpenGLStaticContext::create()) {
            if ((supportedRenderers & QWindowsOpenGLTester::DisableRotationFlag)
                && !QWindowsScreen::setOrientationPreference(Qt::LandscapeOrientation)) {
                qCWarning(lcQpaGl, "Unable to disable rotation.");
            }
            return glCtx;
        }
    }
    if (QWindowsOpenGLTester::Renderers glesRenderers = supportedRenderers & QWindowsOpenGLTester::GlesMask) {
        if (QWindowsEGLStaticContext *eglCtx = QWindowsEGLStaticContext::create(glesRenderers))
            return eglCtx;
    }
    return QOpenGLStaticContext::create(true);
#elif defined(QT_OPENGL_ES_2)
    QWindowsOpenGLTester::Renderers glesRenderers = QWindowsOpenGLTester::requestedGlesRenderer();
    if (glesRenderers == QWindowsOpenGLTester::InvalidRenderer)
        glesRenderers = QWindowsOpenGLTester::supportedRenderers(QWindowsOpenGLTester::AngleRendererD3d11);
    return QWindowsEGLStaticContext::create(glesRenderers);
#elif !defined(QT_NO_OPENGL)
    return QOpenGLStaticContext::create();
#endif
}

QWindowsStaticOpenGLContext *QWindowsStaticOpenGLContext::create()
{
    return QWindowsStaticOpenGLContext::doCreate();
}

QPlatformOpenGLContext *QWindowsIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    qCDebug(lcQpaGl) << __FUNCTION__ << context->format();
    if (QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext()) {
        QScopedPointer<QWindowsOpenGLContext> result(staticOpenGLContext->createContext(context));
        if (result->isValid())
            return result.take();
    }
    return 0;
}

QOpenGLContext::OpenGLModuleType QWindowsIntegration::openGLModuleType()
{
#if defined(QT_OPENGL_ES_2)
    return QOpenGLContext::LibGLES;
#elif !defined(QT_OPENGL_DYNAMIC)
    return QOpenGLContext::LibGL;
#else
    if (const QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext())
        return staticOpenGLContext->moduleType();
    return QOpenGLContext::LibGL;
#endif
}

QWindowsStaticOpenGLContext *QWindowsIntegration::staticOpenGLContext()
{
    QWindowsIntegration *integration = QWindowsIntegration::instance();
    if (!integration)
        return 0;
    QWindowsIntegrationPrivate *d = integration->d.data();
    QMutexLocker lock(&d->m_staticContextLock);
    if (d->m_staticOpenGLContext.isNull())
        d->m_staticOpenGLContext.reset(QWindowsStaticOpenGLContext::create());
    return d->m_staticOpenGLContext.data();
}
#endif // !QT_NO_OPENGL

QPlatformFontDatabase *QWindowsIntegration::fontDatabase() const
{
    if (!d->m_fontDatabase) {
#ifdef QT_NO_FREETYPE
        d->m_fontDatabase = new QWindowsFontDatabase();
#else // QT_NO_FREETYPE
        if (d->m_options & QWindowsIntegration::FontDatabaseFreeType)
            d->m_fontDatabase = new QWindowsFontDatabaseFT;
        else
            d->m_fontDatabase = new QWindowsFontDatabase;
#endif // QT_NO_FREETYPE
    }
    return d->m_fontDatabase;
}

#ifdef SPI_GETKEYBOARDSPEED
static inline int keyBoardAutoRepeatRateMS()
{
  DWORD time = 0;
  if (SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &time, 0))
      return time ? 1000 / static_cast<int>(time) : 500;
  return 30;
}
#endif

QVariant QWindowsIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    switch (hint) {
    case QPlatformIntegration::CursorFlashTime:
        if (const unsigned timeMS = GetCaretBlinkTime())
            return QVariant(timeMS != INFINITE ? int(timeMS) * 2 : 0);
        break;
#ifdef SPI_GETKEYBOARDSPEED
    case KeyboardAutoRepeatRate:
        return QVariant(keyBoardAutoRepeatRateMS());
#endif
    case QPlatformIntegration::ShowIsMaximized:
    case QPlatformIntegration::StartDragTime:
    case QPlatformIntegration::StartDragDistance:
    case QPlatformIntegration::KeyboardInputInterval:
    case QPlatformIntegration::ShowIsFullScreen:
    case QPlatformIntegration::PasswordMaskDelay:
    case QPlatformIntegration::StartDragVelocity:
        break; // Not implemented
    case QPlatformIntegration::FontSmoothingGamma:
        return QVariant(QWindowsFontDatabase::fontSmoothingGamma());
    case QPlatformIntegration::MouseDoubleClickInterval:
        if (const UINT ms = GetDoubleClickTime())
            return QVariant(int(ms));
        break;
    case QPlatformIntegration::UseRtlExtensions:
        return QVariant(d->m_context.useRTLExtensions());
    default:
        break;
    }
    return QPlatformIntegration::styleHint(hint);
}

Qt::KeyboardModifiers QWindowsIntegration::queryKeyboardModifiers() const
{
    return QWindowsKeyMapper::queryKeyboardModifiers();
}

QList<int> QWindowsIntegration::possibleKeys(const QKeyEvent *e) const
{
    return d->m_context.possibleKeys(e);
}

#if QT_CONFIG(clipboard)
QPlatformClipboard * QWindowsIntegration::clipboard() const
{
    return &d->m_clipboard;
}
#  if QT_CONFIG(draganddrop)
QPlatformDrag *QWindowsIntegration::drag() const
{
    return &d->m_drag;
}
#  endif // QT_CONFIG(draganddrop)
#endif // !QT_NO_CLIPBOARD

QPlatformInputContext * QWindowsIntegration::inputContext() const
{
    return d->m_inputContext.data();
}

#if QT_CONFIG(accessibility)
QPlatformAccessibility *QWindowsIntegration::accessibility() const
{
    return &d->m_accessibility;
}
#endif

unsigned QWindowsIntegration::options() const
{
    return d->m_options;
}

#if QT_CONFIG(sessionmanager)
QPlatformSessionManager *QWindowsIntegration::createPlatformSessionManager(const QString &id, const QString &key) const
{
    return new QWindowsSessionManager(id, key);
}
#endif

QAbstractEventDispatcher * QWindowsIntegration::createEventDispatcher() const
{
    return new QWindowsGuiEventDispatcher;
}

QStringList QWindowsIntegration::themeNames() const
{
    return QStringList(QLatin1String(QWindowsTheme::name));
}

QPlatformTheme *QWindowsIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1String(QWindowsTheme::name))
        return new QWindowsTheme;
    return QPlatformIntegration::createPlatformTheme(name);
}

QPlatformServices *QWindowsIntegration::services() const
{
    return &d->m_services;
}

void QWindowsIntegration::beep() const
{
    MessageBeep(MB_OK);  // For QApplication
}

#if QT_CONFIG(vulkan)
QPlatformVulkanInstance *QWindowsIntegration::createPlatformVulkanInstance(QVulkanInstance *instance) const
{
    return new QWindowsVulkanInstance(instance);
}
#endif

QT_END_NAMESPACE
