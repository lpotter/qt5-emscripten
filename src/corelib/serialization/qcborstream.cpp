/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
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

#include "qcborstream.h"

#include <private/qnumeric_p.h>
#include <private/qutfcodec_p.h>
#include <qbuffer.h>
#include <qdebug.h>
#include <qstack.h>

QT_BEGIN_NAMESPACE

#ifdef QT_NO_DEBUG
#  define NDEBUG 1
#endif
#undef assert
#define assert Q_ASSERT

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wunused-function")
QT_WARNING_DISABLE_CLANG("-Wundefined-internal")
QT_WARNING_DISABLE_MSVC(4334) // '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)

#define CBOR_ENCODER_NO_CHECK_USER

#define CBOR_NO_VALIDATION_API  1
#define CBOR_NO_PRETTY_API      1
#define CBOR_API static inline
#define CBOR_PRIVATE_API static inline
#define CBOR_INLINE_API static inline

#include <cbor.h>

static CborError qt_cbor_encoder_write_callback(void *token, const void *data, size_t len, CborEncoderAppendType);
static bool qt_cbor_decoder_can_read(void *token, size_t len);
static void qt_cbor_decoder_advance(void *token, size_t len);
static void *qt_cbor_decoder_read(void *token, void *userptr, size_t offset, size_t len);
static CborError qt_cbor_decoder_transfer_string(void *token, const void **userptr, size_t offset, size_t len);

#define CBOR_ENCODER_WRITER_CONTROL     1
#define CBOR_ENCODER_WRITE_FUNCTION     qt_cbor_encoder_write_callback

#define CBOR_PARSER_READER_CONTROL              1
#define CBOR_PARSER_CAN_READ_BYTES_FUNCTION     qt_cbor_decoder_can_read
#define CBOR_PARSER_ADVANCE_BYTES_FUNCTION      qt_cbor_decoder_advance
#define CBOR_PARSER_TRANSFER_STRING_FUNCTION    qt_cbor_decoder_transfer_string
#define CBOR_PARSER_READ_BYTES_FUNCTION         qt_cbor_decoder_read

#include "../3rdparty/tinycbor/src/cborencoder.c"
#include "../3rdparty/tinycbor/src/cborerrorstrings.c"
#include "../3rdparty/tinycbor/src/cborparser.c"

// silence compilers that complain about this being a static function declared
// but never defined
static CborError cbor_encoder_close_container_checked(CborEncoder*, const CborEncoder*)
{
    Q_UNREACHABLE();
    return CborErrorInternalError;
}
static CborError _cbor_value_dup_string(const CborValue *, void **, size_t *, CborValue *)
{
    Q_UNREACHABLE();
    return CborErrorInternalError;
}
QT_WARNING_POP

Q_DECLARE_TYPEINFO(CborEncoder, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(CborValue, Q_PRIMITIVE_TYPE);

// confirm our constants match TinyCBOR's
Q_STATIC_ASSERT(int(QCborStreamReader::UnsignedInteger) == CborIntegerType);
Q_STATIC_ASSERT(int(QCborStreamReader::ByteString) == CborByteStringType);
Q_STATIC_ASSERT(int(QCborStreamReader::TextString) == CborTextStringType);
Q_STATIC_ASSERT(int(QCborStreamReader::Array) == CborArrayType);
Q_STATIC_ASSERT(int(QCborStreamReader::Map) == CborMapType);
Q_STATIC_ASSERT(int(QCborStreamReader::Tag) == CborTagType);
Q_STATIC_ASSERT(int(QCborStreamReader::SimpleType) == CborSimpleType);
Q_STATIC_ASSERT(int(QCborStreamReader::HalfFloat) == CborHalfFloatType);
Q_STATIC_ASSERT(int(QCborStreamReader::Float) == CborFloatType);
Q_STATIC_ASSERT(int(QCborStreamReader::Double) == CborDoubleType);
Q_STATIC_ASSERT(int(QCborStreamReader::Invalid) == CborInvalidType);

/*!
   \headerfile <QtCborCommon>

   \brief The <QtCborCommon> header contains definitions common to both the
   streaming classes (QCborStreamReader and QCborStreamWriter) and to
   QCborValue.
 */

/*!
   \enum QCborSimpleType
   \relates <QtCborCommon>

   This enum contains the possible "Simple Types" for CBOR. Simple Types range
   from 0 to 255 and are types that carry no further value.

   The following values are currently known:

   \value False             A "false" boolean.
   \value True              A "true" boolean.
   \value Null              Absence of value (null).
   \value Undefined         Missing or deleted value, usually an error.

   Qt CBOR API supports encoding and decoding any Simple Type, whether one of
   those above or any other value.

   Applications should only use further values if a corresponding specification
   has been published, otherwise interpretation and validation by the remote
   may fail. Values 24 to 31 are reserved and must not be used.

   The current authoritative list is maintained by IANA in the
   \l{https://www.iana.org/assignments/cbor-simple-values/cbor-simple-values.xml}{Simple
   Values registry}.

   \sa QCborStreamWriter::append(QCborSimpleType), QCborStreamReader::isSimpleType(),
       QCborStreamReader::toSimpleType(), QCborValue::isSimpleType(), QCborValue::toSimpleType()
 */

Q_CORE_EXPORT const char *qt_cbor_simpletype_id(QCborSimpleType st)
{
    switch (st) {
    case QCborSimpleType::False:
        return "False";
    case QCborSimpleType::True:
        return "True";
    case QCborSimpleType::Null:
        return "Null";
    case QCborSimpleType::Undefined:
        return "Undefined";
    }
    return nullptr;
}

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug dbg, QCborSimpleType st)
{
    QDebugStateSaver saver(dbg);
    const char *id = qt_cbor_simpletype_id(st);
    if (id)
        return dbg.nospace() << "QCborSimpleType::" << id;

    return dbg.nospace() << "QCborSimpleType(" << uint(st) << ')';
}
#endif

/*!
   \enum QCborTag
   \relates <QtCborCommon>

   This enum contains no enumeration and is used only to provide type-safe
   access to a CBOR tag.

   CBOR tags are 64-bit numbers that are attached to generic CBOR types to
   provide further semantic meaning. QCborTag may be constructed from an
   enumeration found in QCborKnownTags or directly by providing the numeric
   representation.

   For example, the following creates a QCborValue containing a byte array
   tagged with a tag 2.

   \code
        QCborValue(QCborTag(2), QByteArray("\x01\0\0\0\0\0\0\0\0", 9));
   \endcode

   \sa QCborKnownTags, QCborStreamWriter::append(QCborTag),
       QCborStreamReader::isTag(), QCborStreamReader::toTag(),
       QCborValue::isTag(), QCborValue::tag()
 */

Q_CORE_EXPORT const char *qt_cbor_tag_id(QCborTag tag)
{
    // Casting to QCborKnownTags's underlying type will make the comparison
    // below fail if the tag value is out of range.
    auto n = std::underlying_type<QCborKnownTags>::type(tag);
    if (QCborTag(n) == tag) {
        switch (QCborKnownTags(n)) {
        case QCborKnownTags::DateTimeString:
            return "DateTimeString";
        case QCborKnownTags::UnixTime_t:
            return "UnixTime_t";
        case QCborKnownTags::PositiveBignum:
            return "PositiveBignum";
        case QCborKnownTags::NegativeBignum:
            return "NegativeBignum";
        case QCborKnownTags::Decimal:
            return "Decimal";
        case QCborKnownTags::Bigfloat:
            return "Bigfloat";
        case QCborKnownTags::COSE_Encrypt0:
            return "COSE_Encrypt0";
        case QCborKnownTags::COSE_Mac0:
            return "COSE_Mac0";
        case QCborKnownTags::COSE_Sign1:
            return "COSE_Sign1";
        case QCborKnownTags::ExpectedBase64url:
            return "ExpectedBase64url";
        case QCborKnownTags::ExpectedBase64:
            return "ExpectedBase64";
        case QCborKnownTags::ExpectedBase16:
            return "ExpectedBase16";
        case QCborKnownTags::EncodedCbor:
            return "EncodedCbor";
        case QCborKnownTags::Url:
            return "Url";
        case QCborKnownTags::Base64url:
            return "Base64url";
        case QCborKnownTags::Base64:
            return "Base64";
        case QCborKnownTags::RegularExpression:
            return "RegularExpression";
        case QCborKnownTags::MimeMessage:
            return "MimeMessage";
        case QCborKnownTags::Uuid:
            return "Uuid";
        case QCborKnownTags::COSE_Encrypt:
            return "COSE_Encrypt";
        case QCborKnownTags::COSE_Mac:
            return "COSE_Mac";
        case QCborKnownTags::COSE_Sign:
            return "COSE_Sign";
        case QCborKnownTags::Signature:
            return "Signature";
        }
    }
    return nullptr;
}

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug dbg, QCborTag tag)
{
    QDebugStateSaver saver(dbg);
    const char *id = qt_cbor_tag_id(tag);
    dbg.nospace() << "QCborTag(";
    if (id)
        dbg.nospace() << "QCborKnownTags::" << id;
    else
        dbg.nospace() << quint64(tag);

    return dbg << ')';
}
#endif

/*!
   \enum QCborKnownTags
   \relates <QtCborCommon>

   This enum contains a list of CBOR tags, known at the time of the Qt
   implementation. This list is not meant to be complete and contains only
   tags that are either backed by an RFC or specifically used by the Qt
   implementation.

   The authoritative list is maintained by IANA in the
   \l{https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml}{CBOR tag
   registry}.

   \value DateTimeString        A date and time string, formatted according to RFC 3339, as refined
                                by RFC 4287. It is the same format as Qt::ISODate and
                                Qt::ISODateWithMs.
   \value UnixTime_t            A numerical representation of seconds elapsed since
                                1970-01-01T00:00Z.
   \value PositiveBignum        A positive number of arbitrary length, encoded as a byte array in
                                network byte order. For example, the number 2\sup{64} is represented by
                                a byte array containing the byte value 0x01 followed by 8 zero bytes.
   \value NegativeBignum        A negative number of arbirary length, encoded as the absolute value
                                of that number, minus one. For example, a byte array containing
                                byte value 0x02 followed by 8 zero bytes represents the number
                                -2\sup{65} - 1.
   \value Decimal               A decimal fraction, encoded as an array of two integers: the first
                                is the exponent of the power of 10, the second the integral
                                mantissa. The value 273.15 would be encoded as array \c{[-2, 27315]}.
   \value Bigfloat              Similar to Decimal, but the exponent is a power of 2 instead.
   \value COSE_Encrypt0         An \c Encrypt0 map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value COSE_Mac0             A \c Mac0 map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value COSE_Sign1            A \c Sign1 map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value ExpectedBase64url     Indicates that the byte array should be encoded using Base64url
                                if the stream is converted to JSON.
   \value ExpectedBase64        Indicates that the byte array should be encoded using Base64
                                if the stream is converted to JSON.
   \value ExpectedBase16        Indicates that the byte array should be encoded using Base16 (hex)
                                if the stream is converted to JSON.
   \value EncodedCbor           Indicates that the byte array contains a CBOR stream.
   \value Url                   Indicates that the string contains a URL.
   \value Base64url             Indicates that the string contains data encoded using Base64url.
   \value Base64                Indicates that the string contains data encoded using Base64.
   \value RegularExpression     Indicates that the string contains a Perl-Compatible Regular
                                Expression pattern.
   \value MimeMessage           Indicates that the string contains a MIME message (according to
                                \l{https://tools.ietf.org/html/rfc2045}){RFC 2045}.
   \value Uuid                  Indicates that the byte array contains a UUID.
   \value COSE_Encrypt          An \c Encrypt map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value COSE_Mac              A \c Mac map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value COSE_Sign             A \c Sign map as specified by \l{https://tools.ietf.org/html/rfc8152}{RFC 8152}
                                (CBOR Object Signing and Encryption).
   \value Signature             No change in interpretation; this tag can be used as the outermost
                                tag in a CBOR stream as the file header.

   The following tags are interpreted by QCborValue during decoding and will
   produce objects with extended Qt types, and it will use those tags when
   encoding the same extended types.

   \value DateTimeString        \l QDateTime
   \value UnixTime_t            \l QDateTime (only in decoding)
   \value Url                   \l QUrl
   \value Uuid                  \l QUuid

   Additionally, if a QCborValue containing a QByteArray is tagged using one of
   \c ExpectedBase64url, \c ExpectedBase64 or \c ExpectedBase16, QCborValue
   will use the expected encoding when converting to JSON (see
   QCborValue::toJsonValue).

   \sa QCborTag, QCborStreamWriter::append(QCborTag),
       QCborStreamReader::isTag(), QCborStreamReader::toTag(),
       QCborValue::isTag(), QCborValue::tag()
 */

#if !defined(QT_NO_DEBUG_STREAM)
QDebug operator<<(QDebug dbg, QCborKnownTags tag)
{
    QDebugStateSaver saver(dbg);
    const char *id = qt_cbor_tag_id(QCborTag(int(tag)));
    if (id)
        return dbg.nospace() << "QCborKnownTags::" << id;

    return dbg.nospace() << "QCborKnownTags(" << int(tag) << ')';
}
#endif

/*!
   \class QCborError
   \inmodule QtCore
   \relates <QtCborCommon>
   \reentrant
   \since 5.12

   \brief The QCborError class holds the error condition found while parsing or
   validating a CBOR stream.

   \sa QCborStreamReader, QCborValue, QCborParserError
 */

/*!
   \enum QCborError::Code

   This enum contains the possible error condition codes.

   \value NoError           No error was detected.
   \value UnknownError      An unknown error occurred and no further details are available.
   \value AdvancePastEnd    QCborStreamReader::next() was called but there are no more elements in
                            the current context.
   \value InputOutputError  An I/O error with the QIODevice occurred.
   \value GarbageAtEnd      Data was found in the input stream after the last element.
   \value EndOfFile         The end of the input stream was unexpectedly reached while processing an
                            element.
   \value UnexpectedBreak   The CBOR stream contains a Break where it is not allowed (data is
                            corrupt and the error is not recoverable).
   \value UnknownType       The CBOR stream contains an unknown/unparseable Type (data is corrupt
                            and the and the error is not recoverable).
   \value IllegalType       The CBOR stream contains a known type in a position it is not allowed
                            to exist (data is corrupt and the error is not recoverable).
   \value IllegalNumber     The CBOR stream appears to be encoding a number larger than 64-bit
                            (data is corrupt and the error is not recoverable).
   \value IllegalSimpleType The CBOR stream contains a Simple Type encoded incorrectly (data is
                            corrupt and the error is not recoverable).
   \value InvalidUtf8String The CBOR stream contains a text string that does not decode properly
                            as UTF (data is corrupt and the error is not recoverable).
   \value DataTooLarge      CBOR string, map or array is too big and cannot be parsed by Qt
                            (internal limitation, but the error is not recoverable).
   \value NestingTooDeep    Too many levels of arrays or maps encountered while processing the
                            input (internal limitation, but the error is not recoverable).
   \value UnsupportedType   The CBOR stream contains a known type that the implementation does not
                            support (internal limitation, but the error is not recoverable).
 */

/*!
   \variable QCborError::c
   \internal
 */

/*!
   \fn QCborError::operator Code() const

   Returns the error code that this QCborError object stores.
 */

/*!
   Returns a text string that matches the error code in this QCborError object.

   Note: the string is not translated. Applications whose interface allow users
   to parse CBOR streams need to provide their own, translated strings.

   \sa QCborError::Code
 */
QString QCborError::toString() const
{
    switch (c) {
    case NoError:
        Q_STATIC_ASSERT(int(NoError) == int(CborNoError));
        return QString();

    case UnknownError:
        Q_STATIC_ASSERT(int(UnknownError) == int(CborUnknownError));
        return QStringLiteral("Unknown error");
    case AdvancePastEnd:
        Q_STATIC_ASSERT(int(AdvancePastEnd) == int(CborErrorAdvancePastEOF));
        return QStringLiteral("Read past end of buffer (more bytes needed)");
    case InputOutputError:
        Q_STATIC_ASSERT(int(InputOutputError) == int(CborErrorIO));
        return QStringLiteral("Input/Output error");
    case GarbageAtEnd:
        Q_STATIC_ASSERT(int(GarbageAtEnd) == int(CborErrorGarbageAtEnd));
        return QStringLiteral("Data found after the end of the stream");
    case EndOfFile:
        Q_STATIC_ASSERT(int(EndOfFile) == int(CborErrorUnexpectedEOF));
        return QStringLiteral("Unexpected end of input data (more bytes needed)");
    case UnexpectedBreak:
        Q_STATIC_ASSERT(int(UnexpectedBreak) == int(CborErrorUnexpectedBreak));
        return QStringLiteral("Invalid CBOR stream: unexpected 'break' byte");
    case UnknownType:
        Q_STATIC_ASSERT(int(UnknownType) == int(CborErrorUnknownType));
        return QStringLiteral("Invalid CBOR stream: unknown type");
    case IllegalType:
        Q_STATIC_ASSERT(int(IllegalType) == int(CborErrorIllegalType));
        return QStringLiteral("Invalid CBOR stream: illegal type found");
    case IllegalNumber:
        Q_STATIC_ASSERT(int(IllegalNumber) == int(CborErrorIllegalNumber));
        return QStringLiteral("Invalid CBOR stream: illegal number encoding (future extension)");
    case IllegalSimpleType:
        Q_STATIC_ASSERT(int(IllegalSimpleType) == int(CborErrorIllegalSimpleType));
        return QStringLiteral("Invalid CBOR stream: illegal simple type");
    case InvalidUtf8String:
        Q_STATIC_ASSERT(int(InvalidUtf8String) == int(CborErrorInvalidUtf8TextString));
        return QStringLiteral("Invalid CBOR stream: invalid UTF-8 text string");
    case DataTooLarge:
        Q_STATIC_ASSERT(int(DataTooLarge) == int(CborErrorDataTooLarge));
        return QStringLiteral("Internal limitation: data set too large");
    case NestingTooDeep:
        Q_STATIC_ASSERT(int(NestingTooDeep) == int(CborErrorNestingTooDeep));
        return QStringLiteral("Internal limitation: data nesting too deep");
    case UnsupportedType:
        Q_STATIC_ASSERT(int(UnsupportedType) == int(CborErrorUnsupportedType));
        return QStringLiteral("Internal limitation: unsupported type");
    }

    // get the error from TinyCBOR
    CborError err = CborError(int(c));
    return QString::fromLatin1(cbor_error_string(err));
}

/*!
   \class QCborStreamWriter
   \inmodule QtCore
   \ingroup cbor
   \reentrant
   \since 5.12

   \brief The QCborStreamWriter class is a simple CBOR encoder operating on a
   one-way stream.

   This class can be used to quickly encode a stream of CBOR content directly
   to either a QByteArray or QIODevice. CBOR is the Concise Binary Object
   Representation, a very compact form of binary data encoding that is
   compatible with JSON. It was created by the IETF Constrained RESTful
   Environments (CoRE) WG, which has used it in many new RFCs. It is meant to
   be used alongside the \l{https://tools.ietf.org/html/rfc7252}{CoAP
   protocol}.

   QCborStreamWriter provides a StAX-like API, similar to that of
   \l{QXmlStreamWriter}. It is rather low-level and requires a bit of knowledge
   of CBOR encoding. For a simpler API, see \l{QCborValue} and especially the
   encoding function QCborValue::toCbor().

   The typical use of QCborStreamWriter is to create the object on the target
   QByteArray or QIODevice, then call one of the append() overloads with the
   desired type to be encoded. To create arrays and maps, QCborStreamWriter
   provides startArray() and startMap() overloads, which must be terminated by
   the corresponding endArray() and endMap() functions.

   The following example encodes the equivalent of this JSON content:

   \div{class="pre"}
     {
       "label": "journald",
       "autoDetect": false,
       "condition": "libs.journald",
       "output": [ "privateFeature" ]
     }
   \enddiv

   \code
     writer.startMap(4);    // 4 elements in the map

     writer.append(QLatin1String("label"));
     writer.append(QLatin1String("journald"));

     writer.append(QLatin1String("autoDetect"));
     writer.append(false);

     writer.append(QLatin1String("condition"));
     writer.append(QLatin1String("libs.journald"));

     writer.append(QLatin1String("output"));
     writer.startArray(1);
     writer.append(QLatin1String("privateFeature"));
     writer.endArray();

     writer.endMap();
   \endcode

   \section1 CBOR support

   QCborStreamWriter supports all CBOR features required to create canonical
   and strict streams. It implements almost all of the features specified in
   \l {https://tools.ietf.org/html/rfc7049}{RFC 7049}.

   The following table lists the CBOR features that QCborStreamWriter supports.

   \table
     \header \li Feature                        \li Support
     \row   \li Unsigned numbers                \li Yes (full range)
     \row   \li Negative numbers                \li Yes (full range)
     \row   \li Byte strings                    \li Yes
     \row   \li Text strings                    \li Yes
     \row   \li Chunked strings                 \li No
     \row   \li Tags                            \li Yes (arbitrary)
     \row   \li Booleans                        \li Yes
     \row   \li Null                            \li Yes
     \row   \li Undefined                       \li Yes
     \row   \li Arbitrary simple values         \li Yes
     \row   \li Half-precision float (16-bit)   \li Yes
     \row   \li Single-precision float (32-bit) \li Yes
     \row   \li Double-precision float (64-bit) \li Yes
     \row   \li Infinities and NaN floating point \li Yes
     \row   \li Determinate-length arrays and maps \li Yes
     \row   \li Indeterminate-length arrays and maps \li Yes
     \row   \li Map key types other than strings and integers \li Yes (arbitrary)
   \endtable

   \section2 Canonical CBOR encoding

   Canonical CBOR encoding is defined by
   \l{https://tools.ietf.org/html/rfc7049#section-3.9}{Section 3.9 of RFC
   7049}. Canonical encoding is not a requirement for Qt's CBOR decoding
   functionality, but it may be required for some protocols. In particular,
   protocols that require the ability to reproduce the same stream identically
   may require this.

   In order to be considered "canonical", a CBOR stream must meet the
   following requirements:

   \list
     \li Integers must be as small as possible. QCborStreamWriter always
         does this (no user action is required and it is not possible
         to write overlong integers).
     \li Array, map and string lengths must be as short as possible. As
         above, QCborStreamWriter automatically does this.
     \li Arrays, maps and strings must use explicit length. QCborStreamWriter
         always does this for strings; for arrays and maps, be sure to call
         startArray() and startMap() overloads with explicit length.
     \li Keys in every map must be sorted in ascending order. QCborStreamWriter
         offers no help in this item: the developer must ensure that before
         calling append() for the map pairs.
     \li Floating point values should be as small as possible. QCborStreamWriter
         will not convert floating point values; it is up to the developer
         to perform this check prior to calling append() (see those functions'
         examples).
   \endlist

   \section2 Strict CBOR mode

   Strict mode is defined by
   \l{https://tools.ietf.org/html/rfc7049#section-3.10}{Section 3.10 of RFC
   7049}. As for Canonical encoding above, QCborStreamWriter makes it possible
   to create strict CBOR streams, but does not require them or validate that
   the output is so.

   \list
     \li Keys in a map must be unique. QCborStreamWriter performs no validation
         of map keys.
     \li Tags may be required to be paired only with the correct types,
         according to their specification. QCborStreamWriter performs no
         validation of tag usage.
     \li Text Strings must be properly-encoded UTF-8. QCborStreamWriter always
         writes proper UTF-8 for strings added with append(), but performs no
         validation for strings added with appendTextString().
   \endlist

   \section2 Invalid CBOR stream

   It is also possible to misuse QCborStreamWriter and produce invalid CBOR
   streams that will fail to be decoded by a receiver. The following actions
   will produce invalid streams:

   \list
     \li Append a tag and not append the corresponding tagged value
         (QCborStreamWriter produces no diagnostic).
     \li Append too many or too few items to an array or map with explicit
         length (endMap() and endArray() will return false and
         QCborStreamWriter will log with qWarning()).
   \endlist

   \sa QCborStreamReader, QCborValue, QXmlStreamWriter
 */

class QCborStreamWriterPrivate
{
public:
    static Q_CONSTEXPR quint64 IndefiniteLength = (std::numeric_limits<quint64>::max)();

    QIODevice *device;
    CborEncoder encoder;
    QStack<CborEncoder> containerStack;
    bool deleteDevice = false;

    QCborStreamWriterPrivate(QIODevice *device)
        : device(device)
    {
        cbor_encoder_init_writer(&encoder, qt_cbor_encoder_write_callback, this);
    }

    ~QCborStreamWriterPrivate()
    {
        if (deleteDevice)
            delete device;
    }

    template <typename... Args> void executeAppend(CborError (*f)(CborEncoder *, Args...), Args... args)
    {
        f(&encoder, std::forward<Args>(args)...);
    }

    void createContainer(CborError (*f)(CborEncoder *, CborEncoder *, size_t), quint64 len = IndefiniteLength)
    {
        Q_STATIC_ASSERT(size_t(IndefiniteLength) == CborIndefiniteLength);
        if (sizeof(len) != sizeof(size_t) && len != IndefiniteLength) {
            if (Q_UNLIKELY(len >= CborIndefiniteLength)) {
                // TinyCBOR can't do this in 32-bit mode
                qWarning("QCborStreamWriter: container of size %llu is too big for a 32-bit build; "
                         "will use indeterminate length instead", len);
                len = CborIndefiniteLength;
            }
        }

        containerStack.push(encoder);
        f(&containerStack.top(), &encoder, len);
    }

    bool closeContainer()
    {
        if (containerStack.isEmpty()) {
            qWarning("QCborStreamWriter: closing map or array that wasn't open");
            return false;
        }

        CborEncoder container = containerStack.pop();
        CborError err = cbor_encoder_close_container(&container, &encoder);
        encoder = container;

        if (Q_UNLIKELY(err)) {
            if (err == CborErrorTooFewItems)
                qWarning("QCborStreamWriter: not enough items added to array or map");
            else if (err == CborErrorTooManyItems)
                qWarning("QCborStreamWriter: too many items added to array or map");
            return false;
        }

        return true;
    }
};

static CborError qt_cbor_encoder_write_callback(void *self, const void *data, size_t len, CborEncoderAppendType)
{
    auto that = static_cast<QCborStreamWriterPrivate *>(self);
    if (!that->device)
        return CborNoError;
    qint64 written = that->device->write(static_cast<const char *>(data), len);
    return (written == qsizetype(len) ? CborNoError : CborErrorIO);
}

/*!
   Creates a QCborStreamWriter object that will write the stream to \a device.
   The device must be opened before the first append() call is made. This
   constructor can be used with any class that derives from QIODevice, such as
   QFile, QProcess or QTcpSocket.

   QCborStreamWriter has no buffering, so every append() call will result in
   one or more calls to the device's \l {QIODevice::}{write()} method.

   The following example writes an empty map to a file:

   \code
      QFile f("output", QIODevice::WriteOnly);
      QCborStreamWriter writer(&f);
      writer.startMap(0);
      writer.endMap();
   \endcode

   QCborStreamWriter does not take ownership of \a device.

   \sa device(), setDevice()
 */
QCborStreamWriter::QCborStreamWriter(QIODevice *device)
    : d(new QCborStreamWriterPrivate(device))
{
}

/*!
   Creates a QCborStreamWriter object that will append the stream to \a data.
   All streaming is done immediately to the byte array, without the need for
   flushing any buffers.

   The following example writes a number to a byte array then returns
   it.

   \code
     QByteArray encodedNumber(qint64 value)
     {
         QByteArray ba;
         QCborStreamWriter writer(&ba);
         writer.append(value);
         return ba;
     }
   \endcode

   QCborStreamWriter does not take ownership of \a data.
 */
QCborStreamWriter::QCborStreamWriter(QByteArray *data)
    : d(new QCborStreamWriterPrivate(new QBuffer(data)))
{
    d->deleteDevice = true;
    d->device->open(QIODevice::WriteOnly | QIODevice::Unbuffered);
}

/*!
   Destroys this QCborStreamWriter object and frees any resources associated.

   QCborStreamWriter does not perform error checking to see if all required
   items were written to the stream prior to the object being destroyed. It is
   the programmer's responsibility to ensure that it was done.
 */
QCborStreamWriter::~QCborStreamWriter()
{
}

/*!
   Replaces the device or byte array that this QCborStreamWriter object is
   writing to with \a device.

   \sa device()
 */
void QCborStreamWriter::setDevice(QIODevice *device)
{
    if (d->deleteDevice)
        delete d->device;
    d->device = device;
    d->deleteDevice = false;
}

/*!
   Returns the QIODevice that this QCborStreamWriter object is writing to. The
   device must have previously been set with either the constructor or with
   setDevice().

   If this object was created by writing to a QByteArray, this function will
   return an internal instance of QBuffer, which is owned by QCborStreamWriter.

   \sa setDevice()
 */
QIODevice *QCborStreamWriter::device() const
{
    return d->device;
}

/*!
   \overload

   Appends the 64-bit unsigned value \a u to the CBOR stream, creating a CBOR
   Unsigned Integer value. In the following example, we write the values 0,
   2\sup{32} and \c UINT64_MAX:

   \code
     writer.append(0U);
     writer.append(Q_UINT64_C(4294967296));
     writer.append(std::numeric_limits<quint64>::max());
   \endcode

   \sa QCborStreamReader::isUnsignedInteger(), QCborStreamReader::toUnsignedInteger()
 */
void QCborStreamWriter::append(quint64 u)
{
    d->executeAppend(cbor_encode_uint, uint64_t(u));
}

/*!
   \overload

   Appends the 64-bit signed value \a i to the CBOR stream. This will create
   either a CBOR Unsigned Integer or CBOR NegativeInteger value based on the
   sign of the parameter. In the following example, we write the values 0, -1,
   2\sup{32} and \c INT64_MAX:

   \code
     writer.append(0);
     writer.append(-1);
     writer.append(Q_INT64_C(4294967296));
     writer.append(std::numeric_limits<qint64>::max());
   \endcode

   \sa QCborStreamReader::isInteger(), QCborStreamReader::toInteger()
 */
void QCborStreamWriter::append(qint64 i)
{
    d->executeAppend(cbor_encode_int, int64_t(i));
}

/*!
   \overload

   Appends the 64-bit negative value \a n to the CBOR stream.
   QCborNegativeInteger is a 64-bit enum that holds the absolute value of the
   negative number we want to write. If n is zero, the value written will be
   equivalent to 2\sup{64} (that is, -18,446,744,073,709,551,616).

   In the following example, we write the values -1, -2\sup{32} and INT64_MIN:
   \code
     writer.append(QCborNegativeInteger(1));
     writer.append(QCborNegativeInteger(Q_INT64_C(4294967296)));
     writer.append(QCborNegativeInteger(-quint64(std::numeric_limits<qint64>::min())));
   \endcode

   Note how this function can be used to encode numbers that cannot fit a
   standard computer's 64-bit signed integer like \l qint64. That is, if \a n
   is larger than \c{std::numeric_limits<qint64>::max()} or is 0, this will
   represent a negative number smaller than
   \c{std::numeric_limits<qint64>::min()}.

   \sa QCborStreamReader::isNegativeInteger(), QCborStreamReader::toNegativeInteger()
 */
void QCborStreamWriter::append(QCborNegativeInteger n)
{
    d->executeAppend(cbor_encode_negative_int, uint64_t(n));
}

/*!
   \fn void QCborStreamWriter::append(const QByteArray &ba)
   \overload

   Appends the byte array \a ba to the stream, creating a CBOR Byte String
   value. QCborStreamWriter will attempt to write the entire string in one
   chunk.

   The following example will load and append the contents of a file to the
   stream:

   \code
     void writeFile(QCborStreamWriter &writer, const QString &fileName)
     {
         QFile f(fileName);
         if (f.open(QIODevice::ReadOnly))
             writer.append(f.readAll());
     }
   \endcode

   As the example shows, unlike JSON, CBOR requires no escaping for binary
   content.

   \sa appendByteString(), QCborStreamReader::isByteArray(),
       QCborStreamReader::readByteArray()
 */

/*!
   \overload

   Appends the text string \a str to the stream, creating a CBOR Text String
   value. QCborStreamWriter will attempt to write the entire string in one
   chunk.

   The following example appends a simple string to the stream:

   \code
     writer.append(QLatin1String("Hello, World"));
   \endcode

   \b{Performance note}: CBOR requires that all Text Strings be encoded in
   UTF-8, so this function will iterate over the characters in the string to
   determine whether the contents are US-ASCII or not. If the string is found
   to contain characters outside of US-ASCII, it will allocate memory and
   convert to UTF-8. If this check is unnecessary, use appendTextString()
   instead.

   \sa QCborStreamReader::isString(), QCborStreamReader::readString()
 */
void QCborStreamWriter::append(QLatin1String str)
{
    // We've got Latin-1 but CBOR wants UTF-8, so check if the string is the
    // common subset (US-ASCII).
    if (QtPrivate::isAscii(str)) {
        // it is plain US-ASCII
        appendTextString(str.latin1(), str.size());
    } else {
        // non-ASCII, so we need a pass-through UTF-16
        append(QString(str));
    }
}

/*!
   \overload

   Appends the text string \a str to the stream, creating a CBOR Text String
   value. QCborStreamWriter will attempt to write the entire string in one
   chunk.

   The following example writes an arbitrary QString to the stream:

   \code
     void writeString(QCborStreamWriter &writer, const QString &str)
     {
         writer.append(str);
     }
   \endcode

   \sa QCborStreamReader::isString(), QCborStreamReader::readString()
 */
void QCborStreamWriter::append(QStringView str)
{
    QByteArray utf8 = str.toUtf8();
    appendTextString(utf8.constData(), utf8.size());
}

/*!
   \overload

   Appends the CBOR tag \a tag to the stream, creating a CBOR Tag value. All
   tags must be followed by another type which they provide meaning for.

   In the following example, we append a CBOR Tag 36 (Regular Expression) and a
   QRegularExpression's pattern to the stream:

   \code
     void writeRxPattern(QCborStreamWriter &writer, const QRegularExpression &rx)
     {
         writer.append(QCborTag(36));
         writer.append(rx.pattern());
     }
   \endcode

   \sa QCborStreamReader::isTag(), QCborStreamReader::toTag()
 */
void QCborStreamWriter::append(QCborTag tag)
{
    d->executeAppend(cbor_encode_tag, CborTag(tag));
}

/*!
   \fn void QCborStreamWriter::append(QCborKnownTags tag)
   \overload

   Appends the CBOR tag \a tag to the stream, creating a CBOR Tag value. All
   tags must be followed by another type which they provide meaning for.

   In the following example, we append a CBOR Tag 1 (Unix \c time_t) and an
   integer representing the current time to the stream, obtained using the \c
   time() function:

   \code
     void writeCurrentTime(QCborStreamWriter &writer)
     {
         writer.append(QCborKnownTags::UnixTime_t);
         writer.append(time(nullptr));
     }
   \endcode

   \sa QCborStreamReader::isTag(), QCborStreamReader::toTag()
 */

/*!
   \overload

   Appends the CBOR simple type \a st to the stream, creating a CBOR Simple
   Type value. In the following example, we write the simple type for Null as
   well as for type 32, which Qt has no support for.

   \code
     writer.append(QCborSimpleType::Null);
     writer.append(QCborSimpleType(32));
   \endcode

   \note Using Simple Types for which there is no specification can lead to
   validation errors by the remote receiver. In addition, simple type values 24
   through 31 (inclusive) are reserved and must not be used.

   \sa QCborStreamReader::isSimpleType(), QCborStreamReader::toSimpleType()
 */
void QCborStreamWriter::append(QCborSimpleType st)
{
    d->executeAppend(cbor_encode_simple_value, uint8_t(st));
}

/*!
   \overload

   Appends the floating point number \a f to the stream, creating a CBOR 16-bit
   Half-Precision Floating Point value. The following code can be used to convert
   a C++ \tt float to \c qfloat16 if there's no loss of precision and append it, or
   instead append the \tt float.

   \code
     void writeFloat(QCborStreamWriter &writer, float f)
     {
         qfloat16 f16 = f;
         if (qIsNaN(f) || f16 == f)
             writer.append(f16);
         else
             writer.append(f);
     }
   \endcode

   \sa QCborStreamReader::isFloat16(), QCborStreamReader::toFloat16()
 */
void QCborStreamWriter::append(qfloat16 f)
{
    d->executeAppend(cbor_encode_half_float, static_cast<const void *>(&f));
}

/*!
   \overload

   Appends the floating point number \a f to the stream, creating a CBOR 32-bit
   Single-Precision Floating Point value. The following code can be used to convert
   a C++ \tt double to \tt float if there's no loss of precision and append it, or
   instead append the \tt double.

   \code
     void writeFloat(QCborStreamWriter &writer, double d)
     {
         float f = d;
         if (qIsNaN(d) || d == f)
             writer.append(f);
         else
             writer.append(d);
     }
   \endcode

   \sa QCborStreamReader::isFloat(), QCborStreamReader::toFloat()
 */
void QCborStreamWriter::append(float f)
{
    d->executeAppend(cbor_encode_float, f);
}

/*!
   \overload

   Appends the floating point number \a d to the stream, creating a CBOR 64-bit
   Double-Precision Floating Point value. QCborStreamWriter always appends the
   number as-is, performing no check for whether the number is the canonical
   form for NaN, an infinite, whether it is denormal or if it could be written
   with a shorter format.

   The following code performs all those checks, except for the denormal one,
   which is expected to be taken into account by the system FPU or floating
   point emulation directly.

   \code
     void writeDouble(QCborStreamWriter &writer, double d)
     {
         float f;
         if (qIsNaN(d)) {
             writer.append(qfloat16(qQNaN()));
         } else if (qIsInf(d)) {
             writer.append(d < 0 ? -qInf() : qInf());
         } else if ((f = d) == d) {
             qfloat16 f16 = f;
             if (f16 == f)
                 writer.append(f16);
             else
                 writer.append(f);
         } else {
             writer.append(d);
         }
     }
   \endcode

   Determining if a double can be converted to an integral with no loss of
   precision is left as an exercise to the reader.

   \sa QCborStreamReader::isDouble(), QCborStreamReader::toDouble()
 */
void QCborStreamWriter::append(double d)
{
    this->d->executeAppend(cbor_encode_double, d);
}

/*!
   Appends \a len bytes of data starting from \a data to the stream, creating a
   CBOR Byte String value. QCborStreamWriter will attempt to write the entire
   string in one chunk.

   Unlike the QByteArray overload of append(), this function is not limited by
   QByteArray's size limits. However, note that neither
   QCborStreamReader::readByteArray() nor QCborValue support reading CBOR
   streams with byte arrays larger than 2 GB.

   \sa append(), appendTextString(),
       QCborStreamReader::isByteArray(), QCborStreamReader::readByteArray()
 */
void QCborStreamWriter::appendByteString(const char *data, qsizetype len)
{
    d->executeAppend(cbor_encode_byte_string, reinterpret_cast<const uint8_t *>(data), size_t(len));
}

/*!
   Appends \a len bytes of text starting from \a utf8 to the stream, creating a
   CBOR Text String value. QCborStreamWriter will attempt to write the entire
   string in one chunk.

   The string pointed to by \a utf8 is expected to be properly encoded UTF-8.
   QCborStreamWriter performs no validation that this is the case.

   Unlike the QLatin1String overload of append(), this function is not limited
   to 2 GB. However, note that neither QCborStreamReader::readString() nor
   QCborValue support reading CBOR streams with text strings larger than 2 GB.

   \sa append(QLatin1String), append(QStringView),
       QCborStreamReader::isString(), QCborStreamReader::readString()
 */
void QCborStreamWriter::appendTextString(const char *utf8, qsizetype len)
{
    d->executeAppend(cbor_encode_text_string, utf8, size_t(len));
}

/*!
   \fn void QCborStreamWriter::append(const char *str, qsizetype size)
   \overload

   Appends \a size bytes of text starting from \a str to the stream, creating a
   CBOR Text String value. QCborStreamWriter will attempt to write the entire
   string in one chunk. If \a size is -1, this function will write \c strlen(\a
   str) bytes.

   The string pointed to by \a str is expected to be properly encoded UTF-8.
   QCborStreamWriter performs no validation that this is the case.

   Unlike the QLatin1String overload of append(), this function is not limited
   to 2 GB. However, note that neither QCborStreamReader nor QCborValue support
   reading CBOR streams with text strings larger than 2 GB.

   \sa append(QLatin1String), append(QStringView),
       QCborStreamReader::isString(), QCborStreamReader::readString()
 */

/*!
   \fn void QCborStreamWriter::append(bool b)
   \overload

   Appends the boolean value \a b to the stream, creating either a CBOR False
   value or a CBOR True value. This function is equivalent to (and implemented
   as):

   \code
     writer.append(b ? QCborSimpleType::True : QCborSimpleType::False);
   \endcode

   \sa appendNull(), appendUndefined(),
       QCborStreamReader::isBool(), QCborStreamReader::toBool()
 */

/*!
   \fn void QCborStreamWriter::append(std::nullptr_t)
   \overload

   Appends a CBOR Null value to the stream. This function is equivalent to (and
   implemented as): The parameter is ignored.

   \code
     writer.append(QCborSimpleType::Null);
   \endcode

   \sa appendNull(), append(QCborSimpleType), QCborStreamReader::isNull()
 */

/*!
   \fn void QCborStreamWriter::appendNull()

   Appends a CBOR Null value to the stream. This function is equivalent to (and
   implemented as):

   \code
     writer.append(QCborSimpleType::Null);
   \endcode

   \sa append(std::nullptr_t), append(QCborSimpleType), QCborStreamReader::isNull()
 */

/*!
   \fn void QCborStreamWriter::appendUndefined()

   Appends a CBOR Undefined value to the stream. This function is equivalent to (and
   implemented as):

   \code
     writer.append(QCborSimpleType::Undefined);
   \endcode

   \sa append(QCborSimpleType), QCborStreamReader::isUndefined()
 */

/*!
   Starts a CBOR Array with indeterminate length in the CBOR stream. Each
   startArray() call must be paired with one endArray() call and the current
   CBOR element extends until the end of the array.

   The array created by this function has no explicit length. Instead, its
   length is implied by the elements contained in it. Note, however, that use
   of indeterminate-length arrays is not compliant with canonical CBOR encoding.

   The following example appends elements from the linked list of strings
   passed as input:

   \code
     void appendList(QCborStreamWriter &writer, const QLinkedList<QString> &list)
     {
         writer.startArray();
         for (const QString &s : list)
             writer.append(s);
         writer.endArray();
     }
   \endcode

   \sa startArray(quint64), endArray(), startMap(), QCborStreamReader::isArray(),
   QCborStreamReader::isLengthKnown()
 */
void QCborStreamWriter::startArray()
{
    d->createContainer(cbor_encoder_create_array);
}

/*!
   \overload

   Starts a CBOR Array with explicit length of \a count items in the CBOR
   stream. Each startArray call must be paired with one endArray() call and the
   current CBOR element extends until the end of the array.

   The array created by this function has an explicit length and therefore
   exactly \a count items must be added to the CBOR stream. Adding fewer or
   more items will result in failure during endArray() and the CBOR stream will
   be corrupt. However, explicit-length arrays are required by canonical CBOR
   encoding.

   The following example appends all strings found in the \l QStringList passed as input:

   \code
     void appendList(QCborStreamWriter &writer, const QStringList &list)
     {
         writer.startArray(list.size());
         for (const QString &s : list)
             writer.append(s);
         writer.endArray();
     }
   \endcode

   \b{Size limitations}: The parameter to this function is quint64, which would
   seem to allow up to 2\sup{64}-1 elements in the array. However, both
   QCborStreamWriter and QCborStreamReader are currently limited to 2\sup{32}-2
   items on 32-bit systems and 2\sup{64}-2 items on 64-bit ones. Also note that
   QCborArray is currently limited to 2\sup{27} elements in any platform.

   \sa startArray(), endArray(), startMap(), QCborStreamReader::isArray(),
   QCborStreamReader::isLengthKnown()
 */
void QCborStreamWriter::startArray(quint64 count)
{
    d->createContainer(cbor_encoder_create_array, count);
}

/*!
   Terminates the array started by either overload of startArray() and returns
   true if the correct number of elements was added to the array. This function
   must be called for every startArray() used.

   A return of false indicates error in the application and an unrecoverable
   error in this stream. QCborStreamWriter also writes a warning using
   qWarning() if that happens.

   Calling this function when the current container is not an array is also an
   error, though QCborStreamWriter cannot currently detect this condition.

   \sa startArray(), startArray(quint64), endMap()
 */
bool QCborStreamWriter::endArray()
{
    return d->closeContainer();
}

/*!
   Starts a CBOR Map with indeterminate length in the CBOR stream. Each
   startMap() call must be paired with one endMap() call and the current CBOR
   element extends until the end of the map.

   The map created by this function has no explicit length. Instead, its length
   is implied by the elements contained in it. Note, however, that use of
   indeterminate-length maps is not compliant with canonical CBOR encoding
   (canonical encoding also requires keys to be unique and in sorted order).

   The following example appends elements from the linked list of int and
   string pairs passed as input:

   \code
     void appendMap(QCborStreamWriter &writer, const QLinkedList<QPair<int, QString>> &list)
     {
         writer.startMap();
         for (const auto pair : list) {
             writer.append(pair.first)
             writer.append(pair.second);
         }
         writer.endMap();
     }
   \endcode

   \sa startMap(quint64), endMap(), startArray(), QCborStreamReader::isMap(),
       QCborStreamReader::isLengthKnown()
 */
void QCborStreamWriter::startMap()
{
    d->createContainer(cbor_encoder_create_map);
}

/*!
   \overload

   Starts a CBOR Map with explicit length of \a count items in the CBOR
   stream. Each startMap call must be paired with one endMap() call and the
   current CBOR element extends until the end of the map.

   The map created by this function has an explicit length and therefore
   exactly \a count pairs of items must be added to the CBOR stream. Adding
   fewer or more items will result in failure during endMap() and the CBOR
   stream will be corrupt. However, explicit-length map are required by
   canonical CBOR encoding.

   The following example appends all strings found in the \l QMap passed as input:

   \code
     void appendMap(QCborStreamWriter &writer, const QMap<int, QString> &map)
     {
         writer.startMap(map.size());
         for (auto it = map.begin(); it != map.end(); ++it) {
             writer.append(it.key());
             writer.append(it.value());
         }
         writer.endMap();
     }
   \endcode

   \b{Size limitations}: The parameter to this function is quint64, which would
   seem to allow up to 2\sup{64}-1 pairs in the map. However, both
   QCborStreamWriter and QCborStreamReader are currently limited to 2\sup{31}-1
   items on 32-bit systems and 2\sup{63}-1 items on 64-bit ones. Also note that
   QCborMap is currently limited to 2\sup{26} elements in any platform.

   \sa startMap(), endMap(), startArray(), QCborStreamReader::isMap(),
       QCborStreamReader::isLengthKnown()
 */
void QCborStreamWriter::startMap(quint64 count)
{
    d->createContainer(cbor_encoder_create_map, count);
}

/*!
   Terminates the map started by either overload of startMap() and returns
   true if the correct number of elements was added to the array. This function
   must be called for every startMap() used.

   A return of false indicates error in the application and an unrecoverable
   error in this stream. QCborStreamWriter also writes a warning using
   qWarning() if that happens.

   Calling this function when the current container is not a map is also an
   error, though QCborStreamWriter cannot currently detect this condition.

   \sa startMap(), startMap(quint64), endArray()
 */
bool QCborStreamWriter::endMap()
{
    return d->closeContainer();
}

/*!
   \class QCborStreamReader
   \inmodule QtCore
   \ingroup cbor
   \reentrant
   \since 5.12

   \brief The QCborStreamReader class is a simple CBOR stream decoder, operating
   on either a QByteArray or QIODevice.

   This class can be used to decode a stream of CBOR content directly from
   either a QByteArray or a QIODevice. CBOR is the Concise Binary Object
   Representation, a very compact form of binary data encoding that is
   compatible with JSON. It was created by the IETF Constrained RESTful
   Environments (CoRE) WG, which has used it in many new RFCs. It is meant to
   be used alongside the \l{https://tools.ietf.org/html/rfc7252}{CoAP
   protocol}.

   QCborStreamReader provides a StAX-like API, similar to that of
   \l{QXmlStreamReader}. Using it requires a bit of knowledge of CBOR encoding.
   For a simpler API, see \l{QCborValue} and especially the decoding function
   QCborValue::fromCbor().

   Typically, one creates a QCborStreamReader by passing the source QByteArray
   or QIODevice as a parameter to the constructor, then pop elements off the
   stream if there were no errors in decoding. There are three kinds of CBOR
   types:

   \table
     \header \li Kind       \li Types       \li Behavior
     \row   \li Fixed-width \li Integers, Tags, Simple types, Floating point
            \li Value is pre-parsed by QCborStreamReader, so accessor functions
                are \c const. Must call next() to advance.
     \row   \li Strings     \li Byte arrays, Text strings
            \li Length (if known) is pre-parsed, but the string itself is not.
                The accessor functions are not const and may allocate memory.
                Once called, the accessor functions automatically advance to
                the next element.
     \row   \li Containers  \li Arrays, Maps
            \li Length (if known) is pre-parsed. To access the elements, you
                must call enterContainer(), read all elements, then call
                leaveContainer(). That function advances to the next element.
   \endtable

   So a processor function typically looks like this:

   \code
   void handleStream(QCborStreamReader &reader)
   {
       switch (reader.type())
       case QCborStreamReader::UnsignedInteger:
       case QCborStreamReader::NegativeInteger:
       case QCborStreamReader::SimpleType:
       case QCborStreamReader::Float16:
       case QCborStreamReader::Float:
       case QCborStreamReader::Double:
           handleFixedWidth(reader);
           reader.next();
           break;
       case QCborStreamReader::ByteArray:
       case QCborStreamReader::String:
           handleString(reader);
           break;
       case QCborStreamReader::Array:
       case QCborStreamReader::Map:
           reader.enterContainer();
           while (reader.lastError() == QCborError::NoError)
               handleStream(reader);
           if (reader.lastError() == QCborError::NoError)
               reader.leaveContainer();
       }
   }
   \endcode

   \section1 CBOR support

   The following table lists the CBOR features that QCborStreamReader supports.

   \table
     \header \li Feature                        \li Support
     \row   \li Unsigned numbers                \li Yes (full range)
     \row   \li Negative numbers                \li Yes (full range)
     \row   \li Byte strings                    \li Yes
     \row   \li Text strings                    \li Yes
     \row   \li Chunked strings                 \li Yes
     \row   \li Tags                            \li Yes (arbitrary)
     \row   \li Booleans                        \li Yes
     \row   \li Null                            \li Yes
     \row   \li Undefined                       \li Yes
     \row   \li Arbitrary simple values         \li Yes
     \row   \li Half-precision float (16-bit)   \li Yes
     \row   \li Single-precision float (32-bit) \li Yes
     \row   \li Double-precision float (64-bit) \li Yes
     \row   \li Infinities and NaN floating point \li Yes
     \row   \li Determinate-length arrays and maps \li Yes
     \row   \li Indeterminate-length arrays and maps \li Yes
     \row   \li Map key types other than strings and integers \li Yes (arbitrary)
   \endtable

   \section1 Dealing with invalid or incomplete CBOR streams

   QCborStreamReader is capable of detecting corrupt input on its own. The
   library it uses has been extensively tested against invalid input of any
   kind and is quite able to report errors. If any is detected,
   QCborStreamReader will set lastError() to a value besides
   QCborError::NoError, indicating which situation was detected.

   Most errors detected by QCborStreamReader during normal item parsing are not
   recoverable. The code using QCborStreamReader may opt to handle the data
   that was properly decoded or it can opt to discard the entire data.

   The only recoverable error is QCborError::EndOfFile, which indicates that
   more data is required in order to complete the parsing. This situation is
   useful when data is being read from an asynchronous source, such as a pipe
   (QProcess) or a socket (QTcpSocket, QUdpSocket, QNetworkReply, etc.). When
   more data arrives, the surrounding code needs to call either addData(), if
   parsing from a QByteArray, or reparse(), if it is instead reading directly
   a the QIDOevice that now has more data available (see setDevice()).

   \sa QCborStreamWriter, QCborValue, QXmlStreamReader
 */

/*!
   \enum QCborStreamReader::Type

   This enumeration contains all possible CBOR types as decoded by
   QCborStreamReader. CBOR has 7 major types, plus a number of simple types
   carrying no value, and floating point values.

   \value UnsignedInteger       (Major type 0) Ranges from 0 to 2\sup{64} - 1
                                (18,446,744,073,709,551,616)
   \value NegativeInteger       (Major type 1) Ranges from -1 to -2\sup{64}
                                (-18,446,744,073,709,551,616)
   \value ByteArray             (Major type 2) Arbitrary binary data.
   \value ByteString            An alias to ByteArray.
   \value String                (Major type 3) Unicode text, possibly containing NULs.
   \value TextString            An alias to String
   \value Array                 (Major type 4) Array of heterogeneous items.
   \value Map                   (Major type 5) Map/dictionary of heterogeneous items.
   \value Tag                   (Major type 6) Numbers giving further semantic value
                                to generic CBOR items. See \l QCborTag for more information.
   \value SimpleType            (Major type 7) Types carrying no further value. Includes
                                booleans (true and false), null, undefined.
   \value Float16               IEEE 754 half-precision floating point (\c qfloat16).
   \value HalfFloat             An alias to Float16.
   \value Float                 IEEE 754 single-precision floating point (\tt float).
   \value Double                IEEE 754 double-precision floating point (\tt double).
   \value Invalid               Not a valid type, either due to parsing error or due to
                                reaching the end of an array or map.
 */

/*!
   \enum QCborStreamReader::StringResultCode

   This enum is returned by readString() and readByteArray() and is used to
   indicate what the status of the parsing is.

   \value EndOfString           The parsing for the string is complete, with no error.
   \value Ok                    The function returned data; there was no error.
   \value Error                 Parsing failed with an error.
 */

/*!
   \class QCborStreamReader::StringResult

   This class is returned by readString() and readByteArray(), with either the
   contents of the string that was read or an indication that the parsing is
   done or found an error.

   The contents of \l data are valid only if \l status is
   \l{StringResultCode}{Ok}. Otherwise, it should be null.
 */

/*!
   \variable Container QCborStreamReader::StringResult::data
   Contains the actual data from the string if \l status is \c Ok.
 */

/*!
   \variable QCborStreamReader::StringResultCode QCborStreamReader::StringResult::status
   Contains the status of the attempt of reading the string from the stream.
 */

/*!
   \fn QCborStreamReader::Type QCborStreamReader::type() const

   Returns the type of the current element. It is one of the valid types or
   Invalid.

   \sa isValid(), isUnsignedInteger(), isNegativeInteger(), isInteger(),
       isByteArray(), isString(), isArray(), isMap(), isTag(), isSimpleType(),
       isBool(), isFalse(), isTrue(), isNull(), isUndefined(), isFloat16(),
       isFloat(), isDouble()
 */

/*!
   \fn bool QCborStreamReader::isValid() const

   Returns true if the current element is valid, false otherwise. The current
   element may be invalid if there was a decoding error or we've just parsed
   the last element in an array or map.

   \note This function is not the opposite of isNull(). Null is a normal CBOR
   type that must be handled by the application.

   \sa type(), isInvalid()
 */

/*!
    \fn bool QCborStreamReader::isInvalid() const

    Returns true if the current element is invalid, false otherwise. The current
   element may be invalid if there was a decoding error or we've just parsed
   the last element in an array or map.

   \note This function is not to be confused with isNull(). Null is a normal
   CBOR type that must be handled by the application.

   \sa type(), isValid()
 */

/*!
   \fn bool QCborStreamReader::isUnsignedInteger() const

   Returns true if the type of the current element is an unsigned integer (that
   is if type() returns QCborStreamReader::UnsignedInteger). If this function
   returns true, you may call toUnsignedInteger() or toInteger() to read that value.

   \sa type(), toUnsignedInteger(), toInteger(), isInteger(), isNegativeInteger()
 */

/*!
   \fn bool QCborStreamReader::isNegativeInteger() const

   Returns true if the type of the current element is a negative integer (that
   is if type() returns QCborStreamReader::NegativeInteger). If this function
   returns true, you may call toNegativeInteger() or toInteger() to read that value.

   \sa type(), toNegativeInteger(), toInteger(), isInteger(), isUnsignedInteger()
 */

/*!
   \fn bool QCborStreamReader::isInteger() const

   Returns true if the type of the current element is either an unsigned
   integer or a negative one (that is, if type() returns
   QCborStreamReader::UnsignedInteger or QCborStreamReader::NegativeInteger).
   If this function returns true, you may call toInteger() to read that
   value.

   \sa type(), toInteger(), toUnsignedInteger(), toNegativeInteger(),
   isUnsignedInteger(), isNegativeInteger()
 */

/*!
   \fn bool QCborStreamReader::isByteArray() const

   Returns true if the type of the current element is a byte array (that is,
   if type() returns QCborStreamReader::ByteArray). If this function returns
   true, you may call readByteArray() to read that data.

   \sa type(), readByteArray(), isString()
 */

/*!
   \fn bool QCborStreamReader::isString() const

   Returns true if the type of the current element is a text string (that is,
   if type() returns QCborStreamReader::String). If this function returns
   true, you may call readString() to read that data.

   \sa type(), readString(), isByteArray()
 */

/*!
   \fn bool QCborStreamReader::isArray() const

   Returns true if the type of the current element is an array (that is,
   if type() returns QCborStreamReader::Array). If this function returns
   true, you may call enterContainer() to begin parsing that container.

   When the current element is an array, you may also call isLengthKnown() to
   find out if the array's size is explicit in the CBOR stream. If it is, that
   size can be obtained by calling length().

   The following example pre-allocates a QVariantList given the array's size
   for more efficient decoding:

   \code
     QVariantList populateFromCbor(QCborStreamReader &reader)
     {
         QVariantList list;
         if (reader.isLengthKnown())
             list.reserve(reader.length());

         reader.enterContainer();
         while (reader.lastError() == QCborError::NoError && reader.hasNext())
             list.append(readOneElement(reader));
         if (reader.lastError() == QCborError::NoError)
             reader.leaveContainer();
     }
   \endcode

   \note The code above does not validate that the length is a sensible value.
   If the input stream reports that the length is 1 billion elements, the above
   function will try to allocate some 16 GB or more of RAM, which can lead to a
   crash.

   \sa type(), isMap(), isLengthKnown(), length(), enterContainer(), leaveContainer()
 */

/*!
   \fn bool QCborStreamReader::isMap() const

   Returns true if the type of the current element is a map (that is, if type()
   returns QCborStreamReader::Map). If this function returns true, you may call
   enterContainer() to begin parsing that container.

   When the current element is a map, you may also call isLengthKnown() to
   find out if the map's size is explicit in the CBOR stream. If it is, that
   size can be obtained by calling length().

   The following example pre-allocates a QVariantMap given the map's size
   for more efficient decoding:

   \code
     QVariantMap populateFromCbor(QCborStreamReader &reader)
     {
         QVariantMap map;
         if (reader.isLengthKnown())
             map.reserve(reader.length());

         reader.enterContainer();
         while (reader.lastError() == QCborError::NoError && reader.hasNext()) {
             QString key = readElementAsString(reader);
             map.insert(key, readOneElement(reader));
         }
         if (reader.lastError() == QCborError::NoError)
             reader.leaveContainer();
     }
   \endcode

   The example above uses a function called \c readElementAsString to read the
   map's keys and obtain a string. That is because CBOR maps may contain any
   type as keys, not just strings. User code needs to either perform this
   conversion, reject non-string keys, or instead use a different container
   besides \l QVariantMap and \l QVariantHash. For example, if the map is
   expected to contain integer keys, which is recommended as it reduces stream
   size and parsing, the correct container would be \c{\l{QMap}<int, QVariant>}
   or \c{\l{QHash}<int, QVariant>}.

   \note The code above does not validate that the length is a sensible value.
   If the input stream reports that the length is 1 billion elements, the above
   function will try to allocate some 24 GB or more of RAM, which can lead to a
   crash.

   \sa type(), isArray(), isLengthKnown(), length(), enterContainer(), leaveContainer()
 */

/*!
   \fn bool QCborStreamReader::isTag() const

   Returns true if the type of the current element is a CBOR tag (that is,
   if type() returns QCborStreamReader::Tag). If this function returns
   true, you may call toTag() to read that data.

   \sa type(), toTag()
 */

/*!
   \fn bool QCborStreamReader::isFloat16() const

   Returns true if the type of the current element is an IEEE 754
   half-precision floating point (that is, if type() returns
   QCborStreamReader::Float16). If this function returns true, you may call
   toFloat16() to read that data.

   \sa type(), toFloat16(), isFloat(), isDouble()
 */

/*!
   \fn bool QCborStreamReader::isFloat() const

   Returns true if the type of the current element is an IEEE 754
   single-precision floating point (that is, if type() returns
   QCborStreamReader::Float). If this function returns true, you may call
   toFloat() to read that data.

   \sa type(), toFloat(), isFloat16(), isDouble()
 */

/*!
   \fn bool QCborStreamReader::isDouble() const

   Returns true if the type of the current element is an IEEE 754
   double-precision floating point (that is, if type() returns
   QCborStreamReader::Double). If this function returns true, you may call
   toDouble() to read that data.

   \sa type(), toDouble(), isFloat16(), isFloat()
 */

/*!
   \fn bool QCborStreamReader::isSimpleType() const

   Returns true if the type of the current element is any CBOR simple type,
   including a boolean value (true and false) as well as null and undefined. To
   find out which simple type this is, call toSimpleType(). Alternatively, to
   test for one specific simple type, call the overload that takes a
   QCborSimpleType parameter.

   CBOR simple types are types that do not carry extra value. There are 255
   possibilities, but there are currently only four values that have defined
   meaning. Code is not expected to cope with unknown simple types and may
   simply discard the stream as invalid if it finds an unknown one.

   \sa QCborSimpleType, type(), isSimpleType(QCborSimpleType), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isSimpleType(QCborSimpleType st) const

   Returns true if the type of the current element is the simple type \a st,
   false otherwise. If this function returns true, then toSimpleType() will
   return \a st.

   CBOR simple types are types that do not carry extra value. There are 255
   possibilities, but there are currently only four values that have defined
   meaning. Code is not expected to cope with unknown simple types and may
   simply discard the stream as invalid if it finds an unknown one.

   \sa QCborSimpleType, type(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isFalse() const

   Returns true if the current element is the \c false value, false if it is
   anything else.

   \sa type(), isTrue(), isBool(), toBool(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isTrue() const

   Returns true if the current element is the \c true value, false if it is
   anything else.

   \sa type(), isFalse(), isBool(), toBool(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isBool() const

   Returns true if the current element is a boolean value (\c true or \c
   false), false if it is anything else. If this function returns true, you may
   call toBool() to retrieve the value of the boolean. You may also call
   toSimpleType() and compare to either QCborSimpleValue::True or
   QCborSimpleValue::False.

   \sa type(), isFalse(), isTrue(), toBool(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isNull() const

   Returns true if the current element is the \c null value, false if it is
   anything else. Null values may be used to indicate the absence of some
   optional data.

   \note This function is not the opposite of isValid(). A Null value is a
   valid CBOR value.

   \sa type(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isUndefined() const

   Returns true if the current element is the \c undefined value, false if it
   is anything else. Undefined values may be encoded to indicate that some
   conversion failed or was not possible when creating the stream.
   QCborStreamReader never performs any replacement and this function will only
   return true if the stream contains an explicit undefined value.

   \sa type(), isSimpleType(), toSimpleType()
 */

/*!
   \fn bool QCborStreamReader::isContainer() const

   Returns true if the current element is a container (that is, an array or a
   map), false if it is anything else. If the current element is a container,
   the isLengthKnown() function may be used to find out if the container's size
   is explicit in the stream and, if so, length() can be used to get that size.

   More importantly, for a container, the enterContainer() function is
   available to begin iterating through the elements contained therein.

   \sa type(), isArray(), isMap(), isLengthKnown(), length(), enterContainer(),
       leaveContainer(), containerDepth()
 */

class QCborStreamReaderPrivate
{
public:
    enum {
        // 9 bytes is the maximum size for any integer, floating point or
        // length in CBOR.
        MaxCborIndividualSize = 9,
        IdealIoBufferSize = 256
    };

    QIODevice *device;
    QByteArray buffer;
    QStack<CborValue> containerStack;

    CborParser parser;
    CborValue currentElement;
    QCborError lastError = {};

    QByteArray::size_type bufferStart;
    bool corrupt = false;

    QCborStreamReaderPrivate(const QByteArray &data)
        : device(nullptr), buffer(data)
    {
        initDecoder();
    }

    QCborStreamReaderPrivate(QIODevice *device)
    {
        setDevice(device);
    }

    ~QCborStreamReaderPrivate()
    {
    }

    void setDevice(QIODevice *dev)
    {
        buffer.clear();
        device = dev;
        initDecoder();
    }

    void initDecoder()
    {
        containerStack.clear();
        bufferStart = 0;
        if (device) {
            buffer.clear();
            buffer.reserve(IdealIoBufferSize);      // sets the CapacityReserved flag
        }

        preread();
        if (CborError err = cbor_parser_init_reader(nullptr, &parser, &currentElement, this))
            handleError(err);
    }

    char *bufferPtr()
    {
        Q_ASSERT(buffer.isDetached());
        return const_cast<char *>(buffer.constData()) + bufferStart;
    }

    void preread()
    {
        if (device && buffer.size() - bufferStart < MaxCborIndividualSize) {
            // load more, but only if there's more to be read
            qint64 avail = device->bytesAvailable();
            Q_ASSERT(avail >= buffer.size());
            if (avail == buffer.size())
                return;

            if (bufferStart)
                device->skip(bufferStart);  // skip what we've already parsed

            if (buffer.size() != IdealIoBufferSize)
                buffer.resize(IdealIoBufferSize);

            bufferStart = 0;
            qint64 read = device->peek(bufferPtr(), IdealIoBufferSize);
            if (read < 0)
                buffer.clear();
            else if (read != IdealIoBufferSize)
                buffer.truncate(read);
        }
    }

    void handleError(CborError err) noexcept
    {
        Q_ASSERT(err);

        // is the error fatal?
        if (err != CborErrorUnexpectedEOF)
            corrupt = true;

        // our error codes are the same (for now)
        lastError = { QCborError::Code(err) };
    }

    void updateBufferAfterString(qsizetype offset, qsizetype size)
    {
        Q_ASSERT(device);

        bufferStart += offset;
        qsizetype newStart = bufferStart + size;
        qsizetype remainingInBuffer = buffer.size() - newStart;

        if (remainingInBuffer <= 0) {
            // We've read from the QIODevice more than what was in the buffer.
            buffer.truncate(0);
        } else {
            // There's still data buffered, but we need to move it around.
            char *ptr = buffer.data();
            memmove(ptr, ptr + newStart, remainingInBuffer);
            buffer.truncate(remainingInBuffer);
        }

        bufferStart = 0;
    }

    bool ensureStringIteration();
};

void qt_cbor_stream_set_error(QCborStreamReaderPrivate *d, QCborError error)
{
    d->handleError(CborError(error.c));
}

static inline bool qt_cbor_decoder_can_read(void *token, size_t len)
{
    Q_ASSERT(len <= QCborStreamReaderPrivate::MaxCborIndividualSize);
    auto self = static_cast<QCborStreamReaderPrivate *>(token);

    qint64 avail = self->buffer.size() - self->bufferStart;
    return len <= quint64(avail);
}

static void qt_cbor_decoder_advance(void *token, size_t len)
{
    Q_ASSERT(len <= QCborStreamReaderPrivate::MaxCborIndividualSize);
    auto self = static_cast<QCborStreamReaderPrivate *>(token);
    Q_ASSERT(len <= size_t(self->buffer.size() - self->bufferStart));

    self->bufferStart += int(len);
    self->preread();
}

static void *qt_cbor_decoder_read(void *token, void *userptr, size_t offset, size_t len)
{
    Q_ASSERT(len == 1 || len == 2 || len == 4 || len == 8);
    Q_ASSERT(offset == 0 || offset == 1);
    auto self = static_cast<const QCborStreamReaderPrivate *>(token);

    // we must have pre-read the data
    Q_ASSERT(len + offset <= size_t(self->buffer.size() - self->bufferStart));
    return memcpy(userptr, self->buffer.constData() + self->bufferStart + offset, len);
}

static CborError qt_cbor_decoder_transfer_string(void *token, const void **userptr, size_t offset, size_t len)
{
    auto self = static_cast<QCborStreamReaderPrivate *>(token);
    Q_ASSERT(offset <= size_t(self->buffer.size()));
    Q_STATIC_ASSERT(sizeof(size_t) >= sizeof(QByteArray::size_type));
    Q_STATIC_ASSERT(sizeof(size_t) == sizeof(qsizetype));

    // check that we will have enough data from the QIODevice before we advance
    // (otherwise, we'd lose the length information)
    qsizetype total;
    if (len > size_t(std::numeric_limits<QByteArray::size_type>::max())
            || add_overflow<qsizetype>(offset, len, &total))
        return CborErrorDataTooLarge;

    // our string transfer is just saving the offset to the userptr
    *userptr = reinterpret_cast<void *>(offset);

    qint64 avail = (self->device ? self->device->bytesAvailable() : self->buffer.size()) -
            self->bufferStart;
    return total > avail ? CborErrorUnexpectedEOF : CborNoError;
}

bool QCborStreamReaderPrivate::ensureStringIteration()
{
    if (currentElement.flags & CborIteratorFlag_IteratingStringChunks)
        return true;

    CborError err = cbor_value_begin_string_iteration(&currentElement);
    if (!err)
        return true;
    handleError(err);
    return false;
}

/*!
   \internal
 */
inline void QCborStreamReader::preparse()
{
    if (lastError() == QCborError::NoError) {
        type_ = cbor_value_get_type(&d->currentElement);

        if (type_ != CborInvalidType) {
            d->lastError = {};
            // Undo the type mapping that TinyCBOR does (we have an explicit type
            // for negative integer and we don't have separate types for Boolean,
            // Null and Undefined).
            if (type_ == CborBooleanType || type_ == CborNullType || type_ == CborUndefinedType) {
                type_ = SimpleType;
                value64 = quint8(d->buffer.at(d->bufferStart)) - CborSimpleType;
            } else {
                // Using internal TinyCBOR API!
                value64 = _cbor_value_extract_int64_helper(&d->currentElement);

                if (cbor_value_is_negative_integer(&d->currentElement))
                    type_ = quint8(QCborStreamReader::NegativeInteger);
            }
        }
    } else {
        type_ = Invalid;
    }
}

/*!
   Creates a QCborStreamReader object with no source data. After construction,
   QCborStreamReader will report an error parsing.

   You can add more data by calling addData() or by setting a different source
   device using setDevice().

   \sa addData(), isValid()
 */
QCborStreamReader::QCborStreamReader()
    : QCborStreamReader(QByteArray())
{
}

/*!
   \overload

   Creates a QCborStreamReader object with \a len bytes of data starting at \a
   data. The pointer must remain valid until QCborStreamReader is destroyed.
 */
QCborStreamReader::QCborStreamReader(const char *data, qsizetype len)
    : QCborStreamReader(QByteArray::fromRawData(data, len))
{
}

/*!
   \overload

   Creates a QCborStreamReader object with \a len bytes of data starting at \a
   data. The pointer must remain valid until QCborStreamReader is destroyed.
 */
QCborStreamReader::QCborStreamReader(const quint8 *data, qsizetype len)
    : QCborStreamReader(QByteArray::fromRawData(reinterpret_cast<const char *>(data), len))
{
}

/*!
   \overload

   Creates a QCborStreamReader object that will parse the CBOR stream found in
   \a data.
 */
QCborStreamReader::QCborStreamReader(const QByteArray &data)
    : d(new QCborStreamReaderPrivate(data))
{
    preparse();
}

/*!
   \overload

   Creates a QCborStreamReader object that will parse the CBOR stream found by
   reading from \a device. QCborStreamReader does not take ownership of \a
   device, so it must remain valid until this oject is destroyed.
 */
QCborStreamReader::QCborStreamReader(QIODevice *device)
    : d(new QCborStreamReaderPrivate(device))
{
    preparse();
}

/*!
   Destroys this QCborStreamReader object and frees any associated resources.
 */
QCborStreamReader::~QCborStreamReader()
{
}

/*!
   Sets the source of data to \a device, resetting the decoder to its initial
   state.
 */
void QCborStreamReader::setDevice(QIODevice *device)
{
    d->setDevice(device);
    preparse();
}

/*!
   Returns the QIODevice that was set with either setDevice() or the
   QCborStreamReader constructor. If this object was reading from a QByteArray,
   this function returns nullptr instead.
 */
QIODevice *QCborStreamReader::device() const
{
    return d->device;
}

/*!
   Adds \a data to the CBOR stream and reparses the current element. This
   function is useful if the end of the data was previously reached while
   processing the stream, but now more data is available.
 */
void QCborStreamReader::addData(const QByteArray &data)
{
    addData(data.constData(), data.size());
}

/*!
   \fn void QCborStreamReader::addData(const quint8 *data, qsizetype len)
   \overload

   Adds \a len bytes of data starting at \a data to the CBOR stream and
   reparses the current element. This function is useful if the end of the data
   was previously reached while processing the stream, but now more data is
   available.
 */

/*!
   \overload

   Adds \a len bytes of data starting at \a data to the CBOR stream and
   reparses the current element. This function is useful if the end of the data
   was previously reached while processing the stream, but now more data is
   available.
 */
void QCborStreamReader::addData(const char *data, qsizetype len)
{
    if (!d->device) {
        if (len > 0)
            d->buffer.append(data, len);
        reparse();
    } else {
        qWarning("QCborStreamReader: addData() with device()");
    }
}

/*!
   Reparses the current element. This function must be called when more data
   becomes available in the source QIODevice after parsing failed due to
   reaching the end of the input data before the end of the CBOR stream.

   When reading from QByteArray(), the addData() function automatically calls
   this function. Calling it when the reading had not failed is a no-op.
 */
void QCborStreamReader::reparse()
{
    d->lastError = {};
    d->preread();
    if (CborError err = cbor_value_reparse(&d->currentElement))
        d->handleError(err);
    else
        preparse();
}

/*!
   Clears the decoder state and resets the input source data to an empty byte
   array. After this function is called, QCborStreamReader will be indicating
   an error parsing.

   Call addData() to add more data to be parsed.

   \sa reset(), setDevice()
 */
void QCborStreamReader::clear()
{
    setDevice(nullptr);
}

/*!
   Resets the source back to the beginning and clears the decoder state. If the
   source data was a QByteArray, QCborStreamReader will restart from the
   beginning of the array.

   If the source data is a QIODevice, this function will call
   QIODevice::reset(), which will seek to byte position 0. If the CBOR stream
   is not found at the beginning of the device (e.g., beginning of a file),
   then this function will likely do the wrong thing. Instead, position the
   QIODevice to the right offset and call setDevice().

   \sa clear(), setDevice()
 */
void QCborStreamReader::reset()
{
    if (d->device)
        d->device->reset();
    d->lastError = {};
    d->initDecoder();
    preparse();
}

/*!
   Returns the last error in decoding the stream, if any. If no error
   was encountered, this returns an QCborError::NoError.

   \sa isValid()
 */
QCborError QCborStreamReader::lastError()
{
    return d->lastError;
}

/*!
   Returns the offset in the input stream of the item currently being decoded.
   The current offset is the number of decoded bytes so far only if the source
   data is a QByteArray or it is a QIODevice that was positioned at its
   beginning when decoding started.

   \sa reset(), clear(), device()
 */
qint64 QCborStreamReader::currentOffset() const
{
    return (d->device ? d->device->pos() : 0) + d->bufferStart;
}

/*!
   Returns the number of containers that this stream has entered with
   enterContainer() but not yet left.

   \sa enterContainer(), leaveContainer()
 */
int QCborStreamReader::containerDepth() const
{
    return d->containerStack.size();
}

/*!
   Returns either QCborStreamReader::Array or QCborStreamReader::Map,
   indicating whether the container that contains the current item was an array
   or map, respectively. If we're currently parsing the root element, this
   function returns QCborStreamReader::Invalid.

   \sa containerDepth(), enterContainer()
 */
QCborStreamReader::Type QCborStreamReader::parentContainerType() const
{
    if (d->containerStack.isEmpty())
        return Invalid;
    return Type(cbor_value_get_type(&qAsConst(d->containerStack).top()));
}

/*!
   Returns true if there are more items to be decoded in the current container
   or false of we've reached its end. If we're parsing the root element,
   hasNext() returning false indicates the parsing is complete; otherwise, if
   the container depth is non-zero, then the outer code needs to call
   leaveContainer().

   \sa parentContainerType(), containerDepth(), leaveContainer()
 */
bool QCborStreamReader::hasNext() const noexcept
{
    return cbor_value_is_valid(&d->currentElement) &&
            !cbor_value_at_end(&d->currentElement);
}

/*!
   Advance the CBOR stream decoding one element. You should usually call this
   function when parsing fixed-width basic elements (that is, integers, simple
   values, tags and floating point values). But this function can be called
   when the current item is a string, array or map too and it will skip over
   that entire element, including all contained elements.

   This function returns true if advancing was successful, false otherwise. It
   may fail if the stream is corrupt, incomplete or if the nesting level of
   arrays and maps exceeds \a maxRecursion. Calling this function when
   hasNext() has returned false is also an error. If this function returns
   false, lastError() will return the error code detailing what the failure
   was.

   \sa lastError(), isValid(), hasNext()
 */
bool QCborStreamReader::next(int maxRecursion)
{
    if (lastError() != QCborError::NoError)
        return false;

    if (!hasNext()) {
        d->handleError(CborErrorAdvancePastEOF);
    } else if (maxRecursion < 0) {
        d->handleError(CborErrorNestingTooDeep);
    } else if (isContainer()) {
        // iterate over each element
        enterContainer();
        while (lastError() == QCborError::NoError && hasNext())
            next(maxRecursion - 1);
        if (lastError() == QCborError::NoError)
            leaveContainer();
    } else if (isString() || isByteArray()) {
        auto r = _readByteArray_helper();
        while (r.status == Ok) {
            if (isString() && !QUtf8::isValidUtf8(r.data, r.data.size()).isValidUtf8) {
                d->handleError(CborErrorInvalidUtf8TextString);
                break;
            }
            r = _readByteArray_helper();
        }
    } else {
        // fixed types
        CborError err = cbor_value_advance_fixed(&d->currentElement);
        if (err)
            d->handleError(err);
    }

    preparse();
    return d->lastError == QCborError::NoError;
}

/*!
   Returns true if the length of the current array, map, byte array or string
   is known (explicit in the CBOR stream), false otherwise. This function
   should only be called if the element is one of those.

   If the length is known, it may be obtained by calling length().

   If the length of a map or an array is not known, it is implied by the number
   of elements present in the stream. QCborStreamReader has no API to calculate
   the length in that condition.

   Strings and byte arrays may also have indeterminate length (that is, they
   may be transmitted in multiple chunks). Those cannot currently be created
   with QCborStreamWriter, but they could be with other encoders, so
   QCborStreamReader supports them.

   \sa length(), QCborStreamWriter::startArray(), QCborStreamWriter::startMap()
 */
bool QCborStreamReader::isLengthKnown() const noexcept
{
    return cbor_value_is_length_known(&d->currentElement);
}

/*!
   Returns the length of the string or byte array, or the number of items in an
   array or the number, of item pairs in a map, if known. This function must
   not be called if the length is unknown (that is, if isLengthKnown() returned
   false). It is an error to do that and it will cause QCborStreamReader to
   stop parsing the input stream.

   \sa isLengthKnown(), QCborStreamWriter::startArray(), QCborStreamWriter::startMap()
 */
quint64 QCborStreamReader::length() const
{
    CborError err;
    switch (type()) {
    case String:
    case ByteArray:
    case Map:
    case Array:
        if (isLengthKnown())
            return value64;
        err = CborErrorUnknownLength;
        break;

    default:
        err = CborErrorIllegalType;
        break;
    }

    d->handleError(err);
    return quint64(-1);
}

/*!
   \fn bool QCborStreamReader::enterContainer()

   Enters the array or map that is the current item and prepares for iterating
   the elements contained in the container. Returns true if entering the
   container succeeded, false otherwise (usually, a parsing error). Each call
   to enterContainer() must be paired with a call to leaveContainer().

   This function may only be called if the current item is an array or a map
   (that is, if isArray(), isMap() or isContainer() is true). Calling it in any
   other condition is an error.

   \sa leaveContainer(), isContainer(), isArray(), isMap()
 */
bool QCborStreamReader::_enterContainer_helper()
{
    d->containerStack.push(d->currentElement);
    CborError err = cbor_value_enter_container(&d->containerStack.top(), &d->currentElement);
    if (!err) {
        preparse();
        return true;
    }
    d->handleError(err);
    return false;
}

/*!
   Leaves the array or map whose items were being processed and positions the
   decoder at the next item after the end of the container. Returns true if
   leaving the container succeeded, false otherwise (usually, a parsing error).
   Each call to enterContainer() must be paired with a call to
   leaveContainer().

   This function may only be called if hasNext() has returned false and
   containerDepth() is not zero. Calling it in any other condition is an error.

   \sa enterContainer(), parentContainerType(), containerDepth()
 */
bool QCborStreamReader::leaveContainer()
{
    if (d->containerStack.isEmpty()) {
        qWarning("QCborStreamReader::leaveContainer: trying to leave top-level element");
        return false;
    }
    if (d->corrupt)
        return false;

    CborValue container = d->containerStack.pop();
    CborError err = cbor_value_leave_container(&container, &d->currentElement);
    d->currentElement = container;
    if (err) {
        d->handleError(err);
        return false;
    }

    preparse();
    return true;
}

/*!
   \fn bool QCborStreamReader::toBool() const

   Returns the boolean value of the current element.

   This function does not perform any type conversions, including from integer.
   Therefore, it may only be called if isTrue(), isFalse() or isBool() returned
   true; calling it in any other condition is an error.

   \sa isBool(), isTrue(), isFalse(), toInteger()
 */

/*!
   \fn QCborTag QCborStreamReader::toTag() const

   Returns the tag value of the current element.

   This function does not perform any type conversions, including from integer.
   Therefore, it may only be called if isTag() is true; calling it in any other
   condition is an error.

   Tags are 64-bit numbers attached to generic CBOR types that give them
   further meaning. For a list of known tags, see the \l QCborKnownTags
   enumeration.

   \sa isTag(), toInteger(), QCborKnownTags
 */

/*!
   \fn quint64 QCborStreamReader::toUnsignedInteger() const

   Returns the unsigned integer value of the current element.

   This function does not perform any type conversions, including from boolean
   or CBOR tag. Therefore, it may only be called if isUnsignedInteger() is
   true; calling it in any other condition is an error.

   This function may be used to obtain numbers beyond the range of the return
   type of toInteger().

   \sa type(), toInteger(), isUnsignedInteger(), isNegativeInteger()
 */

/*!
   \fn QCborNegativeValue QCborStreamReader::toNegativeInteger() const

   Returns the negative integer value of the current element.
   QCborNegativeValue is a 64-bit unsigned integer containing the absolute
   value of the negative number that was stored in the CBOR stream.
   Additionally, QCborNegativeValue(0) represents the number -2\sup{64}.

   This function does not perform any type conversions, including from boolean
   or CBOR tag. Therefore, it may only be called if isNegativeInteger() is
   true; calling it in any other condition is an error.

   This function may be used to obtain numbers beyond the range of the return
   type of toInteger(). However, use of negative numbers smaller than -2\sup{63}
   is extremely discouraged.

   \sa type(), toInteger(), isNegativeInteger(), isUnsignedInteger()
 */

/*!
   \fn qint64 QCborStreamReader::toInteger() const

   Returns the integer value of the current element, be it negative, positive
   or zero. If the value is larger than 2\sup{63} - 1 or smaller than
   -2\sup{63}, the returned value will overflow and will have an incorrect
   sign. If handling those values is required, use toUnsignedInteger() or
   toNegativeInteger() instead.

   This function does not perform any type conversions, including from boolean
   or CBOR tag. Therefore, it may only be called if isInteger() is true;
   calling it in any other condition is an error.

   \sa isInteger(), toUnsignedInteger(), toNegativeInteger()
 */

/*!
   \fn QCborSimpleType QCborStreamReader::toSimpleType() const

   Returns value of the current simple type.

   This function does not perform any type conversions, including from integer.
   Therefore, it may only be called if isSimpleType() is true; calling it in
   any other condition is an error.

   \sa isSimpleType(), isTrue(), isFalse(), isBool(), isNull(), isUndefined()
 */

/*!
   \fn qfloat16 QCborStreamReader::toFloat16() const

   Returns the 16-bit half-precision floating point value of the current element.

   This function does not perform any type conversions, including from other
   floating point types or from integer values. Therefore, it may only be
   called if isFloat16() is true; calling it in any other condition is an
   error.

   \sa isFloat16(), toFloat(), toDouble()
 */

/*!
   \fn float QCborStreamReader::toFloat() const

   Returns the 32-bit single-precision floating point value of the current
   element.

   This function does not perform any type conversions, including from other
   floating point types or from integer values. Therefore, it may only be
   called if isFloat() is true; calling it in any other condition is an error.

   \sa isFloat(), toFloat16(), toDouble()
 */

/*!
   \fn double QCborStreamReader::toDouble() const

   Returns the 64-bit double-precision floating point value of the current
   element.

   This function does not perform any type conversions, including from other
   floating point types or from integer values. Therefore, it may only be
   called if isDouble() is true; calling it in any other condition is an error.

   \sa isDouble(), toFloat16(), toFloat()
 */

/*!
   \fn QCborStreamReader::StringResult<QString> QCborStreamReader::readString()

   Decodes one string chunk from the CBOR string and returns it. This function
   is used for both regular and chunked string contents, so the caller must
   always loop around calling this function, even if isLengthKnown() has
   is true. The typical use of this function is as follows:

   \code
     QString decodeString(QCborStreamReader &reader)
     {
         QString result;
         auto r = reader.readString();
         while (r.code == QCborStreamReader::Ok) {
             result += r.data;
             r = reader.readString();
         }

         if (r.code == QCborStreamReader::Error) {
             // handle error condition
             result.clear();
         }
         return result;
     }
   \endcode

   This function does not perform any type conversions, including from integers
   or from byte arrays. Therefore, it may only be called if isString() returned
   true; calling it in any other condition is an error.

   \sa readByteArray(), isString(), readStringChunk()
 */
QCborStreamReader::StringResult<QString> QCborStreamReader::_readString_helper()
{
    auto r = _readByteArray_helper();
    QCborStreamReader::StringResult<QString> result;
    result.status = r.status;

    if (r.status == Ok) {
        QTextCodec::ConverterState cs;
        result.data = QUtf8::convertToUnicode(r.data, r.data.size(), &cs);
        if (cs.invalidChars == 0 && cs.remainingChars == 0)
            return result;

        d->handleError(CborErrorInvalidUtf8TextString);
        result.data.clear();
        result.status = Error;
        return result;
    }
    return result;
}

/*!
   \fn QCborStreamReader::StringResult<QString> QCborStreamReader::readByteArray()

   Decodes one byte array chunk from the CBOR string and returns it. This
   function is used for both regular and chunked contents, so the caller must
   always loop around calling this function, even if isLengthKnown() has
   is true. The typical use of this function is as follows:

   \code
     QBytearray decodeBytearray(QCborStreamReader &reader)
     {
         QBytearray result;
         auto r = reader.readBytearray();
         while (r.code == QCborStreamReader::Ok) {
             result += r.data;
             r = reader.readByteArray();
         }

         if (r.code == QCborStreamReader::Error) {
             // handle error condition
             result.clear();
         }
         return result;
     }
   \endcode

   This function does not perform any type conversions, including from integers
   or from strings. Therefore, it may only be called if isByteArray() is true;
   calling it in any other condition is an error.

   \sa readString(), isByteArray(), readStringChunk()
 */
QCborStreamReader::StringResult<QByteArray> QCborStreamReader::_readByteArray_helper()
{
    QCborStreamReader::StringResult<QByteArray> result;
    result.status = Error;
    qsizetype len = _currentStringChunkSize();
    if (len < 0)
        return result;

    result.data.resize(len);
    auto r = readStringChunk(result.data.data(), len);
    Q_ASSERT(r.status != Ok || r.data == len);
    result.status = r.status;
    return result;
}

/*!
    \fn qsizetype QCborStreamReader::currentStringChunkSize() const

    Returns the size of the current text or byte string chunk. If the CBOR
    stream contains a non-chunked string (that is, if isLengthKnown() returns
    \c true), this function returns the size of the entire string, the same as
    length().

    This function is useful to pre-allocate the buffer whose pointer can be passed
    to readStringChunk() later.

    \sa readString(), readByteArray(), readStringChunk()
 */
qsizetype QCborStreamReader::_currentStringChunkSize() const
{
    if (!d->ensureStringIteration())
        return -1;

    size_t len;
    CborError err = cbor_value_get_string_chunk_size(&d->currentElement, &len);
    if (err == CborErrorNoMoreStringChunks)
        return 0;           // not a real error
    else if (err)
        d->handleError(err);
    else if (qsizetype(len) < 0)
        d->handleError(CborErrorDataTooLarge);
    else
        return qsizetype(len);
    return -1;
}

/*!
    Reads the current string chunk into the buffer pointed to by \a ptr, whose
    size is \a maxlen. This function returns a \l StringResult object, with the
    number of bytes copied into \a ptr saved in the \c \l StringResult::data
    member. The \c \l StringResult::status member indicates whether there was
    an error reading the string, whether data was copied or whether this was
    the last chunk.

    This function can be called for both \l String and \l ByteArray types.
    For the latter, this function will read the same data that readByteArray()
    would have returned. For strings, it returns the UTF-8 equivalent of the \l
    QString that would have been returned.

    This function is usually used alongside currentStringChunkSize() in a loop.
    For example:

    \code
        QCborStreamReader<qsizetype> result;
        do {
            qsizetype size = reader.currentStringChunkSize();
            qsizetype oldsize = buffer.size();
            buffer.resize(oldsize + size);
            result = reader.readStringChunk(buffer.data() + oldsize, size);
        } while (result.status() == QCborStreamReader::Ok);
    \endcode

    Unlike readByteArray() and readString(), this function is not limited by
    implementation limits of QByteArray and QString.

    \note This function does not perform verification that the UTF-8 contents
    are properly formatted. That means this function does not produce the
    QCborError::InvalidUtf8String error, even when readString() does.

    \sa currentStringChunkSize(), readString(), readByteArray(),
        isString(), isByteArray()
 */
QCborStreamReader::StringResult<qsizetype>
QCborStreamReader::readStringChunk(char *ptr, qsizetype maxlen)
{
    CborError err;
    size_t len;
    const void *content = nullptr;
    QCborStreamReader::StringResult<qsizetype> result;
    result.data = 0;
    result.status = Error;

    d->lastError = {};
    if (!d->ensureStringIteration())
        return result;

#if 1
    // Using internal TinyCBOR API!
    err = _cbor_value_get_string_chunk(&d->currentElement, &content, &len, &d->currentElement);
#else
    // the above is effectively the same as:
    if (cbor_value_is_byte_string(&currentElement))
        err = cbor_value_get_byte_string_chunk(&d->currentElement, reinterpret_cast<const uint8_t **>(&content),
                                               &len, &d->currentElement);
    else
        err = cbor_value_get_text_string_chunk(&d->currentElement, reinterpret_cast<const char **>(&content),
                                               &len, &d->currentElement);
#endif

    // Range check: using implementation-defined behavior in converting an
    // unsigned value out of range of the destination signed type (same as
    // "len > size_t(std::numeric_limits<qsizetype>::max())", but generates
    // better code with ICC and MSVC).
    if (!err && qsizetype(len) < 0)
        err = CborErrorDataTooLarge;

    if (err) {
        if (err == CborErrorNoMoreStringChunks) {
            d->preread();
            err = cbor_value_finish_string_iteration(&d->currentElement);
            result.status = EndOfString;
        }
        if (err)
            d->handleError(err);
        else
            preparse();
        return result;
    }

    // Read the chunk into the user's buffer.
    qint64 actuallyRead;
    qptrdiff offset = qptrdiff(content);
    qsizetype toRead = qsizetype(len);
    qsizetype left = toRead - maxlen;
    if (left < 0)
        left = 0;               // buffer bigger than string
    else
        toRead = maxlen;        // buffer smaller than string

    if (d->device) {
        // This first skip can't fail because we've already read this many bytes.
        d->device->skip(d->bufferStart + qptrdiff(content));
        actuallyRead = d->device->read(ptr, toRead);

        if (actuallyRead != toRead)  {
            actuallyRead = -1;
        } else if (left) {
            qint64 skipped = d->device->skip(left);
            if (skipped != left)
                actuallyRead = -1;
        }

        if (actuallyRead < 0) {
            d->handleError(CborErrorIO);
            return result;
        }

        d->updateBufferAfterString(offset, len);
    } else {
        actuallyRead = toRead;
        memcpy(ptr, d->buffer.constData() + d->bufferStart + offset, toRead);
        d->bufferStart += QByteArray::size_type(offset + len);
    }

    d->preread();
    result.data = actuallyRead;
    result.status = Ok;
    return result;
}

QT_END_NAMESPACE

#include "moc_qcborcommon.cpp"
#include "moc_qcborstream.cpp"
