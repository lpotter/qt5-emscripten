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

#include "qplatformdefs.h"

#include <stdlib.h>
#include <string.h>

/*
    Define the container allocation functions in a separate file, so that our
    users can easily override them.
*/

QT_BEGIN_NAMESPACE

#if !QT_DEPRECATED_SINCE(5, 0)
// Make sure they're defined to be exported
Q_CORE_EXPORT void *qMalloc(size_t size) Q_ALLOC_SIZE(1);
Q_CORE_EXPORT void qFree(void *ptr);
Q_CORE_EXPORT void *qRealloc(void *ptr, size_t size) Q_ALLOC_SIZE(2);
#endif


void *qMalloc(size_t size)
{
    return ::malloc(size);
}

void qFree(void *ptr)
{
    ::free(ptr);
}

void *qRealloc(void *ptr, size_t size)
{
    return ::realloc(ptr, size);
}

void *qMallocAligned(size_t size, size_t alignment)
{
    return qReallocAligned(0, size, 0, alignment);
}

void *qReallocAligned(void *oldptr, size_t newsize, size_t oldsize, size_t alignment)
{
    // fake an aligned allocation
    void *actualptr = oldptr ? static_cast<void **>(oldptr)[-1] : 0;
    if (alignment <= sizeof(void*)) {
        // special, fast case
        void **newptr = static_cast<void **>(realloc(actualptr, newsize + sizeof(void*)));
        if (!newptr)
            return 0;
        if (newptr == actualptr) {
            // realloc succeeded without reallocating
            return oldptr;
        }

        *newptr = newptr;
        return newptr + 1;
    }

    // malloc returns pointers aligned at least at sizeof(size_t) boundaries
    // but usually more (8- or 16-byte boundaries).
    // So we overallocate by alignment-sizeof(size_t) bytes, so we're guaranteed to find a
    // somewhere within the first alignment-sizeof(size_t) that is properly aligned.

    // However, we need to store the actual pointer, so we need to allocate actually size +
    // alignment anyway.

    void *real = realloc(actualptr, newsize + alignment);
    if (!real)
        return 0;

    quintptr faked = reinterpret_cast<quintptr>(real) + alignment;
    faked &= ~(alignment - 1);
    void **faked_ptr = reinterpret_cast<void **>(faked);

    if (oldptr) {
        qptrdiff oldoffset = static_cast<char *>(oldptr) - static_cast<char *>(actualptr);
        qptrdiff newoffset = reinterpret_cast<char *>(faked_ptr) - static_cast<char *>(real);
        if (oldoffset != newoffset)
            memmove(faked_ptr, static_cast<char *>(real) + oldoffset, qMin(oldsize, newsize));
    }

    // now save the value of the real pointer at faked-sizeof(void*)
    // by construction, alignment > sizeof(void*) and is a power of 2, so
    // faked-sizeof(void*) is properly aligned for a pointer
    faked_ptr[-1] = real;

    return faked_ptr;
}

void qFreeAligned(void *ptr)
{
    if (!ptr)
        return;
    void **ptr2 = static_cast<void **>(ptr);
    free(ptr2[-1]);
}

QT_END_NAMESPACE

