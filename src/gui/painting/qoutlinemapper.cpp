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

#include "qoutlinemapper_p.h"

#include "qbezier_p.h"
#include "qmath.h"
#include "qpainterpath_p.h"

#include <stdlib.h>

QT_BEGIN_NAMESPACE

#define qreal_to_fixed_26_6(f) (qRound(f * 64))




static const QRectF boundingRect(const QPointF *points, int pointCount)
{
    const QPointF *e = points;
    const QPointF *last = points + pointCount;
    qreal minx, maxx, miny, maxy;
    minx = maxx = e->x();
    miny = maxy = e->y();
    while (++e < last) {
        if (e->x() < minx)
            minx = e->x();
        else if (e->x() > maxx)
            maxx = e->x();
        if (e->y() < miny)
            miny = e->y();
        else if (e->y() > maxy)
            maxy = e->y();
    }
    return QRectF(QPointF(minx, miny), QPointF(maxx, maxy));
}

void QOutlineMapper::curveTo(const QPointF &cp1, const QPointF &cp2, const QPointF &ep) {
#ifdef QT_DEBUG_CONVERT
    printf("QOutlineMapper::curveTo() (%f, %f)\n", ep.x(), ep.y());
#endif

    QBezier bezier = QBezier::fromPoints(m_elements.last(), cp1, cp2, ep);

    bool outsideClip = false;
    // Test one point first before doing a full intersection test.
    if (!QRectF(m_clip_rect).contains(m_transform.map(ep))) {
        QRectF potentialCurveArea = m_transform.mapRect(bezier.bounds());
        outsideClip = !potentialCurveArea.intersects(m_clip_rect);
    }
    if (outsideClip) {
        // The curve is entirely outside the clip rect, so just
        // approximate it with a line that closes the path.
        lineTo(ep);
    } else {
        bezier.addToPolygon(m_elements, m_curve_threshold);
        m_element_types.reserve(m_elements.size());
        for (int i = m_elements.size() - m_element_types.size(); i; --i)
            m_element_types << QPainterPath::LineToElement;
    }
    Q_ASSERT(m_elements.size() == m_element_types.size());
}


QT_FT_Outline *QOutlineMapper::convertPath(const QPainterPath &path)
{
    Q_ASSERT(!path.isEmpty());
    int elmCount = path.elementCount();
#ifdef QT_DEBUG_CONVERT
    printf("QOutlineMapper::convertPath(), size=%d\n", elmCount);
#endif
    beginOutline(path.fillRule());

    for (int index=0; index<elmCount; ++index) {
        const QPainterPath::Element &elm = path.elementAt(index);

        switch (elm.type) {

        case QPainterPath::MoveToElement:
            if (index == elmCount - 1)
                continue;
            moveTo(elm);
            break;

        case QPainterPath::LineToElement:
            lineTo(elm);
            break;

        case QPainterPath::CurveToElement:
            curveTo(elm, path.elementAt(index + 1), path.elementAt(index + 2));
            index += 2;
            break;

        default:
            break; // This will never hit..
        }
    }

    endOutline();
    return outline();
}

QT_FT_Outline *QOutlineMapper::convertPath(const QVectorPath &path)
{
    int count = path.elementCount();

#ifdef QT_DEBUG_CONVERT
    printf("QOutlineMapper::convertPath(VP), size=%d\n", count);
#endif
    beginOutline(path.hasWindingFill() ? Qt::WindingFill : Qt::OddEvenFill);

    if (path.elements()) {
        // TODO: if we do closing of subpaths in convertElements instead we
        // could avoid this loop
        const QPainterPath::ElementType *elements = path.elements();
        const QPointF *points = reinterpret_cast<const QPointF *>(path.points());

        for (int index = 0; index < count; ++index) {
            switch (elements[index]) {
                case QPainterPath::MoveToElement:
                    if (index == count - 1)
                        continue;
                    moveTo(points[index]);
                    break;

                case QPainterPath::LineToElement:
                    lineTo(points[index]);
                    break;

                case QPainterPath::CurveToElement:
                    curveTo(points[index], points[index+1], points[index+2]);
                    index += 2;
                    break;

                default:
                    break; // This will never hit..
            }
        }

    } else {
        // ### We can kill this copying and just use the buffer straight...

        m_elements.resize(count);
        if (count)
            memcpy(static_cast<void *>(m_elements.data()), static_cast<const void *>(path.points()), count* sizeof(QPointF));

        m_element_types.resize(0);
    }

    endOutline();
    return outline();
}


void QOutlineMapper::endOutline()
{
    closeSubpath();

    if (m_elements.isEmpty()) {
        memset(&m_outline, 0, sizeof(m_outline));
        return;
    }

    QPointF *elements = m_elements.data();

    // Transform the outline
    if (m_transform.isIdentity()) {
        // Nothing to do
    } else if (m_transform.type() < QTransform::TxProject) {
        for (int i = 0; i < m_elements.size(); ++i)
            elements[i] = m_transform.map(elements[i]);
    } else {
        const QVectorPath vp((qreal *)elements, m_elements.size(),
                             m_element_types.size() ? m_element_types.data() : 0);
        QPainterPath path = vp.convertToPainterPath();
        path = m_transform.map(path);
        if (!(m_outline.flags & QT_FT_OUTLINE_EVEN_ODD_FILL))
            path.setFillRule(Qt::WindingFill);
        if (path.isEmpty()) {
            m_valid = false;
        } else {
            QTransform oldTransform = m_transform;
            m_transform.reset();
            convertPath(path);
            m_transform = oldTransform;
        }
        return;
    }

    controlPointRect = boundingRect(elements, m_elements.size());

#ifdef QT_DEBUG_CONVERT
    printf(" - control point rect (%.2f, %.2f) %.2f x %.2f, clip=(%d,%d, %dx%d)\n",
           controlPointRect.x(), controlPointRect.y(),
           controlPointRect.width(), controlPointRect.height(),
           m_clip_rect.x(), m_clip_rect.y(), m_clip_rect.width(), m_clip_rect.height());
#endif


    // Check for out of dev bounds...
    const bool do_clip = !m_in_clip_elements && ((controlPointRect.left() < -QT_RASTER_COORD_LIMIT
                          || controlPointRect.right() > QT_RASTER_COORD_LIMIT
                          || controlPointRect.top() < -QT_RASTER_COORD_LIMIT
                          || controlPointRect.bottom() > QT_RASTER_COORD_LIMIT
                          || controlPointRect.width() > QT_RASTER_COORD_LIMIT
                          || controlPointRect.height() > QT_RASTER_COORD_LIMIT));

    if (do_clip) {
        clipElements(elements, elementTypes(), m_elements.size());
    } else {
        convertElements(elements, elementTypes(), m_elements.size());
    }
}

void QOutlineMapper::convertElements(const QPointF *elements,
                                       const QPainterPath::ElementType *types,
                                       int element_count)
{

    if (types) {
        // Translate into FT coords
        const QPointF *e = elements;
        for (int i=0; i<element_count; ++i) {
            switch (*types) {
            case QPainterPath::MoveToElement:
                {
                    QT_FT_Vector pt_fixed = { qreal_to_fixed_26_6(e->x()),
                                              qreal_to_fixed_26_6(e->y()) };
                    if (i != 0)
                        m_contours << m_points.size() - 1;
                    m_points << pt_fixed;
                    m_tags <<  QT_FT_CURVE_TAG_ON;
                }
                break;

            case QPainterPath::LineToElement:
                {
                    QT_FT_Vector pt_fixed = { qreal_to_fixed_26_6(e->x()),
                                              qreal_to_fixed_26_6(e->y()) };
                    m_points << pt_fixed;
                    m_tags << QT_FT_CURVE_TAG_ON;
                }
                break;

            case QPainterPath::CurveToElement:
                {
                    QT_FT_Vector cp1_fixed = { qreal_to_fixed_26_6(e->x()),
                                               qreal_to_fixed_26_6(e->y()) };
                    ++e;
                    QT_FT_Vector cp2_fixed = { qreal_to_fixed_26_6((e)->x()),
                                               qreal_to_fixed_26_6((e)->y()) };
                    ++e;
                    QT_FT_Vector ep_fixed = { qreal_to_fixed_26_6((e)->x()),
                                              qreal_to_fixed_26_6((e)->y()) };

                    m_points << cp1_fixed << cp2_fixed << ep_fixed;
                    m_tags << QT_FT_CURVE_TAG_CUBIC
                           << QT_FT_CURVE_TAG_CUBIC
                           << QT_FT_CURVE_TAG_ON;

                    types += 2;
                    i += 2;
                }
                break;
            default:
                break;
            }
            ++types;
            ++e;
        }
    } else {
        // Plain polygon...
        const QPointF *last = elements + element_count;
        const QPointF *e = elements;
        while (e < last) {
            QT_FT_Vector pt_fixed = { qreal_to_fixed_26_6(e->x()),
                                      qreal_to_fixed_26_6(e->y()) };
            m_points << pt_fixed;
            m_tags << QT_FT_CURVE_TAG_ON;
            ++e;
        }
    }

    // close the very last subpath
    m_contours << m_points.size() - 1;

    m_outline.n_contours = m_contours.size();
    m_outline.n_points = m_points.size();

    m_outline.points = m_points.data();
    m_outline.tags = m_tags.data();
    m_outline.contours = m_contours.data();

#ifdef QT_DEBUG_CONVERT
    printf("QOutlineMapper::endOutline\n");

    printf(" - contours: %d\n", m_outline.n_contours);
    for (int i=0; i<m_outline.n_contours; ++i) {
        printf("   - %d\n", m_outline.contours[i]);
    }

    printf(" - points: %d\n", m_outline.n_points);
    for (int i=0; i<m_outline.n_points; ++i) {
        printf("   - %d -- %.2f, %.2f, (%d, %d)\n", i,
               (double) (m_outline.points[i].x / 64.0),
               (double) (m_outline.points[i].y / 64.0),
               (int) m_outline.points[i].x, (int) m_outline.points[i].y);
    }
#endif
}

void QOutlineMapper::clipElements(const QPointF *elements,
                                    const QPainterPath::ElementType *types,
                                    int element_count)
{
    // We could save a bit of time by actually implementing them fully
    // instead of going through convenience functionallity, but since
    // this part of code hardly every used, it shouldn't matter.

    m_in_clip_elements = true;

    QPainterPath path;

    if (!(m_outline.flags & QT_FT_OUTLINE_EVEN_ODD_FILL))
        path.setFillRule(Qt::WindingFill);

    if (types) {
        for (int i=0; i<element_count; ++i) {
            switch (types[i]) {
            case QPainterPath::MoveToElement:
                path.moveTo(elements[i]);
                break;

            case QPainterPath::LineToElement:
                path.lineTo(elements[i]);
                break;

            case QPainterPath::CurveToElement:
                path.cubicTo(elements[i], elements[i+1], elements[i+2]);
                i += 2;
                break;
            default:
                break;
            }
        }
    } else {
        path.moveTo(elements[0]);
        for (int i=1; i<element_count; ++i)
            path.lineTo(elements[i]);
    }

    QPainterPath clipPath;
    clipPath.addRect(m_clip_rect);
    QPainterPath clippedPath = path.intersected(clipPath);
    if (clippedPath.isEmpty()) {
        m_valid = false;
    } else {
        QTransform oldTransform = m_transform;
        m_transform.reset();
        convertPath(clippedPath);
        m_transform = oldTransform;
    }

    m_in_clip_elements = false;
}

QT_END_NAMESPACE
