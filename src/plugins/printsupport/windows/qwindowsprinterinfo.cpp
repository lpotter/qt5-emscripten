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

#include "qprinterinfo.h"
#include "qprinterinfo_p.h"

#include <qstringlist.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

extern QPrinter::PaperSize mapDevmodePaperSize(int s);

//QList<QPrinterInfo> QPrinterInfo::availablePrinters()
//{
//    QList<QPrinterInfo> printers;

//    DWORD needed = 0;
//    DWORD returned = 0;
//    if (!EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, 0, 0, &needed, &returned)) {
//        LPBYTE buffer = new BYTE[needed];
//        if (EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 4, buffer, needed, &needed, &returned)) {
//            PPRINTER_INFO_4 infoList = reinterpret_cast<PPRINTER_INFO_4>(buffer);
//            QPrinterInfo defPrn = defaultPrinter();
//            for (uint i = 0; i < returned; ++i) {
//                QString printerName(QString::fromWCharArray(infoList[i].pPrinterName));

//                QPrinterInfo printerInfo(printerName);
//                if (printerInfo.printerName() == defPrn.printerName())
//                    printerInfo.d_ptr->isDefault = true;
//                printers.append(printerInfo);
//            }
//        }
//        delete [] buffer;
//    }

//    return printers;
//}

//QPrinterInfo QPrinterInfo::defaultPrinter()
//{
//    QString noPrinters(QLatin1String("qt_no_printers"));
//    wchar_t buffer[256];
//    GetProfileString(L"windows", L"device", (wchar_t*)noPrinters.utf16(), buffer, 256);
//    QString output = QString::fromWCharArray(buffer);
//    if (output != noPrinters) {
//        // Filter out the name of the printer, which should be everything before a comma.
//        QString printerName = output.split(QLatin1Char(',')).value(0);
//        QPrinterInfo printerInfo(printerName);
//        printerInfo.d_ptr->isDefault = true;
//        return printerInfo;
//    }

//    return QPrinterInfo();
//}

//QList<QPrinter::PaperSize> QPrinterInfo::supportedPaperSizes() const
//{
//    const Q_D(QPrinterInfo);

//    QList<QPrinter::PaperSize> paperSizes;
//    if (isNull())
//        return paperSizes;

//    DWORD size = DeviceCapabilities(reinterpret_cast<const wchar_t*>(d->name.utf16()),
//                                    NULL, DC_PAPERS, NULL, NULL);
//    if ((int)size != -1) {
//        wchar_t *papers = new wchar_t[size];
//        size = DeviceCapabilities(reinterpret_cast<const wchar_t*>(d->name.utf16()),
//                                  NULL, DC_PAPERS, papers, NULL);
//        for (int c = 0; c < (int)size; ++c)
//            paperSizes.append(mapDevmodePaperSize(papers[c]));
//        delete [] papers;
//    }

//    return paperSizes;
//}

#endif // QT_NO_PRINTER

QT_END_NAMESPACE
