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

#include "qfont.h"
#include "qpaintdevice.h"
#include "qfontmetrics.h"

#include "qfont_p.h"
#include "qfontengine_p.h"

QT_BEGIN_NAMESPACE


extern void qt_format_text(const QFont& font, const QRectF &_r,
                           int tf, const QString &text, QRectF *brect,
                           int tabStops, int *tabArray, int tabArrayLen,
                           QPainter *painter);

/*****************************************************************************
  QFontMetrics member functions
 *****************************************************************************/

/*!
    \class QFontMetrics
    \reentrant
    \inmodule QtGui

    \brief The QFontMetrics class provides font metrics information.

    \ingroup painting
    \ingroup shared

    QFontMetrics functions calculate the size of characters and
    strings for a given font. There are three ways you can create a
    QFontMetrics object:

    \list 1
    \li Calling the QFontMetrics constructor with a QFont creates a
    font metrics object for a screen-compatible font, i.e. the font
    cannot be a printer font. If the font is changed
    later, the font metrics object is \e not updated.

    (Note: If you use a printer font the values returned may be
    inaccurate. Printer fonts are not always accessible so the nearest
    screen font is used if a printer font is supplied.)

    \li QWidget::fontMetrics() returns the font metrics for a widget's
    font. This is equivalent to QFontMetrics(widget->font()). If the
    widget's font is changed later, the font metrics object is \e not
    updated.

    \li QPainter::fontMetrics() returns the font metrics for a
    painter's current font. If the painter's font is changed later, the
    font metrics object is \e not updated.
    \endlist

    Once created, the object provides functions to access the
    individual metrics of the font, its characters, and for strings
    rendered in the font.

    There are several functions that operate on the font: ascent(),
    descent(), height(), leading() and lineSpacing() return the basic
    size properties of the font. The underlinePos(), overlinePos(),
    strikeOutPos() and lineWidth() functions, return the properties of
    the line that underlines, overlines or strikes out the
    characters. These functions are all fast.

    There are also some functions that operate on the set of glyphs in
    the font: minLeftBearing(), minRightBearing() and maxWidth().
    These are by necessity slow, and we recommend avoiding them if
    possible.

    For each character, you can get its width(), leftBearing() and
    rightBearing() and find out whether it is in the font using
    inFont(). You can also treat the character as a string, and use
    the string functions on it.

    The string functions include width(), to return the width of a
    string in pixels (or points, for a printer), boundingRect(), to
    return a rectangle large enough to contain the rendered string,
    and size(), to return the size of that rectangle.

    Example:
    \snippet code/src_gui_text_qfontmetrics.cpp 0

    \sa QFont, QFontInfo, QFontDatabase, {Character Map Example}
*/

/*!
    \fn QRect QFontMetrics::boundingRect(int x, int y, int width, int height,
        int flags, const QString &text, int tabStops, int *tabArray) const
    \overload

    Returns the bounding rectangle for the given \a text within the
    rectangle specified by the \a x and \a y coordinates, \a width, and
    \a height.

    If Qt::TextExpandTabs is set in \a flags and \a tabArray is
    non-null, it specifies a 0-terminated sequence of pixel-positions
    for tabs; otherwise, if \a tabStops is non-zero, it is used as the
    tab spacing (in pixels).
*/

/*!
    Constructs a font metrics object for \a font.

    The font metrics will be compatible with the paintdevice used to
    create \a font.

    The font metrics object holds the information for the font that is
    passed in the constructor at the time it is created, and is not
    updated if the font's attributes are changed later.

    Use QFontMetrics(const QFont &, QPaintDevice *) to get the font
    metrics that are compatible with a certain paint device.
*/
QFontMetrics::QFontMetrics(const QFont &font)
    : d(font.d)
{
}

/*!
    Constructs a font metrics object for \a font and \a paintdevice.

    The font metrics will be compatible with the paintdevice passed.
    If the \a paintdevice is 0, the metrics will be screen-compatible,
    ie. the metrics you get if you use the font for drawing text on a
    \l{QWidget}{widgets} or \l{QPixmap}{pixmaps},
    not on a QPicture or QPrinter.

    The font metrics object holds the information for the font that is
    passed in the constructor at the time it is created, and is not
    updated if the font's attributes are changed later.
*/
QFontMetrics::QFontMetrics(const QFont &font, QPaintDevice *paintdevice)
{
    int dpi = paintdevice ? paintdevice->logicalDpiY() : qt_defaultDpi();
    const int screen = 0;
    if (font.d->dpi != dpi || font.d->screen != screen ) {
        d = new QFontPrivate(*font.d);
        d->dpi = dpi;
        d->screen = screen;
    } else {
        d = font.d;
    }

}

/*!
    Constructs a copy of \a fm.
*/
QFontMetrics::QFontMetrics(const QFontMetrics &fm)
    : d(fm.d)
{
}

/*!
    Destroys the font metrics object and frees all allocated
    resources.
*/
QFontMetrics::~QFontMetrics()
{
}

/*!
    Assigns the font metrics \a fm.
*/
QFontMetrics &QFontMetrics::operator=(const QFontMetrics &fm)
{
    d = fm.d;
    return *this;
}

/*!
    \fn QFontMetrics &QFontMetrics::operator=(QFontMetrics &&other)

    Move-assigns \a other to this QFontMetrics instance.

    \since 5.2
*/
/*!
    \fn QFontMetricsF &QFontMetricsF::operator=(QFontMetricsF &&other)

    Move-assigns \a other to this QFontMetricsF instance.
*/

/*!
    \fn void QFontMetrics::swap(QFontMetrics &other)
    \since 5.0

    Swaps this font metrics instance with \a other. This function is
    very fast and never fails.
*/

/*!
    Returns \c true if \a other is equal to this object; otherwise
    returns \c false.

    Two font metrics are considered equal if they were constructed
    from the same QFont and the paint devices they were constructed
    for are considered compatible.

    \sa operator!=()
*/
bool QFontMetrics::operator ==(const QFontMetrics &other) const
{
    return d == other.d;
}

/*!
    \fn bool QFontMetrics::operator !=(const QFontMetrics &other) const

    Returns \c true if \a other is not equal to this object; otherwise returns \c false.

    Two font metrics are considered equal if they were constructed
    from the same QFont and the paint devices they were constructed
    for are considered compatible.

    \sa operator==()
*/

/*!
    Returns the ascent of the font.

    The ascent of a font is the distance from the baseline to the
    highest position characters extend to. In practice, some font
    designers break this rule, e.g. when they put more than one accent
    on top of a character, or to accommodate an unusual character in
    an exotic language, so it is possible (though rare) that this
    value will be too small.

    \sa descent()
*/
int QFontMetrics::ascent() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->ascent());
}

/*!
    Returns the cap height of the font.

    \since 5.8

    The cap height of a font is the height of a capital letter above
    the baseline. It specifically is the height of capital letters
    that are flat - such as H or I - as opposed to round letters such
    as O, or pointed letters like A, both of which may display overshoot.

    \sa ascent()
*/
int QFontMetrics::capHeight() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->capHeight());
}

/*!
    Returns the descent of the font.

    The descent is the distance from the base line to the lowest point
    characters extend to. In practice, some font designers break this rule,
    e.g. to accommodate an unusual character in an exotic language, so
    it is possible (though rare) that this value will be too small.

    \sa ascent()
*/
int QFontMetrics::descent() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->descent());
}

/*!
    Returns the height of the font.

    This is always equal to ascent()+descent().

    \sa leading(), lineSpacing()
*/
int QFontMetrics::height() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->ascent()) + qRound(engine->descent());
}

/*!
    Returns the leading of the font.

    This is the natural inter-line spacing.

    \sa height(), lineSpacing()
*/
int QFontMetrics::leading() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->leading());
}

/*!
    Returns the distance from one base line to the next.

    This value is always equal to leading()+height().

    \sa height(), leading()
*/
int QFontMetrics::lineSpacing() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->leading()) + qRound(engine->ascent()) + qRound(engine->descent());
}

/*!
    Returns the minimum left bearing of the font.

    This is the smallest leftBearing(char) of all characters in the
    font.

    Note that this function can be very slow if the font is large.

    \sa minRightBearing(), leftBearing()
*/
int QFontMetrics::minLeftBearing() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->minLeftBearing());
}

/*!
    Returns the minimum right bearing of the font.

    This is the smallest rightBearing(char) of all characters in the
    font.

    Note that this function can be very slow if the font is large.

    \sa minLeftBearing(), rightBearing()
*/
int QFontMetrics::minRightBearing() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->minRightBearing());
}

/*!
    Returns the width of the widest character in the font.
*/
int QFontMetrics::maxWidth() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->maxCharWidth());
}

/*!
    Returns the 'x' height of the font. This is often but not always
    the same as the height of the character 'x'.
*/
int QFontMetrics::xHeight() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    if (d->capital == QFont::SmallCaps)
        return qRound(d->smallCapsFontPrivate()->engineForScript(QChar::Script_Common)->ascent());
    return qRound(engine->xHeight());
}

/*!
    \since 4.2

    Returns the average width of glyphs in the font.
*/
int QFontMetrics::averageCharWidth() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->averageCharWidth());
}

/*!
    Returns \c true if character \a ch is a valid character in the font;
    otherwise returns \c false.
*/
bool QFontMetrics::inFont(QChar ch) const
{
    return inFontUcs4(ch.unicode());
}

/*!
   Returns \c true if the character \a ucs4 encoded in UCS-4/UTF-32 is a valid
   character in the font; otherwise returns \c false.
*/
bool QFontMetrics::inFontUcs4(uint ucs4) const
{
    const int script = QChar::script(ucs4);
    QFontEngine *engine = d->engineForScript(script);
    Q_ASSERT(engine != 0);
    if (engine->type() == QFontEngine::Box)
        return false;
    return engine->canRender(ucs4);
}

/*!
    Returns the left bearing of character \a ch in the font.

    The left bearing is the right-ward distance of the left-most pixel
    of the character from the logical origin of the character. This
    value is negative if the pixels of the character extend to the
    left of the logical origin.

    See width() for a graphical description of this metric.

    \sa rightBearing(), minLeftBearing(), width()
*/
int QFontMetrics::leftBearing(QChar ch) const
{
    const int script = ch.script();
    QFontEngine *engine;
    if (d->capital == QFont::SmallCaps && ch.isLower())
        engine = d->smallCapsFontPrivate()->engineForScript(script);
    else
        engine = d->engineForScript(script);
    Q_ASSERT(engine != 0);
    if (engine->type() == QFontEngine::Box)
        return 0;

    d->alterCharForCapitalization(ch);

    glyph_t glyph = engine->glyphIndex(ch.unicode());

    qreal lb;
    engine->getGlyphBearings(glyph, &lb);
    return qRound(lb);
}

/*!
    Returns the right bearing of character \a ch in the font.

    The right bearing is the left-ward distance of the right-most
    pixel of the character from the logical origin of a subsequent
    character. This value is negative if the pixels of the character
    extend to the right of the width() of the character.

    See width() for a graphical description of this metric.

    \sa leftBearing(), minRightBearing(), width()
*/
int QFontMetrics::rightBearing(QChar ch) const
{
    const int script = ch.script();
    QFontEngine *engine;
    if (d->capital == QFont::SmallCaps && ch.isLower())
        engine = d->smallCapsFontPrivate()->engineForScript(script);
    else
        engine = d->engineForScript(script);
    Q_ASSERT(engine != 0);
    if (engine->type() == QFontEngine::Box)
        return 0;

    d->alterCharForCapitalization(ch);

    glyph_t glyph = engine->glyphIndex(ch.unicode());

    qreal rb;
    engine->getGlyphBearings(glyph, 0, &rb);
    return qRound(rb);
}

#if QT_DEPRECATED_SINCE(5, 11)
/*!
    Returns the width in pixels of the first \a len characters of \a
    text. If \a len is negative (the default), the entire string is
    used.

    Note that this value is \e not equal to boundingRect().width();
    boundingRect() returns a rectangle describing the pixels this
    string will cover whereas width() returns the distance to where
    the next string should be drawn.

    \deprecated in Qt 5.11. Use horizontalAdvance() instead.

    \sa boundingRect()
*/
int QFontMetrics::width(const QString &text, int len) const
{
    return horizontalAdvance(text, len);
}

/*!
    \internal
*/
int QFontMetrics::width(const QString &text, int len, int flags) const
{
#if QT_DEPRECATED_SINCE(5, 11) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (flags & Qt::TextBypassShaping) {
        int pos = text.indexOf(QLatin1Char('\x9c'));
        if (pos != -1) {
            len = (len < 0) ? pos : qMin(pos, len);
        } else if (len < 0) {
            len = text.length();
        }
        if (len == 0)
            return 0;

        // Skip complex shaping, only use advances
        int numGlyphs = len;
        QVarLengthGlyphLayoutArray glyphs(numGlyphs);
        QFontEngine *engine = d->engineForScript(QChar::Script_Common);
        if (!engine->stringToCMap(text.data(), len, &glyphs, &numGlyphs, 0))
            Q_UNREACHABLE();

        QFixed width;
        for (int i = 0; i < numGlyphs; ++i)
            width += glyphs.advances[i];
        return qRound(width);
    }
#else
    Q_UNUSED(flags)
#endif

    return horizontalAdvance(text, len);
}

/*!
    \overload

    \image bearings.png Bearings

    Returns the logical width of character \a ch in pixels. This is a
    distance appropriate for drawing a subsequent character after \a
    ch.

    Some of the metrics are described in the image to the right. The
    central dark rectangles cover the logical width() of each
    character. The outer pale rectangles cover the leftBearing() and
    rightBearing() of each character. Notice that the bearings of "f"
    in this particular font are both negative, while the bearings of
    "o" are both positive.

    \deprecated in Qt 5.11. Use horizontalAdvance() instead.

    \warning This function will produce incorrect results for Arabic
    characters or non-spacing marks in the middle of a string, as the
    glyph shaping and positioning of marks that happens when
    processing strings cannot be taken into account. When implementing
    an interactive text control, use QTextLayout instead.

    \sa boundingRect()
*/
int QFontMetrics::width(QChar ch) const
{
    return horizontalAdvance(ch);
}
#endif // QT_DEPRECATED_SINCE(5, 11)

/*!
    Returns the horizontal advance in pixels of the first \a len characters of \a
    text. If \a len is negative (the default), the entire string is
    used.

    This is the distance appropriate for drawing a subsequent character
    after \a text.

    \since 5.11

    \sa boundingRect()
*/
int QFontMetrics::horizontalAdvance(const QString &text, int len) const
{
    int pos = text.indexOf(QLatin1Char('\x9c'));
    if (pos != -1) {
        len = (len < 0) ? pos : qMin(pos, len);
    } else if (len < 0) {
        len = text.length();
    }
    if (len == 0)
        return 0;

    QStackTextEngine layout(text, QFont(d.data()));
    return qRound(layout.width(0, len));
}

/*!
    \overload

    \image bearings.png Bearings

    Returns the horizontal advance of character \a ch in pixels. This is a
    distance appropriate for drawing a subsequent character after \a
    ch.

    Some of the metrics are described in the image. The
    central dark rectangles cover the logical horizontalAdvance() of each
    character. The outer pale rectangles cover the leftBearing() and
    rightBearing() of each character. Notice that the bearings of "f"
    in this particular font are both negative, while the bearings of
    "o" are both positive.

    \warning This function will produce incorrect results for Arabic
    characters or non-spacing marks in the middle of a string, as the
    glyph shaping and positioning of marks that happens when
    processing strings cannot be taken into account. When implementing
    an interactive text control, use QTextLayout instead.

    \since 5.11

    \sa boundingRect()
*/
int QFontMetrics::horizontalAdvance(QChar ch) const
{
    if (QChar::category(ch.unicode()) == QChar::Mark_NonSpacing)
        return 0;

    const int script = ch.script();
    QFontEngine *engine;
    if (d->capital == QFont::SmallCaps && ch.isLower())
        engine = d->smallCapsFontPrivate()->engineForScript(script);
    else
        engine = d->engineForScript(script);
    Q_ASSERT(engine != 0);

    d->alterCharForCapitalization(ch);

    glyph_t glyph = engine->glyphIndex(ch.unicode());
    QFixed advance;

    QGlyphLayout glyphs;
    glyphs.numGlyphs = 1;
    glyphs.glyphs = &glyph;
    glyphs.advances = &advance;
    engine->recalcAdvances(&glyphs, 0);

    return qRound(advance);
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
/*! \obsolete

    Returns the width of the character at position \a pos in the
    string \a text.

    The whole string is needed, as the glyph drawn may change
    depending on the context (the letter before and after the current
    one) for some languages (e.g. Arabic).

    This function also takes non spacing marks and ligatures into
    account.
*/
int QFontMetrics::charWidth(const QString &text, int pos) const
{
    int width = 0;
    if (pos < 0 || pos > (int)text.length())
        return width;

    QChar ch = text.at(pos);
    const int script = ch.script();
    if (script != QChar::Script_Common) {
        // complex script shaping. Have to do some hard work
        int from = qMax(0, pos - 8);
        int to = qMin(text.length(), pos + 8);
        QString cstr = QString::fromRawData(text.unicode() + from, to - from);
        QStackTextEngine layout(cstr, QFont(d.data()));
        layout.ignoreBidi = true;
        layout.itemize();
        width = qRound(layout.width(pos-from, 1));
    } else if (ch.category() != QChar::Mark_NonSpacing) {
        QFontEngine *engine;
        if (d->capital == QFont::SmallCaps && ch.isLower())
            engine = d->smallCapsFontPrivate()->engineForScript(script);
        else
            engine = d->engineForScript(script);
        Q_ASSERT(engine != 0);

        d->alterCharForCapitalization(ch);

        glyph_t glyph = engine->glyphIndex(ch.unicode());
        QFixed advance;

        QGlyphLayout glyphs;
        glyphs.numGlyphs = 1;
        glyphs.glyphs = &glyph;
        glyphs.advances = &advance;
        engine->recalcAdvances(&glyphs, 0);

        width = qRound(advance);
    }
    return width;
}
#endif

/*!
    Returns the bounding rectangle of the characters in the string
    specified by \a text. The bounding rectangle always covers at least
    the set of pixels the text would cover if drawn at (0, 0).

    Note that the bounding rectangle may extend to the left of (0, 0),
    e.g. for italicized fonts, and that the width of the returned
    rectangle might be different than what the width() method returns.

    If you want to know the advance width of the string (to lay out
    a set of strings next to each other), use horizontalAdvance() instead.

    Newline characters are processed as normal characters, \e not as
    linebreaks.

    The height of the bounding rectangle is at least as large as the
    value returned by height().

    \sa width(), height(), QPainter::boundingRect(), tightBoundingRect()
*/
QRect QFontMetrics::boundingRect(const QString &text) const
{
    if (text.length() == 0)
        return QRect();

    QStackTextEngine layout(text, QFont(d.data()));
    layout.itemize();
    glyph_metrics_t gm = layout.boundingBox(0, text.length());
    return QRect(qRound(gm.x), qRound(gm.y), qRound(gm.width), qRound(gm.height));
}

/*!
    Returns the rectangle that is covered by ink if character \a ch
    were to be drawn at the origin of the coordinate system.

    Note that the bounding rectangle may extend to the left of (0, 0)
    (e.g., for italicized fonts), and that the text output may cover \e
    all pixels in the bounding rectangle. For a space character the rectangle
    will usually be empty.

    Note that the rectangle usually extends both above and below the
    base line.

    \warning The width of the returned rectangle is not the advance width
    of the character. Use boundingRect(const QString &) or horizontalAdvance() instead.

    \sa width()
*/
QRect QFontMetrics::boundingRect(QChar ch) const
{
    const int script = ch.script();
    QFontEngine *engine;
    if (d->capital == QFont::SmallCaps && ch.isLower())
        engine = d->smallCapsFontPrivate()->engineForScript(script);
    else
        engine = d->engineForScript(script);
    Q_ASSERT(engine != 0);

    d->alterCharForCapitalization(ch);

    glyph_t glyph = engine->glyphIndex(ch.unicode());

    glyph_metrics_t gm = engine->boundingBox(glyph);
    return QRect(qRound(gm.x), qRound(gm.y), qRound(gm.width), qRound(gm.height));
}

/*!
    \overload

    Returns the bounding rectangle of the characters in the string
    specified by \a text, which is the set of pixels the text would
    cover if drawn at (0, 0). The drawing, and hence the bounding
    rectangle, is constrained to the rectangle \a rect.

    The \a flags argument is the bitwise OR of the following flags:
    \list
    \li Qt::AlignLeft aligns to the left border, except for
          Arabic and Hebrew where it aligns to the right.
    \li Qt::AlignRight aligns to the right border, except for
          Arabic and Hebrew where it aligns to the left.
    \li Qt::AlignJustify produces justified text.
    \li Qt::AlignHCenter aligns horizontally centered.
    \li Qt::AlignTop aligns to the top border.
    \li Qt::AlignBottom aligns to the bottom border.
    \li Qt::AlignVCenter aligns vertically centered
    \li Qt::AlignCenter (== \c{Qt::AlignHCenter | Qt::AlignVCenter})
    \li Qt::TextSingleLine ignores newline characters in the text.
    \li Qt::TextExpandTabs expands tabs (see below)
    \li Qt::TextShowMnemonic interprets "&x" as \underline{x}; i.e., underlined.
    \li Qt::TextWordWrap breaks the text to fit the rectangle.
    \endlist

    Qt::Horizontal alignment defaults to Qt::AlignLeft and vertical
    alignment defaults to Qt::AlignTop.

    If several of the horizontal or several of the vertical alignment
    flags are set, the resulting alignment is undefined.

    If Qt::TextExpandTabs is set in \a flags, then: if \a tabArray is
    non-null, it specifies a 0-terminated sequence of pixel-positions
    for tabs; otherwise if \a tabStops is non-zero, it is used as the
    tab spacing (in pixels).

    Note that the bounding rectangle may extend to the left of (0, 0),
    e.g. for italicized fonts, and that the text output may cover \e
    all pixels in the bounding rectangle.

    Newline characters are processed as linebreaks.

    Despite the different actual character heights, the heights of the
    bounding rectangles of "Yes" and "yes" are the same.

    The bounding rectangle returned by this function is somewhat larger
    than that calculated by the simpler boundingRect() function. This
    function uses the \l{minLeftBearing()}{maximum left} and
    \l{minRightBearing()}{right} font bearings as is
    necessary for multi-line text to align correctly. Also,
    fontHeight() and lineSpacing() are used to calculate the height,
    rather than individual character heights.

    \sa width(), QPainter::boundingRect(), Qt::Alignment
*/
QRect QFontMetrics::boundingRect(const QRect &rect, int flags, const QString &text, int tabStops,
                                 int *tabArray) const
{
    int tabArrayLen = 0;
    if (tabArray)
        while (tabArray[tabArrayLen])
            tabArrayLen++;

    QRectF rb;
    QRectF rr(rect);
    qt_format_text(QFont(d.data()), rr, flags | Qt::TextDontPrint, text, &rb, tabStops, tabArray,
                   tabArrayLen, 0);

    return rb.toAlignedRect();
}

/*!
    Returns the size in pixels of \a text.

    The \a flags argument is the bitwise OR of the following flags:
    \list
    \li Qt::TextSingleLine ignores newline characters.
    \li Qt::TextExpandTabs expands tabs (see below)
    \li Qt::TextShowMnemonic interprets "&x" as \underline{x}; i.e., underlined.
    \li Qt::TextWordWrap breaks the text to fit the rectangle.
    \endlist

    If Qt::TextExpandTabs is set in \a flags, then: if \a tabArray is
    non-null, it specifies a 0-terminated sequence of pixel-positions
    for tabs; otherwise if \a tabStops is non-zero, it is used as the
    tab spacing (in pixels).

    Newline characters are processed as linebreaks.

    Despite the different actual character heights, the heights of the
    bounding rectangles of "Yes" and "yes" are the same.

    \sa boundingRect()
*/
QSize QFontMetrics::size(int flags, const QString &text, int tabStops, int *tabArray) const
{
    return boundingRect(QRect(0,0,0,0), flags | Qt::TextLongestVariant, text, tabStops, tabArray).size();
}

/*!
  \since 4.3

    Returns a tight bounding rectangle around the characters in the
    string specified by \a text. The bounding rectangle always covers
    at least the set of pixels the text would cover if drawn at (0,
    0).

    Note that the bounding rectangle may extend to the left of (0, 0),
    e.g. for italicized fonts, and that the width of the returned
    rectangle might be different than what the width() method returns.

    If you want to know the advance width of the string (to lay out
    a set of strings next to each other), use horizontalAdvance() instead.

    Newline characters are processed as normal characters, \e not as
    linebreaks.

    \warning Calling this method is very slow on Windows.

    \sa width(), height(), boundingRect()
*/
QRect QFontMetrics::tightBoundingRect(const QString &text) const
{
    if (text.length() == 0)
        return QRect();

    QStackTextEngine layout(text, QFont(d.data()));
    layout.itemize();
    glyph_metrics_t gm = layout.tightBoundingBox(0, text.length());
    return QRect(qRound(gm.x), qRound(gm.y), qRound(gm.width), qRound(gm.height));
}


/*!
    \since 4.2

    If the string \a text is wider than \a width, returns an elided
    version of the string (i.e., a string with "..." in it).
    Otherwise, returns the original string.

    The \a mode parameter specifies whether the text is elided on the
    left (e.g., "...tech"), in the middle (e.g., "Tr...ch"), or on
    the right (e.g., "Trol...").

    The \a width is specified in pixels, not characters.

    The \a flags argument is optional and currently only supports
    Qt::TextShowMnemonic as value.

    The elide mark follows the \l{Qt::LayoutDirection}{layoutdirection}.
    For example, it will be on the right side of the text for right-to-left
    layouts if the \a mode is \c{Qt::ElideLeft}, and on the left side of the
    text if the \a mode is \c{Qt::ElideRight}.

*/
QString QFontMetrics::elidedText(const QString &text, Qt::TextElideMode mode, int width, int flags) const
{
    QString _text = text;
    if (!(flags & Qt::TextLongestVariant)) {
        int posA = 0;
        int posB = _text.indexOf(QLatin1Char('\x9c'));
        while (posB >= 0) {
            QString portion = _text.mid(posA, posB - posA);
            if (size(flags, portion).width() <= width)
                return portion;
            posA = posB + 1;
            posB = _text.indexOf(QLatin1Char('\x9c'), posA);
        }
        _text = _text.mid(posA);
    }
    QStackTextEngine engine(_text, QFont(d.data()));
    return engine.elidedText(mode, width, flags);
}

/*!
    Returns the distance from the base line to where an underscore
    should be drawn.

    \sa overlinePos(), strikeOutPos(), lineWidth()
*/
int QFontMetrics::underlinePos() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->underlinePosition());
}

/*!
    Returns the distance from the base line to where an overline
    should be drawn.

    \sa underlinePos(), strikeOutPos(), lineWidth()
*/
int QFontMetrics::overlinePos() const
{
    return ascent() + 1;
}

/*!
    Returns the distance from the base line to where the strikeout
    line should be drawn.

    \sa underlinePos(), overlinePos(), lineWidth()
*/
int QFontMetrics::strikeOutPos() const
{
    int pos = ascent() / 3;
    return pos > 0 ? pos : 1;
}

/*!
    Returns the width of the underline and strikeout lines, adjusted
    for the point size of the font.

    \sa underlinePos(), overlinePos(), strikeOutPos()
*/
int QFontMetrics::lineWidth() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return qRound(engine->lineThickness());
}




/*****************************************************************************
  QFontMetricsF member functions
 *****************************************************************************/

/*!
    \class QFontMetricsF
    \reentrant
    \inmodule QtGui

    \brief The QFontMetricsF class provides font metrics information.

    \ingroup painting
    \ingroup shared

    QFontMetricsF functions calculate the size of characters and
    strings for a given font. You can construct a QFontMetricsF object
    with an existing QFont to obtain metrics for that font. If the
    font is changed later, the font metrics object is \e not updated.

    Once created, the object provides functions to access the
    individual metrics of the font, its characters, and for strings
    rendered in the font.

    There are several functions that operate on the font: ascent(),
    descent(), height(), leading() and lineSpacing() return the basic
    size properties of the font. The underlinePos(), overlinePos(),
    strikeOutPos() and lineWidth() functions, return the properties of
    the line that underlines, overlines or strikes out the
    characters. These functions are all fast.

    There are also some functions that operate on the set of glyphs in
    the font: minLeftBearing(), minRightBearing() and maxWidth().
    These are by necessity slow, and we recommend avoiding them if
    possible.

    For each character, you can get its width(), leftBearing() and
    rightBearing() and find out whether it is in the font using
    inFont(). You can also treat the character as a string, and use
    the string functions on it.

    The string functions include width(), to return the width of a
    string in pixels (or points, for a printer), boundingRect(), to
    return a rectangle large enough to contain the rendered string,
    and size(), to return the size of that rectangle.

    Example:
    \snippet code/src_gui_text_qfontmetrics.cpp 1

    \sa QFont, QFontInfo, QFontDatabase
*/

/*!
    \since 4.2

    Constructs a font metrics object with floating point precision
    from the given \a fontMetrics object.
*/
QFontMetricsF::QFontMetricsF(const QFontMetrics &fontMetrics)
    : d(fontMetrics.d)
{
}

/*!
    \since 4.2

    Assigns \a other to this object.
*/
QFontMetricsF &QFontMetricsF::operator=(const QFontMetrics &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QFontMetricsF::swap(QFontMetricsF &other)
    \since 5.0

    Swaps this font metrics instance with \a other. This function is
    very fast and never fails.
*/



/*!
    Constructs a font metrics object for \a font.

    The font metrics will be compatible with the paintdevice used to
    create \a font.

    The font metrics object holds the information for the font that is
    passed in the constructor at the time it is created, and is not
    updated if the font's attributes are changed later.

    Use QFontMetricsF(const QFont &, QPaintDevice *) to get the font
    metrics that are compatible with a certain paint device.
*/
QFontMetricsF::QFontMetricsF(const QFont &font)
    : d(font.d)
{
}

/*!
    Constructs a font metrics object for \a font and \a paintdevice.

    The font metrics will be compatible with the paintdevice passed.
    If the \a paintdevice is 0, the metrics will be screen-compatible,
    ie. the metrics you get if you use the font for drawing text on a
    \l{QWidget}{widgets} or \l{QPixmap}{pixmaps},
    not on a QPicture or QPrinter.

    The font metrics object holds the information for the font that is
    passed in the constructor at the time it is created, and is not
    updated if the font's attributes are changed later.
*/
QFontMetricsF::QFontMetricsF(const QFont &font, QPaintDevice *paintdevice)
{
    int dpi = paintdevice ? paintdevice->logicalDpiY() : qt_defaultDpi();
    const int screen = 0;
    if (font.d->dpi != dpi || font.d->screen != screen ) {
        d = new QFontPrivate(*font.d);
        d->dpi = dpi;
        d->screen = screen;
    } else {
        d = font.d;
    }

}

/*!
    Constructs a copy of \a fm.
*/
QFontMetricsF::QFontMetricsF(const QFontMetricsF &fm)
    : d(fm.d)
{
}

/*!
    Destroys the font metrics object and frees all allocated
    resources.
*/
QFontMetricsF::~QFontMetricsF()
{
}

/*!
    Assigns the font metrics \a fm to this font metrics object.
*/
QFontMetricsF &QFontMetricsF::operator=(const QFontMetricsF &fm)
{
    d = fm.d;
    return *this;
}

/*!
  Returns \c true if the font metrics are equal to the \a other font
  metrics; otherwise returns \c false.

  Two font metrics are considered equal if they were constructed from the
  same QFont and the paint devices they were constructed for are
  considered to be compatible.
*/
bool QFontMetricsF::operator ==(const QFontMetricsF &other) const
{
    return d == other.d;
}

/*!
    \fn bool QFontMetricsF::operator !=(const QFontMetricsF &other) const
    \overload

    Returns \c true if the font metrics are not equal to the \a other font
    metrics; otherwise returns \c false.

    \sa operator==()
*/

/*!
    Returns the ascent of the font.

    The ascent of a font is the distance from the baseline to the
    highest position characters extend to. In practice, some font
    designers break this rule, e.g. when they put more than one accent
    on top of a character, or to accommodate an unusual character in
    an exotic language, so it is possible (though rare) that this
    value will be too small.

    \sa descent()
*/
qreal QFontMetricsF::ascent() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return engine->ascent().toReal();
}

/*!
    Returns the cap height of the font.

    \since 5.8

    The cap height of a font is the height of a capital letter above
    the baseline. It specifically is the height of capital letters
    that are flat - such as H or I - as opposed to round letters such
    as O, or pointed letters like A, both of which may display overshoot.

    \sa ascent()
*/
qreal QFontMetricsF::capHeight() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return engine->capHeight().toReal();
}

/*!
    Returns the descent of the font.

    The descent is the distance from the base line to the lowest point
    characters extend to. (Note that this is different from X, which
    adds 1 pixel.) In practice, some font designers break this rule,
    e.g. to accommodate an unusual character in an exotic language, so
    it is possible (though rare) that this value will be too small.

    \sa ascent()
*/
qreal QFontMetricsF::descent() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return engine->descent().toReal();
}

/*!
    Returns the height of the font.

    This is always equal to ascent()+descent().

    \sa leading(), lineSpacing()
*/
qreal QFontMetricsF::height() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);

    return (engine->ascent() + engine->descent()).toReal();
}

/*!
    Returns the leading of the font.

    This is the natural inter-line spacing.

    \sa height(), lineSpacing()
*/
qreal QFontMetricsF::leading() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return engine->leading().toReal();
}

/*!
    Returns the distance from one base line to the next.

    This value is always equal to leading()+height().

    \sa height(), leading()
*/
qreal QFontMetricsF::lineSpacing() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return (engine->leading() + engine->ascent() + engine->descent()).toReal();
}

/*!
    Returns the minimum left bearing of the font.

    This is the smallest leftBearing(char) of all characters in the
    font.

    Note that this function can be very slow if the font is large.

    \sa minRightBearing(), leftBearing()
*/
qreal QFontMetricsF::minLeftBearing() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return engine->minLeftBearing();
}

/*!
    Returns the minimum right bearing of the font.

    This is the smallest rightBearing(char) of all characters in the
    font.

    Note that this function can be very slow if the font is large.

    \sa minLeftBearing(), rightBearing()
*/
qreal QFontMetricsF::minRightBearing() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return engine->minRightBearing();
}

/*!
    Returns the width of the widest character in the font.
*/
qreal QFontMetricsF::maxWidth() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return engine->maxCharWidth();
}

/*!
    Returns the 'x' height of the font. This is often but not always
    the same as the height of the character 'x'.
*/
qreal QFontMetricsF::xHeight() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    if (d->capital == QFont::SmallCaps)
        return d->smallCapsFontPrivate()->engineForScript(QChar::Script_Common)->ascent().toReal();
    return engine->xHeight().toReal();
}

/*!
    \since 4.2

    Returns the average width of glyphs in the font.
*/
qreal QFontMetricsF::averageCharWidth() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return engine->averageCharWidth().toReal();
}

/*!
    Returns \c true if character \a ch is a valid character in the font;
    otherwise returns \c false.
*/
bool QFontMetricsF::inFont(QChar ch) const
{
    return inFontUcs4(ch.unicode());
}

/*!
    \fn bool QFontMetricsF::inFontUcs4(uint ch) const

    Returns \c true if the character given by \a ch, encoded in UCS-4/UTF-32,
    is a valid character in the font; otherwise returns \c false.
*/
bool QFontMetricsF::inFontUcs4(uint ucs4) const
{
    const int script = QChar::script(ucs4);
    QFontEngine *engine = d->engineForScript(script);
    Q_ASSERT(engine != 0);
    if (engine->type() == QFontEngine::Box)
        return false;
    return engine->canRender(ucs4);
}

/*!
    Returns the left bearing of character \a ch in the font.

    The left bearing is the right-ward distance of the left-most pixel
    of the character from the logical origin of the character. This
    value is negative if the pixels of the character extend to the
    left of the logical origin.

    See width() for a graphical description of this metric.

    \sa rightBearing(), minLeftBearing(), width()
*/
qreal QFontMetricsF::leftBearing(QChar ch) const
{
    const int script = ch.script();
    QFontEngine *engine;
    if (d->capital == QFont::SmallCaps && ch.isLower())
        engine = d->smallCapsFontPrivate()->engineForScript(script);
    else
        engine = d->engineForScript(script);
    Q_ASSERT(engine != 0);
    if (engine->type() == QFontEngine::Box)
        return 0;

    d->alterCharForCapitalization(ch);

    glyph_t glyph = engine->glyphIndex(ch.unicode());

    qreal lb;
    engine->getGlyphBearings(glyph, &lb);
    return lb;
}

/*!
    Returns the right bearing of character \a ch in the font.

    The right bearing is the left-ward distance of the right-most
    pixel of the character from the logical origin of a subsequent
    character. This value is negative if the pixels of the character
    extend to the right of the width() of the character.

    See width() for a graphical description of this metric.

    \sa leftBearing(), minRightBearing(), width()
*/
qreal QFontMetricsF::rightBearing(QChar ch) const
{
    const int script = ch.script();
    QFontEngine *engine;
    if (d->capital == QFont::SmallCaps && ch.isLower())
        engine = d->smallCapsFontPrivate()->engineForScript(script);
    else
        engine = d->engineForScript(script);
    Q_ASSERT(engine != 0);
    if (engine->type() == QFontEngine::Box)
        return 0;

    d->alterCharForCapitalization(ch);

    glyph_t glyph = engine->glyphIndex(ch.unicode());

    qreal rb;
    engine->getGlyphBearings(glyph, 0, &rb);
    return rb;

}

#if QT_DEPRECATED_SINCE(5, 11)
/*!
    Returns the width in pixels of the characters in the given \a text.

    Note that this value is \e not equal to the width returned by
    boundingRect().width() because boundingRect() returns a rectangle
    describing the pixels this string will cover whereas width()
    returns the distance to where the next string should be drawn.

    \deprecated in Qt 5.11. Use horizontalAdvance() instead.

    \sa boundingRect()
*/
qreal QFontMetricsF::width(const QString &text) const
{
    return horizontalAdvance(text);
}

/*!
    \overload

    \image bearings.png Bearings

    Returns the logical width of character \a ch in pixels. This is a
    distance appropriate for drawing a subsequent character after \a
    ch.

    Some of the metrics are described in the image to the right. The
    central dark rectangles cover the logical width() of each
    character. The outer pale rectangles cover the leftBearing() and
    rightBearing() of each character. Notice that the bearings of "f"
    in this particular font are both negative, while the bearings of
    "o" are both positive.

    \deprecated in Qt 5.11. Use horizontalAdvance() instead.

    \warning This function will produce incorrect results for Arabic
    characters or non-spacing marks in the middle of a string, as the
    glyph shaping and positioning of marks that happens when
    processing strings cannot be taken into account. When implementing
    an interactive text control, use QTextLayout instead.

    \sa boundingRect()
*/
qreal QFontMetricsF::width(QChar ch) const
{
    return horizontalAdvance(ch);
}
#endif

/*!
    Returns the horizontal advance in pixels of the first \a length characters of \a
    text. If \a length is negative (the default), the entire string is
    used.

    The advance is the distance appropriate for drawing a subsequent
    character after \a text.

    \since 5.11

    \sa boundingRect()
*/
qreal QFontMetricsF::horizontalAdvance(const QString &text, int length) const
{
    int pos = text.indexOf(QLatin1Char('\x9c'));
    if (pos != -1)
        length = (length < 0) ? pos : qMin(pos, length);
    else if (length < 0)
        length = text.length();

    if (length == 0)
        return 0;

    QStackTextEngine layout(text, QFont(d.data()));
    layout.itemize();
    return layout.width(0, length).toReal();
}

/*!
    \overload

    \image bearings.png Bearings

    Returns the horizontal advance of character \a ch in pixels. This is a
    distance appropriate for drawing a subsequent character after \a
    ch.

    Some of the metrics are described in the image to the right. The
    central dark rectangles cover the logical width() of each
    character. The outer pale rectangles cover the leftBearing() and
    rightBearing() of each character. Notice that the bearings of "f"
    in this particular font are both negative, while the bearings of
    "o" are both positive.

    \warning This function will produce incorrect results for Arabic
    characters or non-spacing marks in the middle of a string, as the
    glyph shaping and positioning of marks that happens when
    processing strings cannot be taken into account. When implementing
    an interactive text control, use QTextLayout instead.

    \since 5.11

    \sa boundingRect()
*/
qreal QFontMetricsF::horizontalAdvance(QChar ch) const
{
    if (ch.category() == QChar::Mark_NonSpacing)
        return 0.;

    const int script = ch.script();
    QFontEngine *engine;
    if (d->capital == QFont::SmallCaps && ch.isLower())
        engine = d->smallCapsFontPrivate()->engineForScript(script);
    else
        engine = d->engineForScript(script);
    Q_ASSERT(engine != 0);

    d->alterCharForCapitalization(ch);

    glyph_t glyph = engine->glyphIndex(ch.unicode());
    QFixed advance;

    QGlyphLayout glyphs;
    glyphs.numGlyphs = 1;
    glyphs.glyphs = &glyph;
    glyphs.advances = &advance;
    engine->recalcAdvances(&glyphs, 0);

    return advance.toReal();
}


/*!
    Returns the bounding rectangle of the characters in the string
    specified by \a text. The bounding rectangle always covers at least
    the set of pixels the text would cover if drawn at (0, 0).

    Note that the bounding rectangle may extend to the left of (0, 0),
    e.g. for italicized fonts, and that the width of the returned
    rectangle might be different than what the width() method returns.

    If you want to know the advance width of the string (to lay out
    a set of strings next to each other), use horizontalAdvance() instead.

    Newline characters are processed as normal characters, \e not as
    linebreaks.

    The height of the bounding rectangle is at least as large as the
    value returned height().

    \sa width(), height(), QPainter::boundingRect()
*/
QRectF QFontMetricsF::boundingRect(const QString &text) const
{
    int len = text.length();
    if (len == 0)
        return QRectF();

    QStackTextEngine layout(text, QFont(d.data()));
    layout.itemize();
    glyph_metrics_t gm = layout.boundingBox(0, len);
    return QRectF(gm.x.toReal(), gm.y.toReal(),
                  gm.width.toReal(), gm.height.toReal());
}

/*!
    Returns the bounding rectangle of the character \a ch relative to
    the left-most point on the base line.

    Note that the bounding rectangle may extend to the left of (0, 0),
    e.g. for italicized fonts, and that the text output may cover \e
    all pixels in the bounding rectangle.

    Note that the rectangle usually extends both above and below the
    base line.

    \sa width()
*/
QRectF QFontMetricsF::boundingRect(QChar ch) const
{
    const int script = ch.script();
    QFontEngine *engine;
    if (d->capital == QFont::SmallCaps && ch.isLower())
        engine = d->smallCapsFontPrivate()->engineForScript(script);
    else
        engine = d->engineForScript(script);
    Q_ASSERT(engine != 0);

    d->alterCharForCapitalization(ch);

    glyph_t glyph = engine->glyphIndex(ch.unicode());

    glyph_metrics_t gm = engine->boundingBox(glyph);
    return QRectF(gm.x.toReal(), gm.y.toReal(), gm.width.toReal(), gm.height.toReal());
}

/*!
    \overload

    Returns the bounding rectangle of the characters in the given \a text.
    This is the set of pixels the text would cover if drawn when constrained
    to the bounding rectangle specified by \a rect.

    The \a flags argument is the bitwise OR of the following flags:
    \list
    \li Qt::AlignLeft aligns to the left border, except for
          Arabic and Hebrew where it aligns to the right.
    \li Qt::AlignRight aligns to the right border, except for
          Arabic and Hebrew where it aligns to the left.
    \li Qt::AlignJustify produces justified text.
    \li Qt::AlignHCenter aligns horizontally centered.
    \li Qt::AlignTop aligns to the top border.
    \li Qt::AlignBottom aligns to the bottom border.
    \li Qt::AlignVCenter aligns vertically centered
    \li Qt::AlignCenter (== \c{Qt::AlignHCenter | Qt::AlignVCenter})
    \li Qt::TextSingleLine ignores newline characters in the text.
    \li Qt::TextExpandTabs expands tabs (see below)
    \li Qt::TextShowMnemonic interprets "&x" as \underline{x}; i.e., underlined.
    \li Qt::TextWordWrap breaks the text to fit the rectangle.
    \endlist

    Qt::Horizontal alignment defaults to Qt::AlignLeft and vertical
    alignment defaults to Qt::AlignTop.

    If several of the horizontal or several of the vertical alignment
    flags are set, the resulting alignment is undefined.

    These flags are defined in \l{Qt::AlignmentFlag}.

    If Qt::TextExpandTabs is set in \a flags, the following behavior is
    used to interpret tab characters in the text:
    \list
    \li If \a tabArray is non-null, it specifies a 0-terminated sequence of
       pixel-positions for tabs in the text.
    \li If \a tabStops is non-zero, it is used as the tab spacing (in pixels).
    \endlist

    Note that the bounding rectangle may extend to the left of (0, 0),
    e.g. for italicized fonts.

    Newline characters are processed as line breaks.

    Despite the different actual character heights, the heights of the
    bounding rectangles of "Yes" and "yes" are the same.

    The bounding rectangle returned by this function is somewhat larger
    than that calculated by the simpler boundingRect() function. This
    function uses the \l{minLeftBearing()}{maximum left} and
    \l{minRightBearing()}{right} font bearings as is
    necessary for multi-line text to align correctly. Also,
    fontHeight() and lineSpacing() are used to calculate the height,
    rather than individual character heights.

    \sa width(), QPainter::boundingRect(), Qt::Alignment
*/
QRectF QFontMetricsF::boundingRect(const QRectF &rect, int flags, const QString& text,
                                   int tabStops, int *tabArray) const
{
    int tabArrayLen = 0;
    if (tabArray)
        while (tabArray[tabArrayLen])
            tabArrayLen++;

    QRectF rb;
    qt_format_text(QFont(d.data()), rect, flags | Qt::TextDontPrint, text, &rb, tabStops, tabArray,
                   tabArrayLen, 0);
    return rb;
}

/*!
    Returns the size in pixels of the characters in the given \a text.

    The \a flags argument is the bitwise OR of the following flags:
    \list
    \li Qt::TextSingleLine ignores newline characters.
    \li Qt::TextExpandTabs expands tabs (see below)
    \li Qt::TextShowMnemonic interprets "&x" as \underline{x}; i.e., underlined.
    \li Qt::TextWordWrap breaks the text to fit the rectangle.
    \endlist

    These flags are defined in the \l{Qt::TextFlag} enum.

    If Qt::TextExpandTabs is set in \a flags, the following behavior is
    used to interpret tab characters in the text:
    \list
    \li If \a tabArray is non-null, it specifies a 0-terminated sequence of
       pixel-positions for tabs in the text.
    \li If \a tabStops is non-zero, it is used as the tab spacing (in pixels).
    \endlist

    Newline characters are processed as line breaks.

    Note: Despite the different actual character heights, the heights of the
    bounding rectangles of "Yes" and "yes" are the same.

    \sa boundingRect()
*/
QSizeF QFontMetricsF::size(int flags, const QString &text, int tabStops, int *tabArray) const
{
    return boundingRect(QRectF(), flags | Qt::TextLongestVariant, text, tabStops, tabArray).size();
}

/*!
  \since 4.3

    Returns a tight bounding rectangle around the characters in the
    string specified by \a text. The bounding rectangle always covers
    at least the set of pixels the text would cover if drawn at (0,
    0).

    Note that the bounding rectangle may extend to the left of (0, 0),
    e.g. for italicized fonts, and that the width of the returned
    rectangle might be different than what the width() method returns.

    If you want to know the advance width of the string (to lay out
    a set of strings next to each other), use horizontalAdvance() instead.

    Newline characters are processed as normal characters, \e not as
    linebreaks.

    \warning Calling this method is very slow on Windows.

    \sa width(), height(), boundingRect()
*/
QRectF QFontMetricsF::tightBoundingRect(const QString &text) const
{
    if (text.length() == 0)
        return QRect();

    QStackTextEngine layout(text, QFont(d.data()));
    layout.itemize();
    glyph_metrics_t gm = layout.tightBoundingBox(0, text.length());
    return QRectF(gm.x.toReal(), gm.y.toReal(), gm.width.toReal(), gm.height.toReal());
}

/*!
    \since 4.2

    If the string \a text is wider than \a width, returns an elided
    version of the string (i.e., a string with "..." in it).
    Otherwise, returns the original string.

    The \a mode parameter specifies whether the text is elided on the
    left (for example, "...tech"), in the middle (for example, "Tr...ch"), or
    on the right (for example, "Trol...").

    The \a width is specified in pixels, not characters.

    The \a flags argument is optional and currently only supports
    Qt::TextShowMnemonic as value.

    The elide mark follows the \l{Qt::LayoutDirection}{layoutdirection}.
    For example, it will be on the right side of the text for right-to-left
    layouts if the \a mode is \c{Qt::ElideLeft}, and on the left side of the
    text if the \a mode is \c{Qt::ElideRight}.
*/
QString QFontMetricsF::elidedText(const QString &text, Qt::TextElideMode mode, qreal width, int flags) const
{
    QString _text = text;
    if (!(flags & Qt::TextLongestVariant)) {
        int posA = 0;
        int posB = _text.indexOf(QLatin1Char('\x9c'));
        while (posB >= 0) {
            QString portion = _text.mid(posA, posB - posA);
            if (size(flags, portion).width() <= width)
                return portion;
            posA = posB + 1;
            posB = _text.indexOf(QLatin1Char('\x9c'), posA);
        }
        _text = _text.mid(posA);
    }
    QStackTextEngine engine(_text, QFont(d.data()));
    return engine.elidedText(mode, QFixed::fromReal(width), flags);
}

/*!
    Returns the distance from the base line to where an underscore
    should be drawn.

    \sa overlinePos(), strikeOutPos(), lineWidth()
*/
qreal QFontMetricsF::underlinePos() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return engine->underlinePosition().toReal();
}

/*!
    Returns the distance from the base line to where an overline
    should be drawn.

    \sa underlinePos(), strikeOutPos(), lineWidth()
*/
qreal QFontMetricsF::overlinePos() const
{
    return ascent() + 1;
}

/*!
    Returns the distance from the base line to where the strikeout
    line should be drawn.

    \sa underlinePos(), overlinePos(), lineWidth()
*/
qreal QFontMetricsF::strikeOutPos() const
{
    return ascent() / 3.;
}

/*!
    Returns the width of the underline and strikeout lines, adjusted
    for the point size of the font.

    \sa underlinePos(), overlinePos(), strikeOutPos()
*/
qreal QFontMetricsF::lineWidth() const
{
    QFontEngine *engine = d->engineForScript(QChar::Script_Common);
    Q_ASSERT(engine != 0);
    return engine->lineThickness().toReal();
}

QT_END_NAMESPACE
