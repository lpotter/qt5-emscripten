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

#include "qharfbuzz_p.h"

#include "qunicodetables_p.h"
#if QT_CONFIG(library)
#include "qlibrary.h"
#endif

QT_USE_NAMESPACE

extern "C" {

void HB_GetUnicodeCharProperties(HB_UChar32 ch, HB_CharCategory *category, int *combiningClass)
{
    const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ch);
    *category = (HB_CharCategory)prop->category;
    *combiningClass = prop->combiningClass;
}

HB_CharCategory HB_GetUnicodeCharCategory(HB_UChar32 ch)
{
    return (HB_CharCategory)QChar::category(ch);
}

int HB_GetUnicodeCharCombiningClass(HB_UChar32 ch)
{
    return QChar::combiningClass(ch);
}

HB_UChar16 HB_GetMirroredChar(HB_UChar16 ch)
{
    return QChar::mirroredChar(ch);
}

void (*HB_Library_Resolve(const char *library, int version, const char *symbol))()
{
#if !QT_CONFIG(library)
    Q_UNUSED(library);
    Q_UNUSED(version);
    Q_UNUSED(symbol);
    return 0;
#else
    return QLibrary::resolve(QLatin1String(library), version, symbol);
#endif
}

} // extern "C"

QT_BEGIN_NAMESPACE

HB_Bool qShapeItem(HB_ShaperItem *item)
{
    return HB_ShapeItem(item);
}

HB_Face qHBNewFace(void *font, HB_GetFontTableFunc tableFunc)
{
    return HB_AllocFace(font, tableFunc);
}

HB_Face qHBLoadFace(HB_Face face)
{
    return HB_LoadFace(face);
}

void qHBFreeFace(HB_Face face)
{
    HB_FreeFace(face);
}

QT_END_NAMESPACE
