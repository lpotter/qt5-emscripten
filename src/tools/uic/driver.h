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

#ifndef DRIVER_H
#define DRIVER_H

#include "option.h"
#include <qhash.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>

QT_BEGIN_NAMESPACE

class QTextStream;
class DomUI;
class DomWidget;
class DomSpacer;
class DomLayout;
class DomLayoutItem;
class DomActionGroup;
class DomAction;
class DomButtonGroup;

class Driver
{
    Q_DISABLE_COPY(Driver)
public:
    Driver();
    virtual ~Driver();

    // tools
    bool printDependencies(const QString &fileName);
    bool uic(const QString &fileName, QTextStream *output = 0);
    bool uic(const QString &fileName, DomUI *ui, QTextStream *output = 0);

    // configuration
    inline QTextStream &output() const { return *m_output; }
    inline Option &option() { return m_option; }

    // initialization
    void reset();

    // error
    inline QStringList problems() { return m_problems; }
    inline void addProblem(const QString &problem) { m_problems.append(problem); }

    // utils
    static QString headerFileName(const QString &fileName);
    QString headerFileName() const;

    static QString normalizedName(const QString &name);
    static QString qtify(const QString &name);
    QString unique(const QString &instanceName=QString(),
                   const QString &className=QString());

    // symbol table
    QString findOrInsertWidget(DomWidget *ui_widget);
    QString findOrInsertSpacer(DomSpacer *ui_spacer);
    QString findOrInsertLayout(DomLayout *ui_layout);
    QString findOrInsertLayoutItem(DomLayoutItem *ui_layoutItem);
    QString findOrInsertName(const QString &name);
    QString findOrInsertActionGroup(DomActionGroup *ui_group);
    QString findOrInsertAction(DomAction *ui_action);
    QString findOrInsertButtonGroup(const DomButtonGroup *ui_group);
    // Find a group by its non-uniqified name
    const DomButtonGroup *findButtonGroup(const QString &attributeName) const;

    inline bool hasName(const QString &name) const
    { return m_nameRepository.contains(name); }

    DomWidget *widgetByName(const QString &name) const;
    DomSpacer *spacerByName(const QString &name) const;
    DomLayout *layoutByName(const QString &name) const;
    DomActionGroup *actionGroupByName(const QString &name) const;
    DomAction *actionByName(const QString &name) const;

    // pixmap
    void insertPixmap(const QString &pixmap);
    bool containsPixmap(const QString &pixmap) const;

    bool useIdBasedTranslations() const { return m_idBasedTranslations; }
    void setUseIdBasedTranslations(bool u) { m_idBasedTranslations = u; }

private:
    Option m_option;
    QTextStream m_stdout;
    QTextStream *m_output;

    QStringList m_problems;

    // symbol tables
    QHash<DomWidget*, QString> m_widgets;
    QHash<DomSpacer*, QString> m_spacers;
    QHash<DomLayout*, QString> m_layouts;
    QHash<DomActionGroup*, QString> m_actionGroups;
    typedef QHash<const DomButtonGroup*, QString> ButtonGroupNameHash;
    ButtonGroupNameHash m_buttonGroups;
    QHash<DomAction*, QString> m_actions;
    QHash<QString, bool> m_nameRepository;
    QHash<QString, bool> m_pixmaps;
    bool m_idBasedTranslations = false;
};

QT_END_NAMESPACE

#endif // DRIVER_H
