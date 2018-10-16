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

#include "qgesture.h"
#include "private/qgesture_p.h"
#include "private/qstandardgestures_p.h"
#if QT_CONFIG(graphicsview)
#include "qgraphicsview.h"
#endif

#include <private/qdebug_p.h>
#ifndef QT_NO_GESTURES

QT_BEGIN_NAMESPACE

 /*!
    \class QGesture
    \since 4.6
    \ingroup gestures
    \inmodule QtWidgets

    \brief The QGesture class represents a gesture, containing properties that
    describe the corresponding user input.

    Gesture objects are not constructed directly by developers. They are created by
    the QGestureRecognizer object that is registered with the application; see
    QGestureRecognizer::registerRecognizer().

    For an overview of gesture handling in Qt and information on using gestures
    in your applications, see the \l{Gestures in Widgets and Graphics View} document.

    \section1 Gesture Properties

    The class has a list of properties that can be queried by the user to get
    some gesture-specific arguments. For example, the pinch gesture has a scale
    factor that is exposed as a property.

    Developers of custom gesture recognizers can add additional properties in
    order to provide additional information about a gesture. This can be done
    by adding new dynamic properties to a QGesture object, or by subclassing
    the QGesture class (or one of its subclasses).

    \section1 Lifecycle of a Gesture Object

    A QGesture instance is implicitly created when needed and is owned by Qt.
    Developers should never destroy them or store them for later use as Qt may
    destroy particular instances of them and create new ones to replace them.

    The registered gesture recognizer monitors the input events for the target
    object via its \l{QGestureRecognizer::}{recognize()} function, updating the
    properties of the gesture object as required.

    The gesture object may be delivered to the target object in a QGestureEvent if
    the corresponding gesture is active or has just been canceled. Each event that
    is delivered contains a list of gesture objects, since support for more than
    one gesture may be enabled for the target object. Due to the way events are
    handled in Qt, gesture events may be filtered by other objects.

    \sa QGestureEvent, QGestureRecognizer
*/

/*!
    Constructs a new gesture object with the given \a parent.

    QGesture objects are created by gesture recognizers in the
    QGestureRecognizer::create() function.
*/
QGesture::QGesture(QObject *parent)
    : QObject(*new QGesturePrivate, parent)
{
    d_func()->gestureType = Qt::CustomGesture;
}

/*!
    \internal
*/
QGesture::QGesture(QGesturePrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
}

/*!
    Destroys the gesture object.
*/
QGesture::~QGesture()
{
}

/*!
    \property QGesture::state
    \brief the current state of the gesture
*/

/*!
    \property QGesture::gestureType
    \brief the type of the gesture
*/

/*!
    \property QGesture::hotSpot

    \brief The point that is used to find the receiver for the gesture event.

    The hot-spot is a point in the global coordinate system, use
    QWidget::mapFromGlobal() or QGestureEvent::mapToGraphicsScene() to get a
    local hot-spot.

    The hot-spot should be set by the gesture recognizer to allow gesture event
    delivery to a QGraphicsObject.
*/

/*!
    \property QGesture::hasHotSpot
    \brief whether the gesture has a hot-spot
*/

Qt::GestureType QGesture::gestureType() const
{
    return d_func()->gestureType;
}

Qt::GestureState QGesture::state() const
{
    return d_func()->state;
}

QPointF QGesture::hotSpot() const
{
    return d_func()->hotSpot;
}

void QGesture::setHotSpot(const QPointF &value)
{
    Q_D(QGesture);
    d->hotSpot = value;
    d->isHotSpotSet = true;
}

bool QGesture::hasHotSpot() const
{
    return d_func()->isHotSpotSet;
}

void QGesture::unsetHotSpot()
{
    d_func()->isHotSpotSet = false;
}

/*!
    \property QGesture::gestureCancelPolicy
    \brief the policy for deciding what happens on accepting a gesture

    On accepting one gesture Qt can automatically cancel other gestures
    that belong to other targets. The policy is normally set to not cancel
    any other gestures and can be set to cancel all active gestures in the
    context. For example for all child widgets.
*/

/*!
    \enum QGesture::GestureCancelPolicy

    This enum describes how accepting a gesture can cancel other gestures
    automatically.

    \value CancelNone On accepting this gesture no other gestures will be affected.

    \value CancelAllInContext On accepting this gesture all gestures that are
    active in the context (respecting the Qt::GestureFlag that were specified
    when subscribed to the gesture) will be cancelled.
*/

void QGesture::setGestureCancelPolicy(GestureCancelPolicy policy)
{
    Q_D(QGesture);
    d->gestureCancelPolicy = static_cast<uint>(policy);
}

QGesture::GestureCancelPolicy QGesture::gestureCancelPolicy() const
{
    Q_D(const QGesture);
    return static_cast<GestureCancelPolicy>(d->gestureCancelPolicy);
}

/*!
    \class QPanGesture
    \since 4.6
    \brief The QPanGesture class describes a panning gesture made by the user.
    \ingroup gestures
    \inmodule QtWidgets

    \image pangesture.png

    For an overview of gesture handling in Qt and information on using gestures
    in your applications, see the \l{Gestures in Widgets and Graphics View} document.

    \sa QPinchGesture, QSwipeGesture
*/

/*!
    \property QPanGesture::lastOffset
    \brief the last offset recorded for this gesture

    The last offset contains the change in position of the user's input as
    reported in the \l offset property when a previous gesture event was
    delivered for this gesture.

    If no previous event was delivered with information about this gesture
    (i.e., this gesture object contains information about the first movement
    in the gesture) then this property contains a zero size.
*/

/*!
    \property QPanGesture::offset
    \brief the total offset from the first input position to the current input
    position

    The offset measures the total change in position of the user's input
    covered by the gesture on the input device.
*/

/*!
    \property QPanGesture::delta
    \brief the offset from the previous input position to the current input

    This is essentially the same as the difference between offset() and
    lastOffset().
*/

/*!
    \property QPanGesture::acceleration
    \brief the acceleration in the motion of the touch point for this gesture
*/

/*!
    \property QPanGesture::horizontalVelocity
    \brief the horizontal component of the motion of the touch point for this
    gesture
    \since 4.7.1
    \internal

    \sa verticalVelocity, acceleration
*/

/*!
    \property QPanGesture::verticalVelocity
    \brief the vertical component of the motion of the touch point for this
    gesture
    \since 4.7.1
    \internal

    \sa horizontalVelocity, acceleration
*/

/*!
    \internal
*/
QPanGesture::QPanGesture(QObject *parent)
    : QGesture(*new QPanGesturePrivate, parent)
{
    d_func()->gestureType = Qt::PanGesture;
}

/*!
    Destructor.
*/
QPanGesture::~QPanGesture()
{
}

QPointF QPanGesture::lastOffset() const
{
    return d_func()->lastOffset;
}

QPointF QPanGesture::offset() const
{
    return d_func()->offset;
}

QPointF QPanGesture::delta() const
{
    Q_D(const QPanGesture);
    return d->offset - d->lastOffset;
}

qreal QPanGesture::acceleration() const
{
    return d_func()->acceleration;
}

void QPanGesture::setLastOffset(const QPointF &value)
{
    d_func()->lastOffset = value;
}

void QPanGesture::setOffset(const QPointF &value)
{
    d_func()->offset = value;
}

void QPanGesture::setAcceleration(qreal value)
{
    d_func()->acceleration = value;
}

/*!
    \class QPinchGesture
    \since 4.6
    \brief The QPinchGesture class describes a pinch gesture made by the user.
    \ingroup touch
    \ingroup gestures
    \inmodule QtWidgets

    A pinch gesture is a form of touch user input in which the user typically
    touches two points on the input device with a thumb and finger, before moving
    them closer together or further apart to change the scale factor, zoom, or level
    of detail of the user interface.

    For an overview of gesture handling in Qt and information on using gestures
    in your applications, see the \l{Gestures in Widgets and Graphics View} document.

    \image pinchgesture.png

    Instead of repeatedly applying the same pinching gesture, the user may
    continue to touch the input device in one place, and apply a second touch
    to a new point, continuing the gesture. When this occurs, gesture events
    will continue to be delivered to the target object, containing an instance
    of QPinchGesture in the Qt::GestureUpdated state.

    \sa QPanGesture, QSwipeGesture
*/

/*!
    \enum QPinchGesture::ChangeFlag

    This enum describes the changes that can occur to the properties of
    the gesture object.

    \value ScaleFactorChanged The scale factor held by scaleFactor changed.
    \value RotationAngleChanged The rotation angle held by rotationAngle changed.
    \value CenterPointChanged The center point held by centerPoint changed.

    \sa changeFlags, totalChangeFlags
*/

/*!
    \property QPinchGesture::totalChangeFlags
    \brief the property of the gesture that has change

    This property indicates which of the other properties has changed since the
    gesture has started. You can use this information to determine which aspect
    of your user interface needs to be updated.

    \sa changeFlags, scaleFactor, rotationAngle, centerPoint
*/

/*!
    \property QPinchGesture::changeFlags
    \brief the property of the gesture that has changed in the current step

    This property indicates which of the other properties has changed since
    the previous gesture event included information about this gesture. You
    can use this information to determine which aspect of your user interface
    needs to be updated.

    \sa totalChangeFlags, scaleFactor, rotationAngle, centerPoint
*/

/*!
    \property QPinchGesture::totalScaleFactor
    \brief the total scale factor

    The total scale factor measures the total change in scale factor from the
    original value to the current scale factor.

    \sa scaleFactor, lastScaleFactor
*/
/*!
    \property QPinchGesture::lastScaleFactor
    \brief the last scale factor recorded for this gesture

    The last scale factor contains the scale factor reported in the
    \l scaleFactor property when a previous gesture event included
    information about this gesture.

    If no previous event was delivered with information about this gesture
    (i.e., this gesture object contains information about the first movement
    in the gesture) then this property contains zero.

    \sa scaleFactor, totalScaleFactor
*/
/*!
    \property QPinchGesture::scaleFactor
    \brief the current scale factor

    The scale factor measures the scale factor associated with the distance
    between two of the user's inputs on a touch device.

    \sa totalScaleFactor, lastScaleFactor
*/

/*!
    \property QPinchGesture::totalRotationAngle
    \brief the total angle covered by the gesture

    This total angle measures the complete angle covered by the gesture. Usually, this
    is equal to the value held by the \l rotationAngle property, except in the case where
    the user performs multiple rotations by removing and repositioning one of the touch
    points, as described above. In this case, the total angle will be the sum of the
    rotation angles for the multiple stages of the gesture.

    \sa rotationAngle, lastRotationAngle
*/
/*!
    \property QPinchGesture::lastRotationAngle
    \brief the last reported angle covered by the gesture motion

    The last rotation angle is the angle as reported in the \l rotationAngle property
    when a previous gesture event was delivered for this gesture.

    \sa rotationAngle, totalRotationAngle
*/
/*!
    \property QPinchGesture::rotationAngle
    \brief the angle covered by the gesture motion

    \sa totalRotationAngle, lastRotationAngle
*/

/*!
    \property QPinchGesture::startCenterPoint
    \brief the starting position of the center point

    \sa centerPoint, lastCenterPoint
*/
/*!
    \property QPinchGesture::lastCenterPoint
    \brief the last position of the center point recorded for this gesture

    \sa centerPoint, startCenterPoint
*/
/*!
    \property QPinchGesture::centerPoint
    \brief the current center point

    The center point is the midpoint between the two input points in the gesture.

    \sa startCenterPoint, lastCenterPoint
*/

/*!
    \internal
*/
QPinchGesture::QPinchGesture(QObject *parent)
    : QGesture(*new QPinchGesturePrivate, parent)
{
    d_func()->gestureType = Qt::PinchGesture;
}

/*!
    Destructor.
*/
QPinchGesture::~QPinchGesture()
{
}

QPinchGesture::ChangeFlags QPinchGesture::totalChangeFlags() const
{
    return d_func()->totalChangeFlags;
}

void QPinchGesture::setTotalChangeFlags(QPinchGesture::ChangeFlags value)
{
    d_func()->totalChangeFlags = value;
}

QPinchGesture::ChangeFlags QPinchGesture::changeFlags() const
{
    return d_func()->changeFlags;
}

void QPinchGesture::setChangeFlags(QPinchGesture::ChangeFlags value)
{
    d_func()->changeFlags = value;
}

QPointF QPinchGesture::startCenterPoint() const
{
    return d_func()->startCenterPoint;
}

QPointF QPinchGesture::lastCenterPoint() const
{
    return d_func()->lastCenterPoint;
}

QPointF QPinchGesture::centerPoint() const
{
    return d_func()->centerPoint;
}

void QPinchGesture::setStartCenterPoint(const QPointF &value)
{
    d_func()->startCenterPoint = value;
}

void QPinchGesture::setLastCenterPoint(const QPointF &value)
{
    d_func()->lastCenterPoint = value;
}

void QPinchGesture::setCenterPoint(const QPointF &value)
{
    d_func()->centerPoint = value;
}


qreal QPinchGesture::totalScaleFactor() const
{
    return d_func()->totalScaleFactor;
}

qreal QPinchGesture::lastScaleFactor() const
{
    return d_func()->lastScaleFactor;
}

qreal QPinchGesture::scaleFactor() const
{
    return d_func()->scaleFactor;
}

void QPinchGesture::setTotalScaleFactor(qreal value)
{
    d_func()->totalScaleFactor = value;
}

void QPinchGesture::setLastScaleFactor(qreal value)
{
    d_func()->lastScaleFactor = value;
}

void QPinchGesture::setScaleFactor(qreal value)
{
    d_func()->scaleFactor = value;
}


qreal QPinchGesture::totalRotationAngle() const
{
    return d_func()->totalRotationAngle;
}

qreal QPinchGesture::lastRotationAngle() const
{
    return d_func()->lastRotationAngle;
}

qreal QPinchGesture::rotationAngle() const
{
    return d_func()->rotationAngle;
}

void QPinchGesture::setTotalRotationAngle(qreal value)
{
    d_func()->totalRotationAngle = value;
}

void QPinchGesture::setLastRotationAngle(qreal value)
{
    d_func()->lastRotationAngle = value;
}

void QPinchGesture::setRotationAngle(qreal value)
{
    d_func()->rotationAngle = value;
}

/*!
    \class QSwipeGesture
    \since 4.6
    \brief The QSwipeGesture class describes a swipe gesture made by the user.
    \ingroup gestures
    \inmodule QtWidgets

    \image swipegesture.png

    For an overview of gesture handling in Qt and information on using gestures
    in your applications, see the \l{Gestures in Widgets and Graphics View} document.

    \sa QPanGesture, QPinchGesture
*/

/*!
    \enum QSwipeGesture::SwipeDirection

    This enum describes the possible directions for the gesture's motion
    along the horizontal and vertical axes.

    \value NoDirection The gesture had no motion associated with it on a particular axis.
    \value Left     The gesture involved a horizontal motion to the left.
    \value Right    The gesture involved a horizontal motion to the right.
    \value Up       The gesture involved an upward vertical motion.
    \value Down     The gesture involved a downward vertical motion.
*/

/*!
    \property QSwipeGesture::horizontalDirection
    \brief the horizontal direction of the gesture

    If the gesture has a horizontal component, the horizontal direction
    is either Left or Right; otherwise, it is NoDirection.

    \sa verticalDirection, swipeAngle
*/

/*!
    \property QSwipeGesture::verticalDirection
    \brief the vertical direction of the gesture

    If the gesture has a vertical component, the vertical direction
    is either Up or Down; otherwise, it is NoDirection.

    \sa horizontalDirection, swipeAngle
*/

/*!
    \property QSwipeGesture::swipeAngle
    \brief the angle of the motion associated with the gesture

    If the gesture has either a horizontal or vertical component, the
    swipe angle describes the angle between the direction of motion and the
    x-axis as defined using the standard widget
    \l{Coordinate System}{coordinate system}.

    \sa horizontalDirection, verticalDirection
*/

/*!
    \property QSwipeGesture::velocity
    \since 4.7.1
    \internal
*/

/*!
    \internal
*/
QSwipeGesture::QSwipeGesture(QObject *parent)
    : QGesture(*new QSwipeGesturePrivate, parent)
{
    d_func()->gestureType = Qt::SwipeGesture;
}

/*!
    Destructor.
*/
QSwipeGesture::~QSwipeGesture()
{
}

QSwipeGesture::SwipeDirection QSwipeGesture::horizontalDirection() const
{
    Q_D(const QSwipeGesture);
    if (d->swipeAngle < 0 || d->swipeAngle == 90 || d->swipeAngle == 270)
        return QSwipeGesture::NoDirection;
    else if (d->swipeAngle < 90 || d->swipeAngle > 270)
        return QSwipeGesture::Right;
    else
        return QSwipeGesture::Left;
}

QSwipeGesture::SwipeDirection QSwipeGesture::verticalDirection() const
{
    Q_D(const QSwipeGesture);
    if (d->swipeAngle <= 0 || d->swipeAngle == 180)
        return QSwipeGesture::NoDirection;
    else if (d->swipeAngle < 180)
        return QSwipeGesture::Up;
    else
        return QSwipeGesture::Down;
}

qreal QSwipeGesture::swipeAngle() const
{
    return d_func()->swipeAngle;
}

void QSwipeGesture::setSwipeAngle(qreal value)
{
    d_func()->swipeAngle = value;
}

/*!
    \class QTapGesture
    \since 4.6
    \brief The QTapGesture class describes a tap gesture made by the user.
    \ingroup gestures
    \inmodule QtWidgets

    For an overview of gesture handling in Qt and information on using gestures
    in your applications, see the \l{Gestures in Widgets and Graphics View} document.

    \sa QPanGesture, QPinchGesture
*/

/*!
    \property QTapGesture::position
    \brief the position of the tap
*/

/*!
    \internal
*/
QTapGesture::QTapGesture(QObject *parent)
    : QGesture(*new QTapGesturePrivate, parent)
{
    d_func()->gestureType = Qt::TapGesture;
}

/*!
    Destructor.
*/
QTapGesture::~QTapGesture()
{
}

QPointF QTapGesture::position() const
{
    return d_func()->position;
}

void QTapGesture::setPosition(const QPointF &value)
{
    d_func()->position = value;
}
/*!
    \class QTapAndHoldGesture
    \since 4.6
    \brief The QTapAndHoldGesture class describes a tap-and-hold (aka LongTap)
    gesture made by the user.
    \ingroup gestures
    \inmodule QtWidgets

    For an overview of gesture handling in Qt and information on using gestures
    in your applications, see the \l{Gestures in Widgets and Graphics View} document.

    \sa QPanGesture, QPinchGesture
*/

/*!
    \property QTapAndHoldGesture::position
    \brief the position of the tap
*/

/*!
    \internal
*/
QTapAndHoldGesture::QTapAndHoldGesture(QObject *parent)
    : QGesture(*new QTapAndHoldGesturePrivate, parent)
{
    d_func()->gestureType = Qt::TapAndHoldGesture;
}

/*!
    Destructor.
*/
QTapAndHoldGesture::~QTapAndHoldGesture()
{
}

QPointF QTapAndHoldGesture::position() const
{
    return d_func()->position;
}

void QTapAndHoldGesture::setPosition(const QPointF &value)
{
    d_func()->position = value;
}

/*!
    Set the timeout, in milliseconds, before the gesture triggers.

    The recognizer will detect a touch down and if \a msecs
    later the touch is still down, it will trigger the QTapAndHoldGesture.
    The default value is 700 milliseconds.
*/
void QTapAndHoldGesture::setTimeout(int msecs)
{
    QTapAndHoldGesturePrivate::Timeout = msecs;
}

/*!
    Gets the timeout, in milliseconds, before the gesture triggers.

    The recognizer will detect a touch down and if timeout()
    later the touch is still down, it will trigger the QTapAndHoldGesture.
    The default value is 700 milliseconds.
*/
int QTapAndHoldGesture::timeout()
{
    return QTapAndHoldGesturePrivate::Timeout;
}

int QTapAndHoldGesturePrivate::Timeout = 700; // in ms


/*!
    \class QGestureEvent
    \since 4.6
    \ingroup events
    \ingroup gestures
    \inmodule QtWidgets

    \brief The QGestureEvent class provides the description of triggered gestures.

    The QGestureEvent class contains a list of gestures, which can be obtained using the
    gestures() function.

    The gestures are either active or canceled. A list of those that are currently being
    executed can be obtained using the activeGestures() function. A list of those which
    were previously active and have been canceled can be accessed using the
    canceledGestures() function. A gesture might be canceled if the current window loses
    focus, for example, or because of a timeout, or for other reasons.

    If the event handler does not accept the event by calling the generic
    QEvent::accept() function, all individual QGesture object that were not
    accepted and in the Qt::GestureStarted state will be propagated up the
    parent widget chain until a widget accepts them individually, by calling
    QGestureEvent::accept() for each of them, or an event filter consumes the
    event.

    \section1 Further Reading

    For an overview of gesture handling in Qt and information on using gestures
    in your applications, see the \l{Gestures in Widgets and Graphics View} document.

    \sa QGesture, QGestureRecognizer,
        QWidget::grabGesture(), QGraphicsObject::grabGesture()
*/

/*!
    Creates new QGestureEvent containing a list of \a gestures.
*/
QGestureEvent::QGestureEvent(const QList<QGesture *> &gestures)
    : QEvent(QEvent::Gesture), m_gestures(gestures), m_widget(0)

{
}

/*!
    Destroys QGestureEvent.
*/
QGestureEvent::~QGestureEvent()
{
}

/*!
    Returns all gestures that are delivered in the event.
*/
QList<QGesture *> QGestureEvent::gestures() const
{
    return m_gestures;
}

/*!
    Returns a gesture object by \a type.
*/
QGesture *QGestureEvent::gesture(Qt::GestureType type) const
{
    for (int i = 0; i < m_gestures.size(); ++i)
        if (m_gestures.at(i)->gestureType() == type)
            return m_gestures.at(i);
    return 0;
}

/*!
    Returns a list of active (not canceled) gestures.
*/
QList<QGesture *> QGestureEvent::activeGestures() const
{
    QList<QGesture *> gestures;
    for (QGesture *gesture : m_gestures) {
        if (gesture->state() != Qt::GestureCanceled)
            gestures.append(gesture);
    }
    return gestures;
}

/*!
    Returns a list of canceled gestures.
*/
QList<QGesture *> QGestureEvent::canceledGestures() const
{
    QList<QGesture *> gestures;
    for (QGesture *gesture : m_gestures) {
        if (gesture->state() == Qt::GestureCanceled)
            gestures.append(gesture);
    }
    return gestures;
}

/*!
    Sets the accept flag of the given \a gesture object to the specified \a value.

    Setting the accept flag indicates that the event receiver wants the \a gesture.
    Unwanted gestures may be propagated to the parent widget.

    By default, gestures in events of type QEvent::Gesture are accepted, and
    gestures in QEvent::GestureOverride events are ignored.

    For convenience, the accept flag can also be set with
    \l{QGestureEvent::accept()}{accept(gesture)}, and cleared with
    \l{QGestureEvent::ignore()}{ignore(gesture)}.
*/
void QGestureEvent::setAccepted(QGesture *gesture, bool value)
{
    if (gesture)
        setAccepted(gesture->gestureType(), value);
}

/*!
    Sets the accept flag of the given \a gesture object, the equivalent of calling
    \l{QGestureEvent::setAccepted()}{setAccepted(gesture, true)}.

    Setting the accept flag indicates that the event receiver wants the
    gesture. Unwanted gestures may be propagated to the parent widget.

    \sa QGestureEvent::ignore()
*/
void QGestureEvent::accept(QGesture *gesture)
{
    if (gesture)
        setAccepted(gesture->gestureType(), true);
}

/*!
    Clears the accept flag parameter of the given \a gesture object, the equivalent
    of calling \l{QGestureEvent::setAccepted()}{setAccepted(gesture, false)}.

    Clearing the accept flag indicates that the event receiver does not
    want the gesture. Unwanted gestures may be propagated to the parent widget.

    \sa QGestureEvent::accept()
*/
void QGestureEvent::ignore(QGesture *gesture)
{
    if (gesture)
        setAccepted(gesture->gestureType(), false);
}

/*!
    Returns \c true if the \a gesture is accepted; otherwise returns \c false.
*/
bool QGestureEvent::isAccepted(QGesture *gesture) const
{
    return gesture ? isAccepted(gesture->gestureType()) : false;
}

/*!
    Sets the accept flag of the given \a gestureType object to the specified
    \a value.

    Setting the accept flag indicates that the event receiver wants to receive
    gestures of the specified type, \a gestureType. Unwanted gestures may be
    propagated to the parent widget.

    By default, gestures in events of type QEvent::Gesture are accepted, and
    gestures in QEvent::GestureOverride events are ignored.

    For convenience, the accept flag can also be set with
    \l{QGestureEvent::accept()}{accept(gestureType)}, and cleared with
    \l{QGestureEvent::ignore()}{ignore(gestureType)}.
*/
void QGestureEvent::setAccepted(Qt::GestureType gestureType, bool value)
{
    setAccepted(false);
    m_accepted[gestureType] = value;
}

/*!
    Sets the accept flag of the given \a gestureType, the equivalent of calling
    \l{QGestureEvent::setAccepted()}{setAccepted(gestureType, true)}.

    Setting the accept flag indicates that the event receiver wants the
    gesture. Unwanted gestures may be propagated to the parent widget.

    \sa QGestureEvent::ignore()
*/
void QGestureEvent::accept(Qt::GestureType gestureType)
{
    setAccepted(gestureType, true);
}

/*!
    Clears the accept flag parameter of the given \a gestureType, the equivalent
    of calling \l{QGestureEvent::setAccepted()}{setAccepted(gesture, false)}.

    Clearing the accept flag indicates that the event receiver does not
    want the gesture. Unwanted gestures may be propgated to the parent widget.

    \sa QGestureEvent::accept()
*/
void QGestureEvent::ignore(Qt::GestureType gestureType)
{
    setAccepted(gestureType, false);
}

/*!
    Returns \c true if the gesture of type \a gestureType is accepted; otherwise
    returns \c false.
*/
bool QGestureEvent::isAccepted(Qt::GestureType gestureType) const
{
    return m_accepted.value(gestureType, true);
}

/*!
    \internal

    Sets the widget for this event to the \a widget specified.
*/
void QGestureEvent::setWidget(QWidget *widget)
{
    m_widget = widget;
}

/*!
    Returns the widget on which the event occurred.
*/
QWidget *QGestureEvent::widget() const
{
    return m_widget;
}

#if QT_CONFIG(graphicsview)
/*!
    Returns the scene-local coordinates if the \a gesturePoint is inside a
    graphics view.

    This functional might be useful when the gesture event is delivered to a
    QGraphicsObject to translate a point in screen coordinates to scene-local
    coordinates.

    \sa QPointF::isNull()
*/
QPointF QGestureEvent::mapToGraphicsScene(const QPointF &gesturePoint) const
{
    QWidget *w = widget();
    if (w) // we get the viewport as widget, not the graphics view
        w = w->parentWidget();
    QGraphicsView *view = qobject_cast<QGraphicsView*>(w);
    if (view) {
        return view->mapToScene(view->mapFromGlobal(gesturePoint.toPoint()));
    }
    return QPointF();
}
#endif // QT_CONFIG(graphicsview)

#ifndef QT_NO_DEBUG_STREAM

static void formatGestureHeader(QDebug d, const char *className, const QGesture *gesture)
{
     d << className << "(state=";
     QtDebugUtils::formatQEnum(d, gesture->state());
     if (gesture->hasHotSpot()) {
         d << ",hotSpot=";
         QtDebugUtils::formatQPoint(d, gesture->hotSpot());
     }
}

Q_WIDGETS_EXPORT QDebug operator<<(QDebug d, const QGesture *gesture)
{
    QDebugStateSaver saver(d);
    d.nospace();
    switch (gesture->gestureType()) {
    case Qt::TapGesture:
        formatGestureHeader(d, "QTapGesture", gesture);
        d << ",position=";
        QtDebugUtils::formatQPoint(d, static_cast<const QTapGesture*>(gesture)->position());
        d << ')';
        break;
    case Qt::TapAndHoldGesture: {
        const QTapAndHoldGesture *tap = static_cast<const QTapAndHoldGesture*>(gesture);
        formatGestureHeader(d, "QTapAndHoldGesture", tap);
        d << ",position=";
        QtDebugUtils::formatQPoint(d, tap->position());
        d << ",timeout=" << tap->timeout() << ')';
    }
        break;
    case Qt::PanGesture: {
        const QPanGesture *pan = static_cast<const QPanGesture*>(gesture);
        formatGestureHeader(d, "QPanGesture", pan);
        d << ",lastOffset=";
        QtDebugUtils::formatQPoint(d, pan->lastOffset());
        d << pan->lastOffset();
        d << ",offset=";
        QtDebugUtils::formatQPoint(d, pan->offset());
        d  << ",acceleration=" << pan->acceleration() << ",delta=";
        QtDebugUtils::formatQPoint(d, pan->delta());
        d << ')';
    }
        break;
    case Qt::PinchGesture: {
        const QPinchGesture *pinch = static_cast<const QPinchGesture*>(gesture);
        formatGestureHeader(d, "QPinchGesture", pinch);
        d << ",totalChangeFlags=" << pinch->totalChangeFlags()
          << ",changeFlags=" << pinch->changeFlags() << ",startCenterPoint=";
        QtDebugUtils::formatQPoint(d, pinch->startCenterPoint());
        d << ",lastCenterPoint=";
        QtDebugUtils::formatQPoint(d, pinch->lastCenterPoint());
        d << ",centerPoint=";
        QtDebugUtils::formatQPoint(d, pinch->centerPoint());
        d << ",totalScaleFactor=" << pinch->totalScaleFactor()
            << ",lastScaleFactor=" << pinch->lastScaleFactor()
            << ",scaleFactor=" << pinch->scaleFactor()
            << ",totalRotationAngle=" << pinch->totalRotationAngle()
            << ",lastRotationAngle=" << pinch->lastRotationAngle()
            << ",rotationAngle=" << pinch->rotationAngle() << ')';
    }
        break;
    case Qt::SwipeGesture: {
        const QSwipeGesture *swipe = static_cast<const QSwipeGesture*>(gesture);
        formatGestureHeader(d, "QSwipeGesture", swipe);
        d << ",horizontalDirection=";
        QtDebugUtils::formatQEnum(d, swipe->horizontalDirection());
        d << ",verticalDirection=";
        QtDebugUtils::formatQEnum(d, swipe->verticalDirection());
        d << ",swipeAngle=" << swipe->swipeAngle() << ')';
    }
        break;
    default:
        formatGestureHeader(d, "Custom gesture", gesture);
        d << ",type=" << gesture->gestureType() << ')';
        break;
    }
    return d;
}

Q_WIDGETS_EXPORT QDebug operator<<(QDebug d, const QGestureEvent *gestureEvent)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "QGestureEvent(" << gestureEvent->gestures() << ')';
    return d;
}

#endif // !QT_NO_DEBUG_STREAM
QT_END_NAMESPACE

#include <moc_qgesture.cpp>

#endif // QT_NO_GESTURES
