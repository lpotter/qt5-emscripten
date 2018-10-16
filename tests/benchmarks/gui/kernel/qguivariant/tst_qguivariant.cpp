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

#include <qtest.h>
#include <QtCore/qvariant.h>

#define ITERATION_COUNT 1e5

class tst_QGuiVariant : public QObject
{
    Q_OBJECT

public:
    tst_QGuiVariant();
    virtual ~tst_QGuiVariant();

private slots:
    void createGuiType_data();
    void createGuiType();
    void createGuiTypeCopy_data();
    void createGuiTypeCopy();
};

tst_QGuiVariant::tst_QGuiVariant()
{
}

tst_QGuiVariant::~tst_QGuiVariant()
{
}

void tst_QGuiVariant::createGuiType_data()
{
    QTest::addColumn<int>("typeId");
    for (int i = QMetaType::FirstGuiType; i <= QMetaType::LastGuiType; ++i)
        QTest::newRow(QMetaType::typeName(i)) << i;
}

// Tests how fast a Qt GUI type can be default-constructed by a
// QVariant. The purpose of this benchmark is to measure the overhead
// of creating (and destroying) a QVariant compared to creating the
// type directly.
void tst_QGuiVariant::createGuiType()
{
    QFETCH(int, typeId);
    QBENCHMARK {
        for (int i = 0; i < ITERATION_COUNT; ++i)
            QVariant(typeId, (void *)0);
    }
}

void tst_QGuiVariant::createGuiTypeCopy_data()
{
    createGuiType_data();
}

// Tests how fast a Qt GUI type can be copy-constructed by a
// QVariant. The purpose of this benchmark is to measure the overhead
// of creating (and destroying) a QVariant compared to creating the
// type directly.
void tst_QGuiVariant::createGuiTypeCopy()
{
    QFETCH(int, typeId);
    QVariant other(typeId, (void *)0);
    const void *copy = other.constData();
    QBENCHMARK {
        for (int i = 0; i < ITERATION_COUNT; ++i)
            QVariant(typeId, copy);
    }
}

QTEST_MAIN(tst_QGuiVariant)
#include "tst_qguivariant.moc"
