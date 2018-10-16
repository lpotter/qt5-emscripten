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

/*!
    \class QImageWriter
    \brief The QImageWriter class provides a format independent interface
    for writing images to files or other devices.

    \inmodule QtGui
    \reentrant
    \ingroup painting
    \ingroup io

    QImageWriter supports setting format specific options, such as the
    gamma level, compression level and quality, prior to storing the
    image. If you do not need such options, you can use QImage::save()
    or QPixmap::save() instead.

    To store an image, you start by constructing a QImageWriter
    object.  Pass either a file name or a device pointer, and the
    image format to QImageWriter's constructor. You can then set
    several options, such as the gamma level (by calling setGamma())
    and quality (by calling setQuality()). canWrite() returns \c true if
    QImageWriter can write the image (i.e., the image format is
    supported and the device is open for writing). Call write() to
    write the image to the device.

    If any error occurs when writing the image, write() will return
    false. You can then call error() to find the type of error that
    occurred, or errorString() to get a human readable description of
    what went wrong.

    Call supportedImageFormats() for a list of formats that
    QImageWriter can write. QImageWriter supports all built-in image
    formats, in addition to any image format plugins that support
    writing.

    \sa QImageReader, QImageIOHandler, QImageIOPlugin
*/

/*!
    \enum QImageWriter::ImageWriterError

    This enum describes errors that can occur when writing images with
    QImageWriter.

    \value DeviceError QImageWriter encountered a device error when
    writing the image data. Consult your device for more details on
    what went wrong.

    \value UnsupportedFormatError Qt does not support the requested
    image format.

    \value InvalidImageError An attempt was made to write an invalid QImage. An
    example of an invalid image would be a null QImage.

    \value UnknownError An unknown error occurred. If you get this
    value after calling write(), it is most likely caused by a bug in
    QImageWriter.
*/

#include "qimagewriter.h"

#include <qbytearray.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qimageiohandler.h>
#include <qset.h>
#include <qvariant.h>

// factory loader
#include <qcoreapplication.h>
#include <private/qfactoryloader_p.h>

// image handlers
#include <private/qbmphandler_p.h>
#include <private/qppmhandler_p.h>
#include <private/qxbmhandler_p.h>
#include <private/qxpmhandler_p.h>
#ifndef QT_NO_IMAGEFORMAT_PNG
#include <private/qpnghandler_p.h>
#endif

#include <private/qimagereaderwriterhelpers_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

static QImageIOHandler *createWriteHandlerHelper(QIODevice *device,
    const QByteArray &format)
{
    QByteArray form = format.toLower();
    QByteArray suffix;
    QImageIOHandler *handler = 0;

#ifndef QT_NO_IMAGEFORMATPLUGIN
    typedef QMultiMap<int, QString> PluginKeyMap;

    // check if any plugins can write the image
    auto l = QImageReaderWriterHelpers::pluginLoader();
    const PluginKeyMap keyMap = l->keyMap();
    int suffixPluginIndex = -1;
#endif

    if (device && format.isEmpty()) {
        // if there's no format, see if \a device is a file, and if so, find
        // the file suffix and find support for that format among our plugins.
        // this allows plugins to override our built-in handlers.
        if (QFile *file = qobject_cast<QFile *>(device)) {
            if (!(suffix = QFileInfo(file->fileName()).suffix().toLower().toLatin1()).isEmpty()) {
#ifndef QT_NO_IMAGEFORMATPLUGIN
                const int index = keyMap.key(QString::fromLatin1(suffix), -1);
                if (index != -1)
                    suffixPluginIndex = index;
#endif
            }
        }
    }

    QByteArray testFormat = !form.isEmpty() ? form : suffix;

#ifndef QT_NO_IMAGEFORMATPLUGIN
    if (suffixPluginIndex != -1) {
        // when format is missing, check if we can find a plugin for the
        // suffix.
        const int index = keyMap.key(QString::fromLatin1(suffix), -1);
        if (index != -1) {
            QImageIOPlugin *plugin = qobject_cast<QImageIOPlugin *>(l->instance(index));
            if (plugin && (plugin->capabilities(device, suffix) & QImageIOPlugin::CanWrite))
                handler = plugin->create(device, suffix);
        }
    }
#endif // QT_NO_IMAGEFORMATPLUGIN

    // check if any built-in handlers can write the image
    if (!handler && !testFormat.isEmpty()) {
        if (false) {
#ifndef QT_NO_IMAGEFORMAT_PNG
        } else if (testFormat == "png") {
            handler = new QPngHandler;
#endif
#ifndef QT_NO_IMAGEFORMAT_BMP
        } else if (testFormat == "bmp") {
            handler = new QBmpHandler;
        } else if (testFormat == "dib") {
            handler = new QBmpHandler(QBmpHandler::DibFormat);
#endif
#ifndef QT_NO_IMAGEFORMAT_XPM
        } else if (testFormat == "xpm") {
            handler = new QXpmHandler;
#endif
#ifndef QT_NO_IMAGEFORMAT_XBM
        } else if (testFormat == "xbm") {
            handler = new QXbmHandler;
            handler->setOption(QImageIOHandler::SubType, testFormat);
#endif
#ifndef QT_NO_IMAGEFORMAT_PPM
        } else if (testFormat == "pbm" || testFormat == "pbmraw" || testFormat == "pgm"
                 || testFormat == "pgmraw" || testFormat == "ppm" || testFormat == "ppmraw") {
            handler = new QPpmHandler;
            handler->setOption(QImageIOHandler::SubType, testFormat);
#endif
        }
    }

#ifndef QT_NO_IMAGEFORMATPLUGIN
    if (!testFormat.isEmpty()) {
        const int keyCount = keyMap.size();
        for (int i = 0; i < keyCount; ++i) {
            QImageIOPlugin *plugin = qobject_cast<QImageIOPlugin *>(l->instance(i));
            if (plugin && (plugin->capabilities(device, testFormat) & QImageIOPlugin::CanWrite)) {
                delete handler;
                handler = plugin->create(device, testFormat);
                break;
            }
        }
    }
#endif // QT_NO_IMAGEFORMATPLUGIN

    if (!handler)
        return 0;

    handler->setDevice(device);
    if (!testFormat.isEmpty())
        handler->setFormat(testFormat);
    return handler;
}

class QImageWriterPrivate
{
public:
    QImageWriterPrivate(QImageWriter *qq);

    bool canWriteHelper();

    // device
    QByteArray format;
    QIODevice *device;
    bool deleteDevice;
    QImageIOHandler *handler;

    // image options
    int quality;
    int compression;
    float gamma;
    QString description;
    QString text;
    QByteArray subType;
    bool optimizedWrite;
    bool progressiveScanWrite;
    QImageIOHandler::Transformations transformation;

    // error
    QImageWriter::ImageWriterError imageWriterError;
    QString errorString;

    QImageWriter *q;
};

/*!
    \internal
*/
QImageWriterPrivate::QImageWriterPrivate(QImageWriter *qq)
{
    device = 0;
    deleteDevice = false;
    handler = 0;
    quality = -1;
    compression = -1;
    gamma = 0.0;
    optimizedWrite = false;
    progressiveScanWrite = false;
    imageWriterError = QImageWriter::UnknownError;
    errorString = QImageWriter::tr("Unknown error");
    transformation = QImageIOHandler::TransformationNone;

    q = qq;
}

bool QImageWriterPrivate::canWriteHelper()
{
    if (!device) {
        imageWriterError = QImageWriter::DeviceError;
        errorString = QImageWriter::tr("Device is not set");
        return false;
    }
    if (!device->isOpen()) {
        if (!device->open(QIODevice::WriteOnly)) {
            imageWriterError = QImageWriter::DeviceError;
            errorString = QImageWriter::tr("Cannot open device for writing: %1").arg(device->errorString());
            return false;
        }
    }
    if (!device->isWritable()) {
        imageWriterError = QImageWriter::DeviceError;
        errorString = QImageWriter::tr("Device not writable");
        return false;
    }
    if (!handler && (handler = createWriteHandlerHelper(device, format)) == 0) {
        imageWriterError = QImageWriter::UnsupportedFormatError;
        errorString = QImageWriter::tr("Unsupported image format");
        return false;
    }
    return true;
}

/*!
    Constructs an empty QImageWriter object. Before writing, you must
    call setFormat() to set an image format, then setDevice() or
    setFileName().
*/
QImageWriter::QImageWriter()
    : d(new QImageWriterPrivate(this))
{
}

/*!
    Constructs a QImageWriter object using the device \a device and
    image format \a format.
*/
QImageWriter::QImageWriter(QIODevice *device, const QByteArray &format)
    : d(new QImageWriterPrivate(this))
{
    d->device = device;
    d->format = format;
}

/*!
    Constructs a QImageWriter objects that will write to a file with
    the name \a fileName, using the image format \a format. If \a
    format is not provided, QImageWriter will detect the image format
    by inspecting the extension of \a fileName.
*/
QImageWriter::QImageWriter(const QString &fileName, const QByteArray &format)
    : QImageWriter(new QFile(fileName), format)
{
    d->deleteDevice = true;
}

/*!
    Destructs the QImageWriter object.
*/
QImageWriter::~QImageWriter()
{
    if (d->deleteDevice)
        delete d->device;
    delete d->handler;
    delete d;
}

/*!
    Sets the format QImageWriter will use when writing images, to \a
    format. \a format is a case insensitive text string. Example:

    \snippet code/src_gui_image_qimagewriter.cpp 0

    You can call supportedImageFormats() for the full list of formats
    QImageWriter supports.

    \sa format()
*/
void QImageWriter::setFormat(const QByteArray &format)
{
    d->format = format;
}

/*!
    Returns the format QImageWriter uses for writing images.

    \sa setFormat()
*/
QByteArray QImageWriter::format() const
{
    return d->format;
}

/*!
    Sets QImageWriter's device to \a device. If a device has already
    been set, the old device is removed from QImageWriter and is
    otherwise left unchanged.

    If the device is not already open, QImageWriter will attempt to
    open the device in \l QIODevice::WriteOnly mode by calling
    open(). Note that this does not work for certain devices, such as
    QProcess, QTcpSocket and QUdpSocket, where more logic is required
    to open the device.

    \sa device(), setFileName()
*/
void QImageWriter::setDevice(QIODevice *device)
{
    if (d->device && d->deleteDevice)
        delete d->device;

    d->device = device;
    d->deleteDevice = false;
    delete d->handler;
    d->handler = 0;
}

/*!
    Returns the device currently assigned to QImageWriter, or 0 if no
    device has been assigned.
*/
QIODevice *QImageWriter::device() const
{
    return d->device;
}

/*!
    Sets the file name of QImageWriter to \a fileName. Internally,
    QImageWriter will create a QFile and open it in \l
    QIODevice::WriteOnly mode, and use this file when writing images.

    \sa fileName(), setDevice()
*/
void QImageWriter::setFileName(const QString &fileName)
{
    setDevice(new QFile(fileName));
    d->deleteDevice = true;
}

/*!
    If the currently assigned device is a QFile, or if setFileName()
    has been called, this function returns the name of the file
    QImageWriter writes to. Otherwise (i.e., if no device has been
    assigned or the device is not a QFile), an empty QString is
    returned.

    \sa setFileName(), setDevice()
*/
QString QImageWriter::fileName() const
{
    QFile *file = qobject_cast<QFile *>(d->device);
    return file ? file->fileName() : QString();
}

/*!
    Sets the quality setting of the image format to \a quality.

    Some image formats, in particular lossy ones, entail a tradeoff between a)
    visual quality of the resulting image, and b) encoding execution time and
    compression level. This function sets the level of that tradeoff for image
    formats that support it. For other formats, this value is ignored.

    The value range of \a quality depends on the image format. For example,
    the "jpeg" format supports a quality range from 0 (low visual quality, high
    compression) to 100 (high visual quality, low compression).

    \sa quality()
*/
void QImageWriter::setQuality(int quality)
{
    d->quality = quality;
}

/*!
    Returns the quality setting of the image format.

    \sa setQuality()
*/
int QImageWriter::quality() const
{
    return d->quality;
}

/*!
    This is an image format specific function that set the compression
    of an image. For image formats that do not support setting the
    compression, this value is ignored.

    The value range of \a compression depends on the image format. For
    example, the "tiff" format supports two values, 0(no compression) and
    1(LZW-compression).

    \sa compression()
*/
void QImageWriter::setCompression(int compression)
{
    d->compression = compression;
}

/*!
    Returns the compression of the image.

    \sa setCompression()
*/
int QImageWriter::compression() const
{
    return d->compression;
}

/*!
    This is an image format specific function that sets the gamma
    level of the image to \a gamma. For image formats that do not
    support setting the gamma level, this value is ignored.

    The value range of \a gamma depends on the image format. For
    example, the "png" format supports a gamma range from 0.0 to 1.0.

    \sa quality()
*/
void QImageWriter::setGamma(float gamma)
{
    d->gamma = gamma;
}

/*!
    Returns the gamma level of the image.

    \sa setGamma()
*/
float QImageWriter::gamma() const
{
    return d->gamma;
}

/*!
    \since 5.4

    This is an image format specific function that sets the
    subtype of the image to \a type. Subtype can be used by
    a handler to determine which format it should use while
    saving the image.

    For example, saving an image in DDS format with A8R8G8R8 subtype:

    \snippet code/src_gui_image_qimagewriter.cpp 3
*/
void QImageWriter::setSubType(const QByteArray &type)
{
    d->subType = type;
}

/*!
    \since 5.4

    Returns the subtype of the image.

    \sa setSubType()
*/
QByteArray QImageWriter::subType() const
{
    return d->subType;
}

/*!
    \since 5.4

    Returns the list of subtypes supported by an image.
*/
QList<QByteArray> QImageWriter::supportedSubTypes() const
{
    if (!supportsOption(QImageIOHandler::SupportedSubTypes))
        return QList<QByteArray>();
    return d->handler->option(QImageIOHandler::SupportedSubTypes).value< QList<QByteArray> >();
}

/*!
    \since 5.5

    This is an image format-specific function which sets the \a optimize flags when
    writing images. For image formats that do not support setting an \a optimize flag,
    this value is ignored.

    The default is false.

    \sa optimizedWrite()
*/
void QImageWriter::setOptimizedWrite(bool optimize)
{
    d->optimizedWrite = optimize;
}

/*!
    \since 5.5

    Returns whether optimization has been turned on for writing the image.

    \sa setOptimizedWrite()
*/
bool QImageWriter::optimizedWrite() const
{
    return d->optimizedWrite;
}

/*!
    \since 5.5

    This is an image format-specific function which turns on \a progressive scanning
    when writing images. For image formats that do not support setting a \a progressive
    scan flag, this value is ignored.

    The default is false.

    \sa progressiveScanWrite()
*/

void QImageWriter::setProgressiveScanWrite(bool progressive)
{
    d->progressiveScanWrite = progressive;
}

/*!
    \since 5.5

    Returns whether the image should be written as a progressive image.

    \sa setProgressiveScanWrite()
*/
bool QImageWriter::progressiveScanWrite() const
{
    return d->progressiveScanWrite;
}

/*!
    \since 5.5

    Sets the image transformations metadata including orientation to \a transform.

    If transformation metadata is not supported by the image format,
    the transform is applied before writing.

    \sa transformation(), write()
*/
void QImageWriter::setTransformation(QImageIOHandler::Transformations transform)
{
    d->transformation = transform;
}

/*!
    \since 5.5

    Returns the transformation and orientation the image has been set to written with.

    \sa setTransformation()
*/
QImageIOHandler::Transformations QImageWriter::transformation() const
{
    return d->transformation;
}

/*!
    \obsolete

    Use setText() instead.

    This is an image format specific function that sets the
    description of the image to \a description. For image formats that
    do not support setting the description, this value is ignored.

    The contents of \a description depends on the image format.

    \sa description()
*/
void QImageWriter::setDescription(const QString &description)
{
    d->description = description;
}

/*!
    \obsolete

    Use QImageReader::text() instead.

    Returns the description of the image.

    \sa setDescription()
*/
QString QImageWriter::description() const
{
    return d->description;
}

/*!
    \since 4.1

    Sets the image text associated with the key \a key to
    \a text. This is useful for storing copyright information
    or other information about the image. Example:

    \snippet code/src_gui_image_qimagewriter.cpp 1

    If you want to store a single block of data
    (e.g., a comment), you can pass an empty key, or use
    a generic key like "Description".

    The key and text will be embedded into the
    image data after calling write().

    Support for this option is implemented through
    QImageIOHandler::Description.

    \sa QImage::setText(), QImageReader::text()
*/
void QImageWriter::setText(const QString &key, const QString &text)
{
    if (!d->description.isEmpty())
        d->description += QLatin1String("\n\n");
    d->description += key.simplified() + QLatin1String(": ") + text.simplified();
}

/*!
    Returns \c true if QImageWriter can write the image; i.e., the image
    format is supported and the assigned device is open for reading.

    \sa write(), setDevice(), setFormat()
*/
bool QImageWriter::canWrite() const
{
    if (QFile *file = qobject_cast<QFile *>(d->device)) {
        const bool remove = !file->isOpen() && !file->exists();
        const bool result = d->canWriteHelper();

        // This looks strange (why remove if it doesn't exist?) but the issue
        // here is that canWriteHelper will create the file in the process of
        // checking if the write can succeed. If it subsequently fails, we
        // should remove that empty file.
        if (!result && remove)
            file->remove();
        return result;
    }

    return d->canWriteHelper();
}

extern void qt_imageTransform(QImage &src, QImageIOHandler::Transformations orient);

/*!
    Writes the image \a image to the assigned device or file
    name. Returns \c true on success; otherwise returns \c false. If the
    operation fails, you can call error() to find the type of error
    that occurred, or errorString() to get a human readable
    description of the error.

    \sa canWrite(), error(), errorString()
*/
bool QImageWriter::write(const QImage &image)
{
    // Do this before canWrite, so it doesn't create a file if this fails.
    if (Q_UNLIKELY(image.isNull())) {
        d->imageWriterError = QImageWriter::InvalidImageError;
        d->errorString = QImageWriter::tr("Image is empty");
        return false;
    }

    if (!canWrite())
        return false;

    QImage img = image;
    if (d->handler->supportsOption(QImageIOHandler::Quality))
        d->handler->setOption(QImageIOHandler::Quality, d->quality);
    if (d->handler->supportsOption(QImageIOHandler::CompressionRatio))
        d->handler->setOption(QImageIOHandler::CompressionRatio, d->compression);
    if (d->handler->supportsOption(QImageIOHandler::Gamma))
        d->handler->setOption(QImageIOHandler::Gamma, d->gamma);
    if (!d->description.isEmpty() && d->handler->supportsOption(QImageIOHandler::Description))
        d->handler->setOption(QImageIOHandler::Description, d->description);
    if (!d->subType.isEmpty() && d->handler->supportsOption(QImageIOHandler::SubType))
        d->handler->setOption(QImageIOHandler::SubType, d->subType);
    if (d->handler->supportsOption(QImageIOHandler::OptimizedWrite))
        d->handler->setOption(QImageIOHandler::OptimizedWrite, d->optimizedWrite);
    if (d->handler->supportsOption(QImageIOHandler::ProgressiveScanWrite))
        d->handler->setOption(QImageIOHandler::ProgressiveScanWrite, d->progressiveScanWrite);
    if (d->handler->supportsOption(QImageIOHandler::ImageTransformation))
        d->handler->setOption(QImageIOHandler::ImageTransformation, int(d->transformation));
    else
        qt_imageTransform(img, d->transformation);

    if (!d->handler->write(img))
        return false;
    if (QFile *file = qobject_cast<QFile *>(d->device))
        file->flush();
    return true;
}

/*!
    Returns the type of error that last occurred.

    \sa ImageWriterError, errorString()
*/
QImageWriter::ImageWriterError QImageWriter::error() const
{
    return d->imageWriterError;
}

/*!
    Returns a human readable description of the last error that occurred.

    \sa error()
*/
QString QImageWriter::errorString() const
{
    return d->errorString;
}

/*!
    \since 4.2

    Returns \c true if the writer supports \a option; otherwise returns
    false.

    Different image formats support different options. Call this function to
    determine whether a certain option is supported by the current format. For
    example, the PNG format allows you to embed text into the image's metadata
    (see text()).

    \snippet code/src_gui_image_qimagewriter.cpp 2

    Options can be tested after the writer has been associated with a format.

    \sa QImageReader::supportsOption(), setFormat()
*/
bool QImageWriter::supportsOption(QImageIOHandler::ImageOption option) const
{
    if (!d->handler && (d->handler = createWriteHandlerHelper(d->device, d->format)) == 0) {
        d->imageWriterError = QImageWriter::UnsupportedFormatError;
        d->errorString = QImageWriter::tr("Unsupported image format");
        return false;
    }

    return d->handler->supportsOption(option);
}

/*!
    Returns the list of image formats supported by QImageWriter.

    By default, Qt can write the following formats:

    \table
    \header \li Format \li MIME type                    \li Description
    \row    \li BMP    \li image/bmp                    \li Windows Bitmap
    \row    \li JPG    \li image/jpeg                   \li Joint Photographic Experts Group
    \row    \li PNG    \li image/png                    \li Portable Network Graphics
    \row    \li PBM    \li image/x-portable-bitmap      \li Portable Bitmap
    \row    \li PGM    \li image/x-portable-graymap     \li Portable Graymap
    \row    \li PPM    \li image/x-portable-pixmap      \li Portable Pixmap
    \row    \li XBM    \li image/x-xbitmap              \li X11 Bitmap
    \row    \li XPM    \li image/x-xpixmap              \li X11 Pixmap
    \endtable

    Reading and writing SVG files is supported through the \l{Qt SVG} module.
    The \l{Qt Image Formats} module provides support for additional image formats.

    Note that the QApplication instance must be created before this function is
    called.

    \sa setFormat(), QImageReader::supportedImageFormats(), QImageIOPlugin
*/
QList<QByteArray> QImageWriter::supportedImageFormats()
{
    return QImageReaderWriterHelpers::supportedImageFormats(QImageReaderWriterHelpers::CanWrite);
}

/*!
    Returns the list of MIME types supported by QImageWriter.

    Note that the QApplication instance must be created before this function is
    called.

    \sa supportedImageFormats(), QImageReader::supportedMimeTypes()
*/
QList<QByteArray> QImageWriter::supportedMimeTypes()
{
    return QImageReaderWriterHelpers::supportedMimeTypes(QImageReaderWriterHelpers::CanWrite);
}

/*!
    \since 5.12

    Returns the list of image formats corresponding to \a mimeType.

    Note that the QGuiApplication instance must be created before this function is
    called.

    \sa supportedImageFormats(), supportedMimeTypes()
*/

QList<QByteArray> QImageWriter::imageFormatsForMimeType(const QByteArray &mimeType)
{
    return QImageReaderWriterHelpers::imageFormatsForMimeType(mimeType,
                                                              QImageReaderWriterHelpers::CanWrite);
}

QT_END_NAMESPACE
