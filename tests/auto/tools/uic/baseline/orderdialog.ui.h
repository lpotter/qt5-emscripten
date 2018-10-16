/*
*********************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the autotests of the Qt Toolkit.
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
*********************************************************************
*/

/********************************************************************************
** Form generated from reading UI file 'orderdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ORDERDIALOG_H
#define ORDERDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class Ui_OrderDialog
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QHBoxLayout *hboxLayout;
    QListWidget *pageList;
    QVBoxLayout *vboxLayout1;
    QToolButton *upButton;
    QToolButton *downButton;
    QSpacerItem *spacerItem;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *qdesigner_internal__OrderDialog)
    {
        if (qdesigner_internal__OrderDialog->objectName().isEmpty())
            qdesigner_internal__OrderDialog->setObjectName(QString::fromUtf8("qdesigner_internal__OrderDialog"));
        qdesigner_internal__OrderDialog->resize(467, 310);
        vboxLayout = new QVBoxLayout(qdesigner_internal__OrderDialog);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        groupBox = new QGroupBox(qdesigner_internal__OrderDialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        hboxLayout = new QHBoxLayout(groupBox);
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        hboxLayout->setContentsMargins(9, 9, 9, 9);
        pageList = new QListWidget(groupBox);
        pageList->setObjectName(QString::fromUtf8("pageList"));
        pageList->setMinimumSize(QSize(344, 0));
        pageList->setDragDropMode(QAbstractItemView::InternalMove);
        pageList->setSelectionMode(QAbstractItemView::ContiguousSelection);
        pageList->setMovement(QListView::Snap);

        hboxLayout->addWidget(pageList);

        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setSpacing(6);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        vboxLayout1->setContentsMargins(0, 0, 0, 0);
        upButton = new QToolButton(groupBox);
        upButton->setObjectName(QString::fromUtf8("upButton"));

        vboxLayout1->addWidget(upButton);

        downButton = new QToolButton(groupBox);
        downButton->setObjectName(QString::fromUtf8("downButton"));

        vboxLayout1->addWidget(downButton);

        spacerItem = new QSpacerItem(20, 99, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacerItem);


        hboxLayout->addLayout(vboxLayout1);


        vboxLayout->addWidget(groupBox);

        buttonBox = new QDialogButtonBox(qdesigner_internal__OrderDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok|QDialogButtonBox::Reset);

        vboxLayout->addWidget(buttonBox);


        retranslateUi(qdesigner_internal__OrderDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), qdesigner_internal__OrderDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), qdesigner_internal__OrderDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(qdesigner_internal__OrderDialog);
    } // setupUi

    void retranslateUi(QDialog *qdesigner_internal__OrderDialog)
    {
        qdesigner_internal__OrderDialog->setWindowTitle(QApplication::translate("qdesigner_internal::OrderDialog", "Change Page Order", nullptr));
        groupBox->setTitle(QApplication::translate("qdesigner_internal::OrderDialog", "Page Order", nullptr));
#ifndef QT_NO_TOOLTIP
        upButton->setToolTip(QApplication::translate("qdesigner_internal::OrderDialog", "Move page up", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        downButton->setToolTip(QApplication::translate("qdesigner_internal::OrderDialog", "Move page down", nullptr));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

} // namespace qdesigner_internal

namespace qdesigner_internal {
namespace Ui {
    class OrderDialog: public Ui_OrderDialog {};
} // namespace Ui
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // ORDERDIALOG_H
