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

#include "qnetworkrequest.h"
#include "qnetworkrequest_p.h"
#include "qplatformdefs.h"
#include "qnetworkcookie.h"
#include "qsslconfiguration.h"
#include "QtCore/qshareddata.h"
#include "QtCore/qlocale.h"
#include "QtCore/qdatetime.h"

#include <ctype.h>
#if QT_CONFIG(datestring)
# include <stdio.h>
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \class QNetworkRequest
    \since 4.4
    \ingroup network
    \ingroup shared
    \inmodule QtNetwork

    \brief The QNetworkRequest class holds a request to be sent with QNetworkAccessManager.

    QNetworkRequest is part of the Network Access API and is the class
    holding the information necessary to send a request over the
    network. It contains a URL and some ancillary information that can
    be used to modify the request.

    \sa QNetworkReply, QNetworkAccessManager
*/

/*!
    \enum QNetworkRequest::KnownHeaders

    List of known header types that QNetworkRequest parses. Each known
    header is also represented in raw form with its full HTTP name.

    \value ContentDispositionHeader  Corresponds to the HTTP
    Content-Disposition header and contains a string containing the
    disposition type (for instance, attachment) and a parameter (for
    instance, filename).

    \value ContentTypeHeader    Corresponds to the HTTP Content-Type
    header and contains a string containing the media (MIME) type and
    any auxiliary data (for instance, charset).

    \value ContentLengthHeader  Corresponds to the HTTP Content-Length
    header and contains the length in bytes of the data transmitted.

    \value LocationHeader       Corresponds to the HTTP Location
    header and contains a URL representing the actual location of the
    data, including the destination URL in case of redirections.

    \value LastModifiedHeader   Corresponds to the HTTP Last-Modified
    header and contains a QDateTime representing the last modification
    date of the contents.

    \value IfModifiedSinceHeader   Corresponds to the HTTP If-Modified-Since
    header and contains a QDateTime. It is usually added to a
    QNetworkRequest. The server shall send a 304 (Not Modified) response
    if the resource has not changed since this time.

    \value ETagHeader              Corresponds to the HTTP ETag
    header and contains a QString representing the last modification
    state of the contents.

    \value IfMatchHeader           Corresponds to the HTTP If-Match
    header and contains a QStringList. It is usually added to a
    QNetworkRequest. The server shall send a 412 (Precondition Failed)
    response if the resource does not match.

    \value IfNoneMatchHeader       Corresponds to the HTTP If-None-Match
    header and contains a QStringList. It is usually added to a
    QNetworkRequest. The server shall send a 304 (Not Modified) response
    if the resource does match.

    \value CookieHeader         Corresponds to the HTTP Cookie header
    and contains a QList<QNetworkCookie> representing the cookies to
    be sent back to the server.

    \value SetCookieHeader      Corresponds to the HTTP Set-Cookie
    header and contains a QList<QNetworkCookie> representing the
    cookies sent by the server to be stored locally.

    \value UserAgentHeader      The User-Agent header sent by HTTP clients.

    \value ServerHeader         The Server header received by HTTP clients.

    \sa header(), setHeader(), rawHeader(), setRawHeader()
*/

/*!
    \enum QNetworkRequest::Attribute
    \since 4.7

    Attribute codes for the QNetworkRequest and QNetworkReply.

    Attributes are extra meta-data that are used to control the
    behavior of the request and to pass further information from the
    reply back to the application. Attributes are also extensible,
    allowing custom implementations to pass custom values.

    The following table explains what the default attribute codes are,
    the QVariant types associated, the default value if said attribute
    is missing and whether it's used in requests or replies.

    \value HttpStatusCodeAttribute
        Replies only, type: QMetaType::Int (no default)
        Indicates the HTTP status code received from the HTTP server
        (like 200, 304, 404, 401, etc.). If the connection was not
        HTTP-based, this attribute will not be present.

    \value HttpReasonPhraseAttribute
        Replies only, type: QMetaType::QByteArray (no default)
        Indicates the HTTP reason phrase as received from the HTTP
        server (like "Ok", "Found", "Not Found", "Access Denied",
        etc.) This is the human-readable representation of the status
        code (see above). If the connection was not HTTP-based, this
        attribute will not be present.

    \value RedirectionTargetAttribute
        Replies only, type: QMetaType::QUrl (no default)
        If present, it indicates that the server is redirecting the
        request to a different URL. The Network Access API does not by
        default follow redirections: the application can
        determine if the requested redirection should be allowed,
        according to its security policies, or it can set
        QNetworkRequest::FollowRedirectsAttribute to true (in which case
        the redirection will be followed and this attribute will not
        be present in the reply).
        The returned URL might be relative. Use QUrl::resolved()
        to create an absolute URL out of it.

    \value ConnectionEncryptedAttribute
        Replies only, type: QMetaType::Bool (default: false)
        Indicates whether the data was obtained through an encrypted
        (secure) connection.

    \value CacheLoadControlAttribute
        Requests only, type: QMetaType::Int (default: QNetworkRequest::PreferNetwork)
        Controls how the cache should be accessed. The possible values
        are those of QNetworkRequest::CacheLoadControl. Note that the
        default QNetworkAccessManager implementation does not support
        caching. However, this attribute may be used by certain
        backends to modify their requests (for example, for caching proxies).

    \value CacheSaveControlAttribute
        Requests only, type: QMetaType::Bool (default: true)
        Controls if the data obtained should be saved to cache for
        future uses. If the value is false, the data obtained will not
        be automatically cached. If true, data may be cached, provided
        it is cacheable (what is cacheable depends on the protocol
        being used).

    \value SourceIsFromCacheAttribute
        Replies only, type: QMetaType::Bool (default: false)
        Indicates whether the data was obtained from cache
        or not.

    \value DoNotBufferUploadDataAttribute
        Requests only, type: QMetaType::Bool (default: false)
        Indicates whether the QNetworkAccessManager code is
        allowed to buffer the upload data, e.g. when doing a HTTP POST.
        When using this flag with sequential upload data, the ContentLengthHeader
        header must be set.

    \value HttpPipeliningAllowedAttribute
        Requests only, type: QMetaType::Bool (default: false)
        Indicates whether the QNetworkAccessManager code is
        allowed to use HTTP pipelining with this request.

    \value HttpPipeliningWasUsedAttribute
        Replies only, type: QMetaType::Bool
        Indicates whether the HTTP pipelining was used for receiving
        this reply.

    \value CustomVerbAttribute
       Requests only, type: QMetaType::QByteArray
       Holds the value for the custom HTTP verb to send (destined for usage
       of other verbs than GET, POST, PUT and DELETE). This verb is set
       when calling QNetworkAccessManager::sendCustomRequest().

    \value CookieLoadControlAttribute
        Requests only, type: QMetaType::Int (default: QNetworkRequest::Automatic)
        Indicates whether to send 'Cookie' headers in the request.
        This attribute is set to false by Qt WebKit when creating a cross-origin
        XMLHttpRequest where withCredentials has not been set explicitly to true by the
        Javascript that created the request.
        See \l{http://www.w3.org/TR/XMLHttpRequest2/#credentials-flag}{here} for more information.
        (This value was introduced in 4.7.)

    \value CookieSaveControlAttribute
        Requests only, type: QMetaType::Int (default: QNetworkRequest::Automatic)
        Indicates whether to save 'Cookie' headers received from the server in reply
        to the request.
        This attribute is set to false by Qt WebKit when creating a cross-origin
        XMLHttpRequest where withCredentials has not been set explicitly to true by the
        Javascript that created the request.
        See \l{http://www.w3.org/TR/XMLHttpRequest2/#credentials-flag} {here} for more information.
        (This value was introduced in 4.7.)

    \value AuthenticationReuseAttribute
        Requests only, type: QMetaType::Int (default: QNetworkRequest::Automatic)
        Indicates whether to use cached authorization credentials in the request,
        if available. If this is set to QNetworkRequest::Manual and the authentication
        mechanism is 'Basic' or 'Digest', Qt will not send an an 'Authorization' HTTP
        header with any cached credentials it may have for the request's URL.
        This attribute is set to QNetworkRequest::Manual by Qt WebKit when creating a cross-origin
        XMLHttpRequest where withCredentials has not been set explicitly to true by the
        Javascript that created the request.
        See \l{http://www.w3.org/TR/XMLHttpRequest2/#credentials-flag} {here} for more information.
        (This value was introduced in 4.7.)

    \omitvalue MaximumDownloadBufferSizeAttribute

    \omitvalue DownloadBufferAttribute

    \omitvalue SynchronousRequestAttribute

    \value BackgroundRequestAttribute
        Type: QMetaType::Bool (default: false)
        Indicates that this is a background transfer, rather than a user initiated
        transfer. Depending on the platform, background transfers may be subject
        to different policies.
        The QNetworkSession ConnectInBackground property will be set according to
        this attribute.

    \value SpdyAllowedAttribute
        Requests only, type: QMetaType::Bool (default: false)
        Indicates whether the QNetworkAccessManager code is
        allowed to use SPDY with this request. This applies only
        to SSL requests, and depends on the server supporting SPDY.

    \value SpdyWasUsedAttribute
        Replies only, type: QMetaType::Bool
        Indicates whether SPDY was used for receiving
        this reply.

    \value HTTP2AllowedAttribute
        Requests only, type: QMetaType::Bool (default: false)
        Indicates whether the QNetworkAccessManager code is
        allowed to use HTTP/2 with this request. This applies
        to SSL requests or 'cleartext' HTTP/2.

    \value HTTP2WasUsedAttribute
        Replies only, type: QMetaType::Bool (default: false)
        Indicates whether HTTP/2 was used for receiving this reply.
        (This value was introduced in 5.9.)

    \value EmitAllUploadProgressSignalsAttribute
        Requests only, type: QMetaType::Bool (default: false)
        Indicates whether all upload signals should be emitted.
        By default, the uploadProgress signal is emitted only
        in 100 millisecond intervals.
        (This value was introduced in 5.5.)

    \value FollowRedirectsAttribute
        Requests only, type: QMetaType::Bool (default: false)
        Indicates whether the Network Access API should automatically follow a
        HTTP redirect response or not. Currently redirects that are insecure,
        that is redirecting from "https" to "http" protocol, are not allowed.
        (This value was introduced in 5.6.)

    \value OriginalContentLengthAttribute
        Replies only, type QMetaType::Int
        Holds the original content-length attribute before being invalidated and
        removed from the header when the data is compressed and the request was
        marked to be decompressed automatically.
        (This value was introduced in 5.9.)

    \value RedirectPolicyAttribute
        Requests only, type: QMetaType::Int, should be one of the
        QNetworkRequest::RedirectPolicy values (default: ManualRedirectPolicy).
        This attribute obsoletes FollowRedirectsAttribute.
        (This value was introduced in 5.9.)

    \value Http2DirectAttribute
        Requests only, type: QMetaType::Bool (default: false)
        If set, this attribute will force QNetworkAccessManager to use
        HTTP/2 protocol without initial HTTP/2 protocol negotiation.
        Use of this attribute implies prior knowledge that a particular
        server supports HTTP/2. The attribute works with SSL or 'cleartext'
        HTTP/2. If a server turns out to not support HTTP/2, when HTTP/2 direct
        was specified, QNetworkAccessManager gives up, without attempting to
        fall back to HTTP/1.1. If both HTTP2AllowedAttribute and
        Http2DirectAttribute are set, Http2DirectAttribute takes priority.
        (This value was introduced in 5.11.)

    \omitvalue ResourceTypeAttribute

    \value User
        Special type. Additional information can be passed in
        QVariants with types ranging from User to UserMax. The default
        implementation of Network Access will ignore any request
        attributes in this range and it will not produce any
        attributes in this range in replies. The range is reserved for
        extensions of QNetworkAccessManager.

    \value UserMax
        Special type. See User.
*/

/*!
    \enum QNetworkRequest::CacheLoadControl

    Controls the caching mechanism of QNetworkAccessManager.

    \value AlwaysNetwork        always load from network and do not
    check if the cache has a valid entry (similar to the
    "Reload" feature in browsers); in addition, force intermediate
    caches to re-validate.

    \value PreferNetwork        default value; load from the network
    if the cached entry is older than the network entry. This will never
    return stale data from the cache, but revalidate resources that
    have become stale.

    \value PreferCache          load from cache if available,
    otherwise load from network. Note that this can return possibly
    stale (but not expired) items from cache.

    \value AlwaysCache          only load from cache, indicating error
    if the item was not cached (i.e., off-line mode)
*/

/*!
    \enum QNetworkRequest::LoadControl
    \since 4.7

    Indicates if an aspect of the request's loading mechanism has been
    manually overridden, e.g. by Qt WebKit.

    \value Automatic            default value: indicates default behaviour.

    \value Manual               indicates behaviour has been manually overridden.
*/

/*!
    \enum QNetworkRequest::RedirectPolicy
    \since 5.9

    Indicates whether the Network Access API should automatically follow a
    HTTP redirect response or not.

    \value ManualRedirectPolicy        Default value: not following any redirects.

    \value NoLessSafeRedirectPolicy    Only "http"->"http", "http" -> "https"
                                       or "https" -> "https" redirects are allowed.
                                       Equivalent to setting the old FollowRedirectsAttribute
                                       to true

    \value SameOriginRedirectPolicy    Require the same protocol, host and port.
                                       Note, http://example.com and http://example.com:80
                                       will fail with this policy (implicit/explicit ports
                                       are considered to be a mismatch).

    \value UserVerifiedRedirectPolicy  Client decides whether to follow each
                                       redirect by handling the redirected()
                                       signal, emitting redirectAllowed() on
                                       the QNetworkReply object to allow
                                       the redirect or aborting/finishing it to
                                       reject the redirect.  This can be used,
                                       for example, to ask the user whether to
                                       accept the redirect, or to decide
                                       based on some app-specific configuration.
*/

class QNetworkRequestPrivate: public QSharedData, public QNetworkHeadersPrivate
{
public:
    static const int maxRedirectCount = 50;
    inline QNetworkRequestPrivate()
        : priority(QNetworkRequest::NormalPriority)
#ifndef QT_NO_SSL
        , sslConfiguration(0)
#endif
        , maxRedirectsAllowed(maxRedirectCount)
    { qRegisterMetaType<QNetworkRequest>(); }
    ~QNetworkRequestPrivate()
    {
#ifndef QT_NO_SSL
        delete sslConfiguration;
#endif
    }


    QNetworkRequestPrivate(const QNetworkRequestPrivate &other)
        : QSharedData(other), QNetworkHeadersPrivate(other)
    {
        url = other.url;
        priority = other.priority;
        maxRedirectsAllowed = other.maxRedirectsAllowed;
#ifndef QT_NO_SSL
        sslConfiguration = 0;
        if (other.sslConfiguration)
            sslConfiguration = new QSslConfiguration(*other.sslConfiguration);
#endif
    }

    inline bool operator==(const QNetworkRequestPrivate &other) const
    {
        return url == other.url &&
            priority == other.priority &&
            rawHeaders == other.rawHeaders &&
            attributes == other.attributes &&
            maxRedirectsAllowed == other.maxRedirectsAllowed;
        // don't compare cookedHeaders
    }

    QUrl url;
    QNetworkRequest::Priority priority;
#ifndef QT_NO_SSL
    mutable QSslConfiguration *sslConfiguration;
#endif
    int maxRedirectsAllowed;
};

/*!
    Constructs a QNetworkRequest object with \a url as the URL to be
    requested.

    \sa url(), setUrl()
*/
QNetworkRequest::QNetworkRequest(const QUrl &url)
    : d(new QNetworkRequestPrivate)
{
    d->url = url;
}

/*!
    Creates a copy of \a other.
*/
QNetworkRequest::QNetworkRequest(const QNetworkRequest &other)
    : d(other.d)
{
}

/*!
    Disposes of the QNetworkRequest object.
*/
QNetworkRequest::~QNetworkRequest()
{
    // QSharedDataPointer auto deletes
    d = 0;
}

/*!
    Returns \c true if this object is the same as \a other (i.e., if they
    have the same URL, same headers and same meta-data settings).

    \sa operator!=()
*/
bool QNetworkRequest::operator==(const QNetworkRequest &other) const
{
    return d == other.d || *d == *other.d;
}

/*!
    \fn bool QNetworkRequest::operator!=(const QNetworkRequest &other) const

    Returns \c false if this object is not the same as \a other.

    \sa operator==()
*/

/*!
    Creates a copy of \a other
*/
QNetworkRequest &QNetworkRequest::operator=(const QNetworkRequest &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QNetworkRequest::swap(QNetworkRequest &other)
    \since 5.0

    Swaps this network request with \a other. This function is very
    fast and never fails.
*/

/*!
    Returns the URL this network request is referring to.

    \sa setUrl()
*/
QUrl QNetworkRequest::url() const
{
    return d->url;
}

/*!
    Sets the URL this network request is referring to be \a url.

    \sa url()
*/
void QNetworkRequest::setUrl(const QUrl &url)
{
    d->url = url;
}

/*!
    Returns the value of the known network header \a header if it is
    present in this request. If it is not present, returns QVariant()
    (i.e., an invalid variant).

    \sa KnownHeaders, rawHeader(), setHeader()
*/
QVariant QNetworkRequest::header(KnownHeaders header) const
{
    return d->cookedHeaders.value(header);
}

/*!
    Sets the value of the known header \a header to be \a value,
    overriding any previously set headers. This operation also sets
    the equivalent raw HTTP header.

    \sa KnownHeaders, setRawHeader(), header()
*/
void QNetworkRequest::setHeader(KnownHeaders header, const QVariant &value)
{
    d->setCookedHeader(header, value);
}

/*!
    Returns \c true if the raw header \a headerName is present in this
    network request.

    \sa rawHeader(), setRawHeader()
*/
bool QNetworkRequest::hasRawHeader(const QByteArray &headerName) const
{
    return d->findRawHeader(headerName) != d->rawHeaders.constEnd();
}

/*!
    Returns the raw form of header \a headerName. If no such header is
    present, an empty QByteArray is returned, which may be
    indistinguishable from a header that is present but has no content
    (use hasRawHeader() to find out if the header exists or not).

    Raw headers can be set with setRawHeader() or with setHeader().

    \sa header(), setRawHeader()
*/
QByteArray QNetworkRequest::rawHeader(const QByteArray &headerName) const
{
    QNetworkHeadersPrivate::RawHeadersList::ConstIterator it =
        d->findRawHeader(headerName);
    if (it != d->rawHeaders.constEnd())
        return it->second;
    return QByteArray();
}

/*!
    Returns a list of all raw headers that are set in this network
    request. The list is in the order that the headers were set.

    \sa hasRawHeader(), rawHeader()
*/
QList<QByteArray> QNetworkRequest::rawHeaderList() const
{
    return d->rawHeadersKeys();
}

/*!
    Sets the header \a headerName to be of value \a headerValue. If \a
    headerName corresponds to a known header (see
    QNetworkRequest::KnownHeaders), the raw format will be parsed and
    the corresponding "cooked" header will be set as well.

    For example:
    \snippet code/src_network_access_qnetworkrequest.cpp 0

    will also set the known header LastModifiedHeader to be the
    QDateTime object of the parsed date.

    \note Setting the same header twice overrides the previous
    setting. To accomplish the behaviour of multiple HTTP headers of
    the same name, you should concatenate the two values, separating
    them with a comma (",") and set one single raw header.

    \sa KnownHeaders, setHeader(), hasRawHeader(), rawHeader()
*/
void QNetworkRequest::setRawHeader(const QByteArray &headerName, const QByteArray &headerValue)
{
    d->setRawHeader(headerName, headerValue);
}

/*!
    Returns the attribute associated with the code \a code. If the
    attribute has not been set, it returns \a defaultValue.

    \note This function does not apply the defaults listed in
    QNetworkRequest::Attribute.

    \sa setAttribute(), QNetworkRequest::Attribute
*/
QVariant QNetworkRequest::attribute(Attribute code, const QVariant &defaultValue) const
{
    return d->attributes.value(code, defaultValue);
}

/*!
    Sets the attribute associated with code \a code to be value \a
    value. If the attribute is already set, the previous value is
    discarded. In special, if \a value is an invalid QVariant, the
    attribute is unset.

    \sa attribute(), QNetworkRequest::Attribute
*/
void QNetworkRequest::setAttribute(Attribute code, const QVariant &value)
{
    if (value.isValid())
        d->attributes.insert(code, value);
    else
        d->attributes.remove(code);
}

#ifndef QT_NO_SSL
/*!
    Returns this network request's SSL configuration. By default this is the same
    as QSslConfiguration::defaultConfiguration().

    \sa setSslConfiguration(), QSslConfiguration::defaultConfiguration()
*/
QSslConfiguration QNetworkRequest::sslConfiguration() const
{
    if (!d->sslConfiguration)
        d->sslConfiguration = new QSslConfiguration(QSslConfiguration::defaultConfiguration());
    return *d->sslConfiguration;
}

/*!
    Sets this network request's SSL configuration to be \a config. The
    settings that apply are the private key, the local certificate,
    the SSL protocol (SSLv2, SSLv3, TLSv1.0 where applicable), the CA
    certificates and the ciphers that the SSL backend is allowed to
    use.

    \sa sslConfiguration(), QSslConfiguration::defaultConfiguration()
*/
void QNetworkRequest::setSslConfiguration(const QSslConfiguration &config)
{
    if (!d->sslConfiguration)
        d->sslConfiguration = new QSslConfiguration(config);
    else
        *d->sslConfiguration = config;
}
#endif

/*!
    \since 4.6

    Allows setting a reference to the \a object initiating
    the request.

    For example Qt WebKit sets the originating object to the
    QWebFrame that initiated the request.

    \sa originatingObject()
*/
void QNetworkRequest::setOriginatingObject(QObject *object)
{
    d->originatingObject = object;
}

/*!
    \since 4.6

    Returns a reference to the object that initiated this
    network request; returns 0 if not set or the object has
    been destroyed.

    \sa setOriginatingObject()
*/
QObject *QNetworkRequest::originatingObject() const
{
    return d->originatingObject.data();
}

/*!
    \since 4.7

    Return the priority of this request.

    \sa setPriority()
*/
QNetworkRequest::Priority QNetworkRequest::priority() const
{
    return d->priority;
}

/*! \enum QNetworkRequest::Priority

  \since 4.7

  This enum lists the possible network request priorities.

  \value HighPriority   High priority
  \value NormalPriority Normal priority
  \value LowPriority    Low priority
 */

/*!
    \since 4.7

    Set the priority of this request to \a priority.

    \note The \a priority is only a hint to the network access
    manager.  It can use it or not. Currently it is used for HTTP to
    decide which request should be sent first to a server.

    \sa priority()
*/
void QNetworkRequest::setPriority(Priority priority)
{
    d->priority = priority;
}

/*!
    \since 5.6

    Returns the maximum number of redirects allowed to be followed for this
    request.

    \sa setMaximumRedirectsAllowed()
*/
int QNetworkRequest::maximumRedirectsAllowed() const
{
    return d->maxRedirectsAllowed;
}

/*!
    \since 5.6

    Sets the maximum number of redirects allowed to be followed for this
    request to \a maxRedirectsAllowed.

    \sa maximumRedirectsAllowed()
*/
void QNetworkRequest::setMaximumRedirectsAllowed(int maxRedirectsAllowed)
{
    d->maxRedirectsAllowed = maxRedirectsAllowed;
}

static QByteArray headerName(QNetworkRequest::KnownHeaders header)
{
    switch (header) {
    case QNetworkRequest::ContentTypeHeader:
        return "Content-Type";

    case QNetworkRequest::ContentLengthHeader:
        return "Content-Length";

    case QNetworkRequest::LocationHeader:
        return "Location";

    case QNetworkRequest::LastModifiedHeader:
        return "Last-Modified";

    case QNetworkRequest::IfModifiedSinceHeader:
        return "If-Modified-Since";

    case QNetworkRequest::ETagHeader:
        return "ETag";

    case QNetworkRequest::IfMatchHeader:
        return "If-Match";

    case QNetworkRequest::IfNoneMatchHeader:
        return "If-None-Match";

    case QNetworkRequest::CookieHeader:
        return "Cookie";

    case QNetworkRequest::SetCookieHeader:
        return "Set-Cookie";

    case QNetworkRequest::ContentDispositionHeader:
        return "Content-Disposition";

    case QNetworkRequest::UserAgentHeader:
        return "User-Agent";

    case QNetworkRequest::ServerHeader:
        return "Server";

    // no default:
    // if new values are added, this will generate a compiler warning
    }

    return QByteArray();
}

static QByteArray headerValue(QNetworkRequest::KnownHeaders header, const QVariant &value)
{
    switch (header) {
    case QNetworkRequest::ContentTypeHeader:
    case QNetworkRequest::ContentLengthHeader:
    case QNetworkRequest::ContentDispositionHeader:
    case QNetworkRequest::UserAgentHeader:
    case QNetworkRequest::ServerHeader:
    case QNetworkRequest::ETagHeader:
    case QNetworkRequest::IfMatchHeader:
    case QNetworkRequest::IfNoneMatchHeader:
        return value.toByteArray();

    case QNetworkRequest::LocationHeader:
        switch (value.userType()) {
        case QMetaType::QUrl:
            return value.toUrl().toEncoded();

        default:
            return value.toByteArray();
        }

    case QNetworkRequest::LastModifiedHeader:
    case QNetworkRequest::IfModifiedSinceHeader:
        switch (value.userType()) {
        case QMetaType::QDate:
        case QMetaType::QDateTime:
            // generate RFC 1123/822 dates:
            return QNetworkHeadersPrivate::toHttpDate(value.toDateTime());

        default:
            return value.toByteArray();
        }

    case QNetworkRequest::CookieHeader: {
        QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie> >(value);
        if (cookies.isEmpty() && value.userType() == qMetaTypeId<QNetworkCookie>())
            cookies << qvariant_cast<QNetworkCookie>(value);

        QByteArray result;
        bool first = true;
        for (const QNetworkCookie &cookie : qAsConst(cookies)) {
            if (!first)
                result += "; ";
            first = false;
            result += cookie.toRawForm(QNetworkCookie::NameAndValueOnly);
        }
        return result;
    }

    case QNetworkRequest::SetCookieHeader: {
        QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie> >(value);
        if (cookies.isEmpty() && value.userType() == qMetaTypeId<QNetworkCookie>())
            cookies << qvariant_cast<QNetworkCookie>(value);

        QByteArray result;
        bool first = true;
        for (const QNetworkCookie &cookie : qAsConst(cookies)) {
            if (!first)
                result += ", ";
            first = false;
            result += cookie.toRawForm(QNetworkCookie::Full);
        }
        return result;
    }
    }

    return QByteArray();
}

static int parseHeaderName(const QByteArray &headerName)
{
    if (headerName.isEmpty())
        return -1;

    switch (tolower(headerName.at(0))) {
    case 'c':
        if (headerName.compare("content-type", Qt::CaseInsensitive) == 0)
            return QNetworkRequest::ContentTypeHeader;
        else if (headerName.compare("content-length", Qt::CaseInsensitive) == 0)
            return QNetworkRequest::ContentLengthHeader;
        else if (headerName.compare("cookie", Qt::CaseInsensitive) == 0)
            return QNetworkRequest::CookieHeader;
        else if (qstricmp(headerName.constData(), "content-disposition") == 0)
            return QNetworkRequest::ContentDispositionHeader;
        break;

    case 'e':
        if (qstricmp(headerName.constData(), "etag") == 0)
            return QNetworkRequest::ETagHeader;
        break;

    case 'i':
        if (qstricmp(headerName.constData(), "if-modified-since") == 0)
            return QNetworkRequest::IfModifiedSinceHeader;
        if (qstricmp(headerName.constData(), "if-match") == 0)
            return QNetworkRequest::IfMatchHeader;
        if (qstricmp(headerName.constData(), "if-none-match") == 0)
            return QNetworkRequest::IfNoneMatchHeader;
        break;

    case 'l':
        if (headerName.compare("location", Qt::CaseInsensitive) == 0)
            return QNetworkRequest::LocationHeader;
        else if (headerName.compare("last-modified", Qt::CaseInsensitive) == 0)
            return QNetworkRequest::LastModifiedHeader;
        break;

    case 's':
        if (headerName.compare("set-cookie", Qt::CaseInsensitive) == 0)
            return QNetworkRequest::SetCookieHeader;
        else if (headerName.compare("server", Qt::CaseInsensitive) == 0)
            return QNetworkRequest::ServerHeader;
        break;

    case 'u':
        if (headerName.compare("user-agent", Qt::CaseInsensitive) == 0)
            return QNetworkRequest::UserAgentHeader;
        break;
    }

    return -1; // nothing found
}

static QVariant parseHttpDate(const QByteArray &raw)
{
    QDateTime dt = QNetworkHeadersPrivate::fromHttpDate(raw);
    if (dt.isValid())
        return dt;
    return QVariant();          // transform an invalid QDateTime into a null QVariant
}

static QVariant parseCookieHeader(const QByteArray &raw)
{
    QList<QNetworkCookie> result;
    const QList<QByteArray> cookieList = raw.split(';');
    for (const QByteArray &cookie : cookieList) {
        QList<QNetworkCookie> parsed = QNetworkCookie::parseCookies(cookie.trimmed());
        if (parsed.count() != 1)
            return QVariant();  // invalid Cookie: header

        result += parsed;
    }

    return QVariant::fromValue(result);
}

static QVariant parseETag(const QByteArray &raw)
{
    const QByteArray trimmed = raw.trimmed();
    if (!trimmed.startsWith('"') && !trimmed.startsWith(R"(W/")"))
        return QVariant();

    if (!trimmed.endsWith('"'))
        return QVariant();

    return QString::fromLatin1(trimmed);
}

static QVariant parseIfMatch(const QByteArray &raw)
{
    const QByteArray trimmedRaw = raw.trimmed();
    if (trimmedRaw == "*")
        return QStringList(QStringLiteral("*"));

    QStringList tags;
    const QList<QByteArray> split = trimmedRaw.split(',');
    for (const QByteArray &element : split) {
        const QByteArray trimmed = element.trimmed();
        if (!trimmed.startsWith('"'))
            continue;

        if (!trimmed.endsWith('"'))
            continue;

        tags += QString::fromLatin1(trimmed);
    }
    return tags;
}

static QVariant parseIfNoneMatch(const QByteArray &raw)
{
    const QByteArray trimmedRaw = raw.trimmed();
    if (trimmedRaw == "*")
        return QStringList(QStringLiteral("*"));

    QStringList tags;
    const QList<QByteArray> split = trimmedRaw.split(',');
    for (const QByteArray &element : split) {
        const QByteArray trimmed = element.trimmed();
        if (!trimmed.startsWith('"') && !trimmed.startsWith(R"(W/")"))
            continue;

        if (!trimmed.endsWith('"'))
            continue;

        tags += QString::fromLatin1(trimmed);
    }
    return tags;
}


static QVariant parseHeaderValue(QNetworkRequest::KnownHeaders header, const QByteArray &value)
{
    // header is always a valid value
    switch (header) {
    case QNetworkRequest::UserAgentHeader:
    case QNetworkRequest::ServerHeader:
    case QNetworkRequest::ContentTypeHeader:
    case QNetworkRequest::ContentDispositionHeader:
        // copy exactly, convert to QString
        return QString::fromLatin1(value);

    case QNetworkRequest::ContentLengthHeader: {
        bool ok;
        qint64 result = value.trimmed().toLongLong(&ok);
        if (ok)
            return result;
        return QVariant();
    }

    case QNetworkRequest::LocationHeader: {
        QUrl result = QUrl::fromEncoded(value, QUrl::StrictMode);
        if (result.isValid() && !result.scheme().isEmpty())
            return result;
        return QVariant();
    }

    case QNetworkRequest::LastModifiedHeader:
    case QNetworkRequest::IfModifiedSinceHeader:
        return parseHttpDate(value);

    case QNetworkRequest::ETagHeader:
        return parseETag(value);

    case QNetworkRequest::IfMatchHeader:
        return parseIfMatch(value);

    case QNetworkRequest::IfNoneMatchHeader:
        return parseIfNoneMatch(value);

    case QNetworkRequest::CookieHeader:
        return parseCookieHeader(value);

    case QNetworkRequest::SetCookieHeader:
        return QVariant::fromValue(QNetworkCookie::parseCookies(value));

    default:
        Q_ASSERT(0);
    }
    return QVariant();
}

QNetworkHeadersPrivate::RawHeadersList::ConstIterator
QNetworkHeadersPrivate::findRawHeader(const QByteArray &key) const
{
    RawHeadersList::ConstIterator it = rawHeaders.constBegin();
    RawHeadersList::ConstIterator end = rawHeaders.constEnd();
    for ( ; it != end; ++it)
        if (it->first.compare(key, Qt::CaseInsensitive) == 0)
            return it;

    return end;                 // not found
}

QNetworkHeadersPrivate::RawHeadersList QNetworkHeadersPrivate::allRawHeaders() const
{
    return rawHeaders;
}

QList<QByteArray> QNetworkHeadersPrivate::rawHeadersKeys() const
{
    QList<QByteArray> result;
    result.reserve(rawHeaders.size());
    RawHeadersList::ConstIterator it = rawHeaders.constBegin(),
                                 end = rawHeaders.constEnd();
    for ( ; it != end; ++it)
        result << it->first;

    return result;
}

void QNetworkHeadersPrivate::setRawHeader(const QByteArray &key, const QByteArray &value)
{
    if (key.isEmpty())
        // refuse to accept an empty raw header
        return;

    setRawHeaderInternal(key, value);
    parseAndSetHeader(key, value);
}

/*!
    \internal
    Sets the internal raw headers list to match \a list. The cooked headers
    will also be updated.

    If \a list contains duplicates, they will be stored, but only the first one
    is usually accessed.
*/
void QNetworkHeadersPrivate::setAllRawHeaders(const RawHeadersList &list)
{
    cookedHeaders.clear();
    rawHeaders = list;

    RawHeadersList::ConstIterator it = rawHeaders.constBegin();
    RawHeadersList::ConstIterator end = rawHeaders.constEnd();
    for ( ; it != end; ++it)
        parseAndSetHeader(it->first, it->second);
}

void QNetworkHeadersPrivate::setCookedHeader(QNetworkRequest::KnownHeaders header,
                                             const QVariant &value)
{
    QByteArray name = headerName(header);
    if (name.isEmpty()) {
        // headerName verifies that \a header is a known value
        qWarning("QNetworkRequest::setHeader: invalid header value KnownHeader(%d) received", header);
        return;
    }

    if (value.isNull()) {
        setRawHeaderInternal(name, QByteArray());
        cookedHeaders.remove(header);
    } else {
        QByteArray rawValue = headerValue(header, value);
        if (rawValue.isEmpty()) {
            qWarning("QNetworkRequest::setHeader: QVariant of type %s cannot be used with header %s",
                     value.typeName(), name.constData());
            return;
        }

        setRawHeaderInternal(name, rawValue);
        cookedHeaders.insert(header, value);
    }
}

void QNetworkHeadersPrivate::setRawHeaderInternal(const QByteArray &key, const QByteArray &value)
{
    auto firstEqualsKey = [&key](const RawHeaderPair &header) {
        return header.first.compare(key, Qt::CaseInsensitive) == 0;
    };
    rawHeaders.erase(std::remove_if(rawHeaders.begin(), rawHeaders.end(),
                                    firstEqualsKey),
                     rawHeaders.end());

    if (value.isNull())
        return;                 // only wanted to erase key

    RawHeaderPair pair;
    pair.first = key;
    pair.second = value;
    rawHeaders.append(pair);
}

void QNetworkHeadersPrivate::parseAndSetHeader(const QByteArray &key, const QByteArray &value)
{
    // is it a known header?
    const int parsedKeyAsInt = parseHeaderName(key);
    if (parsedKeyAsInt != -1) {
        const QNetworkRequest::KnownHeaders parsedKey
                = static_cast<QNetworkRequest::KnownHeaders>(parsedKeyAsInt);
        if (value.isNull()) {
            cookedHeaders.remove(parsedKey);
        } else if (parsedKey == QNetworkRequest::ContentLengthHeader
                 && cookedHeaders.contains(QNetworkRequest::ContentLengthHeader)) {
            // Only set the cooked header "Content-Length" once.
            // See bug QTBUG-15311
        } else {
            cookedHeaders.insert(parsedKey, parseHeaderValue(parsedKey, value));
        }

    }
}

// Fast month string to int conversion. This code
// assumes that the Month name is correct and that
// the string is at least three chars long.
static int name_to_month(const char* month_str)
{
    switch (month_str[0]) {
    case 'J':
        switch (month_str[1]) {
        case 'a':
            return 1;
        case 'u':
            switch (month_str[2] ) {
            case 'n':
                return 6;
            case 'l':
                return 7;
            }
        }
        break;
    case 'F':
        return 2;
    case 'M':
        switch (month_str[2] ) {
        case 'r':
            return 3;
        case 'y':
            return 5;
        }
        break;
    case 'A':
        switch (month_str[1]) {
        case 'p':
            return 4;
        case 'u':
            return 8;
        }
        break;
    case 'O':
        return 10;
    case 'S':
        return 9;
    case 'N':
        return 11;
    case 'D':
        return 12;
    }

    return 0;
}

QDateTime QNetworkHeadersPrivate::fromHttpDate(const QByteArray &value)
{
    // HTTP dates have three possible formats:
    //  RFC 1123/822      -   ddd, dd MMM yyyy hh:mm:ss "GMT"
    //  RFC 850           -   dddd, dd-MMM-yy hh:mm:ss "GMT"
    //  ANSI C's asctime  -   ddd MMM d hh:mm:ss yyyy
    // We only handle them exactly. If they deviate, we bail out.

    int pos = value.indexOf(',');
    QDateTime dt;
#if QT_CONFIG(datestring)
    if (pos == -1) {
        // no comma -> asctime(3) format
        dt = QDateTime::fromString(QString::fromLatin1(value), Qt::TextDate);
    } else {
        // Use sscanf over QLocal/QDateTimeParser for speed reasons. See the
        // Qt WebKit performance benchmarks to get an idea.
        if (pos == 3) {
            char month_name[4];
            int day, year, hour, minute, second;
#ifdef Q_CC_MSVC
            // Use secure version to avoid compiler warning
            if (sscanf_s(value.constData(), "%*3s, %d %3s %d %d:%d:%d 'GMT'", &day, month_name, 4, &year, &hour, &minute, &second) == 6)
#else
            // The POSIX secure mode is %ms (which allocates memory), too bleeding edge for now
            // In any case this is already safe as field width is specified.
            if (sscanf(value.constData(), "%*3s, %d %3s %d %d:%d:%d 'GMT'", &day, month_name, &year, &hour, &minute, &second) == 6)
#endif
                dt = QDateTime(QDate(year, name_to_month(month_name), day), QTime(hour, minute, second));
        } else {
            QLocale c = QLocale::c();
            // eat the weekday, the comma and the space following it
            QString sansWeekday = QString::fromLatin1(value.constData() + pos + 2);
            // must be RFC 850 date
            dt = c.toDateTime(sansWeekday, QLatin1String("dd-MMM-yy hh:mm:ss 'GMT'"));
        }
    }
#endif // datestring

    if (dt.isValid())
        dt.setTimeSpec(Qt::UTC);
    return dt;
}

QByteArray QNetworkHeadersPrivate::toHttpDate(const QDateTime &dt)
{
    return QLocale::c().toString(dt, QLatin1String("ddd, dd MMM yyyy hh:mm:ss 'GMT'"))
        .toLatin1();
}

QT_END_NAMESPACE
