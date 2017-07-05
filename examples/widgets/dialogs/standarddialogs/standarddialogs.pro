QT += widgets

HEADERS       = dialog.h
SOURCES       = dialog.cpp \
                main.cpp

QMAKE_LFLAGS += -L/home/sami/src/qt/qt5-emscripten/plugins/imageformats -lqjpeg -lqgif -lqico /home/sami/src/qt/qt5-emscripten/plugins/imageformats/libqjpeg.a

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/dialogs/standarddialogs
INSTALLS += target

wince50standard-x86-msvc2005: LIBS += libcmt.lib corelibc.lib ole32.lib oleaut32.lib uuid.lib commctrl.lib coredll.lib winsock.lib ws2.lib
