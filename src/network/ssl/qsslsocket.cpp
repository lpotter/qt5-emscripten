/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
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


//#define QSSLSOCKET_DEBUG

/*!
    \class QSslSocket
    \brief The QSslSocket class provides an SSL encrypted socket for both
    clients and servers.
    \since 4.3

    \reentrant
    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    QSslSocket establishes a secure, encrypted TCP connection you can
    use for transmitting encrypted data. It can operate in both client
    and server mode, and it supports modern SSL protocols, including
    SSL 3 and TLS 1.2. By default, QSslSocket uses only SSL protocols
    which are considered to be secure (QSsl::SecureProtocols), but you can
    change the SSL protocol by calling setProtocol() as long as you do
    it before the handshake has started.

    SSL encryption operates on top of the existing TCP stream after
    the socket enters the ConnectedState. There are two simple ways to
    establish a secure connection using QSslSocket: With an immediate
    SSL handshake, or with a delayed SSL handshake occurring after the
    connection has been established in unencrypted mode.

    The most common way to use QSslSocket is to construct an object
    and start a secure connection by calling connectToHostEncrypted().
    This method starts an immediate SSL handshake once the connection
    has been established.

    \snippet code/src_network_ssl_qsslsocket.cpp 0

    As with a plain QTcpSocket, QSslSocket enters the HostLookupState,
    ConnectingState, and finally the ConnectedState, if the connection
    is successful. The handshake then starts automatically, and if it
    succeeds, the encrypted() signal is emitted to indicate the socket
    has entered the encrypted state and is ready for use.

    Note that data can be written to the socket immediately after the
    return from connectToHostEncrypted() (i.e., before the encrypted()
    signal is emitted). The data is queued in QSslSocket until after
    the encrypted() signal is emitted.

    An example of using the delayed SSL handshake to secure an
    existing connection is the case where an SSL server secures an
    incoming connection. Suppose you create an SSL server class as a
    subclass of QTcpServer. You would override
    QTcpServer::incomingConnection() with something like the example
    below, which first constructs an instance of QSslSocket and then
    calls setSocketDescriptor() to set the new socket's descriptor to
    the existing one passed in. It then initiates the SSL handshake
    by calling startServerEncryption().

    \snippet code/src_network_ssl_qsslsocket.cpp 1

    If an error occurs, QSslSocket emits the sslErrors() signal. In this
    case, if no action is taken to ignore the error(s), the connection
    is dropped. To continue, despite the occurrence of an error, you
    can call ignoreSslErrors(), either from within this slot after the
    error occurs, or any time after construction of the QSslSocket and
    before the connection is attempted. This will allow QSslSocket to
    ignore the errors it encounters when establishing the identity of
    the peer. Ignoring errors during an SSL handshake should be used
    with caution, since a fundamental characteristic of secure
    connections is that they should be established with a successful
    handshake.

    Once encrypted, you use QSslSocket as a regular QTcpSocket. When
    readyRead() is emitted, you can call read(), canReadLine() and
    readLine(), or getChar() to read decrypted data from QSslSocket's
    internal buffer, and you can call write() or putChar() to write
    data back to the peer. QSslSocket will automatically encrypt the
    written data for you, and emit encryptedBytesWritten() once
    the data has been written to the peer.

    As a convenience, QSslSocket supports QTcpSocket's blocking
    functions waitForConnected(), waitForReadyRead(),
    waitForBytesWritten(), and waitForDisconnected(). It also provides
    waitForEncrypted(), which will block the calling thread until an
    encrypted connection has been established.

    \snippet code/src_network_ssl_qsslsocket.cpp 2

    QSslSocket provides an extensive, easy-to-use API for handling
    cryptographic ciphers, private keys, and local, peer, and
    Certification Authority (CA) certificates. It also provides an API
    for handling errors that occur during the handshake phase.

    The following features can also be customized:

    \list
    \li The socket's cryptographic cipher suite can be customized before
    the handshake phase with setCiphers() and setDefaultCiphers().
    \li The socket's local certificate and private key can be customized
    before the handshake phase with setLocalCertificate() and
    setPrivateKey().
    \li The CA certificate database can be extended and customized with
    addCaCertificate(), addCaCertificates(), addDefaultCaCertificate(),
    addDefaultCaCertificates(), and QSslConfiguration::defaultConfiguration().setCaCertificates().
    \endlist

    \note If available, root certificates on Unix (excluding \macos) will be
    loaded on demand from the standard certificate directories. If you do not
    want to load root certificates on demand, you need to call either
    QSslConfiguration::defaultConfiguration().setCaCertificates() before the first
    SSL handshake is made in your application (for example, via passing
    QSslSocket::systemCaCertificates() to it), or call
    QSslConfiguration::defaultConfiguration()::setCaCertificates() on your QSslSocket instance
    prior to the SSL handshake.

    For more information about ciphers and certificates, refer to QSslCipher and
    QSslCertificate.

    This product includes software developed by the OpenSSL Project
    for use in the OpenSSL Toolkit (\l{http://www.openssl.org/}).

    \note Be aware of the difference between the bytesWritten() signal and
    the encryptedBytesWritten() signal. For a QTcpSocket, bytesWritten()
    will get emitted as soon as data has been written to the TCP socket.
    For a QSslSocket, bytesWritten() will get emitted when the data
    is being encrypted and encryptedBytesWritten()
    will get emitted as soon as data has been written to the TCP socket.

    \sa QSslCertificate, QSslCipher, QSslError
*/

/*!
    \enum QSslSocket::SslMode

    Describes the connection modes available for QSslSocket.

    \value UnencryptedMode The socket is unencrypted. Its
    behavior is identical to QTcpSocket.

    \value SslClientMode The socket is a client-side SSL socket.
    It is either alreayd encrypted, or it is in the SSL handshake
    phase (see QSslSocket::isEncrypted()).

    \value SslServerMode The socket is a server-side SSL socket.
    It is either already encrypted, or it is in the SSL handshake
    phase (see QSslSocket::isEncrypted()).
*/

/*!
    \enum QSslSocket::PeerVerifyMode
    \since 4.4

    Describes the peer verification modes for QSslSocket. The default mode is
    AutoVerifyPeer, which selects an appropriate mode depending on the
    socket's QSocket::SslMode.

    \value VerifyNone QSslSocket will not request a certificate from the
    peer. You can set this mode if you are not interested in the identity of
    the other side of the connection. The connection will still be encrypted,
    and your socket will still send its local certificate to the peer if it's
    requested.

    \value QueryPeer QSslSocket will request a certificate from the peer, but
    does not require this certificate to be valid. This is useful when you
    want to display peer certificate details to the user without affecting the
    actual SSL handshake. This mode is the default for servers.

    \value VerifyPeer QSslSocket will request a certificate from the peer
    during the SSL handshake phase, and requires that this certificate is
    valid. On failure, QSslSocket will emit the QSslSocket::sslErrors()
    signal. This mode is the default for clients.

    \value AutoVerifyPeer QSslSocket will automatically use QueryPeer for
    server sockets and VerifyPeer for client sockets.

    \sa QSslSocket::peerVerifyMode()
*/

/*!
    \fn void QSslSocket::encrypted()

    This signal is emitted when QSslSocket enters encrypted mode. After this
    signal has been emitted, QSslSocket::isEncrypted() will return true, and
    all further transmissions on the socket will be encrypted.

    \sa QSslSocket::connectToHostEncrypted(), QSslSocket::isEncrypted()
*/

/*!
    \fn void QSslSocket::modeChanged(QSslSocket::SslMode mode)

    This signal is emitted when QSslSocket changes from \l
    QSslSocket::UnencryptedMode to either \l QSslSocket::SslClientMode or \l
    QSslSocket::SslServerMode. \a mode is the new mode.

    \sa QSslSocket::mode()
*/

/*!
    \fn void QSslSocket::encryptedBytesWritten(qint64 written)
    \since 4.4

    This signal is emitted when QSslSocket writes its encrypted data to the
    network. The \a written parameter contains the number of bytes that were
    successfully written.

    \sa QIODevice::bytesWritten()
*/

/*!
    \fn void QSslSocket::peerVerifyError(const QSslError &error)
    \since 4.4

    QSslSocket can emit this signal several times during the SSL handshake,
    before encryption has been established, to indicate that an error has
    occurred while establishing the identity of the peer. The \a error is
    usually an indication that QSslSocket is unable to securely identify the
    peer.

    This signal provides you with an early indication when something's wrong.
    By connecting to this signal, you can manually choose to tear down the
    connection from inside the connected slot before the handshake has
    completed. If no action is taken, QSslSocket will proceed to emitting
    QSslSocket::sslErrors().

    \sa sslErrors()
*/

/*!
    \fn void QSslSocket::sslErrors(const QList<QSslError> &errors);

    QSslSocket emits this signal after the SSL handshake to indicate that one
    or more errors have occurred while establishing the identity of the
    peer. The errors are usually an indication that QSslSocket is unable to
    securely identify the peer. Unless any action is taken, the connection
    will be dropped after this signal has been emitted.

    If you want to continue connecting despite the errors that have occurred,
    you must call QSslSocket::ignoreSslErrors() from inside a slot connected to
    this signal. If you need to access the error list at a later point, you
    can call sslErrors() (without arguments).

    \a errors contains one or more errors that prevent QSslSocket from
    verifying the identity of the peer.

    \note You cannot use Qt::QueuedConnection when connecting to this signal,
    or calling QSslSocket::ignoreSslErrors() will have no effect.

    \sa peerVerifyError()
*/

/*!
    \fn void QSslSocket::preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator *authenticator)
    \since 5.5

    QSslSocket emits this signal when it negotiates a PSK ciphersuite, and
    therefore a PSK authentication is then required.

    When using PSK, the client must send to the server a valid identity and a
    valid pre shared key, in order for the SSL handshake to continue.
    Applications can provide this information in a slot connected to this
    signal, by filling in the passed \a authenticator object according to their
    needs.

    \note Ignoring this signal, or failing to provide the required credentials,
    will cause the handshake to fail, and therefore the connection to be aborted.

    \note The \a authenticator object is owned by the socket and must not be
    deleted by the application.

    \sa QSslPreSharedKeyAuthenticator
*/

#include "qssl_p.h"
#include "qsslsocket.h"
#include "qsslcipher.h"
#ifndef QT_NO_OPENSSL
#include "qsslsocket_openssl_p.h"
#endif
#ifdef Q_OS_WINRT
#include "qsslsocket_winrt_p.h"
#endif
#ifdef QT_SECURETRANSPORT
#include "qsslsocket_mac_p.h"
#endif
#include "qsslconfiguration_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qmutex.h>
#include <QtCore/qurl.h>
#include <QtCore/qelapsedtimer.h>
#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qhostinfo.h>

QT_BEGIN_NAMESPACE

class QSslSocketGlobalData
{
public:
    QSslSocketGlobalData()
        : config(new QSslConfigurationPrivate),
          dtlsConfig(new QSslConfigurationPrivate)
    {
#if QT_CONFIG(dtls)
        dtlsConfig->protocol = QSsl::DtlsV1_2OrLater;
#endif // dtls
    }

    QMutex mutex;
    QList<QSslCipher> supportedCiphers;
    QVector<QSslEllipticCurve> supportedEllipticCurves;
    QExplicitlySharedDataPointer<QSslConfigurationPrivate> config;
    QExplicitlySharedDataPointer<QSslConfigurationPrivate> dtlsConfig;
};
Q_GLOBAL_STATIC(QSslSocketGlobalData, globalData)

/*!
    Constructs a QSslSocket object. \a parent is passed to QObject's
    constructor. The new socket's \l {QSslCipher} {cipher} suite is
    set to the one returned by the static method defaultCiphers().
*/
QSslSocket::QSslSocket(QObject *parent)
    : QTcpSocket(*new QSslSocketBackendPrivate, parent)
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::QSslSocket(" << parent << "), this =" << (void *)this;
#endif
    d->q_ptr = this;
    d->init();
}

/*!
    Destroys the QSslSocket.
*/
QSslSocket::~QSslSocket()
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::~QSslSocket(), this =" << (void *)this;
#endif
    delete d->plainSocket;
    d->plainSocket = nullptr;
}

/*!
    \reimp

    \since 5.0

    Continues data transfer on the socket after it has been paused. If
    "setPauseMode(QAbstractSocket::PauseOnSslErrors);" has been called on
    this socket and a sslErrors() signal is received, calling this method
    is necessary for the socket to continue.

    \sa QAbstractSocket::pauseMode(), QAbstractSocket::setPauseMode()
*/
void QSslSocket::resume()
{
    // continuing might emit signals, rather do this through the event loop
    QMetaObject::invokeMethod(this, "_q_resumeImplementation", Qt::QueuedConnection);
}

/*!
    Starts an encrypted connection to the device \a hostName on \a
    port, using \a mode as the \l OpenMode. This is equivalent to
    calling connectToHost() to establish the connection, followed by a
    call to startClientEncryption(). The \a protocol parameter can be
    used to specify which network protocol to use (eg. IPv4 or IPv6).

    QSslSocket first enters the HostLookupState. Then, after entering
    either the event loop or one of the waitFor...() functions, it
    enters the ConnectingState, emits connected(), and then initiates
    the SSL client handshake. At each state change, QSslSocket emits
    signal stateChanged().

    After initiating the SSL client handshake, if the identity of the
    peer can't be established, signal sslErrors() is emitted. If you
    want to ignore the errors and continue connecting, you must call
    ignoreSslErrors(), either from inside a slot function connected to
    the sslErrors() signal, or prior to entering encrypted mode. If
    ignoreSslErrors() is not called, the connection is dropped, signal
    disconnected() is emitted, and QSslSocket returns to the
    UnconnectedState.

    If the SSL handshake is successful, QSslSocket emits encrypted().

    \snippet code/src_network_ssl_qsslsocket.cpp 3

    \note The example above shows that text can be written to
    the socket immediately after requesting the encrypted connection,
    before the encrypted() signal has been emitted. In such cases, the
    text is queued in the object and written to the socket \e after
    the connection is established and the encrypted() signal has been
    emitted.

    The default for \a mode is \l ReadWrite.

    If you want to create a QSslSocket on the server side of a connection, you
    should instead call startServerEncryption() upon receiving the incoming
    connection through QTcpServer.

    \sa connectToHost(), startClientEncryption(), waitForConnected(), waitForEncrypted()
*/
void QSslSocket::connectToHostEncrypted(const QString &hostName, quint16 port, OpenMode mode, NetworkLayerProtocol protocol)
{
    Q_D(QSslSocket);
    if (d->state == ConnectedState || d->state == ConnectingState) {
        qCWarning(lcSsl,
                  "QSslSocket::connectToHostEncrypted() called when already connecting/connected");
        return;
    }

    if (!supportsSsl()) {
        qCWarning(lcSsl, "QSslSocket::connectToHostEncrypted: TLS initialization failed");
        d->setErrorAndEmit(QAbstractSocket::SslInternalError, tr("TLS initialization failed"));
        return;
    }

    d->init();
    d->autoStartHandshake = true;
    d->initialized = true;

    // Note: When connecting to localhost, some platforms (e.g., HP-UX and some BSDs)
    // establish the connection immediately (i.e., first attempt).
    connectToHost(hostName, port, mode, protocol);
}

/*!
    \since 4.6
    \overload

    In addition to the original behaviour of connectToHostEncrypted,
    this overloaded method enables the usage of a different hostname
    (\a sslPeerName) for the certificate validation instead of
    the one used for the TCP connection (\a hostName).

    \sa connectToHostEncrypted()
*/
void QSslSocket::connectToHostEncrypted(const QString &hostName, quint16 port,
                                        const QString &sslPeerName, OpenMode mode,
                                        NetworkLayerProtocol protocol)
{
    Q_D(QSslSocket);
    if (d->state == ConnectedState || d->state == ConnectingState) {
        qCWarning(lcSsl,
                  "QSslSocket::connectToHostEncrypted() called when already connecting/connected");
        return;
    }

    if (!supportsSsl()) {
        qCWarning(lcSsl, "QSslSocket::connectToHostEncrypted: TLS initialization failed");
        d->setErrorAndEmit(QAbstractSocket::SslInternalError, tr("TLS initialization failed"));
        return;
    }

    d->init();
    d->autoStartHandshake = true;
    d->initialized = true;
    d->verificationPeerName = sslPeerName;

    // Note: When connecting to localhost, some platforms (e.g., HP-UX and some BSDs)
    // establish the connection immediately (i.e., first attempt).
    connectToHost(hostName, port, mode, protocol);
}

/*!
    Initializes QSslSocket with the native socket descriptor \a
    socketDescriptor. Returns \c true if \a socketDescriptor is accepted
    as a valid socket descriptor; otherwise returns \c false.
    The socket is opened in the mode specified by \a openMode, and
    enters the socket state specified by \a state.

    \note It is not possible to initialize two sockets with the same
    native socket descriptor.

    \sa socketDescriptor()
*/
bool QSslSocket::setSocketDescriptor(qintptr socketDescriptor, SocketState state, OpenMode openMode)
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::setSocketDescriptor(" << socketDescriptor << ','
             << state << ',' << openMode << ')';
#endif
    if (!d->plainSocket)
        d->createPlainSocket(openMode);
    bool retVal = d->plainSocket->setSocketDescriptor(socketDescriptor, state, openMode);
    d->cachedSocketDescriptor = d->plainSocket->socketDescriptor();
    d->setError(d->plainSocket->error(), d->plainSocket->errorString());
    setSocketState(state);
    setOpenMode(openMode);
    setLocalPort(d->plainSocket->localPort());
    setLocalAddress(d->plainSocket->localAddress());
    setPeerPort(d->plainSocket->peerPort());
    setPeerAddress(d->plainSocket->peerAddress());
    setPeerName(d->plainSocket->peerName());
    d->readChannelCount = d->plainSocket->readChannelCount();
    d->writeChannelCount = d->plainSocket->writeChannelCount();
    return retVal;
}

/*!
    \since 4.6
    Sets the given \a option to the value described by \a value.

    \sa socketOption()
*/
void QSslSocket::setSocketOption(QAbstractSocket::SocketOption option, const QVariant &value)
{
    Q_D(QSslSocket);
    if (d->plainSocket)
        d->plainSocket->setSocketOption(option, value);
}

/*!
    \since 4.6
    Returns the value of the \a option option.

    \sa setSocketOption()
*/
QVariant QSslSocket::socketOption(QAbstractSocket::SocketOption option)
{
    Q_D(QSslSocket);
    if (d->plainSocket)
        return d->plainSocket->socketOption(option);
    else
        return QVariant();
}

/*!
    Returns the current mode for the socket; either UnencryptedMode, where
    QSslSocket behaves identially to QTcpSocket, or one of SslClientMode or
    SslServerMode, where the client is either negotiating or in encrypted
    mode.

    When the mode changes, QSslSocket emits modeChanged()

    \sa SslMode
*/
QSslSocket::SslMode QSslSocket::mode() const
{
    Q_D(const QSslSocket);
    return d->mode;
}

/*!
    Returns \c true if the socket is encrypted; otherwise, false is returned.

    An encrypted socket encrypts all data that is written by calling write()
    or putChar() before the data is written to the network, and decrypts all
    incoming data as the data is received from the network, before you call
    read(), readLine() or getChar().

    QSslSocket emits encrypted() when it enters encrypted mode.

    You can call sessionCipher() to find which cryptographic cipher is used to
    encrypt and decrypt your data.

    \sa mode()
*/
bool QSslSocket::isEncrypted() const
{
    Q_D(const QSslSocket);
    return d->connectionEncrypted;
}

/*!
    Returns the socket's SSL protocol. By default, \l QSsl::SecureProtocols is used.

    \sa setProtocol()
*/
QSsl::SslProtocol QSslSocket::protocol() const
{
    Q_D(const QSslSocket);
    return d->configuration.protocol;
}

/*!
    Sets the socket's SSL protocol to \a protocol. This will affect the next
    initiated handshake; calling this function on an already-encrypted socket
    will not affect the socket's protocol.
*/
void QSslSocket::setProtocol(QSsl::SslProtocol protocol)
{
    Q_D(QSslSocket);
    d->configuration.protocol = protocol;
}

/*!
    \since 4.4

    Returns the socket's verify mode. This mode decides whether
    QSslSocket should request a certificate from the peer (i.e., the client
    requests a certificate from the server, or a server requesting a
    certificate from the client), and whether it should require that this
    certificate is valid.

    The default mode is AutoVerifyPeer, which tells QSslSocket to use
    VerifyPeer for clients and QueryPeer for servers.

    \sa setPeerVerifyMode(), peerVerifyDepth(), mode()
*/
QSslSocket::PeerVerifyMode QSslSocket::peerVerifyMode() const
{
    Q_D(const QSslSocket);
    return d->configuration.peerVerifyMode;
}

/*!
    \since 4.4

    Sets the socket's verify mode to \a mode. This mode decides whether
    QSslSocket should request a certificate from the peer (i.e., the client
    requests a certificate from the server, or a server requesting a
    certificate from the client), and whether it should require that this
    certificate is valid.

    The default mode is AutoVerifyPeer, which tells QSslSocket to use
    VerifyPeer for clients and QueryPeer for servers.

    Setting this mode after encryption has started has no effect on the
    current connection.

    \sa peerVerifyMode(), setPeerVerifyDepth(), mode()
*/
void QSslSocket::setPeerVerifyMode(QSslSocket::PeerVerifyMode mode)
{
    Q_D(QSslSocket);
    d->configuration.peerVerifyMode = mode;
}

/*!
    \since 4.4

    Returns the maximum number of certificates in the peer's certificate chain
    to be checked during the SSL handshake phase, or 0 (the default) if no
    maximum depth has been set, indicating that the whole certificate chain
    should be checked.

    The certificates are checked in issuing order, starting with the peer's
    own certificate, then its issuer's certificate, and so on.

    \sa setPeerVerifyDepth(), peerVerifyMode()
*/
int QSslSocket::peerVerifyDepth() const
{
    Q_D(const QSslSocket);
    return d->configuration.peerVerifyDepth;
}

/*!
    \since 4.4

    Sets the maximum number of certificates in the peer's certificate chain to
    be checked during the SSL handshake phase, to \a depth. Setting a depth of
    0 means that no maximum depth is set, indicating that the whole
    certificate chain should be checked.

    The certificates are checked in issuing order, starting with the peer's
    own certificate, then its issuer's certificate, and so on.

    \sa peerVerifyDepth(), setPeerVerifyMode()
*/
void QSslSocket::setPeerVerifyDepth(int depth)
{
    Q_D(QSslSocket);
    if (depth < 0) {
        qCWarning(lcSsl, "QSslSocket::setPeerVerifyDepth: cannot set negative depth of %d", depth);
        return;
    }
    d->configuration.peerVerifyDepth = depth;
}

/*!
    \since 4.8

    Returns the different hostname for the certificate validation, as set by
    setPeerVerifyName or by connectToHostEncrypted.

    \sa setPeerVerifyName(), connectToHostEncrypted()
*/
QString QSslSocket::peerVerifyName() const
{
    Q_D(const QSslSocket);
    return d->verificationPeerName;
}

/*!
    \since 4.8

    Sets a different host name, given by \a hostName, for the certificate
    validation instead of the one used for the TCP connection.

    \sa connectToHostEncrypted()
*/
void QSslSocket::setPeerVerifyName(const QString &hostName)
{
    Q_D(QSslSocket);
    d->verificationPeerName = hostName;
}

/*!
    \reimp

    Returns the number of decrypted bytes that are immediately available for
    reading.
*/
qint64 QSslSocket::bytesAvailable() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return QIODevice::bytesAvailable() + (d->plainSocket ? d->plainSocket->bytesAvailable() : 0);
    return QIODevice::bytesAvailable();
}

/*!
    \reimp

    Returns the number of unencrypted bytes that are waiting to be encrypted
    and written to the network.
*/
qint64 QSslSocket::bytesToWrite() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return d->plainSocket ? d->plainSocket->bytesToWrite() : 0;
    return d->writeBuffer.size();
}

/*!
    \since 4.4

    Returns the number of encrypted bytes that are awaiting decryption.
    Normally, this function will return 0 because QSslSocket decrypts its
    incoming data as soon as it can.
*/
qint64 QSslSocket::encryptedBytesAvailable() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return 0;
    return d->plainSocket->bytesAvailable();
}

/*!
    \since 4.4

    Returns the number of encrypted bytes that are waiting to be written to
    the network.
*/
qint64 QSslSocket::encryptedBytesToWrite() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return 0;
    return d->plainSocket->bytesToWrite();
}

/*!
    \reimp

    Returns \c true if you can read one while line (terminated by a single ASCII
    '\\n' character) of decrypted characters; otherwise, false is returned.
*/
bool QSslSocket::canReadLine() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return QIODevice::canReadLine() || (d->plainSocket && d->plainSocket->canReadLine());
    return QIODevice::canReadLine();
}

/*!
    \reimp
*/
void QSslSocket::close()
{
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::close()";
#endif
    Q_D(QSslSocket);
    if (encryptedBytesToWrite() || !d->writeBuffer.isEmpty())
        flush();
    if (d->plainSocket)
        d->plainSocket->close();
    QTcpSocket::close();

    // must be cleared, reading/writing not possible on closed socket:
    d->buffer.clear();
    d->writeBuffer.clear();
}

/*!
    \reimp
*/
bool QSslSocket::atEnd() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return QIODevice::atEnd() && (!d->plainSocket || d->plainSocket->atEnd());
    return QIODevice::atEnd();
}

/*!
    This function writes as much as possible from the internal write buffer to
    the underlying network socket, without blocking. If any data was written,
    this function returns \c true; otherwise false is returned.

    Call this function if you need QSslSocket to start sending buffered data
    immediately. The number of bytes successfully written depends on the
    operating system. In most cases, you do not need to call this function,
    because QAbstractSocket will start sending data automatically once control
    goes back to the event loop. In the absence of an event loop, call
    waitForBytesWritten() instead.

    \sa write(), waitForBytesWritten()
*/
bool QSslSocket::flush()
{
    return d_func()->flush();
}

/*!
    \since 4.4

    Sets the size of QSslSocket's internal read buffer to be \a size bytes.
*/
void QSslSocket::setReadBufferSize(qint64 size)
{
    Q_D(QSslSocket);
    d->readBufferMaxSize = size;

    if (d->plainSocket)
        d->plainSocket->setReadBufferSize(size);
}

/*!
    Aborts the current connection and resets the socket. Unlike
    disconnectFromHost(), this function immediately closes the socket,
    clearing any pending data in the write buffer.

    \sa disconnectFromHost(), close()
*/
void QSslSocket::abort()
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::abort()";
#endif
    if (d->plainSocket)
        d->plainSocket->abort();
    close();
}

/*!
    \since 4.4

    Returns the socket's SSL configuration state. The default SSL
    configuration of a socket is to use the default ciphers,
    default CA certificates, no local private key or certificate.

    The SSL configuration also contains fields that can change with
    time without notice.

    \sa localCertificate(), peerCertificate(), peerCertificateChain(),
        sessionCipher(), privateKey(), ciphers(), caCertificates()
*/
QSslConfiguration QSslSocket::sslConfiguration() const
{
    Q_D(const QSslSocket);

    // create a deep copy of our configuration
    QSslConfigurationPrivate *copy = new QSslConfigurationPrivate(d->configuration);
    copy->ref.store(0);              // the QSslConfiguration constructor refs up
    copy->sessionCipher = d->sessionCipher();
    copy->sessionProtocol = d->sessionProtocol();

    return QSslConfiguration(copy);
}

/*!
    \since 4.4

    Sets the socket's SSL configuration to be the contents of \a configuration.
    This function sets the local certificate, the ciphers, the private key and the CA
    certificates to those stored in \a configuration.

    It is not possible to set the SSL-state related fields.

    \sa setLocalCertificate(), setPrivateKey(), setCaCertificates(), setCiphers()
*/
void QSslSocket::setSslConfiguration(const QSslConfiguration &configuration)
{
    Q_D(QSslSocket);
    d->configuration.localCertificateChain = configuration.localCertificateChain();
    d->configuration.privateKey = configuration.privateKey();
    d->configuration.ciphers = configuration.ciphers();
    d->configuration.ellipticCurves = configuration.ellipticCurves();
    d->configuration.preSharedKeyIdentityHint = configuration.preSharedKeyIdentityHint();
    d->configuration.dhParams = configuration.diffieHellmanParameters();
    d->configuration.caCertificates = configuration.caCertificates();
    d->configuration.peerVerifyDepth = configuration.peerVerifyDepth();
    d->configuration.peerVerifyMode = configuration.peerVerifyMode();
    d->configuration.protocol = configuration.protocol();
    d->configuration.backendConfig = configuration.backendConfiguration();
    d->configuration.sslOptions = configuration.d->sslOptions;
    d->configuration.sslSession = configuration.sessionTicket();
    d->configuration.sslSessionTicketLifeTimeHint = configuration.sessionTicketLifeTimeHint();
    d->configuration.nextAllowedProtocols = configuration.allowedNextProtocols();
    d->configuration.nextNegotiatedProtocol = configuration.nextNegotiatedProtocol();
    d->configuration.nextProtocolNegotiationStatus = configuration.nextProtocolNegotiationStatus();

    // if the CA certificates were set explicitly (either via
    // QSslConfiguration::setCaCertificates() or QSslSocket::setCaCertificates(),
    // we cannot load the certificates on demand
    if (!configuration.d->allowRootCertOnDemandLoading)
        d->allowRootCertOnDemandLoading = false;
}

/*!
    Sets the certificate chain to be presented to the peer during the
    SSL handshake to be \a localChain.

    \sa QSslConfiguration::setLocalCertificateChain()
    \since 5.1
 */
void QSslSocket::setLocalCertificateChain(const QList<QSslCertificate> &localChain)
{
    Q_D(QSslSocket);
    d->configuration.localCertificateChain = localChain;
}

/*!
    Returns the socket's local \l {QSslCertificate} {certificate} chain,
    or an empty list if no local certificates have been assigned.

    \sa setLocalCertificateChain()
    \since 5.1
*/
QList<QSslCertificate> QSslSocket::localCertificateChain() const
{
    Q_D(const QSslSocket);
    return d->configuration.localCertificateChain;
}

/*!
    Sets the socket's local certificate to \a certificate. The local
    certificate is necessary if you need to confirm your identity to the
    peer. It is used together with the private key; if you set the local
    certificate, you must also set the private key.

    The local certificate and private key are always necessary for server
    sockets, but are also rarely used by client sockets if the server requires
    the client to authenticate.

    \note Secure Transport SSL backend on macOS may update the default keychain
    (the default is probably your login keychain) by importing your local certificates
    and keys. This can also result in system dialogs showing up and asking for
    permission when your application is using these private keys. If such behavior
    is undesired, set the QT_SSL_USE_TEMPORARY_KEYCHAIN environment variable to a
    non-zero value; this will prompt QSslSocket to use its own temporary keychain.

    \sa localCertificate(), setPrivateKey()
*/
void QSslSocket::setLocalCertificate(const QSslCertificate &certificate)
{
    Q_D(QSslSocket);
    d->configuration.localCertificateChain = QList<QSslCertificate>();
    d->configuration.localCertificateChain += certificate;
}

/*!
    \overload

    Sets the socket's local \l {QSslCertificate} {certificate} to the
    first one found in file \a path, which is parsed according to the
    specified \a format.
*/
void QSslSocket::setLocalCertificate(const QString &path,
                                     QSsl::EncodingFormat format)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        setLocalCertificate(QSslCertificate(file.readAll(), format));

}

/*!
    Returns the socket's local \l {QSslCertificate} {certificate}, or
    an empty certificate if no local certificate has been assigned.

    \sa setLocalCertificate(), privateKey()
*/
QSslCertificate QSslSocket::localCertificate() const
{
    Q_D(const QSslSocket);
    if (d->configuration.localCertificateChain.isEmpty())
        return QSslCertificate();
    return d->configuration.localCertificateChain[0];
}

/*!
    Returns the peer's digital certificate (i.e., the immediate
    certificate of the host you are connected to), or a null
    certificate, if the peer has not assigned a certificate.

    The peer certificate is checked automatically during the
    handshake phase, so this function is normally used to fetch
    the certificate for display or for connection diagnostic
    purposes. It contains information about the peer, including
    its host name, the certificate issuer, and the peer's public
    key.

    Because the peer certificate is set during the handshake phase, it
    is safe to access the peer certificate from a slot connected to
    the sslErrors() signal or the encrypted() signal.

    If a null certificate is returned, it can mean the SSL handshake
    failed, or it can mean the host you are connected to doesn't have
    a certificate, or it can mean there is no connection.

    If you want to check the peer's complete chain of certificates,
    use peerCertificateChain() to get them all at once.

    \sa peerCertificateChain()
*/
QSslCertificate QSslSocket::peerCertificate() const
{
    Q_D(const QSslSocket);
    return d->configuration.peerCertificate;
}

/*!
    Returns the peer's chain of digital certificates, or an empty list
    of certificates.

    Peer certificates are checked automatically during the handshake
    phase. This function is normally used to fetch certificates for
    display, or for performing connection diagnostics. Certificates
    contain information about the peer and the certificate issuers,
    including host name, issuer names, and issuer public keys.

    The peer certificates are set in QSslSocket during the handshake
    phase, so it is safe to call this function from a slot connected
    to the sslErrors() signal or the encrypted() signal.

    If an empty list is returned, it can mean the SSL handshake
    failed, or it can mean the host you are connected to doesn't have
    a certificate, or it can mean there is no connection.

    If you want to get only the peer's immediate certificate, use
    peerCertificate().

    \sa peerCertificate()
*/
QList<QSslCertificate> QSslSocket::peerCertificateChain() const
{
    Q_D(const QSslSocket);
    return d->configuration.peerCertificateChain;
}

/*!
    Returns the socket's cryptographic \l {QSslCipher} {cipher}, or a
    null cipher if the connection isn't encrypted. The socket's cipher
    for the session is set during the handshake phase. The cipher is
    used to encrypt and decrypt data transmitted through the socket.

    QSslSocket also provides functions for setting the ordered list of
    ciphers from which the handshake phase will eventually select the
    session cipher. This ordered list must be in place before the
    handshake phase begins.

    \sa ciphers(), setCiphers(), setDefaultCiphers(), defaultCiphers(),
    supportedCiphers()
*/
QSslCipher QSslSocket::sessionCipher() const
{
    Q_D(const QSslSocket);
    return d->sessionCipher();
}

/*!
    Returns the socket's SSL/TLS protocol or UnknownProtocol if the
    connection isn't encrypted. The socket's protocol for the session
    is set during the handshake phase.

    \sa protocol(), setProtocol()
    \since 5.4
*/
QSsl::SslProtocol QSslSocket::sessionProtocol() const
{
    Q_D(const QSslSocket);
    return d->sessionProtocol();
}


/*!
    Sets the socket's private \l {QSslKey} {key} to \a key. The
    private key and the local \l {QSslCertificate} {certificate} are
    used by clients and servers that must prove their identity to
    SSL peers.

    Both the key and the local certificate are required if you are
    creating an SSL server socket. If you are creating an SSL client
    socket, the key and local certificate are required if your client
    must identify itself to an SSL server.

    \sa privateKey(), setLocalCertificate()
*/
void QSslSocket::setPrivateKey(const QSslKey &key)
{
    Q_D(QSslSocket);
    d->configuration.privateKey = key;
}

/*!
    \overload

    Reads the string in file \a fileName and decodes it using
    a specified \a algorithm and encoding \a format to construct
    an \l {QSslKey} {SSL key}. If the encoded key is encrypted,
    \a passPhrase is used to decrypt it.

    The socket's private key is set to the constructed key. The
    private key and the local \l {QSslCertificate} {certificate} are
    used by clients and servers that must prove their identity to SSL
    peers.

    Both the key and the local certificate are required if you are
    creating an SSL server socket. If you are creating an SSL client
    socket, the key and local certificate are required if your client
    must identify itself to an SSL server.

    \sa privateKey(), setLocalCertificate()
*/
void QSslSocket::setPrivateKey(const QString &fileName, QSsl::KeyAlgorithm algorithm,
                               QSsl::EncodingFormat format, const QByteArray &passPhrase)
{
    Q_D(QSslSocket);
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        d->configuration.privateKey = QSslKey(file.readAll(), algorithm,
                                              format, QSsl::PrivateKey, passPhrase);
    }
}

/*!
    Returns this socket's private key.

    \sa setPrivateKey(), localCertificate()
*/
QSslKey QSslSocket::privateKey() const
{
    Q_D(const QSslSocket);
    return d->configuration.privateKey;
}

/*!
    \deprecated

    Use QSslConfiguration::ciphers() instead.

    Returns this socket's current cryptographic cipher suite. This
    list is used during the socket's handshake phase for choosing a
    session cipher. The returned list of ciphers is ordered by
    descending preference. (i.e., the first cipher in the list is the
    most preferred cipher). The session cipher will be the first one
    in the list that is also supported by the peer.

    By default, the handshake phase can choose any of the ciphers
    supported by this system's SSL libraries, which may vary from
    system to system. The list of ciphers supported by this system's
    SSL libraries is returned by supportedCiphers(). You can restrict
    the list of ciphers used for choosing the session cipher for this
    socket by calling setCiphers() with a subset of the supported
    ciphers. You can revert to using the entire set by calling
    setCiphers() with the list returned by supportedCiphers().

    You can restrict the list of ciphers used for choosing the session
    cipher for \e all sockets by calling setDefaultCiphers() with a
    subset of the supported ciphers. You can revert to using the
    entire set by calling setCiphers() with the list returned by
    supportedCiphers().

    \sa setCiphers(), defaultCiphers(), setDefaultCiphers(), supportedCiphers()
*/
QList<QSslCipher> QSslSocket::ciphers() const
{
    Q_D(const QSslSocket);
    return d->configuration.ciphers;
}

/*!
    \deprecated

    USe QSslConfiguration::setCiphers() instead.

    Sets the cryptographic cipher suite for this socket to \a ciphers,
    which must contain a subset of the ciphers in the list returned by
    supportedCiphers().

    Restricting the cipher suite must be done before the handshake
    phase, where the session cipher is chosen.

    \sa ciphers(), setDefaultCiphers(), supportedCiphers()
*/
void QSslSocket::setCiphers(const QList<QSslCipher> &ciphers)
{
    Q_D(QSslSocket);
    d->configuration.ciphers = ciphers;
}

/*!
    \deprecated

    Use QSslConfiguration::setCiphers() instead.

    Sets the cryptographic cipher suite for this socket to \a ciphers, which
    is a colon-separated list of cipher suite names. The ciphers are listed in
    order of preference, starting with the most preferred cipher. For example:

    \snippet code/src_network_ssl_qsslsocket.cpp 4

    Each cipher name in \a ciphers must be the name of a cipher in the
    list returned by supportedCiphers().  Restricting the cipher suite
    must be done before the handshake phase, where the session cipher
    is chosen.

    \sa ciphers(), setDefaultCiphers(), supportedCiphers()
*/
void QSslSocket::setCiphers(const QString &ciphers)
{
    Q_D(QSslSocket);
    d->configuration.ciphers.clear();
    const auto cipherNames = ciphers.split(QLatin1Char(':'), QString::SkipEmptyParts);
    for (const QString &cipherName : cipherNames) {
        QSslCipher cipher(cipherName);
        if (!cipher.isNull())
            d->configuration.ciphers << cipher;
    }
}

/*!
    \deprecated

    Use QSslConfiguration::setCiphers() on the default QSslConfiguration instead.

    Sets the default cryptographic cipher suite for all sockets in
    this application to \a ciphers, which must contain a subset of the
    ciphers in the list returned by supportedCiphers().

    Restricting the default cipher suite only affects SSL sockets
    that perform their handshake phase after the default cipher
    suite has been changed.

    \sa setCiphers(), defaultCiphers(), supportedCiphers()
*/
void QSslSocket::setDefaultCiphers(const QList<QSslCipher> &ciphers)
{
    QSslSocketPrivate::setDefaultCiphers(ciphers);
}

/*!
    \deprecated

    Use QSslConfiguration::ciphers() on the default QSslConfiguration instead.

    Returns the default cryptographic cipher suite for all sockets in
    this application. This list is used during the socket's handshake
    phase when negotiating with the peer to choose a session cipher.
    The list is ordered by preference (i.e., the first cipher in the
    list is the most preferred cipher).

    By default, the handshake phase can choose any of the ciphers
    supported by this system's SSL libraries, which may vary from
    system to system. The list of ciphers supported by this system's
    SSL libraries is returned by supportedCiphers().

    \sa supportedCiphers()
*/
QList<QSslCipher> QSslSocket::defaultCiphers()
{
    return QSslSocketPrivate::defaultCiphers();
}

/*!
    \deprecated

    Use QSslConfiguration::supportedCiphers() instead.

    Returns the list of cryptographic ciphers supported by this
    system. This list is set by the system's SSL libraries and may
    vary from system to system.

    \sa defaultCiphers(), ciphers(), setCiphers()
*/
QList<QSslCipher> QSslSocket::supportedCiphers()
{
    return QSslSocketPrivate::supportedCiphers();
}

/*!
  Searches all files in the \a path for certificates encoded in the
  specified \a format and adds them to this socket's CA certificate
  database. \a path must be a file or a pattern matching one or more
  files, as specified by \a syntax. Returns \c true if one or more
  certificates are added to the socket's CA certificate database;
  otherwise returns \c false.

  The CA certificate database is used by the socket during the
  handshake phase to validate the peer's certificate.

  For more precise control, use addCaCertificate().

  \sa addCaCertificate(), QSslCertificate::fromPath()
*/
bool QSslSocket::addCaCertificates(const QString &path, QSsl::EncodingFormat format,
                                   QRegExp::PatternSyntax syntax)
{
    Q_D(QSslSocket);
    QList<QSslCertificate> certs = QSslCertificate::fromPath(path, format, syntax);
    if (certs.isEmpty())
        return false;

    d->configuration.caCertificates += certs;
    return true;
}

/*!
  Adds the \a certificate to this socket's CA certificate database.
  The CA certificate database is used by the socket during the
  handshake phase to validate the peer's certificate.

  To add multiple certificates, use addCaCertificates().

  \sa caCertificates(), setCaCertificates()
*/
void QSslSocket::addCaCertificate(const QSslCertificate &certificate)
{
    Q_D(QSslSocket);
    d->configuration.caCertificates += certificate;
}

/*!
  Adds the \a certificates to this socket's CA certificate database.
  The CA certificate database is used by the socket during the
  handshake phase to validate the peer's certificate.

  For more precise control, use addCaCertificate().

  \sa caCertificates(), addDefaultCaCertificate()
*/
void QSslSocket::addCaCertificates(const QList<QSslCertificate> &certificates)
{
    Q_D(QSslSocket);
    d->configuration.caCertificates += certificates;
}

/*!
  \deprecated

  Use QSslConfiguration::setCaCertificates() instead.

  Sets this socket's CA certificate database to be \a certificates.
  The certificate database must be set prior to the SSL handshake.
  The CA certificate database is used by the socket during the
  handshake phase to validate the peer's certificate.

  The CA certificate database can be reset to the current default CA
  certificate database by calling this function with the list of CA
  certificates returned by defaultCaCertificates().

  \sa defaultCaCertificates()
*/
void QSslSocket::setCaCertificates(const QList<QSslCertificate> &certificates)
{
    Q_D(QSslSocket);
    d->configuration.caCertificates = certificates;
    d->allowRootCertOnDemandLoading = false;
}

/*!
  \deprecated

  Use QSslConfiguration::caCertificates() instead.

  Returns this socket's CA certificate database. The CA certificate
  database is used by the socket during the handshake phase to
  validate the peer's certificate. It can be moodified prior to the
  handshake with addCaCertificate(), addCaCertificates(), and
  setCaCertificates().

  \note On Unix, this method may return an empty list if the root
  certificates are loaded on demand.

  \sa addCaCertificate(), addCaCertificates(), setCaCertificates()
*/
QList<QSslCertificate> QSslSocket::caCertificates() const
{
    Q_D(const QSslSocket);
    return d->configuration.caCertificates;
}

/*!
    Searches all files in the \a path for certificates with the
    specified \a encoding and adds them to the default CA certificate
    database. \a path can be an explicit file, or it can contain
    wildcards in the format specified by \a syntax. Returns \c true if
    any CA certificates are added to the default database.

    Each SSL socket's CA certificate database is initialized to the
    default CA certificate database.

    \sa defaultCaCertificates(), addCaCertificates(), addDefaultCaCertificate()
*/
bool QSslSocket::addDefaultCaCertificates(const QString &path, QSsl::EncodingFormat encoding,
                                          QRegExp::PatternSyntax syntax)
{
    return QSslSocketPrivate::addDefaultCaCertificates(path, encoding, syntax);
}

/*!
    Adds \a certificate to the default CA certificate database.  Each
    SSL socket's CA certificate database is initialized to the default
    CA certificate database.

    \sa defaultCaCertificates(), addCaCertificates()
*/
void QSslSocket::addDefaultCaCertificate(const QSslCertificate &certificate)
{
    QSslSocketPrivate::addDefaultCaCertificate(certificate);
}

/*!
    Adds \a certificates to the default CA certificate database.  Each
    SSL socket's CA certificate database is initialized to the default
    CA certificate database.

    \sa defaultCaCertificates(), addCaCertificates()
*/
void QSslSocket::addDefaultCaCertificates(const QList<QSslCertificate> &certificates)
{
    QSslSocketPrivate::addDefaultCaCertificates(certificates);
}

/*!
    \deprecated

    Use QSslConfiguration::setCaCertificates() on the default QSslConfiguration instead.

    Sets the default CA certificate database to \a certificates. The
    default CA certificate database is originally set to your system's
    default CA certificate database. You can override the default CA
    certificate database with your own CA certificate database using
    this function.

    Each SSL socket's CA certificate database is initialized to the
    default CA certificate database.

    \sa addDefaultCaCertificate()
*/
void QSslSocket::setDefaultCaCertificates(const QList<QSslCertificate> &certificates)
{
    QSslSocketPrivate::setDefaultCaCertificates(certificates);
}

/*!
    \deprecated

    Use QSslConfiguration::caCertificates() on the default QSslConfiguration instead.

    Returns the current default CA certificate database. This database
    is originally set to your system's default CA certificate database.
    If no system default database is found, an empty database will be
    returned. You can override the default CA certificate database
    with your own CA certificate database using setDefaultCaCertificates().

    Each SSL socket's CA certificate database is initialized to the
    default CA certificate database.

    \note On Unix, this method may return an empty list if the root
    certificates are loaded on demand.

    \sa caCertificates()
*/
QList<QSslCertificate> QSslSocket::defaultCaCertificates()
{
    return QSslSocketPrivate::defaultCaCertificates();
}

/*!
    \deprecated

    Use QSslConfiguration::systemDefaultCaCertificates instead.

    This function provides the CA certificate database
    provided by the operating system. The CA certificate database
    returned by this function is used to initialize the database
    returned by defaultCaCertificates(). You can replace that database
    with your own with setDefaultCaCertificates().

    \note: On OS X, only certificates that are either trusted for all
    purposes or trusted for the purpose of SSL in the keychain will be
    returned.

    \sa caCertificates(), defaultCaCertificates(), setDefaultCaCertificates()
*/
QList<QSslCertificate> QSslSocket::systemCaCertificates()
{
    // we are calling ensureInitialized() in the method below
    return QSslSocketPrivate::systemCaCertificates();
}

/*!
    Waits until the socket is connected, or \a msecs milliseconds,
    whichever happens first. If the connection has been established,
    this function returns \c true; otherwise it returns \c false.

    \sa QAbstractSocket::waitForConnected()
*/
bool QSslSocket::waitForConnected(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket)
        return false;
    bool retVal = d->plainSocket->waitForConnected(msecs);
    if (!retVal) {
        setSocketState(d->plainSocket->state());
        d->setError(d->plainSocket->error(), d->plainSocket->errorString());
    }
    return retVal;
}

/*!
    Waits until the socket has completed the SSL handshake and has
    emitted encrypted(), or \a msecs milliseconds, whichever comes
    first. If encrypted() has been emitted, this function returns
    true; otherwise (e.g., the socket is disconnected, or the SSL
    handshake fails), false is returned.

    The following example waits up to one second for the socket to be
    encrypted:

    \snippet code/src_network_ssl_qsslsocket.cpp 5

    If msecs is -1, this function will not time out.

    \sa startClientEncryption(), startServerEncryption(), encrypted(), isEncrypted()
*/
bool QSslSocket::waitForEncrypted(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket || d->connectionEncrypted)
        return false;
    if (d->mode == UnencryptedMode && !d->autoStartHandshake)
        return false;

    QElapsedTimer stopWatch;
    stopWatch.start();

    if (d->plainSocket->state() != QAbstractSocket::ConnectedState) {
        // Wait until we've entered connected state.
        if (!d->plainSocket->waitForConnected(msecs))
            return false;
    }

    while (!d->connectionEncrypted) {
        // Start the handshake, if this hasn't been started yet.
        if (d->mode == UnencryptedMode)
            startClientEncryption();
        // Loop, waiting until the connection has been encrypted or an error
        // occurs.
        if (!d->plainSocket->waitForReadyRead(qt_subtract_from_timeout(msecs, stopWatch.elapsed())))
            return false;
    }
    return d->connectionEncrypted;
}

/*!
    \reimp
*/
bool QSslSocket::waitForReadyRead(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket)
        return false;
    if (d->mode == UnencryptedMode && !d->autoStartHandshake)
        return d->plainSocket->waitForReadyRead(msecs);

    // This function must return true if and only if readyRead() *was* emitted.
    // So we initialize "readyReadEmitted" to false and check if it was set to true.
    // waitForReadyRead() could be called recursively, so we can't use the same variable
    // (the inner waitForReadyRead() may fail, but the outer one still succeeded)
    bool readyReadEmitted = false;
    bool *previousReadyReadEmittedPointer = d->readyReadEmittedPointer;
    d->readyReadEmittedPointer = &readyReadEmitted;

    QElapsedTimer stopWatch;
    stopWatch.start();

    if (!d->connectionEncrypted) {
        // Wait until we've entered encrypted mode, or until a failure occurs.
        if (!waitForEncrypted(msecs)) {
            d->readyReadEmittedPointer = previousReadyReadEmittedPointer;
            return false;
        }
    }

    if (!d->writeBuffer.isEmpty()) {
        // empty our cleartext write buffer first
        d->transmit();
    }

    // test readyReadEmitted first because either operation above
    // (waitForEncrypted or transmit) may have set it
    while (!readyReadEmitted &&
           d->plainSocket->waitForReadyRead(qt_subtract_from_timeout(msecs, stopWatch.elapsed()))) {
    }

    d->readyReadEmittedPointer = previousReadyReadEmittedPointer;
    return readyReadEmitted;
}

/*!
    \reimp
*/
bool QSslSocket::waitForBytesWritten(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket)
        return false;
    if (d->mode == UnencryptedMode)
        return d->plainSocket->waitForBytesWritten(msecs);

    QElapsedTimer stopWatch;
    stopWatch.start();

    if (!d->connectionEncrypted) {
        // Wait until we've entered encrypted mode, or until a failure occurs.
        if (!waitForEncrypted(msecs))
            return false;
    }
    if (!d->writeBuffer.isEmpty()) {
        // empty our cleartext write buffer first
        d->transmit();
    }

    return d->plainSocket->waitForBytesWritten(qt_subtract_from_timeout(msecs, stopWatch.elapsed()));
}

/*!
    Waits until the socket has disconnected or \a msecs milliseconds,
    whichever comes first. If the connection has been disconnected,
    this function returns \c true; otherwise it returns \c false.

    \sa QAbstractSocket::waitForDisconnected()
*/
bool QSslSocket::waitForDisconnected(int msecs)
{
    Q_D(QSslSocket);

    // require calling connectToHost() before waitForDisconnected()
    if (state() == UnconnectedState) {
        qCWarning(lcSsl, "QSslSocket::waitForDisconnected() is not allowed in UnconnectedState");
        return false;
    }

    if (!d->plainSocket)
        return false;
    // Forward to the plain socket unless the connection is secure.
    if (d->mode == UnencryptedMode && !d->autoStartHandshake)
        return d->plainSocket->waitForDisconnected(msecs);

    QElapsedTimer stopWatch;
    stopWatch.start();

    if (!d->connectionEncrypted) {
        // Wait until we've entered encrypted mode, or until a failure occurs.
        if (!waitForEncrypted(msecs))
            return false;
    }
    // We are delaying the disconnect, if the write buffer is not empty.
    // So, start the transmission.
    if (!d->writeBuffer.isEmpty())
        d->transmit();

    // At this point, the socket might be disconnected, if disconnectFromHost()
    // was called just after the connectToHostEncrypted() call. Also, we can
    // lose the connection as a result of the transmit() call.
    if (state() == UnconnectedState)
        return true;

    bool retVal = d->plainSocket->waitForDisconnected(qt_subtract_from_timeout(msecs, stopWatch.elapsed()));
    if (!retVal) {
        setSocketState(d->plainSocket->state());
        d->setError(d->plainSocket->error(), d->plainSocket->errorString());
    }
    return retVal;
}

/*!
    Returns a list of the last SSL errors that occurred. This is the
    same list as QSslSocket passes via the sslErrors() signal. If the
    connection has been encrypted with no errors, this function will
    return an empty list.

    \sa connectToHostEncrypted()
*/
QList<QSslError> QSslSocket::sslErrors() const
{
    Q_D(const QSslSocket);
    return d->sslErrors;
}

/*!
    Returns \c true if this platform supports SSL; otherwise, returns
    false. If the platform doesn't support SSL, the socket will fail
    in the connection phase.
*/
bool QSslSocket::supportsSsl()
{
    return QSslSocketPrivate::supportsSsl();
}

/*!
    \since 5.0
    Returns the version number of the SSL library in use. Note that
    this is the version of the library in use at run-time not compile
    time. If no SSL support is available then this will return an
    undefined value.
*/
long QSslSocket::sslLibraryVersionNumber()
{
    return QSslSocketPrivate::sslLibraryVersionNumber();
}

/*!
    \since 5.0
    Returns the version string of the SSL library in use. Note that
    this is the version of the library in use at run-time not compile
    time. If no SSL support is available then this will return an empty value.
*/
QString QSslSocket::sslLibraryVersionString()
{
    return QSslSocketPrivate::sslLibraryVersionString();
}

/*!
    \since 5.4
    Returns the version number of the SSL library in use at compile
    time. If no SSL support is available then this will return an
    undefined value.

    \sa sslLibraryVersionNumber()
*/
long QSslSocket::sslLibraryBuildVersionNumber()
{
    return QSslSocketPrivate::sslLibraryBuildVersionNumber();
}

/*!
    \since 5.4
    Returns the version string of the SSL library in use at compile
    time. If no SSL support is available then this will return an
    empty value.

    \sa sslLibraryVersionString()
*/
QString QSslSocket::sslLibraryBuildVersionString()
{
    return QSslSocketPrivate::sslLibraryBuildVersionString();
}

/*!
    Starts a delayed SSL handshake for a client connection. This
    function can be called when the socket is in the \l ConnectedState
    but still in the \l UnencryptedMode. If it is not yet connected,
    or if it is already encrypted, this function has no effect.

    Clients that implement STARTTLS functionality often make use of
    delayed SSL handshakes. Most other clients can avoid calling this
    function directly by using connectToHostEncrypted() instead, which
    automatically performs the handshake.

    \sa connectToHostEncrypted(), startServerEncryption()
*/
void QSslSocket::startClientEncryption()
{
    Q_D(QSslSocket);
    if (d->mode != UnencryptedMode) {
        qCWarning(lcSsl,
                  "QSslSocket::startClientEncryption: cannot start handshake on non-plain connection");
        return;
    }
    if (state() != ConnectedState) {
        qCWarning(lcSsl,
                  "QSslSocket::startClientEncryption: cannot start handshake when not connected");
        return;
    }

    if (!supportsSsl()) {
        qCWarning(lcSsl, "QSslSocket::startClientEncryption: TLS initialization failed");
        d->setErrorAndEmit(QAbstractSocket::SslInternalError, tr("TLS initialization failed"));
        return;
    }
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::startClientEncryption()";
#endif
    d->mode = SslClientMode;
    emit modeChanged(d->mode);
    d->startClientEncryption();
}

/*!
    Starts a delayed SSL handshake for a server connection. This
    function can be called when the socket is in the \l ConnectedState
    but still in \l UnencryptedMode. If it is not connected or it is
    already encrypted, the function has no effect.

    For server sockets, calling this function is the only way to
    initiate the SSL handshake. Most servers will call this function
    immediately upon receiving a connection, or as a result of having
    received a protocol-specific command to enter SSL mode (e.g, the
    server may respond to receiving the string "STARTTLS\\r\\n" by
    calling this function).

    The most common way to implement an SSL server is to create a
    subclass of QTcpServer and reimplement
    QTcpServer::incomingConnection(). The returned socket descriptor
    is then passed to QSslSocket::setSocketDescriptor().

    \sa connectToHostEncrypted(), startClientEncryption()
*/
void QSslSocket::startServerEncryption()
{
    Q_D(QSslSocket);
    if (d->mode != UnencryptedMode) {
        qCWarning(lcSsl, "QSslSocket::startServerEncryption: cannot start handshake on non-plain connection");
        return;
    }
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::startServerEncryption()";
#endif
    if (!supportsSsl()) {
        qCWarning(lcSsl, "QSslSocket::startServerEncryption: TLS initialization failed");
        d->setErrorAndEmit(QAbstractSocket::SslInternalError, tr("TLS initialization failed"));
        return;
    }
    d->mode = SslServerMode;
    emit modeChanged(d->mode);
    d->startServerEncryption();
}

/*!
    This slot tells QSslSocket to ignore errors during QSslSocket's
    handshake phase and continue connecting. If you want to continue
    with the connection even if errors occur during the handshake
    phase, then you must call this slot, either from a slot connected
    to sslErrors(), or before the handshake phase. If you don't call
    this slot, either in response to errors or before the handshake,
    the connection will be dropped after the sslErrors() signal has
    been emitted.

    If there are no errors during the SSL handshake phase (i.e., the
    identity of the peer is established with no problems), QSslSocket
    will not emit the sslErrors() signal, and it is unnecessary to
    call this function.

    \warning Be sure to always let the user inspect the errors
    reported by the sslErrors() signal, and only call this method
    upon confirmation from the user that proceeding is ok.
    If there are unexpected errors, the connection should be aborted.
    Calling this method without inspecting the actual errors will
    most likely pose a security risk for your application. Use it
    with great care!

    \sa sslErrors()
*/
void QSslSocket::ignoreSslErrors()
{
    Q_D(QSslSocket);
    d->ignoreAllSslErrors = true;
}

/*!
    \overload
    \since 4.6

    This method tells QSslSocket to ignore only the errors given in \a
    errors.

    \note Because most SSL errors are associated with a certificate, for most
    of them you must set the expected certificate this SSL error is related to.
    If, for instance, you want to connect to a server that uses
    a self-signed certificate, consider the following snippet:

    \snippet code/src_network_ssl_qsslsocket.cpp 6

    Multiple calls to this function will replace the list of errors that
    were passed in previous calls.
    You can clear the list of errors you want to ignore by calling this
    function with an empty list.

    \sa sslErrors()
*/
void QSslSocket::ignoreSslErrors(const QList<QSslError> &errors)
{
    Q_D(QSslSocket);
    d->ignoreErrorsList = errors;
}

/*!
    \internal
*/
void QSslSocket::connectToHost(const QString &hostName, quint16 port, OpenMode openMode, NetworkLayerProtocol protocol)
{
    Q_D(QSslSocket);
    d->preferredNetworkLayerProtocol = protocol;
    if (!d->initialized)
        d->init();
    d->initialized = false;

#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::connectToHost("
             << hostName << ',' << port << ',' << openMode << ')';
#endif
    if (!d->plainSocket) {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << "\tcreating internal plain socket";
#endif
        d->createPlainSocket(openMode);
    }
#ifndef QT_NO_NETWORKPROXY
    d->plainSocket->setProxy(proxy());
#endif
    QIODevice::open(openMode);
    d->readChannelCount = d->writeChannelCount = 0;
    d->plainSocket->connectToHost(hostName, port, openMode, d->preferredNetworkLayerProtocol);
    d->cachedSocketDescriptor = d->plainSocket->socketDescriptor();
}

/*!
    \internal
*/
void QSslSocket::disconnectFromHost()
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::disconnectFromHost()";
#endif
    if (!d->plainSocket)
        return;
    if (d->state == UnconnectedState)
        return;
    if (d->mode == UnencryptedMode && !d->autoStartHandshake) {
        d->plainSocket->disconnectFromHost();
        return;
    }
    if (d->state <= ConnectingState) {
        d->pendingClose = true;
        return;
    }

    // Perhaps emit closing()
    if (d->state != ClosingState) {
        d->state = ClosingState;
        emit stateChanged(d->state);
    }

    if (!d->writeBuffer.isEmpty()) {
        d->pendingClose = true;
        return;
    }

    if (d->mode == UnencryptedMode) {
        d->plainSocket->disconnectFromHost();
    } else {
        d->disconnectFromHost();
    }
}

/*!
    \reimp
*/
qint64 QSslSocket::readData(char *data, qint64 maxlen)
{
    Q_D(QSslSocket);
    qint64 readBytes = 0;

    if (d->mode == UnencryptedMode && !d->autoStartHandshake) {
        readBytes = d->plainSocket->read(data, maxlen);
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << "QSslSocket::readData(" << (void *)data << ',' << maxlen << ") =="
                 << readBytes;
#endif
    } else {
        // possibly trigger another transmit() to decrypt more data from the socket
        if (d->plainSocket->bytesAvailable())
            QMetaObject::invokeMethod(this, "_q_flushReadBuffer", Qt::QueuedConnection);
        else if (d->state != QAbstractSocket::ConnectedState)
            return maxlen ? qint64(-1) : qint64(0);
    }

    return readBytes;
}

/*!
    \reimp
*/
qint64 QSslSocket::writeData(const char *data, qint64 len)
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::writeData(" << (void *)data << ',' << len << ')';
#endif
    if (d->mode == UnencryptedMode && !d->autoStartHandshake)
        return d->plainSocket->write(data, len);

    d->writeBuffer.append(data, len);

    // make sure we flush to the plain socket's buffer
    if (!d->flushTriggered) {
        d->flushTriggered = true;
        QMetaObject::invokeMethod(this, "_q_flushWriteBuffer", Qt::QueuedConnection);
    }

    return len;
}

/*!
    \internal
*/
QSslSocketPrivate::QSslSocketPrivate()
    : initialized(false)
    , mode(QSslSocket::UnencryptedMode)
    , autoStartHandshake(false)
    , connectionEncrypted(false)
    , shutdown(false)
    , ignoreAllSslErrors(false)
    , readyReadEmittedPointer(nullptr)
    , allowRootCertOnDemandLoading(true)
    , plainSocket(nullptr)
    , paused(false)
    , flushTriggered(false)
{
    QSslConfigurationPrivate::deepCopyDefaultConfiguration(&configuration);
}

/*!
    \internal
*/
QSslSocketPrivate::~QSslSocketPrivate()
{
}

/*!
    \internal
*/
void QSslSocketPrivate::init()
{
    mode = QSslSocket::UnencryptedMode;
    autoStartHandshake = false;
    connectionEncrypted = false;
    ignoreAllSslErrors = false;
    shutdown = false;
    pendingClose = false;
    flushTriggered = false;

    // we don't want to clear the ignoreErrorsList, so
    // that it is possible setting it before connecting
//    ignoreErrorsList.clear();

    buffer.clear();
    writeBuffer.clear();
    configuration.peerCertificate.clear();
    configuration.peerCertificateChain.clear();
}

/*!
    \internal
*/
QList<QSslCipher> QSslSocketPrivate::defaultCiphers()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return globalData()->config->ciphers;
}

/*!
    \internal
*/
QList<QSslCipher> QSslSocketPrivate::supportedCiphers()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return globalData()->supportedCiphers;
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultCiphers(const QList<QSslCipher> &ciphers)
{
    QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->config->ciphers = ciphers;
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultSupportedCiphers(const QList<QSslCipher> &ciphers)
{
    QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->supportedCiphers = ciphers;
}

/*!
    \internal
*/
void q_setDefaultDtlsCiphers(const QList<QSslCipher> &ciphers)
{
    QMutexLocker locker(&globalData()->mutex);
    globalData()->dtlsConfig.detach();
    globalData()->dtlsConfig->ciphers = ciphers;
}

/*!
    \internal
*/
QList<QSslCipher> q_getDefaultDtlsCiphers()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return globalData()->dtlsConfig->ciphers;
}

/*!
    \internal
*/
QVector<QSslEllipticCurve> QSslSocketPrivate::supportedEllipticCurves()
{
    QSslSocketPrivate::ensureInitialized();
    const QMutexLocker locker(&globalData()->mutex);
    return globalData()->supportedEllipticCurves;
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultSupportedEllipticCurves(const QVector<QSslEllipticCurve> &curves)
{
    const QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->dtlsConfig.detach();
    globalData()->supportedEllipticCurves = curves;
}

/*!
    \internal
*/
QList<QSslCertificate> QSslSocketPrivate::defaultCaCertificates()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return globalData()->config->caCertificates;
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultCaCertificates(const QList<QSslCertificate> &certs)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->config->caCertificates = certs;
    globalData()->dtlsConfig.detach();
    globalData()->dtlsConfig->caCertificates = certs;
    // when the certificates are set explicitly, we do not want to
    // load the system certificates on demand
    s_loadRootCertsOnDemand = false;
}

/*!
    \internal
*/
bool QSslSocketPrivate::addDefaultCaCertificates(const QString &path, QSsl::EncodingFormat format,
                                                 QRegExp::PatternSyntax syntax)
{
    QSslSocketPrivate::ensureInitialized();
    QList<QSslCertificate> certs = QSslCertificate::fromPath(path, format, syntax);
    if (certs.isEmpty())
        return false;

    QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->config->caCertificates += certs;
    globalData()->dtlsConfig.detach();
    globalData()->dtlsConfig->caCertificates += certs;
    return true;
}

/*!
    \internal
*/
void QSslSocketPrivate::addDefaultCaCertificate(const QSslCertificate &cert)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->config->caCertificates += cert;
    globalData()->dtlsConfig.detach();
    globalData()->dtlsConfig->caCertificates += cert;
}

/*!
    \internal
*/
void QSslSocketPrivate::addDefaultCaCertificates(const QList<QSslCertificate> &certs)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    globalData()->config.detach();
    globalData()->config->caCertificates += certs;
    globalData()->dtlsConfig.detach();
    globalData()->dtlsConfig->caCertificates += certs;
}

/*!
    \internal
*/
QSslConfiguration QSslConfigurationPrivate::defaultConfiguration()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return QSslConfiguration(globalData()->config.data());
}

/*!
    \internal
*/
void QSslConfigurationPrivate::setDefaultConfiguration(const QSslConfiguration &configuration)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    if (globalData()->config == configuration.d)
        return;                 // nothing to do

    globalData()->config = const_cast<QSslConfigurationPrivate*>(configuration.d.constData());
}

/*!
    \internal
*/
void QSslConfigurationPrivate::deepCopyDefaultConfiguration(QSslConfigurationPrivate *ptr)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    const QSslConfigurationPrivate *global = globalData()->config.constData();

    if (!global)
        return;

    ptr->ref.store(1);
    ptr->peerCertificate = global->peerCertificate;
    ptr->peerCertificateChain = global->peerCertificateChain;
    ptr->localCertificateChain = global->localCertificateChain;
    ptr->privateKey = global->privateKey;
    ptr->sessionCipher = global->sessionCipher;
    ptr->sessionProtocol = global->sessionProtocol;
    ptr->ciphers = global->ciphers;
    ptr->caCertificates = global->caCertificates;
    ptr->protocol = global->protocol;
    ptr->peerVerifyMode = global->peerVerifyMode;
    ptr->peerVerifyDepth = global->peerVerifyDepth;
    ptr->sslOptions = global->sslOptions;
    ptr->ellipticCurves = global->ellipticCurves;
    ptr->backendConfig = global->backendConfig;
#if QT_CONFIG(dtls)
    ptr->dtlsCookieEnabled = global->dtlsCookieEnabled;
#endif
}

/*!
    \internal
*/
QSslConfiguration QSslConfigurationPrivate::defaultDtlsConfiguration()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);

    return QSslConfiguration(globalData()->dtlsConfig.data());
}

/*!
    \internal
*/
void QSslConfigurationPrivate::setDefaultDtlsConfiguration(const QSslConfiguration &configuration)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    if (globalData()->dtlsConfig == configuration.d)
        return;                 // nothing to do

    globalData()->dtlsConfig = const_cast<QSslConfigurationPrivate*>(configuration.d.constData());
}

/*!
    \internal
*/
void QSslSocketPrivate::createPlainSocket(QIODevice::OpenMode openMode)
{
    Q_Q(QSslSocket);
    q->setOpenMode(openMode); // <- from QIODevice
    q->setSocketState(QAbstractSocket::UnconnectedState);
    q->setSocketError(QAbstractSocket::UnknownSocketError);
    q->setLocalPort(0);
    q->setLocalAddress(QHostAddress());
    q->setPeerPort(0);
    q->setPeerAddress(QHostAddress());
    q->setPeerName(QString());

    plainSocket = new QTcpSocket(q);
#ifndef QT_NO_BEARERMANAGEMENT
    //copy network session down to the plain socket (if it has been set)
    plainSocket->setProperty("_q_networksession", q->property("_q_networksession"));
#endif
    q->connect(plainSocket, SIGNAL(connected()),
               q, SLOT(_q_connectedSlot()),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(hostFound()),
               q, SLOT(_q_hostFoundSlot()),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(disconnected()),
               q, SLOT(_q_disconnectedSlot()),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
               q, SLOT(_q_stateChangedSlot(QAbstractSocket::SocketState)),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(error(QAbstractSocket::SocketError)),
               q, SLOT(_q_errorSlot(QAbstractSocket::SocketError)),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(readyRead()),
               q, SLOT(_q_readyReadSlot()),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(channelReadyRead(int)),
               q, SLOT(_q_channelReadyReadSlot(int)),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(bytesWritten(qint64)),
               q, SLOT(_q_bytesWrittenSlot(qint64)),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(channelBytesWritten(int, qint64)),
               q, SLOT(_q_channelBytesWrittenSlot(int, qint64)),
               Qt::DirectConnection);
    q->connect(plainSocket, SIGNAL(readChannelFinished()),
               q, SLOT(_q_readChannelFinishedSlot()),
               Qt::DirectConnection);
#ifndef QT_NO_NETWORKPROXY
    q->connect(plainSocket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
               q, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));
#endif

    buffer.clear();
    writeBuffer.clear();
    connectionEncrypted = false;
    configuration.peerCertificate.clear();
    configuration.peerCertificateChain.clear();
    mode = QSslSocket::UnencryptedMode;
    q->setReadBufferSize(readBufferMaxSize);
}

void QSslSocketPrivate::pauseSocketNotifiers(QSslSocket *socket)
{
    if (!socket->d_func()->plainSocket)
        return;
    QAbstractSocketPrivate::pauseSocketNotifiers(socket->d_func()->plainSocket);
}

void QSslSocketPrivate::resumeSocketNotifiers(QSslSocket *socket)
{
    if (!socket->d_func()->plainSocket)
        return;
    QAbstractSocketPrivate::resumeSocketNotifiers(socket->d_func()->plainSocket);
}

bool QSslSocketPrivate::isPaused() const
{
    return paused;
}

bool QSslSocketPrivate::bind(const QHostAddress &address, quint16 port, QAbstractSocket::BindMode mode)
{
    // this function is called from QAbstractSocket::bind
    if (!initialized)
        init();
    initialized = false;

#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::bind(" << address << ',' << port << ',' << mode << ')';
#endif
    if (!plainSocket) {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << "\tcreating internal plain socket";
#endif
        createPlainSocket(QIODevice::ReadWrite);
    }
    bool ret = plainSocket->bind(address, port, mode);
    localPort = plainSocket->localPort();
    localAddress = plainSocket->localAddress();
    cachedSocketDescriptor = plainSocket->socketDescriptor();
    readChannelCount = writeChannelCount = 0;
    return ret;
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_connectedSlot()
{
    Q_Q(QSslSocket);
    q->setLocalPort(plainSocket->localPort());
    q->setLocalAddress(plainSocket->localAddress());
    q->setPeerPort(plainSocket->peerPort());
    q->setPeerAddress(plainSocket->peerAddress());
    q->setPeerName(plainSocket->peerName());
    cachedSocketDescriptor = plainSocket->socketDescriptor();
    readChannelCount = plainSocket->readChannelCount();
    writeChannelCount = plainSocket->writeChannelCount();

#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_connectedSlot()";
    qCDebug(lcSsl) << "\tstate =" << q->state();
    qCDebug(lcSsl) << "\tpeer =" << q->peerName() << q->peerAddress() << q->peerPort();
    qCDebug(lcSsl) << "\tlocal =" << QHostInfo::fromName(q->localAddress().toString()).hostName()
             << q->localAddress() << q->localPort();
#endif

    if (autoStartHandshake)
        q->startClientEncryption();

    emit q->connected();

    if (pendingClose && !autoStartHandshake) {
        pendingClose = false;
        q->disconnectFromHost();
    }
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_hostFoundSlot()
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_hostFoundSlot()";
    qCDebug(lcSsl) << "\tstate =" << q->state();
#endif
    emit q->hostFound();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_disconnectedSlot()
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_disconnectedSlot()";
    qCDebug(lcSsl) << "\tstate =" << q->state();
#endif
    disconnected();
    emit q->disconnected();

    q->setLocalPort(0);
    q->setLocalAddress(QHostAddress());
    q->setPeerPort(0);
    q->setPeerAddress(QHostAddress());
    q->setPeerName(QString());
    cachedSocketDescriptor = -1;
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_stateChangedSlot(QAbstractSocket::SocketState state)
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_stateChangedSlot(" << state << ')';
#endif
    q->setSocketState(state);
    emit q->stateChanged(state);
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_errorSlot(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
#ifdef QSSLSOCKET_DEBUG
    Q_Q(QSslSocket);
    qCDebug(lcSsl) << "QSslSocket::_q_errorSlot(" << error << ')';
    qCDebug(lcSsl) << "\tstate =" << q->state();
    qCDebug(lcSsl) << "\terrorString =" << q->errorString();
#endif
    // this moves encrypted bytes from plain socket into our buffer
    if (plainSocket->bytesAvailable()) {
        qint64 tmpReadBufferMaxSize = readBufferMaxSize;
        readBufferMaxSize = 0; // reset temporarily so the plain sockets completely drained drained
        transmit();
        readBufferMaxSize = tmpReadBufferMaxSize;
    }

    setErrorAndEmit(plainSocket->error(), plainSocket->errorString());
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_readyReadSlot()
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_readyReadSlot() -" << plainSocket->bytesAvailable() << "bytes available";
#endif
    if (mode == QSslSocket::UnencryptedMode) {
        if (readyReadEmittedPointer)
            *readyReadEmittedPointer = true;
        emit q->readyRead();
        return;
    }

    transmit();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_channelReadyReadSlot(int channel)
{
    Q_Q(QSslSocket);
    if (mode == QSslSocket::UnencryptedMode)
        emit q->channelReadyRead(channel);
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_bytesWrittenSlot(qint64 written)
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocket::_q_bytesWrittenSlot(" << written << ')';
#endif

    if (mode == QSslSocket::UnencryptedMode)
        emit q->bytesWritten(written);
    else
        emit q->encryptedBytesWritten(written);
    if (state == QAbstractSocket::ClosingState && writeBuffer.isEmpty())
        q->disconnectFromHost();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_channelBytesWrittenSlot(int channel, qint64 written)
{
    Q_Q(QSslSocket);
    if (mode == QSslSocket::UnencryptedMode)
        emit q->channelBytesWritten(channel, written);
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_readChannelFinishedSlot()
{
    Q_Q(QSslSocket);
    emit q->readChannelFinished();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_flushWriteBuffer()
{
    Q_Q(QSslSocket);

    // need to notice if knock-on effects of this flush (e.g. a readReady() via transmit())
    // make another necessary, so clear flag before calling:
    flushTriggered = false;
    if (!writeBuffer.isEmpty())
        q->flush();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_flushReadBuffer()
{
    // trigger a read from the plainSocket into SSL
    if (mode != QSslSocket::UnencryptedMode)
        transmit();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_resumeImplementation()
{
    if (plainSocket)
        plainSocket->resume();
    paused = false;
    if (!connectionEncrypted) {
        if (verifyErrorsHaveBeenIgnored()) {
            continueHandshake();
        } else {
            setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError, sslErrors.constFirst().errorString());
            plainSocket->disconnectFromHost();
            return;
        }
    }
    transmit();
}

/*!
    \internal
*/
bool QSslSocketPrivate::verifyErrorsHaveBeenIgnored()
{
    bool doEmitSslError;
    if (!ignoreErrorsList.empty()) {
        // check whether the errors we got are all in the list of expected errors
        // (applies only if the method QSslSocket::ignoreSslErrors(const QList<QSslError> &errors)
        // was called)
        doEmitSslError = false;
        for (int a = 0; a < sslErrors.count(); a++) {
            if (!ignoreErrorsList.contains(sslErrors.at(a))) {
                doEmitSslError = true;
                break;
            }
        }
    } else {
        // if QSslSocket::ignoreSslErrors(const QList<QSslError> &errors) was not called and
        // we get an SSL error, emit a signal unless we ignored all errors (by calling
        // QSslSocket::ignoreSslErrors() )
        doEmitSslError = !ignoreAllSslErrors;
    }
    return !doEmitSslError;
}

/*!
    \internal
*/
qint64 QSslSocketPrivate::peek(char *data, qint64 maxSize)
{
    if (mode == QSslSocket::UnencryptedMode && !autoStartHandshake) {
        //unencrypted mode - do not use QIODevice::peek, as it reads ahead data from the plain socket
        //peek at data already in the QIODevice buffer (from a previous read)
        qint64 r = buffer.peek(data, maxSize, transactionPos);
        if (r == maxSize)
            return r;
        data += r;
        //peek at data in the plain socket
        if (plainSocket) {
            qint64 r2 = plainSocket->peek(data, maxSize - r);
            if (r2 < 0)
                return (r > 0 ? r : r2);
            return r + r2;
        } else {
            return -1;
        }
    } else {
        //encrypted mode - the socket engine will read and decrypt data into the QIODevice buffer
        return QTcpSocketPrivate::peek(data, maxSize);
    }
}

/*!
    \internal
*/
QByteArray QSslSocketPrivate::peek(qint64 maxSize)
{
    if (mode == QSslSocket::UnencryptedMode && !autoStartHandshake) {
        //unencrypted mode - do not use QIODevice::peek, as it reads ahead data from the plain socket
        //peek at data already in the QIODevice buffer (from a previous read)
        QByteArray ret;
        ret.reserve(maxSize);
        ret.resize(buffer.peek(ret.data(), maxSize, transactionPos));
        if (ret.length() == maxSize)
            return ret;
        //peek at data in the plain socket
        if (plainSocket)
            return ret + plainSocket->peek(maxSize - ret.length());
        else
            return QByteArray();
    } else {
        //encrypted mode - the socket engine will read and decrypt data into the QIODevice buffer
        return QTcpSocketPrivate::peek(maxSize);
    }
}

/*!
    \internal
*/
qint64 QSslSocketPrivate::skip(qint64 maxSize)
{
    if (mode == QSslSocket::UnencryptedMode && !autoStartHandshake)
        return plainSocket->skip(maxSize);

    // In encrypted mode, the SSL backend writes decrypted data directly into the
    // QIODevice's read buffer. As this buffer is always emptied by the caller,
    // we need to wait for more incoming data.
    return (state == QAbstractSocket::ConnectedState) ? Q_INT64_C(0) : Q_INT64_C(-1);
}

/*!
    \internal
*/
bool QSslSocketPrivate::flush()
{
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QSslSocketPrivate::flush()";
#endif
    if (mode != QSslSocket::UnencryptedMode) {
        // encrypt any unencrypted bytes in our buffer
        transmit();
    }

    return plainSocket && plainSocket->flush();
}

/*!
    \internal
*/
bool QSslSocketPrivate::rootCertOnDemandLoadingSupported()
{
    return s_loadRootCertsOnDemand;
}

/*!
    \internal
*/
QList<QByteArray> QSslSocketPrivate::unixRootCertDirectories()
{
    return QList<QByteArray>() <<  "/etc/ssl/certs/" // (K)ubuntu, OpenSUSE, Mandriva ...
                               << "/usr/lib/ssl/certs/" // Gentoo, Mandrake
                               << "/usr/share/ssl/" // Centos, Redhat, SuSE
                               << "/usr/local/ssl/" // Normal OpenSSL Tarball
                               << "/var/ssl/certs/" // AIX
                               << "/usr/local/ssl/certs/" // Solaris
                               << "/etc/openssl/certs/" // BlackBerry
                               << "/opt/openssl/certs/" // HP-UX
                               << "/etc/ssl/"; // OpenBSD
}

/*!
    \internal
*/
void QSslSocketPrivate::checkSettingSslContext(QSslSocket* socket, QSharedPointer<QSslContext> sslContext)
{
    if (socket->d_func()->sslContextPointer.isNull())
        socket->d_func()->sslContextPointer = sslContext;
}

/*!
    \internal
*/
QSharedPointer<QSslContext> QSslSocketPrivate::sslContext(QSslSocket *socket)
{
    return (socket) ? socket->d_func()->sslContextPointer : QSharedPointer<QSslContext>();
}

bool QSslSocketPrivate::isMatchingHostname(const QSslCertificate &cert, const QString &peerName)
{
    const QString lowerPeerName = QString::fromLatin1(QUrl::toAce(peerName));
    const QStringList commonNames = cert.subjectInfo(QSslCertificate::CommonName);

    for (const QString &commonName : commonNames) {
        if (isMatchingHostname(commonName, lowerPeerName))
            return true;
    }

    const auto subjectAlternativeNames = cert.subjectAlternativeNames();
    const auto altNames = subjectAlternativeNames.equal_range(QSsl::DnsEntry);
    for (auto it = altNames.first; it != altNames.second; ++it) {
        if (isMatchingHostname(*it, lowerPeerName))
            return true;
    }

    return false;
}

/*! \internal
   Checks if the certificate's name \a cn matches the \a hostname.
   \a hostname must be normalized in ASCII-Compatible Encoding, but \a cn is not normalized
 */
bool QSslSocketPrivate::isMatchingHostname(const QString &cn, const QString &hostname)
{
    int wildcard = cn.indexOf(QLatin1Char('*'));

    // Check this is a wildcard cert, if not then just compare the strings
    if (wildcard < 0)
        return QLatin1String(QUrl::toAce(cn)) == hostname;

    int firstCnDot = cn.indexOf(QLatin1Char('.'));
    int secondCnDot = cn.indexOf(QLatin1Char('.'), firstCnDot+1);

    // Check at least 3 components
    if ((-1 == secondCnDot) || (secondCnDot+1 >= cn.length()))
        return false;

    // Check * is last character of 1st component (ie. there's a following .)
    if (wildcard+1 != firstCnDot)
        return false;

    // Check only one star
    if (cn.lastIndexOf(QLatin1Char('*')) != wildcard)
        return false;

    // Reject wildcard character embedded within the A-labels or U-labels of an internationalized
    // domain name (RFC6125 section 7.2)
    if (cn.startsWith(QLatin1String("xn--"), Qt::CaseInsensitive))
        return false;

    // Check characters preceding * (if any) match
    if (wildcard && hostname.leftRef(wildcard).compare(cn.leftRef(wildcard), Qt::CaseInsensitive) != 0)
        return false;

    // Check characters following first . match
    int hnDot = hostname.indexOf(QLatin1Char('.'));
    if (hostname.midRef(hnDot + 1) != cn.midRef(firstCnDot + 1)
        && hostname.midRef(hnDot + 1) != QLatin1String(QUrl::toAce(cn.mid(firstCnDot + 1)))) {
        return false;
    }

    // Check if the hostname is an IP address, if so then wildcards are not allowed
    QHostAddress addr(hostname);
    if (!addr.isNull())
        return false;

    // Ok, I guess this was a wildcard CN and the hostname matches.
    return true;
}

QT_END_NAMESPACE

#include "moc_qsslsocket.cpp"
