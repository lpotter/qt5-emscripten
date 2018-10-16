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

#include "qmouseeventtransition.h"
#include "qbasicmouseeventtransition_p.h"
#include <QtCore/qstatemachine.h>
#include <QtGui/qpainterpath.h>
#include <private/qeventtransition_p.h>

QT_BEGIN_NAMESPACE

/*!
  \class QMouseEventTransition

  \brief The QMouseEventTransition class provides a transition for mouse events.

  \since 4.6
  \ingroup statemachine
  \inmodule QtWidgets

  QMouseEventTransition is part of \l{The State Machine Framework}.

  \sa QState::addTransition()
*/

/*!
    \property QMouseEventTransition::button

    \brief the button that this mouse event transition is associated with
*/

/*!
    \property QMouseEventTransition::modifierMask

    \brief the keyboard modifier mask that this mouse event transition checks for
*/

class QMouseEventTransitionPrivate : public QEventTransitionPrivate
{
    Q_DECLARE_PUBLIC(QMouseEventTransition)
public:
    QMouseEventTransitionPrivate();

    QBasicMouseEventTransition *transition;
};

QMouseEventTransitionPrivate::QMouseEventTransitionPrivate()
{
}

/*!
  Constructs a new mouse event transition with the given \a sourceState.
*/
QMouseEventTransition::QMouseEventTransition(QState *sourceState)
    : QEventTransition(*new QMouseEventTransitionPrivate, sourceState)
{
    Q_D(QMouseEventTransition);
    d->transition = new QBasicMouseEventTransition();
}

/*!
  Constructs a new mouse event transition for events of the given \a type for
  the given \a object, with the given \a button and \a sourceState.
*/
QMouseEventTransition::QMouseEventTransition(QObject *object, QEvent::Type type,
                                             Qt::MouseButton button,
                                             QState *sourceState)
    : QEventTransition(*new QMouseEventTransitionPrivate, object, type, sourceState)
{
    Q_D(QMouseEventTransition);
    d->transition = new QBasicMouseEventTransition(type, button);
}

/*!
  Destroys this mouse event transition.
*/
QMouseEventTransition::~QMouseEventTransition()
{
    Q_D(QMouseEventTransition);
    delete d->transition;
}

/*!
  Returns the button that this mouse event transition checks for.
*/
Qt::MouseButton QMouseEventTransition::button() const
{
    Q_D(const QMouseEventTransition);
    return d->transition->button();
}

/*!
  Sets the \a button that this mouse event transition will check for.
*/
void QMouseEventTransition::setButton(Qt::MouseButton button)
{
    Q_D(QMouseEventTransition);
    d->transition->setButton(button);
}

/*!
  Returns the keyboard modifier mask that this mouse event transition checks
  for.
*/
Qt::KeyboardModifiers QMouseEventTransition::modifierMask() const
{
    Q_D(const QMouseEventTransition);
    return d->transition->modifierMask();
}

/*!
  Sets the keyboard modifier mask that this mouse event transition will
  check for to \a modifierMask.
*/
void QMouseEventTransition::setModifierMask(Qt::KeyboardModifiers modifierMask)
{
    Q_D(QMouseEventTransition);
    d->transition->setModifierMask(modifierMask);
}

/*!
  Returns the hit test path for this mouse event transition.
*/
QPainterPath QMouseEventTransition::hitTestPath() const
{
    Q_D(const QMouseEventTransition);
    return d->transition->hitTestPath();
}

/*!
  Sets the hit test path for this mouse event transition to \a path.
  If a valid path has been set, the transition will only trigger if the mouse
  event position (QMouseEvent::pos()) is inside the path.

  \sa QPainterPath::contains()
*/
void QMouseEventTransition::setHitTestPath(const QPainterPath &path)
{
    Q_D(QMouseEventTransition);
    d->transition->setHitTestPath(path);
}

/*!
  \reimp
*/
bool QMouseEventTransition::eventTest(QEvent *event)
{
    Q_D(const QMouseEventTransition);
    if (!QEventTransition::eventTest(event))
        return false;
    QStateMachine::WrappedEvent *we = static_cast<QStateMachine::WrappedEvent*>(event);
    d->transition->setEventType(we->event()->type());
    return QAbstractTransitionPrivate::get(d->transition)->callEventTest(we->event());
}

/*!
  \reimp
*/
void QMouseEventTransition::onTransition(QEvent *event)
{
    QEventTransition::onTransition(event);
}

QT_END_NAMESPACE

#include "moc_qmouseeventtransition.cpp"
