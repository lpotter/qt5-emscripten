/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qaccessiblemenu_p.h"

#if QT_CONFIG(menu)
#include <qmenu.h>
#endif
#if QT_CONFIG(menubar)
#include <qmenubar.h>
#endif
#include <QtWidgets/QAction>
#include <qstyle.h>

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

#if QT_CONFIG(menu)

QString qt_accStripAmp(const QString &text);
QString qt_accHotKey(const QString &text);

QAccessibleInterface *getOrCreateMenu(QWidget *menu, QAction *action)
{
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(action);
    if (!iface) {
        iface = new QAccessibleMenuItem(menu, action);
        QAccessible::registerAccessibleInterface(iface);
    }
    return iface;
}

QAccessibleMenu::QAccessibleMenu(QWidget *w)
: QAccessibleWidget(w)
{
    Q_ASSERT(menu());
}

QMenu *QAccessibleMenu::menu() const
{
    return qobject_cast<QMenu*>(object());
}

int QAccessibleMenu::childCount() const
{
    return menu()->actions().count();
}

QAccessibleInterface *QAccessibleMenu::childAt(int x, int y) const
{
    QAction *act = menu()->actionAt(menu()->mapFromGlobal(QPoint(x,y)));
    if(act && act->isSeparator())
        act = 0;
    return act ? getOrCreateMenu(menu(), act) : 0;
}

QString QAccessibleMenu::text(QAccessible::Text t) const
{
    QString tx = QAccessibleWidget::text(t);
    if (!tx.isEmpty())
        return tx;

    if (t == QAccessible::Name)
        return menu()->windowTitle();
    return tx;
}

QAccessible::Role QAccessibleMenu::role() const
{
    return QAccessible::PopupMenu;
}

QAccessibleInterface *QAccessibleMenu::child(int index) const
{
    if (index < childCount())
        return getOrCreateMenu(menu(), menu()->actions().at(index));
    return 0;
}

QAccessibleInterface *QAccessibleMenu::parent() const
{
    if (QAction *menuAction = menu()->menuAction()) {
        const QList<QWidget*> parentCandidates =
                QList<QWidget*>() << menu()->parentWidget() << menuAction->associatedWidgets();
        for (QWidget *w : parentCandidates) {
            if (qobject_cast<QMenu*>(w)
#if QT_CONFIG(menubar)
                || qobject_cast<QMenuBar*>(w)
#endif
                ) {
                if (w->actions().indexOf(menuAction) != -1)
                    return getOrCreateMenu(w, menuAction);
            }
        }
    }
    return QAccessibleWidget::parent();
}

int QAccessibleMenu::indexOfChild( const QAccessibleInterface *child) const
{
    QAccessible::Role r = child->role();
    if ((r == QAccessible::MenuItem || r == QAccessible::Separator) && menu()) {
        return menu()->actions().indexOf(qobject_cast<QAction*>(child->object()));
    }
    return -1;
}

#if QT_CONFIG(menubar)
QAccessibleMenuBar::QAccessibleMenuBar(QWidget *w)
    : QAccessibleWidget(w, QAccessible::MenuBar)
{
    Q_ASSERT(menuBar());
}

QMenuBar *QAccessibleMenuBar::menuBar() const
{
    return qobject_cast<QMenuBar*>(object());
}

int QAccessibleMenuBar::childCount() const
{
    return menuBar()->actions().count();
}

QAccessibleInterface *QAccessibleMenuBar::child(int index) const
{
    if (index < childCount()) {
        return getOrCreateMenu(menuBar(), menuBar()->actions().at(index));
    }
    return 0;
}

int QAccessibleMenuBar::indexOfChild(const QAccessibleInterface *child) const
{
    QAccessible::Role r = child->role();
    if ((r == QAccessible::MenuItem || r == QAccessible::Separator) && menuBar()) {
        return menuBar()->actions().indexOf(qobject_cast<QAction*>(child->object()));
    }
    return -1;
}

#endif // QT_CONFIG(menubar)

QAccessibleMenuItem::QAccessibleMenuItem(QWidget *owner, QAction *action)
: m_action(action), m_owner(owner)
{
}

QAccessibleMenuItem::~QAccessibleMenuItem()
{}

QAccessibleInterface *QAccessibleMenuItem::childAt(int x, int y ) const
{
    for (int i = childCount() - 1; i >= 0; --i) {
        QAccessibleInterface *childInterface = child(i);
        if (childInterface->rect().contains(x,y)) {
            return childInterface;
        }
    }
    return 0;
}

int QAccessibleMenuItem::childCount() const
{
    return m_action->menu() ? 1 : 0;
}

int QAccessibleMenuItem::indexOfChild(const QAccessibleInterface * child) const
{
    if (child && child->role() == QAccessible::PopupMenu && child->object() == m_action->menu())
        return 0;
    return -1;
}

bool QAccessibleMenuItem::isValid() const
{
    return m_action && m_owner;
}

QAccessibleInterface *QAccessibleMenuItem::parent() const
{
    return QAccessible::queryAccessibleInterface(owner());
}

QAccessibleInterface *QAccessibleMenuItem::child(int index) const
{
    if (index == 0 && action()->menu())
        return QAccessible::queryAccessibleInterface(action()->menu());
    return 0;
}

void *QAccessibleMenuItem::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ActionInterface)
        return static_cast<QAccessibleActionInterface*>(this);
    return 0;
}

QObject *QAccessibleMenuItem::object() const
{
    return m_action;
}

/*! \reimp */
QWindow *QAccessibleMenuItem::window() const
{
    QWindow *result = nullptr;
    if (!m_owner.isNull()) {
        result = m_owner->windowHandle();
        if (!result) {
            if (const QWidget *nativeParent = m_owner->nativeParentWidget())
                result = nativeParent->windowHandle();
        }
    }
    return result;
}

QRect QAccessibleMenuItem::rect() const
{
    QRect rect;
    QWidget *own = owner();
#if QT_CONFIG(menubar)
    if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(own)) {
        rect = menuBar->actionGeometry(m_action);
        QPoint globalPos = menuBar->mapToGlobal(QPoint(0,0));
        rect = rect.translated(globalPos);
    } else
#endif // QT_CONFIG(menubar)
    if (QMenu *menu = qobject_cast<QMenu*>(own)) {
        rect = menu->actionGeometry(m_action);
        QPoint globalPos = menu->mapToGlobal(QPoint(0,0));
        rect = rect.translated(globalPos);
    }
    return rect;
}

QAccessible::Role QAccessibleMenuItem::role() const
{
    return m_action->isSeparator() ? QAccessible::Separator : QAccessible::MenuItem;
}

void QAccessibleMenuItem::setText(QAccessible::Text /*t*/, const QString & /*text */)
{
}

QAccessible::State QAccessibleMenuItem::state() const
{
    QAccessible::State s;
    QWidget *own = owner();

    if (own && (own->testAttribute(Qt::WA_WState_Visible) == false || m_action->isVisible() == false)) {
        s.invisible = true;
    }

    if (QMenu *menu = qobject_cast<QMenu*>(own)) {
        if (menu->activeAction() == m_action)
            s.focused = true;
#if QT_CONFIG(menubar)
    } else if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(own)) {
        if (menuBar->activeAction() == m_action)
            s.focused = true;
#endif
    }
    if (own && own->style()->styleHint(QStyle::SH_Menu_MouseTracking))
        s.hotTracked = true;
    if (m_action->isSeparator() || !m_action->isEnabled())
        s.disabled = true;
    if (m_action->isChecked())
        s.checked = true;

    return s;
}

QString QAccessibleMenuItem::text(QAccessible::Text t) const
{
    QString str;
    switch (t) {
    case QAccessible::Name:
        str = qt_accStripAmp(m_action->text());
        break;
    case QAccessible::Accelerator: {
#ifndef QT_NO_SHORTCUT
        QKeySequence key = m_action->shortcut();
        if (!key.isEmpty()) {
            str = key.toString();
        } else
#endif
        {
            str = qt_accHotKey(m_action->text());
        }
        break;
    }
    default:
        break;
    }
    return str;
}

QStringList QAccessibleMenuItem::actionNames() const
{
    QStringList actions;
    if (!m_action || m_action->isSeparator())
        return actions;

    if (m_action->menu()) {
        actions << showMenuAction();
    } else {
        actions << pressAction();
    }
    return actions;
}

void QAccessibleMenuItem::doAction(const QString &actionName)
{
    if (!m_action->isEnabled())
        return;

    if (actionName == pressAction()) {
        m_action->trigger();
    } else if (actionName == showMenuAction()) {
#if QT_CONFIG(menubar)
        if (QMenuBar *bar = qobject_cast<QMenuBar*>(owner())) {
            if (m_action->menu() && m_action->menu()->isVisible()) {
                m_action->menu()->hide();
            } else {
                bar->setActiveAction(m_action);
            }
        } else
#endif
          if (QMenu *menu = qobject_cast<QMenu*>(owner())){
            if (m_action->menu() && m_action->menu()->isVisible()) {
                m_action->menu()->hide();
            } else {
                menu->setActiveAction(m_action);
            }
        }
    }
}

QStringList QAccessibleMenuItem::keyBindingsForAction(const QString &) const
{
    return QStringList();
}


QAction *QAccessibleMenuItem::action() const
{
    return m_action;
}

QWidget *QAccessibleMenuItem::owner() const
{
    return m_owner;
}

#endif // QT_CONFIG(menu)

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY

