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

/*!
    \class QPointer
    \inmodule QtCore
    \brief The QPointer class is a template class that provides guarded pointers to QObject.

    \ingroup objectmodel

    A guarded pointer, QPointer<T>, behaves like a normal C++
    pointer \c{T *}, except that it is automatically set to 0 when the
    referenced object is destroyed (unlike normal C++ pointers, which
    become "dangling pointers" in such cases). \c T must be a
    subclass of QObject.

    Guarded pointers are useful whenever you need to store a pointer
    to a QObject that is owned by someone else, and therefore might be
    destroyed while you still hold a reference to it. You can safely
    test the pointer for validity.

    Note that Qt 5 introduces a slight change in behavior when using QPointer.

    \list

    \li When using QPointer on a QWidget (or a subclass of QWidget), previously
    the QPointer would be cleared by the QWidget destructor. Now, the QPointer
    is cleared by the QObject destructor (since this is when QWeakPointer objects are
    cleared). Any QPointers tracking a widget will \b NOT be cleared before the
    QWidget destructor destroys the children for the widget being tracked.

    \endlist

    Qt also provides QSharedPointer, an implementation of a reference-counted
    shared pointer object, which can be used to maintain a collection of
    references to an individual pointer.

    Example:

    \snippet pointer/pointer.cpp 0
    \dots
    \snippet pointer/pointer.cpp 1
    \snippet pointer/pointer.cpp 2

    If the QLabel is deleted in the meantime, the \c label variable
    will hold 0 instead of an invalid address, and the last line will
    never be executed.

    The functions and operators available with a QPointer are the
    same as those available with a normal unguarded pointer, except
    the pointer arithmetic operators (\c{+}, \c{-}, \c{++}, and
    \c{--}), which are normally used only with arrays of objects.

    Use QPointers like normal pointers and you will not need to read
    this class documentation.

    For creating guarded pointers, you can construct or assign to them
    from a T* or from another guarded pointer of the same type. You
    can compare them with each other using operator==() and
    operator!=(), or test for 0 with isNull(). You can dereference
    them using either the \c *x or the \c x->member notation.

    A guarded pointer will automatically cast to a \c T *, so you can
    freely mix guarded and unguarded pointers. This means that if you
    have a QPointer<QWidget>, you can pass it to a function that
    requires a QWidget *. For this reason, it is of little value to
    declare functions to take a QPointer as a parameter; just use
    normal pointers. Use a QPointer when you are storing a pointer
    over time.

    Note that class \c T must inherit QObject, or a compilation or
    link error will result.

    \sa QSharedPointer, QObject, QObjectCleanupHandler
*/

/*!
    \fn template <class T> QPointer<T>::QPointer()

    Constructs a 0 guarded pointer.

    \sa isNull()
*/

/*!
    \fn template <class T> QPointer<T>::QPointer(T* p)

    Constructs a guarded pointer that points to the same object that \a p
    points to.
*/

/*!
    \fn template <class T> QPointer<T>::~QPointer()

    Destroys the guarded pointer. Just like a normal pointer,
    destroying a guarded pointer does \e not destroy the object being
    pointed to.
*/

/*!
    \fn template <class T> void QPointer<T>::swap(QPointer &other)
    \since 5.6

    Swaps the contents of this QPointer with the contents of \a other.
    This operation is very fast and never fails.
*/

/*!
    \fn template <class T> QPointer<T> & QPointer<T>::operator=(T* p)

    Assignment operator. This guarded pointer will now point to the
    same object that \a p points to.
*/

/*!
    \fn template <class T> T* QPointer<T>::data() const
    \since 4.4

    Returns the pointer to the object being guarded.
*/

/*!
    \fn template <class T> bool QPointer<T>::isNull() const

    Returns \c true if the referenced object has been destroyed or if
    there is no referenced object; otherwise returns \c false.
*/

/*!
    \fn template <class T> void QPointer<T>::clear()
    \since 5.0

    Clears this QPointer object.

    \sa isNull()
*/

/*!
    \fn template <class T> T* QPointer<T>::operator->() const

    Overloaded arrow operator; implements pointer semantics. Just use
    this operator as you would with a normal C++ pointer.
*/

/*!
    \fn template <class T> T& QPointer<T>::operator*() const

    Dereference operator; implements pointer semantics. Just use this
    operator as you would with a normal C++ pointer.
*/

/*!
    \fn template <class T> QPointer<T>::operator T*() const

    Cast operator; implements pointer semantics. Because of this
    function you can pass a QPointer\<T\> to a function where a T*
    is required.
*/

/*!
    \fn template <class T> bool operator==(const T *o, const QPointer<T> &p)
    \relates QPointer

    Equality operator. Returns \c true if \a o and the guarded
    pointer \a p are pointing to the same object, otherwise
    returns \c false.

*/
/*!
    \fn template <class T> bool operator==(const QPointer<T> &p, const T *o)
    \relates QPointer

    Equality operator. Returns \c true if \a o and the guarded
    pointer \a p are pointing to the same object, otherwise
    returns \c false.

*/
/*!
    \fn template <class T> bool operator==(T *o, const QPointer<T> &p)
    \relates QPointer

    Equality operator. Returns \c true if \a o and the guarded
    pointer \a p are pointing to the same object, otherwise
    returns \c false.

*/
/*!
    \fn template <class T> bool operator==(const QPointer<T> &p, T *o)
    \relates QPointer

    Equality operator. Returns \c true if \a o and the guarded
    pointer \a p are pointing to the same object, otherwise
    returns \c false.

*/
/*!
    \fn template <class T> bool operator==(const QPointer<T> &p1, const QPointer<T> &p2)
    \relates QPointer

    Equality operator. Returns \c true if the guarded pointers \a p1 and \a p2
    are pointing to the same object, otherwise
    returns \c false.

*/


/*!
    \fn template <class T> bool operator!=(const T *o, const QPointer<T> &p)
    \relates QPointer

    Inequality operator. Returns \c true if \a o and the guarded
    pointer \a p are not pointing to the same object, otherwise
    returns \c false.
*/
/*!
    \fn template <class T> bool operator!=(const QPointer<T> &p, const T *o)
    \relates QPointer

    Inequality operator. Returns \c true if \a o and the guarded
    pointer \a p are not pointing to the same object, otherwise
    returns \c false.
*/
/*!
    \fn template <class T> bool operator!=(T *o, const QPointer<T> &p)
    \relates QPointer

    Inequality operator. Returns \c true if \a o and the guarded
    pointer \a p are not pointing to the same object, otherwise
    returns \c false.
*/
/*!
    \fn template <class T> bool operator!=(const QPointer<T> &p, T *o)
    \relates QPointer

    Inequality operator. Returns \c true if \a o and the guarded
    pointer \a p are not pointing to the same object, otherwise
    returns \c false.
*/
/*!
    \fn template <class T> bool operator!=(const QPointer<T> &p1, const QPointer<T> &p2)
    \relates QPointer

    Inequality operator. Returns \c true if  the guarded pointers \a p1 and
    \a p2 are not pointing to the same object, otherwise
    returns \c false.
*/
/*!
    \fn template <typename T> QPointer<T> qPointerFromVariant(const QVariant &variant)

    \internal

    Returns a guarded pointer that points to the same object that
    \a variant holds.
*/
