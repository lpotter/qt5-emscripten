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

#include "qnetworkaccessfilebackend_p.h"
#include "qfileinfo.h"
#if QT_CONFIG(ftp)
#include "qurlinfo_p.h"
#endif
#include "qdir.h"
#include "private/qnoncontiguousbytedevice_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>

QT_BEGIN_NAMESPACE

QStringList QNetworkAccessFileBackendFactory::supportedSchemes() const
{
    QStringList schemes;
    schemes << QStringLiteral("file")
            << QStringLiteral("qrc");
#if defined(Q_OS_ANDROID)
    schemes << QStringLiteral("assets");
#endif
    return schemes;
}

QNetworkAccessBackend *
QNetworkAccessFileBackendFactory::create(QNetworkAccessManager::Operation op,
                                         const QNetworkRequest &request) const
{
    // is it an operation we know of?
    switch (op) {
    case QNetworkAccessManager::GetOperation:
    case QNetworkAccessManager::PutOperation:
        break;

    default:
        // no, we can't handle this operation
        return 0;
    }

    QUrl url = request.url();
    if (url.scheme().compare(QLatin1String("qrc"), Qt::CaseInsensitive) == 0
#if defined(Q_OS_ANDROID)
            || url.scheme().compare(QLatin1String("assets"), Qt::CaseInsensitive) == 0
#endif
            || url.isLocalFile()) {
        return new QNetworkAccessFileBackend;
    } else if (!url.scheme().isEmpty() && url.authority().isEmpty() && (url.scheme().length() > 1)) {
        // check if QFile could, in theory, open this URL via the file engines
        // it has to be in the format:
        //    prefix:path/to/file
        // or prefix:/path/to/file
        //
        // this construct here must match the one below in open()
        QFileInfo fi(url.toString(QUrl::RemoveAuthority | QUrl::RemoveFragment | QUrl::RemoveQuery));
        if (fi.exists() || (op == QNetworkAccessManager::PutOperation && fi.dir().exists()))
            return new QNetworkAccessFileBackend;
    }

    return 0;
}

QNetworkAccessFileBackend::QNetworkAccessFileBackend()
    : totalBytes(0), hasUploadFinished(false)
{
}

QNetworkAccessFileBackend::~QNetworkAccessFileBackend()
{
}

void QNetworkAccessFileBackend::open()
{
    QUrl url = this->url();

    if (url.host() == QLatin1String("localhost"))
        url.setHost(QString());
#if !defined(Q_OS_WIN)
    // do not allow UNC paths on Unix
    if (!url.host().isEmpty()) {
        // we handle only local files
        error(QNetworkReply::ProtocolInvalidOperationError,
              QCoreApplication::translate("QNetworkAccessFileBackend", "Request for opening non-local file %1").arg(url.toString()));
        finished();
        return;
    }
#endif // !defined(Q_OS_WIN)
    if (url.path().isEmpty())
        url.setPath(QLatin1String("/"));
    setUrl(url);

    QString fileName = url.toLocalFile();
    if (fileName.isEmpty()) {
        if (url.scheme() == QLatin1String("qrc")) {
            fileName = QLatin1Char(':') + url.path();
        } else {
#if defined(Q_OS_ANDROID)
            if (url.scheme() == QLatin1String("assets"))
                fileName = QLatin1String("assets:") + url.path();
            else
#endif
                fileName = url.toString(QUrl::RemoveAuthority | QUrl::RemoveFragment | QUrl::RemoveQuery);
        }
    }
    file.setFileName(fileName);

    if (operation() == QNetworkAccessManager::GetOperation) {
        if (!loadFileInfo())
            return;
    }

    QIODevice::OpenMode mode;
    switch (operation()) {
    case QNetworkAccessManager::GetOperation:
        mode = QIODevice::ReadOnly;
        break;
    case QNetworkAccessManager::PutOperation:
        mode = QIODevice::WriteOnly | QIODevice::Truncate;
        createUploadByteDevice();
        QObject::connect(uploadByteDevice.data(), SIGNAL(readyRead()), this, SLOT(uploadReadyReadSlot()));
        QMetaObject::invokeMethod(this, "uploadReadyReadSlot", Qt::QueuedConnection);
        break;
    default:
        Q_ASSERT_X(false, "QNetworkAccessFileBackend::open",
                   "Got a request operation I cannot handle!!");
        return;
    }

    mode |= QIODevice::Unbuffered;
    bool opened = file.open(mode);

    // could we open the file?
    if (!opened) {
        QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Error opening %1: %2")
                                                .arg(this->url().toString(), file.errorString());

        // why couldn't we open the file?
        // if we're opening for reading, either it doesn't exist, or it's access denied
        // if we're opening for writing, not existing means it's access denied too
        if (file.exists() || operation() == QNetworkAccessManager::PutOperation)
            error(QNetworkReply::ContentAccessDenied, msg);
        else
            error(QNetworkReply::ContentNotFoundError, msg);
        finished();
    }
}

void QNetworkAccessFileBackend::uploadReadyReadSlot()
{
    if (hasUploadFinished)
        return;

    forever {
        qint64 haveRead;
        const char *readPointer = uploadByteDevice->readPointer(-1, haveRead);
        if (haveRead == -1) {
            // EOF
            hasUploadFinished = true;
            file.flush();
            file.close();
            finished();
            break;
        } else if (haveRead == 0 || readPointer == 0) {
            // nothing to read right now, we will be called again later
            break;
        } else {
            qint64 haveWritten;
            haveWritten = file.write(readPointer, haveRead);

            if (haveWritten < 0) {
                // write error!
                QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Write error writing to %1: %2")
                              .arg(url().toString(), file.errorString());
                error(QNetworkReply::ProtocolFailure, msg);

                finished();
                return;
            } else {
                uploadByteDevice->advanceReadPointer(haveWritten);
            }


            file.flush();
        }
    }
}

void QNetworkAccessFileBackend::closeDownstreamChannel()
{
    if (operation() == QNetworkAccessManager::GetOperation) {
        file.close();
    }
}

void QNetworkAccessFileBackend::downstreamReadyWrite()
{
    Q_ASSERT_X(operation() == QNetworkAccessManager::GetOperation, "QNetworkAccessFileBackend",
               "We're being told to download data but operation isn't GET!");

    readMoreFromFile();
}

bool QNetworkAccessFileBackend::loadFileInfo()
{
    QFileInfo fi(file);
    setHeader(QNetworkRequest::LastModifiedHeader, fi.lastModified());
    setHeader(QNetworkRequest::ContentLengthHeader, fi.size());

    // signal we're open
    metaDataChanged();

    if (fi.isDir()) {
        error(QNetworkReply::ContentOperationNotPermittedError,
              QCoreApplication::translate("QNetworkAccessFileBackend", "Cannot open %1: Path is a directory").arg(url().toString()));
        finished();
        return false;
    }

    return true;
}

bool QNetworkAccessFileBackend::readMoreFromFile()
{
    qint64 wantToRead;
    while ((wantToRead = nextDownstreamBlockSize()) > 0) {
        // ### FIXME!!
        // Obtain a pointer from the ringbuffer!
        // Avoid extra copy
        QByteArray data;
        data.reserve(wantToRead);
        qint64 actuallyRead = file.read(data.data(), wantToRead);
        if (actuallyRead <= 0) {
            // EOF or error
            if (file.error() != QFile::NoError) {
                QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Read error reading from %1: %2")
                              .arg(url().toString(), file.errorString());
                error(QNetworkReply::ProtocolFailure, msg);

                finished();
                return false;
            }

            finished();
            return true;
        }

        data.resize(actuallyRead);
        totalBytes += actuallyRead;

        QByteDataBuffer list;
        list.append(data);
        data.clear(); // important because of implicit sharing!
        writeDownstreamData(list);
    }
    return true;
}

QT_END_NAMESPACE
