/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//#define QTCPSERVER_DEBUG

/*! \class QTcpServer

    \brief The QTcpServer class provides a TCP-based server.

    \reentrant
    \ingroup network
    \inmodule QtNetwork

    This class makes it possible to accept incoming TCP connections.
    You can specify the port or have QTcpServer pick one
    automatically. You can listen on a specific address or on all the
    machine's addresses.

    Call listen() to have the server listen for incoming connections.
    The newConnection() signal is then emitted each time a client
    connects to the server.

    Call nextPendingConnection() to accept the pending connection as
    a connected QTcpSocket. The function returns a pointer to a
    QTcpSocket in QAbstractSocket::ConnectedState that you can use for
    communicating with the client.

    If an error occurs, serverError() returns the type of error, and
    errorString() can be called to get a human readable description of
    what happened.

    When listening for connections, the address and port on which the
    server is listening are available as serverAddress() and
    serverPort().

    Calling close() makes QTcpServer stop listening for incoming
    connections.

    Although QTcpServer is mostly designed for use with an event
    loop, it's possible to use it without one. In that case, you must
    use waitForNewConnection(), which blocks until either a
    connection is available or a timeout expires.

    \sa QTcpSocket, {Fortune Server Example}, {Threaded Fortune Server Example},
        {Loopback Example}, {Torrent Example}
*/

/*! \fn void QTcpServer::newConnection()

    This signal is emitted every time a new connection is available.

    \sa hasPendingConnections(), nextPendingConnection()
*/

/*! \fn void QTcpServer::acceptError(QAbstractSocket::SocketError socketError)
    \since 5.0

    This signal is emitted when accepting a new connection results in an error.
    The \a socketError parameter describes the type of error that occurred.

    \sa pauseAccepting(), resumeAccepting()
*/

#include "qtcpserver.h"
#include "qtcpserver_p.h"

#include "qalgorithms.h"
#include "qhostaddress.h"
#include "qlist.h"
#include "qpointer.h"
#include "qabstractsocketengine_p.h"
#include "qtcpsocket.h"
#include "qnetworkproxy.h"

QT_BEGIN_NAMESPACE

#define Q_CHECK_SOCKETENGINE(returnValue) do { \
    if (!d->socketEngine) { \
        return returnValue; \
    } } while (0)

/*! \internal
*/
QTcpServerPrivate::QTcpServerPrivate()
 : port(0)
 , socketType(QAbstractSocket::UnknownSocketType)
 , state(QAbstractSocket::UnconnectedState)
 , socketEngine(0)
 , serverSocketError(QAbstractSocket::UnknownSocketError)
 , maxConnections(30)
{
}

/*! \internal
*/
QTcpServerPrivate::~QTcpServerPrivate()
{
}

#ifndef QT_NO_NETWORKPROXY
/*! \internal

    Resolve the proxy to its final value.
*/
QNetworkProxy QTcpServerPrivate::resolveProxy(const QHostAddress &address, quint16 port)
{
    if (address.isLoopback())
        return QNetworkProxy::NoProxy;

    QList<QNetworkProxy> proxies;
    if (proxy.type() != QNetworkProxy::DefaultProxy) {
        // a non-default proxy was set with setProxy
        proxies << proxy;
    } else {
        // try the application settings instead
        QNetworkProxyQuery query(port, QString(),
                                 socketType == QAbstractSocket::SctpSocket ?
                                 QNetworkProxyQuery::SctpServer :
                                 QNetworkProxyQuery::TcpServer);
        proxies = QNetworkProxyFactory::proxyForQuery(query);
    }

    // return the first that we can use
    for (const QNetworkProxy &p : qAsConst(proxies)) {
        if (socketType == QAbstractSocket::TcpSocket &&
            (p.capabilities() & QNetworkProxy::ListeningCapability) != 0)
            return p;

        if (socketType == QAbstractSocket::SctpSocket &&
            (p.capabilities() & QNetworkProxy::SctpListeningCapability) != 0)
            return p;
    }

    // no proxy found
    // DefaultProxy will raise an error
    return QNetworkProxy(QNetworkProxy::DefaultProxy);
}
#endif

/*! \internal
*/
void QTcpServerPrivate::configureCreatedSocket()
{
#if defined(Q_OS_UNIX)
    // Under Unix, we want to be able to bind to the port, even if a socket on
    // the same address-port is in TIME_WAIT. Under Windows this is possible
    // anyway -- furthermore, the meaning of reusable on Windows is different:
    // it means that you can use the same address-port for multiple listening
    // sockets.
    // Don't abort though if we can't set that option. For example the socks
    // engine doesn't support that option, but that shouldn't prevent us from
    // trying to bind/listen.
    socketEngine->setOption(QAbstractSocketEngine::AddressReusable, 1);
#endif
}

/*! \internal
*/
void QTcpServerPrivate::readNotification()
{
    Q_Q(QTcpServer);
    for (;;) {
        if (pendingConnections.count() >= maxConnections) {
#if defined (QTCPSERVER_DEBUG)
            qDebug("QTcpServerPrivate::_q_processIncomingConnection() too many connections");
#endif
            if (socketEngine->isReadNotificationEnabled())
                socketEngine->setReadNotificationEnabled(false);
            return;
        }

        int descriptor = socketEngine->accept();
        if (descriptor == -1) {
            if (socketEngine->error() != QAbstractSocket::TemporaryError) {
                q->pauseAccepting();
                serverSocketError = socketEngine->error();
                serverSocketErrorString = socketEngine->errorString();
                emit q->acceptError(serverSocketError);
            }
            break;
        }
#if defined (QTCPSERVER_DEBUG)
        qDebug("QTcpServerPrivate::_q_processIncomingConnection() accepted socket %i", descriptor);
#endif
        q->incomingConnection(descriptor);

        QPointer<QTcpServer> that = q;
        emit q->newConnection();
        if (!that || !q->isListening())
            return;
    }
}

/*!
    Constructs a QTcpServer object.

    \a parent is passed to the QObject constructor.

    \sa listen(), setSocketDescriptor()
*/
QTcpServer::QTcpServer(QObject *parent)
    : QObject(*new QTcpServerPrivate, parent)
{
    Q_D(QTcpServer);
#if defined(QTCPSERVER_DEBUG)
    qDebug("QTcpServer::QTcpServer(%p)", parent);
#endif
    d->socketType = QAbstractSocket::TcpSocket;
}

/*!
    Destroys the QTcpServer object. If the server is listening for
    connections, the socket is automatically closed.

    Any client \l{QTcpSocket}s that are still connected must either
    disconnect or be reparented before the server is deleted.

    \sa close()
*/
QTcpServer::~QTcpServer()
{
#if defined(QTCPSERVER_DEBUG)
    qDebug("QTcpServer::~QTcpServer()");
#endif
    close();
}

/*! \internal

    Constructs a new server object with socket of type \a socketType. The \a
    parent argument is passed to QObject's constructor.
*/
QTcpServer::QTcpServer(QAbstractSocket::SocketType socketType, QTcpServerPrivate &dd,
                       QObject *parent) : QObject(dd, parent)
{
    Q_D(QTcpServer);
#if defined(QTCPSERVER_DEBUG)
    qDebug("QTcpServer::QTcpServer(%sSocket, QTcpServerPrivate == %p, parent == %p)",
           socketType == QAbstractSocket::TcpSocket ? "Tcp"
           : socketType == QAbstractSocket::UdpSocket ? "Udp"
           : socketType == QAbstractSocket::SctpSocket ? "Sctp"
           : "Unknown", &dd, parent);
#endif
    d->socketType = socketType;
}

/*!
    Tells the server to listen for incoming connections on address \a
    address and port \a port. If \a port is 0, a port is chosen
    automatically. If \a address is QHostAddress::Any, the server
    will listen on all network interfaces.

    Returns \c true on success; otherwise returns \c false.

    \sa isListening()
*/
bool QTcpServer::listen(const QHostAddress &address, quint16 port)
{
    Q_D(QTcpServer);
    if (d->state == QAbstractSocket::ListeningState) {
        qWarning("QTcpServer::listen() called when already listening");
        return false;
    }

    QAbstractSocket::NetworkLayerProtocol proto = address.protocol();
    QHostAddress addr = address;

#ifdef QT_NO_NETWORKPROXY
    static const QNetworkProxy &proxy = *(QNetworkProxy *)0;
#else
    QNetworkProxy proxy = d->resolveProxy(addr, port);
#endif

    delete d->socketEngine;
    d->socketEngine = QAbstractSocketEngine::createSocketEngine(d->socketType, proxy, this);
    if (!d->socketEngine) {
        d->serverSocketError = QAbstractSocket::UnsupportedSocketOperationError;
        d->serverSocketErrorString = tr("Operation on socket is not supported");
        return false;
    }
#ifndef QT_NO_BEARERMANAGEMENT
    //copy network session down to the socket engine (if it has been set)
    d->socketEngine->setProperty("_q_networksession", property("_q_networksession"));
#endif
    if (!d->socketEngine->initialize(d->socketType, proto)) {
        d->serverSocketError = d->socketEngine->error();
        d->serverSocketErrorString = d->socketEngine->errorString();
        return false;
    }
    proto = d->socketEngine->protocol();
    if (addr.protocol() == QAbstractSocket::AnyIPProtocol && proto == QAbstractSocket::IPv4Protocol)
        addr = QHostAddress::AnyIPv4;

    d->configureCreatedSocket();

    if (!d->socketEngine->bind(addr, port)) {
        d->serverSocketError = d->socketEngine->error();
        d->serverSocketErrorString = d->socketEngine->errorString();
        return false;
    }

    if (!d->socketEngine->listen()) {
        d->serverSocketError = d->socketEngine->error();
        d->serverSocketErrorString = d->socketEngine->errorString();
        return false;
    }

    d->socketEngine->setReceiver(d);
    d->socketEngine->setReadNotificationEnabled(true);

    d->state = QAbstractSocket::ListeningState;
    d->address = d->socketEngine->localAddress();
    d->port = d->socketEngine->localPort();

#if defined (QTCPSERVER_DEBUG)
    qDebug("QTcpServer::listen(%i, \"%s\") == true (listening on port %i)", port,
           address.toString().toLatin1().constData(), d->socketEngine->localPort());
#endif
    return true;
}

/*!
    Returns \c true if the server is currently listening for incoming
    connections; otherwise returns \c false.

    \sa listen()
*/
bool QTcpServer::isListening() const
{
    Q_D(const QTcpServer);
    Q_CHECK_SOCKETENGINE(false);
    return d->socketEngine->state() == QAbstractSocket::ListeningState;
}

/*!
    Closes the server. The server will no longer listen for incoming
    connections.

    \sa listen()
*/
void QTcpServer::close()
{
    Q_D(QTcpServer);

    qDeleteAll(d->pendingConnections);
    d->pendingConnections.clear();

    if (d->socketEngine) {
        d->socketEngine->close();
        QT_TRY {
            d->socketEngine->deleteLater();
        } QT_CATCH(const std::bad_alloc &) {
            // in out of memory situations, the socketEngine
            // will be deleted in ~QTcpServer (it's a child-object of this)
        }
        d->socketEngine = 0;
    }

    d->state = QAbstractSocket::UnconnectedState;
}

/*!
    Returns the native socket descriptor the server uses to listen
    for incoming instructions, or -1 if the server is not listening.

    If the server is using QNetworkProxy, the returned descriptor may
    not be usable with native socket functions.

    \sa setSocketDescriptor(), isListening()
*/
qintptr QTcpServer::socketDescriptor() const
{
    Q_D(const QTcpServer);
    Q_CHECK_SOCKETENGINE(-1);
    return d->socketEngine->socketDescriptor();
}

/*!
    Sets the socket descriptor this server should use when listening
    for incoming connections to \a socketDescriptor. Returns \c true if
    the socket is set successfully; otherwise returns \c false.

    The socket is assumed to be in listening state.

    \sa socketDescriptor(), isListening()
*/
bool QTcpServer::setSocketDescriptor(qintptr socketDescriptor)
{
    Q_D(QTcpServer);
    if (isListening()) {
        qWarning("QTcpServer::setSocketDescriptor() called when already listening");
        return false;
    }

    if (d->socketEngine)
        delete d->socketEngine;
    d->socketEngine = QAbstractSocketEngine::createSocketEngine(socketDescriptor, this);
    if (!d->socketEngine) {
        d->serverSocketError = QAbstractSocket::UnsupportedSocketOperationError;
        d->serverSocketErrorString = tr("Operation on socket is not supported");
        return false;
    }
#ifndef QT_NO_BEARERMANAGEMENT
    //copy network session down to the socket engine (if it has been set)
    d->socketEngine->setProperty("_q_networksession", property("_q_networksession"));
#endif
    if (!d->socketEngine->initialize(socketDescriptor, QAbstractSocket::ListeningState)) {
        d->serverSocketError = d->socketEngine->error();
        d->serverSocketErrorString = d->socketEngine->errorString();
#if defined (QTCPSERVER_DEBUG)
        qDebug("QTcpServer::setSocketDescriptor(%i) failed (%s)", socketDescriptor,
               d->serverSocketErrorString.toLatin1().constData());
#endif
        return false;
    }

    d->socketEngine->setReceiver(d);
    d->socketEngine->setReadNotificationEnabled(true);

    d->state = d->socketEngine->state();
    d->address = d->socketEngine->localAddress();
    d->port = d->socketEngine->localPort();

#if defined (QTCPSERVER_DEBUG)
    qDebug("QTcpServer::setSocketDescriptor(%i) succeeded.", socketDescriptor);
#endif
    return true;
}

/*!
    Returns the server's port if the server is listening for
    connections; otherwise returns 0.

    \sa serverAddress(), listen()
*/
quint16 QTcpServer::serverPort() const
{
    Q_D(const QTcpServer);
    Q_CHECK_SOCKETENGINE(0);
    return d->socketEngine->localPort();
}

/*!
    Returns the server's address if the server is listening for
    connections; otherwise returns QHostAddress::Null.

    \sa serverPort(), listen()
*/
QHostAddress QTcpServer::serverAddress() const
{
    Q_D(const QTcpServer);
    Q_CHECK_SOCKETENGINE(QHostAddress(QHostAddress::Null));
    return d->socketEngine->localAddress();
}

/*!
    Waits for at most \a msec milliseconds or until an incoming
    connection is available. Returns \c true if a connection is
    available; otherwise returns \c false. If the operation timed out
    and \a timedOut is not 0, *\a timedOut will be set to true.

    This is a blocking function call. Its use is disadvised in a
    single-threaded GUI application, since the whole application will
    stop responding until the function returns.
    waitForNewConnection() is mostly useful when there is no event
    loop available.

    The non-blocking alternative is to connect to the newConnection()
    signal.

    If msec is -1, this function will not time out.

    \sa hasPendingConnections(), nextPendingConnection()
*/
bool QTcpServer::waitForNewConnection(int msec, bool *timedOut)
{
    Q_D(QTcpServer);
    if (d->state != QAbstractSocket::ListeningState)
        return false;

    if (!d->socketEngine->waitForRead(msec, timedOut)) {
        d->serverSocketError = d->socketEngine->error();
        d->serverSocketErrorString = d->socketEngine->errorString();
        return false;
    }

    if (timedOut && *timedOut)
        return false;

    d->readNotification();

    return true;
}

/*!
    Returns \c true if the server has a pending connection; otherwise
    returns \c false.

    \sa nextPendingConnection(), setMaxPendingConnections()
*/
bool QTcpServer::hasPendingConnections() const
{
    return !d_func()->pendingConnections.isEmpty();
}

/*!
    Returns the next pending connection as a connected QTcpSocket
    object.

    The socket is created as a child of the server, which means that
    it is automatically deleted when the QTcpServer object is
    destroyed. It is still a good idea to delete the object
    explicitly when you are done with it, to avoid wasting memory.

    0 is returned if this function is called when there are no pending
    connections.

    \note The returned QTcpSocket object cannot be used from another
    thread. If you want to use an incoming connection from another thread,
    you need to override incomingConnection().

    \sa hasPendingConnections()
*/
QTcpSocket *QTcpServer::nextPendingConnection()
{
    Q_D(QTcpServer);
    if (d->pendingConnections.isEmpty())
        return 0;

    if (!d->socketEngine) {
        qWarning("QTcpServer::nextPendingConnection() called while not listening");
    } else if (!d->socketEngine->isReadNotificationEnabled()) {
        d->socketEngine->setReadNotificationEnabled(true);
    }

    return d->pendingConnections.takeFirst();
}

/*!
    This virtual function is called by QTcpServer when a new
    connection is available. The \a socketDescriptor argument is the
    native socket descriptor for the accepted connection.

    The base implementation creates a QTcpSocket, sets the socket
    descriptor and then stores the QTcpSocket in an internal list of
    pending connections. Finally newConnection() is emitted.

    Reimplement this function to alter the server's behavior when a
    connection is available.

    If this server is using QNetworkProxy then the \a socketDescriptor
    may not be usable with native socket functions, and should only be
    used with QTcpSocket::setSocketDescriptor().

    \note If another socket is created in the reimplementation
    of this method, it needs to be added to the Pending Connections mechanism
    by calling addPendingConnection().

    \note If you want to handle an incoming connection as a new QTcpSocket
    object in another thread you have to pass the socketDescriptor
    to the other thread and create the QTcpSocket object there and
    use its setSocketDescriptor() method.

    \sa newConnection(), nextPendingConnection(), addPendingConnection()
*/
void QTcpServer::incomingConnection(qintptr socketDescriptor)
{
#if defined (QTCPSERVER_DEBUG)
    qDebug("QTcpServer::incomingConnection(%i)", socketDescriptor);
#endif

    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    addPendingConnection(socket);
}

/*!
    This function is called by QTcpServer::incomingConnection()
    to add the \a socket to the list of pending incoming connections.

    \note Don't forget to call this member from reimplemented
    incomingConnection() if you do not want to break the
    Pending Connections mechanism.

    \sa incomingConnection()
    \since 4.7
*/
void QTcpServer::addPendingConnection(QTcpSocket* socket)
{
    d_func()->pendingConnections.append(socket);
}

/*!
    Sets the maximum number of pending accepted connections to \a
    numConnections. QTcpServer will accept no more than \a
    numConnections incoming connections before
    nextPendingConnection() is called. By default, the limit is 30
    pending connections.

    Clients may still able to connect after the server has reached
    its maximum number of pending connections (i.e., QTcpSocket can
    still emit the connected() signal). QTcpServer will stop
    accepting the new connections, but the operating system may
    still keep them in queue.

    \sa maxPendingConnections(), hasPendingConnections()
*/
void QTcpServer::setMaxPendingConnections(int numConnections)
{
    d_func()->maxConnections = numConnections;
}

/*!
    Returns the maximum number of pending accepted connections. The
    default is 30.

    \sa setMaxPendingConnections(), hasPendingConnections()
*/
int QTcpServer::maxPendingConnections() const
{
    return d_func()->maxConnections;
}

/*!
    Returns an error code for the last error that occurred.

    \sa errorString()
*/
QAbstractSocket::SocketError QTcpServer::serverError() const
{
    return d_func()->serverSocketError;
}

/*!
    Returns a human readable description of the last error that
    occurred.

    \sa serverError()
*/
QString QTcpServer::errorString() const
{
    return d_func()->serverSocketErrorString;
}

/*!
    \since 5.0

    Pauses accepting new connections. Queued connections will remain in queue.

    \sa resumeAccepting()
*/
void QTcpServer::pauseAccepting()
{
    d_func()->socketEngine->setReadNotificationEnabled(false);
}

/*!
    \since 5.0

    Resumes accepting new connections.

    \sa pauseAccepting()
*/
void QTcpServer::resumeAccepting()
{
    d_func()->socketEngine->setReadNotificationEnabled(true);
}

#ifndef QT_NO_NETWORKPROXY
/*!
    \since 4.1

    Sets the explicit network proxy for this socket to \a networkProxy.

    To disable the use of a proxy for this socket, use the
    QNetworkProxy::NoProxy proxy type:

    \snippet code/src_network_socket_qtcpserver.cpp 0

    \sa proxy(), QNetworkProxy
*/
void QTcpServer::setProxy(const QNetworkProxy &networkProxy)
{
    Q_D(QTcpServer);
    d->proxy = networkProxy;
}

/*!
    \since 4.1

    Returns the network proxy for this socket.
    By default QNetworkProxy::DefaultProxy is used.

    \sa setProxy(), QNetworkProxy
*/
QNetworkProxy QTcpServer::proxy() const
{
    Q_D(const QTcpServer);
    return d->proxy;
}
#endif // QT_NO_NETWORKPROXY

QT_END_NAMESPACE

#include "moc_qtcpserver.cpp"

