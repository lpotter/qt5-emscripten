HEADERS += Test.h
SOURCES += main.cpp Test.cpp
QT = core network testlib

CONFIG -= app_bundle
CONFIG += console
DESTDIR = ./
MOC_DIR = .moc/
TMP_DIR = .tmp/
