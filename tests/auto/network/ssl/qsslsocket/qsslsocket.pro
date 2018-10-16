CONFIG += testcase

SOURCES += tst_qsslsocket.cpp
QT = core core-private network-private testlib

TARGET = tst_qsslsocket

win32 {
  CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
  }
}

TESTDATA += certs

DEFINES += SRCDIR=\\\"$$PWD/\\\"

requires(qtConfig(private_tests))
