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

#include "qlocalsocket.h"
#include "qlocalsocket_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QLocalSocket
    \since 4.4
    \inmodule QtNetwork

    \brief The QLocalSocket class provides a local socket.

    On Windows this is a named pipe and on Unix this is a local domain socket.

    If an error occurs, socketError() returns the type of error, and
    errorString() can be called to get a human readable description
    of what happened.

    Although QLocalSocket is designed for use with an event loop, it's possible
    to use it without one. In that case, you must use waitForConnected(),
    waitForReadyRead(), waitForBytesWritten(), and waitForDisconnected()
    which blocks until the operation is complete or the timeout expires.

    \sa QLocalServer
*/

/*!
    \fn void QLocalSocket::connectToServer(OpenMode openMode)
    \since 5.1

    Attempts to make a connection to serverName().
    setServerName() must be called before you open the connection.
    Alternatively you can use connectToServer(const QString &name, OpenMode openMode);

    The socket is opened in the given \a openMode and first enters ConnectingState.
    If a connection is established, QLocalSocket enters ConnectedState and emits connected().

    After calling this function, the socket can emit error() to signal that an error occurred.

    \sa state(), serverName(), waitForConnected()
*/

/*!
    \fn void QLocalSocket::open(OpenMode openMode)

    Equivalent to connectToServer(OpenMode mode).
    The socket is opened in the given \a openMode to the server defined by setServerName().

    Note that unlike in most other QIODevice subclasses, open() may not open the device directly.
    The function return false if the socket was already connected or if the server to connect
    to was not defined and true in any other case. The connected() or error() signals will be
    emitted once the device is actualy open (or the connection failed).

    See connectToServer() for more details.
*/

/*!
    \fn void QLocalSocket::connected()

    This signal is emitted after connectToServer() has been called and
    a connection has been successfully established.

    \sa connectToServer(), disconnected()
*/

/*!
    \fn bool QLocalSocket::setSocketDescriptor(qintptr socketDescriptor,
        LocalSocketState socketState, OpenMode openMode)

    Initializes QLocalSocket with the native socket descriptor
    \a socketDescriptor. Returns \c true if socketDescriptor is accepted
    as a valid socket descriptor; otherwise returns \c false. The socket is
    opened in the mode specified by \a openMode, and enters the socket state
    specified by \a socketState.

    \note It is not possible to initialize two local sockets with the same
    native socket descriptor.

    \sa socketDescriptor(), state(), openMode()
*/

/*!
    \fn qintptr QLocalSocket::socketDescriptor() const

    Returns the native socket descriptor of the QLocalSocket object if
    this is available; otherwise returns -1.

    The socket descriptor is not available when QLocalSocket
    is in UnconnectedState.
    The type of the descriptor depends on the platform:

    \list
        \li On Windows, the returned value is a
        \l{https://msdn.microsoft.com/en-us/library/windows/desktop/ms740522(v=vs.85).aspx}
        {Winsock 2 Socket Handle}.

        \li With WinRT and on INTEGRITY, the returned value is the
        QTcpSocket socket descriptor and the type is defined by
        \l{QTcpSocket::socketDescriptor}{socketDescriptor}.

        \li On all other UNIX-like operating systems, the type is
        a file descriptor representing a socket.
    \endlist

    \sa setSocketDescriptor()
*/

/*!
    \fn qint64 QLocalSocket::readData(char *data, qint64 c)
    \reimp
*/

/*!
    \fn qint64 QLocalSocket::writeData(const char *data, qint64 c)
    \reimp
*/

/*!
    \fn void QLocalSocket::abort()

    Aborts the current connection and resets the socket.
    Unlike disconnectFromServer(), this function immediately closes the socket,
    clearing any pending data in the write buffer.

    \sa disconnectFromServer(), close()
*/

/*!
    \fn qint64 QLocalSocket::bytesAvailable() const
    \reimp
*/

/*!
    \fn qint64 QLocalSocket::bytesToWrite() const
    \reimp
*/

/*!
    \fn bool QLocalSocket::canReadLine() const
    \reimp
*/

/*!
    \fn void QLocalSocket::close()
    \reimp
*/

/*!
    \fn bool QLocalSocket::waitForBytesWritten(int msecs)
    \reimp
*/

/*!
    \fn bool QLocalSocket::flush()

    This function writes as much as possible from the internal write buffer
    to the socket, without blocking.  If any data was written, this function
    returns \c true; otherwise false is returned.

    Call this function if you need QLocalSocket to start sending buffered data
    immediately. The number of bytes successfully written depends on the
    operating system. In most cases, you do not need to call this function,
    because QLocalSocket will start sending data automatically once control
    goes back to the event loop. In the absence of an event loop, call
    waitForBytesWritten() instead.

    \sa write(), waitForBytesWritten()
*/

/*!
    \fn void QLocalSocket::disconnectFromServer()

    Attempts to close the socket. If there is pending data waiting to be
    written, QLocalSocket will enter ClosingState and wait until all data
    has been written. Eventually, it will enter UnconnectedState and emit
    the disconnectedFromServer() signal.

    \sa connectToServer()
*/

/*!
    \fn QLocalSocket::LocalSocketError QLocalSocket::error() const

    Returns the type of error that last occurred.

    \sa state(), errorString()
*/

/*!
    \fn bool QLocalSocket::isValid() const

    Returns \c true if the socket is valid and ready for use; otherwise
    returns \c false.

    \note The socket's state must be ConnectedState before reading
    and writing can occur.

    \sa state(), connectToServer()
*/

/*!
    \fn qint64 QLocalSocket::readBufferSize() const

    Returns the size of the internal read buffer. This limits the amount of
    data that the client can receive before you call read() or readAll().
    A read buffer size of 0 (the default) means that the buffer has no size
    limit, ensuring that no data is lost.

    \sa setReadBufferSize(), read()
*/

/*!
    \fn void QLocalSocket::setReadBufferSize(qint64 size)

    Sets the size of QLocalSocket's internal read buffer to be \a size bytes.

    If the buffer size is limited to a certain size, QLocalSocket won't
    buffer more than this size of data. Exceptionally, a buffer size of 0
    means that the read buffer is unlimited and all incoming data is buffered.
    This is the default.

    This option is useful if you only read the data at certain points in
    time (e.g., in a real-time streaming application) or if you want to
    protect your socket against receiving too much data, which may eventually
    cause your application to run out of memory.

    \sa readBufferSize(), read()
*/

/*!
    \fn bool QLocalSocket::waitForConnected(int msecs)

    Waits until the socket is connected, up to \a msecs milliseconds. If the
    connection has been established, this function returns \c true; otherwise
    it returns \c false. In the case where it returns \c false, you can call
    error() to determine the cause of the error.

    The following example waits up to one second for a connection
    to be established:

    \snippet code/src_network_socket_qlocalsocket_unix.cpp 0

    If \a msecs is -1, this function will not time out.

    \sa connectToServer(), connected()
*/

/*!
    \fn bool QLocalSocket::waitForDisconnected(int msecs)

    Waits until the socket has disconnected, up to \a msecs
    milliseconds. If the connection has been disconnected, this
    function returns \c true; otherwise it returns \c false. In the case
    where it returns \c false, you can call error() to determine
    the cause of the error.

    The following example waits up to one second for a connection
    to be closed:

    \snippet code/src_network_socket_qlocalsocket_unix.cpp 1

    If \a msecs is -1, this function will not time out.

    \sa disconnectFromServer(), close()
*/

/*!
    \fn bool QLocalSocket::waitForReadyRead(int msecs)

    This function blocks until data is available for reading and the
    \l{QIODevice::}{readyRead()} signal has been emitted. The function
    will timeout after \a msecs milliseconds; the default timeout is
    30000 milliseconds.

    The function returns \c true if data is available for reading;
    otherwise it returns \c false (if an error occurred or the
    operation timed out).

    \sa waitForBytesWritten()
*/

/*!
    \fn void QLocalSocket::disconnected()

    This signal is emitted when the socket has been disconnected.

    \sa connectToServer(), disconnectFromServer(), abort(), connected()
*/

/*!
    \fn void QLocalSocket::error(QLocalSocket::LocalSocketError socketError)

    This signal is emitted after an error occurred. The \a socketError
    parameter describes the type of error that occurred.

    QLocalSocket::LocalSocketError is not a registered metatype, so for queued
    connections, you will have to register it with Q_DECLARE_METATYPE() and
    qRegisterMetaType().

    \sa error(), errorString(), {Creating Custom Qt Types}
*/

/*!
    \fn void QLocalSocket::stateChanged(QLocalSocket::LocalSocketState socketState)

    This signal is emitted whenever QLocalSocket's state changes.
    The \a socketState parameter is the new state.

    QLocalSocket::SocketState is not a registered metatype, so for queued
    connections, you will have to register it with Q_DECLARE_METATYPE() and
    qRegisterMetaType().

    \sa state(), {Creating Custom Qt Types}
*/

/*!
    Creates a new local socket. The \a parent argument is passed to
    QObject's constructor.
 */
QLocalSocket::QLocalSocket(QObject * parent)
    : QIODevice(*new QLocalSocketPrivate, parent)
{
    Q_D(QLocalSocket);
    d->init();
}

/*!
    Destroys the socket, closing the connection if necessary.
 */
QLocalSocket::~QLocalSocket()
{
    QLocalSocket::close();
#if !defined(Q_OS_WIN) && !defined(QT_LOCALSOCKET_TCP)
    Q_D(QLocalSocket);
    d->unixSocket.setParent(0);
#endif
}

bool QLocalSocket::open(OpenMode openMode)
{
    connectToServer(openMode);
    return isOpen();
}

/*! \overload

    Set the server \a name and attempts to make a connection to it.

    The socket is opened in the given \a openMode and first enters ConnectingState.
    If a connection is established, QLocalSocket enters ConnectedState and emits connected().

    After calling this function, the socket can emit error() to signal that an error occurred.

    \sa state(), serverName(), waitForConnected()
*/
void QLocalSocket::connectToServer(const QString &name, OpenMode openMode)
{
    setServerName(name);
    connectToServer(openMode);
}

/*!
    \since 5.1

    Set the \a name of the peer to connect to.
    On Windows name is the name of a named pipe; on Unix name is the name of a local domain socket.

    This function must be called when the socket is not connected.
*/
void QLocalSocket::setServerName(const QString & name)
{
    Q_D(QLocalSocket);
    if (d->state != UnconnectedState) {
        qWarning("QLocalSocket::setServerName() called while not in unconnected state");
        return;
    }
    d->serverName = name;
}

/*!
    Returns the name of the peer as specified by setServerName(), or an
    empty QString if setServerName() has not been called or connectToServer() failed.

    \sa connectToServer(), fullServerName()

 */
QString QLocalSocket::serverName() const
{
    Q_D(const QLocalSocket);
    return d->serverName;
}

/*!
    Returns the server path that the socket is connected to.

    \note The return value of this function is platform specific.

    \sa connectToServer(), serverName()
 */
QString QLocalSocket::fullServerName() const
{
    Q_D(const QLocalSocket);
    return d->fullServerName;
}

/*!
    Returns the state of the socket.

    \sa error()
 */
QLocalSocket::LocalSocketState QLocalSocket::state() const
{
    Q_D(const QLocalSocket);
    return d->state;
}

/*! \reimp
*/
bool QLocalSocket::isSequential() const
{
    return true;
}

/*!
    \enum QLocalSocket::LocalSocketError

    The LocalServerError enumeration represents the errors that can occur.
    The most recent error can be retrieved through a call to
    \l QLocalSocket::error().

    \value ConnectionRefusedError The connection was refused by
        the peer (or timed out).
    \value PeerClosedError  The remote socket closed the connection.
        Note that the client socket (i.e., this socket) will be closed
        after the remote close notification has been sent.
    \value ServerNotFoundError  The local socket name was not found.
    \value SocketAccessError The socket operation failed because the
        application lacked the required privileges.
    \value SocketResourceError The local system ran out of resources
        (e.g., too many sockets).
    \value SocketTimeoutError The socket operation timed out.
    \value DatagramTooLargeError The datagram was larger than the operating
        system's limit (which can be as low as 8192 bytes).
    \value ConnectionError An error occurred with the connection.
    \value UnsupportedSocketOperationError The requested socket operation
        is not supported by the local operating system.
    \value OperationError An operation was attempted while the socket was in a state that
           did not permit it.
    \value UnknownSocketError An unidentified error occurred.
 */

/*!
    \enum QLocalSocket::LocalSocketState

    This enum describes the different states in which a socket can be.

    \sa QLocalSocket::state()

    \value UnconnectedState The socket is not connected.
    \value ConnectingState The socket has started establishing a connection.
    \value ConnectedState A connection is established.
    \value ClosingState The socket is about to close
        (data may still be waiting to be written).
 */

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, QLocalSocket::LocalSocketError error)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    switch (error) {
    case QLocalSocket::ConnectionRefusedError:
        debug << "QLocalSocket::ConnectionRefusedError";
        break;
    case QLocalSocket::PeerClosedError:
        debug << "QLocalSocket::PeerClosedError";
        break;
    case QLocalSocket::ServerNotFoundError:
        debug << "QLocalSocket::ServerNotFoundError";
        break;
    case QLocalSocket::SocketAccessError:
        debug << "QLocalSocket::SocketAccessError";
        break;
    case QLocalSocket::SocketResourceError:
        debug << "QLocalSocket::SocketResourceError";
        break;
    case QLocalSocket::SocketTimeoutError:
        debug << "QLocalSocket::SocketTimeoutError";
        break;
    case QLocalSocket::DatagramTooLargeError:
        debug << "QLocalSocket::DatagramTooLargeError";
        break;
    case QLocalSocket::ConnectionError:
        debug << "QLocalSocket::ConnectionError";
        break;
    case QLocalSocket::UnsupportedSocketOperationError:
        debug << "QLocalSocket::UnsupportedSocketOperationError";
        break;
    case QLocalSocket::UnknownSocketError:
        debug << "QLocalSocket::UnknownSocketError";
        break;
    default:
        debug << "QLocalSocket::SocketError(" << int(error) << ')';
        break;
    }
    return debug;
}

QDebug operator<<(QDebug debug, QLocalSocket::LocalSocketState state)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();
    switch (state) {
    case QLocalSocket::UnconnectedState:
        debug << "QLocalSocket::UnconnectedState";
        break;
    case QLocalSocket::ConnectingState:
        debug << "QLocalSocket::ConnectingState";
        break;
    case QLocalSocket::ConnectedState:
        debug << "QLocalSocket::ConnectedState";
        break;
    case QLocalSocket::ClosingState:
        debug << "QLocalSocket::ClosingState";
        break;
    default:
        debug << "QLocalSocket::SocketState(" << int(state) << ')';
        break;
    }
    return debug;
}
#endif

QT_END_NAMESPACE

#include "moc_qlocalsocket.cpp"
