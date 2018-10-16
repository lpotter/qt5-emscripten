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

#include "qnetworkconfigmanager.h"

#include "qnetworkconfigmanager_p.h"
#include "qbearerengine_p.h"

#include <QtCore/qstringlist.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/private/qcoreapplication_p.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

static QBasicAtomicPointer<QNetworkConfigurationManagerPrivate> connManager_ptr;
static QBasicAtomicInt appShutdown;

static void connManager_prepare()
{
    int shutdown = appShutdown.fetchAndStoreAcquire(0);
    Q_ASSERT(shutdown == 0 || shutdown == 1);
    Q_UNUSED(shutdown);
}

static void connManager_cleanup()
{
    // this is not atomic or thread-safe!
    int shutdown = appShutdown.fetchAndStoreAcquire(1);
    Q_ASSERT(shutdown == 0);
    Q_UNUSED(shutdown);
    QNetworkConfigurationManagerPrivate *cmp = connManager_ptr.fetchAndStoreAcquire(0);
    if (cmp)
        cmp->cleanup();
}

void QNetworkConfigurationManagerPrivate::addPreAndPostRoutine()
{
    qAddPreRoutine(connManager_prepare);
    qAddPostRoutine(connManager_cleanup);
}

QNetworkConfigurationManagerPrivate *qNetworkConfigurationManagerPrivate()
{
    QNetworkConfigurationManagerPrivate *ptr = connManager_ptr.loadAcquire();
    int shutdown = appShutdown.loadAcquire();
    if (!ptr && !shutdown) {
        static QBasicMutex connManager_mutex;
        QMutexLocker locker(&connManager_mutex);
        if (!(ptr = connManager_ptr.loadAcquire())) {
            ptr = new QNetworkConfigurationManagerPrivate;

            if (QCoreApplicationPrivate::mainThread() == QThread::currentThread()) {
                // right thread or no main thread yet
                ptr->addPreAndPostRoutine();
                ptr->initialize();
            } else {
                // wrong thread, we need to make the main thread do this
                QObject *obj = new QObject;
                QObject::connect(obj, SIGNAL(destroyed()), ptr, SLOT(addPreAndPostRoutine()), Qt::DirectConnection);
                ptr->initialize(); // this moves us to the right thread
                obj->moveToThread(QCoreApplicationPrivate::mainThread());
                obj->deleteLater();
            }

            connManager_ptr.storeRelease(ptr);
        }
    }
    return ptr;
}

/*!
    \class QNetworkConfigurationManager

    \brief The QNetworkConfigurationManager class manages the network configurations provided
    by the system.

    \since 4.7

    \inmodule QtNetwork
    \ingroup network

    QNetworkConfigurationManager provides access to the network configurations known to the system and
    enables applications to detect the system capabilities (with regards to network sessions) at runtime.

    A QNetworkConfiguration abstracts a set of configuration options describing how a
    network interface has to be configured to connect to a particular target network.
    QNetworkConfigurationManager maintains and updates the global list of
    QNetworkConfigurations. Applications can access and filter this list via
    allConfigurations(). If a new configuration is added or an existing one is removed or changed
    the configurationAdded(), configurationRemoved() and configurationChanged() signals are emitted
    respectively.

    The defaultConfiguration() can be used when intending to immediately create a new
    network session without caring about the particular configuration. It returns
    a \l QNetworkConfiguration::Discovered configuration. If there are not any
    discovered ones an invalid configuration is returned.

    Some configuration updates may require some time to perform updates. A WLAN scan is
    such an example. Unless the platform performs internal updates it may be required to
    manually trigger configuration updates via QNetworkConfigurationManager::updateConfigurations().
    The completion of the update process is indicated by emitting the updateCompleted()
    signal. The update process ensures that every existing QNetworkConfiguration instance
    is updated. There is no need to ask for a renewed configuration list via allConfigurations().

    \sa QNetworkConfiguration
*/

/*!
    \fn void QNetworkConfigurationManager::configurationAdded(const QNetworkConfiguration &config)

    This signal is emitted whenever a new network configuration is added to the system. The new
    configuration is specified by \a config.
*/

/*!
    \fn void QNetworkConfigurationManager::configurationRemoved(const QNetworkConfiguration &config)

    This signal is emitted when a configuration is about to be removed from the system. The removed
    configuration, specified by \a config, is invalid but retains name and identifier.
*/

/*!
    \fn void QNetworkConfigurationManager::updateCompleted()

    This signal is emitted when the configuration update has been completed. Such an update can
    be initiated via \l updateConfigurations().
*/

/*! \fn void QNetworkConfigurationManager::configurationChanged(const QNetworkConfiguration &config)

    This signal is emitted when the \l {QNetworkConfiguration::state()}{state} of \a config changes.
*/

/*!
    \fn void QNetworkConfigurationManager::onlineStateChanged(bool isOnline)

    This signal is emitted when the device changes from online to offline mode or vice versa.
    \a isOnline represents the new state of the device.

    The state is considered to be online for as long as
    \l{allConfigurations()}{allConfigurations}(QNetworkConfiguration::Active) returns a list with
    at least one entry.
*/

/*!
    \enum QNetworkConfigurationManager::Capability

    Specifies the system capabilities of the bearer API. The possible values are:

    \value CanStartAndStopInterfaces Network sessions and their underlying access points can be
                                     started and stopped. If this flag is not set QNetworkSession
                                     can only monitor but not influence the state of access points.
                                     On some platforms this feature may require elevated user
                                     permissions. This option is platform specific and may not
                                     always be available.
    \value DirectConnectionRouting   Network sessions and their sockets can be bound to a
                                     particular network interface. Any packet that passes through
                                     the socket goes to the specified network interface and thus
                                     disregards standard routing table entries. This may be useful
                                     when two interfaces can reach overlapping IP ranges or an
                                     application has specific needs in regards to target networks.
                                     This option is platform specific and may not always be
                                     available.
    \value SystemSessionSupport      If this flag is set the underlying platform ensures that a
                                     network interface is not shut down until the last network
                                     session has been \l{QNetworkSession::close()}{closed()}. This
                                     works across multiple processes. If the platform session
                                     support is missing this API can only ensure the above behavior
                                     for network sessions within the same process.
                                     In general mobile platforms have such
                                     support whereas most desktop platform lack this capability.
    \value ApplicationLevelRoaming   The system gives applications control over the systems roaming
                                     behavior. Applications can initiate roaming (in case the
                                     current link is not suitable) and are consulted if the system
                                     has identified a more suitable access point.
    \value ForcedRoaming             The system disconnects an existing access point and reconnects
                                     via a more suitable one. The application does not have any
                                     control over this process and has to reconnect its active
                                     sockets.
    \value DataStatistics            If this flag is set QNetworkSession can provide statistics
                                     about transmitted and received data.
    \value NetworkSessionRequired    If this flag is set the platform requires that a network
                                     session is created before network operations can be performed.
*/

/*!
    Constructs a QNetworkConfigurationManager with the given \a parent.

    Note that to ensure a valid list of current configurations immediately available, updating
    is done during construction which causes some delay.
*/
QNetworkConfigurationManager::QNetworkConfigurationManager(QObject *parent)
    : QObject(parent)
{
    QNetworkConfigurationManagerPrivate *priv = qNetworkConfigurationManagerPrivate();

    connect(priv, SIGNAL(configurationAdded(QNetworkConfiguration)),
            this, SIGNAL(configurationAdded(QNetworkConfiguration)));
    connect(priv, SIGNAL(configurationRemoved(QNetworkConfiguration)),
            this, SIGNAL(configurationRemoved(QNetworkConfiguration)));
    connect(priv, SIGNAL(configurationChanged(QNetworkConfiguration)),
            this, SIGNAL(configurationChanged(QNetworkConfiguration)));
    connect(priv, SIGNAL(onlineStateChanged(bool)),
            this, SIGNAL(onlineStateChanged(bool)));
    connect(priv, SIGNAL(configurationUpdateComplete()),
            this, SIGNAL(updateCompleted()));

    priv->enablePolling();
}

/*!
    Frees the resources associated with the QNetworkConfigurationManager object.
*/
QNetworkConfigurationManager::~QNetworkConfigurationManager()
{
    QNetworkConfigurationManagerPrivate *priv = qNetworkConfigurationManagerPrivate();
    if (priv)
        priv->disablePolling();
}


/*!
    Returns the default configuration to be used. This function always returns a discovered
    configuration; otherwise an invalid configuration.

    In some cases it may be required to call updateConfigurations() and wait for the
    updateCompleted() signal before calling this function.

    \sa allConfigurations()
*/
QNetworkConfiguration QNetworkConfigurationManager::defaultConfiguration() const
{
    QNetworkConfigurationManagerPrivate *priv = qNetworkConfigurationManagerPrivate();
    if (priv)
        return priv->defaultConfiguration();

    return QNetworkConfiguration();
}

/*!
    Returns the list of configurations which comply with the given \a filter.

    By default this function returns all (defined and undefined) configurations.

    A wireless network with a particular SSID may only be accessible in a
    certain area despite the fact that the system has a valid configuration
    for it. Therefore the filter flag may be used to limit the list to
    discovered and possibly connected configurations only.

    If \a filter is set to zero this function returns all possible configurations.

    Note that this function returns the states for all configurations as they are known at
    the time of this function call. If for instance a configuration of type WLAN is defined
    the system may have to perform a WLAN scan in order to determine whether it is
    actually available. To obtain the most accurate state updateConfigurations() should
    be used to update each configuration's state. Note that such an update may require
    some time. It's completion is signalled by updateCompleted(). In the absence of a
    configuration update this function returns the best estimate at the time of the call.
    Therefore, if WLAN configurations are of interest, it is recommended that
    updateConfigurations() is called once after QNetworkConfigurationManager
    instantiation (WLAN scans are too time consuming to perform in constructor).
    After this the data is kept automatically up-to-date as the system reports
    any changes.
*/
QList<QNetworkConfiguration> QNetworkConfigurationManager::allConfigurations(QNetworkConfiguration::StateFlags filter) const
{
    QNetworkConfigurationManagerPrivate *priv = qNetworkConfigurationManagerPrivate();
    if (priv)
        return priv->allConfigurations(filter);

    return QList<QNetworkConfiguration>();
}

/*!
    Returns the QNetworkConfiguration for \a identifier; otherwise returns an
    invalid QNetworkConfiguration.

    \sa QNetworkConfiguration::identifier()
*/
QNetworkConfiguration QNetworkConfigurationManager::configurationFromIdentifier(const QString &identifier) const
{
    QNetworkConfigurationManagerPrivate *priv = qNetworkConfigurationManagerPrivate();
    if (priv)
        return priv->configurationFromIdentifier(identifier);

    return QNetworkConfiguration();
}

/*!
    Returns \c true if the system is considered to be connected to another device via an active
    network interface; otherwise returns \c false.

    This is equivalent to the following code snippet:

    \snippet code/src_network_bearer_qnetworkconfigmanager.cpp 0

    \sa onlineStateChanged()
*/
bool QNetworkConfigurationManager::isOnline() const
{
    QNetworkConfigurationManagerPrivate *priv = qNetworkConfigurationManagerPrivate();
    if (priv)
        return priv->isOnline();

    return false;
}

/*!
    Returns the capabilities supported by the current platform.
*/
QNetworkConfigurationManager::Capabilities QNetworkConfigurationManager::capabilities() const
{
    QNetworkConfigurationManagerPrivate *priv = qNetworkConfigurationManagerPrivate();
    if (priv)
        return priv->capabilities();

    return QNetworkConfigurationManager::Capabilities(0);
}

/*!
    Initiates an update of all configurations. This may be used to initiate WLAN scans or other
    time consuming updates which may be required to obtain the correct state for configurations.

    This call is asynchronous. On completion of this update the updateCompleted() signal is
    emitted. If new configurations are discovered or old ones were removed or changed the update
    process may trigger the emission of one or multiple configurationAdded(),
    configurationRemoved() and configurationChanged() signals.

    If a configuration state changes as a result of this update all existing QNetworkConfiguration
    instances are updated automatically.

    \sa allConfigurations()
*/
void QNetworkConfigurationManager::updateConfigurations()
{
    QNetworkConfigurationManagerPrivate *priv = qNetworkConfigurationManagerPrivate();
    if (priv)
        priv->performAsyncConfigurationUpdate();
}

QT_END_NAMESPACE

#include "moc_qnetworkconfigmanager.cpp"

#endif // QT_NO_BEARERMANAGEMENT
