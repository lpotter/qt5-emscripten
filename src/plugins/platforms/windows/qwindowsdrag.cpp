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

#include "qwindowsdrag.h"
#include "qwindowscontext.h"
#include "qwindowsscreen.h"
#if QT_CONFIG(clipboard)
#  include "qwindowsclipboard.h"
#endif
#include "qwindowsintegration.h"
#include "qwindowsdropdataobject.h"
#include <QtCore/qt_windows.h>
#include "qwindowswindow.h"
#include "qwindowsmousehandler.h"
#include "qwindowscursor.h"

#include <QtGui/qevent.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qpainter.h>
#include <QtGui/qrasterwindow.h>
#include <QtGui/qguiapplication.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <QtGui/private/qdnd_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qpoint.h>

#include <shlobj.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsDragCursorWindow
    \brief A toplevel window showing the drag icon in case of touch drag.

    \sa QWindowsOleDropSource
    \internal
    \ingroup qt-lighthouse-win
*/

class QWindowsDragCursorWindow : public QRasterWindow
{
public:
    explicit QWindowsDragCursorWindow(QWindow *parent = 0);

    void setPixmap(const QPixmap &p);

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.drawPixmap(0, 0, m_pixmap);
    }

private:
    QPixmap m_pixmap;
};

QWindowsDragCursorWindow::QWindowsDragCursorWindow(QWindow *parent)
    : QRasterWindow(parent)
{
    QSurfaceFormat windowFormat = format();
    windowFormat.setAlphaBufferSize(8);
    setFormat(windowFormat);
    setObjectName(QStringLiteral("QWindowsDragCursorWindow"));
    setFlags(Qt::Popup | Qt::NoDropShadowWindowHint
             | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
             | Qt::WindowDoesNotAcceptFocus | Qt::WindowTransparentForInput);
}

void QWindowsDragCursorWindow::setPixmap(const QPixmap &p)
{
    if (p.cacheKey() == m_pixmap.cacheKey())
        return;
    const QSize oldSize = m_pixmap.size();
    QSize newSize = p.size();
    qCDebug(lcQpaMime) << __FUNCTION__ << p.cacheKey() << newSize;
    m_pixmap = p;
    if (oldSize != newSize) {
        const qreal pixDevicePixelRatio = p.devicePixelRatio();
        if (pixDevicePixelRatio > 1.0 && qFuzzyCompare(pixDevicePixelRatio, devicePixelRatio()))
            newSize /= qRound(pixDevicePixelRatio);
        resize(newSize);
    }
    if (isVisible())
        update();
}

/*!
    \class QWindowsDropMimeData
    \brief Special mime data class for data retrieval from Drag operations.

    Implementation of QWindowsInternalMimeDataBase which retrieves the
    current drop data object from QWindowsDrag.

    \sa QWindowsDrag
    \internal
    \ingroup qt-lighthouse-win
*/

IDataObject *QWindowsDropMimeData::retrieveDataObject() const
{
    return QWindowsDrag::instance()->dropDataObject();
}

static inline Qt::DropActions translateToQDragDropActions(DWORD pdwEffects)
{
    Qt::DropActions actions = Qt::IgnoreAction;
    if (pdwEffects & DROPEFFECT_LINK)
        actions |= Qt::LinkAction;
    if (pdwEffects & DROPEFFECT_COPY)
        actions |= Qt::CopyAction;
    if (pdwEffects & DROPEFFECT_MOVE)
        actions |= Qt::MoveAction;
    return actions;
}

static inline Qt::DropAction translateToQDragDropAction(DWORD pdwEffect)
{
    if (pdwEffect & DROPEFFECT_LINK)
        return Qt::LinkAction;
    if (pdwEffect & DROPEFFECT_COPY)
        return Qt::CopyAction;
    if (pdwEffect & DROPEFFECT_MOVE)
        return Qt::MoveAction;
    return Qt::IgnoreAction;
}

static inline DWORD translateToWinDragEffects(Qt::DropActions action)
{
    DWORD effect = DROPEFFECT_NONE;
    if (action & Qt::LinkAction)
        effect |= DROPEFFECT_LINK;
    if (action & Qt::CopyAction)
        effect |= DROPEFFECT_COPY;
    if (action & Qt::MoveAction)
        effect |= DROPEFFECT_MOVE;
    return effect;
}

static inline Qt::KeyboardModifiers toQtKeyboardModifiers(DWORD keyState)
{
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    if (keyState & MK_SHIFT)
        modifiers |= Qt::ShiftModifier;
    if (keyState & MK_CONTROL)
        modifiers |= Qt::ControlModifier;
    if (keyState & MK_ALT)
        modifiers |= Qt::AltModifier;

    return modifiers;
}

static inline Qt::MouseButtons toQtMouseButtons(DWORD keyState)
{
    Qt::MouseButtons buttons = Qt::NoButton;

    if (keyState & MK_LBUTTON)
        buttons |= Qt::LeftButton;
    if (keyState & MK_RBUTTON)
        buttons |= Qt::RightButton;
    if (keyState & MK_MBUTTON)
        buttons |= Qt::MidButton;

    return buttons;
}

/*!
    \class QWindowsOleDropSource
    \brief Implementation of IDropSource

    Used for drag operations.

    \sa QWindowsDrag
    \internal
    \ingroup qt-lighthouse-win
*/

class QWindowsOleDropSource : public QWindowsComBase<IDropSource>
{
public:
    enum Mode {
        MouseDrag,
        TouchDrag // Mouse cursor suppressed, use window as cursor.
    };

    explicit QWindowsOleDropSource(QWindowsDrag *drag);
    ~QWindowsOleDropSource() override;

    void createCursors();

    // IDropSource methods
    STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState);
    STDMETHOD(GiveFeedback)(DWORD dwEffect);

private:
    struct CursorEntry {
        CursorEntry() : cacheKey(0) {}
        CursorEntry(const QPixmap &p, qint64 cK, const CursorHandlePtr &c, const QPoint &h) :
            pixmap(p), cacheKey(cK), cursor(c), hotSpot(h) {}

        QPixmap pixmap;
        qint64 cacheKey; // Cache key of cursor
        CursorHandlePtr cursor;
        QPoint hotSpot;
    };

    typedef QMap<Qt::DropAction, CursorEntry> ActionCursorMap;

    Mode m_mode;
    QWindowsDrag *m_drag;
    QPointer<QWindow> m_windowUnderMouse;
    Qt::MouseButtons m_currentButtons;
    ActionCursorMap m_cursors;
    QWindowsDragCursorWindow *m_touchDragWindow;

#ifndef QT_NO_DEBUG_STREAM
    friend QDebug operator<<(QDebug, const QWindowsOleDropSource::CursorEntry &);
#endif
};

QWindowsOleDropSource::QWindowsOleDropSource(QWindowsDrag *drag)
    : m_mode(QWindowsCursor::cursorState() != QWindowsCursor::State::Suppressed ? MouseDrag : TouchDrag)
    , m_drag(drag)
    , m_windowUnderMouse(QWindowsContext::instance()->windowUnderMouse())
    , m_currentButtons(Qt::NoButton)
    , m_touchDragWindow(0)
{
    qCDebug(lcQpaMime) << __FUNCTION__ << m_mode;
}

QWindowsOleDropSource::~QWindowsOleDropSource()
{
    m_cursors.clear();
    delete m_touchDragWindow;
    qCDebug(lcQpaMime) << __FUNCTION__;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QWindowsOleDropSource::CursorEntry &e)
{
    d << "CursorEntry:" << e.pixmap.size() << '#' << e.cacheKey
      << "HCURSOR" << e.cursor->handle() << "hotspot:" << e.hotSpot;
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

/*!
    \brief Blend custom pixmap with cursors.
*/

void QWindowsOleDropSource::createCursors()
{
    const QDrag *drag = m_drag->currentDrag();
    const QPixmap pixmap = drag->pixmap();
    const bool hasPixmap = !pixmap.isNull();

    // Find screen for drag. Could be obtained from QDrag::source(), but that might be a QWidget.
    const QPlatformScreen *platformScreen = QWindowsContext::instance()->screenManager().screenAtDp(QWindowsCursor::mousePosition());
    if (!platformScreen) {
        if (const QScreen *primaryScreen = QGuiApplication::primaryScreen())
            platformScreen = primaryScreen->handle();
    }
    Q_ASSERT(platformScreen);
    QPlatformCursor *platformCursor = platformScreen->cursor();

    if (GetSystemMetrics (SM_REMOTESESSION) != 0) {
        /* Workaround for RDP issues with large cursors.
         * Touch drag window seems to work just fine...
         * 96 pixel is a 'large' mouse cursor, according to RDP spec */
        const int rdpLargeCursor = qRound(qreal(96) / QHighDpiScaling::factor(platformScreen));
        if (pixmap.width() > rdpLargeCursor || pixmap.height() > rdpLargeCursor)
            m_mode = TouchDrag;
    }

    qreal pixmapScaleFactor = 1;
    qreal hotSpotScaleFactor = 1;
    if (m_mode != TouchDrag) { // Touch drag: pixmap is shown in a separate QWindow, which will be scaled.)
        hotSpotScaleFactor = QHighDpiScaling::factor(platformScreen);
        pixmapScaleFactor = hotSpotScaleFactor / pixmap.devicePixelRatio();
    }
    QPixmap scaledPixmap = qFuzzyCompare(pixmapScaleFactor, 1.0)
        ? pixmap
        :  pixmap.scaled((QSizeF(pixmap.size()) * pixmapScaleFactor).toSize(),
                         Qt::KeepAspectRatio, Qt::SmoothTransformation);
    scaledPixmap.setDevicePixelRatio(1);

    Qt::DropAction actions[] = { Qt::MoveAction, Qt::CopyAction, Qt::LinkAction, Qt::IgnoreAction };
    int actionCount = int(sizeof(actions) / sizeof(actions[0]));
    if (!hasPixmap)
        --actionCount; // No Qt::IgnoreAction unless pixmap
    const QPoint hotSpot = qFuzzyCompare(hotSpotScaleFactor, 1.0)
        ?  drag->hotSpot()
        : (QPointF(drag->hotSpot()) * hotSpotScaleFactor).toPoint();
    for (int cnum = 0; cnum < actionCount; ++cnum) {
        const Qt::DropAction action = actions[cnum];
        QPixmap cursorPixmap = drag->dragCursor(action);
        if (cursorPixmap.isNull() && platformCursor)
            cursorPixmap = static_cast<QWindowsCursor *>(platformCursor)->dragDefaultCursor(action);
        const qint64 cacheKey = cursorPixmap.cacheKey();
        const auto it = m_cursors.find(action);
        if (it != m_cursors.end() && it.value().cacheKey == cacheKey)
            continue;
        if (cursorPixmap.isNull()) {
            qWarning("%s: Unable to obtain drag cursor for %d.", __FUNCTION__, action);
            continue;
        }

        QPoint newHotSpot(0, 0);
        QPixmap newPixmap = cursorPixmap;

        if (hasPixmap) {
            const int x1 = qMin(-hotSpot.x(), 0);
            const int x2 = qMax(scaledPixmap.width() - hotSpot.x(), cursorPixmap.width());
            const int y1 = qMin(-hotSpot.y(), 0);
            const int y2 = qMax(scaledPixmap.height() - hotSpot.y(), cursorPixmap.height());
            QPixmap newCursor(x2 - x1 + 1, y2 - y1 + 1);
            newCursor.fill(Qt::transparent);
            QPainter p(&newCursor);
            const QPoint pmDest = QPoint(qMax(0, -hotSpot.x()), qMax(0, -hotSpot.y()));
            p.drawPixmap(pmDest, scaledPixmap);
            p.drawPixmap(qMax(0, hotSpot.x()),qMax(0, hotSpot.y()), cursorPixmap);
            newPixmap = newCursor;
            newHotSpot = QPoint(qMax(0, hotSpot.x()), qMax(0, hotSpot.y()));
        }

        if (const HCURSOR sysCursor = QWindowsCursor::createPixmapCursor(newPixmap, newHotSpot)) {
            const CursorEntry entry(newPixmap, cacheKey, CursorHandlePtr(new CursorHandle(sysCursor)), newHotSpot);
            if (it == m_cursors.end())
                m_cursors.insert(action, entry);
            else
                it.value() = entry;
        }
    }
#ifndef QT_NO_DEBUG_OUTPUT
    if (lcQpaMime().isDebugEnabled())
        qCDebug(lcQpaMime) << __FUNCTION__ << "pixmap" << pixmap.size() << m_cursors.size() << "cursors:\n" << m_cursors;
#endif // !QT_NO_DEBUG_OUTPUT
}

/*!
    \brief Check for cancel.
*/

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    Qt::MouseButtons buttons = toQtMouseButtons(grfKeyState);

    SCODE result = S_OK;
    if (fEscapePressed || QWindowsDrag::isCanceled()) {
        result = DRAGDROP_S_CANCEL;
        buttons = Qt::NoButton;
    } else {
        if (buttons && !m_currentButtons) {
            m_currentButtons = buttons;
        } else if (!(m_currentButtons & buttons)) { // Button changed: Complete Drop operation.
            result = DRAGDROP_S_DROP;
        }
    }

    switch (result) {
        case DRAGDROP_S_DROP:
        case DRAGDROP_S_CANCEL:
            if (!m_windowUnderMouse.isNull() && m_mode != TouchDrag && fEscapePressed == FALSE
                && buttons != QGuiApplicationPrivate::mouse_buttons) {
                // QTBUG 66447: Synthesize a mouse release to the window under mouse at
                // start of the DnD operation as Windows does not send any.
                const QPoint globalPos = QWindowsCursor::mousePosition();
                const QPoint localPos = m_windowUnderMouse->handle()->mapFromGlobal(globalPos);
                QWindowSystemInterface::handleMouseEvent(m_windowUnderMouse.data(),
                                                         QPointF(localPos), QPointF(globalPos),
                                                         QWindowsMouseHandler::queryMouseButtons(),
                                                         Qt::LeftButton, QEvent::MouseButtonRelease);
            }
            m_currentButtons = Qt::NoButton;
            break;

        default:
            QGuiApplication::processEvents();
            break;
    }

    if (QWindowsContext::verbose > 1 || result != S_OK) {
        qCDebug(lcQpaMime) << __FUNCTION__ << "fEscapePressed=" << fEscapePressed
            << "grfKeyState=" << grfKeyState << "buttons" << m_currentButtons
            << "returns 0x" << hex << int(result) << dec;
    }
    return ResultFromScode(result);
}

/*!
    \brief Give feedback: Change cursor accoding to action.
*/

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropSource::GiveFeedback(DWORD dwEffect)
{
    const Qt::DropAction action = translateToQDragDropAction(dwEffect);
    m_drag->updateAction(action);

    const qint64 currentCacheKey = m_drag->currentDrag()->dragCursor(action).cacheKey();
    auto it = m_cursors.constFind(action);
    // If a custom drag cursor is set, check its cache key to detect changes.
    if (it == m_cursors.constEnd() || (currentCacheKey && currentCacheKey != it.value().cacheKey)) {
        createCursors();
        it = m_cursors.constFind(action);
    }

    if (it != m_cursors.constEnd()) {
        const CursorEntry &e = it.value();
        switch (m_mode) {
        case MouseDrag:
            SetCursor(e.cursor->handle());
            break;
        case TouchDrag:
            // "Touch drag" with an unsuppressed cursor may happen with RDP (see createCursors())
            if (QWindowsCursor::cursorState() != QWindowsCursor::State::Suppressed)
                SetCursor(nullptr);
            if (!m_touchDragWindow)
                m_touchDragWindow = new QWindowsDragCursorWindow;
            m_touchDragWindow->setPixmap(e.pixmap);
            m_touchDragWindow->setFramePosition(QCursor::pos() - e.hotSpot);
            if (!m_touchDragWindow->isVisible())
                m_touchDragWindow->show();
            break;
        }
        return ResultFromScode(S_OK);
    }

    return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
}

/*!
    \class QWindowsOleDropTarget
    \brief Implementation of IDropTarget

    To be registered for each window. Currently, drop sites
    are enabled for top levels. The child window handling
    (sending DragEnter/Leave, etc) is handled in here.

    \sa QWindowsDrag
    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsOleDropTarget::QWindowsOleDropTarget(QWindow *w) : m_window(w)
{
    qCDebug(lcQpaMime) << __FUNCTION__ << this << w;
}

QWindowsOleDropTarget::~QWindowsOleDropTarget()
{
    qCDebug(lcQpaMime) << __FUNCTION__ <<  this;
}

void QWindowsOleDropTarget::handleDrag(QWindow *window, DWORD grfKeyState,
                                       const QPoint &point, LPDWORD pdwEffect)
{
    Q_ASSERT(window);
    m_lastPoint = point;
    m_lastKeyState = grfKeyState;

    QWindowsDrag *windowsDrag = QWindowsDrag::instance();
    const Qt::DropActions actions = translateToQDragDropActions(*pdwEffect);
    const Qt::KeyboardModifiers keyboardModifiers = toQtKeyboardModifiers(grfKeyState);
    const Qt::MouseButtons mouseButtons = toQtMouseButtons(grfKeyState);

    const QPlatformDragQtResponse response =
          QWindowSystemInterface::handleDrag(window, windowsDrag->dropData(),
                                             m_lastPoint, actions,
                                             mouseButtons, keyboardModifiers);

    m_answerRect = response.answerRect();
    const Qt::DropAction action = response.acceptedAction();
    if (response.isAccepted()) {
        m_chosenEffect = translateToWinDragEffects(action);
    } else {
        m_chosenEffect = DROPEFFECT_NONE;
    }
    *pdwEffect = m_chosenEffect;
    qCDebug(lcQpaMime) << __FUNCTION__ << m_window
        << windowsDrag->dropData() << " supported actions=" << actions
        << " mods=" << keyboardModifiers << " mouse=" << mouseButtons
        << " accepted: " << response.isAccepted() << action
        << m_answerRect << " effect" << *pdwEffect;
}

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::DragEnter(LPDATAOBJECT pDataObj, DWORD grfKeyState,
                                 POINTL pt, LPDWORD pdwEffect)
{
    if (IDropTargetHelper* dh = QWindowsDrag::instance()->dropHelper())
        dh->DragEnter(reinterpret_cast<HWND>(m_window->winId()), pDataObj, reinterpret_cast<POINT*>(&pt), *pdwEffect);

    qCDebug(lcQpaMime) << __FUNCTION__ << "widget=" << m_window << " key=" << grfKeyState
        << "pt=" << pt.x << pt.y;

    QWindowsDrag::instance()->setDropDataObject(pDataObj);
    pDataObj->AddRef();
    const QPoint point = QWindowsGeometryHint::mapFromGlobal(m_window, QPoint(pt.x,pt.y));
    handleDrag(m_window, grfKeyState, point, pdwEffect);
    return NOERROR;
}

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    if (IDropTargetHelper* dh = QWindowsDrag::instance()->dropHelper())
        dh->DragOver(reinterpret_cast<POINT*>(&pt), *pdwEffect);

    qCDebug(lcQpaMime) << __FUNCTION__ << "m_window" << m_window << "key=" << grfKeyState
        << "pt=" << pt.x << pt.y;
    const QPoint tmpPoint = QWindowsGeometryHint::mapFromGlobal(m_window, QPoint(pt.x,pt.y));
    // see if we should compress this event
    if ((tmpPoint == m_lastPoint || m_answerRect.contains(tmpPoint))
        && m_lastKeyState == grfKeyState) {
        *pdwEffect = m_chosenEffect;
        qCDebug(lcQpaMime) << __FUNCTION__ << "compressed event";
        return NOERROR;
    }

    handleDrag(m_window, grfKeyState, tmpPoint, pdwEffect);
    return NOERROR;
}

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::DragLeave()
{
    if (IDropTargetHelper* dh = QWindowsDrag::instance()->dropHelper())
        dh->DragLeave();

    qCDebug(lcQpaMime) << __FUNCTION__ << ' ' << m_window;

    QWindowSystemInterface::handleDrag(m_window, 0, QPoint(), Qt::IgnoreAction,
                                       Qt::NoButton, Qt::NoModifier);

    if (!QDragManager::self()->source())
        m_lastKeyState = 0;
    QWindowsDrag::instance()->releaseDropDataObject();

    return NOERROR;
}

#define KEY_STATE_BUTTON_MASK (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::Drop(LPDATAOBJECT pDataObj, DWORD grfKeyState,
                            POINTL pt, LPDWORD pdwEffect)
{
    if (IDropTargetHelper* dh = QWindowsDrag::instance()->dropHelper())
        dh->Drop(pDataObj, reinterpret_cast<POINT*>(&pt), *pdwEffect);

    qCDebug(lcQpaMime) << __FUNCTION__ << ' ' << m_window
        << "keys=" << grfKeyState << "pt=" << pt.x << ',' << pt.y;

    m_lastPoint = QWindowsGeometryHint::mapFromGlobal(m_window, QPoint(pt.x,pt.y));

    QWindowsDrag *windowsDrag = QWindowsDrag::instance();

    const QPlatformDropQtResponse response =
        QWindowSystemInterface::handleDrop(m_window, windowsDrag->dropData(),
                                           m_lastPoint,
                                           translateToQDragDropActions(*pdwEffect),
                                           toQtMouseButtons(grfKeyState),
                                           toQtKeyboardModifiers(grfKeyState));

    m_lastKeyState = grfKeyState;

    if (response.isAccepted()) {
        const Qt::DropAction action = response.acceptedAction();
        if (action == Qt::MoveAction || action == Qt::TargetMoveAction) {
            if (action == Qt::MoveAction)
                m_chosenEffect = DROPEFFECT_MOVE;
            else
                m_chosenEffect = DROPEFFECT_COPY;
            HGLOBAL hData = GlobalAlloc(0, sizeof(DWORD));
            if (hData) {
                DWORD *moveEffect = reinterpret_cast<DWORD *>(GlobalLock(hData));
                *moveEffect = DROPEFFECT_MOVE;
                GlobalUnlock(hData);
                STGMEDIUM medium;
                memset(&medium, 0, sizeof(STGMEDIUM));
                medium.tymed = TYMED_HGLOBAL;
                medium.hGlobal = hData;
                FORMATETC format;
                format.cfFormat = CLIPFORMAT(RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT));
                format.tymed = TYMED_HGLOBAL;
                format.ptd = 0;
                format.dwAspect = 1;
                format.lindex = -1;
                windowsDrag->dropDataObject()->SetData(&format, &medium, true);
            }
        } else {
            m_chosenEffect = translateToWinDragEffects(action);
        }
    } else {
        m_chosenEffect = DROPEFFECT_NONE;
    }
    *pdwEffect = m_chosenEffect;

    windowsDrag->releaseDropDataObject();
    return NOERROR;
}


/*!
    \class QWindowsDrag
    \brief Windows drag implementation.
    \internal
    \ingroup qt-lighthouse-win
*/

bool QWindowsDrag::m_canceled = false;

QWindowsDrag::QWindowsDrag() = default;

QWindowsDrag::~QWindowsDrag()
{
    if (m_cachedDropTargetHelper)
        m_cachedDropTargetHelper->Release();
}

/*!
    \brief Return data for a drop in process. If it stems from a current drag, use a shortcut.
*/

QMimeData *QWindowsDrag::dropData()
{
    if (const QDrag *drag = currentDrag())
        return drag->mimeData();
    return &m_dropData;
}

/*!
    \brief May be used to handle extended cursors functionality for drags from outside the app.
*/
IDropTargetHelper* QWindowsDrag::dropHelper() {
    if (!m_cachedDropTargetHelper) {
        CoCreateInstance(CLSID_DragDropHelper, 0, CLSCTX_INPROC_SERVER,
                         IID_IDropTargetHelper,
                         reinterpret_cast<void**>(&m_cachedDropTargetHelper));
    }
    return m_cachedDropTargetHelper;
}

Qt::DropAction QWindowsDrag::drag(QDrag *drag)
{
    // TODO: Accessibility handling?
    QMimeData *dropData = drag->mimeData();
    Qt::DropAction dragResult = Qt::IgnoreAction;

    DWORD resultEffect;
    QWindowsDrag::m_canceled = false;
    QWindowsOleDropSource *windowDropSource = new QWindowsOleDropSource(this);
    windowDropSource->createCursors();
    QWindowsDropDataObject *dropDataObject = new QWindowsDropDataObject(dropData);
    const Qt::DropActions possibleActions = drag->supportedActions();
    const DWORD allowedEffects = translateToWinDragEffects(possibleActions);
    qCDebug(lcQpaMime) << '>' << __FUNCTION__ << "possible Actions=0x"
        << hex << int(possibleActions) << "effects=0x" << allowedEffects << dec;
    const HRESULT r = DoDragDrop(dropDataObject, windowDropSource, allowedEffects, &resultEffect);
    const DWORD  reportedPerformedEffect = dropDataObject->reportedPerformedEffect();
    if (r == DRAGDROP_S_DROP) {
        if (reportedPerformedEffect == DROPEFFECT_MOVE && resultEffect != DROPEFFECT_MOVE) {
            dragResult = Qt::TargetMoveAction;
            resultEffect = DROPEFFECT_MOVE;
        } else {
            dragResult = translateToQDragDropAction(resultEffect);
        }
        // Force it to be a copy if an unsupported operation occurred.
        // This indicates a bug in the drop target.
        if (resultEffect != DROPEFFECT_NONE && !(resultEffect & allowedEffects)) {
            qWarning("%s: Forcing Qt::CopyAction", __FUNCTION__);
            dragResult = Qt::CopyAction;
        }
    }
    // clean up
    dropDataObject->releaseQt();
    dropDataObject->Release();        // Will delete obj if refcount becomes 0
    windowDropSource->Release();        // Will delete src if refcount becomes 0
    qCDebug(lcQpaMime) << '<' << __FUNCTION__ << hex << "allowedEffects=0x" << allowedEffects
        << "reportedPerformedEffect=0x" << reportedPerformedEffect
        <<  " resultEffect=0x" << resultEffect << "hr=0x" << int(r) << dec << "dropAction=" << dragResult;
    return dragResult;
}

QWindowsDrag *QWindowsDrag::instance()
{
    return static_cast<QWindowsDrag *>(QWindowsIntegration::instance()->drag());
}

void QWindowsDrag::releaseDropDataObject()
{
    qCDebug(lcQpaMime) << __FUNCTION__ << m_dropDataObject;
    if (m_dropDataObject) {
        m_dropDataObject->Release();
        m_dropDataObject = 0;
    }
}

QT_END_NAMESPACE
