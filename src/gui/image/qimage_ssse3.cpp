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

#include <qimage.h>
#include <private/qimage_p.h>
#include <private/qsimd_p.h>

#ifdef QT_COMPILER_SUPPORTS_SSSE3

QT_BEGIN_NAMESPACE

// Convert a scanline of RGB888 (src) to RGB32 (dst)
// src must be at least len * 3 bytes
// dst must be at least len * 4 bytes
Q_GUI_EXPORT void QT_FASTCALL qt_convert_rgb888_to_rgb32_ssse3(quint32 *dst, const uchar *src, int len)
{
    int i = 0;

    // Prologue, align dst to 16 bytes.
    ALIGNMENT_PROLOGUE_16BYTES(dst, i, len) {
        dst[i] = qRgb(src[0], src[1], src[2]);
        src += 3;
    }

    // Mask the 4 first colors of the RGB888 vector
    const __m128i shuffleMask = _mm_set_epi8(char(0xff), 9, 10, 11, char(0xff), 6, 7, 8, char(0xff), 3, 4, 5, char(0xff), 0, 1, 2);

    // Mask the 4 last colors of a RGB888 vector with an offset of 1 (so the last 3 bytes are RGB)
    const __m128i shuffleMaskEnd = _mm_set_epi8(char(0xff), 13, 14, 15, char(0xff), 10, 11, 12, char(0xff), 7, 8, 9, char(0xff), 4, 5, 6);

    // Mask to have alpha = 0xff
    const __m128i alphaMask = _mm_set1_epi32(0xff000000);

    const __m128i *inVectorPtr = (const __m128i *)src;
    __m128i *dstVectorPtr = (__m128i *)(dst + i);

    for (; i < (len - 15); i += 16) { // one iteration in the loop converts 16 pixels
        /*
         RGB888 has 5 pixels per vector, + 1 byte from the next pixel. The idea here is
         to load vectors of RGB888 and use palignr to select a vector out of two vectors.

         After 3 loads of RGB888 and 3 stores of RGB32, we have 4 pixels left in the last
         vector of RGB888, we can mask it directly to get a last store or RGB32. After that,
         the first next byte is a R, and we can loop for the next 16 pixels.

         The conversion itself is done with a byte permutation (pshufb).
         */
        __m128i firstSrcVector = _mm_lddqu_si128(inVectorPtr);
        __m128i outputVector = _mm_shuffle_epi8(firstSrcVector, shuffleMask);
        _mm_store_si128(dstVectorPtr, _mm_or_si128(outputVector, alphaMask));
        ++inVectorPtr;
        ++dstVectorPtr;

        // There are 4 unused bytes left in srcVector, we need to load the next 16 bytes
        // and load the next input with palignr
        __m128i secondSrcVector = _mm_lddqu_si128(inVectorPtr);
        __m128i srcVector = _mm_alignr_epi8(secondSrcVector, firstSrcVector, 12);
        outputVector = _mm_shuffle_epi8(srcVector, shuffleMask);
        _mm_store_si128(dstVectorPtr, _mm_or_si128(outputVector, alphaMask));
        ++inVectorPtr;
        ++dstVectorPtr;
        firstSrcVector = secondSrcVector;

        // We now have 8 unused bytes left in firstSrcVector
        secondSrcVector = _mm_lddqu_si128(inVectorPtr);
        srcVector = _mm_alignr_epi8(secondSrcVector, firstSrcVector, 8);
        outputVector = _mm_shuffle_epi8(srcVector, shuffleMask);
        _mm_store_si128(dstVectorPtr, _mm_or_si128(outputVector, alphaMask));
        ++inVectorPtr;
        ++dstVectorPtr;

        // There are now 12 unused bytes in firstSrcVector.
        // We can mask them directly, almost there.
        outputVector = _mm_shuffle_epi8(secondSrcVector, shuffleMaskEnd);
        _mm_store_si128(dstVectorPtr, _mm_or_si128(outputVector, alphaMask));
        ++dstVectorPtr;
    }
    src = (const uchar *)inVectorPtr;

    SIMD_EPILOGUE(i, len, 15) {
        dst[i] = qRgb(src[0], src[1], src[2]);
        src += 3;
    }
}

void convert_RGB888_to_RGB32_ssse3(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_RGB888);
    Q_ASSERT(dest->format == QImage::Format_RGB32 || dest->format == QImage::Format_ARGB32 || dest->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const uchar *src_data = (uchar *) src->data;
    quint32 *dest_data = (quint32 *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        qt_convert_rgb888_to_rgb32_ssse3(dest_data, src_data, src->width);
        src_data += src->bytes_per_line;
        dest_data = (quint32 *)((uchar*)dest_data + dest->bytes_per_line);
    }
}

QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_SSSE3
