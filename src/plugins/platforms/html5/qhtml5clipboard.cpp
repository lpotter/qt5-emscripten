/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhtml5clipboard.h"
#include "qhtml5window.h"

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>

#include <QCoreApplication>

using namespace emscripten;

// there has got to be a better way...
QByteArray m_clipboardArray;
QByteArray m_clipboardFormat;

val getClipboardData()
{
    return val(m_clipboardArray.constData());
}

val getClipboardFormat()
{
    return val(m_clipboardFormat.constData());
}

EMSCRIPTEN_BINDINGS(clipboard_module) {
    function("getClipboardData", &getClipboardData);
    function("getClipboardFormat", &getClipboardFormat);
}

QHtml5Clipboard::QHtml5Clipboard()
{
}

QHtml5Clipboard::~QHtml5Clipboard()
{
}

QMimeData* QHtml5Clipboard::mimeData(QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return nullptr;

    return QPlatformClipboard::mimeData(mode);
}

void QHtml5Clipboard::setMimeData(QMimeData* mimeData, QClipboard::Mode mode)
{
    qDebug() << Q_FUNC_INFO << mimeData->formats();

    if (mimeData->hasText()) {
        m_clipboardFormat = "text/plain";
        m_clipboardArray = mimeData->text().toUtf8().constData();
    } else if (mimeData->hasHtml()) {
        m_clipboardFormat = "text/html";
        m_clipboardArray = mimeData->html().toUtf8().constData();
    } else if (mimeData->formats().at(0).startsWith("image") ) { // "image/*"
        qDebug() << Q_FUNC_INFO << "has image";
        m_clipboardArray = mimeData->data(mimeData->formats().at(0));
        m_clipboardFormat = mimeData->formats().at(0).toUtf8().constData();
    } else if (mimeData->hasColor()) { //application/x-color
        qDebug() << Q_FUNC_INFO << "has color";
        //    else if (mMimeData->hasFormat())
    }

    qDebug() << Q_FUNC_INFO << "copyKeyMode" << copyKeyMode << "pasteKeyMode" << pasteKeyMode;
    if (copyKeyMode || !pasteKeyMode) {//menu copy need to manually copy up
        EM_ASM(
                document.execCommand('copy');
                );
    }

    QPlatformClipboard::setMimeData(mimeData, mode);
}

bool QHtml5Clipboard::supportsMode(QClipboard::Mode mode) const
{
    return mode == QClipboard::Clipboard;
}

bool QHtml5Clipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

void QHtml5Clipboard::QHtml5ClipboardPaste(QMimeData *mData)
{
    qDebug() << Q_FUNC_INFO;
    QHtml5Integration::get()->clipboard()->setMimeData(mData, QClipboard::Clipboard);
}
