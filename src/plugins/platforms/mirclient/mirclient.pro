TARGET = qmirclient

QT += \
    core-private gui-private dbus \
    theme_support-private eventdispatcher_support-private \
    fontdatabase_support-private egl_support-private

qtHaveModule(linuxaccessibility_support-private): \
    QT += linuxaccessibility_support-private

DEFINES += MESA_EGL_NO_X11_HEADERS
# CONFIG += c++11 # only enables C++0x
QMAKE_CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden -std=c++11 -Werror -Wall
QMAKE_LFLAGS += -std=c++11 -Wl,-no-undefined

QMAKE_USE_PRIVATE += mirclient

SOURCES = \
    qmirclientappstatecontroller.cpp \
    qmirclientbackingstore.cpp \
    qmirclientclipboard.cpp \
    qmirclientcursor.cpp \
    qmirclientdebugextension.cpp \
    qmirclientdesktopwindow.cpp \
    qmirclientglcontext.cpp \
    qmirclientinput.cpp \
    qmirclientintegration.cpp \
    qmirclientnativeinterface.cpp \
    qmirclientplatformservices.cpp \
    qmirclientplugin.cpp \
    qmirclientscreen.cpp \
    qmirclientscreenobserver.cpp \
    qmirclienttheme.cpp \
    qmirclientwindow.cpp

HEADERS = \
    qmirclientappstatecontroller.h \
    qmirclientbackingstore.h \
    qmirclientclipboard.h \
    qmirclientcursor.h \
    qmirclientdebugextension.h \
    qmirclientdesktopwindow.h \
    qmirclientglcontext.h \
    qmirclientinput.h \
    qmirclientintegration.h \
    qmirclientlogging.h \
    qmirclientnativeinterface.h \
    qmirclientorientationchangeevent_p.h \
    qmirclientplatformservices.h \
    qmirclientplugin.h \
    qmirclientscreen.h \
    qmirclientscreenobserver.h \
    qmirclienttheme.h \
    qmirclientwindow.h

# libxkbcommon
!qtConfig(xkbcommon-system) {
    include(../../../3rdparty/xkbcommon.pri)
} else {
    QMAKE_USE += xkbcommon
}

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = MirServerIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
