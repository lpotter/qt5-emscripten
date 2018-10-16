TEMPLATE		= app
TARGET		= quotedfilenames
SOURCES		= main.cpp

CONFIG += no_batch

INCLUDEPATH += "include folder"

RCCINPUT = "rc folder/test.qrc"
RCCOUTPUT = "cpp folder/test.cpp"

qtPrepareTool(QMAKE_RCC, rcc)

rcc_test.commands = $$QMAKE_RCC -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
rcc_test.output = $$RCCOUTPUT
rcc_test.input = RCCINPUT
rcc_test.variable_out = SOURCES
rcc_test.name = RCC_TEST
rcc_test.CONFIG += no_link
rcc_test.depends = $$QMAKE_RCC_EXE

QMAKE_EXTRA_COMPILERS += rcc_test

DESTDIR		= ./
