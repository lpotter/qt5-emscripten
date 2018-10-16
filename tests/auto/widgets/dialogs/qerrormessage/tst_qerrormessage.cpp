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
#include <QtTest/QtTest>
#include <QErrorMessage>
#include <QDebug>
#include <QCheckBox>

class tst_QErrorMessage : public QObject
{
    Q_OBJECT

private slots:
    void dontShowAgain();
    void dontShowCategoryAgain();

};

void tst_QErrorMessage::dontShowAgain()
{
    QString plainString = QLatin1String("foo");
    QString htmlString = QLatin1String("foo<br>bar");
    QCheckBox *checkBox = 0;

    QErrorMessage errorMessageDialog(0);

    // show an error with plain string
    errorMessageDialog.showMessage(plainString);
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    errorMessageDialog.close();

    errorMessageDialog.showMessage(plainString);
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(false);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(plainString);
    QVERIFY(!errorMessageDialog.isVisible());

    // show an error with an html string
    errorMessageDialog.showMessage(htmlString);
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(!checkBox->isChecked());
    checkBox->setChecked(true);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString);
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(false);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString);
    QVERIFY(!errorMessageDialog.isVisible());
}

void tst_QErrorMessage::dontShowCategoryAgain()
{
    QString htmlString = QLatin1String("foo<br>bar");
    QString htmlString2 = QLatin1String("foo2<br>bar2");
    QCheckBox *checkBox = 0;

    QErrorMessage errorMessageDialog(0);

    errorMessageDialog.showMessage(htmlString,"Cat 1");
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(true);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString,"Cat 1");
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(true);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString2,"Cat 1");
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(true);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString,"Cat 1");
    QVERIFY(errorMessageDialog.isVisible());
    checkBox = errorMessageDialog.findChild<QCheckBox*>();
    QVERIFY(checkBox);
    QVERIFY(checkBox->isChecked());
    checkBox->setChecked(false);
    errorMessageDialog.close();

    errorMessageDialog.showMessage(htmlString2,"Cat 1");
    QVERIFY(!errorMessageDialog.isVisible());

    errorMessageDialog.showMessage(htmlString,"Cat 1");
    QVERIFY(!errorMessageDialog.isVisible());

    errorMessageDialog.showMessage(htmlString);
    QVERIFY(errorMessageDialog.isVisible());

    errorMessageDialog.showMessage(htmlString,"Cat 2");
    QVERIFY(errorMessageDialog.isVisible());
}

QTEST_MAIN(tst_QErrorMessage)
#include "tst_qerrormessage.moc"
