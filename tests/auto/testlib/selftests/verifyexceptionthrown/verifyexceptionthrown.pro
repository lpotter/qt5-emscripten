SOURCES += tst_verifyexceptionthrown.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target
CONFIG += exceptions

TARGET = verifyexceptionthrown
