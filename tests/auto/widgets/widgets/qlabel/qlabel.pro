CONFIG += testcase
TARGET = tst_qlabel

QT += widgets widgets-private testlib
QT += core-private gui-private

SOURCES += tst_qlabel.cpp

TESTDATA += testdata/* *.png
