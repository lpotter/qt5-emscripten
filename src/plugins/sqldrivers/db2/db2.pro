TARGET = qsqldb2

HEADERS += $$PWD/qsql_db2_p.h
SOURCES += $$PWD/qsql_db2.cpp $$PWD/main.cpp

QMAKE_USE += db2

OTHER_FILES += db2.json

equals(QT_ARCH, x86_64): DEFINES += ODBC64

PLUGIN_CLASS_NAME = QDB2DriverPlugin
include(../qsqldriverbase.pri)
