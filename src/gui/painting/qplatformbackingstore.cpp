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

#include "qplatformbackingstore.h"
#include <qwindow.h>
#include <qpixmap.h>
#include <private/qwindow_p.h>

#include <qopengl.h>
#include <qopenglcontext.h>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#ifndef QT_NO_OPENGL
#include <QtGui/qopengltextureblitter.h>
#include <QtGui/qoffscreensurface.h>
#endif
#include <qpa/qplatformgraphicsbuffer.h>
#include <qpa/qplatformgraphicsbufferhelper.h>

#ifndef GL_TEXTURE_BASE_LEVEL
#define GL_TEXTURE_BASE_LEVEL             0x813C
#endif
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL              0x813D
#endif
#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH              0x0CF2
#endif
#ifndef GL_RGB10_A2
#define GL_RGB10_A2                       0x8059
#endif
#ifndef GL_UNSIGNED_INT_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#endif

#ifndef GL_FRAMEBUFFER_SRB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
#ifndef GL_FRAMEBUFFER_SRGB_CAPABLE
#define GL_FRAMEBUFFER_SRGB_CAPABLE 0x8DBA
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaBackingStore, "qt.qpa.backingstore", QtWarningMsg);

class QPlatformBackingStorePrivate
{
public:
    QPlatformBackingStorePrivate(QWindow *w)
        : window(w)
        , backingStore(0)
#ifndef QT_NO_OPENGL
        , textureId(0)
        , blitter(0)
#endif
    {
    }

    ~QPlatformBackingStorePrivate()
    {
#ifndef QT_NO_OPENGL
        if (context) {
            QOffscreenSurface offscreenSurface;
            offscreenSurface.setFormat(context->format());
            offscreenSurface.create();
            context->makeCurrent(&offscreenSurface);
            if (textureId)
                context->functions()->glDeleteTextures(1, &textureId);
            if (blitter)
                blitter->destroy();
        }
        delete blitter;
#endif
    }
    QWindow *window;
    QBackingStore *backingStore;
#ifndef QT_NO_OPENGL
    QScopedPointer<QOpenGLContext> context;
    mutable GLuint textureId;
    mutable QSize textureSize;
    mutable bool needsSwizzle;
    mutable bool premultiplied;
    QOpenGLTextureBlitter *blitter;
#endif
};

#ifndef QT_NO_OPENGL

struct QBackingstoreTextureInfo
{
    void *source; // may be null
    GLuint textureId;
    QRect rect;
    QRect clipRect;
    QPlatformTextureList::Flags flags;
};

Q_DECLARE_TYPEINFO(QBackingstoreTextureInfo, Q_MOVABLE_TYPE);

class QPlatformTextureListPrivate : public QObjectPrivate
{
public:
    QPlatformTextureListPrivate()
        : locked(false)
    {
    }

    QVector<QBackingstoreTextureInfo> textures;
    bool locked;
};

QPlatformTextureList::QPlatformTextureList(QObject *parent)
: QObject(*new QPlatformTextureListPrivate, parent)
{
}

QPlatformTextureList::~QPlatformTextureList()
{
}

int QPlatformTextureList::count() const
{
    Q_D(const QPlatformTextureList);
    return d->textures.count();
}

GLuint QPlatformTextureList::textureId(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).textureId;
}

void *QPlatformTextureList::source(int index)
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).source;
}

QPlatformTextureList::Flags QPlatformTextureList::flags(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).flags;
}

QRect QPlatformTextureList::geometry(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).rect;
}

QRect QPlatformTextureList::clipRect(int index) const
{
    Q_D(const QPlatformTextureList);
    return d->textures.at(index).clipRect;
}

void QPlatformTextureList::lock(bool on)
{
    Q_D(QPlatformTextureList);
    if (on != d->locked) {
        d->locked = on;
        emit locked(on);
    }
}

bool QPlatformTextureList::isLocked() const
{
    Q_D(const QPlatformTextureList);
    return d->locked;
}

void QPlatformTextureList::appendTexture(void *source, GLuint textureId, const QRect &geometry,
                                         const QRect &clipRect, Flags flags)
{
    Q_D(QPlatformTextureList);
    QBackingstoreTextureInfo bi;
    bi.source = source;
    bi.textureId = textureId;
    bi.rect = geometry;
    bi.clipRect = clipRect;
    bi.flags = flags;
    d->textures.append(bi);
}

void QPlatformTextureList::clear()
{
    Q_D(QPlatformTextureList);
    d->textures.clear();
}
#endif // QT_NO_OPENGL

/*!
    \class QPlatformBackingStore
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformBackingStore class provides the drawing area for top-level
    windows.
*/

#ifndef QT_NO_OPENGL

static inline QRect deviceRect(const QRect &rect, QWindow *window)
{
    QRect deviceRect(rect.topLeft() * window->devicePixelRatio(),
                     rect.size() * window->devicePixelRatio());
    return deviceRect;
}

static inline QPoint deviceOffset(const QPoint &pt, QWindow *window)
{
    return pt * window->devicePixelRatio();
}

static QRegion deviceRegion(const QRegion &region, QWindow *window, const QPoint &offset)
{
    if (offset.isNull() && window->devicePixelRatio() <= 1)
        return region;

    QVector<QRect> rects;
    rects.reserve(region.rectCount());
    for (const QRect &rect : region)
        rects.append(deviceRect(rect.translated(offset), window));

    QRegion deviceRegion;
    deviceRegion.setRects(rects.constData(), rects.count());
    return deviceRegion;
}

static inline QRect toBottomLeftRect(const QRect &topLeftRect, int windowHeight)
{
    return QRect(topLeftRect.x(), windowHeight - topLeftRect.bottomRight().y() - 1,
                 topLeftRect.width(), topLeftRect.height());
}

static void blitTextureForWidget(const QPlatformTextureList *textures, int idx, QWindow *window, const QRect &deviceWindowRect,
                                 QOpenGLTextureBlitter *blitter, const QPoint &offset, bool canUseSrgb)
{
    const QRect clipRect = textures->clipRect(idx);
    if (clipRect.isEmpty())
        return;

    QRect rectInWindow = textures->geometry(idx);
    // relative to the TLW, not necessarily our window (if the flush is for a native child widget), have to adjust
    rectInWindow.translate(-offset);

    const QRect clippedRectInWindow = rectInWindow & clipRect.translated(rectInWindow.topLeft());
    const QRect srcRect = toBottomLeftRect(clipRect, rectInWindow.height());

    const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(deviceRect(clippedRectInWindow, window),
                                                                     deviceWindowRect);

    const QMatrix3x3 source = QOpenGLTextureBlitter::sourceTransform(deviceRect(srcRect, window),
                                                                     deviceRect(rectInWindow, window).size(),
                                                                     QOpenGLTextureBlitter::OriginBottomLeft);

    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    const bool srgb = textures->flags(idx).testFlag(QPlatformTextureList::TextureIsSrgb);
    if (srgb && canUseSrgb)
        funcs->glEnable(GL_FRAMEBUFFER_SRGB);

    blitter->blit(textures->textureId(idx), target, source);

    if (srgb && canUseSrgb)
        funcs->glDisable(GL_FRAMEBUFFER_SRGB);
}

/*!
    Flushes the given \a region from the specified \a window onto the
    screen, and composes it with the specified \a textures.

    The default implementation retrieves the contents using toTexture()
    and composes using OpenGL. May be reimplemented in subclasses if there
    is a more efficient native way to do it.

    \note \a region is relative to the window which may not be top-level in case
    \a window corresponds to a native child widget. \a offset is the position of
    the native child relative to the top-level window.
 */

void QPlatformBackingStore::composeAndFlush(QWindow *window, const QRegion &region,
                                            const QPoint &offset,
                                            QPlatformTextureList *textures,
                                            bool translucentBackground)
{
    if (!qt_window_private(window)->receivedExpose)
        return;

    if (!d_ptr->context) {
        d_ptr->context.reset(new QOpenGLContext);
        d_ptr->context->setFormat(d_ptr->window->requestedFormat());
        d_ptr->context->setScreen(d_ptr->window->screen());
        d_ptr->context->setShareContext(qt_window_private(d_ptr->window)->shareContext());
        if (!d_ptr->context->create()) {
            qCWarning(lcQpaBackingStore, "composeAndFlush: QOpenGLContext creation failed");
            return;
        }
    }

    if (!d_ptr->context->makeCurrent(window)) {
        qCWarning(lcQpaBackingStore, "composeAndFlush: makeCurrent() failed");
        return;
    }

    qCDebug(lcQpaBackingStore) << "Composing and flushing" << region << "of" << window
        << "at offset" << offset << "with" << textures->count() << "texture(s) in" << textures;

    QWindowPrivate::get(window)->lastComposeTime.start();

    QOpenGLFunctions *funcs = d_ptr->context->functions();
    funcs->glViewport(0, 0, qRound(window->width() * window->devicePixelRatio()), qRound(window->height() * window->devicePixelRatio()));
    funcs->glClearColor(0, 0, 0, translucentBackground ? 0 : 1);
    funcs->glClear(GL_COLOR_BUFFER_BIT);

    if (!d_ptr->blitter) {
        d_ptr->blitter = new QOpenGLTextureBlitter;
        d_ptr->blitter->create();
    }

    d_ptr->blitter->bind();

    const QRect deviceWindowRect = deviceRect(QRect(QPoint(), window->size()), window);
    const QPoint deviceWindowOffset = deviceOffset(offset, window);

    bool canUseSrgb = false;
    // If there are any sRGB textures in the list, check if the destination
    // framebuffer is sRGB capable.
    for (int i = 0; i < textures->count(); ++i) {
        if (textures->flags(i).testFlag(QPlatformTextureList::TextureIsSrgb)) {
            GLint cap = 0;
            funcs->glGetIntegerv(GL_FRAMEBUFFER_SRGB_CAPABLE, &cap);
            if (cap)
                canUseSrgb = true;
            break;
        }
    }

    // Textures for renderToTexture widgets.
    for (int i = 0; i < textures->count(); ++i) {
        if (!textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop))
            blitTextureForWidget(textures, i, window, deviceWindowRect, d_ptr->blitter, offset, canUseSrgb);
    }

    // Backingstore texture with the normal widgets.
    GLuint textureId = 0;
    QOpenGLTextureBlitter::Origin origin = QOpenGLTextureBlitter::OriginTopLeft;
    if (QPlatformGraphicsBuffer *graphicsBuffer = this->graphicsBuffer()) {
        if (graphicsBuffer->size() != d_ptr->textureSize) {
            if (d_ptr->textureId)
                funcs->glDeleteTextures(1, &d_ptr->textureId);
            funcs->glGenTextures(1, &d_ptr->textureId);
            funcs->glBindTexture(GL_TEXTURE_2D, d_ptr->textureId);
            QOpenGLContext *ctx = QOpenGLContext::currentContext();
            if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
                funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
                funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            }
            funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            if (QPlatformGraphicsBufferHelper::lockAndBindToTexture(graphicsBuffer, &d_ptr->needsSwizzle, &d_ptr->premultiplied)) {
                d_ptr->textureSize = graphicsBuffer->size();
            } else {
                d_ptr->textureSize = QSize(0,0);
            }

            graphicsBuffer->unlock();
        } else if (!region.isEmpty()){
            funcs->glBindTexture(GL_TEXTURE_2D, d_ptr->textureId);
            QPlatformGraphicsBufferHelper::lockAndBindToTexture(graphicsBuffer, &d_ptr->needsSwizzle, &d_ptr->premultiplied);
        }

        if (graphicsBuffer->origin() == QPlatformGraphicsBuffer::OriginBottomLeft)
            origin = QOpenGLTextureBlitter::OriginBottomLeft;
        textureId = d_ptr->textureId;
    } else {
        TextureFlags flags = 0;
        textureId = toTexture(deviceRegion(region, window, offset), &d_ptr->textureSize, &flags);
        d_ptr->needsSwizzle = (flags & TextureSwizzle) != 0;
        d_ptr->premultiplied = (flags & TexturePremultiplied) != 0;
        if (flags & TextureFlip)
            origin = QOpenGLTextureBlitter::OriginBottomLeft;
    }

    funcs->glEnable(GL_BLEND);
    if (d_ptr->premultiplied)
        funcs->glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    else
        funcs->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    if (textureId) {
        if (d_ptr->needsSwizzle)
            d_ptr->blitter->setRedBlueSwizzle(true);
        // The backingstore is for the entire tlw.
        // In case of native children offset tells the position relative to the tlw.
        const QRect srcRect = toBottomLeftRect(deviceWindowRect.translated(deviceWindowOffset), d_ptr->textureSize.height());
        const QMatrix3x3 source = QOpenGLTextureBlitter::sourceTransform(srcRect,
                                                                         d_ptr->textureSize,
                                                                         origin);
        d_ptr->blitter->blit(textureId, QMatrix4x4(), source);
        if (d_ptr->needsSwizzle)
            d_ptr->blitter->setRedBlueSwizzle(false);
    }

    // Textures for renderToTexture widgets that have WA_AlwaysStackOnTop set.
    for (int i = 0; i < textures->count(); ++i) {
        if (textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop))
            blitTextureForWidget(textures, i, window, deviceWindowRect, d_ptr->blitter, offset, canUseSrgb);
    }

    funcs->glDisable(GL_BLEND);
    d_ptr->blitter->release();

    d_ptr->context->swapBuffers(window);
}
#endif
/*!
  Implemented in subclasses to return the content of the backingstore as a QImage.

  If QPlatformIntegration::RasterGLSurface is supported, either this function or
  toTexture() must be implemented.

  \sa toTexture()
 */
QImage QPlatformBackingStore::toImage() const
{
    return QImage();
}
#ifndef QT_NO_OPENGL
/*!
  May be reimplemented in subclasses to return the content of the
  backingstore as an OpenGL texture. \a dirtyRegion is the part of the
  backingstore which may have changed since the last call to this function. The
  caller of this function must ensure that there is a current context.

  The size of the texture is returned in \a textureSize.

  The ownership of the texture is not transferred. The caller must not store
  the return value between calls, but instead call this function before each use.

  The default implementation returns a cached texture if \a dirtyRegion is empty and
  \a textureSize matches the backingstore size, otherwise it retrieves the content using
  toImage() and performs a texture upload. This works only if the value of \a textureSize
  is preserved between the calls to this function.

  If the red and blue components have to swapped, \a flags will be set to include \c
  TextureSwizzle. This allows creating textures from images in formats like
  QImage::Format_RGB32 without any further image conversion. Instead, the swizzling will
  be done in the shaders when performing composition. Other formats, that do not need
  such swizzling due to being already byte ordered RGBA, for example
  QImage::Format_RGBA8888, must result in having \a needsSwizzle set to false.

  If the image has to be flipped (e.g. because the texture is attached to an FBO), \a
  flags will be set to include \c TextureFlip.

  \note \a dirtyRegion is relative to the backingstore so no adjustment is needed.
 */
GLuint QPlatformBackingStore::toTexture(const QRegion &dirtyRegion, QSize *textureSize, TextureFlags *flags) const
{
    Q_ASSERT(textureSize);
    Q_ASSERT(flags);

    QImage image = toImage();
    QSize imageSize = image.size();

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    GLenum internalFormat = GL_RGBA;
    GLuint pixelType = GL_UNSIGNED_BYTE;

    bool needsConversion = false;
    *flags = 0;
    switch (image.format()) {
    case QImage::Format_ARGB32_Premultiplied:
        *flags |= TexturePremultiplied;
        Q_FALLTHROUGH();
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
        *flags |= TextureSwizzle;
        break;
    case QImage::Format_RGBA8888_Premultiplied:
        *flags |= TexturePremultiplied;
        Q_FALLTHROUGH();
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
        break;
    case QImage::Format_BGR30:
    case QImage::Format_A2BGR30_Premultiplied:
        if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
            pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
            internalFormat = GL_RGB10_A2;
            *flags |= TexturePremultiplied;
        } else {
            needsConversion = true;
        }
        break;
    case QImage::Format_RGB30:
    case QImage::Format_A2RGB30_Premultiplied:
        if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
            pixelType = GL_UNSIGNED_INT_2_10_10_10_REV;
            internalFormat = GL_RGB10_A2;
            *flags |= TextureSwizzle | TexturePremultiplied;
        } else {
            needsConversion = true;
        }
        break;
    default:
        needsConversion = true;
        break;
    }
    if (imageSize.isEmpty()) {
        *textureSize = imageSize;
        return 0;
    }

    // Must rely on the input only, not d_ptr.
    // With the default composeAndFlush() textureSize is &d_ptr->textureSize.
    bool resized = *textureSize != imageSize;
    if (dirtyRegion.isEmpty() && !resized)
        return d_ptr->textureId;

    *textureSize = imageSize;

    if (needsConversion)
        image = image.convertToFormat(QImage::Format_RGBA8888);

    // The image provided by the backingstore may have a stride larger than width * 4, for
    // instance on platforms that manually implement client-side decorations.
    static const int bytesPerPixel = 4;
    const int strideInPixels = image.bytesPerLine() / bytesPerPixel;
    const bool hasUnpackRowLength = !ctx->isOpenGLES() || ctx->format().majorVersion() >= 3;

    QOpenGLFunctions *funcs = ctx->functions();

    if (hasUnpackRowLength) {
        funcs->glPixelStorei(GL_UNPACK_ROW_LENGTH, strideInPixels);
    } else if (strideInPixels != image.width()) {
        // No UNPACK_ROW_LENGTH on ES 2.0 and yet we would need it. This case is typically
        // hit with QtWayland which is rarely used in combination with a ES2.0-only GL
        // implementation.  Therefore, accept the performance hit and do a copy.
        image = image.copy();
    }

    if (resized) {
        if (d_ptr->textureId)
            funcs->glDeleteTextures(1, &d_ptr->textureId);
        funcs->glGenTextures(1, &d_ptr->textureId);
        funcs->glBindTexture(GL_TEXTURE_2D, d_ptr->textureId);
        if (!ctx->isOpenGLES() || ctx->format().majorVersion() >= 3) {
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        }
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        funcs->glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, imageSize.width(), imageSize.height(), 0, GL_RGBA, pixelType,
                            const_cast<uchar*>(image.constBits()));
    } else {
        funcs->glBindTexture(GL_TEXTURE_2D, d_ptr->textureId);
        QRect imageRect = image.rect();
        QRect rect = dirtyRegion.boundingRect() & imageRect;

        if (hasUnpackRowLength) {
            funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, pixelType,
                                   image.constScanLine(rect.y()) + rect.x() * bytesPerPixel);
        } else {
            // if the rect is wide enough it's cheaper to just
            // extend it instead of doing an image copy
            if (rect.width() >= imageRect.width() / 2) {
                rect.setX(0);
                rect.setWidth(imageRect.width());
            }

            // if the sub-rect is full-width we can pass the image data directly to
            // OpenGL instead of copying, since there's no gap between scanlines

            if (rect.width() == imageRect.width()) {
                funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, rect.y(), rect.width(), rect.height(), GL_RGBA, pixelType,
                                       image.constScanLine(rect.y()));
            } else {
                funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x(), rect.y(), rect.width(), rect.height(), GL_RGBA, pixelType,
                                       image.copy(rect).constBits());
            }
        }
    }

    if (hasUnpackRowLength)
        funcs->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    return d_ptr->textureId;
}
#endif // QT_NO_OPENGL

/*!
    \fn QPaintDevice* QPlatformBackingStore::paintDevice()

    Implement this function to return the appropriate paint device.
*/

/*!
    Constructs an empty surface for the given top-level \a window.
*/
QPlatformBackingStore::QPlatformBackingStore(QWindow *window)
    : d_ptr(new QPlatformBackingStorePrivate(window))
{
}

/*!
    Destroys this surface.
*/
QPlatformBackingStore::~QPlatformBackingStore()
{
    delete d_ptr;
}

/*!
    Returns a pointer to the top-level window associated with this
    surface.
*/
QWindow* QPlatformBackingStore::window() const
{
    return d_ptr->window;
}

/*!
    Sets the backing store associated with this surface.
*/
void QPlatformBackingStore::setBackingStore(QBackingStore *backingStore)
{
    d_ptr->backingStore = backingStore;
}

/*!
    Returns a pointer to the backing store associated with this
    surface.
*/
QBackingStore *QPlatformBackingStore::backingStore() const
{
    return d_ptr->backingStore;
}

/*!
    This function is called before painting onto the surface begins,
    with the \a region in which the painting will occur.

    \sa endPaint(), paintDevice()
*/

void QPlatformBackingStore::beginPaint(const QRegion &)
{
}

/*!
    This function is called after painting onto the surface has ended.

    \sa beginPaint(), paintDevice()
*/

void QPlatformBackingStore::endPaint()
{
}

/*!
    Accessor for a backingstores graphics buffer abstraction
*/
QPlatformGraphicsBuffer *QPlatformBackingStore::graphicsBuffer() const
{
    return nullptr;
}

/*!
    Scrolls the given \a area \a dx pixels to the right and \a dy
    downward; both \a dx and \a dy may be negative.

    Returns \c true if the area was scrolled successfully; false otherwise.
*/
bool QPlatformBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    Q_UNUSED(area);
    Q_UNUSED(dx);
    Q_UNUSED(dy);

    return false;
}

QT_END_NAMESPACE
