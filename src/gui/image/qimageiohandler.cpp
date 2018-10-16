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
    \class QImageIOHandler
    \brief The QImageIOHandler class defines the common image I/O
    interface for all image formats in Qt.
    \reentrant
    \inmodule QtGui

    Qt uses QImageIOHandler for reading and writing images through
    QImageReader and QImageWriter. You can also derive from this class
    to write your own image format handler using Qt's plugin mechanism.

    Call setDevice() to assign a device to the handler, and
    setFormat() to assign a format to it. One QImageIOHandler may
    support more than one image format. canRead() returns \c true if an
    image can be read from the device, and read() and write() return
    true if reading or writing an image was completed successfully.

    QImageIOHandler also has support for animations formats, through
    the functions loopCount(), imageCount(), nextImageDelay() and
    currentImageNumber().

    In order to determine what options an image handler supports, Qt
    will call supportsOption() and setOption(). Make sure to
    reimplement these functions if you can provide support for any of
    the options in the ImageOption enum.

    To write your own image handler, you must at least reimplement
    canRead() and read(). Then create a QImageIOPlugin that
    can create the handler. Finally, install your plugin, and
    QImageReader and QImageWriter will then automatically load the
    plugin, and start using it.

    \sa QImageIOPlugin, QImageReader, QImageWriter
*/

/*! \enum QImageIOHandler::ImageOption

    This enum describes the different options supported by
    QImageIOHandler.  Some options are used to query an image for
    properties, and others are used to toggle the way in which an
    image should be written.

    \value Size The original size of an image. A handler that supports
    this option is expected to read the size of the image from the
    image metadata, and return this size from option() as a QSize.

    \value ClipRect The clip rect, or ROI (Region Of Interest). A
    handler that supports this option is expected to only read the
    provided QRect area from the original image in read(), before any
    other transformation is applied.

    \value ScaledSize The scaled size of the image. A handler that
    supports this option is expected to scale the image to the
    provided size (a QSize), after applying any clip rect
    transformation (ClipRect). If the handler does not support this
    option, QImageReader will perform the scaling after the image has
    been read.

    \value ScaledClipRect The scaled clip rect (or ROI, Region Of
    Interest) of the image. A handler that supports this option is
    expected to apply the provided clip rect (a QRect), after applying
    any scaling (ScaleSize) or regular clipping (ClipRect). If the
    handler does not support this option, QImageReader will apply the
    scaled clip rect after the image has been read.

    \value Description The image description. Some image formats,
    such as GIF and PNG, allow embedding of text
    or comments into the image data (e.g., for storing copyright
    information). It's common that the text is stored in key-value
    pairs, but some formats store all text in one continuous block.
    QImageIOHandler returns the text as one
    QString, where keys and values are separated by a ':', and
    keys-value pairs are separated by two newlines (\\n\\n). For example,
    "Title: Sunset\\n\\nAuthor: Jim Smith\\nSarah Jones\\n\\n". Formats that
    store text in a single block can use "Description" as the key.

    \value CompressionRatio The compression ratio of the image data. A
    handler that supports this option is expected to set its
    compression rate depending on the value of this option (an int)
    when writing.

    \value Gamma The gamma level of the image. A handler that supports
    this option is expected to set the image gamma level depending on
    the value of this option (a float) when writing.

    \value Quality The quality level of the image. A handler that
    supports this option is expected to set the image quality level
    depending on the value of this option (an int) when writing.

    \value Name The name of the image. A handler that supports this
    option is expected to read the name from the image metadata and
    return this as a QString, or when writing an image it is expected
    to store the name in the image metadata.

    \value SubType The subtype of the image. A handler that supports
    this option can use the subtype value to help when reading and
    writing images. For example, a PPM handler may have a subtype
    value of "ppm" or "ppmraw".

    \value IncrementalReading A handler that supports this option is
    expected to read the image in several passes, as if it was an
    animation. QImageReader will treat the image as an animation.

    \value Endianness The endianness of the image. Certain image
    formats can be stored as BigEndian or LittleEndian. A handler that
    supports Endianness uses the value of this option to determine how
    the image should be stored.

    \value Animation Image formats that support animation return
    true for this value in supportsOption(); otherwise, false is returned.

    \value BackgroundColor Certain image formats allow the
    background color to be specified. A handler that supports
    BackgroundColor initializes the background color to this option
    (a QColor) when reading an image.

    \value ImageFormat The image's data format returned by the handler.
    This can be any of the formats listed in QImage::Format.

    \value SupportedSubTypes Image formats that support different saving
    variants should return a list of supported variant names
    (QList<QByteArray>) in this option.

    \value OptimizedWrite. A handler which supports this option
    is expected to turn on optimization flags when writing.

    \value ProgressiveScanWrite. A handler which supports
    this option is expected to write the image as a progressive scan image.

    \value ImageTransformation. A handler which supports this option can read
    the transformation metadata of an image. A handler that supports this option
    should not apply the transformation itself.

    \value TransformedByDefault. A handler that reports support for this feature
    will have image transformation metadata applied by default on read.
*/

/*! \enum QImageIOHandler::Transformation
    \since 5.5

    This enum describes the different transformations or orientations
    supported by some image formats, usually through EXIF.

    \value TransformationNone No transformation should be applied.

    \value TransformationMirror Mirror the image horizontally.

    \value TransformationFlip Mirror the image vertically.

    \value TransformationRotate180 Rotate the image 180 degrees.
    This is the same as mirroring it both horizontally and vertically.

    \value TransformationRotate90 Rotate the image 90 degrees.

    \value TransformationMirrorAndRotate90 Mirror the image horizontally
    and then rotate it 90 degrees.

    \value TransformationFlipAndRotate90 Mirror the image vertically
    and then rotate it 90 degrees.

    \value TransformationRotate270 Rotate the image 270 degrees.
    This is the same as mirroring it both horizontally, vertically and
    then rotating it 90 degrees.

    \sa QImageReader::transformation(), QImageReader::setAutoTransform(), QImageWriter::setTransformation()
*/

/*!
    \class QImageIOPlugin
    \inmodule QtGui
    \brief The QImageIOPlugin class defines an interface for writing
    an image format plugin.
    \reentrant

    \ingroup plugins

    QImageIOPlugin is a factory for creating QImageIOHandler objects,
    which are used internally by QImageReader and QImageWriter to add
    support for different image formats to Qt.

    Writing an image I/O plugin is achieved by subclassing this
    base class, reimplementing the pure virtual functions capabilities()
    and create(), and exporting the class with the
    Q_PLUGIN_METADATA() macro. See \l{How to Create Qt Plugins} for details.

    An image format plugin can support three capabilities: reading (\l
    CanRead), writing (\l CanWrite) and \e incremental reading (\l
    CanReadIncremental). Reimplement capabilities() in you subclass to
    expose the capabilities of your image format.

    create() should create an instance of your QImageIOHandler
    subclass, with the provided device and format properly set, and
    return this handler.

    The json metadata file for the plugin needs to contain information
    about the image formats the plugins supports, together with the
    corresponding MIME types (one for each format). For a jpeg plugin, this
    could, for example, look as follows:

    \code
    {
      "Keys": [ "jpg", "jpeg" ],
      "MimeTypes": [ "image/jpeg", "image/jpeg" ]
    }
    \endcode

    Different plugins can support different capabilities. For example,
    you may have one plugin that supports reading the GIF format, and
    another that supports writing. Qt will select the correct plugin
    for the job, depending on the return value of capabilities(). If
    several plugins support the same capability, Qt will select one
    arbitrarily.

    \sa QImageIOHandler, {How to Create Qt Plugins}
*/

/*!
    \enum QImageIOPlugin::Capability

    This enum describes the capabilities of a QImageIOPlugin.

    \value CanRead The plugin can read images.
    \value CanWrite The plugin can write images.
    \value CanReadIncremental The plugin can read images incrementally.
*/

#include "qimageiohandler.h"

#include <qbytearray.h>
#include <qimage.h>
#include <qvariant.h>

QT_BEGIN_NAMESPACE

class QIODevice;

class QImageIOHandlerPrivate
{
    Q_DECLARE_PUBLIC(QImageIOHandler)
public:
    QImageIOHandlerPrivate(QImageIOHandler *q);
    virtual ~QImageIOHandlerPrivate();

    QIODevice *device;
    mutable QByteArray format;

    QImageIOHandler *q_ptr;
};

QImageIOHandlerPrivate::QImageIOHandlerPrivate(QImageIOHandler *q)
{
    device = 0;
    q_ptr = q;
}

QImageIOHandlerPrivate::~QImageIOHandlerPrivate()
{
}

/*!
    Constructs a QImageIOHandler object.
*/
QImageIOHandler::QImageIOHandler()
    : d_ptr(new QImageIOHandlerPrivate(this))
{
}

/*! \internal

    Constructs a QImageIOHandler object, using the private member \a
    dd.
*/
QImageIOHandler::QImageIOHandler(QImageIOHandlerPrivate &dd)
    : d_ptr(&dd)
{
}

/*!
    Destructs the QImageIOHandler object.
*/
QImageIOHandler::~QImageIOHandler()
{
}

/*!
    Sets the device of the QImageIOHandler to \a device. The image
    handler will use this device when reading and writing images.

    The device can only be set once and must be set before calling
    canRead(), read(), write(), etc. If you need to read multiple
    files, construct multiple instances of the appropriate
    QImageIOHandler subclass.

    \sa device()
*/
void QImageIOHandler::setDevice(QIODevice *device)
{
    Q_D(QImageIOHandler);
    d->device = device;
}

/*!
    Returns the device currently assigned to the QImageIOHandler. If
    not device has been assigned, 0 is returned.
*/
QIODevice *QImageIOHandler::device() const
{
    Q_D(const QImageIOHandler);
    return d->device;
}

/*!
    Sets the format of the QImageIOHandler to \a format. The format is
    most useful for handlers that support multiple image formats.

    \sa format()
*/
void QImageIOHandler::setFormat(const QByteArray &format)
{
    Q_D(QImageIOHandler);
    d->format = format;
}

/*!
    Sets the format of the QImageIOHandler to \a format. The format is
    most useful for handlers that support multiple image formats.

    This function is declared const so that it can be called from canRead().

    \sa format()
*/
void QImageIOHandler::setFormat(const QByteArray &format) const
{
    Q_D(const QImageIOHandler);
    d->format = format;
}

/*!
    Returns the format that is currently assigned to
    QImageIOHandler. If no format has been assigned, an empty string
    is returned.

    \sa setFormat()
*/
QByteArray QImageIOHandler::format() const
{
    Q_D(const QImageIOHandler);
    return d->format;
}

/*!
    \fn bool QImageIOHandler::read(QImage *image)

    Read an image from the device, and stores it in \a image.
    Returns \c true if the image is successfully read; otherwise returns
    false.

    For image formats that support incremental loading, and for animation
    formats, the image handler can assume that \a image points to the
    previous frame.

    \sa canRead()
*/

/*!
    \fn bool QImageIOHandler::canRead() const

    Returns \c true if an image can be read from the device (i.e., the
    image format is supported, the device can be read from and the
    initial header information suggests that the image can be read);
    otherwise returns \c false.

    When reimplementing canRead(), make sure that the I/O device
    (device()) is left in its original state (e.g., by using peek()
    rather than read()).

    \sa read(), QIODevice::peek()
*/

/*!
    \obsolete

    Use format() instead.
*/

QByteArray QImageIOHandler::name() const
{
    return format();
}

/*!
    Writes the image \a image to the assigned device. Returns \c true on
    success; otherwise returns \c false.

    The default implementation does nothing, and simply returns \c false.
*/
bool QImageIOHandler::write(const QImage &image)
{
    Q_UNUSED(image);
    return false;
}

/*!
    Sets the option \a option with the value \a value.

    \sa option(), ImageOption
*/
void QImageIOHandler::setOption(ImageOption option, const QVariant &value)
{
    Q_UNUSED(option);
    Q_UNUSED(value);
}

/*!
    Returns the value assigned to \a option as a QVariant. The type of
    the value depends on the option. For example, option(Size) returns
    a QSize variant.

    \sa setOption(), supportsOption()
*/
QVariant QImageIOHandler::option(ImageOption option) const
{
    Q_UNUSED(option);
    return QVariant();
}

/*!
    Returns \c true if the QImageIOHandler supports the option \a option;
    otherwise returns \c false. For example, if the QImageIOHandler
    supports the \l Size option, supportsOption(Size) must return
    true.

    \sa setOption(), option()
*/
bool QImageIOHandler::supportsOption(ImageOption option) const
{
    Q_UNUSED(option);
    return false;
}

/*!
    For image formats that support animation, this function returns
    the sequence number of the current image in the animation. If
    this function is called before any image is read(), -1 is
    returned. The number of the first image in the sequence is 0.

    If the image format does not support animation, 0 is returned.

    \sa read()
*/
int QImageIOHandler::currentImageNumber() const
{
    return 0;
}

/*!
    Returns the rect of the current image. If no rect is defined for the
    image, and empty QRect() is returned.

    This function is useful for animations, where only parts of the frame
    may be updated at a time.
*/
QRect QImageIOHandler::currentImageRect() const
{
    return QRect();
}

/*!
    For image formats that support animation, this function returns
    the number of images in the animation. If the image format does
    not support animation, or if it is unable to determine the number
    of images, 0 is returned.

    The default implementation returns 1 if canRead() returns \c true;
    otherwise 0 is returned.
*/
int QImageIOHandler::imageCount() const
{
    return canRead() ? 1 : 0;
}

/*!
   For image formats that support animation, this function jumps to the
   next image.

   The default implementation does nothing, and returns \c false.
*/
bool QImageIOHandler::jumpToNextImage()
{
    return false;
}

/*!
   For image formats that support animation, this function jumps to the image
   whose sequence number is \a imageNumber. The next call to read() will
   attempt to read this image.

   The default implementation does nothing, and returns \c false.
*/
bool QImageIOHandler::jumpToImage(int imageNumber)
{
    Q_UNUSED(imageNumber);
    return false;
}

/*!
    For image formats that support animation, this function returns
    the number of times the animation should loop. If the image format
    does not support animation, 0 is returned.
*/
int QImageIOHandler::loopCount() const
{
    return 0;
}

/*!
    For image formats that support animation, this function returns
    the number of milliseconds to wait until reading the next
    image. If the image format does not support animation, 0 is
    returned.
*/
int QImageIOHandler::nextImageDelay() const
{
    return 0;
}

#ifndef QT_NO_IMAGEFORMATPLUGIN

/*!
    Constructs an image plugin with the given \a parent. This is
    invoked automatically by the moc generated code that exports the plugin.
*/
QImageIOPlugin::QImageIOPlugin(QObject *parent)
    : QObject(parent)
{
}

/*!
    Destroys the picture format plugin.

    You never have to call this explicitly. Qt destroys a plugin
    automatically when it is no longer used.
*/
QImageIOPlugin::~QImageIOPlugin()
{
}

/*! \fn QImageIOPlugin::capabilities(QIODevice *device, const QByteArray &format) const

    Returns the capabilities of the plugin, based on the data in \a
    device and the format \a format.  If \a device is \c 0, it should
    simply report whether the format can be read or written.  Otherwise,
    it should attempt to determine whether the given format (or any
    format supported by the plugin if \a format is empty) can be read
    from or written to \a device.  It should do this without changing
    the state of \a device (typically by using QIODevice::peek()).

    For example, if the QImageIOPlugin supports the BMP format, \a format
    is either empty or \c "bmp", and the data in the device starts with the
    characters \c "BM", this function should return \l CanRead. If \a format
    is \c "bmp", \a device is \c 0 and the handler supports both reading and
    writing, this function should return \l CanRead | \l CanWrite.

    Format names are always given in lower case.
*/

/*!
    \fn QImageIOHandler *QImageIOPlugin::create(QIODevice *device, const QByteArray &format) const

    Creates and returns a QImageIOHandler subclass, with \a device
    and \a format set. The \a format must come from the values listed
    in the \c "Keys" entry in the plugin metadata, or be empty.  If it is
    empty, the data in \a device must have been recognized by the
    capabilities() method (with a likewise empty format).

    Format names are always given in lower case.
*/

#endif // QT_NO_IMAGEFORMATPLUGIN

QT_END_NAMESPACE
