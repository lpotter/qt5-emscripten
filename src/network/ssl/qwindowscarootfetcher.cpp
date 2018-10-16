/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qwindowscarootfetcher_p.h"

#include <QtCore/QThread>
#include <QtGlobal>

#ifdef QSSLSOCKET_DEBUG
#include "qssl_p.h" // for debug categories
#include <QtCore/QElapsedTimer>
#endif

#include "qsslsocket_p.h" // Transitively includes Wincrypt.h

QT_BEGIN_NAMESPACE

class QWindowsCaRootFetcherThread : public QThread
{
public:
    QWindowsCaRootFetcherThread()
    {
        qRegisterMetaType<QSslCertificate>();
        setObjectName(QStringLiteral("QWindowsCaRootFetcher"));
        start();
    }
    ~QWindowsCaRootFetcherThread()
    {
        quit();
        wait(15500); // worst case, a running request can block for 15 seconds
    }
};

Q_GLOBAL_STATIC(QWindowsCaRootFetcherThread, windowsCaRootFetcherThread);

QWindowsCaRootFetcher::QWindowsCaRootFetcher(const QSslCertificate &certificate, QSslSocket::SslMode sslMode)
    : cert(certificate), mode(sslMode)
{
    moveToThread(windowsCaRootFetcherThread());
}

QWindowsCaRootFetcher::~QWindowsCaRootFetcher()
{
}

void QWindowsCaRootFetcher::start()
{
    QByteArray der = cert.toDer();
    PCCERT_CONTEXT wincert = CertCreateCertificateContext(X509_ASN_ENCODING, (const BYTE *)der.constData(), der.length());
    if (!wincert) {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl, "QWindowsCaRootFetcher failed to convert certificate to windows form");
#endif
        emit finished(cert, QSslCertificate());
        deleteLater();
        return;
    }

    CERT_CHAIN_PARA parameters;
    memset(&parameters, 0, sizeof(parameters));
    parameters.cbSize = sizeof(parameters);
    // set key usage constraint
    parameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    parameters.RequestedUsage.Usage.cUsageIdentifier = 1;
    LPSTR oid = (LPSTR)(mode == QSslSocket::SslClientMode ? szOID_PKIX_KP_SERVER_AUTH : szOID_PKIX_KP_CLIENT_AUTH);
    parameters.RequestedUsage.Usage.rgpszUsageIdentifier = &oid;

#ifdef QSSLSOCKET_DEBUG
    QElapsedTimer stopwatch;
    stopwatch.start();
#endif
    PCCERT_CHAIN_CONTEXT chain;
    BOOL result = CertGetCertificateChain(
        nullptr, //default engine
        wincert,
        nullptr, //current date/time
        nullptr, //default store
        &parameters,
        0, //default dwFlags
        nullptr, //reserved
        &chain);
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "QWindowsCaRootFetcher" << stopwatch.elapsed() << "ms to get chain";
#endif

    QSslCertificate trustedRoot;
    if (result) {
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << "QWindowsCaRootFetcher - examining windows chains";
        if (chain->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR)
            qCDebug(lcSsl) << " - TRUSTED";
        else
            qCDebug(lcSsl) << " - NOT TRUSTED" << chain->TrustStatus.dwErrorStatus;
        if (chain->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED)
            qCDebug(lcSsl) << " - SELF SIGNED";
        qCDebug(lcSsl) << "QSslSocketBackendPrivate::fetchCaRootForCert - dumping simple chains";
        for (unsigned int i = 0; i < chain->cChain; i++) {
            if (chain->rgpChain[i]->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR)
                qCDebug(lcSsl) << " - TRUSTED SIMPLE CHAIN" << i;
            else
                qCDebug(lcSsl) << " - UNTRUSTED SIMPLE CHAIN" << i << "reason:" << chain->rgpChain[i]->TrustStatus.dwErrorStatus;
            for (unsigned int j = 0; j < chain->rgpChain[i]->cElement; j++) {
                QSslCertificate foundCert(QByteArray((const char *)chain->rgpChain[i]->rgpElement[j]->pCertContext->pbCertEncoded
                    , chain->rgpChain[i]->rgpElement[j]->pCertContext->cbCertEncoded), QSsl::Der);
                qCDebug(lcSsl) << "   - " << foundCert;
            }
        }
        qCDebug(lcSsl) << " - and" << chain->cLowerQualityChainContext << "low quality chains"; //expect 0, we haven't asked for them
#endif

        //based on http://msdn.microsoft.com/en-us/library/windows/desktop/aa377182%28v=vs.85%29.aspx
        //about the final chain rgpChain[cChain-1] which must begin with a trusted root to be valid
        if (chain->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR
            && chain->cChain > 0) {
            const PCERT_SIMPLE_CHAIN finalChain = chain->rgpChain[chain->cChain - 1];
            // http://msdn.microsoft.com/en-us/library/windows/desktop/aa377544%28v=vs.85%29.aspx
            // rgpElement[0] is the end certificate chain element. rgpElement[cElement-1] is the self-signed "root" certificate element.
            if (finalChain->TrustStatus.dwErrorStatus == CERT_TRUST_NO_ERROR
                && finalChain->cElement > 0) {
                    trustedRoot = QSslCertificate(QByteArray((const char *)finalChain->rgpElement[finalChain->cElement - 1]->pCertContext->pbCertEncoded
                        , finalChain->rgpElement[finalChain->cElement - 1]->pCertContext->cbCertEncoded), QSsl::Der);
            }
        }
        CertFreeCertificateChain(chain);
    }
    CertFreeCertificateContext(wincert);

    emit finished(cert, trustedRoot);
    deleteLater();
}

QT_END_NAMESPACE
