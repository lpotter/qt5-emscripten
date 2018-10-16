/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QGET_H
#define QGET_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>

class TransferItem : public QObject
{
    Q_OBJECT
public:
    enum Method {Get,Put,Post};
    TransferItem(const QNetworkRequest &r, const QString &u, const QString &p, QNetworkAccessManager &n, Method m);
    void start();
signals:
    void downloadFinished(TransferItem *self);
public slots:
    void progress(qint64,qint64);
public:
    Method method;
    QNetworkRequest request;
    QNetworkReply *reply;
    QNetworkAccessManager &nam;
    QFile *inputFile;
    QFile *outputFile;
    QList<QUrl> redirects;
    QString user;
    QString password;
};

class DownloadItem : public TransferItem
{
    Q_OBJECT
public:
    DownloadItem(const QNetworkRequest &r, const QString &user, const QString &password, QNetworkAccessManager &nam);
    ~DownloadItem();

private slots:
    void readyRead();
    void finished();
private:
};

class UploadItem : public TransferItem
{
    Q_OBJECT
public:
    UploadItem(const QNetworkRequest &r, const QString &user, const QString &password, QNetworkAccessManager &nam, QFile *f, TransferItem::Method method);
    ~UploadItem();
private slots:
    void finished();
};

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    DownloadManager();
    ~DownloadManager();
    void get(const QNetworkRequest &request, const QString &user, const QString &password);
    void upload(const QNetworkRequest &request, const QString &user, const QString &password, const QString &filename, TransferItem::Method method);
    void setProxy(const QNetworkProxy &proxy) { nam.setProxy(proxy); }
    void setProxyUser(const QString &user) { proxyUser = user; }
    void setProxyPassword(const QString &password) { proxyPassword = password; }
    enum QueueMode { Parallel, Serial };
    void setQueueMode(QueueMode mode);

public slots:
    void checkForAllDone();

private slots:
    void finished(QNetworkReply *reply);
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
#endif
    void downloadFinished(TransferItem *item);
private:
    TransferItem *findTransfer(QNetworkReply *reply);

    QNetworkAccessManager nam;
    QList<TransferItem*> transfers;
    QString proxyUser;
    QString proxyPassword;
    QueueMode queueMode;
};

#endif // QGET_H
