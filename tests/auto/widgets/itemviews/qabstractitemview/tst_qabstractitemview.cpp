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
#include <QtTest/private/qtesthelpers_p.h>

#include <qabstractitemview.h>
#include <qstandarditemmodel.h>
#include <qapplication.h>
#include <qevent.h>
#include <qlistview.h>
#include <qlistwidget.h>
#include <qtableview.h>
#include <qtablewidget.h>
#include <qtreeview.h>
#include <qtreewidget.h>
#include <qheaderview.h>
#include <qspinbox.h>
#include <qitemdelegate.h>
#include <qpushbutton.h>
#include <qscrollbar.h>
#include <qboxlayout.h>
#include <qitemdelegate.h>
#include <qlineedit.h>
#include <qscreen.h>
#include <qscopedpointer.h>
#include <qstyleditemdelegate.h>
#include <qstringlistmodel.h>
#include <qsortfilterproxymodel.h>
#include <qproxystyle.h>
#include <qdialog.h>

Q_DECLARE_METATYPE(Qt::ItemFlags);

using namespace QTestPrivate;

// Move cursor out of widget area to avoid undesired interaction on Mac.
static inline void moveCursorAway(const QWidget *topLevel)
{
#ifndef QT_NO_CURSOR
    QCursor::setPos(topLevel->geometry().topRight() + QPoint(100, 0));
#else
    Q_UNUSED(topLevel)
#endif
}

class GeometriesTestView : public QTableView
{
    Q_OBJECT
public:
    GeometriesTestView() : QTableView(), updateGeometriesCalled(false) {}
    bool updateGeometriesCalled;
protected slots:
    void updateGeometries() override { updateGeometriesCalled = true; QTableView::updateGeometries(); }
};

class tst_QAbstractItemView : public QObject
{
    Q_OBJECT

public:
    void basic_tests(QAbstractItemView *view);

private slots:
    void cleanup();
    void getSetCheck();
    void emptyModels_data();
    void emptyModels();
    void setModel_data();
    void setModel();
    void noModel();
    void dragSelect();
    void rowDelegate();
    void columnDelegate();
    void selectAll();
    void ctrlA();
    void persistentEditorFocus();
    void setItemDelegate();
    void setItemDelegate_data();
    // The dragAndDrop() test doesn't work, and is thus disabled on Mac and Windows
    // for the following reasons:
    //   Mac: use of GetCurrentEventButtonState() in QDragManager::drag()
    //   Win: unknown reason
#if !defined(Q_OS_MAC) && !defined(Q_OS_WIN)
#if 0
    void dragAndDrop();
    void dragAndDropOnChild();
#endif
#endif
    void noFallbackToRoot();
    void setCurrentIndex_data();
    void setCurrentIndex();

    void task221955_selectedEditor();
    void task250754_fontChange();
    void task200665_itemEntered();
    void task257481_emptyEditor();
    void shiftArrowSelectionAfterScrolling();
    void shiftSelectionAfterRubberbandSelection();
    void ctrlRubberbandSelection();
    void QTBUG6407_extendedSelection();
    void QTBUG6753_selectOnSelection();
    void testDelegateDestroyEditor();
    void testClickedSignal();
    void testChangeEditorState();
    void deselectInSingleSelection();
    void testNoActivateOnDisabledItem();
    void testFocusPolicy_data();
    void testFocusPolicy();
    void QTBUG31411_noSelection();
    void QTBUG39324_settingSameInstanceOfIndexWidget();
    void sizeHintChangeTriggersLayout();
    void shiftSelectionAfterChangingModelContents();
    void QTBUG48968_reentrant_updateEditorGeometries();
    void QTBUG50102_SH_ItemView_ScrollMode();
    void QTBUG50535_update_on_new_selection_model();
    void testSelectionModelInSyncWithView();
    void testClickToSelect();
    void testDialogAsEditor();
    void QTBUG46785_mouseout_hover_state();
    void testClearModelInClickedSignal();
    void inputMethodEnabled_data();
    void inputMethodEnabled();
    void currentFollowsIndexWidget_data();
    void currentFollowsIndexWidget();
    void checkFocusAfterActivationChanges_data();
    void checkFocusAfterActivationChanges();
};

class MyAbstractItemDelegate : public QAbstractItemDelegate
{
public:
    MyAbstractItemDelegate() : QAbstractItemDelegate() { calledVirtualDtor = false; }
    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const {}
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const { return size; }
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
    {
        openedEditor = new QWidget(parent);
        return openedEditor;
    }
    void destroyEditor(QWidget *editor, const QModelIndex &) const
    {
        calledVirtualDtor = true;
        editor->deleteLater();
    }
    void changeSize() { size = QSize(50, 50); emit sizeHintChanged(QModelIndex()); }
    mutable bool calledVirtualDtor;
    mutable QWidget *openedEditor;
    QSize size;
};

class DialogItemDelegate : public QStyledItemDelegate
{
public:
    DialogItemDelegate() : QStyledItemDelegate() { }
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const
    {
        openedEditor = new QDialog(parent);
        return openedEditor;
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
    {
        Q_UNUSED(model)
        Q_UNUSED(index)

        QDialog *dialog = qobject_cast<QDialog *>(editor);
        result = static_cast<QDialog::DialogCode>(dialog->result());
    }

    mutable QDialog::DialogCode result;
    mutable QDialog *openedEditor;
};

// Testing get/set functions
void tst_QAbstractItemView::getSetCheck()
{
    QListView view;
    QAbstractItemView *obj1 = &view;
    // QAbstractItemDelegate * QAbstractItemView::itemDelegate()
    // void QAbstractItemView::setItemDelegate(QAbstractItemDelegate *)
    MyAbstractItemDelegate *var1 = new MyAbstractItemDelegate;
    obj1->setItemDelegate(var1);
    QCOMPARE((QAbstractItemDelegate*)var1, obj1->itemDelegate());
    obj1->setItemDelegate((QAbstractItemDelegate *)0);
    QCOMPARE((QAbstractItemDelegate *)0, obj1->itemDelegate());
    delete var1;

    // EditTriggers QAbstractItemView::editTriggers()
    // void QAbstractItemView::setEditTriggers(EditTriggers)
    obj1->setEditTriggers(QAbstractItemView::EditTriggers(QAbstractItemView::NoEditTriggers));
    QCOMPARE(QAbstractItemView::EditTriggers(QAbstractItemView::NoEditTriggers), obj1->editTriggers());
    obj1->setEditTriggers(QAbstractItemView::EditTriggers(QAbstractItemView::CurrentChanged));
    QCOMPARE(QAbstractItemView::EditTriggers(QAbstractItemView::CurrentChanged), obj1->editTriggers());
    obj1->setEditTriggers(QAbstractItemView::EditTriggers(QAbstractItemView::DoubleClicked));
    QCOMPARE(QAbstractItemView::EditTriggers(QAbstractItemView::DoubleClicked), obj1->editTriggers());
    obj1->setEditTriggers(QAbstractItemView::EditTriggers(QAbstractItemView::SelectedClicked));
    QCOMPARE(QAbstractItemView::EditTriggers(QAbstractItemView::SelectedClicked), obj1->editTriggers());
    obj1->setEditTriggers(QAbstractItemView::EditTriggers(QAbstractItemView::EditKeyPressed));
    QCOMPARE(QAbstractItemView::EditTriggers(QAbstractItemView::EditKeyPressed), obj1->editTriggers());
    obj1->setEditTriggers(QAbstractItemView::EditTriggers(QAbstractItemView::AnyKeyPressed));
    QCOMPARE(QAbstractItemView::EditTriggers(QAbstractItemView::AnyKeyPressed), obj1->editTriggers());
    obj1->setEditTriggers(QAbstractItemView::EditTriggers(QAbstractItemView::AllEditTriggers));
    QCOMPARE(QAbstractItemView::EditTriggers(QAbstractItemView::AllEditTriggers), obj1->editTriggers());

    // bool QAbstractItemView::tabKeyNavigation()
    // void QAbstractItemView::setTabKeyNavigation(bool)
    obj1->setTabKeyNavigation(false);
    QCOMPARE(false, obj1->tabKeyNavigation());
    obj1->setTabKeyNavigation(true);
    QCOMPARE(true, obj1->tabKeyNavigation());

    // bool QAbstractItemView::dragEnabled()
    // void QAbstractItemView::setDragEnabled(bool)
#if QT_CONFIG(draganddrop)
    obj1->setDragEnabled(false);
    QCOMPARE(false, obj1->dragEnabled());
    obj1->setDragEnabled(true);
    QCOMPARE(true, obj1->dragEnabled());
#endif
    // bool QAbstractItemView::alternatingRowColors()
    // void QAbstractItemView::setAlternatingRowColors(bool)
    obj1->setAlternatingRowColors(false);
    QCOMPARE(false, obj1->alternatingRowColors());
    obj1->setAlternatingRowColors(true);
    QCOMPARE(true, obj1->alternatingRowColors());

    // State QAbstractItemView::state()
    // void QAbstractItemView::setState(State)
    obj1->setState(QAbstractItemView::NoState);
    QCOMPARE(QAbstractItemView::NoState, obj1->state());
    obj1->setState(QAbstractItemView::DraggingState);
    QCOMPARE(QAbstractItemView::DraggingState, obj1->state());
    obj1->setState(QAbstractItemView::DragSelectingState);
    QCOMPARE(QAbstractItemView::DragSelectingState, obj1->state());
    obj1->setState(QAbstractItemView::EditingState);
    QCOMPARE(QAbstractItemView::EditingState, obj1->state());
    obj1->setState(QAbstractItemView::ExpandingState);
    QCOMPARE(QAbstractItemView::ExpandingState, obj1->state());
    obj1->setState(QAbstractItemView::CollapsingState);
    QCOMPARE(QAbstractItemView::CollapsingState, obj1->state());

    // QWidget QAbstractScrollArea::viewport()
    // void setViewport(QWidget*)
    QWidget *vp = new QWidget;
    obj1->setViewport(vp);
    QCOMPARE(vp, obj1->viewport());

    QCOMPARE(16, obj1->autoScrollMargin());
    obj1->setAutoScrollMargin(20);
    QCOMPARE(20, obj1->autoScrollMargin());
    obj1->setAutoScrollMargin(16);
    QCOMPARE(16, obj1->autoScrollMargin());
}

void tst_QAbstractItemView::cleanup()
{
    QVERIFY(QApplication::topLevelWidgets().isEmpty());
}

void tst_QAbstractItemView::emptyModels_data()
{
    QTest::addColumn<QString>("viewType");

    QTest::newRow("QListView") << "QListView";
    QTest::newRow("QTableView") << "QTableView";
    QTest::newRow("QTreeView") << "QTreeView";
    QTest::newRow("QHeaderView") << "QHeaderView";
}

void tst_QAbstractItemView::emptyModels()
{
    QFETCH(QString, viewType);

    QScopedPointer<QAbstractItemView> view;
    if (viewType == "QListView")
        view.reset(new QListView());
    else if (viewType == "QTableView")
        view.reset(new QTableView());
    else if (viewType == "QTreeView")
        view.reset(new QTreeView());
    else if (viewType == "QHeaderView")
        view.reset(new QHeaderView(Qt::Vertical));
    else
        QVERIFY(0);
    centerOnScreen(view.data());
    moveCursorAway(view.data());
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QVERIFY(!view->model());
    QVERIFY(!view->selectionModel());
    //QVERIFY(view->itemDelegate() != 0);

    basic_tests(view.data());
}

void tst_QAbstractItemView::setModel_data()
{
    QTest::addColumn<QString>("viewType");

    QTest::newRow("QListView") << "QListView";
    QTest::newRow("QTableView") << "QTableView";
    QTest::newRow("QTreeView") << "QTreeView";
    QTest::newRow("QHeaderView") << "QHeaderView";
}

void tst_QAbstractItemView::setModel()
{
    QFETCH(QString, viewType);

    QScopedPointer<QAbstractItemView> view;

    if (viewType == "QListView")
        view.reset(new QListView());
    else if (viewType == "QTableView")
        view.reset(new QTableView());
    else if (viewType == "QTreeView")
        view.reset(new QTreeView());
    else if (viewType == "QHeaderView")
        view.reset(new QHeaderView(Qt::Vertical));
    else
        QVERIFY(0);
    centerOnScreen(view.data());
    moveCursorAway(view.data());
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QStandardItemModel model(20,20);
    view->setModel(0);
    view->setModel(&model);
    basic_tests(view.data());
}

void tst_QAbstractItemView::basic_tests(QAbstractItemView *view)
{
    // setSelectionModel
    // Will assert as it should
    //view->setSelectionModel(0);
    // setItemDelegate
    //view->setItemDelegate(0);
    // Will asswert as it should

    // setSelectionMode
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    QCOMPARE(view->selectionMode(), QAbstractItemView::SingleSelection);
    view->setSelectionMode(QAbstractItemView::ContiguousSelection);
    QCOMPARE(view->selectionMode(), QAbstractItemView::ContiguousSelection);
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QCOMPARE(view->selectionMode(), QAbstractItemView::ExtendedSelection);
    view->setSelectionMode(QAbstractItemView::MultiSelection);
    QCOMPARE(view->selectionMode(), QAbstractItemView::MultiSelection);
    view->setSelectionMode(QAbstractItemView::NoSelection);
    QCOMPARE(view->selectionMode(), QAbstractItemView::NoSelection);

    // setSelectionBehavior
    view->setSelectionBehavior(QAbstractItemView::SelectItems);
    QCOMPARE(view->selectionBehavior(), QAbstractItemView::SelectItems);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    QCOMPARE(view->selectionBehavior(), QAbstractItemView::SelectRows);
    view->setSelectionBehavior(QAbstractItemView::SelectColumns);
    QCOMPARE(view->selectionBehavior(), QAbstractItemView::SelectColumns);

    // setEditTriggers
    view->setEditTriggers(QAbstractItemView::EditKeyPressed);
    QCOMPARE(view->editTriggers(), QAbstractItemView::EditKeyPressed);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QCOMPARE(view->editTriggers(), QAbstractItemView::NoEditTriggers);
    view->setEditTriggers(QAbstractItemView::CurrentChanged);
    QCOMPARE(view->editTriggers(), QAbstractItemView::CurrentChanged);
    view->setEditTriggers(QAbstractItemView::DoubleClicked);
    QCOMPARE(view->editTriggers(), QAbstractItemView::DoubleClicked);
    view->setEditTriggers(QAbstractItemView::SelectedClicked);
    QCOMPARE(view->editTriggers(), QAbstractItemView::SelectedClicked);
    view->setEditTriggers(QAbstractItemView::AnyKeyPressed);
    QCOMPARE(view->editTriggers(), QAbstractItemView::AnyKeyPressed);
    view->setEditTriggers(QAbstractItemView::AllEditTriggers);
    QCOMPARE(view->editTriggers(), QAbstractItemView::AllEditTriggers);

    // setAutoScroll
    view->setAutoScroll(false);
    QCOMPARE(view->hasAutoScroll(), false);
    view->setAutoScroll(true);
    QCOMPARE(view->hasAutoScroll(), true);

    // setTabKeyNavigation
    view->setTabKeyNavigation(false);
    QCOMPARE(view->tabKeyNavigation(), false);
    view->setTabKeyNavigation(true);
    QCOMPARE(view->tabKeyNavigation(), true);

#if QT_CONFIG(draganddrop)
    // setDropIndicatorShown
    view->setDropIndicatorShown(false);
    QCOMPARE(view->showDropIndicator(), false);
    view->setDropIndicatorShown(true);
    QCOMPARE(view->showDropIndicator(), true);

    // setDragEnabled
    view->setDragEnabled(false);
    QCOMPARE(view->dragEnabled(), false);
    view->setDragEnabled(true);
    QCOMPARE(view->dragEnabled(), true);
#endif

    // setAlternatingRowColors
    view->setAlternatingRowColors(false);
    QCOMPARE(view->alternatingRowColors(), false);
    view->setAlternatingRowColors(true);
    QCOMPARE(view->alternatingRowColors(), true);

    // setIconSize
    view->setIconSize(QSize(16, 16));
    QCOMPARE(view->iconSize(), QSize(16, 16));
    QSignalSpy spy(view, &QAbstractItemView::iconSizeChanged);
    QVERIFY(spy.isValid());
    view->setIconSize(QSize(32, 32));
    QCOMPARE(view->iconSize(), QSize(32, 32));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<QSize>(), QSize(32, 32));
    // Should this happen?
    view->setIconSize(QSize(-1, -1));
    QCOMPARE(view->iconSize(), QSize(-1, -1));
    QCOMPARE(spy.count(), 2);

    QCOMPARE(view->currentIndex(), QModelIndex());
    QCOMPARE(view->rootIndex(), QModelIndex());

    view->keyboardSearch("");
    view->keyboardSearch("foo");
    view->keyboardSearch("1");

    QCOMPARE(view->visualRect(QModelIndex()), QRect());

    view->scrollTo(QModelIndex());

    QCOMPARE(view->sizeHintForIndex(QModelIndex()), QSize());
    QCOMPARE(view->indexAt(QPoint(-1, -1)), QModelIndex());

    if (!view->model()){
        QCOMPARE(view->indexAt(QPoint(10, 10)), QModelIndex());
        QCOMPARE(view->sizeHintForRow(0), -1);
        QCOMPARE(view->sizeHintForColumn(0), -1);
    }else if (view->itemDelegate()){
        view->sizeHintForRow(0);
        view->sizeHintForColumn(0);
    }
    view->openPersistentEditor(QModelIndex());
    view->closePersistentEditor(QModelIndex());

    view->reset();
    view->setRootIndex(QModelIndex());
    view->doItemsLayout();
    view->selectAll();
    view->edit(QModelIndex());
    view->clearSelection();
    view->setCurrentIndex(QModelIndex());

    // protected methods
    view->dataChanged(QModelIndex(), QModelIndex());
    view->rowsInserted(QModelIndex(), -1, -1);
    view->rowsAboutToBeRemoved(QModelIndex(), -1, -1);
    view->selectionChanged(QItemSelection(), QItemSelection());
    if (view->model()){
        view->currentChanged(QModelIndex(), QModelIndex());
        view->currentChanged(QModelIndex(), view->model()->index(0,0));
    }
    view->updateEditorData();
    view->updateEditorGeometries();
    view->updateGeometries();
    view->verticalScrollbarAction(QAbstractSlider::SliderSingleStepAdd);
    view->horizontalScrollbarAction(QAbstractSlider::SliderSingleStepAdd);
    view->verticalScrollbarValueChanged(10);
    view->horizontalScrollbarValueChanged(10);
    view->closeEditor(0, QAbstractItemDelegate::NoHint);
    view->commitData(0);
    view->editorDestroyed(0);

    view->setHorizontalStepsPerItem(2);
    view->horizontalStepsPerItem();
    view->setVerticalStepsPerItem(2);
    view->verticalStepsPerItem();

    // Will assert as it should
    // view->setIndexWidget(QModelIndex(), 0);

    view->moveCursor(QAbstractItemView::MoveUp, Qt::NoModifier);
    view->horizontalOffset();
    view->verticalOffset();

//    view->isIndexHidden(QModelIndex()); // will (correctly) assert
    if(view->model())
        view->isIndexHidden(view->model()->index(0,0));

    view->setSelection(QRect(0, 0, 10, 10), QItemSelectionModel::ClearAndSelect);
    view->setSelection(QRect(-1, -1, -1, -1), QItemSelectionModel::ClearAndSelect);
    view->visualRegionForSelection(QItemSelection());
    view->selectedIndexes();

    view->edit(QModelIndex(), QAbstractItemView::NoEditTriggers, 0);

    view->selectionCommand(QModelIndex(), 0);

#if QT_CONFIG(draganddrop)
    if (!view->model())
        view->startDrag(Qt::CopyAction);

    view->viewOptions();

    view->setState(QAbstractItemView::NoState);
    QVERIFY(view->state()==QAbstractItemView::NoState);
    view->setState(QAbstractItemView::DraggingState);
    QVERIFY(view->state()==QAbstractItemView::DraggingState);
    view->setState(QAbstractItemView::DragSelectingState);
    QVERIFY(view->state()==QAbstractItemView::DragSelectingState);
    view->setState(QAbstractItemView::EditingState);
    QVERIFY(view->state()==QAbstractItemView::EditingState);
    view->setState(QAbstractItemView::ExpandingState);
    QVERIFY(view->state()==QAbstractItemView::ExpandingState);
    view->setState(QAbstractItemView::CollapsingState);
    QVERIFY(view->state()==QAbstractItemView::CollapsingState);
#endif

    view->startAutoScroll();
    view->stopAutoScroll();
    view->doAutoScroll();

    // testing mouseFoo and key functions
//     QTest::mousePress(view, Qt::LeftButton, Qt::NoModifier, QPoint(0,0));
//     mouseMove(view, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
//     QTest::mouseRelease(view, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
//     QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
//     mouseDClick(view, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
//     QTest::keyClick(view, Qt::Key_A);
}

void tst_QAbstractItemView::noModel()
{
    // From task #85415

    QStandardItemModel model(20,20);
    QTreeView view;
    setFrameless(&view);

    view.setModel(&model);
    // Make the viewport smaller than the contents, so that we can scroll
    view.resize(100,100);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // make sure that the scrollbars are not at value 0
    view.scrollTo(view.model()->index(10,10));
    QApplication::processEvents();

    view.setModel(0);
    // Due to the model is removed, this will generate a valueChanged signal on both scrollbars. (value to 0)
    QApplication::processEvents();
    QCOMPARE(view.model(), nullptr);
}

void tst_QAbstractItemView::dragSelect()
{
    // From task #86108

    QStandardItemModel model(64,64);

    QTableView view;
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.setVisible(true);
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const int delay = 2;
    for (int i = 0; i < 2; ++i) {
        bool tracking = (i == 1);
        view.setMouseTracking(false);
        QTest::mouseMove(&view, QPoint(0, 0), delay);
        view.setMouseTracking(tracking);
        QTest::mouseMove(&view, QPoint(50, 50), delay);
        QVERIFY(view.selectionModel()->selectedIndexes().isEmpty());
    }
}

void tst_QAbstractItemView::rowDelegate()
{
    QStandardItemModel model(4,4);
    MyAbstractItemDelegate delegate;

    QTableView view;
    view.setModel(&model);
    view.setItemDelegateForRow(3, &delegate);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index = model.index(3, 0);
    QVERIFY(!view.isPersistentEditorOpen(index));
    view.openPersistentEditor(index);
    QVERIFY(view.isPersistentEditorOpen(index));
    QWidget *w = view.indexWidget(index);
    QVERIFY(w);
    QCOMPARE(w->metaObject()->className(), "QWidget");
}

void tst_QAbstractItemView::columnDelegate()
{
    QStandardItemModel model(4,4);
    MyAbstractItemDelegate delegate;

    QTableView view;
    view.setModel(&model);
    view.setItemDelegateForColumn(3, &delegate);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index = model.index(0, 3);
    QVERIFY(!view.isPersistentEditorOpen(index));
    view.openPersistentEditor(index);
    QVERIFY(view.isPersistentEditorOpen(index));
    QWidget *w = view.indexWidget(index);
    QVERIFY(w);
    QCOMPARE(w->metaObject()->className(), "QWidget");
}

void tst_QAbstractItemView::sizeHintChangeTriggersLayout()
{
    QStandardItemModel model(4, 4);
    MyAbstractItemDelegate delegate;
    MyAbstractItemDelegate rowDelegate;
    MyAbstractItemDelegate columnDelegate;

    GeometriesTestView view;
    view.setModel(&model);
    view.setItemDelegate(&delegate);
    view.setItemDelegateForRow(1, &rowDelegate);
    view.setItemDelegateForColumn(2, &columnDelegate);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.updateGeometriesCalled = false;
    delegate.changeSize();
    QCoreApplication::sendPostedEvents();
    QVERIFY(view.updateGeometriesCalled);
    view.updateGeometriesCalled = false;
    rowDelegate.changeSize();
    QCoreApplication::sendPostedEvents();
    QVERIFY(view.updateGeometriesCalled);
    view.updateGeometriesCalled = false;
    columnDelegate.changeSize();
    QCoreApplication::sendPostedEvents();
    QVERIFY(view.updateGeometriesCalled);
}

void tst_QAbstractItemView::selectAll()
{
    QStandardItemModel model(4,4);
    QTableView view;
    view.setModel(&model);

    QAbstractItemView *tst_view = &view;

    QCOMPARE(tst_view->selectedIndexes().count(), 0);
    view.selectAll();
    QCOMPARE(tst_view->selectedIndexes().count(), 4*4);
}

void tst_QAbstractItemView::ctrlA()
{
    QStandardItemModel model(4,4);
    QTableView view;
    view.setModel(&model);

    QAbstractItemView *tst_view = &view;

    QCOMPARE(tst_view->selectedIndexes().count(), 0);
    QTest::keyClick(&view, Qt::Key_A, Qt::ControlModifier);
    QCOMPARE(tst_view->selectedIndexes().count(), 4*4);
}

void tst_QAbstractItemView::persistentEditorFocus()
{
    // one row, three columns
    QStandardItemModel model(1, 3);
    for(int i = 0; i < model.columnCount(); ++i)
        model.setData(model.index(0, i), i);
    QTableView view;
    view.setModel(&model);

    view.openPersistentEditor(model.index(0, 1));
    view.openPersistentEditor(model.index(0, 2));

    //these are spinboxes because we put numbers inside
    QList<QSpinBox*> list = view.viewport()->findChildren<QSpinBox*>();
    QCOMPARE(list.count(), 2); //these should be the 2 editors

    view.setCurrentIndex(model.index(0, 0));
    QCOMPARE(view.currentIndex(), model.index(0, 0));
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    for (int i = 0; i < list.count(); ++i) {
        QTRY_VERIFY(list.at(i)->isVisible());
        QPoint p = QPoint(5, 5);
        QMouseEvent mouseEvent(QEvent::MouseButtonPress, p, Qt::LeftButton,
                               Qt::LeftButton, Qt::NoModifier);
        qApp->sendEvent(list.at(i), &mouseEvent);
        if (!qApp->focusWidget())
            QSKIP("Some window managers don't handle focus that well");
        QTRY_COMPARE(qApp->focusWidget(), static_cast<QWidget *>(list.at(i)));
    }
}


#if !defined(Q_OS_MAC) && !defined(Q_OS_WIN)

#if 0

static void sendMouseMove(QWidget *widget, QPoint pos = QPoint())
{
    if (pos.isNull())
        pos = widget->rect().center();
    QMouseEvent event(QEvent::MouseMove, pos, widget->mapToGlobal(pos), Qt::NoButton, 0, 0);
    QCursor::setPos(widget->mapToGlobal(pos));
    qApp->processEvents();
    QVERIFY(QTest::qWaitForWindowExposed(widget));
    QApplication::sendEvent(widget, &event);
}

static void sendMousePress(
    QWidget *widget, QPoint pos = QPoint(), Qt::MouseButton button = Qt::LeftButton)
{
    if (pos.isNull())
         pos = widget->rect().center();
    QMouseEvent event(QEvent::MouseButtonPress, pos, widget->mapToGlobal(pos), button, 0, 0);
    QApplication::sendEvent(widget, &event);
}

static void sendMouseRelease(
    QWidget *widget, QPoint pos = QPoint(), Qt::MouseButton button = Qt::LeftButton)
{
    if (pos.isNull())
         pos = widget->rect().center();
    QMouseEvent event(QEvent::MouseButtonRelease, pos, widget->mapToGlobal(pos), button, 0, 0);
    QApplication::sendEvent(widget, &event);
}

class DnDTestModel : public QStandardItemModel
{
    Q_OBJECT
    bool dropMimeData(const QMimeData *md, Qt::DropAction action, int r, int c, const QModelIndex &p)
    {
        dropAction_result = action;
        QStandardItemModel::dropMimeData(md, action, r, c, p);
        return true;
    }
    Qt::DropActions supportedDropActions() const { return Qt::CopyAction | Qt::MoveAction; }

    Qt::DropAction dropAction_result;
public:
    DnDTestModel() : QStandardItemModel(20, 20), dropAction_result(Qt::IgnoreAction) {
        for (int i = 0; i < rowCount(); ++i)
            setData(index(i, 0), QString::number(i));
    }
    Qt::DropAction dropAction() const { return dropAction_result; }
};

class DnDTestView : public QTreeView
{
    Q_OBJECT

    QPoint dropPoint;
    Qt::DropAction dropAction;

    void dragEnterEvent(QDragEnterEvent *event)
    {
        QAbstractItemView::dragEnterEvent(event);
    }

    void dropEvent(QDropEvent *event)
    {
        event->setDropAction(dropAction);
        QTreeView::dropEvent(event);
    }

    void timerEvent(QTimerEvent *event)
    {
        killTimer(event->timerId());
        sendMouseMove(this, dropPoint);
        sendMouseRelease(this);
    }

    void mousePressEvent(QMouseEvent *e)
    {
        QTreeView::mousePressEvent(e);

        startTimer(0);
        setState(DraggingState);
        startDrag(dropAction);
    }

public:
    DnDTestView(Qt::DropAction dropAction, QAbstractItemModel *model)
        : dropAction(dropAction)
    {
        header()->hide();
        setModel(model);
        setDragDropMode(QAbstractItemView::DragDrop);
        setAcceptDrops(true);
        setDragEnabled(true);
    }

    void dragAndDrop(QPoint drag, QPoint drop)
    {
        dropPoint = drop;
        setCurrentIndex(indexAt(drag));
        sendMousePress(viewport(), drag);
    }
};

class DnDTestWidget : public QWidget
{
    Q_OBJECT

    Qt::DropAction dropAction_request;
    Qt::DropAction dropAction_result;
    QWidget *dropTarget;

    void timerEvent(QTimerEvent *event)
    {
        killTimer(event->timerId());
        sendMouseMove(dropTarget);
        sendMouseRelease(dropTarget);
    }

    void mousePressEvent(QMouseEvent *)
    {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setData("application/x-qabstractitemmodeldatalist", QByteArray(""));
        drag->setMimeData(mimeData);
        startTimer(0);
        dropAction_result = drag->start(dropAction_request);
    }

public:
    Qt::DropAction dropAction() const { return dropAction_result; }

    void dragAndDrop(QWidget *dropTarget, Qt::DropAction dropAction)
    {
        this->dropTarget = dropTarget;
        dropAction_request = dropAction;
        sendMousePress(this);
    }
};

void tst_QAbstractItemView::dragAndDrop()
{
    // From Task 137729


    const int attempts = 10;
    int successes = 0;
    for (int i = 0; i < attempts; ++i) {
        Qt::DropAction dropAction = Qt::MoveAction;

        DnDTestModel model;
        DnDTestView view(dropAction, &model);
        DnDTestWidget widget;

        const int size = 200;
        widget.setFixedSize(size, size);
        view.setFixedSize(size, size);

        widget.move(0, 0);
        view.move(int(size * 1.5), int(size * 1.5));

        widget.show();
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&widget));
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        widget.dragAndDrop(&view, dropAction);
        if (model.dropAction() == dropAction
            && widget.dropAction() == dropAction)
            ++successes;
    }

    if (successes < attempts) {
        QString msg = QString("# successes (%1) < # attempts (%2)").arg(successes).arg(attempts);
        QWARN(msg.toLatin1());
    }
    QVERIFY(successes > 0); // allow for some "event unstability" (i.e. unless
                            // successes == 0, QAbstractItemView is probably ok!)
}

void tst_QAbstractItemView::dragAndDropOnChild()
{

    const int attempts = 10;
    int successes = 0;
    for (int i = 0; i < attempts; ++i) {
        Qt::DropAction dropAction = Qt::MoveAction;

        DnDTestModel model;
        QModelIndex parent = model.index(0, 0);
        model.insertRow(0, parent);
        model.insertColumn(0, parent);
        QModelIndex child = model.index(0, 0, parent);
        model.setData(child, "child");
        QCOMPARE(model.rowCount(parent), 1);
        DnDTestView view(dropAction, &model);
        view.setExpanded(parent, true);
        view.setDragDropMode(QAbstractItemView::InternalMove);

        const int size = 200;
        view.setFixedSize(size, size);
        view.move(int(size * 1.5), int(size * 1.5));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        view.dragAndDrop(view.visualRect(parent).center(),
                         view.visualRect(child).center());
        if (model.dropAction() == dropAction)
            ++successes;
    }

    QCOMPARE(successes, 0);
}

#endif // 0
#endif // !Q_OS_MAC && !Q_OS_WIN

class TestModel : public QStandardItemModel
{
public:
    TestModel(int rows, int columns) : QStandardItemModel(rows, columns)
    {
        setData_count = 0;
    }

    virtual bool setData(const QModelIndex &/*index*/, const QVariant &/*value*/, int /*role = Qt::EditRole*/)
    {
        ++setData_count;
        return true;
    }

    int setData_count;
};

typedef QList<int> IntList;

void tst_QAbstractItemView::setItemDelegate_data()
{
    // default is rows, a -1 will switch to columns
    QTest::addColumn<IntList>("rowsOrColumnsWithDelegate");
    QTest::addColumn<QPoint>("cellToEdit");
    QTest::newRow("4 columndelegates")
                << (IntList() << -1 << 0 << 1 << 2 << 3)
                << QPoint(0, 0);
    QTest::newRow("2 identical rowdelegates on the same row")
                << (IntList() << 0 << 0)
                << QPoint(0, 0);
    QTest::newRow("2 identical columndelegates on the same column")
                << (IntList() << -1 << 2 << 2)
                << QPoint(2, 0);
    QTest::newRow("2 duplicate delegates, 1 row and 1 column")
                << (IntList() << 0 << -1 << 2)
                << QPoint(2, 0);
    QTest::newRow("4 duplicate delegates, 2 row and 2 column")
                << (IntList() << 0 << 0 << -1 << 2 << 2)
                << QPoint(2, 0);

}

void tst_QAbstractItemView::setItemDelegate()
{
    QFETCH(IntList, rowsOrColumnsWithDelegate);
    QFETCH(QPoint, cellToEdit);
    QTableView v;
    QItemDelegate *delegate = new QItemDelegate(&v);
    TestModel model(5, 5);
    v.setModel(&model);

    bool row = true;
    foreach (int rc, rowsOrColumnsWithDelegate) {
        if (rc == -1) {
            row = !row;
        } else {
            if (row) {
                v.setItemDelegateForRow(rc, delegate);
            } else {
                v.setItemDelegateForColumn(rc, delegate);
            }
        }
    }
    centerOnScreen(&v);
    moveCursorAway(&v);
    v.show();
#if 0 // Used to be included in Qt4 for Q_WS_X11
    QCursor::setPos(v.geometry().center());
#endif
    QApplication::setActiveWindow(&v);
    QVERIFY(QTest::qWaitForWindowActive(&v));

    QModelIndex index = model.index(cellToEdit.y(), cellToEdit.x());
    v.edit(index);

    // This will close the editor
    QTRY_VERIFY(QApplication::focusWidget());
    QWidget *editor = QApplication::focusWidget();
    QVERIFY(editor);
    editor->hide();
    delete editor;
    QCOMPARE(model.setData_count, 1);
    delete delegate;
}

void tst_QAbstractItemView::noFallbackToRoot()
{
    QStandardItemModel model(0, 1);
    for (int i = 0; i < 5; ++i)
        model.appendRow(new QStandardItem("top" + QString::number(i)));
    QStandardItem *par1 = model.item(1);
    for (int j = 0; j < 15; ++j)
        par1->appendRow(new QStandardItem("sub" + QString::number(j)));
    QStandardItem *par2 = par1->child(2);
    for (int k = 0; k < 10; ++k)
        par2->appendRow(new QStandardItem("bot" + QString::number(k)));
    QStandardItem *it1 = par2->child(5);

    QModelIndex parent1 = model.indexFromItem(par1);
    QModelIndex parent2 = model.indexFromItem(par2);
    QModelIndex item1 = model.indexFromItem(it1);

    QTreeView v;
    v.setModel(&model);
    v.setRootIndex(parent1);
    v.setCurrentIndex(item1);
    QCOMPARE(v.currentIndex(), item1);
    QVERIFY(model.removeRows(0, 10, parent2));
    QCOMPARE(v.currentIndex(), parent2);
    QVERIFY(model.removeRows(0, 15, parent1));
    QCOMPARE(v.currentIndex(), QModelIndex());
}

void tst_QAbstractItemView::setCurrentIndex_data()
{
    QTest::addColumn<QString>("viewType");
    QTest::addColumn<int>("itemFlags");
    QTest::addColumn<bool>("result");

    QStringList widgets;
    widgets << "QListView" << "QTreeView" << "QHeaderView" << "QTableView";

    foreach(QString widget, widgets) {
        QTest::newRow((widget+QLatin1String(": no flags")).toLocal8Bit().constData())
            << widget << (int)0 << false;
        QTest::newRow((widget+QLatin1String(": checkable")).toLocal8Bit().constData())
            << widget << (int)Qt::ItemIsUserCheckable << false;
        QTest::newRow((widget+QLatin1String(": selectable")).toLocal8Bit().constData())
            << widget << (int)Qt::ItemIsSelectable << false;
        QTest::newRow((widget+QLatin1String(": enabled")).toLocal8Bit().constData())
            << widget << (int)Qt::ItemIsEnabled << true;
        QTest::newRow((widget+QLatin1String(": enabled|selectable")).toLocal8Bit().constData())
            << widget << (int)(Qt::ItemIsSelectable|Qt::ItemIsEnabled) << true;
    }
}

void tst_QAbstractItemView::setCurrentIndex()
{
    QFETCH(QString, viewType);
    QFETCH(int, itemFlags);
    QFETCH(bool, result);

    QScopedPointer<QAbstractItemView> view;

    if (viewType == "QListView")
        view.reset(new QListView());
    else if (viewType == "QTableView")
        view.reset(new QTableView());
    else if (viewType == "QTreeView")
        view.reset(new QTreeView());
    else if (viewType == "QHeaderView")
        view.reset(new QHeaderView(Qt::Vertical));
    else
        QVERIFY(0);

    centerOnScreen(view.data());
    moveCursorAway(view.data());
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QStandardItemModel *model = new QStandardItemModel(view.data());
    QStandardItem *item = new QStandardItem("first item");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    model->appendRow(item);

    item = new QStandardItem("test item");
    item->setFlags(Qt::ItemFlags(itemFlags));
    model->appendRow(item);

    view->setModel(model);

    view->setCurrentIndex(model->index(0,0));
    QCOMPARE(view->currentIndex(), model->index(0,0));
    view->setCurrentIndex(model->index(1,0));
    QVERIFY(view->currentIndex() == model->index(result ? 1 : 0,0));
}

void tst_QAbstractItemView::task221955_selectedEditor()
{
    QPushButton *button;

    QTreeWidget tree;
    tree.setColumnCount(2);

    tree.addTopLevelItem(new QTreeWidgetItem(QStringList() << "Foo" <<"1"));
    tree.addTopLevelItem(new QTreeWidgetItem(QStringList() << "Bar" <<"2"));
    tree.addTopLevelItem(new QTreeWidgetItem(QStringList() << "Baz" <<"3"));

    QTreeWidgetItem *dummy = new QTreeWidgetItem();
    tree.addTopLevelItem(dummy);
    tree.setItemWidget(dummy, 0, button = new QPushButton("More..."));
    button->setAutoFillBackground(true);  // as recommended in doc

    centerOnScreen(&tree);
    moveCursorAway(&tree);
    tree.show();
    tree.setFocus();
    tree.setCurrentIndex(tree.model()->index(1,0));
    QApplication::setActiveWindow(&tree);
    QVERIFY(QTest::qWaitForWindowActive(&tree));

    QVERIFY(! tree.selectionModel()->selectedIndexes().contains(tree.model()->index(3,0)));

    //We set the focus to the button, the index need to be selected
    button->setFocus();
    QTRY_VERIFY(tree.selectionModel()->selectedIndexes().contains(tree.model()->index(3,0)));

    tree.setCurrentIndex(tree.model()->index(1,0));
    QVERIFY(! tree.selectionModel()->selectedIndexes().contains(tree.model()->index(3,0)));

    //Same thing but with the flag NoSelection,   nothing can be selected.
    tree.setFocus();
    tree.setSelectionMode(QAbstractItemView::NoSelection);
    tree.clearSelection();
    QVERIFY(tree.selectionModel()->selectedIndexes().isEmpty());
    button->setFocus();
    QTest::qWait(50);
    QVERIFY(tree.selectionModel()->selectedIndexes().isEmpty());
}

void tst_QAbstractItemView::task250754_fontChange()
{
    QString app_css = qApp->styleSheet();
    qApp->setStyleSheet("/*  */");

    QWidget w;
    QTreeView tree(&w);
    QVBoxLayout *vLayout = new QVBoxLayout(&w);
    vLayout->addWidget(&tree);

    QStandardItemModel *m = new QStandardItemModel(this);
    for (int i=0; i<20; ++i) {
        QStandardItem *item = new QStandardItem(QStringLiteral("Item number ") + QString::number(i));
        for (int j=0; j<5; ++j) {
            QStandardItem *child = new QStandardItem(QStringLiteral("Child Item number ") + QString::number(j));
            item->setChild(j, 0, child);
        }
        m->setItem(i, 0, item);
    }
    tree.setModel(m);

    tree.setHeaderHidden(true); // The header is (in certain styles) dpi dependent
    w.resize(160, 350); // Minimum width for windows with frame on Windows 8
    centerOnScreen(&w);
    moveCursorAway(&w);
    w.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QFont font = tree.font();
    font.setPixelSize(10);
    tree.setFont(font);
    QTRY_VERIFY(!tree.verticalScrollBar()->isVisible());

    font.setPixelSize(60);
    tree.setFont(font);
#ifdef Q_OS_WINRT
    QSKIP("Resizing the widget does not work as expected for WinRT, so the scroll bar might not be visible");
#endif
    //now with the huge items, the scrollbar must be visible
    QTRY_VERIFY(tree.verticalScrollBar()->isVisible());

    qApp->setStyleSheet(app_css);
}

void tst_QAbstractItemView::task200665_itemEntered()
{
    //we test that view will emit entered
    //when the scrollbar move but not the mouse itself
    QStandardItemModel model(1000,1);
    QListView view;
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QCursor::setPos(view.geometry().center());
    QTRY_COMPARE(QCursor::pos(), view.geometry().center());
    QSignalSpy spy(&view, SIGNAL(entered(QModelIndex)));
    view.verticalScrollBar()->setValue(view.verticalScrollBar()->maximum());

    QTRY_COMPARE(spy.count(), 1);
}

void tst_QAbstractItemView::task257481_emptyEditor()
{
    QIcon icon = qApp->style()->standardIcon(QStyle::SP_ComputerIcon);

    QStandardItemModel model;

    model.appendRow( new QStandardItem(icon, QString()) );
    model.appendRow( new QStandardItem(icon, "Editor works") );
    model.appendRow( new QStandardItem( QString() ) );

    QTreeView treeView;
    treeView.setRootIsDecorated(false);
    treeView.setModel(&model);
    centerOnScreen(&treeView);
    moveCursorAway(&treeView);
    treeView.show();
    QVERIFY(QTest::qWaitForWindowExposed(&treeView));

    treeView.edit(model.index(0,0));
    QList<QLineEdit *> lineEditors = treeView.viewport()->findChildren<QLineEdit *>();
    QCOMPARE(lineEditors.count(), 1);
    QVERIFY(!lineEditors.first()->size().isEmpty());

    treeView.edit(model.index(1,0));
    lineEditors = treeView.viewport()->findChildren<QLineEdit *>();
    QCOMPARE(lineEditors.count(), 1);
    QVERIFY(!lineEditors.first()->size().isEmpty());

    treeView.edit(model.index(2,0));
    lineEditors = treeView.viewport()->findChildren<QLineEdit *>();
    QCOMPARE(lineEditors.count(), 1);
    QVERIFY(!lineEditors.first()->size().isEmpty());
}

void tst_QAbstractItemView::shiftArrowSelectionAfterScrolling()
{
    QStandardItemModel model;
    for (int i=0; i<10; ++i) {
        QStandardItem *item = new QStandardItem(QString::number(i));
        model.setItem(i, 0, item);
    }

    QListView view;
    view.setFixedSize(160, 250); // Minimum width for windows with frame on Windows 8
    view.setFlow(QListView::LeftToRight);
    view.setGridSize(QSize(100, 100));
    view.setSelectionMode(QListView::ExtendedSelection);
    view.setViewMode(QListView::IconMode);
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index0 = model.index(0, 0);
    QModelIndex index1 = model.index(1, 0);
    QModelIndex index9 = model.index(9, 0);

    view.selectionModel()->setCurrentIndex(index0, QItemSelectionModel::NoUpdate);
    QCOMPARE(view.currentIndex(), index0);

    view.scrollTo(index9);
    QTest::keyClick(&view, Qt::Key_Down, Qt::ShiftModifier);

    QCOMPARE(view.currentIndex(), index1);
    QModelIndexList selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 2);
    QVERIFY(selected.contains(index0));
    QVERIFY(selected.contains(index1));
}

void tst_QAbstractItemView::shiftSelectionAfterRubberbandSelection()
{
    QStandardItemModel model;
    for (int i=0; i<3; ++i) {
        QStandardItem *item = new QStandardItem(QString::number(i));
        model.setItem(i, 0, item);
    }

    QListView view;
    view.setFixedSize(160, 450); // Minimum width for windows with frame on Windows 8
    view.setFlow(QListView::LeftToRight);
    view.setGridSize(QSize(100, 100));
    view.setSelectionMode(QListView::ExtendedSelection);
    view.setViewMode(QListView::IconMode);
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index0 = model.index(0, 0);
    QModelIndex index1 = model.index(1, 0);
    QModelIndex index2 = model.index(2, 0);

    view.setCurrentIndex(index0);
    QCOMPARE(view.currentIndex(), index0);

    // Determine the points where the rubberband selection starts and ends
    QPoint pressPos = view.visualRect(index1).bottomRight() + QPoint(1, 1);
    QPoint releasePos = view.visualRect(index1).center();
    QVERIFY(!view.indexAt(pressPos).isValid());
    QCOMPARE(view.indexAt(releasePos), index1);

    // Select item 1 using a rubberband selection
    // The mouse move event has to be created manually because the QTest framework does not
    // contain a function for mouse moves with buttons pressed
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, pressPos);
    QMouseEvent moveEvent(QEvent::MouseMove, releasePos, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    bool moveEventReceived = qApp->notify(view.viewport(), &moveEvent);
    QVERIFY(moveEventReceived);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, releasePos);
    QCOMPARE(view.currentIndex(), index1);

    // Shift-click item 2
    QPoint item2Pos = view.visualRect(index2).center();
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, item2Pos);
    QCOMPARE(view.currentIndex(), index2);

    // Verify that the selection worked OK
    QModelIndexList selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 2);
    QVERIFY(selected.contains(index1));
    QVERIFY(selected.contains(index2));

    // Select item 0 to revert the selection
    view.setCurrentIndex(index0);
    QCOMPARE(view.currentIndex(), index0);

    // Repeat the same steps as above, but with a Shift-Arrow selection
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::NoModifier, pressPos);
    QMouseEvent moveEvent2(QEvent::MouseMove, releasePos, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    moveEventReceived = qApp->notify(view.viewport(), &moveEvent2);
    QVERIFY(moveEventReceived);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::NoModifier, releasePos);
    QCOMPARE(view.currentIndex(), index1);

    // Press Shift-Down
    QTest::keyClick(&view, Qt::Key_Down, Qt::ShiftModifier);
    QCOMPARE(view.currentIndex(), index2);

    // Verify that the selection worked OK
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 2);
    QVERIFY(selected.contains(index1));
    QVERIFY(selected.contains(index2));
}

void tst_QAbstractItemView::ctrlRubberbandSelection()
{
    QStandardItemModel model;
    for (int i=0; i<3; ++i) {
        QStandardItem *item = new QStandardItem(QString::number(i));
        model.setItem(i, 0, item);
    }

    QListView view;
    view.setFixedSize(160, 450); // Minimum width for windows with frame on Windows 8
    view.setFlow(QListView::LeftToRight);
    view.setGridSize(QSize(100, 100));
    view.setSelectionMode(QListView::ExtendedSelection);
    view.setViewMode(QListView::IconMode);
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QModelIndex index1 = model.index(1, 0);
    QModelIndex index2 = model.index(2, 0);

    // Select item 1
    view.setCurrentIndex(index1);
    QModelIndexList selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 1);
    QVERIFY(selected.contains(index1));

    // Now press control and draw a rubberband around items 1 and 2.
    // The mouse move event has to be created manually because the QTest framework does not
    // contain a function for mouse moves with buttons pressed.
    QPoint pressPos = view.visualRect(index1).topLeft() - QPoint(1, 1);
    QPoint releasePos = view.visualRect(index2).bottomRight() + QPoint(1, 1);
    QTest::mousePress(view.viewport(), Qt::LeftButton, Qt::ControlModifier, pressPos);
    QMouseEvent moveEvent(QEvent::MouseMove, releasePos, Qt::NoButton, Qt::LeftButton, Qt::ControlModifier);
    bool moveEventReceived = qApp->notify(view.viewport(), &moveEvent);
    QVERIFY(moveEventReceived);
    QTest::mouseRelease(view.viewport(), Qt::LeftButton, Qt::ControlModifier, releasePos);

    // Verify that item 2 is selected now
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 1);
    QVERIFY(selected.contains(index2));
}

void tst_QAbstractItemView::QTBUG6407_extendedSelection()
{
    QListWidget view;
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    for(int i = 0; i < 50; ++i)
        view.addItem(QString::number(i));

    QFont font = view.font();
    font.setPixelSize(10);
    view.setFont(font);
    view.resize(200,240);
    centerOnScreen(&view);
    moveCursorAway(&view);

    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(static_cast<QWidget *>(&view), QApplication::activeWindow());

    view.verticalScrollBar()->setValue(view.verticalScrollBar()->maximum());

    QModelIndex index49 = view.model()->index(49,0);
    QPoint p = view.visualRect(index49).center();
    QVERIFY(view.viewport()->rect().contains(p));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, p);
    QCOMPARE(view.currentIndex(), index49);
    QCOMPARE(view.selectedItems().count(), 1);

    QModelIndex index47 = view.model()->index(47,0);
    p = view.visualRect(index47).center();
    QVERIFY(view.viewport()->rect().contains(p));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, p);
    QCOMPARE(view.currentIndex(), index47);
    QCOMPARE(view.selectedItems().count(), 3); //49, 48, 47;

    QModelIndex index44 = view.model()->index(44,0);
    p = view.visualRect(index44).center();
    QVERIFY(view.viewport()->rect().contains(p));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, p);
    QCOMPARE(view.currentIndex(), index44);
    QCOMPARE(view.selectedItems().count(), 6); //49 .. 44;

}

void tst_QAbstractItemView::QTBUG6753_selectOnSelection()
{
    QTableWidget table(5, 5);
    for (int i = 0; i < table.rowCount(); ++i)
        for (int j = 0; j < table.columnCount(); ++j)
            table.setItem(i, j, new QTableWidgetItem("choo-be-doo-wah"));

    centerOnScreen(&table);
    moveCursorAway(&table);
    table.show();
    QVERIFY(QTest::qWaitForWindowExposed(&table));

    table.setSelectionMode(QAbstractItemView::ExtendedSelection);
    table.selectAll();
    QVERIFY(QTest::qWaitForWindowExposed(&table));
    QModelIndex item = table.model()->index(1,1);
    QRect itemRect = table.visualRect(item);
    QTest::mouseMove(table.viewport(), itemRect.center());
    QTest::mouseClick(table.viewport(), Qt::LeftButton, Qt::NoModifier, itemRect.center());

    QCOMPARE(table.selectedItems().count(), 1);
    QCOMPARE(table.selectedItems().first(), table.item(item.row(), item.column()));
}

void tst_QAbstractItemView::testDelegateDestroyEditor()
{
    QTableWidget table(5, 5);
    MyAbstractItemDelegate delegate;
    table.setItemDelegate(&delegate);
    table.edit(table.model()->index(1, 1));
    QAbstractItemView *tv = &table;
    QVERIFY(!delegate.calledVirtualDtor);
    tv->closeEditor(delegate.openedEditor, QAbstractItemDelegate::NoHint);
    QVERIFY(delegate.calledVirtualDtor);
}

void tst_QAbstractItemView::testClickedSignal()
{
    QTableWidget view(5, 5);

    centerOnScreen(&view);
    moveCursorAway(&view);
    view.showNormal();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(static_cast<QWidget *>(&view), QApplication::activeWindow());

    QModelIndex index49 = view.model()->index(49,0);
    QPoint p = view.visualRect(index49).center();
    QVERIFY(view.viewport()->rect().contains(p));

    QSignalSpy clickedSpy(&view, SIGNAL(clicked(QModelIndex)));

    QTest::mouseClick(view.viewport(), Qt::LeftButton, 0, p);
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Fails on WinRT - QTBUG-68297", Abort);
#endif
    QCOMPARE(clickedSpy.count(), 1);

    QTest::mouseClick(view.viewport(), Qt::RightButton, 0, p);
    // We expect that right-clicks do not cause the clicked() signal to
    // be emitted.
    QCOMPARE(clickedSpy.count(), 1);

}

class StateChangeDelegate : public QItemDelegate {
  Q_OBJECT

public:
  explicit StateChangeDelegate(QObject* parent = 0) :
    QItemDelegate(parent)
  {}

  void setEditorData(QWidget *editor, const QModelIndex &index) const override {
      Q_UNUSED(index);
      static bool w = true;
      editor->setEnabled(w);
      w = !w;
  }
};

class StateChangeModel : public QStandardItemModel {
  Q_OBJECT

public:
  explicit StateChangeModel(QObject *parent = 0) :
    QStandardItemModel(parent)
  {}

  void emitDataChanged() {
    emit dataChanged(index(0, 0), index(0, 1));
  }
};


void tst_QAbstractItemView::testChangeEditorState()
{
    // Test for QTBUG-25370
    StateChangeModel model;
    {
      QStandardItem* item = new QStandardItem("a");
      model.setItem(0, 0, item);
    }
    {
      QStandardItem* item = new QStandardItem("b");
      model.setItem(0, 1, item);
    }

    QTableView view;
    view.setEditTriggers(QAbstractItemView::CurrentChanged);
    view.setItemDelegate(new StateChangeDelegate(&view));
    view.setModel(&model);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(static_cast<QWidget *>(&view), QApplication::activeWindow());

    model.emitDataChanged();
    // No segfault - the test passes.
}

void tst_QAbstractItemView::deselectInSingleSelection()
{
    QTableView view;
    QStandardItemModel s;
    s.setRowCount(10);
    s.setColumnCount(10);
    view.setModel(&s);
    centerOnScreen(&view);
    moveCursorAway(&view);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    view.setSelectionMode(QAbstractItemView::SingleSelection);
    view.setEditTriggers(QAbstractItemView::NoEditTriggers);
    QApplication::setActiveWindow(&view);
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    // mouse
    QModelIndex index22 = s.index(2, 2);
    QRect rect22 = view.visualRect(index22);
    QPoint clickpos = rect22.center();
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, clickpos);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, clickpos);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 0);

    // second click with modifier however does select
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ControlModifier, clickpos);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);

    // keyboard
    QTest::keyClick(&view, Qt::Key_Space, Qt::NoModifier);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);
    QTest::keyClick(&view, Qt::Key_Space, Qt::ControlModifier);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 0);

    // second keypress with modifier however does select
    QTest::keyClick(&view, Qt::Key_Space, Qt::ControlModifier);
    QCOMPARE(view.currentIndex(), index22);
    QCOMPARE(view.selectionModel()->selectedIndexes().count(), 1);
}

void tst_QAbstractItemView::testNoActivateOnDisabledItem()
{
    QTreeView treeView;
    QStandardItemModel model(1, 1);
    QStandardItem *item = new QStandardItem("item");
    model.setItem(0, 0, item);
    item->setFlags(Qt::NoItemFlags);
    treeView.setModel(&model);
    centerOnScreen(&treeView);
    moveCursorAway(&treeView);
    treeView.show();

    QApplication::setActiveWindow(&treeView);
    QVERIFY(QTest::qWaitForWindowActive(&treeView));

    QSignalSpy activatedSpy(&treeView, SIGNAL(activated(QModelIndex)));

    // Ensure clicking on a disabled item doesn't emit itemActivated.
    QModelIndex itemIndex = treeView.model()->index(0, 0);
    QPoint clickPos = treeView.visualRect(itemIndex).center();
    QTest::mouseClick(treeView.viewport(), Qt::LeftButton, 0, clickPos);

    QCOMPARE(activatedSpy.count(), 0);
}

void tst_QAbstractItemView::testFocusPolicy_data()
{
    QTest::addColumn<QAbstractItemDelegate*>("delegate");

    QAbstractItemDelegate *styledItemDelegate = new QStyledItemDelegate(this);
    QAbstractItemDelegate *itemDelegate = new QItemDelegate(this);

    QTest::newRow("QStyledItemDelegate") << styledItemDelegate;
    QTest::newRow("QItemDelegate") << itemDelegate;
}

void tst_QAbstractItemView::testFocusPolicy()
{
    QFETCH(QAbstractItemDelegate*, delegate);

    QWidget window;
    QTableView *table = new QTableView(&window);
    table->setItemDelegate(delegate);
    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->addWidget(table);

    QStandardItemModel model;
    model.setRowCount(10);
    model.setColumnCount(10);
    table->setModel(&model);
    table->setCurrentIndex(model.index(1, 1));

    centerOnScreen(&window);
    moveCursorAway(&window);

    window.show();
    QApplication::setActiveWindow(&window);
    QVERIFY(QTest::qWaitForWindowActive(&window));

    // itemview accepts focus => editor is closed => return focus to the itemview
    QPoint clickpos = table->visualRect(model.index(1, 1)).center();
    QTest::mouseDClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, clickpos);
    QWidget *editor = qApp->focusWidget();
    QVERIFY(editor);
    QTest::keyClick(editor, Qt::Key_Escape, Qt::NoModifier);
    QCOMPARE(qApp->focusWidget(), table);

    // itemview doesn't accept focus => editor is closed => clear the focus
    table->setFocusPolicy(Qt::NoFocus);
    QTest::mouseDClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, clickpos);
    editor = qApp->focusWidget();
    QVERIFY(editor);
    QTest::keyClick(editor, Qt::Key_Escape, Qt::NoModifier);
    QVERIFY(!qApp->focusWidget());
}

void tst_QAbstractItemView::QTBUG31411_noSelection()
{
    QWidget window;
    QTableView *table = new QTableView(&window);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    QVBoxLayout *layout = new QVBoxLayout(&window);
    layout->addWidget(table);

    QStandardItemModel model;
    model.setRowCount(10);
    model.setColumnCount(10);
    table->setModel(&model);
    table->setCurrentIndex(model.index(1, 1));

    centerOnScreen(&window);
    moveCursorAway(&window);

    window.show();
    QApplication::setActiveWindow(&window);
    QVERIFY(QTest::qWaitForWindowActive(&window));

    qRegisterMetaType<QItemSelection>();
    QSignalSpy selectionChangeSpy(table->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)));
    QVERIFY(selectionChangeSpy.isValid());

    QPoint clickpos = table->visualRect(model.index(1, 1)).center();
    QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, clickpos);
    QTest::mouseDClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, clickpos);

    QPointer<QWidget> editor1 = qApp->focusWidget();
    QVERIFY(editor1);
    QTest::keyClick(editor1, Qt::Key_Tab, Qt::NoModifier);

    QPointer<QWidget> editor2 = qApp->focusWidget();
    QVERIFY(editor2);
    QTest::keyClick(editor2, Qt::Key_Escape, Qt::NoModifier);

    QCOMPARE(selectionChangeSpy.count(), 0);
}

void tst_QAbstractItemView::QTBUG39324_settingSameInstanceOfIndexWidget()
{
    QStringList list;
    list << "FOO" << "bar";
    QScopedPointer<QStringListModel> model(new QStringListModel(list));

    QScopedPointer<QTableView> table(new QTableView());
    table->setModel(model.data());

    QModelIndex index = model->index(0,0);
    QLineEdit *lineEdit = new QLineEdit();
    table->setIndexWidget(index, lineEdit);
    table->setIndexWidget(index, lineEdit);
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    table->show();
}

void tst_QAbstractItemView::shiftSelectionAfterChangingModelContents()
{
    QStringList list;
    list << "A" << "B" << "C" << "D" << "E" << "F";
    QStringListModel model(list);
    QSortFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&model);
    proxyModel.sort(0, Qt::AscendingOrder);
    proxyModel.setDynamicSortFilter(true);

    QPersistentModelIndex indexA(proxyModel.index(0, 0));
    QPersistentModelIndex indexB(proxyModel.index(1, 0));
    QPersistentModelIndex indexC(proxyModel.index(2, 0));
    QPersistentModelIndex indexD(proxyModel.index(3, 0));
    QPersistentModelIndex indexE(proxyModel.index(4, 0));
    QPersistentModelIndex indexF(proxyModel.index(5, 0));

    QListView view;
    view.setModel(&proxyModel);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    // Click "C"
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, view.visualRect(indexC).center());
    QModelIndexList selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 1);
    QVERIFY(selected.contains(indexC));

    // Insert new item "B1"
    model.insertRow(0);
    model.setData(model.index(0, 0), "B1");

    // Shift-click "D" -> we expect that "C" and "D" are selected
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, view.visualRect(indexD).center());
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 2);
    QVERIFY(selected.contains(indexC));
    QVERIFY(selected.contains(indexD));

    // Click "D"
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, view.visualRect(indexD).center());
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 1);
    QVERIFY(selected.contains(indexD));

    // Remove items "B" and "C"
    model.removeRows(proxyModel.mapToSource(indexB).row(), 1);
    model.removeRows(proxyModel.mapToSource(indexC).row(), 1);
    QVERIFY(!indexB.isValid());
    QVERIFY(!indexC.isValid());

    // Shift-click "F" -> we expect that "D", "E", and "F" are selected
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, view.visualRect(indexF).center());
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 3);
    QVERIFY(selected.contains(indexD));
    QVERIFY(selected.contains(indexE));
    QVERIFY(selected.contains(indexF));

    // Move to "A" by pressing "Up" repeatedly
    while (view.currentIndex() != indexA) {
        QTest::keyClick(&view, Qt::Key_Up);
    }
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 1);
    QVERIFY(selected.contains(indexA));

    // Change the sort order
    proxyModel.sort(0, Qt::DescendingOrder);

    // Shift-click "F" -> All items should be selected
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::ShiftModifier, view.visualRect(indexF).center());
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), model.rowCount());

    // Restore the old sort order
    proxyModel.sort(0, Qt::AscendingOrder);

    // Click "D"
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, view.visualRect(indexD).center());
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 1);
    QVERIFY(selected.contains(indexD));

    // Insert new item "B2"
    model.insertRow(0);
    model.setData(model.index(0, 0), "B2");

    // Press Shift+Down -> "D" and "E" should be selected.
    QTest::keyClick(&view, Qt::Key_Down, Qt::ShiftModifier);
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 2);
    QVERIFY(selected.contains(indexD));
    QVERIFY(selected.contains(indexE));

    // Click "A" to reset the current selection starting point;
    //then select "D" via QAbstractItemView::setCurrentIndex(const QModelIndex&).
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, view.visualRect(indexA).center());
    view.setCurrentIndex(indexD);
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 1);
    QVERIFY(selected.contains(indexD));

    // Insert new item "B3"
    model.insertRow(0);
    model.setData(model.index(0, 0), "B3");

    // Press Shift+Down -> "D" and "E" should be selected.
    QTest::keyClick(&view, Qt::Key_Down, Qt::ShiftModifier);
    selected = view.selectionModel()->selectedIndexes();
    QCOMPARE(selected.count(), 2);
    QVERIFY(selected.contains(indexD));
    QVERIFY(selected.contains(indexE));
}

void tst_QAbstractItemView::QTBUG48968_reentrant_updateEditorGeometries()
{

    QStandardItemModel *m = new QStandardItemModel(this);
    for (int i=0; i<10; ++i) {
        QStandardItem *item = new QStandardItem(QString("Item number %1").arg(i));
        item->setEditable(true);
        for (int j=0; j<5; ++j) {
            QStandardItem *child = new QStandardItem(QString("Child Item number %1").arg(j));
            item->setChild(j, 0, child);
        }
        m->setItem(i, 0, item);
    }

    QTreeView tree;
    tree.setModel(m);
    tree.setRootIsDecorated(false);
    QObject::connect(&tree, SIGNAL(doubleClicked(QModelIndex)), &tree, SLOT(setRootIndex(QModelIndex)));
    tree.show();
    QVERIFY(QTest::qWaitForWindowActive(&tree));

    // Trigger editing idx
    QModelIndex idx = m->index(1, 0);
    const QPoint pos = tree.visualRect(idx).center();
    QTest::mouseClick(tree.viewport(), Qt::LeftButton, Qt::NoModifier, pos);
    QTest::mouseDClick(tree.viewport(), Qt::LeftButton, Qt::NoModifier, pos);

    // Add more children to idx
    QStandardItem *item = m->itemFromIndex(idx);
    for (int j=5; j<10; ++j) {
        QStandardItem *child = new QStandardItem(QString("Child Item number %1").arg(j));
        item->setChild(j, 0, child);
    }

    // No crash, all fine.
}

class ScrollModeProxyStyle: public QProxyStyle
{
public:
    ScrollModeProxyStyle(QAbstractItemView::ScrollMode sm, QStyle *style = 0)
        : QProxyStyle(style)
        , scrollMode(sm == QAbstractItemView::ScrollPerItem ?
                     QAbstractItemView::ScrollPerPixel : QAbstractItemView::ScrollPerItem)
    { }

    int styleHint(QStyle::StyleHint hint, const QStyleOption *opt, const QWidget *w, QStyleHintReturn *returnData) const override
    {
        if (hint == SH_ItemView_ScrollMode)
            return scrollMode;

        return baseStyle()->styleHint(hint, opt, w, returnData);
    }

    QAbstractItemView::ScrollMode scrollMode;
};

void tst_QAbstractItemView::QTBUG50102_SH_ItemView_ScrollMode()
{
    QListView view;

    // Default comes from the style
    auto styleScrollMode = static_cast<QAbstractItemView::ScrollMode>(view.style()->styleHint(QStyle::SH_ItemView_ScrollMode, 0, &view, 0));
    QCOMPARE(view.verticalScrollMode(), styleScrollMode);
    QCOMPARE(view.horizontalScrollMode(), styleScrollMode);

    // Change style, get new value
    ScrollModeProxyStyle proxyStyle1(styleScrollMode);
    view.setStyle(&proxyStyle1);
    auto proxyScrollMode = static_cast<QAbstractItemView::ScrollMode>(view.style()->styleHint(QStyle::SH_ItemView_ScrollMode, 0, &view, 0));
    QVERIFY(styleScrollMode != proxyScrollMode);
    QCOMPARE(view.verticalScrollMode(), proxyScrollMode);
    QCOMPARE(view.horizontalScrollMode(), proxyScrollMode);

    // Explicitly set vertical, same value
    view.setVerticalScrollMode(proxyScrollMode);
    QCOMPARE(view.verticalScrollMode(), proxyScrollMode);
    QCOMPARE(view.horizontalScrollMode(), proxyScrollMode);

    // Change style, won't change value for vertical, will change for horizontal
    ScrollModeProxyStyle proxyStyle2(proxyScrollMode);
    view.setStyle(&proxyStyle2);
    QCOMPARE(view.verticalScrollMode(), proxyScrollMode);
    QCOMPARE(view.horizontalScrollMode(), styleScrollMode);
}

void tst_QAbstractItemView::QTBUG50535_update_on_new_selection_model()
{
    QStandardItemModel model;
    for (int i = 0; i < 10; ++i)
        model.appendRow(new QStandardItem(QStringLiteral("%1").arg(i)));

    class ListView : public QListView
    {
    public:
        ListView()
            : m_paintEventsCount(0), m_deselectedMustBeEmpty(false), m_selectionChangedOk(true)
        {
        }

        void setSelectionModel(QItemSelectionModel *model) override
        {
            m_deselectedMustBeEmpty = !selectionModel() || !model || selectionModel()->model() != model->model();
            QListView::setSelectionModel(model);
            m_deselectedMustBeEmpty = false;
        }
        int m_paintEventsCount;
        bool selectionChangedOk() const { return m_selectionChangedOk; }

    protected:
        bool viewportEvent(QEvent *event) override
        {
            if (event->type() == QEvent::Paint)
                ++m_paintEventsCount;
            return QListView::viewportEvent(event);
        }

        void selectionChanged(const QItemSelection &selected,
                              const QItemSelection &deselected) override
        {
            if (m_deselectedMustBeEmpty && !deselected.isEmpty())
                m_selectionChangedOk = false;

            // Make sure both selections belong to the same model
            foreach (const QModelIndex &nmi, selected.indexes()) {
                foreach (const QModelIndex &omi, deselected.indexes()) {
                    m_selectionChangedOk = m_selectionChangedOk && (nmi.model() == omi.model());
                }
            }
            QListView::selectionChanged(selected, deselected);
        }
    private:
        bool m_deselectedMustBeEmpty;
        bool m_selectionChangedOk;
    };

    // keep the current/selected row in the "low range", i.e. be sure it's visible, otherwise we
    // don't get updates and the test fails.

    ListView view;
    view.setModel(&model);
    view.selectionModel()->setCurrentIndex(model.index(1, 0), QItemSelectionModel::SelectCurrent);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QVERIFY(view.selectionChangedOk());

    QItemSelectionModel selectionModel(&model);
    selectionModel.setCurrentIndex(model.index(2, 0), QItemSelectionModel::Current);

    int oldPaintEventsCount = view.m_paintEventsCount;
    view.setSelectionModel(&selectionModel);
    QTRY_VERIFY(view.m_paintEventsCount > oldPaintEventsCount);
    QVERIFY(view.selectionChangedOk());


    QItemSelectionModel selectionModel2(&model);
    selectionModel2.select(model.index(0, 0), QItemSelectionModel::ClearAndSelect);
    selectionModel2.setCurrentIndex(model.index(1, 0), QItemSelectionModel::Current);

    oldPaintEventsCount = view.m_paintEventsCount;
    view.setSelectionModel(&selectionModel2);
    QTRY_VERIFY(view.m_paintEventsCount > oldPaintEventsCount);
    QVERIFY(view.selectionChangedOk());

    // Tests QAbstractItemView::selectionChanged
    QStandardItemModel model1;
    for (int i = 0; i < 10; ++i)
        model1.appendRow(new QStandardItem(QString::number(i)));
    view.setModel(&model1);

    QItemSelectionModel selectionModel1(&model1);
    selectionModel1.select(model1.index(0, 0), QItemSelectionModel::ClearAndSelect);
    selectionModel1.setCurrentIndex(model1.index(1, 0), QItemSelectionModel::Current);
    view.setSelectionModel(&selectionModel1);
    QVERIFY(view.selectionChangedOk());
}

void tst_QAbstractItemView::testSelectionModelInSyncWithView()
{
    QStandardItemModel model;
    for (int i = 0; i < 10; ++i)
        model.appendRow(new QStandardItem(QStringLiteral("%1").arg(i)));

    class ListView : public QListView
    {
    public:
        using QListView::selectedIndexes;
    };

    ListView view;
    QVERIFY(!view.selectionModel());

    view.setModel(&model);
    QVERIFY(view.selectionModel());
    QVERIFY(view.selectedIndexes().isEmpty());
    QVERIFY(view.selectionModel()->selection().isEmpty());

    view.setCurrentIndex(model.index(0, 0));
    QCOMPARE(view.currentIndex(), model.index(0, 0));
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(0, 0));

    view.selectionModel()->setCurrentIndex(model.index(1, 0), QItemSelectionModel::SelectCurrent);
    QCOMPARE(view.currentIndex(), model.index(1, 0));
    QCOMPARE(view.selectedIndexes(), QModelIndexList() << model.index(1, 0));
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(1, 0));
    QCOMPARE(view.selectionModel()->selection().indexes(), QModelIndexList() << model.index(1, 0));

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QItemSelectionModel selectionModel(&model);
    selectionModel.setCurrentIndex(model.index(2, 0), QItemSelectionModel::Current);

    view.setSelectionModel(&selectionModel);
    QCOMPARE(view.currentIndex(), model.index(2, 0));
    QCOMPARE(view.selectedIndexes(), QModelIndexList());
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(2, 0));
    QCOMPARE(view.selectionModel()->selection().indexes(),  QModelIndexList());


    QItemSelectionModel selectionModel2(&model);
    selectionModel2.select(model.index(0, 0), QItemSelectionModel::ClearAndSelect);
    selectionModel2.setCurrentIndex(model.index(1, 0), QItemSelectionModel::Current);

    view.setSelectionModel(&selectionModel2);
    QCOMPARE(view.currentIndex(), model.index(1, 0));
    QCOMPARE(view.selectedIndexes(), QModelIndexList() << model.index(0, 0));
    QCOMPARE(view.selectionModel()->currentIndex(), model.index(1, 0));
    QCOMPARE(view.selectionModel()->selection().indexes(),  QModelIndexList() << model.index(0, 0));
}

class SetSelectionTestView : public QListView
{
    Q_OBJECT
public:
    SetSelectionTestView() : QListView() {}

signals:
    void setSelectionCalled(const QRect &rect);

protected:
    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags flags) override
    {
        emit setSelectionCalled(rect);
        QListView::setSelection(rect, flags);
    }
};

void tst_QAbstractItemView::testClickToSelect()
{
    // This test verifies that the QRect that is passed from QAbstractItemView::mousePressEvent
    // to the virtual method QAbstractItemView::setSelection(const QRect &, SelectionFlags)
    // is the 1x1 rect which conains exactly the clicked pixel if no modifiers are pressed.

    QStringList list;
    list << "A" << "B" << "C";
    QStringListModel model(list);

    SetSelectionTestView view;
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QSignalSpy spy(&view, &SetSelectionTestView::setSelectionCalled);

    const QModelIndex indexA(model.index(0, 0));
    const QRect visualRectA = view.visualRect(indexA);
    const QPoint centerA = visualRectA.center();

    // Click the center of the visualRect of item "A"
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, centerA);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.back().front().value<QRect>(), QRect(centerA, QSize(1, 1)));

    // Click a point slightly away from the center
    const QPoint nearCenterA = centerA + QPoint(1, 1);
    QVERIFY(visualRectA.contains(nearCenterA));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, nearCenterA);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.back().front().value<QRect>(), QRect(nearCenterA, QSize(1, 1)));
}

void tst_QAbstractItemView::testDialogAsEditor()
{
    DialogItemDelegate delegate;

    QStandardItemModel model;
    model.appendRow(new QStandardItem(QStringLiteral("editme")));

    QListView view;
    view.setItemDelegate(&delegate);
    view.setModel(&model);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    view.edit(model.index(0,0));

    QVERIFY(QTest::qWaitForWindowExposed(delegate.openedEditor));

    delegate.openedEditor->reject();
    QApplication::processEvents();

    QCOMPARE(delegate.result, QDialog::Rejected);

    view.edit(model.index(0,0));

    QVERIFY(QTest::qWaitForWindowExposed(delegate.openedEditor));

    delegate.openedEditor->accept();
    QApplication::processEvents();

    QCOMPARE(delegate.result, QDialog::Accepted);
}

class HoverItemDelegate : public QItemDelegate
{
public:
    HoverItemDelegate()
        : QItemDelegate()
        , m_paintedWithoutHover(false)
    { }

    void paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const override
    {
        Q_UNUSED(painter);

        if (!(opt.state & QStyle::State_MouseOver)) {

            // We don't want to set m_paintedWithoutHover for any item so check for the item at 0,0
            if (index.row() == 0 && index.column() == 0) {
                m_paintedWithoutHover = true;
            }
        }
    }

    mutable bool m_paintedWithoutHover;
};

void tst_QAbstractItemView::QTBUG46785_mouseout_hover_state()
{
    HoverItemDelegate delegate;

    QTableWidget table(5, 5);
    table.verticalHeader()->hide();
    table.horizontalHeader()->hide();
    table.setMouseTracking(true);
    table.setItemDelegate(&delegate);
    centerOnScreen(&table);
    table.show();
    QVERIFY(QTest::qWaitForWindowActive(&table));

    QModelIndex item = table.model()->index(0, 0);
    QRect itemRect = table.visualRect(item);

    // Move the mouse into the center of the item at 0,0 to cause a paint event to occur
    QTest::mouseMove(table.viewport(), itemRect.center());
    QTest::mouseClick(table.viewport(), Qt::LeftButton, 0, itemRect.center());

    delegate.m_paintedWithoutHover = false;

    QTest::mouseMove(table.viewport(), QPoint(-50, 0));

#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "QTest::mouseMove does not work on WinRT", Abort);
#endif
    QTRY_VERIFY(delegate.m_paintedWithoutHover);
}

void tst_QAbstractItemView::testClearModelInClickedSignal()
{
    QStringList list{"A", "B"};
    QStringListModel model(list);

    QListView view;
    view.setModel(&model);
    view.show();

    QWidget::connect(&view, &QListView::clicked, [&view](const QModelIndex &index)
    {
        view.setModel(nullptr);
        QCOMPARE(index.data().toString(), QStringLiteral("B"));
    });

    QModelIndex index = view.model()->index(1, 0);
    QVERIFY(index.isValid());
    QPoint p = view.visualRect(index).center();

    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, p);

    QCOMPARE(view.model(), nullptr);
}

void tst_QAbstractItemView::inputMethodEnabled_data()
{
    QTest::addColumn<QByteArray>("viewType");
    QTest::addColumn<Qt::ItemFlags>("itemFlags");
    QTest::addColumn<bool>("result");

    QList<QByteArray> widgets;
    widgets << "QListView" << "QTreeView" << "QTableView";

    for (const QByteArray &widget : qAsConst(widgets)) {
        QTest::newRow(widget + ": no flags") << widget << Qt::ItemFlags(Qt::NoItemFlags) << false;
        QTest::newRow(widget + ": checkable") << widget << Qt::ItemFlags(Qt::ItemIsUserCheckable) << false;
        QTest::newRow(widget + ": selectable") << widget << Qt::ItemFlags(Qt::ItemIsSelectable) << false;
        QTest::newRow(widget + ": enabled") << widget << Qt::ItemFlags(Qt::ItemIsEnabled) << false;
        QTest::newRow(widget + ": selectable|enabled")
            << widget << Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled) << false;
        QTest::newRow(widget + ": editable|enabled")
            << widget << Qt::ItemFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled) << true;
        QTest::newRow(widget + ": editable|enabled|selectable")
            << widget << Qt::ItemFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable) << true;
    }
}

void tst_QAbstractItemView::inputMethodEnabled()
{
    QFETCH(QByteArray, viewType);
    QFETCH(Qt::ItemFlags, itemFlags);
    QFETCH(bool, result);

    QScopedPointer<QAbstractItemView> view;
    if (viewType == "QListView")
        view.reset(new QListView());
    else if (viewType == "QTableView")
        view.reset(new QTableView());
    else if (viewType == "QTreeView")
        view.reset(new QTreeView());
    else
        QVERIFY(0);

    centerOnScreen(view.data());
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QStandardItemModel *model = new QStandardItemModel(view.data());
    QStandardItem *item = new QStandardItem("first item");
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    model->appendRow(item);

    QStandardItem *secondItem = new QStandardItem("test item");
    secondItem->setFlags(Qt::ItemFlags(itemFlags));
    model->appendRow(secondItem);

    view->setModel(model);

    // Check current changed
    view->setCurrentIndex(model->index(0, 0));
    QVERIFY(!view->testAttribute(Qt::WA_InputMethodEnabled));
    view->setCurrentIndex(model->index(1, 0));
    QCOMPARE(view->testAttribute(Qt::WA_InputMethodEnabled), result);
    view->setCurrentIndex(model->index(0, 0));
    QVERIFY(!view->testAttribute(Qt::WA_InputMethodEnabled));

    // Check focus by switching the activation of the window to force a focus in
    view->setCurrentIndex(model->index(1, 0));
    QApplication::setActiveWindow(0);
    QApplication::setActiveWindow(view.data());
    QVERIFY(QTest::qWaitForWindowActive(view.data()));
    QCOMPARE(view->testAttribute(Qt::WA_InputMethodEnabled), result);

    view->setCurrentIndex(QModelIndex());
    QVERIFY(!view->testAttribute(Qt::WA_InputMethodEnabled));
    QApplication::setActiveWindow(0);
    QApplication::setActiveWindow(view.data());
    QVERIFY(QTest::qWaitForWindowActive(view.data()));
    QModelIndex index = model->index(1, 0);
    QPoint p = view->visualRect(index).center();
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, p);
    if (itemFlags & Qt::ItemIsEnabled)
        QCOMPARE(view->currentIndex(), index);
    QCOMPARE(view->testAttribute(Qt::WA_InputMethodEnabled), result);

    index = model->index(0, 0);
    QApplication::setActiveWindow(0);
    QApplication::setActiveWindow(view.data());
    QVERIFY(QTest::qWaitForWindowActive(view.data()));
    p = view->visualRect(index).center();
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QCOMPARE(view->currentIndex(), index);
    QVERIFY(!view->testAttribute(Qt::WA_InputMethodEnabled));

    // There is a case when it goes to the first visible item so we
    // make the flags of the first item match the ones we are testing
    // to check the attribute correctly
    QApplication::setActiveWindow(0);
    view->setCurrentIndex(QModelIndex());
    view->reset();
    item->setFlags(Qt::ItemFlags(itemFlags));
    QApplication::setActiveWindow(view.data());
    QVERIFY(QTest::qWaitForWindowActive(view.data()));
    QCOMPARE(view->testAttribute(Qt::WA_InputMethodEnabled), result);
}

void tst_QAbstractItemView::currentFollowsIndexWidget_data()
{
    QTest::addColumn<QByteArray>("viewType");

    QList<QByteArray> widgets;
    widgets << "QListView" << "QTreeView" << "QTableView";
    for (const QByteArray &widget : qAsConst(widgets))
        QTest::newRow(widget) << widget;
}

void tst_QAbstractItemView::currentFollowsIndexWidget()
{
    QFETCH(QByteArray, viewType);

    QScopedPointer<QAbstractItemView> view;
    if (viewType == "QListView")
        view.reset(new QListView());
    else if (viewType == "QTableView")
        view.reset(new QTableView());
    else if (viewType == "QTreeView")
        view.reset(new QTreeView());
    else
        QVERIFY(0);

    centerOnScreen(view.data());
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));

    QStandardItemModel *model = new QStandardItemModel(view.data());
    QStandardItem *item1 = new QStandardItem("first item");
    item1->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    model->appendRow(item1);

    QStandardItem *item2 = new QStandardItem("test item");
    item2->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    model->appendRow(item2);

    view->setModel(model);
    QLineEdit *lineEdit1 = new QLineEdit;
    QLineEdit *lineEdit2 = new QLineEdit;
    view->setIndexWidget(item1->index(), lineEdit1);
    view->setIndexWidget(item2->index(), lineEdit2);

    lineEdit2->setFocus();
    QTRY_VERIFY(lineEdit2->hasFocus());
    QCOMPARE(view->currentIndex(), item2->index());
    lineEdit1->setFocus();
    QTRY_VERIFY(lineEdit1->hasFocus());
    QCOMPARE(view->currentIndex(), item1->index());
}

class EditorItemDelegate : public QItemDelegate
{
public:
    EditorItemDelegate() : QItemDelegate(), openedEditor(nullptr) { }
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
                          const QModelIndex &) const override
    {
        openedEditor = new QLineEdit(parent);
        return openedEditor;
    }
    mutable QPointer<QWidget> openedEditor;
};

// Testing the case reported in QTBUG-62253.
// When an itemview with an editor that has focus loses focus
// due to a change in the active window then we need to check
// that the itemview gets focus once the activation is back
// on the original window.
void tst_QAbstractItemView::checkFocusAfterActivationChanges_data()
{
    QTest::addColumn<QString>("viewType");

    QTest::newRow("QListView") << "QListView";
    QTest::newRow("QTableView") << "QTableView";
    QTest::newRow("QTreeView") << "QTreeView";
}

void tst_QAbstractItemView::checkFocusAfterActivationChanges()
{
    QFETCH(QString, viewType);

    const QRect availableGeo = qApp->primaryScreen()->availableGeometry();
    const int halfWidth = availableGeo.width() / 2;
    QWidget otherTopLevel;
    otherTopLevel.setGeometry(availableGeo.x(), availableGeo.y(),
                              halfWidth, availableGeo.height());
    otherTopLevel.show();

    QWidget w;
    w.setGeometry(availableGeo.x() + halfWidth, availableGeo.y(),
                              halfWidth, availableGeo.height());
    QLineEdit *le = new QLineEdit(&w);
    QAbstractItemView *view = 0;
    if (viewType == "QListView")
        view = new QListView(&w);
    else if (viewType == "QTableView")
        view = new QTableView(&w);
    else if (viewType == "QTreeView")
        view = new QTreeView(&w);

    QStandardItemModel model(5, 5);
    view->setModel(&model);
    view->move(0, 50);
    EditorItemDelegate delegate;
    view->setItemDelegate(&delegate);
    w.show();

    QVERIFY(QTest::qWaitForWindowActive(&w));
    QVERIFY(le->hasFocus());

    view->setFocus();
    QVERIFY(view->hasFocus());

    view->edit(model.index(0,0));
    QVERIFY(QTest::qWaitForWindowExposed(delegate.openedEditor));
    QVERIFY(delegate.openedEditor->hasFocus());

    QApplication::setActiveWindow(&otherTopLevel);
    otherTopLevel.setFocus();
    QTRY_VERIFY(!delegate.openedEditor);

    QApplication::setActiveWindow(&w);
    QVERIFY(QTest::qWaitForWindowActive(&w));
    QVERIFY(view->hasFocus());
}

QTEST_MAIN(tst_QAbstractItemView)
#include "tst_qabstractitemview.moc"
