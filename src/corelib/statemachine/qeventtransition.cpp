/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qeventtransition.h"
#include "qeventtransition_p.h"
#include "qstate.h"
#include "qstate_p.h"
#include "qstatemachine.h"
#include "qstatemachine_p.h"
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*!
  \class QEventTransition
  \inmodule QtCore

  \brief The QEventTransition class provides a QObject-specific transition for Qt events.

  \since 4.6
  \ingroup statemachine

  A QEventTransition object binds an event to a particular QObject.
  QEventTransition is part of \l{The State Machine Framework}.

  Example:

  \code
  QPushButton *button = ...;
  QState *s1 = ...;
  QState *s2 = ...;
  // If in s1 and the button receives an Enter event, transition to s2
  QEventTransition *enterTransition = new QEventTransition(button, QEvent::Enter);
  enterTransition->setTargetState(s2);
  s1->addTransition(enterTransition);
  // If in s2 and the button receives an Exit event, transition back to s1
  QEventTransition *leaveTransition = new QEventTransition(button, QEvent::Leave);
  leaveTransition->setTargetState(s1);
  s2->addTransition(leaveTransition);
  \endcode

  \section1 Subclassing

  When reimplementing the eventTest() function, you should first call the base
  implementation to verify that the event is a QStateMachine::WrappedEvent for
  the proper object and event type. You may then cast the event to a
  QStateMachine::WrappedEvent and get the original event by calling
  QStateMachine::WrappedEvent::event(), and perform additional checks on that
  object.

  \sa QState::addTransition()
*/

/*!
    \property QEventTransition::eventSource

    \brief the event source that this event transition is associated with
*/

/*!
    \property QEventTransition::eventType

    \brief the type of event that this event transition is associated with
*/
QEventTransitionPrivate::QEventTransitionPrivate()
{
    object = 0;
    eventType = QEvent::None;
    registered = false;
}

QEventTransitionPrivate::~QEventTransitionPrivate()
{
}

void QEventTransitionPrivate::unregister()
{
    Q_Q(QEventTransition);
    if (!registered || !machine())
        return;
    QStateMachinePrivate::get(machine())->unregisterEventTransition(q);
}

void QEventTransitionPrivate::maybeRegister()
{
    Q_Q(QEventTransition);
    if (QStateMachine *mach = machine())
        QStateMachinePrivate::get(mach)->maybeRegisterEventTransition(q);
}

/*!
  Constructs a new QEventTransition object with the given \a sourceState.
*/
QEventTransition::QEventTransition(QState *sourceState)
    : QAbstractTransition(*new QEventTransitionPrivate, sourceState)
{
}

/*!
  Constructs a new QEventTransition object associated with events of the given
  \a type for the given \a object, and with the given \a sourceState.
*/
QEventTransition::QEventTransition(QObject *object, QEvent::Type type,
                                   QState *sourceState)
    : QAbstractTransition(*new QEventTransitionPrivate, sourceState)
{
    Q_D(QEventTransition);
    d->registered = false;
    d->object = object;
    d->eventType = type;
    d->maybeRegister();
}

/*!
  \internal
*/
QEventTransition::QEventTransition(QEventTransitionPrivate &dd, QState *parent)
    : QAbstractTransition(dd, parent)
{
}

/*!
  \internal
*/
QEventTransition::QEventTransition(QEventTransitionPrivate &dd, QObject *object,
                                   QEvent::Type type, QState *parent)
    : QAbstractTransition(dd, parent)
{
    Q_D(QEventTransition);
    d->registered = false;
    d->object = object;
    d->eventType = type;
    d->maybeRegister();
}

/*!
  Destroys this QObject event transition.
*/
QEventTransition::~QEventTransition()
{
}

/*!
  Returns the event type that this event transition is associated with.
*/
QEvent::Type QEventTransition::eventType() const
{
    Q_D(const QEventTransition);
    return d->eventType;
}

/*!
  Sets the event \a type that this event transition is associated with.
*/
void QEventTransition::setEventType(QEvent::Type type)
{
    Q_D(QEventTransition);
    if (d->eventType == type)
        return;
    d->unregister();
    d->eventType = type;
    d->maybeRegister();
}

/*!
  Returns the event source associated with this event transition.
*/
QObject *QEventTransition::eventSource() const
{
    Q_D(const QEventTransition);
    return d->object;
}

/*!
  Sets the event source associated with this event transition to be the given
  \a object.
*/
void QEventTransition::setEventSource(QObject *object)
{
    Q_D(QEventTransition);
    if (d->object == object)
        return;
    d->unregister();
    d->object = object;
    d->maybeRegister();
}

/*!
  \reimp
*/
bool QEventTransition::eventTest(QEvent *event)
{
    Q_D(const QEventTransition);
    if (event->type() == QEvent::StateMachineWrapped) {
        QStateMachine::WrappedEvent *we = static_cast<QStateMachine::WrappedEvent*>(event);
        return (we->object() == d->object)
            && (we->event()->type() == d->eventType);
    }
    return false;
}

/*!
  \reimp
*/
void QEventTransition::onTransition(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  \reimp
*/
bool QEventTransition::event(QEvent *e)
{
    return QAbstractTransition::event(e);
}

QT_END_NAMESPACE

#include "moc_qeventtransition.cpp"
