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
#include <qabstracttextdocumentlayout.h>
#include <qdebug.h>
#include <qpainter.h>
#include <qtexttable.h>
#ifndef QT_NO_WIDGETS
#include <qtextedit.h>
#include <qscrollbar.h>
#endif

class tst_QTextDocumentLayout : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void cleanupTestCase();
    void defaultPageSizeHandling();
    void idealWidth();
    void lineSeparatorFollowingTable();
#ifndef QT_NO_WIDGETS
    void wrapAtWordBoundaryOrAnywhere();
#endif
    void inlineImage();
    void clippedTableCell();
    void floatingTablePageBreak();
    void imageAtRightAlignedTab();
    void blockVisibility();

private:
    QTextDocument *doc;
};

void tst_QTextDocumentLayout::init()
{
    doc = new QTextDocument;
}

void tst_QTextDocumentLayout::cleanup()
{
    delete doc;
    doc = 0;
}

void tst_QTextDocumentLayout::cleanupTestCase()
{
    if (qgetenv("QTEST_KEEP_IMAGEDATA").toInt() == 0) {
        QFile::remove(QLatin1String("expected.png"));
        QFile::remove(QLatin1String("img.png"));
    }
}

void tst_QTextDocumentLayout::defaultPageSizeHandling()
{
    QAbstractTextDocumentLayout *layout = doc->documentLayout();
    QVERIFY(layout);

    QVERIFY(!doc->pageSize().isValid());
    QSizeF docSize = layout->documentSize();
    QVERIFY(docSize.width() > 0 && docSize.width() < 1000);
    QVERIFY(docSize.height() > 0 && docSize.height() < 1000);

    doc->setPlainText("Some text\nwith a few lines\nand not real information\nor anything otherwise useful");

    docSize = layout->documentSize();
    QVERIFY(docSize.isValid());
    QVERIFY(docSize.width() != INT_MAX);
    QVERIFY(docSize.height() != INT_MAX);
}

void tst_QTextDocumentLayout::idealWidth()
{
    doc->setPlainText("Some text\nwith a few lines\nand not real information\nor anything otherwise useful");
    doc->setTextWidth(1000);
    QCOMPARE(doc->textWidth(), qreal(1000));
    QCOMPARE(doc->size().width(), doc->textWidth());
    QVERIFY(doc->idealWidth() < doc->textWidth());
    QVERIFY(doc->idealWidth() > 0);

    QTextBlockFormat fmt;
    fmt.setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    QTextCursor cursor(doc);
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(fmt);

    QCOMPARE(doc->textWidth(), qreal(1000));
    QCOMPARE(doc->size().width(), doc->textWidth());
    QVERIFY(doc->idealWidth() < doc->textWidth());
    QVERIFY(doc->idealWidth() > 0);
}

// none of the QTextLine items in the document should intersect with the margin rect
void tst_QTextDocumentLayout::lineSeparatorFollowingTable()
{
    QString html_begin("<html><table border=1><tr><th>Column 1</th></tr><tr><td>Data</td></tr></table><br>");
    QString html_text("bla bla bla bla bla bla bla bla<br>");
    QString html_end("<table border=1><tr><th>Column 1</th></tr><tr><td>Data</td></tr></table></html>");

    QString html = html_begin;

    for (int i = 0; i < 80; ++i)
        html += html_text;

    html += html_end;

    doc->setHtml(html);

    QTextCursor cursor(doc);
    cursor.movePosition(QTextCursor::Start);

    const int margin = 87;
    const int pageWidth = 873;
    const int pageHeight = 1358;

    QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
    fmt.setMargin(margin);
    doc->rootFrame()->setFrameFormat(fmt);

    QFont font(doc->defaultFont());
    font.setPointSize(10);
    doc->setDefaultFont(font);
    doc->setPageSize(QSizeF(pageWidth, pageHeight));

    QRectF marginRect(QPointF(0, pageHeight - margin), QSizeF(pageWidth, 2 * margin));

    // force layouting
    doc->pageCount();

    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        QTextLayout *layout = block.layout();
        for (int i = 0; i < layout->lineCount(); ++i) {
            QTextLine line = layout->lineAt(i);
            QRectF rect = line.rect().translated(layout->position());
            QVERIFY(!rect.intersects(marginRect));
        }
    }
}

#ifndef QT_NO_WIDGETS
void tst_QTextDocumentLayout::wrapAtWordBoundaryOrAnywhere()
{
    //task 150562
    QTextEdit edit;
    edit.setText("<table><tr><td>hello hello hello"
            "thisisabigwordthisisabigwordthisisabigwordthisisabigwordthisisabigword"
            "hello hello hello</td></tr></table>");
    edit.setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    edit.resize(100, 100);
    edit.show();
    QVERIFY(!edit.horizontalScrollBar()->isVisible());
}
#endif

void tst_QTextDocumentLayout::inlineImage()
{
    doc->setPageSize(QSizeF(800, 500));

    QImage img(400, 400, QImage::Format_RGB32);
    QLatin1String name("bigImage");

    doc->addResource(QTextDocument::ImageResource, QUrl(name), img);

    QTextImageFormat imgFormat;
    imgFormat.setName(name);
    imgFormat.setWidth(img.width());

    QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
    qreal height = doc->pageSize().height() - fmt.topMargin() - fmt.bottomMargin();
    imgFormat.setHeight(height);

    QTextCursor cursor(doc);
    cursor.insertImage(imgFormat);

    QCOMPARE(doc->pageCount(), 1);
}

void tst_QTextDocumentLayout::clippedTableCell()
{
    const char *html =
        "<table style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\""
        "border=\"0\" margin=\"0\" cellpadding=\"0\" cellspacing=\"0\"><tr><td></td></tr></table>";

    doc->setHtml(html);
    doc->pageSize();

    QTextCursor cursor(doc);
    cursor.movePosition(QTextCursor::Right);

    QTextTable *table = cursor.currentTable();
    QVERIFY(table);

    QTextCursor cellCursor = table->cellAt(0, 0).firstCursorPosition();
    QImage src(16, 16, QImage::Format_ARGB32_Premultiplied);
    src.fill(0xffff0000);
    cellCursor.insertImage(src);

    QTextBlock block = cellCursor.block();
    QRectF r = doc->documentLayout()->blockBoundingRect(block);

    QRectF rect(0, 0, r.left() + 1, 64);

    QImage img(64, 64, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x0);
    QImage expected = img;
    QPainter p(&img);
    doc->drawContents(&p, rect);
    p.end();
    p.begin(&expected);
    r.setWidth(1);
    p.fillRect(r, Qt::red);
    p.end();

    img.save("img.png");
    expected.save("expected.png");
    QCOMPARE(img, expected);
}

void tst_QTextDocumentLayout::floatingTablePageBreak()
{
    doc->clear();

    QTextCursor cursor(doc);

    QTextTableFormat tableFormat;
    tableFormat.setPosition(QTextFrameFormat::FloatLeft);
    QTextTable *table = cursor.insertTable(50, 1, tableFormat);
    Q_UNUSED(table);

    // Make height of document 2/3 of the table, fitting the table into two pages
    QSizeF documentSize = doc->size();
    documentSize.rheight() *= 2.0 / 3.0;

    doc->setPageSize(documentSize);

    QCOMPARE(doc->pageCount(), 2);
}

void tst_QTextDocumentLayout::imageAtRightAlignedTab()
{
    doc->clear();

    QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
    fmt.setMargin(0);
    doc->rootFrame()->setFrameFormat(fmt);

    QTextCursor cursor(doc);
    QTextBlockFormat blockFormat;
    QList<QTextOption::Tab> tabs;
    QTextOption::Tab tab;
    tab.position = 300;
    tab.type = QTextOption::RightTab;
    tabs.append(tab);
    blockFormat.setTabPositions(tabs);

    // First block: text, some of it right-aligned
    cursor.insertBlock(blockFormat);
    cursor.insertText("first line\t");
    cursor.insertText("right-aligned text");

    // Second block: text, then right-aligned image
    cursor.insertBlock(blockFormat);
    cursor.insertText("second line\t");
    QImage img(48, 48, QImage::Format_RGB32);
    const QString name = QString::fromLatin1("image");
    doc->addResource(QTextDocument::ImageResource, QUrl(name), img);
    QTextImageFormat imgFormat;
    imgFormat.setName(name);
    cursor.insertImage(imgFormat);

    // Everything should fit into the 300 pixels
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Fails on winrt. Figure out why - QTBUG-68297", Continue);
#endif
    QCOMPARE(doc->idealWidth(), 300.0);
}

void tst_QTextDocumentLayout::blockVisibility()
{
    QTextCursor cursor(doc);
    for (int i = 0; i < 10; ++i) {
        if (!doc->isEmpty())
            cursor.insertBlock();
        cursor.insertText("A");
    }

    qreal margin = doc->documentMargin();
    QSizeF emptySize(2 * margin, 2 * margin);
    QSizeF halfSize = doc->size();
    halfSize.rheight() -= 2 * margin;
    halfSize.rheight() /= 2;
    halfSize.rheight() += 2 * margin;

    for (int i = 0; i < 10; i += 2) {
        QTextBlock block = doc->findBlockByNumber(i);
        block.setVisible(false);
        doc->markContentsDirty(block.position(), block.length());
    }

    QCOMPARE(doc->size(), halfSize);

    for (int i = 1; i < 10; i += 2) {
        QTextBlock block = doc->findBlockByNumber(i);
        block.setVisible(false);
        doc->markContentsDirty(block.position(), block.length());
    }

    QCOMPARE(doc->size(), emptySize);

    for (int i = 0; i < 10; i += 2) {
        QTextBlock block = doc->findBlockByNumber(i);
        block.setVisible(true);
        doc->markContentsDirty(block.position(), block.length());
    }

    QCOMPARE(doc->size(), halfSize);
}

QTEST_MAIN(tst_QTextDocumentLayout)
#include "tst_qtextdocumentlayout.moc"
