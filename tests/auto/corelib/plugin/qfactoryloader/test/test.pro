CONFIG += testcase
TARGET  = ../tst_qfactoryloader
QT = core-private testlib

SOURCES = \
    ../tst_qfactoryloader.cpp

HEADERS = \
    ../plugin1/plugininterface1.h \
    ../plugin2/plugininterface2.h

win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qfactoryloader
    } else {
        TARGET = ../../release/tst_qfactoryloader
    }
}

!qtConfig(library) {
    LIBS += -L ../bin/ -lplugin1 -lplugin2
}

android {
    libs.prefix = android_test_data
    libs.base = $$OUT_PWD/..
    libs.files += $$OUT_PWD/../bin

    RESOURCES += libs
}
