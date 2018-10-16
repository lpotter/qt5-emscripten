/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#ifndef ANDROIDJNIINPUT_H
#define ANDROIDJNIINPUT_H

#include <jni.h>
#include <QtCore/qglobal.h>
#include <QtCore/QRect>

QT_BEGIN_NAMESPACE

namespace QtAndroidInput
{
    // Software keyboard support
    void showSoftwareKeyboard(int top, int left, int width, int height, int inputHints, int enterKeyType);
    void resetSoftwareKeyboard();
    void hideSoftwareKeyboard();
    bool isSoftwareKeyboardVisible();
    QRect softwareKeyboardRect();
    void updateSelection(int selStart, int selEnd, int candidatesStart, int candidatesEnd);
    // Software keyboard support

    // cursor/selection handles
    void updateHandles(int handleCount, QPoint editMenuPos = QPoint(), uint32_t editButtons = 0, QPoint cursor = QPoint(), QPoint anchor = QPoint(), bool rtl = false);

    bool registerNatives(JNIEnv *env);

    void releaseMouse(int x, int y);
}

QT_END_NAMESPACE

#endif // ANDROIDJNIINPUT_H
