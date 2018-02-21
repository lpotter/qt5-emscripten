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


// there has got to be a better way...
QByteArray m_clipboardArray;
QByteArray m_clipboardFormat;
using namespace emscripten;

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
    : mMimeData(new QMimeData)
{
    qDebug() << Q_FUNC_INFO << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << this;
}

QHtml5Clipboard::~QHtml5Clipboard()
{
    delete mMimeData;
}

QMimeData* QHtml5Clipboard::mimeData(QClipboard::Mode mode)
{
    qDebug() << Q_FUNC_INFO << this<< mMimeData << mode;
    if (mode != QClipboard::Clipboard)
        return nullptr;

    // Blocks dataChanged() signal from being emitted. Makes no sense to emit it from
    // inside the data getter.
//    const QSignalBlocker blocker(this);

    qDebug() << Q_FUNC_INFO << mMimeData->text();
    return mMimeData;
}

void QHtml5Clipboard::setMimeData(QMimeData* mimeData, QClipboard::Mode mode)
{
    qDebug() << Q_FUNC_INFO << mode  << this << mimeData->text()
             << mimeData->formats();
    mMimeData = mimeData;
    if (copyMode) {
        if (mimeData->hasText()) {
            m_clipboardFormat = "text/plain";
            m_clipboardArray = mimeData->text().toUtf8().constData();
        } else if (mimeData->hasHtml()) {
            m_clipboardFormat = "text/html";
            m_clipboardArray = mimeData->html().toUtf8().constData();
        } else if (mimeData->hasImage()) {
//            m_clipboardFormat = mimeData->formats().at(0).toUtf8().constData();
//            m_clipboardArray = mimeData->imageData().to;
        } else if (mimeData->hasColor()) {
            //    else if (mMimeData->hasFormat())
        }

    }//copymode
    emitChanged(QClipboard::Clipboard);
}

void QHtml5Clipboard::updateMimeData()
{
    qDebug() << Q_FUNC_INFO;
    delete mMimeData;
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

void QHtml5Clipboard::QHtml5ClipboardCopy()
{
    qDebug() << Q_FUNC_INFO;
    // set mimedata
//   mMimeData = new QMimeData;
  //  mMimeData->setText(text);
  //  setMimeData(data, QClipboard::Clipboard); // Selection
//    QHtml5Integration::get()->clipboard()->setMimeData(mData, QClipboard::Clipboard);

}

void QHtml5Clipboard::QHtml5ClipboardPaste(QMimeData *mData)
{
    //set minedata
    qDebug() << Q_FUNC_INFO << mData->text();
    QHtml5Integration::get()->clipboard()->setMimeData(mData, QClipboard::Clipboard);
}

//void QHtml5Clipboard::setClipboard(emscripten::val format, emscripten::val data)
//{
////    EM_ASM_ARGS({
////                 copyData = $0;
////                 clipbordFormat = $1;
////                 console.log("setting clipboard format "+clipbordFormat)
////                }, data, format);
//}
