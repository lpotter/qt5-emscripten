/********************************************************************************
** Form generated from reading UI file 'downloads.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef DOWNLOADS_H
#define DOWNLOADS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include "edittableview.h"

QT_BEGIN_NAMESPACE

class Ui_DownloadDialog
{
public:
    QGridLayout *gridLayout;
    EditTableView *downloadsView;
    QHBoxLayout *horizontalLayout;
    QPushButton *cleanupButton;
    QSpacerItem *spacerItem;
    QLabel *itemCount;
    QSpacerItem *horizontalSpacer;

    void setupUi(QDialog *DownloadDialog)
    {
        if (DownloadDialog->objectName().isEmpty())
            DownloadDialog->setObjectName(QString::fromUtf8("DownloadDialog"));
        DownloadDialog->resize(332, 252);
        gridLayout = new QGridLayout(DownloadDialog);
        gridLayout->setSpacing(0);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        downloadsView = new EditTableView(DownloadDialog);
        downloadsView->setObjectName(QString::fromUtf8("downloadsView"));

        gridLayout->addWidget(downloadsView, 0, 0, 1, 3);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        cleanupButton = new QPushButton(DownloadDialog);
        cleanupButton->setObjectName(QString::fromUtf8("cleanupButton"));
        cleanupButton->setEnabled(false);

        horizontalLayout->addWidget(cleanupButton);

        spacerItem = new QSpacerItem(58, 24, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(spacerItem);


        gridLayout->addLayout(horizontalLayout, 1, 0, 1, 1);

        itemCount = new QLabel(DownloadDialog);
        itemCount->setObjectName(QString::fromUtf8("itemCount"));

        gridLayout->addWidget(itemCount, 1, 1, 1, 1);

        horizontalSpacer = new QSpacerItem(148, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 1, 2, 1, 1);


        retranslateUi(DownloadDialog);

        QMetaObject::connectSlotsByName(DownloadDialog);
    } // setupUi

    void retranslateUi(QDialog *DownloadDialog)
    {
        DownloadDialog->setWindowTitle(QApplication::translate("DownloadDialog", "Downloads", nullptr));
        cleanupButton->setText(QApplication::translate("DownloadDialog", "Clean up", nullptr));
        itemCount->setText(QApplication::translate("DownloadDialog", "0 Items", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DownloadDialog: public Ui_DownloadDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // DOWNLOADS_H
