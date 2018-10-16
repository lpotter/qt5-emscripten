/****************************************************************************
 **
 ** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#include "qppsattribute_p.h"
#include "qppsattributeprivate_p.h"

#include <QDebug>
#include <QVariant>

QT_BEGIN_NAMESPACE

///////////////////////////
//
// QPpsAttributePrivate
//
///////////////////////////

QPpsAttributePrivate::QPpsAttributePrivate() : type(QPpsAttribute::None)
{
}

QPpsAttribute QPpsAttributePrivate::createPpsAttribute(int value, QPpsAttribute::Flags flags)
{
    QPpsAttribute attribute;
    attribute.d->type = QPpsAttribute::Number;
    attribute.d->data = value;
    attribute.d->flags = flags;
    return attribute;
}

QPpsAttribute QPpsAttributePrivate::createPpsAttribute(long long value, QPpsAttribute::Flags flags)
{
    QPpsAttribute attribute;
    attribute.d->type = QPpsAttribute::Number;
    attribute.d->data = value;
    attribute.d->flags = flags;
    return attribute;
}

QPpsAttribute QPpsAttributePrivate::createPpsAttribute(double value, QPpsAttribute::Flags flags)
{
    QPpsAttribute attribute;
    attribute.d->type = QPpsAttribute::Number;
    attribute.d->data = value;
    attribute.d->flags = flags;
    return attribute;
}

QPpsAttribute QPpsAttributePrivate::createPpsAttribute(bool value, QPpsAttribute::Flags flags)
{
    QPpsAttribute attribute;
    attribute.d->type = QPpsAttribute::Bool;
    attribute.d->data = value;
    attribute.d->flags = flags;
    return attribute;
}

QPpsAttribute QPpsAttributePrivate::createPpsAttribute(const QString &value,
                                                       QPpsAttribute::Flags flags)
{
    QPpsAttribute attribute;
    attribute.d->type = QPpsAttribute::String;
    attribute.d->data = value;
    attribute.d->flags = flags;
    return attribute;
}

QPpsAttribute QPpsAttributePrivate::createPpsAttribute(const QPpsAttributeList &value,
                                                       QPpsAttribute::Flags flags)
{
    QPpsAttribute attribute;
    attribute.d->type = QPpsAttribute::Array;
    attribute.d->data = QVariant::fromValue(value);
    attribute.d->flags = flags;
    return attribute;
}

QPpsAttribute QPpsAttributePrivate::createPpsAttribute(const QPpsAttributeMap &value,
                                                       QPpsAttribute::Flags flags)
{
    QPpsAttribute attribute;
    attribute.d->type = QPpsAttribute::Object;
    attribute.d->data = QVariant::fromValue(value);
    attribute.d->flags = flags;
    return attribute;
}

///////////////////////////
//
// QPpsAttribute
//
///////////////////////////

QPpsAttribute::QPpsAttribute() : d(new QPpsAttributePrivate())
{
}

QPpsAttribute::~QPpsAttribute()
{
}

QPpsAttribute::QPpsAttribute(const QPpsAttribute &other) : d(other.d)
{
}

QPpsAttribute &QPpsAttribute::operator=(const QPpsAttribute &other)
{
    d = other.d;
    return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
QPpsAttribute::QPpsAttribute(QPpsAttribute &&other) : d(other.d)
{
    other.d->type = QPpsAttribute::None;
}

QPpsAttribute &QPpsAttribute::operator=(QPpsAttribute &&other)
{
    qSwap(d, other.d);
    return *this;
}
#endif

bool QPpsAttribute::operator==(const QPpsAttribute &other) const
{
    if (type() != other.type())
        return false;
    if (flags() != other.flags())
        return false;

    switch (type()) {
    case QPpsAttribute::Number:
    case QPpsAttribute::Bool:
    case QPpsAttribute::String:
        // QVariant can compare double, int, longlong, bool, and QString for us.
        return d->data == other.d->data;
    case QPpsAttribute::Array:
        // QVariant can't compare custom types (like QPpsAttributeList), always returning false.
        // So we pull the lists out manually and compare them.
        return toList() == other.toList();
    case QPpsAttribute::Object:
        // QVariant can't compare custom types (like QPpsAttributeMap), always returning false.
        // So we pull the maps out manually and compare them.
        return toMap() == other.toMap();
    case QPpsAttribute::None:
        // Both are "None" type, so the actual content doesn't matter.
        return true;
    }
    return d->data == other.d->data;
}

bool QPpsAttribute::isValid() const
{
    return d->type != QPpsAttribute::None;
}

QPpsAttribute::Type QPpsAttribute::type() const
{
    return d->type;
}

bool QPpsAttribute::isNumber() const
{
    return type() == QPpsAttribute::Number;
}

bool QPpsAttribute::isBool() const
{
    return type() == QPpsAttribute::Bool;
}

bool QPpsAttribute::isString() const
{
    return type() == QPpsAttribute::String;
}

bool QPpsAttribute::isArray() const
{
    return type() == QPpsAttribute::Array;
}

bool QPpsAttribute::isObject() const
{
    return type() == QPpsAttribute::Object;
}

double QPpsAttribute::toDouble() const
{
    return d->data.toDouble();
}

qlonglong QPpsAttribute::toLongLong() const
{
    return d->data.toLongLong();
}

int QPpsAttribute::toInt() const
{
    return d->data.toInt();
}

bool QPpsAttribute::toBool() const
{
    return d->data.toBool();
}

QString QPpsAttribute::toString() const
{
    return d->data.toString();
}

QPpsAttributeList QPpsAttribute::toList() const
{
    return d->data.value< QPpsAttributeList >();
}

QPpsAttributeMap QPpsAttribute::toMap() const
{
    return d->data.value< QPpsAttributeMap >();
}

QPpsAttribute::Flags QPpsAttribute::flags() const
{
    return d->flags;
}

QVariant QPpsAttribute::toVariant() const
{
    return d->data;
}

QDebug operator<<(QDebug dbg, const QPpsAttribute &attribute)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QPpsAttribute(";

    switch (attribute.type()) {
    case QPpsAttribute::Number:
        switch (attribute.toVariant().type()) {
        case QVariant::Int:
            dbg << "Number, " << attribute.flags() << ", " << attribute.toInt();
            break;
        case QVariant::LongLong:
            dbg << "Number, " << attribute.flags() << ", " << attribute.toLongLong();
            break;
        default:
            dbg << "Number, " << attribute.flags() << ", " << attribute.toDouble();
            break;
        }
        break;
    case QPpsAttribute::Bool:
        dbg << "Bool, " << attribute.flags() << ", " << attribute.toBool();
        break;
    case QPpsAttribute::String:
        dbg << "String, " << attribute.flags() << ", " << attribute.toString();
        break;
    case QPpsAttribute::Array:
        dbg << "Array, " << attribute.flags() << ", " << attribute.toList();
        break;
    case QPpsAttribute::Object:
        dbg << "Object, " << attribute.flags() << ", " << attribute.toMap();
        break;
    case QPpsAttribute::None:
        dbg << "None";
        break;
    }

    dbg << ')';

    return dbg;
}

QT_END_NAMESPACE
