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

#include "qssl_p.h"
#include "qsslconfiguration.h"
#include "qsslconfiguration_p.h"
#include "qsslsocket.h"
#include "qsslsocket_p.h"
#include "qmutex.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

const QSsl::SslOptions QSslConfigurationPrivate::defaultSslOptions = QSsl::SslOptionDisableEmptyFragments
                                                                    |QSsl::SslOptionDisableLegacyRenegotiation
                                                                    |QSsl::SslOptionDisableCompression
                                                                    |QSsl::SslOptionDisableSessionPersistence;

const char QSslConfiguration::ALPNProtocolHTTP2[] = "h2";
const char QSslConfiguration::NextProtocolSpdy3_0[] = "spdy/3";
const char QSslConfiguration::NextProtocolHttp1_1[] = "http/1.1";

/*!
    \class QSslConfiguration
    \brief The QSslConfiguration class holds the configuration and state of an SSL connection.
    \since 4.4

    \reentrant
    \inmodule QtNetwork
    \ingroup network
    \ingroup ssl
    \ingroup shared

    QSslConfiguration is used by Qt networking classes to relay
    information about an open SSL connection and to allow the
    application to control certain features of that connection.

    The settings that QSslConfiguration currently supports are:

    \list
      \li The SSL/TLS protocol to be used
      \li The certificate to be presented to the peer during connection
         and its associated private key
      \li The ciphers allowed to be used for encrypting the connection
      \li The list of Certificate Authorities certificates that are
         used to validate the peer's certificate
    \endlist

    These settings are applied only during the connection
    handshake. Setting them after the connection has been established
    has no effect.

    The state that QSslConfiguration supports are:
    \list
      \li The certificate the peer presented during handshake, along
         with the chain leading to a CA certificate
      \li The cipher used to encrypt this session
    \endlist

    The state can only be obtained once the SSL connection starts, but
    not necessarily before it's done. Some settings may change during
    the course of the SSL connection without need to restart it (for
    instance, the cipher can be changed over time).

    State in QSslConfiguration objects cannot be changed.

    QSslConfiguration can be used with QSslSocket and the Network
    Access API.

    Note that changing settings in QSslConfiguration is not enough to
    change the settings in the related SSL connection. You must call
    setSslConfiguration on a modified QSslConfiguration object to
    achieve that. The following example illustrates how to change the
    protocol to TLSv1_0 in a QSslSocket object:

    \snippet code/src_network_ssl_qsslconfiguration.cpp 0

    \sa QSsl::SslProtocol, QSslCertificate, QSslCipher, QSslKey,
        QSslSocket, QNetworkAccessManager,
        QSslSocket::sslConfiguration(), QSslSocket::setSslConfiguration()
*/

/*!
    \enum QSslConfiguration::NextProtocolNegotiationStatus

    Describes the status of the Next Protocol Negotiation (NPN) or
    Application-Layer Protocol Negotiation (ALPN).

    \value NextProtocolNegotiationNone No application protocol
    has been negotiated (yet).

    \value NextProtocolNegotiationNegotiated A next protocol
    has been negotiated (see nextNegotiatedProtocol()).

    \value NextProtocolNegotiationUnsupported The client and
    server could not agree on a common next application protocol.
*/

/*!
    \variable QSslConfiguration::NextProtocolSpdy3_0
    \brief The value used for negotiating SPDY 3.0 during the Next
    Protocol Negotiation.
*/

/*!
    \variable QSslConfiguration::NextProtocolHttp1_1
    \brief The value used for negotiating HTTP 1.1 during the Next
    Protocol Negotiation.
*/

/*!
    Constructs an empty SSL configuration. This configuration contains
    no valid settings and the state will be empty. isNull() will
    return true after this constructor is called.

    Once any setter methods are called, isNull() will return false.
*/
QSslConfiguration::QSslConfiguration()
    : d(new QSslConfigurationPrivate)
{
}

/*!
    Copies the configuration and state of \a other. If \a other is
    null, this object will be null too.
*/
QSslConfiguration::QSslConfiguration(const QSslConfiguration &other)
    : d(other.d)
{
}

/*!
    Releases any resources held by QSslConfiguration.
*/
QSslConfiguration::~QSslConfiguration()
{
    // QSharedDataPointer deletes d for us if necessary
}

/*!
    Copies the configuration and state of \a other. If \a other is
    null, this object will be null too.
*/
QSslConfiguration &QSslConfiguration::operator=(const QSslConfiguration &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QSslConfiguration::swap(QSslConfiguration &other)
    \since 5.0

    Swaps this SSL configuration instance with \a other. This function
    is very fast and never fails.
*/

/*!
    Returns \c true if this QSslConfiguration object is equal to \a
    other.

    Two QSslConfiguration objects are considered equal if they have
    the exact same settings and state.

    \sa operator!=()
*/
bool QSslConfiguration::operator==(const QSslConfiguration &other) const
{
    if (d == other.d)
        return true;
    return d->peerCertificate == other.d->peerCertificate &&
        d->peerCertificateChain == other.d->peerCertificateChain &&
        d->localCertificateChain == other.d->localCertificateChain &&
        d->privateKey == other.d->privateKey &&
        d->sessionCipher == other.d->sessionCipher &&
        d->sessionProtocol == other.d->sessionProtocol &&
        d->preSharedKeyIdentityHint == other.d->preSharedKeyIdentityHint &&
        d->ciphers == other.d->ciphers &&
        d->ellipticCurves == other.d->ellipticCurves &&
        d->ephemeralServerKey == other.d->ephemeralServerKey &&
        d->dhParams == other.d->dhParams &&
        d->caCertificates == other.d->caCertificates &&
        d->protocol == other.d->protocol &&
        d->peerVerifyMode == other.d->peerVerifyMode &&
        d->peerVerifyDepth == other.d->peerVerifyDepth &&
        d->allowRootCertOnDemandLoading == other.d->allowRootCertOnDemandLoading &&
        d->backendConfig == other.d->backendConfig &&
        d->sslOptions == other.d->sslOptions &&
        d->sslSession == other.d->sslSession &&
        d->sslSessionTicketLifeTimeHint == other.d->sslSessionTicketLifeTimeHint &&
        d->nextAllowedProtocols == other.d->nextAllowedProtocols &&
        d->nextNegotiatedProtocol == other.d->nextNegotiatedProtocol &&
        d->nextProtocolNegotiationStatus == other.d->nextProtocolNegotiationStatus &&
        d->dtlsCookieEnabled == other.d->dtlsCookieEnabled;
}

/*!
    \fn QSslConfiguration::operator!=(const QSslConfiguration &other) const

    Returns \c true if this QSslConfiguration differs from \a other. Two
    QSslConfiguration objects are considered different if any state or
    setting is different.

    \sa operator==()
*/

/*!
    Returns \c true if this is a null QSslConfiguration object.

    A QSslConfiguration object is null if it has been
    default-constructed and no setter methods have been called.

    \sa setProtocol(), setLocalCertificate(), setPrivateKey(),
        setCiphers(), setCaCertificates()
*/
bool QSslConfiguration::isNull() const
{
    return (d->protocol == QSsl::SecureProtocols &&
            d->peerVerifyMode == QSslSocket::AutoVerifyPeer &&
            d->peerVerifyDepth == 0 &&
            d->allowRootCertOnDemandLoading == true &&
            d->caCertificates.count() == 0 &&
            d->ciphers.count() == 0 &&
            d->ellipticCurves.isEmpty() &&
            d->ephemeralServerKey.isNull() &&
            d->dhParams == QSslDiffieHellmanParameters::defaultParameters() &&
            d->localCertificateChain.isEmpty() &&
            d->privateKey.isNull() &&
            d->peerCertificate.isNull() &&
            d->peerCertificateChain.count() == 0 &&
            d->backendConfig.isEmpty() &&
            d->sslOptions == QSslConfigurationPrivate::defaultSslOptions &&
            d->sslSession.isNull() &&
            d->sslSessionTicketLifeTimeHint == -1 &&
            d->preSharedKeyIdentityHint.isNull() &&
            d->nextAllowedProtocols.isEmpty() &&
            d->nextNegotiatedProtocol.isNull() &&
            d->nextProtocolNegotiationStatus == QSslConfiguration::NextProtocolNegotiationNone);
}

/*!
    Returns the protocol setting for this SSL configuration.

    \sa setProtocol()
*/
QSsl::SslProtocol QSslConfiguration::protocol() const
{
    return d->protocol;
}

/*!
    Sets the protocol setting for this configuration to be \a
    protocol.

    Setting the protocol once the connection has already been
    established has no effect.

    \sa protocol()
*/
void QSslConfiguration::setProtocol(QSsl::SslProtocol protocol)
{
    d->protocol = protocol;
}

/*!
    Returns the verify mode. This mode decides whether QSslSocket should
    request a certificate from the peer (i.e., the client requests a
    certificate from the server, or a server requesting a certificate from the
    client), and whether it should require that this certificate is valid.

    The default mode is AutoVerifyPeer, which tells QSslSocket to use
    VerifyPeer for clients, QueryPeer for servers.

    \sa setPeerVerifyMode()
*/
QSslSocket::PeerVerifyMode QSslConfiguration::peerVerifyMode() const
{
    return d->peerVerifyMode;
}

/*!
    Sets the verify mode to \a mode. This mode decides whether QSslSocket
    should request a certificate from the peer (i.e., the client requests a
    certificate from the server, or a server requesting a certificate from the
    client), and whether it should require that this certificate is valid.

    The default mode is AutoVerifyPeer, which tells QSslSocket to use
    VerifyPeer for clients, QueryPeer for servers.

    \sa peerVerifyMode()
*/
void QSslConfiguration::setPeerVerifyMode(QSslSocket::PeerVerifyMode mode)
{
    d->peerVerifyMode = mode;
}


/*!
    Returns the maximum number of certificates in the peer's certificate chain
    to be checked during the SSL handshake phase, or 0 (the default) if no
    maximum depth has been set, indicating that the whole certificate chain
    should be checked.

    The certificates are checked in issuing order, starting with the peer's
    own certificate, then its issuer's certificate, and so on.

    \sa setPeerVerifyDepth(), peerVerifyMode()
*/
int QSslConfiguration::peerVerifyDepth() const
{
    return d->peerVerifyDepth;
}

/*!
    Sets the maximum number of certificates in the peer's certificate chain to
    be checked during the SSL handshake phase, to \a depth. Setting a depth of
    0 means that no maximum depth is set, indicating that the whole
    certificate chain should be checked.

    The certificates are checked in issuing order, starting with the peer's
    own certificate, then its issuer's certificate, and so on.

    \sa peerVerifyDepth(), setPeerVerifyMode()
*/
void QSslConfiguration::setPeerVerifyDepth(int depth)
{
    if (depth < 0) {
        qCWarning(lcSsl,
                 "QSslConfiguration::setPeerVerifyDepth: cannot set negative depth of %d", depth);
        return;
    }
    d->peerVerifyDepth = depth;
}

/*!
    Returns the certificate chain to be presented to the peer during
    the SSL handshake process.

    \sa localCertificate()
    \since 5.1
*/
QList<QSslCertificate> QSslConfiguration::localCertificateChain() const
{
    return d->localCertificateChain;
}

/*!
    Sets the certificate chain to be presented to the peer during the
    SSL handshake to be \a localChain.

    Setting the certificate chain once the connection has been
    established has no effect.

    A certificate is the means of identification used in the SSL
    process. The local certificate is used by the remote end to verify
    the local user's identity against its list of Certification
    Authorities. In most cases, such as in HTTP web browsing, only
    servers identify to the clients, so the client does not send a
    certificate.

    Unlike QSslConfiguration::setLocalCertificate() this method allows
    you to specify any intermediate certificates required in order to
    validate your certificate. The first item in the list must be the
    leaf certificate.

    \sa localCertificateChain()
    \since 5.1
 */
void QSslConfiguration::setLocalCertificateChain(const QList<QSslCertificate> &localChain)
{
    d->localCertificateChain = localChain;
}

/*!
    Returns the certificate to be presented to the peer during the SSL
    handshake process.

    \sa setLocalCertificate()
*/
QSslCertificate QSslConfiguration::localCertificate() const
{
    if (d->localCertificateChain.isEmpty())
        return QSslCertificate();
    return d->localCertificateChain[0];
}

/*!
    Sets the certificate to be presented to the peer during SSL
    handshake to be \a certificate.

    Setting the certificate once the connection has been established
    has no effect.

    A certificate is the means of identification used in the SSL
    process. The local certificate is used by the remote end to verify
    the local user's identity against its list of Certification
    Authorities. In most cases, such as in HTTP web browsing, only
    servers identify to the clients, so the client does not send a
    certificate.

    \sa localCertificate()
*/
void QSslConfiguration::setLocalCertificate(const QSslCertificate &certificate)
{
    d->localCertificateChain = QList<QSslCertificate>();
    d->localCertificateChain += certificate;
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
    the QSslSocket::sslErrors() signal, QNetworkReply::sslErrors()
    signal, or the QSslSocket::encrypted() signal.

    If a null certificate is returned, it can mean the SSL handshake
    failed, or it can mean the host you are connected to doesn't have
    a certificate, or it can mean there is no connection.

    If you want to check the peer's complete chain of certificates,
    use peerCertificateChain() to get them all at once.

    \sa peerCertificateChain(),
        QSslSocket::sslErrors(), QSslSocket::ignoreSslErrors(),
        QNetworkReply::sslErrors(), QNetworkReply::ignoreSslErrors()
*/
QSslCertificate QSslConfiguration::peerCertificate() const
{
    return d->peerCertificate;
}

/*!
    Returns the peer's chain of digital certificates, starting with
    the peer's immediate certificate and ending with the CA's
    certificate.

    Peer certificates are checked automatically during the handshake
    phase. This function is normally used to fetch certificates for
    display, or for performing connection diagnostics. Certificates
    contain information about the peer and the certificate issuers,
    including host name, issuer names, and issuer public keys.

    Because the peer certificate is set during the handshake phase, it
    is safe to access the peer certificate from a slot connected to
    the QSslSocket::sslErrors() signal, QNetworkReply::sslErrors()
    signal, or the QSslSocket::encrypted() signal.

    If an empty list is returned, it can mean the SSL handshake
    failed, or it can mean the host you are connected to doesn't have
    a certificate, or it can mean there is no connection.

    If you want to get only the peer's immediate certificate, use
    peerCertificate().

    \sa peerCertificate(),
        QSslSocket::sslErrors(), QSslSocket::ignoreSslErrors(),
        QNetworkReply::sslErrors(), QNetworkReply::ignoreSslErrors()
*/
QList<QSslCertificate> QSslConfiguration::peerCertificateChain() const
{
    return d->peerCertificateChain;
}

/*!
    Returns the socket's cryptographic \l {QSslCipher} {cipher}, or a
    null cipher if the connection isn't encrypted. The socket's cipher
    for the session is set during the handshake phase. The cipher is
    used to encrypt and decrypt data transmitted through the socket.

    The SSL infrastructure also provides functions for setting the
    ordered list of ciphers from which the handshake phase will
    eventually select the session cipher. This ordered list must be in
    place before the handshake phase begins.

    \sa ciphers(), setCiphers(), QSslSocket::supportedCiphers()
*/
QSslCipher QSslConfiguration::sessionCipher() const
{
    return d->sessionCipher;
}

/*!
    Returns the socket's SSL/TLS protocol or UnknownProtocol if the
    connection isn't encrypted. The socket's protocol for the session
    is set during the handshake phase.

    \sa protocol(), setProtocol()
    \since 5.4
*/
QSsl::SslProtocol QSslConfiguration::sessionProtocol() const
{
    return d->sessionProtocol;
}

/*!
    Returns the \l {QSslKey} {SSL key} assigned to this connection or
    a null key if none has been assigned yet.

    \sa setPrivateKey(), localCertificate()
*/
QSslKey QSslConfiguration::privateKey() const
{
    return d->privateKey;
}

/*!
    Sets the connection's private \l {QSslKey} {key} to \a key. The
    private key and the local \l {QSslCertificate} {certificate} are
    used by clients and servers that must prove their identity to
    SSL peers.

    Both the key and the local certificate are required if you are
    creating an SSL server socket. If you are creating an SSL client
    socket, the key and local certificate are required if your client
    must identify itself to an SSL server.

    \sa privateKey(), setLocalCertificate()
*/
void QSslConfiguration::setPrivateKey(const QSslKey &key)
{
    d->privateKey = key;
}

/*!
    Returns this connection's current cryptographic cipher suite. This
    list is used during the handshake phase for choosing a
    session cipher. The returned list of ciphers is ordered by
    descending preference. (i.e., the first cipher in the list is the
    most preferred cipher). The session cipher will be the first one
    in the list that is also supported by the peer.

    By default, the handshake phase can choose any of the ciphers
    supported by this system's SSL libraries, which may vary from
    system to system. The list of ciphers supported by this system's
    SSL libraries is returned by QSslSocket::supportedCiphers(). You can restrict
    the list of ciphers used for choosing the session cipher for this
    socket by calling setCiphers() with a subset of the supported
    ciphers. You can revert to using the entire set by calling
    setCiphers() with the list returned by QSslSocket::supportedCiphers().

    \sa setCiphers(), QSslSocket::supportedCiphers()
*/
QList<QSslCipher> QSslConfiguration::ciphers() const
{
    return d->ciphers;
}

/*!
    Sets the cryptographic cipher suite for this socket to \a ciphers,
    which must contain a subset of the ciphers in the list returned by
    supportedCiphers().

    Restricting the cipher suite must be done before the handshake
    phase, where the session cipher is chosen.

    \sa ciphers(), QSslSocket::supportedCiphers()
*/
void QSslConfiguration::setCiphers(const QList<QSslCipher> &ciphers)
{
    d->ciphers = ciphers;
}

/*!
    \since 5.5

    Returns the list of cryptographic ciphers supported by this
    system. This list is set by the system's SSL libraries and may
    vary from system to system.

    \sa ciphers(), setCiphers()
*/
QList<QSslCipher> QSslConfiguration::supportedCiphers()
{
    return QSslSocketPrivate::supportedCiphers();
}

/*!
  Returns this connection's CA certificate database. The CA certificate
  database is used by the socket during the handshake phase to
  validate the peer's certificate. It can be modified prior to the
  handshake with setCaCertificates(), or with \l{QSslSocket}'s
  \l{QSslSocket::}{addCaCertificate()} and
  \l{QSslSocket::}{addCaCertificates()}.

  \sa setCaCertificates()
*/
QList<QSslCertificate> QSslConfiguration::caCertificates() const
{
    return d->caCertificates;
}

/*!
  Sets this socket's CA certificate database to be \a certificates.
  The certificate database must be set prior to the SSL handshake.
  The CA certificate database is used by the socket during the
  handshake phase to validate the peer's certificate.

  \note The default configuration uses the system CA certificate database. If
  that is not available (as is commonly the case on iOS), the default database
  is empty.

  \sa caCertificates()
*/
void QSslConfiguration::setCaCertificates(const QList<QSslCertificate> &certificates)
{
    d->caCertificates = certificates;
    d->allowRootCertOnDemandLoading = false;
}

/*!
    \since 5.5

    This function provides the CA certificate database
    provided by the operating system. The CA certificate database
    returned by this function is used to initialize the database
    returned by caCertificates() on the default QSslConfiguration.

    \sa caCertificates(), setCaCertificates(), defaultConfiguration()
*/
QList<QSslCertificate> QSslConfiguration::systemCaCertificates()
{
    // we are calling ensureInitialized() in the method below
    return QSslSocketPrivate::systemCaCertificates();
}

/*!
  Enables or disables an SSL compatibility \a option. If \a on
  is true, the \a option is enabled. If \a on is false, the
  \a option is disabled.

  \sa testSslOption()
*/
void QSslConfiguration::setSslOption(QSsl::SslOption option, bool on)
{
    d->sslOptions.setFlag(option, on);
}

/*!
  \since 4.8

  Returns \c true if the specified SSL compatibility \a option is enabled.

  \sa setSslOption()
*/
bool QSslConfiguration::testSslOption(QSsl::SslOption option) const
{
    return d->sslOptions & option;
}

/*!
  \since 5.2

  If QSsl::SslOptionDisableSessionPersistence was turned off, this
  function returns the session ticket used in the SSL handshake in ASN.1
  format, suitable to e.g. be persisted to disk. If no session ticket was
  used or QSsl::SslOptionDisableSessionPersistence was not turned off,
  this function returns an empty QByteArray.

  \note When persisting the session ticket to disk or similar, be
  careful not to expose the session to a potential attacker, as
  knowledge of the session allows for eavesdropping on data
  encrypted with the session parameters.

  \sa setSessionTicket(), QSsl::SslOptionDisableSessionPersistence, setSslOption()
 */
QByteArray QSslConfiguration::sessionTicket() const
{
    return d->sslSession;
}

/*!
  \since 5.2

  Sets the session ticket to be used in an SSL handshake.
  QSsl::SslOptionDisableSessionPersistence must be turned off
  for this to work, and \a sessionTicket must be in ASN.1 format
  as returned by sessionTicket().

  \sa sessionTicket(), QSsl::SslOptionDisableSessionPersistence, setSslOption()
 */
void QSslConfiguration::setSessionTicket(const QByteArray &sessionTicket)
{
    d->sslSession = sessionTicket;
}

/*!
  \since 5.2

  If QSsl::SslOptionDisableSessionPersistence was turned off, this
  function returns the session ticket life time hint sent by the
  server (which might be 0).
  If the server did not send a session ticket (e.g. when
  resuming a session or when the server does not support it) or
  QSsl::SslOptionDisableSessionPersistence was not turned off,
  this function returns -1.

  \sa sessionTicket(), QSsl::SslOptionDisableSessionPersistence, setSslOption()
 */
int QSslConfiguration::sessionTicketLifeTimeHint() const
{
    return d->sslSessionTicketLifeTimeHint;
}

/*!
   \since 5.7

   Returns the ephemeral server key used for cipher algorithms
   with forward secrecy, e.g. DHE-RSA-AES128-SHA.

   The ephemeral key is only available when running in client mode, i.e.
   QSslSocket::SslClientMode. When running in server mode or using a
   cipher algorithm without forward secrecy a null key is returned.
   The ephemeral server key will be set before emitting the encrypted()
   signal.
 */
QSslKey QSslConfiguration::ephemeralServerKey() const
{
    return d->ephemeralServerKey;
}

/*!
    \since 5.5

    Returns this connection's current list of elliptic curves. This
    list is used during the handshake phase for choosing an
    elliptic curve (when using an elliptic curve cipher).
    The returned list of curves is ordered by descending preference
    (i.e., the first curve in the list is the most preferred one).

    By default, the handshake phase can choose any of the curves
    supported by this system's SSL libraries, which may vary from
    system to system. The list of curves supported by this system's
    SSL libraries is returned by QSslSocket::supportedEllipticCurves().

    You can restrict the list of curves used for choosing the session cipher
    for this socket by calling setEllipticCurves() with a subset of the
    supported ciphers. You can revert to using the entire set by calling
    setEllipticCurves() with the list returned by
    QSslSocket::supportedEllipticCurves().

    \sa setEllipticCurves
 */
QVector<QSslEllipticCurve> QSslConfiguration::ellipticCurves() const
{
    return d->ellipticCurves;
}

/*!
    \since 5.5

    Sets the list of elliptic curves to be used by this socket to \a curves,
    which must contain a subset of the curves in the list returned by
    supportedEllipticCurves().

    Restricting the elliptic curves must be done before the handshake
    phase, where the session cipher is chosen.

    \sa ellipticCurves
 */
void QSslConfiguration::setEllipticCurves(const QVector<QSslEllipticCurve> &curves)
{
    d->ellipticCurves = curves;
}

/*!
    \since 5.5

    Returns the list of elliptic curves supported by this
    system. This list is set by the system's SSL libraries and may
    vary from system to system.

    \sa ellipticCurves(), setEllipticCurves()
*/
QVector<QSslEllipticCurve> QSslConfiguration::supportedEllipticCurves()
{
    return QSslSocketPrivate::supportedEllipticCurves();
}

/*!
    \since 5.8

    Returns the identity hint.

    \sa setPreSharedKeyIdentityHint()
*/
QByteArray QSslConfiguration::preSharedKeyIdentityHint() const
{
    return d->preSharedKeyIdentityHint;
}

/*!
    \since 5.8

    Sets the identity hint for a preshared key authentication to \a hint. This will
    affect the next initiated handshake; calling this function on an already-encrypted
    socket will not affect the socket's identity hint.

    The identity hint is used in QSslSocket::SslServerMode only!
*/
void QSslConfiguration::setPreSharedKeyIdentityHint(const QByteArray &hint)
{
    d->preSharedKeyIdentityHint = hint;
}

/*!
    \since 5.8

    Retrieves the current set of Diffie-Hellman parameters.

    If no Diffie-Hellman parameters have been set, the QSslConfiguration object
    defaults to using the 1024-bit MODP group from RFC 2409.
 */
QSslDiffieHellmanParameters QSslConfiguration::diffieHellmanParameters() const
{
    return d->dhParams;
}

/*!
    \since 5.8

    Sets a custom set of Diffie-Hellman parameters to be used by this socket when functioning as
    a server to \a dhparams.

    If no Diffie-Hellman parameters have been set, the QSslConfiguration object
    defaults to using the 1024-bit MODP group from RFC 2409.
 */
void QSslConfiguration::setDiffieHellmanParameters(const QSslDiffieHellmanParameters &dhparams)
{
    d->dhParams = dhparams;
}

/*!
    \since 5.11

    Returns the backend-specific configuration.

    Only options set by setBackendConfigurationOption() or setBackendConfiguration() will be
    returned. The internal standard configuration of the backend is not reported.

    \sa setBackendConfigurationOption(), setBackendConfiguration()
 */
QMap<QByteArray, QVariant> QSslConfiguration::backendConfiguration() const
{
    return d->backendConfig;
}

/*!
    \since 5.11

    Sets the option \a name in the backend-specific configuration to \a value.

    Options supported by the OpenSSL (>= 1.0.2) backend are available in the \l
    {https://www.openssl.org/docs/manmaster/man3/SSL_CONF_cmd.html#SUPPORTED-CONFIGURATION-FILE-COMMANDS}
    {supported configuration file commands} documentation. The expected type for
    the \a value parameter is a QByteArray for all options. The \l
    {https://www.openssl.org/docs/manmaster/man3/SSL_CONF_cmd.html#EXAMPLES}{examples}
    show how to use some of the options.

    \note The backend-specific configuration will be applied after the general
    configuration. Using the backend-specific configuration to set a general
    configuration option again will overwrite the general configuration option.

    \sa backendConfiguration(), setBackendConfiguration()
 */
void QSslConfiguration::setBackendConfigurationOption(const QByteArray &name, const QVariant &value)
{
    d->backendConfig[name] = value;
}

/*!
    \since 5.11

    Sets or clears the backend-specific configuration.

    Without a \a backendConfiguration parameter this function will clear the
    backend-specific configuration. More information about the supported
    options is available in the documentation of setBackendConfigurationOption().

    \sa backendConfiguration(), setBackendConfigurationOption()
 */
void QSslConfiguration::setBackendConfiguration(const QMap<QByteArray, QVariant> &backendConfiguration)
{
    d->backendConfig = backendConfiguration;
}

/*!
  \since 5.3

  This function returns the protocol negotiated with the server
  if the Next Protocol Negotiation (NPN) or Application-Layer Protocol
  Negotiation (ALPN) TLS extension was enabled.
  In order for the NPN/ALPN extension to be enabled, setAllowedNextProtocols()
  needs to be called explicitly before connecting to the server.

  If no protocol could be negotiated or the extension was not enabled,
  this function returns a QByteArray which is null.

  \sa setAllowedNextProtocols(), nextProtocolNegotiationStatus()
 */
QByteArray QSslConfiguration::nextNegotiatedProtocol() const
{
    return d->nextNegotiatedProtocol;
}

/*!
  \since 5.3

  This function sets the allowed \a protocols to be negotiated with the
  server through the Next Protocol Negotiation (NPN) or Application-Layer
  Protocol Negotiation (ALPN) TLS extension; each
  element in \a protocols must define one allowed protocol.
  The function must be called explicitly before connecting to send the NPN/ALPN
  extension in the SSL handshake.
  Whether or not the negotiation succeeded can be queried through
  nextProtocolNegotiationStatus().

  \sa nextNegotiatedProtocol(), nextProtocolNegotiationStatus(), allowedNextProtocols(), QSslConfiguration::NextProtocolSpdy3_0, QSslConfiguration::NextProtocolHttp1_1
 */
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
void QSslConfiguration::setAllowedNextProtocols(const QList<QByteArray> &protocols)
#else
void QSslConfiguration::setAllowedNextProtocols(QList<QByteArray> protocols)
#endif
{
    d->nextAllowedProtocols = protocols;
}

/*!
  \since 5.3

  This function returns the allowed protocols to be negotiated with the
  server through the Next Protocol Negotiation (NPN) or Application-Layer
  Protocol Negotiation (ALPN) TLS extension, as set by setAllowedNextProtocols().

  \sa nextNegotiatedProtocol(), nextProtocolNegotiationStatus(), setAllowedNextProtocols(), QSslConfiguration::NextProtocolSpdy3_0, QSslConfiguration::NextProtocolHttp1_1
 */
QList<QByteArray> QSslConfiguration::allowedNextProtocols() const
{
    return d->nextAllowedProtocols;
}

/*!
  \since 5.3

  This function returns the status of the Next Protocol Negotiation (NPN)
  or Application-Layer Protocol Negotiation (ALPN).
  If the feature has not been enabled through setAllowedNextProtocols(),
  this function returns NextProtocolNegotiationNone.
  The status will be set before emitting the encrypted() signal.

  \sa setAllowedNextProtocols(), allowedNextProtocols(), nextNegotiatedProtocol(), QSslConfiguration::NextProtocolNegotiationStatus
 */
QSslConfiguration::NextProtocolNegotiationStatus QSslConfiguration::nextProtocolNegotiationStatus() const
{
    return d->nextProtocolNegotiationStatus;
}

/*!
    Returns the default SSL configuration to be used in new SSL
    connections.

    The default SSL configuration consists of:

    \list
      \li no local certificate and no private key
      \li protocol \l{QSsl::SecureProtocols}{SecureProtocols}
      \li the system's default CA certificate list
      \li the cipher list equal to the list of the SSL libraries'
         supported SSL ciphers that are 128 bits or more
    \endlist

    \sa QSslSocket::supportedCiphers(), setDefaultConfiguration()
*/
QSslConfiguration QSslConfiguration::defaultConfiguration()
{
    return QSslConfigurationPrivate::defaultConfiguration();
}

/*!
    Sets the default SSL configuration to be used in new SSL
    connections to be \a configuration. Existing connections are not
    affected by this call.

    \sa QSslSocket::supportedCiphers(), defaultConfiguration()
*/
void QSslConfiguration::setDefaultConfiguration(const QSslConfiguration &configuration)
{
    QSslConfigurationPrivate::setDefaultConfiguration(configuration);
}

#if QT_CONFIG(dtls) || defined(Q_CLANG_QDOC)

/*!
  This function returns true if DTLS cookie verification was enabled on a
  server-side socket.

  \sa setDtlsCookieVerificationEnabled()
 */
bool QSslConfiguration::dtlsCookieVerificationEnabled() const
{
    return d->dtlsCookieEnabled;
}

/*!
  This function enables DTLS cookie verification when \a enable is true.

  \sa dtlsCookieVerificationEnabled()
 */
void QSslConfiguration::setDtlsCookieVerificationEnabled(bool enable)
{
    d->dtlsCookieEnabled = enable;
}

/*!
    Returns the default DTLS configuration to be used in new DTLS
    connections.

    The default DTLS configuration consists of:

    \list
      \li no local certificate and no private key
      \li protocol DtlsV1_2OrLater
      \li the system's default CA certificate list
      \li the cipher list equal to the list of the SSL libraries'
         supported TLS 1.2 ciphers that use 128 or more secret bits
         for the cipher.
    \endlist

    \sa setDefaultDtlsConfiguration()
*/
QSslConfiguration QSslConfiguration::defaultDtlsConfiguration()
{
    return QSslConfigurationPrivate::defaultDtlsConfiguration();
}

/*!
    Sets the default DTLS configuration to be used in new DTLS
    connections to be \a configuration. Existing connections are not
    affected by this call.

    \sa defaultDtlsConfiguration()
*/
void QSslConfiguration::setDefaultDtlsConfiguration(const QSslConfiguration &configuration)
{
    QSslConfigurationPrivate::setDefaultDtlsConfiguration(configuration);
}

#endif // dtls

/*! \internal
*/
bool QSslConfigurationPrivate::peerSessionWasShared(const QSslConfiguration &configuration) {
        return configuration.d->peerSessionShared;
    }

QT_END_NAMESPACE
