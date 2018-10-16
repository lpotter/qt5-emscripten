TARGET = wasm
CONFIG += static plugin
QT += \
    core-private gui-private \
    eventdispatcher_support-private fontdatabase_support-private egl_support-private

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

SOURCES = \
    main.cpp \
    qwasmintegration.cpp \
    qwasmwindow.cpp \
    qwasmscreen.cpp \
    qwasmfontdatabase.cpp \
    qwasmeventtranslator.cpp \
    qwasmeventdispatcher.cpp \
    qwasmcompositor.cpp \
    qwasmcursor.cpp \
    qwasmopenglcontext.cpp \
    qwasmtheme.cpp

HEADERS = \
    qwasmintegration.h \
    qwasmwindow.h \
    qwasmscreen.h \
    qwasmfontdatabase.h \
    qwasmeventtranslator.h \
    qwasmeventdispatcher.h \
    qwasmcompositor.h \
    qwasmstylepixmaps_p.h \
    qwasmcursor.h \
    qwasmopenglcontext.h \
    qwasmtheme.h

wasmfonts.files = \
    ../../../3rdparty/wasm/Vera.ttf \
    ../../../3rdparty/wasm/DejaVuSans.ttf
wasmfonts.prefix = /fonts
wasmfonts.base = ../../../3rdparty/wasm
RESOURCES += wasmfonts

qtConfig(opengl) {
    SOURCES += qwasmbackingstore.cpp
    HEADERS += qwasmbackingstore.h
}
CONFIG += egl

OTHER_FILES += \
    wasm.json \
    wasm_shell.html \
    qtloader.js

shell_files.path = $$[QT_INSTALL_PLUGINS]/platforms
shell_files.files = \
    wasm_shell.html \
    qtloader.js \
    qtlogo.svg

INSTALLS += shell_files

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QWasmIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
