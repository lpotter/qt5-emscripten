/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include "tst_qsortfilterproxymodel.h"
#include "dynamictreemodel.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QStandardItem>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QTableView>

#include <qdebug.h>

// Testing get/set functions
void tst_QSortFilterProxyModel::getSetCheck()
{
    QSortFilterProxyModel obj1;
    QCOMPARE(obj1.sourceModel(), (QAbstractItemModel *)0);
    // int QSortFilterProxyModel::filterKeyColumn()
    // void QSortFilterProxyModel::setFilterKeyColumn(int)
    obj1.setFilterKeyColumn(0);
    QCOMPARE(0, obj1.filterKeyColumn());
    obj1.setFilterKeyColumn(INT_MIN);
    QCOMPARE(INT_MIN, obj1.filterKeyColumn());
    obj1.setFilterKeyColumn(INT_MAX);
    QCOMPARE(INT_MAX, obj1.filterKeyColumn());
}

tst_QSortFilterProxyModel::tst_QSortFilterProxyModel()
    : m_model(0), m_proxy(0)
{
    qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>();
}

void tst_QSortFilterProxyModel::initTestCase()
{
    qRegisterMetaType<QList<QPersistentModelIndex> >();
    m_model = new QStandardItemModel(0, 1);
    m_proxy = new QSortFilterProxyModel();
    m_proxy->setSourceModel(m_model);
}

void tst_QSortFilterProxyModel::cleanupTestCase()
{
    delete m_proxy;
    delete m_model;
}

void tst_QSortFilterProxyModel::cleanup()
{
    switch (m_filterType) {
    case FilterType::RegExp:
        m_proxy->setFilterRegExp(QRegExp());
        break;
    case FilterType::RegularExpression:
        m_proxy->setFilterRegularExpression(QRegularExpression());
        break;
    }

    m_proxy->sort(-1, Qt::AscendingOrder);
    m_model->clear();
    m_model->insertColumns(0, 1);
}

/*
  tests
*/

void tst_QSortFilterProxyModel::sort_data()
{
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<int>("sortCaseSensitivity");
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("flat descending") << static_cast<int>(Qt::DescendingOrder)
                                  << int(Qt::CaseSensitive)
                                  << (QStringList()
                                      << "delta"
                                      << "yankee"
                                      << "bravo"
                                      << "lima"
                                      << "charlie"
                                      << "juliet"
                                      << "tango"
                                      << "hotel"
                                      << "uniform"
                                      << "alpha"
                                      << "echo"
                                      << "golf"
                                      << "quebec"
                                      << "foxtrot"
                                      << "india"
                                      << "romeo"
                                      << "november"
                                      << "oskar"
                                      << "zulu"
                                      << "kilo"
                                      << "whiskey"
                                      << "mike"
                                      << "papa"
                                      << "sierra"
                                      << "xray"
                                      << "viktor")
                                  << (QStringList()
                                      << "zulu"
                                      << "yankee"
                                      << "xray"
                                      << "whiskey"
                                      << "viktor"
                                      << "uniform"
                                      << "tango"
                                      << "sierra"
                                      << "romeo"
                                      << "quebec"
                                      << "papa"
                                      << "oskar"
                                      << "november"
                                      << "mike"
                                      << "lima"
                                      << "kilo"
                                      << "juliet"
                                      << "india"
                                      << "hotel"
                                      << "golf"
                                      << "foxtrot"
                                      << "echo"
                                      << "delta"
                                      << "charlie"
                                      << "bravo"
                                      << "alpha");
    QTest::newRow("flat ascending") <<  static_cast<int>(Qt::AscendingOrder)
                                 << int(Qt::CaseSensitive)
                                 << (QStringList()
                                     << "delta"
                                     << "yankee"
                                     << "bravo"
                                     << "lima"
                                     << "charlie"
                                     << "juliet"
                                     << "tango"
                                     << "hotel"
                                     << "uniform"
                                     << "alpha"
                                     << "echo"
                                     << "golf"
                                     << "quebec"
                                     << "foxtrot"
                                     << "india"
                                     << "romeo"
                                     << "november"
                                     << "oskar"
                                     << "zulu"
                                     << "kilo"
                                     << "whiskey"
                                     << "mike"
                                     << "papa"
                                     << "sierra"
                                     << "xray"
                                     << "viktor")
                                 << (QStringList()
                                     << "alpha"
                                     << "bravo"
                                     << "charlie"
                                     << "delta"
                                     << "echo"
                                     << "foxtrot"
                                     << "golf"
                                     << "hotel"
                                     << "india"
                                     << "juliet"
                                     << "kilo"
                                     << "lima"
                                     << "mike"
                                     << "november"
                                     << "oskar"
                                     << "papa"
                                     << "quebec"
                                     << "romeo"
                                     << "sierra"
                                     << "tango"
                                     << "uniform"
                                     << "viktor"
                                     << "whiskey"
                                     << "xray"
                                     << "yankee"
                                     << "zulu");
    QTest::newRow("case insensitive") <<  static_cast<int>(Qt::AscendingOrder)
                                 << int(Qt::CaseInsensitive)
                                 << (QStringList()
                                     << "alpha" << "BETA" << "Gamma" << "delta")
                                 << (QStringList()
                                     << "alpha" << "BETA" << "delta" << "Gamma");
    QTest::newRow("case sensitive") <<  static_cast<int>(Qt::AscendingOrder)
                                 << int(Qt::CaseSensitive)
                                 << (QStringList()
                                     << "alpha" << "BETA" << "Gamma" << "delta")
                                 << (QStringList()
                                     << "BETA" << "Gamma" << "alpha" << "delta");

    QStringList list;
    for (int i = 10000; i < 20000; ++i)
        list.append(QStringLiteral("Number: ") + QString::number(i));
    QTest::newRow("large set ascending") <<  static_cast<int>(Qt::AscendingOrder) << int(Qt::CaseSensitive) << list << list;
}

void tst_QSortFilterProxyModel::sort()
{
    QFETCH(int, sortOrder);
    QFETCH(int, sortCaseSensitivity);
    QFETCH(QStringList, initial);
    QFETCH(QStringList, expected);

    // prepare model
    QStandardItem *root = m_model->invisibleRootItem ();
    QList<QStandardItem *> items;
    for (int i = 0; i < initial.count(); ++i) {
        items.append(new QStandardItem(initial.at(i)));
    }
    root->insertRows(0, items);
    QCOMPARE(m_model->rowCount(QModelIndex()), initial.count());
    QCOMPARE(m_model->columnCount(QModelIndex()), 1);

    // make sure the proxy is unsorted
    QCOMPARE(m_proxy->columnCount(QModelIndex()), 1);
    QCOMPARE(m_proxy->rowCount(QModelIndex()), initial.count());
    for (int row = 0; row < m_proxy->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_proxy->index(row, 0, QModelIndex());
        QCOMPARE(m_proxy->data(index, Qt::DisplayRole).toString(), initial.at(row));
    }

    // sort
    m_proxy->sort(0, static_cast<Qt::SortOrder>(sortOrder));
    m_proxy->setSortCaseSensitivity(static_cast<Qt::CaseSensitivity>(sortCaseSensitivity));

    // make sure the model is unchanged
    for (int row = 0; row < m_model->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_model->index(row, 0, QModelIndex());
        QCOMPARE(m_model->data(index, Qt::DisplayRole).toString(), initial.at(row));
    }
    // make sure the proxy is sorted
    for (int row = 0; row < m_proxy->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_proxy->index(row, 0, QModelIndex());
        QCOMPARE(m_proxy->data(index, Qt::DisplayRole).toString(), expected.at(row));
    }

    // restore the unsorted order
    m_proxy->sort(-1);

    // make sure the proxy is unsorted again
    for (int row = 0; row < m_proxy->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_proxy->index(row, 0, QModelIndex());
        QCOMPARE(m_proxy->data(index, Qt::DisplayRole).toString(), initial.at(row));
    }
}

void tst_QSortFilterProxyModel::sortHierarchy_data()
{
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("flat ascending")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList()
            << "c" << "f" << "d" << "e" << "a" << "b")
        << (QStringList()
            << "a" << "b" << "c" << "d" << "e" << "f");

    QTest::newRow("simple hierarchy")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList() << "a" << "<" << "b" << "<" << "c" << ">" << ">")
        << (QStringList() << "a" << "<" << "b" << "<" << "c" << ">" << ">");

    QTest::newRow("hierarchical ascending")
        << static_cast<int>(Qt::AscendingOrder)
        << (QStringList()
            << "c"
                   << "<"
                   << "h"
                          << "<"
                          << "2"
                          << "0"
                          << "1"
                          << ">"
                   << "g"
                   << "i"
                   << ">"
            << "b"
                   << "<"
                   << "l"
                   << "k"
                          << "<"
                          << "8"
                          << "7"
                          << "9"
                          << ">"
                   << "m"
                   << ">"
            << "a"
                   << "<"
                   << "z"
                   << "y"
                   << "x"
                   << ">")
        << (QStringList()
            << "a"
                   << "<"
                   << "x"
                   << "y"
                   << "z"
                   << ">"
            << "b"
                   << "<"
                   << "k"
                          << "<"
                          << "7"
                          << "8"
                          << "9"
                          << ">"
                   << "l"
                   << "m"
                   << ">"
            << "c"
                   << "<"
                   << "g"
                   << "h"
                          << "<"
                          << "0"
                          << "1"
                          << "2"
                          << ">"
                   << "i"
                   << ">");
}

void tst_QSortFilterProxyModel::sortHierarchy()
{
    QFETCH(int, sortOrder);
    QFETCH(QStringList, initial);
    QFETCH(QStringList, expected);

    buildHierarchy(initial, m_model);
    checkHierarchy(initial, m_model);
    checkHierarchy(initial, m_proxy);
    m_proxy->sort(0, static_cast<Qt::SortOrder>(sortOrder));
    checkHierarchy(initial, m_model);
    checkHierarchy(expected, m_proxy);
}

void tst_QSortFilterProxyModel::insertRows_data()
{
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<QStringList>("expected");
    QTest::addColumn<QStringList>("insert");
    QTest::addColumn<int>("position");

    QTest::newRow("insert one row in the middle")
        << (QStringList()
            << "One"
            << "Two"
            << "Four"
            << "Five")
        << (QStringList()
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << (QStringList()
            << "Three")
        << 2;

    QTest::newRow("insert one row in the beginning")
        << (QStringList()
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << (QStringList()
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << (QStringList()
            << "One")
        << 0;

    QTest::newRow("insert one row in the end")
        << (QStringList()
            << "One"
            << "Two"
            << "Three"
            << "Four")
        << (QStringList()
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << (QStringList()
            <<"Five")
        << 4;
}

void tst_QSortFilterProxyModel::insertRows()
{
    QFETCH(QStringList, initial);
    QFETCH(QStringList, expected);
    QFETCH(QStringList, insert);
    QFETCH(int, position);
    // prepare model
    m_model->insertRows(0, initial.count(), QModelIndex());
    //m_model->insertColumns(0, 1, QModelIndex());
    QCOMPARE(m_model->columnCount(QModelIndex()), 1);
    QCOMPARE(m_model->rowCount(QModelIndex()), initial.count());
    for (int row = 0; row < m_model->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_model->index(row, 0, QModelIndex());
        m_model->setData(index, initial.at(row), Qt::DisplayRole);
    }
    // make sure the model correct before insert
    for (int row = 0; row < m_model->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_model->index(row, 0, QModelIndex());
        QCOMPARE(m_model->data(index, Qt::DisplayRole).toString(), initial.at(row));
    }
    // make sure the proxy is correct before insert
    for (int row = 0; row < m_proxy->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_proxy->index(row, 0, QModelIndex());
        QCOMPARE(m_proxy->data(index, Qt::DisplayRole).toString(), initial.at(row));
    }

    // insert the row
    m_proxy->insertRows(position, insert.count(), QModelIndex());
    QCOMPARE(m_model->rowCount(QModelIndex()), expected.count());
    QCOMPARE(m_proxy->rowCount(QModelIndex()), expected.count());

    // set the data for the inserted row
    for (int i = 0; i < insert.count(); ++i) {
        QModelIndex index = m_proxy->index(position + i, 0, QModelIndex());
        m_proxy->setData(index, insert.at(i), Qt::DisplayRole);
    }

    // make sure the model correct after insert
    for (int row = 0; row < m_model->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_model->index(row, 0, QModelIndex());
        QCOMPARE(m_model->data(index, Qt::DisplayRole).toString(), expected.at(row));
    }

    // make sure the proxy is correct after insert
    for (int row = 0; row < m_proxy->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_proxy->index(row, 0, QModelIndex());
        QCOMPARE(m_proxy->data(index, Qt::DisplayRole).toString(), expected.at(row));
    }
}

void tst_QSortFilterProxyModel::prependRow()
{
    //this tests that data is correctly handled by the sort filter when prepending a row
    QStandardItemModel model;
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    QStandardItem item("root");
    model.appendRow(&item);

    QStandardItem sub("sub");
    item.appendRow(&sub);

    sub.appendRow(new QStandardItem("test1"));
    sub.appendRow(new QStandardItem("test2"));

    QStandardItem sub2("sub2");
    sub2.appendRow(new QStandardItem("sub3"));
    item.insertRow(0, &sub2);

    QModelIndex index_sub2 = proxy.mapFromSource(model.indexFromItem(&sub2));

    QCOMPARE(sub2.rowCount(), proxy.rowCount(index_sub2));
    QCOMPARE(proxy.rowCount(QModelIndex()), 1); //only the "root" item is there
}

void tst_QSortFilterProxyModel::appendRowFromCombobox_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<QString>("newitem");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("filter_out_second_last_item")
        << "^[0-9]*$"
        << (QStringList() << "a" << "1")
        << "2"
        << (QStringList() << "a" << "1" << "2");

    QTest::newRow("filter_out_everything")
        << "^c*$"
        << (QStringList() << "a" << "b")
        << "c"
        << (QStringList() << "a" << "b" << "c");

    QTest::newRow("no_filter")
        << ""
        << (QStringList() << "0" << "1")
        << "2"
        << (QStringList() << "0" << "1" << "2");

    QTest::newRow("filter_out_last_item")
        << "^[a-z]*$"
        << (QStringList() << "a" << "1")
        << "b"
        << (QStringList() << "a" << "1" << "b");
}

void tst_QSortFilterProxyModel::appendRowFromCombobox()
{
    QFETCH(QString, pattern);
    QFETCH(QStringList, initial);
    QFETCH(QString, newitem);
    QFETCH(QStringList, expected);

    QStringListModel model(initial);

    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setFilterRegExp(pattern);

    QComboBox comboBox;
    comboBox.setModel(&proxy);
    comboBox.addItem(newitem);

    QCOMPARE(model.stringList(), expected);
}

void tst_QSortFilterProxyModel::removeRows_data()
{
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<QString>("filter");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("count");
    QTest::addColumn<bool>("success");
    QTest::addColumn<QStringList>("expectedProxy");
    QTest::addColumn<QStringList>("expectedSource");

    QTest::newRow("remove one row in the middle [no sorting/filter]")
        << (QStringList()
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << -1 // no sorting
        << QString() // no filter
        << 2 // position
        << 1 // count
        << true // success
        << (QStringList() // expectedProxy
            << "One"
            << "Two"
            << "Four"
            << "Five")
        << (QStringList() // expectedSource
            << "One"
            << "Two"
            << "Four"
            << "Five");

    QTest::newRow("remove one row in the beginning [no sorting/filter]")
        << (QStringList()
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << -1 // no sorting
        << QString() // no filter
        << 0 // position
        << 1 // count
        << true // success
        << (QStringList() // expectedProxy
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << (QStringList() // expectedSource
            << "Two"
            << "Three"
            << "Four"
            << "Five");

    QTest::newRow("remove one row in the end [no sorting/filter]")
        << (QStringList()
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << -1 // no sorting
        << QString() // no filter
        << 4 // position
        << 1 // count
        << true // success
        << (QStringList() // expectedProxy
            << "One"
            << "Two"
            << "Three"
            << "Four")
        << (QStringList() // expectedSource
            << "One"
            << "Two"
            << "Three"
            << "Four");

    QTest::newRow("remove all [no sorting/filter]")
        << (QStringList()
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << -1 // no sorting
        << QString() // no filter
        << 0 // position
        << 5 // count
        << true // success
        << QStringList() // expectedProxy
        << QStringList(); // expectedSource

    QTest::newRow("remove one row past the end [no sorting/filter]")
        << (QStringList()
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << -1 // no sorting
        << QString() // no filter
        << 5 // position
        << 1 // count
        << false // success
        << (QStringList() // expectedProxy
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << (QStringList() // expectedSource
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five");

    QTest::newRow("remove row -1 [no sorting/filter]")
        << (QStringList()
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << -1 // no sorting
        << QString() // no filter
        << -1 // position
        << 1 // count
        << false // success
        << (QStringList() // expectedProxy
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << (QStringList() // expectedSource
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five");

    QTest::newRow("remove three rows in the middle [no sorting/filter]")
        << (QStringList()
            << "One"
            << "Two"
            << "Three"
            << "Four"
            << "Five")
        << -1 // no sorting
        << QString() // no filter
        << 1 // position
        << 3 // count
        << true // success
        << (QStringList() // expectedProxy
            << "One"
            << "Five")
        << (QStringList() // expectedSource
            << "One"
            << "Five");

    QTest::newRow("remove one row in the middle [ascending sorting, no filter]")
        << (QStringList()
            << "1"
            << "5"
            << "2"
            << "4"
            << "3")
        << static_cast<int>(Qt::AscendingOrder)
        << QString() // no filter
        << 2 // position
        << 1 // count
        << true // success
        << (QStringList() // expectedProxy
            << "1"
            << "2"
            << "4"
            << "5")
        << (QStringList() // expectedSource
            << "1"
            << "5"
            << "2"
            << "4");

    QTest::newRow("remove two rows in the middle [ascending sorting, no filter]")
        << (QStringList()
            << "1"
            << "5"
            << "2"
            << "4"
            << "3")
        << static_cast<int>(Qt::AscendingOrder)
        << QString() // no filter
        << 2 // position
        << 2 // count
        << true // success
        << (QStringList() // expectedProxy
            << "1"
            << "2"
            << "5")
        << (QStringList() // expectedSource
            << "1"
            << "5"
            << "2");

    QTest::newRow("remove two rows in the middle [descending sorting, no filter]")
        << (QStringList()
            << "1"
            << "5"
            << "2"
            << "4"
            << "3")
        << static_cast<int>(Qt::DescendingOrder)
        << QString() // no filter
        << 2 // position
        << 2 // count
        << true // success
        << (QStringList() // expectedProxy
            << "5"
            << "4"
            << "1")
        << (QStringList() // expectedSource
            << "1"
            << "5"
            << "4");

    QTest::newRow("remove one row in the middle [no sorting, filter=5|2|3]")
        << (QStringList()
            << "1"
            << "5"
            << "2"
            << "4"
            << "3")
        << -1 // no sorting
        << QString("5|2|3")
        << 1 // position
        << 1 // count
        << true // success
        << (QStringList() // expectedProxy
            << "5"
            << "3")
        << (QStringList() // expectedSource
            << "1"
            << "5"
            << "4"
            << "3");

    QTest::newRow("remove all [ascending sorting, no filter]")
        << (QStringList()
            << "1"
            << "5"
            << "2"
            << "4"
            << "3")
        << static_cast<int>(Qt::AscendingOrder)
        << QString() // no filter
        << 0 // position
        << 5 // count
        << true // success
        << QStringList() // expectedProxy
        << QStringList(); // expectedSource
}

void tst_QSortFilterProxyModel::removeRows()
{
    QFETCH(QStringList, initial);
    QFETCH(int, sortOrder);
    QFETCH(QString, filter);
    QFETCH(int, position);
    QFETCH(int, count);
    QFETCH(bool, success);
    QFETCH(QStringList, expectedProxy);
    QFETCH(QStringList, expectedSource);

    QStandardItemModel model;
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    // prepare model
    foreach (QString s, initial)
        model.appendRow(new QStandardItem(s));

    if (sortOrder != -1)
        proxy.sort(0, static_cast<Qt::SortOrder>(sortOrder));

    if (!filter.isEmpty())
        setupFilter(&proxy, filter);

    // remove the rows
    QCOMPARE(proxy.removeRows(position, count, QModelIndex()), success);
    QCOMPARE(model.rowCount(QModelIndex()), expectedSource.count());
    QCOMPARE(proxy.rowCount(QModelIndex()), expectedProxy.count());

    // make sure the model is correct after remove
    for (int row = 0; row < model.rowCount(QModelIndex()); ++row)
        QCOMPARE(model.item(row)->text(), expectedSource.at(row));

    // make sure the proxy is correct after remove
    for (int row = 0; row < proxy.rowCount(QModelIndex()); ++row) {
        QModelIndex index = proxy.index(row, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole).toString(), expectedProxy.at(row));
    }
}

class MyFilteredColumnProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    MyFilteredColumnProxyModel(FilterType filterType, QObject *parent = 0) :
        QSortFilterProxyModel(parent),
        m_filterType(filterType)
    { }

protected:
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex &) const
    {
        QString key = sourceModel()->headerData(sourceColumn, Qt::Horizontal).toString();
        bool result = false;
        switch (m_filterType) {
        case FilterType::RegExp:
            result = key.contains(filterRegExp());
            break;
        case FilterType::RegularExpression:
            result = key.contains(filterRegularExpression());
            break;
        }
        return result;
    }

private:
    FilterType m_filterType;
};

void tst_QSortFilterProxyModel::removeColumns_data()
{
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<QString>("filter");
    QTest::addColumn<int>("position");
    QTest::addColumn<int>("count");
    QTest::addColumn<bool>("success");
    QTest::addColumn<QStringList>("expectedProxy");
    QTest::addColumn<QStringList>("expectedSource");

    QTest::newRow("remove one column in the middle [no filter]")
        << (QStringList()
            << "1"
            << "2"
            << "3"
            << "4"
            << "5")
        << QString() // no filter
        << 2 // position
        << 1 // count
        << true // success
        << (QStringList() // expectedProxy
            << "1"
            << "2"
            << "4"
            << "5")
        << (QStringList() // expectedSource
            << "1"
            << "2"
            << "4"
            << "5");

    QTest::newRow("remove one column in the end [no filter]")
        << (QStringList()
            << "1"
            << "2"
            << "3"
            << "4"
            << "5")
        << QString() // no filter
        << 4 // position
        << 1 // count
        << true // success
        << (QStringList() // expectedProxy
            << "1"
            << "2"
            << "3"
            << "4")
        << (QStringList() // expectedSource
            << "1"
            << "2"
            << "3"
            << "4");

    QTest::newRow("remove one column past the end [no filter]")
        << (QStringList()
            << "1"
            << "2"
            << "3"
            << "4"
            << "5")
        << QString() // no filter
        << 5 // position
        << 1 // count
        << false // success
        << (QStringList() // expectedProxy
            << "1"
            << "2"
            << "3"
            << "4"
            << "5")
        << (QStringList() // expectedSource
            << "1"
            << "2"
            << "3"
            << "4"
            << "5");

    QTest::newRow("remove column -1 [no filter]")
        << (QStringList()
            << "1"
            << "2"
            << "3"
            << "4"
            << "5")
        << QString() // no filter
        << -1 // position
        << 1 // count
        << false // success
        << (QStringList() // expectedProxy
            << "1"
            << "2"
            << "3"
            << "4"
            << "5")
        << (QStringList() // expectedSource
            << "1"
            << "2"
            << "3"
            << "4"
            << "5");

    QTest::newRow("remove all columns [no filter]")
        << (QStringList()
            << "1"
            << "2"
            << "3"
            << "4"
            << "5")
        << QString() // no filter
        << 0 // position
        << 5 // count
        << true // success
        << QStringList() // expectedProxy
        << QStringList(); // expectedSource

    QTest::newRow("remove one column in the middle [filter=1|3|5]")
        << (QStringList()
            << "1"
            << "2"
            << "3"
            << "4"
            << "5")
        << QString("1|3|5")
        << 1 // position
        << 1 // count
        << true // success
        << (QStringList() // expectedProxy
            << "1"
            << "5")
        << (QStringList() // expectedSource
            << "1"
            << "2"
            << "4"
            << "5");

    QTest::newRow("remove one column in the end [filter=1|3|5]")
        << (QStringList()
            << "1"
            << "2"
            << "3"
            << "4"
            << "5")
        << QString("1|3|5")
        << 2 // position
        << 1 // count
        << true // success
        << (QStringList() // expectedProxy
            << "1"
            << "3")
        << (QStringList() // expectedSource
            << "1"
            << "2"
            << "3"
            << "4");

    QTest::newRow("remove one column past the end [filter=1|3|5]")
        << (QStringList()
            << "1"
            << "2"
            << "3"
            << "4"
            << "5")
        << QString("1|3|5")
        << 3 // position
        << 1 // count
        << false // success
        << (QStringList() // expectedProxy
            << "1"
            << "3"
            << "5")
        << (QStringList() // expectedSource
            << "1"
            << "2"
            << "3"
            << "4"
            << "5");

    QTest::newRow("remove all columns [filter=1|3|5]")
        << (QStringList()
            << "1"
            << "2"
            << "3"
            << "4"
            << "5")
        << QString("1|3|5")
        << 0 // position
        << 3 // count
        << true // success
        << QStringList() // expectedProxy
        << (QStringList() // expectedSource
            << "2"
            << "4");
}

void tst_QSortFilterProxyModel::removeColumns()
{
    QFETCH(QStringList, initial);
    QFETCH(QString, filter);
    QFETCH(int, position);
    QFETCH(int, count);
    QFETCH(bool, success);
    QFETCH(QStringList, expectedProxy);
    QFETCH(QStringList, expectedSource);

    QStandardItemModel model;
    MyFilteredColumnProxyModel proxy(m_filterType);
    proxy.setSourceModel(&model);
    if (!filter.isEmpty())
        setupFilter(&proxy, filter);

    // prepare model
    model.setHorizontalHeaderLabels(initial);

    // remove the columns
    QCOMPARE(proxy.removeColumns(position, count, QModelIndex()), success);
    QCOMPARE(model.columnCount(QModelIndex()), expectedSource.count());
    QCOMPARE(proxy.columnCount(QModelIndex()), expectedProxy.count());

    // make sure the model is correct after remove
    for (int col = 0; col < model.columnCount(QModelIndex()); ++col)
        QCOMPARE(model.horizontalHeaderItem(col)->text(), expectedSource.at(col));

    // make sure the proxy is correct after remove
    for (int col = 0; col < proxy.columnCount(QModelIndex()); ++col) {
        QCOMPARE(proxy.headerData(col, Qt::Horizontal, Qt::DisplayRole).toString(),
                 expectedProxy.at(col));
    }
}

void tst_QSortFilterProxyModel::filterColumns_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<bool>("data");

    QTest::newRow("all") << "a"
                         << (QStringList()
                             << "delta"
                             << "yankee"
                             << "bravo"
                             << "lima")
                         << true;

    QTest::newRow("some") << "lie"
                          << (QStringList()
                              << "charlie"
                              << "juliet"
                              << "tango"
                              << "hotel")
                          << true;

    QTest::newRow("nothing") << "zoo"
                             << (QStringList()
                                 << "foxtrot"
                                 << "uniform"
                                 << "alpha"
                                 << "golf")
                             << false;
}

void tst_QSortFilterProxyModel::filterColumns()
{
    QFETCH(QString, pattern);
    QFETCH(QStringList, initial);
    QFETCH(bool, data);
    // prepare model
    m_model->setColumnCount(initial.count());
    m_model->setRowCount(1);
    QCOMPARE(m_model->columnCount(QModelIndex()), initial.count());
    QCOMPARE(m_model->rowCount(QModelIndex()), 1);
    // set data
    QCOMPARE(m_model->rowCount(QModelIndex()), 1);
    for (int col = 0; col < m_model->columnCount(QModelIndex()); ++col) {
        QModelIndex index = m_model->index(0, col, QModelIndex());
        m_model->setData(index, initial.at(col), Qt::DisplayRole);
    }
    setupFilter(m_proxy, pattern);

    m_proxy->setFilterKeyColumn(-1);
    // make sure the model is unchanged
    for (int col = 0; col < m_model->columnCount(QModelIndex()); ++col) {
        QModelIndex index = m_model->index(0, col, QModelIndex());
        QCOMPARE(m_model->data(index, Qt::DisplayRole).toString(), initial.at(col));
    }
    // make sure the proxy is filtered
    QModelIndex index = m_proxy->index(0, 0, QModelIndex());
    QCOMPARE(index.isValid(), data);
}

void tst_QSortFilterProxyModel::filter_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("flat") << "e"
                          << (QStringList()
                           << "delta"
                           << "yankee"
                           << "bravo"
                           << "lima"
                           << "charlie"
                           << "juliet"
                           << "tango"
                           << "hotel"
                           << "uniform"
                           << "alpha"
                           << "echo"
                           << "golf"
                           << "quebec"
                           << "foxtrot"
                           << "india"
                           << "romeo"
                           << "november"
                           << "oskar"
                           << "zulu"
                           << "kilo"
                           << "whiskey"
                           << "mike"
                           << "papa"
                           << "sierra"
                           << "xray"
                           << "viktor")
                       << (QStringList()
                           << "delta"
                           << "yankee"
                           << "charlie"
                           << "juliet"
                           << "hotel"
                           << "echo"
                           << "quebec"
                           << "romeo"
                           << "november"
                           << "whiskey"
                           << "mike"
                           << "sierra");
}

void tst_QSortFilterProxyModel::filter()
{
    QFETCH(QString, pattern);
    QFETCH(QStringList, initial);
    QFETCH(QStringList, expected);

    // prepare model
    QVERIFY(m_model->insertRows(0, initial.count(), QModelIndex()));
    QCOMPARE(m_model->rowCount(QModelIndex()), initial.count());
    // set data
    QCOMPARE(m_model->columnCount(QModelIndex()), 1);
    for (int row = 0; row < m_model->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_model->index(row, 0, QModelIndex());
        m_model->setData(index, initial.at(row), Qt::DisplayRole);
    }
    setupFilter(m_proxy, pattern);
    // make sure the proxy is unfiltered
    QCOMPARE(m_proxy->columnCount(QModelIndex()), 1);
    QCOMPARE(m_proxy->rowCount(QModelIndex()), expected.count());
    // make sure the model is unchanged
    for (int row = 0; row < m_model->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_model->index(row, 0, QModelIndex());
        QCOMPARE(m_model->data(index, Qt::DisplayRole).toString(), initial.at(row));
    }
    // make sure the proxy is filtered
    for (int row = 0; row < m_proxy->rowCount(QModelIndex()); ++row) {
        QModelIndex index = m_proxy->index(row, 0, QModelIndex());
        QCOMPARE(m_proxy->data(index, Qt::DisplayRole).toString(), expected.at(row));
    }
}

void tst_QSortFilterProxyModel::filterHierarchy_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QStringList>("initial");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("flat") << ".*oo"
        << (QStringList()
            << "foo" << "boo" << "baz" << "moo" << "laa" << "haa")
        << (QStringList()
            << "foo" << "boo" << "moo");

    QTest::newRow("simple hierarchy") << "b.*z"
        << (QStringList() << "baz" << "<" << "boz" << "<" << "moo" << ">" << ">")
        << (QStringList() << "baz" << "<" << "boz" << ">");
}

void tst_QSortFilterProxyModel::filterHierarchy()
{
    QFETCH(QString, pattern);
    QFETCH(QStringList, initial);
    QFETCH(QStringList, expected);
    buildHierarchy(initial, m_model);
    setupFilter(m_proxy, pattern);
    checkHierarchy(initial, m_model);
    checkHierarchy(expected, m_proxy);
}

void tst_QSortFilterProxyModel::buildHierarchy(const QStringList &l, QAbstractItemModel *m)
{
    int ind = 0;
    int row = 0;
    QStack<int> row_stack;
    QModelIndex parent;
    QStack<QModelIndex> parent_stack;
    for (int i = 0; i < l.count(); ++i) {
        QString token = l.at(i);
        if (token == QLatin1String("<")) { // start table
            ++ind;
            parent_stack.push(parent);
            row_stack.push(row);
            parent = m->index(row - 1, 0, parent);
            row = 0;
            QVERIFY(m->insertColumns(0, 1, parent)); // add column
        } else if (token == QLatin1String(">")) { // end table
            --ind;
            parent = parent_stack.pop();
            row = row_stack.pop();
        } else { // append row
            QVERIFY(m->insertRows(row, 1, parent));
            QModelIndex index = m->index(row, 0, parent);
            QVERIFY(index.isValid());
            m->setData(index, token, Qt::DisplayRole);
            ++row;
        }
    }
}

void tst_QSortFilterProxyModel::checkHierarchy(const QStringList &l, const QAbstractItemModel *m)
{
    int row = 0;
    int indent = 0;
    QStack<int> row_stack;
    QModelIndex parent;
    QStack<QModelIndex> parent_stack;
    for (int i = 0; i < l.count(); ++i) {
        QString token = l.at(i);
        if (token == QLatin1String("<")) { // start table
            ++indent;
            parent_stack.push(parent);
            row_stack.push(row);
            parent = m->index(row - 1, 0, parent);
            QVERIFY(parent.isValid());
            row = 0;
        } else if (token == QLatin1String(">")) { // end table
            --indent;
            parent = parent_stack.pop();
            row = row_stack.pop();
        } else { // compare row
            QModelIndex index = m->index(row, 0, parent);
            QVERIFY(index.isValid());
            QString str =  m->data(index, Qt::DisplayRole).toString();
            QCOMPARE(str, token);
            ++row;
        }
    }
}

void tst_QSortFilterProxyModel::setupFilter(QSortFilterProxyModel *model, const QString& pattern)
{
    switch (m_filterType) {
    case FilterType::RegExp:
        model->setFilterRegExp(pattern);
        break;
    case FilterType::RegularExpression:
        model->setFilterRegularExpression(pattern);
        break;
    }
}

class TestModel: public QAbstractTableModel
{
public:
    int rowCount(const QModelIndex &) const { return 10000; }
    int columnCount(const QModelIndex &) const { return 1; }
    QVariant data(const QModelIndex &index, int role) const
    {
        if (role != Qt::DisplayRole)
            return QVariant();
        return QString::number(index.row());
    }
};

void tst_QSortFilterProxyModel::filterTable()
{
    TestModel model;
    QSortFilterProxyModel filter;
    filter.setSourceModel(&model);
    setupFilter(&filter, QLatin1String("9"));

    for (int i = 0; i < filter.rowCount(); ++i)
        QVERIFY(filter.data(filter.index(i, 0)).toString().contains(QLatin1Char('9')));
}

void tst_QSortFilterProxyModel::insertAfterSelect()
{
    QStandardItemModel model(10, 2);
    for (int i = 0; i<10;i++)
        model.setData(model.index(i, 0), QVariant(i));
    QSortFilterProxyModel filter;
    filter.setSourceModel(&model);
    QTreeView view;
    view.setModel(&filter);
    view.show();
    QModelIndex firstIndex = filter.mapFromSource(model.index(0, 0, QModelIndex()));
    QCOMPARE(firstIndex.model(), (const QAbstractItemModel *)view.model());
    QVERIFY(firstIndex.isValid());
    int itemOffset = view.visualRect(firstIndex).width() / 2;
    QPoint p(itemOffset, 1);
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QVERIFY(view.selectionModel()->selectedIndexes().size() > 0);
    model.insertRows(5, 1, QModelIndex());
    QVERIFY(view.selectionModel()->selectedIndexes().size() > 0); // Should still have a selection
}

void tst_QSortFilterProxyModel::removeAfterSelect()
{
    QStandardItemModel model(10, 2);
    for (int i = 0; i<10;i++)
        model.setData(model.index(i, 0), QVariant(i));
    QSortFilterProxyModel filter;
    filter.setSourceModel(&model);
    QTreeView view;
    view.setModel(&filter);
    view.show();
    QModelIndex firstIndex = filter.mapFromSource(model.index(0, 0, QModelIndex()));
    QCOMPARE(firstIndex.model(), (const QAbstractItemModel *)view.model());
    QVERIFY(firstIndex.isValid());
    int itemOffset = view.visualRect(firstIndex).width() / 2;
    QPoint p(itemOffset, 1);
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QVERIFY(view.selectionModel()->selectedIndexes().size() > 0);
    model.removeRows(5, 1, QModelIndex());
    QVERIFY(view.selectionModel()->selectedIndexes().size() > 0); // Should still have a selection
}

void tst_QSortFilterProxyModel::filterCurrent()
{
    QStandardItemModel model(2, 1);
    model.setData(model.index(0, 0), QString("AAA"));
    model.setData(model.index(1, 0), QString("BBB"));
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    QTreeView view;

    view.show();
    view.setModel(&proxy);
    QSignalSpy spy(view.selectionModel(), &QItemSelectionModel::currentChanged);
    QVERIFY(spy.isValid());

    view.setCurrentIndex(proxy.index(0, 0));
    QCOMPARE(spy.count(), 1);
    setupFilter(&proxy, QLatin1String("^B"));
    QCOMPARE(spy.count(), 2);
}

void tst_QSortFilterProxyModel::filter_qtbug30662()
{
    QStringListModel model;
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    // make sure the filter does not match any entry
    setupFilter(&proxy, QLatin1String("[0-9]+"));

    QStringList slSource;
    slSource << "z" << "x" << "a" << "b";

    proxy.setDynamicSortFilter(true);
    proxy.sort(0);
    model.setStringList(slSource);

    // without fix for QTBUG-30662 this will make all entries visible - but unsorted
    setupFilter(&proxy, QLatin1String("[a-z]+"));

    QStringList slResult;
    for (int i = 0; i < proxy.rowCount(); ++i)
      slResult.append(proxy.index(i, 0).data().toString());

    slSource.sort();
    QCOMPARE(slResult, slSource);
}

void tst_QSortFilterProxyModel::changeSourceLayout()
{
    QStandardItemModel model(2, 1);
    model.setData(model.index(0, 0), QString("b"));
    model.setData(model.index(1, 0), QString("a"));
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    QList<QPersistentModelIndex> persistentSourceIndexes;
    QList<QPersistentModelIndex> persistentProxyIndexes;
    for (int row = 0; row < model.rowCount(); ++row) {
        persistentSourceIndexes.append(model.index(row, 0));
        persistentProxyIndexes.append(proxy.index(row, 0));
    }

    // change layout of source model
    model.sort(0, Qt::AscendingOrder);

    for (int row = 0; row < model.rowCount(); ++row) {
        QCOMPARE(persistentProxyIndexes.at(row).row(),
                 persistentSourceIndexes.at(row).row());
    }
}

void tst_QSortFilterProxyModel::changeSourceLayoutFilteredOut()
{
    QStandardItemModel model(2, 1);
    model.setData(model.index(0, 0), QString("b"));
    model.setData(model.index(1, 0), QString("a"));
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    int beforeSortFilter = proxy.rowCount();

    QSignalSpy removeSpy(&proxy, &QSortFilterProxyModel::rowsRemoved);
    // Filter everything out
    setupFilter(&proxy, QLatin1String("c"));

    QCOMPARE(removeSpy.count(), 1);
    QCOMPARE(0, proxy.rowCount());

    // change layout of source model
    model.sort(0, Qt::AscendingOrder);

    QSignalSpy insertSpy(&proxy, &QSortFilterProxyModel::rowsInserted);
    // Remove filter; we expect an insert
    setupFilter(&proxy, "");

    QCOMPARE(insertSpy.count(), 1);
    QCOMPARE(beforeSortFilter, proxy.rowCount());
}

void tst_QSortFilterProxyModel::removeSourceRows_data()
{
    QTest::addColumn<QStringList>("sourceItems");
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("count");
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<IntPairList>("expectedRemovedProxyIntervals");
    QTest::addColumn<QStringList>("expectedProxyItems");

    QTest::newRow("remove one, no sorting")
        << (QStringList() << "a" << "b") // sourceItems
        << 0 // start
        << 1 // count
        << -1 // sortOrder (no sorting)
        << (IntPairList() << IntPair(0, 0)) // expectedRemovedIntervals
        << (QStringList() << "b") // expectedProxyItems
        ;
    QTest::newRow("remove one, ascending sort (same order)")
        << (QStringList() << "a" << "b") // sourceItems
        << 0 // start
        << 1 // count
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << (IntPairList() << IntPair(0, 0)) // expectedRemovedIntervals
        << (QStringList() << "b") // expectedProxyItems
        ;
    QTest::newRow("remove one, ascending sort (reverse order)")
        << (QStringList() << "b" << "a") // sourceItems
        << 0 // start
        << 1 // count
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << (IntPairList() << IntPair(1, 1)) // expectedRemovedIntervals
        << (QStringList() << "a") // expectedProxyItems
        ;
    QTest::newRow("remove two, multiple proxy intervals")
        << (QStringList() << "c" << "d" << "a" << "b") // sourceItems
        << 1 // start
        << 2 // count
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << (IntPairList() << IntPair(3, 3) << IntPair(0, 0)) // expectedRemovedIntervals
        << (QStringList() << "b" << "c") // expectedProxyItems
        ;
    QTest::newRow("remove three, multiple proxy intervals")
        << (QStringList() << "b" << "d" << "f" << "a" << "c" << "e") // sourceItems
        << 3 // start
        << 3 // count
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << (IntPairList() << IntPair(4, 4) << IntPair(2, 2) << IntPair(0, 0)) // expectedRemovedIntervals
        << (QStringList() << "b" << "d" << "f") // expectedProxyItems
        ;
    QTest::newRow("remove all, single proxy intervals")
        << (QStringList() << "a" << "b" << "c" << "d" << "e" << "f") // sourceItems
        << 0 // start
        << 6 // count
        << static_cast<int>(Qt::DescendingOrder) // sortOrder
        << (IntPairList() << IntPair(0, 5)) // expectedRemovedIntervals
        << QStringList() // expectedProxyItems
        ;
}

// Check that correct proxy model rows are removed when rows are removed
// from the source model
void tst_QSortFilterProxyModel::removeSourceRows()
{
    QFETCH(QStringList, sourceItems);
    QFETCH(int, start);
    QFETCH(int, count);
    QFETCH(int, sortOrder);
    QFETCH(IntPairList, expectedRemovedProxyIntervals);
    QFETCH(QStringList, expectedProxyItems);

    QStandardItemModel model;
    QSortFilterProxyModel proxy;

    proxy.setSourceModel(&model);
    model.insertColumns(0, 1);
    model.insertRows(0, sourceItems.count());

    for (int i = 0; i < sourceItems.count(); ++i) {
        QModelIndex sindex = model.index(i, 0, QModelIndex());
        model.setData(sindex, sourceItems.at(i), Qt::DisplayRole);
        QModelIndex pindex = proxy.index(i, 0, QModelIndex());
        QCOMPARE(proxy.data(pindex, Qt::DisplayRole), model.data(sindex, Qt::DisplayRole));
    }

    if (sortOrder != -1)
        proxy.sort(0, static_cast<Qt::SortOrder>(sortOrder));
    (void)proxy.rowCount(QModelIndex()); // force mapping

    QSignalSpy removeSpy(&proxy, &QSortFilterProxyModel::rowsRemoved);
    QSignalSpy insertSpy(&proxy, &QSortFilterProxyModel::rowsInserted);
    QSignalSpy aboutToRemoveSpy(&proxy, &QSortFilterProxyModel::rowsAboutToBeRemoved);
    QSignalSpy aboutToInsertSpy(&proxy, &QSortFilterProxyModel::rowsAboutToBeInserted);

    QVERIFY(removeSpy.isValid());
    QVERIFY(insertSpy.isValid());
    QVERIFY(aboutToRemoveSpy.isValid());
    QVERIFY(aboutToInsertSpy.isValid());

    model.removeRows(start, count, QModelIndex());

    QCOMPARE(aboutToRemoveSpy.count(), expectedRemovedProxyIntervals.count());
    for (int i = 0; i < aboutToRemoveSpy.count(); ++i) {
        QList<QVariant> args = aboutToRemoveSpy.at(i);
        QCOMPARE(args.at(1).type(), QVariant::Int);
        QCOMPARE(args.at(2).type(), QVariant::Int);
        QCOMPARE(args.at(1).toInt(), expectedRemovedProxyIntervals.at(i).first);
        QCOMPARE(args.at(2).toInt(), expectedRemovedProxyIntervals.at(i).second);
    }
    QCOMPARE(removeSpy.count(), expectedRemovedProxyIntervals.count());
    for (int i = 0; i < removeSpy.count(); ++i) {
        QList<QVariant> args = removeSpy.at(i);
        QCOMPARE(args.at(1).type(), QVariant::Int);
        QCOMPARE(args.at(2).type(), QVariant::Int);
        QCOMPARE(args.at(1).toInt(), expectedRemovedProxyIntervals.at(i).first);
        QCOMPARE(args.at(2).toInt(), expectedRemovedProxyIntervals.at(i).second);
    }

    QCOMPARE(insertSpy.count(), 0);
    QCOMPARE(aboutToInsertSpy.count(), 0);

    QCOMPARE(proxy.rowCount(QModelIndex()), expectedProxyItems.count());
    for (int i = 0; i < expectedProxyItems.count(); ++i) {
        QModelIndex pindex = proxy.index(i, 0, QModelIndex());
        QCOMPARE(proxy.data(pindex, Qt::DisplayRole).toString(), expectedProxyItems.at(i));
    }
}

void tst_QSortFilterProxyModel::insertSourceRows_data()
{
    QTest::addColumn<QStringList>("sourceItems");
    QTest::addColumn<int>("start");
    QTest::addColumn<QStringList>("newItems");
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<QStringList>("proxyItems");

    QTest::newRow("insert (1)")
        << (QStringList() << "c" << "b") // sourceItems
        << 1 // start
        << (QStringList() << "a") // newItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << (QStringList() << "a" << "b" << "c") // proxyItems
        ;

    QTest::newRow("insert (2)")
        << (QStringList() << "d" << "b" << "c") // sourceItems
        << 3 // start
        << (QStringList() << "a") // newItems
        << static_cast<int>(Qt::DescendingOrder) // sortOrder
        << (QStringList() << "d" << "c" << "b" << "a") // proxyItems
        ;
}

// Check that rows are inserted at correct position in proxy model when
// rows are inserted into the source model
void tst_QSortFilterProxyModel::insertSourceRows()
{
    QFETCH(QStringList, sourceItems);
    QFETCH(int, start);
    QFETCH(QStringList, newItems);
    QFETCH(int, sortOrder);
    QFETCH(QStringList, proxyItems);

    QStandardItemModel model;
    QSortFilterProxyModel proxy;
    proxy.setDynamicSortFilter(true);

    proxy.setSourceModel(&model);
    model.insertColumns(0, 1);
    model.insertRows(0, sourceItems.count());

    for (int i = 0; i < sourceItems.count(); ++i) {
        QModelIndex index = model.index(i, 0, QModelIndex());
        model.setData(index, sourceItems.at(i), Qt::DisplayRole);
    }

    proxy.sort(0, static_cast<Qt::SortOrder>(sortOrder));
    (void)proxy.rowCount(QModelIndex()); // force mapping

    model.insertRows(start, newItems.size(), QModelIndex());

    QCOMPARE(proxy.rowCount(QModelIndex()), proxyItems.count());
    for (int i = 0; i < newItems.count(); ++i) {
        QModelIndex index = model.index(start + i, 0, QModelIndex());
        model.setData(index, newItems.at(i), Qt::DisplayRole);
    }

    for (int i = 0; i < proxyItems.count(); ++i) {
        QModelIndex index = proxy.index(i, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole).toString(), proxyItems.at(i));
    }
}

void tst_QSortFilterProxyModel::changeFilter_data()
{
    QTest::addColumn<QStringList>("sourceItems");
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<QString>("initialFilter");
    QTest::addColumn<IntPairList>("initialRemoveIntervals");
    QTest::addColumn<QStringList>("initialProxyItems");
    QTest::addColumn<QString>("finalFilter");
    QTest::addColumn<IntPairList>("finalRemoveIntervals");
    QTest::addColumn<IntPairList>("insertIntervals");
    QTest::addColumn<QStringList>("finalProxyItems");

    QTest::newRow("filter (1)")
        << (QStringList() << "a" << "b" << "c" << "d" << "e" << "f") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "a|b|c" // initialFilter
        << (IntPairList() << IntPair(3, 5)) // initialRemoveIntervals
        << (QStringList() << "a" << "b" << "c") // initialProxyItems
        << "b|d|f" // finalFilter
        << (IntPairList() << IntPair(2, 2) << IntPair(0, 0)) // finalRemoveIntervals
        << (IntPairList() << IntPair(1, 2)) // insertIntervals
        << (QStringList() << "b" << "d" << "f") // finalProxyItems
        ;

    QTest::newRow("filter (2)")
        << (QStringList() << "a" << "b" << "c" << "d" << "e" << "f") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "a|c|e" // initialFilter
        << (IntPairList() << IntPair(5, 5) << IntPair(3, 3) << IntPair(1, 1)) // initialRemoveIntervals
        << (QStringList() << "a" << "c" << "e") // initialProxyItems
        << "" // finalFilter
        << IntPairList() // finalRemoveIntervals
        << (IntPairList() << IntPair(3, 3) << IntPair(2, 2) << IntPair(1, 1)) // insertIntervals
        << (QStringList() << "a" << "b" << "c" << "d" << "e" << "f") // finalProxyItems
        ;

    QTest::newRow("filter (3)")
        << (QStringList() << "a" << "b" << "c") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "a" // initialFilter
        << (IntPairList() << IntPair(1, 2)) // initialRemoveIntervals
        << (QStringList() << "a") // initialProxyItems
        << "a" // finalFilter
        << IntPairList() // finalRemoveIntervals
        << IntPairList() // insertIntervals
        << (QStringList() << "a") // finalProxyItems
        ;
}

// Check that rows are added/removed when filter changes
void tst_QSortFilterProxyModel::changeFilter()
{
    QFETCH(QStringList, sourceItems);
    QFETCH(int, sortOrder);
    QFETCH(QString, initialFilter);
    QFETCH(IntPairList, initialRemoveIntervals);
    QFETCH(QStringList, initialProxyItems);
    QFETCH(QString, finalFilter);
    QFETCH(IntPairList, finalRemoveIntervals);
    QFETCH(IntPairList, insertIntervals);
    QFETCH(QStringList, finalProxyItems);

    QStandardItemModel model;
    QSortFilterProxyModel proxy;

    proxy.setSourceModel(&model);
    model.insertColumns(0, 1);
    model.insertRows(0, sourceItems.count());

    for (int i = 0; i < sourceItems.count(); ++i) {
        QModelIndex index = model.index(i, 0, QModelIndex());
        model.setData(index, sourceItems.at(i), Qt::DisplayRole);
    }

    proxy.sort(0, static_cast<Qt::SortOrder>(sortOrder));
    (void)proxy.rowCount(QModelIndex()); // force mapping

    QSignalSpy initialRemoveSpy(&proxy, &QSortFilterProxyModel::rowsRemoved);
    QSignalSpy initialInsertSpy(&proxy, &QSortFilterProxyModel::rowsInserted);

    QVERIFY(initialRemoveSpy.isValid());
    QVERIFY(initialInsertSpy.isValid());

    setupFilter(&proxy, initialFilter);

    QCOMPARE(initialRemoveSpy.count(), initialRemoveIntervals.count());
    QCOMPARE(initialInsertSpy.count(), 0);
    for (int i = 0; i < initialRemoveSpy.count(); ++i) {
        QList<QVariant> args = initialRemoveSpy.at(i);
        QCOMPARE(args.at(1).type(), QVariant::Int);
        QCOMPARE(args.at(2).type(), QVariant::Int);
        QCOMPARE(args.at(1).toInt(), initialRemoveIntervals.at(i).first);
        QCOMPARE(args.at(2).toInt(), initialRemoveIntervals.at(i).second);
    }

    QCOMPARE(proxy.rowCount(QModelIndex()), initialProxyItems.count());
    for (int i = 0; i < initialProxyItems.count(); ++i) {
        QModelIndex index = proxy.index(i, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole).toString(), initialProxyItems.at(i));
    }

    QSignalSpy finalRemoveSpy(&proxy, &QSortFilterProxyModel::rowsRemoved);
    QSignalSpy finalInsertSpy(&proxy, &QSortFilterProxyModel::rowsInserted);

    QVERIFY(finalRemoveSpy.isValid());
    QVERIFY(finalInsertSpy.isValid());

    setupFilter(&proxy, finalFilter);

    QCOMPARE(finalRemoveSpy.count(), finalRemoveIntervals.count());
    for (int i = 0; i < finalRemoveSpy.count(); ++i) {
        QList<QVariant> args = finalRemoveSpy.at(i);
        QCOMPARE(args.at(1).type(), QVariant::Int);
        QCOMPARE(args.at(2).type(), QVariant::Int);
        QCOMPARE(args.at(1).toInt(), finalRemoveIntervals.at(i).first);
        QCOMPARE(args.at(2).toInt(), finalRemoveIntervals.at(i).second);
    }

    QCOMPARE(finalInsertSpy.count(), insertIntervals.count());
    for (int i = 0; i < finalInsertSpy.count(); ++i) {
        QList<QVariant> args = finalInsertSpy.at(i);
        QCOMPARE(args.at(1).type(), QVariant::Int);
        QCOMPARE(args.at(2).type(), QVariant::Int);
        QCOMPARE(args.at(1).toInt(), insertIntervals.at(i).first);
        QCOMPARE(args.at(2).toInt(), insertIntervals.at(i).second);
    }

    QCOMPARE(proxy.rowCount(QModelIndex()), finalProxyItems.count());
    for (int i = 0; i < finalProxyItems.count(); ++i) {
        QModelIndex index = proxy.index(i, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole).toString(), finalProxyItems.at(i));
    }
}

void tst_QSortFilterProxyModel::changeSourceData_data()
{
    QTest::addColumn<QStringList>("sourceItems");
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<QString>("filter");
    QTest::addColumn<QStringList>("expectedInitialProxyItems");
    QTest::addColumn<bool>("dynamic");
    QTest::addColumn<int>("row");
    QTest::addColumn<QString>("newValue");
    QTest::addColumn<IntPairList>("removeIntervals");
    QTest::addColumn<IntPairList>("insertIntervals");
    QTest::addColumn<int>("expectedDataChangedRow"); // -1 if no dataChanged signal expected
    QTest::addColumn<bool>("expectedLayoutChanged");
    QTest::addColumn<QStringList>("proxyItems");

    QTest::newRow("move_to_end_ascending")
        << (QStringList() << "c" << "b" << "a") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "" // filter
        << (QStringList() << "a" << "b" << "c") // expectedInitialProxyItems
        << true // dynamic
        << 2 // row
        << "z" // newValue
        << IntPairList() // removeIntervals
        << IntPairList() // insertIntervals
        << 2 // dataChanged(row 2) is emitted, see comment "Make sure we also emit dataChanged for the rows" in the source code (unclear why, though)
        << true // layoutChanged
        << (QStringList() << "b" << "c" << "z") // proxyItems
        ;

    QTest::newRow("move_to_end_descending")
        << (QStringList() << "b" << "c" << "z") // sourceItems
        << static_cast<int>(Qt::DescendingOrder) // sortOrder
        << "" // filter
        << (QStringList() << "z" << "c" << "b") // expectedInitialProxyItems
        << true // dynamic
        << 1 // row
        << "a" // newValue
        << IntPairList() // removeIntervals
        << IntPairList() // insertIntervals
        << 2 // dataChanged(row 2) is emitted, see comment "Make sure we also emit dataChanged for the rows" in the source code (unclear why, though)
        << true // layoutChanged
        << (QStringList() << "z" << "b" << "a") // proxyItems
        ;

    QTest::newRow("no_op_change")
        << (QStringList() << "a" << "b") // sourceItems
        << static_cast<int>(Qt::DescendingOrder) // sortOrder
        << "" // filter
        << (QStringList() << "b" << "a") // expectedInitialProxyItems
        << true // dynamic
        << 0 // row
        << "a" // newValue
        << IntPairList() // removeIntervals
        << IntPairList() // insertIntervals
        << -1 // no dataChanged signal
        << false // layoutChanged
        << (QStringList() << "b" << "a") // proxyItems
        ;

    QTest::newRow("no_effect_on_filtering")
        << (QStringList() << "a" << "b") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "" // filter
        << (QStringList() << "a" << "b") // expectedInitialProxyItems
        << true // dynamic
        << 1 // row
        << "z" // newValue
        << IntPairList() // removeIntervals
        << IntPairList() // insertIntervals
        << 1 // expectedDataChangedRow
        << false // layoutChanged
        << (QStringList() << "a" << "z") // proxyItems
        ;

    QTest::newRow("filtered_out_value_stays_out")
        << (QStringList() << "a" << "b" << "c" << "d") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "a|c" // filter
        << (QStringList() << "a" << "c") // expectedInitialProxyItems
        << true // dynamic
        << 1 // row
        << "x" // newValue
        << IntPairList() // removeIntervals
        << IntPairList() // insertIntervals
        << -1 // no dataChanged signal
        << false // layoutChanged
        << (QStringList() << "a" << "c") // proxyItems
        ;

    QTest::newRow("filtered_out_now_matches")
        << (QStringList() << "a" << "b" << "c" << "d") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "a|c|x" // filter
        << (QStringList() << "a" << "c") // expectedInitialProxyItems
        << true // dynamic
        << 1 // row
        << "x" // newValue
        << IntPairList() // removeIntervals
        << (IntPairList() << IntPair(2, 2)) // insertIntervals
        << -1 // no dataChanged signal
        << false // layoutChanged
        << (QStringList() << "a" << "c" << "x") // proxyItems
        ;

    QTest::newRow("value_is_now_filtered_out")
        << (QStringList() << "a" << "b" << "c" << "d") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "a|c" // filter
        << (QStringList() << "a" << "c") // expectedInitialProxyItems
        << true // dynamic
        << 2 // row
        << "x" // newValue
        << (IntPairList() << IntPair(1, 1)) // removeIntervals
        << IntPairList() // insertIntervals
        << -1 // no dataChanged signal
        << false // layoutChanged
        << (QStringList() << "a") // proxyItems
        ;

    QTest::newRow("non_dynamic_filter_does_not_update_sort")
        << (QStringList() << "c" << "b" << "a") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "" // filter
        << (QStringList() << "a" << "b" << "c") // expectedInitialProxyItems
        << false // dynamic
        << 2 // row
        << "x" // newValue
        << IntPairList() // removeIntervals
        << IntPairList() // insertIntervals
        << 0 // expectedDataChangedRow
        << false // layoutChanged
        << (QStringList() << "x" << "b" << "c") // proxyItems
        ;
}

void tst_QSortFilterProxyModel::changeSourceData()
{
    QFETCH(QStringList, sourceItems);
    QFETCH(int, sortOrder);
    QFETCH(QString, filter);
    QFETCH(QStringList, expectedInitialProxyItems);
    QFETCH(bool, dynamic);
    QFETCH(int, row);
    QFETCH(QString, newValue);
    QFETCH(IntPairList, removeIntervals);
    QFETCH(IntPairList, insertIntervals);
    QFETCH(int, expectedDataChangedRow);
    QFETCH(bool, expectedLayoutChanged);
    QFETCH(QStringList, proxyItems);

    QStandardItemModel model;
    QSortFilterProxyModel proxy;

    proxy.setDynamicSortFilter(dynamic);
    proxy.setSourceModel(&model);
    model.insertColumns(0, 1);
    model.insertRows(0, sourceItems.count());

    for (int i = 0; i < sourceItems.count(); ++i) {
        QModelIndex index = model.index(i, 0, QModelIndex());
        model.setData(index, sourceItems.at(i), Qt::DisplayRole);
    }

    proxy.sort(0, static_cast<Qt::SortOrder>(sortOrder));
    (void)proxy.rowCount(QModelIndex()); // force mapping

    setupFilter(&proxy, filter);

    QCOMPARE(proxy.rowCount(), expectedInitialProxyItems.count());
    for (int i = 0; i < expectedInitialProxyItems.count(); ++i) {
        const QModelIndex index = proxy.index(i, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole).toString(), expectedInitialProxyItems.at(i));
    }

    QSignalSpy removeSpy(&proxy, &QSortFilterProxyModel::rowsRemoved);
    QSignalSpy insertSpy(&proxy, &QSortFilterProxyModel::rowsInserted);
    QSignalSpy dataChangedSpy(&proxy, &QSortFilterProxyModel::dataChanged);
    QSignalSpy layoutChangedSpy(&proxy, &QSortFilterProxyModel::layoutChanged);

    QVERIFY(removeSpy.isValid());
    QVERIFY(insertSpy.isValid());
    QVERIFY(dataChangedSpy.isValid());
    QVERIFY(layoutChangedSpy.isValid());

    {
        QModelIndex index = model.index(row, 0, QModelIndex());
        model.setData(index, newValue, Qt::DisplayRole);
    }

    QCOMPARE(removeSpy.count(), removeIntervals.count());
    for (int i = 0; i < removeSpy.count(); ++i) {
        QList<QVariant> args = removeSpy.at(i);
        QCOMPARE(args.at(1).type(), QVariant::Int);
        QCOMPARE(args.at(2).type(), QVariant::Int);
        QCOMPARE(args.at(1).toInt(), removeIntervals.at(i).first);
        QCOMPARE(args.at(2).toInt(), removeIntervals.at(i).second);
    }

    QCOMPARE(insertSpy.count(), insertIntervals.count());
    for (int i = 0; i < insertSpy.count(); ++i) {
        QList<QVariant> args = insertSpy.at(i);
        QCOMPARE(args.at(1).type(), QVariant::Int);
        QCOMPARE(args.at(2).type(), QVariant::Int);
        QCOMPARE(args.at(1).toInt(), insertIntervals.at(i).first);
        QCOMPARE(args.at(2).toInt(), insertIntervals.at(i).second);
    }

    QCOMPARE(proxy.rowCount(QModelIndex()), proxyItems.count());
    for (int i = 0; i < proxyItems.count(); ++i) {
        QModelIndex index = proxy.index(i, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole).toString(), proxyItems.at(i));
    }

    if (expectedDataChangedRow == -1) {
        QCOMPARE(dataChangedSpy.count(), 0);
    } else {
        QCOMPARE(dataChangedSpy.count(), 1);
        const QModelIndex idx = dataChangedSpy.at(0).at(0).value<QModelIndex>();
        QCOMPARE(idx.row(), expectedDataChangedRow);
        QCOMPARE(idx.column(), 0);
    }

    QCOMPARE(layoutChangedSpy.count(), expectedLayoutChanged ? 1 : 0);
}

// Checks that the model is a table, and that each and every row is like this:
// i-th row:   ( rows.at(i), i )
static void checkSortedTableModel(const QAbstractItemModel *model, const QStringList &rows)
{
    QCOMPARE(model->rowCount(), rows.length());
    QCOMPARE(model->columnCount(), 2);

    for (int row = 0; row < model->rowCount(); ++row) {
        const QString column0 = model->index(row, 0).data().toString();
        const int column1 = model->index(row, 1).data().toString().toInt();

        QCOMPARE(column0, rows.at(row));
        QCOMPARE(column1, row);
    }
}

void tst_QSortFilterProxyModel::changeSourceDataKeepsStableSorting_qtbug1548()
{
    // Check that emitting dataChanged from the source model
    // for a change of a role which is not the sorting role
    // doesn't alter the sorting. In this case, we sort on the DisplayRole,
    // and play with other roles.

    static const QStringList rows
            = QStringList() << "a" << "b" << "b" << "b" << "c" << "c" << "x";

    // Build a table of pairs (string, #row) in each row
    QStandardItemModel model(0, 2);

    for (int rowNumber = 0; rowNumber < rows.length(); ++rowNumber) {
        QStandardItem *column0 = new QStandardItem(rows.at(rowNumber));
        column0->setCheckable(true);
        column0->setCheckState(Qt::Unchecked);

        QStandardItem *column1 = new QStandardItem(QString::number(rowNumber));

        const QList<QStandardItem *> row
                = QList<QStandardItem *>() << column0 << column1;

        model.appendRow(row);
    }

    checkSortedTableModel(&model, rows);

    // Build the proxy model
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setDynamicSortFilter(true);
    proxy.sort(0);

    // The proxy is now sorted by the first column, check that the sorting
    // * is correct (the input is already sorted, so it must not have changed)
    // * was stable (by looking at the second column)
    checkSortedTableModel(&model, rows);

    // Change the check status of an item. That must not break the stable sorting
    // changes the middle "b"
    model.item(2)->setCheckState(Qt::Checked);
    checkSortedTableModel(&model, rows);

    // changes the starting "a"
    model.item(0)->setCheckState(Qt::Checked);
    checkSortedTableModel(&model, rows);

    // change the background color of the first "c"
    model.item(4)->setBackground(Qt::red);
    checkSortedTableModel(&model, rows);

    // change the background color of the second "c"
    model.item(5)->setBackground(Qt::red);
    checkSortedTableModel(&model, rows);
}

void tst_QSortFilterProxyModel::changeSourceDataForwardsRoles_qtbug35440()
{
    QStringList strings;
    for (int i = 0; i < 100; ++i)
        strings << QString::number(i);

    QStringListModel model(strings);

    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.sort(0, Qt::AscendingOrder);

    QSignalSpy spy(&proxy, &QAbstractItemModel::dataChanged);
    QVERIFY(spy.isValid());
    QCOMPARE(spy.length(), 0);

    QModelIndex index;

    // QStringListModel doesn't distinguish between edit and display roles,
    // so changing one always changes the other, too.
    QVector<int> expectedChangedRoles;
    expectedChangedRoles.append(Qt::DisplayRole);
    expectedChangedRoles.append(Qt::EditRole);

    index = model.index(0, 0);
    QVERIFY(index.isValid());
    model.setData(index, QStringLiteral("teststring"), Qt::DisplayRole);
    QCOMPARE(spy.length(), 1);
    QCOMPARE(spy.at(0).at(2).value<QVector<int> >(), expectedChangedRoles);

    index = model.index(1, 0);
    QVERIFY(index.isValid());
    model.setData(index, QStringLiteral("teststring2"), Qt::EditRole);
    QCOMPARE(spy.length(), 2);
    QCOMPARE(spy.at(1).at(2).value<QVector<int> >(), expectedChangedRoles);
}

void tst_QSortFilterProxyModel::sortFilterRole()
{
    QStandardItemModel model;
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    model.insertColumns(0, 1);

    QList<QPair<QVariant, QVariant> > sourceItems;
    sourceItems = QList<QPair<QVariant, QVariant> >()
                  << QPair<QVariant, QVariant>("b", 3)
                  << QPair<QVariant, QVariant>("c", 2)
                  << QPair<QVariant, QVariant>("a", 1);

    QList<int> orderedItems;
    orderedItems = QList<int>()
                  << 2 << 1;

    model.insertRows(0, sourceItems.count());
    for (int i = 0; i < sourceItems.count(); ++i) {
        QModelIndex index = model.index(i, 0, QModelIndex());
        model.setData(index, sourceItems.at(i).first, Qt::DisplayRole);
        model.setData(index, sourceItems.at(i).second, Qt::UserRole);
    }

    setupFilter(&proxy, QLatin1String("2"));

    QCOMPARE(proxy.rowCount(), 0); // Qt::DisplayRole is default role

    proxy.setFilterRole(Qt::UserRole);
    QCOMPARE(proxy.rowCount(), 1);

    proxy.setFilterRole(Qt::DisplayRole);
    QCOMPARE(proxy.rowCount(), 0);

    setupFilter(&proxy, QLatin1String("1|2|3"));

    QCOMPARE(proxy.rowCount(), 0);

    proxy.setFilterRole(Qt::UserRole);
    QCOMPARE(proxy.rowCount(), 3);

    proxy.sort(0, Qt::AscendingOrder);
    QCOMPARE(proxy.rowCount(), 3);

    proxy.setSortRole(Qt::UserRole);
    proxy.setFilterRole(Qt::DisplayRole);
    setupFilter(&proxy, QLatin1String("a|c"));

    QCOMPARE(proxy.rowCount(), orderedItems.count());
    for (int i = 0; i < proxy.rowCount(); ++i) {
        QModelIndex index = proxy.index(i, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole), sourceItems.at(orderedItems.at(i)).first);
    }
}

void tst_QSortFilterProxyModel::selectionFilteredOut()
{
    QStandardItemModel model(2, 1);
    model.setData(model.index(0, 0), QString("AAA"));
    model.setData(model.index(1, 0), QString("BBB"));
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    QTreeView view;

    view.show();
    view.setModel(&proxy);
    QSignalSpy spy(view.selectionModel(), &QItemSelectionModel::currentChanged);
    QVERIFY(spy.isValid());

    view.setCurrentIndex(proxy.index(0, 0));
    QCOMPARE(spy.count(), 1);

    setupFilter(&proxy, QLatin1String("^B"));
    QCOMPARE(spy.count(), 2);
}

void tst_QSortFilterProxyModel::match_data()
{
    QTest::addColumn<QStringList>("sourceItems");
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<QString>("filter");
    QTest::addColumn<int>("proxyStartRow");
    QTest::addColumn<QString>("what");
    QTest::addColumn<int>("matchFlags");
    QTest::addColumn<IntList>("expectedProxyItems");
    QTest::newRow("1")
        << (QStringList() << "a") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "" // filter
        << 0 // proxyStartRow
        << "a" // what
        << static_cast<int>(Qt::MatchExactly) // matchFlags
        << (IntList() << 0); // expectedProxyItems
    QTest::newRow("2")
        << (QStringList() << "a" << "b") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "" // filter
        << 0 // proxyStartRow
        << "b" // what
        << static_cast<int>(Qt::MatchExactly) // matchFlags
        << (IntList() << 1); // expectedProxyItems
    QTest::newRow("3")
        << (QStringList() << "a" << "b") // sourceItems
        << static_cast<int>(Qt::DescendingOrder) // sortOrder
        << "" // filter
        << 0 // proxyStartRow
        << "a" // what
        << static_cast<int>(Qt::MatchExactly) // matchFlags
        << (IntList() << 1); // expectedProxyItems
    QTest::newRow("4")
        << (QStringList() << "b" << "d" << "a" << "c") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "" // filter
        << 1 // proxyStartRow
        << "a" // what
        << static_cast<int>(Qt::MatchExactly) // matchFlags
        << IntList(); // expectedProxyItems
    QTest::newRow("5")
        << (QStringList() << "b" << "d" << "a" << "c") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "a|b" // filter
        << 0 // proxyStartRow
        << "c" // what
        << static_cast<int>(Qt::MatchExactly) // matchFlags
        << IntList(); // expectedProxyItems
    QTest::newRow("6")
        << (QStringList() << "b" << "d" << "a" << "c") // sourceItems
        << static_cast<int>(Qt::DescendingOrder) // sortOrder
        << "a|b" // filter
        << 0 // proxyStartRow
        << "b" // what
        << static_cast<int>(Qt::MatchExactly) // matchFlags
        << (IntList() << 0); // expectedProxyItems
}

void tst_QSortFilterProxyModel::match()
{
    QFETCH(QStringList, sourceItems);
    QFETCH(int, sortOrder);
    QFETCH(QString, filter);
    QFETCH(int, proxyStartRow);
    QFETCH(QString, what);
    QFETCH(int, matchFlags);
    QFETCH(IntList, expectedProxyItems);

    QStandardItemModel model;
    QSortFilterProxyModel proxy;

    proxy.setSourceModel(&model);
    model.insertColumns(0, 1);
    model.insertRows(0, sourceItems.count());

    for (int i = 0; i < sourceItems.count(); ++i) {
        QModelIndex index = model.index(i, 0, QModelIndex());
        model.setData(index, sourceItems.at(i), Qt::DisplayRole);
    }

    proxy.sort(0, static_cast<Qt::SortOrder>(sortOrder));
    setupFilter(&proxy, filter);

    QModelIndex startIndex = proxy.index(proxyStartRow, 0);
    QModelIndexList indexes = proxy.match(startIndex, Qt::DisplayRole, what,
                                          expectedProxyItems.count(),
                                          Qt::MatchFlags(matchFlags));
    QCOMPARE(indexes.count(), expectedProxyItems.count());
    for (int i = 0; i < indexes.count(); ++i)
        QCOMPARE(indexes.at(i).row(), expectedProxyItems.at(i));
}

void tst_QSortFilterProxyModel::insertIntoChildrenlessItem()
{
    QStandardItemModel model;
    QStandardItem *itemA = new QStandardItem("a");
    model.appendRow(itemA);
    QStandardItem *itemB = new QStandardItem("b");
    model.appendRow(itemB);
    QStandardItem *itemC = new QStandardItem("c");
    model.appendRow(itemC);

    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    QSignalSpy colsInsertedSpy(&proxy, &QSortFilterProxyModel::columnsInserted);
    QSignalSpy rowsInsertedSpy(&proxy, &QSortFilterProxyModel::rowsInserted);

    QVERIFY(colsInsertedSpy.isValid());
    QVERIFY(rowsInsertedSpy.isValid());

    (void)proxy.rowCount(QModelIndex()); // force mapping of "a", "b", "c"
    QCOMPARE(colsInsertedSpy.count(), 0);
    QCOMPARE(rowsInsertedSpy.count(), 0);

    // now add a child to itemB ==> should get insert notification from the proxy
    itemB->appendRow(new QStandardItem("a.0"));
    QCOMPARE(colsInsertedSpy.count(), 1);
    QCOMPARE(rowsInsertedSpy.count(), 1);

    QVariantList args = colsInsertedSpy.takeFirst();
    QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), proxy.mapFromSource(itemB->index()));
    QCOMPARE(qvariant_cast<int>(args.at(1)), 0);
    QCOMPARE(qvariant_cast<int>(args.at(2)), 0);

    args = rowsInsertedSpy.takeFirst();
    QCOMPARE(qvariant_cast<QModelIndex>(args.at(0)), proxy.mapFromSource(itemB->index()));
    QCOMPARE(qvariant_cast<int>(args.at(1)), 0);
    QCOMPARE(qvariant_cast<int>(args.at(2)), 0);
}

void tst_QSortFilterProxyModel::invalidateMappedChildren()
{
    QStandardItemModel model;

    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    QStandardItem *itemA = new QStandardItem("a");
    model.appendRow(itemA);
    QStandardItem *itemB = new QStandardItem("b");
    itemA->appendRow(itemB);

    QStandardItem *itemC = new QStandardItem("c");
    itemB->appendRow(itemC);
    itemC->appendRow(new QStandardItem("d"));

    // force mappings
    (void)proxy.hasChildren(QModelIndex());
    (void)proxy.hasChildren(proxy.mapFromSource(itemA->index()));
    (void)proxy.hasChildren(proxy.mapFromSource(itemB->index()));
    (void)proxy.hasChildren(proxy.mapFromSource(itemC->index()));

    itemB->removeRow(0); // should invalidate mapping of itemC
    itemC = new QStandardItem("c");
    itemA->appendRow(itemC);
    itemC->appendRow(new QStandardItem("d"));

    itemA->removeRow(1); // should invalidate mapping of itemC
    itemC = new QStandardItem("c");
    itemB->appendRow(itemC);
    itemC->appendRow(new QStandardItem("d"));

    QCOMPARE(proxy.rowCount(proxy.mapFromSource(itemA->index())), 1);
    QCOMPARE(proxy.rowCount(proxy.mapFromSource(itemB->index())), 1);
    QCOMPARE(proxy.rowCount(proxy.mapFromSource(itemC->index())), 1);
}

class EvenOddFilterModel : public QSortFilterProxyModel
{
public:
    virtual bool filterAcceptsRow(int srcRow, const QModelIndex& srcParent) const
    {
        if (srcParent.isValid())
            return (srcParent.row() % 2) ^ !(srcRow % 2);
        return (srcRow % 2);
    }
};

void tst_QSortFilterProxyModel::insertRowIntoFilteredParent()
{
    QStandardItemModel model;
    EvenOddFilterModel proxy;
    proxy.setSourceModel(&model);

    QSignalSpy spy(&proxy, &EvenOddFilterModel::rowsInserted);
    QVERIFY(spy.isValid());

    QStandardItem *itemA = new QStandardItem();
    model.appendRow(itemA); // A will be filtered
    QStandardItem *itemB = new QStandardItem();
    itemA->appendRow(itemB);

    QCOMPARE(spy.count(), 0);

    itemA->removeRow(0);

    QCOMPARE(spy.count(), 0);
}

void tst_QSortFilterProxyModel::filterOutParentAndFilterInChild()
{
    QStandardItemModel model;
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    setupFilter(&proxy, QLatin1String("A|B"));

    QStandardItem *itemA = new QStandardItem("A");
    model.appendRow(itemA); // not filtered
    QStandardItem *itemB = new QStandardItem("B");
    itemA->appendRow(itemB); // not filtered
    QStandardItem *itemC = new QStandardItem("C");
    itemA->appendRow(itemC); // filtered

    QSignalSpy removedSpy(&proxy, &QSortFilterProxyModel::rowsRemoved);
    QSignalSpy insertedSpy(&proxy, &QSortFilterProxyModel::rowsInserted);

    QVERIFY(removedSpy.isValid());
    QVERIFY(insertedSpy.isValid());

    setupFilter(&proxy, QLatin1String("C")); // A and B will be filtered out, C filtered in

    // we should now have been notified that the subtree represented by itemA has been removed
    QCOMPARE(removedSpy.count(), 1);
    // we should NOT get any inserts; itemC should be filtered because its parent (itemA) is
    QCOMPARE(insertedSpy.count(), 0);
}

void tst_QSortFilterProxyModel::sourceInsertRows()
{
    QStandardItemModel model;
    QSortFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&model);

    model.insertColumns(0, 1, QModelIndex());
    model.insertRows(0, 2, QModelIndex());

    {
      QModelIndex parent = model.index(0, 0, QModelIndex());
      model.insertColumns(0, 1, parent);
      model.insertRows(0, 1, parent);
    }

    {
      QModelIndex parent = model.index(1, 0, QModelIndex());
      model.insertColumns(0, 1, parent);
      model.insertRows(0, 1, parent);
    }

    model.insertRows(0, 1, QModelIndex());
    model.insertRows(0, 1, QModelIndex());

    QVERIFY(true); // if you got here without asserting, it's all good
}

void tst_QSortFilterProxyModel::sourceModelDeletion()
{
    QSortFilterProxyModel proxyModel;
    {
        QStandardItemModel model;
        proxyModel.setSourceModel(&model);
        QCOMPARE(proxyModel.sourceModel(), static_cast<QAbstractItemModel*>(&model));
    }
    QCOMPARE(proxyModel.sourceModel(), static_cast<QAbstractItemModel*>(0));
}

void tst_QSortFilterProxyModel::sortColumnTracking1()
{
    QStandardItemModel model;
    QSortFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&model);

    model.insertColumns(0, 10);
    model.insertRows(0, 10);

    proxyModel.sort(1);
    QCOMPARE(proxyModel.sortColumn(), 1);

    model.insertColumn(8);
    QCOMPARE(proxyModel.sortColumn(), 1);

    model.removeColumn(8);
    QCOMPARE(proxyModel.sortColumn(), 1);

    model.insertColumn(2);
    QCOMPARE(proxyModel.sortColumn(), 1);

    model.removeColumn(2);
    QCOMPARE(proxyModel.sortColumn(), 1);

    model.insertColumn(1);
    QCOMPARE(proxyModel.sortColumn(), 2);

    model.removeColumn(1);
    QCOMPARE(proxyModel.sortColumn(), 1);

    model.removeColumn(1);
    QCOMPARE(proxyModel.sortColumn(), -1);
}

void tst_QSortFilterProxyModel::sortColumnTracking2()
{
    QStandardItemModel model;
    QSortFilterProxyModel proxyModel;
    proxyModel.setDynamicSortFilter(true);
    proxyModel.setSourceModel(&model);

    proxyModel.sort(0);
    QCOMPARE(proxyModel.sortColumn(), 0);

    QList<QStandardItem *> items; // Stable sorting: Items with invalid data should move to the end
    items << new QStandardItem << new QStandardItem("foo") << new QStandardItem("bar")
        << new QStandardItem("some") << new QStandardItem("others") << new QStandardItem("item")
        << new QStandardItem("aa") << new QStandardItem("zz") << new QStandardItem;

    model.insertColumn(0,items);
    QCOMPARE(proxyModel.sortColumn(), 0);
    QCOMPARE(proxyModel.data(proxyModel.index(0,0)).toString(),QString::fromLatin1("aa"));
    const int zzIndex = items.count() - 3; // 2 invalid at end.
    QCOMPARE(proxyModel.data(proxyModel.index(zzIndex,0)).toString(),QString::fromLatin1("zz"));
}

void tst_QSortFilterProxyModel::sortStable()
{
    QStandardItemModel* model = new QStandardItemModel(5, 2);
    for (int r = 0; r < 5; r++) {
        const QString prefix = QLatin1String("Row:") + QString::number(r) + QLatin1String(", Column:");
        for (int c = 0; c < 2; c++)  {
            QStandardItem* item = new QStandardItem(prefix + QString::number(c));
            for (int i = 0; i < 3; i++) {
                QStandardItem* child = new QStandardItem(QLatin1String("Item ") + QString::number(i));
                item->appendRow( child );
            }
            model->setItem(r, c, item);
        }
    }
    model->setHorizontalHeaderItem( 0, new QStandardItem( "Name" ));
    model->setHorizontalHeaderItem( 1, new QStandardItem( "Value" ));

    QSortFilterProxyModel *filterModel = new QSortFilterProxyModel(model);
    filterModel->setSourceModel(model);

    QTreeView *view = new QTreeView;
    view->setModel(filterModel);
    QModelIndex firstRoot = filterModel->index(0,0);
    view->expand(firstRoot);
    view->setSortingEnabled(true);

    view->model()->sort(1, Qt::DescendingOrder);
    QVariant lastItemData =filterModel->index(2,0, firstRoot).data();
    view->model()->sort(1, Qt::DescendingOrder);
    QCOMPARE(lastItemData, filterModel->index(2,0, firstRoot).data());
}

void tst_QSortFilterProxyModel::hiddenColumns()
{
    class MyStandardItemModel : public QStandardItemModel
    {
    public:
        MyStandardItemModel() : QStandardItemModel(0,5) {}
        void reset()
        { beginResetModel(); endResetModel(); }
        friend class tst_QSortFilterProxyModel;
    } model;
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    QTableView view;
    view.setModel(&proxy);

    view.hideColumn(0);

    QVERIFY(view.isColumnHidden(0));
    model.blockSignals(true);
    model.setRowCount(1);
    model.blockSignals(false);
    model.reset();

    // In the initial bug report that spawned this test, this would be false
    // because resetting model would also reset the hidden columns.
    QVERIFY(view.isColumnHidden(0));
}

void tst_QSortFilterProxyModel::insertRowsSort()
{
    QStandardItemModel model(2,2);
    QSortFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&model);

    proxyModel.sort(0);
    QCOMPARE(proxyModel.sortColumn(), 0);

    model.insertColumns(0, 3, model.index(0,0));
    QCOMPARE(proxyModel.sortColumn(), 0);

    model.removeColumns(0, 3, model.index(0,0));
    QCOMPARE(proxyModel.sortColumn(), 0);
}

void tst_QSortFilterProxyModel::staticSorting()
{
    QStandardItemModel model(0, 1);
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setDynamicSortFilter(false);
    QStringList initial = QString("bateau avion dragon hirondelle flamme camion elephant").split(QLatin1Char(' '));

    // prepare model
    QStandardItem *root = model.invisibleRootItem ();
    QList<QStandardItem *> items;
    for (int i = 0; i < initial.count(); ++i) {
        items.append(new QStandardItem(initial.at(i)));
    }
    root->insertRows(0, items);
    QCOMPARE(model.rowCount(QModelIndex()), initial.count());
    QCOMPARE(model.columnCount(QModelIndex()), 1);

    // make sure the proxy is unsorted
    QCOMPARE(proxy.columnCount(QModelIndex()), 1);
    QCOMPARE(proxy.rowCount(QModelIndex()), initial.count());
    for (int row = 0; row < proxy.rowCount(QModelIndex()); ++row) {
        QModelIndex index = proxy.index(row, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole).toString(), initial.at(row));
    }

    // sort
    proxy.sort(0);

    QStringList expected = initial;
    expected.sort();
    // make sure the proxy is sorted
    for (int row = 0; row < proxy.rowCount(QModelIndex()); ++row) {
        QModelIndex index = proxy.index(row, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole).toString(), expected.at(row));
    }

    //update one item.
    items.first()->setData("girafe", Qt::DisplayRole);

    // make sure the proxy is updated but not sorted
    expected.replaceInStrings("bateau", "girafe");
    for (int row = 0; row < proxy.rowCount(QModelIndex()); ++row) {
        QModelIndex index = proxy.index(row, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole).toString(), expected.at(row));
    }

    // sort again
    proxy.sort(0);
    expected.sort();

    // make sure the proxy is sorted
    for (int row = 0; row < proxy.rowCount(QModelIndex()); ++row) {
        QModelIndex index = proxy.index(row, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole).toString(), expected.at(row));
    }
}

void tst_QSortFilterProxyModel::dynamicSorting()
{
    QStringListModel model1;
    const QStringList initial = QString("bateau avion dragon hirondelle flamme camion elephant").split(QLatin1Char(' '));
    model1.setStringList(initial);
    QSortFilterProxyModel proxy1;
    proxy1.setDynamicSortFilter(false);
    proxy1.sort(0);
    proxy1.setSourceModel(&model1);

    QCOMPARE(proxy1.columnCount(QModelIndex()), 1);
    //the model should not be sorted because sorting has not been set to dynamic yet.
    QCOMPARE(proxy1.rowCount(QModelIndex()), initial.count());
    for (int row = 0; row < proxy1.rowCount(QModelIndex()); ++row) {
        QModelIndex index = proxy1.index(row, 0, QModelIndex());
        QCOMPARE(proxy1.data(index, Qt::DisplayRole).toString(), initial.at(row));
    }

    proxy1.setDynamicSortFilter(true);

    //now the model should be sorted.
    QStringList expected = initial;
    expected.sort();
    for (int row = 0; row < proxy1.rowCount(QModelIndex()); ++row) {
        QModelIndex index = proxy1.index(row, 0, QModelIndex());
        QCOMPARE(proxy1.data(index, Qt::DisplayRole).toString(), expected.at(row));
    }

    QStringList initial2 = initial;
    initial2.replaceInStrings("bateau", "girafe");
    model1.setStringList(initial2); //this will cause a reset

    QStringList expected2 = initial2;
    expected2.sort();

    //now the model should still be sorted.
    for (int row = 0; row < proxy1.rowCount(QModelIndex()); ++row) {
        QModelIndex index = proxy1.index(row, 0, QModelIndex());
        QCOMPARE(proxy1.data(index, Qt::DisplayRole).toString(), expected2.at(row));
    }

    QStringListModel model2(initial);
    proxy1.setSourceModel(&model2);

    //the model should again be sorted
    for (int row = 0; row < proxy1.rowCount(QModelIndex()); ++row) {
        QModelIndex index = proxy1.index(row, 0, QModelIndex());
        QCOMPARE(proxy1.data(index, Qt::DisplayRole).toString(), expected.at(row));
    }

    //set up the sorting before seting the model up
    QSortFilterProxyModel proxy2;
    proxy2.sort(0);
    proxy2.setSourceModel(&model2);
    for (int row = 0; row < proxy2.rowCount(QModelIndex()); ++row) {
        QModelIndex index = proxy2.index(row, 0, QModelIndex());
        QCOMPARE(proxy2.data(index, Qt::DisplayRole).toString(), expected.at(row));
    }
}

class QtTestModel: public QAbstractItemModel
{
public:
    QtTestModel(int _rows, int _cols, QObject *parent = 0)
        : QAbstractItemModel(parent)
        , rows(_rows)
        , cols(_cols)
        , wrongIndex(false)
    {
    }

    bool canFetchMore(const QModelIndex &idx) const
    {
        return !fetched.contains(idx);
    }

    void fetchMore(const QModelIndex &idx)
    {
        if (fetched.contains(idx))
            return;
        beginInsertRows(idx, 0, rows-1);
        fetched.insert(idx);
        endInsertRows();
    }

    bool hasChildren(const QModelIndex & = QModelIndex()) const
    {
        return true;
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const
    {
        return fetched.contains(parent) ? rows : 0;
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const
    {
        Q_UNUSED(parent);
        return cols;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
    {
        if (row < 0 || column < 0 || column >= cols || row >= rows) {
            return QModelIndex();
        }
        QModelIndex i = createIndex(row, column, int(parent.internalId() + 1));
        parentHash[i] = parent;
        return i;
    }

    QModelIndex parent(const QModelIndex &index) const
    {
        if (!parentHash.contains(index))
            return QModelIndex();
        return parentHash[index];
    }

    QVariant data(const QModelIndex &idx, int role) const
    {
        if (!idx.isValid())
            return QVariant();

        if (role == Qt::DisplayRole) {
            if (idx.row() < 0 || idx.column() < 0 || idx.column() >= cols || idx.row() >= rows) {
                wrongIndex = true;
                qWarning("Invalid modelIndex [%d,%d,%p]", idx.row(), idx.column(),
                idx.internalPointer());
            }
            return QLatin1Char('[') + QString::number(idx.row()) + QLatin1Char(',')
                + QString::number(idx.column()) + QLatin1Char(']');
        }
        return QVariant();
    }

    QSet<QModelIndex> fetched;
    int rows, cols;
    mutable bool wrongIndex;
    mutable QMap<QModelIndex,QModelIndex> parentHash;
};

void tst_QSortFilterProxyModel::fetchMore()
{
    QtTestModel model(10,10);
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    QVERIFY(proxy.canFetchMore(QModelIndex()));
    QVERIFY(proxy.hasChildren());
    while (proxy.canFetchMore(QModelIndex()))
        proxy.fetchMore(QModelIndex());
    QCOMPARE(proxy.rowCount(), 10);
    QCOMPARE(proxy.columnCount(), 10);

    QModelIndex idx = proxy.index(1,1);
    QVERIFY(idx.isValid());
    QVERIFY(proxy.canFetchMore(idx));
    QVERIFY(proxy.hasChildren(idx));
    while (proxy.canFetchMore(idx))
        proxy.fetchMore(idx);
    QCOMPARE(proxy.rowCount(idx), 10);
    QCOMPARE(proxy.columnCount(idx), 10);
}

void tst_QSortFilterProxyModel::hiddenChildren()
{
    QStandardItemModel model;
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.setDynamicSortFilter(true);

    QStandardItem *itemA = new QStandardItem("A VISIBLE");
    model.appendRow(itemA);
    QStandardItem *itemB = new QStandardItem("B VISIBLE");
    itemA->appendRow(itemB);
    QStandardItem *itemC = new QStandardItem("C");
    itemA->appendRow(itemC);
    setupFilter(&proxy, QLatin1String("VISIBLE"));

    QCOMPARE(proxy.rowCount(QModelIndex()) , 1);
    QPersistentModelIndex indexA = proxy.index(0,0);
    QCOMPARE(proxy.data(indexA).toString(), QString::fromLatin1("A VISIBLE"));

    QCOMPARE(proxy.rowCount(indexA) , 1);
    QPersistentModelIndex indexB = proxy.index(0, 0, indexA);
    QCOMPARE(proxy.data(indexB).toString(), QString::fromLatin1("B VISIBLE"));

    itemA->setText("A");
    QCOMPARE(proxy.rowCount(QModelIndex()), 0);
    QVERIFY(!indexA.isValid());
    QVERIFY(!indexB.isValid());

    itemB->setText("B");
    itemA->setText("A VISIBLE");
    itemC->setText("C VISIBLE");

    QCOMPARE(proxy.rowCount(QModelIndex()), 1);
    indexA = proxy.index(0,0);
    QCOMPARE(proxy.data(indexA).toString(), QString::fromLatin1("A VISIBLE"));

    QCOMPARE(proxy.rowCount(indexA) , 1);
    QModelIndex indexC = proxy.index(0, 0, indexA);
    QCOMPARE(proxy.data(indexC).toString(), QString::fromLatin1("C VISIBLE"));

    setupFilter(&proxy, QLatin1String("C"));

    QCOMPARE(proxy.rowCount(QModelIndex()), 0);
    itemC->setText("invisible");
    itemA->setText("AC");

    QCOMPARE(proxy.rowCount(QModelIndex()), 1);
    indexA = proxy.index(0,0);
    QCOMPARE(proxy.data(indexA).toString(), QString::fromLatin1("AC"));
    QCOMPARE(proxy.rowCount(indexA) , 0);
}

void tst_QSortFilterProxyModel::mapFromToSource()
{
    QtTestModel source(10,10);
    source.fetchMore(QModelIndex());
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&source);
    QCOMPARE(proxy.mapFromSource(source.index(5, 4)), proxy.index(5, 4));
    QCOMPARE(proxy.mapToSource(proxy.index(3, 2)), source.index(3, 2));
    QCOMPARE(proxy.mapFromSource(QModelIndex()), QModelIndex());
    QCOMPARE(proxy.mapToSource(QModelIndex()), QModelIndex());

#ifdef QT_NO_DEBUG  //if Qt is compiled in debug mode, this will assert
    QTest::ignoreMessage(QtWarningMsg, "QSortFilterProxyModel: index from wrong model passed to mapToSource");
    QCOMPARE(proxy.mapToSource(source.index(2, 3)), QModelIndex());
    QTest::ignoreMessage(QtWarningMsg, "QSortFilterProxyModel: index from wrong model passed to mapFromSource");
    QCOMPARE(proxy.mapFromSource(proxy.index(6, 2)), QModelIndex());
#endif
}

static QStandardItem *addEntry(QStandardItem* pParent, const QString &description)
{
    QStandardItem* pItem = new QStandardItem(description);
    pParent->appendRow(pItem);
    return pItem;
}

void tst_QSortFilterProxyModel::removeRowsRecursive()
{
    QStandardItemModel pModel;
    QStandardItem *pItem1    = new QStandardItem("root");
    pModel.appendRow(pItem1);
    QList<QStandardItem *> items;

    QStandardItem *pItem11   = addEntry(pItem1,"Sub-heading");
    items << pItem11;
    QStandardItem *pItem111 = addEntry(pItem11,"A");
    items << pItem111;
    items << addEntry(pItem111,"A1");
    items << addEntry(pItem111,"A2");
    QStandardItem *pItem112 = addEntry(pItem11,"B");
    items << pItem112;
    items << addEntry(pItem112,"B1");
    items << addEntry(pItem112,"B2");
    QStandardItem *pItem1123 = addEntry(pItem112,"B3");
    items << pItem1123;
    items << addEntry(pItem1123,"B3-");

    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&pModel);

    QList<QPersistentModelIndex> sourceIndexes;
    QList<QPersistentModelIndex> proxyIndexes;
    foreach (QStandardItem *item, items) {
        QModelIndex idx = item->index();
        sourceIndexes << idx;
        proxyIndexes << proxy.mapFromSource(idx);
    }

    foreach (const QPersistentModelIndex &pidx, sourceIndexes)
        QVERIFY(pidx.isValid());
    foreach (const QPersistentModelIndex &pidx, proxyIndexes)
        QVERIFY(pidx.isValid());

    QList<QStandardItem*> itemRow = pItem1->takeRow(0);

    QCOMPARE(itemRow.count(), 1);
    QCOMPARE(itemRow.first(), pItem11);

    foreach (const QPersistentModelIndex &pidx, sourceIndexes)
        QVERIFY(!pidx.isValid());
    foreach (const QPersistentModelIndex &pidx, proxyIndexes)
        QVERIFY(!pidx.isValid());

    delete pItem11;
}

void tst_QSortFilterProxyModel::doubleProxySelectionSetSourceModel()
{
    QStandardItemModel *model1 = new QStandardItemModel;
    QStandardItem *parentItem = model1->invisibleRootItem();
    for (int i = 0; i < 4; ++i) {
        QStandardItem *item = new QStandardItem(QLatin1String("model1 item ") + QString::number(i));
        parentItem->appendRow(item);
        parentItem = item;
    }

    QStandardItemModel *model2 = new QStandardItemModel;
    QStandardItem *parentItem2 = model2->invisibleRootItem();
    for (int i = 0; i < 4; ++i) {
        QStandardItem *item = new QStandardItem(QLatin1String("model2 item ") + QString::number(i));
        parentItem2->appendRow(item);
        parentItem2 = item;
    }

    QSortFilterProxyModel *toggleProxy = new QSortFilterProxyModel;
    toggleProxy->setSourceModel(model1);

    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel;
    proxyModel->setSourceModel(toggleProxy);

    QModelIndex mi = proxyModel->index(0, 0, proxyModel->index(0, 0, proxyModel->index(0, 0)));
    QItemSelectionModel ism(proxyModel);
    ism.select(mi, QItemSelectionModel::Select);
    QModelIndexList mil = ism.selectedIndexes();
    QCOMPARE(mil.count(), 1);
    QCOMPARE(mil.first(), mi);

    toggleProxy->setSourceModel(model2);
    // No crash, it's good news!
    QVERIFY(ism.selection().isEmpty());
}

void tst_QSortFilterProxyModel::appearsAndSort()
{
    class PModel : public QSortFilterProxyModel
    {
    public:
        PModel() : mVisible(false) {};
    protected:
        bool filterAcceptsRow(int, const QModelIndex &) const
        {
            return mVisible;
        }

    public:
        void updateXX()
        {
            mVisible = true;
            invalidate();
        }
    private:
        bool mVisible;
    } proxyModel;

    QStringListModel sourceModel;
    QStringList list;
    list << "b" << "a" << "c";
    sourceModel.setStringList(list);

    proxyModel.setSourceModel(&sourceModel);
    proxyModel.setDynamicSortFilter(true);
    proxyModel.sort(0, Qt::AscendingOrder);

    QApplication::processEvents();
    QCOMPARE(sourceModel.rowCount(), 3);
    QCOMPARE(proxyModel.rowCount(), 0); //all rows are hidden at first;

    QSignalSpy spyAbout1(&proxyModel, &PModel::layoutAboutToBeChanged);
    QSignalSpy spyChanged1(&proxyModel, &PModel::layoutChanged);

    QVERIFY(spyAbout1.isValid());
    QVERIFY(spyChanged1.isValid());

    //introducing secondProxyModel to test the layoutChange when many items appears at once
    QSortFilterProxyModel secondProxyModel;
    secondProxyModel.setSourceModel(&proxyModel);
    secondProxyModel.setDynamicSortFilter(true);
    secondProxyModel.sort(0, Qt::DescendingOrder);
    QCOMPARE(secondProxyModel.rowCount(), 0); //all rows are hidden at first;
    QSignalSpy spyAbout2(&secondProxyModel, &QSortFilterProxyModel::layoutAboutToBeChanged);
    QSignalSpy spyChanged2(&secondProxyModel, &QSortFilterProxyModel::layoutChanged);

    QVERIFY(spyAbout2.isValid());
    QVERIFY(spyChanged2.isValid());

    proxyModel.updateXX();
    QApplication::processEvents();
    //now rows should be visible, and sorted
    QCOMPARE(proxyModel.rowCount(), 3);
    QCOMPARE(proxyModel.data(proxyModel.index(0,0), Qt::DisplayRole).toString(), QString::fromLatin1("a"));
    QCOMPARE(proxyModel.data(proxyModel.index(1,0), Qt::DisplayRole).toString(), QString::fromLatin1("b"));
    QCOMPARE(proxyModel.data(proxyModel.index(2,0), Qt::DisplayRole).toString(), QString::fromLatin1("c"));

    //now rows should be visible, and sorted
    QCOMPARE(secondProxyModel.rowCount(), 3);
    QCOMPARE(secondProxyModel.data(secondProxyModel.index(0,0), Qt::DisplayRole).toString(), QString::fromLatin1("c"));
    QCOMPARE(secondProxyModel.data(secondProxyModel.index(1,0), Qt::DisplayRole).toString(), QString::fromLatin1("b"));
    QCOMPARE(secondProxyModel.data(secondProxyModel.index(2,0), Qt::DisplayRole).toString(), QString::fromLatin1("a"));

    QCOMPARE(spyAbout1.count(), 1);
    QCOMPARE(spyChanged1.count(), 1);
    QCOMPARE(spyAbout2.count(), 1);
    QCOMPARE(spyChanged2.count(), 1);
}

void tst_QSortFilterProxyModel::unnecessaryDynamicSorting()
{
    QStringListModel model;
    const QStringList initial = QString("bravo charlie delta echo").split(QLatin1Char(' '));
    model.setStringList(initial);
    QSortFilterProxyModel proxy;
    proxy.setDynamicSortFilter(false);
    proxy.setSourceModel(&model);
    proxy.sort(Qt::AscendingOrder);

    //append two rows
    int maxrows = proxy.rowCount(QModelIndex());
    model.insertRows(maxrows, 2);
    model.setData(model.index(maxrows, 0), QString("alpha"));
    model.setData(model.index(maxrows + 1, 0), QString("fondue"));

    //append new items to the initial string list and compare with model
    QStringList expected = initial;
    expected << QString("alpha") << QString("fondue");

    //if bug 7716 is present, new rows were prepended, when they should have been appended
    for (int row = 0; row < proxy.rowCount(QModelIndex()); ++row) {
        QModelIndex index = proxy.index(row, 0, QModelIndex());
        QCOMPARE(proxy.data(index, Qt::DisplayRole).toString(), expected.at(row));
    }
}

class SelectionProxyModel : QAbstractProxyModel
{
    Q_OBJECT
public:
    SelectionProxyModel()
        : QAbstractProxyModel(), selectionModel(0)
    {
    }

    QModelIndex mapFromSource(QModelIndex const&) const
    { return QModelIndex(); }

    QModelIndex mapToSource(QModelIndex const&) const
    { return QModelIndex(); }

    QModelIndex index(int, int, const QModelIndex&) const
    { return QModelIndex(); }

    QModelIndex parent(const QModelIndex&) const
    { return QModelIndex(); }

    int rowCount(const QModelIndex&) const
    { return 0; }

    int columnCount(const QModelIndex&) const
    { return 0; }

    void setSourceModel( QAbstractItemModel *sourceModel )
    {
        beginResetModel();
        disconnect( sourceModel, SIGNAL(modelAboutToBeReset()), this, SLOT(sourceModelAboutToBeReset()) );
        QAbstractProxyModel::setSourceModel( sourceModel );
        connect( sourceModel, SIGNAL(modelAboutToBeReset()), this, SLOT(sourceModelAboutToBeReset()) );
        endResetModel();
    }

    void setSelectionModel( QItemSelectionModel *_selectionModel )
    {
        selectionModel = _selectionModel;
    }

private slots:
    void sourceModelAboutToBeReset()
    {
        QVERIFY( selectionModel->selectedIndexes().size() == 1 );
        beginResetModel();
    }

    void sourceModelReset()
    {
        endResetModel();
    }

private:
    QItemSelectionModel *selectionModel;
};

void tst_QSortFilterProxyModel::testMultipleProxiesWithSelection()
{
    QStringListModel model;
    const QStringList initial = QString("bravo charlie delta echo").split(QLatin1Char(' '));
    model.setStringList(initial);

    QSortFilterProxyModel proxy;
    proxy.setSourceModel( &model );

    SelectionProxyModel proxy1;
    QSortFilterProxyModel proxy2;

    // Note that the order here matters. The order of the sourceAboutToBeReset
    // exposes the bug in QSortFilterProxyModel.
    proxy2.setSourceModel( &proxy );
    proxy1.setSourceModel( &proxy );

    QItemSelectionModel selectionModel(&proxy2);
    proxy1.setSelectionModel( &selectionModel );

    selectionModel.select( proxy2.index( 0, 0 ), QItemSelectionModel::Select );

    // trick the proxy into emitting begin/end reset signals.
    proxy.setSourceModel(0);
}

static bool isValid(const QItemSelection &selection)
{
    foreach (const QItemSelectionRange &range, selection)
        if (!range.isValid())
            return false;
    return true;
}

void tst_QSortFilterProxyModel::mapSelectionFromSource()
{
    QStringListModel model;
    const QStringList initial = QString("bravo charlie delta echo").split(QLatin1Char(' '));
    model.setStringList(initial);

    QSortFilterProxyModel proxy;
    proxy.setDynamicSortFilter(true);
    setupFilter(&proxy, QLatin1String("d.*"));

    proxy.setSourceModel(&model);

    // Only "delta" remains.
    QCOMPARE(proxy.rowCount(), 1);

    QItemSelection selection;
    QModelIndex charlie = model.index(1, 0);
    selection.append(QItemSelectionRange(charlie, charlie));
    QModelIndex delta = model.index(2, 0);
    selection.append(QItemSelectionRange(delta, delta));
    QModelIndex echo = model.index(3, 0);
    selection.append(QItemSelectionRange(echo, echo));

    QVERIFY(isValid(selection));

    QItemSelection proxiedSelection = proxy.mapSelectionFromSource(selection);

    // Only "delta" is in the mapped result.
    QCOMPARE(proxiedSelection.size(), 1);
    QVERIFY(isValid(proxiedSelection));
}

class Model10287 : public QStandardItemModel
{
    Q_OBJECT

public:
    Model10287(QObject *parent = 0)
        : QStandardItemModel(0, 1, parent)
    {
        parentItem = new QStandardItem("parent");
        parentItem->setData(false, Qt::UserRole);
        appendRow(parentItem);

        childItem = new QStandardItem("child");
        childItem->setData(true, Qt::UserRole);
        parentItem->appendRow(childItem);

        childItem2 = new QStandardItem("child2");
        childItem2->setData(true, Qt::UserRole);
        parentItem->appendRow(childItem2);
    }

    void removeChild()
    {
        childItem2->setData(false, Qt::UserRole);
        parentItem->removeRow(0);
    }

private:
    QStandardItem *parentItem, *childItem, *childItem2;
};

class Proxy10287 : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    Proxy10287(QAbstractItemModel *model, QObject *parent = 0)
        : QSortFilterProxyModel(parent)
    {
        setSourceModel(model);
        setDynamicSortFilter(true);
    }

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
    {
        // Filter based on UserRole in model
        QModelIndex i = sourceModel()->index(source_row, 0, source_parent);
        return i.data(Qt::UserRole).toBool();
    }
};

void tst_QSortFilterProxyModel::unnecessaryMapCreation()
{
    Model10287 m;
    Proxy10287 p(&m);
    m.removeChild();
    // No assert failure, it passes.
}

class FilteredColumnProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    FilteredColumnProxyModel(QObject *parent = 0)
        : QSortFilterProxyModel(parent)
    {
    }

protected:
    bool filterAcceptsColumn(int column, const QModelIndex & /* source_parent */) const
    {
        return column % 2 != 0;
    }
};

void tst_QSortFilterProxyModel::filteredColumns()
{
    DynamicTreeModel *model = new DynamicTreeModel(this);

    FilteredColumnProxyModel *proxy = new FilteredColumnProxyModel(this);
    proxy->setSourceModel(model);

    new QAbstractItemModelTester(proxy, this);

    ModelInsertCommand *insertCommand = new ModelInsertCommand(model, this);
    insertCommand->setNumCols(2);
    insertCommand->setStartRow(0);
    insertCommand->setEndRow(0);
    // Parent is QModelIndex()
    insertCommand->doCommand();
}

class ChangableHeaderData : public QStringListModel
{
    Q_OBJECT
public:
    explicit ChangableHeaderData(QObject *parent = 0)
      : QStringListModel(parent)
    {

    }

    void emitHeaderDataChanged()
    {
        headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
    }
};


void tst_QSortFilterProxyModel::headerDataChanged()
{
    ChangableHeaderData *model = new ChangableHeaderData(this);

    QStringList numbers;
    for (int i = 0; i < 10; ++i)
        numbers.append(QString::number(i));
    model->setStringList(numbers);

    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);

    new QAbstractItemModelTester(proxy, this);

    model->emitHeaderDataChanged();
}

void tst_QSortFilterProxyModel::resetInvalidate_data()
{
    QTest::addColumn<int>("test");
    QTest::addColumn<bool>("works");

    QTest::newRow("nothing") << 0 << false;
    QTest::newRow("reset") << 1 << true;
    QTest::newRow("invalidate") << 2 << true;
    QTest::newRow("invalidate_filter") << 3 << true;
}

void tst_QSortFilterProxyModel::resetInvalidate()
{
    QFETCH(int, test);
    QFETCH(bool, works);

    struct Proxy : QSortFilterProxyModel {
        QString pattern;
        virtual bool filterAcceptsRow(int source_row, const QModelIndex&) const
        {
            return sourceModel()->data(sourceModel()->index(source_row, 0)).toString().contains(pattern);
        }
        void notifyChange(int test)
        {
            switch (test) {
            case 0: break;
            case 1:
                beginResetModel();
                endResetModel();
                break;
            case 2: invalidate(); break;
            case 3: invalidateFilter(); break;
            }
        }
    };

    QStringListModel sourceModel(QStringList() << "Poisson" << "Vache" << "Brebis"
                                               << "Elephant" << "Cochon" << "Serpent"
                                               << "Mouton" << "Ecureuil" << "Mouche");
    Proxy proxy;
    proxy.pattern = QString::fromLatin1("n");
    proxy.setSourceModel(&sourceModel);

    QCOMPARE(proxy.rowCount(), 5);
    for (int i = 0; i < proxy.rowCount(); i++) {
        QVERIFY(proxy.data(proxy.index(i,0)).toString().contains('n'));
    }

    proxy.pattern = QString::fromLatin1("o");
    proxy.notifyChange(test);

    QCOMPARE(proxy.rowCount(), works ? 4 : 5);
    bool ok = true;
    for (int i = 0; i < proxy.rowCount(); i++) {
        if (!proxy.data(proxy.index(i,0)).toString().contains('o'))
            ok = false;
    }
    QCOMPARE(ok, works);
}

/**
 * A proxy which changes the background color for items ending in 'y' or 'r'
 */
class CustomDataProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    CustomDataProxy(QObject *parent = 0)
        : QSortFilterProxyModel(parent)
    {
        setDynamicSortFilter(true);
    }

    void setSourceModel(QAbstractItemModel *sourceModel)
    {
        // It would be possible to use only the modelReset signal of the source model to clear
        // the data in *this, however, this requires that the slot is connected
        // before QSortFilterProxyModel::setSourceModel is called, and even then depends
        // on the order of invocation of slots being the same as the order of connection.
        // ie, not reliable.
//         connect(sourceModel, SIGNAL(modelReset()), SLOT(resetInternalData()));
        QSortFilterProxyModel::setSourceModel(sourceModel);
        // Making the connect after the setSourceModel call clears the data too late.
//         connect(sourceModel, SIGNAL(modelReset()), SLOT(resetInternalData()));

        // This could be done in data(), but the point is to need to cache something in the proxy
        // which needs to be cleared on reset.
        for (int i = 0; i < sourceModel->rowCount(); ++i)
        {
            if (sourceModel->index(i, 0).data().toString().endsWith(QLatin1Char('y')))
            {
                m_backgroundColours.insert(i, Qt::blue);
            } else if (sourceModel->index(i, 0).data().toString().endsWith(QLatin1Char('r')))
            {
                m_backgroundColours.insert(i, Qt::red);
            }
        }
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if (role != Qt::BackgroundRole)
            return QSortFilterProxyModel::data(index, role);
        return m_backgroundColours.value(index.row());
    }

private slots:
  void resetInternalData()
  {
      m_backgroundColours.clear();
  }

private:
    QHash<int, QColor> m_backgroundColours;
};

class ModelObserver : public QObject
{
    Q_OBJECT
public:
  ModelObserver(QAbstractItemModel *model, QObject *parent = 0)
    : QObject(parent), m_model(model)
  {
    connect(m_model, SIGNAL(modelAboutToBeReset()), SLOT(modelAboutToBeReset()));
    connect(m_model, SIGNAL(modelReset()), SLOT(modelReset()));
  }

public slots:
  void modelAboutToBeReset()
  {
    int reds = 0, blues = 0;
    for (int i = 0; i < m_model->rowCount(); ++i)
    {
      QColor color = m_model->index(i, 0).data(Qt::BackgroundRole).value<QColor>();
      if (color == Qt::blue)
        ++blues;
      if (color == Qt::red)
        ++reds;
    }
    QCOMPARE(blues, 11);
    QCOMPARE(reds, 4);
  }

  void modelReset()
  {
    int reds = 0, blues = 0;
    for (int i = 0; i < m_model->rowCount(); ++i)
    {
      QColor color = m_model->index(i, 0).data(Qt::BackgroundRole).value<QColor>();
      if (color == Qt::blue)
        ++blues;
      if (color == Qt::red)
        ++reds;
    }
    QCOMPARE(reds, 0);
    QCOMPARE(blues, 0);
  }

private:
  QAbstractItemModel * const m_model;

};

void tst_QSortFilterProxyModel::testResetInternalData()
{

    QStringListModel model(QStringList() << "Monday"
                                         << "Tuesday"
                                         << "Wednesday"
                                         << "Thursday"
                                         << "Friday"
                                         << "January"
                                         << "February"
                                         << "March"
                                         << "April"
                                         << "May"
                                         << "Saturday"
                                         << "June"
                                         << "Sunday"
                                         << "July"
                                         << "August"
                                         << "September"
                                         << "October"
                                         << "November"
                                         << "December");

    CustomDataProxy proxy;
    proxy.setSourceModel(&model);

    ModelObserver observer(&proxy);

    // Cause the source model to reset.
    model.setStringList(QStringList() << "Spam" << "Eggs");

}

void tst_QSortFilterProxyModel::testParentLayoutChanged()
{
    QStandardItemModel model;
    QStandardItem *parentItem = model.invisibleRootItem();
    for (int i = 0; i < 4; ++i) {
        {
            QStandardItem *item = new QStandardItem(QLatin1String("item ") + QString::number(i));
            parentItem->appendRow(item);
        }
        {
            QStandardItem *item = new QStandardItem(QLatin1String("item 1") + QString::number(i));
            parentItem->appendRow(item);
            parentItem = item;
        }
    }
    // item 0
    // item 10
    //  - item 1
    //  - item 11
    //    - item 2
    //    - item 12
    // ...

    QSortFilterProxyModel proxy;
    proxy.sort(0, Qt::AscendingOrder);
    proxy.setDynamicSortFilter(true);

    proxy.setSourceModel(&model);
    proxy.setObjectName("proxy");

    // When Proxy1 emits layoutChanged(QList<QPersistentModelIndex>) this
    // one will too, with mapped indexes.
    QSortFilterProxyModel proxy2;
    proxy2.sort(0, Qt::AscendingOrder);
    proxy2.setDynamicSortFilter(true);

    proxy2.setSourceModel(&proxy);
    proxy2.setObjectName("proxy2");

    QSignalSpy dataChangedSpy(&model, &QSortFilterProxyModel::dataChanged);

    QVERIFY(dataChangedSpy.isValid());

    // Verify that the no-arg signal is still emitted.
    QSignalSpy layoutAboutToBeChangedSpy(&proxy, &QSortFilterProxyModel::layoutAboutToBeChanged);
    QSignalSpy layoutChangedSpy(&proxy, &QSortFilterProxyModel::layoutChanged);

    QVERIFY(layoutAboutToBeChangedSpy.isValid());
    QVERIFY(layoutChangedSpy.isValid());

    QSignalSpy parentsAboutToBeChangedSpy(&proxy, &QSortFilterProxyModel::layoutAboutToBeChanged);
    QSignalSpy parentsChangedSpy(&proxy, &QSortFilterProxyModel::layoutChanged);

    QVERIFY(parentsAboutToBeChangedSpy.isValid());
    QVERIFY(parentsChangedSpy.isValid());

    QSignalSpy proxy2ParentsAboutToBeChangedSpy(&proxy2, &QSortFilterProxyModel::layoutAboutToBeChanged);
    QSignalSpy proxy2ParentsChangedSpy(&proxy2, &QSortFilterProxyModel::layoutChanged);

    QVERIFY(proxy2ParentsAboutToBeChangedSpy.isValid());
    QVERIFY(proxy2ParentsChangedSpy.isValid());

    QStandardItem *item = model.invisibleRootItem()->child(1)->child(1);
    QCOMPARE(item->text(), QStringLiteral("item 11"));

    // Ensure mapped:
    proxy.mapFromSource(model.indexFromItem(item));

    item->setText("Changed");

    QCOMPARE(dataChangedSpy.size(), 1);
    QCOMPARE(layoutAboutToBeChangedSpy.size(), 1);
    QCOMPARE(layoutChangedSpy.size(), 1);
    QCOMPARE(parentsAboutToBeChangedSpy.size(), 1);
    QCOMPARE(parentsChangedSpy.size(), 1);
    QCOMPARE(proxy2ParentsAboutToBeChangedSpy.size(), 1);
    QCOMPARE(proxy2ParentsChangedSpy.size(), 1);

    QVariantList beforeSignal = parentsAboutToBeChangedSpy.first();
    QVariantList afterSignal = parentsChangedSpy.first();

    QCOMPARE(beforeSignal.size(), 2);
    QCOMPARE(afterSignal.size(), 2);

    QList<QPersistentModelIndex> beforeParents = beforeSignal.first().value<QList<QPersistentModelIndex> >();
    QList<QPersistentModelIndex> afterParents = afterSignal.first().value<QList<QPersistentModelIndex> >();

    QCOMPARE(beforeParents.size(), 1);
    QCOMPARE(afterParents.size(), 1);

    QVERIFY(beforeParents.first().isValid());
    QVERIFY(beforeParents.first() == afterParents.first());

    QVERIFY(beforeParents.first() == proxy.mapFromSource(model.indexFromItem(model.invisibleRootItem()->child(1))));

    QList<QPersistentModelIndex> proxy2BeforeList = proxy2ParentsAboutToBeChangedSpy.first().first().value<QList<QPersistentModelIndex> >();
    QList<QPersistentModelIndex> proxy2AfterList = proxy2ParentsChangedSpy.first().first().value<QList<QPersistentModelIndex> >();

    QCOMPARE(proxy2BeforeList.size(), beforeParents.size());
    QCOMPARE(proxy2AfterList.size(), afterParents.size());
    foreach (const QPersistentModelIndex &idx, proxy2BeforeList)
        QVERIFY(beforeParents.contains(proxy2.mapToSource(idx)));
    foreach (const QPersistentModelIndex &idx, proxy2AfterList)
        QVERIFY(afterParents.contains(proxy2.mapToSource(idx)));
}

class SignalArgumentChecker : public QObject
{
    Q_OBJECT
public:
    SignalArgumentChecker(QAbstractItemModel *model, QAbstractProxyModel *proxy, QObject *parent = 0)
      : QObject(parent), m_proxy(proxy)
    {
        connect(model, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)), SLOT(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), SLOT(rowsMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(proxy, SIGNAL(layoutAboutToBeChanged(QList<QPersistentModelIndex>)), SLOT(layoutAboutToBeChanged(QList<QPersistentModelIndex>)));
        connect(proxy, SIGNAL(layoutChanged(QList<QPersistentModelIndex>)), SLOT(layoutChanged(QList<QPersistentModelIndex>)));
    }

private slots:
    void rowsAboutToBeMoved(const QModelIndex &source, int, int, const QModelIndex &destination, int)
    {
        m_p1PersistentBefore = source;
        m_p2PersistentBefore = destination;
        m_p2FirstProxyChild = m_proxy->index(0, 0, m_proxy->mapFromSource(destination));
    }

    void rowsMoved(const QModelIndex &source, int, int, const QModelIndex &destination, int)
    {
        m_p1PersistentAfter = source;
        m_p2PersistentAfter = destination;
    }

    void layoutAboutToBeChanged(const QList<QPersistentModelIndex> &parents)
    {
        QVERIFY(m_p1PersistentBefore.isValid());
        QVERIFY(m_p2PersistentBefore.isValid());
        QCOMPARE(parents.size(), 2);
        QVERIFY(parents.first() != parents.at(1));
        QVERIFY(parents.contains(m_proxy->mapFromSource(m_p1PersistentBefore)));
        QVERIFY(parents.contains(m_proxy->mapFromSource(m_p2PersistentBefore)));
    }

    void layoutChanged(const QList<QPersistentModelIndex> &parents)
    {
        QVERIFY(m_p1PersistentAfter.isValid());
        QVERIFY(m_p2PersistentAfter.isValid());
        QCOMPARE(parents.size(), 2);
        QVERIFY(parents.first() != parents.at(1));
        QVERIFY(parents.contains(m_proxy->mapFromSource(m_p1PersistentAfter)));
        QVERIFY(parents.contains(m_proxy->mapFromSource(m_p2PersistentAfter)));

        // In the source model, the rows were moved to row 1 in the parent.
        // m_p2FirstProxyChild was created with row 0 in the proxy.
        // The moved rows in the proxy do not appear at row 1 because of sorting.
        // Sorting causes them to appear at row 0 instead, pushing what used to
        // be row 0 in the proxy down by two rows.
        QCOMPARE(m_p2FirstProxyChild.row(), 2);
    }

private:
    QAbstractProxyModel *m_proxy;
    QPersistentModelIndex m_p1PersistentBefore;
    QPersistentModelIndex m_p2PersistentBefore;
    QPersistentModelIndex m_p1PersistentAfter;
    QPersistentModelIndex m_p2PersistentAfter;

    QPersistentModelIndex m_p2FirstProxyChild;
};

void tst_QSortFilterProxyModel::moveSourceRows()
{
    DynamicTreeModel model;

    {
        ModelInsertCommand insertCommand(&model);
        insertCommand.setStartRow(0);
        insertCommand.setEndRow(9);
        insertCommand.doCommand();
    }
    {
        ModelInsertCommand insertCommand(&model);
        insertCommand.setAncestorRowNumbers(QList<int>() << 2);
        insertCommand.setStartRow(0);
        insertCommand.setEndRow(9);
        insertCommand.doCommand();
    }
    {
        ModelInsertCommand insertCommand(&model);
        insertCommand.setAncestorRowNumbers(QList<int>() << 5);
        insertCommand.setStartRow(0);
        insertCommand.setEndRow(9);
        insertCommand.doCommand();
    }

    QSortFilterProxyModel proxy;
    proxy.setDynamicSortFilter(true);
    proxy.sort(0, Qt::AscendingOrder);

    // We need to check the arguments at emission time
    SignalArgumentChecker checker(&model, &proxy);

    proxy.setSourceModel(&model);

    QSortFilterProxyModel filterProxy;
    filterProxy.setDynamicSortFilter(true);
    filterProxy.sort(0, Qt::AscendingOrder);
    filterProxy.setSourceModel(&proxy);
    setupFilter(&filterProxy, QLatin1String("6")); // One of the parents

    QSortFilterProxyModel filterBothProxy;
    filterBothProxy.setDynamicSortFilter(true);
    filterBothProxy.sort(0, Qt::AscendingOrder);
    filterBothProxy.setSourceModel(&proxy);
    setupFilter(&filterBothProxy, QLatin1String("5")); // The parents are 6 and 3. This filters both out.

    QSignalSpy modelBeforeSpy(&model, &DynamicTreeModel::rowsAboutToBeMoved);
    QSignalSpy modelAfterSpy(&model, &DynamicTreeModel::rowsMoved);
    QSignalSpy proxyBeforeMoveSpy(m_proxy, &QSortFilterProxyModel::rowsAboutToBeMoved);
    QSignalSpy proxyAfterMoveSpy(m_proxy, &QSortFilterProxyModel::rowsMoved);
    QSignalSpy proxyBeforeParentLayoutSpy(&proxy, &QSortFilterProxyModel::layoutAboutToBeChanged);
    QSignalSpy proxyAfterParentLayoutSpy(&proxy, &QSortFilterProxyModel::layoutChanged);
    QSignalSpy filterBeforeParentLayoutSpy(&filterProxy, &QSortFilterProxyModel::layoutAboutToBeChanged);
    QSignalSpy filterAfterParentLayoutSpy(&filterProxy, &QSortFilterProxyModel::layoutChanged);
    QSignalSpy filterBothBeforeParentLayoutSpy(&filterBothProxy, &QSortFilterProxyModel::layoutAboutToBeChanged);
    QSignalSpy filterBothAfterParentLayoutSpy(&filterBothProxy, &QSortFilterProxyModel::layoutChanged);

    QVERIFY(modelBeforeSpy.isValid());
    QVERIFY(modelAfterSpy.isValid());
    QVERIFY(proxyBeforeMoveSpy.isValid());
    QVERIFY(proxyAfterMoveSpy.isValid());
    QVERIFY(proxyBeforeParentLayoutSpy.isValid());
    QVERIFY(proxyAfterParentLayoutSpy.isValid());
    QVERIFY(filterBeforeParentLayoutSpy.isValid());
    QVERIFY(filterAfterParentLayoutSpy.isValid());
    QVERIFY(filterBothBeforeParentLayoutSpy.isValid());
    QVERIFY(filterBothAfterParentLayoutSpy.isValid());

    {
        ModelMoveCommand moveCommand(&model, 0);
        moveCommand.setAncestorRowNumbers(QList<int>() << 2);
        moveCommand.setDestAncestors(QList<int>() << 5);
        moveCommand.setStartRow(3);
        moveCommand.setEndRow(4);
        moveCommand.setDestRow(1);
        moveCommand.doCommand();
    }

    // Proxy notifies layout change
    QCOMPARE(modelBeforeSpy.size(), 1);
    QCOMPARE(proxyBeforeParentLayoutSpy.size(), 1);
    QCOMPARE(modelAfterSpy.size(), 1);
    QCOMPARE(proxyAfterParentLayoutSpy.size(), 1);

    // But it doesn't notify a move.
    QCOMPARE(proxyBeforeMoveSpy.size(), 0);
    QCOMPARE(proxyAfterMoveSpy.size(), 0);

    QCOMPARE(filterBeforeParentLayoutSpy.size(), 1);
    QCOMPARE(filterAfterParentLayoutSpy.size(), 1);

    QList<QPersistentModelIndex> filterBeforeParents = filterBeforeParentLayoutSpy.first().first().value<QList<QPersistentModelIndex> >();
    QList<QPersistentModelIndex> filterAfterParents = filterAfterParentLayoutSpy.first().first().value<QList<QPersistentModelIndex> >();

    QCOMPARE(filterBeforeParents.size(), 1);
    QCOMPARE(filterAfterParents.size(), 1);

    QCOMPARE(
        filterBeforeParentLayoutSpy.first().at(1).value<QAbstractItemModel::LayoutChangeHint>(),
        QAbstractItemModel::NoLayoutChangeHint);
    QCOMPARE(filterAfterParentLayoutSpy.first().at(1).value<QAbstractItemModel::LayoutChangeHint>(),
             QAbstractItemModel::NoLayoutChangeHint);

    QCOMPARE(filterBothBeforeParentLayoutSpy.size(), 0);
    QCOMPARE(filterBothAfterParentLayoutSpy.size(), 0);
}

class FilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    FilterProxy(QObject *parent = 0)
      : QSortFilterProxyModel(parent),
        mode(false)
    {

    }

public slots:
    void setMode(bool on)
    {
        mode = on;
        invalidateFilter();
    }

protected:
    virtual bool filterAcceptsRow ( int source_row, const QModelIndex & source_parent ) const
    {
        if (mode) {
            if (!source_parent.isValid()) {
                return true;
            } else {
                return (source_row % 2) != 0;
            }
        } else {
            if (!source_parent.isValid()) {
                return source_row >= 2 && source_row < 10;
            } else {
                return true;
            }
        }
    }

private:
    bool mode;
};

void tst_QSortFilterProxyModel::hierarchyFilterInvalidation()
{
    QStandardItemModel model;
    for (int i = 0; i < 10; ++i) {
        const QString rowText = QLatin1String("Row ") + QString::number(i);
        QStandardItem *child = new QStandardItem(rowText);
        for (int j = 0; j < 1; ++j) {
            child->appendRow(new QStandardItem(rowText + QLatin1Char('/') + QString::number(j)));
        }
        model.appendRow(child);
    }

    FilterProxy proxy;
    proxy.setSourceModel(&model);

    QTreeView view;
    view.setModel(&proxy);

    view.setCurrentIndex(proxy.index(2, 0).child(0, 0));

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    proxy.setMode(true);
}

class FilterProxy2 : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    FilterProxy2(QObject *parent = 0)
      : QSortFilterProxyModel(parent),
        mode(false)
    {

    }

public slots:
    void setMode(bool on)
    {
        mode = on;
        invalidateFilter();
    }

protected:
    virtual bool filterAcceptsRow ( int source_row, const QModelIndex & source_parent ) const
    {
        if (source_parent.isValid()) {
            return true;
        } else {
            if (0 == source_row) {
                return true;
            } else {
                return !mode;
            }
        }
    }

private:
    bool mode;
};

void tst_QSortFilterProxyModel::simpleFilterInvalidation()
{
    QStandardItemModel model;
    for (int i = 0; i < 2; ++i) {
        QStandardItem *child = new QStandardItem(QLatin1String("Row ") + QString::number(i));
        child->appendRow(new QStandardItem("child"));
        model.appendRow(child);
    }

    FilterProxy2 proxy;
    proxy.setSourceModel(&model);

    QTreeView view;
    view.setModel(&proxy);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    proxy.setMode(true);
    model.insertRow(0, new QStandardItem("extra"));
}

class CustomRoleNameModel : public QAbstractListModel
{
    Q_OBJECT
public:
    CustomRoleNameModel(QObject *parent = 0) : QAbstractListModel(parent) {}

    QVariant data(const QModelIndex &index, int role) const
    {
        Q_UNUSED(index);
        Q_UNUSED(role);
        return QVariant();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        Q_UNUSED(parent);
        return 0;
    }

    QHash<int, QByteArray> roleNames() const
    {
        QHash<int, QByteArray> rn = QAbstractListModel::roleNames();
        rn[Qt::UserRole + 1] = "custom";
        return rn;
    }
};

void tst_QSortFilterProxyModel::chainedProxyModelRoleNames()
{
    QSortFilterProxyModel proxy1;
    QSortFilterProxyModel proxy2;
    CustomRoleNameModel customModel;

    proxy2.setSourceModel(&proxy1);

    // changing the sourceModel of proxy1 must also update roleNames of proxy2
    proxy1.setSourceModel(&customModel);
    QVERIFY(proxy2.roleNames().value(Qt::UserRole + 1) == "custom");
}

// A source model with ABABAB rows, where only A rows accept drops.
// It will then be sorted by a QSFPM.
class DropOnOddRows : public QAbstractListModel
{
    Q_OBJECT
public:
    DropOnOddRows(QObject *parent = 0) : QAbstractListModel(parent) {}

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DisplayRole)
            return (index.row() % 2 == 0) ? "A" : "B";
        return QVariant();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return 10;
    }

    bool canDropMimeData(const QMimeData *, Qt::DropAction,
                         int row, int column, const QModelIndex &parent) const override
    {
        Q_UNUSED(row);
        Q_UNUSED(column);
        return parent.row() % 2 == 0;
    }
};

class SourceAssertion : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SourceAssertion(QObject *parent = 0)
      : QSortFilterProxyModel(parent)
    {

    }

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override
    {
      Q_ASSERT(sourceModel());
      return QSortFilterProxyModel::mapToSource(proxyIndex);
    }
};

void tst_QSortFilterProxyModel::noMapAfterSourceDelete()
{
    SourceAssertion proxy;
    QStringListModel *model = new QStringListModel(QStringList() << "Foo" << "Bar");

    proxy.setSourceModel(model);

    // Create mappings
    QPersistentModelIndex persistent = proxy.index(0, 0);

    QVERIFY(persistent.isValid());

    delete model;

    QVERIFY(!persistent.isValid());
}

// QTBUG-39549, test whether canDropMimeData(), dropMimeData() are proxied as well
// by invoking them on a QSortFilterProxyModel proxying a QStandardItemModel that allows drops
// on row #1, filtering for that row.
class DropTestModel : public QStandardItemModel {
public:
    explicit DropTestModel(QObject *parent = 0) : QStandardItemModel(0, 1, parent)
    {
        appendRow(new QStandardItem(QStringLiteral("Row0")));
        appendRow(new QStandardItem(QStringLiteral("Row1")));
    }

    bool canDropMimeData(const QMimeData *, Qt::DropAction,
                         int row, int /* column */, const QModelIndex & /* parent */) const override
    { return row == 1; }

    bool dropMimeData(const QMimeData *, Qt::DropAction,
                      int row, int /* column */, const QModelIndex & /* parent */) override
    { return row == 1; }
};

void tst_QSortFilterProxyModel::forwardDropApi()
{
    QSortFilterProxyModel model;
    model.setSourceModel(new DropTestModel(&model));
    model.setFilterFixedString(QStringLiteral("Row1"));
    QCOMPARE(model.rowCount(), 1);
    QVERIFY(model.canDropMimeData(0, Qt::CopyAction, 0, 0, QModelIndex()));
    QVERIFY(model.dropMimeData(0, Qt::CopyAction, 0, 0, QModelIndex()));
}

static QString rowTexts(QAbstractItemModel *model) {
    QString str;
    for (int row = 0 ; row < model->rowCount(); ++row)
        str += model->index(row, 0).data().toString();
    return str;
}

void tst_QSortFilterProxyModel::canDropMimeData()
{
    // Given a source model which only supports dropping on even rows
    DropOnOddRows sourceModel;
    QCOMPARE(rowTexts(&sourceModel), QString("ABABABABAB"));

    // and a proxy model that sorts the rows
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&sourceModel);
    proxy.sort(0, Qt::AscendingOrder);
    QCOMPARE(rowTexts(&proxy), QString("AAAAABBBBB"));

    // the proxy should correctly map canDropMimeData to the source model,
    // i.e. accept drops on the first 5 rows and refuse drops on the next 5.
    for (int row = 0; row < proxy.rowCount(); ++row)
        QCOMPARE(proxy.canDropMimeData(0, Qt::CopyAction, -1, -1, proxy.index(row, 0)), row < 5);
}

void tst_QSortFilterProxyModel::resortingDoesNotBreakTreeModels()
{
    QStandardItemModel *treeModel = new QStandardItemModel(this);
    QStandardItem *e1 = new QStandardItem("Loading...");
    e1->appendRow(new QStandardItem("entry10"));
    treeModel->appendRow(e1);
    QStandardItem *e0 = new QStandardItem("Loading...");
    e0->appendRow(new QStandardItem("entry00"));
    e0->appendRow(new QStandardItem("entry01"));
    treeModel->appendRow(e0);

    QSortFilterProxyModel proxy;
    proxy.setDynamicSortFilter(true);
    proxy.sort(0);
    proxy.setSourceModel(treeModel);

    QAbstractItemModelTester modelTester(&proxy);

    QCOMPARE(proxy.rowCount(), 2);
    e1->setText("entry1");
    e0->setText("entry0");

    QModelIndex pi0 = proxy.index(0, 0);
    QCOMPARE(pi0.data().toString(), QString("entry0"));
    QCOMPARE(proxy.rowCount(pi0), 2);

    QModelIndex pi01 = proxy.index(1, 0, pi0);
    QCOMPARE(pi01.data().toString(), QString("entry01"));

    QModelIndex pi1 = proxy.index(1, 0);
    QCOMPARE(pi1.data().toString(), QString("entry1"));
    QCOMPARE(proxy.rowCount(pi1), 1);
}

void tst_QSortFilterProxyModel::filterHint()
{
    // test that a filtering model does not emit layoutChanged with a hint
    QStringListModel model(QStringList() << "one"
                                         << "two"
                                         << "three"
                                         << "four"
                                         << "five"
                                         << "six");
    QSortFilterProxyModel proxy1;
    proxy1.setSourceModel(&model);
    proxy1.setSortRole(Qt::DisplayRole);
    proxy1.setDynamicSortFilter(true);
    proxy1.sort(0);

    QSortFilterProxyModel proxy2;
    proxy2.setSourceModel(&proxy1);
    proxy2.setFilterRole(Qt::DisplayRole);
    setupFilter(&proxy2, QLatin1String("^[^ ]*$"));
    proxy2.setDynamicSortFilter(true);

    QSignalSpy proxy1BeforeSpy(&proxy1, &QSortFilterProxyModel::layoutAboutToBeChanged);
    QSignalSpy proxy1AfterSpy(&proxy1, &QSortFilterProxyModel::layoutChanged);
    QSignalSpy proxy2BeforeSpy(&proxy2, &QSortFilterProxyModel::layoutAboutToBeChanged);
    QSignalSpy proxy2AfterSpy(&proxy2, &QSortFilterProxyModel::layoutChanged);

    model.setData(model.index(2), QStringLiteral("modified three"), Qt::DisplayRole);

    // The first proxy was re-sorted as one item as changed.
    QCOMPARE(proxy1BeforeSpy.size(), 1);
    QCOMPARE(proxy1BeforeSpy.first().at(1).value<QAbstractItemModel::LayoutChangeHint>(),
             QAbstractItemModel::VerticalSortHint);
    QCOMPARE(proxy1AfterSpy.size(), 1);
    QCOMPARE(proxy1AfterSpy.first().at(1).value<QAbstractItemModel::LayoutChangeHint>(),
             QAbstractItemModel::VerticalSortHint);

    // But the second proxy must not have the VerticalSortHint since an item was filtered
    QCOMPARE(proxy2BeforeSpy.size(), 1);
    QCOMPARE(proxy2BeforeSpy.first().at(1).value<QAbstractItemModel::LayoutChangeHint>(),
             QAbstractItemModel::NoLayoutChangeHint);
    QCOMPARE(proxy2AfterSpy.size(), 1);
    QCOMPARE(proxy2AfterSpy.first().at(1).value<QAbstractItemModel::LayoutChangeHint>(),
             QAbstractItemModel::NoLayoutChangeHint);
}

/**

  Creates a model where each item has one child, to a set depth,
  and the last item has no children.  For a model created with
  setDepth(4):

    - 1
    - - 2
    - - - 3
    - - - - 4
*/
class StepTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    StepTreeModel(QObject * parent = 0)
        : QAbstractItemModel(parent), m_depth(0) {}

    int columnCount(const QModelIndex& = QModelIndex()) const override { return 1; }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        quintptr parentId = (parent.isValid()) ? parent.internalId() : 0;
        return (parentId < m_depth) ? 1 : 0;
    }

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole)
            return QVariant();

        return QString::number(index.internalId());
    }

    QModelIndex index(int, int, const QModelIndex& parent = QModelIndex()) const override
    {
        // QTBUG-44962: Would we always expect the parent to belong to the model
        qDebug() << parent.model() << this;
        Q_ASSERT(!parent.isValid() || parent.model() == this);

        quintptr parentId = (parent.isValid()) ? parent.internalId() : 0;
        if (parentId >= m_depth)
            return QModelIndex();

        return createIndex(0, 0, parentId + 1);
    }

    QModelIndex parent(const QModelIndex& index) const override
    {
        if (index.internalId() == 0)
            return QModelIndex();

        return createIndex(0, 0, index.internalId() - 1);
    }

    void setDepth(quintptr depth)
    {
        int parentIdWithLayoutChange = (m_depth < depth) ? m_depth : depth;

        QList<QPersistentModelIndex> parentsOfLayoutChange;
        parentsOfLayoutChange.push_back(createIndex(0, 0, parentIdWithLayoutChange));

        layoutAboutToBeChanged(parentsOfLayoutChange);

        auto existing = persistentIndexList();

        QList<QModelIndex> updated;

        for (auto idx : existing) {
            if (indexDepth(idx) <= depth)
                updated.push_back(idx);
            else
                updated.push_back({});
        }

        m_depth = depth;

        changePersistentIndexList(existing, updated);

        layoutChanged(parentsOfLayoutChange);
    }

private:
    static quintptr indexDepth(QModelIndex const& index)
    {
        return (index.isValid()) ? 1 + indexDepth(index.parent()) : 0;
    }

private:
    quintptr m_depth;
};

void tst_QSortFilterProxyModel::sourceLayoutChangeLeavesValidPersistentIndexes()
{
    StepTreeModel model;
    Q_SET_OBJECT_NAME(model);
    model.setDepth(4);

    QSortFilterProxyModel proxy1;
    proxy1.setSourceModel(&model);
    Q_SET_OBJECT_NAME(proxy1);
    setupFilter(&proxy1, QLatin1String("1|2"));

    // The current state of things:
    //  model         proxy
    //   - 1           - 1
    //   - - 2         - - 2
    //   - - - 3
    //   - - - - 4

    // The setDepth call below removes '4' with a layoutChanged call.
    // Because the proxy filters that out anyway, the proxy doesn't need
    // to emit any signals or update persistent indexes.

    QPersistentModelIndex persistentIndex = proxy1.index(0, 0, proxy1.index(0, 0));

    model.setDepth(3);

    // Calling parent() causes the internalPointer to be used.
    // Before fixing QTBUG-47711, that could be a dangling pointer.
    // The use of qDebug here makes sufficient use of the heap to
    // cause corruption at runtime with normal use on linux (before
    // the fix). valgrind confirms the fix.
    qDebug() << persistentIndex.parent();
    QVERIFY(persistentIndex.parent().isValid());
}

void tst_QSortFilterProxyModel::rowMoveLeavesValidPersistentIndexes()
{
    DynamicTreeModel model;
    Q_SET_OBJECT_NAME(model);

    QList<int> ancestors;
    for (auto i = 0; i < 5; ++i)
    {
        Q_UNUSED(i);
        ModelInsertCommand insertCommand(&model);
        insertCommand.setAncestorRowNumbers(ancestors);
        insertCommand.setStartRow(0);
        insertCommand.setEndRow(0);
        insertCommand.doCommand();
        ancestors.push_back(0);
    }

    QSortFilterProxyModel proxy1;
    proxy1.setSourceModel(&model);
    Q_SET_OBJECT_NAME(proxy1);

    setupFilter(&proxy1, QLatin1String("1|2"));

    auto item5 = model.match(model.index(0, 0), Qt::DisplayRole, "5", 1, Qt::MatchRecursive).first();
    auto item3 = model.match(model.index(0, 0), Qt::DisplayRole, "3", 1, Qt::MatchRecursive).first();

    Q_ASSERT(item5.isValid());
    Q_ASSERT(item3.isValid());

    QPersistentModelIndex persistentIndex = proxy1.match(proxy1.index(0, 0), Qt::DisplayRole, "2", 1, Qt::MatchRecursive).first();

    ModelMoveCommand moveCommand(&model, 0);
    moveCommand.setAncestorRowNumbers(QList<int>{0, 0, 0, 0});
    moveCommand.setStartRow(0);
    moveCommand.setEndRow(0);
    moveCommand.setDestRow(0);
    moveCommand.setDestAncestors(QList<int>{0, 0, 0});
    moveCommand.doCommand();

    // Calling parent() causes the internalPointer to be used.
    // Before fixing QTBUG-47711 (moveRows case), that could be
    // a dangling pointer.
    QVERIFY(persistentIndex.parent().isValid());
}

void tst_QSortFilterProxyModel::emitLayoutChangedOnlyIfSortingChanged_data()
{
    QTest::addColumn<int>("changedRow");
    QTest::addColumn<Qt::ItemDataRole>("changedRole");
    QTest::addColumn<QString>("newData");
    QTest::addColumn<QString>("expectedSourceRowTexts");
    QTest::addColumn<QString>("expectedProxyRowTexts");
    QTest::addColumn<int>("expectedLayoutChanged");

    // Starting point:
    // a source model with 8,7,6,5,4,3,2,1
    // a proxy model keeping only even rows and sorting them, therefore showing 2,4,6,8

    // When setData changes ordering, layoutChanged should be emitted
    QTest::newRow("ordering_change") << 0 << Qt::DisplayRole << "0" << "07654321" << "0246" << 1;

    // When setData on visible row doesn't change ordering, layoutChanged should not be emitted
    QTest::newRow("no_ordering_change") << 6 << Qt::DisplayRole << "0" << "87654301" << "0468" << 0;

    // When setData happens on a filtered out row, layoutChanged should not be emitted
    QTest::newRow("filtered_out") << 1 << Qt::DisplayRole << "9" << "89654321" << "2468" << 0;

    // When setData makes a row visible, layoutChanged should not be emitted (rowsInserted is emitted instead)
    QTest::newRow("make_row_visible") << 7 << Qt::DisplayRole << "0" << "87654320" << "02468" << 0;

    // When setData makes a row hidden, layoutChanged should not be emitted (rowsRemoved is emitted instead)
    QTest::newRow("make_row_hidden") << 4 << Qt::DisplayRole << "1" << "87651321" << "268" << 0;

    // When setData happens on an unrelated role, layoutChanged should not be emitted
    QTest::newRow("unrelated_role") << 0 << Qt::DecorationRole << "" << "87654321" << "2468" << 0;

    // When many changes happen together... and trigger removal, insertion, and layoutChanged
    QTest::newRow("many_changes") << -1 << Qt::DisplayRole << "3,4,2,5,6,0,7,9" << "34256079" << "0246" << 1;

    // When many changes happen together... and trigger removal, insertion, but no change in ordering of visible rows => no layoutChanged
    QTest::newRow("many_changes_no_layoutChanged") << -1 << Qt::DisplayRole << "7,5,4,3,2,1,0,8" << "75432108" << "0248" << 0;
}

// Custom version of QStringListModel which supports emitting dataChanged for many rows at once
class CustomStringListModel : public QAbstractListModel
{
public:
    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (index.row() >= 0 && index.row() < lst.size()
            && (role == Qt::EditRole || role == Qt::DisplayRole)) {
            lst.replace(index.row(), value.toString());
            emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
            return true;
        }
        return false;
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return lst.at(index.row());
        return QVariant();
    }
    int rowCount(const QModelIndex & = QModelIndex()) const override { return lst.count(); }

    void replaceData(const QStringList &newData)
    {
        lst = newData;
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0), { Qt::DisplayRole, Qt::EditRole });
    }

    void emitDecorationChangedSignal()
    {
        const QModelIndex idx = index(0, 0);
        emit dataChanged(idx, idx, { Qt::DecorationRole });
    }

private:
    QStringList lst;
};

void tst_QSortFilterProxyModel::emitLayoutChangedOnlyIfSortingChanged()
{
    QFETCH(int, changedRow);
    QFETCH(QString, newData);
    QFETCH(Qt::ItemDataRole, changedRole);
    QFETCH(QString, expectedSourceRowTexts);
    QFETCH(QString, expectedProxyRowTexts);
    QFETCH(int, expectedLayoutChanged);

    CustomStringListModel model;
    QStringList strings;
    for (auto i = 8; i >= 1; --i)
        strings.append(QString::number(i));
    model.replaceData(strings);
    QCOMPARE(rowTexts(&model), QStringLiteral("87654321"));

    class FilterEvenRowsProxyModel : public QSortFilterProxyModel
    {
    public:
        bool filterAcceptsRow(int srcRow, const QModelIndex& srcParent) const override
        {
            return sourceModel()->index(srcRow, 0, srcParent).data().toInt() % 2 == 0;
        }
    };

    FilterEvenRowsProxyModel proxy;
    proxy.sort(0);
    proxy.setSourceModel(&model);
    QCOMPARE(rowTexts(&proxy), QStringLiteral("2468"));

    QSignalSpy modelDataChangedSpy(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy proxyLayoutChangedSpy(&proxy, &QAbstractItemModel::layoutChanged);

    if (changedRole == Qt::DecorationRole)
        model.emitDecorationChangedSignal();
    else if (changedRow == -1)
        model.replaceData(newData.split(QLatin1Char(',')));
    else
        model.setData(model.index(changedRow, 0), newData, changedRole);

    QCOMPARE(rowTexts(&model), expectedSourceRowTexts);
    QCOMPARE(rowTexts(&proxy), expectedProxyRowTexts);
    QCOMPARE(modelDataChangedSpy.size(), 1);
    QCOMPARE(proxyLayoutChangedSpy.size(), expectedLayoutChanged);
}

void tst_QSortFilterProxyModel::removeIntervals_data()
{
    QTest::addColumn<QStringList>("sourceItems");
    QTest::addColumn<int>("sortOrder");
    QTest::addColumn<QString>("filter");
    QTest::addColumn<QStringList>("replacementSourceItems");
    QTest::addColumn<IntPairList>("expectedRemovedProxyIntervals");
    QTest::addColumn<QStringList>("expectedProxyItems");

    QTest::newRow("filter all, sort ascending")
        << (QStringList() << "a"
                          << "b"
                          << "c") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "[^x]" // filter
        << (QStringList() << "x"
                          << "x"
                          << "x") // replacementSourceItems
        << (IntPairList() << IntPair(0, 2)) // expectedRemovedIntervals
        << QStringList() // expectedProxyItems
        ;

    QTest::newRow("filter all, sort descending")
        << (QStringList() << "a"
                          << "b"
                          << "c") // sourceItems
        << static_cast<int>(Qt::DescendingOrder) // sortOrder
        << "[^x]" // filter
        << (QStringList() << "x"
                          << "x"
                          << "x") // replacementSourceItems
        << (IntPairList() << IntPair(0, 2)) // expectedRemovedIntervals
        << QStringList() // expectedProxyItems
        ;

    QTest::newRow("filter first and last, sort ascending")
        << (QStringList() << "a"
                          << "b"
                          << "c") // sourceItems
        << static_cast<int>(Qt::AscendingOrder) // sortOrder
        << "[^x]" // filter
        << (QStringList() << "x"
                          << "b"
                          << "x") // replacementSourceItems
        << (IntPairList() << IntPair(2, 2) << IntPair(0, 0)) // expectedRemovedIntervals
        << (QStringList() << "b") // expectedProxyItems
        ;

    QTest::newRow("filter first and last, sort descending")
        << (QStringList() << "a"
                          << "b"
                          << "c") // sourceItems
        << static_cast<int>(Qt::DescendingOrder) // sortOrder
        << "[^x]" // filter
        << (QStringList() << "x"
                          << "b"
                          << "x") // replacementSourceItems
        << (IntPairList() << IntPair(2, 2) << IntPair(0, 0)) // expectedRemovedIntervals
        << (QStringList() << "b") // expectedProxyItems
        ;
}

void tst_QSortFilterProxyModel::removeIntervals()
{
    QFETCH(QStringList, sourceItems);
    QFETCH(int, sortOrder);
    QFETCH(QString, filter);
    QFETCH(QStringList, replacementSourceItems);
    QFETCH(IntPairList, expectedRemovedProxyIntervals);
    QFETCH(QStringList, expectedProxyItems);

    CustomStringListModel model;
    QSortFilterProxyModel proxy;

    model.replaceData(sourceItems);
    proxy.setSourceModel(&model);

    for (int i = 0; i < sourceItems.count(); ++i) {
        QModelIndex sindex = model.index(i, 0, QModelIndex());
        QModelIndex pindex = proxy.index(i, 0, QModelIndex());
        QCOMPARE(proxy.data(pindex, Qt::DisplayRole), model.data(sindex, Qt::DisplayRole));
    }

    proxy.setDynamicSortFilter(true);

    if (sortOrder != -1)
        proxy.sort(0, static_cast<Qt::SortOrder>(sortOrder));
    if (!filter.isEmpty())
        setupFilter(&proxy, filter);

    (void)proxy.rowCount(QModelIndex()); // force mapping

    QSignalSpy removeSpy(&proxy, &QSortFilterProxyModel::rowsRemoved);
    QSignalSpy insertSpy(&proxy, &QSortFilterProxyModel::rowsInserted);
    QSignalSpy aboutToRemoveSpy(&proxy, &QSortFilterProxyModel::rowsAboutToBeRemoved);
    QSignalSpy aboutToInsertSpy(&proxy, &QSortFilterProxyModel::rowsAboutToBeInserted);

    QVERIFY(removeSpy.isValid());
    QVERIFY(insertSpy.isValid());
    QVERIFY(aboutToRemoveSpy.isValid());
    QVERIFY(aboutToInsertSpy.isValid());

    model.replaceData(replacementSourceItems);

    QCOMPARE(aboutToRemoveSpy.count(), expectedRemovedProxyIntervals.count());
    for (int i = 0; i < aboutToRemoveSpy.count(); ++i) {
        QList<QVariant> args = aboutToRemoveSpy.at(i);
        QCOMPARE(args.at(1).type(), QVariant::Int);
        QCOMPARE(args.at(2).type(), QVariant::Int);
        QCOMPARE(args.at(1).toInt(), expectedRemovedProxyIntervals.at(i).first);
        QCOMPARE(args.at(2).toInt(), expectedRemovedProxyIntervals.at(i).second);
    }
    QCOMPARE(removeSpy.count(), expectedRemovedProxyIntervals.count());
    for (int i = 0; i < removeSpy.count(); ++i) {
        QList<QVariant> args = removeSpy.at(i);
        QCOMPARE(args.at(1).type(), QVariant::Int);
        QCOMPARE(args.at(2).type(), QVariant::Int);
        QCOMPARE(args.at(1).toInt(), expectedRemovedProxyIntervals.at(i).first);
        QCOMPARE(args.at(2).toInt(), expectedRemovedProxyIntervals.at(i).second);
    }

    QCOMPARE(insertSpy.count(), 0);
    QCOMPARE(aboutToInsertSpy.count(), 0);

    QCOMPARE(proxy.rowCount(QModelIndex()), expectedProxyItems.count());
    for (int i = 0; i < expectedProxyItems.count(); ++i) {
        QModelIndex pindex = proxy.index(i, 0, QModelIndex());
        QCOMPARE(proxy.data(pindex, Qt::DisplayRole).toString(), expectedProxyItems.at(i));
    }
}

void tst_QSortFilterProxyModel::dynamicFilterWithoutSort()
{
    QStringListModel model;
    const QStringList initial = QString("bravo charlie delta echo").split(QLatin1Char(' '));
    model.setStringList(initial);
    QSortFilterProxyModel proxy;
    proxy.setDynamicSortFilter(true);
    proxy.setSourceModel(&model);

    QSignalSpy layoutChangeSpy(&proxy, &QAbstractItemModel::layoutChanged);
    QSignalSpy resetSpy(&proxy, &QAbstractItemModel::modelReset);

    QVERIFY(layoutChangeSpy.isValid());
    QVERIFY(resetSpy.isValid());

    model.setStringList(QStringList() << "Monday" << "Tuesday" << "Wednesday" << "Thursday" << "Friday");

    QVERIFY(layoutChangeSpy.isEmpty());

    QCOMPARE(model.stringList(), QStringList() << "Monday" << "Tuesday" << "Wednesday" << "Thursday" << "Friday");

    QCOMPARE(resetSpy.count(), 1);
}

void tst_QSortFilterProxyModel::checkSetNewModel()
{
    QTreeView tv;
    StepTreeModel model1;
    model1.setDepth(4);

    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&model1);
    tv.setModel(&proxy);
    tv.show();
    QVERIFY(QTest::qWaitForWindowExposed(&tv));
    tv.expandAll();
    {
        StepTreeModel model2;
        model2.setDepth(4);
        proxy.setSourceModel(&model2);
        tv.expandAll();
        proxy.setSourceModel(&model1);
        tv.expandAll();
        // the destruction of model2 here caused a proxy model reset due to
        // missing disconnect in setSourceModel()
    }
    // handle repaint events, will assert when qsortfilterproxymodel is in wrong state
    QCoreApplication::processEvents();
}

enum ColumnFilterMode {
    FilterNothing,
    FilterOutMiddle,
    FilterOutBeginEnd,
    FilterAll
};
Q_DECLARE_METATYPE(ColumnFilterMode)

void tst_QSortFilterProxyModel::filterAndInsertColumn_data()
{

    QTest::addColumn<int>("insertCol");
    QTest::addColumn<ColumnFilterMode>("filterMode");
    QTest::addColumn<QStringList>("expectedModelList");
    QTest::addColumn<QStringList>("expectedProxyModelList");

    QTest::newRow("at_beginning_filter_out_middle")
        << 0
        << FilterOutMiddle
        << QStringList{{"", "A1", "B1", "C1", "D1"}}
        << QStringList{{"", "D1"}}
    ;
    QTest::newRow("at_end_filter_out_middle")
        << 2
        << FilterOutMiddle
        << QStringList{{"A1", "B1", "C1", "D1", ""}}
        << QStringList{{"A1", ""}}
    ;
    QTest::newRow("in_the_middle_filter_out_middle")
        << 1
        << FilterOutMiddle
        << QStringList{{"A1", "B1", "C1", "", "D1"}}
        << QStringList{{"A1", "D1"}}
    ;
    QTest::newRow("at_beginning_filter_out_begin_and_end")
        << 0
        << FilterOutBeginEnd
        << QStringList{{"A1", "", "B1", "C1", "D1"}}
        << QStringList{{"", "B1", "C1"}}
    ;
    QTest::newRow("at_end_filter_out_begin_and_end")
        << 2
        << FilterOutBeginEnd
        << QStringList{{"A1", "B1", "C1", "D1", ""}}
        << QStringList{{"B1", "C1", "D1"}}
    ;
    QTest::newRow("in_the_middle_filter_out_begin_and_end")
        << 1
        << FilterOutBeginEnd
        << QStringList{{"A1", "B1", "", "C1", "D1"}}
        << QStringList{{"B1", "", "C1"}}
    ;

    QTest::newRow("at_beginning_filter_nothing")
        << 0
        << FilterAll
        << QStringList{{"", "A1", "B1", "C1", "D1"}}
        << QStringList{{"", "A1", "B1", "C1", "D1"}}
    ;
    QTest::newRow("at_end_filter_nothing")
        << 4
        << FilterAll
        << QStringList{{"A1", "B1", "C1", "D1", ""}}
        << QStringList{{"A1", "B1", "C1", "D1", ""}}
    ;
    QTest::newRow("in_the_middle_nothing")
        << 2
        << FilterAll
        << QStringList{{"A1", "B1", "", "C1", "D1"}}
        << QStringList{{"A1", "B1", "", "C1", "D1"}}
    ;

    QTest::newRow("filter_all")
        << 0
        << FilterNothing
        << QStringList{{"A1", "B1", "C1", "D1", ""}}
        << QStringList{}
    ;
}
void tst_QSortFilterProxyModel::filterAndInsertColumn()
{

    class ColumnFilterProxy : public QSortFilterProxyModel {
        Q_DISABLE_COPY(ColumnFilterProxy)
        ColumnFilterMode filerMode;
    public:
        ColumnFilterProxy(ColumnFilterMode mode)
            : filerMode(mode)
        {}
        bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override
        {
            Q_UNUSED(source_parent)
            switch (filerMode){
            case FilterAll:
                return true;
            case FilterNothing:
                return false;
            case FilterOutMiddle:
                return source_column == 0 || source_column == sourceModel()->columnCount() - 1;
            case FilterOutBeginEnd:
                return source_column > 0 && source_column< sourceModel()->columnCount() - 1;
            }
            Q_UNREACHABLE();
        }
    };
    QFETCH(int, insertCol);
    QFETCH(ColumnFilterMode, filterMode);
    QFETCH(QStringList, expectedModelList);
    QFETCH(QStringList, expectedProxyModelList);
    QStandardItemModel model;
    model.insertColumns(0, 4);
    model.insertRows(0, 1);
    for (int i = 0; i < model.rowCount(); ++i) {
        for (int j = 0; j < model.columnCount(); ++j)
            model.setData(model.index(i, j), QString('A' + j) + QString::number(i + 1));
    }
    ColumnFilterProxy proxy(filterMode);
    proxy.setSourceModel(&model);
    QVERIFY(proxy.insertColumn(insertCol));
    proxy.invalidate();
    QStringList modelStringList;
    for (int i = 0; i < model.rowCount(); ++i) {
        for (int j = 0; j < model.columnCount(); ++j)
            modelStringList.append(model.index(i, j).data().toString());
    }
    QCOMPARE(expectedModelList, modelStringList);
    modelStringList.clear();
    for (int i = 0; i < proxy.rowCount(); ++i) {
        for (int j = 0; j < proxy.columnCount(); ++j)
            modelStringList.append(proxy.index(i, j).data().toString());
    }
    QCOMPARE(expectedProxyModelList, modelStringList);
}

void tst_QSortFilterProxyModel::filterAndInsertRow_data()
{
    QTest::addColumn<QStringList>("initialModelList");
    QTest::addColumn<int>("row");
    QTest::addColumn<QString>("filterRegExp");
    QTest::addColumn<QStringList>("expectedModelList");
    QTest::addColumn<QStringList>("expectedProxyModelList");

    QTest::newRow("at_beginning_filter_out_middle")
        << QStringList{{"A5", "B5", "B6", "A7"}}
        << 0
        << "^A"
        << QStringList{{"", "A5", "B5", "B6", "A7"}}
        << QStringList{{"A5", "A7"}};
    QTest::newRow("at_end_filter_out_middle")
        << QStringList{{"A5", "B5", "B6", "A7"}}
        << 2
        << "^A"
        << QStringList{{"A5", "B5", "B6", "A7", ""}}
        << QStringList{{"A5", "A7"}};
    QTest::newRow("in_the_middle_filter_out_middle")
        << QStringList{{"A5", "B5", "B6", "A7"}}
        << 1
        << "^A"
        << QStringList{{"A5", "B5", "B6", "", "A7"}}
        << QStringList{{"A5", "A7"}};

    QTest::newRow("at_beginning_filter_out_first_and_last")
        << QStringList{{"A5", "B5", "B6", "A7"}}
        << 0
        << "^B"
        << QStringList{{"A5", "", "B5", "B6", "A7"}}
        << QStringList{{"B5", "B6"}};
    QTest::newRow("at_end_filter_out_first_and_last")
        << QStringList{{"A5", "B5", "B6", "A7"}}
        << 2
        << "^B"
        << QStringList{{"A5", "B5", "B6", "A7", ""}}
        << QStringList{{"B5", "B6"}};
    QTest::newRow("in_the_middle_filter_out_first_and_last")
        << QStringList{{"A5", "B5", "B6", "A7"}}
        << 1
        << "^B"
        << QStringList{{"A5", "B5", "", "B6", "A7"}}
        << QStringList{{"B5", "B6"}};

    QTest::newRow("at_beginning_no_filter")
        << QStringList{{"A5", "B5", "B6", "A7"}}
        << 0
        << ".*"
        << QStringList{{"", "A5", "B5", "B6", "A7"}}
        << QStringList{{"", "A5", "B5", "B6", "A7"}};
    QTest::newRow("at_end_no_filter")
        << QStringList{{"A5", "B5", "B6", "A7"}}
        << 4
        << ".*"
        << QStringList{{"A5", "B5", "B6", "A7", ""}}
        << QStringList{{"A5", "B5", "B6", "A7", ""}};
    QTest::newRow("in_the_middle_no_filter")
        << QStringList{{"A5", "B5", "B6", "A7"}}
        << 2
        << ".*"
        << QStringList{{"A5", "B5", "", "B6", "A7"}}
        << QStringList{{"A5", "B5", "", "B6", "A7"}};

    QTest::newRow("filter_all")
        << QStringList{{"A5", "B5", "B6", "A7"}}
        << 0
        << "$a"
        << QStringList{{"A5", "B5", "B6", "A7", ""}}
        << QStringList{};
}

void tst_QSortFilterProxyModel::filterAndInsertRow()
{
    QFETCH(QStringList, initialModelList);
    QFETCH(int, row);
    QFETCH(QString, filterRegExp);
    QFETCH(QStringList, expectedModelList);
    QFETCH(QStringList, expectedProxyModelList);
    QStringListModel model;
    QSortFilterProxyModel proxyModel;

    model.setStringList(initialModelList);
    proxyModel.setSourceModel(&model);
    proxyModel.setDynamicSortFilter(true);
    proxyModel.setFilterRegExp(filterRegExp);

    QVERIFY(proxyModel.insertRow(row));
    QCOMPARE(model.stringList(), expectedModelList);
    QCOMPARE(proxyModel.rowCount(), expectedProxyModelList.count());
    for (int r = 0; r < proxyModel.rowCount(); ++r) {
      QModelIndex index = proxyModel.index(r, 0);
      QVERIFY(index.isValid());
      QCOMPARE(proxyModel.data(index).toString(), expectedProxyModelList.at(r));
    }
}

#include "tst_qsortfilterproxymodel.moc"
