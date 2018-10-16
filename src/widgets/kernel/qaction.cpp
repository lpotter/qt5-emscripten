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

#include "qaction.h"
#include "qactiongroup.h"

#ifndef QT_NO_ACTION
#include "qaction_p.h"
#include "qapplication.h"
#include "qevent.h"
#include "qlist.h"
#include "qstylehints.h"
#include <private/qshortcutmap_p.h>
#include <private/qapplication_p.h>
#if QT_CONFIG(menu)
#include <private/qmenu_p.h>
#endif
#include <private/qdebug_p.h>

#define QAPP_CHECK(functionName) \
    if (Q_UNLIKELY(!qApp)) { \
        qWarning("QAction: Initialize QApplication before calling '" functionName "'."); \
        return; \
    }

QT_BEGIN_NAMESPACE

/*
  internal: guesses a descriptive text from a text suited for a menu entry
 */
static QString qt_strippedText(QString s)
{
    s.remove(QLatin1String("..."));
    for (int i = 0; i < s.size(); ++i) {
        if (s.at(i) == QLatin1Char('&'))
            s.remove(i, 1);
    }
    return s.trimmed();
}


QActionPrivate::QActionPrivate() : group(0), enabled(1), forceDisabled(0),
                                   visible(1), forceInvisible(0), checkable(0), checked(0), separator(0), fontSet(false),
                                   iconVisibleInMenu(-1),
                                   shortcutVisibleInContextMenu(-1),
                                   menuRole(QAction::TextHeuristicRole),
                                   priority(QAction::NormalPriority)
{
#ifndef QT_NO_SHORTCUT
    shortcutId = 0;
    shortcutContext = Qt::WindowShortcut;
    autorepeat = true;
#endif
}

QActionPrivate::~QActionPrivate()
{
}

bool QActionPrivate::showStatusText(QWidget *widget, const QString &str)
{
#if !QT_CONFIG(statustip)
    Q_UNUSED(widget);
    Q_UNUSED(str);
#else
    if(QObject *object = widget ? widget : parent) {
        QStatusTipEvent tip(str);
        QApplication::sendEvent(object, &tip);
        return true;
    }
#endif
    return false;
}

void QActionPrivate::sendDataChanged()
{
    Q_Q(QAction);
    QActionEvent e(QEvent::ActionChanged, q);
    for (int i = 0; i < widgets.size(); ++i) {
        QWidget *w = widgets.at(i);
        QApplication::sendEvent(w, &e);
    }
#if QT_CONFIG(graphicsview)
    for (int i = 0; i < graphicsWidgets.size(); ++i) {
        QGraphicsWidget *w = graphicsWidgets.at(i);
        QApplication::sendEvent(w, &e);
    }
#endif
    QApplication::sendEvent(q, &e);

    emit q->changed();
}

#ifndef QT_NO_SHORTCUT
void QActionPrivate::redoGrab(QShortcutMap &map)
{
    Q_Q(QAction);
    if (shortcutId)
        map.removeShortcut(shortcutId, q);
    if (shortcut.isEmpty())
        return;
    shortcutId = map.addShortcut(q, shortcut, shortcutContext, qWidgetShortcutContextMatcher);
    if (!enabled)
        map.setShortcutEnabled(false, shortcutId, q);
    if (!autorepeat)
        map.setShortcutAutoRepeat(false, shortcutId, q);
}

void QActionPrivate::redoGrabAlternate(QShortcutMap &map)
{
    Q_Q(QAction);
    for(int i = 0; i < alternateShortcutIds.count(); ++i) {
        if (const int id = alternateShortcutIds.at(i))
            map.removeShortcut(id, q);
    }
    alternateShortcutIds.clear();
    if (alternateShortcuts.isEmpty())
        return;
    for(int i = 0; i < alternateShortcuts.count(); ++i) {
        const QKeySequence& alternate = alternateShortcuts.at(i);
        if (!alternate.isEmpty())
            alternateShortcutIds.append(map.addShortcut(q, alternate, shortcutContext, qWidgetShortcutContextMatcher));
        else
            alternateShortcutIds.append(0);
    }
    if (!enabled) {
        for(int i = 0; i < alternateShortcutIds.count(); ++i) {
            const int id = alternateShortcutIds.at(i);
            map.setShortcutEnabled(false, id, q);
        }
    }
    if (!autorepeat) {
        for(int i = 0; i < alternateShortcutIds.count(); ++i) {
            const int id = alternateShortcutIds.at(i);
            map.setShortcutAutoRepeat(false, id, q);
        }
    }
}

void QActionPrivate::setShortcutEnabled(bool enable, QShortcutMap &map)
{
    Q_Q(QAction);
    if (shortcutId)
        map.setShortcutEnabled(enable, shortcutId, q);
    for(int i = 0; i < alternateShortcutIds.count(); ++i) {
        if (const int id = alternateShortcutIds.at(i))
            map.setShortcutEnabled(enable, id, q);
    }
}
#endif // QT_NO_SHORTCUT


/*!
    \class QAction
    \brief The QAction class provides an abstract user interface
    action that can be inserted into widgets.

    \ingroup mainwindow-classes
    \inmodule QtWidgets

    \omit
        * parent and widget are different
        * parent does not define context
    \endomit

    In applications many common commands can be invoked via menus,
    toolbar buttons, and keyboard shortcuts. Since the user expects
    each command to be performed in the same way, regardless of the
    user interface used, it is useful to represent each command as
    an \e action.

    Actions can be added to menus and toolbars, and will
    automatically keep them in sync. For example, in a word processor,
    if the user presses a Bold toolbar button, the Bold menu item
    will automatically be checked.

    Actions can be created as independent objects, but they may
    also be created during the construction of menus; the QMenu class
    contains convenience functions for creating actions suitable for
    use as menu items.

    A QAction may contain an icon, menu text, a shortcut, status text,
    "What's This?" text, and a tooltip. Most of these can be set in
    the constructor. They can also be set independently with
    setIcon(), setText(), setIconText(), setShortcut(),
    setStatusTip(), setWhatsThis(), and setToolTip(). For menu items,
    it is possible to set an individual font with setFont().

    Actions are added to widgets using QWidget::addAction() or
    QGraphicsWidget::addAction(). Note that an action must be added to a
    widget before it can be used; this is also true when the shortcut should
    be global (i.e., Qt::ApplicationShortcut as Qt::ShortcutContext).

    Once a QAction has been created it should be added to the relevant
    menu and toolbar, then connected to the slot which will perform
    the action. For example:

    \snippet mainwindows/application/mainwindow.cpp 19

    We recommend that actions are created as children of the window
    they are used in. In most cases actions will be children of
    the application's main window.

    \sa QMenu, QToolBar, {Application Example}
*/

/*!
    \fn void QAction::trigger()

    This is a convenience slot that calls activate(Trigger).
*/

/*!
    \fn void QAction::hover()

    This is a convenience slot that calls activate(Hover).
*/

/*!
    \enum QAction::MenuRole

    This enum describes how an action should be moved into the application menu on \macos.

    \value NoRole This action should not be put into the application menu
    \value TextHeuristicRole This action should be put in the application menu based on the action's text
           as described in the QMenuBar documentation.
    \value ApplicationSpecificRole This action should be put in the application menu with an application specific role
    \value AboutQtRole This action handles the "About Qt" menu item.
    \value AboutRole This action should be placed where the "About" menu item is in the application menu. The text of
           the menu item will be set to "About <application name>". The application name is fetched from the
           \c{Info.plist} file in the application's bundle (See \l{Qt for macOS - Deployment}).
    \value PreferencesRole This action should be placed where the  "Preferences..." menu item is in the application menu.
    \value QuitRole This action should be placed where the Quit menu item is in the application menu.

    Setting this value only has effect on items that are in the immediate menus
    of the menubar, not the submenus of those menus. For example, if you have
    File menu in your menubar and the File menu has a submenu, setting the
    MenuRole for the actions in that submenu have no effect. They will never be moved.
*/

/*!
    Constructs an action with \a parent. If \a parent is an action
    group the action will be automatically inserted into the group.

    \note The \a parent argument is optional since Qt 5.7.
*/
QAction::QAction(QObject* parent)
    : QAction(*new QActionPrivate, parent)
{
}


/*!
    Constructs an action with some \a text and \a parent. If \a
    parent is an action group the action will be automatically
    inserted into the group.

    The action uses a stripped version of \a text (e.g. "\&Menu
    Option..." becomes "Menu Option") as descriptive text for
    tool buttons. You can override this by setting a specific
    description with setText(). The same text will be used for
    tooltips unless you specify a different text using
    setToolTip().

*/
QAction::QAction(const QString &text, QObject* parent)
    : QAction(parent)
{
    Q_D(QAction);
    d->text = text;
}

/*!
    Constructs an action with an \a icon and some \a text and \a
    parent. If \a parent is an action group the action will be
    automatically inserted into the group.

    The action uses a stripped version of \a text (e.g. "\&Menu
    Option..." becomes "Menu Option") as descriptive text for
    tool buttons. You can override this by setting a specific
    description with setText(). The same text will be used for
    tooltips unless you specify a different text using
    setToolTip().
*/
QAction::QAction(const QIcon &icon, const QString &text, QObject* parent)
    : QAction(text, parent)
{
    Q_D(QAction);
    d->icon = icon;
}

/*!
    \internal
*/
QAction::QAction(QActionPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QAction);
    d->group = qobject_cast<QActionGroup *>(parent);
    if (d->group)
        d->group->addAction(this);
}

/*!
    Returns the parent widget.
*/
QWidget *QAction::parentWidget() const
{
    QObject *ret = parent();
    while (ret && !ret->isWidgetType())
        ret = ret->parent();
    return (QWidget*)ret;
}

/*!
  \since 4.2
  Returns a list of widgets this action has been added to.

  \sa QWidget::addAction(), associatedGraphicsWidgets()
*/
QList<QWidget *> QAction::associatedWidgets() const
{
    Q_D(const QAction);
    return d->widgets;
}

#if QT_CONFIG(graphicsview)
/*!
  \since 4.5
  Returns a list of widgets this action has been added to.

  \sa QWidget::addAction(), associatedWidgets()
*/
QList<QGraphicsWidget *> QAction::associatedGraphicsWidgets() const
{
    Q_D(const QAction);
    return d->graphicsWidgets;
}
#endif

#ifndef QT_NO_SHORTCUT
/*!
    \property QAction::shortcut
    \brief the action's primary shortcut key

    Valid keycodes for this property can be found in \l Qt::Key and
    \l Qt::Modifier. There is no default shortcut key.
*/
void QAction::setShortcut(const QKeySequence &shortcut)
{
    QAPP_CHECK("setShortcut");

    Q_D(QAction);
    if (d->shortcut == shortcut)
        return;

    d->shortcut = shortcut;
    d->redoGrab(qApp->d_func()->shortcutMap);
    d->sendDataChanged();
}

/*!
    \since 4.2

    Sets \a shortcuts as the list of shortcuts that trigger the
    action. The first element of the list is the primary shortcut.

    \sa shortcut
*/
void QAction::setShortcuts(const QList<QKeySequence> &shortcuts)
{
    Q_D(QAction);

    QList <QKeySequence> listCopy = shortcuts;

    QKeySequence primary;
    if (!listCopy.isEmpty())
        primary = listCopy.takeFirst();

    if (d->shortcut == primary && d->alternateShortcuts == listCopy)
        return;

    QAPP_CHECK("setShortcuts");

    d->shortcut = primary;
    d->alternateShortcuts = listCopy;
    d->redoGrab(qApp->d_func()->shortcutMap);
    d->redoGrabAlternate(qApp->d_func()->shortcutMap);
    d->sendDataChanged();
}

/*!
    \since 4.2

    Sets a platform dependent list of shortcuts based on the \a key.
    The result of calling this function will depend on the currently running platform.
    Note that more than one shortcut can assigned by this action.
    If only the primary shortcut is required, use setShortcut instead.

    \sa QKeySequence::keyBindings()
*/
void QAction::setShortcuts(QKeySequence::StandardKey key)
{
    QList <QKeySequence> list = QKeySequence::keyBindings(key);
    setShortcuts(list);
}

/*!
    Returns the primary shortcut.

    \sa setShortcuts()
*/
QKeySequence QAction::shortcut() const
{
    Q_D(const QAction);
    return d->shortcut;
}

/*!
    \since 4.2

    Returns the list of shortcuts, with the primary shortcut as
    the first element of the list.

    \sa setShortcuts()
*/
QList<QKeySequence> QAction::shortcuts() const
{
    Q_D(const QAction);
    QList <QKeySequence> shortcuts;
    if (!d->shortcut.isEmpty())
        shortcuts << d->shortcut;
    if (!d->alternateShortcuts.isEmpty())
        shortcuts << d->alternateShortcuts;
    return shortcuts;
}

/*!
    \property QAction::shortcutContext
    \brief the context for the action's shortcut

    Valid values for this property can be found in \l Qt::ShortcutContext.
    The default value is Qt::WindowShortcut.
*/
void QAction::setShortcutContext(Qt::ShortcutContext context)
{
    Q_D(QAction);
    if (d->shortcutContext == context)
        return;
    QAPP_CHECK("setShortcutContext");
    d->shortcutContext = context;
    d->redoGrab(qApp->d_func()->shortcutMap);
    d->redoGrabAlternate(qApp->d_func()->shortcutMap);
    d->sendDataChanged();
}

Qt::ShortcutContext QAction::shortcutContext() const
{
    Q_D(const QAction);
    return d->shortcutContext;
}

/*!
    \property QAction::autoRepeat
    \brief whether the action can auto repeat
    \since 4.2

    If true, the action will auto repeat when the keyboard shortcut
    combination is held down, provided that keyboard auto repeat is
    enabled on the system.
    The default value is true.
*/
void QAction::setAutoRepeat(bool on)
{
    Q_D(QAction);
    if (d->autorepeat == on)
        return;
    QAPP_CHECK("setAutoRepeat");
    d->autorepeat = on;
    d->redoGrab(qApp->d_func()->shortcutMap);
    d->redoGrabAlternate(qApp->d_func()->shortcutMap);
    d->sendDataChanged();
}

bool QAction::autoRepeat() const
{
    Q_D(const QAction);
    return d->autorepeat;
}
#endif // QT_NO_SHORTCUT

/*!
    \property QAction::font
    \brief the action's font

    The font property is used to render the text set on the
    QAction. The font will can be considered a hint as it will not be
    consulted in all cases based upon application and style.

    By default, this property contains the application's default font.

    \sa QAction::setText(), QStyle
*/
void QAction::setFont(const QFont &font)
{
    Q_D(QAction);
    if (d->font == font)
        return;

    d->fontSet = true;
    d->font = font;
    d->sendDataChanged();
}

QFont QAction::font() const
{
    Q_D(const QAction);
    return d->font;
}


/*!
    Destroys the object and frees allocated resources.
*/
QAction::~QAction()
{
    Q_D(QAction);
    for (int i = d->widgets.size()-1; i >= 0; --i) {
        QWidget *w = d->widgets.at(i);
        w->removeAction(this);
    }
#if QT_CONFIG(graphicsview)
    for (int i = d->graphicsWidgets.size()-1; i >= 0; --i) {
        QGraphicsWidget *w = d->graphicsWidgets.at(i);
        w->removeAction(this);
    }
#endif
    if (d->group)
        d->group->removeAction(this);
#ifndef QT_NO_SHORTCUT
    if (d->shortcutId && qApp) {
        qApp->d_func()->shortcutMap.removeShortcut(d->shortcutId, this);
        for(int i = 0; i < d->alternateShortcutIds.count(); ++i) {
            const int id = d->alternateShortcutIds.at(i);
            qApp->d_func()->shortcutMap.removeShortcut(id, this);
        }
    }
#endif
}

/*!
  Sets this action group to \a group. The action will be automatically
  added to the group's list of actions.

  Actions within the group will be mutually exclusive.

  \sa QActionGroup, QAction::actionGroup()
*/
void QAction::setActionGroup(QActionGroup *group)
{
    Q_D(QAction);
    if(group == d->group)
        return;

    if(d->group)
        d->group->removeAction(this);
    d->group = group;
    if(group)
        group->addAction(this);
    d->sendDataChanged();
}

/*!
  Returns the action group for this action. If no action group manages
  this action then 0 will be returned.

  \sa QActionGroup, QAction::setActionGroup()
*/
QActionGroup *QAction::actionGroup() const
{
    Q_D(const QAction);
    return d->group;
}


/*!
    \property QAction::icon
    \brief the action's icon

    In toolbars, the icon is used as the tool button icon; in menus,
    it is displayed to the left of the menu text. There is no default
    icon.

    If a null icon (QIcon::isNull()) is passed into this function,
    the icon of the action is cleared.
*/
void QAction::setIcon(const QIcon &icon)
{
    Q_D(QAction);
    d->icon = icon;
    d->sendDataChanged();
}

QIcon QAction::icon() const
{
    Q_D(const QAction);
    return d->icon;
}

#if QT_CONFIG(menu)
/*!
  Returns the menu contained by this action. Actions that contain
  menus can be used to create menu items with submenus, or inserted
  into toolbars to create buttons with popup menus.

  \sa QMenu::addAction()
*/
QMenu *QAction::menu() const
{
    Q_D(const QAction);
    return d->menu;
}

/*!
    Sets the menu contained by this action to the specified \a menu.
*/
void QAction::setMenu(QMenu *menu)
{
    Q_D(QAction);
    if (d->menu)
        d->menu->d_func()->setOverrideMenuAction(0); //we reset the default action of any previous menu
    d->menu = menu;
    if (menu)
        menu->d_func()->setOverrideMenuAction(this);
    d->sendDataChanged();
}
#endif // QT_CONFIG(menu)

/*!
  If \a b is true then this action will be considered a separator.

  How a separator is represented depends on the widget it is inserted
  into. Under most circumstances the text, submenu, and icon will be
  ignored for separator actions.

  \sa QAction::isSeparator()
*/
void QAction::setSeparator(bool b)
{
    Q_D(QAction);
    if (d->separator == b)
        return;

    d->separator = b;
    d->sendDataChanged();
}

/*!
  Returns \c true if this action is a separator action; otherwise it
  returns \c false.

  \sa QAction::setSeparator()
*/
bool QAction::isSeparator() const
{
    Q_D(const QAction);
    return d->separator;
}

/*!
    \property QAction::text
    \brief the action's descriptive text

    If the action is added to a menu, the menu option will consist of
    the icon (if there is one), the text, and the shortcut (if there
    is one). If the text is not explicitly set in the constructor, or
    by using setText(), the action's description icon text will be
    used as text. There is no default text.

    \sa iconText
*/
void QAction::setText(const QString &text)
{
    Q_D(QAction);
    if (d->text == text)
        return;

    d->text = text;
    d->sendDataChanged();
}

QString QAction::text() const
{
    Q_D(const QAction);
    QString s = d->text;
    if(s.isEmpty()) {
        s = d->iconText;
        s.replace(QLatin1Char('&'), QLatin1String("&&"));
    }
    return s;
}





/*!
    \property QAction::iconText
    \brief the action's descriptive icon text

    If QToolBar::toolButtonStyle is set to a value that permits text to
    be displayed, the text defined held in this property appears as a
    label in the relevant tool button.

    It also serves as the default text in menus and tooltips if the action
    has not been defined with setText() or setToolTip(), and will
    also be used in toolbar buttons if no icon has been defined using setIcon().

    If the icon text is not explicitly set, the action's normal text will be
    used for the icon text.

    By default, this property contains an empty string.

    \sa setToolTip(), setStatusTip()
*/
void QAction::setIconText(const QString &text)
{
    Q_D(QAction);
    if (d->iconText == text)
        return;

    d->iconText = text;
    d->sendDataChanged();
}

QString QAction::iconText() const
{
    Q_D(const QAction);
    if (d->iconText.isEmpty())
        return qt_strippedText(d->text);
    return d->iconText;
}

/*!
    \property QAction::toolTip
    \brief the action's tooltip

    This text is used for the tooltip. If no tooltip is specified,
    the action's text is used.

    By default, this property contains the action's text.

    \sa setStatusTip(), setShortcut()
*/
void QAction::setToolTip(const QString &tooltip)
{
    Q_D(QAction);
    if (d->tooltip == tooltip)
        return;

    d->tooltip = tooltip;
    d->sendDataChanged();
}

QString QAction::toolTip() const
{
    Q_D(const QAction);
    if (d->tooltip.isEmpty()) {
        if (!d->text.isEmpty())
            return qt_strippedText(d->text);
        return qt_strippedText(d->iconText);
    }
    return d->tooltip;
}

/*!
    \property QAction::statusTip
    \brief the action's status tip

    The status tip is displayed on all status bars provided by the
    action's top-level parent widget.

    By default, this property contains an empty string.

    \sa setToolTip(), showStatusText()
*/
void QAction::setStatusTip(const QString &statustip)
{
    Q_D(QAction);
    if (d->statustip == statustip)
        return;

    d->statustip = statustip;
    d->sendDataChanged();
}

QString QAction::statusTip() const
{
    Q_D(const QAction);
    return d->statustip;
}

/*!
    \property QAction::whatsThis
    \brief the action's "What's This?" help text

    The "What's This?" text is used to provide a brief description of
    the action. The text may contain rich text. There is no default
    "What's This?" text.

    \sa QWhatsThis
*/
void QAction::setWhatsThis(const QString &whatsthis)
{
    Q_D(QAction);
    if (d->whatsthis == whatsthis)
        return;

    d->whatsthis = whatsthis;
    d->sendDataChanged();
}

QString QAction::whatsThis() const
{
    Q_D(const QAction);
    return d->whatsthis;
}

/*!
    \enum QAction::Priority
    \since 4.6

    This enum defines priorities for actions in user interface.

    \value LowPriority The action should not be prioritized in
    the user interface.

    \value NormalPriority

    \value HighPriority The action should be prioritized in
    the user interface.

    \sa priority
*/


/*!
    \property QAction::priority
    \since 4.6

    \brief the actions's priority in the user interface.

    This property can be set to indicate how the action should be prioritized
    in the user interface.

    For instance, when toolbars have the Qt::ToolButtonTextBesideIcon
    mode set, then actions with LowPriority will not show the text
    labels.
*/
void QAction::setPriority(Priority priority)
{
    Q_D(QAction);
    if (d->priority == priority)
        return;

    d->priority = priority;
    d->sendDataChanged();
}

QAction::Priority QAction::priority() const
{
    Q_D(const QAction);
    return d->priority;
}

/*!
    \property QAction::checkable
    \brief whether the action is a checkable action

    A checkable action is one which has an on/off state. For example,
    in a word processor, a Bold toolbar button may be either on or
    off. An action which is not a toggle action is a command action;
    a command action is simply executed, e.g. file save.
    By default, this property is \c false.

    In some situations, the state of one toggle action should depend
    on the state of others. For example, "Left Align", "Center" and
    "Right Align" toggle actions are mutually exclusive. To achieve
    exclusive toggling, add the relevant toggle actions to a
    QActionGroup with the QActionGroup::exclusive property set to
    true.

    \sa QAction::setChecked()
*/
void QAction::setCheckable(bool b)
{
    Q_D(QAction);
    if (d->checkable == b)
        return;

    d->checkable = b;
    d->checked = false;
    d->sendDataChanged();
}

bool QAction::isCheckable() const
{
    Q_D(const QAction);
    return d->checkable;
}

/*!
    \fn void QAction::toggle()

    This is a convenience function for the \l checked property.
    Connect to it to change the checked state to its opposite state.
*/
void QAction::toggle()
{
    Q_D(QAction);
    setChecked(!d->checked);
}

/*!
    \property QAction::checked
    \brief whether the action is checked.

    Only checkable actions can be checked.  By default, this is false
    (the action is unchecked).

    \note The notifier signal for this property is toggled(). As toggling
    a QAction changes its state, it will also emit a changed() signal.

    \sa checkable, toggled()
*/
void QAction::setChecked(bool b)
{
    Q_D(QAction);
    if (!d->checkable || d->checked == b)
        return;

    QPointer<QAction> guard(this);
    d->checked = b;
    d->sendDataChanged();
    if (guard)
        emit toggled(b);
}

bool QAction::isChecked() const
{
    Q_D(const QAction);
    return d->checked;
}

/*!
    \fn void QAction::setDisabled(bool b)

    This is a convenience function for the \l enabled property, that
    is useful for signals--slots connections. If \a b is true the
    action is disabled; otherwise it is enabled.
*/

/*!
    \property QAction::enabled
    \brief whether the action is enabled

    Disabled actions cannot be chosen by the user. They do not
    disappear from menus or toolbars, but they are displayed in a way
    which indicates that they are unavailable. For example, they might
    be displayed using only shades of gray.

    \uicontrol{What's This?} help on disabled actions is still available, provided
    that the QAction::whatsThis property is set.

    An action will be disabled when all widgets to which it is added
    (with QWidget::addAction()) are disabled or not visible. When an
    action is disabled, it is not possible to trigger it through its
    shortcut.

    By default, this property is \c true (actions are enabled).

    \sa text
*/
void QAction::setEnabled(bool b)
{
    Q_D(QAction);
    if (b == d->enabled && b != d->forceDisabled)
        return;
    d->forceDisabled = !b;
    if (b && (!d->visible || (d->group && !d->group->isEnabled())))
        return;
    QAPP_CHECK("setEnabled");
    d->enabled = b;
#ifndef QT_NO_SHORTCUT
    d->setShortcutEnabled(b, qApp->d_func()->shortcutMap);
#endif
    d->sendDataChanged();
}

bool QAction::isEnabled() const
{
    Q_D(const QAction);
    return d->enabled;
}

/*!
    \property QAction::visible
    \brief whether the action can be seen (e.g. in menus and toolbars)

    If \e visible is true the action can be seen (e.g. in menus and
    toolbars) and chosen by the user; if \e visible is false the
    action cannot be seen or chosen by the user.

    Actions which are not visible are \e not grayed out; they do not
    appear at all.

    By default, this property is \c true (actions are visible).
*/
void QAction::setVisible(bool b)
{
    Q_D(QAction);
    if (b == d->visible && b != d->forceInvisible)
        return;
    QAPP_CHECK("setVisible");
    d->forceInvisible = !b;
    d->visible = b;
    d->enabled = b && !d->forceDisabled && (!d->group || d->group->isEnabled()) ;
#ifndef QT_NO_SHORTCUT
    d->setShortcutEnabled(d->enabled, qApp->d_func()->shortcutMap);
#endif
    d->sendDataChanged();
}


bool QAction::isVisible() const
{
    Q_D(const QAction);
    return d->visible;
}

/*!
  \reimp
*/
bool
QAction::event(QEvent *e)
{
#ifndef QT_NO_SHORTCUT
    if (e->type() == QEvent::Shortcut) {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        Q_ASSERT_X(se->key() == d_func()->shortcut || d_func()->alternateShortcuts.contains(se->key()),
                   "QAction::event",
                   "Received shortcut event from incorrect shortcut");
        if (se->isAmbiguous())
            qWarning("QAction::event: Ambiguous shortcut overload: %s", se->key().toString(QKeySequence::NativeText).toLatin1().constData());
        else
            activate(Trigger);
        return true;
    }
#endif
    return QObject::event(e);
}

/*!
  Returns the user data as set in QAction::setData.

  \sa setData()
*/
QVariant
QAction::data() const
{
    Q_D(const QAction);
    return d->userData;
}

/*!
  \fn void QAction::setData(const QVariant &userData)

  Sets the action's internal data to the given \a userData.

  \sa data()
*/
void
QAction::setData(const QVariant &data)
{
    Q_D(QAction);
    if (d->userData == data)
        return;
    d->userData = data;
    d->sendDataChanged();
}


/*!
  Updates the relevant status bar for the \a widget specified by sending a
  QStatusTipEvent to its parent widget. Returns \c true if an event was sent;
  otherwise returns \c false.

  If a null widget is specified, the event is sent to the action's parent.

  \sa statusTip
*/
bool
QAction::showStatusText(QWidget *widget)
{
    return d_func()->showStatusText(widget, statusTip());
}

/*!
  Sends the relevant signals for ActionEvent \a event.

  Action based widgets use this API to cause the QAction
  to emit signals as well as emitting their own.
*/
void QAction::activate(ActionEvent event)
{
    Q_D(QAction);
    if(event == Trigger) {
        QPointer<QObject> guard = this;
        if(d->checkable) {
            // the checked action of an exclusive group cannot be  unchecked
            if (d->checked && (d->group && d->group->isExclusive()
                               && d->group->checkedAction() == this)) {
                if (!guard.isNull())
                    emit triggered(true);
                return;
            }
            setChecked(!d->checked);
        }
        if (!guard.isNull())
            emit triggered(d->checked);
    } else if(event == Hover) {
        emit hovered();
    }
}

/*!
    \fn void QAction::triggered(bool checked)

    This signal is emitted when an action is activated by the user;
    for example, when the user clicks a menu option, toolbar button,
    or presses an action's shortcut key combination, or when trigger()
    was called. Notably, it is \e not emitted when setChecked() or
    toggle() is called.

    If the action is checkable, \a checked is true if the action is
    checked, or false if the action is unchecked.

    \sa QAction::activate(), QAction::toggled(), checked
*/

/*!
    \fn void QAction::toggled(bool checked)

    This signal is emitted whenever a checkable action changes its
    isChecked() status. This can be the result of a user interaction,
    or because setChecked() was called. As setChecked() changes the
    QAction, it emits changed() in addition to toggled().

    \a checked is true if the action is checked, or false if the
    action is unchecked.

    \sa QAction::activate(), QAction::triggered(), checked
*/

/*!
    \fn void QAction::hovered()

    This signal is emitted when an action is highlighted by the user;
    for example, when the user pauses with the cursor over a menu option,
    toolbar button, or presses an action's shortcut key combination.

    \sa QAction::activate()
*/

/*!
    \fn void QAction::changed()

    This signal is emitted when an action has changed. If you
    are only interested in actions in a given widget, you can
    watch for QWidget::actionEvent() sent with an
    QEvent::ActionChanged.

    \sa QWidget::actionEvent()
*/

/*!
    \enum QAction::ActionEvent

    This enum type is used when calling QAction::activate()

    \value Trigger this will cause the QAction::triggered() signal to be emitted.

    \value Hover this will cause the QAction::hovered() signal to be emitted.
*/

/*!
    \property QAction::menuRole
    \brief the action's menu role
    \since 4.2

    This indicates what role the action serves in the application menu on
    \macos. By default all actions have the TextHeuristicRole, which means that
    the action is added based on its text (see QMenuBar for more information).

    The menu role can only be changed before the actions are put into the menu
    bar in \macos (usually just before the first application window is
    shown).
*/
void QAction::setMenuRole(MenuRole menuRole)
{
    Q_D(QAction);
    if (d->menuRole == menuRole)
        return;

    d->menuRole = menuRole;
    d->sendDataChanged();
}

QAction::MenuRole QAction::menuRole() const
{
    Q_D(const QAction);
    return d->menuRole;
}

/*!
    \property QAction::iconVisibleInMenu
    \brief Whether or not an action should show an icon in a menu
    \since 4.4

    In some applications, it may make sense to have actions with icons in the
    toolbar, but not in menus. If true, the icon (if valid) is shown in the menu, when it
    is false, it is not shown.

    The default is to follow whether the Qt::AA_DontShowIconsInMenus attribute
    is set for the application. Explicitly settings this property overrides
    the presence (or abscence) of the attribute.

    For example:
    \snippet code/src_gui_kernel_qaction.cpp 0

    \sa QAction::icon, QCoreApplication::setAttribute()
*/
void QAction::setIconVisibleInMenu(bool visible)
{
    Q_D(QAction);
    if (d->iconVisibleInMenu == -1 || visible != bool(d->iconVisibleInMenu)) {
        int oldValue = d->iconVisibleInMenu;
        d->iconVisibleInMenu = visible;
        // Only send data changed if we really need to.
        if (oldValue != -1
            || visible == !QApplication::instance()->testAttribute(Qt::AA_DontShowIconsInMenus)) {
            d->sendDataChanged();
        }
    }
}

bool QAction::isIconVisibleInMenu() const
{
    Q_D(const QAction);
    if (d->iconVisibleInMenu == -1) {
        return !QApplication::instance()->testAttribute(Qt::AA_DontShowIconsInMenus);
    }
    return d->iconVisibleInMenu;
}

/*!
    \property QAction::shortcutVisibleInContextMenu
    \brief Whether or not an action should show a shortcut in a context menu
    \since 5.10

    In some applications, it may make sense to have actions with shortcuts in
    context menus. If true, the shortcut (if valid) is shown when the action is
    shown via a context menu, when it is false, it is not shown.

    The default is to follow whether the Qt::AA_DontShowShortcutsInContextMenus attribute
    is set for the application, falling back to the widget style hint.
    Explicitly setting this property overrides the presence (or abscence) of the attribute.

    \sa QAction::shortcut, QCoreApplication::setAttribute()
*/
void QAction::setShortcutVisibleInContextMenu(bool visible)
{
    Q_D(QAction);
    if (d->shortcutVisibleInContextMenu == -1 || visible != bool(d->shortcutVisibleInContextMenu)) {
        int oldValue = d->shortcutVisibleInContextMenu;
        d->shortcutVisibleInContextMenu = visible;
        // Only send data changed if we really need to.
        if (oldValue != -1
            || visible == !QApplication::instance()->testAttribute(Qt::AA_DontShowShortcutsInContextMenus)) {
            d->sendDataChanged();
        }
    }
}

bool QAction::isShortcutVisibleInContextMenu() const
{
    Q_D(const QAction);
    if (d->shortcutVisibleInContextMenu == -1) {
        return !QCoreApplication::testAttribute(Qt::AA_DontShowShortcutsInContextMenus)
            && QGuiApplication::styleHints()->showShortcutsInContextMenus();
    }
    return d->shortcutVisibleInContextMenu;
}

#ifndef QT_NO_DEBUG_STREAM
Q_WIDGETS_EXPORT QDebug operator<<(QDebug d, const QAction *action)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "QAction(" << static_cast<const void *>(action);
    if (action) {
        d << " text=" << action->text();
        if (!action->toolTip().isEmpty())
            d << " toolTip=" << action->toolTip();
        if (action->isCheckable())
            d << " checked=" << action->isChecked();
#ifndef QT_NO_SHORTCUT
        if (!action->shortcut().isEmpty())
            d << " shortcut=" << action->shortcut();
#endif
        d << " menuRole=";
        QtDebugUtils::formatQEnum(d, action->menuRole());
        d << " visible=" << action->isVisible();
    } else {
        d << '0';
    }
    d << ')';
    return d;
}
#endif // QT_NO_DEBUG_STREAM

QT_END_NAMESPACE

#include "moc_qaction.cpp"

#endif // QT_NO_ACTION
