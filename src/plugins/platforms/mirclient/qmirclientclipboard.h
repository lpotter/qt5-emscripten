/****************************************************************************
**
** Copyright (C) 2016 Canonical, Ltd.
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


#ifndef QMIRCLIENTCLIPBOARD_H
#define QMIRCLIENTCLIPBOARD_H

#include <qpa/qplatformclipboard.h>

#include <QMimeData>
#include <QPointer>

namespace com {
    namespace ubuntu {
        namespace content {
            class Hub;
        }
    }
}

class QDBusPendingCallWatcher;

class QMirClientClipboard : public QObject, public QPlatformClipboard
{
    Q_OBJECT
public:
    QMirClientClipboard();
    virtual ~QMirClientClipboard();

    // QPlatformClipboard methods.
    QMimeData* mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData* data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;
    bool ownsMode(QClipboard::Mode mode) const override;

private Q_SLOTS:
    void onApplicationStateChanged(Qt::ApplicationState state);

private:
    void updateMimeData();
    void requestMimeData();

    QMimeData *mMimeData;

    enum {
        OutdatedClipboard, // Our mimeData is outdated, need to fetch latest from ContentHub
        SyncingClipboard, // Our mimeData is outdated and we are waiting for ContentHub to reply with the latest paste
        SyncedClipboard // Our mimeData is in sync with what ContentHub has
    } mClipboardState{OutdatedClipboard};

    com::ubuntu::content::Hub *mContentHub;

    QDBusPendingCallWatcher *mPasteReply{nullptr};
};

#endif // QMIRCLIENTCLIPBOARD_H
