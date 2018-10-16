/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qimage.h>
#include <qimagereader.h>
#include <qlist.h>
#include <qmatrix.h>
#include <qrandom.h>
#include <stdio.h>

#include <qpainter.h>
#include <private/qimage_p.h>
#include <private/qdrawhelper_p.h>

#ifdef Q_OS_DARWIN
#include <CoreGraphics/CoreGraphics.h>
#endif

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
#  include <qt_windows.h>
#endif

Q_DECLARE_METATYPE(QImage::Format)
Q_DECLARE_METATYPE(Qt::GlobalColor)

class tst_QImage : public QObject
{
    Q_OBJECT

public:
    tst_QImage();

private slots:
    void initTestCase();
    void swap();
    void create();
    void createInvalidXPM();
    void createFromUChar();
    void formatHandlersInput_data();
    void formatHandlersInput();

    void setAlphaChannel_data();
    void setAlphaChannel();

    void alphaChannel();

    void convertToFormat_data();
    void convertToFormat();
    void convertToFormatWithColorTable();

    void convertToFormatRgb888ToRGB32();

    void createAlphaMask_data();
    void createAlphaMask();
#ifndef QT_NO_IMAGE_HEURISTIC_MASK
    void createHeuristicMask();
#endif

    void dotsPerMeterZero();
    void dotsPerMeterAndDpi();

    void convertToFormatPreserveDotsPrMeter();
    void convertToFormatPreserveText();

    void rotate_data();
    void rotate();

    void copy();

    void load();
    void loadFromData();
#if !defined(QT_NO_DATASTREAM)
    void loadFromDataStream();
#endif

    void setPixel_data();
    void setPixel();

    void defaultColorTable_data();
    void defaultColorTable();
    void setColorCount();
    void setColor();

    void rasterClipping();

    void pointOverloads();
    void destructor();
    void cacheKey();

    void smoothScale();
    void smoothScale2_data();
    void smoothScale2();
    void smoothScale3_data();
    void smoothScale3();
    void smoothScale4_data();
    void smoothScale4();

    void smoothScaleBig();
    void smoothScaleAlpha();

    void transformed_data();
    void transformed();
    void transformed2();

    void scaled();

    void paintEngine();
    void setAlphaChannelWhilePainting();

    void smoothScaledSubImage();

    void nullSize_data();
    void nullSize();

    void premultipliedAlphaConsistency();

    void compareIndexed();

    void fillColor_data();
    void fillColor();

    void fillColorWithAlpha();

    void fillRGB888();

    void fillPixel_data();
    void fillPixel();

    void rgbSwapped_data();
    void rgbSwapped();

    void mirrored_data();
    void mirrored();

    void inplaceRgbSwapped_data();
    void inplaceRgbSwapped();

    void inplaceMirrored_data();
    void inplaceMirrored();

    void inplaceMirroredOdd_data();
    void inplaceMirroredOdd();

    void inplaceRgbMirrored();

    void genericRgbConversion_data();
    void genericRgbConversion();

    void inplaceRgbConversion_data();
    void inplaceRgbConversion();

    void deepCopyWhenPaintingActive();
    void scaled_QTBUG19157();

    void convertOverUnPreMul();

    void scaled_QTBUG35972();

    void convertToPixelFormat();
    void convertToImageFormat_data();
    void convertToImageFormat();

    void invertPixelsRGB_data();
    void invertPixelsRGB();

    void exifOrientation_data();
    void exifOrientation();

    void exif_QTBUG45865();
    void exifInvalidData_data();
    void exifInvalidData();
    void exifReadComments();

    void cleanupFunctions();

    void devicePixelRatio();
    void rgb30Unpremul();
    void rgb30Repremul_data();
    void rgb30Repremul();

    void metadataPassthrough();

    void pixelColor();
    void pixel();

    void ditherGradient_data();
    void ditherGradient();

    void reinterpretAsFormat_data();
    void reinterpretAsFormat();

    void reinterpretAsFormat2();

    void complexTransform8bit();

#ifdef Q_OS_DARWIN
    void toCGImage_data();
    void toCGImage();
#endif

    void hugeQImage();

    void convertColorTable();

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
    void toWinHBITMAP_data();
    void toWinHBITMAP();
#endif // Q_OS_WIN && !Q_OS_WINRT

private:
    const QString m_prefix;
};

static QLatin1String formatToString(QImage::Format format)
{
    switch (format) {
    case QImage::Format_Invalid:
        return QLatin1String("Invalid");
    case QImage::Format_Mono:
        return QLatin1String("Mono");
    case QImage::Format_MonoLSB:
        return QLatin1String("MonoLSB");
    case QImage::Format_Indexed8:
        return QLatin1String("Indexed8");
    case QImage::Format_RGB32:
        return QLatin1String("RGB32");
    case QImage::Format_ARGB32:
        return QLatin1String("ARGB32");
    case QImage::Format_ARGB32_Premultiplied:
        return QLatin1String("ARGB32pm");
    case QImage::Format_RGB16:
        return QLatin1String("RGB16");
    case QImage::Format_ARGB8565_Premultiplied:
        return QLatin1String("ARGB8565pm");
    case QImage::Format_RGB666:
        return QLatin1String("RGB666");
    case QImage::Format_ARGB6666_Premultiplied:
        return QLatin1String("ARGB6666pm");
    case QImage::Format_RGB555:
        return QLatin1String("RGB555");
    case QImage::Format_ARGB8555_Premultiplied:
        return QLatin1String("ARGB8555pm");
    case QImage::Format_RGB888:
        return QLatin1String("RGB888");
    case QImage::Format_RGB444:
        return QLatin1String("RGB444");
    case QImage::Format_ARGB4444_Premultiplied:
        return QLatin1String("ARGB4444pm");
    case QImage::Format_RGBX8888:
        return QLatin1String("RGBx88888");
    case QImage::Format_RGBA8888:
        return QLatin1String("RGBA88888");
    case QImage::Format_RGBA8888_Premultiplied:
        return QLatin1String("RGBA88888pm");
    case QImage::Format_BGR30:
        return QLatin1String("BGR30");
    case QImage::Format_A2BGR30_Premultiplied:
        return QLatin1String("A2BGR30pm");
    case QImage::Format_RGB30:
        return QLatin1String("RGB30");
    case QImage::Format_A2RGB30_Premultiplied:
        return QLatin1String("A2RGB30pm");
    case QImage::Format_Alpha8:
        return QLatin1String("Alpha8");
    case QImage::Format_Grayscale8:
        return QLatin1String("Grayscale8");
    case QImage::Format_RGBX64:
        return QLatin1String("RGBx64");
    case QImage::Format_RGBA64:
        return QLatin1String("RGBA64");
    case QImage::Format_RGBA64_Premultiplied:
        return QLatin1String("RGBA64pm");
    default:
        break;
    };
    Q_UNREACHABLE();
    qWarning("Unhandled image format");
    return QLatin1String("unknown");
}

tst_QImage::tst_QImage()
    : m_prefix(QFINDTESTDATA("images/"))
{
}

void tst_QImage::initTestCase()
{
    QVERIFY(!m_prefix.isEmpty());
}

void tst_QImage::swap()
{
    QImage i1( 16, 16, QImage::Format_RGB32 ), i2( 32, 32, QImage::Format_RGB32 );
    i1.fill( Qt::white );
    i2.fill( Qt::black );
    const qint64 i1k = i1.cacheKey();
    const qint64 i2k = i2.cacheKey();
    i1.swap(i2);
    QCOMPARE(i1.cacheKey(), i2k);
    QCOMPARE(i1.size(), QSize(32,32));
    QCOMPARE(i2.cacheKey(), i1k);
    QCOMPARE(i2.size(), QSize(16,16));
}

// Test if QImage (or any functions called from QImage) throws an
// exception when creating an extremely large image.
// QImage::create() should return "false" in this case.
void tst_QImage::create()
{
    bool cr = true;
    QT_TRY {
        QImage image(700000000, 70000000, QImage::Format_Indexed8);
        image.setColorCount(256);
        cr = !image.isNull();
    } QT_CATCH (...) {
    }
    QVERIFY( !cr );
}

void tst_QImage::createInvalidXPM()
{
    QTest::ignoreMessage(QtWarningMsg, "QImage::QImage(), XPM is not supported");
    const char *xpm[] = {""};
    QImage invalidXPM(xpm);
    QVERIFY(invalidXPM.isNull());
}

void tst_QImage::createFromUChar()
{
    uint data[] = { 0xff010101U,
                    0xff020202U,
                    0xff030303U,
                    0xff040404U };

    // When the data is const, nothing you do to the image will change the source data.
    QImage i1((const uchar*)data, 2, 2, 8, QImage::Format_RGB32);
    QCOMPARE(i1.pixel(0,0), 0xFF010101U);
    QCOMPARE(i1.pixel(1,0), 0xFF020202U);
    QCOMPARE(i1.pixel(0,1), 0xFF030303U);
    QCOMPARE(i1.pixel(1,1), 0xFF040404U);
    {
        QImage i(i1);
        i.setPixel(0,0,5);
    }
    QCOMPARE(i1.pixel(0,0), 0xFF010101U);
    QCOMPARE(*(QRgb*)data, 0xFF010101U);
    *((QRgb*)i1.bits()) = 0xFF070707U;
    QCOMPARE(i1.pixel(0,0), 0xFF070707U);
    QCOMPARE(*(QRgb*)data, 0xFF010101U);

    // Changing copies should not change the original image or data.
    {
        QImage i(i1);
        i.setPixel(0,0,5);
        QCOMPARE(*(QRgb*)data, 0xFF010101U);
        i1.setPixel(0,0,9);
        QCOMPARE(i1.pixel(0,0), 0xFF000009U);
        QCOMPARE(i.pixel(0,0), 0xFF000005U);
    }
    QCOMPARE(i1.pixel(0,0), 0xFF000009U);

    // When the data is non-const, nothing you do to copies of the image will change the source data,
    // but changing the image (here via bits()) will change the source data.
    QImage i2((uchar*)data, 2, 2, 8, QImage::Format_RGB32);
    QCOMPARE(i2.pixel(0,0), 0xFF010101U);
    QCOMPARE(i2.pixel(1,0), 0xFF020202U);
    QCOMPARE(i2.pixel(0,1), 0xFF030303U);
    QCOMPARE(i2.pixel(1,1), 0xFF040404U);
    {
        QImage i(i2);
        i.setPixel(0,0,5);
    }
    QCOMPARE(i2.pixel(0,0), 0xFF010101U);
    QCOMPARE(*(QRgb*)data, 0xFF010101U);
    *((QRgb*)i2.bits()) = 0xFF070707U;
    QCOMPARE(i2.pixel(0,0), 0xFF070707U);
    QCOMPARE(*(QRgb*)data, 0xFF070707U);

    // Changing the data will change the image in either case.
    QImage i3((uchar*)data, 2, 2, 8, QImage::Format_RGB32);
    QImage i4((const uchar*)data, 2, 2, 8, QImage::Format_RGB32);
    *(QRgb*)data = 0xFF060606U;
    QCOMPARE(i3.pixel(0,0), 0xFF060606U);
    QCOMPARE(i4.pixel(0,0), 0xFF060606U);
}

void tst_QImage::formatHandlersInput_data()
{
    QTest::addColumn<QString>("testFormat");
    QTest::addColumn<QString>("testFile");

    // add a new line here when a file is added
    QTest::newRow("ICO") << "ICO" << m_prefix + "image.ico";
    QTest::newRow("PNG") << "PNG" << m_prefix + "image.png";
    QTest::newRow("GIF") << "GIF" << m_prefix + "image.gif";
    QTest::newRow("BMP") << "BMP" << m_prefix + "image.bmp";
    QTest::newRow("JPEG") << "JPEG" << m_prefix + "image.jpg";
    QTest::newRow("PBM") << "PBM" << m_prefix + "image.pbm";
    QTest::newRow("PGM") << "PGM" << m_prefix + "image.pgm";
    QTest::newRow("PPM") << "PPM" << m_prefix + "image.ppm";
    QTest::newRow("XBM") << "XBM" << m_prefix + "image.xbm";
    QTest::newRow("XPM") << "XPM" << m_prefix + "image.xpm";
}

void tst_QImage::formatHandlersInput()
{
    QFETCH(QString, testFormat);
    QFETCH(QString, testFile);
    QList<QByteArray> formats = QImageReader::supportedImageFormats();
   // qDebug("Image input formats : %s", formats.join(" | ").latin1());

    bool formatSupported = false;
    for (QList<QByteArray>::Iterator it = formats.begin(); it != formats.end(); ++it) {
        if (*it == testFormat.toLower()) {
            formatSupported = true;
            break;
        }
    }
    if (formatSupported) {
//     qDebug(QImage::imageFormat(testFile));
        QCOMPARE(testFormat.toLatin1().toLower(), QImageReader::imageFormat(testFile));
    } else {
        QString msg = "Format not supported : ";
        QSKIP(QString(msg + testFormat).toLatin1());
    }
}

void tst_QImage::setAlphaChannel_data()
{
    QTest::addColumn<int>("red");
    QTest::addColumn<int>("green");
    QTest::addColumn<int>("blue");
    QTest::addColumn<int>("alpha");
    QTest::addColumn<bool>("gray");

    QTest::newRow("red at 0%, gray") << 255 << 0 << 0 << 0 << true;
    QTest::newRow("red at 25%, gray") << 255 << 0 << 0 << 63 << true;
    QTest::newRow("red at 50%, gray") << 255 << 0 << 0 << 127 << true;
    QTest::newRow("red at 100%, gray") << 255 << 0 << 0 << 191 << true;
    QTest::newRow("red at 0%, 32bit") << 255 << 0 << 0 << 0 << false;
    QTest::newRow("red at 25%, 32bit") << 255 << 0 << 0 << 63 << false;
    QTest::newRow("red at 50%, 32bit") << 255 << 0 << 0 << 127 << false;
    QTest::newRow("red at 100%, 32bit") << 255 << 0 << 0 << 191 << false;

    QTest::newRow("green at 0%, gray") << 0 << 255 << 0 << 0 << true;
    QTest::newRow("green at 25%, gray") << 0 << 255 << 0 << 63 << true;
    QTest::newRow("green at 50%, gray") << 0 << 255 << 0 << 127 << true;
    QTest::newRow("green at 100%, gray") << 0 << 255 << 0 << 191 << true;
    QTest::newRow("green at 0%, 32bit") << 0 << 255 << 0 << 0 << false;
    QTest::newRow("green at 25%, 32bit") << 0 << 255 << 0 << 63 << false;
    QTest::newRow("green at 50%, 32bit") << 0 << 255 << 0 << 127 << false;
    QTest::newRow("green at 100%, 32bit") << 0 << 255 << 0 << 191 << false;

    QTest::newRow("blue at 0%, gray") << 0 << 0 << 255 << 0 << true;
    QTest::newRow("blue at 25%, gray") << 0 << 0 << 255 << 63 << true;
    QTest::newRow("blue at 50%, gray") << 0 << 0 << 255 << 127 << true;
    QTest::newRow("blue at 100%, gray") << 0 << 0 << 255 << 191 << true;
    QTest::newRow("blue at 0%, 32bit") << 0 << 0 << 255 << 0 << false;
    QTest::newRow("blue at 25%, 32bit") << 0 << 0 << 255 << 63 << false;
    QTest::newRow("blue at 50%, 32bit") << 0 << 0 << 255 << 127 << false;
    QTest::newRow("blue at 100%, 32bit") << 0 << 0 << 255 << 191 << false;
}

void tst_QImage::setAlphaChannel()
{
    QFETCH(int, red);
    QFETCH(int, green);
    QFETCH(int, blue);
    QFETCH(int, alpha);
    QFETCH(bool, gray);

    int width = 100;
    int height = 100;

    QImage image(width, height, QImage::Format_RGB32);
    image.fill(qRgb(red, green, blue));

    // Create the alpha channel
    QImage alphaChannel;
    if (gray) {
        alphaChannel = QImage(width, height, QImage::Format_Indexed8);
        alphaChannel.setColorCount(256);
        for (int i=0; i<256; ++i)
            alphaChannel.setColor(i, qRgb(i, i, i));
        alphaChannel.fill(alpha);
    } else {
        alphaChannel = QImage(width, height, QImage::Format_ARGB32);
        alphaChannel.fill(qRgb(alpha, alpha, alpha));
    }

    image.setAlphaChannel(alphaChannel);
    image = image.convertToFormat(QImage::Format_ARGB32);
    QCOMPARE(image.format(), QImage::Format_ARGB32);

    // alpha of 0 becomes black at a=0 due to premultiplication
    QRgb pixel = alpha == 0 ? 0 : qRgba(red, green, blue, alpha);
    bool allPixelsOK = true;
    for (int y=0; y<height; ++y) {
        for (int x=0; x<width; ++x) {
            allPixelsOK &= image.pixel(x, y) == pixel;
        }
    }
    QVERIFY(allPixelsOK);

    QImage outAlpha = image.alphaChannel();
    QCOMPARE(outAlpha.size(), image.size());

    bool allAlphaOk = true;
    for (int y=0; y<height; ++y) {
        for (int x=0; x<width; ++x) {
            allAlphaOk &= outAlpha.pixelIndex(x, y) == alpha;
        }
    }
    QVERIFY(allAlphaOk);

}

void tst_QImage::alphaChannel()
{
    QImage img(10, 10, QImage::Format_Mono);
    img.setColor(0, Qt::transparent);
    img.setColor(1, Qt::black);
    img.fill(0);

    QPainter p(&img);
    p.fillRect(2, 2, 6, 6, Qt::black);
    p.end();

    QCOMPARE(img.alphaChannel(), img.convertToFormat(QImage::Format_ARGB32).alphaChannel());
}

void tst_QImage::convertToFormat_data()
{
    QTest::addColumn<int>("inFormat");
    QTest::addColumn<uint>("inPixel");
    QTest::addColumn<int>("resFormat");
    QTest::addColumn<uint>("resPixel");

    QTest::newRow("red rgb32 -> argb32") << int(QImage::Format_RGB32) << 0xffff0000
                                      << int(QImage::Format_ARGB32) << 0xffff0000;
    QTest::newRow("green rgb32 -> argb32") << int(QImage::Format_RGB32) << 0xff00ff00
                                        << int(QImage::Format_ARGB32) << 0xff00ff00;
    QTest::newRow("blue rgb32 -> argb32") << int(QImage::Format_RGB32) << 0xff0000ff
                                       << int(QImage::Format_ARGB32) << 0xff0000ff;

    QTest::newRow("red rgb32 -> rgb16") << int(QImage::Format_RGB32) << 0xffff0000
                                      << int(QImage::Format_RGB16) << 0xffff0000;
    QTest::newRow("green rgb32 -> rgb16") << int(QImage::Format_RGB32) << 0xff00ff00
                                        << int(QImage::Format_RGB16) << 0xff00ff00;
    QTest::newRow("blue rgb32 -> rgb16") << int(QImage::Format_RGB32) << 0xff0000ff
                                       << int(QImage::Format_RGB16) << 0xff0000ff;
    QTest::newRow("funky rgb32 -> rgb16") << int(QImage::Format_RGB32) << 0xfff0c080
                                       << int(QImage::Format_RGB16) << 0xfff7c384;

    QTest::newRow("red rgb32 -> argb32_pm") << int(QImage::Format_RGB32) << 0xffff0000
                                         << int(QImage::Format_ARGB32_Premultiplied) << 0xffff0000;
    QTest::newRow("green rgb32 -> argb32_pm") << int(QImage::Format_RGB32) << 0xff00ff00
                                           << int(QImage::Format_ARGB32_Premultiplied) <<0xff00ff00;
    QTest::newRow("blue rgb32 -> argb32_pm") << int(QImage::Format_RGB32) << 0xff0000ff
                                          << int(QImage::Format_ARGB32_Premultiplied) << 0xff0000ff;

    QTest::newRow("semired argb32 -> pm") << int(QImage::Format_ARGB32) << 0x7fff0000u
                                       << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u;
    QTest::newRow("semigreen argb32 -> pm") << int(QImage::Format_ARGB32) << 0x7f00ff00u
                                         << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u;
    QTest::newRow("semiblue argb32 -> pm") << int(QImage::Format_ARGB32) << 0x7f0000ffu
                                        << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu;
    QTest::newRow("semiwhite argb32 -> pm") << int(QImage::Format_ARGB32) << 0x7fffffffu
                                         << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu;
    QTest::newRow("semiblack argb32 -> pm") << int(QImage::Format_ARGB32) << 0x7f000000u
                                         << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u;

    QTest::newRow("semired pm -> argb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u
                                       << int(QImage::Format_ARGB32) << 0x7fff0000u;
    QTest::newRow("semigreen pm -> argb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u
                                         << int(QImage::Format_ARGB32) << 0x7f00ff00u;
    QTest::newRow("semiblue pm -> argb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu
                                        << int(QImage::Format_ARGB32) << 0x7f0000ffu;
    QTest::newRow("semiwhite pm -> argb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu
                                         << int(QImage::Format_ARGB32) << 0x7fffffffu;
    QTest::newRow("semiblack pm -> argb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u
                                         << int(QImage::Format_ARGB32) << 0x7f000000u;

    QTest::newRow("semired pm -> rgb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u
                                       << int(QImage::Format_RGB32) << 0xffff0000u;
    QTest::newRow("semigreen pm -> rgb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u
                                         << int(QImage::Format_RGB32) << 0xff00ff00u;
    QTest::newRow("semiblue pm -> rgb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu
                                        << int(QImage::Format_RGB32) << 0xff0000ffu;
    QTest::newRow("semiwhite pm -> rgb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu
                                         << int(QImage::Format_RGB32) << 0xffffffffu;
    QTest::newRow("semiblack pm -> rgb32") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u
                                         << int(QImage::Format_RGB32) << 0xff000000u;

    QTest::newRow("semired argb32 -> rgb32") << int(QImage::Format_ARGB32) << 0x7fff0000u
                                             << int(QImage::Format_RGB32) << 0xffff0000u;
    QTest::newRow("semigreen argb32 -> rgb32") << int(QImage::Format_ARGB32) << 0x7f00ff00u
                                               << int(QImage::Format_RGB32) << 0xff00ff00u;
    QTest::newRow("semiblue argb32 -> rgb32") << int(QImage::Format_ARGB32) << 0x7f0000ffu
                                              << int(QImage::Format_RGB32) << 0xff0000ffu;
    QTest::newRow("semiwhite argb -> rgb32") << int(QImage::Format_ARGB32) << 0x7fffffffu
                                             << int(QImage::Format_RGB32) << 0xffffffffu;
    QTest::newRow("semiblack argb -> rgb32") << int(QImage::Format_ARGB32) << 0x7f000000u
                                             << int(QImage::Format_RGB32) << 0xff000000u;

    QTest::newRow("black mono -> rgb32") << int(QImage::Format_Mono) << 0x00000000u
                                         << int(QImage::Format_RGB32) << 0xff000000u;

    QTest::newRow("white mono -> rgb32") << int(QImage::Format_Mono) << 0x00000001u
                                         << int(QImage::Format_RGB32) << 0xffffffffu;
    QTest::newRow("red rgb16 -> argb32") << int(QImage::Format_RGB16) << 0xffff0000
                                         << int(QImage::Format_ARGB32) << 0xffff0000;
    QTest::newRow("green rgb16 -> argb32") << int(QImage::Format_RGB16) << 0xff00ff00
                                           << int(QImage::Format_ARGB32) << 0xff00ff00;
    QTest::newRow("blue rgb16 -> argb32") << int(QImage::Format_RGB16) << 0xff0000ff
                                          << int(QImage::Format_ARGB32) << 0xff0000ff;
    QTest::newRow("red rgb16 -> rgb16") << int(QImage::Format_RGB32) << 0xffff0000
                                         << int(QImage::Format_RGB16) << 0xffff0000;
    QTest::newRow("green rgb16 -> rgb16") << int(QImage::Format_RGB32) << 0xff00ff00
                                           << int(QImage::Format_RGB16) << 0xff00ff00;
    QTest::newRow("blue rgb16 -> rgb16") << int(QImage::Format_RGB32) << 0xff0000ff
                                          << int(QImage::Format_RGB16) << 0xff0000ff;
    QTest::newRow("semired argb32 -> rgb16") << int(QImage::Format_ARGB32) << 0x7fff0000u
                                             << int(QImage::Format_RGB16) << 0xffff0000;
    QTest::newRow("semigreen argb32 -> rgb16") << int(QImage::Format_ARGB32) << 0x7f00ff00u
                                               << int(QImage::Format_RGB16) << 0xff00ff00;
    QTest::newRow("semiblue argb32 -> rgb16") << int(QImage::Format_ARGB32) << 0x7f0000ffu
                                              << int(QImage::Format_RGB16) << 0xff0000ff;
    QTest::newRow("semired pm -> rgb16") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u
                                       << int(QImage::Format_RGB16) << 0xffff0000u;

    QTest::newRow("semigreen pm -> rgb16") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u
                                         << int(QImage::Format_RGB16) << 0xff00ff00u;
    QTest::newRow("semiblue pm -> rgb16") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu
                                        << int(QImage::Format_RGB16) << 0xff0000ffu;
    QTest::newRow("semiwhite pm -> rgb16") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu
                                         << int(QImage::Format_RGB16) << 0xffffffffu;
    QTest::newRow("semiblack pm -> rgb16") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u
                                         << int(QImage::Format_RGB16) << 0xff000000u;

    QTest::newRow("mono -> mono lsb") << int(QImage::Format_Mono) << 1u
                                      << int(QImage::Format_MonoLSB) << 0xffffffffu;
    QTest::newRow("mono lsb -> mono") << int(QImage::Format_MonoLSB) << 1u
                                      << int(QImage::Format_Mono) << 0xffffffffu;

    QTest::newRow("red rgb32 -> rgb666") << int(QImage::Format_RGB32) << 0xffff0000
                                        << int(QImage::Format_RGB666) << 0xffff0000;
    QTest::newRow("green rgb32 -> rgb666") << int(QImage::Format_RGB32) << 0xff00ff00
                                          << int(QImage::Format_RGB666) << 0xff00ff00;
    QTest::newRow("blue rgb32 -> rgb666") << int(QImage::Format_RGB32) << 0xff0000ff
                                         << int(QImage::Format_RGB666) << 0xff0000ff;

    QTest::newRow("red rgb16 -> rgb666") << int(QImage::Format_RGB16) << 0xffff0000
                                        << int(QImage::Format_RGB666) << 0xffff0000;
    QTest::newRow("green rgb16 -> rgb666") << int(QImage::Format_RGB16) << 0xff00ff00
                                          << int(QImage::Format_RGB666) << 0xff00ff00;
    QTest::newRow("blue rgb16 -> rgb666") << int(QImage::Format_RGB16) << 0xff0000ff
                                         << int(QImage::Format_RGB666) << 0xff0000ff;

    QTest::newRow("red rgb32 -> rgb15") << int(QImage::Format_RGB32) << 0xffff0000
                                        << int(QImage::Format_RGB555) << 0xffff0000;
    QTest::newRow("green rgb32 -> rgb15") << int(QImage::Format_RGB32) << 0xff00ff00
                                          << int(QImage::Format_RGB555) << 0xff00ff00;
    QTest::newRow("blue rgb32 -> rgb15") << int(QImage::Format_RGB32) << 0xff0000ff
                                         << int(QImage::Format_RGB555) << 0xff0000ff;
    QTest::newRow("funky rgb32 -> rgb15") << int(QImage::Format_RGB32) << 0xfff0c080
                                          << int(QImage::Format_RGB555) << 0xfff7c684;

    QTest::newRow("red rgb16 -> rgb15") << int(QImage::Format_RGB16) << 0xffff0000
                                        << int(QImage::Format_RGB555) << 0xffff0000;
    QTest::newRow("green rgb16 -> rgb15") << int(QImage::Format_RGB16) << 0xff00ff00
                                          << int(QImage::Format_RGB555) << 0xff00ff00;
    QTest::newRow("blue rgb16 -> rgb15") << int(QImage::Format_RGB16) << 0xff0000ff
                                         << int(QImage::Format_RGB555) << 0xff0000ff;
    QTest::newRow("funky rgb16 -> rgb15") << int(QImage::Format_RGB16) << 0xfff0c080
                                          << int(QImage::Format_RGB555) << 0xfff7c684;

    QTest::newRow("red rgb32 -> argb8565") << int(QImage::Format_RGB32) << 0xffff0000
                                           << int(QImage::Format_ARGB8565_Premultiplied) << 0xffff0000;
    QTest::newRow("green rgb32 -> argb8565") << int(QImage::Format_RGB32) << 0xff00ff00
                                             << int(QImage::Format_ARGB8565_Premultiplied) << 0xff00ff00;
    QTest::newRow("blue rgb32 -> argb8565") << int(QImage::Format_RGB32) << 0xff0000ff
                                            << int(QImage::Format_ARGB8565_Premultiplied) << 0xff0000ff;

    QTest::newRow("red rgb16 -> argb8565") << int(QImage::Format_RGB16) << 0xffff0000
                                           << int(QImage::Format_ARGB8565_Premultiplied) << 0xffff0000;
    QTest::newRow("green rgb16 -> argb8565") << int(QImage::Format_RGB16) << 0xff00ff00
                                             << int(QImage::Format_ARGB8565_Premultiplied) << 0xff00ff00;
    QTest::newRow("blue rgb16 -> argb8565") << int(QImage::Format_RGB16) << 0xff0000ff
                                            << int(QImage::Format_ARGB8565_Premultiplied) << 0xff0000ff;

    QTest::newRow("red argb8565 -> argb32") << int(QImage::Format_ARGB8565_Premultiplied) << 0xffff0000
                                            << int(QImage::Format_ARGB32) << 0xffff0000;
    QTest::newRow("green argb8565 -> argb32") << int(QImage::Format_ARGB8565_Premultiplied) << 0xff00ff00
                                              << int(QImage::Format_ARGB32) << 0xff00ff00;
    QTest::newRow("blue argb8565 -> argb32") << int(QImage::Format_ARGB8565_Premultiplied) << 0xff0000ff
                                             << int(QImage::Format_ARGB32) << 0xff0000ff;

    QTest::newRow("semired argb32 -> argb8565") << int(QImage::Format_ARGB32) << 0x7fff0000u
                                                << int(QImage::Format_ARGB8565_Premultiplied) << 0x7f7b0000u;
    QTest::newRow("semigreen argb32 -> argb8565") << int(QImage::Format_ARGB32) << 0x7f00ff00u
                                                  << int(QImage::Format_ARGB8565_Premultiplied) << 0x7f007d00u;
    QTest::newRow("semiblue argb32 -> argb8565") << int(QImage::Format_ARGB32) << 0x7f0000ffu
                                                 << int(QImage::Format_ARGB8565_Premultiplied) << 0x7f00007bu;

    QTest::newRow("semired pm -> argb8565") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u
                                            << int(QImage::Format_ARGB8565_Premultiplied) << 0x7f7b0000u;
    QTest::newRow("semigreen pm -> argb8565") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u
                                              << int(QImage::Format_ARGB8565_Premultiplied) << 0x7f007d00u;
    QTest::newRow("semiblue pm -> argb8565") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu
                                             << int(QImage::Format_ARGB8565_Premultiplied) << 0x7f00007bu;
    QTest::newRow("semiwhite pm -> argb8565") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu
                                              << int(QImage::Format_ARGB8565_Premultiplied) << 0x7f7b7d7bu;
    QTest::newRow("semiblack pm -> argb8565") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u
                                              << int(QImage::Format_ARGB8565_Premultiplied) << 0x7f000000u;

    QTest::newRow("red rgb666 -> argb32") << int(QImage::Format_RGB666) << 0xffff0000
                                         << int(QImage::Format_ARGB32) << 0xffff0000;
    QTest::newRow("green rgb666 -> argb32") << int(QImage::Format_RGB666) << 0xff00ff00
                                           << int(QImage::Format_ARGB32) << 0xff00ff00;
    QTest::newRow("blue rgb666 -> argb32") << int(QImage::Format_RGB666) << 0xff0000ff
                                          << int(QImage::Format_ARGB32) << 0xff0000ff;

    QTest::newRow("semired argb32 -> rgb666") << int(QImage::Format_ARGB32) << 0x7fff0000u
                                             << int(QImage::Format_RGB666) << 0xffff0000;
    QTest::newRow("semigreen argb32 -> rgb666") << int(QImage::Format_ARGB32) << 0x7f00ff00u
                                               << int(QImage::Format_RGB666) << 0xff00ff00;
    QTest::newRow("semiblue argb32 -> rgb666") << int(QImage::Format_ARGB32) << 0x7f0000ffu
                                              << int(QImage::Format_RGB666) << 0xff0000ff;

    QTest::newRow("semired pm -> rgb666") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u
                                         << int(QImage::Format_RGB666) << 0xffff0000u;
    QTest::newRow("semigreen pm -> rgb666") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u
                                           << int(QImage::Format_RGB666) << 0xff00ff00u;
    QTest::newRow("semiblue pm -> rgb666") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu
                                          << int(QImage::Format_RGB666) << 0xff0000ffu;
    QTest::newRow("semiwhite pm -> rgb666") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu
                                           << int(QImage::Format_RGB666) << 0xffffffffu;
    QTest::newRow("semiblack pm -> rgb666") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u
                                           << int(QImage::Format_RGB666) << 0xff000000u;

    QTest::newRow("red rgb15 -> argb32") << int(QImage::Format_RGB555) << 0xffff0000
                                         << int(QImage::Format_ARGB32) << 0xffff0000;
    QTest::newRow("green rgb15 -> argb32") << int(QImage::Format_RGB555) << 0xff00ff00
                                           << int(QImage::Format_ARGB32) << 0xff00ff00;
    QTest::newRow("blue rgb15 -> argb32") << int(QImage::Format_RGB555) << 0xff0000ff
                                          << int(QImage::Format_ARGB32) << 0xff0000ff;

    QTest::newRow("semired argb32 -> rgb15") << int(QImage::Format_ARGB32) << 0x7fff0000u
                                             << int(QImage::Format_RGB555) << 0xffff0000;
    QTest::newRow("semigreen argb32 -> rgb15") << int(QImage::Format_ARGB32) << 0x7f00ff00u
                                               << int(QImage::Format_RGB555) << 0xff00ff00;
    QTest::newRow("semiblue argb32 -> rgb15") << int(QImage::Format_ARGB32) << 0x7f0000ffu
                                              << int(QImage::Format_RGB555) << 0xff0000ff;

    QTest::newRow("semired pm -> rgb15") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u
                                         << int(QImage::Format_RGB555) << 0xffff0000u;
    QTest::newRow("semigreen pm -> rgb15") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u
                                           << int(QImage::Format_RGB555) << 0xff00ff00u;
    QTest::newRow("semiblue pm -> rgb15") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu
                                          << int(QImage::Format_RGB555) << 0xff0000ffu;
    QTest::newRow("semiwhite pm -> rgb15") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu
                                           << int(QImage::Format_RGB555) << 0xffffffffu;
    QTest::newRow("semiblack pm -> rgb15") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u
                                           << int(QImage::Format_RGB555) << 0xff000000u;


    QTest::newRow("red rgb32 -> argb8555") << int(QImage::Format_RGB32) << 0xffff0000
                                           << int(QImage::Format_ARGB8555_Premultiplied) << 0xffff0000;
    QTest::newRow("green rgb32 -> argb8555") << int(QImage::Format_RGB32) << 0xff00ff00
                                             << int(QImage::Format_ARGB8555_Premultiplied) << 0xff00ff00;
    QTest::newRow("blue rgb32 -> argb8555") << int(QImage::Format_RGB32) << 0xff0000ff
                                            << int(QImage::Format_ARGB8555_Premultiplied) << 0xff0000ff;

    QTest::newRow("red rgb16 -> argb8555") << int(QImage::Format_RGB16) << 0xffff0000
                                           << int(QImage::Format_ARGB8555_Premultiplied) << 0xffff0000;
    QTest::newRow("green rgb16 -> argb8555") << int(QImage::Format_RGB16) << 0xff00ff00
                                             << int(QImage::Format_ARGB8555_Premultiplied) << 0xff00ff00;
    QTest::newRow("blue rgb16 -> argb8555") << int(QImage::Format_RGB16) << 0xff0000ff
                                            << int(QImage::Format_ARGB8555_Premultiplied) << 0xff0000ff;

    QTest::newRow("red argb8555 -> argb32") << int(QImage::Format_ARGB8555_Premultiplied) << 0xffff0000
                                            << int(QImage::Format_ARGB32) << 0xffff0000;
    QTest::newRow("green argb8555 -> argb32") << int(QImage::Format_ARGB8555_Premultiplied) << 0xff00ff00
                                              << int(QImage::Format_ARGB32) << 0xff00ff00;
    QTest::newRow("blue argb8555 -> argb32") << int(QImage::Format_ARGB8555_Premultiplied) << 0xff0000ff
                                             << int(QImage::Format_ARGB32) << 0xff0000ff;

    QTest::newRow("semired argb32 -> argb8555") << int(QImage::Format_ARGB32) << 0x7fff0000u
                                                << int(QImage::Format_ARGB8555_Premultiplied) << 0x7f7b0000u;
    QTest::newRow("semigreen argb32 -> argb8555") << int(QImage::Format_ARGB32) << 0x7f00ff00u
                                                  << int(QImage::Format_ARGB8555_Premultiplied) << 0x7f007b00u;
    QTest::newRow("semiblue argb32 -> argb8555") << int(QImage::Format_ARGB32) << 0x7f0000ffu
                                                 << int(QImage::Format_ARGB8555_Premultiplied) << 0x7f00007bu;

    QTest::newRow("semired pm -> argb8555") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u
                                            << int(QImage::Format_ARGB8555_Premultiplied) << 0x7f7b0000u;
    QTest::newRow("semigreen pm -> argb8555") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u
                                              << int(QImage::Format_ARGB8555_Premultiplied) << 0x7f007b00u;
    QTest::newRow("semiblue pm -> argb8555") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu
                                             << int(QImage::Format_ARGB8555_Premultiplied) << 0x7f00007bu;
    QTest::newRow("semiwhite pm -> argb8555") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu
                                              << int(QImage::Format_ARGB8555_Premultiplied) << 0x7f7b7b7bu;
    QTest::newRow("semiblack pm -> argb8555") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u
                                              << int(QImage::Format_ARGB8555_Premultiplied) << 0x7f000000u;

    QTest::newRow("red rgb32 -> rgb888") << int(QImage::Format_RGB32) << 0xffff0000
                                         << int(QImage::Format_RGB888) << 0xffff0000;
    QTest::newRow("green rgb32 -> rgb888") << int(QImage::Format_RGB32) << 0xff00ff00
                                           << int(QImage::Format_RGB888) << 0xff00ff00;
    QTest::newRow("blue rgb32 -> rgb888") << int(QImage::Format_RGB32) << 0xff0000ff
                                          << int(QImage::Format_RGB888) << 0xff0000ff;

    QTest::newRow("red rgb16 -> rgb888") << int(QImage::Format_RGB16) << 0xffff0000
                                         << int(QImage::Format_RGB888) << 0xffff0000;
    QTest::newRow("green rgb16 -> rgb888") << int(QImage::Format_RGB16) << 0xff00ff00
                                           << int(QImage::Format_RGB888) << 0xff00ff00;
    QTest::newRow("blue rgb16 -> rgb888") << int(QImage::Format_RGB16) << 0xff0000ff
                                          << int(QImage::Format_RGB888) << 0xff0000ff;

    QTest::newRow("red rgb888 -> argb32") << int(QImage::Format_RGB888) << 0xffff0000
                                          << int(QImage::Format_ARGB32) << 0xffff0000;
    QTest::newRow("green rgb888 -> argb32") << int(QImage::Format_RGB888) << 0xff00ff00
                                            << int(QImage::Format_ARGB32) << 0xff00ff00;
    QTest::newRow("blue rgb888 -> argb32") << int(QImage::Format_RGB888) << 0xff0000ff
                                           << int(QImage::Format_ARGB32) << 0xff0000ff;

    QTest::newRow("red rgb888 -> rgbx8888") << int(QImage::Format_RGB888) << 0xffff0000
                                            << int(QImage::Format_RGBX8888) << 0xffff0000;
    QTest::newRow("green rgb888 -> rgbx8888") << int(QImage::Format_RGB888) << 0xff00ff00
                                              << int(QImage::Format_RGBX8888) << 0xff00ff00;
    QTest::newRow("blue rgb888 -> rgbx8888") << int(QImage::Format_RGB888) << 0xff0000ff
                                             << int(QImage::Format_RGBX8888) << 0xff0000ff;

    QTest::newRow("semired argb32 -> rgb888") << int(QImage::Format_ARGB32) << 0x7fff0000u
                                              << int(QImage::Format_RGB888) << 0xffff0000;
    QTest::newRow("semigreen argb32 -> rgb888") << int(QImage::Format_ARGB32) << 0x7f00ff00u
                                                << int(QImage::Format_RGB888) << 0xff00ff00;
    QTest::newRow("semiblue argb32 -> rgb888") << int(QImage::Format_ARGB32) << 0x7f0000ffu
                                               << int(QImage::Format_RGB888) << 0xff0000ff;

    QTest::newRow("semired pm -> rgb888") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u
                                          << int(QImage::Format_RGB888) << 0xffff0000u;
    QTest::newRow("semigreen pm -> rgb888") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u
                                            << int(QImage::Format_RGB888) << 0xff00ff00u;
    QTest::newRow("semiblue pm -> rgb888") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu
                                           << int(QImage::Format_RGB888) << 0xff0000ffu;
    QTest::newRow("semiwhite pm -> rgb888") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu
                                            << int(QImage::Format_RGB888) << 0xffffffffu;
    QTest::newRow("semiblack pm -> rgb888") << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u
                                            << int(QImage::Format_RGB888) << 0xff000000u;

    QTest::newRow("red rgba8888 -> argb32") << int(QImage::Format_RGBA8888) << 0xffff0000
                                      << int(QImage::Format_ARGB32) << 0xffff0000;
    QTest::newRow("green rgba8888 -> argb32") << int(QImage::Format_RGBA8888) << 0xff00ff00
                                        << int(QImage::Format_ARGB32) << 0xff00ff00;
    QTest::newRow("blue rgba8888 -> argb32") << int(QImage::Format_RGBA8888) << 0xff0000ff
                                       << int(QImage::Format_ARGB32) << 0xff0000ff;

    QTest::newRow("semired rgba8888 -> argb pm") << int(QImage::Format_RGBA8888) << 0x7fff0000u
                                       << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f0000u;
    QTest::newRow("semigreen rgba8888 -> argb pm") << int(QImage::Format_RGBA8888) << 0x7f00ff00u
                                         << int(QImage::Format_ARGB32_Premultiplied) << 0x7f007f00u;
    QTest::newRow("semiblue rgba8888 -> argb pm") << int(QImage::Format_RGBA8888) << 0x7f0000ffu
                                        << int(QImage::Format_ARGB32_Premultiplied) << 0x7f00007fu;
    QTest::newRow("semiwhite rgba8888 -> argb pm") << int(QImage::Format_RGBA8888) << 0x7fffffffu
                                         << int(QImage::Format_ARGB32_Premultiplied) << 0x7f7f7f7fu;
    QTest::newRow("semiblack rgba8888 -> argb pm") << int(QImage::Format_RGBA8888) << 0x7f000000u
                                         << int(QImage::Format_ARGB32_Premultiplied) << 0x7f000000u;

    QTest::newRow("red rgb30 -> argb32") << int(QImage::Format_RGB30) << 0xffff0000
                                         << int(QImage::Format_ARGB32) << 0xffff0000;
    QTest::newRow("green rgb30 -> argb32") << int(QImage::Format_RGB30) << 0xff00ff00
                                           << int(QImage::Format_ARGB32) << 0xff00ff00;
    QTest::newRow("blue rgb30 -> argb32") << int(QImage::Format_RGB30) << 0xff0000ff
                                          << int(QImage::Format_ARGB32) << 0xff0000ff;

    QTest::newRow("white gray8 -> argb pm") << int(QImage::Format_Grayscale8) << 0xfffffeffu
                                            << int(QImage::Format_ARGB32_Premultiplied) << 0xfffefefeu;
    QTest::newRow("gray gray8 -> argb pm") << int(QImage::Format_Grayscale8) << 0xff565557u
                                            << int(QImage::Format_ARGB32_Premultiplied) << 0xff555555u;
    QTest::newRow("black gray8 -> argb pm") << int(QImage::Format_Grayscale8) << 0xff000100u
                                            << int(QImage::Format_ARGB32_Premultiplied) << 0xff000000u;
}


void tst_QImage::convertToFormat()
{
    QFETCH(int, inFormat);
    QFETCH(uint, inPixel);
    QFETCH(int, resFormat);
    QFETCH(uint, resPixel);

    QImage src(32, 32, QImage::Format(inFormat));

    if (inFormat == QImage::Format_Mono) {
        src.setColor(0, qRgba(0,0,0,0xff));
        src.setColor(1, qRgba(255,255,255,0xff));
    }

    for (int y=0; y<src.height(); ++y)
        for (int x=0; x<src.width(); ++x)
            src.setPixel(x, y, inPixel);

    QImage result = src.convertToFormat(QImage::Format(resFormat));

    QCOMPARE(src.width(), result.width());
    QCOMPARE(src.height(), result.height());

    bool same = true;
    for (int y=0; y<result.height(); ++y) {
        for (int x=0; x<result.width(); ++x) {
            QRgb pixel = result.pixel(x, y);
            same &= (pixel == resPixel);
            if (!same) {
                printf("expect=%08x, result=%08x\n", resPixel, pixel);
                y = 100000;
                break;
            }

        }
    }
    QVERIFY(same);

    // repeat tests converting from an image with nonstandard stride

    int dp = (src.depth() < 8 || result.depth() < 8) ? 8 : 1;
    QImage src2(src.bits() + (dp*src.depth())/8,
                src.width() - dp*2,
                src.height() - 1, src.bytesPerLine(),
                src.format());
    if (src.depth() < 8)
        src2.setColorTable(src.colorTable());

    const QImage result2 = src2.convertToFormat(QImage::Format(resFormat));

    QCOMPARE(src2.width(), result2.width());
    QCOMPARE(src2.height(), result2.height());

    QImage expected2(result.bits() + (dp*result.depth())/8,
                     result.width() - dp*2,
                     result.height() - 1, result.bytesPerLine(),
                     result.format());
    if (result.depth() < 8)
        expected2.setColorTable(result.colorTable());

    result2.save("result2.xpm", "XPM");
    expected2.save("expected2.xpm", "XPM");

    QCOMPARE(result2, expected2);
    QFile::remove(QLatin1String("result2.xpm"));
    QFile::remove(QLatin1String("expected2.xpm"));
}

void tst_QImage::convertToFormatWithColorTable()
{
    QVector<QRgb> colors(2);
    colors[0] = 0xFF000000;
    colors[1] = 0xFFFFFFFF;
    for (int format = QImage::Format_RGB32; format < QImage::Format_Alpha8; ++format) {
        QImage fromImage(10, 10, (QImage::Format)format);
        QImage bitmap = fromImage.convertToFormat(QImage::Format_Mono, colors);
        QVERIFY(!bitmap.isNull());
    }
}

void tst_QImage::convertToFormatRgb888ToRGB32()
{
    // 545 so width % 4 != 0. This ensure there is padding at the end of the scanlines
    const int height = 545;
    const int width = 545;
    QImage source(width, height, QImage::Format_RGB888);
    for (int y = 0; y < height; ++y) {
        uchar *srcPixels = source.scanLine(y);
        for (int x = 0; x < width * 3; ++x)
            srcPixels[x] = x;
    }

    QImage rgb32Image = source.convertToFormat(QImage::Format_RGB888);
    QCOMPARE(rgb32Image.format(), QImage::Format_RGB888);
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y)
            QCOMPARE(rgb32Image.pixel(x, y), source.pixel(x, y));
    }
}

void tst_QImage::createAlphaMask_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");
    QTest::addColumn<int>("alpha1");
    QTest::addColumn<int>("alpha2");

    int alphas[] = { 0, 127, 255 };

    for (unsigned a1 = 0; a1 < sizeof(alphas) / sizeof(int); ++a1) {
        const QByteArray a1B = QByteArray::number(alphas[a1]);
        for (unsigned a2 = 0; a2 < sizeof(alphas) / sizeof(int); ++a2) {
            if (a1 == a2)
                continue;
            const QByteArray a2B = QByteArray::number(alphas[a2]);
            for (int x=10; x<18; x+=3) {
                const QByteArray xB = QByteArray::number(x);
                for (int y=100; y<108; y+=3) {
                    const QByteArray testName = "x=" + xB + ", y=" + QByteArray::number(y)
                        + ", a1=" + a1B + ", a2=" + a2B;
                    QTest::newRow(testName.constData()) << x << y << alphas[a1] << alphas[a2];
                }
            }
        }
    }
}

void tst_QImage::createAlphaMask()
{
    QFETCH(int, x);
    QFETCH(int, y);
    QFETCH(int, alpha1);
    QFETCH(int, alpha2);

    QSize size(255, 255);
    int pixelsInLines = size.width() + size.height() - 1;
    int pixelsOutofLines = size.width() * size.height() - pixelsInLines;

    // Generate an white image with two lines, horizontal at y and vertical at x.
    // Lines have alpha of alpha2, rest has alpha of alpha1
    QImage image(size, QImage::Format_ARGB32);
    for (int cy=0; cy<image.height(); ++cy) {
        for (int cx=0; cx<image.width(); ++cx) {
            int alpha = (y == cy || x == cx) ? alpha2 : alpha1;
            image.setPixel(cx, cy, qRgba(255, 255, 255, alpha));
        }
    }

    QImage mask = image.createAlphaMask(Qt::OrderedAlphaDither);

    // Sanity check...
    QCOMPARE(mask.width(), image.width());
    QCOMPARE(mask.height(), image.height());

    // Sum up the number of pixels set for both lines and other area
    int sumAlpha1 = 0;
    int sumAlpha2 = 0;
    for (int cy=0; cy<image.height(); ++cy) {
        for (int cx=0; cx<image.width(); ++cx) {
            int *alpha = (y == cy || x == cx) ? &sumAlpha2 : &sumAlpha1;
            *alpha += mask.pixelIndex(cx, cy);
        }
    }

    // Compare the set bits to whats expected for that alpha.
    const int threshold = 5;
    QVERIFY(qAbs(sumAlpha1 * 255 / pixelsOutofLines - alpha1) < threshold);
    QVERIFY(qAbs(sumAlpha2 * 255 / pixelsInLines - alpha2) < threshold);
}

void tst_QImage::dotsPerMeterZero()
{
    QImage img(100, 100, QImage::Format_RGB32);
    QVERIFY(!img.isNull());

    int defaultDpmX = img.dotsPerMeterX();
    int defaultDpmY = img.dotsPerMeterY();
    QVERIFY(defaultDpmX != 0);
    QVERIFY(defaultDpmY != 0);

    img.setDotsPerMeterX(0);
    img.setDotsPerMeterY(0);

    QCOMPARE(img.dotsPerMeterX(), defaultDpmX);
    QCOMPARE(img.dotsPerMeterY(), defaultDpmY);

}

// verify that setting dotsPerMeter has an effect on the dpi.
void tst_QImage::dotsPerMeterAndDpi()
{
    QImage img(100, 100, QImage::Format_RGB32);
    QVERIFY(!img.isNull());

    QPoint defaultLogicalDpi(img.logicalDpiX(), img.logicalDpiY());
    QPoint defaultPhysicalDpi(img.physicalDpiX(), img.physicalDpiY());

    img.setDotsPerMeterX(100);  // set x
    QCOMPARE(img.logicalDpiY(), defaultLogicalDpi.y()); // no effect on y
    QCOMPARE(img.physicalDpiY(), defaultPhysicalDpi.y());
    QVERIFY(img.logicalDpiX() != defaultLogicalDpi.x()); // x changed
    QVERIFY(img.physicalDpiX() != defaultPhysicalDpi.x());

    img.setDotsPerMeterY(200);  // set y
    QVERIFY(img.logicalDpiY() != defaultLogicalDpi.y()); // y changed
    QVERIFY(img.physicalDpiY() != defaultPhysicalDpi.y());
}

void tst_QImage::rotate_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<int>("degrees");

    QVector<int> degrees;
    degrees << 0 << 90 << 180 << 270;

    foreach (int d, degrees) {
        const QString dB = QString::number(d);
        for (int i = QImage::Format_Indexed8; i < QImage::NImageFormats; i++) {
            QImage::Format format = static_cast<QImage::Format>(i);
            QTest::newRow(qPrintable(dB + " " + formatToString(format))) << format << d;
        }
    }
}

void tst_QImage::rotate()
{
    QFETCH(QImage::Format, format);
    QFETCH(int, degrees);

    // test if rotate90 is lossless
    int w = 54;
    int h = 13;
    QImage original(w, h, format);
    original.fill(qRgb(255,255,255));

    if (format == QImage::Format_Indexed8) {
        original.setColorCount(256);
        for (int i = 0; i < 255; ++i)
            original.setColor(i, qRgb(0, i, i));
    }

    if (original.colorTable().isEmpty()) {
        for (int x = 0; x < w; ++x) {
            original.setPixel(x,0, qRgb(x,0,128));
            original.setPixel(x,h - 1, qRgb(0,255 - x,128));
        }
        for (int y = 0; y < h; ++y) {
            original.setPixel(0, y, qRgb(y,0,255));
            original.setPixel(w - 1, y, qRgb(0,255 - y,255));
        }
    } else {
        const int n = original.colorTable().size();
        for (int x = 0; x < w; ++x) {
            original.setPixel(x, 0, x % n);
            original.setPixel(x, h - 1, n - (x % n) - 1);
        }
        for (int y = 0; y < h; ++y) {
            original.setPixel(0, y, y % n);
            original.setPixel(w - 1, y, n - (y % n) - 1);
        }
    }

    // original.save("rotated90_original.png", "png");

    // Initialize the matrix manually (do not use rotate) to avoid rounding errors
    QMatrix matRotate90;
    matRotate90.rotate(degrees);
    QImage dest = original;
    // And rotate it 4 times, then the image should be identical to the original
    for (int i = 0; i < 4 ; ++i) {
        dest = dest.transformed(matRotate90);
    }

    // Make sure they are similar in format before we compare them.
    dest = dest.convertToFormat(format);

    // dest.save("rotated90_result.png","png");
    QCOMPARE(original, dest);

    // Test with QMatrix::rotate 90 also, since we trust that now
    matRotate90.rotate(degrees);
    dest = original;
    // And rotate it 4 times, then the image should be identical to the original
    for (int i = 0; i < 4 ; ++i) {
        dest = dest.transformed(matRotate90);
    }

    // Make sure they are similar in format before we compare them.
    dest = dest.convertToFormat(format);

    QCOMPARE(original, dest);
}

void tst_QImage::copy()
{
    // Task 99250
    {
        QImage img(16,16,QImage::Format_ARGB32);
        img.copy(QRect(1000,1,1,1));
    }
}

void tst_QImage::load()
{
    const QString filePath = m_prefix + QLatin1String("image.jpg");

    QImage dest(filePath);
    QVERIFY(!dest.isNull());
    QVERIFY(!dest.load("image_that_does_not_exist.png"));
    QVERIFY(dest.isNull());
    QVERIFY(dest.load(filePath));
    QVERIFY(!dest.isNull());
}

void tst_QImage::loadFromData()
{
    const QString filePath = m_prefix + QLatin1String("image.jpg");

    QImage original(filePath);
    QVERIFY(!original.isNull());

    QByteArray ba;
    {
        QBuffer buf(&ba);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QVERIFY(original.save(&buf, "BMP"));
    }
    QVERIFY(!ba.isEmpty());

    QImage dest;
    QVERIFY(dest.loadFromData(ba, "BMP"));
    QVERIFY(!dest.isNull());

    QCOMPARE(original, dest);

    QVERIFY(!dest.loadFromData(QByteArray()));
    QVERIFY(dest.isNull());
}

#if !defined(QT_NO_DATASTREAM)
void tst_QImage::loadFromDataStream()
{
    const QString filePath = m_prefix + QLatin1String("image.jpg");

    QImage original(filePath);
    QVERIFY(!original.isNull());

    QByteArray ba;
    {
        QDataStream s(&ba, QIODevice::WriteOnly);
        s << original;
    }
    QVERIFY(!ba.isEmpty());

    QImage dest;
    {
        QDataStream s(&ba, QIODevice::ReadOnly);
        s >> dest;
    }
    QVERIFY(!dest.isNull());

    QCOMPARE(original, dest);

    {
        ba.clear();
        QDataStream s(&ba, QIODevice::ReadOnly);
        s >> dest;
    }
    QVERIFY(dest.isNull());
}
#endif // QT_NO_DATASTREAM

void tst_QImage::setPixel_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<uint>("value");
    QTest::addColumn<uint>("expected");

    QTest::newRow("ARGB32 red") << int(QImage::Format_ARGB32)
                                << 0xffff0000 << 0xffff0000;
    QTest::newRow("ARGB32 green") << int(QImage::Format_ARGB32)
                                  << 0xff00ff00 << 0xff00ff00;
    QTest::newRow("ARGB32 blue") << int(QImage::Format_ARGB32)
                                 << 0xff0000ff << 0xff0000ff;
    QTest::newRow("RGB16 red") << int(QImage::Format_RGB16)
                               << 0xffff0000 << 0xf800u;
    QTest::newRow("RGB16 green") << int(QImage::Format_RGB16)
                                 << 0xff00ff00 << 0x07e0u;
    QTest::newRow("RGB16 blue") << int(QImage::Format_RGB16)
                                << 0xff0000ff << 0x001fu;
    QTest::newRow("ARGB8565_Premultiplied red") << int(QImage::Format_ARGB8565_Premultiplied)
                                  << 0xffff0000 << 0xf800ffu;
    QTest::newRow("ARGB8565_Premultiplied green") << int(QImage::Format_ARGB8565_Premultiplied)
                                    << 0xff00ff00 << 0x07e0ffu;
    QTest::newRow("ARGB8565_Premultiplied blue") << int(QImage::Format_ARGB8565_Premultiplied)
                                   << 0xff0000ff << 0x001fffu;
    QTest::newRow("RGB666 red") << int(QImage::Format_RGB666)
                                << 0xffff0000 << 0x03f000u;
    QTest::newRow("RGB666 green") << int(QImage::Format_RGB666)
                                  << 0xff00ff00 << 0x000fc0u;
    QTest::newRow("RGB666 blue") << int(QImage::Format_RGB666)
                                 << 0xff0000ff << 0x00003fu;
    QTest::newRow("RGB555 red") << int(QImage::Format_RGB555)
                                << 0xffff0000 << 0x7c00u;
    QTest::newRow("RGB555 green") << int(QImage::Format_RGB555)
                                  << 0xff00ff00 << 0x03e0u;
    QTest::newRow("RGB555 blue") << int(QImage::Format_RGB555)
                                 << 0xff0000ff << 0x001fu;
    QTest::newRow("ARGB8555_Premultiplied red") << int(QImage::Format_ARGB8555_Premultiplied)
                                  << 0xffff0000 << 0x7c00ffu;
    QTest::newRow("ARGB8555_Premultiplied green") << int(QImage::Format_ARGB8555_Premultiplied)
                                    << 0xff00ff00 << 0x03e0ffu;
    QTest::newRow("ARGB8555_Premultiplied blue") << int(QImage::Format_ARGB8555_Premultiplied)
                                   << 0xff0000ff << 0x001fffu;
    QTest::newRow("RGB888 red") << int(QImage::Format_RGB888)
                                << 0xffff0000 << 0xff0000u;
    QTest::newRow("RGB888 green") << int(QImage::Format_RGB888)
                                  << 0xff00ff00 << 0x00ff00u;
    QTest::newRow("RGB888 blue") << int(QImage::Format_RGB888)
                                 << 0xff0000ff << 0x0000ffu;
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    QTest::newRow("RGBA8888 red") << int(QImage::Format_RGBA8888)
                                << 0xffff0000u << 0xff0000ffu;
    QTest::newRow("RGBA8888 green") << int(QImage::Format_RGBA8888)
                                  << 0xff00ff00u << 0x00ff00ffu;
    QTest::newRow("RGBA8888 blue") << int(QImage::Format_RGBA8888)
                                 << 0xff0000ffu << 0x0000ffffu;
#else
    QTest::newRow("RGBA8888 red") << int(QImage::Format_RGBA8888)
                                << 0xffff0000u << 0xff0000ffu;
    QTest::newRow("RGBA8888 green") << int(QImage::Format_RGBA8888)
                                  << 0xff00ff00u << 0xff00ff00u;
    QTest::newRow("RGBA8888 blue") << int(QImage::Format_RGBA8888)
                                 << 0xff0000ffu << 0xffff0000u;
#endif
    QTest::newRow("A2BGR30_Premultiplied red") << int(QImage::Format_A2BGR30_Premultiplied)
                                 << 0xffff0000u << 0xc00003ffu;
    QTest::newRow("A2BGR30_Premultiplied green") << int(QImage::Format_A2BGR30_Premultiplied)
                                   << 0xff00ff00u << 0xc00ffc00u;
    QTest::newRow("A2BGR30_Premultiplied blue") << int(QImage::Format_A2BGR30_Premultiplied)
                                 << 0xff0000ffu << 0xfff00000u;
    QTest::newRow("RGB30 red") << int(QImage::Format_RGB30)
                               << 0xffff0000u << 0xfff00000u;
    QTest::newRow("RGB30 green") << int(QImage::Format_RGB30)
                                 << 0xff00ff00u << 0xc00ffc00u;
    QTest::newRow("RGB30 blue") << int(QImage::Format_RGB30)
                                << 0xff0000ffu << 0xc00003ffu;
}

void tst_QImage::setPixel()
{
    QFETCH(int, format);
    QFETCH(uint, value);
    QFETCH(uint, expected);

    const int w = 13;
    const int h = 15;

    QImage img(w, h, QImage::Format(format));

    // fill image
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            img.setPixel(x, y, value);

    // check pixel values
    switch (format) {
    case int(QImage::Format_RGB32):
    case int(QImage::Format_ARGB32):
    case int(QImage::Format_ARGB32_Premultiplied):
    case int(QImage::Format_RGBX8888):
    case int(QImage::Format_RGBA8888):
    case int(QImage::Format_RGBA8888_Premultiplied):
    case int(QImage::Format_A2BGR30_Premultiplied):
    case int(QImage::Format_RGB30):
    {
        for (int y = 0; y < h; ++y) {
            const quint32 *row = (const quint32*)(img.scanLine(y));
            for (int x = 0; x < w; ++x) {
                quint32 result = row[x];
                if (result != expected)
                    printf("[x,y]: %d,%d, expected=%08x, result=%08x\n",
                           x, y, expected, result);
                QCOMPARE(uint(result), expected);
            }
        }
        break;
    }
    case int(QImage::Format_RGB555):
    case int(QImage::Format_RGB16):
    {
        for (int y = 0; y < h; ++y) {
            const quint16 *row = (const quint16*)(img.scanLine(y));
            for (int x = 0; x < w; ++x) {
                quint16 result = row[x];
                if (result != expected)
                    printf("[x,y]: %d,%d, expected=%04x, result=%04x\n",
                           x, y, expected, result);
                QCOMPARE(uint(result), expected);
            }
        }
        break;
    }
    case int(QImage::Format_RGB666):
    case int(QImage::Format_ARGB8565_Premultiplied):
    case int(QImage::Format_ARGB8555_Premultiplied):
    case int(QImage::Format_RGB888):
    {
        for (int y = 0; y < h; ++y) {
            const quint24 *row = (const quint24*)(img.scanLine(y));
            for (int x = 0; x < w; ++x) {
                quint32 result = row[x];
                if (result != expected)
                    printf("[x,y]: %d,%d, expected=%04x, result=%04x\n",
                           x, y, expected, result);
                QCOMPARE(result, expected);
            }
        }
        break;
    }
    default:
        qFatal("Test not implemented for format %d", format);
    }
}

void tst_QImage::convertToFormatPreserveDotsPrMeter()
{
    QImage img(100, 100, QImage::Format_ARGB32_Premultiplied);

    int dpmx = 123;
    int dpmy = 234;
    img.setDotsPerMeterX(dpmx);
    img.setDotsPerMeterY(dpmy);
    img.fill(0x12345678);

    img = img.convertToFormat(QImage::Format_RGB32);

    QCOMPARE(img.dotsPerMeterX(), dpmx);
    QCOMPARE(img.dotsPerMeterY(), dpmy);
}

void tst_QImage::convertToFormatPreserveText()
{
    QImage img(100, 100, QImage::Format_ARGB32_Premultiplied);

    img.setText("foo", "bar");
    img.setText("foo2", "bar2");
    img.fill(0x12345678);

    QStringList listResult;
    listResult << "foo" << "foo2";
    QString result = "foo: bar\n\nfoo2: bar2";

    QImage imgResult1 = img.convertToFormat(QImage::Format_RGB32);
    QCOMPARE(imgResult1.text(), result);
    QCOMPARE(imgResult1.textKeys(), listResult);

    QVector<QRgb> colorTable(4);
    for (int i = 0; i < 4; ++i)
        colorTable[i] = QRgb(42);
    QImage imgResult2 = img.convertToFormat(QImage::Format_MonoLSB,
                                            colorTable);
    QCOMPARE(imgResult2.text(), result);
    QCOMPARE(imgResult2.textKeys(), listResult);
}

void tst_QImage::defaultColorTable_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<int>("createdDataCount");
    QTest::addColumn<int>("externalDataCount");

    // For historical reasons, internally created mono images get a default colormap.
    // Externally created and Indexed8 images do not.
    QTest::newRow("Mono") << QImage::Format_Mono << 2 << 0;
    QTest::newRow("MonoLSB") << QImage::Format_MonoLSB << 2 << 0;
    QTest::newRow("Indexed8") << QImage::Format_Indexed8 << 0 << 0;
    QTest::newRow("ARGB32_PM") << QImage::Format_A2BGR30_Premultiplied << 0 << 0;
}

void tst_QImage::defaultColorTable()
{
    QFETCH(QImage::Format, format);
    QFETCH(int, createdDataCount);
    QFETCH(int, externalDataCount);

    QImage img1(1, 1, format);
    QCOMPARE(img1.colorCount(), createdDataCount);
    QCOMPARE(img1.colorTable().size(), createdDataCount);

    quint32 buf;
    QImage img2(reinterpret_cast<uchar *>(&buf), 1, 1, format);
    QCOMPARE(img2.colorCount(), externalDataCount);

    QImage nullImg(0, 0, format);
    QCOMPARE(nullImg.colorCount(), 0);
}

void tst_QImage::setColorCount()
{
    QImage img(0, 0, QImage::Format_Indexed8);
    QTest::ignoreMessage(QtWarningMsg, "QImage::setColorCount: null image");
    img.setColorCount(256);
    QCOMPARE(img.colorCount(), 0);
}

void tst_QImage::setColor()
{
    QImage img(0, 0, QImage::Format_Indexed8);
    img.setColor(0, qRgba(18, 219, 108, 128));
    QCOMPARE(img.colorCount(), 0);

    QImage img2(1, 1, QImage::Format_Indexed8);
    img2.setColor(0, qRgba(18, 219, 108, 128));
    QCOMPARE(img2.colorCount(), 1);
}

/* Just some sanity checking that we don't draw outside the buffer of
 * the image. Hopefully this will create crashes or at least some
 * random test fails when broken.
 */
void tst_QImage::rasterClipping()
{
    QImage image(10, 10, QImage::Format_RGB32);
    image.fill(0xffffffff);

    QPainter p(&image);

    p.drawLine(-1000, 5, 5, 5);
    p.drawLine(-1000, 5, 1000, 5);
    p.drawLine(5, 5, 1000, 5);

    p.drawLine(5, -1000, 5, 5);
    p.drawLine(5, -1000, 5, 1000);
    p.drawLine(5, 5, 5, 1000);

    p.setBrush(Qt::red);

    p.drawEllipse(3, 3, 4, 4);
    p.drawEllipse(-100, -100, 210, 210);

    p.drawEllipse(-1000, 0, 2010, 2010);
    p.drawEllipse(0, -1000, 2010, 2010);
    p.drawEllipse(-2010, -1000, 2010, 2010);
    p.drawEllipse(-1000, -2010, 2010, 2010);
    QVERIFY(true);
}

// Tests the new QPoint overloads in QImage in Qt 4.2
void tst_QImage::pointOverloads()
{
    QImage image(100, 100, QImage::Format_RGB32);
    image.fill(0xff00ff00);

    // IsValid
    QVERIFY(image.valid(QPoint(0, 0)));
    QVERIFY(image.valid(QPoint(99, 0)));
    QVERIFY(image.valid(QPoint(0, 99)));
    QVERIFY(image.valid(QPoint(99, 99)));

    QVERIFY(!image.valid(QPoint(50, -1))); // outside on the top
    QVERIFY(!image.valid(QPoint(50, 100))); // outside on the bottom
    QVERIFY(!image.valid(QPoint(-1, 50))); // outside on the left
    QVERIFY(!image.valid(QPoint(100, 50))); // outside on the right

    // Test the pixel setter
    image.setPixel(QPoint(10, 10), 0xff0000ff);
    QCOMPARE(image.pixel(10, 10), 0xff0000ff);

    // pixel getter
    QCOMPARE(image.pixel(QPoint(10, 10)), 0xff0000ff);

    // pixelIndex()
    QImage indexed = image.convertToFormat(QImage::Format_Indexed8);
    QCOMPARE(indexed.pixelIndex(10, 10), indexed.pixelIndex(QPoint(10, 10)));
}

void tst_QImage::destructor()
{
    QPolygon poly(6);
    poly.setPoint(0,-1455, 1435);

    QImage image(100, 100, QImage::Format_RGB32);
    QPainter ptPix(&image);
    ptPix.setPen(Qt::black);
    ptPix.setBrush(Qt::black);
    ptPix.drawPolygon(poly, Qt::WindingFill);
    ptPix.end();

}


/* XPM */
static const char *monoPixmap[] = {
/* width height ncolors chars_per_pixel */
"4 4 2 1",
"x c #000000",
". c #ffffff",
/* pixels */
"xxxx",
"x..x",
"x..x",
"xxxx"
};


#ifndef QT_NO_IMAGE_HEURISTIC_MASK
void tst_QImage::createHeuristicMask()
{
    QImage img(monoPixmap);
    img = img.convertToFormat(QImage::Format_MonoLSB);
    QImage mask = img.createHeuristicMask();
    QImage newMask = mask.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    // line 2
    QVERIFY(newMask.pixel(0,1) != newMask.pixel(1,1));
    QCOMPARE(newMask.pixel(1,1), newMask.pixel(2,1));
    QVERIFY(newMask.pixel(2,1) != newMask.pixel(3,1));

    // line 3
    QVERIFY(newMask.pixel(0,2) != newMask.pixel(1,2));
    QCOMPARE(newMask.pixel(1,2), newMask.pixel(2,2));
    QVERIFY(newMask.pixel(2,2) != newMask.pixel(3,2));
}
#endif

void tst_QImage::cacheKey()
{
    QImage image1(2, 2, QImage::Format_RGB32);
    qint64 image1_key = image1.cacheKey();
    QImage image2 = image1;

    QCOMPARE(image2.cacheKey(), image1.cacheKey());
    image2.detach();
    QVERIFY(image2.cacheKey() != image1.cacheKey());
    QCOMPARE(image1.cacheKey(), image1_key);
}

void tst_QImage::smoothScale()
{
    unsigned int data[2] = { qRgba(0, 0, 0, 0), qRgba(128, 128, 128, 128) };

    QImage imgX((unsigned char *)data, 2, 1, QImage::Format_ARGB32_Premultiplied);
    QImage imgY((unsigned char *)data, 1, 2, QImage::Format_ARGB32_Premultiplied);

    QImage scaledX = imgX.scaled(QSize(4, 1), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage scaledY = imgY.scaled(QSize(1, 4), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    uint *scaled[2] = {
        (unsigned int *)scaledX.bits(),
        (unsigned int *)scaledY.bits()
    };

    int expected[4] = { 0, 32, 96, 128 };
    for (int image = 0; image < 2; ++image) {
        for (int index = 0; index < 4; ++index) {
            for (int component = 0; component < 4; ++component) {
                int pixel = scaled[image][index];
                int val = (pixel >> (component * 8)) & 0xff;

                QCOMPARE(val, expected[index]);
            }
        }
    }
}

// test area sampling
void tst_QImage::smoothScale2_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<int>("size");

    int sizes[] = { 2, 3, 4, 6, 7, 8, 10, 16, 20, 32, 40, 64, 100, 101, 128, 0 };
    QImage::Format formats[] = { QImage::Format_RGB32, QImage::Format_ARGB32_Premultiplied, QImage::Format_RGBX64, QImage::Format_RGBA64_Premultiplied, QImage::Format_Invalid };
    for (int j = 0; formats[j] != QImage::Format_Invalid; ++j) {
        QString formatstr = formatToString(formats[j]);
        for (int i = 0; sizes[i] != 0; ++i) {
            const QByteArray sizeB = QByteArray::number(sizes[i]);
            QTest::newRow(QString("%1 %2x%2").arg(formatstr).arg(sizes[i]).toUtf8()) << formats[j] << sizes[i];
        }
    }
}

void tst_QImage::smoothScale2()
{
    QFETCH(QImage::Format, format);
    QFETCH(int, size);

    bool opaque = (format == QImage::Format_RGB32 || format == QImage::Format_RGBX64);

    QRgb expected = opaque ? qRgb(63, 127, 255) : qRgba(31, 63, 127, 127);

    QImage img(size, size, format);
    img.fill(expected);

    // scale x down, y down
    QImage scaled = img.scaled(QSize(1, 1), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QRgb pixel = scaled.pixel(0, 0);
    QCOMPARE(qAlpha(pixel), qAlpha(expected));
    QCOMPARE(qRed(pixel), qRed(expected));
    QCOMPARE(qGreen(pixel), qGreen(expected));
    QCOMPARE(qBlue(pixel), qBlue(expected));

    // scale x down, y up
    scaled = img.scaled(QSize(1, size * 2), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    for (int y = 0; y < scaled.height(); ++y) {
        pixel = scaled.pixel(0, y);
        QCOMPARE(qAlpha(pixel), qAlpha(expected));
        QCOMPARE(qRed(pixel), qRed(expected));
        QCOMPARE(qGreen(pixel), qGreen(expected));
        QCOMPARE(qBlue(pixel), qBlue(expected));
    }

    // scale x up, y down
    scaled = img.scaled(QSize(size * 2, 1), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    for (int x = 0; x < scaled.width(); ++x) {
        pixel = scaled.pixel(x, 0);
        QCOMPARE(qAlpha(pixel), qAlpha(expected));
        QCOMPARE(qRed(pixel), qRed(expected));
        QCOMPARE(qGreen(pixel), qGreen(expected));
        QCOMPARE(qBlue(pixel), qBlue(expected));
    }

    // scale x up
    scaled = img.scaled(QSize(size, size * 2), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    for (int y = 0; y < scaled.height(); ++y) {
        for (int x = 0; x < scaled.width(); ++x) {
            pixel = scaled.pixel(x, y);
            QCOMPARE(qAlpha(pixel), qAlpha(expected));
            QCOMPARE(qRed(pixel), qRed(expected));
            QCOMPARE(qGreen(pixel), qGreen(expected));
            QCOMPARE(qBlue(pixel), qBlue(expected));
        }
    }

    // scale y up
    scaled = img.scaled(QSize(size * 2, size), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    for (int y = 0; y < scaled.height(); ++y) {
        for (int x = 0; x < scaled.width(); ++x) {
            pixel = scaled.pixel(x, y);
            QCOMPARE(qAlpha(pixel), qAlpha(expected));
            QCOMPARE(qRed(pixel), qRed(expected));
            QCOMPARE(qGreen(pixel), qGreen(expected));
            QCOMPARE(qBlue(pixel), qBlue(expected));
        }
    }

    // scale x up, y up
    scaled = img.scaled(QSize(size * 2, size * 2), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    for (int y = 0; y < scaled.height(); ++y) {
        for (int x = 0; x < scaled.width(); ++x) {
            pixel = scaled.pixel(x, y);
            QCOMPARE(qAlpha(pixel), qAlpha(expected));
            QCOMPARE(qRed(pixel), qRed(expected));
            QCOMPARE(qGreen(pixel), qGreen(expected));
            QCOMPARE(qBlue(pixel), qBlue(expected));
        }
    }
}

static inline int rand8()
{
    return QRandomGenerator::global()->bounded(256);
}

void tst_QImage::smoothScale3_data()
{
    QTest::addColumn<QImage>("img");
    QTest::addColumn<qreal>("scale_x");
    QTest::addColumn<qreal>("scale_y");

    QImage img(128, 128, QImage::Format_RGB32);
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const int red = rand8();
            const int green = rand8();
            const int blue = rand8();
            const int alpha = 255;

            img.setPixel(x, y, qRgba(red, green, blue, alpha));
        }
    }

    QTest::newRow("(0.5, 0.5)") << img << qreal(0.5) << qreal(0.5);
    QTest::newRow("(0.5, 1.0)") << img << qreal(0.5) << qreal(1.0);
    QTest::newRow("(1.0, 0.5)") << img << qreal(1.0) << qreal(0.5);
    QTest::newRow("(0.5, 2.0)") << img << qreal(0.5) << qreal(2.0);
    QTest::newRow("(1.0, 2.0)") << img << qreal(1.0) << qreal(2.0);
    QTest::newRow("(2.0, 0.5)") << img << qreal(2.0) << qreal(0.5);
    QTest::newRow("(2.0, 1.0)") << img << qreal(2.0) << qreal(1.0);
    QTest::newRow("(2.0, 2.0)") << img << qreal(2) << qreal(2);
}
// compares img.scale against the bilinear filtering used by QPainter
void tst_QImage::smoothScale3()
{
    QFETCH(QImage, img);
    QFETCH(qreal, scale_x);
    QFETCH(qreal, scale_y);

    QImage a = img.scaled(img.width() * scale_x, img.height() * scale_y, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage b(a.size(), a.format());
    b.fill(0x0);

    QPainter p(&b);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.scale(scale_x, scale_y);
    p.drawImage(0, 0, img);
    p.end();
    int err = 0;

    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            QRgb ca = a.pixel(x, y);
            QRgb cb = b.pixel(x, y);

            // tolerate a little bit of rounding errors
            int tolerance = 3;
            bool r = true;
            r &= qAbs(qRed(ca) - qRed(cb)) <= tolerance;
            r &= qAbs(qGreen(ca) - qGreen(cb)) <= tolerance;
            r &= qAbs(qBlue(ca) - qBlue(cb)) <= tolerance;
            if (!r)
                err++;
        }
    }
    QCOMPARE(err, 0);
}

// Tests smooth upscale is smooth
void tst_QImage::smoothScale4_data()
{
    QTest::addColumn<QImage::Format>("format");

    QTest::newRow("RGB32") << QImage::Format_RGB32;
    QTest::newRow("RGBx64") << QImage::Format_RGBX64;
}

void tst_QImage::smoothScale4()
{
    QFETCH(QImage::Format, format);
    QImage img(4, 4, format);
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            img.setPixel(x, y, qRgb(x * 255 / 3, y * 255 / 3, 0));
        }
    }
    QImage scaled = img.scaled(37, 23, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QCOMPARE(scaled.format(), format);
    for (int y = 0; y < scaled.height(); ++y) {
        for (int x = 0; x < scaled.width(); ++x) {
            if (x > 0)
                QVERIFY(scaled.pixelColor(x, y).redF() >= scaled.pixelColor(x - 1, y).redF());
            if (y > 0)
                QVERIFY(scaled.pixelColor(x, y).greenF() >= scaled.pixelColor(x, y - 1).greenF());
        }
    }
}

void tst_QImage::smoothScaleBig()
{
    int bigValue = 200000;
    QImage tall(4, bigValue, QImage::Format_ARGB32);
    tall.fill(0x0);

    QImage wide(bigValue, 4, QImage::Format_ARGB32);
    wide.fill(0x0);

    QImage tallScaled = tall.scaled(4, tall.height() / 4, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage wideScaled = wide.scaled(wide.width() / 4, 4, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QCOMPARE(tallScaled.pixel(0, 0), QRgb(0x0));
    QCOMPARE(wideScaled.pixel(0, 0), QRgb(0x0));
}

void tst_QImage::smoothScaleAlpha()
{
    QImage src(128, 128, QImage::Format_ARGB32_Premultiplied);
    src.fill(0x0);

    QPainter srcPainter(&src);
    srcPainter.setPen(Qt::NoPen);
    srcPainter.setBrush(Qt::white);
    srcPainter.drawEllipse(QRect(QPoint(), src.size()));
    srcPainter.end();

    QImage dst(32, 32, QImage::Format_ARGB32_Premultiplied);
    dst.fill(0xffffffff);
    QImage expected = dst;

    QPainter dstPainter(&dst);
    dstPainter.drawImage(0, 0, src.scaled(dst.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    dstPainter.end();

    QCOMPARE(dst, expected);
}

static int count(const QImage &img, int x, int y, int dx, int dy, QRgb pixel)
{
    int i = 0;
    while (x >= 0 && x < img.width() && y >= 0 && y < img.height()) {
        i += (img.pixel(x, y) == pixel);
        x += dx;
        y += dy;
    }
    return i;
}

const int transformed_image_width = 128;
const int transformed_image_height = 128;

void tst_QImage::transformed_data()
{
    QTest::addColumn<QTransform>("transform");

    {
        QTransform transform;
        transform.translate(10.4, 10.4);
        QTest::newRow("Translate") << transform;
    }
    {
        QTransform transform;
        transform.scale(1.5, 1.5);
        QTest::newRow("Scale") << transform;
    }
    {
        QTransform transform;
        transform.rotate(30);
        QTest::newRow("Rotate 30") << transform;
    }
    {
        QTransform transform;
        transform.rotate(90);
        QTest::newRow("Rotate 90") << transform;
    }
    {
        QTransform transform;
        transform.rotate(180);
        QTest::newRow("Rotate 180") << transform;
    }
    {
        QTransform transform;
        transform.rotate(270);
        QTest::newRow("Rotate 270") << transform;
    }
    {
        QTransform transform;
        transform.translate(transformed_image_width/2, transformed_image_height/2);
        transform.rotate(155, Qt::XAxis);
        transform.translate(-transformed_image_width/2, -transformed_image_height/2);
        QTest::newRow("Perspective 1") << transform;
    }
    {
        QTransform transform;
        transform.rotate(155, Qt::XAxis);
        transform.translate(-transformed_image_width/2, -transformed_image_height/2);
        QTest::newRow("Perspective 2") << transform;
    }
}

void tst_QImage::transformed()
{
    QFETCH(QTransform, transform);

    QImage img(transformed_image_width, transformed_image_height, QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    p.fillRect(0, 0, img.width(), img.height(), Qt::red);
    p.drawRect(0, 0, img.width()-1, img.height()-1);
    p.end();

    QImage transformed = img.transformed(transform, Qt::SmoothTransformation);

    // all borders should have touched pixels

    QVERIFY(count(transformed, 0, 0, 1, 0, 0x0) < transformed.width());
    QVERIFY(count(transformed, 0, 0, 0, 1, 0x0) < transformed.height());

    QVERIFY(count(transformed, 0, img.height() - 1, 1, 0, 0x0) < transformed.width());
    QVERIFY(count(transformed, img.width() - 1, 0, 0, 1, 0x0) < transformed.height());

    QImage transformedPadded(transformed.width() + 2, transformed.height() + 2, img.format());
    transformedPadded.fill(0x0);

    p.begin(&transformedPadded);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setRenderHint(QPainter::Antialiasing);
    p.setTransform(transformed.trueMatrix(transform, img.width(), img.height()) * QTransform().translate(1, 1));
    p.drawImage(0, 0, img);
    p.end();

    // no borders should have touched pixels since we have a one-pixel padding

    QCOMPARE(count(transformedPadded, 0, 0, 1, 0, 0x0), transformedPadded.width());
    QCOMPARE(count(transformedPadded, 0, 0, 0, 1, 0x0), transformedPadded.height());

    QCOMPARE(count(transformedPadded, 0, transformedPadded.height() - 1, 1, 0, 0x0), transformedPadded.width());
    QCOMPARE(count(transformedPadded, transformedPadded.width() - 1, 0, 0, 1, 0x0), transformedPadded.height());
}

void tst_QImage::transformed2()
{
    QImage img(3, 3, QImage::Format_Mono);
    QPainter p(&img);
    p.fillRect(0, 0, 3, 3, Qt::white);
    p.fillRect(0, 0, 3, 3, Qt::Dense4Pattern);
    p.end();

    QTransform transform;
    transform.scale(3, 3);

    QImage expected(9, 9, QImage::Format_Mono);
    p.begin(&expected);
    p.fillRect(0, 0, 9, 9, Qt::white);
    p.setBrush(Qt::black);
    p.setPen(Qt::NoPen);
    p.drawRect(3, 0, 3, 3);
    p.drawRect(0, 3, 3, 3);
    p.drawRect(6, 3, 3, 3);
    p.drawRect(3, 6, 3, 3);
    p.end();

    {
        QImage actual = img.transformed(transform);

        QCOMPARE(actual.format(), expected.format());
        QCOMPARE(actual.size(), expected.size());
        QCOMPARE(actual, expected);
    }

    {
        transform.rotate(-90);
        QImage actual = img.transformed(transform);

        QCOMPARE(actual.convertToFormat(QImage::Format_ARGB32_Premultiplied),
                 expected.convertToFormat(QImage::Format_ARGB32_Premultiplied));
    }
}

void tst_QImage::scaled()
{
    QImage img(102, 3, QImage::Format_Mono);
    QPainter p(&img);
    p.fillRect(0, 0, img.width(), img.height(), Qt::white);
    p.end();

    QImage scaled = img.scaled(1994, 10);

    QImage expected(1994, 10, QImage::Format_Mono);
    p.begin(&expected);
    p.fillRect(0, 0, expected.width(), expected.height(), Qt::white);
    p.end();

    QCOMPARE(scaled, expected);
}

void tst_QImage::paintEngine()
{
    QImage img;

    QPaintEngine *engine;
    {
        QImage temp(100, 100, QImage::Format_RGB32);
        temp.fill(0xff000000);

        QPainter p(&temp);
        p.fillRect(80,80,10,10,Qt::blue);
        p.end();

        img = temp;

        engine = temp.paintEngine();
    }

    {
        QPainter p(&img);

        p.fillRect(80,10,10,10,Qt::yellow);
        p.end();
    }

    QImage expected(100, 100, QImage::Format_RGB32);
    expected.fill(0xff000000);

    QPainter p(&expected);
    p.fillRect(80,80,10,10,Qt::blue);
    p.fillRect(80,10,10,10,Qt::yellow);
    p.end();

    QCOMPARE(engine, img.paintEngine());
    QCOMPARE(img, expected);
}

void tst_QImage::setAlphaChannelWhilePainting()
{
    QImage image(100, 100, QImage::Format_ARGB32);
    image.fill(Qt::black);
    QPainter p(&image);

    image.setAlphaChannel(image.createMaskFromColor(QColor(Qt::black).rgb(), Qt::MaskInColor));
}


// See task 240047 for details
void tst_QImage::smoothScaledSubImage()
{
    QImage original(128, 128, QImage::Format_RGB32);
    QPainter p(&original);
    p.fillRect(0, 0, 64, 128, Qt::black);
    p.fillRect(64, 0, 64, 128, Qt::white);
    p.end();

    QImage subimage(((const QImage &) original).bits(), 32, 32, original.bytesPerLine(), QImage::Format_RGB32); // only in the black part of the source...

    QImage scaled = subimage.scaled(8, 8, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    for (int y=0; y<scaled.height(); ++y)
        for (int x=0; x<scaled.width(); ++x)
            QCOMPARE(scaled.pixel(x, y), 0xff000000);
}

void tst_QImage::nullSize_data()
{
    QTest::addColumn<QImage>("image");
    QTest::newRow("null image") << QImage();
    QTest::newRow("zero-size image") << QImage(0, 0, QImage::Format_RGB32);
}

void tst_QImage::nullSize()
{
    QFETCH(QImage, image);
    QCOMPARE(image.isNull(), true);
    QCOMPARE(image.width(), image.size().width());
    QCOMPARE(image.height(), image.size().height());
}

void tst_QImage::premultipliedAlphaConsistency()
{
    QImage img(256, 1, QImage::Format_ARGB32);
    for (int x = 0; x < 256; ++x)
        img.setPixel(x, 0, (x << 24) | 0xffffff);

    QImage converted = img.convertToFormat(QImage::Format_ARGB8565_Premultiplied);
    QImage pm32 = converted.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    for (int i = 0; i < pm32.width(); ++i) {
        QRgb pixel = pm32.pixel(i, 0);
        QVERIFY(qRed(pixel) <= qAlpha(pixel));
        QVERIFY(qGreen(pixel) <= qAlpha(pixel));
        QVERIFY(qBlue(pixel) <= qAlpha(pixel));
    }
}

void tst_QImage::compareIndexed()
{
    QImage img(256, 1, QImage::Format_Indexed8);

    QVector<QRgb> colorTable(256);
    for (int i = 0; i < 256; ++i)
        colorTable[i] = qRgb(i, i, i);
    img.setColorTable(colorTable);

    for (int i = 0; i < 256; ++i) {
        img.setPixel(i, 0, i);
    }

    QImage imgInverted(256, 1, QImage::Format_Indexed8);
    QVector<QRgb> invertedColorTable(256);
    for (int i = 0; i < 256; ++i)
        invertedColorTable[255-i] = qRgb(i, i, i);
    imgInverted.setColorTable(invertedColorTable);

    for (int i = 0; i < 256; ++i) {
        imgInverted.setPixel(i, 0, (255-i));
    }

    QCOMPARE(img, imgInverted);
}

void tst_QImage::fillColor_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<Qt::GlobalColor>("color");
    QTest::addColumn<uint>("pixelValue");

    QTest::newRow("Mono, color0") << QImage::Format_Mono << Qt::color0 << 0u;
    QTest::newRow("Mono, color1") << QImage::Format_Mono << Qt::color1 << 1u;

    QTest::newRow("MonoLSB, color0") << QImage::Format_MonoLSB << Qt::color0 << 0u;
    QTest::newRow("MonoLSB, color1") << QImage::Format_MonoLSB << Qt::color1 << 1u;

    const char *names[] = {
        "Indexed8",
        "RGB32",
        "ARGB32",
        "ARGB32pm",
        "RGB16",
        "ARGB8565pm",
        "RGB666",
        "ARGB6666pm",
        "RGB555",
        "ARGB8555pm",
        "RGB888",
        "RGB444",
        "ARGB4444pm",
        "RGBx8888",
        "RGBA8888pm",
        "BGR30",
        "A2RGB30pm",
        0
    };

    QImage::Format formats[] = {
        QImage::Format_Indexed8,
        QImage::Format_RGB32,
        QImage::Format_ARGB32,
        QImage::Format_ARGB32_Premultiplied,
        QImage::Format_RGB16,
        QImage::Format_ARGB8565_Premultiplied,
        QImage::Format_RGB666,
        QImage::Format_ARGB6666_Premultiplied,
        QImage::Format_RGB555,
        QImage::Format_ARGB8555_Premultiplied,
        QImage::Format_RGB888,
        QImage::Format_RGB444,
        QImage::Format_ARGB4444_Premultiplied,
        QImage::Format_RGBX8888,
        QImage::Format_RGBA8888_Premultiplied,
        QImage::Format_BGR30,
        QImage::Format_A2RGB30_Premultiplied,
    };

    for (int i=0; names[i] != 0; ++i) {
        QByteArray name;
        name.append(names[i]).append(", ");

        QTest::newRow(QByteArray(name).append("black").constData()) << formats[i] << Qt::black << 0xff000000;
        QTest::newRow(QByteArray(name).append("white").constData()) << formats[i] << Qt::white << 0xffffffff;
        QTest::newRow(QByteArray(name).append("red").constData())   << formats[i] << Qt::red   << 0xffff0000;
        QTest::newRow(QByteArray(name).append("green").constData()) << formats[i] << Qt::green << 0xff00ff00;
        QTest::newRow(QByteArray(name).append("blue").constData())  << formats[i] << Qt::blue  << 0xff0000ff;
    }

    QTest::newRow("RGB16, transparent") << QImage::Format_RGB16 << Qt::transparent << 0xff000000;
    QTest::newRow("RGB32, transparent") << QImage::Format_RGB32 << Qt::transparent << 0xff000000;
    QTest::newRow("ARGB32, transparent") << QImage::Format_ARGB32 << Qt::transparent << 0x00000000u;
    QTest::newRow("ARGB32pm, transparent") << QImage::Format_ARGB32_Premultiplied << Qt::transparent << 0x00000000u;
    QTest::newRow("RGBA8888pm, transparent") << QImage::Format_RGBA8888_Premultiplied << Qt::transparent << 0x00000000u;
    QTest::newRow("A2RGB30pm, transparent") << QImage::Format_A2RGB30_Premultiplied << Qt::transparent << 0x00000000u;
}

void tst_QImage::fillColor()
{
    QFETCH(QImage::Format, format);
    QFETCH(Qt::GlobalColor, color);
    QFETCH(uint, pixelValue);

    QImage image(1, 1, format);

    if (image.depth() == 8) {
        QVector<QRgb> table;
        table << 0xff000000;
        table << 0xffffffff;
        table << 0xffff0000;
        table << 0xff00ff00;
        table << 0xff0000ff;
        image.setColorTable(table);
    }

    image.fill(color);
    if (image.depth() == 1) {
        QCOMPARE(image.pixelIndex(0, 0), (int) pixelValue);
    } else {
        QCOMPARE(image.pixel(0, 0), pixelValue);
    }

    image.fill(QColor(color));
    if (image.depth() == 1) {
        QCOMPARE(image.pixelIndex(0, 0), (int) pixelValue);
    } else {
        QCOMPARE(image.pixel(0, 0), pixelValue);
    }
}

void tst_QImage::fillColorWithAlpha()
{
    QImage argb32(1, 1, QImage::Format_ARGB32);
    argb32.fill(QColor(255, 0, 0, 127));
    QCOMPARE(argb32.pixel(0, 0), qRgba(255, 0, 0, 127));

    QImage argb32pm(1, 1, QImage::Format_ARGB32_Premultiplied);
    argb32pm.fill(QColor(255, 0, 0, 127));
    QCOMPARE(argb32pm.pixel(0, 0), 0x7f7f0000u);
}

void tst_QImage::fillRGB888()
{
    QImage expected(1, 1, QImage::Format_RGB888);
    QImage actual(1, 1, QImage::Format_RGB888);

    for (int c = Qt::black; c < Qt::transparent; ++c) {
        QColor color = QColor(Qt::GlobalColor(c));

        expected.fill(color);
        actual.fill(color.rgba());

        QCOMPARE(actual.pixel(0, 0), expected.pixel(0, 0));
    }
}

void tst_QImage::fillPixel_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<uint>("color");
    QTest::addColumn<uint>("pixelValue");

    QTest::newRow("RGB16, transparent") << QImage::Format_RGB16 << 0x0u << 0xff000000u;
    QTest::newRow("RGB32, transparent") << QImage::Format_RGB32 << 0x0u << 0xff000000u;
    QTest::newRow("RGBx8888, transparent") << QImage::Format_RGBX8888 << 0x0u << 0xff000000u;
    QTest::newRow("ARGB32, transparent") << QImage::Format_ARGB32 << 0x0u << 0x00000000u;
    QTest::newRow("ARGB32pm, transparent") << QImage::Format_ARGB32_Premultiplied << 0x0u << 0x00000000u;
    QTest::newRow("RGBA8888pm, transparent") << QImage::Format_RGBA8888_Premultiplied << 0x0u << 0x00000000u;
    QTest::newRow("Alpha8, transparent") << QImage::Format_Alpha8 << 0x0u << 0x00000000u;

    QTest::newRow("RGB16, red") << QImage::Format_RGB16 << (uint)qConvertRgb32To16(0xffff0000) << 0xffff0000u;
    QTest::newRow("RGB32, red") << QImage::Format_RGB32 << 0xffff0000u << 0xffff0000u;
    QTest::newRow("ARGB32, red") << QImage::Format_ARGB32 << 0xffff0000u << 0xffff0000u;
    QTest::newRow("RGBA8888, red") << QImage::Format_RGBA8888 << 0xff0000ffu << 0xffff0000u;

    QTest::newRow("Grayscale8, grey") << QImage::Format_Grayscale8 << 0xff808080u << 0xff808080u;

    QTest::newRow("RGB32, semi-red") << QImage::Format_RGB32 << 0x80ff0000u << 0xffff0000u;
    QTest::newRow("ARGB32, semi-red") << QImage::Format_ARGB32 << 0x80ff0000u << 0x80ff0000u;
    QTest::newRow("ARGB32pm, semi-red") << QImage::Format_ARGB32 << 0x80800000u << 0x80800000u;
    QTest::newRow("RGBA8888pm, semi-red") << QImage::Format_RGBA8888_Premultiplied << 0x80000080u << 0x80800000u;
    QTest::newRow("Alpha8, semi-red") << QImage::Format_Alpha8 << 0x80000080u << 0x80000000u;
}

void tst_QImage::fillPixel()
{
    QFETCH(QImage::Format, format);
    QFETCH(uint, color);
    QFETCH(uint, pixelValue);

    QImage image(1, 1, format);

    image.fill(color);
    QCOMPARE(image.pixel(0, 0), pixelValue);
}

void tst_QImage::rgbSwapped_data()
{
    QTest::addColumn<QImage::Format>("format");

    for (int i = QImage::Format_Indexed8; i < QImage::NImageFormats; ++i) {
        if (i == QImage::Format_Alpha8 || i == QImage::Format_Grayscale8)
            continue;
        QTest::addRow("%s", formatToString(QImage::Format(i)).data()) << QImage::Format(i);
    }
}

void tst_QImage::rgbSwapped()
{
    QFETCH(QImage::Format, format);

    QImage image(100, 1, format);
    image.fill(0);

    QVector<QColor> testColor(image.width());

    for (int i = 0; i < image.width(); ++i)
        testColor[i] = QColor(i, 10 + i, 20 + i * 2, 30 + i);

    if (format != QImage::Format_Indexed8) {
        QPainter p(&image);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        for (int i = 0; i < image.width(); ++i)
            p.fillRect(QRect(i, 0, 1, 1), testColor[i].rgb());
    } else {
        image.setColorCount(image.width());
        for (int i = 0; i < image.width(); ++i) {
            image.setColor(0, testColor[i].rgba());
            image.setPixel(i, 0, i);
        }
    }

    QImage imageSwapped = image.rgbSwapped();

    for (int i = 0; i < image.width(); ++i) {
        QColor referenceColor = QColor(image.pixel(i, 0));
        QColor swappedColor = QColor(imageSwapped.pixel(i, 0));

        QCOMPARE(swappedColor.alpha(), referenceColor.alpha());
        QCOMPARE(swappedColor.red(), referenceColor.blue());
        QCOMPARE(swappedColor.green(), referenceColor.green());
        QCOMPARE(swappedColor.blue(), referenceColor.red());
    }

    QImage imageSwappedTwice = imageSwapped.rgbSwapped();

    QCOMPARE(image, imageSwappedTwice);

    QCOMPARE(memcmp(image.constBits(), imageSwappedTwice.constBits(), image.sizeInBytes()), 0);
}

void tst_QImage::mirrored_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<bool>("swap_vertical");
    QTest::addColumn<bool>("swap_horizontal");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");

    QTest::newRow("Format_RGB32, vertical") << QImage::Format_RGB32 << true << false << 16 << 16;
    QTest::newRow("Format_ARGB32, vertical") << QImage::Format_ARGB32 << true << false << 16 << 16;
    QTest::newRow("Format_ARGB32_Premultiplied, vertical") << QImage::Format_ARGB32_Premultiplied << true << false << 16 << 16;
    QTest::newRow("Format_RGB16, vertical") << QImage::Format_RGB16 << true << false << 16 << 16;
    QTest::newRow("Format_ARGB8565_Premultiplied, vertical") << QImage::Format_ARGB8565_Premultiplied << true << false << 16 << 16;
    QTest::newRow("Format_ARGB6666_Premultiplied, vertical") << QImage::Format_ARGB6666_Premultiplied << true << false << 16 << 16;
    QTest::newRow("Format_ARGB4444_Premultiplied, vertical") << QImage::Format_ARGB4444_Premultiplied << true << false << 16 << 16;
    QTest::newRow("Format_RGB666, vertical") << QImage::Format_RGB666 << true << false << 16 << 16;
    QTest::newRow("Format_RGB555, vertical") << QImage::Format_RGB555 << true << false << 16 << 16;
    QTest::newRow("Format_ARGB8555_Premultiplied, vertical") << QImage::Format_ARGB8555_Premultiplied << true << false << 16 << 16;
    QTest::newRow("Format_RGB888, vertical") << QImage::Format_RGB888 << true << false << 16 << 16;
    QTest::newRow("Format_RGB444, vertical") << QImage::Format_RGB444 << true << false << 16 << 16;
    QTest::newRow("Format_RGBX8888, vertical") << QImage::Format_RGBX8888 << true << false << 16 << 16;
    QTest::newRow("Format_RGBA8888_Premultiplied, vertical") << QImage::Format_RGBA8888_Premultiplied << true << false << 16 << 16;
    QTest::newRow("Format_A2BGR30_Premultiplied, vertical") << QImage::Format_A2BGR30_Premultiplied << true << false << 16 << 16;
    QTest::newRow("Format_RGB30, vertical") << QImage::Format_RGB30 << true << false << 16 << 16;
    QTest::newRow("Format_Indexed8, vertical") << QImage::Format_Indexed8 << true << false << 16 << 16;
    QTest::newRow("Format_Mono, vertical") << QImage::Format_Mono << true << false << 16 << 16;
    QTest::newRow("Format_MonoLSB, vertical") << QImage::Format_MonoLSB << true << false << 16 << 16;

    QTest::newRow("Format_ARGB32_Premultiplied, horizontal") << QImage::Format_ARGB32_Premultiplied << false << true << 16 << 16;
    QTest::newRow("Format_RGB888, horizontal") << QImage::Format_RGB888 << false << true << 16 << 16;
    QTest::newRow("Format_RGB16, horizontal") << QImage::Format_RGB16 << false << true << 16 << 16;
    QTest::newRow("Format_Indexed8, horizontal") << QImage::Format_Indexed8 << false << true << 16 << 16;
    QTest::newRow("Format_Mono, horizontal") << QImage::Format_Mono << false << true << 16 << 16;
    QTest::newRow("Format_MonoLSB, horizontal") << QImage::Format_MonoLSB << false << true << 16 << 16;

    QTest::newRow("Format_ARGB32_Premultiplied, horizontal+vertical") << QImage::Format_ARGB32_Premultiplied << true << true << 16 << 16;
    QTest::newRow("Format_RGB888, horizontal+vertical") << QImage::Format_RGB888 << true << true << 16 << 16;
    QTest::newRow("Format_RGB16, horizontal+vertical") << QImage::Format_RGB16 << true << true << 16 << 16;
    QTest::newRow("Format_Indexed8, horizontal+vertical") << QImage::Format_Indexed8 << true << true << 16 << 16;
    QTest::newRow("Format_Mono, horizontal+vertical") << QImage::Format_Mono << true << true << 16 << 16;
    QTest::newRow("Format_MonoLSB, horizontal+vertical") << QImage::Format_MonoLSB << true << true << 16 << 16;

    QTest::newRow("Format_RGB32, vertical") << QImage::Format_RGB32 << true << false << 8 << 16;
    QTest::newRow("Format_ARGB32, vertical") << QImage::Format_ARGB32 << true << false << 16 << 8;
    QTest::newRow("Format_Mono, vertical, non-aligned") << QImage::Format_Mono << true << false << 19 << 25;
    QTest::newRow("Format_MonoLSB, vertical, non-aligned") << QImage::Format_MonoLSB << true << false << 19 << 25;

    // Non-aligned horizontal 1-bit needs special handling so test this.
    QTest::newRow("Format_Mono, horizontal, non-aligned") << QImage::Format_Mono << false << true << 13 << 17;
    QTest::newRow("Format_Mono, horizontal, non-aligned") << QImage::Format_Mono << false << true << 19 << 25;
    QTest::newRow("Format_Mono, horizontal+vertical, non-aligned") << QImage::Format_Mono << true << true << 25 << 47;
    QTest::newRow("Format_Mono, horizontal+vertical, non-aligned") << QImage::Format_Mono << true << true << 21 << 16;

    QTest::newRow("Format_MonoLSB, horizontal, non-aligned") << QImage::Format_MonoLSB << false << true << 13 << 17;
    QTest::newRow("Format_MonoLSB, horizontal, non-aligned") << QImage::Format_MonoLSB << false << true << 19 << 25;
    QTest::newRow("Format_MonoLSB, horizontal+vertical, non-aligned") << QImage::Format_MonoLSB << true << true << 25 << 47;
    QTest::newRow("Format_MonoLSB, horizontal+vertical, non-aligned") << QImage::Format_MonoLSB << true << true << 21 << 16;
}

void tst_QImage::mirrored()
{
    QFETCH(QImage::Format, format);
    QFETCH(bool, swap_vertical);
    QFETCH(bool, swap_horizontal);
    QFETCH(int, width);
    QFETCH(int, height);

    QImage image(width, height, format);

    switch (format) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        for (int i = 0; i < image.height(); ++i) {
            ushort* scanLine = (ushort*)image.scanLine(i);
            *scanLine = (i % 2) ? 0x5555U : 0xCCCCU;
        }
        break;
    case QImage::Format_Indexed8:
        for (int i = 0; i < image.height(); ++i) {
            for (int j = 0; j < image.width(); ++j) {
                image.setColor(i*16+j, qRgb(j*16, i*16, 0));
                image.setPixel(j, i, i*16+j);
            }
        }
        break;
    default:
        for (int i = 0; i < image.height(); ++i)
            for (int j = 0; j < image.width(); ++j)
                image.setPixel(j, i, qRgb(j*16, i*16, 0));
        break;
    }

    QImage imageMirrored = image.mirrored(swap_horizontal, swap_vertical);

    for (int i = 0; i < image.height(); ++i) {
        int mirroredI = swap_vertical ? (image.height() - i - 1) : i;
        for (int j = 0; j < image.width(); ++j) {
            QRgb referenceColor = image.pixel(j, i);
            int mirroredJ = swap_horizontal ? (image.width() - j - 1) : j;
            QRgb mirroredColor = imageMirrored.pixel(mirroredJ, mirroredI);
            QCOMPARE(mirroredColor, referenceColor);
        }
    }

    QImage imageMirroredTwice = imageMirrored.mirrored(swap_horizontal, swap_vertical);

    QCOMPARE(image, imageMirroredTwice);

    if (format != QImage::Format_Mono && format != QImage::Format_MonoLSB)
        QCOMPARE(memcmp(image.constBits(), imageMirroredTwice.constBits(), image.sizeInBytes()), 0);
    else {
        for (int i = 0; i < image.height(); ++i)
            for (int j = 0; j < image.width(); ++j)
                QCOMPARE(image.pixel(j,i), imageMirroredTwice.pixel(j,i));
    }
}

void tst_QImage::inplaceRgbSwapped_data()
{
    rgbSwapped_data();
}

void tst_QImage::inplaceRgbSwapped()
{
#if defined(Q_COMPILER_REF_QUALIFIERS)
    QFETCH(QImage::Format, format);

    QImage image(64, 1, format);
    image.fill(0);

    QVector<QRgb> testColor(image.width());
    for (int i = 0; i < image.width(); ++i)
        testColor[i] = qRgb(i * 2, i * 3, 255 - i * 4);

    if (format == QImage::Format_Indexed8) {
        for (int i = 0; i < image.width(); ++i) {
            image.setColor(i, testColor[i]);
            image.setPixel(i, 0, i);
        }
    } else {
        for (int i = 0; i < image.width(); ++i)
            image.setPixel(i, 0, testColor[i]);
    }

    const uchar* orginalPtr = image.constScanLine(0);
    QImage imageSwapped = std::move(image).rgbSwapped();

    for (int i = 0; i < imageSwapped.width(); ++i) {
        QRgb referenceColor = testColor[i];
        QRgb swappedColor = imageSwapped.pixel(i, 0);
        QCOMPARE(qRed(swappedColor) & 0xf0, qBlue(referenceColor) & 0xf0);
        QCOMPARE(qGreen(swappedColor) & 0xf0, qGreen(referenceColor) & 0xf0);
        QCOMPARE(qBlue(swappedColor) & 0xf0, qRed(referenceColor) & 0xf0);
    }

    QCOMPARE(imageSwapped.constScanLine(0), orginalPtr);

    for (int rw = 0; rw <= 1; rw++) {
        // Test attempted inplace conversion of images created on existing buffer
        uchar *volatileData = 0;
        QImage orig = imageSwapped;
        QImage dataSwapped;
        {
            QVERIFY(!orig.isNull());
            volatileData = new uchar[orig.sizeInBytes()];
            memcpy(volatileData, orig.constBits(), orig.sizeInBytes());

            QImage dataImage;
            if (rw)
                dataImage = QImage(volatileData, orig.width(), orig.height(), orig.format());
            else
                dataImage = QImage((const uchar *)volatileData, orig.width(), orig.height(), orig.format());

            if (orig.colorCount())
                dataImage.setColorTable(orig.colorTable());

            dataSwapped = std::move(dataImage).rgbSwapped();
            QVERIFY(!dataSwapped.isNull());
            delete[] volatileData;
        }

        QVERIFY2(dataSwapped.constBits() != volatileData, rw ? "non-const" : "const");
        QCOMPARE(dataSwapped, orig.rgbSwapped());
    }

#endif
}


void tst_QImage::inplaceMirrored_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<bool>("swap_vertical");
    QTest::addColumn<bool>("swap_horizontal");

    for (int i = QImage::Format_Mono; i < QImage::NImageFormats; ++i) {
        if (i == QImage::Format_Alpha8 || i == QImage::Format_Grayscale8)
            continue;
        if (i == QImage::Format_RGB444 || i == QImage::Format_ARGB4444_Premultiplied)
            continue;
        const auto fmt = formatToString(QImage::Format(i));
        QTest::addRow("%s, vertical", fmt.data())
                << QImage::Format(i) << true << false;
        QTest::addRow("%s, horizontal", fmt.data())
                << QImage::Format(i) << false << true;
        QTest::addRow("%s, horizontal+vertical", fmt.data())
                << QImage::Format(i) << true << true;
    }
}

void tst_QImage::inplaceMirrored()
{
#if defined(Q_COMPILER_REF_QUALIFIERS)
    QFETCH(QImage::Format, format);
    QFETCH(bool, swap_vertical);
    QFETCH(bool, swap_horizontal);

    QImage image(16, 16, format);

    switch (format) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        for (int i = 0; i < image.height(); ++i) {
            ushort* scanLine = (ushort*)image.scanLine(i);
            *scanLine = (i % 2) ? 0x0fffU : 0xf000U;
        }
        break;
    case QImage::Format_Indexed8:
        for (int i = 0; i < image.height(); ++i) {
            for (int j = 0; j < image.width(); ++j) {
                image.setColor(i*16+j, qRgb(j*16, i*16, 0));
                image.setPixel(j, i, i*16+j);
            }
        }
        break;
    default:
        for (int i = 0; i < image.height(); ++i)
            for (int j = 0; j < image.width(); ++j)
                image.setPixel(j, i, qRgb(j*16, i*16, 0));
    }

    const uchar* originalPtr = image.constScanLine(0);

    QImage imageMirrored = std::move(image).mirrored(swap_horizontal, swap_vertical);
    if (format != QImage::Format_Mono && format != QImage::Format_MonoLSB) {
        for (int i = 0; i < imageMirrored.height(); ++i) {
            int mirroredI = swap_vertical ? (imageMirrored.height() - i - 1) : i;
            for (int j = 0; j < imageMirrored.width(); ++j) {
                int mirroredJ = swap_horizontal ? (imageMirrored.width() - j - 1) : j;
                QRgb mirroredColor = imageMirrored.pixel(mirroredJ, mirroredI);
                QCOMPARE(qRed(mirroredColor) & 0xF8, j * 16);
                QCOMPARE(qGreen(mirroredColor) & 0xF8, i * 16);
            }
        }
    } else  {
        for (int i = 0; i < imageMirrored.height(); ++i) {
            ushort* scanLine = (ushort*)imageMirrored.scanLine(i);
            ushort expect;
            if (swap_vertical && swap_horizontal)
                expect = (i % 2) ? 0x000fU : 0xfff0U;
            else if (swap_vertical)
                expect = (i % 2) ? 0xf000U : 0x0fffU;
            else
                expect = (i % 2) ? 0xfff0U : 0x000fU;
            QCOMPARE(*scanLine, expect);
        }
    }
    QCOMPARE(imageMirrored.constScanLine(0), originalPtr);

    for (int rw = 0; rw <= 1; rw++) {
        // Test attempted inplace conversion of images created on existing buffer
        uchar *volatileData = 0;
        QImage orig = imageMirrored;
        QImage dataSwapped;
        {
            QVERIFY(!orig.isNull());
            volatileData = new uchar[orig.sizeInBytes()];
            memcpy(volatileData, orig.constBits(), orig.sizeInBytes());

            QImage dataImage;
            if (rw)
                dataImage = QImage(volatileData, orig.width(), orig.height(), orig.format());
            else
                dataImage = QImage((const uchar *)volatileData, orig.width(), orig.height(), orig.format());

            if (orig.colorCount())
                dataImage.setColorTable(orig.colorTable());

            dataSwapped = std::move(dataImage).mirrored(swap_horizontal, swap_vertical);
            QVERIFY(!dataSwapped.isNull());
            delete[] volatileData;
        }

        QVERIFY2(dataSwapped.constBits() != volatileData, rw ? "non-const" : "const");
        QCOMPARE(dataSwapped, orig.mirrored(swap_horizontal, swap_vertical));
    }

#endif
}

void tst_QImage::inplaceMirroredOdd_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<bool>("swap_vertical");
    QTest::addColumn<bool>("swap_horizontal");

    QTest::newRow("Format_ARGB32, vertical") << QImage::Format_ARGB32 << true << false;
    QTest::newRow("Format_RGB888, vertical") << QImage::Format_RGB888 << true << false;
    QTest::newRow("Format_RGB16, vertical") << QImage::Format_RGB16 << true << false;

    QTest::newRow("Format_ARGB32, horizontal") << QImage::Format_ARGB32 << false << true;
    QTest::newRow("Format_RGB888, horizontal") << QImage::Format_RGB888 << false << true;
    QTest::newRow("Format_RGB16, horizontal") << QImage::Format_RGB16 << false << true;

    QTest::newRow("Format_ARGB32, horizontal+vertical") << QImage::Format_ARGB32 << true << true;
    QTest::newRow("Format_RGB888, horizontal+vertical") << QImage::Format_RGB888 << true << true;
    QTest::newRow("Format_RGB16, horizontal+vertical") << QImage::Format_RGB16 << true << true;
}

void tst_QImage::inplaceMirroredOdd()
{
#if defined(Q_COMPILER_REF_QUALIFIERS)
    QFETCH(QImage::Format, format);
    QFETCH(bool, swap_vertical);
    QFETCH(bool, swap_horizontal);

    QImage image(15, 15, format);

    for (int i = 0; i < image.height(); ++i)
        for (int j = 0; j < image.width(); ++j)
            image.setPixel(j, i, qRgb(j*16, i*16, 0));

    const uchar* originalPtr = image.constScanLine(0);

    QImage imageMirrored = std::move(image).mirrored(swap_horizontal, swap_vertical);
    for (int i = 0; i < imageMirrored.height(); ++i) {
        int mirroredI = swap_vertical ? (imageMirrored.height() - i - 1) : i;
        for (int j = 0; j < imageMirrored.width(); ++j) {
            int mirroredJ = swap_horizontal ? (imageMirrored.width() - j - 1) : j;
            QRgb mirroredColor = imageMirrored.pixel(mirroredJ, mirroredI);
            QCOMPARE(qRed(mirroredColor) & 0xF8, j * 16);
            QCOMPARE(qGreen(mirroredColor) & 0xF8, i * 16);
        }
    }
    QCOMPARE(imageMirrored.constScanLine(0), originalPtr);
#endif
}

void tst_QImage::inplaceRgbMirrored()
{
#if defined(Q_COMPILER_REF_QUALIFIERS)
    QImage image1(32, 32, QImage::Format_ARGB32);
    QImage image2(32, 32, QImage::Format_ARGB32);
    image1.fill(0);
    image2.fill(0);
    const uchar* originalPtr1 = image1.constScanLine(0);
    const uchar* originalPtr2 = image2.constScanLine(0);

    QCOMPARE(std::move(image1).rgbSwapped().mirrored().constScanLine(0), originalPtr1);
    QCOMPARE(std::move(image2).mirrored().rgbSwapped().constScanLine(0), originalPtr2);
#endif
}

void tst_QImage::genericRgbConversion_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<QImage::Format>("dest_format");

    for (int i = QImage::Format_RGB32; i < QImage::NImageFormats; ++i) {
        if (i == QImage::Format_Alpha8 || i == QImage::Format_Grayscale8)
            continue;
        const QLatin1String formatI = formatToString(QImage::Format(i));
        for (int j = QImage::Format_RGB32; j < QImage::NImageFormats; ++j) {
            if (j == QImage::Format_Alpha8 || j == QImage::Format_Grayscale8)
                continue;
            if (i == j)
                continue;
            QTest::addRow("%s -> %s", formatI.data(), formatToString(QImage::Format(j)).data())
                    << QImage::Format(i) << QImage::Format(j);
        }
    }
}

void tst_QImage::genericRgbConversion()
{
    // Test that all RGB conversions work and maintain at least 4bit of color accuracy.
    QFETCH(QImage::Format, format);
    QFETCH(QImage::Format, dest_format);

    QImage image(16, 16, format);

    for (int i = 0; i < image.height(); ++i)
        for (int j = 0; j < image.width(); ++j)
            image.setPixel(j, i, qRgb(j*16, i*16, 0));

    QImage imageConverted = image.convertToFormat(dest_format);
    QCOMPARE(imageConverted.format(), dest_format);
    for (int i = 0; i < imageConverted.height(); ++i) {
        for (int j = 0; j < imageConverted.width(); ++j) {
            QRgb convertedColor = imageConverted.pixel(j,i);
            QCOMPARE(qRed(convertedColor) & 0xF0, j * 16);
            QCOMPARE(qGreen(convertedColor) & 0xF0, i * 16);
        }
    }
}

void tst_QImage::inplaceRgbConversion_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<QImage::Format>("dest_format");

    for (int i = QImage::Format_RGB32; i < QImage::NImageFormats; ++i) {
        if (i == QImage::Format_Alpha8 || i == QImage::Format_Grayscale8)
            continue;
        for (int j = QImage::Format_RGB32; j < QImage::NImageFormats; ++j) {
            if (j == QImage::Format_Alpha8 || j == QImage::Format_Grayscale8)
                continue;
            if (i == j)
                continue;
            QTest::addRow("%s -> %s", formatToString(QImage::Format(i)).data(), formatToString(QImage::Format(j)).data())
                    << QImage::Format(i) << QImage::Format(j);
        }
    }
}

void tst_QImage::inplaceRgbConversion()
{
    // Test that conversions between RGB formats of the same bitwidth can be done inplace.
#if defined(Q_COMPILER_REF_QUALIFIERS)
    QFETCH(QImage::Format, format);
    QFETCH(QImage::Format, dest_format);

    QImage image(16, 16, format);

    for (int i = 0; i < image.height(); ++i)
        for (int j = 0; j < image.width(); ++j)
            image.setPixel(j, i, qRgb(j*16, i*16, 0));

    const uchar* originalPtr = image.constScanLine(0);

    QImage imageConverted = std::move(image).convertToFormat(dest_format);
    QCOMPARE(imageConverted.format(), dest_format);
    for (int i = 0; i < imageConverted.height(); ++i) {
        for (int j = 0; j < imageConverted.width(); ++j) {
            QRgb convertedColor = imageConverted.pixel(j,i);
            QCOMPARE(qRed(convertedColor) & 0xF0, j * 16);
            QCOMPARE(qGreen(convertedColor) & 0xF0, i * 16);
        }
    }
    if (qt_depthForFormat(format) == qt_depthForFormat(dest_format))
        QCOMPARE(imageConverted.constScanLine(0), originalPtr);

    if (qt_depthForFormat(format) <= 32) {
        // Test attempted inplace conversion of images created on existing buffer
        static const quint32 readOnlyData[] = { 0xff0102ffU, 0xff0506ffU, 0xff0910ffU, 0xff1314ffU };
        quint32 readWriteData[] = { 0xff0102ffU, 0xff0506ffU, 0xff0910ffU, 0xff1314ffU };

        QImage roInplaceConverted;
        QImage rwInplaceConverted;

        {
            QImage roImage((const uchar *)readOnlyData, 2, 2, format);
            roInplaceConverted = std::move(roImage).convertToFormat(dest_format);

            QImage rwImage((uchar *)readWriteData, 2, 2, format);
            rwInplaceConverted = std::move(rwImage).convertToFormat(dest_format);
        }

        QImage roImage2((const uchar *)readOnlyData, 2, 2, format);
        QImage normalConverted = roImage2.convertToFormat(dest_format);

        QVERIFY(roInplaceConverted.constBits() != (const uchar *)readOnlyData);
        QCOMPARE(normalConverted, roInplaceConverted);

        QVERIFY(rwInplaceConverted.constBits() != (const uchar *)readWriteData);
        QCOMPARE(normalConverted, rwInplaceConverted);
    }
#endif
}

void tst_QImage::deepCopyWhenPaintingActive()
{
    QImage image(64, 64, QImage::Format_ARGB32_Premultiplied);
    image.fill(0);

    QPainter painter(&image);
    QImage copy = image;

    painter.setBrush(Qt::black);
    painter.drawEllipse(image.rect());

    QVERIFY(copy != image);
}

void tst_QImage::scaled_QTBUG19157()
{
    QImage foo(5000, 1, QImage::Format_RGB32);
    foo = foo.scaled(1024, 1024, Qt::KeepAspectRatio);
    QVERIFY(!foo.isNull());
}

void tst_QImage::convertOverUnPreMul()
{
    QImage image(256, 256, QImage::Format_ARGB32_Premultiplied);

    for (int j = 0; j < 256; j++) {
        for (int i = 0; i <= j; i++) {
            image.setPixel(i, j, qRgba(i, i, i, j));
        }
    }

    QImage image2 = image.convertToFormat(QImage::Format_ARGB32).convertToFormat(QImage::Format_ARGB32_Premultiplied);

    for (int j = 0; j < 256; j++) {
        for (int i = 0; i <= j; i++) {
            QCOMPARE(qAlpha(image2.pixel(i, j)), qAlpha(image.pixel(i, j)));
            QCOMPARE(qGray(image2.pixel(i, j)), qGray(image.pixel(i, j)));
        }
    }
}

void tst_QImage::scaled_QTBUG35972()
{
    QImage src(532,519,QImage::Format_ARGB32_Premultiplied);
    src.fill(QColor(Qt::white));
    QImage dest(1000,1000,QImage::Format_ARGB32_Premultiplied);
    dest.fill(QColor(Qt::white));
    QPainter painter1(&dest);
    const QTransform trf(1.25, 0,
                         0, 1.25,
                         /*dx */ 15.900000000000034, /* dy */ 72.749999999999986);
    painter1.setTransform(trf);
    painter1.drawImage(QRectF(-2.6, -2.6, 425.6, 415.20000000000005), src, QRectF(0,0,532,519));

    const quint32 *pixels = reinterpret_cast<const quint32 *>(dest.constBits());
    int size = dest.width()*dest.height();
    for (int i = 0; i < size; ++i)
        QCOMPARE(pixels[i], 0xffffffff);
}

void tst_QImage::convertToPixelFormat()
{
    QPixelFormat rgb565 = qPixelFormatRgba(5,6,5,0,QPixelFormat::IgnoresAlpha, QPixelFormat::AtBeginning, QPixelFormat::NotPremultiplied, QPixelFormat::UnsignedShort);
    QPixelFormat rgb565ImageFormat = QImage::toPixelFormat(QImage::Format_RGB16);
    QCOMPARE(rgb565, rgb565ImageFormat);
}

void tst_QImage::convertToImageFormat_data()
{
    QTest::addColumn<QImage::Format>("image_format");
    QTest::newRow("Convert Format_Invalid") << QImage::Format_Invalid;
    QTest::newRow("Convert Format_Mono") << QImage::Format_Mono;
    //This ends up being a QImage::Format_Mono since we cant specify LSB in QPixelFormat
    //QTest::newRow("Convert Format_MonoLSB") << QImage::Format_MonoLSB;
    QTest::newRow("Convert Format_Indexed8") << QImage::Format_Indexed8;
    QTest::newRow("Convert Format_RGB32") << QImage::Format_RGB32;
    QTest::newRow("Convert Format_ARGB32") << QImage::Format_ARGB32;
    QTest::newRow("Convert Format_ARGB32_Premultiplied") << QImage::Format_ARGB32_Premultiplied;
    QTest::newRow("Convert Format_RGB16") << QImage::Format_RGB16;
    QTest::newRow("Convert Format_ARGB8565_Premultiplied") << QImage::Format_ARGB8565_Premultiplied;
    QTest::newRow("Convert Format_RGB666") << QImage::Format_RGB666;
    QTest::newRow("Convert Format_ARGB6666_Premultiplied") << QImage::Format_ARGB6666_Premultiplied;
    QTest::newRow("Convert Format_RGB555") << QImage::Format_RGB555;
    QTest::newRow("Convert Format_ARGB8555_Premultiplied") << QImage::Format_ARGB8555_Premultiplied;
    QTest::newRow("Convert Format_RGB888") << QImage::Format_RGB888;
    QTest::newRow("Convert Format_RGB444") << QImage::Format_RGB444;
    QTest::newRow("Convert Format_ARGB4444_Premultiplied") << QImage::Format_ARGB4444_Premultiplied;
    QTest::newRow("Convert Format_RGBX8888") << QImage::Format_RGBX8888;
    QTest::newRow("Convert Format_RGBA8888") << QImage::Format_RGBA8888;
    QTest::newRow("Convert Format_RGBA8888_Premultiplied") << QImage::Format_RGBA8888_Premultiplied;
}

void tst_QImage::convertToImageFormat()
{
    QFETCH(QImage::Format, image_format);

    QPixelFormat pixel_format = QImage::toPixelFormat(image_format);
    QImage::Format format = QImage::toImageFormat(pixel_format);
    QCOMPARE(format, image_format);
}

void tst_QImage::invertPixelsRGB_data()
{
    QTest::addColumn<QImage::Format>("image_format");

    for (int i = QImage::Format_RGB32; i < QImage::NImageFormats; ++i) {
        if (i == QImage::Format_Alpha8 || i == QImage::Format_Grayscale8)
            continue;
        QTest::addRow("%s", formatToString(QImage::Format(i)).data()) << QImage::Format(i);
    }
}

void tst_QImage::invertPixelsRGB()
{
    QFETCH(QImage::Format, image_format);

    QImage image(1, 1, image_format);
    image.fill(QColor::fromRgb(32, 64, 96));
    image.invertPixels();

    QCOMPARE(image.format(), image_format);

    uint pixel = image.pixel(0, 0);
    QCOMPARE(qRed(pixel) >> 4, (255 - 32) >> 4);
    QCOMPARE(qGreen(pixel) >> 4, (255 - 64) >> 4);
    QCOMPARE(qBlue(pixel) >> 4, (255 - 96) >> 4);
}

void tst_QImage::exifOrientation_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("orientation");
    QTest::addColumn<int>("dpmx");
    QTest::addColumn<int>("dpmy");
    QTest::newRow("Orientation 1, Intel format") << m_prefix + "jpeg_exif_orientation_value_1.jpg" << (int)QImageIOHandler::TransformationNone << 39 << 39;
    QTest::newRow("Orientation 2, Intel format") << m_prefix + "jpeg_exif_orientation_value_2.jpg" << (int)QImageIOHandler::TransformationMirror << 39 << 39;
    QTest::newRow("Orientation 3, Intel format") << m_prefix + "jpeg_exif_orientation_value_3.jpg" << (int)QImageIOHandler::TransformationRotate180 << 39 << 39;
    QTest::newRow("Orientation 4, Intel format") << m_prefix + "jpeg_exif_orientation_value_4.jpg" << (int)QImageIOHandler::TransformationFlip << 39 << 39;
    QTest::newRow("Orientation 5, Intel format") << m_prefix + "jpeg_exif_orientation_value_5.jpg" << (int)QImageIOHandler::TransformationFlipAndRotate90 << 39 << 39;
    QTest::newRow("Orientation 6, Intel format") << m_prefix + "jpeg_exif_orientation_value_6.jpg" << (int)QImageIOHandler::TransformationRotate90 << 39 << 39;
    QTest::newRow("Orientation 6, Motorola format") << m_prefix + "jpeg_exif_orientation_value_6_motorola.jpg" << (int)QImageIOHandler::TransformationRotate90 << 39 << 39;
    QTest::newRow("Orientation 7, Intel format") << m_prefix + "jpeg_exif_orientation_value_7.jpg" << (int)QImageIOHandler::TransformationMirrorAndRotate90 << 39 << 39;
    QTest::newRow("Orientation 8, Intel format") << m_prefix + "jpeg_exif_orientation_value_8.jpg" << (int)QImageIOHandler::TransformationRotate270 << 39 << 39;
}

QT_BEGIN_NAMESPACE
extern void qt_imageTransform(QImage &src, QImageIOHandler::Transformations orient);
QT_END_NAMESPACE
QT_USE_NAMESPACE

void tst_QImage::exifOrientation()
{
    QFETCH(QString, fileName);
    QFETCH(int, orientation);
    QFETCH(int, dpmx);
    QFETCH(int, dpmy);

    QImageReader imageReader(fileName);
    imageReader.setAutoTransform(true);
    QCOMPARE(imageReader.transformation(), orientation);
    QImage img = imageReader.read();
    QCOMPARE(img.dotsPerMeterX(), dpmx);
    QCOMPARE(img.dotsPerMeterY(), dpmy);
    QRgb px;
    QVERIFY(!img.isNull());
    px = img.pixel(0, 0);
    QVERIFY(qRed(px) > 250 && qGreen(px) < 5 && qBlue(px) < 5);

    px = img.pixel(img.width() - 1, 0);
    QVERIFY(qRed(px) < 5 && qGreen(px) < 5 && qBlue(px) > 250);

    QImageReader imageReader2(fileName);
    QCOMPARE(imageReader2.autoTransform(), false);
    QCOMPARE(imageReader2.transformation(), orientation);
    QImage img2 = imageReader2.read();
    qt_imageTransform(img2, imageReader2.transformation());
    QCOMPARE(img, img2);
}

void tst_QImage::exif_QTBUG45865()
{
    QFile file(m_prefix + "jpeg_exif_QTBUG-45865.jpg");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray byteArray = file.readAll();
    QImage image = QImage::fromData(byteArray);
    QCOMPARE(image.size(), QSize(5, 8));
}

void tst_QImage::exifInvalidData_data()
{
    QTest::addColumn<bool>("$never used");
    QTest::newRow("QTBUG-46870");
    QTest::newRow("back_pointers");
    QTest::newRow("past_end");
    QTest::newRow("too_many_ifds");
    QTest::newRow("too_many_tags");
}

void tst_QImage::exifInvalidData()
{
    QImage image;
    QVERIFY(image.load(m_prefix + "jpeg_exif_invalid_data_" + QTest::currentDataTag() + ".jpg"));
    QVERIFY(!image.isNull());
}

void tst_QImage::exifReadComments()
{
    QImage image;
    QVERIFY(image.load(m_prefix + "jpeg_exif_utf8_comment.jpg"));
    QVERIFY(!image.isNull());
    QCOMPARE(image.textKeys().size(), 1);
    QCOMPARE(image.textKeys().first(), "Description");
    // check if exif comment is read as utf-8
    QCOMPARE(image.text("Description"), QString::fromUtf8("some unicode chars: ÖÄÜ€@"));

    QByteArray ba;
    {
        QBuffer buf(&ba);
        QVERIFY(buf.open(QIODevice::WriteOnly));
        QVERIFY(image.save(&buf, "JPG"));
    }
    QVERIFY(!ba.isEmpty());
    image = QImage();
    QCOMPARE(image.textKeys().size(), 0);
    {
        QBuffer buf(&ba);
        QVERIFY(buf.open(QIODevice::ReadOnly));
        QVERIFY(image.load(&buf, "JPG"));
    }
    // compare written (and reread) description text
    QCOMPARE(image.text("Description"), QString::fromUtf8("some unicode chars: ÖÄÜ€@"));
}

static void cleanupFunction(void* info)
{
    bool *called = static_cast<bool*>(info);
    *called = true;
}

void tst_QImage::cleanupFunctions()
{
    QImage bufferImage(64, 64, QImage::Format_ARGB32);
    bufferImage.fill(0);

    bool called;

    {
        called = false;
        {
            QImage image(bufferImage.bits(), bufferImage.width(), bufferImage.height(), bufferImage.format(), cleanupFunction, &called);
        }
        QVERIFY(called);
    }

    {
        called = false;
        QImage *copy = 0;
        {
            QImage image(bufferImage.bits(), bufferImage.width(), bufferImage.height(), bufferImage.format(), cleanupFunction, &called);
            copy = new QImage(image);
        }
        QVERIFY(!called);
        delete copy;
        QVERIFY(called);
    }

}

// test image devicePixelRatio setting and detaching
void tst_QImage::devicePixelRatio()
{
    // create image
    QImage a(64, 64, QImage::Format_ARGB32);
    a.fill(Qt::white);
    QCOMPARE(a.devicePixelRatio(), qreal(1.0));
    QCOMPARE(a.isDetached(), true);

    // copy image
    QImage b = a;
    QCOMPARE(b.devicePixelRatio(), qreal(1.0));
    QCOMPARE(a.isDetached(), false);
    QCOMPARE(b.isDetached(), false);

    // set devicePixelRatio to the current value: does not detach
    a.setDevicePixelRatio(qreal(1.0));
    QCOMPARE(a.isDetached(), false);
    QCOMPARE(b.isDetached(), false);

    // set devicePixelRatio to a new value: may detach (currently
    // does, but we may want to avoid the data copy the future)
    a.setDevicePixelRatio(qreal(2.0));
    QCOMPARE(a.devicePixelRatio(), qreal(2.0));
    QCOMPARE(b.devicePixelRatio(), qreal(1.0));
}

void tst_QImage::rgb30Unpremul()
{
    QImage a(3, 1, QImage::Format_A2RGB30_Premultiplied);
    ((uint*)a.bits())[0] = (3U << 30) | (128 << 20) | (256 << 10) | 512;
    ((uint*)a.bits())[1] = (2U << 30) | (131 << 20) | (259 << 10) | 515;
    ((uint*)a.bits())[2] = (1U << 30) | ( 67 << 20) | (131 << 10) | 259;

    QImage b = a.convertToFormat(QImage::Format_RGB30);
    const uint* bbits = (const uint*)b.bits();
    QCOMPARE(bbits[0], (3U << 30) | (128 << 20) | (256 << 10) | 512);
    QCOMPARE(bbits[1], (3U << 30) | (196 << 20) | (388 << 10) | 772);
    QCOMPARE(bbits[2], (3U << 30) | (201 << 20) | (393 << 10) | 777);
}

void tst_QImage::rgb30Repremul_data()
{
    QTest::addColumn<uint>("color");
    for (int i = 255; i > 0; i -= 15) {
        QTest::addRow("100%% red=%d",  i) << qRgba(i, 0, 0, 0xff);
        QTest::addRow("75%% red=%d",   i) << qRgba(i, 0, 0, 0xc0);
        QTest::addRow("50%% red=%d",   i) << qRgba(i, 0, 0, 0x80);
        QTest::addRow("37.5%% red=%d", i) << qRgba(i, 0, 0, 0x60);
    }
}

void tst_QImage::rgb30Repremul()
{
    QFETCH(uint, color);

    QImage a(1, 1, QImage::Format_ARGB32);
    a.setPixel(0, 0, color);

    QImage b = a.convertToFormat(QImage::Format_A2BGR30_Premultiplied);
    b = b.convertToFormat(QImage::Format_ARGB32);
    uint expectedColor = qUnpremultiply(qPremultiply(color));
    uint newColor = b.pixel(0, 0);
    QVERIFY(qAbs(qRed(newColor) - qRed(expectedColor)) <= 1);
}

void tst_QImage::metadataPassthrough()
{
    QImage a(64, 64, QImage::Format_ARGB32);
    a.fill(Qt::white);
    a.setText(QStringLiteral("Test"), QStringLiteral("Text"));
    a.setDotsPerMeterX(100);
    a.setDotsPerMeterY(80);
    a.setDevicePixelRatio(2.0);

    QImage scaled = a.scaled(QSize(32, 32), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QCOMPARE(scaled.text(QStringLiteral("Test")), a.text(QStringLiteral("Test")));
    QCOMPARE(scaled.dotsPerMeterX(), a.dotsPerMeterX());
    QCOMPARE(scaled.dotsPerMeterY(), a.dotsPerMeterY());
    QCOMPARE(scaled.devicePixelRatio(), a.devicePixelRatio());

    scaled = a.scaled(QSize(128, 128), Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QCOMPARE(scaled.text(QStringLiteral("Test")), a.text(QStringLiteral("Test")));
    QCOMPARE(scaled.dotsPerMeterX(), a.dotsPerMeterX());
    QCOMPARE(scaled.dotsPerMeterY(), a.dotsPerMeterY());
    QCOMPARE(scaled.devicePixelRatio(), a.devicePixelRatio());

    QImage mirrored = a.mirrored();
    QCOMPARE(mirrored.text(QStringLiteral("Test")), a.text(QStringLiteral("Test")));
    QCOMPARE(mirrored.dotsPerMeterX(), a.dotsPerMeterX());
    QCOMPARE(mirrored.dotsPerMeterY(), a.dotsPerMeterY());
    QCOMPARE(mirrored.devicePixelRatio(), a.devicePixelRatio());

    QImage swapped = a.rgbSwapped();
    QCOMPARE(swapped.text(QStringLiteral("Test")), a.text(QStringLiteral("Test")));
    QCOMPARE(swapped.dotsPerMeterX(), a.dotsPerMeterX());
    QCOMPARE(swapped.dotsPerMeterY(), a.dotsPerMeterY());
    QCOMPARE(swapped.devicePixelRatio(), a.devicePixelRatio());
}

void tst_QImage::pixelColor()
{
    QImage argb32(1, 1, QImage::Format_ARGB32);
    QImage argb32pm(1, 1, QImage::Format_ARGB32_Premultiplied);

    QColor c(Qt::red);
    c.setAlpha(128);
    argb32.setPixelColor(QPoint(0, 0), c);
    argb32pm.setPixelColor(QPoint(0, 0), c);
    QCOMPARE(argb32.pixelColor(QPoint(0, 0)), c);
    QCOMPARE(argb32pm.pixelColor(QPoint(0, 0)), c);

    QImage t = argb32.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QCOMPARE(t.pixel(0,0), argb32pm.pixel(0,0));

    // Try specifying an invalid position.
    QTest::ignoreMessage(QtWarningMsg, "QImage::setPixelColor: coordinate (-1,-1) out of range");
    argb32.setPixelColor(-1, -1, QColor(Qt::red));

    // Try setting an invalid color.
    QTest::ignoreMessage(QtWarningMsg, "QImage::setPixelColor: color is invalid");
    argb32.setPixelColor(0, 0, QColor());
}

void tst_QImage::pixel()
{
    {
        QImage mono(1, 1, QImage::Format_Mono);
        QImage monolsb(1, 1, QImage::Format_MonoLSB);
        QImage indexed(1, 1, QImage::Format_Indexed8);

        mono.fill(0);
        monolsb.fill(0);
        indexed.fill(0);

        QCOMPARE(QColor(mono.pixel(0, 0)), QColor(Qt::black));
        QCOMPARE(QColor(monolsb.pixel(0, 0)), QColor(Qt::black));
        indexed.pixel(0, 0); // Don't crash
    }

    {
        uchar a = 0;
        QImage mono(&a, 1, 1, QImage::Format_Mono);
        QImage monolsb(&a, 1, 1, QImage::Format_MonoLSB);
        QImage indexed(&a, 1, 1, QImage::Format_Indexed8);

        mono.pixel(0, 0); // Don't crash
        monolsb.pixel(0, 0); // Don't crash
        indexed.pixel(0, 0); // Don't crash
    }
}

void tst_QImage::ditherGradient_data()
{
    QTest::addColumn<QImage>("image");
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<int>("flags");
    QTest::addColumn<int>("minimumExpectedGradient");

    QImage rgb32(256, 16, QImage::Format_RGB32);
    QLinearGradient gradient(QRectF(rgb32.rect()).topLeft(), QRectF(rgb32.rect()).topRight());
    gradient.setColorAt(0.0, QColor(0, 0, 0));
    gradient.setColorAt(1.0, QColor(255, 255, 255));
    QPainter p;
    p.begin(&rgb32);
    p.fillRect(rgb32.rect(), gradient);
    p.end();

    QTest::newRow("rgb32 -> rgb444 (no dither)") << rgb32 << QImage::Format_RGB444 << 0 << 16;
    QTest::newRow("rgb32 -> rgb444 (dithering)") << rgb32 << QImage::Format_RGB444 << int(Qt::PreferDither | Qt::OrderedDither) << 33;
    QTest::newRow("rgb32 -> argb4444pm (dithering)") << rgb32 << QImage::Format_ARGB4444_Premultiplied << int(Qt::PreferDither | Qt::OrderedDither) << 33;
    QTest::newRow("rgb32 -> rgb16 (no dither)") << rgb32 << QImage::Format_RGB16 << 0 << 32;
    QTest::newRow("rgb32 -> rgb16 (dithering)") << rgb32 << QImage::Format_RGB16 << int(Qt::PreferDither | Qt::OrderedDither) << 65;
    QTest::newRow("rgb32 -> rgb666 (no dither)") << rgb32 << QImage::Format_RGB666 << 0 << 64;
    QTest::newRow("rgb32 -> rgb666 (dithering)") << rgb32 << QImage::Format_RGB666 << int(Qt::PreferDither | Qt::OrderedDither) << 129;

    // Test we get the same results for opaque input in the ARGBPM implementation.
    rgb32 = qMove(rgb32).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QTest::newRow("argb32pm -> argb4444pm (no dither)") << rgb32 << QImage::Format_ARGB4444_Premultiplied << 0 << 16;
    QTest::newRow("argb32pm -> rgb444 (dithering)") << rgb32 << QImage::Format_RGB444 << int(Qt::PreferDither | Qt::OrderedDither) << 33;
    QTest::newRow("argb32pm -> argb4444pm (dithering)") << rgb32 << QImage::Format_ARGB4444_Premultiplied << int(Qt::PreferDither | Qt::OrderedDither) << 33;
    QTest::newRow("argb32pm -> argb8565pm (no dither)") << rgb32 << QImage::Format_ARGB8565_Premultiplied << 0 << 32;
    QTest::newRow("argb32pm -> argb8565pm (dithering)") << rgb32 << QImage::Format_ARGB8565_Premultiplied << int(Qt::PreferDither | Qt::OrderedDither) << 65;
    QTest::newRow("argb32pm -> argb6666pm (no dither)") << rgb32 << QImage::Format_ARGB6666_Premultiplied << 0 << 64;
    QTest::newRow("argb32pm -> argb6666pm (dithering)") << rgb32 << QImage::Format_ARGB6666_Premultiplied << int(Qt::PreferDither | Qt::OrderedDither) << 129;

    QImage rgb30(1024, 16, QImage::Format_RGB30);
    QLinearGradient gradient30(QRectF(rgb30.rect()).topLeft(), QRectF(rgb30.rect()).topRight());
    gradient30.setColorAt(0.0, QColor(0, 0, 0));
    gradient30.setColorAt(1.0, QColor(255, 255, 255));
    p.begin(&rgb30);
    p.fillRect(rgb30.rect(), gradient30);
    p.end();

    QTest::newRow("rgb30 -> rgb32 (no dither)") << rgb30 << QImage::Format_RGB32 << 0 << 256;
    QTest::newRow("rgb30 -> rgb32 (dithering)") << rgb30 << QImage::Format_RGB32 << int(Qt::PreferDither | Qt::OrderedDither) << 513;
    QTest::newRow("rgb30 -> rgb888 (no dither)") << rgb30 << QImage::Format_RGB888 << 0 << 256;
    QTest::newRow("rgb30 -> rgb888 (dithering)") << rgb30 << QImage::Format_RGB888 << int(Qt::PreferDither | Qt::OrderedDither) << 513;
}

void tst_QImage::ditherGradient()
{
    QFETCH(QImage, image);
    QFETCH(QImage::Format, format);
    QFETCH(int, flags);
    QFETCH(int, minimumExpectedGradient);

    QImage converted = image.convertToFormat(format, (Qt::ImageConversionFlags)flags);
    int observedGradientSteps = 0;
    int lastTotal = -1;
    for (int i = 0; i < converted.width(); ++i) {
        int total = 0;
        for (int j = 0; j < converted.height(); ++j) {
            uint c = converted.pixel(i, j);
            QCOMPARE(qAlpha(c), 255);
            total += qRed(c);
        }
        if (total > lastTotal) {
            observedGradientSteps++;
            lastTotal = total;
        }
    }
    QVERIFY(observedGradientSteps >= minimumExpectedGradient);
}

void tst_QImage::reinterpretAsFormat_data()
{
    QTest::addColumn<QImage::Format>("in_format");
    QTest::addColumn<QImage::Format>("out_format");
    QTest::addColumn<QColor>("in_color");
    QTest::addColumn<QColor>("out_color");

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    QTest::newRow("rgb32 -> rgbx8888") << QImage::Format_RGB32 << QImage::Format_RGBX8888 << QColor(Qt::red) << QColor(Qt::blue);
    QTest::newRow("rgba8888 -> argb32") << QImage::Format_RGBA8888 << QImage::Format_ARGB32 << QColor(Qt::red) << QColor(Qt::blue);
    QTest::newRow("argb32pm -> rgba8888pm") << QImage::Format_RGBA8888_Premultiplied << QImage::Format_ARGB32_Premultiplied << QColor(Qt::green) << QColor(Qt::green);
#endif
    QTest::newRow("rgb32 -> argb32") << QImage::Format_RGB32 << QImage::Format_ARGB32 << QColor(Qt::cyan) << QColor(Qt::cyan);
    QTest::newRow("argb32pm -> rgb32") << QImage::Format_ARGB32_Premultiplied << QImage::Format_RGB32 << QColor(Qt::transparent) << QColor(Qt::black);
    QTest::newRow("argb32 -> rgb32") << QImage::Format_ARGB32 << QImage::Format_RGB32 << QColor(255, 0, 0, 127) << QColor(255, 0, 0);
    QTest::newRow("argb32pm -> rgb32") << QImage::Format_ARGB32_Premultiplied << QImage::Format_RGB32 << QColor(255, 0, 0, 127) << QColor(127, 0, 0);
}

void tst_QImage::reinterpretAsFormat()
{
    QFETCH(QImage::Format, in_format);
    QFETCH(QImage::Format, out_format);
    QFETCH(QColor, in_color);
    QFETCH(QColor, out_color);

    QImage image(1, 1, in_format);
    image.setPixelColor(0, 0, in_color);
    QVERIFY(image.reinterpretAsFormat(out_format));
    QCOMPARE(image.pixelColor(0, 0), out_color);
}

void tst_QImage::reinterpretAsFormat2()
{
    const uint imageData[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    {
        QImage image(reinterpret_cast<const uchar*>(imageData), 4, 2, QImage::Format_RGB32);
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::black));
        QVERIFY(image.isDetached());
        QVERIFY(image.reinterpretAsFormat(QImage::Format_ARGB32_Premultiplied));
        QCOMPARE(image.constBits(), reinterpret_cast<const uchar*>(imageData));
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::transparent));

        QVERIFY(!image.reinterpretAsFormat(QImage::Format_Grayscale8));
    }
    {
        QImage image(reinterpret_cast<const uchar*>(imageData), 8, 4, QImage::Format_Indexed8);
        image.setColor(0, qRgb(255, 255, 255));
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::white));
        QVERIFY(image.reinterpretAsFormat(QImage::Format_Grayscale8));
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::black));
        QVERIFY(image.reinterpretAsFormat(QImage::Format_Alpha8));
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::transparent));

        QVERIFY(!image.reinterpretAsFormat(QImage::Format_RGB16));
    }
}

void tst_QImage::complexTransform8bit()
{
    QImage img1(100, 100, QImage::Format_RGB32);
    img1.fill(Qt::green);
    img1 = img1.convertToFormat(QImage::Format_Indexed8);
    QImage img2 = img1.transformed(QTransform().rotate(45), Qt::SmoothTransformation);
    // Currently the format is always QImage::Format_ARGB32_Premultiplied, but it
    // doesn't have to be, and if it becomes indexed this test is no longer be valid.
    QVERIFY(img2.format() > QImage::Format_Indexed8);
    QCOMPARE(img2.colorCount(), 0);
}

#ifdef Q_OS_DARWIN

void tst_QImage::toCGImage_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<bool>("supported");

    // Populate test data with supported status for all QImage formats.
    QSet<QImage::Format> supported =
        { QImage::Format_ARGB32, QImage::Format_RGB32, QImage::Format_RGBA8888_Premultiplied,
          QImage::Format_RGBA8888, QImage::Format_RGBX8888, QImage::Format_ARGB32_Premultiplied };

    for (int i = QImage::Format_Invalid; i < QImage::Format_Grayscale8; ++i) {
        QTest::addRow("%s", formatToString(QImage::Format(i)).data())
            << QImage::Format(i) << supported.contains(QImage::Format(i));
    }
}

// Verify that toCGImage() returns a valid CGImageRef for supported image formats.
void tst_QImage::toCGImage()
{
    QFETCH(QImage::Format, format);
    QFETCH(bool, supported);

    QImage qimage(64, 64, format);
    qimage.fill(Qt::red);

    CGImageRef cgimage = qimage.toCGImage();
    QCOMPARE(cgimage != nullptr, supported);

    CGImageRelease(cgimage);
}

#endif

void tst_QImage::hugeQImage()
{
#if Q_PROCESSOR_WORDSIZE < 8
    QSKIP("Test only makes sense on 64-bit machines");
#else
    QImage image(25000, 25000, QImage::Format_RGB32);

    QVERIFY(!image.isNull());
    QCOMPARE(image.height(), 25000);
    QCOMPARE(image.width(), 25000);
    QCOMPARE(image.sizeInBytes(), qsizetype(25000)*25000*4);
    QCOMPARE(image.bytesPerLine(), 25000 * 4);

    QCOMPARE(image.constScanLine(24990), image.constBits() + qsizetype(25000)*24990*4);

    image.setPixel(20000, 24990, 0xffaabbcc);
    QCOMPARE(image.pixel(20000, 24990), 0xffaabbcc);
    QCOMPARE((reinterpret_cast<const unsigned int *>(image.constScanLine(24990)))[20000], 0xffaabbcc);

    QImage canvas(100, 100, QImage::Format_RGB32);
    QPainter painter(&canvas);
    painter.drawImage(0,0, image, 19950, 24900, 100, 100);
    painter.end();
    QCOMPARE(reinterpret_cast<const unsigned int *>(canvas.constScanLine(90))[50], 0xffaabbcc);
#endif
}

void tst_QImage::convertColorTable()
{
    QImage image(10, 10, QImage::Format_Indexed8);
    image.setColor(0, 0x80ffffff);
    image.fill(0);
    QImage argb32 = image.convertToFormat(QImage::Format_ARGB32);
    QCOMPARE(argb32.pixel(0,0), 0x80ffffff);
    QImage argb32pm = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QCOMPARE(argb32pm.pixel(0,0), 0x80808080);
    QImage rgb32 = image.convertToFormat(QImage::Format_RGB32);
    QCOMPARE(rgb32.pixel(0,0), 0xffffffff);
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
QT_BEGIN_NAMESPACE
Q_GUI_EXPORT HBITMAP qt_imageToWinHBITMAP(const QImage &p, int hbitmapFormat = 0);
Q_GUI_EXPORT QImage qt_imageFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat = 0);
QT_END_NAMESPACE

static inline QColor COLORREFToQColor(COLORREF cr)
{
    return QColor(GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

void tst_QImage::toWinHBITMAP_data()
{
    QTest::addColumn<QImage::Format>("format");
    QTest::addColumn<QColor>("color");
    QTest::addColumn<QColor>("bottomRightColor");

    const QColor red(Qt::red);
    const QColor green(Qt::green);
    const QColor blue(Qt::blue);
    const QColor gray(Qt::gray);
    const QColor gray555(0x5a, 0x5a, 0x5a); // Note: Interpolation 8<->5 bit occurs.
    const QColor white(Qt::white);
    const QColor black(Qt::black);

    QTest::newRow("argb32p-red")    << QImage::Format_ARGB32_Premultiplied << red << gray;
    QTest::newRow("argb32p-green")  << QImage::Format_ARGB32_Premultiplied << green << gray;
    QTest::newRow("argb32p-blue")   << QImage::Format_ARGB32_Premultiplied << blue << gray;
    QTest::newRow("rgb888-red")     << QImage::Format_RGB888 << red << gray;
    QTest::newRow("rgb888-green")   << QImage::Format_RGB888 << green << gray;
    QTest::newRow("rgb888-blue")    << QImage::Format_RGB888 << blue << gray;
    QTest::newRow("indexed8-red")   << QImage::Format_Indexed8 << red << gray;
    QTest::newRow("indexed8-green") << QImage::Format_Indexed8 << green << gray;
    QTest::newRow("indexed8-blue")  << QImage::Format_Indexed8 << blue << gray;
    QTest::newRow("rgb555-red")     << QImage::Format_RGB555 << red << gray555;
    QTest::newRow("rgb555-green")   << QImage::Format_RGB555 << green << gray555;
    QTest::newRow("rgb555-blue")    << QImage::Format_RGB555 << blue << gray555;
    QTest::newRow("mono")           << QImage::Format_Mono << white << black;
}

// Test image filled with color, black pixel at botttom right corner.
static inline QImage createTestImage(QImage::Format format, int width, int height,
                                     const QColor &fillColor, const QColor &bottomRightColor)
{
    QImage image(QSize(width, height), format);
    image.fill(fillColor);
    QPainter painter(&image);
    QPen pen = painter.pen();
    pen.setColor(bottomRightColor);
    painter.setPen(pen);
    painter.drawPoint(width -1, height - 1);
    return image;
}

void tst_QImage::toWinHBITMAP()
{
    static const int width = 73;
    static const int height = 57;

    QFETCH(QImage::Format, format);
    QFETCH(QColor, color);
    QFETCH(QColor, bottomRightColor);

    // Cannot paint on indexed/mono images.
    const QImage image = format == QImage::Format_Indexed8 || format == QImage::Format_Mono
        ? createTestImage(QImage::Format_RGB32, width, height, color, bottomRightColor).convertToFormat(format)
        : createTestImage(format, width, height, color, bottomRightColor);

    const HBITMAP bitmap = qt_imageToWinHBITMAP(image);

    QVERIFY(bitmap != 0);

    // Verify size
    BITMAP bitmapInfo;
    memset(&bitmapInfo, 0, sizeof(BITMAP));

    const int res = GetObject(bitmap, sizeof(BITMAP), &bitmapInfo);
    QVERIFY(res);
    QCOMPARE(width, int(bitmapInfo.bmWidth));
    QCOMPARE(height, int(bitmapInfo.bmHeight));

    const HDC displayDc = GetDC(0);
    const HDC bitmapDc = CreateCompatibleDC(displayDc);

    const HBITMAP nullBitmap = static_cast<HBITMAP>(SelectObject(bitmapDc, bitmap));

    QCOMPARE(COLORREFToQColor(GetPixel(bitmapDc, 0, 0)), color);
    QCOMPARE(COLORREFToQColor(GetPixel(bitmapDc, width - 1, 3)), color);
    QCOMPARE(COLORREFToQColor(GetPixel(bitmapDc, 3, height - 1)), color);
    QCOMPARE(COLORREFToQColor(GetPixel(bitmapDc, width - 1, height - 1)), bottomRightColor);

    const QImage convertedBack = qt_imageFromWinHBITMAP(bitmap);
    QCOMPARE(convertedBack.convertToFormat(QImage::Format_ARGB32_Premultiplied),
             image.convertToFormat(QImage::Format_ARGB32_Premultiplied));

    // Clean up
    SelectObject(bitmapDc, nullBitmap);
    DeleteObject(bitmap);
    DeleteDC(bitmapDc);
    ReleaseDC(0, displayDc);
}
#endif // Q_OS_WIN && !Q_OS_WINRT

QTEST_GUILESS_MAIN(tst_QImage)
#include "tst_qimage.moc"
