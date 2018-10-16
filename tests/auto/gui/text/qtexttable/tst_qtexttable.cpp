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


#include <qtextdocument.h>
#include <qtextdocumentfragment.h>
#include <qtexttable.h>
#include <qdebug.h>
#include <qtextcursor.h>
#include <qtextdocument.h>
#ifndef QT_NO_WIDGETS
#include <qtextedit.h>
#endif
#ifndef QT_NO_PRINTER
#include <QPagedPaintDevice>
#include <QPainter>
#include <QPaintEngine>
#endif

typedef QList<int> IntList;

QT_FORWARD_DECLARE_CLASS(QTextDocument)

class tst_QTextTable : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void cursorPositioning();
    void variousTableModifications();
    void tableShrinking();
    void spans();
    void variousModifications2();
    void tableManager_undo();
    void tableManager_removeCell();
    void rowAt();
    void rowAtWithSpans();
    void multiBlockCells();
    void insertRows();
    void deleteInTable();
    void mergeCells();
    void mergeAndInsert();
    void splitCells();
    void blocksForTableShouldHaveEmptyFormat();
    void removeTableByRemoveRows();
    void removeTableByRemoveColumns();
    void setCellFormat();
    void removeRows1();
    void removeRows2();
    void removeRows3();
    void removeRows4();
    void removeRows5();
    void removeColumns1();
    void removeColumns2();
    void removeColumns3();
    void removeColumns4();
    void removeColumns5();
    void removeColumnsInTableWithMergedRows();
#ifndef QT_NO_WIDGETS
    void QTBUG11282_insertBeforeMergedEnding_data();
    void QTBUG11282_insertBeforeMergedEnding();
#endif
    void QTBUG22011_insertBeforeRowSpan();
#ifndef QT_NO_PRINTER
    void QTBUG31330_renderBackground();
#endif

private:
    QTextTable *create2x2Table();
    QTextTable *create4x4Table();

    QTextTable *createTable(int rows, int cols);

    QTextDocument *doc;
    QTextCursor cursor;
};

void tst_QTextTable::init()
{
    doc = new QTextDocument;
    cursor = QTextCursor(doc);
}

void tst_QTextTable::cleanup()
{
    cursor = QTextCursor();
    delete doc;
    doc = 0;
}

void tst_QTextTable::cursorPositioning()
{
    // ensure the cursor is placed at the beginning of the first cell upon
    // table creation
    QTextTable *table = cursor.insertTable(2, 2);

    QVERIFY(cursor == table->cellAt(0, 0).firstCursorPosition());
    QVERIFY(table->cellAt(0, 0).firstPosition() == table->firstPosition());
}

void tst_QTextTable::variousTableModifications()
{
    QTextTableFormat tableFmt;

    QTextTable *tab = cursor.insertTable(2, 2, tableFmt);
    QCOMPARE(doc->toPlainText().length(), 5);
    QCOMPARE(tab, cursor.currentTable());
    QCOMPARE(tab->columns(), 2);
    QCOMPARE(tab->rows(), 2);

    QCOMPARE(cursor.position(), 1);
    QTextCharFormat fmt = cursor.charFormat();
    QCOMPARE(fmt.objectIndex(), -1);
    QTextTableCell cell = tab->cellAt(cursor);
    QVERIFY(cell.isValid());
    QCOMPARE(cell.row(), 0);
    QCOMPARE(cell.column(), 0);

    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.position(), 2);
    fmt = cursor.charFormat();
    QCOMPARE(fmt.objectIndex(), -1);
    cell = tab->cellAt(cursor);
    QVERIFY(cell.isValid());
    QCOMPARE(cell.row(), 0);
    QCOMPARE(cell.column(), 1);

    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.position(), 3);
    fmt = cursor.charFormat();
    QCOMPARE(fmt.objectIndex(), -1);
    cell = tab->cellAt(cursor);
    QVERIFY(cell.isValid());
    QCOMPARE(cell.row(), 1);
    QCOMPARE(cell.column(), 0);

    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.position(), 4);
    fmt = cursor.charFormat();
    QCOMPARE(fmt.objectIndex(), -1);
    cell = tab->cellAt(cursor);
    QVERIFY(cell.isValid());
    QCOMPARE(cell.row(), 1);
    QCOMPARE(cell.column(), 1);

    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.position(), 5);
    fmt = cursor.charFormat();
    QCOMPARE(fmt.objectIndex(), -1);
    cell = tab->cellAt(cursor);
    QVERIFY(!cell.isValid());

    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.position(), 5);

    // check we can't delete the cells with the cursor
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.position(), 1);
    cursor.deleteChar();
    QCOMPARE(doc->toPlainText().length(), 5);
    cursor.movePosition(QTextCursor::NextBlock);
    QCOMPARE(cursor.position(), 2);
    cursor.deleteChar();
    QCOMPARE(doc->toPlainText().length(), 5);
    cursor.deletePreviousChar();
    QCOMPARE(cursor.position(), 2);
    QCOMPARE(doc->toPlainText().length(), 5);

    QTextTable *table = cursor.currentTable();
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);

    table->insertRows(2, 1);
    QCOMPARE(table->rows(), 3);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(doc->toPlainText().length(), 7);
    table->insertColumns(2, 2);
    QCOMPARE(table->rows(), 3);
    QCOMPARE(table->columns(), 4);
    QCOMPARE(doc->toPlainText().length(), 13);

    table->resize(4, 5);
    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 5);
    QCOMPARE(doc->toPlainText().length(), 21);
}

void tst_QTextTable::tableShrinking()
{
    QTextTableFormat tableFmt;

    cursor.insertTable(3, 4, tableFmt);
    QCOMPARE(doc->toPlainText().length(), 13);

    QTextTable *table = cursor.currentTable();
    QCOMPARE(table->rows(), 3);
    QCOMPARE(table->columns(), 4);

    table->removeRows(1, 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 4);
    QCOMPARE(doc->toPlainText().length(), 9);
    table->removeColumns(1, 2);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(doc->toPlainText().length(), 5);

    table->resize(1, 1);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->columns(), 1);
    QCOMPARE(doc->toPlainText().length(), 2);
}

void tst_QTextTable::spans()
{
    QTextTableFormat tableFmt;

    cursor.insertTable(2, 2, tableFmt);

    QTextTable *table = cursor.currentTable();
    QVERIFY(table->cellAt(0, 0) != table->cellAt(0, 1));
    table->mergeCells(0, 0, 1, 2);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);
    QVERIFY(table->cellAt(0, 0) == table->cellAt(0, 1));
    table->mergeCells(0, 0, 2, 2);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);
}

void tst_QTextTable::variousModifications2()
{
    QTextTableFormat tableFmt;

    cursor.insertTable(2, 5, tableFmt);
    QCOMPARE(doc->toPlainText().length(), 11);
    QTextTable *table = cursor.currentTable();
    QCOMPARE(cursor.position(), 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 5);

    table->insertColumns(0, 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 6);
    table->insertColumns(6, 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 7);

    table->insertRows(0, 1);
    QCOMPARE(table->rows(), 3);
    QCOMPARE(table->columns(), 7);
    table->insertRows(3, 1);
    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 7);

    table->removeRows(0, 1);
    QCOMPARE(table->rows(), 3);
    QCOMPARE(table->columns(), 7);
    table->removeRows(2, 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 7);

    table->removeColumns(0, 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 6);
    table->removeColumns(5, 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 5);

    tableFmt = table->format();
    table->insertColumns(2, 1);
    table->setFormat(tableFmt);
    table->insertColumns(2, 1);
    QCOMPARE(table->columns(), 7);
}

void tst_QTextTable::tableManager_undo()
{
    QTextTableFormat fmt;
    fmt.setBorder(10);
    QTextTable *table = cursor.insertTable(2, 2, fmt);
    QVERIFY(table);

    QCOMPARE(table->format().border(), qreal(10));

    fmt.setBorder(20);
    table->setFormat(fmt);

    QCOMPARE(table->format().border(), qreal(20));

    doc->undo();

    QCOMPARE(table->format().border(), qreal(10));
}

void tst_QTextTable::tableManager_removeCell()
{
    // essentially a test for TableManager::removeCell, in particular to remove empty items from the rowlist.
    // If it fails it'll triger assertions inside TableManager. Yeah, not pretty, should VERIFY here ;(
    cursor.insertTable(2, 2, QTextTableFormat());
    doc->undo();
    // ###
    QVERIFY(true);
}

void tst_QTextTable::rowAt()
{
    // test TablePrivate::rowAt
    QTextTable *table = cursor.insertTable(4, 2);

    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 2);

    QTextCursor cell00Cursor = table->cellAt(0, 0).firstCursorPosition();
    QTextCursor cell10Cursor = table->cellAt(1, 0).firstCursorPosition();
    QTextCursor cell20Cursor = table->cellAt(2, 0).firstCursorPosition();
    QTextCursor cell21Cursor = table->cellAt(2, 1).firstCursorPosition();
    QTextCursor cell30Cursor = table->cellAt(3, 0).firstCursorPosition();
    QCOMPARE(table->cellAt(cell00Cursor).firstCursorPosition(), cell00Cursor);
    QCOMPARE(table->cellAt(cell10Cursor).firstCursorPosition(), cell10Cursor);
    QCOMPARE(table->cellAt(cell20Cursor).firstCursorPosition(), cell20Cursor);
    QCOMPARE(table->cellAt(cell30Cursor).firstCursorPosition(), cell30Cursor);

    table->mergeCells(1, 0, 2, 1);

    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 2);

    QVERIFY(cell00Cursor == table->cellAt(0, 0).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(1, 0).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(2, 0).firstCursorPosition());
    QVERIFY(cell21Cursor == table->cellAt(2, 1).firstCursorPosition());
    QVERIFY(cell30Cursor == table->cellAt(3, 0).firstCursorPosition());

    table->mergeCells(1, 0, 2, 2);

    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 2);

    QVERIFY(cell00Cursor == table->cellAt(0, 0).firstCursorPosition());
    QVERIFY(cell00Cursor == table->cellAt(0, 0).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(1, 0).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(1, 1).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(2, 0).firstCursorPosition());
    QVERIFY(cell10Cursor == table->cellAt(2, 1).firstCursorPosition());
    QVERIFY(cell30Cursor == table->cellAt(3, 0).firstCursorPosition());
}

void tst_QTextTable::rowAtWithSpans()
{
    QTextTable *table = cursor.insertTable(2, 2);

    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);

    table->mergeCells(0, 0, 2, 1);
    QVERIFY(table->cellAt(0, 0).rowSpan() == 2);

    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);

    table->mergeCells(0, 0, 2, 2);
    QVERIFY(table->cellAt(0, 0).columnSpan() == 2);

    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);
}

void tst_QTextTable::multiBlockCells()
{
    // little testcase for multi-block cells
    QTextTable *table = cursor.insertTable(2, 2);

    QVERIFY(cursor == table->cellAt(0, 0).firstCursorPosition());

    cursor.insertText("Hello");
    cursor.insertBlock(QTextBlockFormat());
    cursor.insertText("World");

    cursor.movePosition(QTextCursor::Left);
    QVERIFY(table->cellAt(0, 0) == table->cellAt(cursor));
}

void tst_QTextTable::insertRows()
{
    // little testcase for multi-block cells
    QTextTable *table = cursor.insertTable(2, 2);

    QVERIFY(cursor == table->cellAt(0, 0).firstCursorPosition());

    table->insertRows(0, 1);
    QCOMPARE(table->rows(), 3);

    table->insertRows(1, 1);
    QCOMPARE(table->rows(), 4);

    table->insertRows(-1, 1);
    QCOMPARE(table->rows(), 5);

    table->insertRows(5, 2);
    QCOMPARE(table->rows(), 7);

}

void tst_QTextTable::deleteInTable()
{
    QTextTable *table = cursor.insertTable(2, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("Blah");
    table->cellAt(0, 1).firstCursorPosition().insertText("Foo");
    table->cellAt(1, 0).firstCursorPosition().insertText("Bar");
    table->cellAt(1, 1).firstCursorPosition().insertText("Hah");

    cursor = table->cellAt(1, 1).firstCursorPosition();
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);

    QCOMPARE(table->cellAt(cursor.position()).row(), 1);
    QCOMPARE(table->cellAt(cursor.position()).column(), 0);

    cursor.removeSelectedText();

    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->rows(), 2);

    // verify table is still all in shape. Only the text inside should get deleted
    for (int row = 0; row < table->rows(); ++row)
        for (int col = 0; col < table->columns(); ++col) {
            const QTextTableCell cell = table->cellAt(row, col);
            QVERIFY(cell.isValid());
            QCOMPARE(cell.rowSpan(), 1);
            QCOMPARE(cell.columnSpan(), 1);
        }
}

QTextTable *tst_QTextTable::create2x2Table()
{
    cleanup();
    init();
    QTextTable *table = cursor.insertTable(2, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("Blah");
    table->cellAt(0, 1).firstCursorPosition().insertText("Foo");
    table->cellAt(1, 0).firstCursorPosition().insertText("Bar");
    table->cellAt(1, 1).firstCursorPosition().insertText("Hah");
    return table;
}

QTextTable *tst_QTextTable::create4x4Table()
{
    cleanup();
    init();
    QTextTable *table = cursor.insertTable(4, 4);
    table->cellAt(0, 0).firstCursorPosition().insertText("Blah");
    table->cellAt(0, 1).firstCursorPosition().insertText("Foo");
    table->cellAt(1, 0).firstCursorPosition().insertText("Bar");
    table->cellAt(1, 1).firstCursorPosition().insertText("Hah");
    return table;
}

QTextTable *tst_QTextTable::createTable(int rows, int cols)
{
    cleanup();
    init();
    QTextTable *table = cursor.insertTable(rows, cols);
    return table;
}

void tst_QTextTable::mergeCells()
{
    QTextTable *table = create4x4Table();

    table->mergeCells(1, 1, 1, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));

    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table = create4x4Table();

    table->mergeCells(1, 1, 2, 1);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));

    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table = create4x4Table();

    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    // should do nothing
    table->mergeCells(1, 1, 1, 1);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table = create2x2Table();

    table->mergeCells(0, 1, 2, 1);
    table->mergeCells(0, 0, 2, 2);
    QVERIFY(table->cellAt(0, 0) == table->cellAt(0, 1));
    QVERIFY(table->cellAt(0, 0) == table->cellAt(1, 0));
    QVERIFY(table->cellAt(0, 0) == table->cellAt(1, 1));

    QTextBlock block = table->cellAt(0, 0).firstCursorPosition().block();

    QCOMPARE(block.text(), QLatin1String("Blah Foo"));
    QCOMPARE(block.next().text(), QLatin1String("Hah"));
    QCOMPARE(block.next().next().text(), QLatin1String("Bar"));

    table = create4x4Table();

    QTextCursor cursor = table->cellAt(3, 3).firstCursorPosition();
    QTextTable *t2 = cursor.insertTable(2, 2);
    t2->cellAt(0, 0).firstCursorPosition().insertText("Test");

    table->mergeCells(2, 2, 2, 2);
    cursor = table->cellAt(2, 2).firstCursorPosition();

    QTextFrame *frame = cursor.currentFrame();

    QTextFrame::iterator it = frame->begin();

    // find the embedded table
    while (it != frame->end() && !it.currentFrame())
        ++it;

    table = qobject_cast<QTextTable *>(it.currentFrame());

    QVERIFY(table);

    if (table) {
        cursor = table->cellAt(0, 0).firstCursorPosition();

        QCOMPARE(cursor.block().text(), QLatin1String("Test"));
    }

    table = create2x2Table();

    table->mergeCells(0, 1, 2, 1);

    QVERIFY(table->cellAt(0, 0) != table->cellAt(0, 1));
    QVERIFY(table->cellAt(0, 1) == table->cellAt(1, 1));

    // should do nothing
    table->mergeCells(0, 0, 1, 2);

    QVERIFY(table->cellAt(0, 0) != table->cellAt(0, 1));
    QVERIFY(table->cellAt(0, 1) == table->cellAt(1, 1));
}

void tst_QTextTable::mergeAndInsert()
{
    QTextTable *table = cursor.insertTable(4,3);
    table->mergeCells(0,1,3,2);
    table->mergeCells(3,0,1,3);
    //Don't crash !
    table->insertColumns(1,2);
    QCOMPARE(table->columns(), 5);
}

void tst_QTextTable::splitCells()
{
    QTextTable *table = create4x4Table();
    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table->splitCell(1, 1, 1, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 2));

    table->splitCell(1, 1, 1, 1);
    QVERIFY(table->cellAt(1, 1) != table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 2));


    table = create4x4Table();
    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table->splitCell(1, 1, 2, 1);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 2));

    table->splitCell(1, 1, 1, 1);
    QVERIFY(table->cellAt(1, 1) != table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 2));


    table = create4x4Table();
    table->mergeCells(1, 1, 2, 2);
    QVERIFY(table->cellAt(1, 1) == table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) == table->cellAt(2, 2));

    table->splitCell(1, 1, 1, 1);
    QVERIFY(table->cellAt(1, 1) != table->cellAt(1, 2));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 1));
    QVERIFY(table->cellAt(1, 1) != table->cellAt(2, 2));

    table = createTable(2, 5);
    table->mergeCells(0, 0, 2, 1);
    table->mergeCells(0, 1, 2, 1);
    QVERIFY(table->cellAt(0, 0) == table->cellAt(1, 0));
    QVERIFY(table->cellAt(0, 1) == table->cellAt(1, 1));
    table->splitCell(0, 0, 1, 1);
    QVERIFY(table->cellAt(0, 0) != table->cellAt(1, 0));
    QVERIFY(table->cellAt(0, 1) == table->cellAt(1, 1));

    table = createTable(2, 5);
    table->mergeCells(0, 4, 2, 1);
    QVERIFY(table->cellAt(0, 4) == table->cellAt(1, 4));

    table->splitCell(0, 4, 1, 1);
    QVERIFY(table->cellAt(0, 4) != table->cellAt(1, 4));
}

void tst_QTextTable::blocksForTableShouldHaveEmptyFormat()
{
    QTextBlockFormat fmt;
    fmt.setProperty(QTextFormat::UserProperty, true);
    cursor.insertBlock(fmt);
    QVERIFY(cursor.blockFormat().hasProperty(QTextFormat::UserProperty));

    QTextTable *table = cursor.insertTable(1, 1);
    QVERIFY(!table->cellAt(0, 0).firstCursorPosition().blockFormat().hasProperty(QTextFormat::UserProperty));

    int userPropCount = 0;
    for (QTextBlock block = doc->begin();
         block.isValid(); block = block.next()) {
        if (block.blockFormat().hasProperty(QTextFormat::UserProperty))
            userPropCount++;
    }
    QCOMPARE(userPropCount, 1);
}

void tst_QTextTable::removeTableByRemoveRows()
{
    QPointer<QTextTable> table1 = QTextCursor(cursor).insertTable(4, 4);
    QPointer<QTextTable> table2 = QTextCursor(cursor).insertTable(4, 4);
    QPointer<QTextTable> table3 = QTextCursor(cursor).insertTable(4, 4);

    QVERIFY(table1);
    QVERIFY(table2);
    QVERIFY(table3);

    table2->removeRows(1, 1);

    QVERIFY(table1);
    QVERIFY(table2);
    QVERIFY(table3);

    table2->removeRows(0, table2->rows());

    QVERIFY(table1);
    QVERIFY(!table2);
    QVERIFY(table3);
}

void tst_QTextTable::removeTableByRemoveColumns()
{
    QPointer<QTextTable> table1 = QTextCursor(cursor).insertTable(4, 4);
    QPointer<QTextTable> table2 = QTextCursor(cursor).insertTable(4, 4);
    QPointer<QTextTable> table3 = QTextCursor(cursor).insertTable(4, 4);

    QVERIFY(table1);
    QVERIFY(table2);
    QVERIFY(table3);

    table2->removeColumns(1, 1);

    QVERIFY(table1);
    QVERIFY(table2);
    QVERIFY(table3);

    table2->removeColumns(0, table2->columns());

    QVERIFY(table1);
    QVERIFY(!table2);
    QVERIFY(table3);
}

void tst_QTextTable::setCellFormat()
{
    QTextTable *table = cursor.insertTable(2, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(1, 0).firstCursorPosition().insertText("Third");
    table->cellAt(1, 1).firstCursorPosition().insertText("Fourth");
    QTextTableCell cell = table->cellAt(0, 0);
    QTextCharFormat fmt;
    fmt.setObjectIndex(23);
    fmt.setBackground(Qt::blue);
    fmt.setTableCellColumnSpan(25);
    fmt.setTableCellRowSpan(42);
    cell.setFormat(fmt);
    QCOMPARE(cell.format().background().color(), QColor(Qt::blue));
    QCOMPARE(cell.format().tableCellColumnSpan(), 1);
    QCOMPARE(cell.format().tableCellRowSpan(), 1);
}

void tst_QTextTable::removeRows1()
{
    QTextTable *table = cursor.insertTable(2, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(1, 0).firstCursorPosition().insertText("Third");
    table->cellAt(1, 1).firstCursorPosition().insertText("Fourth");
    table->removeRows(0, 1);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("Third"));
    QCOMPARE(table->cellAt(0, 1).firstCursorPosition().block().text(), QString("Fourth"));
}

void tst_QTextTable::removeRows2()
{
    QTextTable *table = cursor.insertTable(2, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(1, 0).firstCursorPosition().insertText("Third");
    table->cellAt(1, 1).firstCursorPosition().insertText("Fourth");
    table->removeRows(1, 1);
    QCOMPARE(table->rows(), 1);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("First"));
    QCOMPARE(table->cellAt(0, 1).firstCursorPosition().block().text(), QString("Second"));
}

void tst_QTextTable::removeRows3()
{
    QTextTable *table = cursor.insertTable(3, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(1, 0).firstCursorPosition().insertText("Third");
    table->cellAt(1, 1).firstCursorPosition().insertText("Fourth");
    table->cellAt(2, 0).firstCursorPosition().insertText("Fifth");
    table->cellAt(2, 1).firstCursorPosition().insertText("Sixth");
    table->removeRows(1, 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("First"));
    QCOMPARE(table->cellAt(0, 1).firstCursorPosition().block().text(), QString("Second"));
    QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("Fifth"));
    QCOMPARE(table->cellAt(1, 1).firstCursorPosition().block().text(), QString("Sixth"));
}

void tst_QTextTable::removeRows4()
{
    QTextTable *table = cursor.insertTable(4, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(1, 0).firstCursorPosition().insertText("Third");
    table->cellAt(1, 1).firstCursorPosition().insertText("Fourth");
    table->cellAt(2, 0).firstCursorPosition().insertText("Fifth");
    table->cellAt(2, 1).firstCursorPosition().insertText("Sixth");
    table->cellAt(3, 0).firstCursorPosition().insertText("Seventh");
    table->cellAt(3, 1).firstCursorPosition().insertText("Eighth");
    table->removeRows(1, 2);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("First"));
    QCOMPARE(table->cellAt(0, 1).firstCursorPosition().block().text(), QString("Second"));
    QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("Seventh"));
    QCOMPARE(table->cellAt(1, 1).firstCursorPosition().block().text(), QString("Eighth"));
}

void tst_QTextTable::removeRows5()
{
    QTextTable *table = cursor.insertTable(2,2);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(1, 0).firstCursorPosition().insertText("Third");
    table->cellAt(1, 1).firstCursorPosition().insertText("Fourth");
    table->insertRows(1,1);
    table->mergeCells(1,0,1,2);
    table->removeRows(1,1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("First"));
    QCOMPARE(table->cellAt(0, 1).firstCursorPosition().block().text(), QString("Second"));
    QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("Third"));
    QCOMPARE(table->cellAt(1, 1).firstCursorPosition().block().text(), QString("Fourth"));
}

void tst_QTextTable::removeColumns1()
{
    QTextTable *table = cursor.insertTable(2, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(1, 0).firstCursorPosition().insertText("Third");
    table->cellAt(1, 1).firstCursorPosition().insertText("Fourth");
    table->removeColumns(0, 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 1);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("Second"));
    QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("Fourth"));
}

void tst_QTextTable::removeColumns2()
{
    QTextTable *table = cursor.insertTable(2, 2);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(1, 0).firstCursorPosition().insertText("Third");
    table->cellAt(1, 1).firstCursorPosition().insertText("Fourth");
    table->removeColumns(1, 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 1);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("First"));
    QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("Third"));
}

void tst_QTextTable::removeColumns3()
{
    QTextTable *table = cursor.insertTable(2, 3);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(0, 2).firstCursorPosition().insertText("Third");
    table->cellAt(1, 0).firstCursorPosition().insertText("Fourth");
    table->cellAt(1, 1).firstCursorPosition().insertText("Fifth");
    table->cellAt(1, 2).firstCursorPosition().insertText("Sixth");
    table->removeColumns(1, 1);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("First"));
    QCOMPARE(table->cellAt(0, 1).firstCursorPosition().block().text(), QString("Third"));
    QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("Fourth"));
    QCOMPARE(table->cellAt(1, 1).firstCursorPosition().block().text(), QString("Sixth"));
}

void tst_QTextTable::removeColumns4()
{
    QTextTable *table = cursor.insertTable(2, 4);
    table->cellAt(0, 0).firstCursorPosition().insertText("First");
    table->cellAt(0, 1).firstCursorPosition().insertText("Second");
    table->cellAt(0, 2).firstCursorPosition().insertText("Third");
    table->cellAt(0, 3).firstCursorPosition().insertText("Fourth");
    table->cellAt(1, 0).firstCursorPosition().insertText("Fifth");
    table->cellAt(1, 1).firstCursorPosition().insertText("Sixth");
    table->cellAt(1, 2).firstCursorPosition().insertText("Seventh");
    table->cellAt(1, 3).firstCursorPosition().insertText("Eighth");
    table->removeColumns(1, 2);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QString("First"));
    QCOMPARE(table->cellAt(0, 1).firstCursorPosition().block().text(), QString("Fourth"));
    QCOMPARE(table->cellAt(1, 0).firstCursorPosition().block().text(), QString("Fifth"));
    QCOMPARE(table->cellAt(1, 1).firstCursorPosition().block().text(), QString("Eighth"));
}

void tst_QTextTable::removeColumns5()
{
    QTextTable *table = cursor.insertTable(4, 4);
    QTextCursor tc (doc);
    tc.setPosition(table->cellAt(2,0).firstPosition());
    tc.setPosition(table->cellAt(3,1).firstPosition(), QTextCursor::KeepAnchor);
    table->mergeCells(tc);
    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 4);
    QCOMPARE(table->cellAt(0, 0).firstPosition(), 1);
    QCOMPARE(table->cellAt(0, 1).firstPosition(), 2);
    QCOMPARE(table->cellAt(0, 2).firstPosition(), 3);
    QCOMPARE(table->cellAt(0, 3).firstPosition(), 4);
    QCOMPARE(table->cellAt(1, 0).firstPosition(), 5);
    QCOMPARE(table->cellAt(1, 1).firstPosition(), 6);
    QCOMPARE(table->cellAt(1, 2).firstPosition(), 7);
    QCOMPARE(table->cellAt(1, 3).firstPosition(), 8);
    QCOMPARE(table->cellAt(2, 0).firstPosition(), 9);
    QCOMPARE(table->cellAt(2, 0).rowSpan(), 2);
    QCOMPARE(table->cellAt(2, 0).columnSpan(), 2);
    QCOMPARE(table->cellAt(2, 1).firstPosition(), 9);
    QCOMPARE(table->cellAt(2, 2).firstPosition(), 10);
    QCOMPARE(table->cellAt(2, 3).firstPosition(), 11);
    QCOMPARE(table->cellAt(3, 0).firstPosition(), 9);
    QCOMPARE(table->cellAt(3, 1).firstPosition(), 9);
    QCOMPARE(table->cellAt(3, 2).firstPosition(), 12);
    QCOMPARE(table->cellAt(3, 3).firstPosition(), 13);

    table->removeColumns(1, 1);
    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 3);
    QCOMPARE(table->cellAt(0, 0).firstPosition(), 1);
    QCOMPARE(table->cellAt(0, 1).firstPosition(), 2);
    QCOMPARE(table->cellAt(0, 2).firstPosition(), 3);
    QCOMPARE(table->cellAt(1, 0).firstPosition(), 4);
    QCOMPARE(table->cellAt(1, 1).firstPosition(), 5);
    QCOMPARE(table->cellAt(1, 2).firstPosition(), 6);
    QCOMPARE(table->cellAt(2, 0).firstPosition(), 7);
    QCOMPARE(table->cellAt(2, 0).rowSpan(), 2);
    QCOMPARE(table->cellAt(2, 0).columnSpan(), 1);
    QCOMPARE(table->cellAt(2, 1).firstPosition(), 8);
    QCOMPARE(table->cellAt(2, 2).firstPosition(), 9);
    QCOMPARE(table->cellAt(3, 0).firstPosition(), 7);
    QCOMPARE(table->cellAt(3, 1).firstPosition(), 10);
    QCOMPARE(table->cellAt(3, 2).firstPosition(), 11);
}

void tst_QTextTable::removeColumnsInTableWithMergedRows()
{
    QTextTable *table = cursor.insertTable(3, 4);
    table->mergeCells(0, 0, 1, 4);
    QCOMPARE(table->rows(), 3);
    QCOMPARE(table->columns(), 4);

    table->removeColumns(0, table->columns() - 1);

    QCOMPARE(table->rows(), 3);
    QCOMPARE(table->columns(), 1);
}

#ifndef QT_NO_WIDGETS
void tst_QTextTable::QTBUG11282_insertBeforeMergedEnding_data()
{
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("columns");
    QTest::addColumn<QList<int> >("merge");
    QTest::addColumn<QList<int> >("insert");

    QTest::newRow("2x3, merge two, insert one") << 2 << 3 << (QList<int>() << 1 << 2 << 2)
            << (QList<int>() << 1 << 1) ;
    QTest::newRow("3x4, merge three, insert one") << 3 << 4 << (QList<int>() << 1 << 3 << 3)
            << (QList<int>() << 1 << 1) ;
    QTest::newRow("4x3, merge two, insert two") << 4 << 3 << (QList<int>() << 1 << 4 << 2)
            << (QList<int>() << 1 << 2) ;
    QTest::newRow("4x4, merge middle two, insert one") << 4 << 4 << (QList<int>() << 1 << 4 << 2)
            << (QList<int>() << 1 << 1) ;
}

void tst_QTextTable::QTBUG11282_insertBeforeMergedEnding()
{
    QFETCH(int, rows);
    QFETCH(int, columns);
    QFETCH(QList<int>, merge);
    QFETCH(QList<int>, insert);
    QTextTable *table = cursor.insertTable(rows, columns);
    QTextEdit *textEdit = new QTextEdit;
    textEdit->setDocument(doc);
    textEdit->show();
    QVERIFY(QTest::qWaitForWindowExposed(textEdit));
    table->mergeCells(0,merge.at(0), merge.at(1), merge.at(2));
    //Don't crash !
    table->insertColumns(insert.at(0), insert.at(1));
    //Check that the final size is what we expected
    QCOMPARE(table->rows(), rows);
    QCOMPARE(table->columns(), columns + insert.at(1));
    delete textEdit;
}
#endif

void tst_QTextTable::QTBUG22011_insertBeforeRowSpan()
{
    QTextDocument doc;
    QTextCursor cursor(&doc);
    QTextTable *table = cursor.insertTable(1,1); // 1x1

    table->appendColumns(1); // 1x2
    table->appendRows(1); // 2x2
    table->mergeCells(0, 0, 2, 1); // 2x2
    table->insertColumns(1, 1); // 2x3
    table->mergeCells(0, 1, 1, 2); // 2x3
    table->appendRows(1); // 3x3
    table->mergeCells(0, 0, 3, 1); // 3x3
    table->appendRows(1); // 4x3
    table->insertColumns(1, 1); // 4x4
    table->mergeCells(0, 1, 1, 3);
    table->mergeCells(1, 1, 1, 2);
    table->mergeCells(2, 1, 1, 2);
    table->mergeCells(3, 0, 1, 2);
    table->insertColumns(3, 1); // 4x5
    table->mergeCells(0, 1, 1, 4);

    table->appendColumns(1); // 4x6

    QCOMPARE(table->rows(), 4);
    QCOMPARE(table->columns(), 6);
}

#ifndef QT_NO_PRINTER
namespace {
class QTBUG31330_PaintDevice : public QPagedPaintDevice
{
public:
    class PaintEngine : public QPaintEngine
    {
    public:
        QList<QRectF> rects;

        PaintEngine()
            : QPaintEngine(0)
        {}
        virtual Type type() const
        {
            return User;
        }
        virtual bool begin(QPaintDevice *)
        {
            return true;
        }
        virtual bool end()
        {
            return true;
        }
        virtual void updateState(const QPaintEngineState &)
        {}
        virtual void drawRects(const QRect *, int)
        {}
        virtual void drawRects(const QRectF *r, int)
        {
            if (painter()->brush() == QBrush(Qt::green))
            {
                rects.append(*r);
            }
        }
        virtual void drawPixmap(const QRectF &, const QPixmap &, const QRectF &)
        {}
    };

    int pages;
    QPaintEngine* engine;

    QTBUG31330_PaintDevice(QPaintEngine* engine)
        : pages(1), engine(engine)
    {
        QPageLayout layout = pageLayout();
        layout.setUnits(QPageLayout::Point);
        setPageLayout(layout);
    }
    virtual int metric(PaintDeviceMetric metric) const
    {
        if (PdmDevicePixelRatio == metric)
            return 1;
        if (PdmDevicePixelRatioScaled == metric)
            return 1 * QPaintDevice::devicePixelRatioFScale();
        if (PdmDpiY == metric)
            return 96;
        if (PdmDpiX == metric)
            return 96;
        if (PdmHeight == metric)
            return 1000;
        if (PdmWidth == metric)
            return 700;
        return 900;
    }
    virtual QPaintEngine *paintEngine() const
    {
        return engine;
    }
    bool newPage()
    {
        ++pages;
        return true;
    }
};
}

void tst_QTextTable::QTBUG31330_renderBackground()
{
    QTextDocument doc;
    QTextCursor cursor(&doc);
    QTextTable* table = cursor.insertTable(4, 2);

    QTextTableCell cell = table->cellAt(3, 0);

    QTextCharFormat cellFormat = cell.format();
    cellFormat.setBackground(QBrush(Qt::green));
    cell.setFormat(cellFormat);

    QTextCursor tc = cell.firstCursorPosition();
    for (int i = 0; i < 60; ++i) {
        tc.insertBlock();
    }
    QTBUG31330_PaintDevice::PaintEngine engine;
    QTBUG31330_PaintDevice paintDevice(&engine);
    paintDevice.setPageSize(QPagedPaintDevice::A4);
    doc.print(&paintDevice);

    QVERIFY(paintDevice.pages >= 2);
    QCOMPARE(engine.rects.count(), paintDevice.pages);
    for (int i = 0; i < engine.rects.count(); ++i) {
        QRectF rect = engine.rects[i];
        QVERIFY(rect.top() > 0);
        QVERIFY(rect.bottom() < 1000);
    }
}
#endif

QTEST_MAIN(tst_QTextTable)
#include "tst_qtexttable.moc"
