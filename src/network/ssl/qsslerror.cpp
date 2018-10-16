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


/*!
    \class QSslError
    \brief The QSslError class provides an SSL error.
    \since 4.3

    \reentrant
    \ingroup network
    \ingroup ssl
    \ingroup shared
    \inmodule QtNetwork

    QSslError provides a simple API for managing errors during QSslSocket's
    SSL handshake.

    \sa QSslSocket, QSslCertificate, QSslCipher
*/

/*!
    \enum QSslError::SslError

    Describes all recognized errors that can occur during an SSL handshake.

    \value NoError
    \value UnableToGetIssuerCertificate
    \value UnableToDecryptCertificateSignature
    \value UnableToDecodeIssuerPublicKey
    \value CertificateSignatureFailed
    \value CertificateNotYetValid
    \value CertificateExpired
    \value InvalidNotBeforeField
    \value InvalidNotAfterField
    \value SelfSignedCertificate
    \value SelfSignedCertificateInChain
    \value UnableToGetLocalIssuerCertificate
    \value UnableToVerifyFirstCertificate
    \value CertificateRevoked
    \value InvalidCaCertificate
    \value PathLengthExceeded
    \value InvalidPurpose
    \value CertificateUntrusted
    \value CertificateRejected
    \value SubjectIssuerMismatch
    \value AuthorityIssuerSerialNumberMismatch
    \value NoPeerCertificate
    \value HostNameMismatch
    \value UnspecifiedError
    \value NoSslSupport
    \value CertificateBlacklisted

    \sa QSslError::errorString()
*/

#include "qsslerror.h"
#include "qsslsocket.h"
#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
#endif

QT_BEGIN_NAMESPACE

class QSslErrorPrivate
{
public:
    QSslError::SslError error;
    QSslCertificate certificate;
};

// RVCT compiler in debug build does not like about default values in const-
// So as an workaround we define all constructor overloads here explicitly
/*!
    Constructs a QSslError object with no error and default certificate.

*/

QSslError::QSslError()
    : d(new QSslErrorPrivate)
{
    d->error = QSslError::NoError;
    d->certificate = QSslCertificate();
}

/*!
    Constructs a QSslError object. The argument specifies the \a
    error that occurred.

*/
QSslError::QSslError(SslError error)
    : d(new QSslErrorPrivate)
{
    d->error = error;
    d->certificate = QSslCertificate();
}

/*!
    Constructs a QSslError object. The two arguments specify the \a
    error that occurred, and which \a certificate the error relates to.

    \sa QSslCertificate
*/
QSslError::QSslError(SslError error, const QSslCertificate &certificate)
    : d(new QSslErrorPrivate)
{
    d->error = error;
    d->certificate = certificate;
}

/*!
    Constructs an identical copy of \a other.
*/
QSslError::QSslError(const QSslError &other)
    : d(new QSslErrorPrivate)
{
    *d.data() = *other.d.data();
}

/*!
    Destroys the QSslError object.
*/
QSslError::~QSslError()
{
}

/*!
    \since 4.4

    Assigns the contents of \a other to this error.
*/
QSslError &QSslError::operator=(const QSslError &other)
{
    *d.data() = *other.d.data();
    return *this;
}

/*!
    \fn void QSslError::swap(QSslError &other)
    \since 5.0

    Swaps this error instance with \a other. This function is very
    fast and never fails.
*/

/*!
    \since 4.4

    Returns \c true if this error is equal to \a other; otherwise returns \c false.
*/
bool QSslError::operator==(const QSslError &other) const
{
    return d->error == other.d->error
        && d->certificate == other.d->certificate;
}

/*!
    \fn bool QSslError::operator!=(const QSslError &other) const
    \since 4.4

    Returns \c true if this error is not equal to \a other; otherwise returns
    false.
*/

/*!
    Returns the type of the error.

    \sa errorString(), certificate()
*/
QSslError::SslError QSslError::error() const
{
    return d->error;
}

/*!
    Returns a short localized human-readable description of the error.

    \sa error(), certificate()
*/
QString QSslError::errorString() const
{
    QString errStr;
    switch (d->error) {
    case NoError:
        errStr = QSslSocket::tr("No error");
        break;
    case UnableToGetIssuerCertificate:
        errStr = QSslSocket::tr("The issuer certificate could not be found");
        break;
    case UnableToDecryptCertificateSignature:
        errStr = QSslSocket::tr("The certificate signature could not be decrypted");
        break;
    case UnableToDecodeIssuerPublicKey:
        errStr = QSslSocket::tr("The public key in the certificate could not be read");
        break;
    case CertificateSignatureFailed:
        errStr = QSslSocket::tr("The signature of the certificate is invalid");
        break;
    case CertificateNotYetValid:
        errStr = QSslSocket::tr("The certificate is not yet valid");
        break;
    case CertificateExpired:
        errStr = QSslSocket::tr("The certificate has expired");
        break;
    case InvalidNotBeforeField:
        errStr = QSslSocket::tr("The certificate's notBefore field contains an invalid time");
        break;
    case InvalidNotAfterField:
        errStr = QSslSocket::tr("The certificate's notAfter field contains an invalid time");
        break;
    case SelfSignedCertificate:
        errStr = QSslSocket::tr("The certificate is self-signed, and untrusted");
        break;
    case SelfSignedCertificateInChain:
        errStr = QSslSocket::tr("The root certificate of the certificate chain is self-signed, and untrusted");
        break;
    case UnableToGetLocalIssuerCertificate:
        errStr = QSslSocket::tr("The issuer certificate of a locally looked up certificate could not be found");
        break;
    case UnableToVerifyFirstCertificate:
        errStr = QSslSocket::tr("No certificates could be verified");
        break;
    case InvalidCaCertificate:
        errStr = QSslSocket::tr("One of the CA certificates is invalid");
        break;
    case PathLengthExceeded:
        errStr = QSslSocket::tr("The basicConstraints path length parameter has been exceeded");
        break;
    case InvalidPurpose:
        errStr = QSslSocket::tr("The supplied certificate is unsuitable for this purpose");
        break;
    case CertificateUntrusted:
        errStr = QSslSocket::tr("The root CA certificate is not trusted for this purpose");
        break;
    case CertificateRejected:
        errStr = QSslSocket::tr("The root CA certificate is marked to reject the specified purpose");
        break;
    case SubjectIssuerMismatch: // hostname mismatch
        errStr = QSslSocket::tr("The current candidate issuer certificate was rejected because its"
                                " subject name did not match the issuer name of the current certificate");
        break;
    case AuthorityIssuerSerialNumberMismatch:
        errStr = QSslSocket::tr("The current candidate issuer certificate was rejected because"
                             " its issuer name and serial number was present and did not match the"
                             " authority key identifier of the current certificate");
        break;
    case NoPeerCertificate:
        errStr = QSslSocket::tr("The peer did not present any certificate");
        break;
    case HostNameMismatch:
        errStr = QSslSocket::tr("The host name did not match any of the valid hosts"
                             " for this certificate");
        break;
    case NoSslSupport:
        break;
    case CertificateBlacklisted:
        errStr = QSslSocket::tr("The peer certificate is blacklisted");
        break;
    default:
        errStr = QSslSocket::tr("Unknown error");
        break;
    }

    return errStr;
}

/*!
    Returns the certificate associated with this error, or a null certificate
    if the error does not relate to any certificate.

    \sa error(), errorString()
*/
QSslCertificate QSslError::certificate() const
{
    return d->certificate;
}

/*!
    Returns the hash value for the \a key, using \a seed to seed the calculation.
    \since 5.4
    \relates QHash
*/
uint qHash(const QSslError &key, uint seed) Q_DECL_NOTHROW
{
    // 2x boost::hash_combine inlined:
    seed ^= qHash(key.error())       + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= qHash(key.certificate()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

#ifndef QT_NO_DEBUG_STREAM
//class QDebug;
QDebug operator<<(QDebug debug, const QSslError &error)
{
    debug << error.errorString();
    return debug;
}
QDebug operator<<(QDebug debug, const QSslError::SslError &error)
{
    debug << QSslError(error).errorString();
    return debug;
}
#endif

QT_END_NAMESPACE
