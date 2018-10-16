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

#include "qqnxvirtualkeyboardpps.h"
#include "qqnxscreen.h"

#include <QtCore/QDebug>
#include <QtCore/QSocketNotifier>
#include <QtCore/private/qcore_unix_p.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/iomsg.h>
#include <sys/pps.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(QQNXVIRTUALKEYBOARD_DEBUG)
#define qVirtualKeyboardDebug qDebug
#else
#define qVirtualKeyboardDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

const char  *QQnxVirtualKeyboardPps::ms_PPSPath = "/pps/services/input/control";
const size_t QQnxVirtualKeyboardPps::ms_bufferSize = 2048;

QQnxVirtualKeyboardPps::QQnxVirtualKeyboardPps()
    : m_encoder(0),
      m_decoder(0),
      m_buffer(0),
      m_fd(-1),
      m_readNotifier(0)
{
}

QQnxVirtualKeyboardPps::~QQnxVirtualKeyboardPps()
{
    close();
}

void QQnxVirtualKeyboardPps::start()
{
    qVirtualKeyboardDebug("starting keyboard event processing");
    if (!connect())
        return;
}

void QQnxVirtualKeyboardPps::close()
{
    delete m_readNotifier;
    m_readNotifier = 0;

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }

    if (m_decoder) {
        pps_decoder_cleanup(m_decoder);
        delete m_decoder;
        m_decoder = 0;
    }

    if (m_encoder) {
        pps_encoder_cleanup(m_encoder);
        delete m_encoder;
        m_encoder = 0;
    }

    delete [] m_buffer;
    m_buffer = 0;
}

bool QQnxVirtualKeyboardPps::connect()
{
    close();

    m_encoder = new pps_encoder_t;
    m_decoder = new pps_decoder_t;

    pps_encoder_initialize(m_encoder, false);
    pps_decoder_initialize(m_decoder, 0);

    errno = 0;
    m_fd = ::open(ms_PPSPath, O_RDWR);
    if (m_fd == -1)
    {
        qVirtualKeyboardDebug() << "Unable to open" << ms_PPSPath
                                               << ':' << strerror(errno);
        close();
        return false;
    }

    m_buffer = new char[ms_bufferSize];
    if (Q_UNLIKELY(!m_buffer)) {
        qCritical("QQnxVirtualKeyboard: Unable to allocate buffer of %d bytes. "
                  "Size is unavailable.",  ms_bufferSize);
        return false;
    }

    if (!queryPPSInfo())
        return false;

    m_readNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Read);
    QObject::connect(m_readNotifier, SIGNAL(activated(int)), this, SLOT(ppsDataReady()));

    return true;
}

bool QQnxVirtualKeyboardPps::queryPPSInfo()
{
    if (!prepareToSend())
        return false;

    // Request info, requires id to regenerate res message.
    pps_encoder_add_string(m_encoder, "msg", "info");
    pps_encoder_add_string(m_encoder, "id", "1");

    return writeCurrentPPSEncoder();
}

void QQnxVirtualKeyboardPps::ppsDataReady()
{
    ssize_t nread = qt_safe_read(m_fd, m_buffer, ms_bufferSize - 1);

    qVirtualKeyboardDebug("keyboardMessage size: %zd", nread);
    if (nread < 0){
        connect(); // reconnect
        return;
    }

    // We sometimes get spurious read notifications when no data is available.
    // Bail out early in this case
    if (nread == 0)
        return;

    // nread is the real space necessary, not the amount read.
    if (Q_UNLIKELY(static_cast<size_t>(nread) > ms_bufferSize - 1)) {
        qCritical("QQnxVirtualKeyboard: Keyboard buffer size too short; need %u.", nread + 1);
        connect(); // reconnect
        return;
    }

    m_buffer[nread] = 0;
    pps_decoder_parse_pps_str(m_decoder, m_buffer);
    pps_decoder_push(m_decoder, 0);
#if defined(QQNXVIRTUALKEYBOARD_DEBUG)
    pps_decoder_dump_tree(m_decoder, stderr);
#endif

    const char *value;
    if (Q_UNLIKELY(pps_decoder_get_string(m_decoder, "error", &value) == PPS_DECODER_OK)) {
        qCritical("QQnxVirtualKeyboard: Keyboard PPS decoder error: %s", value ? value : "[null]");
        return;
    }

    if (pps_decoder_get_string(m_decoder, "msg", &value) == PPS_DECODER_OK) {
        if (strcmp(value, "show") == 0)
            setVisible(true);
        else if (strcmp(value, "hide") == 0)
            setVisible(false);
        else if (strcmp(value, "info") == 0)
            handleKeyboardInfoMessage();
        else if (strcmp(value, "connect") == 0)
            qVirtualKeyboardDebug("Unhandled command 'connect'");
        else
            qCritical("QQnxVirtualKeyboard: Unexpected keyboard PPS msg value: %s", value ? value : "[null]");
    } else if (pps_decoder_get_string(m_decoder, "res", &value) == PPS_DECODER_OK) {
        if (strcmp(value, "info") == 0)
            handleKeyboardInfoMessage();
        else
            qCritical("QQnxVirtualKeyboard: Unexpected keyboard PPS res value: %s", value ? value : "[null]");
    } else {
        qCritical("QQnxVirtualKeyboard: Unexpected keyboard PPS message type");
    }
}

void QQnxVirtualKeyboardPps::handleKeyboardInfoMessage()
{
    int newHeight = 0;

    if (Q_UNLIKELY(pps_decoder_push(m_decoder, "dat") != PPS_DECODER_OK)) {
        qCritical("QQnxVirtualKeyboard: Keyboard PPS dat object not found");
        return;
    }
    if (Q_UNLIKELY(pps_decoder_get_int(m_decoder, "size", &newHeight) != PPS_DECODER_OK)) {
        qCritical("QQnxVirtualKeyboard: Keyboard PPS size field not found");
        return;
    }
    setHeight(newHeight);

    qVirtualKeyboardDebug("size=%d", newHeight);
}

bool QQnxVirtualKeyboardPps::showKeyboard()
{
    qVirtualKeyboardDebug();

    if (!prepareToSend())
        return false;

    // NOTE:  This must be done everytime the keyboard is shown even if there is no change because
    // hiding the keyboard wipes the setting.
    applyKeyboardOptions();

    if (isVisible())
        return true;

    pps_encoder_reset(m_encoder);

    // Send the show message.
    pps_encoder_add_string(m_encoder, "msg", "show");

    return writeCurrentPPSEncoder();
}

bool QQnxVirtualKeyboardPps::hideKeyboard()
{
    qVirtualKeyboardDebug();

    if (!prepareToSend())
        return false;

    pps_encoder_add_string(m_encoder, "msg", "hide");

    return writeCurrentPPSEncoder();
}

bool QQnxVirtualKeyboardPps::prepareToSend()
{
    if (m_fd == -1 && !connect())
        return false;

    pps_encoder_reset(m_encoder);
    return true;
}

bool QQnxVirtualKeyboardPps::writeCurrentPPSEncoder()
{
    if (::write(m_fd, pps_encoder_buffer(m_encoder), pps_encoder_length(m_encoder)) == -1) {
        close();
        return false;
    }
    return true;
}

void QQnxVirtualKeyboardPps::applyKeyboardOptions()
{
    if (!prepareToSend())
        return;

    // Send the options message.
    pps_encoder_add_string(m_encoder, "msg", "options");
    pps_encoder_start_object(m_encoder, "dat");

    pps_encoder_add_string(m_encoder, "enter", enterKeyTypeStr());
    pps_encoder_add_string(m_encoder, "type", keyboardModeStr());

    pps_encoder_end_object(m_encoder);

    writeCurrentPPSEncoder();
}

const char* QQnxVirtualKeyboardPps::keyboardModeStr() const
{
    switch (keyboardMode()) {
    case Url:
        return "url";
    case Email:
        return "email";
    case Web:
        return "web";
    case NumPunc:
        return "num_punc";
    case Number:
        return "number";
    case Symbol:
        return "symbol";
    case Phone:
        return "phone";
    case Pin:
        return "pin";
    case Password:
        return "password";
    case Alphanumeric:
        return "alphanumeric";
    case Default:
        return "default";
    }

    return "";
}

const char* QQnxVirtualKeyboardPps::enterKeyTypeStr() const
{
    switch (enterKeyType()) {
    case DefaultReturn:
        return "enter.default";
    case Connect:
        return "enter.connect";
    case Done:
        return "enter.done";
    case Go:
        return "enter.go";
    case Join:
        return "enter.join";
    case Next:
        return "enter.next";
    case Search:
        return "enter.search";
    case Send:
        return "enter.send";
    case Submit:
        return "enter.submit";
    }

    return "";
}

QT_END_NAMESPACE
