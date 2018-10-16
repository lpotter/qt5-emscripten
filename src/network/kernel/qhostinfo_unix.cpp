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

//#define QHOSTINFO_DEBUG

#include "qplatformdefs.h"

#include "qhostinfo_p.h"
#include "private/qnativesocketengine_p.h"
#include "qiodevice.h"
#include <qbytearray.h>
#if QT_CONFIG(library)
#include <qlibrary.h>
#endif
#include <qbasicatomic.h>
#include <qurl.h>
#include <qfile.h>
#include <private/qnet_unix_p.h>

#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#if defined(Q_OS_VXWORKS)
#  include <hostLib.h>
#else
#  include <resolv.h>
#endif

#if defined(__GNU_LIBRARY__) && !defined(__UCLIBC__)
#  include <gnu/lib-names.h>
#endif

QT_BEGIN_NAMESPACE

// Almost always the same. If not, specify in qplatformdefs.h.
#if !defined(QT_SOCKOPTLEN_T)
# define QT_SOCKOPTLEN_T QT_SOCKLEN_T
#endif

// HP-UXi has a bug in getaddrinfo(3) that makes it thread-unsafe
// with this flag. So disable it in that platform.
#if defined(AI_ADDRCONFIG) && !defined(Q_OS_HPUX)
#  define Q_ADDRCONFIG          AI_ADDRCONFIG
#endif

enum LibResolvFeature {
    NeedResInit,
    NeedResNInit
};

typedef struct __res_state *res_state_ptr;

typedef int (*res_init_proto)(void);
static res_init_proto local_res_init = 0;
typedef int (*res_ninit_proto)(res_state_ptr);
static res_ninit_proto local_res_ninit = 0;
typedef void (*res_nclose_proto)(res_state_ptr);
static res_nclose_proto local_res_nclose = 0;
static res_state_ptr local_res = 0;

#if QT_CONFIG(library) && !defined(Q_OS_QNX)
namespace {
struct LibResolv
{
    enum {
#ifdef RES_NORELOAD
        // If RES_NORELOAD is defined, then the libc is capable of watching
        // /etc/resolv.conf for changes and reloading as necessary. So accept
        // whatever is configured.
        ReinitNecessary = false
#else
        ReinitNecessary = true
#endif
    };

    QLibrary lib;
    LibResolv();
    ~LibResolv() { lib.unload(); }
};
}

LibResolv::LibResolv()
{
    QLibrary lib;
#ifdef LIBRESOLV_SO
    lib.setFileName(QStringLiteral(LIBRESOLV_SO));
    if (!lib.load())
#endif
    {
        lib.setFileName(QLatin1String("resolv"));
        if (!lib.load())
            return;
    }

    // res_ninit is required for localDomainName()
    local_res_ninit = res_ninit_proto(lib.resolve("__res_ninit"));
    if (!local_res_ninit)
        local_res_ninit = res_ninit_proto(lib.resolve("res_ninit"));
    if (local_res_ninit) {
        // we must now find res_nclose
        local_res_nclose = res_nclose_proto(lib.resolve("res_nclose"));
        if (!local_res_nclose)
            local_res_nclose = res_nclose_proto(lib.resolve("__res_nclose"));
        if (!local_res_nclose)
            local_res_ninit = nullptr;
    }

    if (ReinitNecessary || !local_res_ninit) {
        local_res_init = res_init_proto(lib.resolve("__res_init"));
        if (!local_res_init)
            local_res_init = res_init_proto(lib.resolve("res_init"));

        if (local_res_init && !local_res_ninit) {
            // if we can't get a thread-safe context, we have to use the global _res state
            local_res = res_state_ptr(lib.resolve("_res"));
        }
    }
}
Q_GLOBAL_STATIC(LibResolv, libResolv)

static void resolveLibrary(LibResolvFeature f)
{
    if (LibResolv::ReinitNecessary || f == NeedResNInit)
        libResolv();
}
#else // QT_CONFIG(library) || Q_OS_QNX
static void resolveLibrary(LibResolvFeature)
{
}
#endif // QT_CONFIG(library) || Q_OS_QNX

QHostInfo QHostInfoAgent::fromName(const QString &hostName)
{
    QHostInfo results;

#if defined(QHOSTINFO_DEBUG)
    qDebug("QHostInfoAgent::fromName(%s) looking up...",
           hostName.toLatin1().constData());
#endif

    // Load res_init on demand.
    resolveLibrary(NeedResInit);

    // If res_init is available, poll it.
    if (local_res_init)
        local_res_init();

    QHostAddress address;
    if (address.setAddress(hostName)) {
        // Reverse lookup
        sockaddr_in sa4;
        sockaddr_in6 sa6;
        sockaddr *sa = 0;
        QT_SOCKLEN_T saSize = 0;
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            sa = (sockaddr *)&sa4;
            saSize = sizeof(sa4);
            memset(&sa4, 0, sizeof(sa4));
            sa4.sin_family = AF_INET;
            sa4.sin_addr.s_addr = htonl(address.toIPv4Address());
        }
        else {
            sa = (sockaddr *)&sa6;
            saSize = sizeof(sa6);
            memset(&sa6, 0, sizeof(sa6));
            sa6.sin6_family = AF_INET6;
            memcpy(sa6.sin6_addr.s6_addr, address.toIPv6Address().c, sizeof(sa6.sin6_addr.s6_addr));
        }

        char hbuf[NI_MAXHOST];
        if (sa && getnameinfo(sa, saSize, hbuf, sizeof(hbuf), 0, 0, 0) == 0)
            results.setHostName(QString::fromLatin1(hbuf));

        if (results.hostName().isEmpty())
            results.setHostName(address.toString());
        results.setAddresses(QList<QHostAddress>() << address);
        return results;
    }

    // IDN support
    QByteArray aceHostname = QUrl::toAce(hostName);
    results.setHostName(hostName);
    if (aceHostname.isEmpty()) {
        results.setError(QHostInfo::HostNotFound);
        results.setErrorString(hostName.isEmpty() ?
                               QCoreApplication::translate("QHostInfoAgent", "No host name given") :
                               QCoreApplication::translate("QHostInfoAgent", "Invalid hostname"));
        return results;
    }

    // Call getaddrinfo, and place all IPv4 addresses at the start and
    // the IPv6 addresses at the end of the address list in results.
    addrinfo *res = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
#ifdef Q_ADDRCONFIG
    hints.ai_flags = Q_ADDRCONFIG;
#endif

    int result = getaddrinfo(aceHostname.constData(), 0, &hints, &res);
# ifdef Q_ADDRCONFIG
    if (result == EAI_BADFLAGS) {
        // if the lookup failed with AI_ADDRCONFIG set, try again without it
        hints.ai_flags = 0;
        result = getaddrinfo(aceHostname.constData(), 0, &hints, &res);
    }
# endif

    if (result == 0) {
        addrinfo *node = res;
        QList<QHostAddress> addresses;
        while (node) {
#ifdef QHOSTINFO_DEBUG
                qDebug() << "getaddrinfo node: flags:" << node->ai_flags << "family:" << node->ai_family << "ai_socktype:" << node->ai_socktype << "ai_protocol:" << node->ai_protocol << "ai_addrlen:" << node->ai_addrlen;
#endif
            if (node->ai_family == AF_INET) {
                QHostAddress addr;
                addr.setAddress(ntohl(((sockaddr_in *) node->ai_addr)->sin_addr.s_addr));
                if (!addresses.contains(addr))
                    addresses.append(addr);
            }
            else if (node->ai_family == AF_INET6) {
                QHostAddress addr;
                sockaddr_in6 *sa6 = (sockaddr_in6 *) node->ai_addr;
                addr.setAddress(sa6->sin6_addr.s6_addr);
                if (sa6->sin6_scope_id)
                    addr.setScopeId(QString::number(sa6->sin6_scope_id));
                if (!addresses.contains(addr))
                    addresses.append(addr);
            }
            node = node->ai_next;
        }
        if (addresses.isEmpty() && node == 0) {
            // Reached the end of the list, but no addresses were found; this
            // means the list contains one or more unknown address types.
            results.setError(QHostInfo::UnknownError);
            results.setErrorString(tr("Unknown address type"));
        }

        results.setAddresses(addresses);
        freeaddrinfo(res);
    } else if (result == EAI_NONAME
               || result ==  EAI_FAIL
#ifdef EAI_NODATA
               // EAI_NODATA is deprecated in RFC 3493
               || result == EAI_NODATA
#endif
               ) {
        results.setError(QHostInfo::HostNotFound);
        results.setErrorString(tr("Host not found"));
    } else {
        results.setError(QHostInfo::UnknownError);
        results.setErrorString(QString::fromLocal8Bit(gai_strerror(result)));
    }


#if defined(QHOSTINFO_DEBUG)
    if (results.error() != QHostInfo::NoError) {
        qDebug("QHostInfoAgent::fromName(): error #%d %s",
               h_errno, results.errorString().toLatin1().constData());
    } else {
        QString tmp;
        QList<QHostAddress> addresses = results.addresses();
        for (int i = 0; i < addresses.count(); ++i) {
            if (i != 0) tmp += ", ";
            tmp += addresses.at(i).toString();
        }
        qDebug("QHostInfoAgent::fromName(): found %i entries for \"%s\": {%s}",
               addresses.count(), hostName.toLatin1().constData(),
               tmp.toLatin1().constData());
    }
#endif
    return results;
}

QString QHostInfo::localDomainName()
{
#if !defined(Q_OS_VXWORKS) && !defined(Q_OS_ANDROID)
    resolveLibrary(NeedResNInit);
    if (local_res_ninit) {
        // using thread-safe version
        res_state_ptr state = res_state_ptr(malloc(sizeof(*state)));
        Q_CHECK_PTR(state);
        memset(state, 0, sizeof(*state));
        local_res_ninit(state);
        QString domainName = QUrl::fromAce(state->defdname);
        if (domainName.isEmpty())
            domainName = QUrl::fromAce(state->dnsrch[0]);
        local_res_nclose(state);
        free(state);

        return domainName;
    }

    if (local_res_init && local_res) {
        // using thread-unsafe version

        local_res_init();
        QString domainName = QUrl::fromAce(local_res->defdname);
        if (domainName.isEmpty())
            domainName = QUrl::fromAce(local_res->dnsrch[0]);
        return domainName;
    }
#endif
    // nothing worked, try doing it by ourselves:
    QFile resolvconf;
#if defined(_PATH_RESCONF)
    resolvconf.setFileName(QFile::decodeName(_PATH_RESCONF));
#else
    resolvconf.setFileName(QLatin1String("/etc/resolv.conf"));
#endif
    if (!resolvconf.open(QIODevice::ReadOnly))
        return QString();       // failure

    QString domainName;
    while (!resolvconf.atEnd()) {
        QByteArray line = resolvconf.readLine().trimmed();
        if (line.startsWith("domain "))
            return QUrl::fromAce(line.mid(sizeof "domain " - 1).trimmed());

        // in case there's no "domain" line, fall back to the first "search" entry
        if (domainName.isEmpty() && line.startsWith("search ")) {
            QByteArray searchDomain = line.mid(sizeof "search " - 1).trimmed();
            int pos = searchDomain.indexOf(' ');
            if (pos != -1)
                searchDomain.truncate(pos);
            domainName = QUrl::fromAce(searchDomain);
        }
    }

    // return the fallen-back-to searched domain
    return domainName;
}

QT_END_NAMESPACE
