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

/*
    A simple model that uses a QStringList as its data source.
*/

#include "qstringlistmodel.h"

#include <QtCore/qvector.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \class QStringListModel
    \inmodule QtCore
    \brief The QStringListModel class provides a model that supplies strings to views.

    \ingroup model-view

    QStringListModel is an editable model that can be used for simple
    cases where you need to display a number of strings in a view
    widget, such as a QListView or a QComboBox.

    The model provides all the standard functions of an editable
    model, representing the data in the string list as a model with
    one column and a number of rows equal to the number of items in
    the list.

    Model indexes corresponding to items are obtained with the
    \l{QAbstractListModel::index()}{index()} function, and item flags
    are obtained with flags().  Item data is read with the data()
    function and written with setData().  The number of rows (and
    number of items in the string list) can be found with the
    rowCount() function.

    The model can be constructed with an existing string list, or
    strings can be set later with the setStringList() convenience
    function. Strings can also be inserted in the usual way with the
    insertRows() function, and removed with removeRows(). The contents
    of the string list can be retrieved with the stringList()
    convenience function.

    An example usage of QStringListModel:

    \snippet qstringlistmodel/main.cpp 0

    \sa QAbstractListModel, QAbstractItemModel, {Model Classes}
*/

/*!
    Constructs a string list model with the given \a parent.
*/

QStringListModel::QStringListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

/*!
    Constructs a string list model containing the specified \a strings
    with the given \a parent.
*/

QStringListModel::QStringListModel(const QStringList &strings, QObject *parent)
    : QAbstractListModel(parent), lst(strings)
{
}

/*!
    Returns the number of rows in the model. This value corresponds to the
    number of items in the model's internal string list.

    The optional \a parent argument is in most models used to specify
    the parent of the rows to be counted. Because this is a list if a
    valid parent is specified, the result will always be 0.

    \sa insertRows(), removeRows(), QAbstractItemModel::rowCount()
*/

int QStringListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return lst.count();
}

/*!
  \reimp
*/
QModelIndex QStringListModel::sibling(int row, int column, const QModelIndex &idx) const
{
    if (!idx.isValid() || column != 0 || row >= lst.count() || row < 0)
        return QModelIndex();

    return createIndex(row, 0);
}

/*!
    Returns data for the specified \a role, from the item with the
    given \a index.

    If the view requests an invalid index, an invalid variant is returned.

    \sa setData()
*/

QVariant QStringListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= lst.size())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return lst.at(index.row());

    return QVariant();
}

/*!
    Returns the flags for the item with the given \a index.

    Valid items are enabled, selectable, editable, drag enabled and drop enabled.

    \sa QAbstractItemModel::flags()
*/

Qt::ItemFlags QStringListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;

    return QAbstractListModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

/*!
    Sets the data for the specified \a role in the item with the given
    \a index in the model, to the provided \a value.

    The dataChanged() signal is emitted if the item is changed.

    \sa Qt::ItemDataRole, data()
*/

bool QStringListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.row() >= 0 && index.row() < lst.size()
        && (role == Qt::EditRole || role == Qt::DisplayRole)) {
        lst.replace(index.row(), value.toString());
        QVector<int> roles;
        roles.reserve(2);
        roles.append(Qt::DisplayRole);
        roles.append(Qt::EditRole);
        emit dataChanged(index, index, roles);
        // once Q_COMPILER_UNIFORM_INIT can be used, change to:
        // emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
        return true;
    }
    return false;
}

/*!
    Inserts \a count rows into the model, beginning at the given \a row.

    The \a parent index of the rows is optional and is only used for
    consistency with QAbstractItemModel. By default, a null index is
    specified, indicating that the rows are inserted in the top level of
    the model.

    \sa QAbstractItemModel::insertRows()
*/

bool QStringListModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (count < 1 || row < 0 || row > rowCount(parent))
        return false;

    beginInsertRows(QModelIndex(), row, row + count - 1);

    for (int r = 0; r < count; ++r)
        lst.insert(row, QString());

    endInsertRows();

    return true;
}

/*!
    Removes \a count rows from the model, beginning at the given \a row.

    The \a parent index of the rows is optional and is only used for
    consistency with QAbstractItemModel. By default, a null index is
    specified, indicating that the rows are removed in the top level of
    the model.

    \sa QAbstractItemModel::removeRows()
*/

bool QStringListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (count <= 0 || row < 0 || (row + count) > rowCount(parent))
        return false;

    beginRemoveRows(QModelIndex(), row, row + count - 1);

    const auto it = lst.begin() + row;
    lst.erase(it, it + count);

    endRemoveRows();

    return true;
}

static bool ascendingLessThan(const QPair<QString, int> &s1, const QPair<QString, int> &s2)
{
    return s1.first < s2.first;
}

static bool decendingLessThan(const QPair<QString, int> &s1, const QPair<QString, int> &s2)
{
    return s1.first > s2.first;
}

/*!
  \reimp
*/
void QStringListModel::sort(int, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged(QList<QPersistentModelIndex>(), VerticalSortHint);

    QVector<QPair<QString, int> > list;
    const int lstCount = lst.count();
    list.reserve(lstCount);
    for (int i = 0; i < lstCount; ++i)
        list.append(QPair<QString, int>(lst.at(i), i));

    if (order == Qt::AscendingOrder)
        std::sort(list.begin(), list.end(), ascendingLessThan);
    else
        std::sort(list.begin(), list.end(), decendingLessThan);

    lst.clear();
    QVector<int> forwarding(lstCount);
    for (int i = 0; i < lstCount; ++i) {
        lst.append(list.at(i).first);
        forwarding[list.at(i).second] = i;
    }

    QModelIndexList oldList = persistentIndexList();
    QModelIndexList newList;
    const int numOldIndexes = oldList.count();
    newList.reserve(numOldIndexes);
    for (int i = 0; i < numOldIndexes; ++i)
        newList.append(index(forwarding.at(oldList.at(i).row()), 0));
    changePersistentIndexList(oldList, newList);

    emit layoutChanged(QList<QPersistentModelIndex>(), VerticalSortHint);
}

/*!
    Returns the string list used by the model to store data.
*/
QStringList QStringListModel::stringList() const
{
    return lst;
}

/*!
    Sets the model's internal string list to \a strings. The model will
    notify any attached views that its underlying data has changed.

    \sa dataChanged()
*/
void QStringListModel::setStringList(const QStringList &strings)
{
    beginResetModel();
    lst = strings;
    endResetModel();
}

/*!
  \reimp
*/
Qt::DropActions QStringListModel::supportedDropActions() const
{
    return QAbstractItemModel::supportedDropActions() | Qt::MoveAction;
}

QT_END_NAMESPACE

#include "moc_qstringlistmodel.cpp"
