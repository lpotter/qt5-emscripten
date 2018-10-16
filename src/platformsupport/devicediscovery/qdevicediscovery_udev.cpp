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

#include "qdevicediscovery_udev_p.h"

#include <QStringList>
#include <QCoreApplication>
#include <QObject>
#include <QHash>
#include <QSocketNotifier>
#include <QLoggingCategory>

#include <linux/input.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcDD, "qt.qpa.input")

QDeviceDiscovery *QDeviceDiscovery::create(QDeviceTypes types, QObject *parent)
{
    qCDebug(lcDD) << "udev device discovery for type" << types;

    QDeviceDiscovery *helper = 0;
    struct udev *udev;

    udev = udev_new();
    if (udev) {
        helper = new QDeviceDiscoveryUDev(types, udev, parent);
    } else {
        qWarning("Failed to get udev library context");
    }

    return helper;
}

QDeviceDiscoveryUDev::QDeviceDiscoveryUDev(QDeviceTypes types, struct udev *udev, QObject *parent) :
    QDeviceDiscovery(types, parent),
    m_udev(udev), m_udevMonitor(0), m_udevMonitorFileDescriptor(-1), m_udevSocketNotifier(0)
{
    if (!m_udev)
        return;

    m_udevMonitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_udevMonitor) {
        qWarning("Unable to create an udev monitor. No devices can be detected.");
        return;
    }

    udev_monitor_filter_add_match_subsystem_devtype(m_udevMonitor, "input", 0);
    udev_monitor_filter_add_match_subsystem_devtype(m_udevMonitor, "drm", 0);
    udev_monitor_enable_receiving(m_udevMonitor);
    m_udevMonitorFileDescriptor = udev_monitor_get_fd(m_udevMonitor);

    m_udevSocketNotifier = new QSocketNotifier(m_udevMonitorFileDescriptor, QSocketNotifier::Read, this);
    connect(m_udevSocketNotifier, SIGNAL(activated(int)), this, SLOT(handleUDevNotification()));
}

QDeviceDiscoveryUDev::~QDeviceDiscoveryUDev()
{
    if (m_udevMonitor)
        udev_monitor_unref(m_udevMonitor);

    if (m_udev)
        udev_unref(m_udev);
}

QStringList QDeviceDiscoveryUDev::scanConnectedDevices()
{
    QStringList devices;

    if (!m_udev)
        return devices;

    udev_enumerate *ue = udev_enumerate_new(m_udev);
    udev_enumerate_add_match_subsystem(ue, "input");
    udev_enumerate_add_match_subsystem(ue, "drm");

    if (m_types & Device_Mouse)
        udev_enumerate_add_match_property(ue, "ID_INPUT_MOUSE", "1");
    if (m_types & Device_Touchpad)
        udev_enumerate_add_match_property(ue, "ID_INPUT_TOUCHPAD", "1");
    if (m_types & Device_Touchscreen)
        udev_enumerate_add_match_property(ue, "ID_INPUT_TOUCHSCREEN", "1");
    if (m_types & Device_Keyboard) {
        udev_enumerate_add_match_property(ue, "ID_INPUT_KEYBOARD", "1");
        udev_enumerate_add_match_property(ue, "ID_INPUT_KEY", "1");
    }
    if (m_types & Device_Tablet)
        udev_enumerate_add_match_property(ue, "ID_INPUT_TABLET", "1");
    if (m_types & Device_Joystick)
        udev_enumerate_add_match_property(ue, "ID_INPUT_JOYSTICK", "1");

    if (udev_enumerate_scan_devices(ue) != 0) {
        qWarning("Failed to scan devices");
        return devices;
    }

    udev_list_entry *entry;
    udev_list_entry_foreach (entry, udev_enumerate_get_list_entry(ue)) {
        const char *syspath = udev_list_entry_get_name(entry);
        udev_device *udevice = udev_device_new_from_syspath(m_udev, syspath);
        QString candidate = QString::fromUtf8(udev_device_get_devnode(udevice));
        if ((m_types & Device_InputMask) && candidate.startsWith(QLatin1String(QT_EVDEV_DEVICE)))
            devices << candidate;
        if ((m_types & Device_VideoMask) && candidate.startsWith(QLatin1String(QT_DRM_DEVICE))) {
            if (m_types & Device_DRM_PrimaryGPU) {
                udev_device *pci = udev_device_get_parent_with_subsystem_devtype(udevice, "pci", 0);
                if (pci) {
                    if (qstrcmp(udev_device_get_sysattr_value(pci, "boot_vga"), "1") == 0)
                        devices << candidate;
                }
            } else
                devices << candidate;
        }

        udev_device_unref(udevice);
    }
    udev_enumerate_unref(ue);

    qCDebug(lcDD) << "Found matching devices" << devices;

    return devices;
}

void QDeviceDiscoveryUDev::handleUDevNotification()
{
    if (!m_udevMonitor)
        return;

    struct udev_device *dev;
    QString devNode;

    dev = udev_monitor_receive_device(m_udevMonitor);
    if (!dev)
        goto cleanup;

    const char *action;
    action = udev_device_get_action(dev);
    if (!action)
        goto cleanup;

    const char *str;
    str = udev_device_get_devnode(dev);
    if (!str)
        goto cleanup;

    const char *subsystem;
    devNode = QString::fromUtf8(str);
    if (devNode.startsWith(QLatin1String(QT_EVDEV_DEVICE)))
        subsystem = "input";
    else if (devNode.startsWith(QLatin1String(QT_DRM_DEVICE)))
        subsystem = "drm";
    else goto cleanup;

    // if we cannot determine a type, walk up the device tree
    if (!checkDeviceType(dev)) {
        // does not increase the refcount
        struct udev_device *parent_dev = udev_device_get_parent_with_subsystem_devtype(dev, subsystem, 0);
        if (!parent_dev)
            goto cleanup;

        if (!checkDeviceType(parent_dev))
            goto cleanup;
    }

    if (qstrcmp(action, "add") == 0)
        emit deviceDetected(devNode);

    if (qstrcmp(action, "remove") == 0)
        emit deviceRemoved(devNode);

cleanup:
    udev_device_unref(dev);
}

bool QDeviceDiscoveryUDev::checkDeviceType(udev_device *dev)
{
    if (!dev)
        return false;

    if ((m_types & Device_Keyboard) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD"), "1") == 0 )) {
        const QString capabilities_key = QString::fromUtf8(udev_device_get_sysattr_value(dev, "capabilities/key"));
        const auto val = capabilities_key.splitRef(QLatin1Char(' '), QString::SkipEmptyParts);
        if (!val.isEmpty()) {
            bool ok;
            unsigned long long keys = val.last().toULongLong(&ok, 16);
            if (ok) {
                // Tests if the letter Q is valid for the device.  We may want to alter this test, but it seems mostly reliable.
                bool test = (keys >> KEY_Q) & 1;
                if (test)
                    return true;
            }
        }
    }

    if ((m_types & Device_Keyboard) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_KEY"), "1") == 0 ))
        return true;

    if ((m_types & Device_Mouse) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_MOUSE"), "1") == 0))
        return true;

    if ((m_types & Device_Touchpad) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_TOUCHPAD"), "1") == 0))
        return true;

    if ((m_types & Device_Touchscreen) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_TOUCHSCREEN"), "1") == 0))
        return true;

    if ((m_types & Device_Tablet) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_TABLET"), "1") == 0))
        return true;

    if ((m_types & Device_Joystick) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_JOYSTICK"), "1") == 0))
        return true;

    if ((m_types & Device_DRM) && (qstrcmp(udev_device_get_subsystem(dev), "drm") == 0))
        return true;

    return false;
}

QT_END_NAMESPACE
