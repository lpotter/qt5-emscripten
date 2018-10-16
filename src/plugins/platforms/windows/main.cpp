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


#include <qpa/qplatformintegrationplugin.h>
#include <QtCore/qstringlist.h>

#include "qwindowsgdiintegration.h"

QT_BEGIN_NAMESPACE

/*!
    \group qt-lighthouse-win
    \title Qt Lighthouse plugin for Windows

    \brief Class documentation of the  Qt Lighthouse plugin for Windows.

    \section1 Supported Parameters

    The following parameters can be passed on to the -platform argument
    of QGuiApplication:

    \list
    \li \c fontengine=native Indicates that native font engine should be used (default)
    \li \c fontengine=freetype Indicates that freetype font engine should be used
    \li \c gl=gdi Indicates that ARB Open GL functionality should not be used
    \endlist

    \section1 Tips

    \list
    \li The environment variable \c QT_QPA_VERBOSE controls
       the debug level. It takes the form
       \c{<keyword1>:<level1>,<keyword2>:<level2>}, where
       keyword is one of \c integration, \c windows, \c backingstore and
       \c fonts. Level is an integer 0..9.
    \endlist
    \internal
 */

/*!
    \class QWindowsIntegrationPlugin
    \brief Plugin.
    \internal
    \ingroup qt-lighthouse-win
 */

/*!
    \namespace QtWindows

    \brief Namespace for enumerations, etc.
    \internal
    \ingroup qt-lighthouse-win
*/

/*!
    \enum QtWindows::WindowsEventType

    \brief Enumerations for WM_XX events.

    With flags that should help to structure the code.

    \internal
    \ingroup qt-lighthouse-win
*/

class QWindowsIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "windows.json")
public:
    QPlatformIntegration *create(const QString&, const QStringList&, int &, char **);
};

QPlatformIntegration *QWindowsIntegrationPlugin::create(const QString& system, const QStringList& paramList, int &, char **)
{
    if (system.compare(system, QLatin1String("windows"), Qt::CaseInsensitive) == 0)
        return new QWindowsGdiIntegration(paramList);
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
