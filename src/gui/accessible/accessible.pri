# Qt accessibility module

qtConfig(accessibility) {
    HEADERS += \
        accessible/qaccessible.h \
        accessible/qaccessiblecache_p.h \
        accessible/qaccessibleobject.h \
        accessible/qaccessibleplugin.h \
        accessible/qplatformaccessibility.h

    SOURCES += accessible/qaccessible.cpp \
        accessible/qaccessiblecache.cpp \
        accessible/qaccessibleobject.cpp \
        accessible/qaccessibleplugin.cpp \
        accessible/qplatformaccessibility.cpp

    HEADERS += accessible/qaccessiblebridge.h
    SOURCES += accessible/qaccessiblebridge.cpp

    mac {
        OBJECTIVE_SOURCES += accessible/qaccessiblecache_mac.mm

        LIBS_PRIVATE += -framework Foundation
    }
}
