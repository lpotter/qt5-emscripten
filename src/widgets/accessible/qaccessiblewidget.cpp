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

#include "qaccessiblewidget.h"

#ifndef QT_NO_ACCESSIBILITY

#include "qaction.h"
#include "qapplication.h"
#if QT_CONFIG(groupbox)
#include "qgroupbox.h"
#endif
#if QT_CONFIG(label)
#include "qlabel.h"
#endif
#include "qtooltip.h"
#if QT_CONFIG(whatsthis)
#include "qwhatsthis.h"
#endif
#include "qwidget.h"
#include "qdebug.h"
#include <qmath.h>
#if QT_CONFIG(rubberband)
#include <QRubberBand>
#endif
#include <QFocusFrame>
#if QT_CONFIG(menu)
#include <QMenu>
#endif
#include <QtWidgets/private/qwidget_p.h>

QT_BEGIN_NAMESPACE

static QList<QWidget*> childWidgets(const QWidget *widget)
{
    QList<QWidget*> widgets;
    for (QObject *o : widget->children()) {
        QWidget *w = qobject_cast<QWidget *>(o);
        if (w && !w->isWindow()
            && !qobject_cast<QFocusFrame*>(w)
#if QT_CONFIG(menu)
            && !qobject_cast<QMenu*>(w)
#endif
            && w->objectName() != QLatin1String("qt_rubberband")
            && w->objectName() != QLatin1String("qt_spinbox_lineedit"))
            widgets.append(w);
    }
    return widgets;
}

static QString buddyString(const QWidget *widget)
{
    if (!widget)
        return QString();
    QWidget *parent = widget->parentWidget();
    if (!parent)
        return QString();
#if QT_CONFIG(shortcut) && QT_CONFIG(label)
    for (QObject *o : parent->children()) {
        QLabel *label = qobject_cast<QLabel*>(o);
        if (label && label->buddy() == widget)
            return label->text();
    }
#endif

#if QT_CONFIG(groupbox)
    QGroupBox *groupbox = qobject_cast<QGroupBox*>(parent);
    if (groupbox)
        return groupbox->title();
#endif

    return QString();
}

/* This function will return the offset of the '&' in the text that would be
   preceding the accelerator character.
   If this text does not have an accelerator, -1 will be returned. */
static int qt_accAmpIndex(const QString &text)
{
#ifndef QT_NO_SHORTCUT
    if (text.isEmpty())
        return -1;

    int fa = 0;
    while ((fa = text.indexOf(QLatin1Char('&'), fa)) != -1) {
        ++fa;
        if (fa < text.length()) {
            // ignore "&&"
            if (text.at(fa) == QLatin1Char('&')) {

                ++fa;
                continue;
            } else {
                return fa - 1;
                break;
            }
        }
    }

    return -1;
#else
    Q_UNUSED(text);
    return -1;
#endif
}

QString qt_accStripAmp(const QString &text)
{
    QString newText(text);
    int ampIndex = qt_accAmpIndex(newText);
    if (ampIndex != -1)
        newText.remove(ampIndex, 1);

    return newText.replace(QLatin1String("&&"), QLatin1String("&"));
}

QString qt_accHotKey(const QString &text)
{
#ifndef QT_NO_SHORTCUT
    int ampIndex = qt_accAmpIndex(text);
    if (ampIndex != -1)
        return QKeySequence(Qt::ALT).toString(QKeySequence::NativeText) + text.at(ampIndex + 1);
#else
    Q_UNUSED(text)
#endif

    return QString();
}

// ### inherit QAccessibleObjectPrivate
class QAccessibleWidgetPrivate
{
public:
    QAccessibleWidgetPrivate()
        :role(QAccessible::Client)
    {}

    QAccessible::Role role;
    QString name;
    QStringList primarySignals;
};

/*!
    \class QAccessibleWidget
    \brief The QAccessibleWidget class implements the QAccessibleInterface for QWidgets.

    \ingroup accessibility
    \inmodule QtWidgets

    This class is part of \l {Accessibility for QWidget Applications}.

    This class is convenient to use as a base class for custom
    implementations of QAccessibleInterfaces that provide information
    about widget objects.

    The class provides functions to retrieve the parentObject() (the
    widget's parent widget), and the associated widget(). Controlling
    signals can be added with addControllingSignal(), and setters are
    provided for various aspects of the interface implementation, for
    example setValue(), setDescription(), setAccelerator(), and
    setHelp().

    \sa QAccessible, QAccessibleObject
*/

/*!
    Creates a QAccessibleWidget object for widget \a w.
    \a role and \a name are optional parameters that set the object's
    role and name properties.
*/
QAccessibleWidget::QAccessibleWidget(QWidget *w, QAccessible::Role role, const QString &name)
: QAccessibleObject(w)
{
    Q_ASSERT(widget());
    d = new QAccessibleWidgetPrivate();
    d->role = role;
    d->name = name;
}

/*! \reimp */
bool QAccessibleWidget::isValid() const
{
    if (!object() || static_cast<QWidget *>(object())->d_func()->data.in_destructor)
        return false;
    return QAccessibleObject::isValid();
}

/*! \reimp */
QWindow *QAccessibleWidget::window() const
{
    const QWidget *w = widget();
    Q_ASSERT(w);
    QWindow *result = w->windowHandle();
    if (!result) {
        if (const QWidget *nativeParent = w->nativeParentWidget())
            result = nativeParent->windowHandle();
    }
    return result;
}

/*!
    Destroys this object.
*/
QAccessibleWidget::~QAccessibleWidget()
{
    delete d;
}

/*!
    Returns the associated widget.
*/
QWidget *QAccessibleWidget::widget() const
{
    return qobject_cast<QWidget*>(object());
}

/*!
    Returns the associated widget's parent object, which is either the
    parent widget, or qApp for top-level widgets.
*/
QObject *QAccessibleWidget::parentObject() const
{
    QWidget *w = widget();
    if (!w || w->isWindow() || !w->parentWidget())
        return qApp;
    return w->parent();
}

/*! \reimp */
QRect QAccessibleWidget::rect() const
{
    QWidget *w = widget();
    if (!w->isVisible())
        return QRect();
    QPoint wpos = w->mapToGlobal(QPoint(0, 0));

    return QRect(wpos.x(), wpos.y(), w->width(), w->height());
}

QT_BEGIN_INCLUDE_NAMESPACE
#include <private/qobject_p.h>
QT_END_INCLUDE_NAMESPACE

class QACConnectionObject : public QObject
{
    Q_DECLARE_PRIVATE(QObject)
public:
    inline bool isSender(const QObject *receiver, const char *signal) const
    { return d_func()->isSender(receiver, signal); }
    inline QObjectList receiverList(const char *signal) const
    { return d_func()->receiverList(signal); }
    inline QObjectList senderList() const
    { return d_func()->senderList(); }
};

/*!
    Registers \a signal as a controlling signal.

    An object is a Controller to any other object connected to a
    controlling signal.
*/
void QAccessibleWidget::addControllingSignal(const QString &signal)
{
    QByteArray s = QMetaObject::normalizedSignature(signal.toLatin1());
    if (Q_UNLIKELY(object()->metaObject()->indexOfSignal(s) < 0))
        qWarning("Signal %s unknown in %s", s.constData(), object()->metaObject()->className());
    d->primarySignals << QLatin1String(s);
}

static inline bool isAncestor(const QObject *obj, const QObject *child)
{
    while (child) {
        if (child == obj)
            return true;
        child = child->parent();
    }
    return false;
}

/*! \reimp */
QVector<QPair<QAccessibleInterface*, QAccessible::Relation> >
QAccessibleWidget::relations(QAccessible::Relation match /*= QAccessible::AllRelations*/) const
{
    QVector<QPair<QAccessibleInterface*, QAccessible::Relation> > rels;
    if (match & QAccessible::Label) {
        const QAccessible::Relation rel = QAccessible::Label;
        if (QWidget *parent = widget()->parentWidget()) {
#if QT_CONFIG(shortcut) && QT_CONFIG(label)
            // first check for all siblings that are labels to us
            // ideally we would go through all objects and check, but that
            // will be too expensive
            const QList<QWidget*> kids = childWidgets(parent);
            for (QWidget *kid : kids) {
                if (QLabel *labelSibling = qobject_cast<QLabel*>(kid)) {
                    if (labelSibling->buddy() == widget()) {
                        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(labelSibling);
                        rels.append(qMakePair(iface, rel));
                    }
                }
            }
#endif
#if QT_CONFIG(groupbox)
            QGroupBox *groupbox = qobject_cast<QGroupBox*>(parent);
            if (groupbox && !groupbox->title().isEmpty()) {
                QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(groupbox);
                rels.append(qMakePair(iface, rel));
            }
#endif
        }
    }

    if (match & QAccessible::Controlled) {
        QObjectList allReceivers;
        QACConnectionObject *connectionObject = (QACConnectionObject*)object();
        for (int sig = 0; sig < d->primarySignals.count(); ++sig) {
            const QObjectList receivers = connectionObject->receiverList(d->primarySignals.at(sig).toLatin1());
            allReceivers += receivers;
        }

        allReceivers.removeAll(object());  //### The object might connect to itself internally

        for (int i = 0; i < allReceivers.count(); ++i) {
            const QAccessible::Relation rel = QAccessible::Controlled;
            QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(allReceivers.at(i));
            if (iface)
                rels.append(qMakePair(iface, rel));
        }
    }

    return rels;
}

/*! \reimp */
QAccessibleInterface *QAccessibleWidget::parent() const
{
    return QAccessible::queryAccessibleInterface(parentObject());
}

/*! \reimp */
QAccessibleInterface *QAccessibleWidget::child(int index) const
{
    Q_ASSERT(widget());
    QWidgetList childList = childWidgets(widget());
    if (index >= 0 && index < childList.size())
        return QAccessible::queryAccessibleInterface(childList.at(index));
    return 0;
}

/*! \reimp */
QAccessibleInterface *QAccessibleWidget::focusChild() const
{
    if (widget()->hasFocus())
        return QAccessible::queryAccessibleInterface(object());

    QWidget *fw = widget()->focusWidget();
    if (!fw)
        return 0;

    if (isAncestor(widget(), fw) || fw == widget())
        return QAccessible::queryAccessibleInterface(fw);
    return 0;
}

/*! \reimp */
int QAccessibleWidget::childCount() const
{
    QWidgetList cl = childWidgets(widget());
    return cl.size();
}

/*! \reimp */
int QAccessibleWidget::indexOfChild(const QAccessibleInterface *child) const
{
    if (!child)
        return -1;
    QWidgetList cl = childWidgets(widget());
    return cl.indexOf(qobject_cast<QWidget *>(child->object()));
}

// from qwidget.cpp
extern QString qt_setWindowTitle_helperHelper(const QString &, const QWidget*);

/*! \reimp */
QString QAccessibleWidget::text(QAccessible::Text t) const
{
    QString str;

    switch (t) {
    case QAccessible::Name:
        if (!d->name.isEmpty()) {
            str = d->name;
        } else if (!widget()->accessibleName().isEmpty()) {
            str = widget()->accessibleName();
        } else if (widget()->isWindow()) {
            if (widget()->isMinimized())
                str = qt_setWindowTitle_helperHelper(widget()->windowIconText(), widget());
            else
                str = qt_setWindowTitle_helperHelper(widget()->windowTitle(), widget());
        } else {
            str = qt_accStripAmp(buddyString(widget()));
        }
        break;
    case QAccessible::Description:
        str = widget()->accessibleDescription();
#ifndef QT_NO_TOOLTIP
        if (str.isEmpty())
            str = widget()->toolTip();
#endif
        break;
    case QAccessible::Help:
#if QT_CONFIG(whatsthis)
        str = widget()->whatsThis();
#endif
        break;
    case QAccessible::Accelerator:
        str = qt_accHotKey(buddyString(widget()));
        break;
    case QAccessible::Value:
        break;
    default:
        break;
    }
    return str;
}

/*! \reimp */
QStringList QAccessibleWidget::actionNames() const
{
    QStringList names;
    if (widget()->isEnabled()) {
        if (widget()->focusPolicy() != Qt::NoFocus)
            names << setFocusAction();
    }
    return names;
}

/*! \reimp */
void QAccessibleWidget::doAction(const QString &actionName)
{
    if (!widget()->isEnabled())
        return;

    if (actionName == setFocusAction()) {
        if (widget()->isWindow())
            widget()->activateWindow();
        widget()->setFocus();
    }
}

/*! \reimp */
QStringList QAccessibleWidget::keyBindingsForAction(const QString & /* actionName */) const
{
    return QStringList();
}

/*! \reimp */
QAccessible::Role QAccessibleWidget::role() const
{
    return d->role;
}

/*! \reimp */
QAccessible::State QAccessibleWidget::state() const
{
    QAccessible::State state;

    QWidget *w = widget();
    if (w->testAttribute(Qt::WA_WState_Visible) == false)
        state.invisible = true;
    if (w->focusPolicy() != Qt::NoFocus)
        state.focusable = true;
    if (w->hasFocus())
        state.focused = true;
    if (!w->isEnabled())
        state.disabled = true;
    if (w->isWindow()) {
        if (w->windowFlags() & Qt::WindowSystemMenuHint)
            state.movable = true;
        if (w->minimumSize() != w->maximumSize())
            state.sizeable = true;
        if (w->isActiveWindow())
            state.active = true;
    }

    return state;
}

/*! \reimp */
QColor QAccessibleWidget::foregroundColor() const
{
    return widget()->palette().color(widget()->foregroundRole());
}

/*! \reimp */
QColor QAccessibleWidget::backgroundColor() const
{
    return widget()->palette().color(widget()->backgroundRole());
}

/*! \reimp */
void *QAccessibleWidget::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ActionInterface)
       return static_cast<QAccessibleActionInterface*>(this);
    return 0;
}

QT_END_NAMESPACE

#endif //QT_NO_ACCESSIBILITY
