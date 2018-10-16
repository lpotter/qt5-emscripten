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

#include "qnswindowdelegate.h"
#include "qcocoahelpers.h"
#include "qcocoawindow.h"
#include "qcocoascreen.h"

#include <QDebug>
#include <QtCore/private/qcore_mac_p.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

static QRegExp whitespaceRegex = QRegExp(QStringLiteral("\\s*"));

@implementation QNSWindowDelegate {
    QCocoaWindow *m_cocoaWindow;
}

- (instancetype)initWithQCocoaWindow:(QCocoaWindow *)cocoaWindow
{
    if ((self = [self init]))
        m_cocoaWindow = cocoaWindow;
    return self;
}

- (BOOL)windowShouldClose:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (m_cocoaWindow) {
        return m_cocoaWindow->windowShouldClose();
    }

    return YES;
}
/*!
    Overridden to ensure that the zoomed state always results in a maximized
    window, which would otherwise not be the case for borderless windows.

    We also keep the window on the same screen as before; something AppKit
    sometimes fails to do using its built in logic.
*/
- (NSRect)windowWillUseStandardFrame:(NSWindow *)window defaultFrame:(NSRect)proposedFrame
{
    Q_UNUSED(proposedFrame);
    Q_ASSERT(window == m_cocoaWindow->nativeWindow());
    const QWindow *w = m_cocoaWindow->window();

    // maximumSize() refers to the client size, but AppKit expects the full frame size
    QSizeF maximumSize = w->maximumSize() + QSize(0, w->frameMargins().top());

    // The window should never be larger than the current screen geometry
    const QRectF screenGeometry = m_cocoaWindow->screen()->geometry();
    maximumSize = maximumSize.boundedTo(screenGeometry.size());

    // Use the current frame position for the initial maximized frame,
    // so that the window stays put and just expand, in case its maximum
    // size is within the screen bounds.
    QRectF maximizedFrame = QRectF(w->framePosition(), maximumSize);

    // But constrain the frame to the screen bounds in case the frame
    // extends beyond the screen bounds as a result of starting out
    // with the current frame position.
    maximizedFrame.translate(QPoint(
        qMax(screenGeometry.left() - maximizedFrame.left(), 0.0) +
        qMin(screenGeometry.right() - maximizedFrame.right(), 0.0),
        qMax(screenGeometry.top() - maximizedFrame.top(), 0.0) +
        qMin(screenGeometry.bottom() - maximizedFrame.bottom(), 0.0)));

    return QCocoaScreen::mapToNative(maximizedFrame);
}

#pragma clang diagnostic push
// NSDisableScreenUpdates and NSEnableScreenUpdates are deprecated, but the
// NSAnimationContext API that replaces them doesn't handle the use-case of
// cross-thread screen update synchronization.
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
- (NSSize)windowWillResize:(NSWindow *)window toSize:(NSSize)frameSize
{
    qCDebug(lcQpaWindow) << window << "will resize to" << QSizeF::fromCGSize(frameSize)
        << "- disabling screen updates temporarily";

    // There may be separate threads rendering to CA layers in this window,
    // and if any of them do a swap while the resize is still in progress,
    // the visual bounds of that layer will be updated before the visual
    // bounds of the window frame, resulting in flickering while resizing.

    // To prevent this we disable screen updates for the whole process until
    // the resize is complete, which makes the whole thing visually atomic.
    NSDisableScreenUpdates();

    return frameSize;
}

- (void)windowDidResize:(NSNotification *)notification
{
    NSWindow *window = notification.object;
    qCDebug(lcQpaWindow) << window << "was resized - re-enabling screen updates";
    NSEnableScreenUpdates();
}
#pragma clang diagnostic pop

- (BOOL)window:(NSWindow *)window shouldPopUpDocumentPathMenu:(NSMenu *)menu
{
    Q_UNUSED(window);
    Q_UNUSED(menu);

    // Only pop up document path if the filename is non-empty. We allow whitespace, to
    // allow faking a window icon by setting the file path to a single space character.
    return !whitespaceRegex.exactMatch(m_cocoaWindow->window()->filePath());
}

- (BOOL)window:(NSWindow *)window shouldDragDocumentWithEvent:(NSEvent *)event from:(NSPoint)dragImageLocation withPasteboard:(NSPasteboard *)pasteboard
{
    Q_UNUSED(window);
    Q_UNUSED(event);
    Q_UNUSED(dragImageLocation);
    Q_UNUSED(pasteboard);

    // Only allow drag if the filename is non-empty. We allow whitespace, to
    // allow faking a window icon by setting the file path to a single space.
    return !whitespaceRegex.exactMatch(m_cocoaWindow->window()->filePath());
}
@end
