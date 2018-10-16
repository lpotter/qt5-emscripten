/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Layouts module of the Qt Toolkit.
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

#include "qlayoutpolicy_p.h"
#include <QtCore/qdebug.h>
#include <QtCore/qdatastream.h>

QT_BEGIN_NAMESPACE

void QLayoutPolicy::setControlType(ControlType type)
{
    /*
        The control type is a flag type, with values 0x1, 0x2, 0x4, 0x8, 0x10,
        etc. In memory, we pack it onto the available bits (CTSize) in
        setControlType(), and unpack it here.

        Example:

            0x00000001 maps to 0
            0x00000002 maps to 1
            0x00000004 maps to 2
            0x00000008 maps to 3
            etc.
    */

    int i = 0;
    while (true) {
        if (type & (0x1 << i)) {
            bits.ctype = i;
            return;
        }
        ++i;
    }
}

QLayoutPolicy::ControlType QLayoutPolicy::controlType() const
{
    return QLayoutPolicy::ControlType(1 << bits.ctype);
}

#ifndef QT_NO_DATASTREAM

/*!
    \relates QLayoutPolicy

    Writes the size \a policy to the data stream \a stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream &operator<<(QDataStream &stream, const QLayoutPolicy &policy)
{
    // The order here is for historical reasons. (compatibility with Qt4)
    quint32 data = (policy.bits.horPolicy |         // [0, 3]
                    policy.bits.verPolicy << 4 |    // [4, 7]
                    policy.bits.hfw << 8 |          // [8]
                    policy.bits.ctype << 9 |        // [9, 13]
                    policy.bits.wfh << 14 |         // [14]
                  //policy.bits.padding << 15 |     // [15]
                    policy.bits.verStretch << 16 |  // [16, 23]
                    policy.bits.horStretch << 24);  // [24, 31]
    return stream << data;
}

#define VALUE_OF_BITS(data, bitstart, bitcount) ((data >> bitstart) & ((1 << bitcount) -1))

/*!
    \relates QLayoutPolicy

    Reads the size \a policy from the data stream \a stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream &operator>>(QDataStream &stream, QLayoutPolicy &policy)
{
    quint32 data;
    stream >> data;
    policy.bits.horPolicy =  VALUE_OF_BITS(data, 0, 4);
    policy.bits.verPolicy =  VALUE_OF_BITS(data, 4, 4);
    policy.bits.hfw =        VALUE_OF_BITS(data, 8, 1);
    policy.bits.ctype =      VALUE_OF_BITS(data, 9, 5);
    policy.bits.wfh =        VALUE_OF_BITS(data, 14, 1);
    policy.bits.padding =   0;
    policy.bits.verStretch = VALUE_OF_BITS(data, 16, 8);
    policy.bits.horStretch = VALUE_OF_BITS(data, 24, 8);
    return stream;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QLayoutPolicy &p)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QLayoutPolicy(horizontalPolicy = " << p.horizontalPolicy()
                  << ", verticalPolicy = " << p.verticalPolicy() << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE
