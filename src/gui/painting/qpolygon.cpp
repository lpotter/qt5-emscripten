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

#include "qpolygon.h"
#include "qrect.h"
#include "qdatastream.h"
#include "qmatrix.h"
#include "qdebug.h"
#include "qpainterpath.h"
#include "qvariant.h"
#include "qpainterpath_p.h"
#include "qbezier_p.h"

#include <stdarg.h>

QT_BEGIN_NAMESPACE

//same as qt_painterpath_isect_line in qpainterpath.cpp
static void qt_polygon_isect_line(const QPointF &p1, const QPointF &p2, const QPointF &pos,
                                  int *winding)
{
    qreal x1 = p1.x();
    qreal y1 = p1.y();
    qreal x2 = p2.x();
    qreal y2 = p2.y();
    qreal y = pos.y();

    int dir = 1;

    if (qFuzzyCompare(y1, y2)) {
        // ignore horizontal lines according to scan conversion rule
        return;
    } else if (y2 < y1) {
        qreal x_tmp = x2; x2 = x1; x1 = x_tmp;
        qreal y_tmp = y2; y2 = y1; y1 = y_tmp;
        dir = -1;
    }

    if (y >= y1 && y < y2) {
        qreal x = x1 + ((x2 - x1) / (y2 - y1)) * (y - y1);

        // count up the winding number if we're
        if (x<=pos.x()) {
            (*winding) += dir;
        }
    }
}

/*!
    \class QPolygon
    \brief The QPolygon class provides a vector of points using
    integer precision.
    \inmodule QtGui

    \reentrant

    \ingroup painting
    \ingroup shared

    A QPolygon object is a QVector<QPoint>.  The easiest way to add
    points to a QPolygon is to use QVector's streaming operator, as
    illustrated below:

    \snippet polygon/polygon.cpp 0

    In addition to the functions provided by QVector, QPolygon
    provides some point-specific functions.

    Each point in a polygon can be retrieved by passing its index to
    the point() function. To populate the polygon, QPolygon provides
    the setPoint() function to set the point at a given index, the
    setPoints() function to set all the points in the polygon
    (resizing it to the given number of points), and the putPoints()
    function which copies a number of given points into the polygon
    from a specified index (resizing the polygon if necessary).

    QPolygon provides the boundingRect() and translate() functions for
    geometry functions. Use the QMatrix::map() function for more
    general transformations of QPolygons.

    The QPolygon class is \l {Implicit Data Sharing}{implicitly
    shared}.

    \sa QVector, QPolygonF, QLine
*/


/*****************************************************************************
  QPolygon member functions
 *****************************************************************************/

/*!
    \fn QPolygon::QPolygon()

    Constructs a polygon with no points.

    \sa QVector::isEmpty()
*/

/*!
    \fn QPolygon::QPolygon(int size)

    Constructs a polygon of the given \a size. Creates an empty
    polygon if \a size == 0.

    \sa QVector::isEmpty()
*/

/*!
    \fn QPolygon::QPolygon(const QPolygon &polygon)

    Constructs a copy of the given \a polygon.

    \sa setPoints()
*/

/*!
    \fn QPolygon::QPolygon(const QVector<QPoint> &points)

    Constructs a polygon containing the specified \a points.

    \sa setPoints()
*/

/*!
    \fn QPolygon::QPolygon(const QRect &rectangle, bool closed)

    Constructs a polygon from the given \a rectangle.  If \a closed is
    false, the polygon just contains the four points of the rectangle
    ordered clockwise, otherwise the polygon's fifth point is set to
    \a {rectangle}.topLeft().

    Note that the bottom-right corner of the rectangle is located at
    (rectangle.x() + rectangle.width(), rectangle.y() +
    rectangle.height()).

    \sa setPoints()
*/

QPolygon::QPolygon(const QRect &r, bool closed)
{
    reserve(closed ? 5 : 4);
    *this << QPoint(r.x(), r.y())
          << QPoint(r.x() + r.width(), r.y())
          << QPoint(r.x() + r.width(), r.y() + r.height())
          << QPoint(r.x(), r.y() + r.height());
    if (closed)
        *this << QPoint(r.left(), r.top());
}

/*!
    \internal
    Constructs a point array with \a nPoints points, taken from the
    \a points array.

    Equivalent to setPoints(nPoints, points).
*/

QPolygon::QPolygon(int nPoints, const int *points)
{
    setPoints(nPoints, points);
}


/*!
    \fn QPolygon::~QPolygon()

    Destroys the polygon.
*/


/*!
    Translates all points in the polygon by (\a{dx}, \a{dy}).

    \sa translated()
*/

void QPolygon::translate(int dx, int dy)
{
    if (dx == 0 && dy == 0)
        return;

    QPoint *p = data();
    int i = size();
    QPoint pt(dx, dy);
    while (i--) {
        *p += pt;
        ++p;
    }
}

/*!
    \fn void QPolygon::translate(const QPoint &offset)
    \overload

    Translates all points in the polygon by the given \a offset.

    \sa translated()
*/

/*!
    Returns a copy of the polygon that is translated by (\a{dx}, \a{dy}).

    \since 4.6
    \sa translate()
*/
QPolygon QPolygon::translated(int dx, int dy) const
{
    QPolygon copy(*this);
    copy.translate(dx, dy);
    return copy;
}

/*!
    \fn void QPolygon::translated(const QPoint &offset) const
    \overload
    \since 4.6

    Returns a copy of the polygon that is translated by the given \a offset.

    \sa translate()
*/

/*!
    Extracts the coordinates of the point at the given \a index to
    *\a{x} and *\a{y} (if they are valid pointers).

    \sa setPoint()
*/

void QPolygon::point(int index, int *x, int *y) const
{
    QPoint p = at(index);
    if (x)
        *x = (int)p.x();
    if (y)
        *y = (int)p.y();
}

/*!
    \fn QPoint QPolygon::point(int index) const
    \overload

    Returns the point at the given \a index.
*/

/*!
    \fn void QPolygon::setPoint(int index, const QPoint &point)
    \overload

    Sets the point at the given \a index to the given \a point.
*/

/*!
    \fn void QPolygon::setPoint(int index, int x, int y)

    Sets the point at the given \a index to the point specified by
    (\a{x}, \a{y}).

    \sa point(), putPoints(), setPoints(),
*/

/*!
    Resizes the polygon to \a nPoints and populates it with the given
    \a points.

    The example code creates a polygon with two points (10, 20) and
    (30, 40):

    \snippet polygon/polygon.cpp 2

    \sa setPoint(), putPoints()
*/

void QPolygon::setPoints(int nPoints, const int *points)
{
    resize(nPoints);
    int i = 0;
    while (nPoints--) {
        setPoint(i++, *points, *(points+1));
        points += 2;
    }
}

/*!
    \overload

    Resizes the polygon to \a nPoints and populates it with the points
    specified by the variable argument list.  The points are given as a
    sequence of integers, starting with \a firstx then \a firsty, and
    so on.

    The example code creates a polygon with two points (10, 20) and
    (30, 40):

    \snippet polygon/polygon.cpp 3
*/

void QPolygon::setPoints(int nPoints, int firstx, int firsty, ...)
{
    va_list ap;
    resize(nPoints);
    setPoint(0, firstx, firsty);
    int i = 0, x, y;
    va_start(ap, firsty);
    while (--nPoints) {
        x = va_arg(ap, int);
        y = va_arg(ap, int);
        setPoint(++i, x, y);
    }
    va_end(ap);
}

/*!
    \overload
    \internal

    Copies \a nPoints points from the \a points coord array into this
    point array, and resizes the point array if \c{index+nPoints}
    exceeds the size of the array.

    \sa setPoint()
*/

void QPolygon::putPoints(int index, int nPoints, const int *points)
{
    if (index + nPoints > size())
        resize(index + nPoints);
    int i = index;
    while (nPoints--) {
        setPoint(i++, *points, *(points+1));
        points += 2;
    }
}

/*!
    Copies \a nPoints points from the variable argument list into this
    polygon from the given \a index.

    The points are given as a sequence of integers, starting with \a
    firstx then \a firsty, and so on. The polygon is resized if
    \c{index+nPoints} exceeds its current size.

    The example code creates a polygon with three points (4,5), (6,7)
    and (8,9), by expanding the polygon from 1 to 3 points:

    \snippet polygon/polygon.cpp 4

    The following code has the same result, but here the putPoints()
    function overwrites rather than extends:

    \snippet polygon/polygon.cpp 5

    \sa setPoints()
*/

void QPolygon::putPoints(int index, int nPoints, int firstx, int firsty, ...)
{
    va_list ap;
    if (index + nPoints > size())
        resize(index + nPoints);
    if (nPoints <= 0)
        return;
    setPoint(index, firstx, firsty);
    int i = index, x, y;
    va_start(ap, firsty);
    while (--nPoints) {
        x = va_arg(ap, int);
        y = va_arg(ap, int);
        setPoint(++i, x, y);
    }
    va_end(ap);
}


/*!
    \fn void QPolygon::putPoints(int index, int nPoints, const QPolygon &fromPolygon, int fromIndex)
    \overload

    Copies \a nPoints points from the given \a fromIndex ( 0 by
    default) in \a fromPolygon into this polygon, starting at the
    specified \a index. For example:

    \snippet polygon/polygon.cpp 6
*/

void QPolygon::putPoints(int index, int nPoints, const QPolygon & from, int fromIndex)
{
    if (index + nPoints > size())
        resize(index + nPoints);
    if (nPoints <= 0)
        return;
    int n = 0;
    while(n < nPoints) {
        setPoint(index + n, from[fromIndex+n]);
        ++n;
    }
}


/*!
    Returns the bounding rectangle of the polygon, or QRect(0, 0, 0,
    0) if the polygon is empty.

    \sa QVector::isEmpty()
*/

QRect QPolygon::boundingRect() const
{
    const QPoint *pd = constData();
    const QPoint *pe = pd + size();
    if (pd == pe)
        return QRect(0, 0, 0, 0);
    int minx, maxx, miny, maxy;
    minx = maxx = pd->x();
    miny = maxy = pd->y();
    ++pd;
    for (; pd != pe; ++pd) {
        if (pd->x() < minx)
            minx = pd->x();
        else if (pd->x() > maxx)
            maxx = pd->x();
        if (pd->y() < miny)
            miny = pd->y();
        else if (pd->y() > maxy)
            maxy = pd->y();
    }
    return QRect(QPoint(minx,miny), QPoint(maxx,maxy));
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QPolygon &a)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QPolygon(";
    for (int i = 0; i < a.count(); ++i)
        dbg.nospace() << a.at(i);
    dbg.nospace() << ')';
    return dbg;
}
#endif


/*!
    \class QPolygonF
    \brief The QPolygonF class provides a vector of points using
    floating point precision.
    \inmodule QtGui

    \reentrant
    \ingroup painting
    \ingroup shared

    A QPolygonF is a QVector<QPointF>. The easiest way to add points
    to a QPolygonF is to use its streaming operator, as illustrated
    below:

    \snippet polygon/polygon.cpp 1

    In addition to the functions provided by QVector, QPolygonF
    provides the boundingRect() and translate() functions for geometry
    operations. Use the QMatrix::map() function for more general
    transformations of QPolygonFs.

    QPolygonF also provides the isClosed() function to determine
    whether a polygon's start and end points are the same, and the
    toPolygon() function returning an integer precision copy of this
    polygon.

    The QPolygonF class is \l {Implicit Data Sharing}{implicitly
    shared}.

    \sa QVector, QPolygon, QLineF
*/


/*****************************************************************************
  QPolygonF member functions
 *****************************************************************************/

/*!
    \fn QPolygonF::QPolygonF()

    Constructs a polygon with no points.

    \sa QVector::isEmpty()
*/

/*!
    \fn QPolygonF::QPolygonF(int size)

    Constructs a polygon of the given \a size. Creates an empty
    polygon if \a size == 0.

    \sa QVector::isEmpty()
*/

/*!
    \fn QPolygonF::QPolygonF(const QPolygonF &polygon)

    Constructs a copy of the given \a polygon.
*/

/*!
    \fn QPolygonF::QPolygonF(const QVector<QPointF> &points)

    Constructs a polygon containing the specified \a points.
*/

/*!
    \fn QPolygonF::QPolygonF(const QRectF &rectangle)

    Constructs a closed polygon from the specified \a rectangle.

    The polygon contains the four vertices of the rectangle in
    clockwise order starting and ending with the top-left vertex.

    \sa isClosed()
*/

QPolygonF::QPolygonF(const QRectF &r)
{
    reserve(5);
    append(QPointF(r.x(), r.y()));
    append(QPointF(r.x() + r.width(), r.y()));
    append(QPointF(r.x() + r.width(), r.y() + r.height()));
    append(QPointF(r.x(), r.y() + r.height()));
    append(QPointF(r.x(), r.y()));
}

/*!
    \fn QPolygonF::QPolygonF(const QPolygon &polygon)

    Constructs a float based polygon from the specified integer based
    \a polygon.

    \sa toPolygon()
*/

QPolygonF::QPolygonF(const QPolygon &a)
{
    reserve(a.size());
    for (int i=0; i<a.size(); ++i)
        append(a.at(i));
}

/*!
    \fn QPolygonF::~QPolygonF()

    Destroys the polygon.
*/


/*!
    Translate all points in the polygon by the given \a offset.

    \sa translated()
*/

void QPolygonF::translate(const QPointF &offset)
{
    if (offset.isNull())
        return;

    QPointF *p = data();
    int i = size();
    while (i--) {
        *p += offset;
        ++p;
    }
}

/*!
    \fn void QPolygonF::translate(qreal dx, qreal dy)
    \overload

    Translates all points in the polygon by (\a{dx}, \a{dy}).

    \sa translated()
*/

/*!
    Returns a copy of the polygon that is translated by the given \a offset.

    \since 4.6
    \sa translate()
*/
QPolygonF QPolygonF::translated(const QPointF &offset) const
{
    QPolygonF copy(*this);
    copy.translate(offset);
    return copy;
}

/*!
    \fn void QPolygonF::translated(qreal dx, qreal dy) const
    \overload
    \since 4.6

    Returns a copy of the polygon that is translated by (\a{dx}, \a{dy}).

    \sa translate()
*/

/*!
    \fn bool QPolygonF::isClosed() const

    Returns \c true if the polygon is closed; otherwise returns \c false.

    A polygon is said to be closed if its start point and end point are equal.

    \sa QVector::first(), QVector::last()
*/

/*!
    Returns the bounding rectangle of the polygon, or QRectF(0,0,0,0)
    if the polygon is empty.

    \sa QVector::isEmpty()
*/

QRectF QPolygonF::boundingRect() const
{
    const QPointF *pd = constData();
    const QPointF *pe = pd + size();
    if (pd == pe)
        return QRectF(0, 0, 0, 0);
    qreal minx, maxx, miny, maxy;
    minx = maxx = pd->x();
    miny = maxy = pd->y();
    ++pd;
    while (pd != pe) {
        if (pd->x() < minx)
            minx = pd->x();
        else if (pd->x() > maxx)
            maxx = pd->x();
        if (pd->y() < miny)
            miny = pd->y();
        else if (pd->y() > maxy)
            maxy = pd->y();
        ++pd;
    }
    return QRectF(minx,miny, maxx - minx, maxy - miny);
}

/*!
    Creates and returns a QPolygon by converting each QPointF to a
    QPoint.

    \sa QPointF::toPoint()
*/

QPolygon QPolygonF::toPolygon() const
{
    QPolygon a;
    a.reserve(size());
    for (int i=0; i<size(); ++i)
        a.append(at(i).toPoint());
    return a;
}

/*!
    \fn void QPolygon::swap(QPolygon &other)
    \since 4.8

    Swaps polygon \a other with this polygon. This operation is very
    fast and never fails.
*/

/*!
    \fn void QPolygonF::swap(QPolygonF &other)
    \since 4.8

    Swaps polygon \a other with this polygon. This operation is very
    fast and never fails.
*/

/*!
   Returns the polygon as a QVariant
*/
QPolygon::operator QVariant() const
{
    return QVariant(QVariant::Polygon, this);
}

/*****************************************************************************
  QPolygon stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QPolygon &polygon)
    \since 4.4
    \relates QPolygon

    Writes the given \a polygon to the given \a stream, and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &s, const QPolygon &a)
{
    const QVector<QPoint> &v = a;
    return s << v;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QPolygon &polygon)
    \since 4.4
    \relates QPolygon

    Reads a polygon from the given \a stream into the given \a
    polygon, and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &s, QPolygon &a)
{
    QVector<QPoint> &v = a;
    return s >> v;
}
#endif // QT_NO_DATASTREAM

/*****************************************************************************
  QPolygonF stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QPolygonF &polygon)
    \relates QPolygonF

    Writes the given \a polygon to the given \a stream, and returns a
    reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator<<(QDataStream &s, const QPolygonF &a)
{
    quint32 len = a.size();
    uint i;

    s << len;
    for (i = 0; i < len; ++i)
        s << a.at(i);
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QPolygonF &polygon)
    \relates QPolygonF

    Reads a polygon from the given \a stream into the given \a
    polygon, and returns a reference to the stream.

    \sa {Serializing Qt Data Types}
*/

QDataStream &operator>>(QDataStream &s, QPolygonF &a)
{
    quint32 len;
    uint i;

    s >> len;
    a.reserve(a.size() + (int)len);
    QPointF p;
    for (i = 0; i < len; ++i) {
        s >> p;
        a.insert(i, p);
    }
    return s;
}
#endif //QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QPolygonF &a)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QPolygonF(";
    for (int i = 0; i < a.count(); ++i)
        dbg.nospace() << a.at(i);
    dbg.nospace() << ')';
    return dbg;
}
#endif


/*!
    \since 4.3

    \fn bool QPolygonF::containsPoint(const QPointF &point, Qt::FillRule fillRule) const

    Returns \c true if the given \a point is inside the polygon according to
    the specified \a fillRule; otherwise returns \c false.
*/
bool QPolygonF::containsPoint(const QPointF &pt, Qt::FillRule fillRule) const
{
    if (isEmpty())
        return false;

    int winding_number = 0;

    QPointF last_pt = at(0);
    QPointF last_start = at(0);
    for (int i = 1; i < size(); ++i) {
        const QPointF &e = at(i);
        qt_polygon_isect_line(last_pt, e, pt, &winding_number);
        last_pt = e;
    }

    // implicitly close last subpath
    if (last_pt != last_start)
        qt_polygon_isect_line(last_pt, last_start, pt, &winding_number);

    return (fillRule == Qt::WindingFill
            ? (winding_number != 0)
            : ((winding_number % 2) != 0));
}

/*!
    \since 4.3

    \fn bool QPolygon::containsPoint(const QPoint &point, Qt::FillRule fillRule) const
    Returns \c true if the given \a point is inside the polygon according to
    the specified \a fillRule; otherwise returns \c false.
*/
bool QPolygon::containsPoint(const QPoint &pt, Qt::FillRule fillRule) const
{
    if (isEmpty())
        return false;

    int winding_number = 0;

    QPoint last_pt = at(0);
    QPoint last_start = at(0);
    for (int i = 1; i < size(); ++i) {
        const QPoint &e = at(i);
        qt_polygon_isect_line(last_pt, e, pt, &winding_number);
        last_pt = e;
    }

    // implicitly close last subpath
    if (last_pt != last_start)
        qt_polygon_isect_line(last_pt, last_start, pt, &winding_number);

    return (fillRule == Qt::WindingFill
            ? (winding_number != 0)
            : ((winding_number % 2) != 0));
}

/*!
    \since 4.3

    Returns a polygon which is the union of this polygon and \a r.

    Set operations on polygons, will treat the polygons as areas, and
    implicitly close the polygon.

    \sa intersected(), subtracted()
*/

QPolygon QPolygon::united(const QPolygon &r) const
{
    QPainterPath subject; subject.addPolygon(*this);
    QPainterPath clip; clip.addPolygon(r);

    return subject.united(clip).toFillPolygon().toPolygon();
}

/*!
    \since 4.3

    Returns a polygon which is the intersection of this polygon and \a r.

    Set operations on polygons will treat the polygons as
    areas. Non-closed polygons will be treated as implicitly closed.

    \sa intersects()
*/

QPolygon QPolygon::intersected(const QPolygon &r) const
{
    QPainterPath subject; subject.addPolygon(*this);
    QPainterPath clip; clip.addPolygon(r);

    return subject.intersected(clip).toFillPolygon().toPolygon();
}

/*!
    \since 4.3

    Returns a polygon which is \a r subtracted from this polygon.

    Set operations on polygons will treat the polygons as
    areas. Non-closed polygons will be treated as implicitly closed.

*/

QPolygon QPolygon::subtracted(const QPolygon &r) const
{
    QPainterPath subject; subject.addPolygon(*this);
    QPainterPath clip; clip.addPolygon(r);

    return subject.subtracted(clip).toFillPolygon().toPolygon();
}

/*!
    \since 5.10

    Returns \c true if the current polygon intersects at any point the given polygon \a p.
    Also returns \c true if the current polygon contains or is contained by any part of \a p.

    Set operations on polygons will treat the polygons as
    areas. Non-closed polygons will be treated as implicitly closed.

    \sa intersected()
*/

bool QPolygon::intersects(const QPolygon &p) const
{
    QPainterPath subject; subject.addPolygon(*this);
    QPainterPath clip; clip.addPolygon(p);

    return subject.intersects(clip);
}

/*!
    \since 4.3

    Returns a polygon which is the union of this polygon and \a r.

    Set operations on polygons will treat the polygons as
    areas. Non-closed polygons will be treated as implicitly closed.

    \sa intersected(), subtracted()
*/

QPolygonF QPolygonF::united(const QPolygonF &r) const
{
    QPainterPath subject; subject.addPolygon(*this);
    QPainterPath clip; clip.addPolygon(r);

    return subject.united(clip).toFillPolygon();
}

/*!
    \since 4.3

    Returns a polygon which is the intersection of this polygon and \a r.

    Set operations on polygons will treat the polygons as
    areas. Non-closed polygons will be treated as implicitly closed.

    \sa intersects()
*/

QPolygonF QPolygonF::intersected(const QPolygonF &r) const
{
    QPainterPath subject; subject.addPolygon(*this);
    QPainterPath clip; clip.addPolygon(r);

    return subject.intersected(clip).toFillPolygon();
}

/*!
    \since 4.3

    Returns a polygon which is \a r subtracted from this polygon.

    Set operations on polygons will treat the polygons as
    areas. Non-closed polygons will be treated as implicitly closed.

*/

QPolygonF QPolygonF::subtracted(const QPolygonF &r) const
{
    QPainterPath subject; subject.addPolygon(*this);
    QPainterPath clip; clip.addPolygon(r);
    return subject.subtracted(clip).toFillPolygon();
}

/*!
    \since 5.10

    Returns \c true if the current polygon intersects at any point the given polygon \a p.
    Also returns \c true if the current polygon contains or is contained by any part of \a p.

    Set operations on polygons will treat the polygons as
    areas. Non-closed polygons will be treated as implicitly closed.

    \sa intersected()
*/

bool QPolygonF::intersects(const QPolygonF &p) const
{
    QPainterPath subject; subject.addPolygon(*this);
    QPainterPath clip; clip.addPolygon(p);

    return subject.intersects(clip);
}

/*!
   Returns the polygon as a QVariant.
*/

QPolygonF::operator QVariant() const
{
    return QVariant(QMetaType::QPolygonF, this);
}

QT_END_NAMESPACE
