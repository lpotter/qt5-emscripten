/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QWINDOWSUIATEXTPROVIDER_H
#define QWINDOWSUIATEXTPROVIDER_H

#include <QtGui/qtguiglobal.h>
#if QT_CONFIG(accessibility)

#include "qwindowsuiabaseprovider.h"
#include "qwindowsuiatextrangeprovider.h"

QT_BEGIN_NAMESPACE

// Implements the Text control pattern provider. Used for text controls.
class QWindowsUiaTextProvider : public QWindowsUiaBaseProvider,
                                public QWindowsComBase<ITextProvider2>
{
    Q_DISABLE_COPY(QWindowsUiaTextProvider)
public:
    explicit QWindowsUiaTextProvider(QAccessible::Id id);
    ~QWindowsUiaTextProvider();

    // IUnknown overrides
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, LPVOID *iface) override;

    // ITextProvider
    HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetVisibleRanges(SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE RangeFromChild(IRawElementProviderSimple *childElement, ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE RangeFromPoint(UiaPoint point, ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_DocumentRange(ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_SupportedTextSelection(SupportedTextSelection *pRetVal) override;

    // ITextProvider2
    HRESULT STDMETHODCALLTYPE RangeFromAnnotation(IRawElementProviderSimple *annotationElement, ITextRangeProvider **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetCaretRange(BOOL *isActive, ITextRangeProvider **pRetVal) override;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

#endif // QWINDOWSUIATEXTPROVIDER_H
