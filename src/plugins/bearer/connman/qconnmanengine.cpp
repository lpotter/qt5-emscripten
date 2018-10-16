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

#include "qconnmanengine.h"
#include "qconnmanservice_linux_p.h"
#include "../qnetworksession_impl.h"

#include <QtNetwork/private/qnetworkconfiguration_p.h>

#include <QtNetwork/qnetworksession.h>

#include <QtCore/qdebug.h>

#include <QtDBus/QtDBus>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QConnmanEngine::QConnmanEngine(QObject *parent)
:   QBearerEngineImpl(parent),
    connmanManager(new QConnmanManagerInterface(this)),
    ofonoManager(new QOfonoManagerInterface(this)),
    ofonoNetwork(0),
    ofonoContextManager(0)
{
    qDBusRegisterMetaType<ConnmanMap>();
    qDBusRegisterMetaType<ConnmanMapList>();
    qRegisterMetaType<ConnmanMapList>("ConnmanMapList");
}

QConnmanEngine::~QConnmanEngine()
{
}

bool QConnmanEngine::connmanAvailable() const
{
    QMutexLocker locker(&mutex);
    return connmanManager->isValid();
}

void QConnmanEngine::initialize()
{
    QMutexLocker locker(&mutex);
    connect(ofonoManager,SIGNAL(modemChanged()),this,SLOT(changedModem()));

    ofonoNetwork = new QOfonoNetworkRegistrationInterface(ofonoManager->currentModem(),this);
    ofonoContextManager = new QOfonoDataConnectionManagerInterface(ofonoManager->currentModem(),this);
    connect(ofonoContextManager,SIGNAL(roamingAllowedChanged(bool)),this,SLOT(reEvaluateCellular()));

    connect(connmanManager,SIGNAL(servicesChanged(ConnmanMapList,QList<QDBusObjectPath>)),
            this, SLOT(updateServices(ConnmanMapList,QList<QDBusObjectPath>)));

    connect(connmanManager,SIGNAL(servicesReady(QStringList)),this,SLOT(servicesReady(QStringList)));
    connect(connmanManager,SIGNAL(scanFinished(bool)),this,SLOT(finishedScan(bool)));

    const auto servPaths = connmanManager->getServices();
    for (const QString &servPath : servPaths)
        addServiceConfiguration(servPath);
    Q_EMIT updateCompleted();
}

void QConnmanEngine::changedModem()
{
    QMutexLocker locker(&mutex);
    if (ofonoNetwork)
        delete ofonoNetwork;

    ofonoNetwork = new QOfonoNetworkRegistrationInterface(ofonoManager->currentModem(),this);

    if (ofonoContextManager)
        delete ofonoContextManager;
    ofonoContextManager = new QOfonoDataConnectionManagerInterface(ofonoManager->currentModem(),this);
}

void QConnmanEngine::servicesReady(const QStringList &list)
{
    QMutexLocker locker(&mutex);
    for (const QString &servPath : list)
        addServiceConfiguration(servPath);

    Q_EMIT updateCompleted();
}

QList<QNetworkConfigurationPrivate *> QConnmanEngine::getConfigurations()
{
    QMutexLocker locker(&mutex);
    QList<QNetworkConfigurationPrivate *> fetchedConfigurations;
    QNetworkConfigurationPrivate* cpPriv = 0;
    const int numFoundConfigurations = foundConfigurations.count();
    fetchedConfigurations.reserve(numFoundConfigurations);

    for (int i = 0; i < numFoundConfigurations; ++i) {
        QNetworkConfigurationPrivate *config = new QNetworkConfigurationPrivate;
        cpPriv = foundConfigurations.at(i);

        config->name = cpPriv->name;
        config->isValid = cpPriv->isValid;
        config->id = cpPriv->id;
        config->state = cpPriv->state;
        config->type = cpPriv->type;
        config->roamingSupported = cpPriv->roamingSupported;
        config->purpose = cpPriv->purpose;
        config->bearerType = cpPriv->bearerType;

        fetchedConfigurations.append(config);
        delete config;
    }
    return fetchedConfigurations;
}

QString QConnmanEngine::getInterfaceFromId(const QString &id)
{
    QMutexLocker locker(&mutex);
    return configInterfaces.value(id);
}

bool QConnmanEngine::hasIdentifier(const QString &id)
{
    QMutexLocker locker(&mutex);
    return accessPointConfigurations.contains(id);
}

void QConnmanEngine::connectToId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QConnmanServiceInterface *serv = connmanServiceInterfaces.value(id);

    if (!serv || !serv->isValid()) {
        emit connectionError(id, QBearerEngineImpl::InterfaceLookupError);
    } else {
        if (serv->type() == QLatin1String("cellular")) {
            if (serv->roaming()) {
                if (!isRoamingAllowed(serv->path())) {
                    emit connectionError(id, QBearerEngineImpl::OperationNotSupported);
                    return;
                }
            }
        }
        if (serv->autoConnect())
            serv->connect();
    }
}

void QConnmanEngine::disconnectFromId(const QString &id)
{
    QMutexLocker locker(&mutex);
    QConnmanServiceInterface *serv = connmanServiceInterfaces.value(id);

    if (!serv || !serv->isValid()) {
        emit connectionError(id, DisconnectionError);
    } else {
        serv->disconnect();
    }
}

void QConnmanEngine::requestUpdate()
{
    QMutexLocker locker(&mutex);
    QTimer::singleShot(0, this, SLOT(doRequestUpdate()));
}

void QConnmanEngine::doRequestUpdate()
{
    bool scanned = connmanManager->requestScan("wifi");
    if (!scanned)
        Q_EMIT updateCompleted();
}

void QConnmanEngine::finishedScan(bool error)
{
    if (error)
        Q_EMIT updateCompleted();
}

void QConnmanEngine::updateServices(const ConnmanMapList &changed, const QList<QDBusObjectPath> &removed)
{
    QMutexLocker locker(&mutex);

    foreach (const QDBusObjectPath &objectPath, removed) {
        removeConfiguration(objectPath.path());
    }

    foreach (const ConnmanMap &connmanMap, changed) {
        const QString id = connmanMap.objectPath.path();
        if (accessPointConfigurations.contains(id)) {
            configurationChange(connmanServiceInterfaces.value(id));
        } else {
            addServiceConfiguration(connmanMap.objectPath.path());
        }
    }
    Q_EMIT updateCompleted();
}

QNetworkSession::State QConnmanEngine::sessionStateForId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

    if (!ptr || !ptr->isValid)
        return QNetworkSession::Invalid;

    QString service = id;
    QConnmanServiceInterface *serv = connmanServiceInterfaces.value(service);
    if (!serv)
        return QNetworkSession::Invalid;

    QString servState = serv->state();

    if (serv->favorite() && (servState == QLatin1String("idle") || servState == QLatin1String("failure"))) {
        return QNetworkSession::Disconnected;
    }

    if (servState == QLatin1String("association") || servState == QLatin1String("configuration")) {
        return QNetworkSession::Connecting;
    }

    if (servState == QLatin1String("online") || servState == QLatin1String("ready")) {
        return QNetworkSession::Connected;
    }

    if ((ptr->state & QNetworkConfiguration::Discovered) ==
                QNetworkConfiguration::Discovered) {
        return QNetworkSession::Disconnected;
    } else if ((ptr->state & QNetworkConfiguration::Defined) == QNetworkConfiguration::Defined) {
        return QNetworkSession::NotAvailable;
    } else if ((ptr->state & QNetworkConfiguration::Undefined) ==
                QNetworkConfiguration::Undefined) {
        return QNetworkSession::NotAvailable;
    }

    return QNetworkSession::Invalid;
}

quint64 QConnmanEngine::bytesWritten(const QString &id)
{//TODO use connman counter API
    QMutexLocker locker(&mutex);
    quint64 result = 0;
    QString devFile = getInterfaceFromId(id);
    QFile tx("/sys/class/net/"+devFile+"/statistics/tx_bytes");
    if (tx.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&tx);
        in >> result;
        tx.close();
    }

    return result;
}

quint64 QConnmanEngine::bytesReceived(const QString &id)
{//TODO use connman counter API
    QMutexLocker locker(&mutex);
    quint64 result = 0;
    QString devFile = getInterfaceFromId(id);
    QFile rx("/sys/class/net/"+devFile+"/statistics/rx_bytes");
    if (rx.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&rx);
        in >> result;
        rx.close();
    }
    return result;
}

quint64 QConnmanEngine::startTime(const QString &/*id*/)
{
    // TODO
    QMutexLocker locker(&mutex);
    if (activeTime.isNull()) {
        return 0;
    }
    return activeTime.secsTo(QDateTime::currentDateTime());
}

QNetworkConfigurationManager::Capabilities QConnmanEngine::capabilities() const
{
    return QNetworkConfigurationManager::ForcedRoaming |
            QNetworkConfigurationManager::DataStatistics |
            QNetworkConfigurationManager::CanStartAndStopInterfaces |
            QNetworkConfigurationManager::NetworkSessionRequired;
}

QNetworkSessionPrivate *QConnmanEngine::createSessionBackend()
{
     return new QNetworkSessionPrivateImpl;
}

QNetworkConfigurationPrivatePointer QConnmanEngine::defaultConfiguration()
{
    const QMutexLocker locker(&mutex);
    const auto servPaths = connmanManager->getServices();
    for (const QString &servPath : servPaths) {
        if (connmanServiceInterfaces.contains(servPath)) {
            if (accessPointConfigurations.contains(servPath))
                return accessPointConfigurations.value(servPath);
        }
    }
    return QNetworkConfigurationPrivatePointer();
}

void QConnmanEngine::serviceStateChanged(const QString &state)
{
    QConnmanServiceInterface *service = qobject_cast<QConnmanServiceInterface *>(sender());
    configurationChange(service);

    if (state == QLatin1String("failure")) {
        emit connectionError(service->path(), ConnectError);
    }
}

void QConnmanEngine::configurationChange(QConnmanServiceInterface *serv)
{
    if (!serv)
        return;
    QMutexLocker locker(&mutex);
    QString id = serv->path();

    if (accessPointConfigurations.contains(id)) {
        bool changed = false;
        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
        QString networkName = serv->name();
        QNetworkConfiguration::StateFlags curState = getStateForService(serv->path());
        ptr->mutex.lock();

        if (!ptr->isValid) {
            ptr->isValid = true;
        }

        if (ptr->name != networkName) {
            ptr->name = networkName;
            changed = true;
        }

        if (ptr->state != curState) {
            ptr->state = curState;
            changed = true;
        }

        ptr->mutex.unlock();

        if (changed) {
            locker.unlock();
            emit configurationChanged(ptr);
            locker.relock();
        }
    }

     locker.unlock();
     emit updateCompleted();
}

QNetworkConfiguration::StateFlags QConnmanEngine::getStateForService(const QString &service)
{
    QMutexLocker locker(&mutex);
    QConnmanServiceInterface *serv = connmanServiceInterfaces.value(service);
    if (!serv)
        return QNetworkConfiguration::Undefined;

    QString state = serv->state();
    QNetworkConfiguration::StateFlags flag = QNetworkConfiguration::Defined;

    if (serv->type() == QLatin1String("cellular")) {

        if (!serv->autoConnect()|| (serv->roaming()
                    && !isRoamingAllowed(serv->path()))) {
            flag = (flag | QNetworkConfiguration::Defined);
        } else {
            flag = (flag | QNetworkConfiguration::Discovered);
        }
    } else {
        if (serv->favorite()) {
            if (serv->autoConnect()) {
                flag = (flag | QNetworkConfiguration::Discovered);
            }
        } else {
            flag = QNetworkConfiguration::Undefined;
        }
    }
    if (state == QLatin1String("online") || state == QLatin1String("ready")) {
        flag = (flag | QNetworkConfiguration::Active);
    }

    return flag;
}

QNetworkConfiguration::BearerType QConnmanEngine::typeToBearer(const QString &type)
{
    if (type == QLatin1String("wifi"))
        return QNetworkConfiguration::BearerWLAN;
    if (type == QLatin1String("ethernet"))
        return QNetworkConfiguration::BearerEthernet;
    if (type == QLatin1String("bluetooth"))
        return QNetworkConfiguration::BearerBluetooth;
    if (type == QLatin1String("cellular")) {
        return ofonoTechToBearerType(type);
    }
    if (type == QLatin1String("wimax"))
        return QNetworkConfiguration::BearerWiMAX;

    return QNetworkConfiguration::BearerUnknown;
}

QNetworkConfiguration::BearerType QConnmanEngine::ofonoTechToBearerType(const QString &/*type*/)
{
    if (ofonoNetwork) {
        QString currentTechnology = ofonoNetwork->getTechnology();
        if (currentTechnology == QLatin1String("gsm")) {
            return QNetworkConfiguration::Bearer2G;
        } else if (currentTechnology == QLatin1String("edge")) {
            return QNetworkConfiguration::BearerCDMA2000; //wrong, I know
        } else if (currentTechnology == QLatin1String("umts")) {
            return QNetworkConfiguration::BearerWCDMA;
        } else if (currentTechnology == QLatin1String("hspa")) {
            return QNetworkConfiguration::BearerHSPA;
        } else if (currentTechnology == QLatin1String("lte")) {
            return QNetworkConfiguration::BearerLTE;
        }
    }
    return QNetworkConfiguration::BearerUnknown;
}

bool QConnmanEngine::isRoamingAllowed(const QString &context)
{
    const auto dcPaths = ofonoContextManager->contexts();
    for (const QString &dcPath : dcPaths) {
        if (dcPath.contains(context.section("_",-1))) {
            return ofonoContextManager->roamingAllowed();
        }
    }
    return false;
}

void QConnmanEngine::removeConfiguration(const QString &id)
{
    QMutexLocker locker(&mutex);

    if (accessPointConfigurations.contains(id)) {

        disconnect(connmanServiceInterfaces.value(id),SIGNAL(stateChanged(QString)),
                this,SLOT(serviceStateChanged(QString)));
        serviceNetworks.removeOne(id);
        QConnmanServiceInterface *service  = connmanServiceInterfaces.take(id);
        delete service;
        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.take(id);
        foundConfigurations.removeOne(ptr.data());
        locker.unlock();
        emit configurationRemoved(ptr);
        locker.relock();
    }
}

void QConnmanEngine::addServiceConfiguration(const QString &servicePath)
{
    QMutexLocker locker(&mutex);
    if (!connmanServiceInterfaces.contains(servicePath)) {
        QConnmanServiceInterface *serv = new QConnmanServiceInterface(servicePath, this);
        connmanServiceInterfaces.insert(serv->path(),serv);
    }

    if (!accessPointConfigurations.contains(servicePath)) {

        serviceNetworks.append(servicePath);

        connect(connmanServiceInterfaces.value(servicePath),SIGNAL(stateChanged(QString)),
                this,SLOT(serviceStateChanged(QString)));

        QNetworkConfigurationPrivate* cpPriv = new QNetworkConfigurationPrivate();
        QConnmanServiceInterface *service = connmanServiceInterfaces.value(servicePath);

        QString networkName = service->name();

        const QString connectionType = service->type();
        if (connectionType == QLatin1String("ethernet")) {
            cpPriv->bearerType = QNetworkConfiguration::BearerEthernet;
        } else if (connectionType == QLatin1String("wifi")) {
            cpPriv->bearerType = QNetworkConfiguration::BearerWLAN;
        } else if (connectionType == QLatin1String("cellular")) {
            cpPriv->bearerType = ofonoTechToBearerType(QLatin1String("cellular"));
            cpPriv->roamingSupported = service->roaming() && isRoamingAllowed(servicePath);
        } else if (connectionType == QLatin1String("wimax")) {
            cpPriv->bearerType = QNetworkConfiguration::BearerWiMAX;
        } else {
            cpPriv->bearerType = QNetworkConfiguration::BearerUnknown;
        }

        cpPriv->name = networkName;
        cpPriv->isValid = true;
        cpPriv->id = servicePath;
        cpPriv->type = QNetworkConfiguration::InternetAccessPoint;

        if (service->security() == QLatin1String("none")) {
            cpPriv->purpose = QNetworkConfiguration::PublicPurpose;
        } else {
            cpPriv->purpose = QNetworkConfiguration::PrivatePurpose;
        }

        cpPriv->state = getStateForService(servicePath);

        QNetworkConfigurationPrivatePointer ptr(cpPriv);
        accessPointConfigurations.insert(ptr->id, ptr);
        if (connectionType == QLatin1String("cellular")) {
            foundConfigurations.append(cpPriv);
        } else {
            foundConfigurations.prepend(cpPriv);
        }
        configInterfaces[cpPriv->id] = service->serviceInterface();

        locker.unlock();
        Q_EMIT configurationAdded(ptr);
        locker.relock();
    }
}

bool QConnmanEngine::requiresPolling() const
{
    return false;
}

void QConnmanEngine::reEvaluateCellular()
{
    const auto servicePaths = connmanManager->getServices();
    for (const QString &servicePath : servicePaths) {
        if (servicePath.contains("cellular") && accessPointConfigurations.contains(servicePath)) {
            configurationChange(connmanServiceInterfaces.value(servicePath));
        }
    }
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
