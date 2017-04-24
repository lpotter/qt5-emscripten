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
    : QNetworkReplyPrivate(), managerPrivate(0)
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

    if (req.attribute(QNetworkRequest::BackgroundRequestAttribute).toBool()) { // Asynchronous open
    } else { // Synch open
        d->doSendRequest(methodName(), req);
    }
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
    qDebug() << Q_FUNC_INFO << d->isFinished
             << QNetworkReply::bytesAvailable();
    if (!d->isFinished)
        return QNetworkReply::bytesAvailable();

    return QNetworkReply::bytesAvailable()/* + d->realFile->bytesAvailable()*/;
}

bool QNetworkReplyEmscriptenImpl::isSequential() const
{
    return true;
}

qint64 QNetworkReplyEmscriptenImpl::size() const
{
    bool ok;
    int size = header(QNetworkRequest::ContentLengthHeader).toInt(&ok);

    qDebug() << Q_FUNC_INFO << size;
    return ok ? size : 0;
}

/*!
    \internal
*/
qint64 QNetworkReplyEmscriptenImpl::readData(char */*data*/, qint64 /*maxlen*/)
{
    Q_D(QNetworkReplyEmscriptenImpl);

    qDebug() << Q_FUNC_INFO;

    if (!d->isFinished)
        return -1;

//    qint64 ret = d->realFile->read(data, maxlen);
//    if (bytesAvailable() == 0)
//        d->realFile->close();
//    if (ret == 0 && bytesAvailable() == 0)
//        return -1;
//    else {
//        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
//        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, QLatin1String("OK"));
//        return ret;
//    }
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


void QNetworkReplyEmscriptenImplPrivate::onLoadCallback(int readyState, int buffer)
{
    qDebug() << Q_FUNC_INFO << (int)readyState << (char *)buffer;
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
        break;
    };
    gHandler->dataReceived((char *)buffer);
 }

void QNetworkReplyEmscriptenImplPrivate::onProgressCallback(int done, int total, uint timestamp)
{
    qDebug() << Q_FUNC_INFO << done << total << timestamp;
    gHandler->emitDataReadProgress(done, total);
}

void QNetworkReplyEmscriptenImplPrivate::onRequestErrorCallback(/*int e, int status*/)
{
    gHandler->emitReplyError(QNetworkReply::UnknownNetworkError);
}

void QNetworkReplyEmscriptenImplPrivate::doSendRequest(const QString &methodName, const QNetworkRequest &request)
{
    jsRequest(methodName, request.url().toString(), (void *)&onLoadCallback,
              (void *)&onProgressCallback, (void *)&onRequestErrorCallback/*, (void *)&onStateChangedCallback */);

}

/* const QString &body, const QList<QPair<QByteArray, QByteArray> > &headers ,*/
void QNetworkReplyEmscriptenImplPrivate::jsRequest(const QString &verb, const QString &url, void *loadCallback, void *progressCallback, void *errorCallback/*, void *stateChangedCallback*/)
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
      //    var onStateChangedCallback = $5;

          var xhr;
          xhr = new XMLHttpRequest();
          xhr.responseType = "arraybuffer";

//xhr.withCredentials = true;
          xhr.open(verb, url, true); //async
  // xhrReq.open(method, url, async, user, password);

        xhr.onprogress = function(e) {
                            console.log(xhr.status);
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
              default: {

              }
              break;
           }
        };
          xhr.onload = function(e) {
            var byteArray = new Uint8Array(this.response);
            var buffer = _malloc(byteArray.length);
            HEAPU8.set(byteArray, buffer);
            Module.Runtime.dynCall('vii', onLoadCallbackPointer, [this.readyState, buffer]);
            _free(buffer);
          };

        // error
        xhr.onerror = function(e) {
            Runtime.dynCall('vii', onErrorCallbackPointer, [this.readyState, this->status]);
        };
        //TODO headers, other operations, handle user/pass, handle binary data
       //xhr.setRequestHeader(header, value);
        xhr.send(null);

      }, verb.toLatin1().data(), url.toLatin1().data(), loadCallback, progressCallback, errorCallback/*, stateChangedCallback*/);
}


void QNetworkReplyEmscriptenImplPrivate::emitReplyError(QNetworkReply::NetworkError errorCode)
{
    qDebug() << Q_FUNC_INFO << this << errorCode;

    Q_Q(QNetworkReplyEmscriptenImpl);
    QMetaObject::invokeMethod(q, "error", Qt::DirectConnection,
                              Q_ARG(QNetworkReply::NetworkError, QNetworkReply::UnknownNetworkError));

    QMetaObject::invokeMethod(q, "finished", Qt::DirectConnection);
    qApp->processEvents();
}

void QNetworkReplyEmscriptenImplPrivate::emitDataReadProgress(qint64 bytesReceived, qint64 bytesTotal)
{

    qDebug() << Q_FUNC_INFO << bytesReceived << bytesTotal;
    Q_Q(QNetworkReplyEmscriptenImpl);

    QMetaObject::invokeMethod(q, "downloadProgress", Qt::QueuedConnection,
                              Q_ARG(qint64, bytesReceived), Q_ARG(qint64, bytesTotal));
    qApp->processEvents();
}

void QNetworkReplyEmscriptenImplPrivate::dataReceived(const char *buffer)
{
  //  qDebug() << Q_FUNC_INFO << (int)data << (char *)buffer;
    qDebug() << Q_FUNC_INFO;
    Q_Q(QNetworkReplyEmscriptenImpl);

    q->setFinished(true);
    q->bytesAvailable();

    QMetaObject::invokeMethod(q, "metaDataChanged", Qt::QueuedConnection);
    QMetaObject::invokeMethod(q, "downloadProgress", Qt::QueuedConnection,
        Q_ARG(qint64, sizeof(buffer)), Q_ARG(qint64, sizeof(buffer)));
    QMetaObject::invokeMethod(q, "readyRead", Qt::QueuedConnection);
    QMetaObject::invokeMethod(q, "finished", Qt::QueuedConnection);

    qApp->processEvents();

}

QT_END_NAMESPACE

#include "moc_qnetworkreplyemscriptenimpl_p.cpp"

