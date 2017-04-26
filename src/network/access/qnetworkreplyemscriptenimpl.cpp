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

#include "qnetworkreplyemscriptenimpl_p.h"

#include "QtCore/qdatetime.h"
#include "qnetworkaccessmanager_p.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include "qnetworkfile_p.h"
#include "qnetworkrequest.h"

QT_BEGIN_NAMESPACE

static QT_MANGLE_NAMESPACE(QNetworkReplyEmscriptenImplPrivate) *gHandler = 0;

QNetworkReplyEmscriptenImplPrivate::QNetworkReplyEmscriptenImplPrivate()
    : QNetworkReplyPrivate(),
      managerPrivate(0),
      downloadBufferReadPosition(0),
      downloadBufferCurrentSize(0),
      totalDownloadSize(0),
      percentFinished(0),
      downloadZerocopyBuffer(0)
{
    qDebug() << Q_FUNC_INFO << this;
    gHandler = this;
}

QNetworkReplyEmscriptenImpl::~QNetworkReplyEmscriptenImpl()
{
    qDebug() << Q_FUNC_INFO;
}

QNetworkReplyEmscriptenImpl::QNetworkReplyEmscriptenImpl(QNetworkAccessManager *manager, const QNetworkRequest &req, const QNetworkAccessManager::Operation op)
    : QNetworkReply(*new QNetworkReplyEmscriptenImplPrivate(), manager)
{
    qDebug() << Q_FUNC_INFO << this;

    setRequest(req);
    setUrl(req.url());
    setOperation(op);
    QNetworkReply::open(QIODevice::ReadOnly);

    QNetworkReplyEmscriptenImplPrivate *d = (QNetworkReplyEmscriptenImplPrivate*) d_func();

    d->managerPrivate = manager->d_func();

//    if (req.attribute(QNetworkRequest::BackgroundRequestAttribute).toBool()) { // Asynchronous open
//    } else { // Synch open
        d->doSendRequest(methodName(), req);
//    }
}

void QNetworkReplyEmscriptenImpl::connectionFinished()
{
    qDebug() << Q_FUNC_INFO;
}

QByteArray QNetworkReplyEmscriptenImpl::methodName() const
{
    switch (operation()) {
    case QNetworkAccessManager::HeadOperation:
        return "HEAD";
    case QNetworkAccessManager::GetOperation:
        return "GET";
    case QNetworkAccessManager::PutOperation:
        return "PUT";
    case QNetworkAccessManager::PostOperation:
        return "POST";
    case QNetworkAccessManager::DeleteOperation:
        return "DELETE";
//    case QNetworkAccessManager::CustomOperation:
//        return d->customVerb;
    default:
        break;
    }
    return QByteArray();
}

void QNetworkReplyEmscriptenImpl::close()
{
    qDebug() << Q_FUNC_INFO;
 //   Q_D(QNetworkReplyEmscriptenImpl);
    QNetworkReply::close();
//    if (d->realFile) {
//        if (d->realFile->thread() == thread())
//            d->realFile->close();
//        else
//            QMetaObject::invokeMethod(d->realFile, "close", Qt::QueuedConnection);
//    }
}

void QNetworkReplyEmscriptenImpl::abort()
{
    qDebug() << Q_FUNC_INFO;
    close();
}

qint64 QNetworkReplyEmscriptenImpl::bytesAvailable() const
{
    Q_D(const QNetworkReplyEmscriptenImpl);

    qDebug() << Q_FUNC_INFO
             << d->isFinished
             << QNetworkReply::bytesAvailable()
             << d->downloadBufferCurrentSize
             << d->downloadBufferReadPosition;

    if (!d->isFinished)
        return QNetworkReply::bytesAvailable();

    return QNetworkReply::bytesAvailable() + d->downloadBufferCurrentSize - d->downloadBufferReadPosition;
}

bool QNetworkReplyEmscriptenImpl::isSequential() const
{
    return true;
}

qint64 QNetworkReplyEmscriptenImpl::size() const
{
    return QNetworkReply::size();
}

/*!
    \internal
*/
qint64 QNetworkReplyEmscriptenImpl::readData(char *data, qint64 maxlen)
{
    Q_D(QNetworkReplyEmscriptenImpl);

    qint64 howMuch = qMin(maxlen, (d->downloadBufferCurrentSize - d->downloadBufferReadPosition));

    qDebug() << Q_FUNC_INFO << "howMuch"<< howMuch
             << "d->downloadBufferReadPosition" << d->downloadBufferReadPosition;

    if (d->downloadZerocopyBuffer) {
        memcpy(data, d->downloadZerocopyBuffer + d->downloadBufferReadPosition, howMuch);
        d->downloadBufferReadPosition += howMuch ;
        return howMuch;
    }
    return 0;
}

void QNetworkReplyEmscriptenImpl::emitReplyError(QNetworkReply::NetworkError errorCode)
{
    qDebug() << Q_FUNC_INFO << this<< errorCode;

    setFinished(true);
    emit error(errorCode);
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
    qApp->processEvents();
}

void QNetworkReplyEmscriptenImplPrivate::onLoadCallback(int readyState, int buffer, int bufferSize)
{
    // FIXME TODO do something with null termination lines ??
    qDebug() << Q_FUNC_INFO << (int)readyState << bufferSize;

    switch(readyState) {
    case 0://unsent
        break;
    case 1://opened
        break;
    case 2://headers received
        break;
    case 3://loading
        break;
    case 4://done
        gHandler->dataReceived((char *)buffer/*, bufferSize*/);
        break;
    };
 }

void QNetworkReplyEmscriptenImplPrivate::onProgressCallback(int done, int total, uint timestamp)
{
    gHandler->emitDataReadProgress(done, total);
}

void QNetworkReplyEmscriptenImplPrivate::onRequestErrorCallback(int state, int status)
{
    qDebug() << Q_FUNC_INFO << state << status;
    gHandler->emitReplyError(QNetworkReply::UnknownNetworkError);
}

void QNetworkReplyEmscriptenImplPrivate::onResponseHeadersCallback(int headers)
{
    gHandler->headersReceived((char *)headers);
}

void QNetworkReplyEmscriptenImplPrivate::doSendRequest(const QString &methodName, const QNetworkRequest &request)
{
    totalDownloadSize = 0;

    jsRequest(methodName, request.url().toString(), (void *)&onLoadCallback,
              (void *)&onProgressCallback, (void *)&onRequestErrorCallback, (void *)&onResponseHeadersCallback);
}


/* const QString &body, const QList<QPair<QByteArray, QByteArray> > &headers ,*/
void QNetworkReplyEmscriptenImplPrivate::jsRequest(const QString &verb, const QString &url, void *loadCallback, void *progressCallback, void *errorCallback, void *onResponseHeadersCallback)
{
    qDebug() << Q_FUNC_INFO << verb << url;
      // Probably a good idea to save any shared pointers as members in C++
      // so the objects they point to survive as long as you need them

        EM_ASM_ARGS({
          var verb = Pointer_stringify($0);
          var url = Pointer_stringify($1);
          var onLoadCallbackPointer = $2;
          var onProgressCallbackPointer = $3;
          var onErrorCallbackPointer = $4;
          var onHeadersCallback = $5;

          var xhr;
          xhr = new XMLHttpRequest();
          xhr.responseType = "arraybuffer";

//xhr.withCredentials = true;
          xhr.open(verb, url, true); //async
  // xhrReq.open(method, url, async, user, password);

        xhr.onprogress = function(e) {
            switch(xhr.status) {
              case 200:
              case 206:
              case 300:
              case 301:
              case 302: {
                 var date = xhr.getResponseHeader('Last-Modified');
                 date = ((date != null) ? new Date(date).getTime() / 1000 : 0);
                 Runtime.dynCall('viii', onProgressCallbackPointer, [e.loaded, e.total, date]);
              }
             break;
           }
        };
        xhr.onreadystatechange = function() {
            if (xhr.readyState == xhr.HEADERS_RECEIVED) {
               var responseStr = xhr.getAllResponseHeaders();
               var ptr = allocate(intArrayFromString(responseStr), 'i8', ALLOC_NORMAL);
               Runtime.dynCall('vi', onHeadersCallback, [ptr]);
               _free(ptr);
              }
        };

       xhr.onload = function(e) {
        var byteArray = new Uint8Array(this.response);
        var buffer = _malloc(byteArray.length);
        HEAPU8.set(byteArray, buffer);
        Module.Runtime.dynCall('viii', onLoadCallbackPointer, [this.readyState, buffer, byteArray.length]);
        _free(buffer);
      };

        // error
        xhr.onerror = function(e) {
            Runtime.dynCall('vii', onErrorCallbackPointer, [xhr.readyState, xhr.status]);
        };
        //TODO headers, other operations, handle user/pass, handle binary data
       //xhr.setRequestHeader(header, value);
        xhr.send(null);

      }, verb.toLatin1().data(), url.toLatin1().data(), loadCallback, progressCallback, errorCallback, onResponseHeadersCallback/*, stateChangedCallback*/);
}

void QNetworkReplyEmscriptenImplPrivate::emitReplyError(QNetworkReply::NetworkError errorCode)
{
    qDebug() << Q_FUNC_INFO << this << errorCode;

    Q_Q(QNetworkReplyEmscriptenImpl);
    emit q->error(QNetworkReply::UnknownNetworkError);

    emit q->finished();
    qApp->processEvents();
}

void QNetworkReplyEmscriptenImplPrivate::emitDataReadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    totalDownloadSize = bytesTotal;

    percentFinished = (bytesReceived / bytesTotal) * 100;
    qDebug() << Q_FUNC_INFO << bytesReceived << bytesTotal << percentFinished << "%";
}

void QNetworkReplyEmscriptenImplPrivate::dataReceived(char *buffer)
{
    Q_Q(QNetworkReplyEmscriptenImpl);

    int bufferSize = strlen(buffer);

    if (bufferSize > 0)
        q->setReadBufferSize(bufferSize);

    bytesDownloaded = bufferSize;

    if (percentFinished != 100) {
        downloadBufferCurrentSize += bufferSize;
    } else {
        downloadBufferCurrentSize = bufferSize;
    }
    totalDownloadSize = downloadBufferCurrentSize;

    emit q->downloadProgress(bytesDownloaded, totalDownloadSize);
    qApp->processEvents();

    qDebug() << Q_FUNC_INFO <<"current size" << downloadBufferCurrentSize;
    qDebug() << Q_FUNC_INFO <<"total size" << totalDownloadSize;// /*<< (int)state*/ << (char *)buffer;

    downloadZerocopyBuffer = buffer;
//     q->setAttribute(QNetworkRequest::DownloadBufferAttribute, downloadZerocopyBuffer);

     if (downloadBufferCurrentSize == totalDownloadSize) {
         q->setFinished(true);
         emit q->readyRead();
         emit q->finished();
     }

    emit q->metaDataChanged();

    qApp->processEvents();
}

//taken from qnetworkrequest.cpp
static int parseHeaderName(const QByteArray &headerName)
{
    if (headerName.isEmpty())
        return -1;

    switch (tolower(headerName.at(0))) {
    case 'c':
        if (qstricmp(headerName.constData(), "content-type") == 0)
            return QNetworkRequest::ContentTypeHeader;
        else if (qstricmp(headerName.constData(), "content-length") == 0)
            return QNetworkRequest::ContentLengthHeader;
        else if (qstricmp(headerName.constData(), "cookie") == 0)
            return QNetworkRequest::CookieHeader;
        break;

    case 'l':
        if (qstricmp(headerName.constData(), "location") == 0)
            return QNetworkRequest::LocationHeader;
        else if (qstricmp(headerName.constData(), "last-modified") == 0)
            return QNetworkRequest::LastModifiedHeader;
        break;

    case 's':
        if (qstricmp(headerName.constData(), "set-cookie") == 0)
            return QNetworkRequest::SetCookieHeader;
        else if (qstricmp(headerName.constData(), "server") == 0)
            return QNetworkRequest::ServerHeader;
        break;

    case 'u':
        if (qstricmp(headerName.constData(), "user-agent") == 0)
            return QNetworkRequest::UserAgentHeader;
        break;
    }

    return -1; // nothing found
}


void QNetworkReplyEmscriptenImplPrivate::headersReceived(char *buffer)
{
    Q_Q(QNetworkReplyEmscriptenImpl);

    QStringList headers = QString(buffer).split("\r\n", QString::SkipEmptyParts);

    for (int i = 0; i < headers.size(); i++) {
        QString headerName = headers.at(i).split(": ").at(0);
        QString headersValue = headers.at(i).split(": ").at(1);
        if (headerName.isEmpty() || headersValue.isEmpty())
            continue;

       q->setHeader(static_cast<QNetworkRequest::KnownHeaders>(parseHeaderName(headerName.toLocal8Bit())), (QVariant)headersValue);
    }
}

QT_END_NAMESPACE

#include "moc_qnetworkreplyemscriptenimpl_p.cpp"

