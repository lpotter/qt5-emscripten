############################################################
# Project file for autotest for file qtextobject.h
############################################################

CONFIG += testcase
TARGET = tst_qtextobject
QT += testlib
qtHaveModule(widgets): QT += widgets
SOURCES += tst_qtextobject.cpp


