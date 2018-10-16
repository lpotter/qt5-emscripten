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

#include <qmenubar.h>

#include <qstyle.h>
#include <qlayout.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
#ifndef QT_NO_ACCESSIBILITY
# include <qaccessible.h>
#endif
#include <qpainter.h>
#include <qstylepainter.h>
#include <qevent.h>
#if QT_CONFIG(mainwindow)
#include <qmainwindow.h>
#endif
#if QT_CONFIG(toolbar)
#include <qtoolbar.h>
#endif
#if QT_CONFIG(toolbutton)
#include <qtoolbutton.h>
#endif
#if QT_CONFIG(whatsthis)
#include <qwhatsthis.h>
#endif
#include <qpa/qplatformtheme.h>
#include "private/qguiapplication_p.h"
#include "qpa/qplatformintegration.h"
#include <private/qdesktopwidget_p.h>

#include "qmenu_p.h"
#include "qmenubar_p.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

class QMenuBarExtension : public QToolButton
{
public:
    explicit QMenuBarExtension(QWidget *parent);

    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *) override;
};

QMenuBarExtension::QMenuBarExtension(QWidget *parent)
    : QToolButton(parent)
{
    setObjectName(QLatin1String("qt_menubar_ext_button"));
    setAutoRaise(true);
#if QT_CONFIG(menu)
    setPopupMode(QToolButton::InstantPopup);
#endif
    setIcon(style()->standardIcon(QStyle::SP_ToolBarHorizontalExtensionButton, 0, parentWidget()));
}

void QMenuBarExtension::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    // We do not need to draw both extension arrows
    opt.features &= ~QStyleOptionToolButton::HasMenu;
    p.drawComplexControl(QStyle::CC_ToolButton, opt);
}


QSize QMenuBarExtension::sizeHint() const
{
    int ext = style()->pixelMetric(QStyle::PM_ToolBarExtensionExtent, 0, parentWidget());
    return QSize(ext, ext);
}


/*!
    \internal
*/
QAction *QMenuBarPrivate::actionAt(QPoint p) const
{
    for(int i = 0; i < actions.size(); ++i) {
        if(actionRect(actions.at(i)).contains(p))
            return actions.at(i);
    }
    return 0;
}

QRect QMenuBarPrivate::menuRect(bool extVisible) const
{
    Q_Q(const QMenuBar);

    int hmargin = q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
    QRect result = q->rect();
    result.adjust(hmargin, 0, -hmargin, 0);

    if (extVisible) {
        if (q->isRightToLeft())
            result.setLeft(result.left() + extension->sizeHint().width());
        else
            result.setWidth(result.width() - extension->sizeHint().width());
    }

    if (leftWidget && leftWidget->isVisible()) {
        QSize sz = leftWidget->sizeHint();
        if (q->isRightToLeft())
            result.setRight(result.right() - sz.width());
        else
            result.setLeft(result.left() + sz.width());
    }

    if (rightWidget && rightWidget->isVisible()) {
        QSize sz = rightWidget->sizeHint();
        if (q->isRightToLeft())
            result.setLeft(result.left() + sz.width());
        else
            result.setRight(result.right() - sz.width());
    }

    return result;
}

bool QMenuBarPrivate::isVisible(QAction *action)
{
    return !hiddenActions.contains(action);
}

void QMenuBarPrivate::updateGeometries()
{
    Q_Q(QMenuBar);
    if(!itemsDirty)
        return;
    int q_width = q->width()-(q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q)*2);
    int q_start = -1;
    if(leftWidget || rightWidget) {
        int vmargin = q->style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, q)
                      + q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
        int hmargin = q->style()->pixelMetric(QStyle::PM_MenuBarHMargin, 0, q)
                      + q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
        if (leftWidget && leftWidget->isVisible()) {
            QSize sz = leftWidget->sizeHint();
            q_width -= sz.width();
            q_start = sz.width();
            QPoint pos(hmargin, (q->height() - leftWidget->height()) / 2);
            QRect vRect = QStyle::visualRect(q->layoutDirection(), q->rect(), QRect(pos, sz));
            leftWidget->setGeometry(vRect);
        }
        if (rightWidget && rightWidget->isVisible()) {
            QSize sz = rightWidget->sizeHint();
            q_width -= sz.width();
            QPoint pos(q->width() - sz.width() - hmargin, vmargin);
            QRect vRect = QStyle::visualRect(q->layoutDirection(), q->rect(), QRect(pos, sz));
            rightWidget->setGeometry(vRect);
        }
    }

#ifdef Q_OS_MAC
    if(q->isNativeMenuBar()) {//nothing to see here folks, move along..
        itemsDirty = false;
        return;
    }
#endif
    calcActionRects(q_width, q_start);
    currentAction = 0;
#ifndef QT_NO_SHORTCUT
    if(itemsDirty) {
        for(int j = 0; j < shortcutIndexMap.size(); ++j)
            q->releaseShortcut(shortcutIndexMap.value(j));
        shortcutIndexMap.clear();
        const int actionsCount = actions.count();
        shortcutIndexMap.reserve(actionsCount);
        for (int i = 0; i < actionsCount; i++)
            shortcutIndexMap.append(q->grabShortcut(QKeySequence::mnemonic(actions.at(i)->text())));
    }
#endif
    itemsDirty = false;

    hiddenActions.clear();
    //this is the menu rectangle without any extension
    QRect menuRect = this->menuRect(false);

    //we try to see if the actions will fit there
    bool hasHiddenActions = false;
    for (int i = 0; i < actions.count(); ++i) {
        const QRect &rect = actionRects.at(i);
        if (rect.isValid() && !menuRect.contains(rect)) {
            hasHiddenActions = true;
            break;
        }
    }

    //...and if not, determine the ones that fit on the menu with the extension visible
    if (hasHiddenActions) {
        menuRect = this->menuRect(true);
        for (int i = 0; i < actions.count(); ++i) {
            const QRect &rect = actionRects.at(i);
            if (rect.isValid() && !menuRect.contains(rect)) {
                hiddenActions.append(actions.at(i));
            }
        }
    }

    if (hiddenActions.count() > 0) {
        QMenu *pop = extension->menu();
        if (!pop) {
            pop = new QMenu(q);
            extension->setMenu(pop);
        }
        pop->clear();
        pop->addActions(hiddenActions);

        int vmargin = q->style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, q);
        int x = q->isRightToLeft()
                ? menuRect.left() - extension->sizeHint().width() + 1
                : menuRect.right();
        extension->setGeometry(x, vmargin, extension->sizeHint().width(), menuRect.height() - vmargin*2);
        extension->show();
    } else {
        extension->hide();
    }
    q->updateGeometry();
}

QRect QMenuBarPrivate::actionRect(QAction *act) const
{
    const int index = actions.indexOf(act);

    //makes sure the geometries are up-to-date
    const_cast<QMenuBarPrivate*>(this)->updateGeometries();

    if (index < 0 || index >= actionRects.count())
        return QRect(); // that can happen in case of native menubar

    return actionRects.at(index);
}

void QMenuBarPrivate::focusFirstAction()
{
    if(!currentAction) {
        updateGeometries();
        int index = 0;
        while (index < actions.count() && actionRects.at(index).isNull()) ++index;
        if (index < actions.count())
            setCurrentAction(actions.at(index));
    }
}

void QMenuBarPrivate::setKeyboardMode(bool b)
{
    Q_Q(QMenuBar);
    if (b && !q->style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation, 0, q)) {
        setCurrentAction(0);
        return;
    }
    keyboardState = b;
    if(b) {
        QWidget *fw = QApplication::focusWidget();
        if (fw && fw != q && fw->window() != QApplication::activePopupWidget())
            keyboardFocusWidget = fw;
        focusFirstAction();
        q->setFocus(Qt::MenuBarFocusReason);
    } else {
        if(!popupState)
            setCurrentAction(0);
        if(keyboardFocusWidget) {
            if (QApplication::focusWidget() == q)
                keyboardFocusWidget->setFocus(Qt::MenuBarFocusReason);
            keyboardFocusWidget = 0;
        }
    }
    q->update();
}

void QMenuBarPrivate::popupAction(QAction *action, bool activateFirst)
{
    Q_Q(QMenuBar);
    if(!action || !action->menu() || closePopupMode)
        return;
    popupState = true;
    if (action->isEnabled() && action->menu()->isEnabled()) {
        closePopupMode = 0;
        activeMenu = action->menu();
        activeMenu->d_func()->causedPopup.widget = q;
        activeMenu->d_func()->causedPopup.action = action;

        QRect adjustedActionRect = actionRect(action);
        QPoint pos(q->mapToGlobal(QPoint(adjustedActionRect.left(), adjustedActionRect.bottom() + 1)));
        QSize popup_size = activeMenu->sizeHint();

        //we put the popup menu on the screen containing the bottom-center of the action rect
        QRect screenRect = QDesktopWidgetPrivate::screenGeometry(pos + QPoint(adjustedActionRect.width() / 2, 0));
        pos = QPoint(qMax(pos.x(), screenRect.x()), qMax(pos.y(), screenRect.y()));

        const bool fitUp = (pos.y() - popup_size.height() >= screenRect.top());
        const bool fitDown = (pos.y() + popup_size.height() <= screenRect.bottom());
        const bool rtl = q->isRightToLeft();
        const int actionWidth = adjustedActionRect.width();

        if (!fitUp && !fitDown) { //we should shift the menu
            bool shouldShiftToRight = !rtl;
            if (rtl && popup_size.width() > pos.x())
                shouldShiftToRight = true;
            else if (actionWidth + popup_size.width() + pos.x() > screenRect.right())
                shouldShiftToRight = false;

            if (shouldShiftToRight) {
                pos.rx() += actionWidth + (rtl ? popup_size.width() : 0);
            } else {
                //shift to left
                if (!rtl)
                    pos.rx() -= popup_size.width();
            }
        } else if (rtl) {
            pos.rx() += actionWidth;
        }

        if(!defaultPopDown || (fitUp && !fitDown))
            pos.setY(qMax(screenRect.y(), q->mapToGlobal(QPoint(0, adjustedActionRect.top()-popup_size.height())).y()));
        activeMenu->popup(pos);
        if(activateFirst)
            activeMenu->d_func()->setFirstActionActive();
    }
    q->update(actionRect(action));
}

void QMenuBarPrivate::setCurrentAction(QAction *action, bool popup, bool activateFirst)
{
    if(currentAction == action && popup == popupState)
        return;

    autoReleaseTimer.stop();

    doChildEffects = (popup && !activeMenu);
    Q_Q(QMenuBar);
    QWidget *fw = 0;
    if(QMenu *menu = activeMenu) {
        activeMenu = 0;
        if (popup) {
            fw = q->window()->focusWidget();
            q->setFocus(Qt::NoFocusReason);
        }
        menu->hide();
    }

    if(currentAction)
        q->update(actionRect(currentAction));

    popupState = popup;
#if QT_CONFIG(statustip)
    QAction *previousAction = currentAction;
#endif
    currentAction = action;
    if (action && action->isEnabled()) {
        activateAction(action, QAction::Hover);
        if(popup)
            popupAction(action, activateFirst);
        q->update(actionRect(action));
#if QT_CONFIG(statustip)
    }  else if (previousAction) {
        QString empty;
        QStatusTipEvent tip(empty);
        QApplication::sendEvent(q, &tip);
#endif
    }
    if (fw)
        fw->setFocus(Qt::NoFocusReason);
}

void QMenuBarPrivate::calcActionRects(int max_width, int start) const
{
    Q_Q(const QMenuBar);

    if(!itemsDirty)
        return;

    //let's reinitialize the buffer
    actionRects.resize(actions.count());
    actionRects.fill(QRect());

    const QStyle *style = q->style();

    const int itemSpacing = style->pixelMetric(QStyle::PM_MenuBarItemSpacing, 0, q);
    int max_item_height = 0, separator = -1, separator_start = 0, separator_len = 0;

    //calculate size
    const QFontMetrics fm = q->fontMetrics();
    const int hmargin = style->pixelMetric(QStyle::PM_MenuBarHMargin, 0, q),
              vmargin = style->pixelMetric(QStyle::PM_MenuBarVMargin, 0, q),
                icone = style->pixelMetric(QStyle::PM_SmallIconSize, 0, q);
    for(int i = 0; i < actions.count(); i++) {
        QAction *action = actions.at(i);
        if(!action->isVisible())
            continue;

        QSize sz;

        //calc what I think the size is..
        if(action->isSeparator()) {
            if (style->styleHint(QStyle::SH_DrawMenuBarSeparator, 0, q))
                separator = i;
            continue; //we don't really position these!
        } else {
            const QString s = action->text();
            QIcon is = action->icon();
            // If an icon is set, only the icon is visible
            if (!is.isNull())
                sz = sz.expandedTo(QSize(icone, icone));
            else if (!s.isEmpty())
                sz = fm.size(Qt::TextShowMnemonic, s);
        }

        //let the style modify the above size..
        QStyleOptionMenuItem opt;
        q->initStyleOption(&opt, action);
        sz = q->style()->sizeFromContents(QStyle::CT_MenuBarItem, &opt, sz, q);

        if(!sz.isEmpty()) {
            { //update the separator state
                int iWidth = sz.width() + itemSpacing;
                if(separator == -1)
                    separator_start += iWidth;
                else
                    separator_len += iWidth;
            }
            //maximum height
            max_item_height = qMax(max_item_height, sz.height());
            //append
            actionRects[i] = QRect(0, 0, sz.width(), sz.height());
        }
    }

    //calculate position
    const int fw = q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
    int x = fw + ((start == -1) ? hmargin : start) + itemSpacing;
    int y = fw + vmargin;
    for(int i = 0; i < actions.count(); i++) {
        QRect &rect = actionRects[i];
        if (rect.isNull())
            continue;

        //resize
        rect.setHeight(max_item_height);

        //move
        if(separator != -1 && i >= separator) { //after the separator
            int left = (max_width - separator_len - hmargin - itemSpacing) + (x - separator_start - hmargin);
            if(left < separator_start) { //wrap
                separator_start = x = hmargin;
                y += max_item_height;
            }
            rect.moveLeft(left);
        } else {
            rect.moveLeft(x);
        }
        rect.moveTop(y);

        //keep moving along..
        x += rect.width() + itemSpacing;

        //make sure we follow the layout direction
        rect = QStyle::visualRect(q->layoutDirection(), q->rect(), rect);
    }
}

void QMenuBarPrivate::activateAction(QAction *action, QAction::ActionEvent action_e)
{
    Q_Q(QMenuBar);
    if (!action || !action->isEnabled())
        return;
    action->activate(action_e);
    if (action_e == QAction::Hover)
        action->showStatusText(q);

//     if(action_e == QAction::Trigger)
//         emit q->activated(action);
//     else if(action_e == QAction::Hover)
//         emit q->highlighted(action);
}


void QMenuBarPrivate::_q_actionTriggered()
{
    Q_Q(QMenuBar);
    if (QAction *action = qobject_cast<QAction *>(q->sender())) {
        emit q->triggered(action);
    }
}

void QMenuBarPrivate::_q_actionHovered()
{
    Q_Q(QMenuBar);
    if (QAction *action = qobject_cast<QAction *>(q->sender())) {
        emit q->hovered(action);
#ifndef QT_NO_ACCESSIBILITY
        if (QAccessible::isActive()) {
            int actionIndex = actions.indexOf(action);
            QAccessibleEvent focusEvent(q, QAccessible::Focus);
            focusEvent.setChild(actionIndex);
            QAccessible::updateAccessibility(&focusEvent);
        }
#endif //QT_NO_ACCESSIBILITY
    }
}

/*!
    Initialize \a option with the values from the menu bar and information from \a action. This method
    is useful for subclasses when they need a QStyleOptionMenuItem, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom(), QMenu::initStyleOption()
*/
void QMenuBar::initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const
{
    if (!option || !action)
        return;
    Q_D(const QMenuBar);
    option->palette = palette();
    option->state = QStyle::State_None;
    if (isEnabled() && action->isEnabled())
        option->state |= QStyle::State_Enabled;
    else
        option->palette.setCurrentColorGroup(QPalette::Disabled);
    option->fontMetrics = fontMetrics();
    if (d->currentAction && d->currentAction == action) {
        option->state |= QStyle::State_Selected;
        if (d->popupState && !d->closePopupMode)
            option->state |= QStyle::State_Sunken;
    }
    if (hasFocus() || d->currentAction)
        option->state |= QStyle::State_HasFocus;
    option->menuRect = rect();
    option->menuItemType = QStyleOptionMenuItem::Normal;
    option->checkType = QStyleOptionMenuItem::NotCheckable;
    option->text = action->text();
    option->icon = action->icon();
}

/*!
    \class QMenuBar
    \brief The QMenuBar class provides a horizontal menu bar.

    \ingroup mainwindow-classes
    \inmodule QtWidgets

    A menu bar consists of a list of pull-down menu items. You add
    menu items with addMenu(). For example, asuming that \c menubar
    is a pointer to a QMenuBar and \c fileMenu is a pointer to a
    QMenu, the following statement inserts the menu into the menu bar:
    \snippet code/src_gui_widgets_qmenubar.cpp 0

    The ampersand in the menu item's text sets Alt+F as a shortcut for
    this menu. (You can use "\&\&" to get a real ampersand in the menu
    bar.)

    There is no need to lay out a menu bar. It automatically sets its
    own geometry to the top of the parent widget and changes it
    appropriately whenever the parent is resized.

    \section1 Usage

    In most main window style applications you would use the
    \l{QMainWindow::}{menuBar()} function provided in QMainWindow,
    adding \l{QMenu}s to the menu bar and adding \l{QAction}s to the
    pop-up menus.

    Example (from the \l{mainwindows/menus}{Menus} example):

    \snippet mainwindows/menus/mainwindow.cpp 9

    Menu items may be removed with removeAction().

    Widgets can be added to menus by using instances of the QWidgetAction
    class to hold them. These actions can then be inserted into menus
    in the usual way; see the QMenu documentation for more details.

    \section1 Platform Dependent Look and Feel

    Different platforms have different requirements for the appearance
    of menu bars and their behavior when the user interacts with them.
    For example, Windows systems are often configured so that the
    underlined character mnemonics that indicate keyboard shortcuts
    for items in the menu bar are only shown when the \uicontrol{Alt} key is
    pressed.

    \section1 QMenuBar as a Global Menu Bar

    On \macos and on certain Linux desktop environments such as
    Ubuntu Unity, QMenuBar is a wrapper for using the system-wide menu bar.
    If you have multiple menu bars in one dialog the outermost menu bar
    (normally inside a widget with widget flag Qt::Window) will
    be used for the system-wide menu bar.

    Qt for \macos also provides a menu bar merging feature to make
    QMenuBar conform more closely to accepted \macos menu bar layout.
    The merging functionality is based on string matching the title of
    a QMenu entry. These strings are translated (using QObject::tr())
    in the "QMenuBar" context. If an entry is moved its slots will still
    fire as if it was in the original place. The table below outlines
    the strings looked for and where the entry is placed if matched:

    \table
    \header \li String matches \li Placement \li Notes
    \row \li about.*
         \li Application Menu | About <application name>
         \li The application name is fetched from the \c {Info.plist} file
            (see note below). If this entry is not found no About item
            will appear in the Application Menu.
    \row \li config, options, setup, settings or preferences
         \li Application Menu | Preferences
         \li If this entry is not found the Settings item will be disabled
    \row \li quit or exit
         \li Application Menu | Quit <application name>
         \li If this entry is not found a default Quit item will be
            created to call QCoreApplication::quit()
    \endtable

    You can override this behavior by using the QAction::menuRole()
    property.

    If you want all windows in a Mac application to share one menu
    bar, you must create a menu bar that does not have a parent.
    Create a parent-less menu bar this way:

    \snippet code/src_gui_widgets_qmenubar.cpp 1

    \b{Note:} Do \e{not} call QMainWindow::menuBar() to create the
    shared menu bar, because that menu bar will have the QMainWindow
    as its parent. That menu bar would only be displayed for the
    parent QMainWindow.

    \b{Note:} The text used for the application name in the \macos menu
    bar is obtained from the value set in the \c{Info.plist} file in
    the application's bundle. See \l{Qt for macOS - Deployment}
    for more information.

    \b{Note:} On Linux, if the com.canonical.AppMenu.Registrar
    service is available on the D-Bus session bus, then Qt will
    communicate with it to install the application's menus into the
    global menu bar, as described.

    \section1 Examples

    The \l{mainwindows/menus}{Menus} example shows how to use QMenuBar
    and QMenu.  The other \l{Main Window Examples}{main window
    application examples} also provide menus using these classes.

    \sa QMenu, QShortcut, QAction,
        {http://developer.apple.com/documentation/UserExperience/Conceptual/AppleHIGuidelines/XHIGIntro/XHIGIntro.html}{Introduction to Apple Human Interface Guidelines},
        {fowler}{GUI Design Handbook: Menu Bar}, {Menus Example}
*/


void QMenuBarPrivate::init()
{
    Q_Q(QMenuBar);
    q->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    q->setAttribute(Qt::WA_CustomWhatsThis);

    if (!QApplication::instance()->testAttribute(Qt::AA_DontUseNativeMenuBar))
        platformMenuBar = QGuiApplicationPrivate::platformTheme()->createPlatformMenuBar();

    if (platformMenuBar)
        q->hide();
    q->setBackgroundRole(QPalette::Button);
    handleReparent();
    q->setMouseTracking(q->style()->styleHint(QStyle::SH_MenuBar_MouseTracking, 0, q));

    extension = new QMenuBarExtension(q);
    extension->setFocusPolicy(Qt::NoFocus);
    extension->hide();
}

//Gets the next action for keyboard navigation
QAction *QMenuBarPrivate::getNextAction(const int _start, const int increment) const
{
    Q_Q(const QMenuBar);
    const_cast<QMenuBarPrivate*>(this)->updateGeometries();
    bool allowActiveAndDisabled = q->style()->styleHint(QStyle::SH_Menu_AllowActiveAndDisabled, 0, q);
    const int start = (_start == -1 && increment == -1) ? actions.count() : _start;
    const int end =  increment == -1 ? 0 : actions.count() - 1;

    for (int i = start; i != end;) {
        i += increment;
        QAction *current = actions.at(i);
        if (!actionRects.at(i).isNull() && (allowActiveAndDisabled || current->isEnabled()))
            return current;
    }

    if (_start != -1) //let's try from the beginning or the end
        return getNextAction(-1, increment);

    return 0;
}

/*!
    Constructs a menu bar with parent \a parent.
*/
QMenuBar::QMenuBar(QWidget *parent) : QWidget(*new QMenuBarPrivate, parent, 0)
{
    Q_D(QMenuBar);
    d->init();
}


/*!
    Destroys the menu bar.
*/
QMenuBar::~QMenuBar()
{
    Q_D(QMenuBar);
    delete d->platformMenuBar;
    d->platformMenuBar = 0;
}

/*!
    This convenience function creates a new action with \a text.
    The function adds the newly created action to the menu's
    list of actions, and returns it.

    \sa QWidget::addAction(), QWidget::actions()
*/
QAction *QMenuBar::addAction(const QString &text)
{
    QAction *ret = new QAction(text, this);
    addAction(ret);
    return ret;
}

/*!
    \overload

    This convenience function creates a new action with the given \a
    text. The action's triggered() signal is connected to the \a
    receiver's \a member slot. The function adds the newly created
    action to the menu's list of actions and returns it.

    \sa QWidget::addAction(), QWidget::actions()
*/
QAction *QMenuBar::addAction(const QString &text, const QObject *receiver, const char* member)
{
    QAction *ret = new QAction(text, this);
    QObject::connect(ret, SIGNAL(triggered(bool)), receiver, member);
    addAction(ret);
    return ret;
}

/*!
    \fn template<typename Obj, typename PointerToMemberFunctionOrFunctor> QAction *QMenuBar::addAction(const QString &text, const Obj *receiver, PointerToMemberFunctionOrFunctor method)

    \since 5.11

    \overload

    This convenience function creates a new action with the given \a
    text. The action's triggered() signal is connected to the
    \a method of the \a receiver. The function adds the newly created
    action to the menu's list of actions and returns it.

    QMenuBar takes ownership of the returned QAction.

    \sa QWidget::addAction(), QWidget::actions()
*/

/*!
    \fn template<typename Functor> QAction *QMenuBar::addAction(const QString &text, Functor functor)

    \since 5.11

    \overload

    This convenience function creates a new action with the given \a
    text. The action's triggered() signal is connected to the
    \a functor. The function adds the newly created
    action to the menu's list of actions and returns it.

    QMenuBar takes ownership of the returned QAction.

    \sa QWidget::addAction(), QWidget::actions()
*/

/*!
  Appends a new QMenu with \a title to the menu bar. The menu bar
  takes ownership of the menu. Returns the new menu.

  \sa QWidget::addAction(), QMenu::menuAction()
*/
QMenu *QMenuBar::addMenu(const QString &title)
{
    QMenu *menu = new QMenu(title, this);
    addAction(menu->menuAction());
    return menu;
}

/*!
  Appends a new QMenu with \a icon and \a title to the menu bar. The menu bar
  takes ownership of the menu. Returns the new menu.

  \sa QWidget::addAction(), QMenu::menuAction()
*/
QMenu *QMenuBar::addMenu(const QIcon &icon, const QString &title)
{
    QMenu *menu = new QMenu(title, this);
    menu->setIcon(icon);
    addAction(menu->menuAction());
    return menu;
}

/*!
    Appends \a menu to the menu bar. Returns the menu's menuAction().

    \note The returned QAction object can be used to hide the corresponding
    menu.

    \sa QWidget::addAction(), QMenu::menuAction()
*/
QAction *QMenuBar::addMenu(QMenu *menu)
{
    QAction *action = menu->menuAction();
    addAction(action);
    return action;
}

/*!
  Appends a separator to the menu.
*/
QAction *QMenuBar::addSeparator()
{
    QAction *ret = new QAction(this);
    ret->setSeparator(true);
    addAction(ret);
    return ret;
}

/*!
    This convenience function creates a new separator action, i.e. an
    action with QAction::isSeparator() returning true. The function inserts
    the newly created action into this menu bar's list of actions before
    action \a before and returns it.

    \sa QWidget::insertAction(), addSeparator()
*/
QAction *QMenuBar::insertSeparator(QAction *before)
{
    QAction *action = new QAction(this);
    action->setSeparator(true);
    insertAction(before, action);
    return action;
}

/*!
  This convenience function inserts \a menu before action \a before
  and returns the menus menuAction().

  \sa QWidget::insertAction(), addMenu()
*/
QAction *QMenuBar::insertMenu(QAction *before, QMenu *menu)
{
    QAction *action = menu->menuAction();
    insertAction(before, action);
    return action;
}

/*!
  Returns the QAction that is currently highlighted. A null pointer
  will be returned if no action is currently selected.
*/
QAction *QMenuBar::activeAction() const
{
    Q_D(const QMenuBar);
    return d->currentAction;
}

/*!
    \since 4.1

    Sets the currently highlighted action to \a act.
*/
void QMenuBar::setActiveAction(QAction *act)
{
    Q_D(QMenuBar);
    d->setCurrentAction(act, true, false);
}


/*!
    Removes all the actions from the menu bar.

    \note On \macos, menu items that have been merged to the system
    menu bar are not removed by this function. One way to handle this
    would be to remove the extra actions yourself. You can set the
    \l{QAction::MenuRole}{menu role} on the different menus, so that
    you know ahead of time which menu items get merged and which do
    not. Then decide what to recreate or remove yourself.

    \sa removeAction()
*/
void QMenuBar::clear()
{
    QList<QAction*> acts = actions();
    for(int i = 0; i < acts.size(); i++)
        removeAction(acts[i]);
}

/*!
    \property QMenuBar::defaultUp
    \brief the popup orientation

    The default popup orientation. By default, menus pop "down" the
    screen. By setting the property to true, the menu will pop "up".
    You might call this for menus that are \e below the document to
    which they refer.

    If the menu would not fit on the screen, the other direction is
    used automatically.
*/
void QMenuBar::setDefaultUp(bool b)
{
    Q_D(QMenuBar);
    d->defaultPopDown = !b;
}

bool QMenuBar::isDefaultUp() const
{
    Q_D(const QMenuBar);
    return !d->defaultPopDown;
}

/*!
  \reimp
*/
void QMenuBar::resizeEvent(QResizeEvent *)
{
    Q_D(QMenuBar);
    d->itemsDirty = true;
    d->updateGeometries();
}

/*!
  \reimp
*/
void QMenuBar::paintEvent(QPaintEvent *e)
{
    Q_D(QMenuBar);
    QPainter p(this);
    QRegion emptyArea(rect());

    //draw the items
    for (int i = 0; i < d->actions.count(); ++i) {
        QAction *action = d->actions.at(i);
        QRect adjustedActionRect = d->actionRect(action);
        if (adjustedActionRect.isEmpty() || !d->isVisible(action))
            continue;
        if(!e->rect().intersects(adjustedActionRect))
            continue;

        emptyArea -= adjustedActionRect;
        QStyleOptionMenuItem opt;
        initStyleOption(&opt, action);
        opt.rect = adjustedActionRect;
        p.setClipRect(adjustedActionRect);
        style()->drawControl(QStyle::CE_MenuBarItem, &opt, &p, this);
    }
     //draw border
    if(int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this)) {
        QRegion borderReg;
        borderReg += QRect(0, 0, fw, height()); //left
        borderReg += QRect(width()-fw, 0, fw, height()); //right
        borderReg += QRect(0, 0, width(), fw); //top
        borderReg += QRect(0, height()-fw, width(), fw); //bottom
        p.setClipRegion(borderReg);
        emptyArea -= borderReg;
        QStyleOptionFrame frame;
        frame.rect = rect();
        frame.palette = palette();
        frame.state = QStyle::State_None;
        frame.lineWidth = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth);
        frame.midLineWidth = 0;
        style()->drawPrimitive(QStyle::PE_PanelMenuBar, &frame, &p, this);
    }
    p.setClipRegion(emptyArea);
    QStyleOptionMenuItem menuOpt;
    menuOpt.palette = palette();
    menuOpt.state = QStyle::State_None;
    menuOpt.menuItemType = QStyleOptionMenuItem::EmptyArea;
    menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
    menuOpt.rect = rect();
    menuOpt.menuRect = rect();
    style()->drawControl(QStyle::CE_MenuBarEmptyArea, &menuOpt, &p, this);
}

/*!
  \reimp
*/
void QMenuBar::setVisible(bool visible)
{
    if (isNativeMenuBar()) {
        if (!visible)
            QWidget::setVisible(false);
        return;
    }
    QWidget::setVisible(visible);
}

/*!
  \reimp
*/
void QMenuBar::mousePressEvent(QMouseEvent *e)
{
    Q_D(QMenuBar);
    if(e->button() != Qt::LeftButton)
        return;

    d->mouseDown = true;

    QAction *action = d->actionAt(e->pos());
    if (!action || !d->isVisible(action) || !action->isEnabled()) {
        d->setCurrentAction(0);
#if QT_CONFIG(whatsthis)
        if (QWhatsThis::inWhatsThisMode())
            QWhatsThis::showText(e->globalPos(), d->whatsThis, this);
#endif
        return;
    }

    if(d->currentAction == action && d->popupState) {
        if(QMenu *menu = d->activeMenu) {
            d->activeMenu = 0;
            menu->setAttribute(Qt::WA_NoMouseReplay);
            menu->hide();
        }
    } else {
        d->setCurrentAction(action, true);
    }
}

/*!
  \reimp
*/
void QMenuBar::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QMenuBar);
    if(e->button() != Qt::LeftButton || !d->mouseDown)
        return;

    d->mouseDown = false;
    QAction *action = d->actionAt(e->pos());

    // do noting if the action is hidden
    if (!d->isVisible(action))
        return;
    if((d->closePopupMode && action == d->currentAction) || !action || !action->menu()) {
        //we set the current action before activating
        //so that we let the leave event set the current back to 0
        d->setCurrentAction(action, false);
        if(action)
            d->activateAction(action, QAction::Trigger);
    }
    d->closePopupMode = 0;
}

/*!
  \reimp
*/
void QMenuBar::keyPressEvent(QKeyEvent *e)
{
    Q_D(QMenuBar);
    d->updateGeometries();
    int key = e->key();
    if(isRightToLeft()) {  // in reverse mode open/close key for submenues are reversed
        if(key == Qt::Key_Left)
            key = Qt::Key_Right;
        else if(key == Qt::Key_Right)
            key = Qt::Key_Left;
    }
    if(key == Qt::Key_Tab) //means right
        key = Qt::Key_Right;
    else if(key == Qt::Key_Backtab) //means left
        key = Qt::Key_Left;

    bool key_consumed = false;
    switch(key) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Enter:
    case Qt::Key_Space:
    case Qt::Key_Return: {
        if(!style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation, 0, this) || !d->currentAction)
           break;
        if(d->currentAction->menu()) {
            d->popupAction(d->currentAction, true);
        } else if(key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Space) {
            d->activateAction(d->currentAction, QAction::Trigger);
            d->setCurrentAction(d->currentAction, false);
            d->setKeyboardMode(false);
        }
        key_consumed = true;
        break; }

    case Qt::Key_Right:
    case Qt::Key_Left: {
        if(d->currentAction) {
            int index = d->actions.indexOf(d->currentAction);
            if (QAction *nextAction = d->getNextAction(index, key == Qt::Key_Left ? -1 : +1)) {
                d->setCurrentAction(nextAction, d->popupState, true);
                key_consumed = true;
            }
        }
        break; }

    default:
        key_consumed = false;
    }

#ifndef QT_NO_SHORTCUT
    if (!key_consumed && e->matches(QKeySequence::Cancel)) {
        d->setCurrentAction(0);
        d->setKeyboardMode(false);
        key_consumed = true;
    }
#endif

    if(!key_consumed &&
       (!e->modifiers() ||
        (e->modifiers()&(Qt::MetaModifier|Qt::AltModifier))) && e->text().length()==1 && !d->popupState) {
        int clashCount = 0;
        QAction *first = 0, *currentSelected = 0, *firstAfterCurrent = 0;
        {
            const QChar c = e->text().at(0).toUpper();
            for(int i = 0; i < d->actions.size(); ++i) {
                if (d->actionRects.at(i).isNull())
                    continue;
                QAction *act = d->actions.at(i);
                QString s = act->text();
                if(!s.isEmpty()) {
                    int ampersand = s.indexOf(QLatin1Char('&'));
                    if(ampersand >= 0) {
                        if(s[ampersand+1].toUpper() == c) {
                            clashCount++;
                            if(!first)
                                first = act;
                            if(act == d->currentAction)
                                currentSelected = act;
                            else if (!firstAfterCurrent && currentSelected)
                                firstAfterCurrent = act;
                        }
                    }
                }
            }
        }
        QAction *next_action = 0;
        if(clashCount >= 1) {
            if(clashCount == 1 || !d->currentAction || (currentSelected && !firstAfterCurrent))
                next_action = first;
            else
                next_action = firstAfterCurrent;
        }
        if(next_action) {
            key_consumed = true;
            d->setCurrentAction(next_action, true, true);
        }
    }
    if(key_consumed)
        e->accept();
    else
        e->ignore();
}

/*!
  \reimp
*/
void QMenuBar::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QMenuBar);
    if (!(e->buttons() & Qt::LeftButton)) {
        d->mouseDown = false;
        // We receive mouse move and mouse press on touch.
        // Mouse move will open the menu and mouse press
        // will close it, so ignore mouse move.
        if (e->source() != Qt::MouseEventNotSynthesized)
            return;
    }

    bool popupState = d->popupState || d->mouseDown;
    QAction *action = d->actionAt(e->pos());
    if ((action && d->isVisible(action)) || !popupState)
        d->setCurrentAction(action, popupState);
}

/*!
  \reimp
*/
void QMenuBar::leaveEvent(QEvent *)
{
    Q_D(QMenuBar);
    if((!hasFocus() && !d->popupState) ||
        (d->currentAction && d->currentAction->menu() == 0))
        d->setCurrentAction(0);
}

QPlatformMenu *QMenuBarPrivate::getPlatformMenu(const QAction *action)
{
    if (!action || !action->menu())
        return 0;

    QPlatformMenu *platformMenu = action->menu()->platformMenu();
    if (!platformMenu && platformMenuBar) {
        platformMenu = platformMenuBar->createMenu();
        if (platformMenu)
            action->menu()->setPlatformMenu(platformMenu);
    }

    return platformMenu;
}

QPlatformMenu *QMenuBarPrivate::findInsertionPlatformMenu(const QAction *action)
{
    Q_Q(QMenuBar);
    QPlatformMenu *beforeMenu = nullptr;
    for (int beforeIndex = indexOf(const_cast<QAction *>(action)) + 1;
         !beforeMenu && (beforeIndex < q->actions().size());
         ++beforeIndex) {
        beforeMenu = getPlatformMenu(q->actions().at(beforeIndex));
    }

    return beforeMenu;
}

void QMenuBarPrivate::copyActionToPlatformMenu(const QAction *action, QPlatformMenu *menu)
{
    const auto tag = reinterpret_cast<quintptr>(action);
    if (menu->tag() != tag)
        menu->setTag(tag);
    menu->setText(action->text());
    menu->setVisible(action->isVisible());
    menu->setEnabled(action->isEnabled());
}

/*!
  \reimp
*/
void QMenuBar::actionEvent(QActionEvent *e)
{
    Q_D(QMenuBar);
    d->itemsDirty = true;

    if (d->platformMenuBar) {
        QPlatformMenuBar *nativeMenuBar = d->platformMenuBar;
        if (!nativeMenuBar)
            return;

        if (e->type() == QEvent::ActionAdded) {
            QPlatformMenu *menu = d->getPlatformMenu(e->action());
            if (menu) {
                d->copyActionToPlatformMenu(e->action(), menu);

                QPlatformMenu *beforeMenu = d->findInsertionPlatformMenu(e->action());
                d->platformMenuBar->insertMenu(menu, beforeMenu);
            }
        } else if (e->type() == QEvent::ActionRemoved) {
            QPlatformMenu *menu = d->getPlatformMenu(e->action());
            if (menu)
                d->platformMenuBar->removeMenu(menu);
        } else if (e->type() == QEvent::ActionChanged) {
            QPlatformMenu *cur = d->platformMenuBar->menuForTag(reinterpret_cast<quintptr>(e->action()));
            QPlatformMenu *menu = d->getPlatformMenu(e->action());

            // the menu associated with the action can change, need to
            // remove and/or insert the new platform menu
            if (menu != cur) {
                if (cur)
                    d->platformMenuBar->removeMenu(cur);
                if (menu) {
                    d->copyActionToPlatformMenu(e->action(), menu);

                    QPlatformMenu *beforeMenu = d->findInsertionPlatformMenu(e->action());
                    d->platformMenuBar->insertMenu(menu, beforeMenu);
                }
            } else if (menu) {
                d->copyActionToPlatformMenu(e->action(), menu);
                d->platformMenuBar->syncMenu(menu);
            }
        }
    }

    if(e->type() == QEvent::ActionAdded) {
        connect(e->action(), SIGNAL(triggered()), this, SLOT(_q_actionTriggered()));
        connect(e->action(), SIGNAL(hovered()), this, SLOT(_q_actionHovered()));
    } else if(e->type() == QEvent::ActionRemoved) {
        e->action()->disconnect(this);
    }
    // updateGeometries() is also needed for native menu bars because
    // it updates shortcutIndexMap
    if (isVisible() || isNativeMenuBar())
        d->updateGeometries();
    if (isVisible())
        update();
}

/*!
  \reimp
*/
void QMenuBar::focusInEvent(QFocusEvent *)
{
    Q_D(QMenuBar);
    if(d->keyboardState)
        d->focusFirstAction();
}

/*!
  \reimp
*/
void QMenuBar::focusOutEvent(QFocusEvent *)
{
    Q_D(QMenuBar);
    if(!d->popupState) {
        d->setCurrentAction(0);
        d->setKeyboardMode(false);
    }
}

/*!
  \reimp
 */
void QMenuBar::timerEvent (QTimerEvent *e)
{
    Q_D(QMenuBar);
    if (e->timerId() == d->autoReleaseTimer.timerId()) {
        d->autoReleaseTimer.stop();
        d->setCurrentAction(0);
    }
    QWidget::timerEvent(e);
}


void QMenuBarPrivate::handleReparent()
{
    Q_Q(QMenuBar);
    QWidget *newParent = q->parentWidget();

    //Note: if parent is reparented, then window may change even if parent doesn't.
    // We need to install an avent filter on each parent up to the parent that is
    // also a window (for shortcuts)
    QWidget *newWindow = newParent ? newParent->window() : nullptr;

    QVector<QPointer<QWidget> > newParents;
    // Remove event filters on ex-parents, keep them on still-parents
    // The parents are always ordered in the vector
    foreach (const QPointer<QWidget> &w, oldParents) {
        if (w) {
            if (newParent == w) {
                newParents.append(w);
                if (newParent != newWindow) //stop at the window
                    newParent = newParent->parentWidget();
            } else {
                w->removeEventFilter(q);
            }
        }
    }

    // At this point, newParent is the next one to be added to newParents
    while (newParent && newParent != newWindow) {
        //install event filters all the way up to (excluding) the window
        newParents.append(newParent);
        newParent->installEventFilter(q);
        newParent = newParent->parentWidget();
    }

    if (newParent && newWindow) {
        // Install the event filter on the window
        newParents.append(newParent);
        newParent->installEventFilter(q);
    }
    oldParents = newParents;

    if (platformMenuBar) {
        if (newWindow) {
            // force the underlying platform window to be created, since
            // the platform menubar needs it (and we have no other way to
            // discover when the platform window is created)
            newWindow->createWinId();
            platformMenuBar->handleReparent(newWindow->windowHandle());
        } else {
            platformMenuBar->handleReparent(0);
        }
    }
}

/*!
  \reimp
*/
void QMenuBar::changeEvent(QEvent *e)
{
    Q_D(QMenuBar);
    if(e->type() == QEvent::StyleChange) {
        d->itemsDirty = true;
        setMouseTracking(style()->styleHint(QStyle::SH_MenuBar_MouseTracking, 0, this));
        if(parentWidget())
            resize(parentWidget()->width(), heightForWidth(parentWidget()->width()));
        d->updateGeometries();
    } else if (e->type() == QEvent::ParentChange) {
        d->handleReparent();
    } else if (e->type() == QEvent::FontChange
               || e->type() == QEvent::ApplicationFontChange) {
        d->itemsDirty = true;
        d->updateGeometries();
    }

    QWidget::changeEvent(e);
}

/*!
  \reimp
*/
bool QMenuBar::event(QEvent *e)
{
    Q_D(QMenuBar);
    switch (e->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = (QKeyEvent*)e;
#if 0
        if(!d->keyboardState) { //all keypresses..
            d->setCurrentAction(0);
            return ;
        }
#endif
        if(ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab) {
            keyPressEvent(ke);
            return true;
        }

    } break;
#ifndef QT_NO_SHORTCUT
    case QEvent::Shortcut: {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        int shortcutId = se->shortcutId();
        for(int j = 0; j < d->shortcutIndexMap.size(); ++j) {
            if (shortcutId == d->shortcutIndexMap.value(j))
                d->_q_internalShortcutActivated(j);
        }
    } break;
#endif
    case QEvent::Show:
        d->_q_updateLayout();
    break;
#ifndef QT_NO_SHORTCUT
    case QEvent::ShortcutOverride: {
        QKeyEvent *kev = static_cast<QKeyEvent*>(e);
        //we only filter out escape if there is a current action
        if (kev->matches(QKeySequence::Cancel) && d->currentAction) {
            e->accept();
            return true;
        }
    }
    break;
#endif
#if QT_CONFIG(whatsthis)
    case QEvent::QueryWhatsThis:
        e->setAccepted(d->whatsThis.size());
        if (QAction *action = d->actionAt(static_cast<QHelpEvent*>(e)->pos())) {
            if (action->whatsThis().size() || action->menu())
                e->accept();
        }
        return true;
#endif
    case QEvent::LayoutDirectionChange:
        d->_q_updateLayout();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

/*!
  \reimp
*/
bool QMenuBar::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QMenuBar);
    if (object && (event->type() == QEvent::ParentChange)) //GrandparentChange
            d->handleReparent();

    if (object == d->leftWidget || object == d->rightWidget) {
        switch (event->type()) {
        case QEvent::ShowToParent:
        case QEvent::HideToParent:
            d->_q_updateLayout();
            break;
        default:
            break;
        }
    }

    if (isNativeMenuBar() && event->type() == QEvent::ShowToParent) {
        // On some desktops like Unity, the D-Bus menu bar is unregistered
        // when the window is hidden. So when the window is shown, we need
        // to forcefully re-register it. The only way to force re-registering
        // with D-Bus menu is the handleReparent method.
        QWidget *widget = qobject_cast<QWidget *>(object);
        QWindow *handle = widget ? widget->windowHandle() : nullptr;
        if (handle != nullptr)
            d->platformMenuBar->handleReparent(handle);
    }

    if (style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation, 0, this)) {
        if (d->altPressed) {
            switch (event->type()) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
            {
                QKeyEvent *kev = static_cast<QKeyEvent*>(event);
                if (kev->key() == Qt::Key_Alt || kev->key() == Qt::Key_Meta) {
                    if (event->type() == QEvent::KeyPress) // Alt-press does not interest us, we have the shortcut-override event
                        break;
                    d->setKeyboardMode(!d->keyboardState);
                }
            }
            Q_FALLTHROUGH();
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseMove:
            case QEvent::FocusIn:
            case QEvent::FocusOut:
            case QEvent::ActivationChange:
            case QEvent::Shortcut:
                d->altPressed = false;
                qApp->removeEventFilter(this);
                break;
            default:
                break;
            }
        } else if (isVisible()) {
            if (event->type() == QEvent::ShortcutOverride) {
                QKeyEvent *kev = static_cast<QKeyEvent*>(event);
                if ((kev->key() == Qt::Key_Alt || kev->key() == Qt::Key_Meta)
                    && kev->modifiers() == Qt::AltModifier) {
                    d->altPressed = true;
                    qApp->installEventFilter(this);
                }
            }
        }
    }

    return false;
}

/*!
  Returns the QAction at \a pt. Returns 0 if there is no action at \a pt or if
the location has a separator.

    \sa addAction(), addSeparator()
*/
QAction *QMenuBar::actionAt(const QPoint &pt) const
{
    Q_D(const QMenuBar);
    return d->actionAt(pt);
}

/*!
  Returns the geometry of action \a act as a QRect.

    \sa actionAt()
*/
QRect QMenuBar::actionGeometry(QAction *act) const
{
    Q_D(const QMenuBar);
    return d->actionRect(act);
}

/*!
  \reimp
*/
QSize QMenuBar::minimumSizeHint() const
{
    Q_D(const QMenuBar);
    const bool as_gui_menubar = !isNativeMenuBar();

    ensurePolished();
    QSize ret(0, 0);
    const_cast<QMenuBarPrivate*>(d)->updateGeometries();
    const int hmargin = style()->pixelMetric(QStyle::PM_MenuBarHMargin, 0, this);
    const int vmargin = style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, this);
    int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
    int spaceBelowMenuBar = style()->styleHint(QStyle::SH_MainWindow_SpaceBelowMenuBar, 0, this);
    if(as_gui_menubar) {
        int w = parentWidget() ? parentWidget()->width() : QDesktopWidgetPrivate::width();
        d->calcActionRects(w - (2 * fw), 0);
        for (int i = 0; ret.isNull() && i < d->actions.count(); ++i)
            ret = d->actionRects.at(i).size();
        if (!d->extension->isHidden())
            ret += QSize(d->extension->sizeHint().width(), 0);
        ret += QSize(2*fw + hmargin, 2*fw + vmargin);
    }
    int margin = 2*vmargin + 2*fw + spaceBelowMenuBar;
    if(d->leftWidget) {
        QSize sz = d->leftWidget->minimumSizeHint();
        ret.setWidth(ret.width() + sz.width());
        if(sz.height() + margin > ret.height())
            ret.setHeight(sz.height() + margin);
    }
    if(d->rightWidget) {
        QSize sz = d->rightWidget->minimumSizeHint();
        ret.setWidth(ret.width() + sz.width());
        if(sz.height() + margin > ret.height())
            ret.setHeight(sz.height() + margin);
    }
    if(as_gui_menubar) {
        QStyleOptionMenuItem opt;
        opt.rect = rect();
        opt.menuRect = rect();
        opt.state = QStyle::State_None;
        opt.menuItemType = QStyleOptionMenuItem::Normal;
        opt.checkType = QStyleOptionMenuItem::NotCheckable;
        opt.palette = palette();
        return (style()->sizeFromContents(QStyle::CT_MenuBar, &opt,
                                         ret.expandedTo(QApplication::globalStrut()),
                                         this));
    }
    return ret;
}

/*!
  \reimp
*/
QSize QMenuBar::sizeHint() const
{
    Q_D(const QMenuBar);
    const bool as_gui_menubar = !isNativeMenuBar();

    ensurePolished();
    QSize ret(0, 0);
    const_cast<QMenuBarPrivate*>(d)->updateGeometries();
    const int hmargin = style()->pixelMetric(QStyle::PM_MenuBarHMargin, 0, this);
    const int vmargin = style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, this);
    int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
    int spaceBelowMenuBar = style()->styleHint(QStyle::SH_MainWindow_SpaceBelowMenuBar, 0, this);
    if(as_gui_menubar) {
        const int w = parentWidget() ? parentWidget()->width() : QDesktopWidgetPrivate::width();
        d->calcActionRects(w - (2 * fw), 0);
        for (int i = 0; i < d->actionRects.count(); ++i) {
            const QRect &actionRect = d->actionRects.at(i);
            ret = ret.expandedTo(QSize(actionRect.x() + actionRect.width(), actionRect.y() + actionRect.height()));
        }
        //the action geometries already contain the top and left
        //margins. So we only need to add those from right and bottom.
        ret += QSize(fw + hmargin, fw + vmargin);
    }
    int margin = 2*vmargin + 2*fw + spaceBelowMenuBar;
    if(d->leftWidget) {
        QSize sz = d->leftWidget->sizeHint();
        sz.rheight() += margin;
        ret = ret.expandedTo(sz);
    }
    if(d->rightWidget) {
        QSize sz = d->rightWidget->sizeHint();
        ret.setWidth(ret.width() + sz.width());
        if(sz.height() + margin > ret.height())
            ret.setHeight(sz.height() + margin);
    }
    if(as_gui_menubar) {
        QStyleOptionMenuItem opt;
        opt.rect = rect();
        opt.menuRect = rect();
        opt.state = QStyle::State_None;
        opt.menuItemType = QStyleOptionMenuItem::Normal;
        opt.checkType = QStyleOptionMenuItem::NotCheckable;
        opt.palette = palette();
        return (style()->sizeFromContents(QStyle::CT_MenuBar, &opt,
                                         ret.expandedTo(QApplication::globalStrut()),
                                         this));
    }
    return ret;
}

/*!
  \reimp
*/
int QMenuBar::heightForWidth(int) const
{
    Q_D(const QMenuBar);
    const bool as_gui_menubar = !isNativeMenuBar();

    const_cast<QMenuBarPrivate*>(d)->updateGeometries();
    int height = 0;
    const int vmargin = style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, this);
    int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
    int spaceBelowMenuBar = style()->styleHint(QStyle::SH_MainWindow_SpaceBelowMenuBar, 0, this);
    if(as_gui_menubar) {
        for (int i = 0; i < d->actionRects.count(); ++i)
            height = qMax(height, d->actionRects.at(i).height());
        if (height) //there is at least one non-null item
            height += spaceBelowMenuBar;
        height += 2*fw;
        height += 2*vmargin;
    }
    int margin = 2*vmargin + 2*fw + spaceBelowMenuBar;
    if(d->leftWidget)
        height = qMax(d->leftWidget->sizeHint().height() + margin, height);
    if(d->rightWidget)
        height = qMax(d->rightWidget->sizeHint().height() + margin, height);
    if(as_gui_menubar) {
        QStyleOptionMenuItem opt;
        opt.init(this);
        opt.menuRect = rect();
        opt.state = QStyle::State_None;
        opt.menuItemType = QStyleOptionMenuItem::Normal;
        opt.checkType = QStyleOptionMenuItem::NotCheckable;
        return style()->sizeFromContents(QStyle::CT_MenuBar, &opt, QSize(0, height), this).height(); //not pretty..
    }
    return height;
}

/*!
  \internal
*/
void QMenuBarPrivate::_q_internalShortcutActivated(int id)
{
    Q_Q(QMenuBar);
    QAction *act = actions.at(id);
    if (act && act->menu()) {
        if (QPlatformMenu *platformMenu = act->menu()->platformMenu()) {
            platformMenu->showPopup(q->windowHandle(), actionRects.at(id), nullptr);
            return;
        }
    }

    keyboardFocusWidget = QApplication::focusWidget();
    setCurrentAction(act, true, true);
    if (act && !act->menu()) {
        activateAction(act, QAction::Trigger);
        //100 is the same as the default value in QPushButton::animateClick
        autoReleaseTimer.start(100, q);
    } else if (act && q->style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation, 0, q)) {
        // When we open a menu using a shortcut, we should end up in keyboard state
        setKeyboardMode(true);
    }
}

void QMenuBarPrivate::_q_updateLayout()
{
    Q_Q(QMenuBar);
    itemsDirty = true;
    if (q->isVisible()) {
        updateGeometries();
        q->update();
    }
}

/*!
    \fn void QMenuBar::setCornerWidget(QWidget *widget, Qt::Corner corner)

    This sets the given \a widget to be shown directly on the left of the first
    menu item, or on the right of the last menu item, depending on \a corner.

    The menu bar takes ownership of \a widget, reparenting it into the menu bar.
    However, if the \a corner already contains a widget, this previous widget
    will no longer be managed and will still be a visible child of the menu bar.

   \note Using a corner other than Qt::TopRightCorner or Qt::TopLeftCorner
    will result in a warning.
*/
void QMenuBar::setCornerWidget(QWidget *w, Qt::Corner corner)
{
    Q_D(QMenuBar);
    switch (corner) {
    case Qt::TopLeftCorner:
        if (d->leftWidget)
            d->leftWidget->removeEventFilter(this);
        d->leftWidget = w;
        break;
    case Qt::TopRightCorner:
        if (d->rightWidget)
            d->rightWidget->removeEventFilter(this);
        d->rightWidget = w;
        break;
    default:
        qWarning("QMenuBar::setCornerWidget: Only TopLeftCorner and TopRightCorner are supported");
        return;
    }

    if (w) {
        w->setParent(this);
        w->installEventFilter(this);
    }

    d->_q_updateLayout();
}

/*!
    Returns the widget on the left of the first or on the right of the last menu
    item, depending on \a corner.

   \note Using a corner other than Qt::TopRightCorner or Qt::TopLeftCorner
    will result in a warning.
*/
QWidget *QMenuBar::cornerWidget(Qt::Corner corner) const
{
    Q_D(const QMenuBar);
    QWidget *w = 0;
    switch(corner) {
    case Qt::TopLeftCorner:
        w = d->leftWidget;
        break;
    case Qt::TopRightCorner:
        w = d->rightWidget;
        break;
    default:
        qWarning("QMenuBar::cornerWidget: Only TopLeftCorner and TopRightCorner are supported");
        break;
    }

    return w;
}

/*!
    \property QMenuBar::nativeMenuBar
    \brief Whether or not a menubar will be used as a native menubar on platforms that support it
    \since 4.6

    This property specifies whether or not the menubar should be used as a native menubar on
    platforms that support it. The currently supported platforms are \macos, and
    Linux desktops which use the com.canonical.dbusmenu D-Bus interface (such as Ubuntu Unity).
    If this property is \c true, the menubar is used in the native menubar and is not in the window of
    its parent; if \c false the menubar remains in the window. On other platforms,
    setting this attribute has no effect, and reading this attribute will always return \c false.

    The default is to follow whether the Qt::AA_DontUseNativeMenuBar attribute
    is set for the application. Explicitly setting this property overrides
    the presence (or absence) of the attribute.
*/

void QMenuBar::setNativeMenuBar(bool nativeMenuBar)
{
    Q_D(QMenuBar);
    if (nativeMenuBar != bool(d->platformMenuBar)) {
        if (!nativeMenuBar) {
            delete d->platformMenuBar;
            d->platformMenuBar = 0;
        } else {
            if (!d->platformMenuBar)
                d->platformMenuBar = QGuiApplicationPrivate::platformTheme()->createPlatformMenuBar();
        }

        updateGeometry();
        if (!nativeMenuBar && parentWidget())
            setVisible(true);
    }
}

bool QMenuBar::isNativeMenuBar() const
{
    Q_D(const QMenuBar);
    return bool(d->platformMenuBar);
}

/*!
    \internal
*/
QPlatformMenuBar *QMenuBar::platformMenuBar()
{
    Q_D(const QMenuBar);
    return d->platformMenuBar;
}

/*!
    \fn void QMenuBar::triggered(QAction *action)

    This signal is emitted when an action in a menu belonging to this menubar
    is triggered as a result of a mouse click; \a action is the action that
    caused the signal to be emitted.

    \note QMenuBar has to have ownership of the QMenu in order this signal to work.

    Normally, you connect each menu action to a single slot using
    QAction::triggered(), but sometimes you will want to connect
    several items to a single slot (most often if the user selects
    from an array). This signal is useful in such cases.

    \sa hovered(), QAction::triggered()
*/

/*!
    \fn void QMenuBar::hovered(QAction *action)

    This signal is emitted when a menu action is highlighted; \a action
    is the action that caused the event to be sent.

    Often this is used to update status information.

    \sa triggered(), QAction::hovered()
*/

// for private slots

QT_END_NAMESPACE

#include <moc_qmenubar.cpp>
