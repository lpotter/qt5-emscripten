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

#include "qdirectfbbackingstore.h"
#include "qdirectfbintegration.h"
#include "qdirectfbblitter.h"
#include "qdirectfbconvenience.h"
#include "qdirectfbwindow.h"
#include <private/qpixmap_blitter_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QDirectFbBackingStore::QDirectFbBackingStore(QWindow *window)
    : QPlatformBackingStore(window), m_pixmap(0), m_pmdata(0)
{
    IDirectFBWindow *dfbWindow = static_cast<QDirectFbWindow *>(window->handle())->dfbWindow();
    dfbWindow->GetSurface(dfbWindow, m_dfbSurface.outPtr());

//WRONGSIZE
    QDirectFbBlitter *blitter = new QDirectFbBlitter(window->size(), m_dfbSurface.data());
    m_pmdata = new QDirectFbBlitterPlatformPixmap;
    m_pmdata->setBlittable(blitter);
    m_pixmap.reset(new QPixmap(m_pmdata));
}

QPaintDevice *QDirectFbBackingStore::paintDevice()
{
    return m_pixmap.data();
}

void QDirectFbBackingStore::flush(QWindow *, const QRegion &region, const QPoint &offset)
{
    m_pmdata->blittable()->unlock();

    for (const QRect &rect : region) {
        DFBRegion dfbReg(rect.x() + offset.x(),rect.y() + offset.y(),rect.right() + offset.x(),rect.bottom() + offset.y());
        m_dfbSurface->Flip(m_dfbSurface.data(), &dfbReg, DFBSurfaceFlipFlags(DSFLIP_BLIT|DSFLIP_ONSYNC));
    }
}

void QDirectFbBackingStore::resize(const QSize &size, const QRegion& reg)
{
    Q_UNUSED(reg);

    if ((m_pmdata->width() == size.width()) &&
        (m_pmdata->height() == size.height()))
        return;

    QDirectFbBlitter *blitter = new QDirectFbBlitter(size, m_dfbSurface.data());
    m_pmdata->setBlittable(blitter);
}

static inline void scrollSurface(IDirectFBSurface *surface, const QRect &r, int dx, int dy)
{
    const DFBRectangle rect(r.x(), r.y(), r.width(), r.height());
    surface->Blit(surface, surface, &rect, r.x() + dx, r.y() + dy);
    const DFBRegion region(rect.x + dx, rect.y + dy, r.right() + dx, r.bottom() + dy);
    surface->Flip(surface, &region, DFBSurfaceFlipFlags(DSFLIP_BLIT));
}

bool QDirectFbBackingStore::scroll(const QRegion &area, int dx, int dy)
{
    m_pmdata->blittable()->unlock();

    if (!m_dfbSurface || area.isEmpty())
        return false;
    m_dfbSurface->SetBlittingFlags(m_dfbSurface.data(), DSBLIT_NOFX);
    if (area.rectCount() == 1) {
        scrollSurface(m_dfbSurface.data(), area.boundingRect(), dx, dy);
    } else {
        for (const QRect &rect : area)
            scrollSurface(m_dfbSurface.data(), rect, dx, dy);
    }
    return true;
}

QImage QDirectFbBackingStore::toImage() const
{
    return m_pixmap.data()->toImage();
}

QT_END_NAMESPACE
