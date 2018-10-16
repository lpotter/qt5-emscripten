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

#ifndef QQNXKEYTRANSLATOR_H
#define QQNXKEYTRANSLATOR_H

#include <sys/keycodes.h>

#if defined(QQNXEVENTTHREAD_DEBUG)
#include <QtCore/QDebug>
#endif

QT_BEGIN_NAMESPACE

int qtKeyForPrivateUseQnxKey( int key )
{
    switch (key) {
    case KEYCODE_PAUSE:       return Qt::Key_Pause;
    case KEYCODE_SCROLL_LOCK: return Qt::Key_ScrollLock;
    case KEYCODE_PRINT:       return Qt::Key_Print;
    case KEYCODE_SYSREQ:      return Qt::Key_SysReq;
//    case KEYCODE_BREAK:
    case KEYCODE_ESCAPE:      return Qt::Key_Escape;
    case KEYCODE_BACKSPACE:   return Qt::Key_Backspace;
    case KEYCODE_TAB:         return Qt::Key_Tab;
    case KEYCODE_BACK_TAB:    return Qt::Key_Backtab;
    case KEYCODE_RETURN:      return Qt::Key_Return;
    case KEYCODE_CAPS_LOCK:   return Qt::Key_CapsLock;
    case KEYCODE_LEFT_SHIFT:  return Qt::Key_Shift;
    case KEYCODE_RIGHT_SHIFT: return Qt::Key_Shift;
    case KEYCODE_LEFT_CTRL:   return Qt::Key_Control;
    case KEYCODE_RIGHT_CTRL:  return Qt::Key_Control;
    case KEYCODE_LEFT_ALT:    return Qt::Key_Alt;
    case KEYCODE_RIGHT_ALT:   return Qt::Key_Alt;
    case KEYCODE_MENU:        return Qt::Key_Menu;
    case KEYCODE_LEFT_HYPER:  return Qt::Key_Hyper_L;
    case KEYCODE_RIGHT_HYPER: return Qt::Key_Hyper_R;
    case KEYCODE_INSERT:      return Qt::Key_Insert;
    case KEYCODE_HOME:        return Qt::Key_Home;
    case KEYCODE_PG_UP:       return Qt::Key_PageUp;
    case KEYCODE_DELETE:      return Qt::Key_Delete;
    case KEYCODE_END:         return Qt::Key_End;
    case KEYCODE_PG_DOWN:     return Qt::Key_PageDown;
    case KEYCODE_LEFT:        return Qt::Key_Left;
    case KEYCODE_RIGHT:       return Qt::Key_Right;
    case KEYCODE_UP:          return Qt::Key_Up;
    case KEYCODE_DOWN:        return Qt::Key_Down;
    case KEYCODE_NUM_LOCK:    return Qt::Key_NumLock;
    case KEYCODE_KP_PLUS:     return Qt::Key_Plus;
    case KEYCODE_KP_MINUS:    return Qt::Key_Minus;
    case KEYCODE_KP_MULTIPLY: return Qt::Key_Asterisk;
    case KEYCODE_KP_DIVIDE:   return Qt::Key_Slash;
    case KEYCODE_KP_ENTER:    return Qt::Key_Enter;
    case KEYCODE_KP_HOME:     return Qt::Key_Home;
    case KEYCODE_KP_UP:       return Qt::Key_Up;
    case KEYCODE_KP_PG_UP:    return Qt::Key_PageUp;
    case KEYCODE_KP_LEFT:     return Qt::Key_Left;
    case KEYCODE_KP_FIVE:     return Qt::Key_5;
    case KEYCODE_KP_RIGHT:    return Qt::Key_Right;
    case KEYCODE_KP_END:      return Qt::Key_End;
    case KEYCODE_KP_DOWN:     return Qt::Key_Down;
    case KEYCODE_KP_PG_DOWN:  return Qt::Key_PageDown;
    case KEYCODE_KP_INSERT:   return Qt::Key_Insert;
    case KEYCODE_KP_DELETE:   return Qt::Key_Delete;
    case KEYCODE_F1:          return Qt::Key_F1;
    case KEYCODE_F2:          return Qt::Key_F2;
    case KEYCODE_F3:          return Qt::Key_F3;
    case KEYCODE_F4:          return Qt::Key_F4;
    case KEYCODE_F5:          return Qt::Key_F5;
    case KEYCODE_F6:          return Qt::Key_F6;
    case KEYCODE_F7:          return Qt::Key_F7;
    case KEYCODE_F8:          return Qt::Key_F8;
    case KEYCODE_F9:          return Qt::Key_F9;
    case KEYCODE_F10:         return Qt::Key_F10;
    case KEYCODE_F11:         return Qt::Key_F11;
    case KEYCODE_F12:         return Qt::Key_F12;

    // See keycodes.h for more, but these are all the basics. And printables are already included.

    default:
#if defined(QQNXEVENTTHREAD_DEBUG)
        qDebug() << "QQNX: unknown key for translation:" << key;
#endif
        break;
    }

    return Qt::Key_unknown;
}

QString keyStringForPrivateUseQnxKey( int key )
{
    switch (key) {
    case KEYCODE_ESCAPE:    return QStringLiteral("\x1B");
    case KEYCODE_BACKSPACE: return QStringLiteral("\b");
    case KEYCODE_TAB:       return QStringLiteral("\t");
    case KEYCODE_RETURN:    return QStringLiteral("\r");
    case KEYCODE_DELETE:    return QStringLiteral("\x7F");
    case KEYCODE_KP_ENTER:  return QStringLiteral("\r");
    }

    return QString();
}

bool isKeypadKey( int key )
{
    switch (key)
    {
    case KEYCODE_KP_PLUS:
    case KEYCODE_KP_MINUS:
    case KEYCODE_KP_MULTIPLY:
    case KEYCODE_KP_DIVIDE:
    case KEYCODE_KP_ENTER:
    case KEYCODE_KP_HOME:
    case KEYCODE_KP_UP:
    case KEYCODE_KP_PG_UP:
    case KEYCODE_KP_LEFT:
    case KEYCODE_KP_FIVE:
    case KEYCODE_KP_RIGHT:
    case KEYCODE_KP_END:
    case KEYCODE_KP_DOWN:
    case KEYCODE_KP_PG_DOWN:
    case KEYCODE_KP_INSERT:
    case KEYCODE_KP_DELETE:
        return true;
    default:
        break;
    }

    return false;
}

QT_END_NAMESPACE

#endif // QQNXKEYTRANSLATOR_H
