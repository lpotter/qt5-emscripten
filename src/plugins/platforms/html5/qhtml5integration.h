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

#ifndef QHTML5INTEGRATION_H
#define QHTML5INTEGRATION_H

#include "qhtml5window.h"

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformscreen.h>


#include <emscripten.h>
#include <emscripten/html5.h>

QT_BEGIN_NAMESPACE

class QHTML5EventTranslator;
class QHtml5FontDatabase;
class QHTML5Window;
class QHtml5EventDispatcher;
class QHTML5Screen;

class QHTML5Integration : public QObject, public QPlatformIntegration
{
    Q_OBJECT
public:
    QHTML5Integration();
    ~QHTML5Integration();

    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;
    QPlatformWindow *createPlatformWindow(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const Q_DECL_OVERRIDE;
#ifndef QT_NO_OPENGL
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const Q_DECL_OVERRIDE;
#endif
    QPlatformFontDatabase *fontDatabase() const Q_DECL_OVERRIDE;
    QAbstractEventDispatcher *createEventDispatcher() const Q_DECL_OVERRIDE;
    QVariant styleHint(QPlatformIntegration::StyleHint hint) const Q_DECL_OVERRIDE;

    static QHTML5Integration *get();
    QHTML5Window *topLevelWindow();
    QHTML5Screen *screen() { return mScreen; }
private:
    mutable QHtml5FontDatabase *mFontDb;
    mutable QHTML5Screen *mScreen;
    mutable QHTML5EventTranslator *m_eventTranslator;
    mutable QHtml5EventDispatcher *m_eventDispatcher;
    static int uiEvent_cb(int eventType, const EmscriptenUiEvent *e, void *userData);

    mutable QHTML5Window *m_topLevelWindow;
};

QT_END_NAMESPACE

#endif // QHTML5INTEGRATION_H
