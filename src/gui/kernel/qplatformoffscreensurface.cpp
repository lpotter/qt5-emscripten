/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qplatformoffscreensurface.h"

#include "qoffscreensurface.h"
#include "qscreen.h"

QT_BEGIN_NAMESPACE

class QPlatformOffscreenSurfacePrivate
{
public:
};

QPlatformOffscreenSurface::QPlatformOffscreenSurface(QOffscreenSurface *offscreenSurface)
    : QPlatformSurface(offscreenSurface)
    , d_ptr(new QPlatformOffscreenSurfacePrivate)
{
}

QPlatformOffscreenSurface::~QPlatformOffscreenSurface()
{
}

QOffscreenSurface *QPlatformOffscreenSurface::offscreenSurface() const
{
    return static_cast<QOffscreenSurface*>(m_surface);
}

/*!
    Returns the platform screen handle corresponding to this QPlatformOffscreenSurface.
*/
QPlatformScreen *QPlatformOffscreenSurface::screen() const
{
    return offscreenSurface()->screen()->handle();
}

/*!
    Returns the actual surface format of the offscreen surface.
*/
QSurfaceFormat QPlatformOffscreenSurface::format() const
{
    return QSurfaceFormat();
}

/*!
    Returns \c true if the platform offscreen surface has been allocated.
*/
bool QPlatformOffscreenSurface::isValid() const
{
    return false;
}

QT_END_NAMESPACE
