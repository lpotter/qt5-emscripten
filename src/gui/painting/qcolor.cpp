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

#include "qcolor.h"
#include "qcolor_p.h"
#include "qnamespace.h"
#include "qdatastream.h"
#include "qvariant.h"
#include "qdebug.h"
#include "private/qtools_p.h"

#include <algorithm>

#include <stdio.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

/*!
    \internal
    If s[0..1] is a valid hex number, returns its integer value,
    otherwise returns -1.
 */
static inline int hex2int(const char *s)
{
    const int hi = QtMiscUtils::fromHex(s[0]);
    if (hi < 0)
        return -1;
    const int lo = QtMiscUtils::fromHex(s[1]);
    if (lo < 0)
        return -1;
    return (hi << 4) | lo;
}

/*!
    \internal
    If s is a valid hex digit, returns its integer value,
    multiplied by 0x11, otherwise returns -1.
 */
static inline int hex2int(char s)
{
    const int h = QtMiscUtils::fromHex(s);
    return h < 0 ? h : (h << 4) | h;
}

static bool get_hex_rgb(const char *name, size_t len, QRgb *rgb)
{
    if (name[0] != '#')
        return false;
    name++;
    --len;
    int a, r, g, b;
    a = 255;
    if (len == 12) {
        r = hex2int(name);
        g = hex2int(name + 4);
        b = hex2int(name + 8);
    } else if (len == 9) {
        r = hex2int(name);
        g = hex2int(name + 3);
        b = hex2int(name + 6);
    } else if (len == 8) {
        a = hex2int(name);
        r = hex2int(name + 2);
        g = hex2int(name + 4);
        b = hex2int(name + 6);
    } else if (len == 6) {
        r = hex2int(name);
        g = hex2int(name + 2);
        b = hex2int(name + 4);
    } else if (len == 3) {
        r = hex2int(name[0]);
        g = hex2int(name[1]);
        b = hex2int(name[2]);
    } else {
        r = g = b = -1;
    }
    if ((uint)r > 255 || (uint)g > 255 || (uint)b > 255 || (uint)a > 255) {
        *rgb = 0;
        return false;
    }
    *rgb = qRgba(r, g ,b, a);
    return true;
}

bool qt_get_hex_rgb(const char *name, QRgb *rgb)
{
    return get_hex_rgb(name, qstrlen(name), rgb);
}

static bool get_hex_rgb(const QChar *str, size_t len, QRgb *rgb)
{
    if (len > 13)
        return false;
    char tmp[16];
    for (size_t i = 0; i < len; ++i)
        tmp[i] = str[i].toLatin1();
    tmp[len] = 0;
    return get_hex_rgb(tmp, len, rgb);
}

#ifndef QT_NO_COLORNAMES

/*
  CSS color names = SVG 1.0 color names + transparent (rgba(0,0,0,0))
*/

#ifdef rgb
#  undef rgb
#endif
#define rgb(r,g,b) (0xff000000 | (r << 16) |  (g << 8) | b)

static const struct RGBData {
    const char name[21];
    uint  value;
} rgbTbl[] = {
    { "aliceblue", rgb(240, 248, 255) },
    { "antiquewhite", rgb(250, 235, 215) },
    { "aqua", rgb( 0, 255, 255) },
    { "aquamarine", rgb(127, 255, 212) },
    { "azure", rgb(240, 255, 255) },
    { "beige", rgb(245, 245, 220) },
    { "bisque", rgb(255, 228, 196) },
    { "black", rgb( 0, 0, 0) },
    { "blanchedalmond", rgb(255, 235, 205) },
    { "blue", rgb( 0, 0, 255) },
    { "blueviolet", rgb(138, 43, 226) },
    { "brown", rgb(165, 42, 42) },
    { "burlywood", rgb(222, 184, 135) },
    { "cadetblue", rgb( 95, 158, 160) },
    { "chartreuse", rgb(127, 255, 0) },
    { "chocolate", rgb(210, 105, 30) },
    { "coral", rgb(255, 127, 80) },
    { "cornflowerblue", rgb(100, 149, 237) },
    { "cornsilk", rgb(255, 248, 220) },
    { "crimson", rgb(220, 20, 60) },
    { "cyan", rgb( 0, 255, 255) },
    { "darkblue", rgb( 0, 0, 139) },
    { "darkcyan", rgb( 0, 139, 139) },
    { "darkgoldenrod", rgb(184, 134, 11) },
    { "darkgray", rgb(169, 169, 169) },
    { "darkgreen", rgb( 0, 100, 0) },
    { "darkgrey", rgb(169, 169, 169) },
    { "darkkhaki", rgb(189, 183, 107) },
    { "darkmagenta", rgb(139, 0, 139) },
    { "darkolivegreen", rgb( 85, 107, 47) },
    { "darkorange", rgb(255, 140, 0) },
    { "darkorchid", rgb(153, 50, 204) },
    { "darkred", rgb(139, 0, 0) },
    { "darksalmon", rgb(233, 150, 122) },
    { "darkseagreen", rgb(143, 188, 143) },
    { "darkslateblue", rgb( 72, 61, 139) },
    { "darkslategray", rgb( 47, 79, 79) },
    { "darkslategrey", rgb( 47, 79, 79) },
    { "darkturquoise", rgb( 0, 206, 209) },
    { "darkviolet", rgb(148, 0, 211) },
    { "deeppink", rgb(255, 20, 147) },
    { "deepskyblue", rgb( 0, 191, 255) },
    { "dimgray", rgb(105, 105, 105) },
    { "dimgrey", rgb(105, 105, 105) },
    { "dodgerblue", rgb( 30, 144, 255) },
    { "firebrick", rgb(178, 34, 34) },
    { "floralwhite", rgb(255, 250, 240) },
    { "forestgreen", rgb( 34, 139, 34) },
    { "fuchsia", rgb(255, 0, 255) },
    { "gainsboro", rgb(220, 220, 220) },
    { "ghostwhite", rgb(248, 248, 255) },
    { "gold", rgb(255, 215, 0) },
    { "goldenrod", rgb(218, 165, 32) },
    { "gray", rgb(128, 128, 128) },
    { "green", rgb( 0, 128, 0) },
    { "greenyellow", rgb(173, 255, 47) },
    { "grey", rgb(128, 128, 128) },
    { "honeydew", rgb(240, 255, 240) },
    { "hotpink", rgb(255, 105, 180) },
    { "indianred", rgb(205, 92, 92) },
    { "indigo", rgb( 75, 0, 130) },
    { "ivory", rgb(255, 255, 240) },
    { "khaki", rgb(240, 230, 140) },
    { "lavender", rgb(230, 230, 250) },
    { "lavenderblush", rgb(255, 240, 245) },
    { "lawngreen", rgb(124, 252, 0) },
    { "lemonchiffon", rgb(255, 250, 205) },
    { "lightblue", rgb(173, 216, 230) },
    { "lightcoral", rgb(240, 128, 128) },
    { "lightcyan", rgb(224, 255, 255) },
    { "lightgoldenrodyellow", rgb(250, 250, 210) },
    { "lightgray", rgb(211, 211, 211) },
    { "lightgreen", rgb(144, 238, 144) },
    { "lightgrey", rgb(211, 211, 211) },
    { "lightpink", rgb(255, 182, 193) },
    { "lightsalmon", rgb(255, 160, 122) },
    { "lightseagreen", rgb( 32, 178, 170) },
    { "lightskyblue", rgb(135, 206, 250) },
    { "lightslategray", rgb(119, 136, 153) },
    { "lightslategrey", rgb(119, 136, 153) },
    { "lightsteelblue", rgb(176, 196, 222) },
    { "lightyellow", rgb(255, 255, 224) },
    { "lime", rgb( 0, 255, 0) },
    { "limegreen", rgb( 50, 205, 50) },
    { "linen", rgb(250, 240, 230) },
    { "magenta", rgb(255, 0, 255) },
    { "maroon", rgb(128, 0, 0) },
    { "mediumaquamarine", rgb(102, 205, 170) },
    { "mediumblue", rgb( 0, 0, 205) },
    { "mediumorchid", rgb(186, 85, 211) },
    { "mediumpurple", rgb(147, 112, 219) },
    { "mediumseagreen", rgb( 60, 179, 113) },
    { "mediumslateblue", rgb(123, 104, 238) },
    { "mediumspringgreen", rgb( 0, 250, 154) },
    { "mediumturquoise", rgb( 72, 209, 204) },
    { "mediumvioletred", rgb(199, 21, 133) },
    { "midnightblue", rgb( 25, 25, 112) },
    { "mintcream", rgb(245, 255, 250) },
    { "mistyrose", rgb(255, 228, 225) },
    { "moccasin", rgb(255, 228, 181) },
    { "navajowhite", rgb(255, 222, 173) },
    { "navy", rgb( 0, 0, 128) },
    { "oldlace", rgb(253, 245, 230) },
    { "olive", rgb(128, 128, 0) },
    { "olivedrab", rgb(107, 142, 35) },
    { "orange", rgb(255, 165, 0) },
    { "orangered", rgb(255, 69, 0) },
    { "orchid", rgb(218, 112, 214) },
    { "palegoldenrod", rgb(238, 232, 170) },
    { "palegreen", rgb(152, 251, 152) },
    { "paleturquoise", rgb(175, 238, 238) },
    { "palevioletred", rgb(219, 112, 147) },
    { "papayawhip", rgb(255, 239, 213) },
    { "peachpuff", rgb(255, 218, 185) },
    { "peru", rgb(205, 133, 63) },
    { "pink", rgb(255, 192, 203) },
    { "plum", rgb(221, 160, 221) },
    { "powderblue", rgb(176, 224, 230) },
    { "purple", rgb(128, 0, 128) },
    { "red", rgb(255, 0, 0) },
    { "rosybrown", rgb(188, 143, 143) },
    { "royalblue", rgb( 65, 105, 225) },
    { "saddlebrown", rgb(139, 69, 19) },
    { "salmon", rgb(250, 128, 114) },
    { "sandybrown", rgb(244, 164, 96) },
    { "seagreen", rgb( 46, 139, 87) },
    { "seashell", rgb(255, 245, 238) },
    { "sienna", rgb(160, 82, 45) },
    { "silver", rgb(192, 192, 192) },
    { "skyblue", rgb(135, 206, 235) },
    { "slateblue", rgb(106, 90, 205) },
    { "slategray", rgb(112, 128, 144) },
    { "slategrey", rgb(112, 128, 144) },
    { "snow", rgb(255, 250, 250) },
    { "springgreen", rgb( 0, 255, 127) },
    { "steelblue", rgb( 70, 130, 180) },
    { "tan", rgb(210, 180, 140) },
    { "teal", rgb( 0, 128, 128) },
    { "thistle", rgb(216, 191, 216) },
    { "tomato", rgb(255, 99, 71) },
    { "transparent", 0 },
    { "turquoise", rgb( 64, 224, 208) },
    { "violet", rgb(238, 130, 238) },
    { "wheat", rgb(245, 222, 179) },
    { "white", rgb(255, 255, 255) },
    { "whitesmoke", rgb(245, 245, 245) },
    { "yellow", rgb(255, 255, 0) },
    { "yellowgreen", rgb(154, 205, 50) }
};

static const int rgbTblSize = sizeof(rgbTbl) / sizeof(RGBData);

#undef rgb

inline bool operator<(const char *name, const RGBData &data)
{ return qstrcmp(name, data.name) < 0; }
inline bool operator<(const RGBData &data, const char *name)
{ return qstrcmp(data.name, name) < 0; }

static bool get_named_rgb_no_space(const char *name_no_space, QRgb *rgb)
{
    const RGBData *r = std::lower_bound(rgbTbl, rgbTbl + rgbTblSize, name_no_space);
    if ((r != rgbTbl + rgbTblSize) && !(name_no_space < *r)) {
        *rgb = r->value;
        return true;
    }
    return false;
}

static bool get_named_rgb(const char *name, int len, QRgb* rgb)
{
    if (len > 255)
        return false;
    char name_no_space[256];
    int pos = 0;
    for (int i = 0; i < len; i++) {
        if (name[i] != '\t' && name[i] != ' ')
            name_no_space[pos++] = QChar::toLower(name[i]);
    }
    name_no_space[pos] = 0;

    return get_named_rgb_no_space(name_no_space, rgb);
}

static bool get_named_rgb(const QChar *name, int len, QRgb *rgb)
{
    if (len > 255)
        return false;
    char name_no_space[256];
    int pos = 0;
    for (int i = 0; i < len; i++) {
        if (name[i] != QLatin1Char('\t') && name[i] != QLatin1Char(' '))
            name_no_space[pos++] = name[i].toLower().toLatin1();
    }
    name_no_space[pos] = 0;
    return get_named_rgb_no_space(name_no_space, rgb);
}

#endif // QT_NO_COLORNAMES

static QStringList get_colornames()
{
    QStringList lst;
#ifndef QT_NO_COLORNAMES
    lst.reserve(rgbTblSize);
    for (int i = 0; i < rgbTblSize; i++)
        lst << QLatin1String(rgbTbl[i].name);
#endif
    return lst;
}

/*!
    \class QColor
    \brief The QColor class provides colors based on RGB, HSV or CMYK values.

    \ingroup painting
    \ingroup appearance
    \inmodule QtGui


    A color is normally specified in terms of RGB (red, green, and
    blue) components, but it is also possible to specify it in terms
    of HSV (hue, saturation, and value) and CMYK (cyan, magenta,
    yellow and black) components. In addition a color can be specified
    using a color name. The color name can be any of the SVG 1.0 color
    names.

    \table
    \header
    \li RGB \li HSV \li CMYK
    \row
    \li \inlineimage qcolor-rgb.png
    \li \inlineimage qcolor-hsv.png
    \li \inlineimage qcolor-cmyk.png
    \endtable

    The QColor constructor creates the color based on RGB values.  To
    create a QColor based on either HSV or CMYK values, use the
    toHsv() and toCmyk() functions respectively. These functions
    return a copy of the color using the desired format. In addition
    the static fromRgb(), fromHsv() and fromCmyk() functions create
    colors from the specified values. Alternatively, a color can be
    converted to any of the three formats using the convertTo()
    function (returning a copy of the color in the desired format), or
    any of the setRgb(), setHsv() and setCmyk() functions altering \e
    this color's format. The spec() function tells how the color was
    specified.

    A color can be set by passing an RGB string (such as "#112233"),
    or an ARGB string (such as "#ff112233") or a color name (such as "blue"),
    to the setNamedColor() function.
    The color names are taken from the SVG 1.0 color names. The name()
    function returns the name of the color in the format
    "#RRGGBB". Colors can also be set using setRgb(), setHsv() and
    setCmyk(). To get a lighter or darker color use the lighter() and
    darker() functions respectively.

    The isValid() function indicates whether a QColor is legal at
    all. For example, a RGB color with RGB values out of range is
    illegal. For performance reasons, QColor mostly disregards illegal
    colors, and for that reason, the result of using an invalid color
    is undefined.

    The color components can be retrieved individually, e.g with
    red(), hue() and cyan(). The values of the color components can
    also be retrieved in one go using the getRgb(), getHsv() and
    getCmyk() functions. Using the RGB color model, the color
    components can in addition be accessed with rgb().

    There are several related non-members: QRgb is a typdef for an
    unsigned int representing the RGB value triplet (r, g, b). Note
    that it also can hold a value for the alpha-channel (for more
    information, see the \l {QColor#Alpha-Blended
    Drawing}{Alpha-Blended Drawing} section). The qRed(), qBlue() and
    qGreen() functions return the respective component of the given
    QRgb value, while the qRgb() and qRgba() functions create and
    return the QRgb triplet based on the given component
    values. Finally, the qAlpha() function returns the alpha component
    of the provided QRgb, and the qGray() function calculates and
    return a gray value based on the given value.

    QColor is platform and device independent. The QColormap class
    maps the color to the hardware.

    For more information about painting in general, see the \l{Paint
    System} documentation.

    \tableofcontents

    \section1 Integer vs. Floating Point Precision

    QColor supports floating point precision and provides floating
    point versions of all the color components functions,
    e.g. getRgbF(), hueF() and fromCmykF(). Note that since the
    components are stored using 16-bit integers, there might be minor
    deviations between the values set using, for example, setRgbF()
    and the values returned by the getRgbF() function due to rounding.

    While the integer based functions take values in the range 0-255
    (except hue() which must have values within the range 0-359),
    the floating point functions accept values in the range 0.0 - 1.0.

    \section1 Alpha-Blended Drawing

    QColor also support alpha-blended outlining and filling. The
    alpha channel of a color specifies the transparency effect, 0
    represents a fully transparent color, while 255 represents a fully
    opaque color. For example:

    \snippet code/src_gui_painting_qcolor.cpp 0

    The code above produces the following output:

    \image alphafill.png

    The alpha channel of a color can be retrieved and set using the
    alpha() and setAlpha() functions if its value is an integer, and
    alphaF() and setAlphaF() if its value is qreal (double). By
    default, the alpha-channel is set to 255 (opaque). To retrieve and
    set \e all the RGB color components (including the alpha-channel)
    in one go, use the rgba() and setRgba() functions.

    \section1 Predefined Colors

    There are 20 predefined QColors described by the Qt::GlobalColor enum,
    including black, white, primary and secondary colors, darker versions
    of these colors and three shades of gray. QColor also recognizes a
    variety of color names; the static colorNames() function returns a
    QStringList color names that QColor knows about.

    \image qt-colors.png Qt Colors

    Additionally, the Qt::color0, Qt::color1 and Qt::transparent colors
    are used for special purposes.

    Qt::color0 (zero pixel value) and Qt::color1 (non-zero pixel value)
    are special colors for drawing in QBitmaps. Painting with Qt::color0
    sets the bitmap bits to 0 (transparent; i.e., background), and painting
    with Qt::color1 sets the bits to 1 (opaque; i.e., foreground).

    Qt::transparent is used to indicate a transparent pixel. When painting
    with this value, a pixel value will be used that is appropriate for the
    underlying pixel format in use.

    \section1 The HSV Color Model

    The RGB model is hardware-oriented. Its representation is close to
    what most monitors show. In contrast, HSV represents color in a way
    more suited to the human perception of color. For example, the
    relationships "stronger than", "darker than", and "the opposite of"
    are easily expressed in HSV but are much harder to express in RGB.

    HSV, like RGB, has three components:

    \list
    \li H, for hue, is in the range 0 to 359 if the color is chromatic (not
    gray), or meaningless if it is gray. It represents degrees on the
    color wheel familiar to most people. Red is 0 (degrees), green is
    120, and blue is 240.

    \inlineimage qcolor-hue.png

    \li S, for saturation, is in the range 0 to 255, and the bigger it is,
    the stronger the color is. Grayish colors have saturation near 0; very
    strong colors have saturation near 255.

    \inlineimage qcolor-saturation.png

    \li V, for value, is in the range 0 to 255 and represents lightness or
    brightness of the color. 0 is black; 255 is as far from black as
    possible.

    \inlineimage qcolor-value.png
    \endlist

    Here are some examples: pure red is H=0, S=255, V=255; a dark red,
    moving slightly towards the magenta, could be H=350 (equivalent to
    -10), S=255, V=180; a grayish light red could have H about 0 (say
    350-359 or 0-10), S about 50-100, and S=255.

    Qt returns a hue value of -1 for achromatic colors. If you pass a
    hue value that is too large, Qt forces it into range. Hue 360 or 720 is
    treated as 0; hue 540 is treated as 180.

    In addition to the standard HSV model, Qt provides an
    alpha-channel to feature \l {QColor#Alpha-Blended
    Drawing}{alpha-blended drawing}.

    \section1 The HSL Color Model

    HSL is similar to HSV, however instead of the Value parameter, HSL
    specifies a Lightness parameter.

    \section1 The CMYK Color Model

    While the RGB and HSV color models are used for display on
    computer monitors, the CMYK model is used in the four-color
    printing process of printing presses and some hard-copy
    devices.

    CMYK has four components, all in the range 0-255: cyan (C),
    magenta (M), yellow (Y) and black (K).  Cyan, magenta and yellow
    are called subtractive colors; the CMYK color model creates color
    by starting with a white surface and then subtracting color by
    applying the appropriate components. While combining cyan, magenta
    and yellow gives the color black, subtracting one or more will
    yield any other color. When combined in various percentages, these
    three colors can create the entire spectrum of colors.

    Mixing 100 percent of cyan, magenta and yellow \e does produce
    black, but the result is unsatisfactory since it wastes ink,
    increases drying time, and gives a muddy colour when printing. For
    that reason, black is added in professional printing to provide a
    solid black tone; hence the term 'four color process'.

    In addition to the standard CMYK model, Qt provides an
    alpha-channel to feature \l {QColor#Alpha-Blended
    Drawing}{alpha-blended drawing}.

    \sa QPalette, QBrush
*/

#define QCOLOR_INT_RANGE_CHECK(fn, var) \
    do { \
        if (var < 0 || var > 255) { \
            qWarning(#fn": invalid value %d", var); \
            var = qMax(0, qMin(var, 255)); \
        } \
    } while (0)

#define QCOLOR_REAL_RANGE_CHECK(fn, var) \
    do { \
        if (var < qreal(0.0) || var > qreal(1.0)) { \
            qWarning(#fn": invalid value %g", var); \
            var = qMax(qreal(0.0), qMin(var, qreal(1.0)));      \
        } \
    } while (0)

/*****************************************************************************
  QColor member functions
 *****************************************************************************/

/*!
    \enum QColor::Spec

    The type of color specified, either RGB, HSV, CMYK or HSL.

    \value Rgb
    \value Hsv
    \value Cmyk
    \value Hsl
    \value Invalid

    \sa spec(), convertTo()
*/

/*!
    \enum QColor::NameFormat

    How to format the output of the name() function

    \value HexRgb #RRGGBB A "#" character followed by three two-digit hexadecimal numbers (i.e. \c{#RRGGBB}).
    \value HexArgb #AARRGGBB A "#" character followed by four two-digit hexadecimal numbers (i.e. \c{#AARRGGBB}).

    \sa name()
*/

/*!
    \fn Spec QColor::spec() const

    Returns how the color was specified.

    \sa Spec, convertTo()
*/


/*!
    \fn QColor::QColor()

    Constructs an invalid color with the RGB value (0, 0, 0). An
    invalid color is a color that is not properly set up for the
    underlying window system.

    The alpha value of an invalid color is unspecified.

    \sa isValid()
*/

/*!
    \overload

    Constructs a new color with a color value of \a color.

    \sa isValid(), {QColor#Predefined Colors}{Predefined Colors}
 */
QColor::QColor(Qt::GlobalColor color) Q_DECL_NOTHROW
{
#define QRGB(r, g, b) \
    QRgb(((0xffu << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff)))
#define QRGBA(r, g, b, a) \
    QRgb(((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff))

    static const QRgb global_colors[] = {
        QRGB(255, 255, 255), // Qt::color0
        QRGB(  0,   0,   0), // Qt::color1
        QRGB(  0,   0,   0), // black
        QRGB(255, 255, 255), // white
        /*
         * From the "The Palette Manager: How and Why" by Ron Gery,
         * March 23, 1992, archived on MSDN:
         *
         *     The Windows system palette is broken up into two
         *     sections, one with fixed colors and one with colors
         *     that can be changed by applications. The system palette
         *     predefines 20 entries; these colors are known as the
         *     static or reserved colors and consist of the 16 colors
         *     found in the Windows version 3.0 VGA driver and 4
         *     additional colors chosen for their visual appeal.  The
         *     DEFAULT_PALETTE stock object is, as the name implies,
         *     the default palette selected into a device context (DC)
         *     and consists of these static colors. Applications can
         *     set the remaining 236 colors using the Palette Manager.
         *
         * The 20 reserved entries have indices in [0,9] and
         * [246,255]. We reuse 17 of them.
         */
        QRGB(128, 128, 128), // index 248   medium gray
        QRGB(160, 160, 164), // index 247   light gray
        QRGB(192, 192, 192), // index 7     light gray
        QRGB(255,   0,   0), // index 249   red
        QRGB(  0, 255,   0), // index 250   green
        QRGB(  0,   0, 255), // index 252   blue
        QRGB(  0, 255, 255), // index 254   cyan
        QRGB(255,   0, 255), // index 253   magenta
        QRGB(255, 255,   0), // index 251   yellow
        QRGB(128,   0,   0), // index 1     dark red
        QRGB(  0, 128,   0), // index 2     dark green
        QRGB(  0,   0, 128), // index 4     dark blue
        QRGB(  0, 128, 128), // index 6     dark cyan
        QRGB(128,   0, 128), // index 5     dark magenta
        QRGB(128, 128,   0), // index 3     dark yellow
        QRGBA(0, 0, 0, 0)    //             transparent
    };
#undef QRGB
#undef QRGBA

    setRgb(qRed(global_colors[color]),
           qGreen(global_colors[color]),
           qBlue(global_colors[color]),
           qAlpha(global_colors[color]));
}

/*!
    \fn QColor::QColor(int r, int g, int b, int a = 255)

    Constructs a color with the RGB value \a r, \a g, \a b, and the
    alpha-channel (transparency) value of \a a.

    The color is left invalid if any of the arguments are invalid.

    \sa setRgba(), isValid()
*/

/*!
    Constructs a color with the value \a color. The alpha component is
    ignored and set to solid.

    \sa fromRgb(), isValid()
*/

QColor::QColor(QRgb color) Q_DECL_NOTHROW
{
    cspec = Rgb;
    ct.argb.alpha = 0xffff;
    ct.argb.red   = qRed(color)   * 0x101;
    ct.argb.green = qGreen(color) * 0x101;
    ct.argb.blue  = qBlue(color)  * 0x101;
    ct.argb.pad   = 0;
}

/*!
    \since 5.6

    Constructs a color with the value \a rgba64.

    \sa fromRgba64()
*/

QColor::QColor(QRgba64 rgba64) Q_DECL_NOTHROW
{
    setRgba64(rgba64);
}

/*!
    \internal

    Constructs a color with the given \a spec.

    This function is primarly present to avoid that QColor::Invalid
    becomes a valid color by accident.
*/

QColor::QColor(Spec spec) Q_DECL_NOTHROW
{
    switch (spec) {
    case Invalid:
        invalidate();
        break;
    case Rgb:
        setRgb(0, 0, 0);
        break;
    case Hsv:
        setHsv(0, 0, 0);
        break;
    case Cmyk:
        setCmyk(0, 0, 0, 0);
        break;
    case Hsl:
        setHsl(0, 0, 0, 0);
        break;
    }
}

/*!
    \fn QColor::QColor(const QString &name)

    Constructs a named color in the same way as setNamedColor() using
    the given \a name.

    The color is left invalid if the \a name cannot be parsed.

    \sa setNamedColor(), name(), isValid()
*/

/*!
    \fn QColor::QColor(const char *name)

    Constructs a named color in the same way as setNamedColor() using
    the given \a name.

    \overload
    \sa setNamedColor(), name(), isValid()
*/

/*!
    \fn QColor::QColor(QLatin1String name)

    Constructs a named color in the same way as setNamedColor() using
    the given \a name.

    \overload
    \since 5.8
    \sa setNamedColor(), name(), isValid()
*/

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
/*!
    \fn QColor::QColor(const QColor &color)

    Constructs a color that is a copy of \a color.

    \sa isValid()
*/
#endif

/*!
    \fn bool QColor::isValid() const

    Returns \c true if the color is valid; otherwise returns \c false.
*/

/*!
    Returns the name of the color in the format "#RRGGBB"; i.e. a "#"
    character followed by three two-digit hexadecimal numbers.

    \sa setNamedColor()
*/

QString QColor::name() const
{
    return name(HexRgb);
}

/*!
    \since 5.2

    Returns the name of the color in the specified \a format.

    \sa setNamedColor(), NameFormat
*/

QString QColor::name(NameFormat format) const
{
    switch (format) {
    case HexRgb:
        return QLatin1Char('#') + QString::number(rgba() | 0x1000000, 16).rightRef(6);
    case HexArgb:
        // it's called rgba() but it does return AARRGGBB
        return QLatin1Char('#') + QString::number(rgba() | Q_INT64_C(0x100000000), 16).rightRef(8);
    }
    return QString();
}

#if QT_STRINGVIEW_LEVEL < 2
/*!
    Sets the RGB value of this QColor to \a name, which may be in one
    of these formats:

    \list
    \li #RGB (each of R, G, and B is a single hex digit)
    \li #RRGGBB
    \li #AARRGGBB (Since 5.2)
    \li #RRRGGGBBB
    \li #RRRRGGGGBBBB
    \li A name from the list of colors defined in the list of \l{http://www.w3.org/TR/SVG/types.html#ColorKeywords}{SVG color keyword names}
       provided by the World Wide Web Consortium; for example, "steelblue" or "gainsboro".
       These color names work on all platforms. Note that these color names are \e not the
       same as defined by the Qt::GlobalColor enums, e.g. "green" and Qt::green does not
       refer to the same color.
    \li \c transparent - representing the absence of a color.
    \endlist

    The color is invalid if \a name cannot be parsed.

    \sa QColor(), name(), isValid()
*/

void QColor::setNamedColor(const QString &name)
{
    setColorFromString(qToStringViewIgnoringNull(name));
}
#endif

/*!
    \overload
    \since 5.10
*/

void QColor::setNamedColor(QStringView name)
{
    setColorFromString(name);
}

/*!
    \overload
    \since 5.8
*/

void QColor::setNamedColor(QLatin1String name)
{
    setColorFromString(name);
}

#if QT_STRINGVIEW_LEVEL < 2
/*!
   \since 4.7

   Returns \c true if the \a name is a valid color name and can
   be used to construct a valid QColor object, otherwise returns
   false.

   It uses the same algorithm used in setNamedColor().

   \sa setNamedColor()
*/
bool QColor::isValidColor(const QString &name)
{
    return isValidColor(qToStringViewIgnoringNull(name));
}
#endif

/*!
    \overload
    \since 5.10
*/
bool QColor::isValidColor(QStringView name) Q_DECL_NOTHROW
{
    return name.size() && QColor().setColorFromString(name);
}

/*!
    \overload
    \since 5.8
*/
bool QColor::isValidColor(QLatin1String name) Q_DECL_NOTHROW
{
    return name.size() && QColor().setColorFromString(name);
}

template <typename String>
bool QColor::setColorFromString(String name)
{
    if (!name.size()) {
        invalidate();
        return true;
    }

    if (name[0] == QLatin1Char('#')) {
        QRgb rgba;
        if (get_hex_rgb(name.data(), name.size(), &rgba)) {
            setRgba(rgba);
            return true;
        } else {
            invalidate();
            return false;
        }
    }

#ifndef QT_NO_COLORNAMES
    QRgb rgb;
    if (get_named_rgb(name.data(), name.size(), &rgb)) {
        setRgba(rgb);
        return true;
    } else
#endif
    {
        invalidate();
        return false;
    }
}

/*!
    Returns a QStringList containing the color names Qt knows about.

    \sa {QColor#Predefined Colors}{Predefined Colors}
*/
QStringList QColor::colorNames()
{
    return get_colornames();
}

/*!
    Sets the contents pointed to by \a h, \a s, \a v, and \a a, to the hue,
    saturation, value, and alpha-channel (transparency) components of the
    color's HSV value.

    These components can be retrieved individually using the hueF(),
    saturationF(), valueF() and alphaF() functions.

    \sa setHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
void QColor::getHsvF(qreal *h, qreal *s, qreal *v, qreal *a) const
{
        if (!h || !s || !v)
        return;

    if (cspec != Invalid && cspec != Hsv) {
        toHsv().getHsvF(h, s, v, a);
        return;
    }

    *h = ct.ahsv.hue == USHRT_MAX ? qreal(-1.0) : ct.ahsv.hue / qreal(36000.0);
    *s = ct.ahsv.saturation / qreal(USHRT_MAX);
    *v = ct.ahsv.value / qreal(USHRT_MAX);

    if (a)
        *a = ct.ahsv.alpha / qreal(USHRT_MAX);
}

/*!
    Sets the contents pointed to by \a h, \a s, \a v, and \a a, to the hue,
    saturation, value, and alpha-channel (transparency) components of the
    color's HSV value.

    These components can be retrieved individually using the hue(),
    saturation(), value() and alpha() functions.

    \sa setHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
void QColor::getHsv(int *h, int *s, int *v, int *a) const
{
    if (!h || !s || !v)
        return;

    if (cspec != Invalid && cspec != Hsv) {
        toHsv().getHsv(h, s, v, a);
        return;
    }

    *h = ct.ahsv.hue == USHRT_MAX ? -1 : ct.ahsv.hue / 100;
    *s = ct.ahsv.saturation >> 8;
    *v = ct.ahsv.value      >> 8;

    if (a)
        *a = ct.ahsv.alpha >> 8;
}

/*!
    Sets a HSV color value; \a h is the hue, \a s is the saturation, \a v is
    the value and \a a is the alpha component of the HSV color.

    All the values must be in the range 0.0-1.0.

    \sa getHsvF(), setHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
void QColor::setHsvF(qreal h, qreal s, qreal v, qreal a)
{
    if (((h < qreal(0.0) || h > qreal(1.0)) && h != qreal(-1.0))
        || (s < qreal(0.0) || s > qreal(1.0))
        || (v < qreal(0.0) || v > qreal(1.0))
        || (a < qreal(0.0) || a > qreal(1.0))) {
        qWarning("QColor::setHsvF: HSV parameters out of range");
        return;
    }

    cspec = Hsv;
    ct.ahsv.alpha      = qRound(a * USHRT_MAX);
    ct.ahsv.hue        = h == qreal(-1.0) ? USHRT_MAX : qRound(h * 36000);
    ct.ahsv.saturation = qRound(s * USHRT_MAX);
    ct.ahsv.value      = qRound(v * USHRT_MAX);
    ct.ahsv.pad        = 0;
}

/*!
    Sets a HSV color value; \a h is the hue, \a s is the saturation, \a v is
    the value and \a a is the alpha component of the HSV color.

    The saturation, value and alpha-channel values must be in the range 0-255,
    and the hue value must be greater than -1.

    \sa getHsv(), setHsvF(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
void QColor::setHsv(int h, int s, int v, int a)
{
    if (h < -1 || (uint)s > 255 || (uint)v > 255 || (uint)a > 255) {
        qWarning("QColor::setHsv: HSV parameters out of range");
        invalidate();
        return;
    }

    cspec = Hsv;
    ct.ahsv.alpha      = a * 0x101;
    ct.ahsv.hue        = h == -1 ? USHRT_MAX : (h % 360) * 100;
    ct.ahsv.saturation = s * 0x101;
    ct.ahsv.value      = v * 0x101;
    ct.ahsv.pad        = 0;
}

/*!
    \since 4.6

    Sets the contents pointed to by \a h, \a s, \a l, and \a a, to the hue,
    saturation, lightness, and alpha-channel (transparency) components of the
    color's HSL value.

    These components can be retrieved individually using the hslHueF(),
    hslSaturationF(), lightnessF() and alphaF() functions.

    \sa setHsl()
*/
void QColor::getHslF(qreal *h, qreal *s, qreal *l, qreal *a) const
{
        if (!h || !s || !l)
        return;

    if (cspec != Invalid && cspec != Hsl) {
        toHsl().getHslF(h, s, l, a);
        return;
    }

    *h = ct.ahsl.hue == USHRT_MAX ? qreal(-1.0) : ct.ahsl.hue / qreal(36000.0);
    *s = ct.ahsl.saturation / qreal(USHRT_MAX);
    *l = ct.ahsl.lightness / qreal(USHRT_MAX);

    if (a)
        *a = ct.ahsl.alpha / qreal(USHRT_MAX);
}

/*!
    \since 4.6

    Sets the contents pointed to by \a h, \a s, \a l, and \a a, to the hue,
    saturation, lightness, and alpha-channel (transparency) components of the
    color's HSL value.

    These components can be retrieved individually using the hslHue(),
    hslSaturation(), lightness() and alpha() functions.

    \sa setHsl()
*/
void QColor::getHsl(int *h, int *s, int *l, int *a) const
{
    if (!h || !s || !l)
        return;

    if (cspec != Invalid && cspec != Hsl) {
        toHsl().getHsl(h, s, l, a);
        return;
    }

    *h = ct.ahsl.hue == USHRT_MAX ? -1 : ct.ahsl.hue / 100;
    *s = ct.ahsl.saturation >> 8;
    *l = ct.ahsl.lightness  >> 8;

    if (a)
        *a = ct.ahsl.alpha >> 8;
}

/*!
    \since 4.6

    Sets a HSL color lightness; \a h is the hue, \a s is the saturation, \a l is
    the lightness and \a a is the alpha component of the HSL color.

    All the values must be in the range 0.0-1.0.

    \sa getHslF(), setHsl()
*/
void QColor::setHslF(qreal h, qreal s, qreal l, qreal a)
{
    if (((h < qreal(0.0) || h > qreal(1.0)) && h != qreal(-1.0))
        || (s < qreal(0.0) || s > qreal(1.0))
        || (l < qreal(0.0) || l > qreal(1.0))
        || (a < qreal(0.0) || a > qreal(1.0))) {
        qWarning("QColor::setHsvF: HSV parameters out of range");
        return;
    }

    cspec = Hsl;
    ct.ahsl.alpha      = qRound(a * USHRT_MAX);
    ct.ahsl.hue        = h == qreal(-1.0) ? USHRT_MAX : qRound(h * 36000);
    ct.ahsl.saturation = qRound(s * USHRT_MAX);
    ct.ahsl.lightness  = qRound(l * USHRT_MAX);
    ct.ahsl.pad        = 0;
}

/*!
    \since 4.6

    Sets a HSL color value; \a h is the hue, \a s is the saturation, \a l is
    the lightness and \a a is the alpha component of the HSL color.

    The saturation, value and alpha-channel values must be in the range 0-255,
    and the hue value must be greater than -1.

    \sa getHsl(), setHslF()
*/
void QColor::setHsl(int h, int s, int l, int a)
{
    if (h < -1 || (uint)s > 255 || (uint)l > 255 || (uint)a > 255) {
        qWarning("QColor::setHsv: HSV parameters out of range");
        invalidate();
        return;
    }

    cspec = Hsl;
    ct.ahsl.alpha      = a * 0x101;
    ct.ahsl.hue        = h == -1 ? USHRT_MAX : (h % 360) * 100;
    ct.ahsl.saturation = s * 0x101;
    ct.ahsl.lightness  = l * 0x101;
    ct.ahsl.pad        = 0;
}

/*!
    Sets the contents pointed to by \a r, \a g, \a b, and \a a, to the red,
    green, blue, and alpha-channel (transparency) components of the color's
    RGB value.

    These components can be retrieved individually using the redF(), greenF(),
    blueF() and alphaF() functions.

    \sa rgb(), setRgb()
*/
void QColor::getRgbF(qreal *r, qreal *g, qreal *b, qreal *a) const
{
    if (!r || !g || !b)
        return;

    if (cspec != Invalid && cspec != Rgb) {
        toRgb().getRgbF(r, g, b, a);
        return;
    }

    *r = ct.argb.red   / qreal(USHRT_MAX);
    *g = ct.argb.green / qreal(USHRT_MAX);
    *b = ct.argb.blue  / qreal(USHRT_MAX);

    if (a)
        *a = ct.argb.alpha / qreal(USHRT_MAX);

}

/*!
    Sets the contents pointed to by \a r, \a g, \a b, and \a a, to the red,
    green, blue, and alpha-channel (transparency) components of the color's
    RGB value.

    These components can be retrieved individually using the red(), green(),
    blue() and alpha() functions.

    \sa rgb(), setRgb()
*/
void QColor::getRgb(int *r, int *g, int *b, int *a) const
{
    if (!r || !g || !b)
        return;

    if (cspec != Invalid && cspec != Rgb) {
        toRgb().getRgb(r, g, b, a);
        return;
    }

    *r = ct.argb.red   >> 8;
    *g = ct.argb.green >> 8;
    *b = ct.argb.blue  >> 8;

    if (a)
        *a = ct.argb.alpha >> 8;
}

/*!
    \fn void QColor::setRgbF(qreal r, qreal g, qreal b, qreal a)

    Sets the color channels of this color to \a r (red), \a g (green),
    \a b (blue) and \a a (alpha, transparency).

    All values must be in the range 0.0-1.0.

    \sa rgb(), getRgbF(), setRgb()
*/
void QColor::setRgbF(qreal r, qreal g, qreal b, qreal a)
{
    if (r < qreal(0.0) || r > qreal(1.0)
        || g < qreal(0.0) || g > qreal(1.0)
        || b < qreal(0.0) || b > qreal(1.0)
        || a < qreal(0.0) || a > qreal(1.0)) {
        qWarning("QColor::setRgbF: RGB parameters out of range");
        invalidate();
        return;
    }

    cspec = Rgb;
    ct.argb.alpha = qRound(a * USHRT_MAX);
    ct.argb.red   = qRound(r * USHRT_MAX);
    ct.argb.green = qRound(g * USHRT_MAX);
    ct.argb.blue  = qRound(b * USHRT_MAX);
    ct.argb.pad   = 0;
}

/*!
    Sets the RGB value to \a r, \a g, \a b and the alpha value to \a a.

    All the values must be in the range 0-255.

    \sa rgb(), getRgb(), setRgbF()
*/
void QColor::setRgb(int r, int g, int b, int a)
{
    if ((uint)r > 255 || (uint)g > 255 || (uint)b > 255 || (uint)a > 255) {
        qWarning("QColor::setRgb: RGB parameters out of range");
        invalidate();
        return;
    }

    cspec = Rgb;
    ct.argb.alpha = a * 0x101;
    ct.argb.red   = r * 0x101;
    ct.argb.green = g * 0x101;
    ct.argb.blue  = b * 0x101;
    ct.argb.pad   = 0;
}

/*!
    \fn QRgb QColor::rgba() const

    Returns the RGB value of the color, including its alpha.

    For an invalid color, the alpha value of the returned color is unspecified.

    \sa setRgba(), rgb(), rgba64()
*/

QRgb QColor::rgba() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().rgba();
    return qRgba(ct.argb.red >> 8, ct.argb.green >> 8, ct.argb.blue >> 8, ct.argb.alpha >> 8);
}

/*!
    Sets the RGB value to \a rgba, including its alpha.

    \sa rgba(), rgb(), setRgba64()
*/
void QColor::setRgba(QRgb rgba) Q_DECL_NOTHROW
{
    cspec = Rgb;
    ct.argb.alpha = qAlpha(rgba) * 0x101;
    ct.argb.red   = qRed(rgba)   * 0x101;
    ct.argb.green = qGreen(rgba) * 0x101;
    ct.argb.blue  = qBlue(rgba)  * 0x101;
    ct.argb.pad   = 0;
}

/*!
    \since 5.6

    Returns the RGB64 value of the color, including its alpha.

    For an invalid color, the alpha value of the returned color is unspecified.

    \sa setRgba64(), rgba(), rgb()
*/

QRgba64 QColor::rgba64() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().rgba64();
    return qRgba64(ct.argb.red, ct.argb.green, ct.argb.blue, ct.argb.alpha);
}

/*!
    \since 5.6

    Sets the RGB64 value to \a rgba, including its alpha.

    \sa \setRgba(), rgba64()
*/
void QColor::setRgba64(QRgba64 rgba) Q_DECL_NOTHROW
{
    cspec = Rgb;
    ct.argb.alpha = rgba.alpha();
    ct.argb.red   = rgba.red();
    ct.argb.green = rgba.green();
    ct.argb.blue  = rgba.blue();
    ct.argb.pad   = 0;
}

/*!
    \fn QRgb QColor::rgb() const

    Returns the RGB value of the color. The alpha value is opaque.

    \sa getRgb(), rgba()
*/
QRgb QColor::rgb() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().rgb();
    return qRgb(ct.argb.red >> 8, ct.argb.green >> 8, ct.argb.blue >> 8);
}

/*!
    \overload

    Sets the RGB value to \a rgb. The alpha value is set to opaque.
*/
void QColor::setRgb(QRgb rgb) Q_DECL_NOTHROW
{
    cspec = Rgb;
    ct.argb.alpha = 0xffff;
    ct.argb.red   = qRed(rgb)   * 0x101;
    ct.argb.green = qGreen(rgb) * 0x101;
    ct.argb.blue  = qBlue(rgb)  * 0x101;
    ct.argb.pad   = 0;
}

/*!
    Returns the alpha color component of this color.

    \sa setAlpha(), alphaF(), {QColor#Alpha-Blended Drawing}{Alpha-Blended Drawing}
*/
int QColor::alpha() const Q_DECL_NOTHROW
{ return ct.argb.alpha >> 8; }


/*!
    Sets the alpha of this color to \a alpha. Integer alpha is specified in the
    range 0-255.

    \sa alpha(), alphaF(), {QColor#Alpha-Blended Drawing}{Alpha-Blended Drawing}
*/

void QColor::setAlpha(int alpha)
{
    QCOLOR_INT_RANGE_CHECK("QColor::setAlpha", alpha);
    ct.argb.alpha = alpha * 0x101;
}

/*!
    Returns the alpha color component of this color.

    \sa setAlphaF(), alpha(), {QColor#Alpha-Blended Drawing}{Alpha-Blended Drawing}
*/
qreal QColor::alphaF() const Q_DECL_NOTHROW
{ return ct.argb.alpha / qreal(USHRT_MAX); }

/*!
    Sets the alpha of this color to \a alpha. qreal alpha is specified in the
    range 0.0-1.0.

    \sa alphaF(), alpha(), {QColor#Alpha-Blended Drawing}{Alpha-Blended Drawing}

*/
void QColor::setAlphaF(qreal alpha)
{
    QCOLOR_REAL_RANGE_CHECK("QColor::setAlphaF", alpha);
    qreal tmp = alpha * USHRT_MAX;
    ct.argb.alpha = qRound(tmp);
}


/*!
    Returns the red color component of this color.

    \sa setRed(), redF(), getRgb()
*/
int QColor::red() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().red();
    return ct.argb.red >> 8;
}

/*!
    Sets the red color component of this color to \a red. Integer components
    are specified in the range 0-255.

    \sa red(), redF(), setRgb()
*/
void QColor::setRed(int red)
{
    QCOLOR_INT_RANGE_CHECK("QColor::setRed", red);
    if (cspec != Rgb)
        setRgb(red, green(), blue(), alpha());
    else
        ct.argb.red = red * 0x101;
}

/*!
    Returns the green color component of this color.

    \sa setGreen(), greenF(), getRgb()
*/
int QColor::green() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().green();
    return ct.argb.green >> 8;
}

/*!
    Sets the green color component of this color to \a green. Integer
    components are specified in the range 0-255.

    \sa green(), greenF(), setRgb()
*/
void QColor::setGreen(int green)
{
    QCOLOR_INT_RANGE_CHECK("QColor::setGreen", green);
    if (cspec != Rgb)
        setRgb(red(), green, blue(), alpha());
    else
        ct.argb.green = green * 0x101;
}


/*!
    Returns the blue color component of this color.

    \sa setBlue(), blueF(), getRgb()
*/
int QColor::blue() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().blue();
    return ct.argb.blue >> 8;
}


/*!
    Sets the blue color component of this color to \a blue. Integer components
    are specified in the range 0-255.

    \sa blue(), blueF(), setRgb()
*/
void QColor::setBlue(int blue)
{
    QCOLOR_INT_RANGE_CHECK("QColor::setBlue", blue);
    if (cspec != Rgb)
        setRgb(red(), green(), blue, alpha());
    else
        ct.argb.blue = blue * 0x101;
}

/*!
    Returns the red color component of this color.

    \sa setRedF(), red(), getRgbF()
*/
qreal QColor::redF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().redF();
    return ct.argb.red / qreal(USHRT_MAX);
}


/*!
    Sets the red color component of this color to \a red. Float components
    are specified in the range 0.0-1.0.

    \sa redF(), red(), setRgbF()
*/
void QColor::setRedF(qreal red)
{
    QCOLOR_REAL_RANGE_CHECK("QColor::setRedF", red);
    if (cspec != Rgb)
        setRgbF(red, greenF(), blueF(), alphaF());
    else
        ct.argb.red = qRound(red * USHRT_MAX);
}

/*!
    Returns the green color component of this color.

    \sa setGreenF(), green(), getRgbF()
*/
qreal QColor::greenF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().greenF();
    return ct.argb.green / qreal(USHRT_MAX);
}


/*!
    Sets the green color component of this color to \a green. Float components
    are specified in the range 0.0-1.0.

    \sa greenF(), green(), setRgbF()
*/
void QColor::setGreenF(qreal green)
{
    QCOLOR_REAL_RANGE_CHECK("QColor::setGreenF", green);
    if (cspec != Rgb)
        setRgbF(redF(), green, blueF(), alphaF());
    else
        ct.argb.green = qRound(green * USHRT_MAX);
}

/*!
    Returns the blue color component of this color.

     \sa setBlueF(), blue(), getRgbF()
*/
qreal QColor::blueF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Rgb)
        return toRgb().blueF();
    return ct.argb.blue / qreal(USHRT_MAX);
}

/*!
    Sets the blue color component of this color to \a blue. Float components
    are specified in the range 0.0-1.0.

    \sa blueF(), blue(), setRgbF()
*/
void QColor::setBlueF(qreal blue)
{
    QCOLOR_REAL_RANGE_CHECK("QColor::setBlueF", blue);
    if (cspec != Rgb)
        setRgbF(redF(), greenF(), blue, alphaF());
    else
        ct.argb.blue = qRound(blue * USHRT_MAX);
}

/*!
    Returns the hue color component of this color.

    The color is implicitly converted to HSV.

    \sa hsvHue(), hueF(), getHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/

int QColor::hue() const Q_DECL_NOTHROW
{
    return hsvHue();
}

/*!
    Returns the hue color component of this color.

    \sa hueF(), getHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
int QColor::hsvHue() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().hue();
    return ct.ahsv.hue == USHRT_MAX ? -1 : ct.ahsv.hue / 100;
}

/*!
    Returns the saturation color component of this color.

    The color is implicitly converted to HSV.

    \sa hsvSaturation(), saturationF(), getHsv(), {QColor#The HSV Color Model}{The HSV Color
    Model}
*/

int QColor::saturation() const Q_DECL_NOTHROW
{
    return hsvSaturation();
}

/*!
    Returns the saturation color component of this color.

    \sa saturationF(), getHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
int QColor::hsvSaturation() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().saturation();
    return ct.ahsv.saturation >> 8;
}

/*!
    Returns the value color component of this color.

    \sa valueF(), getHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
int QColor::value() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().value();
    return ct.ahsv.value >> 8;
}

/*!
    Returns the hue color component of this color.

    The color is implicitly converted to HSV.

    \sa hsvHueF(), hue(), getHsvF(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
qreal QColor::hueF() const Q_DECL_NOTHROW
{
    return hsvHueF();
}

/*!
    Returns the hue color component of this color.

    \sa hue(), getHsvF(), {QColor#The HSV Color Model}{The HSV Color
    Model}
*/
qreal QColor::hsvHueF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().hueF();
    return ct.ahsv.hue == USHRT_MAX ? qreal(-1.0) : ct.ahsv.hue / qreal(36000.0);
}

/*!
    Returns the saturation color component of this color.

     The color is implicitly converted to HSV.

    \sa hsvSaturationF(), saturation(), getHsvF(), {QColor#The HSV Color Model}{The HSV Color
    Model}
*/
qreal QColor::saturationF() const Q_DECL_NOTHROW
{
    return hsvSaturationF();
}

/*!
    Returns the saturation color component of this color.

    \sa saturation(), getHsvF(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
qreal QColor::hsvSaturationF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().saturationF();
    return ct.ahsv.saturation / qreal(USHRT_MAX);
}

/*!
    Returns the value color component of this color.

    \sa value(), getHsvF(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
qreal QColor::valueF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsv)
        return toHsv().valueF();
    return ct.ahsv.value / qreal(USHRT_MAX);
}

/*!
    \since 4.6

    Returns the hue color component of this color.

    \sa getHslF(), getHsl()
*/
int QColor::hslHue() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().hslHue();
    return ct.ahsl.hue == USHRT_MAX ? -1 : ct.ahsl.hue / 100;
}

/*!
    \since 4.6

    Returns the saturation color component of this color.

    \sa saturationF(), getHsv(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
int QColor::hslSaturation() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().hslSaturation();
    return ct.ahsl.saturation >> 8;
}

/*!
    \since 4.6

    Returns the lightness color component of this color.

    \sa lightnessF(), getHsl()
*/
int QColor::lightness() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().lightness();
    return ct.ahsl.lightness >> 8;
}

/*!
    \since 4.6

    Returns the hue color component of this color.

    \sa hue(), getHslF()
*/
qreal QColor::hslHueF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().hslHueF();
    return ct.ahsl.hue == USHRT_MAX ? qreal(-1.0) : ct.ahsl.hue / qreal(36000.0);
}

/*!
    \since 4.6

    Returns the saturation color component of this color.

    \sa saturationF(), getHslF()
*/
qreal QColor::hslSaturationF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().hslSaturationF();
    return ct.ahsl.saturation / qreal(USHRT_MAX);
}

/*!
    \since 4.6

    Returns the lightness color component of this color.

    \sa value(), getHslF()
*/
qreal QColor::lightnessF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Hsl)
        return toHsl().lightnessF();
    return ct.ahsl.lightness / qreal(USHRT_MAX);
}

/*!
    Returns the cyan color component of this color.

    \sa cyanF(), getCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
int QColor::cyan() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().cyan();
    return ct.acmyk.cyan >> 8;
}

/*!
    Returns the magenta color component of this color.

    \sa magentaF(), getCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
int QColor::magenta() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().magenta();
    return ct.acmyk.magenta >> 8;
}

/*!
    Returns the yellow color component of this color.

    \sa yellowF(), getCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
int QColor::yellow() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().yellow();
    return ct.acmyk.yellow >> 8;
}

/*!
    Returns the black color component of this color.

    \sa blackF(), getCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}

*/
int QColor::black() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().black();
    return ct.acmyk.black >> 8;
}

/*!
    Returns the cyan color component of this color.

    \sa cyan(), getCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
qreal QColor::cyanF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().cyanF();
    return ct.acmyk.cyan / qreal(USHRT_MAX);
}

/*!
    Returns the magenta color component of this color.

    \sa magenta(), getCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
qreal QColor::magentaF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().magentaF();
    return ct.acmyk.magenta / qreal(USHRT_MAX);
}

/*!
    Returns the yellow color component of this color.

     \sa yellow(), getCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
qreal QColor::yellowF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().yellowF();
    return ct.acmyk.yellow / qreal(USHRT_MAX);
}

/*!
    Returns the black color component of this color.

    \sa black(), getCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
qreal QColor::blackF() const Q_DECL_NOTHROW
{
    if (cspec != Invalid && cspec != Cmyk)
        return toCmyk().blackF();
    return ct.acmyk.black / qreal(USHRT_MAX);
}

/*!
    Create and returns an RGB QColor based on this color.

    \sa fromRgb(), convertTo(), isValid()
*/
QColor QColor::toRgb() const Q_DECL_NOTHROW
{
    if (!isValid() || cspec == Rgb)
        return *this;

    QColor color;
    color.cspec = Rgb;
    color.ct.argb.alpha = ct.argb.alpha;
    color.ct.argb.pad = 0;

    switch (cspec) {
    case Hsv:
        {
            if (ct.ahsv.saturation == 0 || ct.ahsv.hue == USHRT_MAX) {
                // achromatic case
                color.ct.argb.red = color.ct.argb.green = color.ct.argb.blue = ct.ahsv.value;
                break;
            }

            // chromatic case
            const qreal h = ct.ahsv.hue == 36000 ? 0 : ct.ahsv.hue / 6000.;
            const qreal s = ct.ahsv.saturation / qreal(USHRT_MAX);
            const qreal v = ct.ahsv.value / qreal(USHRT_MAX);
            const int i = int(h);
            const qreal f = h - i;
            const qreal p = v * (qreal(1.0) - s);

            if (i & 1) {
                const qreal q = v * (qreal(1.0) - (s * f));

                switch (i) {
                case 1:
                    color.ct.argb.red   = qRound(q * USHRT_MAX);
                    color.ct.argb.green = qRound(v * USHRT_MAX);
                    color.ct.argb.blue  = qRound(p * USHRT_MAX);
                    break;
                case 3:
                    color.ct.argb.red   = qRound(p * USHRT_MAX);
                    color.ct.argb.green = qRound(q * USHRT_MAX);
                    color.ct.argb.blue  = qRound(v * USHRT_MAX);
                    break;
                case 5:
                    color.ct.argb.red   = qRound(v * USHRT_MAX);
                    color.ct.argb.green = qRound(p * USHRT_MAX);
                    color.ct.argb.blue  = qRound(q * USHRT_MAX);
                    break;
                }
            } else {
                const qreal t = v * (qreal(1.0) - (s * (qreal(1.0) - f)));

                switch (i) {
                case 0:
                    color.ct.argb.red   = qRound(v * USHRT_MAX);
                    color.ct.argb.green = qRound(t * USHRT_MAX);
                    color.ct.argb.blue  = qRound(p * USHRT_MAX);
                    break;
                case 2:
                    color.ct.argb.red   = qRound(p * USHRT_MAX);
                    color.ct.argb.green = qRound(v * USHRT_MAX);
                    color.ct.argb.blue  = qRound(t * USHRT_MAX);
                    break;
                case 4:
                    color.ct.argb.red   = qRound(t * USHRT_MAX);
                    color.ct.argb.green = qRound(p * USHRT_MAX);
                    color.ct.argb.blue  = qRound(v * USHRT_MAX);
                    break;
                }
            }
            break;
        }
    case Hsl:
        {
            if (ct.ahsl.saturation == 0 || ct.ahsl.hue == USHRT_MAX) {
                // achromatic case
                color.ct.argb.red = color.ct.argb.green = color.ct.argb.blue = ct.ahsl.lightness;
            } else if (ct.ahsl.lightness == 0) {
                // lightness 0
                color.ct.argb.red = color.ct.argb.green = color.ct.argb.blue = 0;
            } else {
                // chromatic case
                const qreal h = ct.ahsl.hue == 36000 ? 0 : ct.ahsl.hue / 36000.;
                const qreal s = ct.ahsl.saturation / qreal(USHRT_MAX);
                const qreal l = ct.ahsl.lightness / qreal(USHRT_MAX);

                qreal temp2;
                if (l < qreal(0.5))
                    temp2 = l * (qreal(1.0) + s);
                else
                    temp2 = l + s - (l * s);

                const qreal temp1 = (qreal(2.0) * l) - temp2;
                qreal temp3[3] = { h + (qreal(1.0) / qreal(3.0)),
                                   h,
                                   h - (qreal(1.0) / qreal(3.0)) };

                for (int i = 0; i != 3; ++i) {
                    if (temp3[i] < qreal(0.0))
                        temp3[i] += qreal(1.0);
                    else if (temp3[i] > qreal(1.0))
                        temp3[i] -= qreal(1.0);

                    const qreal sixtemp3 = temp3[i] * qreal(6.0);
                    if (sixtemp3 < qreal(1.0))
                        color.ct.array[i+1] = qRound((temp1 + (temp2 - temp1) * sixtemp3) * USHRT_MAX);
                    else if ((temp3[i] * qreal(2.0)) < qreal(1.0))
                        color.ct.array[i+1] = qRound(temp2 * USHRT_MAX);
                    else if ((temp3[i] * qreal(3.0)) < qreal(2.0))
                        color.ct.array[i+1] = qRound((temp1 + (temp2 -temp1) * (qreal(2.0) /qreal(3.0) - temp3[i]) * qreal(6.0)) * USHRT_MAX);
                    else
                        color.ct.array[i+1] = qRound(temp1 * USHRT_MAX);
                }
                color.ct.argb.red = color.ct.argb.red == 1 ? 0 : color.ct.argb.red;
                color.ct.argb.green = color.ct.argb.green == 1 ? 0 : color.ct.argb.green;
                color.ct.argb.blue = color.ct.argb.blue == 1 ? 0 : color.ct.argb.blue;
            }
            break;
        }
    case Cmyk:
        {
            const qreal c = ct.acmyk.cyan / qreal(USHRT_MAX);
            const qreal m = ct.acmyk.magenta / qreal(USHRT_MAX);
            const qreal y = ct.acmyk.yellow / qreal(USHRT_MAX);
            const qreal k = ct.acmyk.black / qreal(USHRT_MAX);

            color.ct.argb.red   = qRound((qreal(1.0) - (c * (qreal(1.0) - k) + k)) * USHRT_MAX);
            color.ct.argb.green = qRound((qreal(1.0) - (m * (qreal(1.0) - k) + k)) * USHRT_MAX);
            color.ct.argb.blue  = qRound((qreal(1.0) - (y * (qreal(1.0) - k) + k)) * USHRT_MAX);
            break;
        }
    default:
        break;
    }

    return color;
}


#define Q_MAX_3(a, b, c) ( ( a > b && a > c) ? a : (b > c ? b : c) )
#define Q_MIN_3(a, b, c) ( ( a < b && a < c) ? a : (b < c ? b : c) )


/*!
    Creates and returns an HSV QColor based on this color.

    \sa fromHsv(), convertTo(), isValid(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
QColor QColor::toHsv() const Q_DECL_NOTHROW
{
    if (!isValid() || cspec == Hsv)
        return *this;

    if (cspec != Rgb)
        return toRgb().toHsv();

    QColor color;
    color.cspec = Hsv;
    color.ct.ahsv.alpha = ct.argb.alpha;
    color.ct.ahsv.pad = 0;

    const qreal r = ct.argb.red   / qreal(USHRT_MAX);
    const qreal g = ct.argb.green / qreal(USHRT_MAX);
    const qreal b = ct.argb.blue  / qreal(USHRT_MAX);
    const qreal max = Q_MAX_3(r, g, b);
    const qreal min = Q_MIN_3(r, g, b);
    const qreal delta = max - min;
    color.ct.ahsv.value = qRound(max * USHRT_MAX);
    if (qFuzzyIsNull(delta)) {
        // achromatic case, hue is undefined
        color.ct.ahsv.hue = USHRT_MAX;
        color.ct.ahsv.saturation = 0;
    } else {
        // chromatic case
        qreal hue = 0;
        color.ct.ahsv.saturation = qRound((delta / max) * USHRT_MAX);
        if (qFuzzyCompare(r, max)) {
            hue = ((g - b) /delta);
        } else if (qFuzzyCompare(g, max)) {
            hue = (qreal(2.0) + (b - r) / delta);
        } else if (qFuzzyCompare(b, max)) {
            hue = (qreal(4.0) + (r - g) / delta);
        } else {
            Q_ASSERT_X(false, "QColor::toHsv", "internal error");
        }
        hue *= qreal(60.0);
        if (hue < qreal(0.0))
            hue += qreal(360.0);
        color.ct.ahsv.hue = qRound(hue * 100);
    }

    return color;
}

/*!
    Creates and returns an HSL QColor based on this color.

    \sa fromHsl(), convertTo(), isValid()
*/
QColor QColor::toHsl() const Q_DECL_NOTHROW
{
    if (!isValid() || cspec == Hsl)
        return *this;

    if (cspec != Rgb)
        return toRgb().toHsl();

    QColor color;
    color.cspec = Hsl;
    color.ct.ahsl.alpha = ct.argb.alpha;
    color.ct.ahsl.pad = 0;

    const qreal r = ct.argb.red   / qreal(USHRT_MAX);
    const qreal g = ct.argb.green / qreal(USHRT_MAX);
    const qreal b = ct.argb.blue  / qreal(USHRT_MAX);
    const qreal max = Q_MAX_3(r, g, b);
    const qreal min = Q_MIN_3(r, g, b);
    const qreal delta = max - min;
    const qreal delta2 = max + min;
    const qreal lightness = qreal(0.5) * delta2;
    color.ct.ahsl.lightness = qRound(lightness * USHRT_MAX);
    if (qFuzzyIsNull(delta)) {
        // achromatic case, hue is undefined
        color.ct.ahsl.hue = USHRT_MAX;
        color.ct.ahsl.saturation = 0;
    } else {
        // chromatic case
        qreal hue = 0;
        if (lightness < qreal(0.5))
            color.ct.ahsl.saturation = qRound((delta / delta2) * USHRT_MAX);
        else
            color.ct.ahsl.saturation = qRound((delta / (qreal(2.0) - delta2)) * USHRT_MAX);
        if (qFuzzyCompare(r, max)) {
            hue = ((g - b) /delta);
        } else if (qFuzzyCompare(g, max)) {
            hue = (qreal(2.0) + (b - r) / delta);
        } else if (qFuzzyCompare(b, max)) {
            hue = (qreal(4.0) + (r - g) / delta);
        } else {
            Q_ASSERT_X(false, "QColor::toHsv", "internal error");
        }
        hue *= qreal(60.0);
        if (hue < qreal(0.0))
            hue += qreal(360.0);
        color.ct.ahsl.hue = qRound(hue * 100);
    }

    return color;
}

/*!
    Creates and returns a CMYK QColor based on this color.

    \sa fromCmyk(), convertTo(), isValid(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
QColor QColor::toCmyk() const Q_DECL_NOTHROW
{
    if (!isValid() || cspec == Cmyk)
        return *this;
    if (cspec != Rgb)
        return toRgb().toCmyk();

    QColor color;
    color.cspec = Cmyk;
    color.ct.acmyk.alpha = ct.argb.alpha;

    // rgb -> cmy
    const qreal r = ct.argb.red   / qreal(USHRT_MAX);
    const qreal g = ct.argb.green / qreal(USHRT_MAX);
    const qreal b = ct.argb.blue  / qreal(USHRT_MAX);
    qreal c = qreal(1.0) - r;
    qreal m = qreal(1.0) - g;
    qreal y = qreal(1.0) - b;

    // cmy -> cmyk
    const qreal k = qMin(c, qMin(m, y));

    if (!qFuzzyIsNull(k - 1)) {
        c = (c - k) / (qreal(1.0) - k);
        m = (m - k) / (qreal(1.0) - k);
        y = (y - k) / (qreal(1.0) - k);
    }

    color.ct.acmyk.cyan    = qRound(c * USHRT_MAX);
    color.ct.acmyk.magenta = qRound(m * USHRT_MAX);
    color.ct.acmyk.yellow  = qRound(y * USHRT_MAX);
    color.ct.acmyk.black   = qRound(k * USHRT_MAX);

    return color;
}

QColor QColor::convertTo(QColor::Spec colorSpec) const Q_DECL_NOTHROW
{
    if (colorSpec == cspec)
        return *this;
    switch (colorSpec) {
    case Rgb:
        return toRgb();
    case Hsv:
        return toHsv();
    case Cmyk:
        return toCmyk();
    case Hsl:
        return toHsl();
    case Invalid:
        break;
    }
    return QColor(); // must be invalid
}


/*!
    Static convenience function that returns a QColor constructed from the
    given QRgb value \a rgb.

    The alpha component of \a rgb is ignored (i.e. it is automatically set to
    255), use the fromRgba() function to include the alpha-channel specified by
    the given QRgb value.

    \sa fromRgba(), fromRgbF(), toRgb(), isValid()
*/

QColor QColor::fromRgb(QRgb rgb) Q_DECL_NOTHROW
{
    return fromRgb(qRed(rgb), qGreen(rgb), qBlue(rgb));
}


/*!
    Static convenience function that returns a QColor constructed from the
    given QRgb value \a rgba.

    Unlike the fromRgb() function, the alpha-channel specified by the given
    QRgb value is included.

    \sa fromRgb(), fromRgba64(), isValid()
*/

QColor QColor::fromRgba(QRgb rgba) Q_DECL_NOTHROW
{
    return fromRgb(qRed(rgba), qGreen(rgba), qBlue(rgba), qAlpha(rgba));
}

/*!
    Static convenience function that returns a QColor constructed from the RGB
    color values, \a r (red), \a g (green), \a b (blue), and \a a
    (alpha-channel, i.e. transparency).

    All the values must be in the range 0-255.

    \sa toRgb(), fromRgba64(), fromRgbF(), isValid()
*/
QColor QColor::fromRgb(int r, int g, int b, int a)
{
    if (r < 0 || r > 255
        || g < 0 || g > 255
        || b < 0 || b > 255
        || a < 0 || a > 255) {
        qWarning("QColor::fromRgb: RGB parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Rgb;
    color.ct.argb.alpha = a * 0x101;
    color.ct.argb.red   = r * 0x101;
    color.ct.argb.green = g * 0x101;
    color.ct.argb.blue  = b * 0x101;
    color.ct.argb.pad   = 0;
    return color;
}

/*!
    Static convenience function that returns a QColor constructed from the RGB
    color values, \a r (red), \a g (green), \a b (blue), and \a a
    (alpha-channel, i.e. transparency).

    All the values must be in the range 0.0-1.0.

    \sa fromRgb(), fromRgba64(), toRgb(), isValid()
*/
QColor QColor::fromRgbF(qreal r, qreal g, qreal b, qreal a)
{
    if (r < qreal(0.0) || r > qreal(1.0)
        || g < qreal(0.0) || g > qreal(1.0)
        || b < qreal(0.0) || b > qreal(1.0)
        || a < qreal(0.0) || a > qreal(1.0)) {
        qWarning("QColor::fromRgbF: RGB parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Rgb;
    color.ct.argb.alpha = qRound(a * USHRT_MAX);
    color.ct.argb.red   = qRound(r * USHRT_MAX);
    color.ct.argb.green = qRound(g * USHRT_MAX);
    color.ct.argb.blue  = qRound(b * USHRT_MAX);
    color.ct.argb.pad   = 0;
    return color;
}


/*!
    \since 5.6

    Static convenience function that returns a QColor constructed from the RGBA64
    color values, \a r (red), \a g (green), \a b (blue), and \a a
    (alpha-channel, i.e. transparency).

    \sa fromRgb(), fromRgbF(), toRgb(), isValid()
*/
QColor QColor::fromRgba64(ushort r, ushort g, ushort b, ushort a) Q_DECL_NOTHROW
{
    QColor color;
    color.setRgba64(qRgba64(r, g, b, a));
    return color;
}

/*!
    \since 5.6

    Static convenience function that returns a QColor constructed from the
    given QRgba64 value \a rgba64.

    \sa fromRgb(), fromRgbF(), toRgb(), isValid()
*/
QColor QColor::fromRgba64(QRgba64 rgba64) Q_DECL_NOTHROW
{
    QColor color;
    color.setRgba64(rgba64);
    return color;
}

/*!
    Static convenience function that returns a QColor constructed from the HSV
    color values, \a h (hue), \a s (saturation), \a v (value), and \a a
    (alpha-channel, i.e. transparency).

    The value of \a s, \a v, and \a a must all be in the range 0-255; the value
    of \a h must be in the range 0-359.

    \sa toHsv(), fromHsvF(), isValid(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
QColor QColor::fromHsv(int h, int s, int v, int a)
{
    if (((h < 0 || h >= 360) && h != -1)
        || s < 0 || s > 255
        || v < 0 || v > 255
        || a < 0 || a > 255) {
        qWarning("QColor::fromHsv: HSV parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Hsv;
    color.ct.ahsv.alpha      = a * 0x101;
    color.ct.ahsv.hue        = h == -1 ? USHRT_MAX : (h % 360) * 100;
    color.ct.ahsv.saturation = s * 0x101;
    color.ct.ahsv.value      = v * 0x101;
    color.ct.ahsv.pad        = 0;
    return color;
}

/*!
    \overload

    Static convenience function that returns a QColor constructed from the HSV
    color values, \a h (hue), \a s (saturation), \a v (value), and \a a
    (alpha-channel, i.e. transparency).

    All the values must be in the range 0.0-1.0.

    \sa toHsv(), fromHsv(), isValid(), {QColor#The HSV Color Model}{The HSV Color Model}
*/
QColor QColor::fromHsvF(qreal h, qreal s, qreal v, qreal a)
{
    if (((h < qreal(0.0) || h > qreal(1.0)) && h != qreal(-1.0))
        || (s < qreal(0.0) || s > qreal(1.0))
        || (v < qreal(0.0) || v > qreal(1.0))
        || (a < qreal(0.0) || a > qreal(1.0))) {
        qWarning("QColor::fromHsvF: HSV parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Hsv;
    color.ct.ahsv.alpha      = qRound(a * USHRT_MAX);
    color.ct.ahsv.hue        = h == qreal(-1.0) ? USHRT_MAX : qRound(h * 36000);
    color.ct.ahsv.saturation = qRound(s * USHRT_MAX);
    color.ct.ahsv.value      = qRound(v * USHRT_MAX);
    color.ct.ahsv.pad        = 0;
    return color;
}

/*!
    \since 4.6

    Static convenience function that returns a QColor constructed from the HSV
    color values, \a h (hue), \a s (saturation), \a l (lightness), and \a a
    (alpha-channel, i.e. transparency).

    The value of \a s, \a l, and \a a must all be in the range 0-255; the value
    of \a h must be in the range 0-359.

    \sa toHsl(), fromHslF(), isValid()
*/
QColor QColor::fromHsl(int h, int s, int l, int a)
{
    if (((h < 0 || h >= 360) && h != -1)
        || s < 0 || s > 255
        || l < 0 || l > 255
        || a < 0 || a > 255) {
        qWarning("QColor::fromHsl: HSL parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Hsl;
    color.ct.ahsl.alpha      = a * 0x101;
    color.ct.ahsl.hue        = h == -1 ? USHRT_MAX : (h % 360) * 100;
    color.ct.ahsl.saturation = s * 0x101;
    color.ct.ahsl.lightness  = l * 0x101;
    color.ct.ahsl.pad        = 0;
    return color;
}

/*!
    \overload
    \since 4.6

    Static convenience function that returns a QColor constructed from the HSV
    color values, \a h (hue), \a s (saturation), \a l (lightness), and \a a
    (alpha-channel, i.e. transparency).

    All the values must be in the range 0.0-1.0.

    \sa toHsl(), fromHsl(), isValid()
*/
QColor QColor::fromHslF(qreal h, qreal s, qreal l, qreal a)
{
    if (((h < qreal(0.0) || h > qreal(1.0)) && h != qreal(-1.0))
        || (s < qreal(0.0) || s > qreal(1.0))
        || (l < qreal(0.0) || l > qreal(1.0))
        || (a < qreal(0.0) || a > qreal(1.0))) {
        qWarning("QColor::fromHslF: HSL parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Hsl;
    color.ct.ahsl.alpha      = qRound(a * USHRT_MAX);
    color.ct.ahsl.hue        = (h == qreal(-1.0)) ? USHRT_MAX : qRound(h * 36000);
    if (color.ct.ahsl.hue == 36000)
        color.ct.ahsl.hue = 0;
    color.ct.ahsl.saturation = qRound(s * USHRT_MAX);
    color.ct.ahsl.lightness  = qRound(l * USHRT_MAX);
    color.ct.ahsl.pad        = 0;
    return color;
}


/*!
    Sets the contents pointed to by \a c, \a m, \a y, \a k, and \a a, to the
    cyan, magenta, yellow, black, and alpha-channel (transparency) components
    of the color's CMYK value.

    These components can be retrieved individually using the cyan(), magenta(),
    yellow(), black() and alpha() functions.

    \sa setCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
void QColor::getCmyk(int *c, int *m, int *y, int *k, int *a)
{
    if (!c || !m || !y || !k)
        return;

    if (cspec != Invalid && cspec != Cmyk) {
        toCmyk().getCmyk(c, m, y, k, a);
        return;
    }

    *c = ct.acmyk.cyan >> 8;
    *m = ct.acmyk.magenta >> 8;
    *y = ct.acmyk.yellow >> 8;
    *k = ct.acmyk.black >> 8;

    if (a)
        *a = ct.acmyk.alpha >> 8;
}

/*!
    Sets the contents pointed to by \a c, \a m, \a y, \a k, and \a a, to the
    cyan, magenta, yellow, black, and alpha-channel (transparency) components
    of the color's CMYK value.

    These components can be retrieved individually using the cyanF(),
    magentaF(), yellowF(), blackF() and alphaF() functions.

    \sa setCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
void QColor::getCmykF(qreal *c, qreal *m, qreal *y, qreal *k, qreal *a)
{
    if (!c || !m || !y || !k)
        return;

    if (cspec != Invalid && cspec != Cmyk) {
        toCmyk().getCmykF(c, m, y, k, a);
        return;
    }

    *c = ct.acmyk.cyan    / qreal(USHRT_MAX);
    *m = ct.acmyk.magenta / qreal(USHRT_MAX);
    *y = ct.acmyk.yellow  / qreal(USHRT_MAX);
    *k = ct.acmyk.black   / qreal(USHRT_MAX);

    if (a)
        *a = ct.acmyk.alpha / qreal(USHRT_MAX);
}

/*!
    Sets the color to CMYK values, \a c (cyan), \a m (magenta), \a y (yellow),
    \a k (black), and \a a (alpha-channel, i.e. transparency).

    All the values must be in the range 0-255.

    \sa getCmyk(), setCmykF(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
void QColor::setCmyk(int c, int m, int y, int k, int a)
{
    if (c < 0 || c > 255
        || m < 0 || m > 255
        || y < 0 || y > 255
        || k < 0 || k > 255
        || a < 0 || a > 255) {
        qWarning("QColor::setCmyk: CMYK parameters out of range");
        return;
    }

    cspec = Cmyk;
    ct.acmyk.alpha   = a * 0x101;
    ct.acmyk.cyan    = c * 0x101;
    ct.acmyk.magenta = m * 0x101;
    ct.acmyk.yellow  = y * 0x101;
    ct.acmyk.black   = k * 0x101;
}

/*!
    \overload

    Sets the color to CMYK values, \a c (cyan), \a m (magenta), \a y (yellow),
    \a k (black), and \a a (alpha-channel, i.e. transparency).

    All the values must be in the range 0.0-1.0.

    \sa getCmykF(), setCmyk(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
void QColor::setCmykF(qreal c, qreal m, qreal y, qreal k, qreal a)
{
    if (c < qreal(0.0) || c > qreal(1.0)
        || m < qreal(0.0) || m > qreal(1.0)
        || y < qreal(0.0) || y > qreal(1.0)
        || k < qreal(0.0) || k > qreal(1.0)
        || a < qreal(0.0) || a > qreal(1.0)) {
        qWarning("QColor::setCmykF: CMYK parameters out of range");
        return;
    }

    cspec = Cmyk;
    ct.acmyk.alpha   = qRound(a * USHRT_MAX);
    ct.acmyk.cyan    = qRound(c * USHRT_MAX);
    ct.acmyk.magenta = qRound(m * USHRT_MAX);
    ct.acmyk.yellow  = qRound(y * USHRT_MAX);
    ct.acmyk.black   = qRound(k * USHRT_MAX);
}

/*!
    Static convenience function that returns a QColor constructed from the
    given CMYK color values: \a c (cyan), \a m (magenta), \a y (yellow), \a k
    (black), and \a a (alpha-channel, i.e. transparency).

    All the values must be in the range 0-255.

    \sa toCmyk(), fromCmykF(), isValid(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
QColor QColor::fromCmyk(int c, int m, int y, int k, int a)
{
    if (c < 0 || c > 255
        || m < 0 || m > 255
        || y < 0 || y > 255
        || k < 0 || k > 255
        || a < 0 || a > 255) {
        qWarning("QColor::fromCmyk: CMYK parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Cmyk;
    color.ct.acmyk.alpha   = a * 0x101;
    color.ct.acmyk.cyan    = c * 0x101;
    color.ct.acmyk.magenta = m * 0x101;
    color.ct.acmyk.yellow  = y * 0x101;
    color.ct.acmyk.black   = k * 0x101;
    return color;
}

/*!
    \overload

    Static convenience function that returns a QColor constructed from the
    given CMYK color values: \a c (cyan), \a m (magenta), \a y (yellow), \a k
    (black), and \a a (alpha-channel, i.e. transparency).

    All the values must be in the range 0.0-1.0.

    \sa toCmyk(), fromCmyk(), isValid(), {QColor#The CMYK Color Model}{The CMYK Color Model}
*/
QColor QColor::fromCmykF(qreal c, qreal m, qreal y, qreal k, qreal a)
{
    if (c < qreal(0.0) || c > qreal(1.0)
        || m < qreal(0.0) || m > qreal(1.0)
        || y < qreal(0.0) || y > qreal(1.0)
        || k < qreal(0.0) || k > qreal(1.0)
        || a < qreal(0.0) || a > qreal(1.0)) {
        qWarning("QColor::fromCmykF: CMYK parameters out of range");
        return QColor();
    }

    QColor color;
    color.cspec = Cmyk;
    color.ct.acmyk.alpha   = qRound(a * USHRT_MAX);
    color.ct.acmyk.cyan    = qRound(c * USHRT_MAX);
    color.ct.acmyk.magenta = qRound(m * USHRT_MAX);
    color.ct.acmyk.yellow  = qRound(y * USHRT_MAX);
    color.ct.acmyk.black   = qRound(k * USHRT_MAX);
    return color;
}

/*!
    \fn QColor QColor::lighter(int factor) const
    \since 4.3

    Returns a lighter (or darker) color, but does not change this object.

    If the \a factor is greater than 100, this functions returns a lighter
    color. Setting \a factor to 150 returns a color that is 50% brighter. If
    the \a factor is less than 100, the return color is darker, but we
    recommend using the darker() function for this purpose. If the \a factor
    is 0 or negative, the return value is unspecified.

    The function converts the current RGB color to HSV, multiplies the value
    (V) component by \a factor and converts the color back to RGB.

    \sa darker(), isValid()
*/

/*!
    \obsolete

    Use lighter(\a factor) instead.
*/
QColor QColor::light(int factor) const Q_DECL_NOTHROW
{
    if (factor <= 0)                                // invalid lightness factor
        return *this;
    else if (factor < 100)                        // makes color darker
        return darker(10000 / factor);

    QColor hsv = toHsv();
    int s = hsv.ct.ahsv.saturation;
    uint v = hsv.ct.ahsv.value;

    v = (factor*v)/100;
    if (v > USHRT_MAX) {
        // overflow... adjust saturation
        s -= v - USHRT_MAX;
        if (s < 0)
            s = 0;
        v = USHRT_MAX;
    }

    hsv.ct.ahsv.saturation = s;
    hsv.ct.ahsv.value = v;

    // convert back to same color spec as original color
    return hsv.convertTo(cspec);
}

/*!
    \fn QColor QColor::darker(int factor) const
    \since 4.3

    Returns a darker (or lighter) color, but does not change this object.

    If the \a factor is greater than 100, this functions returns a darker
    color. Setting \a factor to 300 returns a color that has one-third the
    brightness. If the \a factor is less than 100, the return color is lighter,
    but we recommend using the lighter() function for this purpose. If the
    \a factor is 0 or negative, the return value is unspecified.

    The function converts the current RGB color to HSV, divides the value (V)
    component by \a factor and converts the color back to RGB.

    \sa lighter(), isValid()
*/

/*!
    \obsolete

    Use darker(\a factor) instead.
*/
QColor QColor::dark(int factor) const Q_DECL_NOTHROW
{
    if (factor <= 0)                                // invalid darkness factor
        return *this;
    else if (factor < 100)                        // makes color lighter
        return lighter(10000 / factor);

    QColor hsv = toHsv();
    hsv.ct.ahsv.value = (hsv.ct.ahsv.value * 100) / factor;

    // convert back to same color spec as original color
    return hsv.convertTo(cspec);
}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
/*!
    Assigns a copy of \a color to this color, and returns a reference to it.
*/
QColor &QColor::operator=(const QColor &color) Q_DECL_NOTHROW
{
    cspec = color.cspec;
    ct.argb = color.ct.argb;
    return *this;
}
#endif

/*! \overload
    Assigns a copy of \a color and returns a reference to this color.
 */
QColor &QColor::operator=(Qt::GlobalColor color) Q_DECL_NOTHROW
{
    return operator=(QColor(color));
}

/*!
    Returns \c true if this color has the same RGB and alpha values as \a color;
    otherwise returns \c false.
*/
bool QColor::operator==(const QColor &color) const Q_DECL_NOTHROW
{
    if (cspec == Hsl && cspec == color.cspec) {
        return (ct.argb.alpha == color.ct.argb.alpha
                && ct.ahsl.hue % 36000 == color.ct.ahsl.hue % 36000
                && (qAbs(ct.ahsl.saturation - color.ct.ahsl.saturation) < 50
                    || ct.ahsl.lightness == 0
                    || color.ct.ahsl.lightness == 0
                    || ct.ahsl.lightness == USHRT_MAX
                    || color.ct.ahsl.lightness == USHRT_MAX)
                && (qAbs(ct.ahsl.lightness - color.ct.ahsl.lightness)) < 50);
    } else {
        return (cspec == color.cspec
                && ct.argb.alpha == color.ct.argb.alpha
                && (((cspec == QColor::Hsv)
                     && ((ct.ahsv.hue % 36000) == (color.ct.ahsv.hue % 36000)))
                    || (ct.ahsv.hue == color.ct.ahsv.hue))
                && ct.argb.green == color.ct.argb.green
                && ct.argb.blue  == color.ct.argb.blue
                && ct.argb.pad   == color.ct.argb.pad);
    }
}

/*!
    Returns \c true if this color has a different RGB and alpha values from
    \a color; otherwise returns \c false.
*/
bool QColor::operator!=(const QColor &color) const Q_DECL_NOTHROW
{ return !operator==(color); }


/*!
    Returns the color as a QVariant
*/
QColor::operator QVariant() const
{
    return QVariant(QVariant::Color, this);
}

/*! \internal

    Marks the color as invalid and sets all components to zero (alpha is set
    to fully opaque for compatibility with Qt 3).
*/
void QColor::invalidate() Q_DECL_NOTHROW
{
    cspec = Invalid;
    ct.argb.alpha = USHRT_MAX;
    ct.argb.red = 0;
    ct.argb.green = 0;
    ct.argb.blue = 0;
    ct.argb.pad = 0;
}

/*****************************************************************************
  QColor stream functions
 *****************************************************************************/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QColor &c)
{
    QDebugStateSaver saver(dbg);
    if (!c.isValid())
        dbg.nospace() << "QColor(Invalid)";
    else if (c.spec() == QColor::Rgb)
        dbg.nospace() << "QColor(ARGB " << c.alphaF() << ", " << c.redF() << ", " << c.greenF() << ", " << c.blueF() << ')';
    else if (c.spec() == QColor::Hsv)
        dbg.nospace() << "QColor(AHSV " << c.alphaF() << ", " << c.hueF() << ", " << c.saturationF() << ", " << c.valueF() << ')';
    else if (c.spec() == QColor::Cmyk)
        dbg.nospace() << "QColor(ACMYK " << c.alphaF() << ", " << c.cyanF() << ", " << c.magentaF() << ", " << c.yellowF() << ", "
                      << c.blackF()<< ')';
    else if (c.spec() == QColor::Hsl)
        dbg.nospace() << "QColor(AHSL " << c.alphaF() << ", " << c.hslHueF() << ", " << c.hslSaturationF() << ", " << c.lightnessF() << ')';

    return dbg;
}
#endif

#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QColor &color)
    \relates QColor

    Writes the \a color to the \a stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &stream, const QColor &color)
{
    if (stream.version() < 7) {
        if (!color.isValid())
            return stream << quint32(0x49000000);
        quint32 p = (quint32)color.rgb();
        if (stream.version() == 1) // Swap red and blue
            p = ((p << 16) & 0xff0000) | ((p >> 16) & 0xff) | (p & 0xff00ff00);
        return stream << p;
    }

    qint8   s = color.cspec;
    quint16 a = color.ct.argb.alpha;
    quint16 r = color.ct.argb.red;
    quint16 g = color.ct.argb.green;
    quint16 b = color.ct.argb.blue;
    quint16 p = color.ct.argb.pad;

    stream << s;
    stream << a;
    stream << r;
    stream << g;
    stream << b;
    stream << p;

    return stream;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QColor &color)
    \relates QColor

    Reads the \a color from the \a stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &stream, QColor &color)
{
    if (stream.version() < 7) {
        quint32 p;
        stream >> p;
        if (p == 0x49000000) {
            color.invalidate();
            return stream;
        }
        if (stream.version() == 1) // Swap red and blue
            p = ((p << 16) & 0xff0000) | ((p >> 16) & 0xff) | (p & 0xff00ff00);
        color.setRgb(p);
        return stream;
    }

    qint8 s;
    quint16 a, r, g, b, p;
    stream >> s;
    stream >> a;
    stream >> r;
    stream >> g;
    stream >> b;
    stream >> p;

    color.cspec = QColor::Spec(s);
    color.ct.argb.alpha = a;
    color.ct.argb.red   = r;
    color.ct.argb.green = g;
    color.ct.argb.blue  = b;
    color.ct.argb.pad   = p;

    return stream;
}
#endif // QT_NO_DATASTREAM

// A table of precalculated results of 0x00ff00ff/alpha use by qUnpremultiply:
const uint qt_inv_premul_factor[256] = {
    0, 16711935, 8355967, 5570645, 4177983, 3342387, 2785322, 2387419,
    2088991, 1856881, 1671193, 1519266, 1392661, 1285533, 1193709, 1114129,
    1044495, 983055, 928440, 879575, 835596, 795806, 759633, 726605,
    696330, 668477, 642766, 618960, 596854, 576273, 557064, 539094,
    522247, 506422, 491527, 477483, 464220, 451673, 439787, 428511,
    417798, 407608, 397903, 388649, 379816, 371376, 363302, 355573,
    348165, 341059, 334238, 327685, 321383, 315319, 309480, 303853,
    298427, 293191, 288136, 283253, 278532, 273966, 269547, 265268,
    261123, 257106, 253211, 249431, 245763, 242201, 238741, 235379,
    232110, 228930, 225836, 222825, 219893, 217038, 214255, 211543,
    208899, 206320, 203804, 201348, 198951, 196611, 194324, 192091,
    189908, 187774, 185688, 183647, 181651, 179698, 177786, 175915,
    174082, 172287, 170529, 168807, 167119, 165464, 163842, 162251,
    160691, 159161, 157659, 156186, 154740, 153320, 151926, 150557,
    149213, 147893, 146595, 145321, 144068, 142837, 141626, 140436,
    139266, 138115, 136983, 135869, 134773, 133695, 132634, 131590,
    130561, 129549, 128553, 127572, 126605, 125653, 124715, 123792,
    122881, 121984, 121100, 120229, 119370, 118524, 117689, 116866,
    116055, 115254, 114465, 113686, 112918, 112160, 111412, 110675,
    109946, 109228, 108519, 107818, 107127, 106445, 105771, 105106,
    104449, 103800, 103160, 102527, 101902, 101284, 100674, 100071,
    99475, 98887, 98305, 97730, 97162, 96600, 96045, 95496,
    94954, 94417, 93887, 93362, 92844, 92331, 91823, 91322,
    90825, 90334, 89849, 89368, 88893, 88422, 87957, 87497,
    87041, 86590, 86143, 85702, 85264, 84832, 84403, 83979,
    83559, 83143, 82732, 82324, 81921, 81521, 81125, 80733,
    80345, 79961, 79580, 79203, 78829, 78459, 78093, 77729,
    77370, 77013, 76660, 76310, 75963, 75619, 75278, 74941,
    74606, 74275, 73946, 73620, 73297, 72977, 72660, 72346,
    72034, 71725, 71418, 71114, 70813, 70514, 70218, 69924,
    69633, 69344, 69057, 68773, 68491, 68211, 67934, 67659,
    67386, 67116, 66847, 66581, 66317, 66055, 65795, 65537
};

/*****************************************************************************
  QColor global functions (documentation only)
 *****************************************************************************/

/*!
    \fn int qRed(QRgb rgb)
    \relates QColor

    Returns the red component of the ARGB quadruplet \a rgb.

    \sa qRgb(), QColor::red()
*/

/*!
    \fn int qGreen(QRgb rgb)
    \relates QColor

    Returns the green component of the ARGB quadruplet \a rgb.

    \sa qRgb(), QColor::green()
*/

/*!
    \fn int qBlue(QRgb rgb)
    \relates QColor

    Returns the blue component of the ARGB quadruplet \a rgb.

    \sa qRgb(), QColor::blue()
*/

/*!
    \fn int qAlpha(QRgb rgba)
    \relates QColor

    Returns the alpha component of the ARGB quadruplet \a rgba.

    \sa qRgb(), QColor::alpha()
*/

/*!
    \fn QRgb qRgb(int r, int g, int b)
    \relates QColor

    Returns the ARGB quadruplet (255, \a{r}, \a{g}, \a{b}).

    \sa qRgba(), qRed(), qGreen(), qBlue()
*/

/*!
    \fn QRgb qRgba(int r, int g, int b, int a)
    \relates QColor

    Returns the ARGB quadruplet (\a{a}, \a{r}, \a{g}, \a{b}).

    \sa qRgb(), qRed(), qGreen(), qBlue()
*/

/*!
    \fn int qGray(int r, int g, int b)
    \relates QColor

    Returns a gray value (0 to 255) from the (\a r, \a g, \a b)
    triplet.

    The gray value is calculated using the formula (\a r * 11 + \a g * 16 +
    \a b * 5)/32.
*/

/*!
    \fn int qGray(QRgb rgb)
    \overload
    \relates QColor

    Returns a gray value (0 to 255) from the given ARGB quadruplet \a rgb.

    The gray value is calculated using the formula (R * 11 + G * 16 + B * 5)/32;
    the alpha-channel is ignored.
*/

/*!
    \fn QRgb qPremultiply(QRgb rgb)
    \since 5.3
    \relates QColor

    Converts an unpremultiplied ARGB quadruplet \a rgb into a premultiplied ARGB quadruplet.

    \sa qUnpremultiply()
*/

/*!
    \fn QRgb qUnpremultiply(QRgb rgb)
    \since 5.3
    \relates QColor

    Converts a premultiplied ARGB quadruplet \a rgb into an unpremultiplied ARGB quadruplet.

    \sa qPremultiply()
*/

/*!
    \fn QColor QColor::convertTo(Spec colorSpec) const

    Creates a copy of \e this color in the format specified by \a colorSpec.

    \sa spec(), toCmyk(), toHsv(), toRgb(), isValid()
*/

/*!
    \typedef QRgb
    \relates QColor

    An ARGB quadruplet on the format #AARRGGBB, equivalent to an unsigned int.

    The type also holds a value for the alpha-channel. The default alpha
    channel is \c ff, i.e opaque. For more information, see the
    \l{QColor#Alpha-Blended Drawing}{Alpha-Blended Drawing} section.

    \sa QColor::rgb(), QColor::rgba()
*/

QT_END_NAMESPACE
