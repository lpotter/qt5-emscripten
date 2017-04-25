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

#ifndef QNETWORKREPLYEMSCRIPTENIMPL_H
#define QNETWORKREPLYEMSCRIPTENIMPL_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "qnetworkreply.h"
#include "qnetworkreply_p.h"
#include "qnetworkaccessmanager.h"
#include <QFile>
#include <private/qabstractfileengine_p.h>

#include <emscripten.h>
#include <emscripten/html5.h>

QT_BEGIN_NAMESPACE

class QIODevice;

class QNetworkReplyEmscriptenImplPrivate;
class QNetworkReplyEmscriptenImpl: public QNetworkReply
{
    Q_OBJECT
public:
    QNetworkReplyEmscriptenImpl(QNetworkAccessManager *manager, const QNetworkRequest &req, const QNetworkAccessManager::Operation op);
    ~QNetworkReplyEmscriptenImpl();
    virtual void abort() Q_DECL_OVERRIDE;

    // reimplemented from QNetworkReply
    virtual void close() Q_DECL_OVERRIDE;
    virtual qint64 bytesAvailable() const Q_DECL_OVERRIDE;
    virtual bool isSequential () const Q_DECL_OVERRIDE;
    qint64 size() const Q_DECL_OVERRIDE;

    virtual qint64 readData(char *data, qint64 maxlen) Q_DECL_OVERRIDE;

    void setup(QNetworkAccessManager::Operation op, const QNetworkRequest &request,
               QIODevice *outgoingData);

    Q_DECLARE_PRIVATE(QNetworkReplyEmscriptenImpl)

    Q_PRIVATE_SLOT(d_func(), void emitReplyError(QNetworkReply::NetworkError errorCode))
    Q_PRIVATE_SLOT(d_func(), void emitDataReadProgress(qint64 done, qint64 total))
    Q_PRIVATE_SLOT(d_func(), void dataReceived(char *buffer))

public Q_SLOTS:
    void emitReplyError(QNetworkReply::NetworkError errorCode);
    void connectionFinished();

private:
    QByteArray methodName() const;
  //  QSharedDataPointer<QNetworkReplyEmscriptenImplPrivate> d;


};

class QNetworkReplyEmscriptenImplPrivate: public QNetworkReplyPrivate
{
public:
    QNetworkReplyEmscriptenImplPrivate();

    QNetworkAccessManagerPrivate *managerPrivate;
    void doSendRequest(const QString &methodName, const QNetworkRequest &request);

    void jsRequest(const QString &verb, const QString &url, void *, void *, void *, void *);

    static void onLoadCallback(int status, int textBuffer, int size);
    static void onProgressCallback(int done, int bytesTotal, uint timestamp);
    static void onRequestErrorCallback(int e, int status);
    static void onStateChangedCallback(int status);
    static void onResponseHeadersCallback(int headers);

    void emitReplyError(QNetworkReply::NetworkError errorCode);
    void emitDataReadProgress(qint64 done, qint64 total);
    void dataReceived(char *buffer/*, qint64 bufferSize*/);

//    QIODevice *outgoingData;

    QSharedPointer<QAtomicInt> pendingDownloadData;
    QSharedPointer<QAtomicInt> pendingDownloadProgress;

//    QSharedPointer<QRingBuffer> outgoingDataBuffer;
    qint64 bytesDownloaded;
    qint64 bytesBuffered;

    qint64 downloadBufferReadPosition;
    qint64 downloadBufferCurrentSize;
    qint64 totalDownloadSize;
    qint64 percentFinished;
    char *downloadZerocopyBuffer;

    Q_DECLARE_PUBLIC(QNetworkReplyEmscriptenImpl)
};

QT_END_NAMESPACE

//Q_DECLARE_METATYPE(QNetworkRequest::KnownHeaders)

#endif // QNETWORKREPLYEMSCRIPTENIMPL_H
