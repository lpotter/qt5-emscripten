QT += widgets
requires(qtConfig(filedialog))
SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/gettingStarted/gsQt/part5
INSTALLS += target

