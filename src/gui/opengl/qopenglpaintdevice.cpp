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

#include <qopenglpaintdevice.h>
#include <qpaintengine.h>
#include <qthreadstorage.h>

#include <private/qopenglpaintdevice_p.h>
#include <private/qobject_p.h>
#include <private/qopenglcontext_p.h>
#include <private/qopenglframebufferobject_p.h>
#include <private/qopenglpaintengine_p.h>

// for qt_defaultDpiX/Y
#include <private/qfont_p.h>

#include <qopenglfunctions.h>

QT_BEGIN_NAMESPACE

/*!
    \class QOpenGLPaintDevice
    \brief The QOpenGLPaintDevice class enables painting to an OpenGL context using QPainter.
    \since 5.0
    \inmodule QtGui

    \ingroup painting-3D

    The QOpenGLPaintDevice uses the \b current QOpenGL context to render
    QPainter draw commands. The context is captured upon construction. It
    requires support for OpenGL (ES) 2.0 or higher.

    \section1 Performance

    The QOpenGLPaintDevice is almost always hardware accelerated and
    has the potential of being much faster than software
    rasterization. However, it is more sensitive to state changes, and
    therefore requires the drawing commands to be carefully ordered to
    achieve optimal performance.

    \section1 Antialiasing and Quality

    Antialiasing in the OpenGL paint engine is done using
    multisampling. Most hardware require significantly more memory to
    do multisampling and the resulting quality is not on par with the
    quality of the software paint engine. The OpenGL paint engine's
    strength lies in its performance, not its visual rendering
    quality.

    \section1 State Changes

    When painting to a QOpenGLPaintDevice using QPainter, the state of
    the current OpenGL context will be altered by the paint engine to
    reflect its needs.  Applications should not rely upon the OpenGL
    state being reset to its original conditions, particularly the
    current shader program, OpenGL viewport, texture units, and
    drawing modes.

    \section1 Mixing QPainter and OpenGL

    When intermixing QPainter and OpenGL, it is important to notify
    QPainter that the OpenGL state may have been cluttered so it can
    restore its internal state. This is achieved by calling \l
    QPainter::beginNativePainting() before starting the OpenGL
    rendering and calling \l QPainter::endNativePainting() after
    finishing.

    \sa {OpenGL Window Example}

*/

/*!
    Constructs a QOpenGLPaintDevice.

    The QOpenGLPaintDevice is only valid for the current context.

    \sa QOpenGLContext::currentContext()
*/
QOpenGLPaintDevice::QOpenGLPaintDevice()
    : d_ptr(new QOpenGLPaintDevicePrivate(QSize()))
{
}

/*!
    Constructs a QOpenGLPaintDevice with the given \a size.

    The QOpenGLPaintDevice is only valid for the current context.

    \sa QOpenGLContext::currentContext()
*/
QOpenGLPaintDevice::QOpenGLPaintDevice(const QSize &size)
    : d_ptr(new QOpenGLPaintDevicePrivate(size))
{
}

/*!
    Constructs a QOpenGLPaintDevice with the given \a width and \a height.

    The QOpenGLPaintDevice is only valid for the current context.

    \sa QOpenGLContext::currentContext()
*/
QOpenGLPaintDevice::QOpenGLPaintDevice(int width, int height)
    : QOpenGLPaintDevice(QSize(width, height))
{
}

/*!
    \internal
 */
QOpenGLPaintDevice::QOpenGLPaintDevice(QOpenGLPaintDevicePrivate &dd)
    : d_ptr(&dd)
{
}

/*!
    Destroys the QOpenGLPaintDevice.
*/

QOpenGLPaintDevice::~QOpenGLPaintDevice()
{
    delete d_ptr->engine;
}

/*!
    \fn int QOpenGLPaintDevice::devType() const
    \internal
    \reimp
*/

QOpenGLPaintDevicePrivate::QOpenGLPaintDevicePrivate(const QSize &sz)
    : size(sz)
    , ctx(QOpenGLContext::currentContext())
    , dpmx(qt_defaultDpiX() * 100. / 2.54)
    , dpmy(qt_defaultDpiY() * 100. / 2.54)
    , devicePixelRatio(1.0)
    , flipped(false)
    , engine(0)
{
}

QOpenGLPaintDevicePrivate::~QOpenGLPaintDevicePrivate()
{
}

class QOpenGLEngineThreadStorage
{
public:
    QPaintEngine *engine() {
        QPaintEngine *&localEngine = storage.localData();
        if (!localEngine)
            localEngine = new QOpenGL2PaintEngineEx;
        return localEngine;
    }

private:
    QThreadStorage<QPaintEngine *> storage;
};

Q_GLOBAL_STATIC(QOpenGLEngineThreadStorage, qt_opengl_engine)

/*!
    \reimp
*/

QPaintEngine *QOpenGLPaintDevice::paintEngine() const
{
    if (d_ptr->engine)
        return d_ptr->engine;

    QPaintEngine *engine = qt_opengl_engine()->engine();
    if (engine->isActive() && engine->paintDevice() != this) {
        d_ptr->engine = new QOpenGL2PaintEngineEx;
        return d_ptr->engine;
    }

    return engine;
}

/*!
    Returns the OpenGL context associated with the paint device.
*/

QOpenGLContext *QOpenGLPaintDevice::context() const
{
    return d_ptr->ctx;
}

/*!
    Returns the pixel size of the paint device.

    \sa setSize()
*/

QSize QOpenGLPaintDevice::size() const
{
    return d_ptr->size;
}

/*!
    Sets the pixel size of the paint device to \a size.

    \sa size()
*/

void QOpenGLPaintDevice::setSize(const QSize &size)
{
    d_ptr->size = size;
}

/*!
    Sets the device pixel ratio for the paint device to \a devicePixelRatio.
*/
void QOpenGLPaintDevice::setDevicePixelRatio(qreal devicePixelRatio)
{
    d_ptr->devicePixelRatio = devicePixelRatio;
}

/*!
    \reimp
*/

int QOpenGLPaintDevice::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    switch (metric) {
    case PdmWidth:
        return d_ptr->size.width();
    case PdmHeight:
        return d_ptr->size.height();
    case PdmDepth:
        return 32;
    case PdmWidthMM:
        return qRound(d_ptr->size.width() * 1000 / d_ptr->dpmx);
    case PdmHeightMM:
        return qRound(d_ptr->size.height() * 1000 / d_ptr->dpmy);
    case PdmNumColors:
        return 0;
    case PdmDpiX:
        return qRound(d_ptr->dpmx * 0.0254);
    case PdmDpiY:
        return qRound(d_ptr->dpmy * 0.0254);
    case PdmPhysicalDpiX:
        return qRound(d_ptr->dpmx * 0.0254);
    case PdmPhysicalDpiY:
        return qRound(d_ptr->dpmy * 0.0254);
    case PdmDevicePixelRatio:
        return d_ptr->devicePixelRatio;
    case PdmDevicePixelRatioScaled:
        return d_ptr->devicePixelRatio * QPaintDevice::devicePixelRatioFScale();

    default:
        qWarning("QOpenGLPaintDevice::metric() - metric %d not known", metric);
        return 0;
    }
}

/*!
    Returns the number of pixels per meter horizontally.

    \sa setDotsPerMeterX()
*/

qreal QOpenGLPaintDevice::dotsPerMeterX() const
{
    return d_ptr->dpmx;
}

/*!
    Returns the number of pixels per meter vertically.

    \sa setDotsPerMeterY()
*/

qreal QOpenGLPaintDevice::dotsPerMeterY() const
{
    return d_ptr->dpmy;
}

/*!
    Sets the number of pixels per meter horizontally to \a dpmx.

    \sa dotsPerMeterX()
*/

void QOpenGLPaintDevice::setDotsPerMeterX(qreal dpmx)
{
    d_ptr->dpmx = dpmx;
}

/*!
    Sets the number of pixels per meter vertically to \a dpmy.

    \sa dotsPerMeterY()
*/

void QOpenGLPaintDevice::setDotsPerMeterY(qreal dpmy)
{
    d_ptr->dpmx = dpmy;
}

/*!
    Sets whether painting should be flipped around the Y-axis or not to \a flipped.

    \sa paintFlipped()
*/
void QOpenGLPaintDevice::setPaintFlipped(bool flipped)
{
    d_ptr->flipped = flipped;
}

/*!
    Returns \c true if painting is flipped around the Y-axis.

    \sa setPaintFlipped()
*/

bool QOpenGLPaintDevice::paintFlipped() const
{
    return d_ptr->flipped;
}

/*!
    This virtual method is provided as a callback to allow re-binding a target
    frame buffer object or context when different QOpenGLPaintDevice instances
    are issuing draw calls alternately.

    \l{QPainter::beginNativePainting()}{beginNativePainting()} will also trigger
    this method.

    The default implementation does nothing.
*/
void QOpenGLPaintDevice::ensureActiveTarget()
{
}

QT_END_NAMESPACE
