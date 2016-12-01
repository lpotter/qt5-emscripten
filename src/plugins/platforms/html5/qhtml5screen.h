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

#ifndef QHTML5SCREEN_H
#define QHTML5SCREEN_H

#include <qpa/qplatformscreen.h>

#include <QtCore/QTextStream>

#include <QtEglSupport/private/qt_egl_p.h>

QT_BEGIN_NAMESPACE

class QPlatformOpenGLContext;
class QHTML5Window;
class QHTML5BackingStore;

class QHTML5Screen : public QObject, public QPlatformScreen
{
    Q_OBJECT
public:

    QHTML5Screen(EGLNativeDisplayType display);
    ~QHTML5Screen();

    QRect geometry() const Q_DECL_OVERRIDE;
    int depth() const Q_DECL_OVERRIDE;
    QImage::Format format() const Q_DECL_OVERRIDE;
    QPlatformOpenGLContext *platformContext() const;
    EGLSurface surface() const { return m_surface; }

    void resizeMaximizedWindows();
    QWindow *topWindow() const;
    QWindow *topLevelAt(const QPoint & p) const Q_DECL_OVERRIDE;

    virtual void addWindow(QHTML5Window *window);
    virtual void removeWindow(QHTML5Window *window);
    virtual void raise(QHTML5Window *window);
    virtual void lower(QHTML5Window *window);
    virtual void topWindowChanged(QWindow *) {}
    virtual int windowCount() const;

    void addPendingBackingStore(QHTML5BackingStore *bs) { mPendingBackingStores << bs; }

    void scheduleUpdate();

public slots:
    virtual void setDirty(const QRect &rect);

protected:
    QList<QHTML5Window *> mWindowStack;
    QRegion mRepaintRegion;

private:
    void createAndSetPlatformContext() const;
    void createAndSetPlatformContext();
    bool mUpdatePending;

    QRect m_geometry;
    int m_depth;
    QImage::Format m_format;
    QPlatformOpenGLContext *m_platformContext;
    EGLDisplay m_dpy;
    EGLSurface m_surface;
    QList<QHTML5BackingStore*> mPendingBackingStores;

};
//Q_DECLARE_OPERATORS_FOR_FLAGS(QHTML5Screen::Flags)

QT_END_NAMESPACE
#endif // QHTML5SCREEN_H
