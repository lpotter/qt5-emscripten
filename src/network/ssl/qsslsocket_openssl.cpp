/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2014 Governikus GmbH & Co. KG
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

/****************************************************************************
**
** In addition, as a special exception, the copyright holders listed above give
** permission to link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of the "OpenSSL" library that use the
** same license as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

//#define QSSLSOCKET_DEBUG

#include "qssl_p.h"
#include "qsslsocket_openssl_p.h"
#include "qsslsocket_openssl_symbols_p.h"
#include "qsslsocket.h"
#include "qsslcertificate_p.h"
#include "qsslcipher_p.h"
#include "qsslkey_p.h"
#include "qsslellipticcurve.h"
#include "qsslpresharedkeyauthenticator.h"
#include "qsslpresharedkeyauthenticator_p.h"

#ifdef Q_OS_WIN
#include "qwindowscarootfetcher_p.h"
#endif

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qscopedvaluerollback.h>

#include <string.h>

QT_BEGIN_NAMESPACE

#if defined(Q_OS_WIN)
    PtrCertOpenSystemStoreW QSslSocketPrivate::ptrCertOpenSystemStoreW = nullptr;
    PtrCertFindCertificateInStore QSslSocketPrivate::ptrCertFindCertificateInStore = nullptr;
    PtrCertCloseStore QSslSocketPrivate::ptrCertCloseStore = nullptr;
#endif

bool QSslSocketPrivate::s_libraryLoaded = false;
bool QSslSocketPrivate::s_loadedCiphersAndCerts = false;
bool QSslSocketPrivate::s_loadRootCertsOnDemand = false;

#if OPENSSL_VERSION_NUMBER >= 0x10001000L
int QSslSocketBackendPrivate::s_indexForSSLExtraData = -1;
#endif

QString QSslSocketBackendPrivate::getErrorsFromOpenSsl()
{
    QString errorString;
    char buf[256] = {}; // OpenSSL docs claim both 120 and 256; use the larger.
    unsigned long errNum;
    while ((errNum = q_ERR_get_error())) {
        if (!errorString.isEmpty())
            errorString.append(QLatin1String(", "));
        q_ERR_error_string_n(errNum, buf, sizeof buf);
        errorString.append(QString::fromLatin1(buf)); // error is ascii according to man ERR_error_string
    }
    return errorString;
}

extern "C" {

#if OPENSSL_VERSION_NUMBER >= 0x10001000L && !defined(OPENSSL_NO_PSK)
static unsigned int q_ssl_psk_client_callback(SSL *ssl,
                                              const char *hint,
                                              char *identity, unsigned int max_identity_len,
                                              unsigned char *psk, unsigned int max_psk_len)
{
    QSslSocketBackendPrivate *d = reinterpret_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData));
    Q_ASSERT(d);
    return d->tlsPskClientCallback(hint, identity, max_identity_len, psk, max_psk_len);
}

static unsigned int q_ssl_psk_server_callback(SSL *ssl,
                                              const char *identity,
                                              unsigned char *psk, unsigned int max_psk_len)
{
    QSslSocketBackendPrivate *d = reinterpret_cast<QSslSocketBackendPrivate *>(q_SSL_get_ex_data(ssl, QSslSocketBackendPrivate::s_indexForSSLExtraData));
    Q_ASSERT(d);
    return d->tlsPskServerCallback(identity, psk, max_psk_len);
}
#endif
} // extern "C"

QSslSocketBackendPrivate::QSslSocketBackendPrivate()
    : ssl(nullptr),
      readBio(nullptr),
      writeBio(nullptr),
      session(nullptr)
{
    // Calls SSL_library_init().
    ensureInitialized();
}

QSslSocketBackendPrivate::~QSslSocketBackendPrivate()
{
    destroySslContext();
}

QSslCipher QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(const SSL_CIPHER *cipher)
{
    QSslCipher ciph;

    char buf [256];
    QString descriptionOneLine = QString::fromLatin1(q_SSL_CIPHER_description(cipher, buf, sizeof(buf)));

    const auto descriptionList = descriptionOneLine.splitRef(QLatin1Char(' '), QString::SkipEmptyParts);
    if (descriptionList.size() > 5) {
        // ### crude code.
        ciph.d->isNull = false;
        ciph.d->name = descriptionList.at(0).toString();

        QString protoString = descriptionList.at(1).toString();
        ciph.d->protocolString = protoString;
        ciph.d->protocol = QSsl::UnknownProtocol;
        if (protoString == QLatin1String("SSLv3"))
            ciph.d->protocol = QSsl::SslV3;
        else if (protoString == QLatin1String("SSLv2"))
            ciph.d->protocol = QSsl::SslV2;
        else if (protoString == QLatin1String("TLSv1"))
            ciph.d->protocol = QSsl::TlsV1_0;
        else if (protoString == QLatin1String("TLSv1.1"))
            ciph.d->protocol = QSsl::TlsV1_1;
        else if (protoString == QLatin1String("TLSv1.2"))
            ciph.d->protocol = QSsl::TlsV1_2;

        if (descriptionList.at(2).startsWith(QLatin1String("Kx=")))
            ciph.d->keyExchangeMethod = descriptionList.at(2).mid(3).toString();
        if (descriptionList.at(3).startsWith(QLatin1String("Au=")))
            ciph.d->authenticationMethod = descriptionList.at(3).mid(3).toString();
        if (descriptionList.at(4).startsWith(QLatin1String("Enc=")))
            ciph.d->encryptionMethod = descriptionList.at(4).mid(4).toString();
        ciph.d->exportable = (descriptionList.size() > 6 && descriptionList.at(6) == QLatin1String("export"));

        ciph.d->bits = q_SSL_CIPHER_get_bits(cipher, &ciph.d->supportedBits);
    }
    return ciph;
}

QSslErrorEntry QSslErrorEntry::fromStoreContext(X509_STORE_CTX *ctx)
{
    return {
        q_X509_STORE_CTX_get_error(ctx),
        q_X509_STORE_CTX_get_error_depth(ctx)
    };
}

// ### This list is shared between all threads, and protected by a
// mutex. Investigate using thread local storage instead.
struct QSslErrorList
{
    QMutex mutex;
    QVector<QSslErrorEntry> errors;
};

Q_GLOBAL_STATIC(QSslErrorList, _q_sslErrorList)

int q_X509Callback(int ok, X509_STORE_CTX *ctx)
{
    if (!ok) {
        // Store the error and at which depth the error was detected.
        _q_sslErrorList()->errors << QSslErrorEntry::fromStoreContext(ctx);
#if !QT_CONFIG(opensslv11)
#ifdef QSSLSOCKET_DEBUG
        qCDebug(lcSsl) << "verification error: dumping bad certificate";
        qCDebug(lcSsl) << QSslCertificatePrivate::QSslCertificate_from_X509(q_X509_STORE_CTX_get_current_cert(ctx)).toPem();
        qCDebug(lcSsl) << "dumping chain";
        const auto certs = QSslSocketBackendPrivate::STACKOFX509_to_QSslCertificates(q_X509_STORE_CTX_get_chain(ctx));
        for (const QSslCertificate &cert : certs) {
            qCDebug(lcSsl) << "Issuer:" << "O=" << cert.issuerInfo(QSslCertificate::Organization)
                << "CN=" << cert.issuerInfo(QSslCertificate::CommonName)
                << "L=" << cert.issuerInfo(QSslCertificate::LocalityName)
                << "OU=" << cert.issuerInfo(QSslCertificate::OrganizationalUnitName)
                << "C=" << cert.issuerInfo(QSslCertificate::CountryName)
                << "ST=" << cert.issuerInfo(QSslCertificate::StateOrProvinceName);
            qCDebug(lcSsl) << "Subject:" << "O=" << cert.subjectInfo(QSslCertificate::Organization)
                << "CN=" << cert.subjectInfo(QSslCertificate::CommonName)
                << "L=" << cert.subjectInfo(QSslCertificate::LocalityName)
                << "OU=" << cert.subjectInfo(QSslCertificate::OrganizationalUnitName)
                << "C=" << cert.subjectInfo(QSslCertificate::CountryName)
                << "ST=" << cert.subjectInfo(QSslCertificate::StateOrProvinceName);
            qCDebug(lcSsl) << "Valid:" << cert.effectiveDate() << '-' << cert.expiryDate();
        }
#endif // QSSLSOCKET_DEBUG
#endif // !QT_CONFIG(opensslv11)
    }
    // Always return OK to allow verification to continue. We handle the
    // errors gracefully after collecting all errors, after verification has
    // completed.
    return 1;
}

static void q_loadCiphersForConnection(SSL *connection, QList<QSslCipher> &ciphers,
                                       QList<QSslCipher> &defaultCiphers)
{
    Q_ASSERT(connection);

    STACK_OF(SSL_CIPHER) *supportedCiphers = q_SSL_get_ciphers(connection);
    for (int i = 0; i < q_sk_SSL_CIPHER_num(supportedCiphers); ++i) {
        if (SSL_CIPHER *cipher = q_sk_SSL_CIPHER_value(supportedCiphers, i)) {
            QSslCipher ciph = QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(cipher);
            if (!ciph.isNull()) {
                // Unconditionally exclude ADH and AECDH ciphers since they offer no MITM protection
                if (!ciph.name().toLower().startsWith(QLatin1String("adh")) &&
                    !ciph.name().toLower().startsWith(QLatin1String("exp-adh")) &&
                    !ciph.name().toLower().startsWith(QLatin1String("aecdh"))) {
                    ciphers << ciph;

                    if (ciph.usedBits() >= 128)
                        defaultCiphers << ciph;
                }
            }
        }
    }
}

// Defined in qsslsocket.cpp
void q_setDefaultDtlsCiphers(const QList<QSslCipher> &ciphers);

long QSslSocketBackendPrivate::setupOpenSslOptions(QSsl::SslProtocol protocol, QSsl::SslOptions sslOptions)
{
    long options;
    if (protocol == QSsl::TlsV1SslV3)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2;
    else if (protocol == QSsl::SecureProtocols)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3;
    else if (protocol == QSsl::TlsV1_0OrLater)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3;
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
    // Choosing Tlsv1_1OrLater or TlsV1_2OrLater on OpenSSL < 1.0.1
    // will cause an error in QSslContext::fromConfiguration, meaning
    // we will never get here.
    else if (protocol == QSsl::TlsV1_1OrLater)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1;
    else if (protocol == QSsl::TlsV1_2OrLater)
        options = SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1|SSL_OP_NO_TLSv1_1;
#endif
    else
        options = SSL_OP_ALL;

    // This option is disabled by default, so we need to be able to clear it
    if (sslOptions & QSsl::SslOptionDisableEmptyFragments)
        options |= SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
    else
        options &= ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;

#ifdef SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION
    // This option is disabled by default, so we need to be able to clear it
    if (sslOptions & QSsl::SslOptionDisableLegacyRenegotiation)
        options &= ~SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
    else
        options |= SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION;
#endif

#ifdef SSL_OP_NO_TICKET
    if (sslOptions & QSsl::SslOptionDisableSessionTickets)
        options |= SSL_OP_NO_TICKET;
#endif
#ifdef SSL_OP_NO_COMPRESSION
    if (sslOptions & QSsl::SslOptionDisableCompression)
        options |= SSL_OP_NO_COMPRESSION;
#endif

    if (!(sslOptions & QSsl::SslOptionDisableServerCipherPreference))
        options |= SSL_OP_CIPHER_SERVER_PREFERENCE;

    return options;
}

bool QSslSocketBackendPrivate::initSslContext()
{
    Q_Q(QSslSocket);

    // If no external context was set (e.g. bei QHttpNetworkConnection) we will create a default context
    if (!sslContextPointer) {
        // create a deep copy of our configuration
        QSslConfigurationPrivate *configurationCopy = new QSslConfigurationPrivate(configuration);
        configurationCopy->ref.store(0);              // the QSslConfiguration constructor refs up
        sslContextPointer = QSslContext::sharedFromConfiguration(mode, configurationCopy, allowRootCertOnDemandLoading);
    }

    if (sslContextPointer->error() != QSslError::NoError) {
        setErrorAndEmit(QAbstractSocket::SslInvalidUserDataError, sslContextPointer->errorString());
        sslContextPointer.clear(); // deletes the QSslContext
        return false;
    }

    // Create and initialize SSL session
    if (!(ssl = sslContextPointer->createSsl())) {
        // ### Bad error code
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Error creating SSL session, %1").arg(getErrorsFromOpenSsl()));
        return false;
    }

    if (configuration.protocol != QSsl::SslV2 &&
        configuration.protocol != QSsl::SslV3 &&
        configuration.protocol != QSsl::UnknownProtocol &&
        mode == QSslSocket::SslClientMode && QSslSocket::sslLibraryVersionNumber() >= 0x00090806fL) {
        // Set server hostname on TLS extension. RFC4366 section 3.1 requires it in ACE format.
        QString tlsHostName = verificationPeerName.isEmpty() ? q->peerName() : verificationPeerName;
        if (tlsHostName.isEmpty())
            tlsHostName = hostName;
        QByteArray ace = QUrl::toAce(tlsHostName);
        // only send the SNI header if the URL is valid and not an IP
        if (!ace.isEmpty()
            && !QHostAddress().setAddress(tlsHostName)
            && !(configuration.sslOptions & QSsl::SslOptionDisableServerNameIndication)) {
            // We don't send the trailing dot from the host header if present see
            // https://tools.ietf.org/html/rfc6066#section-3
            if (ace.endsWith('.'))
                ace.chop(1);
            if (!q_SSL_ctrl(ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, ace.data()))
                qCWarning(lcSsl, "could not set SSL_CTRL_SET_TLSEXT_HOSTNAME, Server Name Indication disabled");
        }
    }

    // Clear the session.
    errorList.clear();

    // Initialize memory BIOs for encryption and decryption.
    readBio = q_BIO_new(q_BIO_s_mem());
    writeBio = q_BIO_new(q_BIO_s_mem());
    if (!readBio || !writeBio) {
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Error creating SSL session: %1").arg(getErrorsFromOpenSsl()));
        return false;
    }

    // Assign the bios.
    q_SSL_set_bio(ssl, readBio, writeBio);

    if (mode == QSslSocket::SslClientMode)
        q_SSL_set_connect_state(ssl);
    else
        q_SSL_set_accept_state(ssl);

#if OPENSSL_VERSION_NUMBER >= 0x10001000L
    // Save a pointer to this object into the SSL structure.
    if (QSslSocket::sslLibraryVersionNumber() >= 0x10001000L)
        q_SSL_set_ex_data(ssl, s_indexForSSLExtraData, this);
#endif

#if OPENSSL_VERSION_NUMBER >= 0x10001000L && !defined(OPENSSL_NO_PSK)
    // Set the client callback for PSK
    if (QSslSocket::sslLibraryVersionNumber() >= 0x10001000L) {
        if (mode == QSslSocket::SslClientMode)
            q_SSL_set_psk_client_callback(ssl, &q_ssl_psk_client_callback);
        else if (mode == QSslSocket::SslServerMode)
            q_SSL_set_psk_server_callback(ssl, &q_ssl_psk_server_callback);
    }
#endif

    return true;
}

void QSslSocketBackendPrivate::destroySslContext()
{
    if (ssl) {
        q_SSL_free(ssl);
        ssl = nullptr;
    }
    sslContextPointer.clear();
}

/*!
    \internal

    Does the minimum amount of initialization to determine whether SSL
    is supported or not.
*/

bool QSslSocketPrivate::supportsSsl()
{
    return ensureLibraryLoaded();
}


/*!
    \internal

    Declared static in QSslSocketPrivate, makes sure the SSL libraries have
    been initialized.
*/

void QSslSocketPrivate::ensureInitialized()
{
    if (!supportsSsl())
        return;

    ensureCiphersAndCertsLoaded();
}

long QSslSocketPrivate::sslLibraryBuildVersionNumber()
{
    return OPENSSL_VERSION_NUMBER;
}

QString QSslSocketPrivate::sslLibraryBuildVersionString()
{
    // Using QStringLiteral to store the version string as unicode and
    // avoid false positives from Google searching the playstore for old
    // SSL versions. See QTBUG-46265
    return QStringLiteral(OPENSSL_VERSION_TEXT);
}

/*!
    \internal

    Declared static in QSslSocketPrivate, backend-dependent loading of
    application-wide global ciphers.
*/
void QSslSocketPrivate::resetDefaultCiphers()
{
#if QT_CONFIG(opensslv11)
    SSL_CTX *myCtx = q_SSL_CTX_new(q_TLS_client_method());
#else
    SSL_CTX *myCtx = q_SSL_CTX_new(q_SSLv23_client_method());
#endif
    SSL *mySsl = q_SSL_new(myCtx);

    QList<QSslCipher> ciphers;
    QList<QSslCipher> defaultCiphers;

    q_loadCiphersForConnection(mySsl, ciphers, defaultCiphers);

    q_SSL_CTX_free(myCtx);
    q_SSL_free(mySsl);

    setDefaultSupportedCiphers(ciphers);
    setDefaultCiphers(defaultCiphers);

#if QT_CONFIG(dtls)
    ciphers.clear();
    defaultCiphers.clear();
    myCtx = q_SSL_CTX_new(q_DTLS_client_method());
    if (myCtx) {
        mySsl = q_SSL_new(myCtx);
        if (mySsl) {
            q_loadCiphersForConnection(mySsl, ciphers, defaultCiphers);
            q_setDefaultDtlsCiphers(defaultCiphers);
            q_SSL_free(mySsl);
        }
        q_SSL_CTX_free(myCtx);
    }
#endif // dtls
}

void QSslSocketPrivate::resetDefaultEllipticCurves()
{
    QVector<QSslEllipticCurve> curves;

#ifndef OPENSSL_NO_EC
    const size_t curveCount = q_EC_get_builtin_curves(nullptr, 0);

    QVarLengthArray<EC_builtin_curve> builtinCurves(static_cast<int>(curveCount));

    if (q_EC_get_builtin_curves(builtinCurves.data(), curveCount) == curveCount) {
        curves.reserve(int(curveCount));
        for (size_t i = 0; i < curveCount; ++i) {
            QSslEllipticCurve curve;
            curve.id = builtinCurves[int(i)].nid;
            curves.append(curve);
        }
    }
#endif // OPENSSL_NO_EC

    // set the list of supported ECs, but not the list
    // of *default* ECs. OpenSSL doesn't like forcing an EC for the wrong
    // ciphersuite, so don't try it -- leave the empty list to mean
    // "the implementation will choose the most suitable one".
    setDefaultSupportedEllipticCurves(curves);
}

#ifndef Q_OS_DARWIN // Apple implementation in qsslsocket_mac_shared.cpp
QList<QSslCertificate> QSslSocketPrivate::systemCaCertificates()
{
    ensureInitialized();
#ifdef QSSLSOCKET_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif
    QList<QSslCertificate> systemCerts;
#if defined(Q_OS_WIN)
    if (ptrCertOpenSystemStoreW && ptrCertFindCertificateInStore && ptrCertCloseStore) {
        HCERTSTORE hSystemStore;
        hSystemStore = ptrCertOpenSystemStoreW(0, L"ROOT");
        if (hSystemStore) {
            PCCERT_CONTEXT pc = nullptr;
            while (1) {
                pc = ptrCertFindCertificateInStore(hSystemStore, X509_ASN_ENCODING, 0, CERT_FIND_ANY, nullptr, pc);
                if (!pc)
                    break;
                QByteArray der(reinterpret_cast<const char *>(pc->pbCertEncoded),
                               static_cast<int>(pc->cbCertEncoded));
                QSslCertificate cert(der, QSsl::Der);
                systemCerts.append(cert);
            }
            ptrCertCloseStore(hSystemStore, 0);
        }
    }
#elif defined(Q_OS_UNIX)
    QSet<QString> certFiles;
    QDir currentDir;
    QStringList nameFilters;
    QList<QByteArray> directories;
    QSsl::EncodingFormat platformEncodingFormat;
# ifndef Q_OS_ANDROID
    directories = unixRootCertDirectories();
    nameFilters << QLatin1String("*.pem") << QLatin1String("*.crt");
    platformEncodingFormat = QSsl::Pem;
# else
    // Q_OS_ANDROID
    QByteArray ministroPath = qgetenv("MINISTRO_SSL_CERTS_PATH"); // Set by Ministro
    directories << ministroPath;
    nameFilters << QLatin1String("*.der");
    platformEncodingFormat = QSsl::Der;
#  ifndef Q_OS_ANDROID_EMBEDDED
    if (ministroPath.isEmpty()) {
        QList<QByteArray> certificateData = fetchSslCertificateData();
        for (int i = 0; i < certificateData.size(); ++i) {
            systemCerts.append(QSslCertificate::fromData(certificateData.at(i), QSsl::Der));
        }
    } else
#  endif //Q_OS_ANDROID_EMBEDDED
# endif //Q_OS_ANDROID
    {
        currentDir.setNameFilters(nameFilters);
        for (int a = 0; a < directories.count(); a++) {
            currentDir.setPath(QLatin1String(directories.at(a)));
            QDirIterator it(currentDir);
            while (it.hasNext()) {
                it.next();
                // use canonical path here to not load the same certificate twice if symlinked
                certFiles.insert(it.fileInfo().canonicalFilePath());
            }
        }
        for (const QString& file : qAsConst(certFiles))
            systemCerts.append(QSslCertificate::fromPath(file, platformEncodingFormat));
# ifndef Q_OS_ANDROID
        systemCerts.append(QSslCertificate::fromPath(QLatin1String("/etc/pki/tls/certs/ca-bundle.crt"), QSsl::Pem)); // Fedora, Mandriva
        systemCerts.append(QSslCertificate::fromPath(QLatin1String("/usr/local/share/certs/ca-root-nss.crt"), QSsl::Pem)); // FreeBSD's ca_root_nss
# endif
    }
#endif
#ifdef QSSLSOCKET_DEBUG
    qCDebug(lcSsl) << "systemCaCertificates retrieval time " << timer.elapsed() << "ms";
    qCDebug(lcSsl) << "imported " << systemCerts.count() << " certificates";
#endif

    return systemCerts;
}
#endif // Q_OS_DARWIN

void QSslSocketBackendPrivate::startClientEncryption()
{
    if (!initSslContext()) {
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Unable to init SSL Context: %1").arg(getErrorsFromOpenSsl()));
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    startHandshake();
    transmit();
}

void QSslSocketBackendPrivate::startServerEncryption()
{
    if (!initSslContext()) {
        setErrorAndEmit(QAbstractSocket::SslInternalError,
                        QSslSocket::tr("Unable to init SSL Context: %1").arg(getErrorsFromOpenSsl()));
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    startHandshake();
    transmit();
}

/*!
    \internal

    Transmits encrypted data between the BIOs and the socket.
*/
void QSslSocketBackendPrivate::transmit()
{
    Q_Q(QSslSocket);

    using ScopedBool = QScopedValueRollback<bool>;

    if (inSetAndEmitError)
        return;

    // If we don't have any SSL context, don't bother transmitting.
    if (!ssl)
        return;

    bool transmitting;
    do {
        transmitting = false;

        // If the connection is secure, we can transfer data from the write
        // buffer (in plain text) to the write BIO through SSL_write.
        if (connectionEncrypted && !writeBuffer.isEmpty()) {
            qint64 totalBytesWritten = 0;
            int nextDataBlockSize;
            while ((nextDataBlockSize = writeBuffer.nextDataBlockSize()) > 0) {
                int writtenBytes = q_SSL_write(ssl, writeBuffer.readPointer(), nextDataBlockSize);
                if (writtenBytes <= 0) {
                    int error = q_SSL_get_error(ssl, writtenBytes);
                    //write can result in a want_write_error - not an error - continue transmitting
                    if (error == SSL_ERROR_WANT_WRITE) {
                        transmitting = true;
                        break;
                    } else if (error == SSL_ERROR_WANT_READ) {
                        //write can result in a want_read error, possibly due to renegotiation - not an error - stop transmitting
                        transmitting = false;
                        break;
                    } else {
                        // ### Better error handling.
                        const ScopedBool bg(inSetAndEmitError, true);
                        setErrorAndEmit(QAbstractSocket::SslInternalError,
                                        QSslSocket::tr("Unable to write data: %1").arg(
                                            getErrorsFromOpenSsl()));
                        return;
                    }
                }
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: encrypted" << writtenBytes << "bytes";
#endif
                writeBuffer.free(writtenBytes);
                totalBytesWritten += writtenBytes;

                if (writtenBytes < nextDataBlockSize) {
                    // break out of the writing loop and try again after we had read
                    transmitting = true;
                    break;
                }
            }

            if (totalBytesWritten > 0) {
                // Don't emit bytesWritten() recursively.
                if (!emittedBytesWritten) {
                    emittedBytesWritten = true;
                    emit q->bytesWritten(totalBytesWritten);
                    emittedBytesWritten = false;
                }
                emit q->channelBytesWritten(0, totalBytesWritten);
            }
        }

        // Check if we've got any data to be written to the socket.
        QVarLengthArray<char, 4096> data;
        int pendingBytes;
        while (plainSocket->isValid() && (pendingBytes = q_BIO_pending(writeBio)) > 0
                && plainSocket->openMode() != QIODevice::NotOpen) {
            // Read encrypted data from the write BIO into a buffer.
            data.resize(pendingBytes);
            int encryptedBytesRead = q_BIO_read(writeBio, data.data(), pendingBytes);

            // Write encrypted data from the buffer to the socket.
            qint64 actualWritten = plainSocket->write(data.constData(), encryptedBytesRead);
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: wrote" << encryptedBytesRead << "encrypted bytes to the socket" << actualWritten << "actual.";
#endif
            if (actualWritten < 0) {
                //plain socket write fails if it was in the pending close state.
                const ScopedBool bg(inSetAndEmitError, true);
                setErrorAndEmit(plainSocket->error(), plainSocket->errorString());
                return;
            }
            transmitting = true;
        }

        // Check if we've got any data to be read from the socket.
        if (!connectionEncrypted || !readBufferMaxSize || buffer.size() < readBufferMaxSize)
            while ((pendingBytes = plainSocket->bytesAvailable()) > 0) {
                // Read encrypted data from the socket into a buffer.
                data.resize(pendingBytes);
                // just peek() here because q_BIO_write could write less data than expected
                int encryptedBytesRead = plainSocket->peek(data.data(), pendingBytes);

#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: read" << encryptedBytesRead << "encrypted bytes from the socket";
#endif
                // Write encrypted data from the buffer into the read BIO.
                int writtenToBio = q_BIO_write(readBio, data.constData(), encryptedBytesRead);

                // Throw away the results.
                if (writtenToBio > 0) {
                    plainSocket->skip(writtenToBio);
                } else {
                    // ### Better error handling.
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(QAbstractSocket::SslInternalError,
                                    QSslSocket::tr("Unable to decrypt data: %1").arg(
                                        getErrorsFromOpenSsl()));
                    return;
                }

                transmitting = true;
            }

        // If the connection isn't secured yet, this is the time to retry the
        // connect / accept.
        if (!connectionEncrypted) {
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: testing encryption";
#endif
            if (startHandshake()) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: encryption established";
#endif
                connectionEncrypted = true;
                transmitting = true;
            } else if (plainSocket->state() != QAbstractSocket::ConnectedState) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: connection lost";
#endif
                break;
            } else if (paused) {
                // just wait until the user continues
                return;
            } else {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: encryption not done yet";
#endif
            }
        }

        // If the request is small and the remote host closes the transmission
        // after sending, there's a chance that startHandshake() will already
        // have triggered a shutdown.
        if (!ssl)
            continue;

        // We always read everything from the SSL decryption buffers, even if
        // we have a readBufferMaxSize. There's no point in leaving data there
        // just so that readBuffer.size() == readBufferMaxSize.
        int readBytes = 0;
        const int bytesToRead = 4096;
        do {
            if (readChannelCount == 0) {
                // The read buffer is deallocated, don't try resize or write to it.
                break;
            }
            // Don't use SSL_pending(). It's very unreliable.
            readBytes = q_SSL_read(ssl, buffer.reserve(bytesToRead), bytesToRead);
            if (readBytes > 0) {
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: decrypted" << readBytes << "bytes";
#endif
                buffer.chop(bytesToRead - readBytes);

                if (readyReadEmittedPointer)
                    *readyReadEmittedPointer = true;
                emit q->readyRead();
                emit q->channelReadyRead(0);
                transmitting = true;
                continue;
            }
            buffer.chop(bytesToRead);

            // Error.
            switch (q_SSL_get_error(ssl, readBytes)) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                // Out of data.
                break;
            case SSL_ERROR_ZERO_RETURN:
                // The remote host closed the connection.
#ifdef QSSLSOCKET_DEBUG
                qCDebug(lcSsl) << "QSslSocketBackendPrivate::transmit: remote disconnect";
#endif
                shutdown = true; // the other side shut down, make sure we do not send shutdown ourselves
                {
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(QAbstractSocket::RemoteHostClosedError,
                                    QSslSocket::tr("The TLS/SSL connection has been closed"));
                }
                return;
            case SSL_ERROR_SYSCALL: // some IO error
            case SSL_ERROR_SSL: // error in the SSL library
                // we do not know exactly what the error is, nor whether we can recover from it,
                // so just return to prevent an endless loop in the outer "while" statement
                {
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(QAbstractSocket::SslInternalError,
                                    QSslSocket::tr("Error while reading: %1").arg(getErrorsFromOpenSsl()));
                }
                return;
            default:
                // SSL_ERROR_WANT_CONNECT, SSL_ERROR_WANT_ACCEPT: can only happen with a
                // BIO_s_connect() or BIO_s_accept(), which we do not call.
                // SSL_ERROR_WANT_X509_LOOKUP: can only happen with a
                // SSL_CTX_set_client_cert_cb(), which we do not call.
                // So this default case should never be triggered.
                {
                    const ScopedBool bg(inSetAndEmitError, true);
                    setErrorAndEmit(QAbstractSocket::SslInternalError,
                                    QSslSocket::tr("Error while reading: %1").arg(getErrorsFromOpenSsl()));
                }
                break;
            }
        } while (ssl && readBytes > 0);
    } while (ssl && transmitting);
}

QSslError _q_OpenSSL_to_QSslError(int errorCode, const QSslCertificate &cert)
{
    QSslError error;
    switch (errorCode) {
    case X509_V_OK:
        // X509_V_OK is also reported if the peer had no certificate.
        break;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        error = QSslError(QSslError::UnableToGetIssuerCertificate, cert); break;
    case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
        error = QSslError(QSslError::UnableToDecryptCertificateSignature, cert); break;
    case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
        error = QSslError(QSslError::UnableToDecodeIssuerPublicKey, cert); break;
    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        error = QSslError(QSslError::CertificateSignatureFailed, cert); break;
    case X509_V_ERR_CERT_NOT_YET_VALID:
        error = QSslError(QSslError::CertificateNotYetValid, cert); break;
    case X509_V_ERR_CERT_HAS_EXPIRED:
        error = QSslError(QSslError::CertificateExpired, cert); break;
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        error = QSslError(QSslError::InvalidNotBeforeField, cert); break;
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        error = QSslError(QSslError::InvalidNotAfterField, cert); break;
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        error = QSslError(QSslError::SelfSignedCertificate, cert); break;
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
        error = QSslError(QSslError::SelfSignedCertificateInChain, cert); break;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        error = QSslError(QSslError::UnableToGetLocalIssuerCertificate, cert); break;
    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
        error = QSslError(QSslError::UnableToVerifyFirstCertificate, cert); break;
    case X509_V_ERR_CERT_REVOKED:
        error = QSslError(QSslError::CertificateRevoked, cert); break;
    case X509_V_ERR_INVALID_CA:
        error = QSslError(QSslError::InvalidCaCertificate, cert); break;
    case X509_V_ERR_PATH_LENGTH_EXCEEDED:
        error = QSslError(QSslError::PathLengthExceeded, cert); break;
    case X509_V_ERR_INVALID_PURPOSE:
        error = QSslError(QSslError::InvalidPurpose, cert); break;
    case X509_V_ERR_CERT_UNTRUSTED:
        error = QSslError(QSslError::CertificateUntrusted, cert); break;
    case X509_V_ERR_CERT_REJECTED:
        error = QSslError(QSslError::CertificateRejected, cert); break;
    default:
        error = QSslError(QSslError::UnspecifiedError, cert); break;
    }
    return error;
}

QString QSslSocketBackendPrivate::msgErrorsDuringHandshake()
{
    return QSslSocket::tr("Error during SSL handshake: %1")
                         .arg(QSslSocketBackendPrivate::getErrorsFromOpenSsl());
}

bool QSslSocketBackendPrivate::startHandshake()
{
    Q_Q(QSslSocket);

    // Check if the connection has been established. Get all errors from the
    // verification stage.

    using ScopedBool = QScopedValueRollback<bool>;

    if (inSetAndEmitError)
        return false;

    QMutexLocker locker(&_q_sslErrorList()->mutex);
    _q_sslErrorList()->errors.clear();
    int result = (mode == QSslSocket::SslClientMode) ? q_SSL_connect(ssl) : q_SSL_accept(ssl);

    const auto &lastErrors = _q_sslErrorList()->errors;
    if (!lastErrors.isEmpty())
        storePeerCertificates();
    for (const auto &currentError : lastErrors) {
        emit q->peerVerifyError(_q_OpenSSL_to_QSslError(currentError.code,
                                configuration.peerCertificateChain.value(currentError.depth)));
        if (q->state() != QAbstractSocket::ConnectedState)
            break;
    }

    errorList << lastErrors;
    locker.unlock();

    // Connection aborted during handshake phase.
    if (q->state() != QAbstractSocket::ConnectedState)
        return false;

    // Check if we're encrypted or not.
    if (result <= 0) {
        switch (q_SSL_get_error(ssl, result)) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // The handshake is not yet complete.
            break;
        default:
            QString errorString = QSslSocketBackendPrivate::msgErrorsDuringHandshake();
#ifdef QSSLSOCKET_DEBUG
            qCDebug(lcSsl) << "QSslSocketBackendPrivate::startHandshake: error!" << errorString;
#endif
            {
                const ScopedBool bg(inSetAndEmitError, true);
                setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError, errorString);
            }
            q->abort();
        }
        return false;
    }

    // store peer certificate chain
    storePeerCertificates();

    // Start translating errors.
    QList<QSslError> errors;

    // check the whole chain for blacklisting (including root, as we check for subjectInfo and issuer)
    for (const QSslCertificate &cert : qAsConst(configuration.peerCertificateChain)) {
        if (QSslCertificatePrivate::isBlacklisted(cert)) {
            QSslError error(QSslError::CertificateBlacklisted, cert);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    bool doVerifyPeer = configuration.peerVerifyMode == QSslSocket::VerifyPeer
                        || (configuration.peerVerifyMode == QSslSocket::AutoVerifyPeer
                            && mode == QSslSocket::SslClientMode);

    // Check the peer certificate itself. First try the subject's common name
    // (CN) as a wildcard, then try all alternate subject name DNS entries the
    // same way.
    if (!configuration.peerCertificate.isNull()) {
        // but only if we're a client connecting to a server
        // if we're the server, don't check CN
        if (mode == QSslSocket::SslClientMode) {
            QString peerName = (verificationPeerName.isEmpty () ? q->peerName() : verificationPeerName);

            if (!isMatchingHostname(configuration.peerCertificate, peerName)) {
                // No matches in common names or alternate names.
                QSslError error(QSslError::HostNameMismatch, configuration.peerCertificate);
                errors << error;
                emit q->peerVerifyError(error);
                if (q->state() != QAbstractSocket::ConnectedState)
                    return false;
            }
        }
    } else {
        // No peer certificate presented. Report as error if the socket
        // expected one.
        if (doVerifyPeer) {
            QSslError error(QSslError::NoPeerCertificate);
            errors << error;
            emit q->peerVerifyError(error);
            if (q->state() != QAbstractSocket::ConnectedState)
                return false;
        }
    }

    // Translate errors from the error list into QSslErrors.
    errors.reserve(errors.size() + errorList.size());
    for (const auto &error : qAsConst(errorList))
        errors << _q_OpenSSL_to_QSslError(error.code, configuration.peerCertificateChain.value(error.depth));

    if (!errors.isEmpty()) {
        sslErrors = errors;

#ifdef Q_OS_WIN
        //Skip this if not using system CAs, or if the SSL errors are configured in advance to be ignorable
        if (doVerifyPeer
            && s_loadRootCertsOnDemand
            && allowRootCertOnDemandLoading
            && !verifyErrorsHaveBeenIgnored()) {
            //Windows desktop versions starting from vista ship with minimal set of roots
            //and download on demand from the windows update server CA roots that are
            //trusted by MS.
            //However, this is only transparent if using WinINET - we have to trigger it
            //ourselves.
            QSslCertificate certToFetch;
            bool fetchCertificate = true;
            for (int i=0; i< sslErrors.count(); i++) {
                switch (sslErrors.at(i).error()) {
                case QSslError::UnableToGetLocalIssuerCertificate: // site presented intermediate cert, but root is unknown
                case QSslError::SelfSignedCertificateInChain: // site presented a complete chain, but root is unknown
                    certToFetch = sslErrors.at(i).certificate();
                    break;
                case QSslError::SelfSignedCertificate:
                case QSslError::CertificateBlacklisted:
                    //With these errors, we know it will be untrusted so save time by not asking windows
                    fetchCertificate = false;
                    break;
                default:
#ifdef QSSLSOCKET_DEBUG
                    qCDebug(lcSsl) << sslErrors.at(i).errorString();
#endif
                    break;
                }
            }
            if (fetchCertificate && !certToFetch.isNull()) {
                fetchCaRootForCert(certToFetch);
                return false;
            }
        }
#endif
        if (!checkSslErrors())
            return false;
        // A slot, attached to sslErrors signal can call
        // abort/close/disconnetFromHost/etc; no need to
        // continue handshake then.
        if (q->state() != QAbstractSocket::ConnectedState)
            return false;
    } else {
        sslErrors.clear();
    }

    continueHandshake();
    return true;
}

void QSslSocketBackendPrivate::storePeerCertificates()
{
    // Store the peer certificate and chain. For clients, the peer certificate
    // chain includes the peer certificate; for servers, it doesn't. Both the
    // peer certificate and the chain may be empty if the peer didn't present
    // any certificate.
    X509 *x509 = q_SSL_get_peer_certificate(ssl);
    configuration.peerCertificate = QSslCertificatePrivate::QSslCertificate_from_X509(x509);
    q_X509_free(x509);
    if (configuration.peerCertificateChain.isEmpty()) {
        configuration.peerCertificateChain = STACKOFX509_to_QSslCertificates(q_SSL_get_peer_cert_chain(ssl));
        if (!configuration.peerCertificate.isNull() && mode == QSslSocket::SslServerMode)
            configuration.peerCertificateChain.prepend(configuration.peerCertificate);
    }
}

bool QSslSocketBackendPrivate::checkSslErrors()
{
    Q_Q(QSslSocket);
    if (sslErrors.isEmpty())
        return true;

    emit q->sslErrors(sslErrors);

    bool doVerifyPeer = configuration.peerVerifyMode == QSslSocket::VerifyPeer
                        || (configuration.peerVerifyMode == QSslSocket::AutoVerifyPeer
                            && mode == QSslSocket::SslClientMode);
    bool doEmitSslError = !verifyErrorsHaveBeenIgnored();
    // check whether we need to emit an SSL handshake error
    if (doVerifyPeer && doEmitSslError) {
        if (q->pauseMode() & QAbstractSocket::PauseOnSslErrors) {
            pauseSocketNotifiers(q);
            paused = true;
        } else {
            setErrorAndEmit(QAbstractSocket::SslHandshakeFailedError, sslErrors.constFirst().errorString());
            plainSocket->disconnectFromHost();
        }
        return false;
    }
    return true;
}

unsigned int QSslSocketBackendPrivate::tlsPskClientCallback(const char *hint,
                                                            char *identity, unsigned int max_identity_len,
                                                            unsigned char *psk, unsigned int max_psk_len)
{
    QSslPreSharedKeyAuthenticator authenticator;

    // Fill in some read-only fields (for the user)
    if (hint)
        authenticator.d->identityHint = QByteArray::fromRawData(hint, int(::strlen(hint))); // it's NUL terminated, but do not include the NUL

    authenticator.d->maximumIdentityLength = int(max_identity_len) - 1; // needs to be NUL terminated
    authenticator.d->maximumPreSharedKeyLength = int(max_psk_len);

    // Let the client provide the remaining bits...
    Q_Q(QSslSocket);
    emit q->preSharedKeyAuthenticationRequired(&authenticator);

    // No PSK set? Return now to make the handshake fail
    if (authenticator.preSharedKey().isEmpty())
        return 0;

    // Copy data back into OpenSSL
    const int identityLength = qMin(authenticator.identity().length(), authenticator.maximumIdentityLength());
    ::memcpy(identity, authenticator.identity().constData(), identityLength);
    identity[identityLength] = 0;

    const int pskLength = qMin(authenticator.preSharedKey().length(), authenticator.maximumPreSharedKeyLength());
    ::memcpy(psk, authenticator.preSharedKey().constData(), pskLength);
    return pskLength;
}

unsigned int QSslSocketBackendPrivate::tlsPskServerCallback(const char *identity,
                                                            unsigned char *psk, unsigned int max_psk_len)
{
    QSslPreSharedKeyAuthenticator authenticator;

    // Fill in some read-only fields (for the user)
    authenticator.d->identityHint = configuration.preSharedKeyIdentityHint;
    authenticator.d->identity = identity;
    authenticator.d->maximumIdentityLength = 0; // user cannot set an identity
    authenticator.d->maximumPreSharedKeyLength = int(max_psk_len);

    // Let the client provide the remaining bits...
    Q_Q(QSslSocket);
    emit q->preSharedKeyAuthenticationRequired(&authenticator);

    // No PSK set? Return now to make the handshake fail
    if (authenticator.preSharedKey().isEmpty())
        return 0;

    // Copy data back into OpenSSL
    const int pskLength = qMin(authenticator.preSharedKey().length(), authenticator.maximumPreSharedKeyLength());
    ::memcpy(psk, authenticator.preSharedKey().constData(), pskLength);
    return pskLength;
}

#ifdef Q_OS_WIN

void QSslSocketBackendPrivate::fetchCaRootForCert(const QSslCertificate &cert)
{
    Q_Q(QSslSocket);
    //The root certificate is downloaded from windows update, which blocks for 15 seconds in the worst case
    //so the request is done in a worker thread.
    QWindowsCaRootFetcher *fetcher = new QWindowsCaRootFetcher(cert, mode);
    QObject::connect(fetcher, SIGNAL(finished(QSslCertificate,QSslCertificate)), q, SLOT(_q_caRootLoaded(QSslCertificate,QSslCertificate)), Qt::QueuedConnection);
    QMetaObject::invokeMethod(fetcher, "start", Qt::QueuedConnection);
    pauseSocketNotifiers(q);
    paused = true;
}

//This is the callback from QWindowsCaRootFetcher, trustedRoot will be invalid (default constructed) if it failed.
void QSslSocketBackendPrivate::_q_caRootLoaded(QSslCertificate cert, QSslCertificate trustedRoot)
{
    Q_Q(QSslSocket);
    if (!trustedRoot.isNull() && !trustedRoot.isBlacklisted()) {
        if (s_loadRootCertsOnDemand) {
            //Add the new root cert to default cert list for use by future sockets
            QSslSocket::addDefaultCaCertificate(trustedRoot);
        }
        //Add the new root cert to this socket for future connections
        q->addCaCertificate(trustedRoot);
        //Remove the broken chain ssl errors (as chain is verified by windows)
        for (int i=sslErrors.count() - 1; i >= 0; --i) {
            if (sslErrors.at(i).certificate() == cert) {
                switch (sslErrors.at(i).error()) {
                case QSslError::UnableToGetLocalIssuerCertificate:
                case QSslError::CertificateUntrusted:
                case QSslError::UnableToVerifyFirstCertificate:
                case QSslError::SelfSignedCertificateInChain:
                    // error can be ignored if OS says the chain is trusted
                    sslErrors.removeAt(i);
                    break;
                default:
                    // error cannot be ignored
                    break;
                }
            }
        }
    }
    // Continue with remaining errors
    if (plainSocket)
        plainSocket->resume();
    paused = false;
    if (checkSslErrors() && ssl) {
        bool willClose = (autoStartHandshake && pendingClose);
        continueHandshake();
        if (!willClose)
            transmit();
    }
}

#endif

void QSslSocketBackendPrivate::disconnectFromHost()
{
    if (ssl) {
        if (!shutdown) {
            q_SSL_shutdown(ssl);
            shutdown = true;
            transmit();
        }
    }
    plainSocket->disconnectFromHost();
}

void QSslSocketBackendPrivate::disconnected()
{
    if (plainSocket->bytesAvailable() <= 0)
        destroySslContext();
    else {
        // Move all bytes into the plain buffer
        qint64 tmpReadBufferMaxSize = readBufferMaxSize;
        readBufferMaxSize = 0; // reset temporarily so the plain socket buffer is completely drained
        transmit();
        readBufferMaxSize = tmpReadBufferMaxSize;
    }
    //if there is still buffered data in the plain socket, don't destroy the ssl context yet.
    //it will be destroyed when the socket is deleted.
}

QSslCipher QSslSocketBackendPrivate::sessionCipher() const
{
    if (!ssl)
        return QSslCipher();

    const SSL_CIPHER *sessionCipher = q_SSL_get_current_cipher(ssl);
    return sessionCipher ? QSslCipher_from_SSL_CIPHER(sessionCipher) : QSslCipher();
}

QSsl::SslProtocol QSslSocketBackendPrivate::sessionProtocol() const
{
    if (!ssl)
        return QSsl::UnknownProtocol;
    int ver = q_SSL_version(ssl);

    switch (ver) {
    case 0x2:
        return QSsl::SslV2;
    case 0x300:
        return QSsl::SslV3;
    case 0x301:
        return QSsl::TlsV1_0;
    case 0x302:
        return QSsl::TlsV1_1;
    case 0x303:
        return QSsl::TlsV1_2;
    }

    return QSsl::UnknownProtocol;
}

QList<QSslCertificate> QSslSocketBackendPrivate::STACKOFX509_to_QSslCertificates(STACK_OF(X509) *x509)
{
    ensureInitialized();
    QList<QSslCertificate> certificates;
    for (int i = 0; i < q_sk_X509_num(x509); ++i) {
        if (X509 *entry = q_sk_X509_value(x509, i))
            certificates << QSslCertificatePrivate::QSslCertificate_from_X509(entry);
    }
    return certificates;
}

QList<QSslError> QSslSocketBackendPrivate::verify(const QList<QSslCertificate> &certificateChain, const QString &hostName)
{
    QList<QSslError> errors;
    if (certificateChain.count() <= 0) {
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    // Setup the store with the default CA certificates
    X509_STORE *certStore = q_X509_STORE_new();
    if (!certStore) {
        qCWarning(lcSsl) << "Unable to create certificate store";
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    if (s_loadRootCertsOnDemand) {
        setDefaultCaCertificates(defaultCaCertificates() + systemCaCertificates());
    }

    const QDateTime now = QDateTime::currentDateTimeUtc();
    const auto caCertificates = QSslConfiguration::defaultConfiguration().caCertificates();
    for (const QSslCertificate &caCertificate : caCertificates) {
        // From https://www.openssl.org/docs/ssl/SSL_CTX_load_verify_locations.html:
        //
        // If several CA certificates matching the name, key identifier, and
        // serial number condition are available, only the first one will be
        // examined. This may lead to unexpected results if the same CA
        // certificate is available with different expiration dates. If a
        // ``certificate expired'' verification error occurs, no other
        // certificate will be searched. Make sure to not have expired
        // certificates mixed with valid ones.
        //
        // See also: QSslContext::fromConfiguration()
        if (caCertificate.expiryDate() >= now) {
            q_X509_STORE_add_cert(certStore, reinterpret_cast<X509 *>(caCertificate.handle()));
        }
    }

    QMutexLocker sslErrorListMutexLocker(&_q_sslErrorList()->mutex);

    // Register a custom callback to get all verification errors.
    q_X509_STORE_set_verify_cb(certStore, q_X509Callback);

    // Build the chain of intermediate certificates
    STACK_OF(X509) *intermediates = nullptr;
    if (certificateChain.length() > 1) {
        intermediates = (STACK_OF(X509) *) q_OPENSSL_sk_new_null();

        if (!intermediates) {
            q_X509_STORE_free(certStore);
            errors << QSslError(QSslError::UnspecifiedError);
            return errors;
        }

        bool first = true;
        for (const QSslCertificate &cert : certificateChain) {
            if (first) {
                first = false;
                continue;
            }

            q_OPENSSL_sk_push((OPENSSL_STACK *)intermediates, reinterpret_cast<X509 *>(cert.handle()));
        }
    }

    X509_STORE_CTX *storeContext = q_X509_STORE_CTX_new();
    if (!storeContext) {
        q_X509_STORE_free(certStore);
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    if (!q_X509_STORE_CTX_init(storeContext, certStore, reinterpret_cast<X509 *>(certificateChain[0].handle()), intermediates)) {
        q_X509_STORE_CTX_free(storeContext);
        q_X509_STORE_free(certStore);
        errors << QSslError(QSslError::UnspecifiedError);
        return errors;
    }

    // Now we can actually perform the verification of the chain we have built.
    // We ignore the result of this function since we process errors via the
    // callback.
    (void) q_X509_verify_cert(storeContext);

    q_X509_STORE_CTX_free(storeContext);
    q_OPENSSL_sk_free((OPENSSL_STACK *)intermediates);

    // Now process the errors
    const auto errorList = std::move(_q_sslErrorList()->errors);
    _q_sslErrorList()->errors.clear();

    sslErrorListMutexLocker.unlock();

    // Translate the errors
    if (QSslCertificatePrivate::isBlacklisted(certificateChain[0])) {
        QSslError error(QSslError::CertificateBlacklisted, certificateChain[0]);
        errors << error;
    }

    // Check the certificate name against the hostname if one was specified
    if ((!hostName.isEmpty()) && (!isMatchingHostname(certificateChain[0], hostName))) {
        // No matches in common names or alternate names.
        QSslError error(QSslError::HostNameMismatch, certificateChain[0]);
        errors << error;
    }

    // Translate errors from the error list into QSslErrors.
    errors.reserve(errors.size() + errorList.size());
    for (const auto &error : qAsConst(errorList))
        errors << _q_OpenSSL_to_QSslError(error.code, certificateChain.value(error.depth));

    q_X509_STORE_free(certStore);

    return errors;
}

bool QSslSocketBackendPrivate::importPkcs12(QIODevice *device,
                                            QSslKey *key, QSslCertificate *cert,
                                            QList<QSslCertificate> *caCertificates,
                                            const QByteArray &passPhrase)
{
    if (!supportsSsl())
        return false;

    // These are required
    Q_ASSERT(device);
    Q_ASSERT(key);
    Q_ASSERT(cert);

    // Read the file into a BIO
    QByteArray pkcs12data = device->readAll();
    if (pkcs12data.size() == 0)
        return false;

    BIO *bio = q_BIO_new_mem_buf(const_cast<char *>(pkcs12data.constData()), pkcs12data.size());

    // Create the PKCS#12 object
    PKCS12 *p12 = q_d2i_PKCS12_bio(bio, nullptr);
    if (!p12) {
        qCWarning(lcSsl, "Unable to read PKCS#12 structure, %s",
                  q_ERR_error_string(q_ERR_get_error(), nullptr));
        q_BIO_free(bio);
        return false;
    }

    // Extract the data
    EVP_PKEY *pkey = nullptr;
    X509 *x509;
    STACK_OF(X509) *ca = nullptr;

    if (!q_PKCS12_parse(p12, passPhrase.constData(), &pkey, &x509, &ca)) {
        qCWarning(lcSsl, "Unable to parse PKCS#12 structure, %s",
                  q_ERR_error_string(q_ERR_get_error(), nullptr));
        q_PKCS12_free(p12);
        q_BIO_free(bio);
        return false;
    }

    // Convert to Qt types
    if (!key->d->fromEVP_PKEY(pkey)) {
        qCWarning(lcSsl, "Unable to convert private key");
        q_OPENSSL_sk_pop_free(reinterpret_cast<OPENSSL_STACK *>(ca),
                              reinterpret_cast<void (*)(void *)>(q_X509_free));
        q_X509_free(x509);
        q_EVP_PKEY_free(pkey);
        q_PKCS12_free(p12);
        q_BIO_free(bio);

        return false;
    }

    *cert = QSslCertificatePrivate::QSslCertificate_from_X509(x509);

    if (caCertificates)
        *caCertificates = QSslSocketBackendPrivate::STACKOFX509_to_QSslCertificates(ca);

    // Clean up
    q_OPENSSL_sk_pop_free(reinterpret_cast<OPENSSL_STACK *>(ca),
                          reinterpret_cast<void (*)(void *)>(q_X509_free));

    q_X509_free(x509);
    q_EVP_PKEY_free(pkey);
    q_PKCS12_free(p12);
    q_BIO_free(bio);

    return true;
}


QT_END_NAMESPACE
