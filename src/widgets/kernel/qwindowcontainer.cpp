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

#include "qwindowcontainer_p.h"
#include "qwidget_p.h"
#include <QtGui/qwindow.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <QDebug>

#if QT_CONFIG(mdiarea)
#include <QMdiSubWindow>
#endif
#include <QAbstractScrollArea>
#include <QPainter>

QT_BEGIN_NAMESPACE

class QWindowContainerPrivate : public QWidgetPrivate
{
public:
    Q_DECLARE_PUBLIC(QWindowContainer)

    QWindowContainerPrivate()
        : window(0)
        , oldFocusWindow(0)
        , usesNativeWidgets(false)
    {
    }

    ~QWindowContainerPrivate() { }

    static QWindowContainerPrivate *get(QWidget *w) {
        QWindowContainer *wc = qobject_cast<QWindowContainer *>(w);
        if (wc)
            return wc->d_func();
        return 0;
    }

    void updateGeometry() {
        Q_Q(QWindowContainer);
        if (!q->isWindow() && (q->geometry().bottom() <= 0 || q->geometry().right() <= 0))
            /* Qt (e.g. QSplitter) sometimes prefer to hide a widget by *not* calling
               setVisible(false). This is often done by setting its coordinates to a sufficiently
               negative value so that its clipped outside the parent. Since a QWindow is not clipped
               to widgets in general, it needs to be dealt with as a special case.
            */
            window->setGeometry(q->geometry());
        else if (usesNativeWidgets)
            window->setGeometry(q->rect());
        else
            window->setGeometry(QRect(q->mapTo(q->window(), QPoint()), q->size()));
    }

    void updateUsesNativeWidgets()
    {
        if (window->parent() == 0)
            return;
        Q_Q(QWindowContainer);
        if (q->internalWinId()) {
            // Allow use native widgets if the window container is already a native widget
            usesNativeWidgets = true;
            return;
        }
        bool nativeWidgetSet = false;
        QWidget *p = q->parentWidget();
        while (p) {
            if (false
#if QT_CONFIG(mdiarea)
                || qobject_cast<QMdiSubWindow *>(p) != 0
#endif
#if QT_CONFIG(scrollarea)
                || qobject_cast<QAbstractScrollArea *>(p) != 0
#endif
                    ) {
                q->winId();
                nativeWidgetSet = true;
                break;
            }
            p = p->parentWidget();
        }
        usesNativeWidgets = nativeWidgetSet;
    }

    void markParentChain() {
        Q_Q(QWindowContainer);
        QWidget *p = q;
        while (p) {
            QWidgetPrivate *d = static_cast<QWidgetPrivate *>(QWidgetPrivate::get(p));
            d->createExtra();
            d->extra->hasWindowContainer = true;
            p = p->parentWidget();
        }
    }

    bool isStillAnOrphan() const {
        return window->parent() == &fakeParent;
    }

    QPointer<QWindow> window;
    QWindow *oldFocusWindow;
    QWindow fakeParent;

    uint usesNativeWidgets : 1;
};



/*!
    \fn QWidget *QWidget::createWindowContainer(QWindow *window, QWidget *parent, Qt::WindowFlags flags);

    Creates a QWidget that makes it possible to embed \a window into
    a QWidget-based application.

    The window container is created as a child of \a parent and with
    window flags \a flags.

    Once the window has been embedded into the container, the
    container will control the window's geometry and
    visibility. Explicit calls to QWindow::setGeometry(),
    QWindow::show() or QWindow::hide() on an embedded window is not
    recommended.

    The container takes over ownership of \a window. The window can
    be removed from the window container with a call to
    QWindow::setParent().

    The window container is attached as a native child window to the
    toplevel window it is a child of. When a window container is used
    as a child of a QAbstractScrollArea or QMdiArea, it will
    create a \l {Native Widgets vs Alien Widgets} {native window} for
    every widget in its parent chain to allow for proper stacking and
    clipping in this use case. Creating a native window for the window
    container also allows for proper stacking and clipping. This must
    be done before showing the window container. Applications with
    many native child windows may suffer from performance issues.

    The window container has a number of known limitations:

    \list

    \li Stacking order; The embedded window will stack on top of the
    widget hierarchy as an opaque box. The stacking order of multiple
    overlapping window container instances is undefined.

    \li Rendering Integration; The window container does not interoperate
    with QGraphicsProxyWidget, QWidget::render() or similar functionality.

    \li Focus Handling; It is possible to let the window container
    instance have any focus policy and it will delegate focus to the
    window via a call to QWindow::requestActivate(). However,
    returning to the normal focus chain from the QWindow instance will
    be up to the QWindow instance implementation itself. For instance,
    when entering a Qt Quick based window with tab focus, it is quite
    likely that further tab presses will only cycle inside the QML
    application. Also, whether QWindow::requestActivate() actually
    gives the window focus, is platform dependent.

    \li Using many window container instances in a QWidget-based
    application can greatly hurt the overall performance of the
    application.

    \endlist
 */

QWidget *QWidget::createWindowContainer(QWindow *window, QWidget *parent, Qt::WindowFlags flags)
{
    return new QWindowContainer(window, parent, flags);
}



/*!
    \internal
 */

QWindowContainer::QWindowContainer(QWindow *embeddedWindow, QWidget *parent, Qt::WindowFlags flags)
    : QWidget(*new QWindowContainerPrivate, parent, flags)
{
    Q_D(QWindowContainer);
    if (Q_UNLIKELY(!embeddedWindow)) {
        qWarning("QWindowContainer: embedded window cannot be null");
        return;
    }

    // The embedded QWindow must use the same logic as QWidget when it comes to the surface type.
    // Otherwise we may end up with BadMatch failures on X11.
    if (embeddedWindow->surfaceType() == QSurface::RasterSurface
        && QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RasterGLSurface)
        && !QApplication::testAttribute(Qt::AA_ForceRasterWidgets))
        embeddedWindow->setSurfaceType(QSurface::RasterGLSurface);

    d->window = embeddedWindow;
    d->window->setParent(&d->fakeParent);
    setAcceptDrops(true);

    connect(QGuiApplication::instance(), SIGNAL(focusWindowChanged(QWindow*)), this, SLOT(focusWindowChanged(QWindow*)));
}

QWindow *QWindowContainer::containedWindow() const
{
    Q_D(const QWindowContainer);
    return d->window;
}

/*!
    \internal
 */

QWindowContainer::~QWindowContainer()
{
    Q_D(QWindowContainer);

    // Call destroy() explicitly first. The dtor would do this too, but
    // QEvent::PlatformSurface delivery relies on virtuals. Getting
    // SurfaceAboutToBeDestroyed can be essential for OpenGL, Vulkan, etc.
    // QWindow subclasses in particular. Keep these working.
    if (d->window)
        d->window->destroy();

    delete d->window;
}



/*!
    \internal
 */

void QWindowContainer::focusWindowChanged(QWindow *focusWindow)
{
    Q_D(QWindowContainer);
    d->oldFocusWindow = focusWindow;
    if (focusWindow == d->window) {
        QWidget *widget = QApplication::focusWidget();
        if (widget)
            widget->clearFocus();
    }
}

/*!
    \internal
 */

bool QWindowContainer::event(QEvent *e)
{
    Q_D(QWindowContainer);
    if (!d->window)
        return QWidget::event(e);

    QEvent::Type type = e->type();
    switch (type) {
    case QEvent::ChildRemoved: {
        QChildEvent *ce = static_cast<QChildEvent *>(e);
        if (ce->child() == d->window)
            d->window = 0;
        break;
    }
    // The only thing we are interested in is making sure our sizes stay
    // in sync, so do a catch-all case.
    case QEvent::Resize:
        d->updateGeometry();
        break;
    case QEvent::Move:
        d->updateGeometry();
        break;
    case QEvent::PolishRequest:
        d->updateGeometry();
        break;
    case QEvent::Show:
        d->updateUsesNativeWidgets();
        if (d->isStillAnOrphan()) {
            d->window->setParent(d->usesNativeWidgets
                                 ? windowHandle()
                                 : window()->windowHandle());
        }
        if (d->window->parent()) {
            d->markParentChain();
            d->window->show();
        }
        break;
    case QEvent::Hide:
        if (d->window->parent())
            d->window->hide();
        break;
    case QEvent::FocusIn:
        if (d->window->parent()) {
            if (d->oldFocusWindow != d->window) {
                d->window->requestActivate();
            } else {
                QWidget *next = nextInFocusChain();
                next->setFocus();
            }
        }
        break;
#if QT_CONFIG(draganddrop)
    case QEvent::Drop:
    case QEvent::DragMove:
    case QEvent::DragLeave:
        QCoreApplication::sendEvent(d->window, e);
        return e->isAccepted();
    case QEvent::DragEnter:
        // Don't reject drag events for the entire widget when one
        // item rejects the drag enter
        QCoreApplication::sendEvent(d->window, e);
        e->accept();
        return true;
#endif

    case QEvent::Paint:
    {
        static bool needsPunch = !QGuiApplicationPrivate::platformIntegration()->hasCapability(
            QPlatformIntegration::TopStackedNativeChildWindows);
        if (needsPunch) {
            QPainter p(this);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.fillRect(rect(), Qt::transparent);
        }
        break;
    }

    default:
        break;
    }

    return QWidget::event(e);
}

typedef void (*qwindowcontainer_traverse_callback)(QWidget *parent);
static void qwindowcontainer_traverse(QWidget *parent, qwindowcontainer_traverse_callback callback)
{
    const QObjectList &children = parent->children();
    for (int i=0; i<children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w) {
            QWidgetPrivate *wd = static_cast<QWidgetPrivate *>(QWidgetPrivate::get(w));
            if (wd->extra && wd->extra->hasWindowContainer)
                callback(w);
        }
    }
}

void QWindowContainer::toplevelAboutToBeDestroyed(QWidget *parent)
{
    if (QWindowContainerPrivate *d = QWindowContainerPrivate::get(parent)) {
        d->window->setParent(&d->fakeParent);
    }
    qwindowcontainer_traverse(parent, toplevelAboutToBeDestroyed);
}

void QWindowContainer::parentWasChanged(QWidget *parent)
{
    if (QWindowContainerPrivate *d = QWindowContainerPrivate::get(parent)) {
        if (d->window->parent()) {
            d->updateUsesNativeWidgets();
            d->markParentChain();
            QWidget *toplevel = d->usesNativeWidgets ? parent : parent->window();
            if (!toplevel->windowHandle()) {
                QWidgetPrivate *tld = static_cast<QWidgetPrivate *>(QWidgetPrivate::get(toplevel));
                tld->createTLExtra();
                tld->createTLSysExtra();
                Q_ASSERT(toplevel->windowHandle());
            }
            d->window->setParent(toplevel->windowHandle());
            d->updateGeometry();
        }
    }
    qwindowcontainer_traverse(parent, parentWasChanged);
}

void QWindowContainer::parentWasMoved(QWidget *parent)
{
    if (QWindowContainerPrivate *d = QWindowContainerPrivate::get(parent)) {
        if (d->window->parent())
            d->updateGeometry();
    }
    qwindowcontainer_traverse(parent, parentWasMoved);
}

void QWindowContainer::parentWasRaised(QWidget *parent)
{
    if (QWindowContainerPrivate *d = QWindowContainerPrivate::get(parent)) {
        if (d->window->parent())
            d->window->raise();
    }
    qwindowcontainer_traverse(parent, parentWasRaised);
}

void QWindowContainer::parentWasLowered(QWidget *parent)
{
    if (QWindowContainerPrivate *d = QWindowContainerPrivate::get(parent)) {
        if (d->window->parent())
            d->window->lower();
    }
    qwindowcontainer_traverse(parent, parentWasLowered);
}

QT_END_NAMESPACE

#include "moc_qwindowcontainer_p.cpp"
