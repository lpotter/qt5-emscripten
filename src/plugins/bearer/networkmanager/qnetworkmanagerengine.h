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

#ifndef QNETWORKMANAGERENGINE_P_H
#define QNETWORKMANAGERENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "../qbearerengine_impl.h"

#include "qnetworkmanagerservice.h"

#include "../linux_common/qofonoservice_linux_p.h"

#include <QMap>
#include <QVariant>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QNetworkManagerEngine : public QBearerEngineImpl
{
    Q_OBJECT

public:
    QNetworkManagerEngine(QObject *parent = 0);
    ~QNetworkManagerEngine();

    bool networkManagerAvailable() const;

    QString getInterfaceFromId(const QString &id) override;
    bool hasIdentifier(const QString &id) override;

    void connectToId(const QString &id) override;
    void disconnectFromId(const QString &id) override;

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate();

    QNetworkSession::State sessionStateForId(const QString &id) override;

    quint64 bytesWritten(const QString &id) override;
    quint64 bytesReceived(const QString &id) override;
    quint64 startTime(const QString &id) override;

    QNetworkConfigurationManager::Capabilities capabilities() const override;

    QNetworkSessionPrivate *createSessionBackend() override;

    QNetworkConfigurationPrivatePointer defaultConfiguration() override;

private Q_SLOTS:
    void interfacePropertiesChanged(const QMap<QString, QVariant> &properties);
    void activeConnectionPropertiesChanged(const QMap<QString, QVariant> &properties);

    void newConnection(const QDBusObjectPath &path, QNetworkManagerSettings *settings = 0);
    void removeConnection(const QString &path);
    void updateConnection();
    void activationFinished(QDBusPendingCallWatcher *watcher);
    void deviceConnectionsChanged(const QStringList &activeConnectionsList);

    void wiredCarrierChanged(bool);

    void nmRegistered(const QString &serviceName = QString());
    void nmUnRegistered(const QString &serviceName = QString());

    void ofonoRegistered(const QString &serviceName = QString());
    void ofonoUnRegistered(const QString &serviceName = QString());

private:
    QNetworkConfigurationPrivate *parseConnection(const QString &settingsPath,
                                                  const QNmSettingsMap &map);
    QNetworkManagerSettingsConnection *connectionFromId(const QString &id) const;

    QNetworkManagerInterface *managerInterface;
    QNetworkManagerSettings *systemSettings;
    QHash<QString, QNetworkManagerInterfaceDeviceWired *> wiredDevices;
    QHash<QString, QNetworkManagerInterfaceDeviceWireless *> wirelessDevices;

    QHash<QString, QNetworkManagerConnectionActive *> activeConnectionsList;
    QList<QNetworkManagerSettingsConnection *> connections;
    QList<QNetworkManagerInterfaceAccessPoint *> accessPoints;
    QHash<QString, QNetworkManagerInterfaceDevice *> interfaceDevices;

    QMap<QString,QString> configuredAccessPoints; //ap, settings path
    QHash<QString,QString> connectionInterfaces; // ac, interface

    QOfonoManagerInterface *ofonoManager;
    QHash <QString, QOfonoDataConnectionManagerInterface *> ofonoContextManagers;
    QNetworkConfiguration::BearerType currentBearerType(const QString &id) const;
    QString contextName(const QString &path) const;

    bool isConnectionActive(const QString &settingsPath) const;
    QDBusServiceWatcher *ofonoWatcher;
    QDBusServiceWatcher *nmWatcher;

    bool isActiveContext(const QString &contextPath) const;
    bool nmAvailable;
    void setupConfigurations();
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS

#endif

