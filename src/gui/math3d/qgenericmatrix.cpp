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

#include "qgenericmatrix.h"

QT_BEGIN_NAMESPACE

/*!
    \class QGenericMatrix
    \brief The QGenericMatrix class is a template class that represents a NxM transformation matrix with N columns and M rows.
    \since 4.6
    \ingroup painting
    \ingroup painting-3D
    \inmodule QtGui

    The QGenericMatrix template has three parameters:

    \table
    \row \li N \li Number of columns.
    \row \li M \li Number of rows.
    \row \li T \li Element type that is visible to users of the class.
    \endtable

    \sa QMatrix4x4
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>::QGenericMatrix()

    Constructs a NxM identity matrix.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>::QGenericMatrix(Qt::Initialization)
    \since 5.5
    \internal

    Constructs a NxM matrix without initializing the contents.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>::QGenericMatrix(const T *values)

    Constructs a matrix from the given N * M floating-point \a values.
    The contents of the array \a values is assumed to be in
    row-major order.

    \sa copyDataTo()
*/

/*!
    \fn template <int N, int M, typename T> const T& QGenericMatrix<N, M, T>::operator()(int row, int column) const

    Returns a constant reference to the element at position
    (\a row, \a column) in this matrix.
*/

/*!
    \fn template <int N, int M, typename T> T& QGenericMatrix<N, M, T>::operator()(int row, int column)

    Returns a reference to the element at position (\a row, \a column)
    in this matrix so that the element can be assigned to.
*/

/*!
    \fn template <int N, int M, typename T> bool QGenericMatrix<N, M, T>::isIdentity() const

    Returns \c true if this matrix is the identity; false otherwise.

    \sa setToIdentity()
*/

/*!
    \fn template <int N, int M, typename T> void QGenericMatrix<N, M, T>::setToIdentity()

    Sets this matrix to the identity.

    \sa isIdentity()
*/

/*!
    \fn template <int N, int M, typename T> void QGenericMatrix<N, M, T>::fill(T value)

    Fills all elements of this matrix with \a value.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<M, N> QGenericMatrix<N, M, T>::transposed() const

    Returns this matrix, transposed about its diagonal.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>& QGenericMatrix<N, M, T>::operator+=(const QGenericMatrix<N, M, T>& other)

    Adds the contents of \a other to this matrix.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>& QGenericMatrix<N, M, T>::operator-=(const QGenericMatrix<N, M, T>& other)

    Subtracts the contents of \a other from this matrix.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>& QGenericMatrix<N, M, T>::operator*=(T factor)

    Multiplies all elements of this matrix by \a factor.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T>& QGenericMatrix<N, M, T>::operator/=(T divisor)

    Divides all elements of this matrix by \a divisor.
*/

/*!
    \fn template <int N, int M, typename T> bool QGenericMatrix<N, M, T>::operator==(const QGenericMatrix<N, M, T>& other) const

    Returns \c true if this matrix is identical to \a other; false otherwise.
*/

/*!
    \fn template <int N, int M, typename T> bool QGenericMatrix<N, M, T>::operator!=(const QGenericMatrix<N, M, T>& other) const

    Returns \c true if this matrix is not identical to \a other; false otherwise.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator+(const QGenericMatrix<N, M, T>& m1, const QGenericMatrix<N, M, T>& m2)
    \relates QGenericMatrix

    Returns the sum of \a m1 and \a m2.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator-(const QGenericMatrix<N, M, T>& m1, const QGenericMatrix<N, M, T>& m2)
    \relates QGenericMatrix

    Returns the difference of \a m1 and \a m2.
*/

/*!
    \fn template<int NN, int M1, int M2, typename TT> QGenericMatrix<M1, M2, TT> operator*(const QGenericMatrix<NN, M2, TT>& m1, const QGenericMatrix<M1, NN, TT>& m2)
    \relates QGenericMatrix

    Returns the product of the NNxM2 matrix \a m1 and the M1xNN matrix \a m2
    to produce a M1xM2 matrix result.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator-(const QGenericMatrix<N, M, T>& matrix)
    \overload
    \relates QGenericMatrix

    Returns the negation of \a matrix.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator*(T factor, const QGenericMatrix<N, M, T>& matrix)
    \relates QGenericMatrix

    Returns the result of multiplying all elements of \a matrix by \a factor.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator*(const QGenericMatrix<N, M, T>& matrix, T factor)
    \relates QGenericMatrix

    Returns the result of multiplying all elements of \a matrix by \a factor.
*/

/*!
    \fn template <int N, int M, typename T> QGenericMatrix<N, M, T> operator/(const QGenericMatrix<N, M, T>& matrix, T divisor)
    \relates QGenericMatrix

    Returns the result of dividing all elements of \a matrix by \a divisor.
*/

/*!
    \fn template <int N, int M, typename T> void QGenericMatrix<N, M, T>::copyDataTo(T *values) const

    Retrieves the N * M items in this matrix and copies them to \a values
    in row-major order.
*/

/*!
    \fn template <int N, int M, typename T> T *QGenericMatrix<N, M, T>::data()

    Returns a pointer to the raw data of this matrix.

    \sa constData()
*/

/*!
    \fn template <int N, int M, typename T> const T *QGenericMatrix<N, M, T>::data() const

    Returns a constant pointer to the raw data of this matrix.

    \sa constData()
*/

/*!
    \fn template <int N, int M, typename T> const T *QGenericMatrix<N, M, T>::constData() const

    Returns a constant pointer to the raw data of this matrix.

    \sa data()
*/

#ifndef QT_NO_DATASTREAM

/*!
    \fn template <int N, int M, typename T> QDataStream &operator<<(QDataStream &stream, const QGenericMatrix<N, M, T> &matrix)
    \relates QGenericMatrix

    Writes the given \a matrix to the given \a stream and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

/*!
    \fn template <int N, int M, typename T> QDataStream &operator>>(QDataStream &stream, QGenericMatrix<N, M, T> &matrix)
    \relates QGenericMatrix

    Reads a NxM matrix from the given \a stream into the given \a matrix
    and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

#endif

/*!
    \typedef QMatrix2x2
    \relates QGenericMatrix

    The QMatrix2x2 type defines a convenient instantiation of the
    QGenericMatrix template for 2 columns, 2 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix2x3
    \relates QGenericMatrix

    The QMatrix2x3 type defines a convenient instantiation of the
    QGenericMatrix template for 2 columns, 3 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix2x4
    \relates QGenericMatrix

    The QMatrix2x4 type defines a convenient instantiation of the
    QGenericMatrix template for 2 columns, 4 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix3x2
    \relates QGenericMatrix

    The QMatrix3x2 type defines a convenient instantiation of the
    QGenericMatrix template for 3 columns, 2 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix3x3
    \relates QGenericMatrix

    The QMatrix3x3 type defines a convenient instantiation of the
    QGenericMatrix template for 3 columns, 3 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix3x4
    \relates QGenericMatrix

    The QMatrix3x4 type defines a convenient instantiation of the
    QGenericMatrix template for 3 columns, 4 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix4x2
    \relates QGenericMatrix

    The QMatrix4x2 type defines a convenient instantiation of the
    QGenericMatrix template for 4 columns, 2 rows, and float as
    the element type.
*/

/*!
    \typedef QMatrix4x3
    \relates QGenericMatrix

    The QMatrix4x3 type defines a convenient instantiation of the
    QGenericMatrix template for 4 columns, 3 rows, and float as
    the element type.
*/

QT_END_NAMESPACE
