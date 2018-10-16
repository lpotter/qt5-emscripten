/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Copyright (C) 2012 Giuseppe D'Angelo <dangelog@gmail.com>.
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

// for rand_s, _CRT_RAND_S must be #defined before #including stdlib.h.
// put it at the beginning so some indirect inclusion doesn't break it
#ifndef _CRT_RAND_S
#define _CRT_RAND_S
#endif
#include <stdlib.h>

#include "qhash.h"

#ifdef truncate
#undef truncate
#endif

#include <qbitarray.h>
#include <qstring.h>
#include <qglobal.h>
#include <qbytearray.h>
#include <qdatetime.h>
#include <qbasicatomic.h>
#include <qendian.h>
#include <private/qsimd_p.h>

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#include <qrandom.h>
#endif // QT_BOOTSTRAPPED

#include <limits.h>

QT_BEGIN_NAMESPACE

/*
    The Java's hashing algorithm for strings is a variation of D. J. Bernstein
    hashing algorithm appeared here http://cr.yp.to/cdb/cdb.txt
    and informally known as DJB33XX - DJB's 33 Times Xor.
    Java uses DJB31XA, that is, 31 Times Add.

    The original algorithm was a loop around
        (h << 5) + h ^ c
    (which is indeed h*33 ^ c); it was then changed to
        (h << 5) - h ^ c
    (so h*31^c: DJB31XX), and the XOR changed to a sum:
        (h << 5) - h + c
    (DJB31XA), which can save some assembly instructions.

    Still, we can avoid writing the multiplication as "(h << 5) - h"
    -- the compiler will turn it into a shift and an addition anyway
    (for instance, gcc 4.4 does that even at -O0).
*/

#if QT_COMPILER_SUPPORTS_HERE(SSE4_2)
static inline bool hasFastCrc32()
{
    return qCpuHasFeature(SSE4_2);
}

template <typename Char>
QT_FUNCTION_TARGET(SSE4_2)
static uint crc32(const Char *ptr, size_t len, uint h)
{
    // The CRC32 instructions from Nehalem calculate a 32-bit CRC32 checksum
    const uchar *p = reinterpret_cast<const uchar *>(ptr);
    const uchar *const e = p + (len * sizeof(Char));
#  ifdef Q_PROCESSOR_X86_64
    // The 64-bit instruction still calculates only 32-bit, but without this
    // variable GCC 4.9 still tries to clear the high bits on every loop
    qulonglong h2 = h;

    p += 8;
    for ( ; p <= e; p += 8)
        h2 = _mm_crc32_u64(h2, qFromUnaligned<qlonglong>(p - 8));
    h = h2;
    p -= 8;

    len = e - p;
    if (len & 4) {
        h = _mm_crc32_u32(h, qFromUnaligned<uint>(p));
        p += 4;
    }
#  else
    p += 4;
    for ( ; p <= e; p += 4)
        h = _mm_crc32_u32(h, qFromUnaligned<uint>(p - 4));
    p -= 4;
    len = e - p;
#  endif
    if (len & 2) {
        h = _mm_crc32_u16(h, qFromUnaligned<ushort>(p));
        p += 2;
    }
    if (sizeof(Char) == 1 && len & 1)
        h = _mm_crc32_u8(h, *p);
    return h;
}
#elif defined(__ARM_FEATURE_CRC32)
static inline bool hasFastCrc32()
{
    return qCpuHasFeature(CRC32);
}

template <typename Char>
#if defined(Q_PROCESSOR_ARM_64)
QT_FUNCTION_TARGET(CRC32)
#endif
static uint crc32(const Char *ptr, size_t len, uint h)
{
    // The crc32[whbd] instructions on Aarch64/Aarch32 calculate a 32-bit CRC32 checksum
    const uchar *p = reinterpret_cast<const uchar *>(ptr);
    const uchar *const e = p + (len * sizeof(Char));

#ifndef __ARM_FEATURE_UNALIGNED
    if (Q_UNLIKELY(reinterpret_cast<quintptr>(p) & 7)) {
        if ((sizeof(Char) == 1) && (reinterpret_cast<quintptr>(p) & 1) && (e - p > 0)) {
            h = __crc32b(h, *p);
            ++p;
        }
        if ((reinterpret_cast<quintptr>(p) & 2) && (e >= p + 2)) {
            h = __crc32h(h, *reinterpret_cast<const uint16_t *>(p));
            p += 2;
        }
        if ((reinterpret_cast<quintptr>(p) & 4) && (e >= p + 4)) {
            h = __crc32w(h, *reinterpret_cast<const uint32_t *>(p));
            p += 4;
        }
    }
#endif

    for ( ; p + 8 <= e; p += 8)
        h = __crc32d(h, *reinterpret_cast<const uint64_t *>(p));

    len = e - p;
    if (len == 0)
        return h;
    if (len & 4) {
        h = __crc32w(h, *reinterpret_cast<const uint32_t *>(p));
        p += 4;
    }
    if (len & 2) {
        h = __crc32h(h, *reinterpret_cast<const uint16_t *>(p));
        p += 2;
    }
    if (sizeof(Char) == 1 && len & 1)
        h = __crc32b(h, *p);
    return h;
}
#else
static inline bool hasFastCrc32()
{
    return false;
}

static uint crc32(...)
{
    Q_UNREACHABLE();
    return 0;
}
#endif

static inline uint hash(const uchar *p, size_t len, uint seed) Q_DECL_NOTHROW
{
    uint h = seed;

    if (seed && hasFastCrc32())
        return crc32(p, len, h);

    for (size_t i = 0; i < len; ++i)
        h = 31 * h + p[i];

    return h;
}

uint qHashBits(const void *p, size_t len, uint seed) Q_DECL_NOTHROW
{
    return hash(static_cast<const uchar*>(p), int(len), seed);
}

static inline uint hash(const QChar *p, size_t len, uint seed) Q_DECL_NOTHROW
{
    uint h = seed;

    if (seed && hasFastCrc32())
        return crc32(p, len, h);

    for (size_t i = 0; i < len; ++i)
        h = 31 * h + p[i].unicode();

    return h;
}

uint qHash(const QByteArray &key, uint seed) Q_DECL_NOTHROW
{
    return hash(reinterpret_cast<const uchar *>(key.constData()), size_t(key.size()), seed);
}

#if QT_STRINGVIEW_LEVEL < 2
uint qHash(const QString &key, uint seed) Q_DECL_NOTHROW
{
    return hash(key.unicode(), size_t(key.size()), seed);
}

uint qHash(const QStringRef &key, uint seed) Q_DECL_NOTHROW
{
    return hash(key.unicode(), size_t(key.size()), seed);
}
#endif

uint qHash(QStringView key, uint seed) Q_DECL_NOTHROW
{
    return hash(key.data(), key.size(), seed);
}

uint qHash(const QBitArray &bitArray, uint seed) Q_DECL_NOTHROW
{
    int m = bitArray.d.size() - 1;
    uint result = hash(reinterpret_cast<const uchar *>(bitArray.d.constData()),
                       size_t(qMax(0, m)), seed);

    // deal with the last 0 to 7 bits manually, because we can't trust that
    // the padding is initialized to 0 in bitArray.d
    int n = bitArray.size();
    if (n & 0x7)
        result = ((result << 4) + bitArray.d.at(m)) & ((1 << n) - 1);
    return result;
}

uint qHash(QLatin1String key, uint seed) Q_DECL_NOTHROW
{
    return hash(reinterpret_cast<const uchar *>(key.data()), size_t(key.size()), seed);
}

/*!
    \internal

    Creates the QHash random seed from various sources.
    In order of decreasing precedence:
    - under Unix, it attemps to read from /dev/urandom;
    - under Unix, it attemps to read from /dev/random;
    - under Windows, it attempts to use rand_s;
    - as a general fallback, the application's PID, a timestamp and the
      address of a stack-local variable are used.
*/
static uint qt_create_qhash_seed()
{
    uint seed = 0;

#ifndef QT_BOOTSTRAPPED
    QByteArray envSeed = qgetenv("QT_HASH_SEED");
    if (!envSeed.isNull()) {
        uint seed = envSeed.toUInt();
        if (seed) {
            // can't use qWarning here (reentrancy)
            fprintf(stderr, "QT_HASH_SEED: forced seed value is not 0, cannot guarantee that the "
                     "hashing functions will produce a stable value.");
        }
        return seed;
    }

    seed = QRandomGenerator::system()->generate();
#endif // QT_BOOTSTRAPPED

    return seed;
}

/*
    The QHash seed itself.
*/
static QBasicAtomicInt qt_qhash_seed = Q_BASIC_ATOMIC_INITIALIZER(-1);

/*!
    \internal

    Seed == -1 means it that it was not initialized yet.

    We let qt_create_qhash_seed return any unsigned integer,
    but convert it to signed in order to initialize the seed.

    We don't actually care about the fact that different calls to
    qt_create_qhash_seed() might return different values,
    as long as in the end everyone uses the very same value.
*/
static void qt_initialize_qhash_seed()
{
    if (qt_qhash_seed.load() == -1) {
        int x(qt_create_qhash_seed() & INT_MAX);
        qt_qhash_seed.testAndSetRelaxed(-1, x);
    }
}

/*! \relates QHash
    \since 5.6

    Returns the current global QHash seed.

    The seed is set in any newly created QHash. See \l{qHash} about how this seed
    is being used by QHash.

    \sa qSetGlobalQHashSeed
 */
int qGlobalQHashSeed()
{
    qt_initialize_qhash_seed();
    return qt_qhash_seed.load();
}

/*! \relates QHash
    \since 5.6

    Sets the global QHash seed to \a newSeed.

    Manually setting the global QHash seed value should be done only for testing
    and debugging purposes, when deterministic and reproducible behavior on a QHash
    is needed. We discourage to do it in production code as it can make your
    application susceptible to \l{algorithmic complexity attacks}.

    From Qt 5.10 and onwards, the only allowed values are 0 and -1. Passing the
    value -1 will reinitialize the global QHash seed to a random value, while
    the value of 0 is used to request a stable algorithm for C++ primitive
    types types (like \c int) and string types (QString, QByteArray).

    The seed is set in any newly created QHash. See \l{qHash} about how this seed
    is being used by QHash.

    If the environment variable \c QT_HASH_SEED is set, calling this function will
    result in a no-op.

    \sa qGlobalQHashSeed
 */
void qSetGlobalQHashSeed(int newSeed)
{
    if (qEnvironmentVariableIsSet("QT_HASH_SEED"))
        return;
    if (newSeed == -1) {
        int x(qt_create_qhash_seed() & INT_MAX);
        qt_qhash_seed.store(x);
    } else {
        if (newSeed) {
            // can't use qWarning here (reentrancy)
            fprintf(stderr, "qSetGlobalQHashSeed: forced seed value is not 0, cannot guarantee that the "
                            "hashing functions will produce a stable value.");
        }
        qt_qhash_seed.store(newSeed & INT_MAX);
    }
}

/*!
    \internal

    Private copy of the implementation of the Qt 4 qHash algorithm for strings,
    (that is, QChar-based arrays, so all QString-like classes),
    to be used wherever the result is somehow stored or reused across multiple
    Qt versions. The public qHash implementation can change at any time,
    therefore one must not rely on the fact that it will always give the same
    results.

    The qt_hash functions must *never* change their results.

    This function can hash discontiguous memory by invoking it on each chunk,
    passing the previous's result in the next call's \a chained argument.
*/
uint qt_hash(QStringView key, uint chained) Q_DECL_NOTHROW
{
    auto n = key.size();
    auto p = key.utf16();

    uint h = chained;

    while (n--) {
        h = (h << 4) + *p++;
        h ^= (h & 0xf0000000) >> 23;
        h &= 0x0fffffff;
    }
    return h;
}

/*
    The prime_deltas array contains the difference between a power
    of two and the next prime number:

    prime_deltas[i] = nextprime(2^i) - 2^i

    Basically, it's sequence A092131 from OEIS, assuming:
    - nextprime(1) = 1
    - nextprime(2) = 2
    and
    - left-extending it for the offset 0 (A092131 starts at i=1)
    - stopping the sequence at i = 28 (the table is big enough...)
*/

static const uchar prime_deltas[] = {
    0,  0,  1,  3,  1,  5,  3,  3,  1,  9,  7,  5,  3, 17, 27,  3,
    1, 29,  3, 21,  7, 17, 15,  9, 43, 35, 15,  0,  0,  0,  0,  0
};

/*
    The primeForNumBits() function returns the prime associated to a
    power of two. For example, primeForNumBits(8) returns 257.
*/

static inline int primeForNumBits(int numBits)
{
    return (1 << numBits) + prime_deltas[numBits];
}

/*
    Returns the smallest integer n such that
    primeForNumBits(n) >= hint.
*/
static int countBits(int hint)
{
    int numBits = 0;
    int bits = hint;

    while (bits > 1) {
        bits >>= 1;
        numBits++;
    }

    if (numBits >= (int)sizeof(prime_deltas)) {
        numBits = sizeof(prime_deltas) - 1;
    } else if (primeForNumBits(numBits) < hint) {
        ++numBits;
    }
    return numBits;
}

/*
    A QHash has initially around pow(2, MinNumBits) buckets. For
    example, if MinNumBits is 4, it has 17 buckets.
*/
const int MinNumBits = 4;

const QHashData QHashData::shared_null = {
    0, 0, Q_REFCOUNT_INITIALIZE_STATIC, 0, 0, MinNumBits, 0, 0, 0, true, false, 0
};

void *QHashData::allocateNode(int nodeAlign)
{
    void *ptr = strictAlignment ? qMallocAligned(nodeSize, nodeAlign) : malloc(nodeSize);
    Q_CHECK_PTR(ptr);
    return ptr;
}

void QHashData::freeNode(void *node)
{
    if (strictAlignment)
        qFreeAligned(node);
    else
        free(node);
}

QHashData *QHashData::detach_helper(void (*node_duplicate)(Node *, void *),
                                    void (*node_delete)(Node *),
                                    int nodeSize,
                                    int nodeAlign)
{
    union {
        QHashData *d;
        Node *e;
    };
    if (this == &shared_null)
        qt_initialize_qhash_seed(); // may throw
    d = new QHashData;
    d->fakeNext = 0;
    d->buckets = 0;
    d->ref.initializeOwned();
    d->size = size;
    d->nodeSize = nodeSize;
    d->userNumBits = userNumBits;
    d->numBits = numBits;
    d->numBuckets = numBuckets;
    d->seed = (this == &shared_null) ? uint(qt_qhash_seed.load()) : seed;
    d->sharable = true;
    d->strictAlignment = nodeAlign > 8;
    d->reserved = 0;

    if (numBuckets) {
        QT_TRY {
            d->buckets = new Node *[numBuckets];
        } QT_CATCH(...) {
            // restore a consistent state for d
            d->numBuckets = 0;
            // roll back
            d->free_helper(node_delete);
            QT_RETHROW;
        }

        Node *this_e = reinterpret_cast<Node *>(this);
        for (int i = 0; i < numBuckets; ++i) {
            Node **nextNode = &d->buckets[i];
            Node *oldNode = buckets[i];
            while (oldNode != this_e) {
                QT_TRY {
                    Node *dup = static_cast<Node *>(allocateNode(nodeAlign));

                    QT_TRY {
                        node_duplicate(oldNode, dup);
                    } QT_CATCH(...) {
                        freeNode( dup );
                        QT_RETHROW;
                    }

                    *nextNode = dup;
                    nextNode = &dup->next;
                    oldNode = oldNode->next;
                } QT_CATCH(...) {
                    // restore a consistent state for d
                    *nextNode = e;
                    d->numBuckets = i+1;
                    // roll back
                    d->free_helper(node_delete);
                    QT_RETHROW;
                }
            }
            *nextNode = e;
        }
    }
    return d;
}

void QHashData::free_helper(void (*node_delete)(Node *))
{
    if (node_delete) {
        Node *this_e = reinterpret_cast<Node *>(this);
        Node **bucket = reinterpret_cast<Node **>(this->buckets);

        int n = numBuckets;
        while (n--) {
            Node *cur = *bucket++;
            while (cur != this_e) {
                Node *next = cur->next;
                node_delete(cur);
                freeNode(cur);
                cur = next;
            }
        }
    }
    delete [] buckets;
    delete this;
}

QHashData::Node *QHashData::nextNode(Node *node)
{
    union {
        Node *next;
        Node *e;
        QHashData *d;
    };
    next = node->next;
    Q_ASSERT_X(next, "QHash", "Iterating beyond end()");
    if (next->next)
        return next;

    int start = (node->h % d->numBuckets) + 1;
    Node **bucket = d->buckets + start;
    int n = d->numBuckets - start;
    while (n--) {
        if (*bucket != e)
            return *bucket;
        ++bucket;
    }
    return e;
}

QHashData::Node *QHashData::previousNode(Node *node)
{
    union {
        Node *e;
        QHashData *d;
    };

    e = node;
    while (e->next)
        e = e->next;

    int start;
    if (node == e)
        start = d->numBuckets - 1;
    else
        start = node->h % d->numBuckets;

    Node *sentinel = node;
    Node **bucket = d->buckets + start;
    while (start >= 0) {
        if (*bucket != sentinel) {
            Node *prev = *bucket;
            while (prev->next != sentinel)
                prev = prev->next;
            return prev;
        }

        sentinel = e;
        --bucket;
        --start;
    }
    Q_ASSERT_X(start >= 0, "QHash", "Iterating backward beyond begin()");
    return e;
}

/*
    If hint is negative, -hint gives the approximate number of
    buckets that should be used for the hash table. If hint is
    nonnegative, (1 << hint) gives the approximate number
    of buckets that should be used.
*/
void QHashData::rehash(int hint)
{
    if (hint < 0) {
        hint = countBits(-hint);
        if (hint < MinNumBits)
            hint = MinNumBits;
        userNumBits = hint;
        while (primeForNumBits(hint) < (size >> 1))
            ++hint;
    } else if (hint < MinNumBits) {
        hint = MinNumBits;
    }

    if (numBits != hint) {
        Node *e = reinterpret_cast<Node *>(this);
        Node **oldBuckets = buckets;
        int oldNumBuckets = numBuckets;

        int nb = primeForNumBits(hint);
        buckets = new Node *[nb];
        numBits = hint;
        numBuckets = nb;
        for (int i = 0; i < numBuckets; ++i)
            buckets[i] = e;

        for (int i = 0; i < oldNumBuckets; ++i) {
            Node *firstNode = oldBuckets[i];
            while (firstNode != e) {
                uint h = firstNode->h;
                Node *lastNode = firstNode;
                while (lastNode->next != e && lastNode->next->h == h)
                    lastNode = lastNode->next;

                Node *afterLastNode = lastNode->next;
                Node **beforeFirstNode = &buckets[h % numBuckets];
                while (*beforeFirstNode != e)
                    beforeFirstNode = &(*beforeFirstNode)->next;
                lastNode->next = *beforeFirstNode;
                *beforeFirstNode = firstNode;
                firstNode = afterLastNode;
            }
        }
        delete [] oldBuckets;
    }
}

#ifdef QT_QHASH_DEBUG

void QHashData::dump()
{
    qDebug("Hash data (ref = %d, size = %d, nodeSize = %d, userNumBits = %d, numBits = %d, numBuckets = %d)",
            int(ref), size, nodeSize, userNumBits, numBits,
            numBuckets);
    qDebug("    %p (fakeNode = %p)", this, fakeNext);
    for (int i = 0; i < numBuckets; ++i) {
        Node *n = buckets[i];
        if (n != reinterpret_cast<Node *>(this)) {
            QString line = QString::asprintf("%d:", i);
            while (n != reinterpret_cast<Node *>(this)) {
                line += QString::asprintf(" -> [%p]", n);
                if (!n) {
                    line += " (CORRUPT)";
                    break;
                }
                n = n->next;
            }
            qDebug("%s", qPrintable(line));
        }
    }
}

void QHashData::checkSanity()
{
    if (Q_UNLIKELY(fakeNext))
        qFatal("Fake next isn't 0");

    for (int i = 0; i < numBuckets; ++i) {
        Node *n = buckets[i];
        Node *p = n;
        if (Q_UNLIKELY(!n))
            qFatal("%d: Bucket entry is 0", i);
        if (n != reinterpret_cast<Node *>(this)) {
            while (n != reinterpret_cast<Node *>(this)) {
                if (Q_UNLIKELY(!n->next))
                    qFatal("%d: Next of %p is 0, should be %p", i, n, this);
                n = n->next;
            }
        }
    }
}
#endif

/*!
    \fn template <typename T1, typename T2> uint qHash(const QPair<T1, T2> &key, uint seed = 0)
    \since 5.0
    \relates QHash

    Returns the hash value for the \a key, using \a seed to seed the calculation.

    Types \c T1 and \c T2 must be supported by qHash().
*/

/*!
    \fn template <typename T1, typename T2> uint qHash(const std::pair<T1, T2> &key, uint seed = 0)
    \since 5.7
    \relates QHash

    Returns the hash value for the \a key, using \a seed to seed the calculation.

    Types \c T1 and \c T2 must be supported by qHash().

    \note The return type of this function is \e{not} the same as that of
    \code
    qHash(qMakePair(key.first, key.second), seed);
    \endcode
    The two functions use different hashing algorithms; due to binary compatibility
    constraints, we cannot change the QPair algorithm to match the std::pair one before Qt 6.
*/

/*! \fn template <typename InputIterator> uint qHashRange(InputIterator first, InputIterator last, uint seed = 0)
    \relates QHash
    \since 5.5

    Returns the hash value for the range [\a{first},\a{last}), using \a seed
    to seed the calculation, by successively applying qHash() to each
    element and combining the hash values into a single one.

    The return value of this function depends on the order of elements
    in the range. That means that

    \code
    {0, 1, 2}
    \endcode

    and
    \code
    {1, 2, 0}
    \endcode

    hash to \b{different} values. If order does not matter, for example for hash
    tables, use qHashRangeCommutative() instead. If you are hashing raw
    memory, use qHashBits().

    Use this function only to implement qHash() for your own custom
    types. For example, here's how you could implement a qHash() overload for
    std::vector<int>:

    \snippet code/src_corelib_tools_qhash.cpp qhashrange

    It bears repeating that the implementation of qHashRange() - like
    the qHash() overloads offered by Qt - may change at any time. You
    \b{must not} rely on the fact that qHashRange() will give the same
    results (for the same inputs) across different Qt versions, even
    if qHash() for the element type would.

    \sa qHashBits(), qHashRangeCommutative()
*/

/*! \fn template <typename InputIterator> uint qHashRangeCommutative(InputIterator first, InputIterator last, uint seed = 0)
    \relates QHash
    \since 5.5

    Returns the hash value for the range [\a{first},\a{last}), using \a seed
    to seed the calculation, by successively applying qHash() to each
    element and combining the hash values into a single one.

    The return value of this function does not depend on the order of
    elements in the range. That means that

    \code
    {0, 1, 2}
    \endcode

    and
    \code
    {1, 2, 0}
    \endcode

    hash to the \b{same} values. If order matters, for example, for vectors
    and arrays, use qHashRange() instead. If you are hashing raw
    memory, use qHashBits().

    Use this function only to implement qHash() for your own custom
    types. For example, here's how you could implement a qHash() overload for
    std::unordered_set<int>:

    \snippet code/src_corelib_tools_qhash.cpp qhashrangecommutative

    It bears repeating that the implementation of
    qHashRangeCommutative() - like the qHash() overloads offered by Qt
    - may change at any time. You \b{must not} rely on the fact that
    qHashRangeCommutative() will give the same results (for the same
    inputs) across different Qt versions, even if qHash() for the
    element type would.

    \sa qHashBits(), qHashRange()
*/

/*! \fn uint qHashBits(const void *p, size_t len, uint seed = 0)
    \relates QHash
    \since 5.4

    Returns the hash value for the memory block of size \a len pointed
    to by \a p, using \a seed to seed the calculation.

    Use this function only to implement qHash() for your own custom
    types. For example, here's how you could implement a qHash() overload for
    std::vector<int>:

    \snippet code/src_corelib_tools_qhash.cpp qhashbits

    This takes advantage of the fact that std::vector lays out its data
    contiguously. If that is not the case, or the contained type has
    padding, you should use qHashRange() instead.

    It bears repeating that the implementation of qHashBits() - like
    the qHash() overloads offered by Qt - may change at any time. You
    \b{must not} rely on the fact that qHashBits() will give the same
    results (for the same inputs) across different Qt versions.

    \sa qHashRange(), qHashRangeCommutative()
*/

/*! \fn uint qHash(char key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(uchar key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(signed char key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(ushort key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(short key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(uint key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(int key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(ulong key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(long key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(quint64 key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(qint64 key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \relates QHash
    \since 5.3

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
uint qHash(float key, uint seed) Q_DECL_NOTHROW
{
    return key != 0.0f ? hash(reinterpret_cast<const uchar *>(&key), sizeof(key), seed) : seed ;
}

/*! \relates QHash
    \since 5.3

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
uint qHash(double key, uint seed) Q_DECL_NOTHROW
{
    return key != 0.0  ? hash(reinterpret_cast<const uchar *>(&key), sizeof(key), seed) : seed ;
}

#if !defined(Q_OS_DARWIN) || defined(Q_CLANG_QDOC)
/*! \relates QHash
    \since 5.3

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/
uint qHash(long double key, uint seed) Q_DECL_NOTHROW
{
    return key != 0.0L ? hash(reinterpret_cast<const uchar *>(&key), sizeof(key), seed) : seed ;
}
#endif

/*! \fn uint qHash(const QChar key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(const QByteArray &key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(const QBitArray &key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(const QString &key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(const QStringRef &key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(QStringView key, uint seed = 0)
    \relates QStringView
    \since 5.10

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn uint qHash(QLatin1String key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*! \fn template <class T> uint qHash(const T *key, uint seed = 0)
    \relates QHash
    \since 5.0

    Returns the hash value for the \a key, using \a seed to seed the calculation.
*/

/*!
    \class QHash
    \inmodule QtCore
    \brief The QHash class is a template class that provides a hash-table-based dictionary.

    \ingroup tools
    \ingroup shared

    \reentrant

    QHash\<Key, T\> is one of Qt's generic \l{container classes}. It
    stores (key, value) pairs and provides very fast lookup of the
    value associated with a key.

    QHash provides very similar functionality to QMap. The
    differences are:

    \list
    \li QHash provides faster lookups than QMap. (See \l{Algorithmic
       Complexity} for details.)
    \li When iterating over a QMap, the items are always sorted by
       key. With QHash, the items are arbitrarily ordered.
    \li The key type of a QMap must provide operator<(). The key
       type of a QHash must provide operator==() and a global
       hash function called qHash() (see \l{qHash}).
    \endlist

    Here's an example QHash with QString keys and \c int values:
    \snippet code/src_corelib_tools_qhash.cpp 0

    To insert a (key, value) pair into the hash, you can use operator[]():

    \snippet code/src_corelib_tools_qhash.cpp 1

    This inserts the following three (key, value) pairs into the
    QHash: ("one", 1), ("three", 3), and ("seven", 7). Another way to
    insert items into the hash is to use insert():

    \snippet code/src_corelib_tools_qhash.cpp 2

    To look up a value, use operator[]() or value():

    \snippet code/src_corelib_tools_qhash.cpp 3

    If there is no item with the specified key in the hash, these
    functions return a \l{default-constructed value}.

    If you want to check whether the hash contains a particular key,
    use contains():

    \snippet code/src_corelib_tools_qhash.cpp 4

    There is also a value() overload that uses its second argument as
    a default value if there is no item with the specified key:

    \snippet code/src_corelib_tools_qhash.cpp 5

    In general, we recommend that you use contains() and value()
    rather than operator[]() for looking up a key in a hash. The
    reason is that operator[]() silently inserts an item into the
    hash if no item exists with the same key (unless the hash is
    const). For example, the following code snippet will create 1000
    items in memory:

    \snippet code/src_corelib_tools_qhash.cpp 6

    To avoid this problem, replace \c hash[i] with \c hash.value(i)
    in the code above.

    Internally, QHash uses a hash table to perform lookups. This
    hash table automatically grows and shrinks to
    provide fast lookups without wasting too much memory. You can
    still control the size of the hash table by calling reserve() if
    you already know approximately how many items the QHash will
    contain, but this isn't necessary to obtain good performance. You
    can also call capacity() to retrieve the hash table's size.

    If you want to navigate through all the (key, value) pairs stored
    in a QHash, you can use an iterator. QHash provides both
    \l{Java-style iterators} (QHashIterator and QMutableHashIterator)
    and \l{STL-style iterators} (QHash::const_iterator and
    QHash::iterator). Here's how to iterate over a QHash<QString,
    int> using a Java-style iterator:

    \snippet code/src_corelib_tools_qhash.cpp 7

    Here's the same code, but using an STL-style iterator:

    \snippet code/src_corelib_tools_qhash.cpp 8

    QHash is unordered, so an iterator's sequence cannot be assumed
    to be predictable. If ordering by key is required, use a QMap.

    Normally, a QHash allows only one value per key. If you call
    insert() with a key that already exists in the QHash, the
    previous value is erased. For example:

    \snippet code/src_corelib_tools_qhash.cpp 9

    However, you can store multiple values per key by using
    insertMulti() instead of insert() (or using the convenience
    subclass QMultiHash). If you want to retrieve all
    the values for a single key, you can use values(const Key &key),
    which returns a QList<T>:

    \snippet code/src_corelib_tools_qhash.cpp 10

    The items that share the same key are available from most
    recently to least recently inserted. A more efficient approach is
    to call find() to get the iterator for the first item with a key
    and iterate from there:

    \snippet code/src_corelib_tools_qhash.cpp 11

    If you only need to extract the values from a hash (not the keys),
    you can also use \l{foreach}:

    \snippet code/src_corelib_tools_qhash.cpp 12

    Items can be removed from the hash in several ways. One way is to
    call remove(); this will remove any item with the given key.
    Another way is to use QMutableHashIterator::remove(). In addition,
    you can clear the entire hash using clear().

    QHash's key and value data types must be \l{assignable data
    types}. You cannot, for example, store a QWidget as a value;
    instead, store a QWidget *.

    \target qHash
    \section2 The qHash() hashing function

    A QHash's key type has additional requirements other than being an
    assignable data type: it must provide operator==(), and there must also be
    a qHash() function in the type's namespace that returns a hash value for an
    argument of the key's type.

    The qHash() function computes a numeric value based on a key. It
    can use any algorithm imaginable, as long as it always returns
    the same value if given the same argument. In other words, if
    \c{e1 == e2}, then \c{qHash(e1) == qHash(e2)} must hold as well.
    However, to obtain good performance, the qHash() function should
    attempt to return different hash values for different keys to the
    largest extent possible.

    For a key type \c{K}, the qHash function must have one of these signatures:

    \code
    uint qHash(K key);
    uint qHash(const K &key);

    uint qHash(K key, uint seed);
    uint qHash(const K &key, uint seed);
    \endcode

    The two-arguments overloads take an unsigned integer that should be used to
    seed the calculation of the hash function. This seed is provided by QHash
    in order to prevent a family of \l{algorithmic complexity attacks}. If both
    a one-argument and a two-arguments overload are defined for a key type,
    the latter is used by QHash (note that you can simply define a
    two-arguments version, and use a default value for the seed parameter).

    Here's a partial list of the C++ and Qt types that can serve as keys in a
    QHash: any integer type (char, unsigned long, etc.), any pointer type,
    QChar, QString, and QByteArray. For all of these, the \c <QHash> header
    defines a qHash() function that computes an adequate hash value. Many other
    Qt classes also declare a qHash overload for their type; please refer to
    the documentation of each class.

    If you want to use other types as the key, make sure that you provide
    operator==() and a qHash() implementation.

    Example:
    \snippet code/src_corelib_tools_qhash.cpp 13

    In the example above, we've relied on Qt's global qHash(const
    QString &, uint) to give us a hash value for the employee's name, and
    XOR'ed this with the day they were born to help produce unique
    hashes for people with the same name.

    Note that the implementation of the qHash() overloads offered by Qt
    may change at any time. You \b{must not} rely on the fact that qHash()
    will give the same results (for the same inputs) across different Qt
    versions.

    \section2 Algorithmic complexity attacks

    All hash tables are vulnerable to a particular class of denial of service
    attacks, in which the attacker carefully pre-computes a set of different
    keys that are going to be hashed in the same bucket of a hash table (or
    even have the very same hash value). The attack aims at getting the
    worst-case algorithmic behavior (O(n) instead of amortized O(1), see
    \l{Algorithmic Complexity} for the details) when the data is fed into the
    table.

    In order to avoid this worst-case behavior, the calculation of the hash
    value done by qHash() can be salted by a random seed, that nullifies the
    attack's extent. This seed is automatically generated by QHash once per
    process, and then passed by QHash as the second argument of the
    two-arguments overload of the qHash() function.

    This randomization of QHash is enabled by default. Even though programs
    should never depend on a particular QHash ordering, there may be situations
    where you temporarily need deterministic behavior, for example for debugging or
    regression testing. To disable the randomization, define the environment
    variable \c QT_HASH_SEED to have the value 0. Alternatively, you can call
    the qSetGlobalQHashSeed() function with the value 0.

    \sa QHashIterator, QMutableHashIterator, QMap, QSet
*/

/*! \fn template <class Key, class T> QHash<Key, T>::QHash()

    Constructs an empty hash.

    \sa clear()
*/

/*!
    \fn template <class Key, class T> QHash<Key, T>::QHash(QHash &&other)

    Move-constructs a QHash instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*! \fn template <class Key, class T> QHash<Key, T>::QHash(std::initializer_list<std::pair<Key,T> > list)
    \since 5.1

    Constructs a hash with a copy of each of the elements in the
    initializer list \a list.

    This function is only available if the program is being
    compiled in C++11 mode.
*/

/*! \fn template <class Key, class T> QHash<Key, T>::QHash(const QHash &other)

    Constructs a copy of \a other.

    This operation occurs in \l{constant time}, because QHash is
    \l{implicitly shared}. This makes returning a QHash from a
    function very fast. If a shared instance is modified, it will be
    copied (copy-on-write), and this takes \l{linear time}.

    \sa operator=()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::~QHash()

    Destroys the hash. References to the values in the hash and all
    iterators of this hash become invalid.
*/

/*! \fn template <class Key, class T> QHash &QHash<Key, T>::operator=(const QHash &other)

    Assigns \a other to this hash and returns a reference to this hash.
*/

/*!
    \fn template <class Key, class T> QHash &QHash<Key, T>::operator=(QHash &&other)

    Move-assigns \a other to this QHash instance.

    \since 5.2
*/

/*! \fn template <class Key, class T> void QHash<Key, T>::swap(QHash &other)
    \since 4.8

    Swaps hash \a other with this hash. This operation is very
    fast and never fails.
*/

/*! \fn template <class Key, class T> void QMultiHash<Key, T>::swap(QMultiHash &other)
    \since 4.8

    Swaps hash \a other with this hash. This operation is very
    fast and never fails.
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::operator==(const QHash &other) const

    Returns \c true if \a other is equal to this hash; otherwise returns
    false.

    Two hashes are considered equal if they contain the same (key,
    value) pairs.

    This function requires the value type to implement \c operator==().

    \sa operator!=()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::operator!=(const QHash &other) const

    Returns \c true if \a other is not equal to this hash; otherwise
    returns \c false.

    Two hashes are considered equal if they contain the same (key,
    value) pairs.

    This function requires the value type to implement \c operator==().

    \sa operator==()
*/

/*! \fn template <class Key, class T> int QHash<Key, T>::size() const

    Returns the number of items in the hash.

    \sa isEmpty(), count()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::isEmpty() const

    Returns \c true if the hash contains no items; otherwise returns
    false.

    \sa size()
*/

/*! \fn template <class Key, class T> int QHash<Key, T>::capacity() const

    Returns the number of buckets in the QHash's internal hash table.

    The sole purpose of this function is to provide a means of fine
    tuning QHash's memory usage. In general, you will rarely ever
    need to call this function. If you want to know how many items are
    in the hash, call size().

    \sa reserve(), squeeze()
*/

/*! \fn template <class Key, class T> void QHash<Key, T>::reserve(int size)

    Ensures that the QHash's internal hash table consists of at least
    \a size buckets.

    This function is useful for code that needs to build a huge hash
    and wants to avoid repeated reallocation. For example:

    \snippet code/src_corelib_tools_qhash.cpp 14

    Ideally, \a size should be slightly more than the maximum number
    of items expected in the hash. \a size doesn't have to be prime,
    because QHash will use a prime number internally anyway. If \a size
    is an underestimate, the worst that will happen is that the QHash
    will be a bit slower.

    In general, you will rarely ever need to call this function.
    QHash's internal hash table automatically shrinks or grows to
    provide good performance without wasting too much memory.

    \sa squeeze(), capacity()
*/

/*! \fn template <class Key, class T> void QHash<Key, T>::squeeze()

    Reduces the size of the QHash's internal hash table to save
    memory.

    The sole purpose of this function is to provide a means of fine
    tuning QHash's memory usage. In general, you will rarely ever
    need to call this function.

    \sa reserve(), capacity()
*/

/*! \fn template <class Key, class T> void QHash<Key, T>::detach()

    \internal

    Detaches this hash from any other hashes with which it may share
    data.

    \sa isDetached()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::isDetached() const

    \internal

    Returns \c true if the hash's internal data isn't shared with any
    other hash object; otherwise returns \c false.

    \sa detach()
*/

/*! \fn template <class Key, class T> void QHash<Key, T>::setSharable(bool sharable)

    \internal
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::isSharedWith(const QHash &other) const

    \internal
*/

/*! \fn template <class Key, class T> void QHash<Key, T>::clear()

    Removes all items from the hash.

    \sa remove()
*/

/*! \fn template <class Key, class T> int QHash<Key, T>::remove(const Key &key)

    Removes all the items that have the \a key from the hash.
    Returns the number of items removed which is usually 1 but will
    be 0 if the key isn't in the hash, or greater than 1 if
    insertMulti() has been used with the \a key.

    \sa clear(), take(), QMultiHash::remove()
*/

/*! \fn template <class Key, class T> T QHash<Key, T>::take(const Key &key)

    Removes the item with the \a key from the hash and returns
    the value associated with it.

    If the item does not exist in the hash, the function simply
    returns a \l{default-constructed value}. If there are multiple
    items for \a key in the hash, only the most recently inserted one
    is removed.

    If you don't use the return value, remove() is more efficient.

    \sa remove()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::contains(const Key &key) const

    Returns \c true if the hash contains an item with the \a key;
    otherwise returns \c false.

    \sa count(), QMultiHash::contains()
*/

/*! \fn template <class Key, class T> const T QHash<Key, T>::value(const Key &key) const

    Returns the value associated with the \a key.

    If the hash contains no item with the \a key, the function
    returns a \l{default-constructed value}. If there are multiple
    items for the \a key in the hash, the value of the most recently
    inserted one is returned.

    \sa key(), values(), contains(), operator[]()
*/

/*! \fn template <class Key, class T> const T QHash<Key, T>::value(const Key &key, const T &defaultValue) const
    \overload

    If the hash contains no item with the given \a key, the function returns
    \a defaultValue.
*/

/*! \fn template <class Key, class T> T &QHash<Key, T>::operator[](const Key &key)

    Returns the value associated with the \a key as a modifiable
    reference.

    If the hash contains no item with the \a key, the function inserts
    a \l{default-constructed value} into the hash with the \a key, and
    returns a reference to it. If the hash contains multiple items
    with the \a key, this function returns a reference to the most
    recently inserted value.

    \sa insert(), value()
*/

/*! \fn template <class Key, class T> const T QHash<Key, T>::operator[](const Key &key) const

    \overload

    Same as value().
*/

/*! \fn template <class Key, class T> QList<Key> QHash<Key, T>::uniqueKeys() const
    \since 4.2

    Returns a list containing all the keys in the map. Keys that occur multiple
    times in the map (because items were inserted with insertMulti(), or
    unite() was used) occur only once in the returned list.

    \sa keys(), values()
*/

/*! \fn template <class Key, class T> QList<Key> QHash<Key, T>::keys() const

    Returns a list containing all the keys in the hash, in an
    arbitrary order. Keys that occur multiple times in the hash
    (because items were inserted with insertMulti(), or unite() was
    used) also occur multiple times in the list.

    To obtain a list of unique keys, where each key from the map only
    occurs once, use uniqueKeys().

    The order is guaranteed to be the same as that used by values().

    \sa uniqueKeys(), values(), key()
*/

/*! \fn template <class Key, class T> QList<Key> QHash<Key, T>::keys(const T &value) const

    \overload

    Returns a list containing all the keys associated with value \a
    value, in an arbitrary order.

    This function can be slow (\l{linear time}), because QHash's
    internal data structure is optimized for fast lookup by key, not
    by value.
*/

/*! \fn template <class Key, class T> QList<T> QHash<Key, T>::values() const

    Returns a list containing all the values in the hash, in an
    arbitrary order. If a key is associated with multiple values, all of
    its values will be in the list, and not just the most recently
    inserted one.

    The order is guaranteed to be the same as that used by keys().

    \sa keys(), value()
*/

/*! \fn template <class Key, class T> QList<T> QHash<Key, T>::values(const Key &key) const

    \overload

    Returns a list of all the values associated with the \a key,
    from the most recently inserted to the least recently inserted.

    \sa count(), insertMulti()
*/

/*! \fn template <class Key, class T> Key QHash<Key, T>::key(const T &value) const

    Returns the first key mapped to \a value.

    If the hash contains no item with the \a value, the function
    returns a \l{default-constructed value}{default-constructed key}.

    This function can be slow (\l{linear time}), because QHash's
    internal data structure is optimized for fast lookup by key, not
    by value.

    \sa value(), keys()
*/

/*!
    \fn template <class Key, class T> Key QHash<Key, T>::key(const T &value, const Key &defaultKey) const
    \since 4.3
    \overload

    Returns the first key mapped to \a value, or \a defaultKey if the
    hash contains no item mapped to \a value.

    This function can be slow (\l{linear time}), because QHash's
    internal data structure is optimized for fast lookup by key, not
    by value.
*/

/*! \fn template <class Key, class T> int QHash<Key, T>::count(const Key &key) const

    Returns the number of items associated with the \a key.

    \sa contains(), insertMulti()
*/

/*! \fn template <class Key, class T> int QHash<Key, T>::count() const

    \overload

    Same as size().
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::begin()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first item in
    the hash.

    \sa constBegin(), end()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::begin() const

    \overload
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::cbegin() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the hash.

    \sa begin(), cend()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::constBegin() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first item
    in the hash.

    \sa begin(), constEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_iterator QHash<Key, T>::keyBegin() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first key
    in the hash.

    \sa keyEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::end()

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary item
    after the last item in the hash.

    \sa begin(), constEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::end() const

    \overload
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::constEnd() const

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the hash.

    \sa constBegin(), end()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::cend() const
    \since 5.0

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last item in the hash.

    \sa cbegin(), end()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_iterator QHash<Key, T>::keyEnd() const
    \since 5.6

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    item after the last key in the hash.

    \sa keyBegin()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_value_iterator QHash<Key, T>::keyValueBegin()
    \since 5.10

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the hash.

    \sa keyValueEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_value_iterator QHash<Key, T>::keyValueEnd()
    \since 5.10

    Returns an \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the hash.

    \sa keyValueBegin()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_key_value_iterator QHash<Key, T>::keyValueBegin() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the hash.

    \sa keyValueEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_key_value_iterator QHash<Key, T>::constKeyValueBegin() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the first entry
    in the hash.

    \sa keyValueBegin()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_key_value_iterator QHash<Key, T>::keyValueEnd() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the hash.

    \sa keyValueBegin()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_key_value_iterator QHash<Key, T>::constKeyValueEnd() const
    \since 5.10

    Returns a const \l{STL-style iterators}{STL-style iterator} pointing to the imaginary
    entry after the last entry in the hash.

    \sa constKeyValueBegin()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::erase(const_iterator pos)
    \since 5.7

    Removes the (key, value) pair associated with the iterator \a pos
    from the hash, and returns an iterator to the next item in the
    hash.

    Unlike remove() and take(), this function never causes QHash to
    rehash its internal data structure. This means that it can safely
    be called while iterating, and won't affect the order of items in
    the hash. For example:

    \snippet code/src_corelib_tools_qhash.cpp 15

    \sa remove(), take(), find()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::erase(iterator pos)
    \overload
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::find(const Key &key)

    Returns an iterator pointing to the item with the \a key in the
    hash.

    If the hash contains no item with the \a key, the function
    returns end().

    If the hash contains multiple items with the \a key, this
    function returns an iterator that points to the most recently
    inserted value. The other values are accessible by incrementing
    the iterator. For example, here's some code that iterates over all
    the items with the same key:

    \snippet code/src_corelib_tools_qhash.cpp 16

    \sa value(), values(), QMultiHash::find()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::find(const Key &key) const

    \overload
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::constFind(const Key &key) const
    \since 4.1

    Returns an iterator pointing to the item with the \a key in the
    hash.

    If the hash contains no item with the \a key, the function
    returns constEnd().

    \sa find(), QMultiHash::constFind()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::insert(const Key &key, const T &value)

    Inserts a new item with the \a key and a value of \a value.

    If there is already an item with the \a key, that item's value
    is replaced with \a value.

    If there are multiple items with the \a key, the most
    recently inserted item's value is replaced with \a value.

    \sa insertMulti()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::insertMulti(const Key &key, const T &value)

    Inserts a new item with the \a key and a value of \a value.

    If there is already an item with the same key in the hash, this
    function will simply create a new one. (This behavior is
    different from insert(), which overwrites the value of an
    existing item.)

    \sa insert(), values()
*/

/*! \fn template <class Key, class T> QHash &QHash<Key, T>::unite(const QHash &other)

    Inserts all the items in the \a other hash into this hash. If a
    key is common to both hashes, the resulting hash will contain the
    key multiple times.

    \sa insertMulti()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::empty() const

    This function is provided for STL compatibility. It is equivalent
    to isEmpty(), returning true if the hash is empty; otherwise
    returns \c false.
*/

/*! \fn template <class Key, class T> QPair<iterator, iterator> QHash<Key, T>::equal_range(const Key &key)
    \since 5.7

    Returns a pair of iterators delimiting the range of values \c{[first, second)}, that
    are stored under \a key. If the range is empty then both iterators will be equal to end().
*/

/*!
    \fn template <class Key, class T> QPair<const_iterator, const_iterator> QHash<Key, T>::equal_range(const Key &key) const
    \overload
    \since 5.7
*/

/*! \typedef QHash::ConstIterator

    Qt-style synonym for QHash::const_iterator.
*/

/*! \typedef QHash::Iterator

    Qt-style synonym for QHash::iterator.
*/

/*! \typedef QHash::difference_type

    Typedef for ptrdiff_t. Provided for STL compatibility.
*/

/*! \typedef QHash::key_type

    Typedef for Key. Provided for STL compatibility.
*/

/*! \typedef QHash::mapped_type

    Typedef for T. Provided for STL compatibility.
*/

/*! \typedef QHash::size_type

    Typedef for int. Provided for STL compatibility.
*/

/*! \typedef QHash::iterator::difference_type
    \internal
*/

/*! \typedef QHash::iterator::iterator_category
    \internal
*/

/*! \typedef QHash::iterator::pointer
    \internal
*/

/*! \typedef QHash::iterator::reference
    \internal
*/

/*! \typedef QHash::iterator::value_type
    \internal
*/

/*! \typedef QHash::const_iterator::difference_type
    \internal
*/

/*! \typedef QHash::const_iterator::iterator_category
    \internal
*/

/*! \typedef QHash::const_iterator::pointer
    \internal
*/

/*! \typedef QHash::const_iterator::reference
    \internal
*/

/*! \typedef QHash::const_iterator::value_type
    \internal
*/

/*! \typedef QHash::key_iterator::difference_type
    \internal
*/

/*! \typedef QHash::key_iterator::iterator_category
    \internal
*/

/*! \typedef QHash::key_iterator::pointer
    \internal
*/

/*! \typedef QHash::key_iterator::reference
    \internal
*/

/*! \typedef QHash::key_iterator::value_type
    \internal
*/

/*! \class QHash::iterator
    \inmodule QtCore
    \brief The QHash::iterator class provides an STL-style non-const iterator for QHash and QMultiHash.

    QHash features both \l{STL-style iterators} and \l{Java-style
    iterators}. The STL-style iterators are more low-level and more
    cumbersome to use; on the other hand, they are slightly faster
    and, for developers who already know STL, have the advantage of
    familiarity.

    QHash\<Key, T\>::iterator allows you to iterate over a QHash (or
    QMultiHash) and to modify the value (but not the key) associated
    with a particular key. If you want to iterate over a const QHash,
    you should use QHash::const_iterator. It is generally good
    practice to use QHash::const_iterator on a non-const QHash as
    well, unless you need to change the QHash through the iterator.
    Const iterators are slightly faster, and can improve code
    readability.

    The default QHash::iterator constructor creates an uninitialized
    iterator. You must initialize it using a QHash function like
    QHash::begin(), QHash::end(), or QHash::find() before you can
    start iterating. Here's a typical loop that prints all the (key,
    value) pairs stored in a hash:

    \snippet code/src_corelib_tools_qhash.cpp 17

    Unlike QMap, which orders its items by key, QHash stores its
    items in an arbitrary order. The only guarantee is that items that
    share the same key (because they were inserted using
    QHash::insertMulti()) will appear consecutively, from the most
    recently to the least recently inserted value.

    Let's see a few examples of things we can do with a
    QHash::iterator that we cannot do with a QHash::const_iterator.
    Here's an example that increments every value stored in the QHash
    by 2:

    \snippet code/src_corelib_tools_qhash.cpp 18

    Here's an example that removes all the items whose key is a
    string that starts with an underscore character:

    \snippet code/src_corelib_tools_qhash.cpp 19

    The call to QHash::erase() removes the item pointed to by the
    iterator from the hash, and returns an iterator to the next item.
    Here's another way of removing an item while iterating:

    \snippet code/src_corelib_tools_qhash.cpp 20

    It might be tempting to write code like this:

    \snippet code/src_corelib_tools_qhash.cpp 21

    However, this will potentially crash in \c{++i}, because \c i is
    a dangling iterator after the call to erase().

    Multiple iterators can be used on the same hash. However, be
    aware that any modification performed directly on the QHash has
    the potential of dramatically changing the order in which the
    items are stored in the hash, as they might cause QHash to rehash
    its internal data structure. There is one notable exception:
    QHash::erase(). This function can safely be called while
    iterating, and won't affect the order of items in the hash. If you
    need to keep iterators over a long period of time, we recommend
    that you use QMap rather than QHash.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QHash::const_iterator, QHash::key_iterator, QMutableHashIterator
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator::iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QHash::begin(), QHash::end()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator::iterator(void *node)

    \internal
*/

/*! \fn template <class Key, class T> const Key &QHash<Key, T>::iterator::key() const

    Returns the current item's key as a const reference.

    There is no direct way of changing an item's key through an
    iterator, although it can be done by calling QHash::erase()
    followed by QHash::insert() or QHash::insertMulti().

    \sa value()
*/

/*! \fn template <class Key, class T> T &QHash<Key, T>::iterator::value() const

    Returns a modifiable reference to the current item's value.

    You can change the value of an item by using value() on
    the left side of an assignment, for example:

    \snippet code/src_corelib_tools_qhash.cpp 22

    \sa key(), operator*()
*/

/*! \fn template <class Key, class T> T &QHash<Key, T>::iterator::operator*() const

    Returns a modifiable reference to the current item's value.

    Same as value().

    \sa key()
*/

/*! \fn template <class Key, class T> T *QHash<Key, T>::iterator::operator->() const

    Returns a pointer to the current item's value.

    \sa value()
*/

/*!
    \fn template <class Key, class T> bool QHash<Key, T>::iterator::operator==(const iterator &other) const
    \fn template <class Key, class T> bool QHash<Key, T>::iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*!
    \fn template <class Key, class T> bool QHash<Key, T>::iterator::operator!=(const iterator &other) const
    \fn template <class Key, class T> bool QHash<Key, T>::iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template <class Key, class T> QHash<Key, T>::iterator &QHash<Key, T>::iterator::operator++()

    The prefix ++ operator (\c{++i}) advances the iterator to the
    next item in the hash and returns an iterator to the new current
    item.

    Calling this function on QHash::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{i++}) advances the iterator to the
    next item in the hash and returns an iterator to the previously
    current item.
*/

/*!
    \fn template <class Key, class T> QHash<Key, T>::iterator &QHash<Key, T>::iterator::operator--()

    The prefix -- operator (\c{--i}) makes the preceding item
    current and returns an iterator pointing to the new current item.

    Calling this function on QHash::begin() leads to undefined
    results.

    \sa operator++()
*/

/*!
    \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::iterator::operator--(int)

    \overload

    The postfix -- operator (\c{i--}) makes the preceding item
    current and returns an iterator pointing to the previously
    current item.
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-()

*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator QHash<Key, T>::iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator &QHash<Key, T>::iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    \sa operator-=(), operator+()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::iterator &QHash<Key, T>::iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    \sa operator+=(), operator-()
*/

/*! \class QHash::const_iterator
    \inmodule QtCore
    \brief The QHash::const_iterator class provides an STL-style const iterator for QHash and QMultiHash.

    QHash features both \l{STL-style iterators} and \l{Java-style
    iterators}. The STL-style iterators are more low-level and more
    cumbersome to use; on the other hand, they are slightly faster
    and, for developers who already know STL, have the advantage of
    familiarity.

    QHash\<Key, T\>::const_iterator allows you to iterate over a
    QHash (or a QMultiHash). If you want to modify the QHash as you
    iterate over it, you must use QHash::iterator instead. It is
    generally good practice to use QHash::const_iterator on a
    non-const QHash as well, unless you need to change the QHash
    through the iterator. Const iterators are slightly faster, and
    can improve code readability.

    The default QHash::const_iterator constructor creates an
    uninitialized iterator. You must initialize it using a QHash
    function like QHash::constBegin(), QHash::constEnd(), or
    QHash::find() before you can start iterating. Here's a typical
    loop that prints all the (key, value) pairs stored in a hash:

    \snippet code/src_corelib_tools_qhash.cpp 23

    Unlike QMap, which orders its items by key, QHash stores its
    items in an arbitrary order. The only guarantee is that items that
    share the same key (because they were inserted using
    QHash::insertMulti()) will appear consecutively, from the most
    recently to the least recently inserted value.

    Multiple iterators can be used on the same hash. However, be aware
    that any modification performed directly on the QHash has the
    potential of dramatically changing the order in which the items
    are stored in the hash, as they might cause QHash to rehash its
    internal data structure. If you need to keep iterators over a long
    period of time, we recommend that you use QMap rather than QHash.

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QHash::iterator, QHashIterator
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator::const_iterator()

    Constructs an uninitialized iterator.

    Functions like key(), value(), and operator++() must not be
    called on an uninitialized iterator. Use operator=() to assign a
    value to it before using it.

    \sa QHash::constBegin(), QHash::constEnd()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator::const_iterator(void *node)

    \internal
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator::const_iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*! \fn template <class Key, class T> const Key &QHash<Key, T>::const_iterator::key() const

    Returns the current item's key.

    \sa value()
*/

/*! \fn template <class Key, class T> const T &QHash<Key, T>::const_iterator::value() const

    Returns the current item's value.

    \sa key(), operator*()
*/

/*! \fn template <class Key, class T> const T &QHash<Key, T>::const_iterator::operator*() const

    Returns the current item's value.

    Same as value().

    \sa key()
*/

/*! \fn template <class Key, class T> const T *QHash<Key, T>::const_iterator::operator->() const

    Returns a pointer to the current item's value.

    \sa value()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::const_iterator::operator==(const const_iterator &other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::const_iterator::operator!=(const const_iterator &other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template <class Key, class T> QHash<Key, T>::const_iterator &QHash<Key, T>::const_iterator::operator++()

    The prefix ++ operator (\c{++i}) advances the iterator to the
    next item in the hash and returns an iterator to the new current
    item.

    Calling this function on QHash::end() leads to undefined results.

    \sa operator--()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::const_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{i++}) advances the iterator to the
    next item in the hash and returns an iterator to the previously
    current item.
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator &QHash<Key, T>::const_iterator::operator--()

    The prefix -- operator (\c{--i}) makes the preceding item
    current and returns an iterator pointing to the new current item.

    Calling this function on QHash::begin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::const_iterator::operator--(int)

    \overload

    The postfix -- operator (\c{i--}) makes the preceding item
    current and returns an iterator pointing to the previously
    current item.
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::const_iterator::operator+(int j) const

    Returns an iterator to the item at \a j positions forward from
    this iterator. (If \a j is negative, the iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator QHash<Key, T>::const_iterator::operator-(int j) const

    Returns an iterator to the item at \a j positions backward from
    this iterator. (If \a j is negative, the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator &QHash<Key, T>::const_iterator::operator+=(int j)

    Advances the iterator by \a j items. (If \a j is negative, the
    iterator goes backward.)

    This operation can be slow for large \a j values.

    \sa operator-=(), operator+()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::const_iterator &QHash<Key, T>::const_iterator::operator-=(int j)

    Makes the iterator go back by \a j items. (If \a j is negative,
    the iterator goes forward.)

    This operation can be slow for large \a j values.

    \sa operator+=(), operator-()
*/

/*! \class QHash::key_iterator
    \inmodule QtCore
    \since 5.6
    \brief The QHash::key_iterator class provides an STL-style const iterator for QHash and QMultiHash keys.

    QHash::key_iterator is essentially the same as QHash::const_iterator
    with the difference that operator*() and operator->() return a key
    instead of a value.

    For most uses QHash::iterator and QHash::const_iterator should be used,
    you can easily access the key by calling QHash::iterator::key():

    \snippet code/src_corelib_tools_qhash.cpp 27

    However, to have interoperability between QHash's keys and STL-style
    algorithms we need an iterator that dereferences to a key instead
    of a value. With QHash::key_iterator we can apply an algorithm to a
    range of keys without having to call QHash::keys(), which is inefficient
    as it costs one QHash iteration and memory allocation to create a temporary
    QList.

    \snippet code/src_corelib_tools_qhash.cpp 28

    QHash::key_iterator is const, it's not possible to modify the key.

    The default QHash::key_iterator constructor creates an uninitialized
    iterator. You must initialize it using a QHash function like
    QHash::keyBegin() or QHash::keyEnd().

    \warning Iterators on implicitly shared containers do not work
    exactly like STL-iterators. You should avoid copying a container
    while iterators are active on that container. For more information,
    read \l{Implicit sharing iterator problem}.

    \sa QHash::const_iterator, QHash::iterator
*/

/*! \fn template <class Key, class T> const T &QHash<Key, T>::key_iterator::operator*() const

    Returns the current item's key.
*/

/*! \fn template <class Key, class T> const T *QHash<Key, T>::key_iterator::operator->() const

    Returns a pointer to the current item's key.
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::key_iterator::operator==(key_iterator other) const

    Returns \c true if \a other points to the same item as this
    iterator; otherwise returns \c false.

    \sa operator!=()
*/

/*! \fn template <class Key, class T> bool QHash<Key, T>::key_iterator::operator!=(key_iterator other) const

    Returns \c true if \a other points to a different item than this
    iterator; otherwise returns \c false.

    \sa operator==()
*/

/*!
    \fn template <class Key, class T> QHash<Key, T>::key_iterator &QHash<Key, T>::key_iterator::operator++()

    The prefix ++ operator (\c{++i}) advances the iterator to the
    next item in the hash and returns an iterator to the new current
    item.

    Calling this function on QHash::keyEnd() leads to undefined results.

    \sa operator--()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_iterator QHash<Key, T>::key_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{i++}) advances the iterator to the
    next item in the hash and returns an iterator to the previous
    item.
*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_iterator &QHash<Key, T>::key_iterator::operator--()

    The prefix -- operator (\c{--i}) makes the preceding item
    current and returns an iterator pointing to the new current item.

    Calling this function on QHash::keyBegin() leads to undefined
    results.

    \sa operator++()
*/

/*! \fn template <class Key, class T> QHash<Key, T>::key_iterator QHash<Key, T>::key_iterator::operator--(int)

    \overload

    The postfix -- operator (\c{i--}) makes the preceding item
    current and returns an iterator pointing to the previous
    item.
*/

/*! \fn template <class Key, class T> const_iterator QHash<Key, T>::key_iterator::base() const
    Returns the underlying const_iterator this key_iterator is based on.
*/

/*! \typedef QHash::const_key_value_iterator
    \inmodule QtCore
    \since 5.10
    \brief The QMap::const_key_value_iterator typedef provides an STL-style const iterator for QHash and QMultiHash.

    QHash::const_key_value_iterator is essentially the same as QHash::const_iterator
    with the difference that operator*() returns a key/value pair instead of a
    value.

    \sa QKeyValueIterator
*/

/*! \typedef QHash::key_value_iterator
    \inmodule QtCore
    \since 5.10
    \brief The QMap::key_value_iterator typedef provides an STL-style iterator for QHash and QMultiHash.

    QHash::key_value_iterator is essentially the same as QHash::iterator
    with the difference that operator*() returns a key/value pair instead of a
    value.

    \sa QKeyValueIterator
*/

/*! \fn template <class Key, class T> QDataStream &operator<<(QDataStream &out, const QHash<Key, T>& hash)
    \relates QHash

    Writes the hash \a hash to stream \a out.

    This function requires the key and value types to implement \c
    operator<<().

    \sa {Serializing Qt Data Types}
*/

/*! \fn template <class Key, class T> QDataStream &operator>>(QDataStream &in, QHash<Key, T> &hash)
    \relates QHash

    Reads a hash from stream \a in into \a hash.

    This function requires the key and value types to implement \c
    operator>>().

    \sa {Serializing Qt Data Types}
*/

/*! \class QMultiHash
    \inmodule QtCore
    \brief The QMultiHash class is a convenience QHash subclass that provides multi-valued hashes.

    \ingroup tools
    \ingroup shared

    \reentrant

    QMultiHash\<Key, T\> is one of Qt's generic \l{container classes}.
    It inherits QHash and extends it with a few convenience functions
    that make it more suitable than QHash for storing multi-valued
    hashes. A multi-valued hash is a hash that allows multiple values
    with the same key; QHash normally doesn't allow that, unless you
    call QHash::insertMulti().

    Because QMultiHash inherits QHash, all of QHash's functionality also
    applies to QMultiHash. For example, you can use isEmpty() to test
    whether the hash is empty, and you can traverse a QMultiHash using
    QHash's iterator classes (for example, QHashIterator). But in
    addition, it provides an insert() function that corresponds to
    QHash::insertMulti(), and a replace() function that corresponds to
    QHash::insert(). It also provides convenient operator+() and
    operator+=().

    Example:
    \snippet code/src_corelib_tools_qhash.cpp 24

    Unlike QHash, QMultiHash provides no operator[]. Use value() or
    replace() if you want to access the most recently inserted item
    with a certain key.

    If you want to retrieve all the values for a single key, you can
    use values(const Key &key), which returns a QList<T>:

    \snippet code/src_corelib_tools_qhash.cpp 25

    The items that share the same key are available from most
    recently to least recently inserted.

    A more efficient approach is to call find() to get
    the STL-style iterator for the first item with a key and iterate from
    there:

    \snippet code/src_corelib_tools_qhash.cpp 26

    QMultiHash's key and value data types must be \l{assignable data
    types}. You cannot, for example, store a QWidget as a value;
    instead, store a QWidget *. In addition, QMultiHash's key type
    must provide operator==(), and there must also be a qHash() function
   in the type's namespace that returns a hash value for an argument of the
    key's type. See the QHash documentation for details.

    \sa QHash, QHashIterator, QMutableHashIterator, QMultiMap
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::QMultiHash()

    Constructs an empty hash.
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::QMultiHash(std::initializer_list<std::pair<Key,T> > list)
    \since 5.1

    Constructs a multi-hash with a copy of each of the elements in the
    initializer list \a list.

    This function is only available if the program is being
    compiled in C++11 mode.
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::QMultiHash(const QHash<Key, T> &other)

    Constructs a copy of \a other (which can be a QHash or a
    QMultiHash).

    \sa operator=()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::iterator QMultiHash<Key, T>::replace(const Key &key, const T &value)

    Inserts a new item with the \a key and a value of \a value.

    If there is already an item with the \a key, that item's value
    is replaced with \a value.

    If there are multiple items with the \a key, the most
    recently inserted item's value is replaced with \a value.

    \sa insert()
*/

/*! \fn template <class Key, class T> QMultiHash<Key, T>::iterator QMultiHash<Key, T>::insert(const Key &key, const T &value)

    Inserts a new item with the \a key and a value of \a value.

    If there is already an item with the same key in the hash, this
    function will simply create a new one. (This behavior is
    different from replace(), which overwrites the value of an
    existing item.)

    \sa replace()
*/

/*! \fn template <class Key, class T> QMultiHash &QMultiHash<Key, T>::operator+=(const QMultiHash &other)

    Inserts all the items in the \a other hash into this hash
    and returns a reference to this hash.

    \sa insert()
*/

/*! \fn template <class Key, class T> QMultiHash QMultiHash<Key, T>::operator+(const QMultiHash &other) const

    Returns a hash that contains all the items in this hash in
    addition to all the items in \a other. If a key is common to both
    hashes, the resulting hash will contain the key multiple times.

    \sa operator+=()
*/

/*!
    \fn template <class Key, class T> bool QMultiHash<Key, T>::contains(const Key &key, const T &value) const
    \since 4.3

    Returns \c true if the hash contains an item with the \a key and
    \a value; otherwise returns \c false.

    \sa QHash::contains()
*/

/*!
    \fn template <class Key, class T> int QMultiHash<Key, T>::remove(const Key &key, const T &value)
    \since 4.3

    Removes all the items that have the \a key and the value \a
    value from the hash. Returns the number of items removed.

    \sa QHash::remove()
*/

/*!
    \fn template <class Key, class T> int QMultiHash<Key, T>::count(const Key &key, const T &value) const
    \since 4.3

    Returns the number of items with the \a key and \a value.

    \sa QHash::count()
*/

/*!
    \fn template <class Key, class T> typename QHash<Key, T>::iterator QMultiHash<Key, T>::find(const Key &key, const T &value)
    \since 4.3

    Returns an iterator pointing to the item with the \a key and \a value.
    If the hash contains no such item, the function returns end().

    If the hash contains multiple items with the \a key and \a value, the
    iterator returned points to the most recently inserted item.

    \sa QHash::find()
*/

/*!
    \fn template <class Key, class T> typename QHash<Key, T>::const_iterator QMultiHash<Key, T>::find(const Key &key, const T &value) const
    \since 4.3
    \overload
*/

/*!
    \fn template <class Key, class T> typename QHash<Key, T>::const_iterator QMultiHash<Key, T>::constFind(const Key &key, const T &value) const
    \since 4.3

    Returns an iterator pointing to the item with the \a key and the
    \a value in the hash.

    If the hash contains no such item, the function returns
    constEnd().

    \sa QHash::constFind()
*/

/*!
    \fn template <class Key, class T> uint qHash(const QHash<Key, T> &key, uint seed = 0)
    \since 5.8
    \relates QHash

    Returns the hash value for the \a key, using \a seed to seed the calculation.

    Type \c T must be supported by qHash().
*/

/*!
    \fn template <class Key, class T> uint qHash(const QMultiHash<Key, T> &key, uint seed = 0)
    \since 5.8
    \relates QMultiHash

    Returns the hash value for the \a key, using \a seed to seed the calculation.

    Type \c T must be supported by qHash().
*/

QT_END_NAMESPACE
