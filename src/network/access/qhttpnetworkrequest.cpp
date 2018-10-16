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

#include "qhttpnetworkrequest_p.h"
#include "private/qnoncontiguousbytedevice_p.h"

QT_BEGIN_NAMESPACE

QHttpNetworkRequestPrivate::QHttpNetworkRequestPrivate(QHttpNetworkRequest::Operation op,
        QHttpNetworkRequest::Priority pri, const QUrl &newUrl)
    : QHttpNetworkHeaderPrivate(newUrl), operation(op), priority(pri), uploadByteDevice(0),
      autoDecompress(false), pipeliningAllowed(false), spdyAllowed(false), http2Allowed(false),
      http2Direct(false), withCredentials(true), preConnect(false), redirectCount(0),
      redirectPolicy(QNetworkRequest::ManualRedirectPolicy)
{
}

QHttpNetworkRequestPrivate::QHttpNetworkRequestPrivate(const QHttpNetworkRequestPrivate &other) // = default
    : QHttpNetworkHeaderPrivate(other),
      operation(other.operation),
      customVerb(other.customVerb),
      priority(other.priority),
      uploadByteDevice(other.uploadByteDevice),
      autoDecompress(other.autoDecompress),
      pipeliningAllowed(other.pipeliningAllowed),
      spdyAllowed(other.spdyAllowed),
      http2Allowed(other.http2Allowed),
      http2Direct(other.http2Direct),
      withCredentials(other.withCredentials),
      ssl(other.ssl),
      preConnect(other.preConnect),
      redirectCount(other.redirectCount),
      redirectPolicy(other.redirectPolicy)
{
}

QHttpNetworkRequestPrivate::~QHttpNetworkRequestPrivate()
{
}

bool QHttpNetworkRequestPrivate::operator==(const QHttpNetworkRequestPrivate &other) const
{
    return QHttpNetworkHeaderPrivate::operator==(other)
        && (operation == other.operation)
        && (priority == other.priority)
        && (uploadByteDevice == other.uploadByteDevice)
        && (autoDecompress == other.autoDecompress)
        && (pipeliningAllowed == other.pipeliningAllowed)
        && (spdyAllowed == other.spdyAllowed)
        && (http2Allowed == other.http2Allowed)
        && (http2Direct == other.http2Direct)
        // we do not clear the customVerb in setOperation
        && (operation != QHttpNetworkRequest::Custom || (customVerb == other.customVerb))
        && (withCredentials == other.withCredentials)
        && (ssl == other.ssl)
        && (preConnect == other.preConnect)
        && (redirectPolicy == other.redirectPolicy);
}

QByteArray QHttpNetworkRequest::methodName() const
{
    switch (d->operation) {
    case QHttpNetworkRequest::Get:
        return "GET";
    case QHttpNetworkRequest::Head:
        return "HEAD";
    case QHttpNetworkRequest::Post:
        return "POST";
    case QHttpNetworkRequest::Options:
        return "OPTIONS";
    case QHttpNetworkRequest::Put:
        return "PUT";
    case QHttpNetworkRequest::Delete:
        return "DELETE";
    case QHttpNetworkRequest::Trace:
        return "TRACE";
    case QHttpNetworkRequest::Connect:
        return "CONNECT";
    case QHttpNetworkRequest::Custom:
        return d->customVerb;
    default:
        break;
    }
    return QByteArray();
}

QByteArray QHttpNetworkRequest::uri(bool throughProxy) const
{
    QUrl::FormattingOptions format(QUrl::RemoveFragment | QUrl::RemoveUserInfo | QUrl::FullyEncoded);

    // for POST, query data is sent as content
    if (d->operation == QHttpNetworkRequest::Post && !d->uploadByteDevice)
        format |= QUrl::RemoveQuery;
    // for requests through proxy, the Request-URI contains full url
    if (!throughProxy)
        format |= QUrl::RemoveScheme | QUrl::RemoveAuthority;
    QUrl copy = d->url;
    if (copy.path().isEmpty())
        copy.setPath(QStringLiteral("/"));
    else
        format |= QUrl::NormalizePathSegments;
    QByteArray uri = copy.toEncoded(format);
    return uri;
}

QByteArray QHttpNetworkRequestPrivate::header(const QHttpNetworkRequest &request, bool throughProxy)
{
    QList<QPair<QByteArray, QByteArray> > fields = request.header();
    QByteArray ba;
    ba.reserve(40 + fields.length()*25); // very rough lower bound estimation

    ba += request.methodName();
    ba += ' ';
    ba += request.uri(throughProxy);

    ba += " HTTP/";
    ba += QByteArray::number(request.majorVersion());
    ba += '.';
    ba += QByteArray::number(request.minorVersion());
    ba += "\r\n";

    QList<QPair<QByteArray, QByteArray> >::const_iterator it = fields.constBegin();
    QList<QPair<QByteArray, QByteArray> >::const_iterator endIt = fields.constEnd();
    for (; it != endIt; ++it) {
        ba += it->first;
        ba += ": ";
        ba += it->second;
        ba += "\r\n";
    }
    if (request.d->operation == QHttpNetworkRequest::Post) {
        // add content type, if not set in the request
        if (request.headerField("content-type").isEmpty() && ((request.d->uploadByteDevice && request.d->uploadByteDevice->size() > 0) || request.d->url.hasQuery())) {
            //Content-Type is mandatory. We can't say anything about the encoding, but x-www-form-urlencoded is the most likely to work.
            //This warning indicates a bug in application code not setting a required header.
            //Note that if using QHttpMultipart, the content-type is set in QNetworkAccessManagerPrivate::prepareMultipart already
            qWarning("content-type missing in HTTP POST, defaulting to application/x-www-form-urlencoded. Use QNetworkRequest::setHeader() to fix this problem.");
            ba += "Content-Type: application/x-www-form-urlencoded\r\n";
        }
        if (!request.d->uploadByteDevice && request.d->url.hasQuery()) {
            QByteArray query = request.d->url.query(QUrl::FullyEncoded).toLatin1();
            ba += "Content-Length: ";
            ba += QByteArray::number(query.size());
            ba += "\r\n\r\n";
            ba += query;
        } else {
            ba += "\r\n";
        }
    } else {
        ba += "\r\n";
    }
     return ba;
}


// QHttpNetworkRequest

QHttpNetworkRequest::QHttpNetworkRequest(const QUrl &url, Operation operation, Priority priority)
    : d(new QHttpNetworkRequestPrivate(operation, priority, url))
{
}

QHttpNetworkRequest::QHttpNetworkRequest(const QHttpNetworkRequest &other)
    : QHttpNetworkHeader(other), d(other.d)
{
}

QHttpNetworkRequest::~QHttpNetworkRequest()
{
}

QUrl QHttpNetworkRequest::url() const
{
    return d->url;
}
void QHttpNetworkRequest::setUrl(const QUrl &url)
{
    d->url = url;
}

bool QHttpNetworkRequest::isSsl() const
{
    return d->ssl;
}
void QHttpNetworkRequest::setSsl(bool s)
{
    d->ssl = s;
}

bool QHttpNetworkRequest::isPreConnect() const
{
    return d->preConnect;
}
void QHttpNetworkRequest::setPreConnect(bool preConnect)
{
    d->preConnect = preConnect;
}

bool QHttpNetworkRequest::isFollowRedirects() const
{
    return d->redirectPolicy != QNetworkRequest::ManualRedirectPolicy;
}

void QHttpNetworkRequest::setRedirectPolicy(QNetworkRequest::RedirectPolicy policy)
{
    d->redirectPolicy = policy;
}

QNetworkRequest::RedirectPolicy QHttpNetworkRequest::redirectPolicy() const
{
    return d->redirectPolicy;
}

int QHttpNetworkRequest::redirectCount() const
{
    return d->redirectCount;
}

void QHttpNetworkRequest::setRedirectCount(int count)
{
    d->redirectCount = count;
}

qint64 QHttpNetworkRequest::contentLength() const
{
    return d->contentLength();
}

void QHttpNetworkRequest::setContentLength(qint64 length)
{
    d->setContentLength(length);
}

QList<QPair<QByteArray, QByteArray> > QHttpNetworkRequest::header() const
{
    return d->fields;
}

QByteArray QHttpNetworkRequest::headerField(const QByteArray &name, const QByteArray &defaultValue) const
{
    return d->headerField(name, defaultValue);
}

void QHttpNetworkRequest::setHeaderField(const QByteArray &name, const QByteArray &data)
{
    d->setHeaderField(name, data);
}

void QHttpNetworkRequest::prependHeaderField(const QByteArray &name, const QByteArray &data)
{
    d->prependHeaderField(name, data);
}

QHttpNetworkRequest &QHttpNetworkRequest::operator=(const QHttpNetworkRequest &other)
{
    d = other.d;
    return *this;
}

bool QHttpNetworkRequest::operator==(const QHttpNetworkRequest &other) const
{
    return d->operator==(*other.d);
}

QHttpNetworkRequest::Operation QHttpNetworkRequest::operation() const
{
    return d->operation;
}

void QHttpNetworkRequest::setOperation(Operation operation)
{
    d->operation = operation;
}

QByteArray QHttpNetworkRequest::customVerb() const
{
    return d->customVerb;
}

void QHttpNetworkRequest::setCustomVerb(const QByteArray &customVerb)
{
    d->customVerb = customVerb;
}

QHttpNetworkRequest::Priority QHttpNetworkRequest::priority() const
{
    return d->priority;
}

void QHttpNetworkRequest::setPriority(Priority priority)
{
    d->priority = priority;
}

bool QHttpNetworkRequest::isPipeliningAllowed() const
{
    return d->pipeliningAllowed;
}

void QHttpNetworkRequest::setPipeliningAllowed(bool b)
{
    d->pipeliningAllowed = b;
}

bool QHttpNetworkRequest::isSPDYAllowed() const
{
    return d->spdyAllowed;
}

void QHttpNetworkRequest::setSPDYAllowed(bool b)
{
    d->spdyAllowed = b;
}

bool QHttpNetworkRequest::isHTTP2Allowed() const
{
    return d->http2Allowed;
}

void QHttpNetworkRequest::setHTTP2Allowed(bool b)
{
    d->http2Allowed = b;
}

bool QHttpNetworkRequest::isHTTP2Direct() const
{
    return d->http2Direct;
}

void QHttpNetworkRequest::setHTTP2Direct(bool b)
{
    d->http2Direct = b;
}

bool QHttpNetworkRequest::withCredentials() const
{
    return d->withCredentials;
}

void QHttpNetworkRequest::setWithCredentials(bool b)
{
    d->withCredentials = b;
}

void QHttpNetworkRequest::setUploadByteDevice(QNonContiguousByteDevice *bd)
{
    d->uploadByteDevice = bd;
}

QNonContiguousByteDevice* QHttpNetworkRequest::uploadByteDevice() const
{
    return d->uploadByteDevice;
}

int QHttpNetworkRequest::majorVersion() const
{
    return 1;
}

int QHttpNetworkRequest::minorVersion() const
{
    return 1;
}


QT_END_NAMESPACE

