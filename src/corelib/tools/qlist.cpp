/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <new>
#include "qlist.h"
#include "qtools_p.h"

#include <string.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

/*
    QList as an array-list combines the easy-of-use of a random
    access interface with fast list operations and the low memory
    management overhead of an array. Accessing elements by index,
    appending, prepending, and removing elements from both the front
    and the back all happen in constant time O(1). Inserting or
    removing elements at random index positions \ai happens in linear
    time, or more precisly in O(min{i,n-i}) <= O(n/2), with n being
    the number of elements in the list.
*/

const QListData::Data QListData::shared_null = { Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, 0, { 0 } };

/*!
 *  Detaches the QListData by allocating new memory for a list which will be bigger
 *  than the copied one and is expected to grow further.
 *  *idx is the desired insertion point and is clamped to the actual size of the list.
 *  num is the number of new elements to insert at the insertion point.
 *  Returns the old (shared) data, it is up to the caller to deref() and free().
 *  For the new data node_copy needs to be called.
 *
 *  \internal
 */
QListData::Data *QListData::detach_grow(int *idx, int num)
{
    Data *x = d;
    int l = x->end - x->begin;
    int nl = l + num;
    auto blockInfo = qCalculateGrowingBlockSize(nl, sizeof(void *), DataHeaderSize);
    Data* t = static_cast<Data *>(::malloc(blockInfo.size));
    Q_CHECK_PTR(t);
    t->alloc = int(uint(blockInfo.elementCount));

    t->ref.initializeOwned();
    // The space reservation algorithm's optimization is biased towards appending:
    // Something which looks like an append will put the data at the beginning,
    // while something which looks like a prepend will put it in the middle
    // instead of at the end. That's based on the assumption that prepending
    // is uncommon and even an initial prepend will eventually be followed by
    // at least some appends.
    int bg;
    if (*idx < 0) {
        *idx = 0;
        bg = (t->alloc - nl) >> 1;
    } else if (*idx > l) {
        *idx = l;
        bg = 0;
    } else if (*idx < (l >> 1)) {
        bg = (t->alloc - nl) >> 1;
    } else {
        bg = 0;
    }
    t->begin = bg;
    t->end = bg + nl;
    d = t;

    return x;
}

/*!
 *  Detaches the QListData by allocating new memory for a list which possibly
 *  has a different size than the copied one.
 *  Returns the old (shared) data, it is up to the caller to deref() and free()
 *  For the new data node_copy needs to be called.
 *
 *  \internal
 */
QListData::Data *QListData::detach(int alloc)
{
    Data *x = d;
    Data* t = static_cast<Data *>(::malloc(qCalculateBlockSize(alloc, sizeof(void*), DataHeaderSize)));
    Q_CHECK_PTR(t);

    t->ref.initializeOwned();
    t->alloc = alloc;
    if (!alloc) {
        t->begin = 0;
        t->end = 0;
    } else {
        t->begin = x->begin;
        t->end   = x->end;
    }
    d = t;

    return x;
}

void QListData::realloc(int alloc)
{
    Q_ASSERT(!d->ref.isShared());
    Data *x = static_cast<Data *>(::realloc(d, qCalculateBlockSize(alloc, sizeof(void *), DataHeaderSize)));
    Q_CHECK_PTR(x);

    d = x;
    d->alloc = alloc;
    if (!alloc)
        d->begin = d->end = 0;
}

void QListData::realloc_grow(int growth)
{
    Q_ASSERT(!d->ref.isShared());
    auto r = qCalculateGrowingBlockSize(d->alloc + growth, sizeof(void *), DataHeaderSize);
    Data *x = static_cast<Data *>(::realloc(d, r.size));
    Q_CHECK_PTR(x);

    d = x;
    d->alloc = int(uint(r.elementCount));
}

void QListData::dispose(Data *d)
{
    Q_ASSERT(!d->ref.isShared());
    free(d);
}

// ensures that enough space is available to append n elements
void **QListData::append(int n)
{
    Q_ASSERT(!d->ref.isShared());
    int e = d->end;
    if (e + n > d->alloc) {
        int b = d->begin;
        if (b - n >= 2 * d->alloc / 3) {
            // we have enough space. Just not at the end -> move it.
            e -= b;
            ::memcpy(d->array, d->array + b, e * sizeof(void *));
            d->begin = 0;
        } else {
            realloc_grow(n);
        }
    }
    d->end = e + n;
    return d->array + e;
}

// ensures that enough space is available to append one element
void **QListData::append()
{
    return append(1);
}

// ensures that enough space is available to append the list
void **QListData::append(const QListData& l)
{
    return append(l.d->end - l.d->begin);
}

void **QListData::prepend()
{
    Q_ASSERT(!d->ref.isShared());
    if (d->begin == 0) {
        if (d->end >= d->alloc / 3)
            realloc_grow(1);

        if (d->end < d->alloc / 3)
            d->begin = d->alloc - 2 * d->end;
        else
            d->begin = d->alloc - d->end;

        ::memmove(d->array + d->begin, d->array, d->end * sizeof(void *));
        d->end += d->begin;
    }
    return d->array + --d->begin;
}

void **QListData::insert(int i)
{
    Q_ASSERT(!d->ref.isShared());
    if (i <= 0)
        return prepend();
    int size = d->end - d->begin;
    if (i >= size)
        return append();

    bool leftward = false;

    if (d->begin == 0) {
        if (d->end == d->alloc) {
            // If the array is full, we expand it and move some items rightward
            realloc_grow(1);
        } else {
            // If there is free space at the end of the array, we move some items rightward
        }
    } else {
        if (d->end == d->alloc) {
            // If there is free space at the beginning of the array, we move some items leftward
            leftward = true;
        } else {
            // If there is free space at both ends, we move as few items as possible
            leftward = (i < size - i);
        }
    }

    if (leftward) {
        --d->begin;
        ::memmove(d->array + d->begin, d->array + d->begin + 1, i * sizeof(void *));
    } else {
        ::memmove(d->array + d->begin + i + 1, d->array + d->begin + i,
                  (size - i) * sizeof(void *));
        ++d->end;
    }
    return d->array + d->begin + i;
}

void QListData::remove(int i)
{
    Q_ASSERT(!d->ref.isShared());
    i += d->begin;
    if (i - d->begin < d->end - i) {
        if (int offset = i - d->begin)
            ::memmove(d->array + d->begin + 1, d->array + d->begin, offset * sizeof(void *));
        d->begin++;
    } else {
        if (int offset = d->end - i - 1)
            ::memmove(d->array + i, d->array + i + 1, offset * sizeof(void *));
        d->end--;
    }
}

void QListData::remove(int i, int n)
{
    Q_ASSERT(!d->ref.isShared());
    i += d->begin;
    int middle = i + n/2;
    if (middle - d->begin < d->end - middle) {
        ::memmove(d->array + d->begin + n, d->array + d->begin,
                   (i - d->begin) * sizeof(void*));
        d->begin += n;
    } else {
        ::memmove(d->array + i, d->array + i + n,
                   (d->end - i - n) * sizeof(void*));
        d->end -= n;
    }
}

void QListData::move(int from, int to)
{
    Q_ASSERT(!d->ref.isShared());
    if (from == to)
        return;

    from += d->begin;
    to += d->begin;
    void *t = d->array[from];

    if (from < to) {
        if (d->end == d->alloc || 3 * (to - from) < 2 * (d->end - d->begin)) {
            ::memmove(d->array + from, d->array + from + 1, (to - from) * sizeof(void *));
        } else {
            // optimization
            if (int offset = from - d->begin)
                ::memmove(d->array + d->begin + 1, d->array + d->begin, offset * sizeof(void *));
            if (int offset = d->end - (to + 1))
                ::memmove(d->array + to + 2, d->array + to + 1, offset * sizeof(void *));
            ++d->begin;
            ++d->end;
            ++to;
        }
    } else {
        if (d->begin == 0 || 3 * (from - to) < 2 * (d->end - d->begin)) {
            ::memmove(d->array + to + 1, d->array + to, (from - to) * sizeof(void *));
        } else {
            // optimization
            if (int offset = to - d->begin)
                ::memmove(d->array + d->begin - 1, d->array + d->begin, offset * sizeof(void *));
            if (int offset = d->end - (from + 1))
                ::memmove(d->array + from, d->array + from + 1, offset * sizeof(void *));
            --d->begin;
            --d->end;
            --to;
        }
    }
    d->array[to] = t;
}

void **QListData::erase(void **xi)
{
    Q_ASSERT(!d->ref.isShared());
    int i = xi - (d->array + d->begin);
    remove(i);
    return d->array + d->begin + i;
}

/*! \class QList
    \inmodule QtCore
    \brief The QList class is a template class that provides lists.

    \ingroup tools
    \ingroup shared

    \reentrant

    QList\<T\> is one of Qt's generic \l{container classes}. It
    stores items in a list that provides fast index-based access
    and index-based insertions and removals.

    QList\<T\>, QLinkedList\<T\>, and QVector\<T\> provide similar
    APIs and functionality. They are often interchangeable, but there
    are performance consequences. Here is an overview of use cases:

    \list
    \li QVector should be your default first choice.
        QVector\<T\> will usually give better performance than QList\<T\>,
        because QVector\<T\> always stores its items sequentially in memory,
        where QList\<T\> will allocate its items on the heap unless
        \c {sizeof(T) <= sizeof(void*)} and T has been declared to be
        either a \c{Q_MOVABLE_TYPE} or a \c{Q_PRIMITIVE_TYPE} using
        \l {Q_DECLARE_TYPEINFO}. See the \l {Pros and Cons of Using QList}
        for an explanation.
    \li However, QList is used throughout the Qt APIs for passing
        parameters and for returning values. Use QList to interface with
        those APIs.
    \li If you need a real linked list, which guarantees
        \l {Algorithmic Complexity}{constant time} insertions mid-list and
        uses iterators to items rather than indexes, use QLinkedList.
    \endlist

    \note QVector and QVarLengthArray both guarantee C-compatible
    array layout. QList does not. This might be important if your
    application must interface with a C API.

    \note Iterators into a QLinkedList and references into
    heap-allocating QLists remain valid as long as the referenced items
    remain in the container. This is not true for iterators and
    references into a QVector and non-heap-allocating QLists.

    Internally, QList\<T\> is represented as an array of T if
    \c{sizeof(T) <= sizeof(void*)} and T has been declared to be
    either a \c{Q_MOVABLE_TYPE} or a \c{Q_PRIMITIVE_TYPE} using
    \l {Q_DECLARE_TYPEINFO}. Otherwise, QList\<T\> is represented
    as an array of T* and the items are allocated on the heap.

    The array representation allows very fast insertions and
    index-based access. The prepend() and append() operations are
    also very fast because QList preallocates memory at both
    ends of its internal array. (See \l{Algorithmic Complexity} for
    details.

    Note, however, that when the conditions specified above are not met,
    each append or insert of a new item requires allocating the new item
    on the heap, and this per item allocation will make QVector a better
    choice for use cases that do a lot of appending or inserting, because
    QVector can allocate memory for many items in a single heap allocation.

    Note that the internal array only ever gets bigger over the life
    of the list. It never shrinks. The internal array is deallocated
    by the destructor and by the assignment operator, when one list
    is assigned to another.

    Here's an example of a QList that stores integers and
    a QList that stores QDate values:

    \snippet code/src_corelib_tools_qlistdata.cpp 0

    Qt includes a QStringList class that inherits QList\<QString\>
    and adds a few convenience functions, such as QStringList::join()
    and QStringList::filter().  QString::split() creates QStringLists
    from strings.

    QList stores a list of items. The default constructor creates an
    empty list. You can use the initializer-list constructor to create
    a list with elements:

    \snippet code/src_corelib_tools_qlistdata.cpp 1a

    QList provides these basic functions to add, move, and remove
    items: insert(), replace(), removeAt(), move(), and swap(). In
    addition, it provides the following convenience functions:
    append(), \l{operator<<()}, \l{operator+=()}, prepend(), removeFirst(),
    and removeLast().

    \l{operator<<()} allows to conveniently add multiple elements to a list:

    \snippet code/src_corelib_tools_qlistdata.cpp 1b

    QList uses 0-based indexes, just like C++ arrays. To access the
    item at a particular index position, you can use operator[](). On
    non-const lists, operator[]() returns a reference to the item and
    can be used on the left side of an assignment:

    \snippet code/src_corelib_tools_qlistdata.cpp 2

    Because QList is implemented as an array of pointers for types
    that are larger than a pointer or are not movable, this operation
    requires (\l{Algorithmic Complexity}{constant time}). For read-only
    access, an alternative syntax is to use at():

    \snippet code/src_corelib_tools_qlistdata.cpp 3

    at() can be faster than operator[](), because it never causes a
    \l{deep copy} to occur.

    A common requirement is to remove an item from a list and do
    something with it. For this, QList provides takeAt(), takeFirst(),
    and takeLast(). Here's a loop that removes the items from a list
    one at a time and calls \c delete on them:

    \snippet code/src_corelib_tools_qlistdata.cpp 4

    Inserting and removing items at either end of the list is very
    fast (\l{Algorithmic Complexity}{constant time} in most cases),
    because QList preallocates extra space on both sides of its
    internal buffer to allow for fast growth at both ends of the list.

    If you want to find all occurrences of a particular value in a
    list, use indexOf() or lastIndexOf(). The former searches forward
    starting from a given index position, the latter searches
    backward. Both return the index of a matching item if they find
    it; otherwise, they return -1. For example:

    \snippet code/src_corelib_tools_qlistdata.cpp 5

    If you simply want to check whether a list contains a particular
    value, use contains(). If you want to find out how many times a
    particular value occurs in the list, use count(). If you want to
    replace all occurrences of a particular value with another, use
    replace().

    QList's value type must be an \l{assignable data type}. This
    covers most data types that are commonly used, but the compiler
    won't let you, for example, store a QWidget as a value; instead,
    store a QWidget *. A few functions have additional requirements;
    for example, indexOf() and lastIndexOf() expect the value type to
    support \c operator==().  These requirements are documented on a
    per-function basis.

    Like the other container classes, QList provides \l{Java-style
    iterators} (QListIterator and QMutableListIterator) and
    \l{STL-style iterators} (QList::const_iterator and
    QList::iterator). In practice, these are rarely used, because you
    can use indexes into the QList. QList is implemented in such a way
    that direct index-based access is just as fast as using iterators.

    QList does \e not support inserting, prepending, appending or
    replacing with references to its own values. Doing so will cause
    your application to abort with an error message.

    To make QList as efficient as possible, its member functions don't
    validate their input before using it. Except for isEmpty(), member
    functions always assume the list is \e not empty. Member functions
    that take index values as parameters always assume their index
    value parameters are in the valid range. This means QList member
    functions can fail. If you define QT_NO_DEBUG when you compile,
    failures will not be detected. If you \e don't define QT_NO_DEBUG,
    failures will be detected using Q_ASSERT() or Q_ASSERT_X() with an
    appropriate message.

    To avoid failures when your list can be empty, call isEmpty()
    before calling other member functions. If you must pass an index
    value that might not be in the valid range, check that it is less
    than the value returned by size() but \e not less than 0.

    \section1 More Members

    If T is a QByteArray, this class has a couple more members that can be
    used. See the documentation for QByteArrayList for more information.

    If T is QString, this class has the following additional members:
    \l{QStringList::filter()}{filter},
    \l{QStringList::join()}{join},
    \l{QStringList::removeDuplicates()}{removeDuplicates},
    \l{QStringList::sort()}{sort}.

    \section1 More Information on Using Qt Containers

    For a detailed discussion comparing Qt containers with each other and
    with STL containers, see \l {Understand the Qt Containers}.

    \sa QListIterator, QMutableListIterator, QLinkedList, QVector
*/

/*!
    \fn template <class T> QList<T>::QList(QList<T> &&other)

    Move-constructs a QList instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*!
    \fn template <class T> QList<T> QList<T>::mid(int pos, int length) const

    Returns a sub-list which includes elements from this list,
    starting at position \a pos. If \a length is -1 (the default), all
    elements from \a pos are included; otherwise \a length elements (or
    all remaining elements if there are less than \a length elements)
    are included.
*/

/*! \fn template <class T> QList<T>::QList()

    Constructs an empty list.
*/

/*! \fn template <class T> QList<T>::QList(const QList<T> &other)

    Constructs a copy of \a other.

    This operation takes \l{Algorithmic Complexity}{constant time},
    because QList is \l{implicitly shared}. This makes returning a
    QList from a function very fast. If a shared instance is modified,
    it will be copied (copy-on-write), and that takes
    \l{Algorithmic Complexity}{linear time}.

    \sa operator=()
*/

/*! \fn template <class T> QList<T>::QList(std::initializer_list<T> args)
    \since 4.8

    Construct a list from the std::initializer_list specified by \a args.

    This constructor is only enabled if the compiler supports C++11 initializer
    lists.
*/

/*! \fn template <class T> QList<T>::~QList()

    Destroys the list. References to the values in the list and all
    iterators of this list become invalid.
*/

/*! \fn template <class T> QList<T> &QList<T>::operator=(const QList<T> &other)

    Assigns \a other to this list and returns a reference to this
    list.
*/

/*!
    \fn template <class T> QList &QList<T>::operator=(QList<T> &&other)

    Move-assigns \a other to this QList instance.

    \since 5.2
*/

/*! \fn template <class T> void QList<T>::swap(QList<T> &other)
    \since 4.8

    Swaps list \a other with this list. This operation is very
    fast and never fails.
*/

/*! \fn template <class T> bool QList<T>::operator==(const QList<T> &other) const

    Returns \c true if \a other is equal to this list; otherwise returns
    false.

    Two lists are considered equal if they contain the same values in
    the same order.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa operator!=()
*/

/*! \fn template <class T> bool QList<T>::operator!=(const QList<T> &other) const

    Returns \c true if \a other is not equal to this list; otherwise
    returns \c false.

    Two lists are considered equal if they contain the same values in
    the same order.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa operator==()
*/

/*! \fn template <class T> bool operator<(const QList<T> &lhs, const QList<T> &rhs)
    \since 5.6
    \relates QList

    Returns \c true if list \a lhs is
    \l{http://en.cppreference.com/w/cpp/algorithm/lexicographical_compare}
    {lexicographically less than} \a rhs; otherwise returns \c false.

    This function requires the value type to have an implementation
    of \c operator<().
*/

/*! \fn template <class T> bool operator<=(const QList<T> &lhs, const QList<T> &rhs)
    \since 5.6
    \relates QList

    Returns \c true if list \a lhs is
    \l{http://en.cppreference.com/w/cpp/algorithm/lexicographical_compare}
    {lexicographically less than or equal to} \a rhs; otherwise returns \c false.

    This function requires the value type to have an implementation
    of \c operator<().
*/

/*! \fn template <class T> bool operator>(const QList<T> &lhs, const QList<T> &rhs)
    \since 5.6
    \relates QList

    Returns \c true if list \a lhs is
    \l{http://en.cppreference.com/w/cpp/algorithm/lexicographical_compare}
    {lexicographically greater than} \a rhs; otherwise returns \c false.

    This function requires the value type to have an implementation
    of \c operator<().
*/

/*! \fn template <class T> bool operator>=(const QList<T> &lhs, const QList<T> &rhs)
    \since 5.6
    \relates QList

    Returns \c true if list \a lhs is
    \l{http://en.cppreference.com/w/cpp/algorithm/lexicographical_compare}
    {lexicographically greater than or equal to} \a rhs; otherwise returns \c false.

    This function requires the value type to have an implementation
    of \c operator<().
*/

/*!
    \fn template <class T> uint qHash(const QList<T> &key, uint seed = 0)
    \since 5.6
    \relates QList

    Returns the hash value for \a key,
    using \a seed to seed the calculation.

    This function requires qHash() to be overloaded for the value type \c T.
*/

/*!
    \fn template <class T> int QList<T>::size() const

    Returns the number of items in the list.

    \sa isEmpty(), count()
*/

/*! \fn template <class T> void QList<T>::detach()

    \internal
*/

/*! \fn template <class T> void QList<T>::detachShared()

    \internal

    like detach(), but does nothing if we're shared_null.
    This prevents needless mallocs, and makes QList more exception safe
    in case of cleanup work done in destructors on empty lists.
*/

/*! \fn template <class T> bool QList<T>::isDetached() const

    \internal
*/

/*! \fn template <class T> void QList<T>::setSharable(bool sharable)

    \internal
*/

/*! \fn template <class T> bool QList<T>::isSharedWith(const QList<T> &other) const

    \internal
*/

/*! \fn template <class T> bool QList<T>::isEmpty() const

    Returns \c true if the list contains no items; otherwise returns
    false.

    \sa size()
*/

/*! \fn template <class T> void QList<T>::clear()

    Removes all items from the list.

    \sa removeAll()
*/

/*! \fn template <class T> const T &QList<T>::at(int i) const

    Returns the item at index position \a i in the list. \a i must be
    a valid index position in the list (i.e., 0 <= \a i < size()).

    This function is very fast (\l{Algorithmic Complexity}{constant time}).

    \sa value(), operator[]()
*/

/*! \fn template <class T> T &QList<T>::operator[](int i)

    Returns the item at index position \a i as a modifiable reference.
    \a i must be a valid index position in the list (i.e., 0 <= \a i <
    size()).

    If this function is called on a list that is currently being shared, it
    will trigger a copy of all elements. Otherwise, this function runs in
    \l{Algorithmic Complexity}{constant time}. If you do not want to modify
    the list you should use QList::at().

    \sa at(), value()
*/

/*! \fn template <class T> const T &QList<T>::operator[](int i) const

    \overload

    Same as at(). This function runs in \l{Algorithmic Complexity}{constant time}.
*/

/*! \fn template <class T> void QList<T>::reserve(int alloc)

    Reserve space for \a alloc elements.

    If \a alloc is smaller than the current size of the list, nothing will happen.

    Use this function to avoid repetetive reallocation of QList's internal
    data if you can predict how many elements will be appended.
    Note that the reservation applies only to the internal pointer array.

    \since 4.7
*/

/*! \fn template <class T> void QList<T>::append(const T &value)

    Inserts \a value at the end of the list.

    Example:
    \snippet code/src_corelib_tools_qlistdata.cpp 6

    This is the same as list.insert(size(), \a value).

    If this list is not shared, this operation is typically
    very fast (amortized \l{Algorithmic Complexity}{constant time}),
    because QList preallocates extra space on both sides of its
    internal buffer to allow for fast growth at both ends of the list.

    \sa operator<<(), prepend(), insert()
*/

/*! \fn template <class T> void QList<T>::append(const QList<T> &value)

    \overload

    \since 4.5

    Appends the items of the \a value list to this list.

    \sa operator<<(), operator+=()
*/

/*! \fn template <class T> void QList<T>::prepend(const T &value)

    Inserts \a value at the beginning of the list.

    Example:
    \snippet code/src_corelib_tools_qlistdata.cpp 7

    This is the same as list.insert(0, \a value).

    If this list is not shared, this operation is typically
    very fast (amortized \l{Algorithmic Complexity}{constant time}),
    because QList preallocates extra space on both sides of its
    internal buffer to allow for fast growth at both ends of the list.

    \sa append(), insert()
*/

/*! \fn template <class T> void QList<T>::insert(int i, const T &value)

    Inserts \a value at index position \a i in the list. If \a i <= 0,
    the value is prepended to the list. If \a i >= size(), the
    value is appended to the list.

    Example:
    \snippet code/src_corelib_tools_qlistdata.cpp 8

    \sa append(), prepend(), replace(), removeAt()
*/

/*! \fn template <class T> QList<T>::iterator QList<T>::insert(iterator before, const T &value)

    \overload

    Inserts \a value in front of the item pointed to by the
    iterator \a before. Returns an iterator pointing at the inserted
    item. Note that the iterator passed to the function will be
    invalid after the call; the returned iterator should be used
    instead.
*/

/*! \fn template <class T> void QList<T>::replace(int i, const T &value)

    Replaces the item at index position \a i with \a value. \a i must
    be a valid index position in the list (i.e., 0 <= \a i < size()).

    \sa operator[](), removeAt()
*/

/*!
    \fn template <class T> int QList<T>::removeAll(const T &value)

    Removes all occurrences of \a value in the list and returns the
    number of entries removed.

    Example:
    \snippet code/src_corelib_tools_qlistdata.cpp 9

    This function requires the value type to have an implementation of
    \c operator==().

    \sa removeOne(), removeAt(), takeAt(), replace()
*/

/*!
    \fn template <class T> bool QList<T>::removeOne(const T &value)
    \since 4.4

    Removes the first occurrence of \a value in the list and returns
    true on success; otherwise returns \c false.

    Example:
    \snippet code/src_corelib_tools_qlistdata.cpp 10

    This function requires the value type to have an implementation of
    \c operator==().

    \sa removeAll(), removeAt(), takeAt(), replace()
*/

/*! \fn template <class T> void QList<T>::removeAt(int i)

    Removes the item at index position \a i. \a i must be a valid
    index position in the list (i.e., 0 <= \a i < size()).

    \sa takeAt(), removeFirst(), removeLast(), removeOne()
*/

/*! \fn template <class T> T QList<T>::takeAt(int i)

    Removes the item at index position \a i and returns it. \a i must
    be a valid index position in the list (i.e., 0 <= \a i < size()).

    If you don't use the return value, removeAt() is more efficient.

    \sa removeAt(), takeFirst(), takeLast()
*/

/*! \fn template <class T> T QList<T>::takeFirst()

    Removes the first item in the list and returns it. This is the
    same as takeAt(0). This function assumes the list is not empty. To
    avoid failure, call isEmpty() before calling this function.

    If this list is not shared, this operation takes
    \l {Algorithmic Complexity}{constant time}.

    If you don't use the return value, removeFirst() is more
    efficient.

    \sa takeLast(), takeAt(), removeFirst()
*/

/*! \fn template <class T> T QList<T>::takeLast()

    Removes the last item in the list and returns it. This is the
    same as takeAt(size() - 1). This function assumes the list is
    not empty. To avoid failure, call isEmpty() before calling this
    function.

    If this list is not shared, this operation takes
    \l {Algorithmic Complexity}{constant time}.

    If you don't use the return value, removeLast() is more
    efficient.

    \sa takeFirst(), takeAt(), removeLast()
*/

/*! \fn template <class T> void QList<T>::move(int from, int to)

    Moves the item at index position \a from to index position \a to.

    Example:
    \snippet code/src_corelib_tools_qlistdata.cpp 11

    This is the same as insert(\a{to}, takeAt(\a{from})).This function
    assumes that both \a from and \a to are at least 0 but less than
    size(). To avoid failure, test that both \a from and \a to are at
    least 0 and less than size().

    \sa swap(), insert(), takeAt()
*/

/*! \fn template <class T> void QList<T>::swap(int i, int j)

    Exchange the item at index position \a i with the item at index
    position \a j. This function assumes that both \a i and \a j are
    at least 0 but less than size(). To avoid failure, test that both
    \a i and \a j are at least 0 and less than size().

    Example:
    \snippet code/src_corelib_tools_qlistdata.cpp 12

    \sa move()
*/

/*! \fn template <class T> int QList<T>::indexOf(const T &value, int from = 0) const

    Returns the index position of the first occurrence of \a value in
    the list, searching forward from index position \a from. Returns
    -1 if no item matched.

    Example:
    \snippet code/src_corelib_tools_qlistdata.cpp 13

    This function requires the value type to have an implementation of
    \c operator==().

    Note that QList uses 0-based indexes, just like C++ arrays. Negative
    indexes are not supported with the exception of the value mentioned
    above.

    \sa lastIndexOf(), contains()
*/

/*! \fn template <class T> int QList<T>::lastIndexOf(const T &value, int from = -1) const

    Returns the index position of the last occurrence of \a value in
    the list, searching backward from index position \a from. If \a
    from is -1 (the default), the search starts at the last item.
    Returns -1 if no item matched.

    Example:
    \snippet code/src_corelib_tools_qlistdata.cpp 14

    This function requires the value type to have an implementation of
    \c operator==().

    Note that QList uses 0-based indexes, just like C++ arrays. Negative
    indexes are not supported with the exception of the value mentioned
    above.

    \sa indexOf()
*/

/*! \fn template <class T> bool QList<T>::contains(const T &value) const

    Returns \c true if the list contains an occurrence of \a value;
    otherwise returns \c false.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa indexOf(), count()
*/

/*! \fn template <class T> int QList<T>::count(const T &value) const

    Returns the number of occurrences of \a value in the list.

    This function requires the value type to have an implementation of
    \c operator==().

    \sa contains(), indexOf()
*/

/*! \fn template <class T> bool QList<T>::startsWith(const T &value) const
    \since 4.5

    Returns \c true if this list is not empty and its first
    item is equal to \a value; otherwise returns \c false.

    \sa isEmpty(), contains()
*/

/*! \fn template <class T> bool QList<T>::endsWith(const T &value) const
    \since 4.5

    Returns \c true if this list is not empty and its last
    item is equal to \a value; otherwise returns \c false.

    \sa isEmpty(), contains()
*/

/*! \fn template <class T> QList<T>::iterator QList<T>::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first item in
    the list.

    \sa constBegin(), end()
*/

/*! \fn template <class T> QList<T>::const_iterator QList<T>::begin() const

    \overload
*/

/*! \fn template <class T> QList<T>::const_iterator QList<T>::cbegin() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the list.

    \sa begin(), cend()
*/

/*! \fn template <class T> QList<T>::const_iterator QList<T>::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the list.

    \sa begin(), constEnd()
*/

/*! \fn template <class T> QList<T>::iterator QList<T>::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary item
    after the last item in the list.

    \sa begin(), constEnd()
*/

/*! \fn template <class T> const_iterator QList<T>::end() const

    \overload
*/

/*! \fn template <class T> QList<T>::const_iterator QList<T>::cend() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the list.

    \sa cbegin(), end()
*/

/*! \fn template <class T> QList<T>::const_iterator QList<T>::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the list.

    \sa constBegin(), end()
*/

/*! \fn template <class T> QList<T>::reverse_iterator QList<T>::rbegin()
    \since 5.6

    Returns a \l{STL-style iterators}{STL-style} reverse iterator pointing to the first
    item in the list, in reverse order.

    \sa begin(), crbegin(), rend()
*/

/*! \fn template <class T> QList<T>::const_reverse_iterator QList<T>::rbegin() const
    \since 5.6
    \overload
*/

/*! \fn template <class T> QList<T>::const_reverse_iterator QList<T>::crbegin() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator pointing to the first
    item in the list, in reverse order.

    \sa begin(), rbegin(), rend()
*/

/*! \fn template <class T> QList<T>::reverse_iterator QList<T>::rend()
    \since 5.6

    Returns a \l{STL-style iterators}{STL-style} reverse iterator pointing to one past
    the last item in the list, in reverse order.

    \sa end(), crend(), rbegin()
*/

/*! \fn template <class T> QList<T>::const_reverse_iterator QList<T>::rend() const
    \since 5.6
    \overload
*/

/*! \fn template <class T> QList<T>::const_reverse_iterator QList<T>::crend() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style} reverse iterator pointing to one
    past the last item in the list, in reverse order.

    \sa end(), rend(), rbegin()
*/

/*! \fn template <class T> QList<T>::iterator QList<T>::erase(iterator pos)

    Removes the item associated with the iterator \a pos from the
    list, and returns an iterator to the next item in the list (which
    may be end()).

    \sa insert(), removeAt()
*/

/*! \fn template <class T> QList<T>::iterator QList<T>::erase(iterator begin, iterator end)

    \overload

    Removes all the items from \a begin up to (but not including) \a
    end. Returns an iterator to the same item that \a end referred to
    before the call.
*/

/*! \typedef QList::Iterator

    Qt-style synonym for QList::iterator.
*/

/*! \typedef QList::ConstIterator

    Qt-style synonym for QList::const_iterator.
*/

/*!
    \typedef QList::size_type

    Typedef for int. Provided for STL compatibility.
*/

/*!
    \typedef QList::value_type

    Typedef for T. Provided for STL compatibility.
*/

/*!
    \typedef QList::difference_type

    Typedef for ptrdiff_t. Provided for STL compatibility.
*/

/*!
    \typedef QList::pointer

    Typedef for T *. Provided for STL compatibility.
*/

/*!
    \typedef QList::const_pointer

    Typedef for const T *. Provided for STL compatibility.
*/

/*!
    \typedef QList::reference

    Typedef for T &. Provided for STL compatibility.
*/

/*!
    \typedef QList::const_reference

    Typedef for const T &. Provided for STL compatibility.
*/

/*! \typedef QList::reverse_iterator
    \since 5.6

    The QList::reverse_iterator typedef provides an STL-style non-const
    reverse iterator for QList.

    It is simply a typedef for \c{std::reverse_iterator<iterator>}.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QList::rbegin(), QList::rend(), QList::const_reverse_iterator, QList::iterator
*/

/*! \typedef QList::const_reverse_iterator
    \since 5.6

    The QList::const_reverse_iterator typedef provides an STL-style const
    reverse iterator for QList.

    It is simply a typedef for \c{std::reverse_iterator<const_iterator>}.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QList::rbegin(), QList::rend(), QList::reverse_iterator, QList::const_iterator
*/

/*! \fn template <class T> int QList<T>::count() const

    Returns the number of items in the list. This is effectively the
    same as size().
*/

/*! \fn template <class T> int QList<T>::length() const
    \since 4.5

    This function is identical to count().

    \sa count()
*/

/*! \fn template <class T> T& QList<T>::first()

    Returns a reference to the first item in the list. The list must
    not be empty. If the list can be empty, call isEmpty() before
    calling this function.

    \sa constFirst(), last(), isEmpty()
*/

/*! \fn template <class T> const T& QList<T>::first() const

    \overload
*/

/*! \fn template <class T> const T& QList<T>::constFirst() const
    \since 5.6

    Returns a const reference to the first item in the list. The list must
    not be empty. If the list can be empty, call isEmpty() before
    calling this function.

    \sa constLast(), isEmpty(), first()
*/

/*! \fn template <class T> T& QList<T>::last()

    Returns a reference to the last item in the list.  The list must
    not be empty. If the list can be empty, call isEmpty() before
    calling this function.

    \sa constLast(), first(), isEmpty()
*/

/*! \fn template <class T> const T& QList<T>::last() const

    \overload
*/

/*! \fn template <class T> const T& QList<T>::constLast() const
    \since 5.6

    Returns a reference to the last item in the list. The list must
    not be empty. If the list can be empty, call isEmpty() before
    calling this function.

    \sa constFirst(), isEmpty(), last()
*/

/*! \fn template <class T> void QList<T>::removeFirst()

    Removes the first item in the list. Calling this function is
    equivalent to calling removeAt(0). The list must not be empty. If
    the list can be empty, call isEmpty() before calling this
    function.

    \sa removeAt(), takeFirst()
*/

/*! \fn template <class T> void QList<T>::removeLast()

    Removes the last item in the list. Calling this function is
    equivalent to calling removeAt(size() - 1). The list must not be
    empty. If the list can be empty, call isEmpty() before calling
    this function.

    \sa removeAt(), takeLast()
*/

/*! \fn template <class T> T QList<T>::value(int i) const

    Returns the value at index position \a i in the list.

    If the index \a i is out of bounds, the function returns a
    \l{default-constructed value}. If you are certain that the index
    is going to be within bounds, you can use at() instead, which is
    slightly faster.

    \sa at(), operator[]()
*/

/*! \fn template <class T> T QList<T>::value(int i, const T &defaultValue) const

    \overload

    If the index \a i is out of bounds, the function returns
    \a defaultValue.
*/

/*! \fn template <class T> void QList<T>::push_back(const T &value)

    This function is provided for STL compatibility. It is equivalent
    to \l{QList::append()}{append(\a value)}.
*/

/*! \fn template <class T> void QList<T>::push_front(const T &value)

    This function is provided for STL compatibility. It is equivalent
    to \l{QList::prepend()}{prepend(\a value)}.
*/

/*! \fn template <class T> T& QList<T>::front()

    This function is provided for STL compatibility. It is equivalent
    to first(). The list must not be empty. If the list can be empty,
    call isEmpty() before calling this function.
*/

/*! \fn template <class T> const T& QList<T>::front() const

    \overload
*/

/*! \fn template <class T> T& QList<T>::back()

    This function is provided for STL compatibility. It is equivalent
    to last(). The list must not be empty. If the list can be empty,
    call isEmpty() before calling this function.
*/

/*! \fn template <class T> const T& QList<T>::back() const

    \overload
*/

/*! \fn template <class T> void QList<T>::pop_front()

    This function is provided for STL compatibility. It is equivalent
    to removeFirst(). The list must not be empty. If the list can be
    empty, call isEmpty() before calling this function.
*/

/*! \fn template <class T> void QList<T>::pop_back()

    This function is provided for STL compatibility. It is equivalent
    to removeLast(). The list must not be empty. If the list can be
    empty, call isEmpty() before calling this function.
*/

/*! \fn template <class T> bool QList<T>::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty() and returns \c true if the list is empty.
*/

/*! \fn template <class T> QList<T> &QList<T>::operator+=(const QList<T> &other)

    Appends the items of the \a other list to this list and returns a
    reference to this list.

    \sa operator+(), append()
*/

/*! \fn template <class T> void QList<T>::operator+=(const T &value)

    \overload

    Appends \a value to the list.

    \sa append(), operator<<()
*/

/*! \fn template <class T> QList<T> QList<T>::operator+(const QList<T> &other) const

    Returns a list that contains all the items in this list followed
    by all the items in the \a other list.

    \sa operator+=()
*/

/*! \fn template <class T> QList<T> &QList<T>::operator<<(const QList<T> &other)

    Appends the items of the \a other list to this list and returns a
    reference to this list.

    \sa operator+=(), append()
*/

/*! \fn template <class T> void QList<T>::operator<<(const T &value)

    \overload

    Appends \a value to the list.
*/

/*! \class QList::iterator
    \inmodule QtCore
    \brief The QList::iterator class provides an STL-style non-const iterator for QList and QQueue.

    QList features both \l{STL-style iterators} and \l{Java-style
    iterators}. The STL-style iterators are more low-level and more
    cumbersome to use; on the other hand, they are slightly faster
    and, for developers who already know STL, have the advantage of
    familiarity.

    QList\<T\>::iterator allows you to iterate over a QList\<T\> (or
    QQueue\<T\>) and to modify the list item associated with the
    iterator. If you want to iterate over a const QList, use
    QList::const_iterator instead. It is generally good practice to
    use QList::const_iterator on a non-const QList as well, unless
    you need to change the QList through the iterator. Const
    iterators are slightly faster, and can improve code readability.

    The default QList::iterator constructor creates an uninitialized
    iterator. You must initialize it using a QList function like
    QList::begin(), QList::end(), or QList::insert() before you can
    start iterating. Here's a typical loop that prints all the items
    stored in a list:

    \snippet code/src_corelib_tools_qlistdata.cpp 15

    Let's see a few examples of things we can do with a
    QList::iterator that we cannot do with a QList::const_iterator.
    Here's an example that increments every value stored in a
    QList\<int\> by 2:

    \snippet code/src_corelib_tools_qlistdata.cpp 16

    Most QList functions accept an integer index rather than an
    iterator. For that reason, iterators are rarely useful in
    connection with QList. One place where STL-style iterators do
    make sense is as arguments to \l{generic algorithms}.

    For example, here's how to delete all the widgets stored in a
    QList\<QWidget *\>:

    \snippet code/src_corelib_tools_qlistdata.cpp 17

    Multiple iterators can be used on the same list. However, be
    aware that any non-const function call performed on the QList
    will render all existing iterators undefined. If you need to keep
    iterators over a long period of time, we recommend that you use
    QLinkedList rather than QList.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QList::const_iterator, QMutableListIterator
*/

/*! \typedef QList::iterator::iterator_category

  A synonym for \e {std::random_access_iterator_tag} indicating
  this iterator is a random access iterator.
*/

/*! \typedef QList::iterator::difference_type

    \internal
*/

/*! \typedef QList::iterator::value_type

    \internal
*/

/*! \typedef QList::iterator::pointer

    \internal
*/

/*! \typedef QList::iterator::reference

    \internal
*/

/*! \fn template <class T> QList<T>::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa QList::begin(), QList::end()
*/

/*! \fn template <class T> QList<T>::iterator::iterator(Node *node)

    \internal
*/

/*! \fn template <class T> QList<T>::iterator::iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn template <class T> T &QList<T>::iterator::operator*() const

    Returns a modifiable reference to the current item.

    You can change the value of an item by using operator*() on the
    left side of an assignment, for example:

    \snippet code/src_corelib_tools_qlistdata.cpp 18

    \sa operator->()
*/

/*! \fn template <class T> T *QList<T>::iterator::operator->() const

    Returns a pointer to the current item.

    \sa operator*()
*/

/*! \fn template <class T> T &QList<T>::iterator::operator[](difference_type j) const

    Returns a modifiable reference to the item at position *this +
    \a{j}.

    This function is provided to make QList iterators behave like C++
    pointers.

    \sa operator+()
*/

/*!
    \fn template <class T> bool QList<T>::iterator::operator==(const iterator &other) const
    \fn template <class T> bool QList<T>::iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn template <class T> bool QList<T>::iterator::operator!=(const iterator &other) const
    \fn template <class T> bool QList<T>::iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template <class T> bool QList<T>::iterator::operator<(const iterator& other) const
    \fn template <class T> bool QList<T>::iterator::operator<(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    the item pointed to by the \a other iterator.
*/

/*!
    \fn template <class T> bool QList<T>::iterator::operator<=(const iterator& other) const
    \fn template <class T> bool QList<T>::iterator::operator<=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    or equal to the item pointed to by the \a other iterator.
*/

/*!
    \fn template <class T> bool QList<T>::iterator::operator>(const iterator& other) const
    \fn template <class T> bool QList<T>::iterator::operator>(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than the item pointed to by the \a other iterator.
*/

/*!
    \fn template <class T> bool QList<T>::iterator::operator>=(const iterator& other) const
    \fn template <class T> bool QList<T>::iterator::operator>=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than or equal to the item pointed to by the \a other iterator.
*/

/*! \fn template <class T> QList<T>::iterator &QList<T>::iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the list and returns an iterator to the new current
    item.

    Calling this function on QList::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn template <class T> QList<T>::iterator QList<T>::iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the list and returns an iterator to the previously
    current item.
*/

/*! \fn template <class T> QList<T>::iterator &QList<T>::iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QList::begin() leads to undefined results.

    \sa operator++()
*/

/*! \fn template <class T> QList<T>::iterator QList<T>::iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn template <class T> QList<T>::iterator &QList<T>::iterator::operator+=(difference_type j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    \sa operator-=(), operator+()
*/

/*! \fn template <class T> QList<T>::iterator &QList<T>::iterator::operator-=(difference_type j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    \sa operator+=(), operator-()
*/

/*! \fn template <class T> QList<T>::iterator QList<T>::iterator::operator+(difference_type j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    \sa operator-(), operator+=()
*/

/*! \fn template <class T> QList<T>::iterator QList<T>::iterator::operator-(difference_type j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    \sa operator+(), operator-=()
*/

/*! \fn template <class T> int QList<T>::iterator::operator-(iterator other) const

    Returns the number of items between the item pointed to by \a
    other and the item pointed to by this iterator.
*/

/*! \class QList::const_iterator
    \inmodule QtCore
    \brief The QList::const_iterator class provides an STL-style const iterator for QList and QQueue.

    QList provides both \l{STL-style iterators} and \l{Java-style
    iterators}. The STL-style iterators are more low-level and more
    cumbersome to use; on the other hand, they are slightly faster
    and, for developers who already know STL, have the advantage of
    familiarity.

    QList\<T\>::const_iterator allows you to iterate over a
    QList\<T\> (or a QQueue\<T\>). If you want to modify the QList as
    you iterate over it, use QList::iterator instead. It is generally
    good practice to use QList::const_iterator on a non-const QList
    as well, unless you need to change the QList through the
    iterator. Const iterators are slightly faster, and can improve
    code readability.

    The default QList::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a QList
    function like QList::constBegin(), QList::constEnd(), or
    QList::insert() before you can start iterating. Here's a typical
    loop that prints all the items stored in a list:

    \snippet code/src_corelib_tools_qlistdata.cpp 19

    Most QList functions accept an integer index rather than an
    iterator. For that reason, iterators are rarely useful in
    connection with QList. One place where STL-style iterators do
    make sense is as arguments to \l{generic algorithms}.

    For example, here's how to delete all the widgets stored in a
    QList\<QWidget *\>:

    \snippet code/src_corelib_tools_qlistdata.cpp 20

    Multiple iterators can be used on the same list. However, be
    aware that any non-const function call performed on the QList
    will render all existing iterators undefined. If you need to keep
    iterators over a long period of time, we recommend that you use
    QLinkedList rather than QList.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QList::iterator, QListIterator
*/

/*! \fn template <class T> QList<T>::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like operator*() and operator++() should not be called
    on an uninitialized iterator. Use operator=() to assign a value
    to it before using it.

    \sa QList::constBegin(), QList::constEnd()
*/

/*! \typedef QList::const_iterator::iterator_category

  A synonym for \e {std::random_access_iterator_tag} indicating
  this iterator is a random access iterator.
*/

/*! \typedef QList::const_iterator::difference_type

    \internal
*/

/*! \typedef QList::const_iterator::value_type

    \internal
*/

/*! \typedef QList::const_iterator::pointer

    \internal
*/

/*! \typedef QList::const_iterator::reference

    \internal
*/

/*! \fn template <class T> QList<T>::const_iterator::const_iterator(Node *node)

    \internal
*/

/*! \fn template <class T> QList<T>::const_iterator::const_iterator(const const_iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn template <class T> QList<T>::const_iterator::const_iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn template <class T> const T &QList<T>::const_iterator::operator*() const

    Returns the current item.

    \sa operator->()
*/

/*! \fn template <class T> const T *QList<T>::const_iterator::operator->() const

    Returns a pointer to the current item.

    \sa operator*()
*/

/*! \fn template <class T> const T &QList<T>::const_iterator::operator[](difference_type j) const

    Returns the item at position *this + \a{j}.

    This function is provided to make QList iterators behave like C++
    pointers.

    \sa operator+()
*/

/*! \fn template <class T> bool QList<T>::const_iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn template <class T> bool QList<T>::const_iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template <class T> bool QList<T>::const_iterator::operator<(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    the item pointed to by the \a other iterator.
*/

/*!
    \fn template <class T> bool QList<T>::const_iterator::operator<=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is less than
    or equal to the item pointed to by the \a other iterator.
*/

/*!
    \fn template <class T> bool QList<T>::const_iterator::operator>(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than the item pointed to by the \a other iterator.
*/

/*!
    \fn template <class T> bool QList<T>::const_iterator::operator>=(const const_iterator& other) const

    Returns \c true if the item pointed to by this iterator is greater
    than or equal to the item pointed to by the \a other iterator.
*/

/*! \fn template <class T> QList<T>::const_iterator &QList<T>::const_iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the
    next item in the list and returns an iterator to the new current
    item.

    Calling this function on QList::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn template <class T> QList<T>::const_iterator QList<T>::const_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the
    next item in the list and returns an iterator to the previously
    current item.
*/

/*! \fn template <class T> QList<T>::const_iterator &QList<T>::const_iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding item
    current and returns an iterator to the new current item.

    Calling this function on QList::begin() leads to undefined results.

    \sa operator++()
*/

/*! \fn template <class T> QList<T>::const_iterator QList<T>::const_iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding item
    current and returns an iterator to the previously current item.
*/

/*! \fn template <class T> QList<T>::const_iterator &QList<T>::const_iterator::operator+=(difference_type j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    \sa operator-=(), operator+()
*/

/*! \fn template <class T> QList<T>::const_iterator &QList<T>::const_iterator::operator-=(difference_type j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    \sa operator+=(), operator-()
*/

/*! \fn template <class T> QList<T>::const_iterator QList<T>::const_iterator::operator+(difference_type j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    \sa operator-(), operator+=()
*/

/*! \fn template <class T> QList<T>::const_iterator QList<T>::const_iterator::operator-(difference_type j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    \sa operator+(), operator-=()
*/

/*! \fn template <class T> int QList<T>::const_iterator::operator-(const_iterator other) const

    Returns the number of items between the item pointed to by \a
    other and the item pointed to by this iterator.
*/

/*! \fn template <class T> QDataStream &operator<<(QDataStream &out, const QList<T> &list)
    \relates QList

    Writes the list \a list to stream \a out.

    This function requires the value type to implement \c
    operator<<().

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/

/*! \fn template <class T> QDataStream &operator>>(QDataStream &in, QList<T> &list)
    \relates QList

    Reads a list from stream \a in into \a list.

    This function requires the value type to implement \c
    operator>>().

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/

/*! \fn template <class T> QList<T> QList<T>::fromVector(const QVector<T> &vector)

    Returns a QList object with the data contained in \a vector.

    Example:

    \snippet code/src_corelib_tools_qlistdata.cpp 21

    \sa fromSet(), toVector(), QVector::toList()
*/

/*! \fn template <class T> QVector<T> QList<T>::toVector() const

    Returns a QVector object with the data contained in this QList.

    Example:

    \snippet code/src_corelib_tools_qlistdata.cpp 22

    \sa toSet(), fromVector(), QVector::fromList()
*/

/*! \fn template <class T> QList<T> QList<T>::fromSet(const QSet<T> &set)

    Returns a QList object with the data contained in \a set. The
    order of the elements in the QList is undefined.

    Example:

    \snippet code/src_corelib_tools_qlistdata.cpp 23

    \sa fromVector(), toSet(), QSet::toList()
*/

/*! \fn template <class T> QSet<T> QList<T>::toSet() const

    Returns a QSet object with the data contained in this QList.
    Since QSet doesn't allow duplicates, the resulting QSet might be
    smaller than the original list was.

    Example:

    \snippet code/src_corelib_tools_qlistdata.cpp 24

    \sa toVector(), fromSet(), QSet::fromList()
*/

/*! \fn template <class T> QList<T> QList<T>::fromStdList(const std::list<T> &list)

    Returns a QList object with the data contained in \a list. The
    order of the elements in the QList is the same as in \a list.

    Example:

    \snippet code/src_corelib_tools_qlistdata.cpp 25

    \sa toStdList(), QVector::fromStdVector()
*/

/*! \fn template <class T> std::list<T> QList<T>::toStdList() const

    Returns a std::list object with the data contained in this QList.
    Example:

    \snippet code/src_corelib_tools_qlistdata.cpp 26

    \sa fromStdList(), QVector::toStdVector()
*/

QT_END_NAMESPACE
