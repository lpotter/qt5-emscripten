/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <QtCore/qglobal.h>

#ifndef QT_NO_ACCESSIBILITY

#include "qaccessibleplugin.h"
#include "qaccessible.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAccessiblePlugin
    \brief The QAccessiblePlugin class provides an abstract base class
    for plugins provinding accessibility information for user interface elements.

    \ingroup plugins
    \ingroup accessibility

    Writing an accessibility plugin is achieved by subclassing this
    base class, reimplementing the pure virtual function create(),
    and exporting the class with the Q_PLUGIN_METADATA() macro.

    \sa {How to Create Qt Plugins}
*/

/*!
    Constructs an accessibility plugin with the given \a parent. This
    is invoked automatically by the plugin loader.
*/
QAccessiblePlugin::QAccessiblePlugin(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys the accessibility plugin.

    You never have to call this explicitly. Qt destroys a plugin
    automatically when it is no longer used.
*/
QAccessiblePlugin::~QAccessiblePlugin()
{
}

/*!
    \fn QAccessibleInterface *QAccessiblePlugin::create(const QString &key, QObject *object)

    Creates and returns a QAccessibleInterface implementation for the
    class \a key and the object \a object. Keys are case sensitive.
*/

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
