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

#ifndef QDIRECTFBCONVENIENCE_H
#define QDIRECTFBCONVENIENCE_H

#include <QtGui/qimage.h>
#include <QtCore/QHash>
#include <QtCore/QEvent>
#include <QtGui/QPixmap>

#include <directfb.h>

QT_BEGIN_NAMESPACE

class QDirectFbScreen;
class QPlatformScreen;

class QDirectFbKeyMap: public QHash<DFBInputDeviceKeySymbol, Qt::Key>
{
public:
    QDirectFbKeyMap();
};


class QDirectFbConvenience
{
public:
    static QImage::Format imageFormatFromSurfaceFormat(const DFBSurfacePixelFormat format, const DFBSurfaceCapabilities caps);
    static bool pixelFomatHasAlpha(const DFBSurfacePixelFormat format) { return (1 << 16) & format; }
    static int colorDepthForSurface(const DFBSurfacePixelFormat format);

    //This is set by the graphicssystem constructor
    static IDirectFB *dfbInterface();
    static IDirectFBDisplayLayer *dfbDisplayLayer(int display = DLID_PRIMARY);

    static IDirectFBSurface *dfbSurfaceForPlatformPixmap(QPlatformPixmap *);

    static Qt::MouseButton mouseButton(DFBInputDeviceButtonIdentifier identifier);
    static Qt::MouseButtons mouseButtons(DFBInputDeviceButtonMask mask);
    static Qt::KeyboardModifiers keyboardModifiers(DFBInputDeviceModifierMask mask);
    static QEvent::Type eventType(DFBWindowEventType type);

    static QDirectFbKeyMap *keyMap();

private:
    static QDirectFbKeyMap *dfbKeymap;
    friend class QDirectFbIntegration;
};

template <typename T> struct QDirectFBInterfaceCleanupHandler
{
    static void cleanup(T *t)
    {
        if (!t)
            return;
        t->Release(t);
    }
};

template <typename T>
class QDirectFBPointer : public QScopedPointer<T, QDirectFBInterfaceCleanupHandler<T> >
{
public:
    QDirectFBPointer(T *t = 0)
        : QScopedPointer<T, QDirectFBInterfaceCleanupHandler<T> >(t)
    {}

    T** outPtr()
    {
        this->reset(0);
        return &this->d;
    }
};

// Helper conversions from internal to DFB types
QDirectFbScreen *toDfbScreen(QWindow *window);
IDirectFBDisplayLayer *toDfbLayer(QPlatformScreen *screen);

#define QDFB_STRINGIFY(x) #x
#define QDFB_TOSTRING(x) QDFB_STRINGIFY(x)
#define QDFB_PRETTY \
    (__FILE__ ":" QDFB_TOSTRING(__LINE__))

QT_END_NAMESPACE


#endif // QDIRECTFBCONVENIENCE_H
