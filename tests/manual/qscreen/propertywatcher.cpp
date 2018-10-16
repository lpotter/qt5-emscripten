/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "propertywatcher.h"
#include <QMetaProperty>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include "propertyfield.h"

PropertyWatcher::PropertyWatcher(QObject *subject, QString annotation, QWidget *parent)
    : QWidget(parent), m_subject(nullptr), m_formLayout(new QFormLayout(this))
{
    setMinimumSize(450, 300);
    m_formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setSubject(subject, annotation);
}

class UpdatesEnabledBlocker
{
    Q_DISABLE_COPY(UpdatesEnabledBlocker);
public:
    explicit UpdatesEnabledBlocker(QWidget *w) : m_widget(w)
    {
        m_widget->setUpdatesEnabled(false);
    }
    ~UpdatesEnabledBlocker()
    {
        m_widget->setUpdatesEnabled(true);
        m_widget->update();
    }

private:
    QWidget *m_widget;
};

void  PropertyWatcher::setSubject(QObject *s, const QString &annotation)
{
    if (s == m_subject)
        return;

    UpdatesEnabledBlocker blocker(this);

    if (m_subject) {
        disconnect(m_subject, &QObject::destroyed, this, &PropertyWatcher::subjectDestroyed);
        for (int i = m_formLayout->count() - 1; i >= 0; --i) {
            QLayoutItem *item = m_formLayout->takeAt(i);
            delete item->widget();
            delete item;
        }
        window()->setWindowTitle(QString());
        window()->setWindowIconText(QString());
    }

    m_subject = s;
    if (!m_subject)
        return;

    const QMetaObject* meta = m_subject->metaObject();
    QString title = QLatin1String("Properties ") + QLatin1String(meta->className());
    if (!m_subject->objectName().isEmpty())
        title += QLatin1Char(' ') + m_subject->objectName();
    if (!annotation.isEmpty())
        title += QLatin1Char(' ') + annotation;
    window()->setWindowTitle(title);

    for (int i = 0, count = meta->propertyCount(); i < count; ++i) {
        const QMetaProperty prop = meta->property(i);
        if (prop.isReadable()) {
            QLabel *label = new QLabel(prop.name(), this);
            PropertyField *field = new PropertyField(m_subject, prop, this);
            m_formLayout->addRow(label, field);
            if (!qstrcmp(prop.name(), "name"))
                window()->setWindowIconText(prop.read(m_subject).toString());
            label->setVisible(true);
            field->setVisible(true);
        }
    }
    connect(m_subject, &QObject::destroyed, this, &PropertyWatcher::subjectDestroyed);

    QPushButton *updateButton = new QPushButton(QLatin1String("Update"), this);
    connect(updateButton, &QPushButton::clicked, this, &PropertyWatcher::updateAllFields);
    m_formLayout->addRow(QString(), updateButton);
}

void PropertyWatcher::updateAllFields()
{
    QList<PropertyField *> fields = findChildren<PropertyField*>();
    foreach (PropertyField *field, fields)
        field->propertyChanged();
    emit updatedAllFields(this);
}

void PropertyWatcher::subjectDestroyed()
{
    qDebug("screen destroyed");
}
