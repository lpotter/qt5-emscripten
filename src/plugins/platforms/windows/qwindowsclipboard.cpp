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

#include "qwindowsclipboard.h"
#include "qwindowscontext.h"
#include "qwindowsole.h"
#include "qwindowsmime.h"

#include <QtGui/qguiapplication.h>
#include <QtGui/qclipboard.h>
#include <QtGui/qcolor.h>
#include <QtGui/qimage.h>

#include <QtCore/qdebug.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qthread.h>
#include <QtCore/qvariant.h>
#include <QtCore/qurl.h>

#include <QtEventDispatcherSupport/private/qwindowsguieventdispatcher_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsClipboard
    \brief Clipboard implementation.

    Registers a non-visible clipboard viewer window that
    receives clipboard events in its own window procedure to be
    able to receive clipboard-changed events, which
    QPlatformClipboard needs to emit. That requires housekeeping
    of the next in the viewer chain.

    \note The OLE-functions used in this class require OleInitialize().

    \internal
    \ingroup qt-lighthouse-win
*/

#ifndef QT_NO_DEBUG_STREAM
static QDebug operator<<(QDebug d, const QMimeData *mimeData)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "QMimeData(";
    if (mimeData) {
        const QStringList formats = mimeData->formats();
        d << "formats=" << formats.join(QLatin1String(", "));
        if (mimeData->hasText())
            d << ", text=" << mimeData->text();
        if (mimeData->hasHtml())
            d << ", html=" << mimeData->html();
        if (mimeData->hasColor())
            d << ", colorData=" << qvariant_cast<QColor>(mimeData->colorData());
        if (mimeData->hasImage())
            d << ", imageData=" << qvariant_cast<QImage>(mimeData->imageData());
        if (mimeData->hasUrls())
             d << ", urls=" << mimeData->urls();
    } else {
        d << '0';
    }
    d << ')';
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

/*!
    \class QWindowsClipboardRetrievalMimeData
    \brief Special mime data class managing delayed retrieval of clipboard data.

    Implementation of QWindowsInternalMimeDataBase that obtains the
    IDataObject from the clipboard.

    \sa QWindowsInternalMimeDataBase, QWindowsClipboard
    \internal
    \ingroup qt-lighthouse-win
*/

IDataObject *QWindowsClipboardRetrievalMimeData::retrieveDataObject() const
{
    IDataObject * pDataObj = 0;
    if (OleGetClipboard(&pDataObj) == S_OK) {
        if (QWindowsContext::verbose > 1)
            qCDebug(lcQpaMime) << __FUNCTION__ << pDataObj;
        return pDataObj;
    }
    return 0;
}

void QWindowsClipboardRetrievalMimeData::releaseDataObject(IDataObject *dataObject) const
{
    dataObject->Release();
}

extern "C" LRESULT QT_WIN_CALLBACK qClipboardViewerWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    if (QWindowsClipboard::instance()
        && QWindowsClipboard::instance()->clipboardViewerWndProc(hwnd, message, wParam, lParam, &result))
        return result;
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// QTBUG-36958, ensure the clipboard is flushed before
// QGuiApplication is destroyed since OleFlushClipboard()
// might query the data again which causes problems
// for QMimeData-derived classes using QPixmap/QImage.
static void cleanClipboardPostRoutine()
{
    if (QWindowsClipboard *cl = QWindowsClipboard::instance())
        cl->cleanup();
}

QWindowsClipboard *QWindowsClipboard::m_instance = 0;

QWindowsClipboard::QWindowsClipboard()
{
    QWindowsClipboard::m_instance = this;
    qAddPostRoutine(cleanClipboardPostRoutine);
}

QWindowsClipboard::~QWindowsClipboard()
{
    cleanup();
    QWindowsClipboard::m_instance = 0;
}

void QWindowsClipboard::cleanup()
{
    unregisterViewer(); // Should release data if owner.
    releaseIData();
}

void QWindowsClipboard::releaseIData()
{
    if (m_data) {
        delete m_data->mimeData();
        m_data->releaseQt();
        m_data->Release();
        m_data = 0;
    }
}

void QWindowsClipboard::registerViewer()
{
    m_clipboardViewer = QWindowsContext::instance()->
        createDummyWindow(QStringLiteral("Qt5ClipboardView"), L"Qt5ClipboardView",
                          qClipboardViewerWndProc, WS_OVERLAPPED);

    // Try format listener API (Vista onwards) first.
    if (QWindowsContext::user32dll.addClipboardFormatListener && QWindowsContext::user32dll.removeClipboardFormatListener) {
        m_formatListenerRegistered = QWindowsContext::user32dll.addClipboardFormatListener(m_clipboardViewer);
        if (!m_formatListenerRegistered)
            qErrnoWarning("AddClipboardFormatListener() failed.");
    }

    if (!m_formatListenerRegistered)
        m_nextClipboardViewer = SetClipboardViewer(m_clipboardViewer);

    qCDebug(lcQpaMime) << __FUNCTION__ << "m_clipboardViewer:" << m_clipboardViewer
        << "format listener:" << m_formatListenerRegistered
        << "next:" << m_nextClipboardViewer;
}

void QWindowsClipboard::unregisterViewer()
{
    if (m_clipboardViewer) {
        if (m_formatListenerRegistered) {
            QWindowsContext::user32dll.removeClipboardFormatListener(m_clipboardViewer);
            m_formatListenerRegistered = false;
        } else {
            ChangeClipboardChain(m_clipboardViewer, m_nextClipboardViewer);
            m_nextClipboardViewer = 0;
        }
        DestroyWindow(m_clipboardViewer);
        m_clipboardViewer = 0;
    }
}

// ### FIXME: Qt 6: Remove the clipboard chain handling code and make the
// format listener the default.

static bool isProcessBeingDebugged(HWND hwnd)
{
    DWORD pid = 0;
    if (!GetWindowThreadProcessId(hwnd, &pid) || !pid)
        return false;
    const HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!processHandle)
        return false;
    BOOL debugged = FALSE;
    CheckRemoteDebuggerPresent(processHandle, &debugged);
    CloseHandle(processHandle);
    return debugged != FALSE;
}

void QWindowsClipboard::propagateClipboardMessage(UINT message, WPARAM wParam, LPARAM lParam) const
{
    if (!m_nextClipboardViewer)
        return;
    // In rare cases, a clipboard viewer can hang (application crashed,
    // suspended by a shell prompt 'Select' or debugger).
    if (IsHungAppWindow(m_nextClipboardViewer)) {
        qWarning("Cowardly refusing to send clipboard message to hung application...");
        return;
    }
    // Do not block if the process is being debugged, specifically, if it is
    // displaying a runtime assert, which is not caught by isHungAppWindow().
    if (isProcessBeingDebugged(m_nextClipboardViewer))
        PostMessage(m_nextClipboardViewer, message, wParam, lParam);
    else
        SendMessage(m_nextClipboardViewer, message, wParam, lParam);
}

/*!
    \brief Windows procedure of the clipboard viewer. Emits changed and does
    housekeeping of the viewer chain.
*/

bool QWindowsClipboard::clipboardViewerWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result)
{
    enum { wMClipboardUpdate = 0x031D };

    *result = 0;
    if (QWindowsContext::verbose)
        qCDebug(lcQpaMime) << __FUNCTION__ << hwnd << message << QWindowsGuiEventDispatcher::windowsMessageName(message);

    switch (message) {
    case WM_CHANGECBCHAIN: {
        const HWND toBeRemoved = reinterpret_cast<HWND>(wParam);
        if (toBeRemoved == m_nextClipboardViewer) {
            m_nextClipboardViewer = reinterpret_cast<HWND>(lParam);
        } else {
            propagateClipboardMessage(message, wParam, lParam);
        }
    }
        return true;
    case wMClipboardUpdate:  // Clipboard Format listener (Vista onwards)
    case WM_DRAWCLIPBOARD: { // Clipboard Viewer Chain handling (up to XP)
        const bool owned = ownsClipboard();
        qCDebug(lcQpaMime) << "Clipboard changed owned " << owned;
        emitChanged(QClipboard::Clipboard);
        // clean up the clipboard object if we no longer own the clipboard
        if (!owned && m_data)
            releaseIData();
        if (!m_formatListenerRegistered)
            propagateClipboardMessage(message, wParam, lParam);
    }
        return true;
    case WM_DESTROY:
        // Recommended shutdown
        if (ownsClipboard()) {
            qCDebug(lcQpaMime) << "Clipboard owner on shutdown, releasing.";
            OleFlushClipboard();
            releaseIData();
        }
        return true;
    } // switch (message)
    return false;
}

QMimeData *QWindowsClipboard::mimeData(QClipboard::Mode mode)
{
    qCDebug(lcQpaMime) << __FUNCTION__ <<  mode;
    if (mode != QClipboard::Clipboard)
        return 0;
    if (ownsClipboard())
        return m_data->mimeData();
    return &m_retrievalData;
}

void QWindowsClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    qCDebug(lcQpaMime) << __FUNCTION__ <<  mode << mimeData;
    if (mode != QClipboard::Clipboard)
        return;

    const bool newData = !m_data || m_data->mimeData() != mimeData;
    if (newData) {
        releaseIData();
        if (mimeData)
            m_data = new QWindowsOleDataObject(mimeData);
    }

    HRESULT src = S_FALSE;
    int attempts = 0;
    for (; attempts < 3; ++attempts) {
        src = OleSetClipboard(m_data);
        if (src != CLIPBRD_E_CANT_OPEN || QWindowsContext::isSessionLocked())
            break;
        QThread::msleep(100);
    }

    if (src != S_OK) {
        QString mimeDataFormats = mimeData ?
            mimeData->formats().join(QLatin1String(", ")) : QString(QStringLiteral("NULL"));
        qErrnoWarning("OleSetClipboard: Failed to set mime data (%s) on clipboard: %s",
                      qPrintable(mimeDataFormats),
                      QWindowsContext::comErrorString(src).constData());
        releaseIData();
        return;
    }
}

void QWindowsClipboard::clear()
{
    const HRESULT src = OleSetClipboard(0);
    if (src != S_OK)
        qErrnoWarning("OleSetClipboard: Failed to clear the clipboard: 0x%lx", src);
}

bool QWindowsClipboard::supportsMode(QClipboard::Mode mode) const
{
        return mode == QClipboard::Clipboard;
}

// Need a non-virtual in destructor.
bool QWindowsClipboard::ownsClipboard() const
{
    return m_data && OleIsCurrentClipboard(m_data) == S_OK;
}

bool QWindowsClipboard::ownsMode(QClipboard::Mode mode) const
{
    const bool result = mode == QClipboard::Clipboard ?
        ownsClipboard() : false;
    qCDebug(lcQpaMime) << __FUNCTION__ <<  mode << result;
    return result;
}

QT_END_NAMESPACE
