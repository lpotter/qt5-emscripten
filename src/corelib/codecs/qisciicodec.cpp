/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
#include "qisciicodec_p.h"
#include "qtextcodec_p.h"
#include "qlist.h"

#ifndef QT_NO_CODECS

QT_BEGIN_NAMESPACE

/*!
    \class QIsciiCodec
    \inmodule QtCore
    \brief The QIsciiCodec class provides conversion to and from the ISCII encoding.

    \internal
*/


struct Codecs {
    const char name[10];
    ushort base;
};

static const Codecs codecs [] = {
    { "iscii-dev", 0x900 },
    { "iscii-bng", 0x980 },
    { "iscii-pnj", 0xa00 },
    { "iscii-gjr", 0xa80 },
    { "iscii-ori", 0xb00 },
    { "iscii-tml", 0xb80 },
    { "iscii-tlg", 0xc00 },
    { "iscii-knd", 0xc80 },
    { "iscii-mlm", 0xd00 }
};

QTextCodec *QIsciiCodec::create(const char *name)
{
    for (int i = 0; i < 9; ++i) {
        if (qTextCodecNameMatch(name, codecs[i].name))
            return new QIsciiCodec(i);
    }
    return 0;
}

QIsciiCodec::~QIsciiCodec()
{
}

QByteArray QIsciiCodec::name() const
{
  return codecs[idx].name;
}

int QIsciiCodec::mibEnum() const
{
    /* There is no MIBEnum for Iscii */
    return -3000-idx;
}

static const uchar inv = 0xFF;

/* iscii range from 0xa0 - 0xff */
static const uchar iscii_to_uni_table[0x60] = {
    0x00, 0x01, 0x02, 0x03,
    0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0e,
    0x0f, 0x20, 0x0d, 0x12,

    0x13, 0x14, 0x11, 0x15,
    0x16, 0x17, 0x18, 0x19,
    0x1a, 0x1b, 0x1c, 0x1d,
    0x1e, 0x1f, 0x20, 0x21,

    0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x28, 0x29,
    0x2a, 0x2b, 0x2c, 0x2d,
    0x2e, 0x2f, 0x5f, 0x30,

    0x31, 0x32, 0x33, 0x34,
    0x35, 0x36, 0x37, 0x38,
    0x39,  inv, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43,

    0x46, 0x47, 0x48, 0x45,
    0x4a, 0x4b, 0x4c, 0x49,
    0x4d, 0x3c, 0x64, 0x00,
    0x00, 0x00, 0x00, 0x00,

    0x00, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x6b, 0x6c,
    0x6d, 0x6e, 0x6f, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static const uchar uni_to_iscii_table[0x80] = {
    0x00, 0xa1, 0xa2, 0xa3,
    0x00, 0xa4, 0xa5, 0xa6,
    0xa7, 0xa8, 0xa9, 0xaa,
    0x00, 0xae, 0xab, 0xac,

    0xad, 0xb2, 0xaf, 0xb0,
    0xb1, 0xb3, 0xb4, 0xb5,
    0xb6, 0xb7, 0xb8, 0xb9,
    0xba, 0xbb, 0xbc, 0xbd,

    0xbe, 0xbf, 0xc0, 0xc1,
    0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9,
    0xca, 0xcb, 0xcc, 0xcd,

    0xcf, 0xd0, 0xd1, 0xd2,
    0xd3, 0xd4, 0xd5, 0xd6,
    0xd7, 0xd8, 0x00, 0x00,
    0xe9, 0x00, 0xda, 0xdb,

    0xdc, 0xdd, 0xde, 0xdf,
    0x00, 0xe3, 0xe0, 0xe1,
    0xe2, 0xe7, 0xe4, 0xe5,
    0xe6, 0xe8, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x01, 0x02, 0x03, 0x04, // decomposable into the uc codes listed here + nukta
    0x05, 0x06, 0x07, 0xce,

    0x00, 0x00, 0x00, 0x00,
    0xea, 0x08, 0xf1, 0xf2,
    0xf3, 0xf4, 0xf5, 0xf6,
    0xf7, 0xf8, 0xf9, 0xfa,

    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

static const uchar uni_to_iscii_pairs[] = {
    0x00, 0x00,
    0x15, 0x3c, // 0x958
    0x16, 0x3c, // 0x959
    0x17, 0x3c, // 0x95a
    0x1c, 0x3c, // 0x95b
    0x21, 0x3c, // 0x95c
    0x22, 0x3c, // 0x95d
    0x2b, 0x3c, // 0x95e
    0x64, 0x64  // 0x965
};


QByteArray QIsciiCodec::convertFromUnicode(const QChar *uc, int len, ConverterState *state) const
{
    char replacement = '?';
    bool halant = false;
    if (state) {
        if (state->flags & ConvertInvalidToNull)
            replacement = 0;
        halant = state->state_data[0];
    }
    int invalid = 0;

    QByteArray result(2 * len, Qt::Uninitialized); //worst case

    uchar *ch = reinterpret_cast<uchar *>(result.data());

    const int base = codecs[idx].base;

    for (int i =0; i < len; ++i) {
        const ushort codePoint = uc[i].unicode();

        /* The low 7 bits of ISCII is plain ASCII. However, we go all the
         * way up to 0xA0 such that we can roundtrip with convertToUnicode()'s
         * behavior. */
        if(codePoint < 0xA0) {
            *ch++ = static_cast<uchar>(codePoint);
            continue;
        }

        const int pos = codePoint - base;
        if (pos > 0 && pos < 0x80) {
            uchar iscii = uni_to_iscii_table[pos];
            if (iscii > 0x80) {
                *ch++ = iscii;
            } else if (iscii) {
                Q_ASSERT((2 * iscii) < (sizeof(uni_to_iscii_pairs) / sizeof(uni_to_iscii_pairs[0])));
                const uchar *pair = uni_to_iscii_pairs + 2*iscii;
                *ch++ = *pair++;
                *ch++ = *pair++;
            } else {
                *ch++ = replacement;
                ++invalid;
            }
        } else {
            if (uc[i].unicode() == 0x200c) { // ZWNJ
                if (halant)
                    // Consonant Halant ZWNJ -> Consonant Halant Halant
                    *ch++ = 0xe8;
            } else if (uc[i].unicode() == 0x200d) { // ZWJ
                if (halant)
                    // Consonant Halant ZWJ -> Consonant Halant Nukta
                    *ch++ = 0xe9;
            } else {
                *ch++ = replacement;
                ++invalid;
            }
        }
        halant = (pos == 0x4d);
    }
    result.truncate(ch - (uchar *)result.data());

    if (state) {
        state->invalidChars += invalid;
        state->state_data[0] = halant;
    }
    return result;
}

QString QIsciiCodec::convertToUnicode(const char* chars, int len, ConverterState *state) const
{
    bool halant = false;
    if (state) {
        halant = state->state_data[0];
    }

    QString result(len, Qt::Uninitialized);
    QChar *uc = result.data();

    const int base = codecs[idx].base;

    for (int i = 0; i < len; ++i) {
        ushort ch = (uchar) chars[i];
        if (ch < 0xa0)
            *uc++ = ch;
        else {
            ushort c = iscii_to_uni_table[ch - 0xa0];
            if (halant && (c == inv || c == 0xe9)) {
                // Consonant Halant inv -> Consonant Halant ZWJ
                // Consonant Halant Nukta -> Consonant Halant ZWJ
                *uc++ = QChar(0x200d);
            } else if (halant && c == 0xe8) {
                // Consonant Halant Halant -> Consonant Halant ZWNJ
                *uc++ = QChar(0x200c);
            } else {
                *uc++ = QChar(c+base);
            }
        }
        halant = ((uchar)chars[i] == 0xe8);
    }
    result.resize(uc - result.unicode());

    if (state) {
        state->state_data[0] = halant;
    }
    return result;
}

QT_END_NAMESPACE

#endif // QT_NO_CODECS
