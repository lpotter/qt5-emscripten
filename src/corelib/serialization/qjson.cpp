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

#include "qjson_p.h"
#include <qalgorithms.h>

QT_BEGIN_NAMESPACE

namespace QJsonPrivate
{

static Q_CONSTEXPR Base emptyArray  = { { qle_uint(sizeof(Base)) }, { 0 }, { qle_uint(0) } };
static Q_CONSTEXPR Base emptyObject = { { qle_uint(sizeof(Base)) }, { qToLittleEndian(1u) }, { qle_uint(0) } };

void Data::compact()
{
    Q_ASSERT(sizeof(Value) == sizeof(offset));

    if (!compactionCounter)
        return;

    Base *base = header->root();
    int reserve = 0;
    if (base->is_object) {
        Object *o = static_cast<Object *>(base);
        for (int i = 0; i < (int)o->length; ++i)
            reserve += o->entryAt(i)->usedStorage(o);
    } else {
        Array *a = static_cast<Array *>(base);
        for (int i = 0; i < (int)a->length; ++i)
            reserve += (*a)[i].usedStorage(a);
    }

    int size = sizeof(Base) + reserve + base->length*sizeof(offset);
    int alloc = sizeof(Header) + size;
    Header *h = (Header *) malloc(alloc);
    h->tag = QJsonDocument::BinaryFormatTag;
    h->version = 1;
    Base *b = h->root();
    b->size = size;
    b->is_object = header->root()->is_object;
    b->length = base->length;
    b->tableOffset = reserve + sizeof(Array);

    int offset = sizeof(Base);
    if (b->is_object) {
        Object *o = static_cast<Object *>(base);
        Object *no = static_cast<Object *>(b);

        for (int i = 0; i < (int)o->length; ++i) {
            no->table()[i] = offset;

            const Entry *e = o->entryAt(i);
            Entry *ne = no->entryAt(i);
            int s = e->size();
            memcpy(ne, e, s);
            offset += s;
            int dataSize = e->value.usedStorage(o);
            if (dataSize) {
                memcpy((char *)no + offset, e->value.data(o), dataSize);
                ne->value.value = offset;
                offset += dataSize;
            }
        }
    } else {
        Array *a = static_cast<Array *>(base);
        Array *na = static_cast<Array *>(b);

        for (int i = 0; i < (int)a->length; ++i) {
            const Value &v = (*a)[i];
            Value &nv = (*na)[i];
            nv = v;
            int dataSize = v.usedStorage(a);
            if (dataSize) {
                memcpy((char *)na + offset, v.data(a), dataSize);
                nv.value = offset;
                offset += dataSize;
            }
        }
    }
    Q_ASSERT(offset == (int)b->tableOffset);

    free(header);
    header = h;
    this->alloc = alloc;
    compactionCounter = 0;
}

bool Data::valid() const
{
    if (header->tag != QJsonDocument::BinaryFormatTag || header->version != 1u)
        return false;

    bool res = false;
    Base *root = header->root();
    int maxSize = alloc - sizeof(Header);
    if (root->is_object)
        res = static_cast<Object *>(root)->isValid(maxSize);
    else
        res = static_cast<Array *>(root)->isValid(maxSize);

    return res;
}


int Base::reserveSpace(uint dataSize, int posInTable, uint numItems, bool replace)
{
    Q_ASSERT(posInTable >= 0 && posInTable <= (int)length);
    if (size + dataSize >= Value::MaxSize) {
        qWarning("QJson: Document too large to store in data structure %d %d %d", (uint)size, dataSize, Value::MaxSize);
        return 0;
    }

    offset off = tableOffset;
    // move table to new position
    if (replace) {
        memmove((char *)(table()) + dataSize, table(), length*sizeof(offset));
    } else {
        memmove((char *)(table() + posInTable + numItems) + dataSize, table() + posInTable, (length - posInTable)*sizeof(offset));
        memmove((char *)(table()) + dataSize, table(), posInTable*sizeof(offset));
    }
    tableOffset += dataSize;
    for (int i = 0; i < (int)numItems; ++i)
        table()[posInTable + i] = off;
    size += dataSize;
    if (!replace) {
        length += numItems;
        size += numItems * sizeof(offset);
    }
    return off;
}

void Base::removeItems(int pos, int numItems)
{
    Q_ASSERT(pos >= 0 && pos <= (int)length);
    if (pos + numItems < (int)length)
        memmove(table() + pos, table() + pos + numItems, (length - pos - numItems)*sizeof(offset));
    length -= numItems;
}

int Object::indexOf(const QString &key, bool *exists) const
{
    int min = 0;
    int n = length;
    while (n > 0) {
        int half = n >> 1;
        int middle = min + half;
        if (*entryAt(middle) >= key) {
            n = half;
        } else {
            min = middle + 1;
            n -= half + 1;
        }
    }
    if (min < (int)length && *entryAt(min) == key) {
        *exists = true;
        return min;
    }
    *exists = false;
    return min;
}

int Object::indexOf(QLatin1String key, bool *exists) const
{
    int min = 0;
    int n = length;
    while (n > 0) {
        int half = n >> 1;
        int middle = min + half;
        if (*entryAt(middle) >= key) {
            n = half;
        } else {
            min = middle + 1;
            n -= half + 1;
        }
    }
    if (min < (int)length && *entryAt(min) == key) {
        *exists = true;
        return min;
    }
    *exists = false;
    return min;
}

bool Object::isValid(int maxSize) const
{
    if (size > (uint)maxSize || tableOffset + length*sizeof(offset) > size)
        return false;

    QString lastKey;
    for (uint i = 0; i < length; ++i) {
        offset entryOffset = table()[i];
        if (entryOffset + sizeof(Entry) >= tableOffset)
            return false;
        Entry *e = entryAt(i);
        if (!e->isValid(tableOffset - table()[i]))
            return false;
        QString key = e->key();
        if (key < lastKey)
            return false;
        if (!e->value.isValid(this))
            return false;
        lastKey = key;
    }
    return true;
}



bool Array::isValid(int maxSize) const
{
    if (size > (uint)maxSize || tableOffset + length*sizeof(offset) > size)
        return false;

    for (uint i = 0; i < length; ++i) {
        if (!at(i).isValid(this))
            return false;
    }
    return true;
}


bool Entry::operator ==(const QString &key) const
{
    if (value.latinKey)
        return (shallowLatin1Key() == key);
    else
        return (shallowKey() == key);
}

bool Entry::operator==(QLatin1String key) const
{
    if (value.latinKey)
        return shallowLatin1Key() == key;
    else
        return shallowKey() == key;
}

bool Entry::operator ==(const Entry &other) const
{
    if (value.latinKey) {
        if (other.value.latinKey)
            return shallowLatin1Key() == other.shallowLatin1Key();
        return shallowLatin1Key() == other.shallowKey();
    } else if (other.value.latinKey) {
        return shallowKey() == other.shallowLatin1Key();
    }
    return shallowKey() == other.shallowKey();
}

bool Entry::operator >=(const Entry &other) const
{
    if (value.latinKey) {
        if (other.value.latinKey)
            return shallowLatin1Key() >= other.shallowLatin1Key();
        return shallowLatin1Key() >= other.shallowKey();
    } else if (other.value.latinKey) {
        return shallowKey() >= other.shallowLatin1Key();
    }
    return shallowKey() >= other.shallowKey();
}


int Value::usedStorage(const Base *b) const
{
    int s = 0;
    switch (type) {
    case QJsonValue::Double:
        if (latinOrIntValue)
            break;
        s = sizeof(double);
        break;
    case QJsonValue::String: {
        char *d = data(b);
        if (latinOrIntValue)
            s = sizeof(ushort) + qFromLittleEndian(*(ushort *)d);
        else
            s = sizeof(int) + sizeof(ushort) * qFromLittleEndian(*(int *)d);
        break;
    }
    case QJsonValue::Array:
    case QJsonValue::Object:
        s = base(b)->size;
        break;
    case QJsonValue::Null:
    case QJsonValue::Bool:
    default:
        break;
    }
    return alignedSize(s);
}

inline bool isValidValueOffset(uint offset, uint tableOffset)
{
    return offset >= sizeof(Base)
        && offset + sizeof(uint) <= tableOffset;
}

bool Value::isValid(const Base *b) const
{
    switch (type) {
    case QJsonValue::Null:
    case QJsonValue::Bool:
        return true;
    case QJsonValue::Double:
        return latinOrIntValue || isValidValueOffset(value, b->tableOffset);
    case QJsonValue::String:
        if (!isValidValueOffset(value, b->tableOffset))
            return false;
        if (latinOrIntValue)
            return asLatin1String(b).isValid(b->tableOffset - value);
        return asString(b).isValid(b->tableOffset - value);
    case QJsonValue::Array:
        return isValidValueOffset(value, b->tableOffset)
            && static_cast<Array *>(base(b))->isValid(b->tableOffset - value);
    case QJsonValue::Object:
        return isValidValueOffset(value, b->tableOffset)
            && static_cast<Object *>(base(b))->isValid(b->tableOffset - value);
    default:
        return false;
    }
}

/*!
    \internal
 */
int Value::requiredStorage(QJsonValue &v, bool *compressed)
{
    *compressed = false;
    switch (v.t) {
    case QJsonValue::Double:
        if (QJsonPrivate::compressedNumber(v.dbl) != INT_MAX) {
            *compressed = true;
            return 0;
        }
        return sizeof(double);
    case QJsonValue::String: {
        QString s = v.toString();
        *compressed = QJsonPrivate::useCompressed(s);
        return QJsonPrivate::qStringSize(s, *compressed);
    }
    case QJsonValue::Array:
    case QJsonValue::Object:
        if (v.d && v.d->compactionCounter) {
            v.detach();
            v.d->compact();
            v.base = static_cast<QJsonPrivate::Base *>(v.d->header->root());
        }
        return v.base ? uint(v.base->size) : sizeof(QJsonPrivate::Base);
    case QJsonValue::Undefined:
    case QJsonValue::Null:
    case QJsonValue::Bool:
        break;
    }
    return 0;
}

/*!
    \internal
 */
uint Value::valueToStore(const QJsonValue &v, uint offset)
{
    switch (v.t) {
    case QJsonValue::Undefined:
    case QJsonValue::Null:
        break;
    case QJsonValue::Bool:
        return v.b;
    case QJsonValue::Double: {
        int c = QJsonPrivate::compressedNumber(v.dbl);
        if (c != INT_MAX)
            return c;
    }
        Q_FALLTHROUGH();
    case QJsonValue::String:
    case QJsonValue::Array:
    case QJsonValue::Object:
        return offset;
    }
    return 0;
}

/*!
    \internal
 */
void Value::copyData(const QJsonValue &v, char *dest, bool compressed)
{
    switch (v.t) {
    case QJsonValue::Double:
        if (!compressed) {
            qToLittleEndian(v.ui, dest);
        }
        break;
    case QJsonValue::String: {
        QString str = v.toString();
        QJsonPrivate::copyString(dest, str, compressed);
        break;
    }
    case QJsonValue::Array:
    case QJsonValue::Object: {
        const QJsonPrivate::Base *b = v.base;
        if (!b)
            b = (v.t == QJsonValue::Array ? &emptyArray : &emptyObject);
        memcpy(dest, b, b->size);
        break;
    }
    default:
        break;
    }
}

} // namespace QJsonPrivate

QT_END_NAMESPACE
