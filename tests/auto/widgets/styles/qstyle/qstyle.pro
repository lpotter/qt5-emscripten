CONFIG += testcase
TARGET = tst_qstyle
QT += widgets testlib testlib-private
SOURCES  += tst_qstyle.cpp

android:!android-embedded {
    RESOURCES += \
        testdata.qrc
}
