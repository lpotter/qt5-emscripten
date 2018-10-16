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

#include "qplatformtheme.h"

#include "qplatformtheme_p.h"

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/qfileinfo.h>
#include <qicon.h>
#include <qpalette.h>
#include <qtextformat.h>
#include <private/qiconloader_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformdialoghelper.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformTheme
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa
    \brief The QPlatformTheme class allows customizing the UI based on themes.

*/

/*!
    \enum QPlatformTheme::ThemeHint

    This enum describes the available theme hints.

    \value CursorFlashTime (int) Cursor flash time in ms, overriding
                                 QPlatformIntegration::styleHint.

    \value KeyboardInputInterval (int) Keyboard input interval in ms, overriding
                                 QPlatformIntegration::styleHint.

    \value MouseDoubleClickInterval (int) Mouse double click interval in ms,
                                    overriding QPlatformIntegration::styleHint.

    \value MouseDoubleClickDistance (int) The maximum distance in logical pixels which the mouse can travel
                        between clicks in order for the click sequence to be handled as a double click.
                        The default value is 5 logical pixels.

    \value MousePressAndHoldInterval (int) Mouse press and hold interval in ms,
                                    overriding QPlatformIntegration::styleHint.

    \value StartDragDistance (int) Start drag distance,
                             overriding QPlatformIntegration::styleHint.

    \value StartDragTime (int) Start drag time in ms,
                               overriding QPlatformIntegration::styleHint.

    \value WheelScrollLines (int) The number of lines to scroll a widget, when the mouse wheel is rotated.
                        The default value is 3.  \sa QApplication::wheelScrollLines()

    \value KeyboardAutoRepeatRate (int) Keyboard auto repeat rate,
                                  overriding QPlatformIntegration::styleHint.

    \value PasswordMaskDelay (int) Pass word mask delay in ms,
                                   overriding QPlatformIntegration::styleHint.

    \value StartDragVelocity (int) Velocity of a drag,
                                   overriding QPlatformIntegration::styleHint.

    \value TextCursorWidth  (int) Determines the width of the text cursor.

    \value DropShadow       (bool) Determines whether the drop shadow effect for
                            tooltips or whatsthis is enabled.

    \value MaximumScrollBarDragDistance (int) Determines the value returned by
                            QStyle::pixelMetric(PM_MaximumDragDistance)

    \value ToolButtonStyle (int) A value representing a Qt::ToolButtonStyle.

    \value ToolBarIconSize Icon size for tool bars.

    \value SystemIconThemeName (QString) Name of the icon theme.

    \value SystemIconFallbackThemeName (QString) Name of the fallback icon theme.

    \value IconThemeSearchPaths (QStringList) Search paths for icons.

    \value ItemViewActivateItemOnSingleClick (bool) Activate items by single click.

    \value StyleNames (QStringList) A list of preferred style names.

    \value WindowAutoPlacement (bool) A boolean value indicating whether Windows
                               (particularly dialogs) are placed by the system
                               (see _NET_WM_FULL_PLACEMENT in X11).

    \value DialogButtonBoxLayout (int) An integer representing a
                                 QDialogButtonBox::ButtonLayout value.

    \value DialogButtonBoxButtonsHaveIcons (bool) A boolean value indicating whether
                                            the buttons of a QDialogButtonBox should have icons.

    \value UseFullScreenForPopupMenu (bool) Pop menus can cover the full screen including task bar.

    \value KeyboardScheme (int) An integer value (enum KeyboardSchemes) specifying the
                           keyboard scheme.

    \value UiEffects (int) A flag value consisting of UiEffect values specifying the enabled UI animations.

    \value SpellCheckUnderlineStyle (int) A QTextCharFormat::UnderlineStyle specifying
                                    the underline style used misspelled words when spell checking.

    \value TabFocusBehavior (int) A Qt::TabFocusBehavior specifying
                         the behavior of focus change when tab key was pressed.
                         This enum value was added in Qt 5.5.

    \value DialogSnapToDefaultButton (bool) Whether the mouse should snap to the default button when a dialog
                                     becomes visible.

    \value ContextMenuOnMouseRelease (bool) Whether the context menu should be shown on mouse release.

    \value TouchDoubleTapDistance (int) The maximum distance in logical pixels which a touchpoint can travel
                        between taps in order for the tap sequence to be handled as a double tap.
                        The default value is double the MouseDoubleClickDistance, or 10 logical pixels
                        if that is not specified.

    \value ShowShortcutsInContextMenus (bool) Whether to display shortcut key sequences in context menus.

    \sa themeHint(), QStyle::pixelMetric()
*/


#ifndef QT_NO_SHORTCUT
// Table of key bindings. It must be sorted on key sequence:
// The integer value of VK_KEY | Modifier Keys (e.g., VK_META, and etc.)
// A priority of 1 indicates that this is the primary key binding when multiple are defined.

enum KeyPlatform {
    KB_Win = (1 << QPlatformTheme::WindowsKeyboardScheme),
    KB_Mac = (1 << QPlatformTheme::MacKeyboardScheme),
    KB_X11 = (1 << QPlatformTheme::X11KeyboardScheme),
    KB_KDE = (1 << QPlatformTheme::KdeKeyboardScheme),
    KB_Gnome = (1 << QPlatformTheme::GnomeKeyboardScheme),
    KB_CDE = (1 << QPlatformTheme::CdeKeyboardScheme),
    KB_All = 0xffff
};

const QKeyBinding QPlatformThemePrivate::keyBindings[] = {
    //   StandardKey                            Priority    Key Sequence                            Platforms
    {QKeySequence::HelpContents,            1,          Qt::CTRL | Qt::Key_Question,            KB_Mac},
    {QKeySequence::HelpContents,            0,          Qt::Key_F1,                             KB_Win | KB_X11},
    {QKeySequence::WhatsThis,               1,          Qt::SHIFT | Qt::Key_F1,                 KB_All},
    {QKeySequence::Open,                    1,          Qt::CTRL | Qt::Key_O,                   KB_All},
    {QKeySequence::Close,                   0,          Qt::CTRL | Qt::Key_F4,                  KB_Mac},
    {QKeySequence::Close,                   1,          Qt::CTRL | Qt::Key_F4,                  KB_Win},
    {QKeySequence::Close,                   1,          Qt::CTRL | Qt::Key_W,                   KB_Mac},
    {QKeySequence::Close,                   0,          Qt::CTRL | Qt::Key_W,                   KB_Win | KB_X11},
    {QKeySequence::Save,                    1,          Qt::CTRL | Qt::Key_S,                   KB_All},
    {QKeySequence::New,                     1,          Qt::CTRL | Qt::Key_N,                   KB_All},
    {QKeySequence::Delete,                  0,          Qt::CTRL | Qt::Key_D,                   KB_X11}, //emacs (line edit only)
    {QKeySequence::Delete,                  1,          Qt::Key_Delete,                         KB_All},
    {QKeySequence::Delete,                  0,          Qt::META | Qt::Key_D,                   KB_Mac},
    {QKeySequence::Cut,                     1,          Qt::CTRL | Qt::Key_X,                   KB_All},
    {QKeySequence::Cut,                     0,          Qt::SHIFT | Qt::Key_Delete,             KB_Win | KB_X11}, //## Check if this should work on mac
    {QKeySequence::Cut,                     0,          Qt::Key_F20,                            KB_X11}, //Cut on sun keyboards
    {QKeySequence::Cut,                     0,          Qt::META | Qt::Key_K,                   KB_Mac},
    {QKeySequence::Copy,                    0,          Qt::CTRL | Qt::Key_Insert,              KB_X11 | KB_Win},
    {QKeySequence::Copy,                    1,          Qt::CTRL | Qt::Key_C,                   KB_All},
    {QKeySequence::Copy,                    0,          Qt::Key_F16,                            KB_X11}, //Copy on sun keyboards
    {QKeySequence::Paste,                   0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Insert,  KB_X11},
    {QKeySequence::Paste,                   1,          Qt::CTRL | Qt::Key_V,                   KB_All},
    {QKeySequence::Paste,                   0,          Qt::SHIFT | Qt::Key_Insert,             KB_Win | KB_X11},
    {QKeySequence::Paste,                   0,          Qt::Key_F18,                            KB_X11}, //Paste on sun keyboards
    {QKeySequence::Paste,                   0,          Qt::META | Qt::Key_Y,                   KB_Mac},
    {QKeySequence::Undo,                    0,          Qt::ALT  | Qt::Key_Backspace,           KB_Win},
    {QKeySequence::Undo,                    1,          Qt::CTRL | Qt::Key_Z,                   KB_All},
    {QKeySequence::Undo,                    0,          Qt::Key_F14,                            KB_X11}, //Undo on sun keyboards
    {QKeySequence::Redo,                    0,          Qt::ALT  | Qt::SHIFT | Qt::Key_Backspace,KB_Win},
    {QKeySequence::Redo,                    0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Z,       KB_Mac},
    {QKeySequence::Redo,                    0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Z,       KB_Win | KB_X11},
    {QKeySequence::Redo,                    1,          Qt::CTRL | Qt::Key_Y,                   KB_Win},
    {QKeySequence::Back,                    1,          Qt::ALT  | Qt::Key_Left,                KB_Win | KB_X11},
    {QKeySequence::Back,                    0,          Qt::CTRL | Qt::Key_Left,                KB_Mac},
    {QKeySequence::Back,                    1,          Qt::CTRL | Qt::Key_BracketLeft,         KB_Mac},
    {QKeySequence::Back,                    0,          Qt::Key_Backspace,                      KB_Win},
    {QKeySequence::Forward,                 1,          Qt::ALT  | Qt::Key_Right,               KB_Win | KB_X11},
    {QKeySequence::Forward,                 0,          Qt::CTRL | Qt::Key_Right,               KB_Mac},
    {QKeySequence::Forward,                 1,          Qt::CTRL | Qt::Key_BracketRight,        KB_Mac},
    {QKeySequence::Forward,                 0,          Qt::SHIFT | Qt::Key_Backspace,          KB_Win},
    {QKeySequence::Refresh,                 1,          Qt::CTRL | Qt::Key_R,                   KB_Gnome | KB_Mac},
    {QKeySequence::Refresh,                 0,          Qt::Key_F5,                             KB_Win | KB_X11},
    {QKeySequence::ZoomIn,                  1,          Qt::CTRL | Qt::Key_Plus,                KB_All},
    {QKeySequence::ZoomOut,                 1,          Qt::CTRL | Qt::Key_Minus,               KB_All},
    {QKeySequence::Print,                   1,          Qt::CTRL | Qt::Key_P,                   KB_All},
    {QKeySequence::AddTab,                  1,          Qt::CTRL | Qt::SHIFT | Qt::Key_N,       KB_KDE},
    {QKeySequence::AddTab,                  0,          Qt::CTRL | Qt::Key_T,                   KB_All},
    {QKeySequence::NextChild,               0,          Qt::CTRL | Qt::Key_F6,                  KB_Win},
    {QKeySequence::NextChild,               0,          Qt::CTRL | Qt::Key_Tab,                 KB_Mac}, //different priority from above
    {QKeySequence::NextChild,               1,          Qt::CTRL | Qt::Key_Tab,                 KB_Win | KB_X11},
    {QKeySequence::NextChild,               1,          Qt::CTRL | Qt::Key_BraceRight,          KB_Mac},
    {QKeySequence::NextChild,               0,          Qt::CTRL | Qt::Key_Comma,               KB_KDE},
    {QKeySequence::NextChild,               0,          Qt::Key_Forward,                        KB_All},
    {QKeySequence::PreviousChild,           0,          Qt::CTRL | Qt::SHIFT | Qt::Key_F6,      KB_Win},
    {QKeySequence::PreviousChild,           0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab, KB_Mac },//different priority from above
    {QKeySequence::PreviousChild,           1,          Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab, KB_Win | KB_X11},
    {QKeySequence::PreviousChild,           1,          Qt::CTRL | Qt::Key_BraceLeft,           KB_Mac},
    {QKeySequence::PreviousChild,           0,          Qt::CTRL | Qt::Key_Period,              KB_KDE},
    {QKeySequence::PreviousChild,           0,          Qt::Key_Back,                           KB_All},
    {QKeySequence::Find,                    0,          Qt::CTRL | Qt::Key_F,                   KB_All},
    {QKeySequence::FindNext,                0,          Qt::CTRL | Qt::Key_G,                   KB_Win},
    {QKeySequence::FindNext,                1,          Qt::CTRL | Qt::Key_G,                   KB_Gnome | KB_Mac},
    {QKeySequence::FindNext,                1,          Qt::Key_F3,                             KB_Win},
    {QKeySequence::FindNext,                0,          Qt::Key_F3,                             KB_X11},
    {QKeySequence::FindPrevious,            0,          Qt::CTRL | Qt::SHIFT | Qt::Key_G,       KB_Win},
    {QKeySequence::FindPrevious,            1,          Qt::CTRL | Qt::SHIFT | Qt::Key_G,       KB_Gnome | KB_Mac},
    {QKeySequence::FindPrevious,            1,          Qt::SHIFT | Qt::Key_F3,                 KB_Win},
    {QKeySequence::FindPrevious,            0,          Qt::SHIFT | Qt::Key_F3,                 KB_X11},
    {QKeySequence::Replace,                 0,          Qt::CTRL | Qt::Key_R,                   KB_KDE},
    {QKeySequence::Replace,                 0,          Qt::CTRL | Qt::Key_H,                   KB_Gnome},
    {QKeySequence::Replace,                 0,          Qt::CTRL | Qt::Key_H,                   KB_Win},
    {QKeySequence::SelectAll,               1,          Qt::CTRL | Qt::Key_A,                   KB_All},
    {QKeySequence::Bold,                    1,          Qt::CTRL | Qt::Key_B,                   KB_All},
    {QKeySequence::Italic,                  0,          Qt::CTRL | Qt::Key_I,                   KB_All},
    {QKeySequence::Underline,               1,          Qt::CTRL | Qt::Key_U,                   KB_All},
    {QKeySequence::MoveToNextChar,          1,          Qt::Key_Right,                          KB_All},
    {QKeySequence::MoveToNextChar,          0,          Qt::META | Qt::Key_F,                   KB_Mac},
    {QKeySequence::MoveToPreviousChar,      1,          Qt::Key_Left,                           KB_All},
    {QKeySequence::MoveToPreviousChar,      0,          Qt::META | Qt::Key_B,                   KB_Mac},
    {QKeySequence::MoveToNextWord,          0,          Qt::ALT  | Qt::Key_Right,               KB_Mac},
    {QKeySequence::MoveToNextWord,          0,          Qt::CTRL | Qt::Key_Right,               KB_Win | KB_X11},
    {QKeySequence::MoveToPreviousWord,      0,          Qt::ALT  | Qt::Key_Left,                KB_Mac},
    {QKeySequence::MoveToPreviousWord,      0,          Qt::CTRL | Qt::Key_Left,                KB_Win | KB_X11},
    {QKeySequence::MoveToNextLine,          1,          Qt::Key_Down,                           KB_All},
    {QKeySequence::MoveToNextLine,          0,          Qt::META | Qt::Key_N,                   KB_Mac},
    {QKeySequence::MoveToPreviousLine,      1,          Qt::Key_Up,                             KB_All},
    {QKeySequence::MoveToPreviousLine,      0,          Qt::META | Qt::Key_P,                   KB_Mac},
    {QKeySequence::MoveToNextPage,          0,          Qt::META | Qt::Key_PageDown,            KB_Mac},
    {QKeySequence::MoveToNextPage,          0,          Qt::META | Qt::Key_Down,                KB_Mac},
    {QKeySequence::MoveToNextPage,          0,          Qt::META | Qt::Key_V,                   KB_Mac},
    {QKeySequence::MoveToNextPage,          0,          Qt::ALT  | Qt::Key_PageDown,            KB_Mac },
    {QKeySequence::MoveToNextPage,          1,          Qt::Key_PageDown,                       KB_All},
    {QKeySequence::MoveToPreviousPage,      0,          Qt::META | Qt::Key_PageUp,              KB_Mac},
    {QKeySequence::MoveToPreviousPage,      0,          Qt::META | Qt::Key_Up,                  KB_Mac},
    {QKeySequence::MoveToPreviousPage,      0,          Qt::ALT  | Qt::Key_PageUp,              KB_Mac },
    {QKeySequence::MoveToPreviousPage,      1,          Qt::Key_PageUp,                         KB_All},
    {QKeySequence::MoveToStartOfLine,       0,          Qt::META | Qt::Key_Left,                KB_Mac},
    {QKeySequence::MoveToStartOfLine,       0,          Qt::CTRL | Qt::Key_Left,                KB_Mac },
    {QKeySequence::MoveToStartOfLine,       0,          Qt::Key_Home,                           KB_Win | KB_X11},
    {QKeySequence::MoveToEndOfLine,         0,          Qt::META | Qt::Key_Right,               KB_Mac},
    {QKeySequence::MoveToEndOfLine,         0,          Qt::CTRL | Qt::Key_Right,               KB_Mac },
    {QKeySequence::MoveToEndOfLine,         0,          Qt::Key_End,                            KB_Win | KB_X11},
    {QKeySequence::MoveToEndOfLine,         0,          Qt::CTRL + Qt::Key_E,                   KB_X11},
    {QKeySequence::MoveToStartOfBlock,      0,          Qt::META | Qt::Key_A,                   KB_Mac},
    {QKeySequence::MoveToStartOfBlock,      1,          Qt::ALT  | Qt::Key_Up,                  KB_Mac}, //mac only
    {QKeySequence::MoveToEndOfBlock,        0,          Qt::META | Qt::Key_E,                   KB_Mac},
    {QKeySequence::MoveToEndOfBlock,        1,          Qt::ALT  | Qt::Key_Down,                KB_Mac}, //mac only
    {QKeySequence::MoveToStartOfDocument,   1,          Qt::CTRL | Qt::Key_Up,                  KB_Mac},
    {QKeySequence::MoveToStartOfDocument,   0,          Qt::CTRL | Qt::Key_Home,                KB_Win | KB_X11},
    {QKeySequence::MoveToStartOfDocument,   0,          Qt::Key_Home,                           KB_Mac},
    {QKeySequence::MoveToEndOfDocument,     1,          Qt::CTRL | Qt::Key_Down,                KB_Mac},
    {QKeySequence::MoveToEndOfDocument,     0,          Qt::CTRL | Qt::Key_End,                 KB_Win | KB_X11},
    {QKeySequence::MoveToEndOfDocument,     0,          Qt::Key_End,                            KB_Mac},
    {QKeySequence::SelectNextChar,          0,          Qt::SHIFT | Qt::Key_Right,              KB_All},
    {QKeySequence::SelectPreviousChar,      0,          Qt::SHIFT | Qt::Key_Left,               KB_All},
    {QKeySequence::SelectNextWord,          0,          Qt::ALT  | Qt::SHIFT | Qt::Key_Right,   KB_Mac},
    {QKeySequence::SelectNextWord,          0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Right,   KB_Win | KB_X11},
    {QKeySequence::SelectPreviousWord,      0,          Qt::ALT  | Qt::SHIFT | Qt::Key_Left,    KB_Mac},
    {QKeySequence::SelectPreviousWord,      0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Left,    KB_Win | KB_X11},
    {QKeySequence::SelectNextLine,          0,          Qt::SHIFT | Qt::Key_Down,               KB_All},
    {QKeySequence::SelectPreviousLine,      0,          Qt::SHIFT | Qt::Key_Up,                 KB_All},
    {QKeySequence::SelectNextPage,          0,          Qt::SHIFT | Qt::Key_PageDown,           KB_All},
    {QKeySequence::SelectPreviousPage,      0,          Qt::SHIFT | Qt::Key_PageUp,             KB_All},
    {QKeySequence::SelectStartOfLine,       0,          Qt::META | Qt::SHIFT | Qt::Key_Left,    KB_Mac},
    {QKeySequence::SelectStartOfLine,       1,          Qt::CTRL | Qt::SHIFT | Qt::Key_Left,    KB_Mac },
    {QKeySequence::SelectStartOfLine,       0,          Qt::SHIFT | Qt::Key_Home,               KB_Win | KB_X11},
    {QKeySequence::SelectEndOfLine,         0,          Qt::META | Qt::SHIFT | Qt::Key_Right,   KB_Mac},
    {QKeySequence::SelectEndOfLine,         1,          Qt::CTRL | Qt::SHIFT | Qt::Key_Right,   KB_Mac },
    {QKeySequence::SelectEndOfLine,         0,          Qt::SHIFT | Qt::Key_End,                KB_Win | KB_X11},
    {QKeySequence::SelectStartOfBlock,      1,          Qt::ALT  | Qt::SHIFT | Qt::Key_Up,      KB_Mac}, //mac only
    {QKeySequence::SelectStartOfBlock,      0,          Qt::META | Qt::SHIFT | Qt::Key_A,       KB_Mac},
    {QKeySequence::SelectEndOfBlock,        1,          Qt::ALT  | Qt::SHIFT | Qt::Key_Down,    KB_Mac}, //mac only
    {QKeySequence::SelectEndOfBlock,        0,          Qt::META | Qt::SHIFT | Qt::Key_E,       KB_Mac},
    {QKeySequence::SelectStartOfDocument,   1,          Qt::CTRL | Qt::SHIFT | Qt::Key_Up,      KB_Mac},
    {QKeySequence::SelectStartOfDocument,   0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Home,    KB_Win | KB_X11},
    {QKeySequence::SelectStartOfDocument,   0,          Qt::SHIFT | Qt::Key_Home,               KB_Mac},
    {QKeySequence::SelectEndOfDocument,     1,          Qt::CTRL | Qt::SHIFT | Qt::Key_Down,    KB_Mac},
    {QKeySequence::SelectEndOfDocument,     0,          Qt::CTRL | Qt::SHIFT | Qt::Key_End,     KB_Win | KB_X11},
    {QKeySequence::SelectEndOfDocument,     0,          Qt::SHIFT | Qt::Key_End,                KB_Mac},
    {QKeySequence::DeleteStartOfWord,       0,          Qt::ALT  | Qt::Key_Backspace,           KB_Mac},
    {QKeySequence::DeleteStartOfWord,       0,          Qt::CTRL | Qt::Key_Backspace,           KB_X11 | KB_Win},
    {QKeySequence::DeleteEndOfWord,         0,          Qt::ALT  | Qt::Key_Delete,              KB_Mac},
    {QKeySequence::DeleteEndOfWord,         0,          Qt::CTRL | Qt::Key_Delete,              KB_X11 | KB_Win},
    {QKeySequence::DeleteEndOfLine,         0,          Qt::CTRL | Qt::Key_K,                   KB_X11}, //emacs (line edit only)
    {QKeySequence::InsertParagraphSeparator,0,          Qt::Key_Enter,                          KB_All},
    {QKeySequence::InsertParagraphSeparator,0,          Qt::Key_Return,                         KB_All},
    {QKeySequence::InsertLineSeparator,     0,          Qt::META | Qt::Key_Enter,               KB_Mac},
    {QKeySequence::InsertLineSeparator,     0,          Qt::META | Qt::Key_Return,              KB_Mac},
    {QKeySequence::InsertLineSeparator,     0,          Qt::SHIFT | Qt::Key_Enter,              KB_All},
    {QKeySequence::InsertLineSeparator,     0,          Qt::SHIFT | Qt::Key_Return,             KB_All},
    {QKeySequence::InsertLineSeparator,     0,          Qt::META | Qt::Key_O,                   KB_Mac},
    {QKeySequence::SaveAs,                  0,          Qt::CTRL | Qt::SHIFT | Qt::Key_S,       KB_Gnome | KB_Mac},
    {QKeySequence::Preferences,             0,          Qt::CTRL | Qt::Key_Comma,               KB_Mac},
    {QKeySequence::Quit,                    0,          Qt::CTRL | Qt::Key_Q,                   KB_X11 | KB_Gnome | KB_KDE | KB_Mac},
    {QKeySequence::FullScreen,              1,          Qt::META | Qt::CTRL | Qt::Key_F,        KB_Mac},
    {QKeySequence::FullScreen,              0,          Qt::ALT  | Qt::Key_Enter,               KB_Win},
    {QKeySequence::FullScreen,              0,          Qt::CTRL | Qt::SHIFT | Qt::Key_F,       KB_KDE},
    {QKeySequence::FullScreen,              1,          Qt::CTRL | Qt::Key_F11,                 KB_Gnome},
    {QKeySequence::FullScreen,              1,          Qt::Key_F11,                            KB_Win | KB_KDE},
    {QKeySequence::Deselect,                0,          Qt::CTRL | Qt::SHIFT | Qt::Key_A,       KB_X11},
    {QKeySequence::DeleteCompleteLine,      0,          Qt::CTRL | Qt::Key_U,                   KB_X11},
    {QKeySequence::Backspace,               0,          Qt::META | Qt::Key_H,                   KB_Mac},
    {QKeySequence::Cancel,                  0,          Qt::Key_Escape,                         KB_All},
    {QKeySequence::Cancel,                  0,          Qt::CTRL | Qt::Key_Period,              KB_Mac}
};

const uint QPlatformThemePrivate::numberOfKeyBindings = sizeof(QPlatformThemePrivate::keyBindings)/(sizeof(QKeyBinding));
#endif

QPlatformThemePrivate::QPlatformThemePrivate()
        : systemPalette(0)
{ }

QPlatformThemePrivate::~QPlatformThemePrivate()
{
    delete systemPalette;
}

Q_GUI_EXPORT QPalette qt_fusionPalette();

void QPlatformThemePrivate::initializeSystemPalette()
{
    Q_ASSERT(!systemPalette);
    systemPalette = new QPalette(qt_fusionPalette());
}

QPlatformTheme::QPlatformTheme()
    : d_ptr(new QPlatformThemePrivate)
{

}

QPlatformTheme::QPlatformTheme(QPlatformThemePrivate *priv)
    : d_ptr(priv)
{ }

QPlatformTheme::~QPlatformTheme()
{

}

bool QPlatformTheme::usePlatformNativeDialog(DialogType type) const
{
    Q_UNUSED(type);
    return false;
}

QPlatformDialogHelper *QPlatformTheme::createPlatformDialogHelper(DialogType type) const
{
    Q_UNUSED(type);
    return 0;
}

const QPalette *QPlatformTheme::palette(Palette type) const
{
    Q_D(const QPlatformTheme);
    if (type == QPlatformTheme::SystemPalette) {
        if (!d->systemPalette)
            const_cast<QPlatformTheme *>(this)->d_ptr->initializeSystemPalette();
        return d->systemPalette;
    }
    return 0;
}

const QFont *QPlatformTheme::font(Font type) const
{
    Q_UNUSED(type)
    return 0;
}

QPixmap QPlatformTheme::standardPixmap(StandardPixmap sp, const QSizeF &size) const
{
    Q_UNUSED(sp);
    Q_UNUSED(size);
    // TODO Should return QCommonStyle pixmaps?
    return QPixmap();
}

/*!
    \brief Return an icon for \a fileInfo, observing \a iconOptions.

    This function is queried by QFileIconProvider and similar classes to obtain
    an icon for a file. If it does not return a non-null icon, fileIconPixmap()
    is queried for a specific size.

    \since 5.8
*/

QIcon QPlatformTheme::fileIcon(const QFileInfo &fileInfo, QPlatformTheme::IconOptions iconOptions) const
{
    Q_UNUSED(fileInfo);
    Q_UNUSED(iconOptions);
    // TODO Should return QCommonStyle pixmaps?
    return QIcon();
}

QVariant QPlatformTheme::themeHint(ThemeHint hint) const
{
    // For theme hints which mirror platform integration style hints, query
    // the platform integration. The base QPlatformIntegration::styleHint()
    // function will in turn query QPlatformTheme::defaultThemeHint() if there
    // is no custom value.
    switch (hint) {
    case QPlatformTheme::CursorFlashTime:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::CursorFlashTime);
    case QPlatformTheme::KeyboardInputInterval:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::KeyboardInputInterval);
    case QPlatformTheme::KeyboardAutoRepeatRate:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::KeyboardAutoRepeatRate);
    case QPlatformTheme::MouseDoubleClickInterval:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::MouseDoubleClickInterval);
    case QPlatformTheme::StartDragDistance:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::StartDragDistance);
    case QPlatformTheme::StartDragTime:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::StartDragTime);
    case QPlatformTheme::StartDragVelocity:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::StartDragVelocity);
    case QPlatformTheme::PasswordMaskDelay:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::PasswordMaskDelay);
    case QPlatformTheme::PasswordMaskCharacter:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::PasswordMaskCharacter);
    case QPlatformTheme::MousePressAndHoldInterval:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::MousePressAndHoldInterval);
    case QPlatformTheme::ItemViewActivateItemOnSingleClick:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::ItemViewActivateItemOnSingleClick);
    case QPlatformTheme::UiEffects:
        return QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::UiEffects);
    default:
        return QPlatformTheme::defaultThemeHint(hint);
    }
}

QVariant QPlatformTheme::defaultThemeHint(ThemeHint hint)
{
    switch (hint) {
    case QPlatformTheme::CursorFlashTime:
        return QVariant(1000);
    case QPlatformTheme::KeyboardInputInterval:
        return QVariant(400);
    case QPlatformTheme::KeyboardAutoRepeatRate:
        return QVariant(30);
    case QPlatformTheme::MouseDoubleClickInterval:
        return QVariant(400);
    case QPlatformTheme::StartDragDistance:
        return QVariant(10);
    case QPlatformTheme::StartDragTime:
        return QVariant(500);
    case QPlatformTheme::PasswordMaskDelay:
        return QVariant(int(0));
    case QPlatformTheme::PasswordMaskCharacter:
        return QVariant(QChar(0x25CF));
    case QPlatformTheme::StartDragVelocity:
        return QVariant(int(0)); // no limit
    case QPlatformTheme::UseFullScreenForPopupMenu:
        return QVariant(false);
    case QPlatformTheme::WindowAutoPlacement:
        return QVariant(false);
    case QPlatformTheme::DialogButtonBoxLayout:
        return QVariant(int(0));
    case QPlatformTheme::DialogButtonBoxButtonsHaveIcons:
        return QVariant(false);
    case QPlatformTheme::ItemViewActivateItemOnSingleClick:
        return QVariant(false);
    case QPlatformTheme::ToolButtonStyle:
        return QVariant(int(Qt::ToolButtonIconOnly));
    case QPlatformTheme::ToolBarIconSize:
        return QVariant(int(0));
    case QPlatformTheme::SystemIconThemeName:
    case QPlatformTheme::SystemIconFallbackThemeName:
        return QVariant(QString());
    case QPlatformTheme::IconThemeSearchPaths:
        return QVariant(QStringList());
    case QPlatformTheme::IconFallbackSearchPaths:
        return QVariant(QStringList());
    case QPlatformTheme::StyleNames:
        return QVariant(QStringList());
    case QPlatformTheme::ShowShortcutsInContextMenus:
        return QVariant(false);
    case TextCursorWidth:
        return QVariant(1);
    case DropShadow:
        return QVariant(false);
    case MaximumScrollBarDragDistance:
        return QVariant(-1);
    case KeyboardScheme:
        return QVariant(int(WindowsKeyboardScheme));
    case UiEffects:
        return QVariant(int(0));
    case SpellCheckUnderlineStyle:
        return QVariant(int(QTextCharFormat::WaveUnderline));
    case TabFocusBehavior:
        return QVariant(int(Qt::TabFocusAllControls));
    case IconPixmapSizes:
        return QVariant::fromValue(QList<int>());
    case DialogSnapToDefaultButton:
    case ContextMenuOnMouseRelease:
        return QVariant(false);
    case MousePressAndHoldInterval:
        return QVariant(800);
    case MouseDoubleClickDistance:
        {
            bool ok = false;
            const int dist = qEnvironmentVariableIntValue("QT_DBL_CLICK_DIST", &ok);
            return QVariant(ok ? dist : 5);
        }
    case WheelScrollLines:
        return QVariant(3);
    case TouchDoubleTapDistance:
        {
            bool ok = false;
            int dist = qEnvironmentVariableIntValue("QT_DBL_TAP_DIST", &ok);
            if (!ok)
                dist = defaultThemeHint(MouseDoubleClickDistance).toInt(&ok) * 2;
            return QVariant(ok ? dist : 10);
        }
    case MouseQuickSelectionThreshold:
        return QVariant(10);
    }
    return QVariant();
}

QPlatformMenuItem *QPlatformTheme::createPlatformMenuItem() const
{
    return 0;
}

QPlatformMenu *QPlatformTheme::createPlatformMenu() const
{
    return 0;
}

QPlatformMenuBar *QPlatformTheme::createPlatformMenuBar() const
{
    return 0;
}

#ifndef QT_NO_SYSTEMTRAYICON
/*!
   Factory function for QSystemTrayIcon. This function will return 0 if the platform
   integration does not support creating any system tray icon.
*/
QPlatformSystemTrayIcon *QPlatformTheme::createPlatformSystemTrayIcon() const
{
    return 0;
}
#endif

/*!
   Factory function for the QIconEngine used by QIcon::fromTheme(). By default this
   function returns a QIconLoaderEngine, but subclasses can reimplement it to
   provide their own.

   It is especially useful to benefit from some platform specific facilities or
   optimizations like an inter-process cache in systems mostly built with Qt.

   \since 5.1
*/
QIconEngine *QPlatformTheme::createIconEngine(const QString &iconName) const
{
    return new QIconLoaderEngine(iconName);
}

#if defined(Q_OS_MACX)
static inline int maybeSwapShortcut(int shortcut)
{
    if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
        uint oldshortcut = shortcut;
        shortcut &= ~(Qt::CTRL | Qt::META);
        if (oldshortcut & Qt::CTRL)
            shortcut |= Qt::META;
        if (oldshortcut & Qt::META)
            shortcut |= Qt::CTRL;
    }
    return shortcut;
}
#endif

#ifndef QT_NO_SHORTCUT
// mixed-mode predicate: all of these overloads are actually needed (but not all for every compiler)
struct ByStandardKey {
    typedef bool result_type;

    bool operator()(QKeySequence::StandardKey lhs, QKeySequence::StandardKey rhs) const
    { return lhs < rhs; }

    bool operator()(const QKeyBinding& lhs, const QKeyBinding& rhs) const
    { return operator()(lhs.standardKey, rhs.standardKey); }

    bool operator()(QKeySequence::StandardKey lhs, const QKeyBinding& rhs) const
    { return operator()(lhs, rhs.standardKey); }

    bool operator()(const QKeyBinding& lhs, QKeySequence::StandardKey rhs) const
    { return operator()(lhs.standardKey, rhs); }
};

/*!
   Returns the key sequence that should be used for a standard action.

  \since 5.2
 */
QList<QKeySequence> QPlatformTheme::keyBindings(QKeySequence::StandardKey key) const
{
    const uint platform = QPlatformThemePrivate::currentKeyPlatforms();
    QList <QKeySequence> list;

    std::pair<const QKeyBinding *, const QKeyBinding *> range =
        std::equal_range(QPlatformThemePrivate::keyBindings,
                         QPlatformThemePrivate::keyBindings + QPlatformThemePrivate::numberOfKeyBindings,
                         key, ByStandardKey());

    for (const QKeyBinding *it = range.first; it < range.second; ++it) {
        if (!(it->platform & platform))
            continue;

        uint shortcut =
#if defined(Q_OS_MACX)
            maybeSwapShortcut(it->shortcut);
#else
            it->shortcut;
#endif
        if (it->priority > 0)
            list.prepend(QKeySequence(shortcut));
        else
            list.append(QKeySequence(shortcut));
    }

    return list;
}
#endif

/*!
   Returns the text of a standard \a button.

  \since 5.3
  \sa QPlatformDialogHelper::StandardButton
 */

QString QPlatformTheme::standardButtonText(int button) const
{
    return QPlatformTheme::defaultStandardButtonText(button);
}

/*!
   Returns the mnemonic that should be used for a standard \a button.

  \since 5.9
  \sa QPlatformDialogHelper::StandardButton
 */

QKeySequence QPlatformTheme::standardButtonShortcut(int button) const
{
    Q_UNUSED(button)
    return QKeySequence();
}

QString QPlatformTheme::defaultStandardButtonText(int button)
{
    switch (button) {
    case QPlatformDialogHelper::Ok:
        return QCoreApplication::translate("QPlatformTheme", "OK");
    case QPlatformDialogHelper::Save:
        return QCoreApplication::translate("QPlatformTheme", "Save");
    case QPlatformDialogHelper::SaveAll:
        return QCoreApplication::translate("QPlatformTheme", "Save All");
    case QPlatformDialogHelper::Open:
        return QCoreApplication::translate("QPlatformTheme", "Open");
    case QPlatformDialogHelper::Yes:
        return QCoreApplication::translate("QPlatformTheme", "&Yes");
    case QPlatformDialogHelper::YesToAll:
        return QCoreApplication::translate("QPlatformTheme", "Yes to &All");
    case QPlatformDialogHelper::No:
        return QCoreApplication::translate("QPlatformTheme", "&No");
    case QPlatformDialogHelper::NoToAll:
        return QCoreApplication::translate("QPlatformTheme", "N&o to All");
    case QPlatformDialogHelper::Abort:
        return QCoreApplication::translate("QPlatformTheme", "Abort");
    case QPlatformDialogHelper::Retry:
        return QCoreApplication::translate("QPlatformTheme", "Retry");
    case QPlatformDialogHelper::Ignore:
        return QCoreApplication::translate("QPlatformTheme", "Ignore");
    case QPlatformDialogHelper::Close:
        return QCoreApplication::translate("QPlatformTheme", "Close");
    case QPlatformDialogHelper::Cancel:
        return QCoreApplication::translate("QPlatformTheme", "Cancel");
    case QPlatformDialogHelper::Discard:
        return QCoreApplication::translate("QPlatformTheme", "Discard");
    case QPlatformDialogHelper::Help:
        return QCoreApplication::translate("QPlatformTheme", "Help");
    case QPlatformDialogHelper::Apply:
        return QCoreApplication::translate("QPlatformTheme", "Apply");
    case QPlatformDialogHelper::Reset:
        return QCoreApplication::translate("QPlatformTheme", "Reset");
    case QPlatformDialogHelper::RestoreDefaults:
        return QCoreApplication::translate("QPlatformTheme", "Restore Defaults");
    default:
        break;
    }
    return QString();
}

QString QPlatformTheme::removeMnemonics(const QString &original)
{
    QString returnText(original.size(), 0);
    int finalDest = 0;
    int currPos = 0;
    int l = original.length();
    while (l) {
        if (original.at(currPos) == QLatin1Char('&')) {
            ++currPos;
            --l;
            if (l == 0)
                break;
        } else if (original.at(currPos) == QLatin1Char('(') && l >= 4 &&
                   original.at(currPos + 1) == QLatin1Char('&') &&
                   original.at(currPos + 2) != QLatin1Char('&') &&
                   original.at(currPos + 3) == QLatin1Char(')')) {
            /* remove mnemonics its format is "\s*(&X)" */
            int n = 0;
            while (finalDest > n && returnText.at(finalDest - n - 1).isSpace())
                ++n;
            finalDest -= n;
            currPos += 4;
            l -= 4;
            continue;
        }
        returnText[finalDest] = original.at(currPos);
        ++currPos;
        ++finalDest;
        --l;
    }
    returnText.truncate(finalDest);
    return returnText;
}

unsigned QPlatformThemePrivate::currentKeyPlatforms()
{
    const uint keyboardScheme = QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::KeyboardScheme).toInt();
    unsigned result = 1u << keyboardScheme;
#ifndef QT_NO_SHORTCUT
    if (keyboardScheme == QPlatformTheme::KdeKeyboardScheme
        || keyboardScheme == QPlatformTheme::GnomeKeyboardScheme
        || keyboardScheme == QPlatformTheme::CdeKeyboardScheme)
        result |= KB_X11;
#endif
    return result;
}

QT_END_NAMESPACE
