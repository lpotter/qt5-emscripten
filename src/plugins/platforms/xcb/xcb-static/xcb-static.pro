#
# Statically compile in code for
# libxcb-fixes, libxcb-randr, libxcb-shm, libxcb-sync, libxcb-image,
# libxcb-keysyms, libxcb-icccm, libxcb-renderutil, libxcb-xkb,
# libxcb-xinerama, libxcb-xinput
#
CONFIG += static

XCB_DIR = $$QT_SOURCE_TREE/src/3rdparty/xcb

MODULE_INCLUDEPATH += $$XCB_DIR/include $$XCB_DIR/sysinclude
INCLUDEPATH += $$XCB_DIR/include/xcb

QMAKE_USE += xcb/nolink

# ignore compiler warnings in 3rdparty code
QMAKE_CFLAGS_STATIC_LIB+=-w

#
# libxcb
#
LIBXCB_DIR = $$XCB_DIR/libxcb

SOURCES += \
    $$LIBXCB_DIR/xfixes.c \
    $$LIBXCB_DIR/randr.c \
    $$LIBXCB_DIR/shm.c \
    $$LIBXCB_DIR/sync.c \
    $$LIBXCB_DIR/render.c \
    $$LIBXCB_DIR/shape.c \
    $$LIBXCB_DIR/xkb.c \
    $$LIBXCB_DIR/xinerama.c \
    $$LIBXCB_DIR/xinput.c

#
# xcb-util
#
XCB_UTIL_DIR = $$XCB_DIR/xcb-util


SOURCES += \
    $$XCB_UTIL_DIR/xcb_aux.c \
    $$XCB_UTIL_DIR/atoms.c \
    $$XCB_UTIL_DIR/event.c

#
# xcb-util-image
#
XCB_IMAGE_DIR = $$XCB_DIR/xcb-util-image

SOURCES += $$XCB_IMAGE_DIR/xcb_image.c

#
# xcb-util-keysyms
#
XCB_KEYSYMS_DIR = $$XCB_DIR/xcb-util-keysyms

SOURCES += $$XCB_KEYSYMS_DIR/keysyms.c

#
# xcb-util-renderutil
#

XCB_RENDERUTIL_DIR = $$XCB_DIR/xcb-util-renderutil

SOURCES += $$XCB_RENDERUTIL_DIR/util.c

#
# xcb-util-wm
#
XCB_WM_DIR = $$XCB_DIR/xcb-util-wm

SOURCES += \
    $$XCB_WM_DIR/icccm.c

OTHER_FILES = $$XCB_DIR/README

TR_EXCLUDE += $$XCB_DIR/*

load(qt_helper_lib)
