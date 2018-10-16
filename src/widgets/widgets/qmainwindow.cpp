/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

//#define QT_EXPERIMENTAL_CLIENT_DECORATIONS

#include "qmainwindow.h"
#include "qmainwindowlayout_p.h"

#if QT_CONFIG(dockwidget)
#include "qdockwidget.h"
#endif
#if QT_CONFIG(toolbar)
#include "qtoolbar.h"
#endif

#include <qapplication.h>
#include <qmenu.h>
#if QT_CONFIG(menubar)
#include <qmenubar.h>
#endif
#if QT_CONFIG(statusbar)
#include <qstatusbar.h>
#endif
#include <qevent.h>
#include <qstyle.h>
#include <qdebug.h>
#include <qpainter.h>

#include <private/qwidget_p.h>
#if QT_CONFIG(toolbar)
#include "qtoolbar_p.h"
#endif
#include "qwidgetanimator_p.h"
#ifdef Q_OS_OSX
#include <qpa/qplatformnativeinterface.h>
#endif

QT_BEGIN_NAMESPACE

class QMainWindowPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QMainWindow)
public:
    inline QMainWindowPrivate()
        : layout(0), explicitIconSize(false), toolButtonStyle(Qt::ToolButtonIconOnly)
#ifdef Q_OS_OSX
            , useUnifiedToolBar(false)
#endif
    { }
    QMainWindowLayout *layout;
    QSize iconSize;
    bool explicitIconSize;
    Qt::ToolButtonStyle toolButtonStyle;
#ifdef Q_OS_OSX
    bool useUnifiedToolBar;
#endif
    void init();

    static inline QMainWindowLayout *mainWindowLayout(const QMainWindow *mainWindow)
    {
        return mainWindow ? mainWindow->d_func()->layout : static_cast<QMainWindowLayout *>(0);
    }
};

QMainWindowLayout *qt_mainwindow_layout(const QMainWindow *mainWindow)
{
    return QMainWindowPrivate::mainWindowLayout(mainWindow);
}

#ifdef QT_EXPERIMENTAL_CLIENT_DECORATIONS
Q_WIDGETS_EXPORT void qt_setMainWindowTitleWidget(QMainWindow *mainWindow, Qt::DockWidgetArea area, QWidget *widget)
{
    QGridLayout *topLayout = qobject_cast<QGridLayout *>(mainWindow->layout());
    Q_ASSERT(topLayout);

    int row = 0;
    int column = 0;

    switch (area) {
    case Qt::LeftDockWidgetArea:
        row = 1;
        column = 0;
        break;
    case Qt::TopDockWidgetArea:
        row = 0;
        column = 1;
        break;
    case Qt::BottomDockWidgetArea:
        row = 2;
        column = 1;
        break;
    case Qt::RightDockWidgetArea:
        row = 1;
        column = 2;
        break;
    default:
        Q_ASSERT_X(false, "qt_setMainWindowTitleWidget", "Unknown area");
        return;
    }

    if (QLayoutItem *oldItem = topLayout->itemAtPosition(row, column))
        delete oldItem->widget();
    topLayout->addWidget(widget, row, column);
}
#endif

void QMainWindowPrivate::init()
{
    Q_Q(QMainWindow);

#ifdef QT_EXPERIMENTAL_CLIENT_DECORATIONS
    QGridLayout *topLayout = new QGridLayout(q);
    topLayout->setContentsMargins(0, 0, 0, 0);

    layout = new QMainWindowLayout(q, topLayout);

    topLayout->addItem(layout, 1, 1);
#else
    layout = new QMainWindowLayout(q, 0);
#endif

    const int metric = q->style()->pixelMetric(QStyle::PM_ToolBarIconSize, 0, q);
    iconSize = QSize(metric, metric);
    q->setAttribute(Qt::WA_Hover);
}

/*
    The Main Window:

    +----------------------------------------------------------+
    | Menu Bar                                                 |
    +----------------------------------------------------------+
    | Tool Bar Area                                            |
    |   +--------------------------------------------------+   |
    |   | Dock Window Area                                 |   |
    |   |   +------------------------------------------+   |   |
    |   |   |                                          |   |   |
    |   |   | Central Widget                           |   |   |
    |   |   |                                          |   |   |
    |   |   |                                          |   |   |
    |   |   |                                          |   |   |
    |   |   |                                          |   |   |
    |   |   |                                          |   |   |
    |   |   |                                          |   |   |
    |   |   |                                          |   |   |
    |   |   |                                          |   |   |
    |   |   |                                          |   |   |
    |   |   |                                          |   |   |
    |   |   +------------------------------------------+   |   |
    |   |                                                  |   |
    |   +--------------------------------------------------+   |
    |                                                          |
    +----------------------------------------------------------+
    | Status Bar                                               |
    +----------------------------------------------------------+

*/

/*!
    \class QMainWindow
    \brief The QMainWindow class provides a main application
           window.
    \ingroup mainwindow-classes
    \inmodule QtWidgets

    \tableofcontents

    \section1 Qt Main Window Framework

    A main window provides a framework for building an
    application's user interface. Qt has QMainWindow and its \l{Main
    Window and Related Classes}{related classes} for main window
    management. QMainWindow has its own layout to which you can add
    \l{QToolBar}s, \l{QDockWidget}s, a
    QMenuBar, and a QStatusBar. The layout has a center area that can
    be occupied by any kind of widget. You can see an image of the
    layout below.

    \image mainwindowlayout.png

    \note Creating a main window without a central widget is not supported.
    You must have a central widget even if it is just a placeholder.

    \section1 Creating Main Window Components

    A central widget will typically be a standard Qt widget such
    as a QTextEdit or a QGraphicsView. Custom widgets can also be
    used for advanced applications. You set the central widget with \c
    setCentralWidget().

    Main windows have either a single (SDI) or multiple (MDI)
    document interface. You create MDI applications in Qt by using a
    QMdiArea as the central widget.

    We will now examine each of the other widgets that can be
    added to a main window. We give examples on how to create and add
    them.

    \section2 Creating Menus

    Qt implements menus in QMenu and QMainWindow keeps them in a
    QMenuBar. \l{QAction}{QAction}s are added to the menus, which
    display them as menu items.

    You can add new menus to the main window's menu bar by calling
    \c menuBar(), which returns the QMenuBar for the window, and then
    add a menu with QMenuBar::addMenu().

    QMainWindow comes with a default menu bar, but you can also
    set one yourself with \c setMenuBar(). If you wish to implement a
    custom menu bar (i.e., not use the QMenuBar widget), you can set it
    with \c setMenuWidget().

    An example of how to create menus follows:

    \code
    void MainWindow::createMenus()
    {
        fileMenu = menuBar()->addMenu(tr("&File"));
        fileMenu->addAction(newAct);
        fileMenu->addAction(openAct);
        fileMenu->addAction(saveAct);
    \endcode

    The \c createPopupMenu() function creates popup menus when the
    main window receives context menu events.  The default
    implementation generates a menu with the checkable actions from
    the dock widgets and toolbars. You can reimplement \c
    createPopupMenu() for a custom menu.

    \section2 Creating Toolbars

    Toolbars are implemented in the QToolBar class.  You add a
    toolbar to a main window with \c addToolBar().

    You control the initial position of toolbars by assigning them
    to a specific Qt::ToolBarArea. You can split an area by inserting
    a toolbar break - think of this as a line break in text editing -
    with \c addToolBarBreak() or \c insertToolBarBreak(). You can also
    restrict placement by the user with QToolBar::setAllowedAreas()
    and QToolBar::setMovable().

    The size of toolbar icons can be retrieved with \c iconSize().
    The sizes are platform dependent; you can set a fixed size with \c
    setIconSize(). You can alter the appearance of all tool buttons in
    the toolbars with \c setToolButtonStyle().

    An example of toolbar creation follows:

    \code
    void MainWindow::createToolBars()
    {
        fileToolBar = addToolBar(tr("File"));
        fileToolBar->addAction(newAct);
    \endcode

    \section2 Creating Dock Widgets

    Dock widgets are implemented in the QDockWidget class. A dock
    widget is a window that can be docked into the main window.  You
    add dock widgets to a main window with \c addDockWidget().

    There are four dock widget areas as given by the
    Qt::DockWidgetArea enum: left, right, top, and bottom. You can
    specify which dock widget area that should occupy the corners
    where the areas overlap with \c setCorner(). By default
    each area can only contain one row (vertical or horizontal) of
    dock widgets, but if you enable nesting with \c
    setDockNestingEnabled(), dock widgets can be added in either
    direction.

    Two dock widgets may also be stacked on top of each other. A
    QTabBar is then used to select which of the widgets should be
    displayed.

    We give an example of how to create and add dock widgets to a
    main window:

    \snippet mainwindowsnippet.cpp 0

    \section2 The Status Bar

    You can set a status bar with \c setStatusBar(), but one is
    created the first time \c statusBar() (which returns the main
    window's status bar) is called. See QStatusBar for information on
    how to use it.

    \section1 Storing State

    QMainWindow can store the state of its layout with \c
    saveState(); it can later be retrieved with \c restoreState(). It
    is the position and size (relative to the size of the main window)
    of the toolbars and dock widgets that are stored.

    \sa QMenuBar, QToolBar, QStatusBar, QDockWidget, {Application
    Example}, {Dock Widgets Example}, {MDI Example}, {SDI Example},
    {Menus Example}
*/

/*!
    \fn void QMainWindow::iconSizeChanged(const QSize &iconSize)

    This signal is emitted when the size of the icons used in the
    window is changed. The new icon size is passed in \a iconSize.

    You can connect this signal to other components to help maintain
    a consistent appearance for your application.

    \sa setIconSize()
*/

/*!
    \fn void QMainWindow::toolButtonStyleChanged(Qt::ToolButtonStyle toolButtonStyle)

    This signal is emitted when the style used for tool buttons in the
    window is changed. The new style is passed in \a toolButtonStyle.

    You can connect this signal to other components to help maintain
    a consistent appearance for your application.

    \sa setToolButtonStyle()
*/

#if QT_CONFIG(dockwidget)
/*!
    \fn void QMainWindow::tabifiedDockWidgetActivated(QDockWidget *dockWidget)

    This signal is emitted when the tabified dock widget is activated by
    selecting the tab. The activated dock widget is passed in \a dockWidget.

    \since 5.8
    \sa tabifyDockWidget(), tabifiedDockWidgets()
*/
#endif

/*!
    Constructs a QMainWindow with the given \a parent and the specified
    widget \a flags.

    QMainWindow sets the Qt::Window flag itself, and will hence
    always be created as a top-level widget.
 */
QMainWindow::QMainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QWidget(*(new QMainWindowPrivate()), parent, flags | Qt::Window)
{
    d_func()->init();
}


/*!
    Destroys the main window.
 */
QMainWindow::~QMainWindow()
{ }

/*! \property QMainWindow::iconSize
    \brief size of toolbar icons in this mainwindow.

    The default is the default tool bar icon size of the GUI style.
    Note that the icons used must be at least of this size as the
    icons are only scaled down.
*/

/*!
    \property QMainWindow::dockOptions
    \brief the docking behavior of QMainWindow
    \since 4.3

    The default value is AnimatedDocks | AllowTabbedDocks.
*/

/*!
    \enum QMainWindow::DockOption
    \since 4.3

    This enum contains flags that specify the docking behavior of QMainWindow.

    \value AnimatedDocks    Identical to the \l animated property.

    \value AllowNestedDocks Identical to the \l dockNestingEnabled property.

    \value AllowTabbedDocks The user can drop one dock widget "on top" of
                            another. The two widgets are stacked and a tab
                            bar appears for selecting which one is visible.

    \value ForceTabbedDocks Each dock area contains a single stack of tabbed
                            dock widgets. In other words, dock widgets cannot
                            be placed next to each other in a dock area. If
                            this option is set, AllowNestedDocks has no effect.

    \value VerticalTabs     The two vertical dock areas on the sides of the
                            main window show their tabs vertically. If this
                            option is not set, all dock areas show their tabs
                            at the bottom. Implies AllowTabbedDocks. See also
                            \l setTabPosition().

    \value GroupedDragging  When dragging the titlebar of a dock, all the tabs
                            that are tabbed with it are going to be dragged.
                            Implies AllowTabbedDocks. Does not work well if
                            some QDockWidgets have restrictions in which area
                            they are allowed. (This enum value was added in Qt
                            5.6.)

    These options only control how dock widgets may be dropped in a QMainWindow.
    They do not re-arrange the dock widgets to conform with the specified
    options. For this reason they should be set before any dock widgets
    are added to the main window. Exceptions to this are the AnimatedDocks and
    VerticalTabs options, which may be set at any time.
*/

void QMainWindow::setDockOptions(DockOptions opt)
{
    Q_D(QMainWindow);
    d->layout->setDockOptions(opt);
}

QMainWindow::DockOptions QMainWindow::dockOptions() const
{
    Q_D(const QMainWindow);
    return d->layout->dockOptions;
}

QSize QMainWindow::iconSize() const
{ return d_func()->iconSize; }

void QMainWindow::setIconSize(const QSize &iconSize)
{
    Q_D(QMainWindow);
    QSize sz = iconSize;
    if (!sz.isValid()) {
        const int metric = style()->pixelMetric(QStyle::PM_ToolBarIconSize, 0, this);
        sz = QSize(metric, metric);
    }
    if (d->iconSize != sz) {
        d->iconSize = sz;
        emit iconSizeChanged(d->iconSize);
    }
    d->explicitIconSize = iconSize.isValid();
}

/*! \property QMainWindow::toolButtonStyle
    \brief style of toolbar buttons in this mainwindow.

    To have the style of toolbuttons follow the system settings, set this property to Qt::ToolButtonFollowStyle.
    On Unix, the user settings from the desktop environment will be used.
    On other platforms, Qt::ToolButtonFollowStyle means icon only.

    The default is Qt::ToolButtonIconOnly.
*/

Qt::ToolButtonStyle QMainWindow::toolButtonStyle() const
{ return d_func()->toolButtonStyle; }

void QMainWindow::setToolButtonStyle(Qt::ToolButtonStyle toolButtonStyle)
{
    Q_D(QMainWindow);
    if (d->toolButtonStyle == toolButtonStyle)
        return;
    d->toolButtonStyle = toolButtonStyle;
    emit toolButtonStyleChanged(d->toolButtonStyle);
}

#if QT_CONFIG(menubar)
/*!
    Returns the menu bar for the main window. This function creates
    and returns an empty menu bar if the menu bar does not exist.

    If you want all windows in a Mac application to share one menu
    bar, don't use this function to create it, because the menu bar
    created here will have this QMainWindow as its parent.  Instead,
    you must create a menu bar that does not have a parent, which you
    can then share among all the Mac windows. Create a parent-less
    menu bar this way:

    \snippet code/src_gui_widgets_qmenubar.cpp 1

    \sa setMenuBar()
*/
QMenuBar *QMainWindow::menuBar() const
{
    QMenuBar *menuBar = qobject_cast<QMenuBar *>(layout()->menuBar());
    if (!menuBar) {
        QMainWindow *self = const_cast<QMainWindow *>(this);
        menuBar = new QMenuBar(self);
        self->setMenuBar(menuBar);
    }
    return menuBar;
}

/*!
    Sets the menu bar for the main window to \a menuBar.

    Note: QMainWindow takes ownership of the \a menuBar pointer and
    deletes it at the appropriate time.

    \sa menuBar()
*/
void QMainWindow::setMenuBar(QMenuBar *menuBar)
{
    QLayout *topLayout = layout();

    if (topLayout->menuBar() && topLayout->menuBar() != menuBar) {
        // Reparent corner widgets before we delete the old menu bar.
        QMenuBar *oldMenuBar = qobject_cast<QMenuBar *>(topLayout->menuBar());
        if (menuBar) {
            // TopLeftCorner widget.
            QWidget *cornerWidget = oldMenuBar->cornerWidget(Qt::TopLeftCorner);
            if (cornerWidget)
                menuBar->setCornerWidget(cornerWidget, Qt::TopLeftCorner);
            // TopRightCorner widget.
            cornerWidget = oldMenuBar->cornerWidget(Qt::TopRightCorner);
            if (cornerWidget)
                menuBar->setCornerWidget(cornerWidget, Qt::TopRightCorner);
        }
        oldMenuBar->hide();
        oldMenuBar->setParent(nullptr);
        oldMenuBar->deleteLater();
    }
    topLayout->setMenuBar(menuBar);
}

/*!
    \since 4.2

    Returns the menu bar for the main window. This function returns
    null if a menu bar hasn't been constructed yet.
*/
QWidget *QMainWindow::menuWidget() const
{
    QWidget *menuBar = d_func()->layout->menuBar();
    return menuBar;
}

/*!
    \since 4.2

    Sets the menu bar for the main window to \a menuBar.

    QMainWindow takes ownership of the \a menuBar pointer and
    deletes it at the appropriate time.
*/
void QMainWindow::setMenuWidget(QWidget *menuBar)
{
    Q_D(QMainWindow);
    if (d->layout->menuBar() && d->layout->menuBar() != menuBar) {
        d->layout->menuBar()->hide();
        d->layout->menuBar()->deleteLater();
    }
    d->layout->setMenuBar(menuBar);
}
#endif // QT_CONFIG(menubar)

#if QT_CONFIG(statusbar)
/*!
    Returns the status bar for the main window. This function creates
    and returns an empty status bar if the status bar does not exist.

    \sa setStatusBar()
*/
QStatusBar *QMainWindow::statusBar() const
{
    QStatusBar *statusbar = d_func()->layout->statusBar();
    if (!statusbar) {
        QMainWindow *self = const_cast<QMainWindow *>(this);
        statusbar = new QStatusBar(self);
        statusbar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        self->setStatusBar(statusbar);
    }
    return statusbar;
}

/*!
    Sets the status bar for the main window to \a statusbar.

    Setting the status bar to 0 will remove it from the main window.
    Note that QMainWindow takes ownership of the \a statusbar pointer
    and deletes it at the appropriate time.

    \sa statusBar()
*/
void QMainWindow::setStatusBar(QStatusBar *statusbar)
{
    Q_D(QMainWindow);
    if (d->layout->statusBar() && d->layout->statusBar() != statusbar) {
        d->layout->statusBar()->hide();
        d->layout->statusBar()->deleteLater();
    }
    d->layout->setStatusBar(statusbar);
}
#endif // QT_CONFIG(statusbar)

/*!
    Returns the central widget for the main window. This function
    returns zero if the central widget has not been set.

    \sa setCentralWidget()
*/
QWidget *QMainWindow::centralWidget() const
{ return d_func()->layout->centralWidget(); }

/*!
    Sets the given \a widget to be the main window's central widget.

    Note: QMainWindow takes ownership of the \a widget pointer and
    deletes it at the appropriate time.

    \sa centralWidget()
*/
void QMainWindow::setCentralWidget(QWidget *widget)
{
    Q_D(QMainWindow);
    if (d->layout->centralWidget() && d->layout->centralWidget() != widget) {
        d->layout->centralWidget()->hide();
        d->layout->centralWidget()->deleteLater();
    }
    d->layout->setCentralWidget(widget);
}

/*!
    Removes the central widget from this main window.

    The ownership of the removed widget is passed to the caller.

    \since 5.2
*/
QWidget *QMainWindow::takeCentralWidget()
{
    Q_D(QMainWindow);
    QWidget *oldcentralwidget = d->layout->centralWidget();
    if (oldcentralwidget) {
        oldcentralwidget->setParent(0);
        d->layout->setCentralWidget(0);
    }
    return oldcentralwidget;
}

#if QT_CONFIG(dockwidget)
/*!
    Sets the given dock widget \a area to occupy the specified \a
    corner.

    \sa corner()
*/
void QMainWindow::setCorner(Qt::Corner corner, Qt::DockWidgetArea area)
{
    bool valid = false;
    switch (corner) {
    case Qt::TopLeftCorner:
        valid = (area == Qt::TopDockWidgetArea || area == Qt::LeftDockWidgetArea);
        break;
    case Qt::TopRightCorner:
        valid = (area == Qt::TopDockWidgetArea || area == Qt::RightDockWidgetArea);
        break;
    case Qt::BottomLeftCorner:
        valid = (area == Qt::BottomDockWidgetArea || area == Qt::LeftDockWidgetArea);
        break;
    case Qt::BottomRightCorner:
        valid = (area == Qt::BottomDockWidgetArea || area == Qt::RightDockWidgetArea);
        break;
    }
    if (Q_UNLIKELY(!valid))
        qWarning("QMainWindow::setCorner(): 'area' is not valid for 'corner'");
    else
        d_func()->layout->setCorner(corner, area);
}

/*!
    Returns the dock widget area that occupies the specified \a
    corner.

    \sa setCorner()
*/
Qt::DockWidgetArea QMainWindow::corner(Qt::Corner corner) const
{ return d_func()->layout->corner(corner); }
#endif

#if QT_CONFIG(toolbar)

static bool checkToolBarArea(Qt::ToolBarArea area, const char *where)
{
    switch (area) {
    case Qt::LeftToolBarArea:
    case Qt::RightToolBarArea:
    case Qt::TopToolBarArea:
    case Qt::BottomToolBarArea:
        return true;
    default:
        break;
    }
    qWarning("%s: invalid 'area' argument", where);
    return false;
}

/*!
    Adds a toolbar break to the given \a area after all the other
    objects that are present.
*/
void QMainWindow::addToolBarBreak(Qt::ToolBarArea area)
{
    if (!checkToolBarArea(area, "QMainWindow::addToolBarBreak"))
        return;
    d_func()->layout->addToolBarBreak(area);
}

/*!
    Inserts a toolbar break before the toolbar specified by \a before.
*/
void QMainWindow::insertToolBarBreak(QToolBar *before)
{ d_func()->layout->insertToolBarBreak(before); }

/*!
    Removes a toolbar break previously inserted before the toolbar specified by \a before.
*/

void QMainWindow::removeToolBarBreak(QToolBar *before)
{
    Q_D(QMainWindow);
    d->layout->removeToolBarBreak(before);
}

/*!
    Adds the \a toolbar into the specified \a area in this main
    window. The \a toolbar is placed at the end of the current tool
    bar block (i.e. line). If the main window already manages \a toolbar
    then it will only move the toolbar to \a area.

    \sa insertToolBar(), addToolBarBreak(), insertToolBarBreak()
*/
void QMainWindow::addToolBar(Qt::ToolBarArea area, QToolBar *toolbar)
{
    if (!checkToolBarArea(area, "QMainWindow::addToolBar"))
        return;

    Q_D(QMainWindow);

    disconnect(this, SIGNAL(iconSizeChanged(QSize)),
               toolbar, SLOT(_q_updateIconSize(QSize)));
    disconnect(this, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
               toolbar, SLOT(_q_updateToolButtonStyle(Qt::ToolButtonStyle)));

    if(toolbar->d_func()->state && toolbar->d_func()->state->dragging) {
        //removing a toolbar which is dragging will cause crash
#if QT_CONFIG(dockwidget)
        bool animated = isAnimated();
        setAnimated(false);
#endif
        toolbar->d_func()->endDrag();
#if QT_CONFIG(dockwidget)
        setAnimated(animated);
#endif
    }

    d->layout->removeToolBar(toolbar);

    toolbar->d_func()->_q_updateIconSize(d->iconSize);
    toolbar->d_func()->_q_updateToolButtonStyle(d->toolButtonStyle);
    connect(this, SIGNAL(iconSizeChanged(QSize)),
            toolbar, SLOT(_q_updateIconSize(QSize)));
    connect(this, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
            toolbar, SLOT(_q_updateToolButtonStyle(Qt::ToolButtonStyle)));

    d->layout->addToolBar(area, toolbar);
}

/*! \overload
    Equivalent of calling addToolBar(Qt::TopToolBarArea, \a toolbar)
*/
void QMainWindow::addToolBar(QToolBar *toolbar)
{ addToolBar(Qt::TopToolBarArea, toolbar); }

/*!
    \overload

    Creates a QToolBar object, setting its window title to \a title,
    and inserts it into the top toolbar area.

    \sa setWindowTitle()
*/
QToolBar *QMainWindow::addToolBar(const QString &title)
{
    QToolBar *toolBar = new QToolBar(this);
    toolBar->setWindowTitle(title);
    addToolBar(toolBar);
    return toolBar;
}

/*!
    Inserts the \a toolbar into the area occupied by the \a before toolbar
    so that it appears before it. For example, in normal left-to-right
    layout operation, this means that \a toolbar will appear to the left
    of the toolbar specified by \a before in a horizontal toolbar area.

    \sa insertToolBarBreak(), addToolBar(), addToolBarBreak()
*/
void QMainWindow::insertToolBar(QToolBar *before, QToolBar *toolbar)
{
    Q_D(QMainWindow);

    d->layout->removeToolBar(toolbar);

    toolbar->d_func()->_q_updateIconSize(d->iconSize);
    toolbar->d_func()->_q_updateToolButtonStyle(d->toolButtonStyle);
    connect(this, SIGNAL(iconSizeChanged(QSize)),
            toolbar, SLOT(_q_updateIconSize(QSize)));
    connect(this, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
            toolbar, SLOT(_q_updateToolButtonStyle(Qt::ToolButtonStyle)));

    d->layout->insertToolBar(before, toolbar);
}

/*!
    Removes the \a toolbar from the main window layout and hides
    it. Note that the \a toolbar is \e not deleted.
*/
void QMainWindow::removeToolBar(QToolBar *toolbar)
{
    if (toolbar) {
        d_func()->layout->removeToolBar(toolbar);
        toolbar->hide();
    }
}

/*!
    Returns the Qt::ToolBarArea for \a toolbar. If \a toolbar has not
    been added to the main window, this function returns \c
    Qt::NoToolBarArea.

    \sa addToolBar(), addToolBarBreak(), Qt::ToolBarArea
*/
Qt::ToolBarArea QMainWindow::toolBarArea(QToolBar *toolbar) const
{ return d_func()->layout->toolBarArea(toolbar); }

/*!

    Returns whether there is a toolbar
    break before the \a toolbar.

    \sa addToolBarBreak(), insertToolBarBreak()
*/
bool QMainWindow::toolBarBreak(QToolBar *toolbar) const
{
    return d_func()->layout->toolBarBreak(toolbar);
}

#endif // QT_CONFIG(toolbar)

#if QT_CONFIG(dockwidget)

/*! \property QMainWindow::animated
    \brief whether manipulating dock widgets and tool bars is animated
    \since 4.2

    When a dock widget or tool bar is dragged over the
    main window, the main window adjusts its contents
    to indicate where the dock widget or tool bar will
    be docked if it is dropped. Setting this property
    causes QMainWindow to move its contents in a smooth
    animation. Clearing this property causes the contents
    to snap into their new positions.

    By default, this property is set. It may be cleared if
    the main window contains widgets which are slow at resizing
    or repainting themselves.

    Setting this property is identical to setting the AnimatedDocks
    option using setDockOptions().
*/

bool QMainWindow::isAnimated() const
{
    Q_D(const QMainWindow);
    return d->layout->dockOptions & AnimatedDocks;
}

void QMainWindow::setAnimated(bool enabled)
{
    Q_D(QMainWindow);

    DockOptions opts = d->layout->dockOptions;
    opts.setFlag(AnimatedDocks, enabled);

    d->layout->setDockOptions(opts);
}

/*! \property QMainWindow::dockNestingEnabled
    \brief whether docks can be nested
    \since 4.2

    If this property is \c false, dock areas can only contain a single row
    (horizontal or vertical) of dock widgets. If this property is \c true,
    the area occupied by a dock widget can be split in either direction to contain
    more dock widgets.

    Dock nesting is only necessary in applications that contain a lot of
    dock widgets. It gives the user greater freedom in organizing their
    main window. However, dock nesting leads to more complex
    (and less intuitive) behavior when a dock widget is dragged over the
    main window, since there are more ways in which a dropped dock widget
    may be placed in the dock area.

    Setting this property is identical to setting the AllowNestedDocks option
    using setDockOptions().
*/

bool QMainWindow::isDockNestingEnabled() const
{
    Q_D(const QMainWindow);
    return d->layout->dockOptions & AllowNestedDocks;
}

void QMainWindow::setDockNestingEnabled(bool enabled)
{
    Q_D(QMainWindow);

    DockOptions opts = d->layout->dockOptions;
    opts.setFlag(AllowNestedDocks, enabled);

    d->layout->setDockOptions(opts);
}

#if 0
// If added back in, add the '!' to the qdoc comment marker as well.
/*
    \property QMainWindow::verticalTabsEnabled
    \brief whether left and right dock areas use vertical tabs
    \since 4.2

    If this property is set to false, dock areas containing tabbed dock widgets
    display horizontal tabs, simmilar to Visual Studio.

    If this property is set to true, then the right and left dock areas display vertical
    tabs, simmilar to KDevelop.

    This property should be set before any dock widgets are added to the main window.
*/

bool QMainWindow::verticalTabsEnabled() const
{
    return d_func()->layout->verticalTabsEnabled();
}

void QMainWindow::setVerticalTabsEnabled(bool enabled)
{
    d_func()->layout->setVerticalTabsEnabled(enabled);
}
#endif

static bool checkDockWidgetArea(Qt::DockWidgetArea area, const char *where)
{
    switch (area) {
    case Qt::LeftDockWidgetArea:
    case Qt::RightDockWidgetArea:
    case Qt::TopDockWidgetArea:
    case Qt::BottomDockWidgetArea:
        return true;
    default:
        break;
    }
    qWarning("%s: invalid 'area' argument", where);
    return false;
}

#if QT_CONFIG(tabbar)
/*!
    \property QMainWindow::documentMode
    \brief whether the tab bar for tabbed dockwidgets is set to document mode.
    \since 4.5

    The default is false.

    \sa QTabBar::documentMode
*/
bool QMainWindow::documentMode() const
{
    return d_func()->layout->documentMode();
}

void QMainWindow::setDocumentMode(bool enabled)
{
    d_func()->layout->setDocumentMode(enabled);
}
#endif // QT_CONFIG(tabbar)

#if QT_CONFIG(tabwidget)
/*!
    \property QMainWindow::tabShape
    \brief the tab shape used for tabbed dock widgets.
    \since 4.5

    The default is \l QTabWidget::Rounded.

    \sa setTabPosition()
*/
QTabWidget::TabShape QMainWindow::tabShape() const
{
    return d_func()->layout->tabShape();
}

void QMainWindow::setTabShape(QTabWidget::TabShape tabShape)
{
    d_func()->layout->setTabShape(tabShape);
}

/*!
    \since 4.5

    Returns the tab position for \a area.

    \note The \l VerticalTabs dock option overrides the tab positions returned
    by this function.

    \sa setTabPosition(), tabShape()
*/
QTabWidget::TabPosition QMainWindow::tabPosition(Qt::DockWidgetArea area) const
{
    if (!checkDockWidgetArea(area, "QMainWindow::tabPosition"))
        return QTabWidget::South;
    return d_func()->layout->tabPosition(area);
}

/*!
    \since 4.5

    Sets the tab position for the given dock widget \a areas to the specified
    \a tabPosition. By default, all dock areas show their tabs at the bottom.

    \note The \l VerticalTabs dock option overrides the tab positions set by
    this method.

    \sa tabPosition(), setTabShape()
*/
void QMainWindow::setTabPosition(Qt::DockWidgetAreas areas, QTabWidget::TabPosition tabPosition)
{
    d_func()->layout->setTabPosition(areas, tabPosition);
}
#endif // QT_CONFIG(tabwidget)

/*!
    Adds the given \a dockwidget to the specified \a area.
*/
void QMainWindow::addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget)
{
    if (!checkDockWidgetArea(area, "QMainWindow::addDockWidget"))
        return;

    Qt::Orientation orientation = Qt::Vertical;
    switch (area) {
    case Qt::TopDockWidgetArea:
    case Qt::BottomDockWidgetArea:
        orientation = Qt::Horizontal;
        break;
    default:
        break;
    }
    d_func()->layout->removeWidget(dockwidget); // in case it was already in here
    addDockWidget(area, dockwidget, orientation);
}

/*!
    Restores the state of \a dockwidget if it is created after the call
    to restoreState(). Returns \c true if the state was restored; otherwise
    returns \c false.

    \sa restoreState(), saveState()
*/

bool QMainWindow::restoreDockWidget(QDockWidget *dockwidget)
{
    return d_func()->layout->restoreDockWidget(dockwidget);
}

/*!
    Adds \a dockwidget into the given \a area in the direction
    specified by the \a orientation.
*/
void QMainWindow::addDockWidget(Qt::DockWidgetArea area, QDockWidget *dockwidget,
                                Qt::Orientation orientation)
{
    if (!checkDockWidgetArea(area, "QMainWindow::addDockWidget"))
        return;

    // add a window to an area, placing done relative to the previous
    d_func()->layout->addDockWidget(area, dockwidget, orientation);
}

/*!
    \fn void QMainWindow::splitDockWidget(QDockWidget *first, QDockWidget *second, Qt::Orientation orientation)

    Splits the space covered by the \a first dock widget into two parts,
    moves the \a first dock widget into the first part, and moves the
    \a second dock widget into the second part.

    The \a orientation specifies how the space is divided: A Qt::Horizontal
    split places the second dock widget to the right of the first; a
    Qt::Vertical split places the second dock widget below the first.

    \e Note: if \a first is currently in a tabbed docked area, \a second will
    be added as a new tab, not as a neighbor of \a first. This is because a
    single tab can contain only one dock widget.

    \e Note: The Qt::LayoutDirection influences the order of the dock widgets
    in the two parts of the divided area. When right-to-left layout direction
    is enabled, the placing of the dock widgets will be reversed.

    \sa tabifyDockWidget(), addDockWidget(), removeDockWidget()
*/
void QMainWindow::splitDockWidget(QDockWidget *after, QDockWidget *dockwidget,
                                  Qt::Orientation orientation)
{
    d_func()->layout->splitDockWidget(after, dockwidget, orientation);
}

/*!
    \fn void QMainWindow::tabifyDockWidget(QDockWidget *first, QDockWidget *second)

    Moves \a second dock widget on top of \a first dock widget, creating a tabbed
    docked area in the main window.

    \sa tabifiedDockWidgets()
*/
void QMainWindow::tabifyDockWidget(QDockWidget *first, QDockWidget *second)
{
    d_func()->layout->tabifyDockWidget(first, second);
}


/*!
    \fn QList<QDockWidget*> QMainWindow::tabifiedDockWidgets(QDockWidget *dockwidget) const

    Returns the dock widgets that are tabified together with \a dockwidget.

    \since 4.5
    \sa tabifyDockWidget()
*/

QList<QDockWidget*> QMainWindow::tabifiedDockWidgets(QDockWidget *dockwidget) const
{
    QList<QDockWidget*> ret;
#if !QT_CONFIG(tabbar)
    Q_UNUSED(dockwidget);
#else
    const QDockAreaLayoutInfo *info = d_func()->layout->layoutState.dockAreaLayout.info(dockwidget);
    if (info && info->tabbed && info->tabBar) {
        for(int i = 0; i < info->item_list.count(); ++i) {
            const QDockAreaLayoutItem &item = info->item_list.at(i);
            if (item.widgetItem) {
                if (QDockWidget *dock = qobject_cast<QDockWidget*>(item.widgetItem->widget())) {
                    if (dock != dockwidget) {
                        ret += dock;
                    }
                }
            }
        }
    }
#endif
    return ret;
}


/*!
    Removes the \a dockwidget from the main window layout and hides
    it. Note that the \a dockwidget is \e not deleted.
*/
void QMainWindow::removeDockWidget(QDockWidget *dockwidget)
{
    if (dockwidget) {
        d_func()->layout->removeWidget(dockwidget);
        dockwidget->hide();
    }
}

/*!
    Returns the Qt::DockWidgetArea for \a dockwidget. If \a dockwidget
    has not been added to the main window, this function returns \c
    Qt::NoDockWidgetArea.

    \sa addDockWidget(), splitDockWidget(), Qt::DockWidgetArea
*/
Qt::DockWidgetArea QMainWindow::dockWidgetArea(QDockWidget *dockwidget) const
{ return d_func()->layout->dockWidgetArea(dockwidget); }


/*!
    \since 5.6
    Resizes the dock widgets in the list \a docks to the corresponding size in
    pixels from the list \a sizes. If \a orientation is Qt::Horizontal, adjusts
    the width, otherwise adjusts the height of the dock widgets.
    The sizes will be adjusted such that the maximum and the minimum sizes are
    respected and the QMainWindow itself will not be resized.
    Any additional/missing space is distributed amongst the widgets according
    to the relative weight of the sizes.

    Example:
    \code
    resizeDocks({blueWidget, yellowWidget}, {20 , 40}, Qt::Horizontal);
    \endcode
    If the blue and the yellow widget are nested on the same level they will be
    resized such that the yellowWidget is twice as big as the blueWidget

    If some widgets are grouped in tabs, only one widget per group should be
    specified. Widgets not in the list might be changed to repect the constraints.
*/
void QMainWindow::resizeDocks(const QList<QDockWidget *> &docks,
                              const QList<int> &sizes, Qt::Orientation orientation)
{
    d_func()->layout->layoutState.dockAreaLayout.resizeDocks(docks, sizes, orientation);
    d_func()->layout->invalidate();
}


#endif // QT_CONFIG(dockwidget)

/*!
    Saves the current state of this mainwindow's toolbars and
    dockwidgets. This includes the corner settings which can
    be set with setCorner(). The \a version number is stored
    as part of the data.

    The \l{QObject::objectName}{objectName} property is used
    to identify each QToolBar and QDockWidget.  You should make sure
    that this property is unique for each QToolBar and QDockWidget you
    add to the QMainWindow

    To restore the saved state, pass the return value and \a version
    number to restoreState().

    To save the geometry when the window closes, you can
    implement a close event like this:

    \snippet code/src_gui_widgets_qmainwindow.cpp 0

    \sa restoreState(), QWidget::saveGeometry(), QWidget::restoreGeometry()
*/
QByteArray QMainWindow::saveState(int version) const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << QMainWindowLayout::VersionMarker;
    stream << version;
    d_func()->layout->saveState(stream);
    return data;
}

/*!
    Restores the \a state of this mainwindow's toolbars and
    dockwidgets. Also restores the corner settings too. The
    \a version number is compared with that stored in \a state.
    If they do not match, the mainwindow's state is left
    unchanged, and this function returns \c false; otherwise, the state
    is restored, and this function returns \c true.

    To restore geometry saved using QSettings, you can use code like
    this:

    \snippet code/src_gui_widgets_qmainwindow.cpp 1

    \sa saveState(), QWidget::saveGeometry(),
    QWidget::restoreGeometry(), restoreDockWidget()
*/
bool QMainWindow::restoreState(const QByteArray &state, int version)
{
    if (state.isEmpty())
        return false;
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    int marker, v;
    stream >> marker;
    stream >> v;
    if (stream.status() != QDataStream::Ok || marker != QMainWindowLayout::VersionMarker || v != version)
        return false;
    bool restored = d_func()->layout->restoreState(stream);
    return restored;
}

/*! \reimp */
bool QMainWindow::event(QEvent *event)
{
    Q_D(QMainWindow);

#if QT_CONFIG(dockwidget)
    if (d->layout && d->layout->windowEvent(event))
        return true;
#endif

    switch (event->type()) {

#if QT_CONFIG(toolbar)
        case QEvent::ToolBarChange: {
            d->layout->toggleToolBarsVisible();
            return true;
        }
#endif

#if QT_CONFIG(statustip)
        case QEvent::StatusTip:
#if QT_CONFIG(statusbar)
            if (QStatusBar *sb = d->layout->statusBar())
                sb->showMessage(static_cast<QStatusTipEvent*>(event)->tip());
            else
#endif
                static_cast<QStatusTipEvent*>(event)->ignore();
            return true;
#endif // QT_CONFIG(statustip)

        case QEvent::StyleChange:
#if QT_CONFIG(dockwidget)
            d->layout->layoutState.dockAreaLayout.styleChangedEvent();
#endif
            if (!d->explicitIconSize)
                setIconSize(QSize());
            break;
        default:
            break;
    }

    return QWidget::event(event);
}

#if QT_CONFIG(toolbar)

/*!
    \property QMainWindow::unifiedTitleAndToolBarOnMac
    \brief whether the window uses the unified title and toolbar look on \macos

    Note that the Qt 5 implementation has several limitations compared to Qt 4:
    \list
        \li Use in windows with OpenGL content is not supported. This includes QGLWidget and QOpenGLWidget.
        \li Using dockable or movable toolbars may result in painting errors and is not recommended
    \endlist

    \since 5.2
*/
void QMainWindow::setUnifiedTitleAndToolBarOnMac(bool set)
{
#ifdef Q_OS_OSX
    Q_D(QMainWindow);
    if (isWindow()) {
        d->useUnifiedToolBar = set;
        createWinId();

        QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
        QPlatformNativeInterface::NativeResourceForIntegrationFunction function =
            nativeInterface->nativeResourceFunctionForIntegration("setContentBorderEnabled");
        if (!function)
            return; // Not Cocoa platform plugin.

        typedef void (*SetContentBorderEnabledFunction)(QWindow *window, bool enable);
        (reinterpret_cast<SetContentBorderEnabledFunction>(function))(window()->windowHandle(), set);
        update();
    }
#else
    Q_UNUSED(set)
#endif
}

bool QMainWindow::unifiedTitleAndToolBarOnMac() const
{
#ifdef Q_OS_OSX
    return d_func()->useUnifiedToolBar;
#endif
    return false;
}

#endif // QT_CONFIG(toolbar)

/*!
    \internal
*/
bool QMainWindow::isSeparator(const QPoint &pos) const
{
#if QT_CONFIG(dockwidget)
    Q_D(const QMainWindow);
    return !d->layout->layoutState.dockAreaLayout.findSeparator(pos).isEmpty();
#else
    Q_UNUSED(pos);
    return false;
#endif
}

#ifndef QT_NO_CONTEXTMENU
/*!
    \reimp
*/
void QMainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    event->ignore();
    // only show the context menu for direct QDockWidget and QToolBar
    // children and for the menu bar as well
    QWidget *child = childAt(event->pos());
    while (child && child != this) {
#if QT_CONFIG(menubar)
        if (QMenuBar *mb = qobject_cast<QMenuBar *>(child)) {
            if (mb->parentWidget() != this)
                return;
            break;
        }
#endif
#if QT_CONFIG(dockwidget)
        if (QDockWidget *dw = qobject_cast<QDockWidget *>(child)) {
            if (dw->parentWidget() != this)
                return;
            if (dw->widget()
                && dw->widget()->geometry().contains(child->mapFrom(this, event->pos()))) {
                // ignore the event if the mouse is over the QDockWidget contents
                return;
            }
            break;
        }
#endif // QT_CONFIG(dockwidget)
#if QT_CONFIG(toolbar)
        if (QToolBar *tb = qobject_cast<QToolBar *>(child)) {
            if (tb->parentWidget() != this)
                return;
            break;
        }
#endif
        child = child->parentWidget();
    }
    if (child == this)
        return;

#if QT_CONFIG(menu)
    QMenu *popup = createPopupMenu();
    if (popup) {
        if (!popup->isEmpty()) {
            popup->setAttribute(Qt::WA_DeleteOnClose);
            popup->popup(event->globalPos());
            event->accept();
        } else {
            delete popup;
        }
    }
#endif
}
#endif // QT_NO_CONTEXTMENU

#if QT_CONFIG(menu)
/*!
    Returns a popup menu containing checkable entries for the toolbars and
    dock widgets present in the main window. If  there are no toolbars and
    dock widgets present, this function returns a null pointer.

    By default, this function is called by the main window when the user
    activates a context menu, typically by right-clicking on a toolbar or a dock
    widget.

    If you want to create a custom popup menu, reimplement this function and
    return a newly-created popup menu. Ownership of the popup menu is transferred
    to the caller.

    \sa addDockWidget(), addToolBar(), menuBar()
*/
QMenu *QMainWindow::createPopupMenu()
{
    Q_D(QMainWindow);
    QMenu *menu = 0;
#if QT_CONFIG(dockwidget)
    QList<QDockWidget *> dockwidgets = findChildren<QDockWidget *>();
    if (dockwidgets.size()) {
        menu = new QMenu(this);
        for (int i = 0; i < dockwidgets.size(); ++i) {
            QDockWidget *dockWidget = dockwidgets.at(i);
            // filter to find out if we own this QDockWidget
            if (dockWidget->parentWidget() == this) {
                if (d->layout->layoutState.dockAreaLayout.indexOf(dockWidget).isEmpty())
                    continue;
            } else if (QDockWidgetGroupWindow *dwgw =
                           qobject_cast<QDockWidgetGroupWindow *>(dockWidget->parentWidget())) {
                if (dwgw->parentWidget() != this)
                    continue;
                if (dwgw->layoutInfo()->indexOf(dockWidget).isEmpty())
                    continue;
            } else {
                continue;
            }
            menu->addAction(dockwidgets.at(i)->toggleViewAction());
        }
        menu->addSeparator();
    }
#endif // QT_CONFIG(dockwidget)
#if QT_CONFIG(toolbar)
    QList<QToolBar *> toolbars = findChildren<QToolBar *>();
    if (toolbars.size()) {
        if (!menu)
            menu = new QMenu(this);
        for (int i = 0; i < toolbars.size(); ++i) {
            QToolBar *toolBar = toolbars.at(i);
            if (toolBar->parentWidget() == this
                && (!d->layout->layoutState.toolBarAreaLayout.indexOf(toolBar).isEmpty())) {
                menu->addAction(toolbars.at(i)->toggleViewAction());
            }
        }
    }
#endif
    Q_UNUSED(d);
    return menu;
}
#endif // QT_CONFIG(menu)

QT_END_NAMESPACE

#include "moc_qmainwindow.cpp"
