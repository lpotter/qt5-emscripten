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

#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qjsonarray.h>
#include <qurl.h>
#include <quuid.h>
#include <qvariant.h>
#include <qstringlist.h>
#include <qdebug.h>

#ifndef QT_BOOTSTRAPPED
#  include <qcborarray.h>
#  include <qcbormap.h>
#endif

#include "qjson_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QJsonValue
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \reentrant
    \since 5.0

    \brief The QJsonValue class encapsulates a value in JSON.

    A value in JSON can be one of 6 basic types:

    JSON is a format to store structured data. It has 6 basic data types:

    \list
    \li bool QJsonValue::Bool
    \li double QJsonValue::Double
    \li string QJsonValue::String
    \li array QJsonValue::Array
    \li object QJsonValue::Object
    \li null QJsonValue::Null
    \endlist

    A value can represent any of the above data types. In addition, QJsonValue has one special
    flag to represent undefined values. This can be queried with isUndefined().

    The type of the value can be queried with type() or accessors like isBool(), isString(), and so on.
    Likewise, the value can be converted to the type stored in it using the toBool(), toString() and so on.

    Values are strictly typed internally and contrary to QVariant will not attempt to do any implicit type
    conversions. This implies that converting to a type that is not stored in the value will return a default
    constructed return value.

    \section1 QJsonValueRef

    QJsonValueRef is a helper class for QJsonArray and QJsonObject.
    When you get an object of type QJsonValueRef, you can
    use it as if it were a reference to a QJsonValue. If you assign to it,
    the assignment will apply to the element in the QJsonArray or QJsonObject
    from which you got the reference.

    The following methods return QJsonValueRef:
    \list
    \li \l {QJsonArray}::operator[](int i)
    \li \l {QJsonObject}::operator[](const QString & key) const
    \endlist

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/

/*!
    Creates a QJsonValue of type \a type.

    The default is to create a Null value.
 */
QJsonValue::QJsonValue(Type type)
    : ui(0), d(0), t(type)
{
}

/*!
    \internal
 */
QJsonValue::QJsonValue(QJsonPrivate::Data *data, QJsonPrivate::Base *base, const QJsonPrivate::Value &v)
    : d(0)
{
    t = (Type)(uint)v.type;
    switch (t) {
    case Undefined:
    case Null:
        dbl = 0;
        break;
    case Bool:
        b = v.toBoolean();
        break;
    case Double:
        dbl = v.toDouble(base);
        break;
    case String: {
        QString s = v.toString(base);
        stringData = s.data_ptr();
        stringData->ref.ref();
        break;
    }
    case Array:
    case Object:
        d = data;
        this->base = v.base(base);
        break;
    }
    if (d)
        d->ref.ref();
}

/*!
    Creates a value of type Bool, with value \a b.
 */
QJsonValue::QJsonValue(bool b)
    : d(0), t(Bool)
{
    this->b = b;
}

/*!
    Creates a value of type Double, with value \a n.
 */
QJsonValue::QJsonValue(double n)
    : d(0), t(Double)
{
    this->dbl = n;
}

/*!
    \overload
    Creates a value of type Double, with value \a n.
 */
QJsonValue::QJsonValue(int n)
    : d(0), t(Double)
{
    this->dbl = n;
}

/*!
    \overload
    Creates a value of type Double, with value \a n.
    NOTE: the integer limits for IEEE 754 double precision data is 2^53 (-9007199254740992 to +9007199254740992).
    If you pass in values outside this range expect a loss of precision to occur.
 */
QJsonValue::QJsonValue(qint64 n)
    : d(0), t(Double)
{
    this->dbl = double(n);
}

/*!
    Creates a value of type String, with value \a s.
 */
QJsonValue::QJsonValue(const QString &s)
    : d(0), t(String)
{
    stringDataFromQStringHelper(s);
}

/*!
    \fn QJsonValue::QJsonValue(const char *s)

    Creates a value of type String with value \a s, assuming
    UTF-8 encoding of the input.

    You can disable this constructor by defining \c
    QT_NO_CAST_FROM_ASCII when you compile your applications.

    \since 5.3
 */

void QJsonValue::stringDataFromQStringHelper(const QString &string)
{
    stringData = *(QStringData **)(&string);
    stringData->ref.ref();
}

/*!
    Creates a value of type String, with value \a s.
 */
QJsonValue::QJsonValue(QLatin1String s)
    : d(0), t(String)
{
    // ### FIXME: Avoid creating the temp QString below
    QString str(s);
    stringDataFromQStringHelper(str);
}

/*!
    Creates a value of type Array, with value \a a.
 */
QJsonValue::QJsonValue(const QJsonArray &a)
    : d(a.d), t(Array)
{
    base = a.a;
    if (d)
        d->ref.ref();
}

/*!
    Creates a value of type Object, with value \a o.
 */
QJsonValue::QJsonValue(const QJsonObject &o)
    : d(o.d), t(Object)
{
    base = o.o;
    if (d)
        d->ref.ref();
}


/*!
    Destroys the value.
 */
QJsonValue::~QJsonValue()
{
    if (t == String && stringData && !stringData->ref.deref())
        free(stringData);

    if (d && !d->ref.deref())
        delete d;
}

/*!
    Creates a copy of \a other.
 */
QJsonValue::QJsonValue(const QJsonValue &other)
{
    t = other.t;
    d = other.d;
    ui = other.ui;
    if (d)
        d->ref.ref();

    if (t == String && stringData)
        stringData->ref.ref();
}

/*!
    Assigns the value stored in \a other to this object.
 */
QJsonValue &QJsonValue::operator =(const QJsonValue &other)
{
    QJsonValue copy(other);
    swap(copy);
    return *this;
}

/*!
    \fn QJsonValue::QJsonValue(QJsonValue &&other)
    \since 5.10

    Move-constructs a QJsonValue from \a other.
*/

/*!
    \fn QJsonValue &QJsonValue::operator =(QJsonValue &&other)
    \since 5.10

    Move-assigns \a other to this value.
*/

/*!
    \fn void QJsonValue::swap(QJsonValue &other)
    \since 5.10

    Swaps the value \a other with this. This operation is very fast and never fails.
*/

/*!
    \fn bool QJsonValue::isNull() const

    Returns \c true if the value is null.
*/

/*!
    \fn bool QJsonValue::isBool() const

    Returns \c true if the value contains a boolean.

    \sa toBool()
 */

/*!
    \fn bool QJsonValue::isDouble() const

    Returns \c true if the value contains a double.

    \sa toDouble()
 */

/*!
    \fn bool QJsonValue::isString() const

    Returns \c true if the value contains a string.

    \sa toString()
 */

/*!
    \fn bool QJsonValue::isArray() const

    Returns \c true if the value contains an array.

    \sa toArray()
 */

/*!
    \fn bool QJsonValue::isObject() const

    Returns \c true if the value contains an object.

    \sa toObject()
 */

/*!
    \fn bool QJsonValue::isUndefined() const

    Returns \c true if the value is undefined. This can happen in certain
    error cases as e.g. accessing a non existing key in a QJsonObject.
 */


/*!
    Converts \a variant to a QJsonValue and returns it.

    The conversion will convert QVariant types as follows:

    \table
    \header
        \li Source type
        \li Destination type
    \row
        \li
            \list
                \li QMetaType::Nullptr
            \endlist
        \li QJsonValue::Null
    \row
        \li
            \list
                \li QMetaType::Bool
            \endlist
        \li QJsonValue::Bool
    \row
        \li
            \list
                \li QMetaType::Int
                \li QMetaType::UInt
                \li QMetaType::LongLong
                \li QMetaType::ULongLong
                \li QMetaType::Float
                \li QMetaType::Double
            \endlist
        \li QJsonValue::Double
    \row
        \li
            \list
                \li QMetaType::QString
            \endlist
        \li QJsonValue::String
    \row
        \li
            \list
                \li QMetaType::QStringList
                \li QMetaType::QVariantList
            \endlist
        \li QJsonValue::Array
    \row
        \li
            \list
                \li QMetaType::QVariantMap
                \li QMetaType::QVariantHash
            \endlist
        \li QJsonValue::Object

    \row
        \li
            \list
                \li QMetaType::QUrl
            \endlist
        \li QJsonValue::String. The conversion will use QUrl::toString() with flag
            QUrl::FullyEncoded, so as to ensure maximum compatibility in parsing the URL
    \row
        \li
            \list
                \li QMetaType::QUuid
            \endlist
        \li QJsonValue::String. Since Qt 5.11, the resulting string will not include braces
    \row
        \li
            \list
                \li QMetaType::QCborValue
            \endlist
        \li Whichever type QCborValue::toJsonValue() returns.
    \row
        \li
            \list
                \li QMetaType::QCborArray
            \endlist
        \li QJsonValue::Array. See QCborValue::toJsonValue() for conversion restrictions.
    \row
        \li
            \list
                \li QMetaType::QCborMap
            \endlist
        \li QJsonValue::Map. See QCborValue::toJsonValue() for conversion restrictions and the
            "stringification" of map keys.
    \endtable

    For all other QVariant types a conversion to a QString will be attempted. If the returned string
    is empty, a Null QJsonValue will be stored, otherwise a String value using the returned QString.

    \sa toVariant()
 */
QJsonValue QJsonValue::fromVariant(const QVariant &variant)
{
    switch (variant.userType()) {
    case QMetaType::Nullptr:
        return QJsonValue(Null);
    case QVariant::Bool:
        return QJsonValue(variant.toBool());
    case QVariant::Int:
    case QMetaType::Float:
    case QVariant::Double:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::UInt:
        return QJsonValue(variant.toDouble());
    case QVariant::String:
        return QJsonValue(variant.toString());
    case QVariant::StringList:
        return QJsonValue(QJsonArray::fromStringList(variant.toStringList()));
    case QVariant::List:
        return QJsonValue(QJsonArray::fromVariantList(variant.toList()));
    case QVariant::Map:
        return QJsonValue(QJsonObject::fromVariantMap(variant.toMap()));
    case QVariant::Hash:
        return QJsonValue(QJsonObject::fromVariantHash(variant.toHash()));
#ifndef QT_BOOTSTRAPPED
    case QVariant::Url:
        return QJsonValue(variant.toUrl().toString(QUrl::FullyEncoded));
    case QVariant::Uuid:
        return variant.toUuid().toString(QUuid::WithoutBraces);
    case QMetaType::QJsonValue:
        return variant.toJsonValue();
    case QMetaType::QJsonObject:
        return variant.toJsonObject();
    case QMetaType::QJsonArray:
        return variant.toJsonArray();
    case QMetaType::QJsonDocument: {
        QJsonDocument doc = variant.toJsonDocument();
        return doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
    }
    case QMetaType::QCborValue:
        return variant.value<QCborValue>().toJsonValue();
    case QMetaType::QCborArray:
        return variant.value<QCborArray>().toJsonArray();
    case QMetaType::QCborMap:
        return variant.value<QCborMap>().toJsonObject();
#endif
    default:
        break;
    }
    QString string = variant.toString();
    if (string.isEmpty())
        return QJsonValue();
    return QJsonValue(string);
}

/*!
    Converts the value to a \l {QVariant::}{QVariant()}.

    The QJsonValue types will be converted as follows:

    \value Null     QMetaType::Nullptr
    \value Bool     QMetaType::Bool
    \value Double   QMetaType::Double
    \value String   QString
    \value Array    QVariantList
    \value Object   QVariantMap
    \value Undefined \l {QVariant::}{QVariant()}

    \sa fromVariant()
 */
QVariant QJsonValue::toVariant() const
{
    switch (t) {
    case Bool:
        return b;
    case Double:
        return dbl;
    case String:
        return toString();
    case Array:
        return d ?
               QJsonArray(d, static_cast<QJsonPrivate::Array *>(base)).toVariantList() :
               QVariantList();
    case Object:
        return d ?
               QJsonObject(d, static_cast<QJsonPrivate::Object *>(base)).toVariantMap() :
               QVariantMap();
    case Null:
        return QVariant::fromValue(nullptr);
    case Undefined:
        break;
    }
    return QVariant();
}

/*!
    \enum QJsonValue::Type

    This enum describes the type of the JSON value.

    \value Null     A Null value
    \value Bool     A boolean value. Use toBool() to convert to a bool.
    \value Double   A double. Use toDouble() to convert to a double.
    \value String   A string. Use toString() to convert to a QString.
    \value Array    An array. Use toArray() to convert to a QJsonArray.
    \value Object   An object. Use toObject() to convert to a QJsonObject.
    \value Undefined The value is undefined. This is usually returned as an
                    error condition, when trying to read an out of bounds value
                    in an array or a non existent key in an object.
*/

/*!
    Returns the type of the value.

    \sa QJsonValue::Type
 */
QJsonValue::Type QJsonValue::type() const
{
    return t;
}

/*!
    Converts the value to a bool and returns it.

    If type() is not bool, the \a defaultValue will be returned.
 */
bool QJsonValue::toBool(bool defaultValue) const
{
    if (t != Bool)
        return defaultValue;
    return b;
}

/*!
    \since 5.2
    Converts the value to an int and returns it.

    If type() is not Double or the value is not a whole number,
    the \a defaultValue will be returned.
 */
int QJsonValue::toInt(int defaultValue) const
{
    if (t == Double && int(dbl) == dbl)
        return int(dbl);
    return defaultValue;
}

/*!
    Converts the value to a double and returns it.

    If type() is not Double, the \a defaultValue will be returned.
 */
double QJsonValue::toDouble(double defaultValue) const
{
    if (t != Double)
        return defaultValue;
    return dbl;
}

/*!
    Converts the value to a QString and returns it.

    If type() is not String, the \a defaultValue will be returned.
 */
QString QJsonValue::toString(const QString &defaultValue) const
{
    if (t != String)
        return defaultValue;
    stringData->ref.ref(); // the constructor below doesn't add a ref.
    QStringDataPtr holder = { stringData };
    return QString(holder);
}

/*!
    Converts the value to a QString and returns it.

    If type() is not String, a null QString will be returned.

    \sa QString::isNull()
 */
QString QJsonValue::toString() const
{
    if (t != String)
        return QString();
    stringData->ref.ref(); // the constructor below doesn't add a ref.
    QStringDataPtr holder = { stringData };
    return QString(holder);
}

/*!
    Converts the value to an array and returns it.

    If type() is not Array, the \a defaultValue will be returned.
 */
QJsonArray QJsonValue::toArray(const QJsonArray &defaultValue) const
{
    if (!d || t != Array)
        return defaultValue;

    return QJsonArray(d, static_cast<QJsonPrivate::Array *>(base));
}

/*!
    \overload

    Converts the value to an array and returns it.

    If type() is not Array, a \l{QJsonArray::}{QJsonArray()} will be returned.
 */
QJsonArray QJsonValue::toArray() const
{
    return toArray(QJsonArray());
}

/*!
    Converts the value to an object and returns it.

    If type() is not Object, the \a defaultValue will be returned.
 */
QJsonObject QJsonValue::toObject(const QJsonObject &defaultValue) const
{
    if (!d || t != Object)
        return defaultValue;

    return QJsonObject(d, static_cast<QJsonPrivate::Object *>(base));
}

/*!
    \overload

    Converts the value to an object and returns it.

    If type() is not Object, the \l {QJsonObject::}{QJsonObject()} will be returned.
*/
QJsonObject QJsonValue::toObject() const
{
    return toObject(QJsonObject());
}

/*!
    Returns a QJsonValue representing the value for the key \a key.

    Equivalent to calling toObject().value(key).

    The returned QJsonValue is QJsonValue::Undefined if the key does not exist,
    or if isObject() is false.

    \since 5.10

    \sa QJsonValue, QJsonValue::isUndefined(), QJsonObject
 */
const QJsonValue QJsonValue::operator[](const QString &key) const
{
    if (!isObject())
        return QJsonValue(QJsonValue::Undefined);

    return toObject().value(key);
}

/*!
    \overload
    \since 5.10
*/
const QJsonValue QJsonValue::operator[](QLatin1String key) const
{
    if (!isObject())
        return QJsonValue(QJsonValue::Undefined);

    return toObject().value(key);
}

/*!
    Returns a QJsonValue representing the value for index \a i.

    Equivalent to calling toArray().at(i).

    The returned QJsonValue is QJsonValue::Undefined, if \a i is out of bounds,
    or if isArray() is false.

    \since 5.10

    \sa QJsonValue, QJsonValue::isUndefined(), QJsonArray
 */
const QJsonValue QJsonValue::operator[](int i) const
{
    if (!isArray())
        return QJsonValue(QJsonValue::Undefined);

    return toArray().at(i);
}

/*!
    Returns \c true if the value is equal to \a other.
 */
bool QJsonValue::operator==(const QJsonValue &other) const
{
    if (t != other.t)
        return false;

    switch (t) {
    case Undefined:
    case Null:
        break;
    case Bool:
        return b == other.b;
    case Double:
        return dbl == other.dbl;
    case String:
        return toString() == other.toString();
    case Array:
        if (base == other.base)
            return true;
        if (!base)
            return !other.base->length;
        if (!other.base)
            return !base->length;
        return QJsonArray(d, static_cast<QJsonPrivate::Array *>(base))
                == QJsonArray(other.d, static_cast<QJsonPrivate::Array *>(other.base));
    case Object:
        if (base == other.base)
            return true;
        if (!base)
            return !other.base->length;
        if (!other.base)
            return !base->length;
        return QJsonObject(d, static_cast<QJsonPrivate::Object *>(base))
                == QJsonObject(other.d, static_cast<QJsonPrivate::Object *>(other.base));
    }
    return true;
}

/*!
    Returns \c true if the value is not equal to \a other.
 */
bool QJsonValue::operator!=(const QJsonValue &other) const
{
    return !(*this == other);
}

/*!
    \internal
 */
void QJsonValue::detach()
{
    if (!d)
        return;

    QJsonPrivate::Data *x = d->clone(base);
    x->ref.ref();
    if (!d->ref.deref())
        delete d;
    d = x;
    base = static_cast<QJsonPrivate::Object *>(d->header->root());
}


/*!
    \class QJsonValueRef
    \inmodule QtCore
    \reentrant
    \brief The QJsonValueRef class is a helper class for QJsonValue.

    \internal

    \ingroup json

    When you get an object of type QJsonValueRef, if you can assign to it,
    the assignment will apply to the character in the string from
    which you got the reference. That is its whole purpose in life.

    You can use it exactly in the same way as a reference to a QJsonValue.

    The QJsonValueRef becomes invalid once modifications are made to the
    string: if you want to keep the character, copy it into a QJsonValue.

    Most of the QJsonValue member functions also exist in QJsonValueRef.
    However, they are not explicitly documented here.
*/


QJsonValueRef &QJsonValueRef::operator =(const QJsonValue &val)
{
    if (is_object)
        o->setValueAt(index, val);
    else
        a->replace(index, val);

    return *this;
}

QJsonValueRef &QJsonValueRef::operator =(const QJsonValueRef &ref)
{
    if (is_object)
        o->setValueAt(index, ref);
    else
        a->replace(index, ref);

    return *this;
}

QVariant QJsonValueRef::toVariant() const
{
    return toValue().toVariant();
}

QJsonArray QJsonValueRef::toArray() const
{
    return toValue().toArray();
}

QJsonObject QJsonValueRef::toObject() const
{
    return toValue().toObject();
}

QJsonValue QJsonValueRef::toValue() const
{
    if (!is_object)
        return a->at(index);
    return o->valueAt(index);
}

uint qHash(const QJsonValue &value, uint seed)
{
    switch (value.type()) {
    case QJsonValue::Null:
        return qHash(nullptr, seed);
    case QJsonValue::Bool:
        return qHash(value.toBool(), seed);
    case QJsonValue::Double:
        return qHash(value.toDouble(), seed);
    case QJsonValue::String:
        return qHash(value.toString(), seed);
    case QJsonValue::Array:
        return qHash(value.toArray(), seed);
    case QJsonValue::Object:
        return qHash(value.toObject(), seed);
    case QJsonValue::Undefined:
        return seed;
    }
    Q_UNREACHABLE();
    return 0;
}

#if !defined(QT_NO_DEBUG_STREAM) && !defined(QT_JSON_READONLY)
QDebug operator<<(QDebug dbg, const QJsonValue &o)
{
    QDebugStateSaver saver(dbg);
    switch (o.t) {
    case QJsonValue::Undefined:
        dbg << "QJsonValue(undefined)";
        break;
    case QJsonValue::Null:
        dbg << "QJsonValue(null)";
        break;
    case QJsonValue::Bool:
        dbg.nospace() << "QJsonValue(bool, " << o.toBool() << ')';
        break;
    case QJsonValue::Double:
        dbg.nospace() << "QJsonValue(double, " << o.toDouble() << ')';
        break;
    case QJsonValue::String:
        dbg.nospace() << "QJsonValue(string, " << o.toString() << ')';
        break;
    case QJsonValue::Array:
        dbg.nospace() << "QJsonValue(array, ";
        dbg << o.toArray();
        dbg << ')';
        break;
    case QJsonValue::Object:
        dbg.nospace() << "QJsonValue(object, ";
        dbg << o.toObject();
        dbg << ')';
        break;
    }
    return dbg;
}
#endif

QT_END_NAMESPACE
