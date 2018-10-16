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

#include "qclipboard.h"

#ifndef QT_NO_CLIPBOARD

#include "qmimedata.h"
#include "qpixmap.h"
#include "qvariant.h"
#include "qbuffer.h"
#include "qimage.h"
#include "qtextcodec.h"

#include "private/qguiapplication_p.h"
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformclipboard.h>

QT_BEGIN_NAMESPACE

/*!
    \class QClipboard
    \brief The QClipboard class provides access to the window system clipboard.
    \inmodule QtGui

    The clipboard offers a simple mechanism to copy and paste data
    between applications.

    QClipboard supports the same data types that QDrag does, and uses
    similar mechanisms. For advanced clipboard usage read \l{Drag and
    Drop}.

    There is a single QClipboard object in an application, accessible
    as QGuiApplication::clipboard().

    Example:
    \snippet code/src_gui_kernel_qclipboard.cpp 0

    QClipboard features some convenience functions to access common
    data types: setText() allows the exchange of Unicode text and
    setPixmap() and setImage() allows the exchange of QPixmaps and
    QImages between applications. The setMimeData() function is the
    ultimate in flexibility: it allows you to add any QMimeData into
    the clipboard. There are corresponding getters for each of these,
    e.g. text(), image() and pixmap(). You can clear the clipboard by
    calling clear().

    A typical example of the use of these functions follows:

    \snippet droparea.cpp 0

    \section1 Notes for X11 Users

    \list

    \li The X11 Window System has the concept of a separate selection
    and clipboard.  When text is selected, it is immediately available
    as the global mouse selection.  The global mouse selection may
    later be copied to the clipboard.  By convention, the middle mouse
    button is used to paste the global mouse selection.

    \li X11 also has the concept of ownership; if you change the
    selection within a window, X11 will only notify the owner and the
    previous owner of the change, i.e. it will not notify all
    applications that the selection or clipboard data changed.

    \li Lastly, the X11 clipboard is event driven, i.e. the clipboard
    will not function properly if the event loop is not running.
    Similarly, it is recommended that the contents of the clipboard
    are stored or retrieved in direct response to user-input events,
    e.g. mouse button or key presses and releases.  You should not
    store or retrieve the clipboard contents in response to timer or
    non-user-input events.

    \li Since there is no standard way to copy and paste files between
    applications on X11, various MIME types and conventions are currently
    in use. For instance, Nautilus expects files to be supplied with a
    \c{x-special/gnome-copied-files} MIME type with data beginning with
    the cut/copy action, a newline character, and the URL of the file.

    \endlist

    \section1 Notes for \macos Users

    \macos supports a separate find buffer that holds the current
    search string in Find operations. This find clipboard can be accessed
    by specifying the FindBuffer mode.

    \section1 Notes for Windows and \macos Users

    \list

    \li Windows and \macos do not support the global mouse
    selection; they only supports the global clipboard, i.e. they
    only add text to the clipboard when an explicit copy or cut is
    made.

    \li Windows and \macos does not have the concept of ownership;
    the clipboard is a fully global resource so all applications are
    notified of changes.

    \endlist

    \section1 Notes for Universal Windows Platform Users

    \list

    \li The Universal Windows Platform only allows to query the
    clipboard in case the application is active and an application
    window has focus. Accessing the clipboard data when in background
    will fail due to access denial.

    \endlist

    \sa QGuiApplication
*/

/*!
    \internal

    Constructs a clipboard object.

    Do not call this function.

    Call QGuiApplication::clipboard() instead to get a pointer to the
    application's global clipboard object.

    There is only one clipboard in the window system, and creating
    more than one object to represent it is almost certainly an error.
*/

QClipboard::QClipboard(QObject *parent)
    : QObject(parent)
{
    // nothing
}

/*!
    \internal

    Destroys the clipboard.

    You should never delete the clipboard. QGuiApplication will do this
    when the application terminates.
*/
QClipboard::~QClipboard()
{
}

/*!
    \fn void QClipboard::changed(QClipboard::Mode mode)
    \since 4.2

    This signal is emitted when the data for the given clipboard \a
    mode is changed.

    \sa dataChanged(), selectionChanged(), findBufferChanged()
*/

/*!
    \fn void QClipboard::dataChanged()

    This signal is emitted when the clipboard data is changed.

    On \macos and with Qt version 4.3 or higher, clipboard
    changes made by other applications will only be detected
    when the application is activated.

    \sa findBufferChanged(), selectionChanged(), changed()
*/

/*!
    \fn void QClipboard::selectionChanged()

    This signal is emitted when the selection is changed. This only
    applies to windowing systems that support selections, e.g. X11.
    Windows and \macos don't support selections.

    \sa dataChanged(), findBufferChanged(), changed()
*/

/*!
    \fn void QClipboard::findBufferChanged()
    \since 4.2

    This signal is emitted when the find buffer is changed. This only
    applies to \macos.

    With Qt version 4.3 or higher, clipboard changes made by other
    applications will only be detected when the application is activated.

    \sa dataChanged(), selectionChanged(), changed()
*/


/*! \enum QClipboard::Mode
    \keyword clipboard mode

    This enum type is used to control which part of the system clipboard is
    used by QClipboard::mimeData(), QClipboard::setMimeData() and related functions.

    \value Clipboard  indicates that data should be stored and retrieved from
    the global clipboard.

    \value Selection  indicates that data should be stored and retrieved from
    the global mouse selection. Support for \c Selection is provided only on
    systems with a global mouse selection (e.g. X11).

    \value FindBuffer indicates that data should be stored and retrieved from
    the Find buffer. This mode is used for holding search strings on \macos.

    \omitvalue LastMode

    \sa QClipboard::supportsSelection()
*/


/*!
    \overload

    Returns the clipboard text in subtype \a subtype, or an empty string
    if the clipboard does not contain any text. If \a subtype is null,
    any subtype is acceptable, and \a subtype is set to the chosen
    subtype.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    text is retrieved from the global clipboard.  If \a mode is
    QClipboard::Selection, the text is retrieved from the global
    mouse selection.

    Common values for \a subtype are "plain" and "html".

    Note that calling this function repeatedly, for instance from a
    key event handler, may be slow. In such cases, you should use the
    \c dataChanged() signal instead.

    \sa setText(), mimeData()
*/
QString QClipboard::text(QString &subtype, Mode mode) const
{
    const QMimeData *const data = mimeData(mode);
    if (!data)
        return QString();

    const QStringList formats = data->formats();
    if (subtype.isEmpty()) {
        if (formats.contains(QLatin1String("text/plain")))
            subtype = QLatin1String("plain");
        else {
            for (int i = 0; i < formats.size(); ++i)
                if (formats.at(i).startsWith(QLatin1String("text/"))) {
                    subtype = formats.at(i).mid(5);
                    break;
                }
            if (subtype.isEmpty())
                return QString();
        }
    } else if (!formats.contains(QLatin1String("text/") + subtype)) {
        return QString();
    }

    const QByteArray rawData = data->data(QLatin1String("text/") + subtype);

#ifndef QT_NO_TEXTCODEC
    QTextCodec* codec = QTextCodec::codecForMib(106); // utf-8 is default
    if (subtype == QLatin1String("html"))
        codec = QTextCodec::codecForHtml(rawData, codec);
    else
        codec = QTextCodec::codecForUtfText(rawData, codec);
    return codec->toUnicode(rawData);
#else //QT_NO_TEXTCODEC
    return rawData;
#endif //QT_NO_TEXTCODEC
}

/*!
    Returns the clipboard text as plain text, or an empty string if the
    clipboard does not contain any text.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    text is retrieved from the global clipboard.  If \a mode is
    QClipboard::Selection, the text is retrieved from the global
    mouse selection. If \a mode is QClipboard::FindBuffer, the
    text is retrieved from the search string buffer.

    \sa setText(), mimeData()
*/
QString QClipboard::text(Mode mode) const
{
    const QMimeData *data = mimeData(mode);
    return data ? data->text() : QString();
}

/*!
    Copies \a text into the clipboard as plain text.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    text is stored in the global clipboard.  If \a mode is
    QClipboard::Selection, the text is stored in the global
    mouse selection. If \a mode is QClipboard::FindBuffer, the
    text is stored in the search string buffer.

    \sa text(), setMimeData()
*/
void QClipboard::setText(const QString &text, Mode mode)
{
    QMimeData *data = new QMimeData;
    data->setText(text);
    setMimeData(data, mode);
}

/*!
    Returns the clipboard image, or returns a null image if the
    clipboard does not contain an image or if it contains an image in
    an unsupported image format.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    image is retrieved from the global clipboard.  If \a mode is
    QClipboard::Selection, the image is retrieved from the global
    mouse selection.

    \sa setImage(), pixmap(), mimeData(), QImage::isNull()
*/
QImage QClipboard::image(Mode mode) const
{
    const QMimeData *data = mimeData(mode);
    if (!data)
        return QImage();
    return qvariant_cast<QImage>(data->imageData());
}

/*!
    Copies the \a image into the clipboard.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    image is stored in the global clipboard.  If \a mode is
    QClipboard::Selection, the data is stored in the global
    mouse selection.

    This is shorthand for:

    \snippet code/src_gui_kernel_qclipboard.cpp 1

    \sa image(), setPixmap(), setMimeData()
*/
void QClipboard::setImage(const QImage &image, Mode mode)
{
    QMimeData *data = new QMimeData;
    data->setImageData(image);
    setMimeData(data, mode);
}

/*!
    Returns the clipboard pixmap, or null if the clipboard does not
    contain a pixmap. Note that this can lose information. For
    example, if the image is 24-bit and the display is 8-bit, the
    result is converted to 8 bits, and if the image has an alpha
    channel, the result just has a mask.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    pixmap is retrieved from the global clipboard.  If \a mode is
    QClipboard::Selection, the pixmap is retrieved from the global
    mouse selection.

    \sa setPixmap(), image(), mimeData(), QPixmap::convertFromImage()
*/
QPixmap QClipboard::pixmap(Mode mode) const
{
    const QMimeData *data = mimeData(mode);
    return data ? qvariant_cast<QPixmap>(data->imageData()) : QPixmap();
}

/*!
    Copies \a pixmap into the clipboard. Note that this is slower
    than setImage() because it needs to convert the QPixmap to a
    QImage first.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    pixmap is stored in the global clipboard.  If \a mode is
    QClipboard::Selection, the pixmap is stored in the global
    mouse selection.

    \sa pixmap(), setImage(), setMimeData()
*/
void QClipboard::setPixmap(const QPixmap &pixmap, Mode mode)
{
    QMimeData *data = new QMimeData;
    data->setImageData(pixmap);
    setMimeData(data, mode);
}


/*!
    \fn QMimeData *QClipboard::mimeData(Mode mode) const

    Returns a pointer to a QMimeData representation of the current
    clipboard data (can be NULL if the given \a mode is not
    supported by the platform).

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    data is retrieved from the global clipboard.  If \a mode is
    QClipboard::Selection, the data is retrieved from the global
    mouse selection. If \a mode is QClipboard::FindBuffer, the
    data is retrieved from the search string buffer.

    The text(), image(), and pixmap() functions are simpler
    wrappers for retrieving text, image, and pixmap data.

    \note The pointer returned might become invalidated when the contents
    of the clipboard changes; either by calling one of the setter functions
    or externally by the system clipboard changing.

    \sa setMimeData()
*/
const QMimeData* QClipboard::mimeData(Mode mode) const
{
    QPlatformClipboard *clipboard = QGuiApplicationPrivate::platformIntegration()->clipboard();
    if (!clipboard->supportsMode(mode)) return 0;
    return clipboard->mimeData(mode);
}

/*!
    \fn void QClipboard::setMimeData(QMimeData *src, Mode mode)

    Sets the clipboard data to \a src. Ownership of the data is
    transferred to the clipboard. If you want to remove the data
    either call clear() or call setMimeData() again with new data.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, the
    data is stored in the global clipboard.  If \a mode is
    QClipboard::Selection, the data is stored in the global
    mouse selection. If \a mode is QClipboard::FindBuffer, the
    data is stored in the search string buffer.

    The setText(), setImage() and setPixmap() functions are simpler
    wrappers for setting text, image and pixmap data respectively.

    \sa mimeData()
*/
void QClipboard::setMimeData(QMimeData* src, Mode mode)
{
    QPlatformClipboard *clipboard = QGuiApplicationPrivate::platformIntegration()->clipboard();
    if (!clipboard->supportsMode(mode)) {
        if (src != 0) {
            qDebug("Data set on unsupported clipboard mode. QMimeData object will be deleted.");
            src->deleteLater();
        }
    } else {
        clipboard->setMimeData(src,mode);
    }
}

/*!
    \fn void QClipboard::clear(Mode mode)
    Clear the clipboard contents.

    The \a mode argument is used to control which part of the system
    clipboard is used.  If \a mode is QClipboard::Clipboard, this
    function clears the global clipboard contents.  If \a mode is
    QClipboard::Selection, this function clears the global mouse
    selection contents. If \a mode is QClipboard::FindBuffer, this
    function clears the search string buffer.

    \sa QClipboard::Mode, supportsSelection()
*/
void QClipboard::clear(Mode mode)
{
    setMimeData(0, mode);
}

/*!
    Returns \c true if the clipboard supports mouse selection; otherwise
    returns \c false.
*/
bool QClipboard::supportsSelection() const
{
    return supportsMode(Selection);
}

/*!
    Returns \c true if the clipboard supports a separate search buffer; otherwise
    returns \c false.
*/
bool QClipboard::supportsFindBuffer() const
{
    return supportsMode(FindBuffer);
}

/*!
    Returns \c true if this clipboard object owns the clipboard data;
    otherwise returns \c false.
*/
bool QClipboard::ownsClipboard() const
{
    return ownsMode(Clipboard);
}

/*!
    Returns \c true if this clipboard object owns the mouse selection
    data; otherwise returns \c false.
*/
bool QClipboard::ownsSelection() const
{
    return ownsMode(Selection);
}

/*!
    \since 4.2

    Returns \c true if this clipboard object owns the find buffer data;
    otherwise returns \c false.
*/
bool QClipboard::ownsFindBuffer() const
{
    return ownsMode(FindBuffer);
}

/*!
    \internal
    \fn bool QClipboard::supportsMode(Mode mode) const;
    Returns \c true if the clipboard supports the clipboard mode speacified by \a mode;
    otherwise returns \c false.
*/
bool QClipboard::supportsMode(Mode mode) const
{
    QPlatformClipboard *clipboard = QGuiApplicationPrivate::platformIntegration()->clipboard();
    return clipboard && clipboard->supportsMode(mode);
}

/*!
    \internal
    \fn bool QClipboard::ownsMode(Mode mode) const;
    Returns \c true if the clipboard supports the clipboard data speacified by \a mode;
    otherwise returns \c false.
*/
bool QClipboard::ownsMode(Mode mode) const
{
    QPlatformClipboard *clipboard = QGuiApplicationPrivate::platformIntegration()->clipboard();
    return clipboard && clipboard->ownsMode(mode);
}

/*!
    \internal
    Emits the appropriate changed signal for \a mode.
*/
void QClipboard::emitChanged(Mode mode)
{
    switch (mode) {
        case Clipboard:
            emit dataChanged();
        break;
        case Selection:
            emit selectionChanged();
        break;
        case FindBuffer:
            emit findBufferChanged();
        break;
        default:
        break;
    }
    emit changed(mode);
}

QT_END_NAMESPACE

#endif // QT_NO_CLIPBOARD
