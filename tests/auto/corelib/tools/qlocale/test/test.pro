CONFIG += console testcase
QT = core testlib core-private
embedded: QT += gui
SOURCES = ../tst_qlocale.cpp

!qtConfig(doubleconversion):!qtConfig(system-doubleconversion) {
    DEFINES += QT_NO_DOUBLECONVERSION
}

TARGET = ../tst_qlocale
win32 {
    CONFIG(debug, debug|release) {
        TARGET = ../../debug/tst_qlocale
    } else {
        TARGET = ../../release/tst_qlocale
    }
}

!android:!winrt: TEST_HELPER_INSTALLS = ../syslocaleapp/syslocaleapp
