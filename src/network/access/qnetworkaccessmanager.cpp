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

#include "qnetworkaccessmanager.h"
#include "qnetworkaccessmanager_p.h"
#include "qnetworkrequest.h"
#include "qnetworkreply.h"
#include "qnetworkreply_p.h"
#include "qnetworkcookie.h"
#include "qnetworkcookiejar.h"
#include "qabstractnetworkcache.h"
#include "qhstspolicy.h"
#include "qhsts_p.h"

#if QT_CONFIG(settings)
#include "qhstsstore_p.h"
#endif // QT_CONFIG(settings)

#include "QtNetwork/qnetworksession.h"
#include "QtNetwork/private/qsharednetworksession_p.h"

#if QT_CONFIG(ftp)
#include "qnetworkaccessftpbackend_p.h"
#endif
#include "qnetworkaccessfilebackend_p.h"
#include "qnetworkaccessdebugpipebackend_p.h"
#include "qnetworkaccesscachebackend_p.h"
#include "qnetworkreplydataimpl_p.h"
#include "qnetworkreplyfileimpl_p.h"

#include "QtCore/qbuffer.h"
#include "QtCore/qurl.h"
#include "QtCore/qvector.h"
#include "QtNetwork/private/qauthenticator_p.h"
#include "QtNetwork/qsslconfiguration.h"
#include "QtNetwork/qnetworkconfigmanager.h"

#if QT_CONFIG(http)
#include "qhttpmultipart.h"
#include "qhttpmultipart_p.h"
#include "qnetworkreplyhttpimpl_p.h"
#endif

#include "qthread.h"

#include <QHostInfo>

#if defined(Q_OS_MACOS)
#include <CoreServices/CoreServices.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <Security/SecKeychain.h>
#endif
#ifdef Q_OS_WASM
#include "qnetworkreplywasmimpl_p.h"
#endif

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QNetworkAccessFileBackendFactory, fileBackend)
#if QT_CONFIG(ftp)
Q_GLOBAL_STATIC(QNetworkAccessFtpBackendFactory, ftpBackend)
#endif // QT_CONFIG(ftp)

#ifdef QT_BUILD_INTERNAL
Q_GLOBAL_STATIC(QNetworkAccessDebugPipeBackendFactory, debugpipeBackend)
#endif

#if defined(Q_OS_MACX)
bool getProxyAuth(const QString& proxyHostname, const QString &scheme, QString& username, QString& password)
{
    OSStatus err;
    SecKeychainItemRef itemRef;
    bool retValue = false;
    SecProtocolType protocolType = kSecProtocolTypeAny;
    if (scheme.compare(QLatin1String("ftp"),Qt::CaseInsensitive)==0) {
        protocolType = kSecProtocolTypeFTPProxy;
    } else if (scheme.compare(QLatin1String("http"),Qt::CaseInsensitive)==0
               || scheme.compare(QLatin1String("preconnect-http"),Qt::CaseInsensitive)==0) {
        protocolType = kSecProtocolTypeHTTPProxy;
    } else if (scheme.compare(QLatin1String("https"),Qt::CaseInsensitive)==0
               || scheme.compare(QLatin1String("preconnect-https"),Qt::CaseInsensitive)==0) {
        protocolType = kSecProtocolTypeHTTPSProxy;
    }
    QByteArray proxyHostnameUtf8(proxyHostname.toUtf8());
    err = SecKeychainFindInternetPassword(NULL,
                                          proxyHostnameUtf8.length(), proxyHostnameUtf8.constData(),
                                          0,NULL,
                                          0, NULL,
                                          0, NULL,
                                          0,
                                          protocolType,
                                          kSecAuthenticationTypeAny,
                                          0, NULL,
                                          &itemRef);
    if (err == noErr) {

        SecKeychainAttribute attr;
        SecKeychainAttributeList attrList;
        UInt32 length;
        void *outData;

        attr.tag = kSecAccountItemAttr;
        attr.length = 0;
        attr.data = NULL;

        attrList.count = 1;
        attrList.attr = &attr;

        if (SecKeychainItemCopyContent(itemRef, NULL, &attrList, &length, &outData) == noErr) {
            username = QString::fromUtf8((const char*)attr.data, attr.length);
            password = QString::fromUtf8((const char*)outData, length);
            SecKeychainItemFreeContent(&attrList,outData);
            retValue = true;
        }
        CFRelease(itemRef);
    }
    return retValue;
}
#endif



static void ensureInitialized()
{
#if QT_CONFIG(ftp)
    (void) ftpBackend();
#endif

#ifdef QT_BUILD_INTERNAL
    (void) debugpipeBackend();
#endif

    // leave this one last since it will query the special QAbstractFileEngines
    (void) fileBackend();
}

/*!
    \class QNetworkAccessManager
    \brief The QNetworkAccessManager class allows the application to
    send network requests and receive replies.
    \since 4.4

    \ingroup network
    \inmodule QtNetwork
    \reentrant

    The Network Access API is constructed around one QNetworkAccessManager
    object, which holds the common configuration and settings for the requests
    it sends. It contains the proxy and cache configuration, as well as the
    signals related to such issues, and reply signals that can be used to
    monitor the progress of a network operation. One QNetworkAccessManager
    instance should be enough for the whole Qt application. Since
    QNetworkAccessManager is based on QObject, it can only be used from the
    thread it belongs to.

    Once a QNetworkAccessManager object has been created, the application can
    use it to send requests over the network. A group of standard functions
    are supplied that take a request and optional data, and each return a
    QNetworkReply object. The returned object is used to obtain any data
    returned in response to the corresponding request.

    A simple download off the network could be accomplished with:
    \snippet code/src_network_access_qnetworkaccessmanager.cpp 0

    QNetworkAccessManager has an asynchronous API.
    When the \tt replyFinished slot above is called, the parameter it
    takes is the QNetworkReply object containing the downloaded data
    as well as meta-data (headers, etc.).

    \note After the request has finished, it is the responsibility of the user
    to delete the QNetworkReply object at an appropriate time. Do not directly
    delete it inside the slot connected to finished(). You can use the
    deleteLater() function.

    \note QNetworkAccessManager queues the requests it receives. The number
    of requests executed in parallel is dependent on the protocol.
    Currently, for the HTTP protocol on desktop platforms, 6 requests are
    executed in parallel for one host/port combination.

    A more involved example, assuming the manager is already existent,
    can be:
    \snippet code/src_network_access_qnetworkaccessmanager.cpp 1

    \section1 Network and Roaming Support

    With the addition of the \l {Bearer Management} API to Qt 4.7
    QNetworkAccessManager gained the ability to manage network connections.
    QNetworkAccessManager can start the network interface if the device is
    offline and terminates the interface if the current process is the last
    one to use the uplink. Note that some platforms utilize grace periods from
    when the last application stops using a uplink until the system actually
    terminates the connectivity link. Roaming is equally transparent. Any
    queued/pending network requests are automatically transferred to the new
    access point.

    Clients wanting to utilize this feature should not require any changes. In fact
    it is likely that existing platform specific connection code can simply be
    removed from the application.

    \note The network and roaming support in QNetworkAccessManager is conditional
    upon the platform supporting connection management. The
    \l QNetworkConfigurationManager::NetworkSessionRequired can be used to
    detect whether QNetworkAccessManager utilizes this feature.

    \sa QNetworkRequest, QNetworkReply, QNetworkProxy
*/

/*!
    \enum QNetworkAccessManager::Operation

    Indicates the operation this reply is processing.

    \value HeadOperation        retrieve headers operation (created
    with head())

    \value GetOperation         retrieve headers and download contents
    (created with get())

    \value PutOperation         upload contents operation (created
    with put())

    \value PostOperation        send the contents of an HTML form for
    processing via HTTP POST (created with post())

    \value DeleteOperation      delete contents operation (created with
    deleteResource())

    \value CustomOperation      custom operation (created with
    sendCustomRequest())    \since 4.7

    \omitvalue UnknownOperation

    \sa QNetworkReply::operation()
*/

/*!
    \enum QNetworkAccessManager::NetworkAccessibility

    Indicates whether the network is accessible via this network access manager.

    \value UnknownAccessibility     The network accessibility cannot be determined.
    \value NotAccessible            The network is not currently accessible, either because there
                                    is currently no network coverage or network access has been
                                    explicitly disabled by a call to setNetworkAccessible().
    \value Accessible               The network is accessible.

    \sa networkAccessible
*/

/*!
    \property QNetworkAccessManager::networkAccessible
    \brief whether the network is currently accessible via this network access manager.

    \since 4.7

    If the network is \l {NotAccessible}{not accessible} the network access manager will not
    process any new network requests, all such requests will fail with an error.  Requests with
    URLs with the file:// scheme will still be processed.

    By default the value of this property reflects the physical state of the device.  Applications
    may override it to disable all network requests via this network access manager by calling

    \snippet code/src_network_access_qnetworkaccessmanager.cpp 4

    Network requests can be re-enabled again, and this property will resume to
    reflect the actual device state by calling

    \snippet code/src_network_access_qnetworkaccessmanager.cpp 5

    \note Calling setNetworkAccessible() does not change the network state.
*/

/*!
    \fn void QNetworkAccessManager::networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible)

    This signal is emitted when the value of the \l networkAccessible property changes.
    \a accessible is the new network accessibility.
*/

/*!
    \fn void QNetworkAccessManager::networkSessionConnected()

    \since 4.7

    \internal

    This signal is emitted when the status of the network session changes into a usable (Connected)
    state. It is used to signal to QNetworkReplys to start or migrate their network operation once
    the network session has been opened or finished roaming.
*/

/*!
    \fn void QNetworkAccessManager::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)

    This signal is emitted whenever a proxy requests authentication
    and QNetworkAccessManager cannot find a valid, cached
    credential. The slot connected to this signal should fill in the
    credentials for the proxy \a proxy in the \a authenticator object.

    QNetworkAccessManager will cache the credentials internally. The
    next time the proxy requests authentication, QNetworkAccessManager
    will automatically send the same credential without emitting the
    proxyAuthenticationRequired signal again.

    If the proxy rejects the credentials, QNetworkAccessManager will
    emit the signal again.

    \sa proxy(), setProxy(), authenticationRequired()
*/

/*!
    \fn void QNetworkAccessManager::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)

    This signal is emitted whenever a final server requests
    authentication before it delivers the requested contents. The slot
    connected to this signal should fill the credentials for the
    contents (which can be determined by inspecting the \a reply
    object) in the \a authenticator object.

    QNetworkAccessManager will cache the credentials internally and
    will send the same values if the server requires authentication
    again, without emitting the authenticationRequired() signal. If it
    rejects the credentials, this signal will be emitted again.

    \note To have the request not send credentials you must not call
    setUser() or setPassword() on the \a authenticator object. This
    will result in the \l finished() signal being emitted with a
    \l QNetworkReply with error \l {QNetworkReply::} {AuthenticationRequiredError}.

    \note It is not possible to use a QueuedConnection to connect to
    this signal, as the connection will fail if the authenticator has
    not been filled in with new information when the signal returns.

    \sa proxyAuthenticationRequired(), QAuthenticator::setUser(), QAuthenticator::setPassword()
*/

/*!
    \fn void QNetworkAccessManager::finished(QNetworkReply *reply)

    This signal is emitted whenever a pending network reply is
    finished. The \a reply parameter will contain a pointer to the
    reply that has just finished. This signal is emitted in tandem
    with the QNetworkReply::finished() signal.

    See QNetworkReply::finished() for information on the status that
    the object will be in.

    \note Do not delete the \a reply object in the slot connected to this
    signal. Use deleteLater().

    \sa QNetworkReply::finished(), QNetworkReply::error()
*/

/*!
    \fn void QNetworkAccessManager::encrypted(QNetworkReply *reply)
    \since 5.1

    This signal is emitted when an SSL/TLS session has successfully
    completed the initial handshake. At this point, no user data
    has been transmitted. The signal can be used to perform
    additional checks on the certificate chain, for example to
    notify users when the certificate for a website has changed. The
    \a reply parameter specifies which network reply is responsible.
    If the reply does not match the expected criteria then it should
    be aborted by calling QNetworkReply::abort() by a slot connected
    to this signal. The SSL configuration in use can be inspected
    using the QNetworkReply::sslConfiguration() method.

    Internally, QNetworkAccessManager may open multiple connections
    to a server, in order to allow it process requests in parallel.
    These connections may be reused, which means that the encrypted()
    signal would not be emitted. This means that you are only
    guaranteed to receive this signal for the first connection to a
    site in the lifespan of the QNetworkAccessManager.

    \sa QSslSocket::encrypted()
    \sa QNetworkReply::encrypted()
*/

/*!
    \fn void QNetworkAccessManager::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)

    This signal is emitted if the SSL/TLS session encountered errors
    during the set up, including certificate verification errors. The
    \a errors parameter contains the list of errors and \a reply is
    the QNetworkReply that is encountering these errors.

    To indicate that the errors are not fatal and that the connection
    should proceed, the QNetworkReply::ignoreSslErrors() function should be called
    from the slot connected to this signal. If it is not called, the
    SSL session will be torn down before any data is exchanged
    (including the URL).

    This signal can be used to display an error message to the user
    indicating that security may be compromised and display the
    SSL settings (see sslConfiguration() to obtain it). If the user
    decides to proceed after analyzing the remote certificate, the
    slot should call ignoreSslErrors().

    \sa QSslSocket::sslErrors(), QNetworkReply::sslErrors(),
    QNetworkReply::sslConfiguration(), QNetworkReply::ignoreSslErrors()
*/

/*!
    \fn void QNetworkAccessManager::preSharedKeyAuthenticationRequired(QNetworkReply *reply, QSslPreSharedKeyAuthenticator *authenticator)
    \since 5.5

    This signal is emitted if the SSL/TLS handshake negotiates a PSK
    ciphersuite, and therefore a PSK authentication is then required.
    The \a reply object is the QNetworkReply that is negotiating
    such ciphersuites.

    When using PSK, the client must send to the server a valid identity and a
    valid pre shared key, in order for the SSL handshake to continue.
    Applications can provide this information in a slot connected to this
    signal, by filling in the passed \a authenticator object according to their
    needs.

    \note Ignoring this signal, or failing to provide the required credentials,
    will cause the handshake to fail, and therefore the connection to be aborted.

    \note The \a authenticator object is owned by the reply and must not be
    deleted by the application.

    \sa QSslPreSharedKeyAuthenticator
*/

/*!
    Constructs a QNetworkAccessManager object that is the center of
    the Network Access API and sets \a parent as the parent object.
*/
QNetworkAccessManager::QNetworkAccessManager(QObject *parent)
    : QObject(*new QNetworkAccessManagerPrivate, parent)
{
    ensureInitialized();

    qRegisterMetaType<QNetworkReply::NetworkError>();
#ifndef QT_NO_NETWORKPROXY
    qRegisterMetaType<QNetworkProxy>();
#endif
#ifndef QT_NO_SSL
    qRegisterMetaType<QList<QSslError> >();
    qRegisterMetaType<QSslConfiguration>();
    qRegisterMetaType<QSslPreSharedKeyAuthenticator *>();
#endif
    qRegisterMetaType<QList<QPair<QByteArray,QByteArray> > >();
#if QT_CONFIG(http)
    qRegisterMetaType<QHttpNetworkRequest>();
#endif
    qRegisterMetaType<QNetworkReply::NetworkError>();
    qRegisterMetaType<QSharedPointer<char> >();

#ifndef QT_NO_BEARERMANAGEMENT
    Q_D(QNetworkAccessManager);
    // if a session is required, we track online state through
    // the QNetworkSession's signals if a request is already made.
    // we need to track current accessibility state by default
    //
    connect(&d->networkConfigurationManager, SIGNAL(onlineStateChanged(bool)),
            SLOT(_q_onlineStateChanged(bool)));
    connect(&d->networkConfigurationManager, SIGNAL(configurationChanged(const QNetworkConfiguration &)),
            SLOT(_q_configurationChanged(const QNetworkConfiguration &)));

#endif
}

/*!
    Destroys the QNetworkAccessManager object and frees up any
    resources. Note that QNetworkReply objects that are returned from
    this class have this object set as their parents, which means that
    they will be deleted along with it if you don't call
    QObject::setParent() on them.
*/
QNetworkAccessManager::~QNetworkAccessManager()
{
#ifndef QT_NO_NETWORKPROXY
    delete d_func()->proxyFactory;
#endif

    // Delete the QNetworkReply children first.
    // Else a QAbstractNetworkCache might get deleted in ~QObject
    // before a QNetworkReply that accesses the QAbstractNetworkCache
    // object in its destructor.
    qDeleteAll(findChildren<QNetworkReply *>());
    // The other children will be deleted in this ~QObject
    // FIXME instead of this "hack" make the QNetworkReplyImpl
    // properly watch the cache deletion, e.g. via a QWeakPointer.
}

#ifndef QT_NO_NETWORKPROXY
/*!
    Returns the QNetworkProxy that the requests sent using this
    QNetworkAccessManager object will use. The default value for the
    proxy is QNetworkProxy::DefaultProxy.

    \sa setProxy(), setProxyFactory(), proxyAuthenticationRequired()
*/
QNetworkProxy QNetworkAccessManager::proxy() const
{
    return d_func()->proxy;
}

/*!
    Sets the proxy to be used in future requests to be \a proxy. This
    does not affect requests that have already been sent. The
    proxyAuthenticationRequired() signal will be emitted if the proxy
    requests authentication.

    A proxy set with this function will be used for all requests
    issued by QNetworkAccessManager. In some cases, it might be
    necessary to select different proxies depending on the type of
    request being sent or the destination host. If that's the case,
    you should consider using setProxyFactory().

    \sa proxy(), proxyAuthenticationRequired()
*/
void QNetworkAccessManager::setProxy(const QNetworkProxy &proxy)
{
    Q_D(QNetworkAccessManager);
    delete d->proxyFactory;
    d->proxy = proxy;
    d->proxyFactory = nullptr;
}

/*!
    \fn QNetworkProxyFactory *QNetworkAccessManager::proxyFactory() const
    \since 4.5

    Returns the proxy factory that this QNetworkAccessManager object
    is using to determine the proxies to be used for requests.

    Note that the pointer returned by this function is managed by
    QNetworkAccessManager and could be deleted at any time.

    \sa setProxyFactory(), proxy()
*/
QNetworkProxyFactory *QNetworkAccessManager::proxyFactory() const
{
    return d_func()->proxyFactory;
}

/*!
    \since 4.5

    Sets the proxy factory for this class to be \a factory. A proxy
    factory is used to determine a more specific list of proxies to be
    used for a given request, instead of trying to use the same proxy
    value for all requests.

    All queries sent by QNetworkAccessManager will have type
    QNetworkProxyQuery::UrlRequest.

    For example, a proxy factory could apply the following rules:
    \list
      \li if the target address is in the local network (for example,
         if the hostname contains no dots or if it's an IP address in
         the organization's range), return QNetworkProxy::NoProxy
      \li if the request is FTP, return an FTP proxy
      \li if the request is HTTP or HTTPS, then return an HTTP proxy
      \li otherwise, return a SOCKSv5 proxy server
    \endlist

    The lifetime of the object \a factory will be managed by
    QNetworkAccessManager. It will delete the object when necessary.

    \note If a specific proxy is set with setProxy(), the factory will not
    be used.

    \sa proxyFactory(), setProxy(), QNetworkProxyQuery
*/
void QNetworkAccessManager::setProxyFactory(QNetworkProxyFactory *factory)
{
    Q_D(QNetworkAccessManager);
    delete d->proxyFactory;
    d->proxyFactory = factory;
    d->proxy = QNetworkProxy();
}
#endif

/*!
    \since 4.5

    Returns the cache that is used to store data obtained from the network.

    \sa setCache()
*/
QAbstractNetworkCache *QNetworkAccessManager::cache() const
{
    Q_D(const QNetworkAccessManager);
    return d->networkCache;
}

/*!
    \since 4.5

    Sets the manager's network cache to be the \a cache specified. The cache
    is used for all requests dispatched by the manager.

    Use this function to set the network cache object to a class that implements
    additional features, like saving the cookies to permanent storage.

    \note QNetworkAccessManager takes ownership of the \a cache object.

    QNetworkAccessManager by default does not have a set cache.
    Qt provides a simple disk cache, QNetworkDiskCache, which can be used.

    \sa cache(), QNetworkRequest::CacheLoadControl
*/
void QNetworkAccessManager::setCache(QAbstractNetworkCache *cache)
{
    Q_D(QNetworkAccessManager);
    if (d->networkCache != cache) {
        delete d->networkCache;
        d->networkCache = cache;
        if (d->networkCache)
            d->networkCache->setParent(this);
    }
}

/*!
    Returns the QNetworkCookieJar that is used to store cookies
    obtained from the network as well as cookies that are about to be
    sent.

    \sa setCookieJar()
*/
QNetworkCookieJar *QNetworkAccessManager::cookieJar() const
{
    Q_D(const QNetworkAccessManager);
    if (!d->cookieJar)
        d->createCookieJar();
    return d->cookieJar;
}

/*!
    Sets the manager's cookie jar to be the \a cookieJar specified.
    The cookie jar is used by all requests dispatched by the manager.

    Use this function to set the cookie jar object to a class that
    implements additional features, like saving the cookies to permanent
    storage.

    \note QNetworkAccessManager takes ownership of the \a cookieJar object.

    If \a cookieJar is in the same thread as this QNetworkAccessManager,
    it will set the parent of the \a cookieJar
    so that the cookie jar is deleted when this
    object is deleted as well. If you want to share cookie jars
    between different QNetworkAccessManager objects, you may want to
    set the cookie jar's parent to 0 after calling this function.

    QNetworkAccessManager by default does not implement any cookie
    policy of its own: it accepts all cookies sent by the server, as
    long as they are well formed and meet the minimum security
    requirements (cookie domain matches the request's and cookie path
    matches the request's). In order to implement your own security
    policy, override the QNetworkCookieJar::cookiesForUrl() and
    QNetworkCookieJar::setCookiesFromUrl() virtual functions. Those
    functions are called by QNetworkAccessManager when it detects a
    new cookie.

    \sa cookieJar(), QNetworkCookieJar::cookiesForUrl(), QNetworkCookieJar::setCookiesFromUrl()
*/
void QNetworkAccessManager::setCookieJar(QNetworkCookieJar *cookieJar)
{
    Q_D(QNetworkAccessManager);
    d->cookieJarCreated = true;
    if (d->cookieJar != cookieJar) {
        if (d->cookieJar && d->cookieJar->parent() == this)
            delete d->cookieJar;
        d->cookieJar = cookieJar;
        if (cookieJar && thread() == cookieJar->thread())
            d->cookieJar->setParent(this);
    }
}

/*!
    \since 5.9

    If \a enabled is \c true, QNetworkAccessManager follows the HTTP Strict Transport
    Security policy (HSTS, RFC6797). When processing a request, QNetworkAccessManager
    automatically replaces the "http" scheme with "https" and uses a secure transport
    for HSTS hosts. If it's set explicitly, port 80 is replaced by port 443.

    When HSTS is enabled, for each HTTP response containing HSTS header and
    received over a secure transport, QNetworkAccessManager will update its HSTS
    cache, either remembering a host with a valid policy or removing a host with
    an expired or disabled HSTS policy.

    \sa isStrictTransportSecurityEnabled()
*/
void QNetworkAccessManager::setStrictTransportSecurityEnabled(bool enabled)
{
    Q_D(QNetworkAccessManager);
    d->stsEnabled = enabled;
}

/*!
    \since 5.9

    Returns true if HTTP Strict Transport Security (HSTS) was enabled. By default
    HSTS is disabled.

    \sa setStrictTransportSecurityEnabled()
*/
bool QNetworkAccessManager::isStrictTransportSecurityEnabled() const
{
    Q_D(const QNetworkAccessManager);
    return d->stsEnabled;
}

/*!
    \since 5.10

    If \a enabled is \c true, the internal HSTS cache will use a persistent store
    to read and write HSTS policies. \a storeDir defines where this store will be
    located. The default location is defined by QStandardPaths::CacheLocation.
    If there is no writable QStandartPaths::CacheLocation and \a storeDir is an
    empty string, the store will be located in the program's working directory.

    \note If HSTS cache already contains HSTS policies by the time persistent
    store is enabled, these policies will be preserved in the store. In case both
    cache and store contain the same known hosts, policies from cache are considered
    to be more up-to-date (and thus will overwrite the previous values in the store).
    If this behavior is undesired, enable HSTS store before enabling Strict Tranport
    Security. By default, the persistent store of HSTS policies is disabled.

    \sa isStrictTransportSecurityStoreEnabled(), setStrictTransportSecurityEnabled(),
    QStandardPaths::standardLocations()
*/

void QNetworkAccessManager::enableStrictTransportSecurityStore(bool enabled, const QString &storeDir)
{
#if QT_CONFIG(settings)
    Q_D(QNetworkAccessManager);
    d->stsStore.reset(enabled ? new QHstsStore(storeDir) : nullptr);
    d->stsCache.setStore(d->stsStore.data());
#else
    Q_UNUSED(enabled) Q_UNUSED(storeDir)
    qWarning("HSTS permanent store requires the feature 'settings' enabled");
#endif // QT_CONFIG(settings)
}

/*!
    \since 5.10

    Returns true if HSTS cache uses a permanent store to load and store HSTS
    policies.

    \sa enableStrictTransportSecurityStore()
*/

bool QNetworkAccessManager::isStrictTransportSecurityStoreEnabled() const
{
#if QT_CONFIG(settings)
    Q_D(const QNetworkAccessManager);
    return bool(d->stsStore.data());
#else
    return false;
#endif // QT_CONFIG(settings)
}

/*!
    \since 5.9

    Adds HTTP Strict Transport Security policies into HSTS cache.
    \a knownHosts contains the known hosts that have QHstsPolicy
    information.

    \note An expired policy will remove a known host from the cache, if previously
    present.

    \note While processing HTTP responses, QNetworkAccessManager can also update
    the HSTS cache, removing or updating exitsting policies or introducing new
    \a knownHosts. The current implementation thus is server-driven, client code
    can provide QNetworkAccessManager with previously known or discovered
    policies, but this information can be overridden by "Strict-Transport-Security"
    response headers.

    \sa addStrictTransportSecurityHosts(), enableStrictTransportSecurityStore(), QHstsPolicy
*/

void QNetworkAccessManager::addStrictTransportSecurityHosts(const QVector<QHstsPolicy> &knownHosts)
{
    Q_D(QNetworkAccessManager);
    d->stsCache.updateFromPolicies(knownHosts);
}

/*!
    \since 5.9

    Returns the list of HTTP Strict Transport Security policies. This list can
    differ from what was initially set via addStrictTransportSecurityHosts() if
    HSTS cache was updated from a "Strict-Transport-Security" response header.

    \sa addStrictTransportSecurityHosts(), QHstsPolicy
*/
QVector<QHstsPolicy> QNetworkAccessManager::strictTransportSecurityHosts() const
{
    Q_D(const QNetworkAccessManager);
    return d->stsCache.policies();
}

/*!
    Posts a request to obtain the network headers for \a request
    and returns a new QNetworkReply object which will contain such headers.

    The function is named after the HTTP request associated (HEAD).
*/
QNetworkReply *QNetworkAccessManager::head(const QNetworkRequest &request)
{
    return d_func()->postProcess(createRequest(QNetworkAccessManager::HeadOperation, request));
}

/*!
    Posts a request to obtain the contents of the target \a request
    and returns a new QNetworkReply object opened for reading which emits the
    \l{QIODevice::readyRead()}{readyRead()} signal whenever new data
    arrives.

    The contents as well as associated headers will be downloaded.

    \sa post(), put(), deleteResource(), sendCustomRequest()
*/
QNetworkReply *QNetworkAccessManager::get(const QNetworkRequest &request)
{
    return d_func()->postProcess(createRequest(QNetworkAccessManager::GetOperation, request));
}

/*!
    Sends an HTTP POST request to the destination specified by \a request
    and returns a new QNetworkReply object opened for reading that will
    contain the reply sent by the server. The contents of  the \a data
    device will be uploaded to the server.

    \a data must be open for reading and must remain valid until the
    finished() signal is emitted for this reply.

    \note Sending a POST request on protocols other than HTTP and
    HTTPS is undefined and will probably fail.

    \sa get(), put(), deleteResource(), sendCustomRequest()
*/
QNetworkReply *QNetworkAccessManager::post(const QNetworkRequest &request, QIODevice *data)
{
    return d_func()->postProcess(createRequest(QNetworkAccessManager::PostOperation, request, data));
}

/*!
    \overload

    Sends the contents of the \a data byte array to the destination
    specified by \a request.
*/
QNetworkReply *QNetworkAccessManager::post(const QNetworkRequest &request, const QByteArray &data)
{
    QBuffer *buffer = new QBuffer;
    buffer->setData(data);
    buffer->open(QIODevice::ReadOnly);

    QNetworkReply *reply = post(request, buffer);
    buffer->setParent(reply);
    return reply;
}

#if QT_CONFIG(http)
/*!
    \since 4.8

    \overload

    Sends the contents of the \a multiPart message to the destination
    specified by \a request.

    This can be used for sending MIME multipart messages over HTTP.

    \sa QHttpMultiPart, QHttpPart, put()
*/
QNetworkReply *QNetworkAccessManager::post(const QNetworkRequest &request, QHttpMultiPart *multiPart)
{
    QNetworkRequest newRequest = d_func()->prepareMultipart(request, multiPart);
    QIODevice *device = multiPart->d_func()->device;
    QNetworkReply *reply = post(newRequest, device);
    return reply;
}

/*!
    \since 4.8

    \overload

    Sends the contents of the \a multiPart message to the destination
    specified by \a request.

    This can be used for sending MIME multipart messages over HTTP.

    \sa QHttpMultiPart, QHttpPart, post()
*/
QNetworkReply *QNetworkAccessManager::put(const QNetworkRequest &request, QHttpMultiPart *multiPart)
{
    QNetworkRequest newRequest = d_func()->prepareMultipart(request, multiPart);
    QIODevice *device = multiPart->d_func()->device;
    QNetworkReply *reply = put(newRequest, device);
    return reply;
}
#endif // QT_CONFIG(http)

/*!
    Uploads the contents of \a data to the destination \a request and
    returns a new QNetworkReply object that will be open for reply.

    \a data must be opened for reading when this function is called
    and must remain valid until the finished() signal is emitted for
    this reply.

    Whether anything will be available for reading from the returned
    object is protocol dependent. For HTTP, the server may send a
    small HTML page indicating the upload was successful (or not).
    Other protocols will probably have content in their replies.

    \note For HTTP, this request will send a PUT request, which most servers
    do not allow. Form upload mechanisms, including that of uploading
    files through HTML forms, use the POST mechanism.

    \sa get(), post(), deleteResource(), sendCustomRequest()
*/
QNetworkReply *QNetworkAccessManager::put(const QNetworkRequest &request, QIODevice *data)
{
    return d_func()->postProcess(createRequest(QNetworkAccessManager::PutOperation, request, data));
}

/*!
    \overload

    Sends the contents of the \a data byte array to the destination
    specified by \a request.
*/
QNetworkReply *QNetworkAccessManager::put(const QNetworkRequest &request, const QByteArray &data)
{
    QBuffer *buffer = new QBuffer;
    buffer->setData(data);
    buffer->open(QIODevice::ReadOnly);

    QNetworkReply *reply = put(request, buffer);
    buffer->setParent(reply);
    return reply;
}

/*!
    \since 4.6

    Sends a request to delete the resource identified by the URL of \a request.

    \note This feature is currently available for HTTP only, performing an
    HTTP DELETE request.

    \sa get(), post(), put(), sendCustomRequest()
*/
QNetworkReply *QNetworkAccessManager::deleteResource(const QNetworkRequest &request)
{
    return d_func()->postProcess(createRequest(QNetworkAccessManager::DeleteOperation, request));
}

#ifndef QT_NO_BEARERMANAGEMENT

/*!
    \since 4.7

    Sets the network configuration that will be used when creating the
    \l {QNetworkSession}{network session} to \a config.

    The network configuration is used to create and open a network session before any request that
    requires network access is process.  If no network configuration is explicitly set via this
    function the network configuration returned by
    QNetworkConfigurationManager::defaultConfiguration() will be used.

    To restore the default network configuration set the network configuration to the value
    returned from QNetworkConfigurationManager::defaultConfiguration().

    Setting a network configuration means that the QNetworkAccessManager instance will only
    be using the specified one. In particular, if the default network configuration changes
    (upon e.g. Wifi being available), this new configuration needs to be enabled
    manually if desired.

    \snippet code/src_network_access_qnetworkaccessmanager.cpp 2

    If an invalid network configuration is set, a network session will not be created.  In this
    case network requests will be processed regardless, but may fail.  For example:

    \snippet code/src_network_access_qnetworkaccessmanager.cpp 3

    \sa configuration(), QNetworkSession
*/
void QNetworkAccessManager::setConfiguration(const QNetworkConfiguration &config)
{
    Q_D(QNetworkAccessManager);
    d->networkConfiguration = config;
    d->customNetworkConfiguration = true;
    d->createSession(config);
}

/*!
    \since 4.7

    Returns the network configuration that will be used to create the
    \l {QNetworkSession}{network session} which will be used when processing network requests.

    \sa setConfiguration(), activeConfiguration()
*/
QNetworkConfiguration QNetworkAccessManager::configuration() const
{
    Q_D(const QNetworkAccessManager);

    QSharedPointer<QNetworkSession> session(d->getNetworkSession());
    if (session) {
        return session->configuration();
    } else {
        return d->networkConfigurationManager.defaultConfiguration();
    }
}

/*!
    \since 4.7

    Returns the current active network configuration.

    If the network configuration returned by configuration() is of type
    QNetworkConfiguration::ServiceNetwork this function will return the current active child
    network configuration of that configuration.  Otherwise returns the same network configuration
    as configuration().

    Use this function to return the actual network configuration currently in use by the network
    session.

    \sa configuration()
*/
QNetworkConfiguration QNetworkAccessManager::activeConfiguration() const
{
    Q_D(const QNetworkAccessManager);

    QSharedPointer<QNetworkSession> networkSession(d->getNetworkSession());
    if (networkSession) {
        return d->networkConfigurationManager.configurationFromIdentifier(
            networkSession->sessionProperty(QLatin1String("ActiveConfiguration")).toString());
    } else {
        return d->networkConfigurationManager.defaultConfiguration();
    }
}

/*!
    \since 4.7

    Overrides the reported network accessibility.  If \a accessible is NotAccessible the reported
    network accessiblity will always be NotAccessible.  Otherwise the reported network
    accessibility will reflect the actual device state.
*/
void QNetworkAccessManager::setNetworkAccessible(QNetworkAccessManager::NetworkAccessibility accessible)
{
    Q_D(QNetworkAccessManager);

    d->defaultAccessControl = accessible == NotAccessible ? false : true;

    if (d->networkAccessible != accessible) {
        NetworkAccessibility previous = networkAccessible();
        d->networkAccessible = accessible;
        NetworkAccessibility current = networkAccessible();
        if (previous != current)
            emit networkAccessibleChanged(current);
    }
}

/*!
    \since 4.7

    Returns the current network accessibility.
*/
QNetworkAccessManager::NetworkAccessibility QNetworkAccessManager::networkAccessible() const
{
    Q_D(const QNetworkAccessManager);

    if (d->customNetworkConfiguration && d->networkConfiguration.state().testFlag(QNetworkConfiguration::Undefined))
        return UnknownAccessibility;

    if (d->networkSessionRequired) {
        QSharedPointer<QNetworkSession> networkSession(d->getNetworkSession());
        if (networkSession) {
            // d->online holds online/offline state of this network session.
            if (d->online)
                return d->networkAccessible;
            else
                return NotAccessible;
        } else {
            if (d->defaultAccessControl) {
                if (d->online)
                    return d->networkAccessible;
                else
                    return NotAccessible;
            }
            return (d->networkAccessible);
        }
    } else {
        if (d->online)
            return d->networkAccessible;
        else
            return NotAccessible;
    }
}

/*!
    \internal

    Returns the network session currently in use.
    This can be changed at any time, ownership remains with the QNetworkAccessManager
*/
const QWeakPointer<const QNetworkSession> QNetworkAccessManagerPrivate::getNetworkSession(const QNetworkAccessManager *q)
{
    return q->d_func()->networkSessionWeakRef;
}

QSharedPointer<QNetworkSession> QNetworkAccessManagerPrivate::getNetworkSession() const
{
    if (networkSessionStrongRef)
        return networkSessionStrongRef;
    return networkSessionWeakRef.toStrongRef();
}

#endif // QT_NO_BEARERMANAGEMENT


#ifndef QT_NO_SSL
/*!
    \since 5.2

    Initiates a connection to the host given by \a hostName at port \a port, using
    \a sslConfiguration. This function is useful to complete the TCP and SSL handshake
    to a host before the HTTPS request is made, resulting in a lower network latency.

    \note Preconnecting a SPDY connection can be done by calling setAllowedNextProtocols()
    on \a sslConfiguration with QSslConfiguration::NextProtocolSpdy3_0 contained in
    the list of allowed protocols. When using SPDY, one single connection per host is
    enough, i.e. calling this method multiple times per host will not result in faster
    network transactions.

    \note This function has no possibility to report errors.

    \sa connectToHost(), get(), post(), put(), deleteResource()
*/
void QNetworkAccessManager::connectToHostEncrypted(const QString &hostName, quint16 port,
                                                   const QSslConfiguration &sslConfiguration)
{
    QUrl url;
    url.setHost(hostName);
    url.setPort(port);
    url.setScheme(QLatin1String("preconnect-https"));
    QNetworkRequest request(url);
    if (sslConfiguration != QSslConfiguration::defaultConfiguration())
        request.setSslConfiguration(sslConfiguration);

    // There is no way to enable SPDY via a request, so we need to check
    // the ssl configuration whether SPDY is allowed here.
    if (sslConfiguration.allowedNextProtocols().contains(
                QSslConfiguration::NextProtocolSpdy3_0))
        request.setAttribute(QNetworkRequest::SpdyAllowedAttribute, true);

    get(request);
}
#endif

/*!
    \since 5.2

    Initiates a connection to the host given by \a hostName at port \a port.
    This function is useful to complete the TCP handshake
    to a host before the HTTP request is made, resulting in a lower network latency.

    \note This function has no possibility to report errors.

    \sa connectToHostEncrypted(), get(), post(), put(), deleteResource()
*/
void QNetworkAccessManager::connectToHost(const QString &hostName, quint16 port)
{
    QUrl url;
    url.setHost(hostName);
    url.setPort(port);
    url.setScheme(QLatin1String("preconnect-http"));
    QNetworkRequest request(url);
    get(request);
}

/*!
    \since 5.9

    Sets the manager's redirect policy to be the \a policy specified. This policy
    will affect all subsequent requests created by the manager.

    Use this function to enable or disable HTTP redirects on the manager's level.

    \note When creating a request QNetworkRequest::RedirectAttributePolicy has
    the highest priority, next by priority is QNetworkRequest::FollowRedirectsAttribute.
    Finally, the manager's policy has the lowest priority.

    For backwards compatibility the default value is QNetworkRequest::ManualRedirectPolicy.
    This may change in the future and some type of auto-redirect policy will become
    the default; clients relying on manual redirect handling are encouraged to set
    this policy explicitly in their code.

    \sa redirectPolicy(), QNetworkRequest::RedirectPolicy,
    QNetworkRequest::FollowRedirectsAttribute
*/
void QNetworkAccessManager::setRedirectPolicy(QNetworkRequest::RedirectPolicy policy)
{
    Q_D(QNetworkAccessManager);
    d->redirectPolicy = policy;
}

/*!
    \since 5.9

    Returns the redirect policy that is used when creating new requests.

    \sa setRedirectPolicy(), QNetworkRequest::RedirectPolicy
*/
QNetworkRequest::RedirectPolicy QNetworkAccessManager::redirectPolicy() const
{
    Q_D(const QNetworkAccessManager);
    return d->redirectPolicy;
}

/*!
    \since 4.7

    Sends a custom request to the server identified by the URL of \a request.

    It is the user's responsibility to send a \a verb to the server that is valid
    according to the HTTP specification.

    This method provides means to send verbs other than the common ones provided
    via get() or post() etc., for instance sending an HTTP OPTIONS command.

    If \a data is not empty, the contents of the \a data
    device will be uploaded to the server; in that case, data must be open for
    reading and must remain valid until the finished() signal is emitted for this reply.

    \note This feature is currently available for HTTP(S) only.

    \sa get(), post(), put(), deleteResource()
*/
QNetworkReply *QNetworkAccessManager::sendCustomRequest(const QNetworkRequest &request, const QByteArray &verb, QIODevice *data)
{
    QNetworkRequest newRequest(request);
    newRequest.setAttribute(QNetworkRequest::CustomVerbAttribute, verb);
    return d_func()->postProcess(createRequest(QNetworkAccessManager::CustomOperation, newRequest, data));
}

/*!
    \since 5.8

    \overload

    Sends the contents of the \a data byte array to the destination
    specified by \a request.
*/
QNetworkReply *QNetworkAccessManager::sendCustomRequest(const QNetworkRequest &request, const QByteArray &verb, const QByteArray &data)
{
    QBuffer *buffer = new QBuffer;
    buffer->setData(data);
    buffer->open(QIODevice::ReadOnly);

    QNetworkReply *reply = sendCustomRequest(request, verb, buffer);
    buffer->setParent(reply);
    return reply;
}

#if QT_CONFIG(http)
/*!
    \since 5.8

    \overload

    Sends a custom request to the server identified by the URL of \a request.

    Sends the contents of the \a multiPart message to the destination
    specified by \a request.

    This can be used for sending MIME multipart messages for custom verbs.

    \sa QHttpMultiPart, QHttpPart, put()
*/
QNetworkReply *QNetworkAccessManager::sendCustomRequest(const QNetworkRequest &request, const QByteArray &verb, QHttpMultiPart *multiPart)
{
    QNetworkRequest newRequest = d_func()->prepareMultipart(request, multiPart);
    QIODevice *device = multiPart->d_func()->device;
    QNetworkReply *reply = sendCustomRequest(newRequest, verb, device);
    return reply;
}
#endif // QT_CONFIG(http)

/*!
    Returns a new QNetworkReply object to handle the operation \a op
    and request \a originalReq. The device \a outgoingData is always 0
    for Get and Head requests, but is the value passed to post() and
    put() in those operations (the QByteArray variants will pass a QBuffer
    object).

    The default implementation calls QNetworkCookieJar::cookiesForUrl()
    on the cookie jar set with setCookieJar() to obtain the cookies to
    be sent to the remote server.

    The returned object must be in an open state.
*/
QNetworkReply *QNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op,
                                                    const QNetworkRequest &originalReq,
                                                    QIODevice *outgoingData)
{
    Q_D(QNetworkAccessManager);

    QNetworkRequest req(originalReq);
    if (redirectPolicy() != QNetworkRequest::ManualRedirectPolicy
        && req.attribute(QNetworkRequest::RedirectPolicyAttribute).isNull()
        && req.attribute(QNetworkRequest::FollowRedirectsAttribute).isNull()) {
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, redirectPolicy());
    }

    bool isLocalFile = req.url().isLocalFile();
    QString scheme = req.url().scheme();

#ifdef Q_OS_WASM
    if (scheme == QLatin1String("http") || scheme == QLatin1String("https")) {
        QNetworkReplyWasmImpl *reply = new QNetworkReplyWasmImpl(this);
        QNetworkReplyWasmImplPrivate *priv = reply->d_func();
        priv->manager = this;
        priv->setup(op, req, outgoingData);
        return reply;
    }
#endif

    // fast path for GET on file:// URLs
    // The QNetworkAccessFileBackend will right now only be used for PUT
    if (op == QNetworkAccessManager::GetOperation
     || op == QNetworkAccessManager::HeadOperation) {
        if (isLocalFile
#ifdef Q_OS_ANDROID
            || scheme == QLatin1String("assets")
#endif
            || scheme == QLatin1String("qrc")) {
            return new QNetworkReplyFileImpl(this, req, op);
        }

        if (scheme == QLatin1String("data"))
            return new QNetworkReplyDataImpl(this, req, op);

        // A request with QNetworkRequest::AlwaysCache does not need any bearer management
        QNetworkRequest::CacheLoadControl mode =
            static_cast<QNetworkRequest::CacheLoadControl>(
                req.attribute(QNetworkRequest::CacheLoadControlAttribute,
                              QNetworkRequest::PreferNetwork).toInt());
        if (mode == QNetworkRequest::AlwaysCache) {
            // FIXME Implement a QNetworkReplyCacheImpl instead, see QTBUG-15106
            QNetworkReplyImpl *reply = new QNetworkReplyImpl(this);
            QNetworkReplyImplPrivate *priv = reply->d_func();
            priv->manager = this;
            priv->backend = new QNetworkAccessCacheBackend();
            priv->backend->manager = this->d_func();
            priv->backend->setParent(reply);
            priv->backend->reply = priv;
            priv->setup(op, req, outgoingData);
            return reply;
        }
    }

#ifndef QT_NO_BEARERMANAGEMENT

    // Return a disabled network reply if network access is disabled.
    // Except if the scheme is empty or file:// or if the host resolves to a loopback address.
    if (d->networkAccessible == NotAccessible && !isLocalFile) {
        QHostAddress dest;
        QString host = req.url().host().toLower();
        if (!(dest.setAddress(host) && dest.isLoopback()) && host != QLatin1String("localhost")
                && host != QHostInfo::localHostName().toLower()) {
            return new QDisabledNetworkReply(this, req, op);
        }
    }

    if (!d->networkSessionStrongRef && (d->initializeSession || !d->networkConfiguration.identifier().isEmpty())) {
        if (!d->networkConfiguration.identifier().isEmpty()) {
            if ((d->networkConfiguration.state() & QNetworkConfiguration::Defined)
                    && d->networkConfiguration != d->networkConfigurationManager.defaultConfiguration())
                d->createSession(d->networkConfigurationManager.defaultConfiguration());
            else
                d->createSession(d->networkConfiguration);

        } else {
            if (d->networkSessionRequired)
                d->createSession(d->networkConfigurationManager.defaultConfiguration());
            else
                d->initializeSession = false;
        }
    }
#endif

    QNetworkRequest request = req;
    if (!request.header(QNetworkRequest::ContentLengthHeader).isValid() &&
        outgoingData && !outgoingData->isSequential()) {
        // request has no Content-Length
        // but the data that is outgoing is random-access
        request.setHeader(QNetworkRequest::ContentLengthHeader, outgoingData->size());
    }

    if (static_cast<QNetworkRequest::LoadControl>
        (request.attribute(QNetworkRequest::CookieLoadControlAttribute,
                           QNetworkRequest::Automatic).toInt()) == QNetworkRequest::Automatic) {
        if (d->cookieJar) {
            QList<QNetworkCookie> cookies = d->cookieJar->cookiesForUrl(request.url());
            if (!cookies.isEmpty())
                request.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(cookies));
        }
    }

#if QT_CONFIG(http)
    // Since Qt 5 we use the new QNetworkReplyHttpImpl
    if (scheme == QLatin1String("http") || scheme == QLatin1String("preconnect-http")
#ifndef QT_NO_SSL
        || scheme == QLatin1String("https") || scheme == QLatin1String("preconnect-https")
#endif
        ) {
#ifndef QT_NO_SSL
        if (isStrictTransportSecurityEnabled() && d->stsCache.isKnownHost(request.url())) {
            QUrl stsUrl(request.url());
            // RFC6797, 8.3:
            // The UA MUST replace the URI scheme with "https" [RFC2818],
            // and if the URI contains an explicit port component of "80",
            // then the UA MUST convert the port component to be "443", or
            // if the URI contains an explicit port component that is not
            // equal to "80", the port component value MUST be preserved;
            // otherwise,
            // if the URI does not contain an explicit port component, the UA
            // MUST NOT add one.
            if (stsUrl.port() == 80)
                stsUrl.setPort(443);
            stsUrl.setScheme(QLatin1String("https"));
            request.setUrl(stsUrl);
        }
#endif
        QNetworkReplyHttpImpl *reply = new QNetworkReplyHttpImpl(this, request, op, outgoingData);
#ifndef QT_NO_BEARERMANAGEMENT
        connect(this, SIGNAL(networkSessionConnected()),
                reply, SLOT(_q_networkSessionConnected()));
#endif
        return reply;
    }
#endif // QT_CONFIG(http)

    // first step: create the reply
    QNetworkReplyImpl *reply = new QNetworkReplyImpl(this);
#ifndef QT_NO_BEARERMANAGEMENT
    if (!isLocalFile) {
        connect(this, SIGNAL(networkSessionConnected()),
                reply, SLOT(_q_networkSessionConnected()));
    }
#endif
    QNetworkReplyImplPrivate *priv = reply->d_func();
    priv->manager = this;

    // second step: fetch cached credentials
    // This is not done for the time being, we should use signal emissions to request
    // the credentials from cache.

    // third step: find a backend
    priv->backend = d->findBackend(op, request);

    if (priv->backend) {
        priv->backend->setParent(reply);
        priv->backend->reply = priv;
    }

#ifndef QT_NO_SSL
    reply->setSslConfiguration(request.sslConfiguration());
#endif

    // fourth step: setup the reply
    priv->setup(op, request, outgoingData);

    return reply;
}

/*!
    \since 5.2

    Lists all the URL schemes supported by the access manager.

    \sa supportedSchemesImplementation()
*/
QStringList QNetworkAccessManager::supportedSchemes() const
{
    QStringList schemes;
    QNetworkAccessManager *self = const_cast<QNetworkAccessManager *>(this); // We know we call a const slot
    QMetaObject::invokeMethod(self, "supportedSchemesImplementation", Qt::DirectConnection,
                              Q_RETURN_ARG(QStringList, schemes));
    schemes.removeDuplicates();
    return schemes;
}

/*!
    \since 5.2

    Lists all the URL schemes supported by the access manager.

    You should not call this function directly; use
    QNetworkAccessManager::supportedSchemes() instead.

    Reimplement this slot to provide your own supported schemes
    in a QNetworkAccessManager subclass. It is for instance necessary
    when your subclass provides support for new protocols.

    Because of binary compatibility constraints, the supportedSchemes()
    method (introduced in Qt 5.2) is not virtual. Instead, supportedSchemes()
    will dynamically detect and call this slot.

    \sa supportedSchemes()
*/
QStringList QNetworkAccessManager::supportedSchemesImplementation() const
{
    Q_D(const QNetworkAccessManager);

    QStringList schemes = d->backendSupportedSchemes();
    // Those ones don't exist in backends
#if QT_CONFIG(http)
    schemes << QStringLiteral("http");
#ifndef QT_NO_SSL
    if (QSslSocket::supportsSsl())
        schemes << QStringLiteral("https");
#endif
#endif
    schemes << QStringLiteral("data");
    return schemes;
}

/*!
    \since 5.0

    Flushes the internal cache of authentication data and network connections.

    This function is useful for doing auto tests.

    \sa clearConnectionCache()
*/
void QNetworkAccessManager::clearAccessCache()
{
    QNetworkAccessManagerPrivate::clearAuthenticationCache(this);
    QNetworkAccessManagerPrivate::clearConnectionCache(this);
}

/*!
    \since 5.9

    Flushes the internal cache of network connections.
    In contrast to clearAccessCache() the authentication data
    is preserved.

    \sa clearAccessCache()
*/
void QNetworkAccessManager::clearConnectionCache()
{
    QNetworkAccessManagerPrivate::clearConnectionCache(this);
}

void QNetworkAccessManagerPrivate::_q_replyFinished()
{
    Q_Q(QNetworkAccessManager);

    QNetworkReply *reply = qobject_cast<QNetworkReply *>(q->sender());
    if (reply)
        emit q->finished(reply);

#ifndef QT_NO_BEARERMANAGEMENT
    // If there are no active requests, release our reference to the network session.
    // It will not be destroyed immediately, but rather when the connection cache is flushed
    // after 2 minutes.
    activeReplyCount--;
    if (networkSessionStrongRef && activeReplyCount == 0)
        networkSessionStrongRef.clear();
#endif
}

void QNetworkAccessManagerPrivate::_q_replyEncrypted()
{
#ifndef QT_NO_SSL
    Q_Q(QNetworkAccessManager);
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(q->sender());
    if (reply)
        emit q->encrypted(reply);
#endif
}

void QNetworkAccessManagerPrivate::_q_replySslErrors(const QList<QSslError> &errors)
{
#ifndef QT_NO_SSL
    Q_Q(QNetworkAccessManager);
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(q->sender());
    if (reply)
        emit q->sslErrors(reply, errors);
#else
    Q_UNUSED(errors);
#endif
}

void QNetworkAccessManagerPrivate::_q_replyPreSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator *authenticator)
{
#ifndef QT_NO_SSL
    Q_Q(QNetworkAccessManager);
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(q->sender());
    if (reply)
    emit q->preSharedKeyAuthenticationRequired(reply, authenticator);
#else
    Q_UNUSED(authenticator);
#endif
}

QNetworkReply *QNetworkAccessManagerPrivate::postProcess(QNetworkReply *reply)
{
    Q_Q(QNetworkAccessManager);
    QNetworkReplyPrivate::setManager(reply, q);
    q->connect(reply, SIGNAL(finished()), SLOT(_q_replyFinished()));
#ifndef QT_NO_SSL
    /* In case we're compiled without SSL support, we don't have this signal and we need to
     * avoid getting a connection error. */
    q->connect(reply, SIGNAL(encrypted()), SLOT(_q_replyEncrypted()));
    q->connect(reply, SIGNAL(sslErrors(QList<QSslError>)), SLOT(_q_replySslErrors(QList<QSslError>)));
    q->connect(reply, SIGNAL(preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)), SLOT(_q_replyPreSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator*)));
#endif
#ifndef QT_NO_BEARERMANAGEMENT
    activeReplyCount++;
#endif

    return reply;
}

void QNetworkAccessManagerPrivate::createCookieJar() const
{
    if (!cookieJarCreated) {
        // keep the ugly hack in here
        QNetworkAccessManagerPrivate *that = const_cast<QNetworkAccessManagerPrivate *>(this);
        that->cookieJar = new QNetworkCookieJar(that->q_func());
        that->cookieJarCreated = true;
    }
}

void QNetworkAccessManagerPrivate::authenticationRequired(QAuthenticator *authenticator,
                                                          QNetworkReply *reply,
                                                          bool synchronous,
                                                          QUrl &url,
                                                          QUrl *urlForLastAuthentication,
                                                          bool allowAuthenticationReuse)
{
    Q_Q(QNetworkAccessManager);

    // don't try the cache for the same URL twice in a row
    // being called twice for the same URL means the authentication failed
    // also called when last URL is empty, e.g. on first call
    if (allowAuthenticationReuse && (urlForLastAuthentication->isEmpty()
            || url != *urlForLastAuthentication)) {
        // if credentials are included in the url, then use them
        if (!url.userName().isEmpty()
            && !url.password().isEmpty()) {
            authenticator->setUser(url.userName(QUrl::FullyDecoded));
            authenticator->setPassword(url.password(QUrl::FullyDecoded));
            *urlForLastAuthentication = url;
            authenticationManager->cacheCredentials(url, authenticator);
            return;
        }

        QNetworkAuthenticationCredential cred = authenticationManager->fetchCachedCredentials(url, authenticator);
        if (!cred.isNull()) {
            authenticator->setUser(cred.user);
            authenticator->setPassword(cred.password);
            *urlForLastAuthentication = url;
            return;
        }
    }

    // if we emit a signal here in synchronous mode, the user might spin
    // an event loop, which might recurse and lead to problems
    if (synchronous)
        return;

    *urlForLastAuthentication = url;
    emit q->authenticationRequired(reply, authenticator);
    if (allowAuthenticationReuse)
        authenticationManager->cacheCredentials(url, authenticator);
}

#ifndef QT_NO_NETWORKPROXY
void QNetworkAccessManagerPrivate::proxyAuthenticationRequired(const QUrl &url,
                                                               const QNetworkProxy &proxy,
                                                               bool synchronous,
                                                               QAuthenticator *authenticator,
                                                               QNetworkProxy *lastProxyAuthentication)
{
    Q_Q(QNetworkAccessManager);
    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(*authenticator);
    if (proxy != *lastProxyAuthentication && (!priv || !priv->hasFailed)) {
        QNetworkAuthenticationCredential cred = authenticationManager->fetchCachedProxyCredentials(proxy);
        if (!cred.isNull()) {
            authenticator->setUser(cred.user);
            authenticator->setPassword(cred.password);
            return;
        }
    }

#if defined(Q_OS_OSX)
    //now we try to get the username and password from keychain
    //if not successful signal will be emitted
    QString username;
    QString password;
    if (getProxyAuth(proxy.hostName(), url.scheme(), username, password)) {
        // only cache the system credentials if they are correct (or if they have changed)
        // to not run into an endless loop in case they are wrong
        QNetworkAuthenticationCredential cred = authenticationManager->fetchCachedProxyCredentials(proxy);
        if (!priv->hasFailed || cred.user != username || cred.password != password) {
            authenticator->setUser(username);
            authenticator->setPassword(password);
            authenticationManager->cacheProxyCredentials(proxy, authenticator);
            return;
        }
    }
#else
    Q_UNUSED(url);
#endif

    // if we emit a signal here in synchronous mode, the user might spin
    // an event loop, which might recurse and lead to problems
    if (synchronous)
        return;

    *lastProxyAuthentication = proxy;
    emit q->proxyAuthenticationRequired(proxy, authenticator);
    authenticationManager->cacheProxyCredentials(proxy, authenticator);
}

QList<QNetworkProxy> QNetworkAccessManagerPrivate::queryProxy(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> proxies;
    if (proxyFactory) {
        proxies = proxyFactory->queryProxy(query);
        if (proxies.isEmpty()) {
            qWarning("QNetworkAccessManager: factory %p has returned an empty result set",
                     proxyFactory);
            proxies << QNetworkProxy::NoProxy;
        }
    } else if (proxy.type() == QNetworkProxy::DefaultProxy) {
        // no proxy set, query the application
        return QNetworkProxyFactory::proxyForQuery(query);
    } else {
        proxies << proxy;
    }

    return proxies;
}
#endif

void QNetworkAccessManagerPrivate::clearAuthenticationCache(QNetworkAccessManager *manager)
{
    manager->d_func()->authenticationManager->clearCache();
}

void QNetworkAccessManagerPrivate::clearConnectionCache(QNetworkAccessManager *manager)
{
    manager->d_func()->objectCache.clear();
    manager->d_func()->destroyThread();
}

QNetworkAccessManagerPrivate::~QNetworkAccessManagerPrivate()
{
    destroyThread();
}

QThread * QNetworkAccessManagerPrivate::createThread()
{
    if (!thread) {
        thread = new QThread;
        thread->setObjectName(QStringLiteral("QNetworkAccessManager thread"));
        thread->start();
    }
    Q_ASSERT(thread);
    return thread;
}

void QNetworkAccessManagerPrivate::destroyThread()
{
    if (thread) {
        thread->quit();
        thread->wait(5000);
        if (thread->isFinished())
            delete thread;
        else
            QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        thread = nullptr;
    }
}

#ifndef QT_NO_BEARERMANAGEMENT
void QNetworkAccessManagerPrivate::createSession(const QNetworkConfiguration &config)
{
    Q_Q(QNetworkAccessManager);

    initializeSession = false;

    //resurrect weak ref if possible
    networkSessionStrongRef = networkSessionWeakRef.toStrongRef();

    QSharedPointer<QNetworkSession> newSession;
    if (config.isValid())
        newSession = QSharedNetworkSessionManager::getSession(config);

    if (networkSessionStrongRef) {
        //do nothing if new and old session are the same
        if (networkSessionStrongRef == newSession)
            return;
        //disconnect from old session
        QObject::disconnect(networkSessionStrongRef.data(), SIGNAL(opened()), q, SIGNAL(networkSessionConnected()));
        QObject::disconnect(networkSessionStrongRef.data(), SIGNAL(closed()), q, SLOT(_q_networkSessionClosed()));
        QObject::disconnect(networkSessionStrongRef.data(), SIGNAL(stateChanged(QNetworkSession::State)),
            q, SLOT(_q_networkSessionStateChanged(QNetworkSession::State)));
        QObject::disconnect(networkSessionStrongRef.data(), SIGNAL(error(QNetworkSession::SessionError)),
                            q, SLOT(_q_networkSessionFailed(QNetworkSession::SessionError)));
    }

    //switch to new session (null if config was invalid)
    networkSessionStrongRef = newSession;
    networkSessionWeakRef = networkSessionStrongRef.toWeakRef();

    if (!networkSessionStrongRef) {

        if (networkAccessible == QNetworkAccessManager::NotAccessible || !online)
            emit q->networkAccessibleChanged(QNetworkAccessManager::NotAccessible);
        else
            emit q->networkAccessibleChanged(QNetworkAccessManager::UnknownAccessibility);

        return;
    }

    //connect to new session
    QObject::connect(networkSessionStrongRef.data(), SIGNAL(opened()), q, SIGNAL(networkSessionConnected()), Qt::QueuedConnection);
    //QueuedConnection is used to avoid deleting the networkSession inside its closed signal
    QObject::connect(networkSessionStrongRef.data(), SIGNAL(closed()), q, SLOT(_q_networkSessionClosed()), Qt::QueuedConnection);
    QObject::connect(networkSessionStrongRef.data(), SIGNAL(stateChanged(QNetworkSession::State)),
                     q, SLOT(_q_networkSessionStateChanged(QNetworkSession::State)), Qt::QueuedConnection);
    QObject::connect(networkSessionStrongRef.data(), SIGNAL(error(QNetworkSession::SessionError)),
                        q, SLOT(_q_networkSessionFailed(QNetworkSession::SessionError)));

    _q_networkSessionStateChanged(networkSessionStrongRef->state());
}

void QNetworkAccessManagerPrivate::_q_networkSessionClosed()
{
    Q_Q(QNetworkAccessManager);
    QSharedPointer<QNetworkSession> networkSession(getNetworkSession());
    if (networkSession) {
        networkConfiguration = networkSession->configuration();

        //disconnect from old session
        QObject::disconnect(networkSession.data(), SIGNAL(opened()), q, SIGNAL(networkSessionConnected()));
        QObject::disconnect(networkSession.data(), SIGNAL(closed()), q, SLOT(_q_networkSessionClosed()));
        QObject::disconnect(networkSession.data(), SIGNAL(stateChanged(QNetworkSession::State)),
            q, SLOT(_q_networkSessionStateChanged(QNetworkSession::State)));
        QObject::disconnect(networkSession.data(), SIGNAL(error(QNetworkSession::SessionError)),
                            q, SLOT(_q_networkSessionFailed(QNetworkSession::SessionError)));

        networkSessionStrongRef.clear();
        networkSessionWeakRef.clear();
    }
}

void QNetworkAccessManagerPrivate::_q_networkSessionStateChanged(QNetworkSession::State state)
{
    Q_Q(QNetworkAccessManager);
    bool reallyOnline = false;
    //Do not emit the networkSessionConnected signal here, except for roaming -> connected
    //transition, otherwise it is emitted twice in a row when opening a connection.
    if (state == QNetworkSession::Connected && lastSessionState != QNetworkSession::Roaming)
        emit q->networkSessionConnected();
    lastSessionState = state;

    if (online && (state == QNetworkSession::Disconnected
                   || state == QNetworkSession::NotAvailable)) {
        const auto cfgs = networkConfigurationManager.allConfigurations();
        for (const QNetworkConfiguration &cfg : cfgs) {
            if (cfg.state().testFlag(QNetworkConfiguration::Active)) {
                reallyOnline = true;
            }
        }
    } else if (state == QNetworkSession::Connected || state == QNetworkSession::Roaming) {
        reallyOnline = true;
    }
    online = reallyOnline;

    if (!reallyOnline) {
        if (state != QNetworkSession::Connected && state != QNetworkSession::Roaming) {
            if (networkAccessible != QNetworkAccessManager::NotAccessible) {
                networkAccessible = QNetworkAccessManager::NotAccessible;
                emit q->networkAccessibleChanged(networkAccessible);
            }
        }
    } else {
        if (defaultAccessControl)
            if (networkAccessible != QNetworkAccessManager::Accessible) {
                networkAccessible = QNetworkAccessManager::Accessible;
                emit q->networkAccessibleChanged(networkAccessible);
            }
    }
    if (online && (state != QNetworkSession::Connected && state != QNetworkSession::Roaming)) {
        _q_networkSessionClosed();
        createSession(q->configuration());
    }
}

void QNetworkAccessManagerPrivate::_q_onlineStateChanged(bool isOnline)
{
   Q_Q(QNetworkAccessManager);

   // if the user set a config, we only care whether this one is active.
    // Otherwise, this QNAM is online if there is an online config.
    if (customNetworkConfiguration) {
        online = (networkConfiguration.state() & QNetworkConfiguration::Active);
    } else {
        if (online != isOnline) {
            online = isOnline;
            _q_networkSessionClosed();
            createSession(q->configuration());
        }
    }
    if (online) {
        if (defaultAccessControl) {
            if (networkAccessible != QNetworkAccessManager::Accessible) {
                networkAccessible = QNetworkAccessManager::Accessible;
                emit q->networkAccessibleChanged(networkAccessible);
            }
        }
    } else {
        if (networkAccessible != QNetworkAccessManager::NotAccessible) {
            networkAccessible = QNetworkAccessManager::NotAccessible;
            emit q->networkAccessibleChanged(networkAccessible);
        }
    }
}

void QNetworkAccessManagerPrivate::_q_configurationChanged(const QNetworkConfiguration &configuration)
{
    const QString id = configuration.identifier();
    if (configuration.state().testFlag(QNetworkConfiguration::Active)) {
        if (!onlineConfigurations.contains(id)) {
            QSharedPointer<QNetworkSession> session(getNetworkSession());
            if (session) {
                if (online && session->configuration().identifier()
                        != networkConfigurationManager.defaultConfiguration().identifier()) {

                    onlineConfigurations.insert(id);
                    // CHECK: If it's having Active flag - why would it be disconnected ???
                    //this one disconnected but another one is online,
                    // close and create new session
                    _q_networkSessionClosed();
                    createSession(networkConfigurationManager.defaultConfiguration());
                }
            }
        }

    } else if (onlineConfigurations.contains(id)) {
        //this one is disconnecting
        // CHECK: If it disconnected while we create a session over a down configuration ???
        onlineConfigurations.remove(id);
        if (!onlineConfigurations.isEmpty()) {
            _q_networkSessionClosed();
            createSession(configuration);
        }
    }
}


void QNetworkAccessManagerPrivate::_q_networkSessionFailed(QNetworkSession::SessionError)
{
    const auto cfgs = networkConfigurationManager.allConfigurations();
    for (const QNetworkConfiguration &cfg : cfgs) {
        if (cfg.state().testFlag(QNetworkConfiguration::Active)) {
            online = true;
            _q_networkSessionClosed();
            createSession(networkConfigurationManager.defaultConfiguration());
            return;
        }
    }
}

#endif // QT_NO_BEARERMANAGEMENT

#if QT_CONFIG(http)
QNetworkRequest QNetworkAccessManagerPrivate::prepareMultipart(const QNetworkRequest &request, QHttpMultiPart *multiPart)
{
    // copy the request, we probably need to add some headers
    QNetworkRequest newRequest(request);

    // add Content-Type header if not there already
    if (!request.header(QNetworkRequest::ContentTypeHeader).isValid()) {
        QByteArray contentType;
        contentType.reserve(34 + multiPart->d_func()->boundary.count());
        contentType += "multipart/";
        switch (multiPart->d_func()->contentType) {
        case QHttpMultiPart::RelatedType:
            contentType += "related";
            break;
        case QHttpMultiPart::FormDataType:
            contentType += "form-data";
            break;
        case QHttpMultiPart::AlternativeType:
            contentType += "alternative";
            break;
        default:
            contentType += "mixed";
            break;
        }
        // putting the boundary into quotes, recommended in RFC 2046 section 5.1.1
        contentType += "; boundary=\"" + multiPart->d_func()->boundary + '"';
        newRequest.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(contentType));
    }

    // add MIME-Version header if not there already (we must include the header
    // if the message conforms to RFC 2045, see section 4 of that RFC)
    QByteArray mimeHeader("MIME-Version");
    if (!request.hasRawHeader(mimeHeader))
        newRequest.setRawHeader(mimeHeader, QByteArray("1.0"));

    QIODevice *device = multiPart->d_func()->device;
    if (!device->isReadable()) {
        if (!device->isOpen()) {
            if (!device->open(QIODevice::ReadOnly))
                qWarning("could not open device for reading");
        } else {
            qWarning("device is not readable");
        }
    }

    return newRequest;
}
#endif // QT_CONFIG(http)

QT_END_NAMESPACE

#include "moc_qnetworkaccessmanager.cpp"
