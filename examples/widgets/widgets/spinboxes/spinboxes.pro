QT += widgets
requires(qtConfig(datetimeedit))

HEADERS       = window.h
SOURCES       = main.cpp \
                window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/spinboxes
INSTALLS += target
