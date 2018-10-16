/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#include "quuid.h"

#include "qcryptographichash.h"
#include "qdatastream.h"
#include "qdebug.h"
#include "qendian.h"
#include "qrandom.h"
#include "private/qtools_p.h"

QT_BEGIN_NAMESPACE

// 16 bytes (a uint, two shorts and a uchar[8]), each represented by two hex
// digits; plus four dashes and a pair of enclosing brace: 16*2 + 4 + 2 = 38.
enum { MaxStringUuidLength = 38 };

template <class Integral>
void _q_toHex(char *&dst, Integral value)
{
    value = qToBigEndian(value);

    const char* p = reinterpret_cast<const char*>(&value);

    for (uint i = 0; i < sizeof(Integral); ++i, dst += 2) {
        dst[0] = QtMiscUtils::toHexLower((p[i] >> 4) & 0xf);
        dst[1] = QtMiscUtils::toHexLower(p[i] & 0xf);
    }
}

template <class Integral>
bool _q_fromHex(const char *&src, Integral &value)
{
    value = 0;

    for (uint i = 0; i < sizeof(Integral) * 2; ++i) {
        uint ch = *src++;
        int tmp = QtMiscUtils::fromHex(ch);
        if (tmp == -1)
            return false;

        value = value * 16 + tmp;
    }

    return true;
}

static char *_q_uuidToHex(const QUuid &uuid, char *dst, QUuid::StringFormat mode = QUuid::WithBraces)
{
    if ((mode & QUuid::WithoutBraces) == 0)
        *dst++ = '{';
    _q_toHex(dst, uuid.data1);
    if ((mode & QUuid::Id128) != QUuid::Id128)
        *dst++ = '-';
    _q_toHex(dst, uuid.data2);
    if ((mode & QUuid::Id128) != QUuid::Id128)
        *dst++ = '-';
    _q_toHex(dst, uuid.data3);
    if ((mode & QUuid::Id128) != QUuid::Id128)
        *dst++ = '-';
    for (int i = 0; i < 2; i++)
        _q_toHex(dst, uuid.data4[i]);
    if ((mode & QUuid::Id128) != QUuid::Id128)
        *dst++ = '-';
    for (int i = 2; i < 8; i++)
        _q_toHex(dst, uuid.data4[i]);
    if ((mode & QUuid::WithoutBraces) == 0)
        *dst++ = '}';
    return dst;
}

/*!
    \internal

    Parses the string representation of a UUID (with optional surrounding "{}")
    by reading at most MaxStringUuidLength (38) characters from \a src, which
    may be \c nullptr. Stops at the first invalid character (which includes a
    premature NUL).

    Returns the successfully parsed QUuid, or a null QUuid in case of failure.
*/
Q_NEVER_INLINE
static QUuid _q_uuidFromHex(const char *src)
{
    uint d1;
    ushort d2, d3;
    uchar d4[8];

    if (src) {
        if (*src == '{')
            src++;
        if (Q_LIKELY(   _q_fromHex(src, d1)
                     && *src++ == '-'
                     && _q_fromHex(src, d2)
                     && *src++ == '-'
                     && _q_fromHex(src, d3)
                     && *src++ == '-'
                     && _q_fromHex(src, d4[0])
                     && _q_fromHex(src, d4[1])
                     && *src++ == '-'
                     && _q_fromHex(src, d4[2])
                     && _q_fromHex(src, d4[3])
                     && _q_fromHex(src, d4[4])
                     && _q_fromHex(src, d4[5])
                     && _q_fromHex(src, d4[6])
                     && _q_fromHex(src, d4[7]))) {
            return QUuid(d1, d2, d3, d4[0], d4[1], d4[2], d4[3], d4[4], d4[5], d4[6], d4[7]);
        }
    }

    return QUuid();
}

static QUuid createFromName(const QUuid &ns, const QByteArray &baseData, QCryptographicHash::Algorithm algorithm, int version)
{
    QByteArray hashResult;

    // create a scope so later resize won't reallocate
    {
        QCryptographicHash hash(algorithm);
        hash.addData(ns.toRfc4122());
        hash.addData(baseData);
        hashResult = hash.result();
    }
    hashResult.resize(16); // Sha1 will be too long

    QUuid result = QUuid::fromRfc4122(hashResult);

    result.data3 &= 0x0FFF;
    result.data3 |= (version << 12);
    result.data4[0] &= 0x3F;
    result.data4[0] |= 0x80;

    return result;
}

/*!
    \class QUuid
    \inmodule QtCore
    \brief The QUuid class stores a Universally Unique Identifier (UUID).

    \reentrant

    Using \e{U}niversally \e{U}nique \e{ID}entifiers (UUID) is a
    standard way to uniquely identify entities in a distributed
    computing environment. A UUID is a 16-byte (128-bit) number
    generated by some algorithm that is meant to guarantee that the
    UUID will be unique in the distributed computing environment where
    it is used. The acronym GUID is often used instead, \e{G}lobally
    \e{U}nique \e{ID}entifiers, but it refers to the same thing.

    \target Variant field
    Actually, the GUID is one \e{variant} of UUID. Multiple variants
    are in use. Each UUID contains a bit field that specifies which
    type (variant) of UUID it is. Call variant() to discover which
    type of UUID an instance of QUuid contains. It extracts the three
    most significant bits of byte 8 of the 16 bytes. In QUuid, byte 8
    is \c{QUuid::data4[0]}. If you create instances of QUuid using the
    constructor that accepts all the numeric values as parameters, use
    the following table to set the three most significant bits of
    parameter \c{b1}, which becomes \c{QUuid::data4[0]} and contains
    the variant field in its three most significant bits. In the
    table, 'x' means \e {don't care}.

    \table
    \header
    \li msb0
    \li msb1
    \li msb2
    \li Variant

    \row
    \li 0
    \li x
    \li x
    \li NCS (Network Computing System)

    \row
    \li 1
    \li 0
    \li x
    \li DCE (Distributed Computing Environment)

    \row
    \li 1
    \li 1
    \li 0
    \li Microsoft (GUID)

    \row
    \li 1
    \li 1
    \li 1
    \li Reserved for future expansion

    \endtable

    \target Version field
    If variant() returns QUuid::DCE, the UUID also contains a
    \e{version} field in the four most significant bits of
    \c{QUuid::data3}, and you can call version() to discover which
    version your QUuid contains. If you create instances of QUuid
    using the constructor that accepts all the numeric values as
    parameters, use the following table to set the four most
    significant bits of parameter \c{w2}, which becomes
    \c{QUuid::data3} and contains the version field in its four most
    significant bits.

    \table
    \header
    \li msb0
    \li msb1
    \li msb2
    \li msb3
    \li Version

    \row
    \li 0
    \li 0
    \li 0
    \li 1
    \li Time

    \row
    \li 0
    \li 0
    \li 1
    \li 0
    \li Embedded POSIX

    \row
    \li 0
    \li 0
    \li 1
    \li 1
    \li Md5(Name)

    \row
    \li 0
    \li 1
    \li 0
    \li 0
    \li Random

    \row
    \li 0
    \li 1
    \li 0
    \li 1
    \li Sha1

    \endtable

    The field layouts for the DCE versions listed in the table above
    are specified in the \l{http://www.ietf.org/rfc/rfc4122.txt}
    {Network Working Group UUID Specification}.

    Most platforms provide a tool for generating new UUIDs, e.g. \c
    uuidgen and \c guidgen. You can also use createUuid().  UUIDs
    generated by createUuid() are of the random type.  Their
    QUuid::Version bits are set to QUuid::Random, and their
    QUuid::Variant bits are set to QUuid::DCE. The rest of the UUID is
    composed of random numbers. Theoretically, this means there is a
    small chance that a UUID generated by createUuid() will not be
    unique. But it is
    \l{http://en.wikipedia.org/wiki/Universally_Unique_Identifier#Random_UUID_probability_of_duplicates}
    {a \e{very} small chance}.

    UUIDs can be constructed from numeric values or from strings, or
    using the static createUuid() function. They can be converted to a
    string with toString(). UUIDs have a variant() and a version(),
    and null UUIDs return true from isNull().
*/

/*!
    \enum QUuid::StringFormat
    \since 5.11

    This enum is used by toString(StringFormat) to control the formatting of the
    string representation. The possible values are:

    \value WithBraces       The default, toString() will return five hex fields, separated by
                            dashes and surrounded by braces. Example:
                            {00000000-0000-0000-0000-000000000000}.
    \value WithoutBraces    Only the five dash-separated fields, without the braces. Example:
                            00000000-0000-0000-0000-000000000000.
    \value Id128            Only the hex digits, without braces or dashes. Note that QUuid
                            cannot parse this back again as input.
*/

/*!
    \fn QUuid::QUuid(const GUID &guid)

    Casts a Windows \a guid to a Qt QUuid.

    \warning This function is only for Windows platforms.
*/

/*!
    \fn QUuid &QUuid::operator=(const GUID &guid)

    Assigns a Windows \a guid to a Qt QUuid.

    \warning This function is only for Windows platforms.
*/

/*!
    \fn QUuid::operator GUID() const

    Returns a Windows GUID from a QUuid.

    \warning This function is only for Windows platforms.
*/

/*!
    \fn QUuid::QUuid()

    Creates the null UUID. toString() will output the null UUID
    as "{00000000-0000-0000-0000-000000000000}".
*/

/*!
    \fn QUuid::QUuid(uint l, ushort w1, ushort w2, uchar b1, uchar b2, uchar b3, uchar b4, uchar b5, uchar b6, uchar b7, uchar b8)

    Creates a UUID with the value specified by the parameters, \a l,
    \a w1, \a w2, \a b1, \a b2, \a b3, \a b4, \a b5, \a b6, \a b7, \a
    b8.

    Example:
    \snippet code/src_corelib_plugin_quuid.cpp 0
*/

/*!
  Creates a QUuid object from the string \a text, which must be
  formatted as five hex fields separated by '-', e.g.,
  "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" where each 'x' is a hex
  digit. The curly braces shown here are optional, but it is normal to
  include them. If the conversion fails, a null UUID is created.  See
  toString() for an explanation of how the five hex fields map to the
  public data members in QUuid.

    \sa toString(), QUuid()
*/
QUuid::QUuid(const QString &text)
    : QUuid(fromString(text))
{
}

/*!
    \since 5.10

    Creates a QUuid object from the string \a text, which must be
    formatted as five hex fields separated by '-', e.g.,
    "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" where each 'x' is a hex
    digit. The curly braces shown here are optional, but it is normal to
    include them. If the conversion fails, a null UUID is returned.  See
    toString() for an explanation of how the five hex fields map to the
    public data members in QUuid.

    \sa toString(), QUuid()
*/
QUuid QUuid::fromString(QStringView text) Q_DECL_NOTHROW
{
    if (text.size() > MaxStringUuidLength)
        text = text.left(MaxStringUuidLength); // text.truncate(MaxStringUuidLength);

    char latin1[MaxStringUuidLength + 1];
    char *dst = latin1;

    for (QChar ch : text)
        *dst++ = ch.toLatin1();

    *dst++ = '\0'; // don't read garbage as potentially valid data

    return _q_uuidFromHex(latin1);
}

/*!
    \since 5.10
    \overload

    Creates a QUuid object from the string \a text, which must be
    formatted as five hex fields separated by '-', e.g.,
    "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" where each 'x' is a hex
    digit. The curly braces shown here are optional, but it is normal to
    include them. If the conversion fails, a null UUID is returned.  See
    toString() for an explanation of how the five hex fields map to the
    public data members in QUuid.

    \sa toString(), QUuid()
*/
QUuid QUuid::fromString(QLatin1String text) Q_DECL_NOTHROW
{
    if (Q_UNLIKELY(text.size() < MaxStringUuidLength - 2
                   || (text.front() == QLatin1Char('{') && text.size() < MaxStringUuidLength - 1))) {
        // Too short. Don't call _q_uuidFromHex(); QL1Ss need not be NUL-terminated,
        // and we don't want to read trailing garbage as potentially valid data.
        text = QLatin1String();
    }
    return _q_uuidFromHex(text.data());
}

/*!
    \internal
*/
QUuid::QUuid(const char *text)
    : QUuid(_q_uuidFromHex(text))
{
}

/*!
  Creates a QUuid object from the QByteArray \a text, which must be
  formatted as five hex fields separated by '-', e.g.,
  "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" where each 'x' is a hex
  digit. The curly braces shown here are optional, but it is normal to
  include them. If the conversion fails, a null UUID is created.  See
  toByteArray() for an explanation of how the five hex fields map to the
  public data members in QUuid.

    \since 4.8

    \sa toByteArray(), QUuid()
*/
QUuid::QUuid(const QByteArray &text)
    : QUuid(fromString(QLatin1String(text.data(), text.size())))
{
}

/*!
  \since 5.0
  \fn QUuid QUuid::createUuidV3(const QUuid &ns, const QByteArray &baseData);

  This function returns a new UUID with variant QUuid::DCE and version QUuid::Md5.
  \a ns is the namespace and \a baseData is the basic data as described by RFC 4122.

  \sa variant(), version(), createUuidV5()
*/

/*!
  \since 5.0
  \fn QUuid QUuid::createUuidV3(const QUuid &ns, const QString &baseData);

  This function returns a new UUID with variant QUuid::DCE and version QUuid::Md5.
  \a ns is the namespace and \a baseData is the basic data as described by RFC 4122.

  \sa variant(), version(), createUuidV5()
*/

/*!
  \since 5.0
  \fn QUuid QUuid::createUuidV5(const QUuid &ns, const QByteArray &baseData);

  This function returns a new UUID with variant QUuid::DCE and version QUuid::Sha1.
  \a ns is the namespace and \a baseData is the basic data as described by RFC 4122.

  \sa variant(), version(), createUuidV3()
*/

/*!
  \since 5.0
  \fn QUuid QUuid::createUuidV5(const QUuid &ns, const QString &baseData);

  This function returns a new UUID with variant QUuid::DCE and version QUuid::Sha1.
  \a ns is the namespace and \a baseData is the basic data as described by RFC 4122.

  \sa variant(), version(), createUuidV3()
*/
#ifndef QT_BOOTSTRAPPED
QUuid QUuid::createUuidV3(const QUuid &ns, const QByteArray &baseData)
{
    return createFromName(ns, baseData, QCryptographicHash::Md5, 3);
}
#endif

QUuid QUuid::createUuidV5(const QUuid &ns, const QByteArray &baseData)
{
    return createFromName(ns, baseData, QCryptographicHash::Sha1, 5);
}

/*!
  Creates a QUuid object from the binary representation of the UUID, as
  specified by RFC 4122 section 4.1.2. See toRfc4122() for a further
  explanation of the order of \a bytes required.

  The byte array accepted is NOT a human readable format.

  If the conversion fails, a null UUID is created.

    \since 4.8

    \sa toRfc4122(), QUuid()
*/
QUuid QUuid::fromRfc4122(const QByteArray &bytes)
{
    if (bytes.isEmpty() || bytes.length() != 16)
        return QUuid();

    uint d1;
    ushort d2, d3;
    uchar d4[8];

    const uchar *data = reinterpret_cast<const uchar *>(bytes.constData());

    d1 = qFromBigEndian<quint32>(data);
    data += sizeof(quint32);
    d2 = qFromBigEndian<quint16>(data);
    data += sizeof(quint16);
    d3 = qFromBigEndian<quint16>(data);
    data += sizeof(quint16);

    for (int i = 0; i < 8; ++i) {
        d4[i] = *(data);
        data++;
    }

    return QUuid(d1, d2, d3, d4[0], d4[1], d4[2], d4[3], d4[4], d4[5], d4[6], d4[7]);
}

/*!
    \fn bool QUuid::operator==(const QUuid &other) const

    Returns \c true if this QUuid and the \a other QUuid are identical;
    otherwise returns \c false.
*/

/*!
    \fn bool QUuid::operator!=(const QUuid &other) const

    Returns \c true if this QUuid and the \a other QUuid are different;
    otherwise returns \c false.
*/

/*!
    Returns the string representation of this QUuid. The string is
    formatted as five hex fields separated by '-' and enclosed in
    curly braces, i.e., "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" where
    'x' is a hex digit.  From left to right, the five hex fields are
    obtained from the four public data members in QUuid as follows:

    \table
    \header
    \li Field #
    \li Source

    \row
    \li 1
    \li data1

    \row
    \li 2
    \li data2

    \row
    \li 3
    \li data3

    \row
    \li 4
    \li data4[0] .. data4[1]

    \row
    \li 5
    \li data4[2] .. data4[7]

    \endtable
*/
QString QUuid::toString() const
{
    char latin1[MaxStringUuidLength];
    const auto end = _q_uuidToHex(*this, latin1);
    Q_ASSERT(end - latin1 == MaxStringUuidLength);
    Q_UNUSED(end);
    return QString::fromLatin1(latin1, MaxStringUuidLength);
}

/*!
    \since 5.11

    Returns the string representation of this QUuid, with the formattiong
    controlled by the \a mode parameter. From left to right, the five hex
    fields are obtained from the four public data members in QUuid as follows:

    \table
    \header
    \li Field #
    \li Source

    \row
    \li 1
    \li data1

    \row
    \li 2
    \li data2

    \row
    \li 3
    \li data3

    \row
    \li 4
    \li data4[0] .. data4[1]

    \row
    \li 5
    \li data4[2] .. data4[7]

    \endtable
*/
QString QUuid::toString(QUuid::StringFormat mode) const
{
    char latin1[MaxStringUuidLength];
    const auto end = _q_uuidToHex(*this, latin1, mode);
    return QString::fromLatin1(latin1, end - latin1);
}

/*!
    Returns the binary representation of this QUuid. The byte array is
    formatted as five hex fields separated by '-' and enclosed in
    curly braces, i.e., "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" where
    'x' is a hex digit.  From left to right, the five hex fields are
    obtained from the four public data members in QUuid as follows:

    \table
    \header
    \li Field #
    \li Source

    \row
    \li 1
    \li data1

    \row
    \li 2
    \li data2

    \row
    \li 3
    \li data3

    \row
    \li 4
    \li data4[0] .. data4[1]

    \row
    \li 5
    \li data4[2] .. data4[7]

    \endtable

    \since 4.8
*/
QByteArray QUuid::toByteArray() const
{
    QByteArray result(MaxStringUuidLength, Qt::Uninitialized);
    const auto end = _q_uuidToHex(*this, const_cast<char*>(result.constData()));
    Q_ASSERT(end - result.constData() == MaxStringUuidLength);
    Q_UNUSED(end);
    return result;
}

/*!
    \since 5.11

    Returns the string representation of this QUuid, with the formattiong
    controlled by the \a mode parameter. From left to right, the five hex
    fields are obtained from the four public data members in QUuid as follows:

    \table
    \header
    \li Field #
    \li Source

    \row
    \li 1
    \li data1

    \row
    \li 2
    \li data2

    \row
    \li 3
    \li data3

    \row
    \li 4
    \li data4[0] .. data4[1]

    \row
    \li 5
    \li data4[2] .. data4[7]

    \endtable
*/
QByteArray QUuid::toByteArray(QUuid::StringFormat mode) const
{
    QByteArray result(MaxStringUuidLength, Qt::Uninitialized);
    const auto end = _q_uuidToHex(*this, const_cast<char*>(result.constData()), mode);
    result.resize(end - result.constData());
    return result;
}

/*!
    Returns the binary representation of this QUuid. The byte array is in big
    endian format, and formatted according to RFC 4122, section 4.1.2 -
    "Layout and byte order".

    The order is as follows:

    \table
    \header
    \li Field #
    \li Source

    \row
    \li 1
    \li data1

    \row
    \li 2
    \li data2

    \row
    \li 3
    \li data3

    \row
    \li 4
    \li data4[0] .. data4[7]

    \endtable

    \since 4.8
*/
QByteArray QUuid::toRfc4122() const
{
    // we know how many bytes a UUID has, I hope :)
    QByteArray bytes(16, Qt::Uninitialized);
    uchar *data = reinterpret_cast<uchar*>(bytes.data());

    qToBigEndian(data1, data);
    data += sizeof(quint32);
    qToBigEndian(data2, data);
    data += sizeof(quint16);
    qToBigEndian(data3, data);
    data += sizeof(quint16);

    for (int i = 0; i < 8; ++i) {
        *(data) = data4[i];
        data++;
    }

    return bytes;
}

#ifndef QT_NO_DATASTREAM
/*!
    \relates QUuid
    Writes the UUID \a id to the data stream \a s.
*/
QDataStream &operator<<(QDataStream &s, const QUuid &id)
{
    QByteArray bytes;
    if (s.byteOrder() == QDataStream::BigEndian) {
        bytes = id.toRfc4122();
    } else {
        // we know how many bytes a UUID has, I hope :)
        bytes = QByteArray(16, Qt::Uninitialized);
        uchar *data = reinterpret_cast<uchar*>(bytes.data());

        qToLittleEndian(id.data1, data);
        data += sizeof(quint32);
        qToLittleEndian(id.data2, data);
        data += sizeof(quint16);
        qToLittleEndian(id.data3, data);
        data += sizeof(quint16);

        for (int i = 0; i < 8; ++i) {
            *(data) = id.data4[i];
            data++;
        }
    }

    if (s.writeRawData(bytes.data(), 16) != 16) {
        s.setStatus(QDataStream::WriteFailed);
    }
    return s;
}

/*!
    \relates QUuid
    Reads a UUID from the stream \a s into \a id.
*/
QDataStream &operator>>(QDataStream &s, QUuid &id)
{
    QByteArray bytes(16, Qt::Uninitialized);
    if (s.readRawData(bytes.data(), 16) != 16) {
        s.setStatus(QDataStream::ReadPastEnd);
        return s;
    }

    if (s.byteOrder() == QDataStream::BigEndian) {
        id = QUuid::fromRfc4122(bytes);
    } else {
        const uchar *data = reinterpret_cast<const uchar *>(bytes.constData());

        id.data1 = qFromLittleEndian<quint32>(data);
        data += sizeof(quint32);
        id.data2 = qFromLittleEndian<quint16>(data);
        data += sizeof(quint16);
        id.data3 = qFromLittleEndian<quint16>(data);
        data += sizeof(quint16);

        for (int i = 0; i < 8; ++i) {
            id.data4[i] = *(data);
            data++;
        }
    }

    return s;
}
#endif // QT_NO_DATASTREAM

/*!
    Returns \c true if this is the null UUID
    {00000000-0000-0000-0000-000000000000}; otherwise returns \c false.
*/
bool QUuid::isNull() const Q_DECL_NOTHROW
{
    return data4[0] == 0 && data4[1] == 0 && data4[2] == 0 && data4[3] == 0 &&
           data4[4] == 0 && data4[5] == 0 && data4[6] == 0 && data4[7] == 0 &&
           data1 == 0 && data2 == 0 && data3 == 0;
}

/*!
    \enum QUuid::Variant

    This enum defines the values used in the \l{Variant field}
    {variant field} of the UUID. The value in the variant field
    determines the layout of the 128-bit value.

    \value VarUnknown Variant is unknown
    \value NCS Reserved for NCS (Network Computing System) backward compatibility
    \value DCE Distributed Computing Environment, the scheme used by QUuid
    \value Microsoft Reserved for Microsoft backward compatibility (GUID)
    \value Reserved Reserved for future definition
*/

/*!
    \enum QUuid::Version

    This enum defines the values used in the \l{Version field}
    {version field} of the UUID. The version field is meaningful
    only if the value in the \l{Variant field} {variant field}
    is QUuid::DCE.

    \value VerUnknown Version is unknown
    \value Time Time-based, by using timestamp, clock sequence, and
    MAC network card address (if available) for the node sections
    \value EmbeddedPOSIX DCE Security version, with embedded POSIX UUIDs
    \value Name Name-based, by using values from a name for all sections
    \value Md5 Alias for Name
    \value Random Random-based, by using random numbers for all sections
    \value Sha1
*/

/*!
    \fn QUuid::Variant QUuid::variant() const

    Returns the value in the \l{Variant field} {variant field} of the
    UUID. If the return value is QUuid::DCE, call version() to see
    which layout it uses. The null UUID is considered to be of an
    unknown variant.

    \sa version()
*/
QUuid::Variant QUuid::variant() const Q_DECL_NOTHROW
{
    if (isNull())
        return VarUnknown;
    // Check the 3 MSB of data4[0]
    if ((data4[0] & 0x80) == 0x00) return NCS;
    else if ((data4[0] & 0xC0) == 0x80) return DCE;
    else if ((data4[0] & 0xE0) == 0xC0) return Microsoft;
    else if ((data4[0] & 0xE0) == 0xE0) return Reserved;
    return VarUnknown;
}

/*!
    \fn QUuid::Version QUuid::version() const

    Returns the \l{Version field} {version field} of the UUID, if the
    UUID's \l{Variant field} {variant field} is QUuid::DCE. Otherwise
    it returns QUuid::VerUnknown.

    \sa variant()
*/
QUuid::Version QUuid::version() const Q_DECL_NOTHROW
{
    // Check the 4 MSB of data3
    Version ver = (Version)(data3>>12);
    if (isNull()
         || (variant() != DCE)
         || ver < Time
         || ver > Sha1)
        return VerUnknown;
    return ver;
}

/*!
    \fn bool QUuid::operator<(const QUuid &other) const

    Returns \c true if this QUuid has the same \l{Variant field}
    {variant field} as the \a other QUuid and is lexicographically
    \e{before} the \a other QUuid. If the \a other QUuid has a
    different variant field, the return value is determined by
    comparing the two \l{QUuid::Variant} {variants}.

    \sa variant()
*/
bool QUuid::operator<(const QUuid &other) const Q_DECL_NOTHROW
{
    if (variant() != other.variant())
        return variant() < other.variant();

#define ISLESS(f1, f2) if (f1!=f2) return (f1<f2);
    ISLESS(data1, other.data1);
    ISLESS(data2, other.data2);
    ISLESS(data3, other.data3);
    for (int n = 0; n < 8; n++) {
        ISLESS(data4[n], other.data4[n]);
    }
#undef ISLESS
    return false;
}

/*!
    \fn bool QUuid::operator>(const QUuid &other) const

    Returns \c true if this QUuid has the same \l{Variant field}
    {variant field} as the \a other QUuid and is lexicographically
    \e{after} the \a other QUuid. If the \a other QUuid has a
    different variant field, the return value is determined by
    comparing the two \l{QUuid::Variant} {variants}.

    \sa variant()
*/
bool QUuid::operator>(const QUuid &other) const Q_DECL_NOTHROW
{
    return other < *this;
}

/*!
    \fn bool operator<=(const QUuid &lhs, const QUuid &rhs)
    \relates QUuid
    \since 5.5

    Returns \c true if \a lhs has the same \l{Variant field}
    {variant field} as \a rhs and is lexicographically
    \e{not after} \a rhs. If \a rhs has a
    different variant field, the return value is determined by
    comparing the two \l{QUuid::Variant} {variants}.

    \sa {QUuid::}{variant()}
*/

/*!
    \fn bool operator>=(const QUuid &lhs, const QUuid &rhs)
    \relates QUuid
    \since 5.5

    Returns \c true if \a lhs has the same \l{Variant field}
    {variant field} as \a rhs and is lexicographically
    \e{not before} \a rhs. If \a rhs has a
    different variant field, the return value is determined by
    comparing the two \l{QUuid::Variant} {variants}.

    \sa {QUuid::}{variant()}
*/

/*!
    \fn QUuid QUuid::createUuid()

    On any platform other than Windows, this function returns a new UUID with
    variant QUuid::DCE and version QUuid::Random. On Windows, a GUID is
    generated using the Windows API and will be of the type that the API
    decides to create.

    \sa variant(), version()
*/
#if defined(Q_OS_WIN)

QT_BEGIN_INCLUDE_NAMESPACE
#include <objbase.h> // For CoCreateGuid
QT_END_INCLUDE_NAMESPACE

QUuid QUuid::createUuid()
{
    GUID guid;
    CoCreateGuid(&guid);
    QUuid result = guid;
    return result;
}

#else // Q_OS_WIN

QUuid QUuid::createUuid()
{
    QUuid result(Qt::Uninitialized);
    uint *data = &(result.data1);
    enum { AmountToRead = 4 };
    QRandomGenerator::system()->fillRange(data, AmountToRead);

    result.data4[0] = (result.data4[0] & 0x3F) | 0x80;        // UV_DCE
    result.data3 = (result.data3 & 0x0FFF) | 0x4000;        // UV_Random

    return result;
}
#endif // !Q_OS_WIN

/*!
    \fn bool QUuid::operator==(const GUID &guid) const

    Returns \c true if this UUID is equal to the Windows GUID \a guid;
    otherwise returns \c false.
*/

/*!
    \fn bool QUuid::operator!=(const GUID &guid) const

    Returns \c true if this UUID is not equal to the Windows GUID \a
    guid; otherwise returns \c false.
*/

#ifndef QT_NO_DEBUG_STREAM
/*!
    \relates QUuid
    Writes the UUID \a id to the output stream for debugging information \a dbg.
*/
QDebug operator<<(QDebug dbg, const QUuid &id)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QUuid(" << id.toString() << ')';
    return dbg;
}
#endif

/*!
    \since 5.0
    \relates QUuid
    Returns a hash of the UUID \a uuid, using \a seed to seed the calculation.
*/
uint qHash(const QUuid &uuid, uint seed) Q_DECL_NOTHROW
{
    return uuid.data1 ^ uuid.data2 ^ (uuid.data3 << 16)
            ^ ((uuid.data4[0] << 24) | (uuid.data4[1] << 16) | (uuid.data4[2] << 8) | uuid.data4[3])
            ^ ((uuid.data4[4] << 24) | (uuid.data4[5] << 16) | (uuid.data4[6] << 8) | uuid.data4[7])
            ^ seed;
}


QT_END_NAMESPACE
