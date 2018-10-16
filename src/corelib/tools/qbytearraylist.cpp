/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2014 by Southwest Research Institute (R)
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

#include <qbytearraylist.h>

QT_BEGIN_NAMESPACE

/*! \typedef QByteArrayListIterator
    \relates QByteArrayList

    The QByteArrayListIterator type definition provides a Java-style const
    iterator for QByteArrayList.

    QByteArrayList provides both \l{Java-style iterators} and
    \l{STL-style iterators}. The Java-style const iterator is simply
    a type definition for QListIterator<QByteArray>.

    \sa QMutableByteArrayListIterator, QByteArrayList::const_iterator
*/

/*! \typedef QMutableByteArrayListIterator
    \relates QByteArrayList

    The QByteArrayListIterator type definition provides a Java-style
    non-const iterator for QByteArrayList.

    QByteArrayList provides both \l{Java-style iterators} and
    \l{STL-style iterators}. The Java-style non-const iterator is
    simply a type definition for QMutableListIterator<QByteArray>.

    \sa QByteArrayListIterator, QByteArrayList::iterator
*/

/*!
    \class QByteArrayList
    \inmodule QtCore
    \since 5.4
    \brief The QByteArrayList class provides a list of byte arrays.

    \ingroup tools
    \ingroup shared
    \ingroup string-processing

    \reentrant

    QByteArrayList is actually just a QList<QByteArray>. It is documented as a
    full class just for simplicity of documenting the member methods that exist
    only in QList<QByteArray>.

    All of QList's functionality also applies to QByteArrayList. For example, you
    can use isEmpty() to test whether the list is empty, and you can call
    functions like append(), prepend(), insert(), replace(), removeAll(),
    removeAt(), removeFirst(), removeLast(), and removeOne() to modify a
    QByteArrayList. In addition, QByteArrayList provides several join()
    methods for concatenating the list into a single QByteArray.

    The purpose of QByteArrayList is quite different from that of QStringList.
    Whereas QStringList has many methods for manipulation of elements within
    the list, QByteArrayList does not.
    Normally, QStringList should be used whenever working with a list of printable
    strings. QByteArrayList should be used to handle and efficiently join large blobs
    of binary data, as when sequentially receiving serialized data through a
    QIODevice.

    \sa QByteArray, QStringList
*/

/*!
    \fn QByteArray QByteArrayList::join() const

    Joins all the byte arrays into a single byte array.
*/

/*!
    \fn QByteArray QByteArrayList::join(const QByteArray &separator) const

    Joins all the byte arrays into a single byte array with each
    element separated by the given \a separator.
*/

/*!
    \fn QByteArray QByteArrayList::join(char separator) const

    Joins all the byte arrays into a single byte array with each
    element separated by the given \a separator.
*/

static int QByteArrayList_joinedSize(const QByteArrayList *that, int seplen)
{
    int totalLength = 0;
    const int size = that->size();

    for (int i = 0; i < size; ++i)
        totalLength += that->at(i).size();

    if (size > 0)
        totalLength += seplen * (size - 1);

    return totalLength;
}

QByteArray QtPrivate::QByteArrayList_join(const QByteArrayList *that, const char *sep, int seplen)
{
    QByteArray res;
    if (const int joinedSize = QByteArrayList_joinedSize(that, seplen))
        res.reserve(joinedSize); // don't call reserve(0) - it allocates one byte for the NUL
    const int size = that->size();
    for (int i = 0; i < size; ++i) {
        if (i)
            res.append(sep, seplen);
        res += that->at(i);
    }
    return res;
}

QT_END_NAMESPACE
