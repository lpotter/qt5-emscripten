TARGET = qconnmanbearer

QT = core network-private dbus

HEADERS += qconnmanservice_linux_p.h \
           ../linux_common/qofonoservice_linux_p.h \
           qconnmanengine.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += main.cpp \
           qconnmanservice_linux.cpp \
           ../linux_common/qofonoservice_linux.cpp \
           qconnmanengine.cpp \
           ../qnetworksession_impl.cpp

OTHER_FILES += connman.json

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QConnmanEnginePlugin
load(qt_plugin)
