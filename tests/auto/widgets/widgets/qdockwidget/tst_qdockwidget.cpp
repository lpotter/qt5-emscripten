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


#include <qaction.h>
#include <qdockwidget.h>
#include <qmainwindow.h>
#include <qlineedit.h>
#include <qtabbar.h>
#include <QScreen>
#include <QtGui/QPainter>
#include "private/qdockwidget_p.h"

bool hasFeature(QDockWidget *dockwidget, QDockWidget::DockWidgetFeature feature)
{ return (dockwidget->features() & feature) == feature; }

void setFeature(QDockWidget *dockwidget, QDockWidget::DockWidgetFeature feature, bool on = true)
{
    const QDockWidget::DockWidgetFeatures features = dockwidget->features();
    dockwidget->setFeatures(on ? features | feature : features & ~feature);
}

class tst_QDockWidget : public QObject
{
     Q_OBJECT

public:
    tst_QDockWidget();

private slots:
    void getSetCheck();
    void widget();
    void features();
    void setFloating();
    void allowedAreas();
    void toggleViewAction();
    void visibilityChanged();
    void updateTabBarOnVisibilityChanged();
    void dockLocationChanged();
    void setTitleBarWidget();
    void titleBarDoubleClick();
    void restoreStateOfFloating();
    void restoreDockWidget();
    void restoreStateWhileStillFloating();
    // task specific tests:
    void task165177_deleteFocusWidget();
    void task169808_setFloating();
    void task237438_setFloatingCrash();
    void task248604_infiniteResize();
    void task258459_visibilityChanged();
    void taskQTBUG_1665_closableChanged();
    void taskQTBUG_9758_undockedGeometry();
};

// Testing get/set functions
void tst_QDockWidget::getSetCheck()
{
    QDockWidget obj1;
    // QWidget * QDockWidget::widget()
    // void QDockWidget::setWidget(QWidget *)
    QWidget *var1 = new QWidget();
    obj1.setWidget(var1);
    QCOMPARE(var1, obj1.widget());
    obj1.setWidget((QWidget *)0);
    QCOMPARE((QWidget *)0, obj1.widget());
    delete var1;

    // DockWidgetFeatures QDockWidget::features()
    // void QDockWidget::setFeatures(DockWidgetFeatures)
    obj1.setFeatures(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetClosable));
    QCOMPARE(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetClosable), obj1.features());
    obj1.setFeatures(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetMovable));
    QCOMPARE(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetMovable), obj1.features());
    obj1.setFeatures(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetFloatable));
    QCOMPARE(QDockWidget::DockWidgetFeatures(QDockWidget::DockWidgetFloatable), obj1.features());
    obj1.setFeatures(QDockWidget::DockWidgetFeatures(QDockWidget::AllDockWidgetFeatures));
    QCOMPARE(QDockWidget::DockWidgetFeatures(QDockWidget::AllDockWidgetFeatures), obj1.features());
    obj1.setFeatures(QDockWidget::DockWidgetFeatures(QDockWidget::NoDockWidgetFeatures));
    QCOMPARE(QDockWidget::DockWidgetFeatures(QDockWidget::NoDockWidgetFeatures), obj1.features());
}

tst_QDockWidget::tst_QDockWidget()
{
    qRegisterMetaType<QDockWidget::DockWidgetFeatures>("QDockWidget::DockWidgetFeatures");
    qRegisterMetaType<Qt::DockWidgetAreas>("Qt::DockWidgetAreas");
}

void tst_QDockWidget::widget()
{
    {
        QDockWidget dw;
        QVERIFY(!dw.widget());
    }

    {
        QDockWidget dw;
        QWidget *w1 = new QWidget;
        QWidget *w2 = new QWidget;

        dw.setWidget(w1);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w1);
        QCOMPARE(w1->parentWidget(), (QWidget*)&dw);

        dw.setWidget(0);
        QVERIFY(!dw.widget());

        dw.setWidget(w2);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w2);
        QCOMPARE(w2->parentWidget(), (QWidget*)&dw);

        dw.setWidget(0);
        QVERIFY(!dw.widget());

        dw.setWidget(w1);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w1);
        QCOMPARE(w1->parentWidget(), (QWidget*)&dw);

        dw.setWidget(w2);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w2);
        QCOMPARE(w2->parentWidget(), (QWidget*)&dw);

        dw.setWidget(0);
        QVERIFY(!dw.widget());
    }

    {
        QDockWidget dw;
        QWidget *w1 = new QWidget;
        QWidget *w2 = new QWidget;

        dw.setWidget(w1);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w1);
        QCOMPARE(w1->parentWidget(), (QWidget*)&dw);

        w1->setParent(0);
        QVERIFY(!dw.widget());

        dw.setWidget(w2);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w2);
        QCOMPARE(w2->parentWidget(), (QWidget*)&dw);

        w2->setParent(0);
        QVERIFY(!dw.widget());

        dw.setWidget(w1);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w1);
        QCOMPARE(w1->parentWidget(), (QWidget*)&dw);

        dw.setWidget(w2);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w2);
        QCOMPARE(w2->parentWidget(), (QWidget*)&dw);

        w1->setParent(0);
        QVERIFY(dw.widget() != 0);
        QCOMPARE(dw.widget(), w2);
        QCOMPARE(w2->parentWidget(), (QWidget*)&dw);

        w2->setParent(0);
        QVERIFY(!dw.widget());
        delete w1;
        delete w2;
    }
}

void tst_QDockWidget::features()
{
    QDockWidget dw;

    QSignalSpy spy(&dw, SIGNAL(featuresChanged(QDockWidget::DockWidgetFeatures)));

    // default features for dock widgets
    int allDockWidgetFeatures = QDockWidget::DockWidgetClosable |
                                QDockWidget::DockWidgetMovable  |
                                QDockWidget::DockWidgetFloatable;

    // defaults
    QCOMPARE(dw.features(), allDockWidgetFeatures);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));

    // test individual setting
    setFeature(&dw, QDockWidget::DockWidgetClosable, false);
    QCOMPARE(dw.features(), QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.count(), 1);
    QCOMPARE((int)*(static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData())),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.count(), 0);
    spy.clear();

    setFeature(&dw, QDockWidget::DockWidgetClosable);
    QCOMPARE(dw.features(), allDockWidgetFeatures);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.count(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.count(), 0);
    spy.clear();

    setFeature(&dw, QDockWidget::DockWidgetMovable, false);
    QCOMPARE(dw.features(), QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.count(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.count(), 0);
    spy.clear();

    setFeature(&dw, QDockWidget::DockWidgetMovable);
    QCOMPARE(dw.features(), allDockWidgetFeatures);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.count(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.count(), 0);
    spy.clear();

    setFeature(&dw, QDockWidget::DockWidgetFloatable, false);
    QCOMPARE(dw.features(), QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.count(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.count(), 0);
    spy.clear();

    setFeature(&dw, QDockWidget::DockWidgetFloatable);
    QCOMPARE(dw.features(), allDockWidgetFeatures);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.count(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.count(), 0);
    spy.clear();

    // set all at once
    dw.setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    QCOMPARE(dw.features(), QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.count(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.count(), 0);
    spy.clear();

    dw.setFeatures(QDockWidget::DockWidgetClosable);
    QCOMPARE(dw.features(), QDockWidget::DockWidgetClosable);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(!hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.count(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.count(), 0);
    spy.clear();

    dw.setFeatures(QDockWidget::AllDockWidgetFeatures);
    QCOMPARE(dw.features(), QDockWidget::AllDockWidgetFeatures);
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetClosable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetMovable));
    QVERIFY(hasFeature(&dw, QDockWidget::DockWidgetFloatable));
    QCOMPARE(spy.count(), 1);
    QCOMPARE((int)*static_cast<const QDockWidget::DockWidgetFeature *>(spy.at(0).value(0).constData()),
            (int)dw.features());
    spy.clear();
    dw.setFeatures(dw.features());
    QCOMPARE(spy.count(), 0);
    spy.clear();
}

void tst_QDockWidget::setFloating()
{
    const QRect deskRect = QGuiApplication::primaryScreen()->availableGeometry();
    QMainWindow mw;
    mw.move(deskRect.left() + deskRect.width() * 2 / 3, deskRect.top() + deskRect.height() / 3);
    QDockWidget dw;
    mw.addDockWidget(Qt::LeftDockWidgetArea, &dw);

    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));

    QVERIFY(!dw.isFloating());
    const QPoint dockedPosition = dw.mapToGlobal(dw.pos());

    QSignalSpy spy(&dw, SIGNAL(topLevelChanged(bool)));

    dw.setFloating(true);
    const QPoint floatingPosition = dw.pos();

    // QTBUG-31044, show approximately at old position, give or take window frame.
    QVERIFY((dockedPosition - floatingPosition).manhattanLength() < 50);

    QVERIFY(dw.isFloating());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).value(0).toBool(), dw.isFloating());
    spy.clear();
    dw.setFloating(dw.isFloating());
    QCOMPARE(spy.count(), 0);
    spy.clear();

    dw.setFloating(false);
    QVERIFY(!dw.isFloating());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).value(0).toBool(), dw.isFloating());
    spy.clear();
    dw.setFloating(dw.isFloating());
    QCOMPARE(spy.count(), 0);
    spy.clear();
}

void tst_QDockWidget::allowedAreas()
{
    QDockWidget dw;

    QSignalSpy spy(&dw, SIGNAL(allowedAreasChanged(Qt::DockWidgetAreas)));

    // default
    QCOMPARE(dw.allowedAreas(), Qt::AllDockWidgetAreas);
    QVERIFY(dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::BottomDockWidgetArea));

    // a single dock window area
    dw.setAllowedAreas(Qt::LeftDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::LeftDockWidgetArea);
    QVERIFY(dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.count(), 0);

    dw.setAllowedAreas(Qt::RightDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::RightDockWidgetArea);
    QVERIFY(!dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.count(), 0);

    dw.setAllowedAreas(Qt::TopDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::TopDockWidgetArea);
    QVERIFY(!dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.count(), 0);

    dw.setAllowedAreas(Qt::BottomDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::BottomDockWidgetArea);
    QVERIFY(!dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.count(), 0);

    // multiple dock window areas
    dw.setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    QVERIFY(!dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.count(), 0);

    dw.setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QVERIFY(dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.count(), 0);

    dw.setAllowedAreas(Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::TopDockWidgetArea | Qt::LeftDockWidgetArea);
    QVERIFY(dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.count(), 0);

    dw.setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    QCOMPARE(dw.allowedAreas(), Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    QVERIFY(!dw.isAreaAllowed(Qt::LeftDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::RightDockWidgetArea));
    QVERIFY(!dw.isAreaAllowed(Qt::TopDockWidgetArea));
    QVERIFY(dw.isAreaAllowed(Qt::BottomDockWidgetArea));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(*static_cast<const Qt::DockWidgetAreas *>(spy.at(0).value(0).constData()),
            dw.allowedAreas());
    spy.clear();
    dw.setAllowedAreas(dw.allowedAreas());
    QCOMPARE(spy.count(), 0);
}

void tst_QDockWidget::toggleViewAction()
{
    QMainWindow mw;
    QDockWidget dw(&mw);
    mw.addDockWidget(Qt::LeftDockWidgetArea, &dw);
    mw.show();
    QAction *toggleViewAction = dw.toggleViewAction();
    QVERIFY(!dw.isHidden());
    toggleViewAction->trigger();
    QVERIFY(dw.isHidden());
    toggleViewAction->trigger();
    QVERIFY(!dw.isHidden());
    toggleViewAction->trigger();
    QVERIFY(dw.isHidden());
}

void tst_QDockWidget::visibilityChanged()
{
    QMainWindow mw;
    QDockWidget dw;
    QSignalSpy spy(&dw, SIGNAL(visibilityChanged(bool)));

    mw.addDockWidget(Qt::LeftDockWidgetArea, &dw);
    mw.show();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    spy.clear();

    dw.hide();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    spy.clear();

    dw.hide();
    QCOMPARE(spy.count(), 0);

    dw.show();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    spy.clear();

    dw.show();
    QCOMPARE(spy.count(), 0);

    QDockWidget dw2;
    mw.tabifyDockWidget(&dw, &dw2);
    dw2.show();
    dw2.raise();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    spy.clear();

    dw2.hide();
    qApp->processEvents();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    spy.clear();

    dw2.show();
    dw2.raise();
    qApp->processEvents();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    spy.clear();

    dw.raise();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
    spy.clear();

    dw.raise();
    QCOMPARE(spy.count(), 0);

    dw2.raise();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);
    spy.clear();

    dw2.raise();
    QCOMPARE(spy.count(), 0);

    mw.addDockWidget(Qt::RightDockWidgetArea, &dw2);
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);
}

void tst_QDockWidget::updateTabBarOnVisibilityChanged()
{
    // QTBUG49045: Populate tabified dock area with 4 widgets, set the tab
    // index to 2 (dw2), hide dw0, dw1 and check that the tab index is 0 (dw3).
    QMainWindow mw;
    mw.setMinimumSize(400, 400);
    mw.setWindowTitle(QTest::currentTestFunction());
    QDockWidget *dw0 = new QDockWidget("d1", &mw);
    dw0->setAllowedAreas(Qt::LeftDockWidgetArea);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw0);
    QDockWidget *dw1 = new QDockWidget("d2", &mw);
    dw1->setAllowedAreas(Qt::LeftDockWidgetArea);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw1);
    QDockWidget *dw2 = new QDockWidget("d3", &mw);
    dw2->setAllowedAreas(Qt::LeftDockWidgetArea);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw2);
    QDockWidget *dw3 = new QDockWidget("d4", &mw);
    dw3->setAllowedAreas(Qt::LeftDockWidgetArea);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw3);
    mw.tabifyDockWidget(dw0, dw1);
    mw.tabifyDockWidget(dw1, dw2);
    mw.tabifyDockWidget(dw2, dw3);

    QTabBar *tabBar = mw.findChild<QTabBar *>();
    QVERIFY(tabBar);
    tabBar->setCurrentIndex(2);

    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));

    QCOMPARE(tabBar->currentIndex(), 2);

    dw0->hide();
    dw1->hide();
    QTRY_COMPARE(tabBar->count(), 2);
    QCOMPARE(tabBar->currentIndex(), 0);
}

Q_DECLARE_METATYPE(Qt::DockWidgetArea)

void tst_QDockWidget::dockLocationChanged()
{
    qRegisterMetaType<Qt::DockWidgetArea>("Qt::DockWidgetArea");

    QMainWindow mw;
    QDockWidget dw;
    dw.setObjectName("dock1");
    QSignalSpy spy(&dw, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)));

    mw.addDockWidget(Qt::LeftDockWidgetArea, &dw);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<Qt::DockWidgetArea>(spy.at(0).at(0)),
                Qt::LeftDockWidgetArea);
    spy.clear();

    mw.addDockWidget(Qt::LeftDockWidgetArea, &dw);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<Qt::DockWidgetArea>(spy.at(0).at(0)),
                Qt::LeftDockWidgetArea);
    spy.clear();

    mw.addDockWidget(Qt::RightDockWidgetArea, &dw);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<Qt::DockWidgetArea>(spy.at(0).at(0)),
                Qt::RightDockWidgetArea);
    spy.clear();

    mw.removeDockWidget(&dw);
    QCOMPARE(spy.count(), 0);

    QDockWidget dw2;
    dw2.setObjectName("dock2");
    mw.addDockWidget(Qt::TopDockWidgetArea, &dw2);
    mw.tabifyDockWidget(&dw2, &dw);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<Qt::DockWidgetArea>(spy.at(0).at(0)),
                Qt::TopDockWidgetArea);
    spy.clear();

    mw.splitDockWidget(&dw2, &dw, Qt::Horizontal);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<Qt::DockWidgetArea>(spy.at(0).at(0)),
                Qt::TopDockWidgetArea);
    spy.clear();

    dw.setFloating(true);
    QTest::qWait(100);
    dw.setFloating(false);
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<Qt::DockWidgetArea>(spy.at(0).at(0)),
             Qt::TopDockWidgetArea);
    spy.clear();

    QByteArray ba = mw.saveState();
    mw.restoreState(ba);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(qvariant_cast<Qt::DockWidgetArea>(spy.at(0).at(0)),
             Qt::TopDockWidgetArea);
}

void tst_QDockWidget::setTitleBarWidget()
{
    //test the successive usage of setTitleBarWidget

    QDockWidget dock;
    QWidget w, w2;

    dock.show();
    qApp->processEvents();

    dock.setTitleBarWidget(&w);
    qApp->processEvents();
    QCOMPARE(w.isVisible(), true);

    //set another widget as the titlebar widget
    dock.setTitleBarWidget(&w2); // this used to crash (task 184668)
    qApp->processEvents();
    QCOMPARE(w.isVisible(), false);
    QCOMPARE(w2.isVisible(), true);

    //tries to reset the titlebarwidget to none
    dock.setTitleBarWidget(0);
    qApp->processEvents();
    QCOMPARE(w.isVisible(), false);
    QCOMPARE(w2.isVisible(), false);
}

void tst_QDockWidget::titleBarDoubleClick()
{
    QMainWindow win;
    QDockWidget dock(&win);
    win.show();
    dock.setFloating(true);

    QEvent e(QEvent::NonClientAreaMouseButtonDblClick);
    QApplication::sendEvent(&dock, &e);
    QVERIFY(dock.isFloating());
    QCOMPARE(win.dockWidgetArea(&dock), Qt::NoDockWidgetArea);

    win.addDockWidget(Qt::TopDockWidgetArea, &dock);
    dock.setFloating(true);
    QApplication::sendEvent(&dock, &e);
    QVERIFY(!dock.isFloating());
    QCOMPARE(win.dockWidgetArea(&dock), Qt::TopDockWidgetArea);
}

static QDockWidget *createTestDock(QMainWindow &parent)
{
    const QString title = QStringLiteral("dock1");
    QDockWidget *dock = new QDockWidget(title, &parent);
    dock->setObjectName(title);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    return dock;
}

void tst_QDockWidget::restoreStateOfFloating()
{
    QMainWindow mw;
    QDockWidget *dock = createTestDock(mw);
    mw.addDockWidget(Qt::TopDockWidgetArea, dock);
    QVERIFY(!dock->isFloating());
    QByteArray ba = mw.saveState();
    dock->setFloating(true);
    QVERIFY(dock->isFloating());
    QVERIFY(mw.restoreState(ba));
    QVERIFY(!dock->isFloating());
}

void tst_QDockWidget::restoreStateWhileStillFloating()
{
    // When the dock widget is already floating then it takes a different code path
    // so this test covers the case where the restoreState() is effectively just
    // moving it back and resizing it
    const QRect availGeom = QGuiApplication::primaryScreen()->availableGeometry();
    const QPoint startingDockPos = availGeom.center();
    QMainWindow mw;
    QDockWidget *dock = createTestDock(mw);
    mw.addDockWidget(Qt::TopDockWidgetArea, dock);
    dock->setFloating(true);
    dock->move(startingDockPos);
    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));
    QVERIFY(dock->isFloating());
    QByteArray ba = mw.saveState();
    const QPoint dockPos = dock->pos();
    dock->move(availGeom.topLeft() + QPoint(10, 10));
    dock->resize(dock->size() + QSize(10, 10));
    QVERIFY(mw.restoreState(ba));
    QVERIFY(dock->isFloating());
    if (!QGuiApplication::platformName().compare("xcb", Qt::CaseInsensitive))
        QTRY_COMPARE(dock->pos(), dockPos);
}

void tst_QDockWidget::restoreDockWidget()
{
    QByteArray geometry;
    QByteArray state;

    const QString name = QStringLiteral("main");
    const QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    const QSize size = availableGeometry.size() / 5;
    const QPoint mainWindowPos = availableGeometry.bottomRight() - QPoint(size.width(), size.height()) - QPoint(100, 100);
    const QPoint dockPos = availableGeometry.center();

    {
        QMainWindow saveWindow;
        saveWindow.setObjectName(name);
        saveWindow.setWindowTitle(QTest::currentTestFunction() + QStringLiteral(" save"));
        saveWindow.resize(size);
        saveWindow.move(mainWindowPos);
        saveWindow.restoreState(QByteArray());
        QDockWidget *dock = createTestDock(saveWindow);
        QVERIFY(!saveWindow.restoreDockWidget(dock)); // Not added, no placeholder
        saveWindow.addDockWidget(Qt::TopDockWidgetArea, dock);
        dock->setFloating(true);
        dock->resize(size);
        dock->move(dockPos);
        saveWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&saveWindow));
        QVERIFY(dock->isFloating());
        state = saveWindow.saveState();
        geometry = saveWindow.saveGeometry();

        // QTBUG-49832: Delete and recreate the dock; it should be restored to the same position.
        delete dock;
        dock = createTestDock(saveWindow);
        QVERIFY(saveWindow.restoreDockWidget(dock));
        dock->show();
        QVERIFY(QTest::qWaitForWindowExposed(dock));
        QTRY_VERIFY(dock->isFloating());
        QTRY_COMPARE(dock->pos(), dockPos);
    }

    QVERIFY(!geometry.isEmpty());
    QVERIFY(!state.isEmpty());

    // QTBUG-45780: Completely recreate the dock widget from the saved state.
    {
        QMainWindow restoreWindow;
        restoreWindow.setObjectName(name);
        restoreWindow.setWindowTitle(QTest::currentTestFunction() + QStringLiteral(" restore"));
        QVERIFY(restoreWindow.restoreState(state));
        QVERIFY(restoreWindow.restoreGeometry(geometry));

        // QMainWindow::restoreDockWidget() restores the state when adding the dock
        // after restoreState().
        QDockWidget *dock = createTestDock(restoreWindow);
        QVERIFY(restoreWindow.restoreDockWidget(dock));
        restoreWindow.show();
        QVERIFY(QTest::qWaitForWindowExposed(&restoreWindow));
        QTRY_VERIFY(dock->isFloating());
        QTRY_COMPARE(dock->pos(), dockPos);
    }
}

void tst_QDockWidget::task165177_deleteFocusWidget()
{
    QMainWindow mw;
    QDockWidget *dw = new QDockWidget(&mw);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw);
    QLineEdit *ledit = new QLineEdit;
    dw->setWidget(ledit);
    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));
    qApp->processEvents();
    dw->setFloating(true);
    delete ledit;
    QCOMPARE(mw.focusWidget(), nullptr);
    QCOMPARE(dw->focusWidget(), nullptr);
}

void tst_QDockWidget::task169808_setFloating()
{
    //we try to test if the sizeHint of the dock widget widget is taken into account

    class MyWidget : public QWidget
    {
    public:
        QSize sizeHint() const
        {
            const QRect& deskRect = QGuiApplication::primaryScreen()->availableGeometry();
            return QSize(qMin(300, deskRect.width() / 2), qMin(300, deskRect.height() / 2));
        }

        QSize minimumSizeHint() const
        {
            return QSize(20,20);
        }

        void paintEvent(QPaintEvent *)
        {
            QPainter p(this);
            p.fillRect(rect(), Qt::red);
        }
    };
    QMainWindow mw;
    mw.setCentralWidget(new MyWidget);
    QDockWidget *dw = new QDockWidget("my dock");
    dw->setWidget(new MyWidget);
    mw.addDockWidget(Qt::LeftDockWidgetArea, dw);
    dw->setFloating(true);
    mw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&mw));

#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Widgets are maximized on WinRT by default", Abort);
#endif
    QCOMPARE(dw->widget()->size(), dw->widget()->sizeHint());

    //and now we try to test if the contents margin is taken into account
    dw->widget()->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    dw->setFloating(false);
    QVERIFY(QTest::qWaitForWindowExposed(&mw));
    qApp->processEvents(); //leave time processing events


    const QSize oldSize = dw->size();
    const int margin = 20;

    dw->setContentsMargins(margin, margin, margin, margin);

    QVERIFY(QTest::qWaitForWindowExposed(&mw));
    qApp->processEvents(); //leave time processing events

    //widget size shouldn't have changed
    QCOMPARE(dw->widget()->size(), dw->widget()->sizeHint());
    //dockwidget should be bigger
    QCOMPARE(dw->size(), oldSize + QSize(margin * 2, margin * 2));


}

void tst_QDockWidget::task237438_setFloatingCrash()
{
    //should not crash
    QDockWidget pqdwDock;
    pqdwDock.setFloating(false);
    pqdwDock.show();
}

void tst_QDockWidget::task248604_infiniteResize()
{
    QDockWidget d;
    QTabWidget *t = new QTabWidget;
    t->addTab(new QWidget, "Foo");
    d.setWidget(t);
    d.setContentsMargins(2, 2, 2, 2);
    d.setMinimumSize(320, 240);
    d.showNormal();
#ifdef Q_OS_WINRT
    QEXPECT_FAIL("", "Widgets are maximized on WinRT by default", Abort);
#endif
    QTRY_COMPARE(d.size(), QSize(320, 240));
}


void tst_QDockWidget::task258459_visibilityChanged()
{
    QMainWindow win;
    QDockWidget dock1, dock2;
    win.addDockWidget(Qt::RightDockWidgetArea, &dock1);
    win.tabifyDockWidget(&dock1, &dock2);
    QSignalSpy spy1(&dock1, SIGNAL(visibilityChanged(bool)));
    QSignalSpy spy2(&dock2, SIGNAL(visibilityChanged(bool)));
    win.show();
    QVERIFY(QTest::qWaitForWindowActive(&win));
    QCOMPARE(spy1.count(), 1);
    QCOMPARE(spy1.first().first().toBool(), false); //dock1 is invisible
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy2.first().first().toBool(), true); //dock1 is visible
}

void tst_QDockWidget::taskQTBUG_1665_closableChanged()
{
    QDockWidget dock;
    dock.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dock));

    QDockWidgetLayout *l = qobject_cast<QDockWidgetLayout*>(dock.layout());

    if (l && !l->nativeWindowDeco())
        QSKIP("this machine doesn't support native dock widget");

    QVERIFY(dock.windowFlags() & Qt::WindowCloseButtonHint);

    //now let's remove the closable attribute
    dock.setFeatures(dock.features() ^ QDockWidget::DockWidgetClosable);
    QVERIFY(!(dock.windowFlags() & Qt::WindowCloseButtonHint));
}

void tst_QDockWidget::taskQTBUG_9758_undockedGeometry()
{
    QMainWindow window;
    QDockWidget dock1(&window);
    QDockWidget dock2(&window);
    window.addDockWidget(Qt::RightDockWidgetArea, &dock1);
    window.addDockWidget(Qt::RightDockWidgetArea, &dock2);
    window.tabifyDockWidget(&dock1, &dock2);
    dock1.hide();
    dock2.hide();
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    dock1.setFloating(true);
    dock1.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dock1));

    QVERIFY(dock1.x() >= 0);
    QVERIFY(dock1.y() >= 0);
}



QTEST_MAIN(tst_QDockWidget)
#include "tst_qdockwidget.moc"
