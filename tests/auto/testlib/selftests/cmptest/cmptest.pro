SOURCES += tst_cmptest.cpp
QT = core testlib
qtHaveModule(gui): QT += gui

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = cmptest
