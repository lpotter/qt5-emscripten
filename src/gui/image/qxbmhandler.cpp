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

#include <qplatformdefs.h>
#include "private/qxbmhandler_p.h"

#ifndef QT_NO_IMAGEFORMAT_XBM

#include <qimage.h>
#include <qiodevice.h>
#include <qvariant.h>

#include <stdio.h>
#include <ctype.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  X bitmap image read/write functions
 *****************************************************************************/

static inline int hex2byte(char *p)
{
    return ((isdigit((uchar) *p) ? *p - '0' : toupper((uchar) *p) - 'A' + 10) << 4) |
           (isdigit((uchar) *(p+1)) ? *(p+1) - '0' : toupper((uchar) *(p+1)) - 'A' + 10);
}

static bool read_xbm_header(QIODevice *device, int& w, int& h)
{
    const int buflen = 300;
    const int maxlen = 4096;
    char buf[buflen + 1];
    QRegExp r1(QLatin1String("^#define[ \t]+[a-zA-Z0-9._]+[ \t]+"));
    QRegExp r2(QLatin1String("[0-9]+"));

    qint64 readBytes = 0;
    qint64 totalReadBytes = 0;

    buf[0] = '\0';

    // skip initial comment, if any
    while (buf[0] != '#') {
        readBytes = device->readLine(buf, buflen);

        // if readBytes >= buflen, it's very probably not a C file
        if (readBytes <= 0 || readBytes >= buflen -1)
            return false;

        // limit xbm headers to the first 4k in the file to prevent
        // excessive reads on non-xbm files
        totalReadBytes += readBytes;
        if (totalReadBytes >= maxlen)
            return false;
    }

    buf[readBytes - 1] = '\0';
    QString sbuf;
    sbuf = QString::fromLatin1(buf);

    // "#define .._width <num>"
    if (r1.indexIn(sbuf) == 0 &&
         r2.indexIn(sbuf, r1.matchedLength()) == r1.matchedLength())
        w = QByteArray(&buf[r1.matchedLength()]).trimmed().toInt();

    // "#define .._height <num>"
    readBytes = device->readLine(buf, buflen);
    if (readBytes <= 0)
        return false;
    buf[readBytes - 1] = '\0';

    sbuf = QString::fromLatin1(buf);

    if (r1.indexIn(sbuf) == 0 &&
         r2.indexIn(sbuf, r1.matchedLength()) == r1.matchedLength())
        h = QByteArray(&buf[r1.matchedLength()]).trimmed().toInt();

    // format error
    if (w <= 0 || w > 32767 || h <= 0 || h > 32767)
        return false;

    return true;
}

static bool read_xbm_body(QIODevice *device, int w, int h, QImage *outImage)
{
    const int buflen = 300;
    char buf[buflen + 1];

    qint64 readBytes = 0;

    char *p;

    // scan for database
    do {
        if ((readBytes = device->readLine(buf, buflen)) <= 0) {
            // end of file
            return false;
        }

        buf[readBytes] = '\0';
        p = strstr(buf, "0x");
    } while (!p);

    if (outImage->size() != QSize(w, h) || outImage->format() != QImage::Format_MonoLSB) {
        *outImage = QImage(w, h, QImage::Format_MonoLSB);
        if (outImage->isNull())
            return false;
    }

    outImage->fill(Qt::color0);       // in case the image data does not cover the full image

    outImage->setColorCount(2);
    outImage->setColor(0, qRgb(255,255,255));        // white
    outImage->setColor(1, qRgb(0,0,0));                // black

    int           x = 0, y = 0;
    uchar *b = outImage->scanLine(0);
    w = (w+7)/8;                                // byte width

    while (y < h) {                                // for all encoded bytes...
        if (p) {                                // p = "0x.."
            *b++ = hex2byte(p+2);
            p += 2;
            if (++x == w && ++y < h) {
                b = outImage->scanLine(y);
                x = 0;
            }
            p = strstr(p, "0x");
        } else {                                // read another line
            if ((readBytes = device->readLine(buf,buflen)) <= 0)        // EOF ==> truncated image
                break;
            buf[readBytes] = '\0';
            p = strstr(buf, "0x");
        }
    }

    return true;
}

static bool read_xbm_image(QIODevice *device, QImage *outImage)
{
    int w = 0, h = 0;
    if (!read_xbm_header(device, w, h))
        return false;
    return read_xbm_body(device, w, h, outImage);
}

static bool write_xbm_image(const QImage &sourceImage, QIODevice *device, const QString &fileName)
{
    QImage image = sourceImage;
    int        w = image.width();
    int        h = image.height();
    int        i;
    QString    s = fileName; // get file base name
    int        msize = s.length() + 100;
    char *buf = new char[msize];

    qsnprintf(buf, msize, "#define %s_width %d\n", s.toUtf8().data(), w);
    device->write(buf, qstrlen(buf));
    qsnprintf(buf, msize, "#define %s_height %d\n", s.toUtf8().data(), h);
    device->write(buf, qstrlen(buf));
    qsnprintf(buf, msize, "static char %s_bits[] = {\n ", s.toUtf8().data());
    device->write(buf, qstrlen(buf));

    if (image.format() != QImage::Format_MonoLSB)
        image = image.convertToFormat(QImage::Format_MonoLSB);

    bool invert = qGray(image.color(0)) < qGray(image.color(1));
    char hexrep[16];
    for (i=0; i<10; i++)
        hexrep[i] = '0' + i;
    for (i=10; i<16; i++)
        hexrep[i] = 'a' -10 + i;
    if (invert) {
        char t;
        for (i=0; i<8; i++) {
            t = hexrep[15-i];
            hexrep[15-i] = hexrep[i];
            hexrep[i] = t;
        }
    }
    int bcnt = 0;
    char *p = buf;
    int bpl = (w+7)/8;
    for (int y = 0; y < h; ++y) {
        const uchar *b = image.constScanLine(y);
        for (i = 0; i < bpl; ++i) {
            *p++ = '0'; *p++ = 'x';
            *p++ = hexrep[*b >> 4];
            *p++ = hexrep[*b++ & 0xf];

            if (i < bpl - 1 || y < h - 1) {
                *p++ = ',';
                if (++bcnt > 14) {
                    *p++ = '\n';
                    *p++ = ' ';
                    *p   = '\0';
                    if ((int)qstrlen(buf) != device->write(buf, qstrlen(buf))) {
                        delete [] buf;
                        return false;
                    }
                    p = buf;
                    bcnt = 0;
                }
            }
        }
    }
#ifdef Q_CC_MSVC
    strcpy_s(p, sizeof(" };\n"), " };\n");
#else
    strcpy(p, " };\n");
#endif
    if ((int)qstrlen(buf) != device->write(buf, qstrlen(buf))) {
        delete [] buf;
        return false;
    }

    delete [] buf;
    return true;
}

QXbmHandler::QXbmHandler()
    : state(Ready)
{
}

bool QXbmHandler::readHeader()
{
    state = Error;
    if (!read_xbm_header(device(), width, height))
        return false;
    state = ReadHeader;
    return true;
}

bool QXbmHandler::canRead() const
{
    if (state == Ready && !canRead(device()))
        return false;

    if (state != Error) {
        setFormat("xbm");
        return true;
    }

    return false;
}

bool QXbmHandler::canRead(QIODevice *device)
{
    QImage image;

    // it's impossible to tell whether we can load an XBM or not when
    // it's from a sequential device, as the only way to do it is to
    // attempt to parse the whole image.
    if (device->isSequential())
        return false;

    qint64 oldPos = device->pos();
    bool success = read_xbm_image(device, &image);
    device->seek(oldPos);

    return success;
}

bool QXbmHandler::read(QImage *image)
{
    if (state == Error)
        return false;

    if (state == Ready && !readHeader()) {
        state = Error;
        return false;
    }

    if (!read_xbm_body(device(), width, height, image)) {
        state = Error;
        return false;
    }

    state = Ready;
    return true;
}

bool QXbmHandler::write(const QImage &image)
{
    return write_xbm_image(image, device(), fileName);
}

bool QXbmHandler::supportsOption(ImageOption option) const
{
    return option == Name
        || option == Size
        || option == ImageFormat;
}

QVariant QXbmHandler::option(ImageOption option) const
{
    if (option == Name) {
        return fileName;
    } else if (option == Size) {
        if (state == Error)
            return QVariant();
        if (state == Ready && !const_cast<QXbmHandler*>(this)->readHeader())
            return QVariant();
        return QSize(width, height);
    } else if (option == ImageFormat) {
        return QImage::Format_MonoLSB;
    }
    return QVariant();
}

void QXbmHandler::setOption(ImageOption option, const QVariant &value)
{
    if (option == Name)
        fileName = value.toString();
}

QByteArray QXbmHandler::name() const
{
    return "xbm";
}

QT_END_NAMESPACE

#endif // QT_NO_IMAGEFORMAT_XBM
