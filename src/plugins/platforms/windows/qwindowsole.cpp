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

#include "qwindowsole.h"
#include "qwindowsmime.h"
#include "qwindowscontext.h"
\
#include <QtGui/qevent.h>
#include <QtGui/qwindow.h>
#include <QtGui/qpainter.h>
#include <QtGui/qcursor.h>
#include <QtGui/qguiapplication.h>

#include <QtCore/qmimedata.h>
#include <QtCore/qdebug.h>

#include <shlobj.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsOleDataObject
    \brief OLE data container

   The following methods are NOT supported for data transfer using the
   clipboard or drag-drop:
   \list
   \li IDataObject::SetData    -- return E_NOTIMPL
   \li IDataObject::DAdvise    -- return OLE_E_ADVISENOTSUPPORTED
   \li ::DUnadvise
   \li ::EnumDAdvise
   \li IDataObject::GetCanonicalFormatEtc -- return E_NOTIMPL
       (NOTE: must set pformatetcOut->ptd = NULL)
   \endlist

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsOleDataObject::QWindowsOleDataObject(QMimeData *mimeData) :
    data(mimeData),
    CF_PERFORMEDDROPEFFECT(RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT))
{
    qCDebug(lcQpaMime) << __FUNCTION__ << mimeData->formats();
}

QWindowsOleDataObject::~QWindowsOleDataObject() = default;

void QWindowsOleDataObject::releaseQt()
{
    data = 0;
}

QMimeData *QWindowsOleDataObject::mimeData() const
{
    return data.data();
}

DWORD QWindowsOleDataObject::reportedPerformedEffect() const
{
    return performedEffect;
}

STDMETHODIMP
QWindowsOleDataObject::GetData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
    HRESULT hr = ResultFromScode(DATA_E_FORMATETC);

    if (data) {
        const QWindowsMimeConverter &mc = QWindowsContext::instance()->mimeConverter();
        if (QWindowsMime *converter = mc.converterFromMime(*pformatetc, data))
            if (converter->convertFromMime(*pformatetc, data, pmedium))
                hr = ResultFromScode(S_OK);
    }

    if (QWindowsContext::verbose > 1 && lcQpaMime().isDebugEnabled())
        qCDebug(lcQpaMime) <<__FUNCTION__ << *pformatetc << "returns" << hex << showbase << quint64(hr);

    return hr;
}

STDMETHODIMP
QWindowsOleDataObject::GetDataHere(LPFORMATETC, LPSTGMEDIUM)
{
    return ResultFromScode(DATA_E_FORMATETC);
}

STDMETHODIMP
QWindowsOleDataObject::QueryGetData(LPFORMATETC pformatetc)
{
    HRESULT hr = ResultFromScode(DATA_E_FORMATETC);

    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) << __FUNCTION__;

    if (data) {
        const QWindowsMimeConverter &mc = QWindowsContext::instance()->mimeConverter();
        hr = mc.converterFromMime(*pformatetc, data) ?
             ResultFromScode(S_OK) : ResultFromScode(S_FALSE);
    }
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) <<  __FUNCTION__ << " returns 0x" << hex << int(hr);
    return hr;
}

STDMETHODIMP
QWindowsOleDataObject::GetCanonicalFormatEtc(LPFORMATETC, LPFORMATETC pformatetcOut)
{
    pformatetcOut->ptd = NULL;
    return ResultFromScode(E_NOTIMPL);
}

STDMETHODIMP
QWindowsOleDataObject::SetData(LPFORMATETC pFormatetc, STGMEDIUM *pMedium, BOOL fRelease)
{
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) << __FUNCTION__;

    HRESULT hr = ResultFromScode(E_NOTIMPL);

    if (pFormatetc->cfFormat == CF_PERFORMEDDROPEFFECT && pMedium->tymed == TYMED_HGLOBAL) {
        DWORD * val = (DWORD*)GlobalLock(pMedium->hGlobal);
        performedEffect = *val;
        GlobalUnlock(pMedium->hGlobal);
        if (fRelease)
            ReleaseStgMedium(pMedium);
        hr = ResultFromScode(S_OK);
    }
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) <<  __FUNCTION__ << " returns 0x" << hex << int(hr);
    return hr;
}


STDMETHODIMP
QWindowsOleDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc)
{
     if (QWindowsContext::verbose > 1)
         qCDebug(lcQpaMime) << __FUNCTION__ << "dwDirection=" << dwDirection;

    if (!data)
        return ResultFromScode(DATA_E_FORMATETC);

    SCODE sc = S_OK;

    QVector<FORMATETC> fmtetcs;
    if (dwDirection == DATADIR_GET) {
        QWindowsMimeConverter &mc = QWindowsContext::instance()->mimeConverter();
        fmtetcs = mc.allFormatsForMime(data);
    } else {
        FORMATETC formatetc;
        formatetc.cfFormat = CLIPFORMAT(CF_PERFORMEDDROPEFFECT);
        formatetc.dwAspect = DVASPECT_CONTENT;
        formatetc.lindex = -1;
        formatetc.ptd = NULL;
        formatetc.tymed = TYMED_HGLOBAL;
        fmtetcs.append(formatetc);
    }

    QWindowsOleEnumFmtEtc *enumFmtEtc = new QWindowsOleEnumFmtEtc(fmtetcs);
    *ppenumFormatEtc = enumFmtEtc;
    if (enumFmtEtc->isNull()) {
        delete enumFmtEtc;
        *ppenumFormatEtc = NULL;
        sc = E_OUTOFMEMORY;
    }

    return ResultFromScode(sc);
}

STDMETHODIMP
QWindowsOleDataObject::DAdvise(FORMATETC FAR*, DWORD,
                       LPADVISESINK, DWORD FAR*)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}


STDMETHODIMP
QWindowsOleDataObject::DUnadvise(DWORD)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

STDMETHODIMP
QWindowsOleDataObject::EnumDAdvise(LPENUMSTATDATA FAR*)
{
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

/*!
    \class QWindowsOleEnumFmtEtc
    \brief Enumerates the FORMATETC structures supported by QWindowsOleDataObject.
    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsOleEnumFmtEtc::QWindowsOleEnumFmtEtc(const QVector<FORMATETC> &fmtetcs)
{
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) << __FUNCTION__ << fmtetcs;
    m_lpfmtetcs.reserve(fmtetcs.count());
    for (int idx = 0; idx < fmtetcs.count(); ++idx) {
        LPFORMATETC destetc = new FORMATETC();
        if (copyFormatEtc(destetc, &(fmtetcs.at(idx)))) {
            m_lpfmtetcs.append(destetc);
        } else {
            m_isNull = true;
            delete destetc;
            break;
        }
    }
}

QWindowsOleEnumFmtEtc::QWindowsOleEnumFmtEtc(const QVector<LPFORMATETC> &lpfmtetcs)
{
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaMime) << __FUNCTION__;
    m_lpfmtetcs.reserve(lpfmtetcs.count());
    for (int idx = 0; idx < lpfmtetcs.count(); ++idx) {
        LPFORMATETC srcetc = lpfmtetcs.at(idx);
        LPFORMATETC destetc = new FORMATETC();
        if (copyFormatEtc(destetc, srcetc)) {
            m_lpfmtetcs.append(destetc);
        } else {
            m_isNull = true;
            delete destetc;
            break;
        }
    }
}

QWindowsOleEnumFmtEtc::~QWindowsOleEnumFmtEtc()
{
    LPMALLOC pmalloc;

    if (CoGetMalloc(MEMCTX_TASK, &pmalloc) == NOERROR) {
        for (int idx = 0; idx < m_lpfmtetcs.count(); ++idx) {
            LPFORMATETC tmpetc = m_lpfmtetcs.at(idx);
            if (tmpetc->ptd)
                pmalloc->Free(tmpetc->ptd);
            delete tmpetc;
        }

        pmalloc->Release();
    }
    m_lpfmtetcs.clear();
}

bool QWindowsOleEnumFmtEtc::isNull() const
{
    return m_isNull;
}

// IEnumFORMATETC methods
STDMETHODIMP
QWindowsOleEnumFmtEtc::Next(ULONG celt, LPFORMATETC rgelt, ULONG FAR* pceltFetched)
{
    ULONG i=0;
    ULONG nOffset;

    if (rgelt == NULL)
        return ResultFromScode(E_INVALIDARG);

    while (i < celt) {
        nOffset = m_nIndex + i;

        if (nOffset < ULONG(m_lpfmtetcs.count())) {
            copyFormatEtc(rgelt + i, m_lpfmtetcs.at(int(nOffset)));
            i++;
        } else {
            break;
        }
    }

    m_nIndex += i;

    if (pceltFetched != NULL)
        *pceltFetched = i;

    if (i != celt)
        return ResultFromScode(S_FALSE);

    return NOERROR;
}

STDMETHODIMP
QWindowsOleEnumFmtEtc::Skip(ULONG celt)
{
    ULONG i=0;
    ULONG nOffset;

    while (i < celt) {
        nOffset = m_nIndex + i;

        if (nOffset < ULONG(m_lpfmtetcs.count())) {
            i++;
        } else {
            break;
        }
    }

    m_nIndex += i;

    if (i != celt)
        return ResultFromScode(S_FALSE);

    return NOERROR;
}

STDMETHODIMP
QWindowsOleEnumFmtEtc::Reset()
{
    m_nIndex = 0;
    return NOERROR;
}

STDMETHODIMP
QWindowsOleEnumFmtEtc::Clone(LPENUMFORMATETC FAR* newEnum)
{
    if (newEnum == NULL)
        return ResultFromScode(E_INVALIDARG);

    QWindowsOleEnumFmtEtc *result = new QWindowsOleEnumFmtEtc(m_lpfmtetcs);
    result->m_nIndex = m_nIndex;

    if (result->isNull()) {
        delete result;
        return ResultFromScode(E_OUTOFMEMORY);
    }

    *newEnum = result;
    return NOERROR;
}

bool QWindowsOleEnumFmtEtc::copyFormatEtc(LPFORMATETC dest, const FORMATETC *src) const
{
    if (dest == NULL || src == NULL)
        return false;

    *dest = *src;

    if (src->ptd) {
        LPMALLOC pmalloc;

        if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR)
            return false;

        pmalloc->Alloc(src->ptd->tdSize);
        memcpy(dest->ptd, src->ptd, size_t(src->ptd->tdSize));

        pmalloc->Release();
    }

    return true;
}

QT_END_NAMESPACE
