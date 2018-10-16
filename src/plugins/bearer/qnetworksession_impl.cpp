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

// see comment in ../platformdefs_win.h.
#define WIN32_LEAN_AND_MEAN 1

#include "qnetworksession_impl.h"
#include "qbearerengine_impl.h"

#include <QtNetwork/qnetworksession.h>
#include <QtNetwork/private/qnetworkconfigmanager_p.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmutex.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

static QBearerEngineImpl *getEngineFromId(const QString &id)
{
    QNetworkConfigurationManagerPrivate *priv = qNetworkConfigurationManagerPrivate();

    const auto engines = priv->engines();
    for (QBearerEngine *engine : engines) {
        QBearerEngineImpl *engineImpl = qobject_cast<QBearerEngineImpl *>(engine);
        if (engineImpl && engineImpl->hasIdentifier(id))
            return engineImpl;
    }

    return 0;
}

class QNetworkSessionManagerPrivate : public QObject
{
    Q_OBJECT

public:
    QNetworkSessionManagerPrivate(QObject *parent = 0) : QObject(parent) {}
    ~QNetworkSessionManagerPrivate() {}

    inline void forceSessionClose(const QNetworkConfiguration &config)
    { emit forcedSessionClose(config); }

Q_SIGNALS:
    void forcedSessionClose(const QNetworkConfiguration &config);
};

#include "qnetworksession_impl.moc"

Q_GLOBAL_STATIC(QNetworkSessionManagerPrivate, sessionManager);

void QNetworkSessionPrivateImpl::syncStateWithInterface()
{
    connect(sessionManager(), SIGNAL(forcedSessionClose(QNetworkConfiguration)),
            this, SLOT(forcedSessionClose(QNetworkConfiguration)));

    opened = false;
    isOpen = false;
    state = QNetworkSession::Invalid;
    lastError = QNetworkSession::UnknownSessionError;

    qRegisterMetaType<QBearerEngineImpl::ConnectionError>();

    switch (publicConfig.type()) {
    case QNetworkConfiguration::InternetAccessPoint:
        activeConfig = publicConfig;
        engine = getEngineFromId(activeConfig.identifier());
        if (engine) {
            qRegisterMetaType<QNetworkConfigurationPrivatePointer>();
            connect(engine, SIGNAL(configurationChanged(QNetworkConfigurationPrivatePointer)),
                    this, SLOT(configurationChanged(QNetworkConfigurationPrivatePointer)),
                    Qt::QueuedConnection);
            connect(engine, SIGNAL(connectionError(QString,QBearerEngineImpl::ConnectionError)),
                    this, SLOT(connectionError(QString,QBearerEngineImpl::ConnectionError)),
                    Qt::QueuedConnection);
        }
        break;
    case QNetworkConfiguration::ServiceNetwork:
        serviceConfig = publicConfig;
        // Defer setting engine and signals until open().
        Q_FALLTHROUGH();
    case QNetworkConfiguration::UserChoice:
        // Defer setting serviceConfig and activeConfig until open().
        Q_FALLTHROUGH();
    default:
        engine = 0;
    }

    networkConfigurationsChanged();
}

void QNetworkSessionPrivateImpl::open()
{
    if (serviceConfig.isValid()) {
        lastError = QNetworkSession::OperationNotSupportedError;
        emit QNetworkSessionPrivate::error(lastError);
    } else if (!isOpen) {
        if ((activeConfig.state() & QNetworkConfiguration::Discovered) != QNetworkConfiguration::Discovered) {
            lastError = QNetworkSession::InvalidConfigurationError;
            state = QNetworkSession::Invalid;
            emit stateChanged(state);
            emit QNetworkSessionPrivate::error(lastError);
            return;
        }
        opened = true;

        if ((activeConfig.state() & QNetworkConfiguration::Active) != QNetworkConfiguration::Active &&
            (activeConfig.state() & QNetworkConfiguration::Discovered) == QNetworkConfiguration::Discovered) {
            state = QNetworkSession::Connecting;
            emit stateChanged(state);

            engine->connectToId(activeConfig.identifier());
        }

        isOpen = (activeConfig.state() & QNetworkConfiguration::Active) == QNetworkConfiguration::Active;
        if (isOpen)
            emit quitPendingWaitsForOpened();
    }
}

void QNetworkSessionPrivateImpl::close()
{
    if (serviceConfig.isValid()) {
        lastError = QNetworkSession::OperationNotSupportedError;
        emit QNetworkSessionPrivate::error(lastError);
    } else if (isOpen) {
        opened = false;
        isOpen = false;
        emit closed();
    }
}

void QNetworkSessionPrivateImpl::stop()
{
    if (serviceConfig.isValid()) {
        lastError = QNetworkSession::OperationNotSupportedError;
        emit QNetworkSessionPrivate::error(lastError);
    } else {
        if ((activeConfig.state() & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
            state = QNetworkSession::Closing;
            emit stateChanged(state);

            engine->disconnectFromId(activeConfig.identifier());

            sessionManager()->forceSessionClose(activeConfig);
        }

        opened = false;
        isOpen = false;
        emit closed();
    }
}

void QNetworkSessionPrivateImpl::migrate()
{
}

void QNetworkSessionPrivateImpl::accept()
{
}

void QNetworkSessionPrivateImpl::ignore()
{
}

void QNetworkSessionPrivateImpl::reject()
{
}

#ifndef QT_NO_NETWORKINTERFACE
QNetworkInterface QNetworkSessionPrivateImpl::currentInterface() const
{
    if (!engine || state != QNetworkSession::Connected || !publicConfig.isValid())
        return QNetworkInterface();

    QString iface = engine->getInterfaceFromId(activeConfig.identifier());
    if (iface.isEmpty())
        return QNetworkInterface();
    return QNetworkInterface::interfaceFromName(iface);
}
#endif

QVariant QNetworkSessionPrivateImpl::sessionProperty(const QString &key) const
{
    if (key == QLatin1String("AutoCloseSessionTimeout")) {
        if (engine && engine->requiresPolling() &&
            !(engine->capabilities() & QNetworkConfigurationManager::CanStartAndStopInterfaces)) {
            return sessionTimeout >= 0 ? sessionTimeout * 10000 : -1;
        }
    }

    return QVariant();
}

void QNetworkSessionPrivateImpl::setSessionProperty(const QString &key, const QVariant &value)
{
    if (key == QLatin1String("AutoCloseSessionTimeout")) {
        if (engine && engine->requiresPolling() &&
            !(engine->capabilities() & QNetworkConfigurationManager::CanStartAndStopInterfaces)) {
            int timeout = value.toInt();
            if (timeout >= 0) {
                connect(engine, SIGNAL(updateCompleted()),
                        this, SLOT(decrementTimeout()), Qt::UniqueConnection);
                sessionTimeout = timeout / 10000;   // convert to poll intervals
            } else {
                disconnect(engine, SIGNAL(updateCompleted()), this, SLOT(decrementTimeout()));
                sessionTimeout = -1;
            }
        }
    }
}

QString QNetworkSessionPrivateImpl::errorString() const
{
    switch (lastError) {
    case QNetworkSession::UnknownSessionError:
        return tr("Unknown session error.");
    case QNetworkSession::SessionAbortedError:
        return tr("The session was aborted by the user or system.");
    case QNetworkSession::OperationNotSupportedError:
        return tr("The requested operation is not supported by the system.");
    case QNetworkSession::InvalidConfigurationError:
        return tr("The specified configuration cannot be used.");
    case QNetworkSession::RoamingError:
        return tr("Roaming was aborted or is not possible.");
    default:
        break;
    }

    return QString();
}

QNetworkSession::SessionError QNetworkSessionPrivateImpl::error() const
{
    return lastError;
}

quint64 QNetworkSessionPrivateImpl::bytesWritten() const
{
    if (engine && state == QNetworkSession::Connected)
        return engine->bytesWritten(activeConfig.identifier());
    return Q_UINT64_C(0);
}

quint64 QNetworkSessionPrivateImpl::bytesReceived() const
{
    if (engine && state == QNetworkSession::Connected)
        return engine->bytesReceived(activeConfig.identifier());
    return Q_UINT64_C(0);
}

quint64 QNetworkSessionPrivateImpl::activeTime() const
{
    if (state == QNetworkSession::Connected && startTime != Q_UINT64_C(0))
        return QDateTime::currentSecsSinceEpoch() - startTime;
    return Q_UINT64_C(0);
}

QNetworkSession::UsagePolicies QNetworkSessionPrivateImpl::usagePolicies() const
{
    return currentPolicies;
}

void QNetworkSessionPrivateImpl::setUsagePolicies(QNetworkSession::UsagePolicies newPolicies)
{
    if (newPolicies != currentPolicies) {
        currentPolicies = newPolicies;
        emit usagePoliciesChanged(currentPolicies);
    }
}

void QNetworkSessionPrivateImpl::updateStateFromServiceNetwork()
{
    QNetworkSession::State oldState = state;

    const auto configs = serviceConfig.children();
    for (const QNetworkConfiguration &config : configs) {
        if ((config.state() & QNetworkConfiguration::Active) != QNetworkConfiguration::Active)
            continue;

        if (activeConfig != config) {
            if (engine) {
                disconnect(engine, SIGNAL(connectionError(QString,QBearerEngineImpl::ConnectionError)),
                           this, SLOT(connectionError(QString,QBearerEngineImpl::ConnectionError)));
            }

            activeConfig = config;
            engine = getEngineFromId(activeConfig.identifier());

            if (engine) {
                connect(engine, SIGNAL(connectionError(QString,QBearerEngineImpl::ConnectionError)),
                        this, SLOT(connectionError(QString,QBearerEngineImpl::ConnectionError)),
                        Qt::QueuedConnection);
            }
            emit newConfigurationActivated();
        }

        state = QNetworkSession::Connected;
        if (state != oldState)
            emit stateChanged(state);

        return;
    }

    if (serviceConfig.children().isEmpty())
        state = QNetworkSession::NotAvailable;
    else
        state = QNetworkSession::Disconnected;

    if (state != oldState)
        emit stateChanged(state);
}

void QNetworkSessionPrivateImpl::updateStateFromActiveConfig()
{
    if (!engine)
        return;

    QNetworkSession::State oldState = state;
    state = engine->sessionStateForId(activeConfig.identifier());

    bool oldActive = isOpen;
    isOpen = (state == QNetworkSession::Connected) ? opened : false;

    if (!oldActive && isOpen)
        emit quitPendingWaitsForOpened();
    if (oldActive && !isOpen)
        emit closed();

    if (oldState != state)
        emit stateChanged(state);
}

void QNetworkSessionPrivateImpl::networkConfigurationsChanged()
{
    if (serviceConfig.isValid())
        updateStateFromServiceNetwork();
    else
        updateStateFromActiveConfig();

    if (engine)
        startTime = engine->startTime(activeConfig.identifier());
}

void QNetworkSessionPrivateImpl::configurationChanged(QNetworkConfigurationPrivatePointer config)
{
    if (serviceConfig.isValid() &&
        (config->id == serviceConfig.identifier() || config->id == activeConfig.identifier())) {
        updateStateFromServiceNetwork();
    } else if (config->id == activeConfig.identifier()) {
        updateStateFromActiveConfig();
    }
}

void QNetworkSessionPrivateImpl::forcedSessionClose(const QNetworkConfiguration &config)
{
    if (activeConfig == config) {
        opened = false;
        isOpen = false;

        emit closed();

        lastError = QNetworkSession::SessionAbortedError;
        emit QNetworkSessionPrivate::error(lastError);
    }
}

void QNetworkSessionPrivateImpl::connectionError(const QString &id, QBearerEngineImpl::ConnectionError error)
{
    if (activeConfig.identifier() == id) {
        networkConfigurationsChanged();
        switch (error) {
        case QBearerEngineImpl::OperationNotSupported:
            lastError = QNetworkSession::OperationNotSupportedError;
            opened = false;
            break;
        case QBearerEngineImpl::InterfaceLookupError:
        case QBearerEngineImpl::ConnectError:
        case QBearerEngineImpl::DisconnectionError:
        default:
            lastError = QNetworkSession::UnknownSessionError;
        }

        emit QNetworkSessionPrivate::error(lastError);
    }
}

void QNetworkSessionPrivateImpl::decrementTimeout()
{
    if (--sessionTimeout <= 0) {
        disconnect(engine, SIGNAL(updateCompleted()), this, SLOT(decrementTimeout()));
        sessionTimeout = -1;
        close();
    }
}

QT_END_NAMESPACE
