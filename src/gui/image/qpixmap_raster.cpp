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

#include "qpixmap.h"

#include <private/qfont_p.h>

#include "qpixmap_raster_p.h"
#include "qimage_p.h"
#include "qpaintengine.h"

#include "qbitmap.h"
#include "qimage.h"
#include <QBuffer>
#include <QImageReader>
#include <QGuiApplication>
#include <QScreen>
#include <private/qsimd_p.h>
#include <private/qdrawhelper_p.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

QPixmap qt_toRasterPixmap(const QImage &image)
{
    QPlatformPixmap *data =
        new QRasterPlatformPixmap(image.depth() == 1
                           ? QPlatformPixmap::BitmapType
                           : QPlatformPixmap::PixmapType);

    data->fromImage(image, Qt::AutoColor);

    return QPixmap(data);
}

QPixmap qt_toRasterPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull())
        return QPixmap();

    if (QPixmap(pixmap).data_ptr()->classId() == QPlatformPixmap::RasterClass)
        return pixmap;

    return qt_toRasterPixmap(pixmap.toImage());
}

QRasterPlatformPixmap::QRasterPlatformPixmap(PixelType type)
    : QPlatformPixmap(type, RasterClass)
{
}

QRasterPlatformPixmap::~QRasterPlatformPixmap()
{
}

QImage::Format QRasterPlatformPixmap::systemNativeFormat()
{
    if (!QGuiApplication::primaryScreen())
        return QImage::Format_RGB32;
    return QGuiApplication::primaryScreen()->handle()->format();
}

QPlatformPixmap *QRasterPlatformPixmap::createCompatiblePlatformPixmap() const
{
    return new QRasterPlatformPixmap(pixelType());
}

void QRasterPlatformPixmap::resize(int width, int height)
{
    QImage::Format format;
    if (pixelType() == BitmapType)
        format = QImage::Format_MonoLSB;
    else
        format = systemNativeFormat();

    image = QImage(width, height, format);
    w = width;
    h = height;
    d = image.depth();
    is_null = (w <= 0 || h <= 0);

    if (pixelType() == BitmapType && !image.isNull()) {
        image.setColorCount(2);
        image.setColor(0, QColor(Qt::color0).rgba());
        image.setColor(1, QColor(Qt::color1).rgba());
    }

    setSerialNumber(image.cacheKey() >> 32);
}

bool QRasterPlatformPixmap::fromData(const uchar *buffer, uint len, const char *format,
                      Qt::ImageConversionFlags flags)
{
    QByteArray a = QByteArray::fromRawData(reinterpret_cast<const char *>(buffer), len);
    QBuffer b(&a);
    b.open(QIODevice::ReadOnly);
    QImage image = QImageReader(&b, format).read();
    if (image.isNull())
        return false;

    createPixmapForImage(std::move(image), flags);
    return !isNull();
}

void QRasterPlatformPixmap::fromImage(const QImage &sourceImage,
                                  Qt::ImageConversionFlags flags)
{
    QImage image = sourceImage;
    createPixmapForImage(std::move(image), flags);
}

void QRasterPlatformPixmap::fromImageInPlace(QImage &sourceImage,
                                             Qt::ImageConversionFlags flags)
{
    createPixmapForImage(std::move(sourceImage), flags);
}

void QRasterPlatformPixmap::fromImageReader(QImageReader *imageReader,
                                        Qt::ImageConversionFlags flags)
{
    Q_UNUSED(flags);
    QImage image = imageReader->read();
    if (image.isNull())
        return;

    createPixmapForImage(std::move(image), flags);
}

// from qbackingstore.cpp
extern void qt_scrollRectInImage(QImage &img, const QRect &rect, const QPoint &offset);

void QRasterPlatformPixmap::copy(const QPlatformPixmap *data, const QRect &rect)
{
    fromImage(data->toImage(rect).copy(), Qt::NoOpaqueDetection);
}

bool QRasterPlatformPixmap::scroll(int dx, int dy, const QRect &rect)
{
    if (!image.isNull())
        qt_scrollRectInImage(image, rect, QPoint(dx, dy));
    return true;
}

void QRasterPlatformPixmap::fill(const QColor &color)
{
    uint pixel;

    if (image.depth() == 1) {
        int gray = qGray(color.rgba());
        // Pick the best approximate color in the image's colortable.
        if (qAbs(qGray(image.color(0)) - gray) < qAbs(qGray(image.color(1)) - gray))
            pixel = 0;
        else
            pixel = 1;
    } else if (image.depth() >= 15) {
        int alpha = color.alpha();
        if (alpha != 255) {
            if (!image.hasAlphaChannel()) {
                QImage::Format toFormat = qt_alphaVersionForPainting(image.format());
                if (!image.reinterpretAsFormat(toFormat))
                    image = QImage(image.width(), image.height(), toFormat);
            }
        }
        image.fill(color);
        return;
    } else if (image.format() == QImage::Format_Alpha8) {
        pixel = qAlpha(color.rgba());
    } else if (image.format() == QImage::Format_Grayscale8) {
        pixel = qGray(color.rgba());
    } else
    {
        pixel = 0;
        // ### what about 8 bit indexed?
    }

    image.fill(pixel);
}

bool QRasterPlatformPixmap::hasAlphaChannel() const
{
    return image.hasAlphaChannel();
}

QImage QRasterPlatformPixmap::toImage() const
{
    if (!image.isNull()) {
        QImageData *data = const_cast<QImage &>(image).data_ptr();
        if (data->paintEngine && data->paintEngine->isActive()
            && data->paintEngine->paintDevice() == &image)
        {
            return image.copy();
        }
    }

    return image;
}

QImage QRasterPlatformPixmap::toImage(const QRect &rect) const
{
    if (rect.isNull())
        return image;

    QRect clipped = rect.intersected(QRect(0, 0, w, h));
    const uint du = uint(d);
    if ((du % 8 == 0) && (((uint(clipped.x()) * du)) % 32 == 0)) {
        QImage newImage(image.scanLine(clipped.y()) + clipped.x() * (du / 8),
                      clipped.width(), clipped.height(),
                      image.bytesPerLine(), image.format());
        newImage.setDevicePixelRatio(image.devicePixelRatio());
        return newImage;
    } else {
        return image.copy(clipped);
    }
}

QPaintEngine* QRasterPlatformPixmap::paintEngine() const
{
    return image.paintEngine();
}

int QRasterPlatformPixmap::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    QImageData *d = image.d;
    if (!d)
        return 0;

    // override the image dpi with the screen dpi when rendering to a pixmap
    switch (metric) {
    case QPaintDevice::PdmWidth:
        return w;
    case QPaintDevice::PdmHeight:
        return h;
    case QPaintDevice::PdmWidthMM:
        return qRound(d->width * 25.4 / qt_defaultDpiX());
    case QPaintDevice::PdmHeightMM:
        return qRound(d->height * 25.4 / qt_defaultDpiY());
    case QPaintDevice::PdmNumColors:
        return d->colortable.size();
    case QPaintDevice::PdmDepth:
        return this->d;
    case QPaintDevice::PdmDpiX:
        return qt_defaultDpiX();
    case QPaintDevice::PdmPhysicalDpiX:
        return qt_defaultDpiX();
    case QPaintDevice::PdmDpiY:
        return qt_defaultDpiY();
    case QPaintDevice::PdmPhysicalDpiY:
        return qt_defaultDpiY();
    case QPaintDevice::PdmDevicePixelRatio:
        return image.devicePixelRatio();
    case QPaintDevice::PdmDevicePixelRatioScaled:
        return image.devicePixelRatio() * QPaintDevice::devicePixelRatioFScale();

    default:
        qWarning("QRasterPlatformPixmap::metric(): Unhandled metric type %d", metric);
        break;
    }

    return 0;
}

void QRasterPlatformPixmap::createPixmapForImage(QImage sourceImage, Qt::ImageConversionFlags flags)
{
    QImage::Format format;
    if (flags & Qt::NoFormatConversion)
        format = sourceImage.format();
    else
    if (pixelType() == BitmapType) {
        format = QImage::Format_MonoLSB;
    } else {
        if (sourceImage.depth() == 1) {
            format = sourceImage.hasAlphaChannel()
                    ? QImage::Format_ARGB32_Premultiplied
                    : QImage::Format_RGB32;
        } else {
            QImage::Format nativeFormat = systemNativeFormat();
            QImage::Format opaqueFormat = qt_opaqueVersionForPainting(nativeFormat);
            QImage::Format alphaFormat = qt_alphaVersionForPainting(nativeFormat);

            if (!sourceImage.hasAlphaChannel()) {
                format = opaqueFormat;
            } else if ((flags & Qt::NoOpaqueDetection) == 0
                       && !sourceImage.data_ptr()->checkForAlphaPixels())
            {
                format = opaqueFormat;
            } else {
                format = alphaFormat;
            }
        }
    }

    // image has alpha format but is really opaque, so try to do a
    // more efficient conversion
    if (format == QImage::Format_RGB32 && (sourceImage.format() == QImage::Format_ARGB32
        || sourceImage.format() == QImage::Format_ARGB32_Premultiplied))
    {
        image = std::move(sourceImage);
        image.reinterpretAsFormat(QImage::Format_RGB32);
    } else {
        image = std::move(sourceImage).convertToFormat(format, flags);
    }

    if (image.d) {
        w = image.d->width;
        h = image.d->height;
        d = image.d->depth;
    } else {
        w = h = d = 0;
    }
    is_null = (w <= 0 || h <= 0);

    //ensure the pixmap and the image resulting from toImage() have the same cacheKey();
    setSerialNumber(image.cacheKey() >> 32);
    if (image.d)
        setDetachNumber(image.d->detach_no);
}

QImage* QRasterPlatformPixmap::buffer()
{
    return &image;
}

qreal QRasterPlatformPixmap::devicePixelRatio() const
{
    return image.devicePixelRatio();
}

void QRasterPlatformPixmap::setDevicePixelRatio(qreal scaleFactor)
{
    image.setDevicePixelRatio(scaleFactor);
}

QT_END_NAMESPACE
