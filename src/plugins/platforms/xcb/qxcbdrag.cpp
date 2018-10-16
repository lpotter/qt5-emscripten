/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qxcbdrag.h"
#include <xcb/xcb.h>
#include "qxcbconnection.h"
#include "qxcbclipboard.h"
#include "qxcbmime.h"
#include "qxcbwindow.h"
#include "qxcbscreen.h"
#include "qwindow.h"
#include "qxcbcursor.h"
#include <private/qdnd_p.h>
#include <qdebug.h>
#include <qevent.h>
#include <qguiapplication.h>
#include <qrect.h>
#include <qpainter.h>
#include <qtimer.h>

#include <qpa/qwindowsysteminterface.h>

#include <private/qguiapplication_p.h>
#include <private/qshapedpixmapdndwindow_p.h>
#include <private/qsimpledrag_p.h>
#include <private/qhighdpiscaling_p.h>

QT_BEGIN_NAMESPACE

const int xdnd_version = 5;

static inline xcb_window_t xcb_window(QPlatformWindow *w)
{
    return static_cast<QXcbWindow *>(w)->xcb_window();
}

static inline xcb_window_t xcb_window(QWindow *w)
{
    return static_cast<QXcbWindow *>(w->handle())->xcb_window();
}

static xcb_window_t xdndProxy(QXcbConnection *c, xcb_window_t w)
{
    xcb_window_t proxy = XCB_NONE;

    auto reply = Q_XCB_REPLY(xcb_get_property, c->xcb_connection(),
                             false, w, c->atom(QXcbAtom::XdndProxy), XCB_ATOM_WINDOW, 0, 1);

    if (reply && reply->type == XCB_ATOM_WINDOW)
        proxy = *((xcb_window_t *)xcb_get_property_value(reply.get()));

    if (proxy == XCB_NONE)
        return proxy;

    // exists and is real?
    reply = Q_XCB_REPLY(xcb_get_property, c->xcb_connection(),
                        false, proxy, c->atom(QXcbAtom::XdndProxy), XCB_ATOM_WINDOW, 0, 1);

    if (reply && reply->type == XCB_ATOM_WINDOW) {
        xcb_window_t p = *((xcb_window_t *)xcb_get_property_value(reply.get()));
        if (proxy != p)
            proxy = 0;
    } else {
        proxy = 0;
    }

    return proxy;
}

class QXcbDropData : public QXcbMime
{
public:
    QXcbDropData(QXcbDrag *d);
    ~QXcbDropData();

protected:
    bool hasFormat_sys(const QString &mimeType) const override;
    QStringList formats_sys() const override;
    QVariant retrieveData_sys(const QString &mimeType, QVariant::Type type) const override;

    QVariant xdndObtainData(const QByteArray &format, QVariant::Type requestedType) const;

    QXcbDrag *drag;
};


QXcbDrag::QXcbDrag(QXcbConnection *c) : QXcbObject(c)
{
    m_dropData = new QXcbDropData(this);

    init();
    cleanup_timer = -1;
}

QXcbDrag::~QXcbDrag()
{
    delete m_dropData;
}

void QXcbDrag::init()
{
    currentWindow.clear();

    accepted_drop_action = Qt::IgnoreAction;

    xdnd_dragsource = XCB_NONE;

    waiting_for_status = false;
    current_target = XCB_NONE;
    current_proxy_target = XCB_NONE;

    source_time = XCB_CURRENT_TIME;
    target_time = XCB_CURRENT_TIME;

    QXcbCursor::queryPointer(connection(), &current_virtual_desktop, 0);
    drag_types.clear();

    //current_embedding_widget = 0;

    dropped = false;
    canceled = false;

    source_sameanswer = QRect();
}

bool QXcbDrag::eventFilter(QObject *o, QEvent *e)
{
    /* We are setting a mouse grab on the QShapedPixmapWindow in order not to
     * lose the grab when the virtual desktop changes, but
     * QBasicDrag::eventFilter() expects the events to be coming from the
     * window where the drag was started. */
    if (initiatorWindow && o == shapedPixmapWindow())
        o = initiatorWindow.data();
    return QBasicDrag::eventFilter(o, e);
}

void QXcbDrag::startDrag()
{
    init();

#ifndef QT_NO_CLIPBOARD
    qCDebug(lcQpaXDnd) << "starting drag where source:" << connection()->clipboard()->owner();
    xcb_set_selection_owner(xcb_connection(), connection()->clipboard()->owner(),
                            atom(QXcbAtom::XdndSelection), connection()->time());
#endif

    QStringList fmts = QXcbMime::formatsHelper(drag()->mimeData());
    for (int i = 0; i < fmts.size(); ++i) {
        QVector<xcb_atom_t> atoms = QXcbMime::mimeAtomsForFormat(connection(), fmts.at(i));
        for (int j = 0; j < atoms.size(); ++j) {
            if (!drag_types.contains(atoms.at(j)))
                drag_types.append(atoms.at(j));
        }
    }
#ifndef QT_NO_CLIPBOARD
    if (drag_types.size() > 3)
        xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, connection()->clipboard()->owner(),
                            atom(QXcbAtom::XdndTypelist),
                            XCB_ATOM_ATOM, 32, drag_types.size(), (const void *)drag_types.constData());
#endif

    setUseCompositing(current_virtual_desktop->compositingActive());
    setScreen(current_virtual_desktop->screens().constFirst()->screen());
    initiatorWindow = QGuiApplicationPrivate::currentMouseWindow;
    QBasicDrag::startDrag();
    if (connection()->mouseGrabber() == nullptr)
        shapedPixmapWindow()->setMouseGrabEnabled(true);

    auto nativePixelPos = QHighDpi::toNativePixels(QCursor::pos(), initiatorWindow);
    move(nativePixelPos, QGuiApplication::mouseButtons(), QGuiApplication::keyboardModifiers());
}

void QXcbDrag::endDrag()
{
    QBasicDrag::endDrag();
    if (!dropped && !canceled && canDrop()) {
        // Set executed drop action when dropping outside application.
        setExecutedDropAction(accepted_drop_action);
    }
    initiatorWindow.clear();
}

static
bool windowInteractsWithPosition(xcb_connection_t *connection, const QPoint & pos, xcb_window_t w, xcb_shape_sk_t shapeType)
{
    bool interacts = false;
    auto reply = Q_XCB_REPLY(xcb_shape_get_rectangles, connection, w, shapeType);
    if (reply) {
        xcb_rectangle_t *rectangles = xcb_shape_get_rectangles_rectangles(reply.get());
        if (rectangles) {
            const int nRectangles = xcb_shape_get_rectangles_rectangles_length(reply.get());
            for (int i = 0; !interacts && i < nRectangles; ++i) {
                interacts = QRect(rectangles[i].x, rectangles[i].y, rectangles[i].width, rectangles[i].height).contains(pos);
            }
        }
    }

    return interacts;
}

xcb_window_t QXcbDrag::findRealWindow(const QPoint & pos, xcb_window_t w, int md, bool ignoreNonXdndAwareWindows)
{
    if (w == shapedPixmapWindow()->handle()->winId())
        return 0;

    if (md) {
        auto reply = Q_XCB_REPLY(xcb_get_window_attributes, xcb_connection(), w);
        if (!reply)
            return 0;

        if (reply->map_state != XCB_MAP_STATE_VIEWABLE)
            return 0;

        auto greply = Q_XCB_REPLY(xcb_get_geometry, xcb_connection(), w);
        if (!greply)
            return 0;

        QRect windowRect(greply->x, greply->y, greply->width, greply->height);
        if (windowRect.contains(pos)) {
            bool windowContainsMouse = !ignoreNonXdndAwareWindows;
            {
                auto reply = Q_XCB_REPLY(xcb_get_property, xcb_connection(),
                                         false, w, connection()->atom(QXcbAtom::XdndAware),
                                         XCB_GET_PROPERTY_TYPE_ANY, 0, 0);
                bool isAware = reply && reply->type != XCB_NONE;
                if (isAware) {
                    const QPoint relPos = pos - windowRect.topLeft();
                    // When ShapeInput and ShapeBounding are not set they return a single rectangle with the geometry of the window, this is why we
                    // need to check both here so that in the case one is set and the other is not we still get the correct result.
                    if (connection()->hasInputShape())
                        windowContainsMouse = windowInteractsWithPosition(xcb_connection(), relPos, w, XCB_SHAPE_SK_INPUT);
                    if (windowContainsMouse && connection()->hasXShape())
                        windowContainsMouse = windowInteractsWithPosition(xcb_connection(), relPos, w, XCB_SHAPE_SK_BOUNDING);
                    if (!connection()->hasInputShape() && !connection()->hasXShape())
                        windowContainsMouse = true;
                    if (windowContainsMouse)
                        return w;
                }
            }

            auto reply = Q_XCB_REPLY(xcb_query_tree, xcb_connection(), w);
            if (!reply)
                return 0;
            int nc = xcb_query_tree_children_length(reply.get());
            xcb_window_t *c = xcb_query_tree_children(reply.get());

            xcb_window_t r = 0;
            for (uint i = nc; !r && i--;)
                r = findRealWindow(pos - windowRect.topLeft(), c[i], md-1, ignoreNonXdndAwareWindows);

            if (r)
                return r;

            // We didn't find a client window!  Just use the
            // innermost window.

            // No children!
            if (!windowContainsMouse)
                return 0;
            else
                return w;
        }
    }
    return 0;
}

bool QXcbDrag::findXdndAwareTarget(const QPoint &globalPos, xcb_window_t *target_out)
{
    xcb_window_t rootwin = current_virtual_desktop->root();
    auto translate = Q_XCB_REPLY(xcb_translate_coordinates, xcb_connection(),
                                 rootwin, rootwin, globalPos.x(), globalPos.y());
    if (!translate)
        return false;

    xcb_window_t target = translate->child;
    int lx = translate->dst_x;
    int ly = translate->dst_y;

    if (target && target != rootwin) {
        xcb_window_t src = rootwin;
        while (target != 0) {
            qCDebug(lcQpaXDnd) << "checking target for XdndAware" << target;

            auto translate = Q_XCB_REPLY(xcb_translate_coordinates, xcb_connection(),
                                         src, target, lx, ly);
            if (!translate) {
                target = 0;
                break;
            }
            lx = translate->dst_x;
            ly = translate->dst_y;
            src = target;
            xcb_window_t child = translate->child;

            auto reply = Q_XCB_REPLY(xcb_get_property, xcb_connection(), false, target,
                                     atom(QXcbAtom::XdndAware), XCB_GET_PROPERTY_TYPE_ANY, 0, 0);
            bool aware = reply && reply->type != XCB_NONE;
            if (aware) {
                qCDebug(lcQpaXDnd) << "found XdndAware on" << target;
                break;
            }

            target = child;
        }

        if (!target || target == shapedPixmapWindow()->handle()->winId()) {
            qCDebug(lcQpaXDnd) << "need to find real window";
            target = findRealWindow(globalPos, rootwin, 6, true);
            if (target == 0)
                target = findRealWindow(globalPos, rootwin, 6, false);
            qCDebug(lcQpaXDnd) << "real window found" << target;
        }
    }

    *target_out = target;
    return true;
}

void QXcbDrag::move(const QPoint &globalPos, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    // The source sends XdndEnter and XdndPosition to the target.
    if (source_sameanswer.contains(globalPos) && source_sameanswer.isValid())
        return;

    QXcbVirtualDesktop *virtualDesktop = nullptr;
    QPoint cursorPos;
    QXcbCursor::queryPointer(connection(), &virtualDesktop, &cursorPos);
    QXcbScreen *screen = virtualDesktop->screenAt(cursorPos);
    QPoint deviceIndependentPos = QHighDpiScaling::mapPositionFromNative(globalPos, screen);

    if (virtualDesktop != current_virtual_desktop) {
        setUseCompositing(virtualDesktop->compositingActive());
        recreateShapedPixmapWindow(static_cast<QPlatformScreen*>(screen)->screen(), deviceIndependentPos);
        if (connection()->mouseGrabber() == nullptr)
            shapedPixmapWindow()->setMouseGrabEnabled(true);

        current_virtual_desktop = virtualDesktop;
    } else {
        QBasicDrag::moveShapedPixmapWindow(deviceIndependentPos);
    }

    xcb_window_t target;
    if (!findXdndAwareTarget(globalPos, &target))
        return;

    QXcbWindow *w = 0;
    if (target) {
        w = connection()->platformWindowFromId(target);
        if (w && (w->window()->type() == Qt::Desktop) /*&& !w->acceptDrops()*/)
            w = 0;
    } else {
        w = 0;
        target = current_virtual_desktop->root();
    }

    xcb_window_t proxy_target = xdndProxy(connection(), target);
    if (!proxy_target)
        proxy_target = target;
    int target_version = 1;

    if (proxy_target) {
        auto reply = Q_XCB_REPLY(xcb_get_property, xcb_connection(),
                                 false, proxy_target,
                                 atom(QXcbAtom::XdndAware), XCB_GET_PROPERTY_TYPE_ANY, 0, 1);
        if (!reply || reply->type == XCB_NONE)
            target = 0;

        target_version = *(uint32_t *)xcb_get_property_value(reply.get());
        target_version = qMin(xdnd_version, target_version ? target_version : 1);
    }

    if (target != current_target) {
        if (current_target)
            send_leave();

        current_target = target;
        current_proxy_target = proxy_target;
        if (target) {
            int flags = target_version << 24;
            if (drag_types.size() > 3)
                flags |= 0x0001;

            xcb_client_message_event_t enter;
            enter.response_type = XCB_CLIENT_MESSAGE;
            enter.sequence = 0;
            enter.window = target;
            enter.format = 32;
            enter.type = atom(QXcbAtom::XdndEnter);
#ifndef QT_NO_CLIPBOARD
            enter.data.data32[0] = connection()->clipboard()->owner();
#else
            enter.data.data32[0] = 0;
#endif
            enter.data.data32[1] = flags;
            enter.data.data32[2] = drag_types.size() > 0 ? drag_types.at(0) : 0;
            enter.data.data32[3] = drag_types.size() > 1 ? drag_types.at(1) : 0;
            enter.data.data32[4] = drag_types.size() > 2 ? drag_types.at(2) : 0;
            // provisionally set the rectangle to 5x5 pixels...
            source_sameanswer = QRect(globalPos.x() - 2, globalPos.y() - 2 , 5, 5);

            qCDebug(lcQpaXDnd) << "sending XdndEnter to target:" << target;

            if (w)
                handleEnter(w, &enter, current_proxy_target);
            else if (target)
                xcb_send_event(xcb_connection(), false, proxy_target, XCB_EVENT_MASK_NO_EVENT, (const char *)&enter);
            waiting_for_status = false;
        }
    }

    if (waiting_for_status)
        return;

    if (target) {
        waiting_for_status = true;
        // The source sends a ClientMessage of type XdndPosition. This tells the target the
        // position of the mouse and the action that the user requested.
        xcb_client_message_event_t move;
        move.response_type = XCB_CLIENT_MESSAGE;
        move.sequence = 0;
        move.window = target;
        move.format = 32;
        move.type = atom(QXcbAtom::XdndPosition);
#ifndef QT_NO_CLIPBOARD
        move.data.data32[0] = connection()->clipboard()->owner();
#else
        move.data.data32[0] = 0;
#endif
        move.data.data32[1] = 0; // flags
        move.data.data32[2] = (globalPos.x() << 16) + globalPos.y();
        move.data.data32[3] = connection()->time();
        move.data.data32[4] = toXdndAction(defaultAction(currentDrag()->supportedActions(), mods));

        qCDebug(lcQpaXDnd) << "sending XdndPosition to target:" << target;

        source_time = connection()->time();

        if (w)
            handle_xdnd_position(w, &move, b, mods);
        else
            xcb_send_event(xcb_connection(), false, proxy_target, XCB_EVENT_MASK_NO_EVENT, (const char *)&move);
    }

    static const bool isUnity = qgetenv("XDG_CURRENT_DESKTOP").toLower() == "unity";
    if (isUnity && xdndCollectionWindow == XCB_NONE) {
        QString name = QXcbWindow::windowTitle(connection(), target);
        if (name == QStringLiteral("XdndCollectionWindowImp"))
            xdndCollectionWindow = target;
    }
    if (target == xdndCollectionWindow) {
        setCanDrop(false);
        updateCursor(Qt::IgnoreAction);
    }
}

void QXcbDrag::drop(const QPoint &globalPos, Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    // XdndDrop is sent from source to target to complete the drop.
    QBasicDrag::drop(globalPos, b, mods);

    if (!current_target)
        return;

    xcb_client_message_event_t drop;
    drop.response_type = XCB_CLIENT_MESSAGE;
    drop.sequence = 0;
    drop.window = current_target;
    drop.format = 32;
    drop.type = atom(QXcbAtom::XdndDrop);
#ifndef QT_NO_CLIPBOARD
    drop.data.data32[0] = connection()->clipboard()->owner();
#else
    drop.data.data32[0] = 0;
#endif
    drop.data.data32[1] = 0; // flags
    drop.data.data32[2] = connection()->time();

    drop.data.data32[3] = 0;
    drop.data.data32[4] = currentDrag()->supportedActions();

    QXcbWindow *w = connection()->platformWindowFromId(current_proxy_target);

    if (w && w->window()->type() == Qt::Desktop) // && !w->acceptDrops()
        w = 0;

    Transaction t = {
        connection()->time(),
        current_target,
        current_proxy_target,
        w,
//        current_embeddig_widget,
        currentDrag(),
        QTime::currentTime()
    };
    transactions.append(t);

    // timer is needed only for drops that came from other processes.
    if (!t.targetWindow && cleanup_timer == -1) {
        cleanup_timer = startTimer(XdndDropTransactionTimeout);
    }

    qCDebug(lcQpaXDnd) << "sending drop to target:" << current_target;

    if (w) {
        handleDrop(w, &drop, b, mods);
    } else {
        xcb_send_event(xcb_connection(), false, current_proxy_target, XCB_EVENT_MASK_NO_EVENT, (const char *)&drop);
    }
}

Qt::DropAction QXcbDrag::toDropAction(xcb_atom_t a) const
{
    if (a == atom(QXcbAtom::XdndActionCopy) || a == 0)
        return Qt::CopyAction;
    if (a == atom(QXcbAtom::XdndActionLink))
        return Qt::LinkAction;
    if (a == atom(QXcbAtom::XdndActionMove))
        return Qt::MoveAction;
    return Qt::CopyAction;
}

xcb_atom_t QXcbDrag::toXdndAction(Qt::DropAction a) const
{
    switch (a) {
    case Qt::CopyAction:
        return atom(QXcbAtom::XdndActionCopy);
    case Qt::LinkAction:
        return atom(QXcbAtom::XdndActionLink);
    case Qt::MoveAction:
    case Qt::TargetMoveAction:
        return atom(QXcbAtom::XdndActionMove);
    case Qt::IgnoreAction:
        return XCB_NONE;
    default:
        return atom(QXcbAtom::XdndActionCopy);
    }
}

int QXcbDrag::findTransactionByWindow(xcb_window_t window)
{
    int at = -1;
    for (int i = 0; i < transactions.count(); ++i) {
        const Transaction &t = transactions.at(i);
        if (t.target == window || t.proxy_target == window) {
            at = i;
            break;
        }
    }
    return at;
}

int QXcbDrag::findTransactionByTime(xcb_timestamp_t timestamp)
{
    int at = -1;
    for (int i = 0; i < transactions.count(); ++i) {
        const Transaction &t = transactions.at(i);
        if (t.timestamp == timestamp) {
            at = i;
            break;
        }
    }
    return at;
}

#if 0
// for embedding only
static QWidget* current_embedding_widget  = 0;
static xcb_client_message_event_t last_enter_event;


static bool checkEmbedded(QWidget* w, const XEvent* xe)
{
    if (!w)
        return false;

    if (current_embedding_widget != 0 && current_embedding_widget != w) {
        current_target = ((QExtraWidget*)current_embedding_widget)->extraData()->xDndProxy;
        current_proxy_target = current_target;
        qt_xdnd_send_leave();
        current_target = 0;
        current_proxy_target = 0;
        current_embedding_widget = 0;
    }

    QWExtra* extra = ((QExtraWidget*)w)->extraData();
    if (extra && extra->xDndProxy != 0) {

        if (current_embedding_widget != w) {

            last_enter_event.xany.window = extra->xDndProxy;
            XSendEvent(X11->display, extra->xDndProxy, False, NoEventMask, &last_enter_event);
            current_embedding_widget = w;
        }

        ((XEvent*)xe)->xany.window = extra->xDndProxy;
        XSendEvent(X11->display, extra->xDndProxy, False, NoEventMask, (XEvent*)xe);
        if (currentWindow != w) {
            currentWindow = w;
        }
        return true;
    }
    current_embedding_widget = 0;
    return false;
}
#endif

void QXcbDrag::handleEnter(QPlatformWindow *, const xcb_client_message_event_t *event, xcb_window_t proxy)
{
    // The target receives XdndEnter.
    qCDebug(lcQpaXDnd) << "target:" << event->window << "received XdndEnter";

    xdnd_types.clear();

    int version = (int)(event->data.data32[1] >> 24);
    if (version > xdnd_version)
        return;

    xdnd_dragsource = event->data.data32[0];
    if (!proxy)
        proxy = xdndProxy(connection(), xdnd_dragsource);
    current_proxy_target = proxy ? proxy : xdnd_dragsource;

    if (event->data.data32[1] & 1) {
        // get the types from XdndTypeList
        auto reply = Q_XCB_REPLY(xcb_get_property, xcb_connection(), false, xdnd_dragsource,
                                 atom(QXcbAtom::XdndTypelist), XCB_ATOM_ATOM,
                                 0, xdnd_max_type);
        if (reply && reply->type != XCB_NONE && reply->format == 32) {
            int length = xcb_get_property_value_length(reply.get()) / 4;
            if (length > xdnd_max_type)
                length = xdnd_max_type;

            xcb_atom_t *atoms = (xcb_atom_t *)xcb_get_property_value(reply.get());
            xdnd_types.reserve(length);
            for (int i = 0; i < length; ++i)
                xdnd_types.append(atoms[i]);
        }
    } else {
        // get the types from the message
        for(int i = 2; i < 5; i++) {
            if (event->data.data32[i])
                xdnd_types.append(event->data.data32[i]);
        }
    }
    for(int i = 0; i < xdnd_types.length(); ++i)
        qCDebug(lcQpaXDnd) << "    " << connection()->atomName(xdnd_types.at(i));
}

void QXcbDrag::handle_xdnd_position(QPlatformWindow *w, const xcb_client_message_event_t *e,
                                    Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    // The target receives XdndPosition. The target window must determine which widget the mouse
    // is in and ask it whether or not it will accept the drop.
    qCDebug(lcQpaXDnd) << "target:" << e->window << "received XdndPosition";

    QPoint p((e->data.data32[2] & 0xffff0000) >> 16, e->data.data32[2] & 0x0000ffff);
    Q_ASSERT(w);
    QRect geometry = w->geometry();
    p -= geometry.topLeft();

    if (!w || !w->window() || (w->window()->type() == Qt::Desktop))
        return;

    if (Q_UNLIKELY(e->data.data32[0] != xdnd_dragsource)) {
        qCDebug(lcQpaXDnd, "xdnd drag position from unexpected source (%x not %x)",
                e->data.data32[0], xdnd_dragsource);
        return;
    }

    currentPosition = p;
    currentWindow = w->window();

    // timestamp from the source
    if (e->data.data32[3] != XCB_NONE) {
        target_time = e->data.data32[3];
    }

    QMimeData *dropData = 0;
    Qt::DropActions supported_actions = Qt::IgnoreAction;
    if (currentDrag()) {
        dropData = currentDrag()->mimeData();
        supported_actions = currentDrag()->supportedActions();
    } else {
        dropData = m_dropData;
        supported_actions = Qt::DropActions(toDropAction(e->data.data32[4]));
    }

    auto buttons = currentDrag() ? b : connection()->queryMouseButtons();
    auto modifiers = currentDrag() ? mods : connection()->queryKeyboardModifiers();

    QPlatformDragQtResponse qt_response = QWindowSystemInterface::handleDrag(
                w->window(), dropData, p, supported_actions, buttons, modifiers);

    // ### FIXME ? - answerRect appears to be unused.
    QRect answerRect(p + geometry.topLeft(), QSize(1,1));
    answerRect = qt_response.answerRect().translated(geometry.topLeft()).intersected(geometry);

    // The target sends a ClientMessage of type XdndStatus. This tells the source whether or not
    // it will accept the drop, and, if so, what action will be taken. It also includes a rectangle
    // that means "don't send another XdndPosition message until the mouse moves out of here".
    xcb_client_message_event_t response;
    response.response_type = XCB_CLIENT_MESSAGE;
    response.sequence = 0;
    response.window = xdnd_dragsource;
    response.format = 32;
    response.type = atom(QXcbAtom::XdndStatus);
    response.data.data32[0] = xcb_window(w);
    response.data.data32[1] = qt_response.isAccepted(); // flags
    response.data.data32[2] = 0; // x, y
    response.data.data32[3] = 0; // w, h
    response.data.data32[4] = toXdndAction(qt_response.acceptedAction()); // action

    accepted_drop_action = qt_response.acceptedAction();

    if (answerRect.left() < 0)
        answerRect.setLeft(0);
    if (answerRect.right() > 4096)
        answerRect.setRight(4096);
    if (answerRect.top() < 0)
        answerRect.setTop(0);
    if (answerRect.bottom() > 4096)
        answerRect.setBottom(4096);
    if (answerRect.width() < 0)
        answerRect.setWidth(0);
    if (answerRect.height() < 0)
        answerRect.setHeight(0);

    // reset
    target_time = XCB_CURRENT_TIME;

    qCDebug(lcQpaXDnd) << "sending XdndStatus to source:" << xdnd_dragsource;

#ifndef QT_NO_CLIPBOARD
    if (xdnd_dragsource == connection()->clipboard()->owner())
        handle_xdnd_status(&response);
    else
#endif
        xcb_send_event(xcb_connection(), false, current_proxy_target,
                       XCB_EVENT_MASK_NO_EVENT, (const char *)&response);
}

namespace
{
    class ClientMessageScanner {
    public:
        ClientMessageScanner(xcb_atom_t a) : atom(a) {}
        xcb_atom_t atom;
        bool operator() (xcb_generic_event_t *event, int type) const {
            if (type != XCB_CLIENT_MESSAGE)
                return false;
            auto clientMessage = reinterpret_cast<xcb_client_message_event_t *>(event);
            return clientMessage->type == atom;
        }
    };
}

void QXcbDrag::handlePosition(QPlatformWindow * w, const xcb_client_message_event_t *event)
{
    xcb_client_message_event_t *lastEvent = const_cast<xcb_client_message_event_t *>(event);
    ClientMessageScanner scanner(atom(QXcbAtom::XdndPosition));
    while (auto nextEvent = connection()->checkEvent(scanner)) {
        if (lastEvent != event)
            free(lastEvent);
        lastEvent = reinterpret_cast<xcb_client_message_event_t *>(nextEvent);
    }

    handle_xdnd_position(w, lastEvent);
    if (lastEvent != event)
        free(lastEvent);
}

void QXcbDrag::handle_xdnd_status(const xcb_client_message_event_t *event)
{
    // The source receives XdndStatus. It can use the action to change the cursor to indicate
    // whether or not the user's requested action will be performed.
    qCDebug(lcQpaXDnd) << "source:" << event->window << "received XdndStatus";
    waiting_for_status = false;
    // ignore late status messages
    if (event->data.data32[0] && event->data.data32[0] != current_target)
        return;

    const bool dropPossible = event->data.data32[1];
    setCanDrop(dropPossible);

    if (dropPossible) {
        accepted_drop_action = toDropAction(event->data.data32[4]);
        updateCursor(accepted_drop_action);
    } else {
        updateCursor(Qt::IgnoreAction);
    }

    if ((event->data.data32[1] & 2) == 0) {
        QPoint p((event->data.data32[2] & 0xffff0000) >> 16, event->data.data32[2] & 0x0000ffff);
        QSize s((event->data.data32[3] & 0xffff0000) >> 16, event->data.data32[3] & 0x0000ffff);
        source_sameanswer = QRect(p, s);
    } else {
        source_sameanswer = QRect();
    }
}

void QXcbDrag::handleStatus(const xcb_client_message_event_t *event)
{
    if (
#ifndef QT_NO_CLIPBOARD
            event->window != connection()->clipboard()->owner() ||
#endif
            !drag())
        return;

    xcb_client_message_event_t *lastEvent = const_cast<xcb_client_message_event_t *>(event);
    xcb_generic_event_t *nextEvent;
    ClientMessageScanner scanner(atom(QXcbAtom::XdndStatus));
    while ((nextEvent = connection()->checkEvent(scanner))) {
        if (lastEvent != event)
            free(lastEvent);
        lastEvent = (xcb_client_message_event_t *)nextEvent;
    }

    handle_xdnd_status(lastEvent);
    if (lastEvent != event)
        free(lastEvent);
}

void QXcbDrag::handleLeave(QPlatformWindow *w, const xcb_client_message_event_t *event)
{
    // If the target receives XdndLeave, it frees any cached data and forgets the whole incident.
    qCDebug(lcQpaXDnd) << "target:" << event->window << "received XdndLeave";

    if (!currentWindow || w != currentWindow.data()->handle())
        return; // sanity

    // ###
//    if (checkEmbedded(current_embedding_widget, event)) {
//        current_embedding_widget = 0;
//        currentWindow.clear();
//        return;
//    }

    if (event->data.data32[0] != xdnd_dragsource) {
        // This often happens - leave other-process window quickly
        qCDebug(lcQpaXDnd, "xdnd drag leave from unexpected source (%x not %x",
                event->data.data32[0], xdnd_dragsource);
    }

    QWindowSystemInterface::handleDrag(w->window(), nullptr, QPoint(), Qt::IgnoreAction, 0, 0);
}

void QXcbDrag::send_leave()
{
    // XdndLeave is sent from the source to the target to cancel the drop.
    if (!current_target)
        return;

    xcb_client_message_event_t leave;
    leave.response_type = XCB_CLIENT_MESSAGE;
    leave.sequence = 0;
    leave.window = current_target;
    leave.format = 32;
    leave.type = atom(QXcbAtom::XdndLeave);
#ifndef QT_NO_CLIPBOARD
    leave.data.data32[0] = connection()->clipboard()->owner();
#else
    leave.data.data32[0] = 0;
#endif
    leave.data.data32[1] = 0; // flags
    leave.data.data32[2] = 0; // x, y
    leave.data.data32[3] = 0; // w, h
    leave.data.data32[4] = 0; // just null

    QXcbWindow *w = connection()->platformWindowFromId(current_proxy_target);

    if (w && (w->window()->type() == Qt::Desktop) /*&& !w->acceptDrops()*/)
        w = 0;

    qCDebug(lcQpaXDnd) << "sending XdndLeave to target:" << current_target;

    if (w)
        handleLeave(w, (const xcb_client_message_event_t *)&leave);
    else
        xcb_send_event(xcb_connection(), false,current_proxy_target,
                       XCB_EVENT_MASK_NO_EVENT, (const char *)&leave);
}

void QXcbDrag::handleDrop(QPlatformWindow *, const xcb_client_message_event_t *event,
                          Qt::MouseButtons b, Qt::KeyboardModifiers mods)
{
    // Target receives XdndDrop. Once it is finished processing the drop, it sends XdndFinished.
    qCDebug(lcQpaXDnd) << "target:" << event->window << "received XdndDrop";

    if (!currentWindow) {
        xdnd_dragsource = 0;
        return; // sanity
    }

    const uint32_t *l = event->data.data32;

    if (l[0] != xdnd_dragsource) {
        qCDebug(lcQpaXDnd, "xdnd drop from unexpected source (%x not %x", l[0], xdnd_dragsource);
        return;
    }

    // update the "user time" from the timestamp in the event.
    if (l[2] != 0)
        target_time = l[2];

    Qt::DropActions supported_drop_actions;
    QMimeData *dropData = 0;
    if (currentDrag()) {
        dropData = currentDrag()->mimeData();
        supported_drop_actions = Qt::DropActions(l[4]);
    } else {
        dropData = m_dropData;
        supported_drop_actions = accepted_drop_action;
    }

    if (!dropData)
        return;
    // ###
    //        int at = findXdndDropTransactionByTime(target_time);
    //        if (at != -1)
    //            dropData = QDragManager::dragPrivate(X11->dndDropTransactions.at(at).object)->data;
    // if we can't find it, then use the data in the drag manager

    auto buttons = currentDrag() ? b : connection()->queryMouseButtons();
    auto modifiers = currentDrag() ? mods : connection()->queryKeyboardModifiers();

    QPlatformDropQtResponse response = QWindowSystemInterface::handleDrop(
                currentWindow.data(), dropData, currentPosition, supported_drop_actions,
                buttons, modifiers);

    setExecutedDropAction(response.acceptedAction());

    xcb_client_message_event_t finished;
    finished.response_type = XCB_CLIENT_MESSAGE;
    finished.sequence = 0;
    finished.window = xdnd_dragsource;
    finished.format = 32;
    finished.type = atom(QXcbAtom::XdndFinished);
    finished.data.data32[0] = currentWindow ? xcb_window(currentWindow.data()) : XCB_NONE;
    finished.data.data32[1] = response.isAccepted(); // flags
    finished.data.data32[2] = toXdndAction(response.acceptedAction());

    qCDebug(lcQpaXDnd) << "sending XdndFinished to source:" << xdnd_dragsource;

    xcb_send_event(xcb_connection(), false, current_proxy_target,
                   XCB_EVENT_MASK_NO_EVENT, (char *)&finished);

    dropped = true;
}

void QXcbDrag::handleFinished(const xcb_client_message_event_t *event)
{
    // Source receives XdndFinished when target is done processing the drop data.
    qCDebug(lcQpaXDnd) << "source:" << event->window << "received XdndFinished";

#ifndef QT_NO_CLIPBOARD
    if (event->window != connection()->clipboard()->owner())
        return;
#endif

    const unsigned long *l = (const unsigned long *)event->data.data32;
    if (l[0]) {
        int at = findTransactionByWindow(l[0]);
        if (at != -1) {

            Transaction t = transactions.takeAt(at);
            if (t.drag)
                t.drag->deleteLater();
//            QDragManager *manager = QDragManager::self();

//            Window target = current_target;
//            Window proxy_target = current_proxy_target;
//            QWidget *embedding_widget = current_embedding_widget;
//            QDrag *currentObject = manager->object;

//            current_target = t.target;
//            current_proxy_target = t.proxy_target;
//            current_embedding_widget = t.embedding_widget;
//            manager->object = t.object;

//            if (!passive)
//                (void) checkEmbedded(currentWindow, xe);

//            current_embedding_widget = 0;
//            current_target = 0;
//            current_proxy_target = 0;

//            current_target = target;
//            current_proxy_target = proxy_target;
//            current_embedding_widget = embedding_widget;
//            manager->object = currentObject;
        } else {
            qWarning("QXcbDrag::handleFinished - drop data has expired");
        }
    }
    waiting_for_status = false;
}

void QXcbDrag::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == cleanup_timer) {
        bool stopTimer = true;
        for (int i = 0; i < transactions.count(); ++i) {
            const Transaction &t = transactions.at(i);
            if (t.targetWindow) {
                // dnd within the same process, don't delete, these are taken care of
                // in handleFinished()
                continue;
            }
            QTime currentTime = QTime::currentTime();
            int delta = t.time.msecsTo(currentTime);
            if (delta > XdndDropTransactionTimeout) {
                /* delete transactions which are older than XdndDropTransactionTimeout. It could mean
                 one of these:
                 - client has crashed and as a result we have never received XdndFinished
                 - showing dialog box on drop event where user's response takes more time than XdndDropTransactionTimeout (QTBUG-14493)
                 - dnd takes unusually long time to process data
                 */
                if (t.drag)
                    t.drag->deleteLater();
                transactions.removeAt(i--);
            } else {
                stopTimer = false;
            }

        }
        if (stopTimer && cleanup_timer != -1) {
            killTimer(cleanup_timer);
            cleanup_timer = -1;
        }
    }
}

void QXcbDrag::cancel()
{
    qCDebug(lcQpaXDnd) << "dnd was canceled";

    QBasicDrag::cancel();
    if (current_target)
        send_leave();

    // remove canceled object
    currentDrag()->deleteLater();

    canceled = true;
}

static xcb_window_t findXdndAwareParent(QXcbConnection *c, xcb_window_t window)
{
    xcb_window_t target = 0;
    forever {
        // check if window has XdndAware
        auto gpReply = Q_XCB_REPLY(xcb_get_property, c->xcb_connection(), false, window,
                                   c->atom(QXcbAtom::XdndAware), XCB_GET_PROPERTY_TYPE_ANY, 0, 0);
        bool aware = gpReply && gpReply->type != XCB_NONE;
        if (aware) {
            target = window;
            break;
        }

        // try window's parent
        auto qtReply = Q_XCB_REPLY_UNCHECKED(xcb_query_tree, c->xcb_connection(), window);
        if (!qtReply)
            break;
        xcb_window_t root = qtReply->root;
        xcb_window_t parent = qtReply->parent;
        if (window == root)
            break;
        window = parent;
    }
    return target;
}

void QXcbDrag::handleSelectionRequest(const xcb_selection_request_event_t *event)
{
    qCDebug(lcQpaXDnd) << "handle selection request from target:" << event->requestor;
    q_padded_xcb_event<xcb_selection_notify_event_t> notify = {};
    notify.response_type = XCB_SELECTION_NOTIFY;
    notify.requestor = event->requestor;
    notify.selection = event->selection;
    notify.target = XCB_NONE;
    notify.property = XCB_NONE;
    notify.time = event->time;

    // which transaction do we use? (note: -2 means use current currentDrag())
    int at = -1;

    // figure out which data the requestor is really interested in
    if (currentDrag() && event->time == source_time) {
        // requestor wants the current drag data
        at = -2;
    } else {
        // if someone has requested data in response to XdndDrop, find the corresponding transaction. the
        // spec says to call xcb_convert_selection() using the timestamp from the XdndDrop
        at = findTransactionByTime(event->time);
        if (at == -1) {
            // no dice, perhaps the client was nice enough to use the same window id in
            // xcb_convert_selection() that we sent the XdndDrop event to.
            at = findTransactionByWindow(event->requestor);
        }

        if (at == -1) {
            xcb_window_t target = findXdndAwareParent(connection(), event->requestor);
            if (target) {
                if (event->time == XCB_CURRENT_TIME && current_target == target)
                    at = -2;
                else
                    at = findTransactionByWindow(target);
            }
        }
    }

    QDrag *transactionDrag = 0;
    if (at >= 0) {
        transactionDrag = transactions.at(at).drag;
    } else if (at == -2) {
        transactionDrag = currentDrag();
    }

    if (transactionDrag) {
        xcb_atom_t atomFormat = event->target;
        int dataFormat = 0;
        QByteArray data;
        if (QXcbMime::mimeDataForAtom(connection(), event->target, transactionDrag->mimeData(),
                                     &data, &atomFormat, &dataFormat)) {
            int dataSize = data.size() / (dataFormat / 8);
            xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, event->requestor, event->property,
                                atomFormat, dataFormat, dataSize, (const void *)data.constData());
            notify.property = event->property;
            notify.target = atomFormat;
        }
    }

    xcb_window_t proxy_target = xdndProxy(connection(), event->requestor);
    if (!proxy_target)
        proxy_target = event->requestor;

    xcb_send_event(xcb_connection(), false, proxy_target, XCB_EVENT_MASK_NO_EVENT, (const char *)&notify);
}


bool QXcbDrag::dndEnable(QXcbWindow *w, bool on)
{
    // Windows announce that they support the XDND protocol by creating a window property XdndAware.
    if (on) {
        QXcbWindow *window = nullptr;
        if (w->window()->type() == Qt::Desktop) {
            if (desktop_proxy) // *WE* already have one.
                return false;

            QXcbConnectionGrabber grabber(connection());

            // As per Xdnd4, use XdndProxy
            xcb_window_t proxy_id = xdndProxy(connection(), w->xcb_window());

            if (!proxy_id) {
                desktop_proxy = new QWindow;
                window = static_cast<QXcbWindow *>(desktop_proxy->handle());
                proxy_id = window->xcb_window();
                xcb_atom_t xdnd_proxy = atom(QXcbAtom::XdndProxy);
                xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, w->xcb_window(), xdnd_proxy,
                                    XCB_ATOM_WINDOW, 32, 1, &proxy_id);
                xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, proxy_id, xdnd_proxy,
                                    XCB_ATOM_WINDOW, 32, 1, &proxy_id);
            }

        } else {
            window = w;
        }
        if (window) {
            qCDebug(lcQpaXDnd) << "setting XdndAware for" << window->xcb_window();
            xcb_atom_t atm = xdnd_version;
            xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, window->xcb_window(),
                                atom(QXcbAtom::XdndAware), XCB_ATOM_ATOM, 32, 1, &atm);
            return true;
        } else {
            return false;
        }
    } else {
        if (w->window()->type() == Qt::Desktop) {
            xcb_delete_property(xcb_connection(), w->xcb_window(), atom(QXcbAtom::XdndProxy));
            delete desktop_proxy;
            desktop_proxy = 0;
        } else {
            qCDebug(lcQpaXDnd) << "not deleting XDndAware";
        }
        return true;
    }
}

bool QXcbDrag::ownsDragObject() const
{
    return true;
}

QXcbDropData::QXcbDropData(QXcbDrag *d)
    : QXcbMime(),
      drag(d)
{
}

QXcbDropData::~QXcbDropData()
{
}

QVariant QXcbDropData::retrieveData_sys(const QString &mimetype, QVariant::Type requestedType) const
{
    QByteArray mime = mimetype.toLatin1();
    QVariant data = xdndObtainData(mime, requestedType);
    return data;
}

QVariant QXcbDropData::xdndObtainData(const QByteArray &format, QVariant::Type requestedType) const
{
    QByteArray result;

    QXcbConnection *c = drag->connection();
    QXcbWindow *xcb_window = c->platformWindowFromId(drag->xdnd_dragsource);
    if (xcb_window && drag->currentDrag() && xcb_window->window()->type() != Qt::Desktop) {
        QMimeData *data = drag->currentDrag()->mimeData();
        if (data->hasFormat(QLatin1String(format)))
            result = data->data(QLatin1String(format));
        return result;
    }

    QVector<xcb_atom_t> atoms = drag->xdnd_types;
    QByteArray encoding;
    xcb_atom_t a = mimeAtomForFormat(c, QLatin1String(format), requestedType, atoms, &encoding);
    if (a == XCB_NONE)
        return result;

#ifndef QT_NO_CLIPBOARD
    if (c->clipboard()->getSelectionOwner(drag->atom(QXcbAtom::XdndSelection)) == XCB_NONE)
        return result; // should never happen?

    xcb_atom_t xdnd_selection = c->atom(QXcbAtom::XdndSelection);
    result = c->clipboard()->getSelection(xdnd_selection, a, xdnd_selection, drag->targetTime());
#endif

    return mimeConvertToFormat(c, a, result, QLatin1String(format), requestedType, encoding);
}

bool QXcbDropData::hasFormat_sys(const QString &format) const
{
    return formats().contains(format);
}

QStringList QXcbDropData::formats_sys() const
{
    QStringList formats;
    for (int i = 0; i < drag->xdnd_types.size(); ++i) {
        QString f = mimeAtomToString(drag->connection(), drag->xdnd_types.at(i));
        if (!formats.contains(f))
            formats.append(f);
    }
    return formats;
}

QT_END_NAMESPACE
