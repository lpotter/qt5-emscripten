SOURCES += tst_keyboard.cpp
QT += testlib testlib-private gui gui-private

macos:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = keyboard
