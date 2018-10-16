/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qstylepainter.h"

QT_BEGIN_NAMESPACE

/*!
    \class QStylePainter

    \brief The QStylePainter class is a convenience class for drawing QStyle
    elements inside a widget.

    \ingroup appearance
    \ingroup painting
    \inmodule QtWidgets

    QStylePainter extends QPainter with a set of high-level \c
    draw...() functions implemented on top of QStyle's API. The
    advantage of using QStylePainter is that the parameter lists get
    considerably shorter. Whereas a QStyle object must be able to
    draw on any widget using any painter (because the application
    normally has one QStyle object shared by all widget), a
    QStylePainter is initialized with a widget, eliminating the need
    to specify the QWidget, the QPainter, and the QStyle for every
    function call.

    Example using QStyle directly:

    \snippet styles/styles.cpp 1

    Example using QStylePainter:

    \snippet styles/styles.cpp 0
    \snippet styles/styles.cpp 4
    \snippet styles/styles.cpp 6

    \sa QStyle, QStyleOption
*/

/*!
    \fn QStylePainter::QStylePainter()

    Constructs a QStylePainter.
*/

/*!
    \fn QStylePainter::QStylePainter(QWidget *widget)

    Construct a QStylePainter using widget \a widget for its paint device.
*/

/*!
    \fn QStylePainter::QStylePainter(QPaintDevice *pd, QWidget *widget)

    Construct a QStylePainter using \a pd for its paint device, and
    attributes from \a widget.
*/


/*!
    \fn bool QStylePainter::begin(QWidget *widget)

    Begin painting operations on the specified \a widget.
    Returns \c true if the painter is ready to use; otherwise returns \c false.

    This is automatically called by the constructor that takes a QWidget.
*/

/*!
    \fn bool QStylePainter::begin(QPaintDevice *pd, QWidget *widget)
    \overload

    Begin painting operations on paint device \a pd as if it was \a
    widget.

    This is automatically called by the constructor that
    takes a QPaintDevice and a QWidget.
*/


/*!
    \fn void QStylePainter::drawPrimitive(QStyle::PrimitiveElement pe, const QStyleOption &option)

    Use the widget's style to draw a primitive element \a pe specified by QStyleOption \a option.

    \sa QStyle::drawPrimitive()
*/

/*!
    \fn void QStylePainter::drawControl(QStyle::ControlElement ce, const QStyleOption &option)

    Use the widget's style to draw a control element \a ce specified by QStyleOption \a option.

    \sa QStyle::drawControl()
*/

/*!
  \fn void QStylePainter::drawComplexControl(QStyle::ComplexControl cc,
                                             const QStyleOptionComplex &option)

    Use the widget's style to draw a complex control \a cc specified by the
    QStyleOptionComplex \a option.

    \sa QStyle::drawComplexControl()
*/

/*!
    \fn void QStylePainter::drawItemText(const QRect &rect, int flags, const QPalette &pal,
                                         bool enabled, const QString &text,
                                         QPalette::ColorRole textRole = QPalette::NoRole)

    Draws the \a text in rectangle \a rect and palette \a pal.
    The text is aligned and wrapped according to \a
    flags.

    The pen color is specified with \a textRole. The \a enabled bool
    indicates whether or not the item is enabled; when reimplementing
    this bool should influence how the item is drawn.

    \sa QStyle::drawItemText(), Qt::Alignment
*/

/*!
    \fn void QStylePainter::drawItemPixmap(const QRect &rect, int flags, const QPixmap &pixmap)

    Draws the \a pixmap in rectangle \a rect.
    The pixmap is aligned according to \a flags.

    \sa QStyle::drawItemPixmap(), Qt::Alignment
*/

/*!
    \fn QStyle *QStylePainter::style() const

    Return the current style used by the QStylePainter.
*/

QT_END_NAMESPACE
