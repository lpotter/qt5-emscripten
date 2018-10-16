TARGET = qtharfbuzz

CONFIG += \
    static \
    hide_symbols \
    exceptions_off rtti_off warn_off

MODULE_INCLUDEPATH += $$PWD/include

load(qt_helper_lib)

# built-in shapers list configuration:
SHAPERS += opentype       # HB's main shaper; enabling it should be enough most of the time

# native shaper on Apple platforms; could be used alone to handle both OT and AAT fonts
darwin: SHAPERS += coretext

# fallback shaper: not really useful with opentype or coretext shaper
#SHAPERS += fallback

DEFINES += HAVE_CONFIG_H
DEFINES += HB_NO_UNICODE_FUNCS HB_DISABLE_DEPRECATED
DEFINES += HB_NDEBUG
DEFINES += HB_EXTERN=

# platform/compiler specific definitions
DEFINES += HAVE_ATEXIT
unix: DEFINES += HAVE_PTHREAD HAVE_SCHED_H HAVE_SCHED_YIELD
win32: DEFINES += HB_NO_WIN1256

#Workaround https://code.google.com/p/android/issues/detail?id=194631
android: DEFINES += _POSIX_C_SOURCE=200112L

# Harfbuzz-NG inside Qt uses the Qt atomics (inline code only)
INCLUDEPATH += $$QT.core.includes
DEFINES += QT_NO_VERSION_TAGGING

SOURCES += \
    $$PWD/src/hb-blob.cc \
    $$PWD/src/hb-buffer.cc \
    $$PWD/src/hb-buffer-serialize.cc \
    $$PWD/src/hb-common.cc \
    $$PWD/src/hb-face.cc \
    $$PWD/src/hb-font.cc \
    $$PWD/src/hb-ot-tag.cc \
    $$PWD/src/hb-set.cc \
    $$PWD/src/hb-shape.cc \
    $$PWD/src/hb-shape-plan.cc \
    $$PWD/src/hb-shaper.cc \
    $$PWD/src/hb-unicode.cc \
    $$PWD/src/hb-warning.cc

HEADERS += \
    $$PWD/src/hb-atomic-private.hh \
    $$PWD/src/hb-buffer-private.hh \
    $$PWD/src/hb-buffer-deserialize-json.hh \
    $$PWD/src/hb-buffer-deserialize-text.hh \
    $$PWD/src/hb-cache-private.hh \
    $$PWD/src/hb-debug.hh \
    $$PWD/src/hb-dsalgs.hh \
    $$PWD/src/hb-face-private.hh \
    $$PWD/src/hb-font-private.hh \
    $$PWD/src/hb-mutex-private.hh \
    $$PWD/src/hb-object-private.hh \
    $$PWD/src/hb-open-file-private.hh \
    $$PWD/src/hb-open-type-private.hh \
    $$PWD/src/hb-ot-cbdt-table.hh \
    $$PWD/src/hb-ot-cmap-table.hh \
    $$PWD/src/hb-ot-glyf-table.hh \
    $$PWD/src/hb-ot-head-table.hh \
    $$PWD/src/hb-ot-hhea-table.hh \
    $$PWD/src/hb-ot-hmtx-table.hh \
    $$PWD/src/hb-ot-maxp-table.hh \
    $$PWD/src/hb-ot-name-table.hh \
    $$PWD/src/hb-ot-os2-table.hh \
    $$PWD/src/hb-ot-post-table.hh \
    $$PWD/src/hb-private.hh \
    $$PWD/src/hb-set-digest-private.hh \
    $$PWD/src/hb-set-private.hh \
    $$PWD/src/hb-shape-plan-private.hh \
    $$PWD/src/hb-shaper-impl-private.hh \
    $$PWD/src/hb-shaper-list.hh \
    $$PWD/src/hb-shaper-private.hh \
    $$PWD/src/hb-string-array.hh \
    $$PWD/src/hb-unicode-private.hh \
    $$PWD/src/hb-utf-private.hh

HEADERS += \
    $$PWD/src/hb.h \
    $$PWD/src/hb-blob.h \
    $$PWD/src/hb-buffer.h \
    $$PWD/src/hb-common.h \
    $$PWD/src/hb-deprecated.h \
    $$PWD/src/hb-face.h \
    $$PWD/src/hb-font.h \
    $$PWD/src/hb-set.h \
    $$PWD/src/hb-shape.h \
    $$PWD/src/hb-shape-plan.h \
    $$PWD/src/hb-unicode.h \
    $$PWD/src/hb-version.h

contains(SHAPERS, opentype) {
    DEFINES += HAVE_OT

    SOURCES += \
        $$PWD/src/hb-ot-font.cc \
        $$PWD/src/hb-ot-layout.cc \
        $$PWD/src/hb-ot-map.cc \
        $$PWD/src/hb-ot-math.cc \
        $$PWD/src/hb-ot-shape.cc \
        $$PWD/src/hb-ot-shape-complex-arabic.cc \
        $$PWD/src/hb-ot-shape-complex-default.cc \
        $$PWD/src/hb-ot-shape-complex-hangul.cc \
        $$PWD/src/hb-ot-shape-complex-hebrew.cc \
        $$PWD/src/hb-ot-shape-complex-indic.cc \
        $$PWD/src/hb-ot-shape-complex-indic-table.cc \
        $$PWD/src/hb-ot-shape-complex-myanmar.cc \
        $$PWD/src/hb-ot-shape-complex-thai.cc \
        $$PWD/src/hb-ot-shape-complex-tibetan.cc \
        $$PWD/src/hb-ot-shape-complex-use.cc \
        $$PWD/src/hb-ot-shape-complex-use-table.cc \
        $$PWD/src/hb-ot-shape-fallback.cc \
        $$PWD/src/hb-ot-shape-normalize.cc \
	$$PWD/src/hb-ot-var.cc

    HEADERS += \
        $$PWD/src/hb-ot-kern-table.hh \
        $$PWD/src/hb-ot-layout-common-private.hh \
        $$PWD/src/hb-ot-layout-gdef-table.hh \
        $$PWD/src/hb-ot-layout-gpos-table.hh \
        $$PWD/src/hb-ot-layout-gsubgpos-private.hh \
        $$PWD/src/hb-ot-layout-gsub-table.hh \
        $$PWD/src/hb-ot-layout-jstf-table.hh \
        $$PWD/src/hb-ot-layout-math-table.hh \
        $$PWD/src/hb-ot-layout-private.hh \
        $$PWD/src/hb-ot-map-private.hh \
        $$PWD/src/hb-ot-math-table.hh \
        $$PWD/src/hb-ot-post-macroman.hh \
        $$PWD/src/hb-ot-shape-complex-arabic-fallback.hh \
        $$PWD/src/hb-ot-shape-complex-arabic-private.hh \
        $$PWD/src/hb-ot-shape-complex-arabic-table.hh \
#        $$PWD/src/hb-ot-shape-complex-arabic-win1256.hh \ # disabled with HB_NO_WIN1256
        $$PWD/src/hb-ot-shape-complex-indic-machine.hh \
        $$PWD/src/hb-ot-shape-complex-indic-private.hh \
        $$PWD/src/hb-ot-shape-complex-myanmar-machine.hh \
        $$PWD/src/hb-ot-shape-complex-private.hh \
        $$PWD/src/hb-ot-shape-complex-use-machine.hh \
        $$PWD/src/hb-ot-shape-complex-use-private.hh \
        $$PWD/src/hb-ot-shape-fallback-private.hh \
        $$PWD/src/hb-ot-shape-normalize-private.hh \
        $$PWD/src/hb-ot-shape-private.hh \
        $$PWD/src/hb-ot-var-avar-table.hh \
        $$PWD/src/hb-ot-var-fvar-table.hh \
        $$PWD/src/hb-ot-var-hvar-table.hh \
        $$PWD/src/hb-ot-var-mvar-table.hh

    HEADERS += \
        $$PWD/src/hb-ot.h \
        $$PWD/src/hb-ot-font.h \
        $$PWD/src/hb-ot-layout.h \
        $$PWD/src/hb-ot-math.h \
        $$PWD/src/hb-ot-shape.h \
        $$PWD/src/hb-ot-tag.h \
        $$PWD/src/hb-ot-var.h
}

contains(SHAPERS, coretext) {
    DEFINES += HAVE_CORETEXT

    SOURCES += \
        $$PWD/src/hb-coretext.cc

    HEADERS += \
        $$PWD/src/hb-coretext.h

    uikit: \
        # On iOS/tvOS/watchOS CoreText and CoreGraphics are stand-alone frameworks
        LIBS_PRIVATE += -framework CoreText -framework CoreGraphics
    else: \
        # On Mac OS they are part of the ApplicationServices umbrella framework,
        # even in 10.8 where they were also made available stand-alone.
        LIBS_PRIVATE += -framework ApplicationServices

    CONFIG += watchos_coretext
}

contains(SHAPERS, fallback)|isEmpty(SHAPERS) {
    DEFINES += HAVE_FALLBACK

    SOURCES += \
        $$PWD/src/hb-fallback-shape.cc
}
