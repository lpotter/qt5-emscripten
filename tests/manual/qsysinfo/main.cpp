/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
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

#include <QCoreApplication>
#include <QOperatingSystemVersion>
#include <QSysInfo>

#include <stdio.h>

#if QT_DEPRECATED_SINCE(5, 9)
#define CASE_VERSION(v)     case QSysInfo::v:   return QT_STRINGIFY(v)

QByteArray windowsVersionToString(QSysInfo::WinVersion v)
{
    switch (v) {
    CASE_VERSION(WV_None);

    CASE_VERSION(WV_32s);
    CASE_VERSION(WV_95);
    CASE_VERSION(WV_98);
    CASE_VERSION(WV_Me);
    case QSysInfo::WV_DOS_based: // shouldn't happen
        break;

    CASE_VERSION(WV_NT);
    CASE_VERSION(WV_2000);
    CASE_VERSION(WV_XP);
    CASE_VERSION(WV_2003);
    CASE_VERSION(WV_VISTA);
    CASE_VERSION(WV_WINDOWS7);
    CASE_VERSION(WV_WINDOWS8);
    CASE_VERSION(WV_WINDOWS8_1);
    CASE_VERSION(WV_WINDOWS10);
    case QSysInfo::WV_NT_based: // shouldn't happen
        break;
    }

    return "WinVersion(0x" + QByteArray::number(v, 16) + ')';
}

QByteArray macVersionToString(QSysInfo::MacVersion v)
{
    switch (v) {
    CASE_VERSION(MV_None);
    CASE_VERSION(MV_Unknown);

    CASE_VERSION(MV_9);
    CASE_VERSION(MV_10_0);
    CASE_VERSION(MV_10_1);
    CASE_VERSION(MV_10_2);
    CASE_VERSION(MV_10_3);
    CASE_VERSION(MV_10_4);
    CASE_VERSION(MV_10_5);
    CASE_VERSION(MV_10_6);
    CASE_VERSION(MV_10_7);
    CASE_VERSION(MV_10_8);
    CASE_VERSION(MV_10_9);
    CASE_VERSION(MV_10_10);
    CASE_VERSION(MV_10_11);

    CASE_VERSION(MV_IOS_4_3);
    CASE_VERSION(MV_IOS_5_0);
    CASE_VERSION(MV_IOS_5_1);
    CASE_VERSION(MV_IOS_6_0);
    CASE_VERSION(MV_IOS_6_1);
    CASE_VERSION(MV_IOS_7_0);
    CASE_VERSION(MV_IOS_7_1);
    CASE_VERSION(MV_IOS_8_0);
    CASE_VERSION(MV_IOS_8_1);
    CASE_VERSION(MV_IOS_8_2);
    CASE_VERSION(MV_IOS_8_3);
    CASE_VERSION(MV_IOS_8_4);
    CASE_VERSION(MV_IOS_9_0);
    case QSysInfo::MV_IOS:      // shouldn't happen:
        break;
    }

    if (v & QSysInfo::MV_IOS) {
        int major = (v >> 4) & 0xf;
        int minor = v & 0xf;
        return "MacVersion(Q_MV_IOS("
                + QByteArray::number(major) + ", "
                + QByteArray::number(minor) + "))";
    }
    return "MacVersion(Q_MV_OSX(10, " + QByteArray::number(v - 2) + "))";
}
#endif

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    printf("QSysInfo::WordSize = %d\n", QSysInfo::WordSize);
    printf("QSysInfo::ByteOrder = QSysInfo::%sEndian\n",
           QSysInfo::ByteOrder == QSysInfo::LittleEndian ? "Little" : "Big");
#if QT_DEPRECATED_SINCE(5, 9)
    printf("QSysInfo::WindowsVersion = QSysInfo::%s\n",
           windowsVersionToString(QSysInfo::WindowsVersion).constData());
    printf("QSysInfo::MacintoshVersion = QSysInfo::%s\n",
           macVersionToString(QSysInfo::MacintoshVersion).constData());
#endif
    printf("QSysInfo::buildCpuArchitecture() = %s\n", qPrintable(QSysInfo::buildCpuArchitecture()));
    printf("QSysInfo::currentCpuArchitecture() = %s\n", qPrintable(QSysInfo::currentCpuArchitecture()));
    printf("QSysInfo::buildAbi() = %s\n", qPrintable(QSysInfo::buildAbi()));
    printf("QSysInfo::kernelType() = %s\n", qPrintable(QSysInfo::kernelType()));
    printf("QSysInfo::kernelVersion() = %s\n", qPrintable(QSysInfo::kernelVersion()));
    printf("QSysInfo::productType() = %s\n", qPrintable(QSysInfo::productType()));
    printf("QSysInfo::productVersion() = %s\n", qPrintable(QSysInfo::productVersion()));
    printf("QSysInfo::prettyProductName() = %s\n", qPrintable(QSysInfo::prettyProductName()));
    printf("QSysInfo::machineHostName() = %s\n", qPrintable(QSysInfo::machineHostName()));
    printf("QSysInfo::machineUniqueId() = %s\n", QSysInfo::machineUniqueId().constData());
    printf("QSysInfo::bootUniqueId() = %s\n", qPrintable(QSysInfo::bootUniqueId()));

    const auto osv = QOperatingSystemVersion::current();
    printf("QOperatingSystemVersion::current() = %s %d.%d.%d\n",
        qPrintable(osv.name()),
        osv.majorVersion(),
        osv.minorVersion(),
        osv.microVersion());

    return 0;
}
