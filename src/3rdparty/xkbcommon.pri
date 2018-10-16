# Requires GNU C extensions
CONFIG -= strict_c

INCLUDEPATH += $$PWD/xkbcommon \
               $$PWD/xkbcommon/xkbcommon \
               $$PWD/xkbcommon/src \
               $$PWD/xkbcommon/src/xkbcomp

include($$shadowed($$PWD/../gui/qtgui-config.pri))

DEFINES += DFLT_XKB_CONFIG_ROOT='\\"/usr/share/X11/xkb\\"' # unused, but needs to be set to something

### RMLVO names can be overwritten with environmental variables (see libxkbcommon documentation)
DEFINES += DEFAULT_XKB_RULES='\\"evdev\\"'
DEFINES += DEFAULT_XKB_MODEL='\\"pc105\\"'
DEFINES += DEFAULT_XKB_LAYOUT='\\"us\\"'
# Need to rename several files, qmake has problems processing a project when
# sub-directories contain files with an equal names.

# libxkbcommon generates some of these files while executing "./autogen.sh"
# and some while executing "make" (actually YACC)
SOURCES += \
    $$PWD/xkbcommon/src/atom.c \
    $$PWD/xkbcommon/src/xkb-compat.c \ # renamed: compat.c -> xkb-compat.c
    $$PWD/xkbcommon/src/context.c \
    $$PWD/xkbcommon/src/xkb-keymap.c \ # renamed: keymap.c -> xkb-keymap.c
    $$PWD/xkbcommon/src/keysym.c \
    $$PWD/xkbcommon/src/keysym-utf.c \
    $$PWD/xkbcommon/src/state.c \
    $$PWD/xkbcommon/src/text.c \
    $$PWD/xkbcommon/src/context-priv.c \
    $$PWD/xkbcommon/src/keymap-priv.c \
    $$PWD/xkbcommon/src/utils.c \
    $$PWD/xkbcommon/src/utf8.c

SOURCES += \
    $$PWD/xkbcommon/src/xkbcomp/action.c \
    $$PWD/xkbcommon/src/xkbcomp/ast-build.c \
    $$PWD/xkbcommon/src/xkbcomp/compat.c \
    $$PWD/xkbcommon/src/xkbcomp/expr.c \
    $$PWD/xkbcommon/src/xkbcomp/include.c \
    $$PWD/xkbcommon/src/xkbcomp/keycodes.c \
    $$PWD/xkbcommon/src/xkbcomp/keymap-dump.c \
    $$PWD/xkbcommon/src/xkbcomp/keymap.c \
    $$PWD/xkbcommon/src/xkbcomp/keywords.c \
    $$PWD/xkbcommon/src/xkbcomp/rules.c \
    $$PWD/xkbcommon/src/xkbcomp/scanner.c \
    $$PWD/xkbcommon/src/xkbcomp/symbols.c \
    $$PWD/xkbcommon/src/xkbcomp/types.c \
    $$PWD/xkbcommon/src/xkbcomp/vmod.c \
    $$PWD/xkbcommon/src/xkbcomp/xkbcomp.c \
    $$PWD/xkbcommon/src/xkbcomp/parser.c

TR_EXCLUDE += $$PWD/*
