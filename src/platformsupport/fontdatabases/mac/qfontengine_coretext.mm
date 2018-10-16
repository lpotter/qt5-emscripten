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

#include "qfontengine_coretext_p.h"

#include <qpa/qplatformfontdatabase.h>
#include <QtCore/qendian.h>
#include <QtCore/qsettings.h>
#include <QtCore/qoperatingsystemversion.h>

#include <private/qimage_p.h>

#include <cmath>

#if defined(Q_OS_MACOS)
#import <AppKit/AppKit.h>
#endif

#if defined(QT_PLATFORM_UIKIT)
#import <UIKit/UIKit.h>
#endif

// These are available cross platform, exported as kCTFontWeightXXX from CoreText.framework,
// but they are not documented and are not in public headers so are private API and exposed
// only through the NSFontWeightXXX and UIFontWeightXXX aliases in AppKit and UIKit (rdar://26109857)
#if defined(Q_OS_MACOS)
#define kCTFontWeightUltraLight NSFontWeightUltraLight
#define kCTFontWeightThin NSFontWeightThin
#define kCTFontWeightLight NSFontWeightLight
#define kCTFontWeightRegular NSFontWeightRegular
#define kCTFontWeightMedium NSFontWeightMedium
#define kCTFontWeightSemibold NSFontWeightSemibold
#define kCTFontWeightBold NSFontWeightBold
#define kCTFontWeightHeavy NSFontWeightHeavy
#define kCTFontWeightBlack NSFontWeightBlack
#elif defined(QT_PLATFORM_UIKIT)
#define kCTFontWeightUltraLight UIFontWeightUltraLight
#define kCTFontWeightThin UIFontWeightThin
#define kCTFontWeightLight UIFontWeightLight
#define kCTFontWeightRegular UIFontWeightRegular
#define kCTFontWeightMedium UIFontWeightMedium
#define kCTFontWeightSemibold UIFontWeightSemibold
#define kCTFontWeightBold UIFontWeightBold
#define kCTFontWeightHeavy UIFontWeightHeavy
#define kCTFontWeightBlack UIFontWeightBlack
#endif

QT_BEGIN_NAMESPACE

static float SYNTHETIC_ITALIC_SKEW = std::tan(14.f * std::acos(0.f) / 90.f);

bool QCoreTextFontEngine::ct_getSfntTable(void *user_data, uint tag, uchar *buffer, uint *length)
{
    CTFontRef ctfont = *(CTFontRef *)user_data;

    QCFType<CFDataRef> table = CTFontCopyTable(ctfont, tag, 0);
    if (!table)
        return false;

    CFIndex tableLength = CFDataGetLength(table);
    if (buffer && int(*length) >= tableLength)
        CFDataGetBytes(table, CFRangeMake(0, tableLength), buffer);
    *length = tableLength;
    Q_ASSERT(int(*length) > 0);
    return true;
}

QFont::Weight QCoreTextFontEngine::qtWeightFromCFWeight(float value)
{
#define COMPARE_WEIGHT_DISTANCE(ct_weight, qt_weight) \
    { \
        float d; \
        if ((d = qAbs(value - ct_weight)) < distance) { \
            distance = d; \
            ret = qt_weight; \
        } \
    }

    float distance = qAbs(value - kCTFontWeightBlack);
    QFont::Weight ret = QFont::Black;

    // Compare distance to system weight to find the closest match.
    // (Note: Must go from high to low, so that midpoints are rounded up)
    COMPARE_WEIGHT_DISTANCE(kCTFontWeightHeavy, QFont::ExtraBold);
    COMPARE_WEIGHT_DISTANCE(kCTFontWeightBold, QFont::Bold);
    COMPARE_WEIGHT_DISTANCE(kCTFontWeightSemibold, QFont::DemiBold);
    COMPARE_WEIGHT_DISTANCE(kCTFontWeightMedium, QFont::Medium);
    COMPARE_WEIGHT_DISTANCE(kCTFontWeightRegular, QFont::Normal);
    COMPARE_WEIGHT_DISTANCE(kCTFontWeightLight, QFont::Light);
    COMPARE_WEIGHT_DISTANCE(kCTFontWeightThin, QFont::ExtraLight);
    COMPARE_WEIGHT_DISTANCE(kCTFontWeightUltraLight, QFont::Thin);

#undef COMPARE_WEIGHT_DISTANCE

    return ret;
}

static void loadAdvancesForGlyphs(CTFontRef ctfont,
                                  QVarLengthArray<CGGlyph> &cgGlyphs,
                                  QGlyphLayout *glyphs, int len,
                                  QFontEngine::ShaperFlags flags,
                                  const QFontDef &fontDef)
{
    Q_UNUSED(flags);
    QVarLengthArray<CGSize> advances(len);
    CTFontGetAdvancesForGlyphs(ctfont, kCTFontOrientationHorizontal, cgGlyphs.data(), advances.data(), len);

    for (int i = 0; i < len; ++i) {
        if (glyphs->glyphs[i] & 0xff000000)
            continue;
        glyphs->advances[i] = QFixed::fromReal(advances[i].width);
    }

    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
        for (int i = 0; i < len; ++i)
            glyphs->advances[i] = glyphs->advances[i].round();
    }
}

static float getTraitValue(CFDictionaryRef allTraits, CFStringRef trait)
{
    if (CFDictionaryContainsKey(allTraits, trait)) {
        CFNumberRef traitNum = (CFNumberRef) CFDictionaryGetValue(allTraits, trait);
        float v = 0;
        CFNumberGetValue(traitNum, kCFNumberFloatType, &v);
        return v;
    }
    return 0;
}

int QCoreTextFontEngine::antialiasingThreshold = 0;
QFontEngine::GlyphFormat QCoreTextFontEngine::defaultGlyphFormat = QFontEngine::Format_A32;

CGAffineTransform qt_transform_from_fontdef(const QFontDef &fontDef)
{
    CGAffineTransform transform = CGAffineTransformIdentity;
    if (fontDef.stretch && fontDef.stretch != 100)
        transform = CGAffineTransformMakeScale(float(fontDef.stretch) / float(100), 1);
    return transform;
}

// Keeps font data alive until engine is disposed
class QCoreTextRawFontEngine : public QCoreTextFontEngine
{
public:
    QCoreTextRawFontEngine(CGFontRef font, const QFontDef &def, const QByteArray &fontData)
        : QCoreTextFontEngine(font, def)
        , m_fontData(fontData)
    {}
    QFontEngine *cloneWithSize(qreal pixelSize) const
    {
        QFontDef newFontDef = fontDef;
        newFontDef.pixelSize = pixelSize;
        newFontDef.pointSize = pixelSize * 72.0 / qt_defaultDpi();

        return new QCoreTextRawFontEngine(cgFont, newFontDef, m_fontData);
    }
    QByteArray m_fontData;
};

QCoreTextFontEngine *QCoreTextFontEngine::create(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
    Q_UNUSED(hintingPreference);

    QCFType<CFDataRef> fontDataReference = fontData.toRawCFData();
    QCFType<CGDataProviderRef> dataProvider = CGDataProviderCreateWithCFData(fontDataReference);

    // Note: CTFontCreateWithGraphicsFont (which we call from the  QCoreTextFontEngine
    // constructor) has a bug causing it to retain the CGFontRef but never release it.
    // The result is that we are leaking the CGFont, CGDataProvider, and CGData, but
    // as the CGData is created from the raw QByteArray data, which we deref in the
    // subclass above during destruction, we're at least not leaking the font data,
    // (unless CoreText copies it internally). http://stackoverflow.com/questions/40805382/
    QCFType<CGFontRef> cgFont = CGFontCreateWithDataProvider(dataProvider);

    if (!cgFont) {
        qWarning("QCoreTextFontEngine::create: CGFontCreateWithDataProvider failed");
        return nullptr;
    }

    QFontDef def;
    def.pixelSize = pixelSize;
    def.pointSize = pixelSize * 72.0 / qt_defaultDpi();
    return new QCoreTextRawFontEngine(cgFont, def, fontData);
}

QCoreTextFontEngine::QCoreTextFontEngine(CTFontRef font, const QFontDef &def)
    : QFontEngine(Mac)
{
    fontDef = def;
    transform = qt_transform_from_fontdef(fontDef);
    ctfont = font;
    CFRetain(ctfont);
    cgFont = CTFontCopyGraphicsFont(font, NULL);
    init();
}

QCoreTextFontEngine::QCoreTextFontEngine(CGFontRef font, const QFontDef &def)
    : QFontEngine(Mac)
{
    fontDef = def;
    transform = qt_transform_from_fontdef(fontDef);
    cgFont = font;
    // Keep reference count balanced
    CFRetain(cgFont);
    ctfont = CTFontCreateWithGraphicsFont(font, fontDef.pixelSize, &transform, NULL);
    init();
}

QCoreTextFontEngine::~QCoreTextFontEngine()
{
    CFRelease(cgFont);
    CFRelease(ctfont);
}

void QCoreTextFontEngine::init()
{
    Q_ASSERT(ctfont != NULL);
    Q_ASSERT(cgFont != NULL);

    face_id.index = 0;
    QCFString name = CTFontCopyName(ctfont, kCTFontUniqueNameKey);
    face_id.filename = QString::fromCFString(name).toUtf8();

    QCFString family = CTFontCopyFamilyName(ctfont);
    fontDef.family = family;

    QCFString styleName = (CFStringRef) CTFontCopyAttribute(ctfont, kCTFontStyleNameAttribute);
    fontDef.styleName = styleName;

    synthesisFlags = 0;
    CTFontSymbolicTraits traits = CTFontGetSymbolicTraits(ctfont);

    if (traits & kCTFontColorGlyphsTrait)
        glyphFormat = QFontEngine::Format_ARGB;
    else
        glyphFormat = defaultGlyphFormat;

    if (traits & kCTFontItalicTrait)
        fontDef.style = QFont::StyleItalic;

    CFDictionaryRef allTraits = CTFontCopyTraits(ctfont);
    fontDef.weight = QCoreTextFontEngine::qtWeightFromCFWeight(getTraitValue(allTraits, kCTFontWeightTrait));
    int slant = static_cast<int>(getTraitValue(allTraits, kCTFontSlantTrait) * 500 + 500);
    if (slant > 500 && !(traits & kCTFontItalicTrait))
        fontDef.style = QFont::StyleOblique;
    CFRelease(allTraits);

    if (fontDef.weight >= QFont::Bold && !(traits & kCTFontBoldTrait))
        synthesisFlags |= SynthesizedBold;
    // XXX: we probably don't need to synthesis italic for oblique font
    if (fontDef.style != QFont::StyleNormal && !(traits & kCTFontItalicTrait))
        synthesisFlags |= SynthesizedItalic;

    avgCharWidth = 0;
    QByteArray os2Table = getSfntTable(MAKE_TAG('O', 'S', '/', '2'));
    unsigned emSize = CTFontGetUnitsPerEm(ctfont);
    if (os2Table.size() >= 10) {
        fsType = qFromBigEndian<quint16>(os2Table.constData() + 8);
        // qAbs is a workaround for weird fonts like Lucida Grande
        qint16 width = qAbs(qFromBigEndian<qint16>(os2Table.constData() + 2));
        avgCharWidth = QFixed::fromReal(width * fontDef.pixelSize / emSize);
    } else
        avgCharWidth = QFontEngine::averageCharWidth();

    underlineThickness = QFixed::fromReal(CTFontGetUnderlineThickness(ctfont));
    underlinePos = -QFixed::fromReal(CTFontGetUnderlinePosition(ctfont));

    cache_cost = (CTFontGetAscent(ctfont) + CTFontGetDescent(ctfont)) * avgCharWidth.toInt() * 2000;

    // HACK hb_coretext requires both CTFont and CGFont but user_data is only void*
    Q_ASSERT((void *)(&ctfont + 1) == (void *)&cgFont);
    faceData.user_data = &ctfont;
    faceData.get_font_table = ct_getSfntTable;

    kerningPairsLoaded = false;
}

glyph_t QCoreTextFontEngine::glyphIndex(uint ucs4) const
{
    int len = 0;

    QChar str[2];
    if (Q_UNLIKELY(QChar::requiresSurrogates(ucs4))) {
        str[len++] = QChar(QChar::highSurrogate(ucs4));
        str[len++] = QChar(QChar::lowSurrogate(ucs4));
    } else {
        str[len++] = QChar(ucs4);
    }

    CGGlyph glyphIndices[2];

    CTFontGetGlyphsForCharacters(ctfont, (const UniChar *)str, glyphIndices, len);

    return glyphIndices[0];
}

bool QCoreTextFontEngine::stringToCMap(const QChar *str, int len, QGlyphLayout *glyphs,
                                       int *nglyphs, QFontEngine::ShaperFlags flags) const
{
    Q_ASSERT(glyphs->numGlyphs >= *nglyphs);
    if (*nglyphs < len) {
        *nglyphs = len;
        return false;
    }

    QVarLengthArray<CGGlyph> cgGlyphs(len);
    CTFontGetGlyphsForCharacters(ctfont, (const UniChar*)str, cgGlyphs.data(), len);

    int glyph_pos = 0;
    for (int i = 0; i < len; ++i) {
        glyphs->glyphs[glyph_pos] = cgGlyphs[i];
        if (glyph_pos < i)
            cgGlyphs[glyph_pos] = cgGlyphs[i];
        glyph_pos++;

        // If it's a non-BMP char, skip the lower part of surrogate pair and go
        // directly to the next char without increasing glyph_pos
        if (str[i].isHighSurrogate() && i < len-1 && str[i+1].isLowSurrogate())
            ++i;
    }

    *nglyphs = glyph_pos;
    glyphs->numGlyphs = glyph_pos;

    if (flags & GlyphIndicesOnly)
        return true;

    QVarLengthArray<CGSize> advances(glyph_pos);
    CTFontGetAdvancesForGlyphs(ctfont, kCTFontOrientationHorizontal, cgGlyphs.data(), advances.data(), glyph_pos);

    for (int i = 0; i < glyph_pos; ++i) {
        if (glyphs->glyphs[i] & 0xff000000)
            continue;
        glyphs->advances[i] = QFixed::fromReal(advances[i].width);
    }

    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
        for (int i = 0; i < glyph_pos; ++i)
            glyphs->advances[i] = glyphs->advances[i].round();
    }
    return true;
}

glyph_metrics_t QCoreTextFontEngine::boundingBox(const QGlyphLayout &glyphs)
{
    QFixed w;
    bool round = fontDef.styleStrategy & QFont::ForceIntegerMetrics;

    for (int i = 0; i < glyphs.numGlyphs; ++i) {
        w += round ? glyphs.effectiveAdvance(i).round()
                   : glyphs.effectiveAdvance(i);
    }
    return glyph_metrics_t(0, -(ascent()), w - lastRightBearing(glyphs, round), ascent()+descent(), w, 0);
}

glyph_metrics_t QCoreTextFontEngine::boundingBox(glyph_t glyph)
{
    glyph_metrics_t ret;
    CGGlyph g = glyph;
    CGRect rect = CTFontGetBoundingRectsForGlyphs(ctfont, kCTFontOrientationHorizontal, &g, 0, 1);
    if (synthesisFlags & QFontEngine::SynthesizedItalic) {
        rect.size.width += rect.size.height * SYNTHETIC_ITALIC_SKEW;
    }
    ret.width = QFixed::fromReal(rect.size.width);
    ret.height = QFixed::fromReal(rect.size.height);
    ret.x = QFixed::fromReal(rect.origin.x);
    ret.y = -QFixed::fromReal(rect.origin.y) - ret.height;
    CGSize advances[1];
    CTFontGetAdvancesForGlyphs(ctfont, kCTFontOrientationHorizontal, &g, advances, 1);
    ret.xoff = QFixed::fromReal(advances[0].width);
    ret.yoff = QFixed::fromReal(advances[0].height);

    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
        ret.xoff = ret.xoff.round();
        ret.yoff = ret.yoff.round();
    }

    return ret;
}

QFixed QCoreTextFontEngine::ascent() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? QFixed::fromReal(CTFontGetAscent(ctfont)).round()
            : QFixed::fromReal(CTFontGetAscent(ctfont));
}

QFixed QCoreTextFontEngine::capHeight() const
{
    QFixed c = QFixed::fromReal(CTFontGetCapHeight(ctfont));
    if (c <= 0)
        return calculatedCapHeight();

    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
        c = c.round();

    return c;
}

QFixed QCoreTextFontEngine::descent() const
{
    QFixed d = QFixed::fromReal(CTFontGetDescent(ctfont));
    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
        d = d.round();

    return d;
}
QFixed QCoreTextFontEngine::leading() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? QFixed::fromReal(CTFontGetLeading(ctfont)).round()
            : QFixed::fromReal(CTFontGetLeading(ctfont));
}
QFixed QCoreTextFontEngine::xHeight() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? QFixed::fromReal(CTFontGetXHeight(ctfont)).round()
            : QFixed::fromReal(CTFontGetXHeight(ctfont));
}

QFixed QCoreTextFontEngine::averageCharWidth() const
{
    return (fontDef.styleStrategy & QFont::ForceIntegerMetrics)
            ? avgCharWidth.round() : avgCharWidth;
}

qreal QCoreTextFontEngine::maxCharWidth() const
{
    // ### FIXME: 'W' might not be the widest character, but this is better than nothing
    const glyph_t glyph = glyphIndex('W');
    glyph_metrics_t bb = const_cast<QCoreTextFontEngine *>(this)->boundingBox(glyph);
    return bb.xoff.toReal();
}

void QCoreTextFontEngine::draw(CGContextRef ctx, qreal x, qreal y, const QTextItemInt &ti, int paintDeviceHeight)
{
    QVarLengthArray<QFixedPoint> positions;
    QVarLengthArray<glyph_t> glyphs;
    QTransform matrix;
    matrix.translate(x, y);
    getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);
    if (glyphs.size() == 0)
        return;

    CGContextSetFontSize(ctx, fontDef.pixelSize);

    CGAffineTransform oldTextMatrix = CGContextGetTextMatrix(ctx);

    CGAffineTransform cgMatrix = CGAffineTransformMake(1, 0, 0, -1, 0, -paintDeviceHeight);

    CGAffineTransformConcat(cgMatrix, oldTextMatrix);

    if (synthesisFlags & QFontEngine::SynthesizedItalic)
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMake(1, 0, -SYNTHETIC_ITALIC_SKEW, 1, 0, 0));

    cgMatrix = CGAffineTransformConcat(cgMatrix, transform);

    CGContextSetTextMatrix(ctx, cgMatrix);

    CGContextSetTextDrawingMode(ctx, kCGTextFill);

    QVarLengthArray<CGPoint> cgPositions(glyphs.size());
    QVarLengthArray<CGGlyph> cgGlyphs(glyphs.size());
    const qreal firstX = positions[0].x.toReal();
    const qreal firstY = positions[0].y.toReal();
    for (int i = 0; i < glyphs.size(); ++i) {
        cgPositions[i].x = positions[i].x.toReal() - firstX;
        cgPositions[i].y = positions[i].y.toReal() - firstY;
        cgGlyphs[i] = glyphs[i];
    }

    //NSLog(@"Font inDraw %@  ctfont %@", CGFontCopyFullName(cgFont), CTFontCopyFamilyName(ctfont));

    CGContextSetTextPosition(ctx, positions[0].x.toReal(), positions[0].y.toReal());
    CTFontDrawGlyphs(ctfont, cgGlyphs.data(), cgPositions.data(), glyphs.size(), ctx);

    if (synthesisFlags & QFontEngine::SynthesizedBold) {
        CGContextSetTextPosition(ctx, positions[0].x.toReal() + 0.5 * lineThickness().toReal(),
                                 positions[0].y.toReal());
        CTFontDrawGlyphs(ctfont, cgGlyphs.data(), cgPositions.data(), glyphs.size(), ctx);
    }

    CGContextSetTextMatrix(ctx, oldTextMatrix);
}

struct ConvertPathInfo
{
    ConvertPathInfo(QPainterPath *newPath, const QPointF &newPos) : path(newPath), pos(newPos) {}
    QPainterPath *path;
    QPointF pos;
};

static void convertCGPathToQPainterPath(void *info, const CGPathElement *element)
{
    ConvertPathInfo *myInfo = static_cast<ConvertPathInfo *>(info);
    switch(element->type) {
        case kCGPathElementMoveToPoint:
            myInfo->path->moveTo(element->points[0].x + myInfo->pos.x(),
                                 element->points[0].y + myInfo->pos.y());
            break;
        case kCGPathElementAddLineToPoint:
            myInfo->path->lineTo(element->points[0].x + myInfo->pos.x(),
                                 element->points[0].y + myInfo->pos.y());
            break;
        case kCGPathElementAddQuadCurveToPoint:
            myInfo->path->quadTo(element->points[0].x + myInfo->pos.x(),
                                 element->points[0].y + myInfo->pos.y(),
                                 element->points[1].x + myInfo->pos.x(),
                                 element->points[1].y + myInfo->pos.y());
            break;
        case kCGPathElementAddCurveToPoint:
            myInfo->path->cubicTo(element->points[0].x + myInfo->pos.x(),
                                  element->points[0].y + myInfo->pos.y(),
                                  element->points[1].x + myInfo->pos.x(),
                                  element->points[1].y + myInfo->pos.y(),
                                  element->points[2].x + myInfo->pos.x(),
                                  element->points[2].y + myInfo->pos.y());
            break;
        case kCGPathElementCloseSubpath:
            myInfo->path->closeSubpath();
            break;
        default:
            qDebug() << "Unhandled path transform type: " << element->type;
    }

}

void QCoreTextFontEngine::addGlyphsToPath(glyph_t *glyphs, QFixedPoint *positions, int nGlyphs,
                                          QPainterPath *path, QTextItem::RenderFlags)
{
    if (glyphFormat == QFontEngine::Format_ARGB)
        return; // We can't convert color-glyphs to path

    CGAffineTransform cgMatrix = CGAffineTransformIdentity;
    cgMatrix = CGAffineTransformScale(cgMatrix, 1, -1);

    if (synthesisFlags & QFontEngine::SynthesizedItalic)
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMake(1, 0, -SYNTHETIC_ITALIC_SKEW, 1, 0, 0));

    for (int i = 0; i < nGlyphs; ++i) {
        QCFType<CGPathRef> cgpath = CTFontCreatePathForGlyph(ctfont, glyphs[i], &cgMatrix);
        ConvertPathInfo info(path, positions[i].toPointF());
        CGPathApply(cgpath, &info, convertCGPathToQPainterPath);
    }
}

static void qcoretextfontengine_scaleMetrics(glyph_metrics_t &br, const QTransform &matrix)
{
    if (matrix.isScaling()) {
        qreal hscale = matrix.m11();
        qreal vscale = matrix.m22();
        br.width  = QFixed::fromReal(br.width.toReal() * hscale);
        br.height = QFixed::fromReal(br.height.toReal() * vscale);
        br.x      = QFixed::fromReal(br.x.toReal() * hscale);
        br.y      = QFixed::fromReal(br.y.toReal() * vscale);
    }
}

glyph_metrics_t QCoreTextFontEngine::alphaMapBoundingBox(glyph_t glyph, QFixed subPixelPosition, const QTransform &matrix, GlyphFormat format)
{
    if (matrix.type() > QTransform::TxScale)
        return QFontEngine::alphaMapBoundingBox(glyph, subPixelPosition, matrix, format);

    glyph_metrics_t br = boundingBox(glyph);
    qcoretextfontengine_scaleMetrics(br, matrix);

    // Normalize width and height
    if (br.width < 0)
        br.width = -br.width;
    if (br.height < 0)
        br.height = -br.height;

    if (format == QFontEngine::Format_A8 || format == QFontEngine::Format_A32) {
        // Drawing a glyph at x-position 0 with anti-aliasing enabled
        // will potentially fill the pixel to the left of 0, as the
        // coordinates are not aligned to the center of pixels. To
        // prevent clipping of this pixel we need to shift the glyph
        // in the bitmap one pixel to the right. The shift needs to
        // be reflected in the glyph metrics as well, so that the final
        // position of the glyph is correct, which is why doing the
        // shift in imageForGlyph() is not enough.
        br.x -= 1;

        // As we've shifted the glyph one pixel to the right, we need
        // to expand the width of the alpha map bounding box as well.
        br.width += 1;

        // But we have the same anti-aliasing problem on the right
        // hand side of the glyph, eg. if the width of the glyph
        // results in the bounding rect landing between two pixels.
        // We pad the bounding rect again to account for the possible
        // anti-aliased drawing.
        br.width += 1;

        // We also shift the glyph to right right based on the subpixel
        // position, so we pad the bounding box to take account for the
        // subpixel positions that may result in the glyph being drawn
        // one pixel to the right of the 0-subpixel position.
        br.width += 1;

        // The same same logic as for the x-position needs to be applied
        // to the y-position, except we don't need to compensate for
        // the subpixel positioning.
        br.y -= 1;
        br.height += 2;
    }

    return br;
}

bool QCoreTextFontEngine::expectsGammaCorrectedBlending() const
{
    // Only works well when font-smoothing is enabled
    return (glyphFormat == Format_A32) && !(fontDef.styleStrategy & (QFont::NoAntialias | QFont::NoSubpixelAntialias));
}

QImage QCoreTextFontEngine::imageForGlyph(glyph_t glyph, QFixed subPixelPosition, bool aa, const QTransform &matrix)
{
    glyph_metrics_t br = alphaMapBoundingBox(glyph, subPixelPosition, matrix, glyphFormat);

    bool isColorGlyph = glyphFormat == QFontEngine::Format_ARGB;
    QImage::Format imageFormat = isColorGlyph ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
    QImage im(br.width.ceil().toInt(), br.height.ceil().toInt(), imageFormat);
    if (!im.width() || !im.height())
        return im;

#if defined(Q_OS_MACOS)
    CGColorRef glyphColor = CGColorGetConstantColor(kCGColorWhite);
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSMojave) {
        // macOS 10.14 uses a new font smoothing algorithm that takes the fill color into
        // account. This means our default approach of drawing white on black to produce
        // the alpha map will result in non-native looking text when then drawn as black
        // on white during the final blit. As a workaround we use the application's current
        // appearance to decide whether to draw with white or black fill, and then invert
        // the glyph image in the latter case, producing an alpha map. This covers the
        // most common use-cases, but longer term we should propagate the fill color all
        // the way from the paint engine, and include it in the key for the glyph cache.
        if (!qt_mac_applicationIsInDarkMode())
            glyphColor = CGColorGetConstantColor(kCGColorBlack);
    }
    const bool blackOnWhiteGlyphs = !isColorGlyph
            && CGColorEqualToColor(glyphColor, CGColorGetConstantColor(kCGColorBlack));
    if (blackOnWhiteGlyphs)
        im.fill(Qt::white);
    else
#endif
        im.fill(0); // Faster than Qt::black

    CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    uint cgflags = isColorGlyph ? kCGImageAlphaPremultipliedFirst : kCGImageAlphaNoneSkipFirst;
#ifdef kCGBitmapByteOrder32Host //only needed because CGImage.h added symbols in the minor version
    cgflags |= kCGBitmapByteOrder32Host;
#endif

    CGContextRef ctx = CGBitmapContextCreate(im.bits(), im.width(), im.height(),
                                             8, im.bytesPerLine(), colorspace,
                                             cgflags);
    Q_ASSERT(ctx);
    CGContextSetFontSize(ctx, fontDef.pixelSize);
    const bool antialias = (aa || fontDef.pointSize > antialiasingThreshold) && !(fontDef.styleStrategy & QFont::NoAntialias);
    CGContextSetShouldAntialias(ctx, antialias);
    const bool smoothing = antialias && !(fontDef.styleStrategy & QFont::NoSubpixelAntialias);
    CGContextSetShouldSmoothFonts(ctx, smoothing);

    CGAffineTransform cgMatrix = CGAffineTransformIdentity;

    if (synthesisFlags & QFontEngine::SynthesizedItalic)
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMake(1, 0, SYNTHETIC_ITALIC_SKEW, 1, 0, 0));

    if (!isColorGlyph) // CTFontDrawGlyphs incorporates the font's matrix already
        cgMatrix = CGAffineTransformConcat(cgMatrix, transform);

    if (matrix.isScaling())
        cgMatrix = CGAffineTransformConcat(cgMatrix, CGAffineTransformMakeScale(matrix.m11(), matrix.m22()));

    CGGlyph cgGlyph = glyph;
    qreal pos_x = -br.x.truncate() + subPixelPosition.toReal();
    qreal pos_y = im.height() + br.y.toReal();

    if (!isColorGlyph) {
        CGContextSetTextMatrix(ctx, cgMatrix);
#if defined(Q_OS_MACOS)
        CGContextSetFillColorWithColor(ctx, glyphColor);
#else
        CGContextSetRGBFillColor(ctx, 1, 1, 1, 1);
#endif
        CGContextSetTextDrawingMode(ctx, kCGTextFill);
        CGContextSetTextPosition(ctx, pos_x, pos_y);

        CTFontDrawGlyphs(ctfont, &cgGlyph, &CGPointZero, 1, ctx);

        if (synthesisFlags & QFontEngine::SynthesizedBold) {
            CGContextSetTextPosition(ctx, pos_x + 0.5 * lineThickness().toReal(), pos_y);
            CTFontDrawGlyphs(ctfont, &cgGlyph, &CGPointZero, 1, ctx);
        }
    } else {
        // CGContextSetTextMatrix does not work with color glyphs, so we use
        // the CTM instead. This means we must translate the CTM as well, to
        // set the glyph position, instead of using CGContextSetTextPosition.
        CGContextTranslateCTM(ctx, pos_x, pos_y);
        CGContextConcatCTM(ctx, cgMatrix);

        // CGContextShowGlyphsWithAdvances does not support the 'sbix' color-bitmap
        // glyphs in the Apple Color Emoji font, so we use CTFontDrawGlyphs instead.
        CTFontDrawGlyphs(ctfont, &cgGlyph, &CGPointZero, 1, ctx);
    }

    CGContextRelease(ctx);
    CGColorSpaceRelease(colorspace);

#if defined(Q_OS_MACOS)
    if (blackOnWhiteGlyphs)
        im.invertPixels();
#endif

    return im;
}

QImage QCoreTextFontEngine::alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition)
{
    return alphaMapForGlyph(glyph, subPixelPosition, QTransform());
}

QImage QCoreTextFontEngine::alphaMapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &x)
{
    if (x.type() > QTransform::TxScale)
        return QFontEngine::alphaMapForGlyph(glyph, subPixelPosition, x);

    QImage im = imageForGlyph(glyph, subPixelPosition, false, x);

    QImage alphaMap(im.width(), im.height(), QImage::Format_Alpha8);

    for (int y=0; y<im.height(); ++y) {
        uint *src = (uint*) im.scanLine(y);
        uchar *dst = alphaMap.scanLine(y);
        for (int x=0; x<im.width(); ++x) {
            *dst = qGray(*src);
            ++dst;
            ++src;
        }
    }

    return alphaMap;
}

QImage QCoreTextFontEngine::alphaRGBMapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &x)
{
    if (x.type() > QTransform::TxScale)
        return QFontEngine::alphaRGBMapForGlyph(glyph, subPixelPosition, x);

    QImage im = imageForGlyph(glyph, subPixelPosition, true, x);
    qGamma_correct_back_to_linear_cs(&im);
    return im;
}

QImage QCoreTextFontEngine::bitmapForGlyph(glyph_t glyph, QFixed subPixelPosition, const QTransform &t)
{
    if (t.type() > QTransform::TxScale)
        return QFontEngine::bitmapForGlyph(glyph, subPixelPosition, t);

    return imageForGlyph(glyph, subPixelPosition, true, t);
}

void QCoreTextFontEngine::recalcAdvances(QGlyphLayout *glyphs, QFontEngine::ShaperFlags flags) const
{
    int i, numGlyphs = glyphs->numGlyphs;
    QVarLengthArray<CGGlyph> cgGlyphs(numGlyphs);

    for (i = 0; i < numGlyphs; ++i) {
        if (glyphs->glyphs[i] & 0xff000000)
            cgGlyphs[i] = 0;
        else
            cgGlyphs[i] = glyphs->glyphs[i];
    }

    loadAdvancesForGlyphs(ctfont, cgGlyphs, glyphs, numGlyphs, flags, fontDef);
}

QFontEngine::FaceId QCoreTextFontEngine::faceId() const
{
    return face_id;
}

bool QCoreTextFontEngine::canRender(const QChar *string, int len) const
{
    QVarLengthArray<CGGlyph> cgGlyphs(len);
    return CTFontGetGlyphsForCharacters(ctfont, (const UniChar *) string, cgGlyphs.data(), len);
}

bool QCoreTextFontEngine::getSfntTableData(uint tag, uchar *buffer, uint *length) const
{
    return ct_getSfntTable((void *)&ctfont, tag, buffer, length);
}

void QCoreTextFontEngine::getUnscaledGlyph(glyph_t glyph, QPainterPath *path, glyph_metrics_t *metric)
{
    CGAffineTransform cgMatrix = CGAffineTransformIdentity;

    qreal emSquare = CTFontGetUnitsPerEm(ctfont);
    qreal scale = emSquare / CTFontGetSize(ctfont);
    cgMatrix = CGAffineTransformScale(cgMatrix, scale, -scale);

    QCFType<CGPathRef> cgpath = CTFontCreatePathForGlyph(ctfont, (CGGlyph) glyph, &cgMatrix);
    ConvertPathInfo info(path, QPointF(0,0));
    CGPathApply(cgpath, &info, convertCGPathToQPainterPath);

    *metric = boundingBox(glyph);
    // scale the metrics too
    metric->width  = QFixed::fromReal(metric->width.toReal() * scale);
    metric->height = QFixed::fromReal(metric->height.toReal() * scale);
    metric->x      = QFixed::fromReal(metric->x.toReal() * scale);
    metric->y      = QFixed::fromReal(metric->y.toReal() * scale);
    metric->xoff   = QFixed::fromReal(metric->xoff.toReal() * scale);
    metric->yoff   = QFixed::fromReal(metric->yoff.toReal() * scale);
}

QFixed QCoreTextFontEngine::emSquareSize() const
{
    return QFixed(int(CTFontGetUnitsPerEm(ctfont)));
}

QFontEngine *QCoreTextFontEngine::cloneWithSize(qreal pixelSize) const
{
    QFontDef newFontDef = fontDef;
    newFontDef.pixelSize = pixelSize;
    newFontDef.pointSize = pixelSize * 72.0 / qt_defaultDpi();

    return new QCoreTextFontEngine(cgFont, newFontDef);
}

Qt::HANDLE QCoreTextFontEngine::handle() const
{
    return (Qt::HANDLE)ctfont;
}

bool QCoreTextFontEngine::supportsTransformation(const QTransform &transform) const
{
    if (transform.type() < QTransform::TxScale)
        return true;
    else if (transform.type() == QTransform::TxScale &&
             transform.m11() >= 0 && transform.m22() >= 0)
        return true;
    else
        return false;
}

QFixed QCoreTextFontEngine::lineThickness() const
{
    return underlineThickness;
}

QFixed QCoreTextFontEngine::underlinePosition() const
{
    return underlinePos;
}

QFontEngine::Properties QCoreTextFontEngine::properties() const
{
    Properties result;

    QCFString psName, copyright;
    psName = CTFontCopyPostScriptName(ctfont);
    copyright = CTFontCopyName(ctfont, kCTFontCopyrightNameKey);
    result.postscriptName = QString::fromCFString(psName).toUtf8();
    result.copyright = QString::fromCFString(copyright).toUtf8();

    qreal emSquare = CTFontGetUnitsPerEm(ctfont);
    qreal scale = emSquare / CTFontGetSize(ctfont);

    CGRect cgRect = CTFontGetBoundingBox(ctfont);
    result.boundingBox = QRectF(cgRect.origin.x * scale,
                                -CTFontGetAscent(ctfont) * scale,
                                cgRect.size.width * scale,
                                cgRect.size.height * scale);

    result.emSquare = emSquareSize();
    result.ascent = QFixed::fromReal(CTFontGetAscent(ctfont) * scale);
    result.descent = QFixed::fromReal(CTFontGetDescent(ctfont) * scale);
    result.leading = QFixed::fromReal(CTFontGetLeading(ctfont) * scale);
    result.italicAngle = QFixed::fromReal(CTFontGetSlantAngle(ctfont));
    result.capHeight = QFixed::fromReal(CTFontGetCapHeight(ctfont) * scale);
    result.lineWidth = QFixed::fromReal(CTFontGetUnderlineThickness(ctfont) * scale);

    if (fontDef.styleStrategy & QFont::ForceIntegerMetrics) {
        result.ascent = result.ascent.round();
        result.descent = result.descent.round();
        result.leading = result.leading.round();
        result.italicAngle = result.italicAngle.round();
        result.capHeight = result.capHeight.round();
        result.lineWidth = result.lineWidth.round();
    }

    return result;
}

void QCoreTextFontEngine::doKerning(QGlyphLayout *g, ShaperFlags flags) const
{
    if (!kerningPairsLoaded) {
        kerningPairsLoaded = true;
        qreal emSquare = CTFontGetUnitsPerEm(ctfont);
        qreal scale = emSquare / CTFontGetSize(ctfont);

        const_cast<QCoreTextFontEngine *>(this)->loadKerningPairs(QFixed::fromReal(scale));
    }

    QFontEngine::doKerning(g, flags);
}

QT_END_NAMESPACE
