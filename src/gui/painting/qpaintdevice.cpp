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

#include "qpaintdevice.h"

QT_BEGIN_NAMESPACE

QPaintDevice::QPaintDevice() Q_DECL_NOEXCEPT
{
    reserved = 0;
    painters = 0;
}

QPaintDevice::~QPaintDevice()
{
    if (paintingActive())
        qWarning("QPaintDevice: Cannot destroy paint device that is being "
                  "painted");
}


/*!
    \internal
*/
void QPaintDevice::initPainter(QPainter *) const
{
}

/*!
    \internal
*/
QPaintDevice *QPaintDevice::redirected(QPoint *) const
{
    return 0;
}

/*!
    \internal
*/
QPainter *QPaintDevice::sharedPainter() const
{
    return 0;
}

Q_GUI_EXPORT int qt_paint_device_metric(const QPaintDevice *device, QPaintDevice::PaintDeviceMetric metric)
{
    return device->metric(metric);
}

int QPaintDevice::metric(PaintDeviceMetric m) const
{
    // Fallback: A subclass has not implemented PdmDevicePixelRatioScaled but might
    // have implemented PdmDevicePixelRatio.
    if (m == PdmDevicePixelRatioScaled)
        return this->metric(PdmDevicePixelRatio) * devicePixelRatioFScale();

    qWarning("QPaintDevice::metrics: Device has no metric information");

    if (m == PdmDpiX) {
        return 72;
    } else if (m == PdmDpiY) {
        return 72;
    } else if (m == PdmNumColors) {
        // FIXME: does this need to be a real value?
        return 256;
    } else if (m == PdmDevicePixelRatio) {
        return 1;
    } else {
        qDebug("Unrecognised metric %d!",m);
        return 0;
    }
}

QT_END_NAMESPACE
