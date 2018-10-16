/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#ifndef QBBSCREEN_H
#define QBBSCREEN_H

#include <qpa/qplatformscreen.h>

#include "qqnxwindow.h"

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include <screen/screen.h>

#if !defined(_SCREEN_VERSION)
#define _SCREEN_MAKE_VERSION(major, minor, patch)  (((major) * 10000) + ((minor) * 100) + (patch))
#define _SCREEN_VERSION _SCREEN_MAKE_VERSION(0, 0, 0)
#endif

// For pre-1.0.0 screen, map some screen property names to the old
// names.
#if _SCREEN_VERSION < _SCREEN_MAKE_VERSION(1, 0, 0)
const int SCREEN_PROPERTY_FLAGS = SCREEN_PROPERTY_KEY_FLAGS;
const int SCREEN_PROPERTY_FOCUS = SCREEN_PROPERTY_KEYBOARD_FOCUS;
const int SCREEN_PROPERTY_MODIFIERS = SCREEN_PROPERTY_KEY_MODIFIERS;
const int SCREEN_PROPERTY_SCAN = SCREEN_PROPERTY_KEY_SCAN;
const int SCREEN_PROPERTY_SYM = SCREEN_PROPERTY_KEY_SYM;
#endif

QT_BEGIN_NAMESPACE

class QQnxWindow;

class QQnxScreen : public QObject, public QPlatformScreen
{
    Q_OBJECT
public:
    QQnxScreen(screen_context_t context, screen_display_t display, bool primaryScreen);
    ~QQnxScreen();

    QPixmap grabWindow(WId window, int x, int y, int width, int height) const override;

    QRect geometry() const override { return m_currentGeometry; }
    QRect availableGeometry() const override;
    int depth() const override;
    QImage::Format format() const override { return (depth() == 32) ? QImage::Format_RGB32 : QImage::Format_RGB16; }
    QSizeF physicalSize() const override { return m_currentPhysicalSize; }

    qreal refreshRate() const override;

    Qt::ScreenOrientation nativeOrientation() const override;
    Qt::ScreenOrientation orientation() const override;

    QWindow *topLevelAt(const QPoint &point) const override;

    bool isPrimaryScreen() const { return m_primaryScreen; }

    int rotation() const { return m_currentRotation; }

    QString name() const override { return m_name; }

    int nativeFormat() const { return (depth() == 32) ? SCREEN_FORMAT_RGBA8888 : SCREEN_FORMAT_RGB565; }
    screen_display_t nativeDisplay() const { return m_display; }
    screen_context_t nativeContext() const { return m_screenContext; }
    const char *windowGroupName() const { return m_rootWindow ? m_rootWindow->groupName().constData() : 0; }

    QQnxWindow *findWindow(screen_window_t windowHandle) const;

    /* Window hierarchy management */
    void addWindow(QQnxWindow *child);
    void removeWindow(QQnxWindow *child);
    void raiseWindow(QQnxWindow *window);
    void lowerWindow(QQnxWindow *window);
    void updateHierarchy();

    void adjustOrientation();

    QQnxWindow *rootWindow() const;
    void setRootWindow(QQnxWindow*);

    QPlatformCursor *cursor() const override;

Q_SIGNALS:
    void foreignWindowCreated(void *window);
    void foreignWindowClosed(void *window);

public Q_SLOTS:
    void setRotation(int rotation);
    void newWindowCreated(void *window);
    void windowClosed(void *window);
    void windowGroupStateChanged(const QByteArray &id, Qt::WindowState state);
    void activateWindowGroup(const QByteArray &id);
    void deactivateWindowGroup(const QByteArray &id);

private Q_SLOTS:
    void keyboardHeightChanged(int height);

private:
    void resizeNativeWidgetWindow(QQnxWindow *w, const QRect &previousScreenGeometry) const;
    void resizeTopLevelWindow(QQnxWindow *w, const QRect &previousScreenGeometry) const;
    void resizeWindows(const QRect &previousScreenGeometry);
    void addOverlayWindow(screen_window_t window);
    void addUnderlayWindow(screen_window_t window);
    void addMultimediaWindow(const QByteArray &id, screen_window_t window);
    void removeOverlayOrUnderlayWindow(screen_window_t window);

    screen_context_t m_screenContext;
    screen_display_t m_display;
    QQnxWindow *m_rootWindow;
    const bool m_primaryScreen;

    int m_initialRotation;
    int m_currentRotation;
    int m_keyboardHeight;
    QString m_name;
    QSize m_initialPhysicalSize;
    QSize m_currentPhysicalSize;
    Qt::ScreenOrientation m_nativeOrientation;
    QRect m_initialGeometry;
    QRect m_currentGeometry;

    QList<QQnxWindow *> m_childWindows;
    QQnxWindow *m_coverWindow;
    QList<screen_window_t> m_overlays;
    QList<screen_window_t> m_underlays;

    QPlatformCursor *m_cursor;
};

QT_END_NAMESPACE

#endif // QBBSCREEN_H
