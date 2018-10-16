/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

//Own
#include "boat.h"
#include "boat_p.h"
#include "bomb.h"
#include "pixmapitem.h"
#include "graphicsscene.h"
#include "animationmanager.h"
#include "qanimationstate.h"

//Qt
#include <QtCore/QPropertyAnimation>
#include <QtCore/QStateMachine>
#include <QtCore/QHistoryState>
#include <QtCore/QFinalState>
#include <QtCore/QState>
#include <QtCore/QSequentialAnimationGroup>

static QAbstractAnimation *setupDestroyAnimation(Boat *boat)
{
    QSequentialAnimationGroup *group = new QSequentialAnimationGroup(boat);
    for (int i = 1; i <= 4; i++) {
        PixmapItem *step = new PixmapItem(QString("explosion/boat/step%1").arg(i),GraphicsScene::Big, boat);
        step->setZValue(6);
        step->setOpacity(0);

        //fade-in
        QPropertyAnimation *anim = new QPropertyAnimation(step, "opacity");
        anim->setEndValue(1);
        anim->setDuration(100);
        group->insertAnimation(i-1, anim);

        //and then fade-out
        QPropertyAnimation *anim2 = new QPropertyAnimation(step, "opacity");
        anim2->setEndValue(0);
        anim2->setDuration(100);
        group->addAnimation(anim2);
    }

    AnimationManager::self()->registerAnimation(group);
    return group;
}



Boat::Boat() : PixmapItem(QString("boat"), GraphicsScene::Big),
    speed(0), bombsAlreadyLaunched(0), direction(Boat::None), movementAnimation(0)
{
    setZValue(4);
    setFlags(QGraphicsItem::ItemIsFocusable);

    //The movement animation used to animate the boat
    movementAnimation = new QPropertyAnimation(this, "pos");

    //The destroy animation used to explode the boat
    destroyAnimation = setupDestroyAnimation(this);

    //We setup the state machine of the boat
    machine = new QStateMachine(this);
    QState *moving = new QState(machine);
    StopState *stopState = new StopState(this, moving);
    machine->setInitialState(moving);
    moving->setInitialState(stopState);
    MoveStateRight *moveStateRight = new MoveStateRight(this, moving);
    MoveStateLeft *moveStateLeft = new MoveStateLeft(this, moving);
    LaunchStateRight *launchStateRight = new LaunchStateRight(this, machine);
    LaunchStateLeft *launchStateLeft = new LaunchStateLeft(this, machine);

    //then setup the transitions for the rightMove state
    KeyStopTransition *leftStopRight = new KeyStopTransition(this, QEvent::KeyPress, Qt::Key_Left);
    leftStopRight->setTargetState(stopState);
    KeyMoveTransition *leftMoveRight = new KeyMoveTransition(this, QEvent::KeyPress, Qt::Key_Left);
    leftMoveRight->setTargetState(moveStateRight);
    KeyMoveTransition *rightMoveRight = new KeyMoveTransition(this, QEvent::KeyPress, Qt::Key_Right);
    rightMoveRight->setTargetState(moveStateRight);
    KeyMoveTransition *rightMoveStop = new KeyMoveTransition(this, QEvent::KeyPress, Qt::Key_Right);
    rightMoveStop->setTargetState(moveStateRight);

    //then setup the transitions for the leftMove state
    KeyStopTransition *rightStopLeft = new KeyStopTransition(this, QEvent::KeyPress, Qt::Key_Right);
    rightStopLeft->setTargetState(stopState);
    KeyMoveTransition *rightMoveLeft = new KeyMoveTransition(this, QEvent::KeyPress, Qt::Key_Right);
    rightMoveLeft->setTargetState(moveStateLeft);
    KeyMoveTransition *leftMoveLeft = new KeyMoveTransition(this, QEvent::KeyPress,Qt::Key_Left);
    leftMoveLeft->setTargetState(moveStateLeft);
    KeyMoveTransition *leftMoveStop = new KeyMoveTransition(this, QEvent::KeyPress,Qt::Key_Left);
    leftMoveStop->setTargetState(moveStateLeft);

    //We set up the right move state
    moveStateRight->addTransition(leftStopRight);
    moveStateRight->addTransition(leftMoveRight);
    moveStateRight->addTransition(rightMoveRight);
    stopState->addTransition(rightMoveStop);

    //We set up the left move state
    moveStateLeft->addTransition(rightStopLeft);
    moveStateLeft->addTransition(leftMoveLeft);
    moveStateLeft->addTransition(rightMoveLeft);
    stopState->addTransition(leftMoveStop);

    //The animation is finished, it means we reached the border of the screen, the boat is stopped so we move to the stop state
    moveStateLeft->addTransition(movementAnimation, SIGNAL(finished()), stopState);
    moveStateRight->addTransition(movementAnimation, SIGNAL(finished()), stopState);

    //We set up the keys for dropping bombs
    KeyLaunchTransition *upFireLeft = new KeyLaunchTransition(this, QEvent::KeyPress, Qt::Key_Up);
    upFireLeft->setTargetState(launchStateRight);
    KeyLaunchTransition *upFireRight = new KeyLaunchTransition(this, QEvent::KeyPress, Qt::Key_Up);
    upFireRight->setTargetState(launchStateRight);
    KeyLaunchTransition *upFireStop = new KeyLaunchTransition(this, QEvent::KeyPress, Qt::Key_Up);
    upFireStop->setTargetState(launchStateRight);
    KeyLaunchTransition *downFireLeft = new KeyLaunchTransition(this, QEvent::KeyPress, Qt::Key_Down);
    downFireLeft->setTargetState(launchStateLeft);
    KeyLaunchTransition *downFireRight = new KeyLaunchTransition(this, QEvent::KeyPress, Qt::Key_Down);
    downFireRight->setTargetState(launchStateLeft);
    KeyLaunchTransition *downFireMove = new KeyLaunchTransition(this, QEvent::KeyPress, Qt::Key_Down);
    downFireMove->setTargetState(launchStateLeft);

    //We set up transitions for fire up
    moveStateRight->addTransition(upFireRight);
    moveStateLeft->addTransition(upFireLeft);
    stopState->addTransition(upFireStop);

    //We set up transitions for fire down
    moveStateRight->addTransition(downFireRight);
    moveStateLeft->addTransition(downFireLeft);
    stopState->addTransition(downFireMove);

    //Finally the launch state should come back to its original state
    QHistoryState *historyState = new QHistoryState(moving);
    launchStateLeft->addTransition(historyState);
    launchStateRight->addTransition(historyState);

    QFinalState *final = new QFinalState(machine);

    //This state play the destroyed animation
    QAnimationState *destroyedState = new QAnimationState(machine);
    destroyedState->setAnimation(destroyAnimation);

    //Play a nice animation when the boat is destroyed
    moving->addTransition(this, SIGNAL(boatDestroyed()), destroyedState);

    //Transition to final state when the destroyed animation is finished
    destroyedState->addTransition(destroyedState, SIGNAL(animationFinished()), final);

    //The machine has finished to be executed, then the boat is dead
    connect(machine,SIGNAL(finished()), this, SIGNAL(boatExecutionFinished()));

}

void Boat::run()
{
    //We register animations
    AnimationManager::self()->registerAnimation(movementAnimation);
    AnimationManager::self()->registerAnimation(destroyAnimation);
    machine->start();
}

void Boat::stop()
{
    movementAnimation->stop();
    machine->stop();
}

void Boat::updateBoatMovement()
{
    if (speed == 0 || direction == Boat::None) {
        movementAnimation->stop();
        return;
    }

    movementAnimation->stop();

    if (direction == Boat::Left) {
        movementAnimation->setEndValue(QPointF(0,y()));
        movementAnimation->setDuration(x()/speed*15);
    }
    else /*if (direction == Boat::Right)*/ {
        movementAnimation->setEndValue(QPointF(scene()->width()-size().width(),y()));
        movementAnimation->setDuration((scene()->width()-size().width()-x())/speed*15);
    }
    movementAnimation->start();
}

void Boat::destroy()
{
    movementAnimation->stop();
    emit boatDestroyed();
}

int Boat::bombsLaunched() const
{
    return bombsAlreadyLaunched;
}

void Boat::setBombsLaunched(int number)
{
    if (number > MAX_BOMB) {
        qWarning("Boat::setBombsLaunched : It impossible to launch that number of bombs");
        return;
    }
    bombsAlreadyLaunched = number;
}

int Boat::currentSpeed() const
{
    return speed;
}

void Boat::setCurrentSpeed(int speed)
{
    if (speed > 3 || speed < 0) {
        qWarning("Boat::setCurrentSpeed: The boat can't run on that speed");
        return;
    }
    this->speed = speed;
}

enum Boat::Movement Boat::currentDirection() const
{
    return direction;
}

void Boat::setCurrentDirection(Movement direction)
{
    this->direction = direction;
}

int Boat::type() const
{
    return Type;
}
