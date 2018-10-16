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

#include <QtCore/qstatemachine.h>
#include <private/qstatemachine_p.h>
#include <QtGui/qevent.h>
#include <QtWidgets/qtwidgetsglobal.h>
#if QT_CONFIG(graphicsview)
#include <QtWidgets/qgraphicssceneevent.h>
#endif

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT const QStateMachinePrivate::Handler *qcoreStateMachineHandler();

static QEvent *cloneEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
        return new QMouseEvent(*static_cast<QMouseEvent*>(e));
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        return new QKeyEvent(*static_cast<QKeyEvent*>(e));
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        return new QFocusEvent(*static_cast<QFocusEvent*>(e));
    case QEvent::Enter:
        return new QEvent(*e);
    case QEvent::Leave:
        return new QEvent(*e);
    case QEvent::Paint:
        Q_ASSERT_X(false, "cloneEvent()", "not implemented");
        break;
    case QEvent::Move:
        return new QMoveEvent(*static_cast<QMoveEvent*>(e));
    case QEvent::Resize:
        return new QResizeEvent(*static_cast<QResizeEvent*>(e));
    case QEvent::Create:
        Q_ASSERT_X(false, "cloneEvent()", "not implemented");
        break;
    case QEvent::Destroy:
        Q_ASSERT_X(false, "cloneEvent()", "not implemented");
        break;
    case QEvent::Show:
        return new QShowEvent(*static_cast<QShowEvent*>(e));
    case QEvent::Hide:
        return new QHideEvent(*static_cast<QHideEvent*>(e));
    case QEvent::Close:
        return new QCloseEvent(*static_cast<QCloseEvent*>(e));
    case QEvent::Quit:
        return new QEvent(*e);
    case QEvent::ParentChange:
        return new QEvent(*e);
    case QEvent::ParentAboutToChange:
        return new QEvent(*e);
    case QEvent::ThreadChange:
        return new QEvent(*e);

    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        return new QEvent(*e);

    case QEvent::ShowToParent:
        return new QEvent(*e);
    case QEvent::HideToParent:
        return new QEvent(*e);
#if QT_CONFIG(wheelevent)
    case QEvent::Wheel:
        return new QWheelEvent(*static_cast<QWheelEvent*>(e));
#endif // QT_CONFIG(wheelevent)
    case QEvent::WindowTitleChange:
        return new QEvent(*e);
    case QEvent::WindowIconChange:
        return new QEvent(*e);
    case QEvent::ApplicationWindowIconChange:
        return new QEvent(*e);
    case QEvent::ApplicationFontChange:
        return new QEvent(*e);
    case QEvent::ApplicationLayoutDirectionChange:
        return new QEvent(*e);
    case QEvent::ApplicationPaletteChange:
        return new QEvent(*e);
    case QEvent::PaletteChange:
        return new QEvent(*e);
    case QEvent::Clipboard:
        Q_ASSERT_X(false, "cloneEvent()", "not implemented");
        break;
    case QEvent::Speech:
        Q_ASSERT_X(false, "cloneEvent()", "not implemented");
        break;
    case QEvent::MetaCall:
        Q_ASSERT_X(false, "cloneEvent()", "not implemented");
        break;
    case QEvent::SockAct:
        return new QEvent(*e);
    case QEvent::WinEventAct:
        return new QEvent(*e);
    case QEvent::DeferredDelete:
        return new QEvent(*e);
#if QT_CONFIG(draganddrop)
   case QEvent::DragEnter:
        return new QDragEnterEvent(*static_cast<QDragEnterEvent*>(e));
    case QEvent::DragMove:
        return new QDragMoveEvent(*static_cast<QDragMoveEvent*>(e));
    case QEvent::DragLeave:
        return new QDragLeaveEvent(*static_cast<QDragLeaveEvent*>(e));
    case QEvent::Drop:
        return new QDropEvent(*static_cast<QDragMoveEvent*>(e));
#endif
    case QEvent::ChildAdded:
        return new QChildEvent(*static_cast<QChildEvent*>(e));
    case QEvent::ChildPolished:
        return new QChildEvent(*static_cast<QChildEvent*>(e));
    case QEvent::ChildRemoved:
        return new QChildEvent(*static_cast<QChildEvent*>(e));
    case QEvent::ShowWindowRequest:
        return new QEvent(*e);
    case QEvent::PolishRequest:
        return new QEvent(*e);
    case QEvent::Polish:
        return new QEvent(*e);
    case QEvent::LayoutRequest:
        return new QEvent(*e);
    case QEvent::UpdateRequest:
        return new QEvent(*e);
    case QEvent::UpdateLater:
        return new QEvent(*e);

    case QEvent::EmbeddingControl:
        return new QEvent(*e);
    case QEvent::ActivateControl:
        return new QEvent(*e);
    case QEvent::DeactivateControl:
        return new QEvent(*e);

#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        return new QContextMenuEvent(*static_cast<QContextMenuEvent*>(e));
#endif
    case QEvent::InputMethod:
        return new QInputMethodEvent(*static_cast<QInputMethodEvent*>(e));
    case QEvent::LocaleChange:
        return new QEvent(*e);
    case QEvent::LanguageChange:
        return new QEvent(*e);
    case QEvent::LayoutDirectionChange:
        return new QEvent(*e);
    case QEvent::Style:
        return new QEvent(*e);
#if QT_CONFIG(tabletevent)
    case QEvent::TabletMove:
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
        return new QTabletEvent(*static_cast<QTabletEvent*>(e));
#endif // QT_CONFIG(tabletevent)
    case QEvent::OkRequest:
        return new QEvent(*e);
    case QEvent::HelpRequest:
        return new QEvent(*e);

    case QEvent::IconDrag:
        return new QIconDragEvent(*static_cast<QIconDragEvent*>(e));

    case QEvent::FontChange:
        return new QEvent(*e);
    case QEvent::EnabledChange:
        return new QEvent(*e);
    case QEvent::ActivationChange:
        return new QEvent(*e);
    case QEvent::StyleChange:
        return new QEvent(*e);
    case QEvent::IconTextChange:
        return new QEvent(*e);
    case QEvent::ModifiedChange:
        return new QEvent(*e);
    case QEvent::MouseTrackingChange:
        return new QEvent(*e);

    case QEvent::WindowBlocked:
        return new QEvent(*e);
    case QEvent::WindowUnblocked:
        return new QEvent(*e);
    case QEvent::WindowStateChange:
        return new QWindowStateChangeEvent(*static_cast<QWindowStateChangeEvent*>(e));

    case QEvent::ToolTip:
        return new QHelpEvent(*static_cast<QHelpEvent*>(e));
    case QEvent::WhatsThis:
        return new QHelpEvent(*static_cast<QHelpEvent*>(e));
#if QT_CONFIG(statustip)
    case QEvent::StatusTip:
        return new QStatusTipEvent(*static_cast<QStatusTipEvent*>(e));
#endif // QT_CONFIG(statustip)
#ifndef QT_NO_ACTION
    case QEvent::ActionChanged:
    case QEvent::ActionAdded:
    case QEvent::ActionRemoved:
        return new QActionEvent(*static_cast<QActionEvent*>(e));
#endif
    case QEvent::FileOpen:
        return new QFileOpenEvent(*static_cast<QFileOpenEvent*>(e));

#ifndef QT_NO_SHORTCUT
    case QEvent::Shortcut:
        return new QShortcutEvent(*static_cast<QShortcutEvent*>(e));
#endif //QT_NO_SHORTCUT
    case QEvent::ShortcutOverride:
        return new QKeyEvent(*static_cast<QKeyEvent*>(e));

#if QT_CONFIG(whatsthis)
    case QEvent::WhatsThisClicked:
        return new QWhatsThisClickedEvent(*static_cast<QWhatsThisClickedEvent*>(e));
#endif // QT_CONFIG(whatsthis)

#if QT_CONFIG(toolbar)
    case QEvent::ToolBarChange:
        return new QToolBarChangeEvent(*static_cast<QToolBarChangeEvent*>(e));
#endif // QT_CONFIG(toolbar)

    case QEvent::ApplicationActivate:
        return new QEvent(*e);
    case QEvent::ApplicationDeactivate:
        return new QEvent(*e);

    case QEvent::QueryWhatsThis:
        return new QHelpEvent(*static_cast<QHelpEvent*>(e));
    case QEvent::EnterWhatsThisMode:
        return new QEvent(*e);
    case QEvent::LeaveWhatsThisMode:
        return new QEvent(*e);

    case QEvent::ZOrderChange:
        return new QEvent(*e);

    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        return new QHoverEvent(*static_cast<QHoverEvent*>(e));

#ifdef QT_KEYPAD_NAVIGATION
    case QEvent::EnterEditFocus:
        return new QEvent(*e);
    case QEvent::LeaveEditFocus:
        return new QEvent(*e);
#endif
    case QEvent::AcceptDropsChange:
        return new QEvent(*e);

    case QEvent::ZeroTimerEvent:
        Q_ASSERT_X(false, "cloneEvent()", "not implemented");
        break;
#if QT_CONFIG(graphicsview)
    case QEvent::GraphicsSceneMouseMove:
    case QEvent::GraphicsSceneMousePress:
    case QEvent::GraphicsSceneMouseRelease:
    case QEvent::GraphicsSceneMouseDoubleClick: {
        QGraphicsSceneMouseEvent *me = static_cast<QGraphicsSceneMouseEvent*>(e);
        QGraphicsSceneMouseEvent *me2 = new QGraphicsSceneMouseEvent(me->type());
        me2->setWidget(me->widget());
        me2->setPos(me->pos());
        me2->setScenePos(me->scenePos());
        me2->setScreenPos(me->screenPos());
// ### for all buttons
        me2->setButtonDownPos(Qt::LeftButton, me->buttonDownPos(Qt::LeftButton));
        me2->setButtonDownPos(Qt::RightButton, me->buttonDownPos(Qt::RightButton));
        me2->setButtonDownScreenPos(Qt::LeftButton, me->buttonDownScreenPos(Qt::LeftButton));
        me2->setButtonDownScreenPos(Qt::RightButton, me->buttonDownScreenPos(Qt::RightButton));
        me2->setLastPos(me->lastPos());
        me2->setLastScenePos(me->lastScenePos());
        me2->setLastScreenPos(me->lastScreenPos());
        me2->setButtons(me->buttons());
        me2->setButton(me->button());
        me2->setModifiers(me->modifiers());
        me2->setSource(me->source());
        me2->setFlags(me->flags());
        return me2;
    }

    case QEvent::GraphicsSceneContextMenu: {
        QGraphicsSceneContextMenuEvent *me = static_cast<QGraphicsSceneContextMenuEvent*>(e);
        QGraphicsSceneContextMenuEvent *me2 = new QGraphicsSceneContextMenuEvent(me->type());
        me2->setWidget(me->widget());
        me2->setPos(me->pos());
        me2->setScenePos(me->scenePos());
        me2->setScreenPos(me->screenPos());
        me2->setModifiers(me->modifiers());
        me2->setReason(me->reason());
        return me2;
    }

    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHoverMove:
    case QEvent::GraphicsSceneHoverLeave: {
        QGraphicsSceneHoverEvent *he = static_cast<QGraphicsSceneHoverEvent*>(e);
        QGraphicsSceneHoverEvent *he2 = new QGraphicsSceneHoverEvent(he->type());
        he2->setPos(he->pos());
        he2->setScenePos(he->scenePos());
        he2->setScreenPos(he->screenPos());
        he2->setLastPos(he->lastPos());
        he2->setLastScenePos(he->lastScenePos());
        he2->setLastScreenPos(he->lastScreenPos());
        he2->setModifiers(he->modifiers());
        return he2;
    }
    case QEvent::GraphicsSceneHelp:
        return new QHelpEvent(*static_cast<QHelpEvent*>(e));
    case QEvent::GraphicsSceneDragEnter:
    case QEvent::GraphicsSceneDragMove:
    case QEvent::GraphicsSceneDragLeave:
    case QEvent::GraphicsSceneDrop: {
        QGraphicsSceneDragDropEvent *dde = static_cast<QGraphicsSceneDragDropEvent*>(e);
        QGraphicsSceneDragDropEvent *dde2 = new QGraphicsSceneDragDropEvent(dde->type());
        dde2->setPos(dde->pos());
        dde2->setScenePos(dde->scenePos());
        dde2->setScreenPos(dde->screenPos());
        dde2->setButtons(dde->buttons());
        dde2->setModifiers(dde->modifiers());
        return dde2;
    }
    case QEvent::GraphicsSceneWheel: {
        QGraphicsSceneWheelEvent *we = static_cast<QGraphicsSceneWheelEvent*>(e);
        QGraphicsSceneWheelEvent *we2 = new QGraphicsSceneWheelEvent(we->type());
        we2->setPos(we->pos());
        we2->setScenePos(we->scenePos());
        we2->setScreenPos(we->screenPos());
        we2->setButtons(we->buttons());
        we2->setModifiers(we->modifiers());
        we2->setOrientation(we->orientation());
        we2->setDelta(we->delta());
        return we2;
    }
#endif
    case QEvent::KeyboardLayoutChange:
        return new QEvent(*e);

    case QEvent::DynamicPropertyChange:
        return new QDynamicPropertyChangeEvent(*static_cast<QDynamicPropertyChangeEvent*>(e));

#if QT_CONFIG(tabletevent)
    case QEvent::TabletEnterProximity:
    case QEvent::TabletLeaveProximity:
        return new QTabletEvent(*static_cast<QTabletEvent*>(e));
#endif // QT_CONFIG(tabletevent)

    case QEvent::NonClientAreaMouseMove:
    case QEvent::NonClientAreaMouseButtonPress:
    case QEvent::NonClientAreaMouseButtonRelease:
    case QEvent::NonClientAreaMouseButtonDblClick:
        return new QMouseEvent(*static_cast<QMouseEvent*>(e));

    case QEvent::MacSizeChange:
        return new QEvent(*e);

    case QEvent::ContentsRectChange:
        return new QEvent(*e);

    case QEvent::MacGLWindowChange:
        return new QEvent(*e);

    case QEvent::FutureCallOut:
        Q_ASSERT_X(false, "cloneEvent()", "not implemented");
        break;
#if QT_CONFIG(graphicsview)
    case QEvent::GraphicsSceneResize: {
        QGraphicsSceneResizeEvent *re = static_cast<QGraphicsSceneResizeEvent*>(e);
        QGraphicsSceneResizeEvent *re2 = new QGraphicsSceneResizeEvent();
        re2->setOldSize(re->oldSize());
        re2->setNewSize(re->newSize());
        return re2;
    }
    case QEvent::GraphicsSceneMove: {
        QGraphicsSceneMoveEvent *me = static_cast<QGraphicsSceneMoveEvent*>(e);
        QGraphicsSceneMoveEvent *me2 = new QGraphicsSceneMoveEvent();
        me2->setWidget(me->widget());
        me2->setNewPos(me->newPos());
        me2->setOldPos(me->oldPos());
        return me2;
    }
#endif
    case QEvent::CursorChange:
        return new QEvent(*e);
    case QEvent::ToolTipChange:
        return new QEvent(*e);

    case QEvent::NetworkReplyUpdated:
        Q_ASSERT_X(false, "cloneEvent()", "not implemented");
        break;

    case QEvent::GrabMouse:
    case QEvent::UngrabMouse:
    case QEvent::GrabKeyboard:
    case QEvent::UngrabKeyboard:
        return new QEvent(*e);

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        return new QTouchEvent(*static_cast<QTouchEvent*>(e));

#ifndef QT_NO_GESTURES
    case QEvent::NativeGesture:
        Q_ASSERT_X(false, "cloneEvent()", "not implemented");
        break;
#endif

    case QEvent::User:
    case QEvent::MaxUser:
        Q_ASSERT_X(false, "cloneEvent()", "not implemented");
        break;
    default:
        ;
    }
    return qcoreStateMachineHandler()->cloneEvent(e);
}

const QStateMachinePrivate::Handler qt_gui_statemachine_handler = {
    cloneEvent
};

static const QStateMachinePrivate::Handler *qt_guistatemachine_last_handler = 0;
void qRegisterGuiStateMachine()
{
    qt_guistatemachine_last_handler = QStateMachinePrivate::handler;
    QStateMachinePrivate::handler = &qt_gui_statemachine_handler;
}
Q_CONSTRUCTOR_FUNCTION(qRegisterGuiStateMachine)

void qUnregisterGuiStateMachine()
{
    QStateMachinePrivate::handler = qt_guistatemachine_last_handler;
}
Q_DESTRUCTOR_FUNCTION(qUnregisterGuiStateMachine)

QT_END_NAMESPACE
