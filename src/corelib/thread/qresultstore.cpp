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

#include "qresultstore.h"

QT_BEGIN_NAMESPACE

namespace QtPrivate {

ResultIteratorBase::ResultIteratorBase()
 : mapIterator(QMap<int, ResultItem>::const_iterator()), m_vectorIndex(0) { }
ResultIteratorBase::ResultIteratorBase(QMap<int, ResultItem>::const_iterator _mapIterator, int _vectorIndex)
 : mapIterator(_mapIterator), m_vectorIndex(_vectorIndex) { }

int ResultIteratorBase::vectorIndex() const { return m_vectorIndex; }
int ResultIteratorBase::resultIndex() const { return mapIterator.key() + m_vectorIndex; }

ResultIteratorBase ResultIteratorBase::operator++()
{
    if (canIncrementVectorIndex()) {
        ++m_vectorIndex;
    } else {
        ++mapIterator;
        m_vectorIndex = 0;
    }
    return *this;
}

int ResultIteratorBase::batchSize() const
{
    return mapIterator.value().count();
}

void ResultIteratorBase::batchedAdvance()
{
    ++mapIterator;
    m_vectorIndex = 0;
}

bool ResultIteratorBase::operator==(const ResultIteratorBase &other) const
{
    return (mapIterator == other.mapIterator && m_vectorIndex == other.m_vectorIndex);
}

bool ResultIteratorBase::operator!=(const ResultIteratorBase &other) const
{
    return !operator==(other);
}

bool ResultIteratorBase::isVector() const
{
    return mapIterator.value().isVector();
}

bool ResultIteratorBase::canIncrementVectorIndex() const
{
    return (m_vectorIndex + 1 < mapIterator.value().m_count);
}

ResultStoreBase::ResultStoreBase()
    : insertIndex(0), resultCount(0), m_filterMode(false), filteredResults(0) { }

ResultStoreBase::~ResultStoreBase()
{
    // QFutureInterface's dtor must delete the contents of m_results.
    Q_ASSERT(m_results.isEmpty());
}

void ResultStoreBase::setFilterMode(bool enable)
{
    m_filterMode = enable;
}

bool ResultStoreBase::filterMode() const
{
    return m_filterMode;
}

void ResultStoreBase::syncResultCount()
{
    ResultIteratorBase it = resultAt(resultCount);
    while (it != end()) {
        resultCount += it.batchSize();
        it = resultAt(resultCount);
    }
}

void ResultStoreBase::insertResultItemIfValid(int index, ResultItem &resultItem)
{
    if (resultItem.isValid()) {
        m_results[index] = resultItem;
        syncResultCount();
    } else {
        filteredResults += resultItem.count();
    }
}

int ResultStoreBase::insertResultItem(int index, ResultItem &resultItem)
{
    int storeIndex;
    if (m_filterMode && index != -1 && index > insertIndex) {
        pendingResults[index] = resultItem;
        storeIndex = index;
    } else {
        storeIndex = updateInsertIndex(index, resultItem.count());
        insertResultItemIfValid(storeIndex - filteredResults, resultItem);
    }
    syncPendingResults();
    return storeIndex;
}

void ResultStoreBase::syncPendingResults()
{
    // check if we can insert any of the pending results:
    QMap<int, ResultItem>::iterator it = pendingResults.begin();
    while (it != pendingResults.end()) {
        int index = it.key();
        if (index != resultCount + filteredResults)
            break;

        ResultItem result = it.value();
        insertResultItemIfValid(index - filteredResults, result);
        pendingResults.erase(it);
        it = pendingResults.begin();
    }
}

int ResultStoreBase::addResult(int index, const void *result)
{
    ResultItem resultItem(result, 0); // 0 means "not a vector"
    return insertResultItem(index, resultItem);
}

int ResultStoreBase::addResults(int index, const void *results, int vectorSize, int totalCount)
{
    if (m_filterMode == false || vectorSize == totalCount) {
        ResultItem resultItem(results, vectorSize);
        return insertResultItem(index, resultItem);
    } else {
        if (vectorSize > 0) {
            ResultItem filteredIn(results, vectorSize);
            insertResultItem(index, filteredIn);
        }
        ResultItem filteredAway(0, totalCount - vectorSize);
        return insertResultItem(index + vectorSize, filteredAway);
    }
}

ResultIteratorBase ResultStoreBase::begin() const
{
    return ResultIteratorBase(m_results.begin());
}

ResultIteratorBase ResultStoreBase::end() const
{
    return ResultIteratorBase(m_results.end());
}

bool ResultStoreBase::hasNextResult() const
{
    return begin() != end();
}

ResultIteratorBase ResultStoreBase::resultAt(int index) const
{
    if (m_results.isEmpty())
        return ResultIteratorBase(m_results.end());
    QMap<int, ResultItem>::const_iterator it = m_results.lowerBound(index);

    // lowerBound returns either an iterator to the result or an iterator
    // to the nearest greater index. If the latter happens it might be
    // that the result is stored in a vector at the previous index.
    if (it == m_results.end()) {
        --it;
        if (it.value().isVector() == false) {
            return ResultIteratorBase(m_results.end());
        }
    } else {
        if (it.key() > index) {
            if (it == m_results.begin())
                return ResultIteratorBase(m_results.end());
            --it;
        }
    }

    const int vectorIndex = index - it.key();

    if (vectorIndex >= it.value().count())
        return ResultIteratorBase(m_results.end());
    else if (it.value().isVector() == false && vectorIndex != 0)
        return ResultIteratorBase(m_results.end());
    return ResultIteratorBase(it, vectorIndex);
}

bool ResultStoreBase::contains(int index) const
{
    return (resultAt(index) != end());
}

int ResultStoreBase::count() const
{
    return resultCount;
}

// returns the insert index, calling this function with
// index equal to -1 returns the next available index.
int ResultStoreBase::updateInsertIndex(int index, int _count)
{
    if (index == -1) {
        index = insertIndex;
        insertIndex += _count;
    } else {
        insertIndex = qMax(index + _count, insertIndex);
    }
    return index;
}

} // namespace QtPrivate

QT_END_NAMESPACE
