HEADERS       = glwidget.h \
                window.h
SOURCES       = glwidget.cpp \
                main.cpp \
                window.cpp

RESOURCES     = textures.qrc

QT           += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/textures
INSTALLS += target
