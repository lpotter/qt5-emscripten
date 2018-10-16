/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef ANDROIDCONNECTIVITYMANAGER_H
#define ANDROIDCONNECTIVITYMANAGER_H

#include <QObject>
#include <QtCore/private/qjni_p.h>

QT_BEGIN_NAMESPACE

class AndroidTrafficStats
{
public:
    static qint64 getMobileTxBytes();
    static qint64 getMobileRxBytes();
    static qint64 getTotalTxBytes();
    static qint64 getTotalRxBytes();
    static bool isTrafficStatsSupported();
};

class AndroidNetworkInfo
{
public:
    // Needs to be in sync with the values from the android api.
    enum NetworkState {
        UnknownState,
        Authenticating,
        Blocked,
        CaptivePortalCheck,
        Connected,
        Connecting,
        Disconnected,
        Disconnecting,
        Failed,
        Idle,
        ObtainingIpAddr,
        Scanning,
        Suspended,
        VerifyingPoorLink
    };

    enum NetworkType {
        Mobile,
        Wifi,
        MobileMms,
        MobileSupl,
        MobileDun,
        MobileHipri,
        Wimax,
        Bluetooth,
        Dummy,
        Ethernet,
        UnknownType
    };

    enum NetworkSubType {
        UnknownSubType,
        Gprs,
        Edge,
        Umts,
        Cdma,
        Evdo0,
        EvdoA,
        Cdma1xRTT,
        Hsdpa,
        Hsupa,
        Hspa,
        Iden,
        EvdoB,
        Lte,
        Ehrpd,
        Hspap
    };

    inline AndroidNetworkInfo(const QJNIObjectPrivate &obj) : m_networkInfo(obj)
    { }

    NetworkState getDetailedState() const;
    QString getExtraInfo() const;
    QString getReason() const;
    NetworkState getState() const;
    NetworkSubType getSubtype() const;
    QString getSubtypeName() const;
    NetworkType getType() const;
    QString getTypeName() const;
    bool isAvailable() const;
    bool isConnected() const;
    bool isConnectedOrConnecting() const;
    bool isFailover() const;
    bool isRoaming() const;
    bool isValid() const;

private:
    QJNIObjectPrivate m_networkInfo;
};

class AndroidConnectivityManager : public QObject
{
    Q_OBJECT
public:
    static AndroidConnectivityManager *getInstance();
    ~AndroidConnectivityManager();

    AndroidNetworkInfo getActiveNetworkInfo() const;
    QList<AndroidNetworkInfo> getAllNetworkInfo() const;
    bool getBackgroundDataSetting() const;
    AndroidNetworkInfo getNetworkInfo(int networkType) const;
    int getNetworkPreference() const;
    bool isActiveNetworkMetered() const;
    static bool isNetworkTypeValid(int networkType);
    bool requestRouteToHost(int networkType, int hostAddress);
    void setNetworkPreference(int preference);
    int startUsingNetworkFeature(int networkType, const QString &feature);
    int stopUsingNetworkFeature(int networkType, const QString &feature);
    inline bool isValid() const
    {
        return m_connectivityManager.isValid();
    }

    Q_SIGNAL void activeNetworkChanged();

private:
    friend struct AndroidConnectivityManagerInstance;
    AndroidConnectivityManager();
    bool registerNatives(JNIEnv *env);
    QJNIObjectPrivate m_connectivityManager;
};

QT_END_NAMESPACE

#endif // ANDROIDCONNECTIVITYMANAGER_H
