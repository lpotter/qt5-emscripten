/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CUSTOMWIDGETSINFO_H
#define CUSTOMWIDGETSINFO_H

#include "treewalker.h"
#include <qstringlist.h>
#include <qmap.h>

QT_BEGIN_NAMESPACE

class Driver;
class DomScript;

class CustomWidgetsInfo : public TreeWalker
{
public:
    CustomWidgetsInfo();

    void acceptUI(DomUI *node) override;

    void acceptCustomWidgets(DomCustomWidgets *node) override;
    void acceptCustomWidget(DomCustomWidget *node) override;

    inline QStringList customWidgets() const
    { return m_customWidgets.keys(); }

    inline bool hasCustomWidget(const QString &name) const
    { return m_customWidgets.contains(name); }

    inline DomCustomWidget *customWidget(const QString &name) const
    { return m_customWidgets.value(name); }

    QString customWidgetAddPageMethod(const QString &name) const;

    QString realClassName(const QString &className) const;

    bool extends(const QString &className, QLatin1String baseClassName) const;

    bool isCustomWidgetContainer(const QString &className) const;

private:
    typedef QMap<QString, DomCustomWidget*> NameCustomWidgetMap;
    NameCustomWidgetMap m_customWidgets;
};

QT_END_NAMESPACE

#endif // CUSTOMWIDGETSINFO_H
