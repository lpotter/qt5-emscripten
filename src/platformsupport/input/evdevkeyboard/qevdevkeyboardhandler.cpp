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

#include "qevdevkeyboardhandler_p.h"

#include <qplatformdefs.h>

#include <QFile>
#include <QSocketNotifier>
#include <QStringList>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <qpa/qwindowsysteminterface.h>
#include <private/qcore_unix_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qinputdevicemanager_p.h>

#ifdef Q_OS_FREEBSD
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEvdevKey, "qt.qpa.input")
Q_LOGGING_CATEGORY(qLcEvdevKeyMap, "qt.qpa.input.keymap")

// simple builtin US keymap
#include "qevdevkeyboard_defaultmap_p.h"

void QFdContainer::reset() Q_DECL_NOTHROW
{
    if (m_fd >= 0)
        qt_safe_close(m_fd);
    m_fd = -1;
}

QEvdevKeyboardHandler::QEvdevKeyboardHandler(const QString &device, QFdContainer &fd, bool disableZap, bool enableCompose, const QString &keymapFile)
    : m_device(device), m_fd(fd.release()), m_notify(nullptr),
      m_modifiers(0), m_composing(0), m_dead_unicode(0xffff),
      m_no_zap(disableZap), m_do_compose(enableCompose),
      m_keymap(0), m_keymap_size(0), m_keycompose(0), m_keycompose_size(0)
{
    qCDebug(qLcEvdevKey) << "Create keyboard handler with for device" << device;

    setObjectName(QLatin1String("LinuxInput Keyboard Handler"));

    memset(m_locks, 0, sizeof(m_locks));

    if (keymapFile.isEmpty() || !loadKeymap(keymapFile))
        unloadKeymap();

    // socket notifier for events on the keyboard device
    m_notify = new QSocketNotifier(m_fd.get(), QSocketNotifier::Read, this);
    connect(m_notify, &QSocketNotifier::activated, this, &QEvdevKeyboardHandler::readKeycode);
}

QEvdevKeyboardHandler::~QEvdevKeyboardHandler()
{
    unloadKeymap();
}

QEvdevKeyboardHandler *QEvdevKeyboardHandler::create(const QString &device,
                                                     const QString &specification,
                                                     const QString &defaultKeymapFile)
{
    qCDebug(qLcEvdevKey) << "Try to create keyboard handler for" << device << specification;

    QString keymapFile = defaultKeymapFile;
    int repeatDelay = 400;
    int repeatRate = 80;
    bool disableZap = false;
    bool enableCompose = false;
    int grab = 0;

    const auto args = specification.splitRef(QLatin1Char(':'));
    for (const QStringRef &arg : args) {
        if (arg.startsWith(QLatin1String("keymap=")))
            keymapFile = arg.mid(7).toString();
        else if (arg == QLatin1String("disable-zap"))
            disableZap = true;
        else if (arg == QLatin1String("enable-compose"))
            enableCompose = true;
        else if (arg.startsWith(QLatin1String("repeat-delay=")))
            repeatDelay = arg.mid(13).toInt();
        else if (arg.startsWith(QLatin1String("repeat-rate=")))
            repeatRate = arg.mid(12).toInt();
        else if (arg.startsWith(QLatin1String("grab=")))
            grab = arg.mid(5).toInt();
    }

    qCDebug(qLcEvdevKey) << "Opening keyboard at" << device;

    QFdContainer fd(qt_safe_open(device.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0));
    if (fd.get() >= 0) {
        ::ioctl(fd.get(), EVIOCGRAB, grab);
        if (repeatDelay > 0 && repeatRate > 0) {
            int kbdrep[2] = { repeatDelay, repeatRate };
            ::ioctl(fd.get(), EVIOCSREP, kbdrep);
        }

        return new QEvdevKeyboardHandler(device, fd, disableZap, enableCompose, keymapFile);
    } else {
        qWarning("Cannot open keyboard input device '%s': %s", qPrintable(device), strerror(errno));
        return 0;
    }
}

void QEvdevKeyboardHandler::switchLed(int led, bool state)
{
    qCDebug(qLcEvdevKey) << "switchLed" << led << state;

    struct ::input_event led_ie;
    ::gettimeofday(&led_ie.time, 0);
    led_ie.type = EV_LED;
    led_ie.code = led;
    led_ie.value = state;

    qt_safe_write(m_fd.get(), &led_ie, sizeof(led_ie));
}

void QEvdevKeyboardHandler::readKeycode()
{
    struct ::input_event buffer[32];
    int n = 0;

    forever {
        int result = qt_safe_read(m_fd.get(), reinterpret_cast<char *>(buffer) + n, sizeof(buffer) - n);

        if (result == 0) {
            qWarning("evdevkeyboard: Got EOF from the input device");
            return;
        } else if (result < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                qErrnoWarning(errno, "evdevkeyboard: Could not read from input device");
                // If the device got disconnected, stop reading, otherwise we get flooded
                // by the above error over and over again.
                if (errno == ENODEV) {
                    delete m_notify;
                    m_notify = nullptr;
                    m_fd.reset();
                }
                return;
            }
        } else {
            n += result;
            if (n % sizeof(buffer[0]) == 0)
                break;
        }
    }

    n /= sizeof(buffer[0]);

    for (int i = 0; i < n; ++i) {
        if (buffer[i].type != EV_KEY)
            continue;

        quint16 code = buffer[i].code;
        qint32 value = buffer[i].value;

        QEvdevKeyboardHandler::KeycodeAction ka;
        ka = processKeycode(code, value != 0, value == 2);

        switch (ka) {
        case QEvdevKeyboardHandler::CapsLockOn:
        case QEvdevKeyboardHandler::CapsLockOff:
            switchLed(LED_CAPSL, ka == QEvdevKeyboardHandler::CapsLockOn);
            break;

        case QEvdevKeyboardHandler::NumLockOn:
        case QEvdevKeyboardHandler::NumLockOff:
            switchLed(LED_NUML, ka == QEvdevKeyboardHandler::NumLockOn);
            break;

        case QEvdevKeyboardHandler::ScrollLockOn:
        case QEvdevKeyboardHandler::ScrollLockOff:
            switchLed(LED_SCROLLL, ka == QEvdevKeyboardHandler::ScrollLockOn);
            break;

        default:
            // ignore console switching and reboot
            break;
        }
    }
}

void QEvdevKeyboardHandler::processKeyEvent(int nativecode, int unicode, int qtcode,
                                            Qt::KeyboardModifiers modifiers, bool isPress, bool autoRepeat)
{
    if (!autoRepeat)
        QGuiApplicationPrivate::inputDeviceManager()->setKeyboardModifiers(QEvdevKeyboardHandler::toQtModifiers(m_modifiers));

    QWindowSystemInterface::handleExtendedKeyEvent(0, (isPress ? QEvent::KeyPress : QEvent::KeyRelease),
                                                   qtcode, modifiers, nativecode + 8, 0, int(modifiers),
                                                   (unicode != 0xffff ) ? QString(unicode) : QString(), autoRepeat);
}

QEvdevKeyboardHandler::KeycodeAction QEvdevKeyboardHandler::processKeycode(quint16 keycode, bool pressed, bool autorepeat)
{
    KeycodeAction result = None;
    bool first_press = pressed && !autorepeat;

    const QEvdevKeyboardMap::Mapping *map_plain = 0;
    const QEvdevKeyboardMap::Mapping *map_withmod = 0;

    quint8 modifiers = m_modifiers;

    // get a specific and plain mapping for the keycode and the current modifiers
    for (int i = 0; i < m_keymap_size && !(map_plain && map_withmod); ++i) {
        const QEvdevKeyboardMap::Mapping *m = m_keymap + i;
        if (m->keycode == keycode) {
            if (m->modifiers == 0)
                map_plain = m;

            quint8 testmods = m_modifiers;
            if (m_locks[0] /*CapsLock*/ && (m->flags & QEvdevKeyboardMap::IsLetter))
                testmods ^= QEvdevKeyboardMap::ModShift;
            if (m->modifiers == testmods)
                map_withmod = m;
        }
    }

    if (m_locks[0] /*CapsLock*/ && map_withmod && (map_withmod->flags & QEvdevKeyboardMap::IsLetter))
        modifiers ^= QEvdevKeyboardMap::ModShift;

    qCDebug(qLcEvdevKeyMap, "Processing key event: keycode=%3d, modifiers=%02x pressed=%d, autorepeat=%d  |  plain=%d, withmod=%d, size=%d",
            keycode, modifiers, pressed ? 1 : 0, autorepeat ? 1 : 0,
            int(map_plain ? map_plain - m_keymap : -1),
            int(map_withmod ? map_withmod - m_keymap : -1),
            m_keymap_size);

    const QEvdevKeyboardMap::Mapping *it = map_withmod ? map_withmod : map_plain;

    if (!it) {
        // we couldn't even find a plain mapping
        qCDebug(qLcEvdevKeyMap, "Could not find a suitable mapping for keycode: %3d, modifiers: %02x", keycode, modifiers);
        return result;
    }

    bool skip = false;
    quint16 unicode = it->unicode;
    quint32 qtcode = it->qtcode;

    if ((it->flags & QEvdevKeyboardMap::IsModifier) && it->special) {
        // this is a modifier, i.e. Shift, Alt, ...
        if (pressed)
            m_modifiers |= quint8(it->special);
        else
            m_modifiers &= ~quint8(it->special);
    } else if (qtcode >= Qt::Key_CapsLock && qtcode <= Qt::Key_ScrollLock) {
        // (Caps|Num|Scroll)Lock
        if (first_press) {
            quint8 &lock = m_locks[qtcode - Qt::Key_CapsLock];
            lock ^= 1;

            switch (qtcode) {
            case Qt::Key_CapsLock  : result = lock ? CapsLockOn : CapsLockOff; break;
            case Qt::Key_NumLock   : result = lock ? NumLockOn : NumLockOff; break;
            case Qt::Key_ScrollLock: result = lock ? ScrollLockOn : ScrollLockOff; break;
            default                : break;
            }
        }
    } else if ((it->flags & QEvdevKeyboardMap::IsSystem) && it->special && first_press) {
        switch (it->special) {
        case QEvdevKeyboardMap::SystemReboot:
            result = Reboot;
            break;

        case QEvdevKeyboardMap::SystemZap:
            if (!m_no_zap)
                qApp->quit();
            break;

        case QEvdevKeyboardMap::SystemConsolePrevious:
            result = PreviousConsole;
            break;

        case QEvdevKeyboardMap::SystemConsoleNext:
            result = NextConsole;
            break;

        default:
            if (it->special >= QEvdevKeyboardMap::SystemConsoleFirst &&
                it->special <= QEvdevKeyboardMap::SystemConsoleLast) {
                result = KeycodeAction(SwitchConsoleFirst + ((it->special & QEvdevKeyboardMap::SystemConsoleMask) & SwitchConsoleMask));
            }
            break;
        }

        skip = true; // no need to tell Qt about it
    } else if ((qtcode == Qt::Key_Multi_key) && m_do_compose) {
        // the Compose key was pressed
        if (first_press)
            m_composing = 2;
        skip = true;
    } else if ((it->flags & QEvdevKeyboardMap::IsDead) && m_do_compose) {
        // a Dead key was pressed
        if (first_press && m_composing == 1 && m_dead_unicode == unicode) { // twice
            m_composing = 0;
            qtcode = Qt::Key_unknown; // otherwise it would be Qt::Key_Dead...
        } else if (first_press && unicode != 0xffff) {
            m_dead_unicode = unicode;
            m_composing = 1;
            skip = true;
        } else {
            skip = true;
        }
    }

    if (!skip) {
        // a normal key was pressed
        const int modmask = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier;

        // we couldn't find a specific mapping for the current modifiers,
        // or that mapping didn't have special modifiers:
        // so just report the plain mapping with additional modifiers.
        if ((it == map_plain && it != map_withmod) ||
            (map_withmod && !(map_withmod->qtcode & modmask))) {
            qtcode |= QEvdevKeyboardHandler::toQtModifiers(modifiers);
        }

        if (m_composing == 2 && first_press && !(it->flags & QEvdevKeyboardMap::IsModifier)) {
            // the last key press was the Compose key
            if (unicode != 0xffff) {
                int idx = 0;
                // check if this code is in the compose table at all
                for ( ; idx < m_keycompose_size; ++idx) {
                    if (m_keycompose[idx].first == unicode)
                        break;
                }
                if (idx < m_keycompose_size) {
                    // found it -> simulate a Dead key press
                    m_dead_unicode = unicode;
                    unicode = 0xffff;
                    m_composing = 1;
                    skip = true;
                } else {
                    m_composing = 0;
                }
            } else {
                m_composing = 0;
            }
        } else if (m_composing == 1 && first_press && !(it->flags & QEvdevKeyboardMap::IsModifier)) {
            // the last key press was a Dead key
            bool valid = false;
            if (unicode != 0xffff) {
                int idx = 0;
                // check if this code is in the compose table at all
                for ( ; idx < m_keycompose_size; ++idx) {
                    if (m_keycompose[idx].first == m_dead_unicode && m_keycompose[idx].second == unicode)
                        break;
                }
                if (idx < m_keycompose_size) {
                    quint16 composed = m_keycompose[idx].result;
                    if (composed != 0xffff) {
                        unicode = composed;
                        qtcode = Qt::Key_unknown;
                        valid = true;
                    }
                }
            }
            if (!valid) {
                unicode = m_dead_unicode;
                qtcode = Qt::Key_unknown;
            }
            m_composing = 0;
        }

        if (!skip) {
            // Up until now qtcode contained both the key and modifiers. Split it.
            Qt::KeyboardModifiers qtmods = Qt::KeyboardModifiers(qtcode & modmask);
            qtcode &= ~modmask;

            // qtmods here is the modifier state before the event, i.e. not
            // including the current key in case it is a modifier.
            qCDebug(qLcEvdevKeyMap, "Processing: uni=%04x, qt=%08x, qtmod=%08x", unicode, qtcode, int(qtmods));

            // If NumLockOff and keypad key pressed remap event sent
            if (!m_locks[1] && (qtmods & Qt::KeypadModifier) &&
                 keycode >= 71 &&
                 keycode <= 83 &&
                 keycode != 74 &&
                 keycode != 78) {

                unicode = 0xffff;
                switch (keycode) {
                case 71: //7 --> Home
                    qtcode = Qt::Key_Home;
                    break;
                case 72: //8 --> Up
                    qtcode = Qt::Key_Up;
                    break;
                case 73: //9 --> PgUp
                    qtcode = Qt::Key_PageUp;
                    break;
                case 75: //4 --> Left
                    qtcode = Qt::Key_Left;
                    break;
                case 76: //5 --> Clear
                    qtcode = Qt::Key_Clear;
                    break;
                case 77: //6 --> right
                    qtcode = Qt::Key_Right;
                    break;
                case 79: //1 --> End
                    qtcode = Qt::Key_End;
                    break;
                case 80: //2 --> Down
                    qtcode = Qt::Key_Down;
                    break;
                case 81: //3 --> PgDn
                    qtcode = Qt::Key_PageDown;
                    break;
                case 82: //0 --> Ins
                    qtcode = Qt::Key_Insert;
                    break;
                case 83: //, --> Del
                    qtcode = Qt::Key_Delete;
                    break;
                }
            }

            // Map SHIFT + Tab to SHIFT + Backtab, QShortcutMap knows about this translation
            if (qtcode == Qt::Key_Tab && (qtmods & Qt::ShiftModifier) == Qt::ShiftModifier)
                qtcode = Qt::Key_Backtab;

            // Generate the QPA event.
            processKeyEvent(keycode, unicode, qtcode, qtmods, pressed, autorepeat);
        }
    }
    return result;
}

void QEvdevKeyboardHandler::unloadKeymap()
{
    qCDebug(qLcEvdevKey) << "Unload current keymap and restore built-in";

    if (m_keymap && m_keymap != s_keymap_default)
        delete [] m_keymap;
    if (m_keycompose && m_keycompose != s_keycompose_default)
        delete [] m_keycompose;

    m_keymap = s_keymap_default;
    m_keymap_size = sizeof(s_keymap_default) / sizeof(s_keymap_default[0]);
    m_keycompose = s_keycompose_default;
    m_keycompose_size = sizeof(s_keycompose_default) / sizeof(s_keycompose_default[0]);

    // reset state, so we could switch keymaps at runtime
    m_modifiers = 0;
    memset(m_locks, 0, sizeof(m_locks));
    m_composing = 0;
    m_dead_unicode = 0xffff;

    //Set locks according to keyboard leds
    quint16 ledbits[1];
    memset(ledbits, 0, sizeof(ledbits));
    if (::ioctl(m_fd.get(), EVIOCGLED(sizeof(ledbits)), ledbits) < 0) {
        qWarning("evdevkeyboard: Failed to query led states");
        switchLed(LED_NUML,false);
        switchLed(LED_CAPSL, false);
        switchLed(LED_SCROLLL,false);
    } else {
        //Capslock
        if ((ledbits[0]&0x02) > 0)
            m_locks[0] = 1;
        //Numlock
        if ((ledbits[0]&0x01) > 0)
            m_locks[1] = 1;
        //Scrollock
        if ((ledbits[0]&0x04) > 0)
            m_locks[2] = 1;
        qCDebug(qLcEvdevKey, "numlock=%d , capslock=%d, scrolllock=%d", m_locks[1], m_locks[0], m_locks[2]);
    }
}

bool QEvdevKeyboardHandler::loadKeymap(const QString &file)
{
    qCDebug(qLcEvdevKey) << "Loading keymap" << file;

    QFile f(file);

    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("Could not open keymap file '%s'", qPrintable(file));
        return false;
    }

    // .qmap files have a very simple structure:
    // quint32 magic           (QKeyboard::FileMagic)
    // quint32 version         (1)
    // quint32 keymap_size     (# of struct QKeyboard::Mappings)
    // quint32 keycompose_size (# of struct QKeyboard::Composings)
    // all QKeyboard::Mappings via QDataStream::operator(<<|>>)
    // all QKeyboard::Composings via QDataStream::operator(<<|>>)

    quint32 qmap_magic, qmap_version, qmap_keymap_size, qmap_keycompose_size;

    QDataStream ds(&f);

    ds >> qmap_magic >> qmap_version >> qmap_keymap_size >> qmap_keycompose_size;

    if (ds.status() != QDataStream::Ok || qmap_magic != QEvdevKeyboardMap::FileMagic || qmap_version != 1 || qmap_keymap_size == 0) {
        qWarning("'%s' is not a valid .qmap keymap file", qPrintable(file));
        return false;
    }

    QEvdevKeyboardMap::Mapping *qmap_keymap = new QEvdevKeyboardMap::Mapping[qmap_keymap_size];
    QEvdevKeyboardMap::Composing *qmap_keycompose = qmap_keycompose_size ? new QEvdevKeyboardMap::Composing[qmap_keycompose_size] : 0;

    for (quint32 i = 0; i < qmap_keymap_size; ++i)
        ds >> qmap_keymap[i];
    for (quint32 i = 0; i < qmap_keycompose_size; ++i)
        ds >> qmap_keycompose[i];

    if (ds.status() != QDataStream::Ok) {
        delete [] qmap_keymap;
        delete [] qmap_keycompose;

        qWarning("Keymap file '%s' can not be loaded.", qPrintable(file));
        return false;
    }

    // unload currently active and clear state
    unloadKeymap();

    m_keymap = qmap_keymap;
    m_keymap_size = qmap_keymap_size;
    m_keycompose = qmap_keycompose;
    m_keycompose_size = qmap_keycompose_size;

    m_do_compose = true;

    return true;
}

QT_END_NAMESPACE
