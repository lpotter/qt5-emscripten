
QT += widgets
SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/gettingStarted/gsQt/part2
INSTALLS += target
LIBS += -L$$[QT_INSTALL_PLUGINS]/platforms -lhtml5

