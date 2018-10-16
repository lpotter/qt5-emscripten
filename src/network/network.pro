TARGET   = QtNetwork
QT = core-private

DEFINES += QT_NO_USING_NAMESPACE QT_NO_FOREACH
#DEFINES += QLOCALSERVER_DEBUG QLOCALSOCKET_DEBUG
#DEFINES += QNETWORKDISKCACHE_DEBUG
#DEFINES += QSSLSOCKET_DEBUG
#DEFINES += QHOSTINFO_DEBUG
#DEFINES += QABSTRACTSOCKET_DEBUG QNATIVESOCKETENGINE_DEBUG
#DEFINES += QTCPSOCKETENGINE_DEBUG QTCPSOCKET_DEBUG QTCPSERVER_DEBUG QSSLSOCKET_DEBUG
#DEFINES += QUDPSOCKET_DEBUG QUDPSERVER_DEBUG
#DEFINES += QSCTPSOCKET_DEBUG QSCTPSERVER_DEBUG
msvc:equals(QT_ARCH, i386): QMAKE_LFLAGS += /BASE:0x64000000

QMAKE_DOCS = $$PWD/doc/qtnetwork.qdocconf

include(access/access.pri)
include(bearer/bearer.pri)
include(kernel/kernel.pri)
include(socket/socket.pri)
include(ssl/ssl.pri)

QMAKE_LIBS += $$QMAKE_LIBS_NETWORK

qtConfig(bearermanagement) {
    ANDROID_BUNDLED_JAR_DEPENDENCIES = \
        jar/QtAndroidBearer.jar
    ANDROID_LIB_DEPENDENCIES = \
        plugins/bearer/libqandroidbearer.so
    MODULE_PLUGIN_TYPES = \
        bearer
    ANDROID_PERMISSIONS += \
        android.permission.ACCESS_NETWORK_STATE
}

MODULE_WINRT_CAPABILITIES = \
    internetClient \
    internetClientServer \
    privateNetworkClientServer

MODULE_PLUGIN_TYPES = \
    bearer
load(qt_module)
