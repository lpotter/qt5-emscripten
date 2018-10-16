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
** Form generated from reading UI file 'phrasebookbox.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PHRASEBOOKBOX_H
#define PHRASEBOOKBOX_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_PhraseBookBox
{
public:
    QHBoxLayout *unnamed;
    QVBoxLayout *inputsLayout;
    QGridLayout *gridLayout;
    QLabel *target;
    QLineEdit *targetLed;
    QLabel *source;
    QLineEdit *definitionLed;
    QLineEdit *sourceLed;
    QLabel *definition;
    QTreeView *phraseList;
    QVBoxLayout *buttonLayout;
    QPushButton *newBut;
    QPushButton *removeBut;
    QPushButton *saveBut;
    QPushButton *closeBut;
    QSpacerItem *spacer1;

    void setupUi(QDialog *PhraseBookBox)
    {
        if (PhraseBookBox->objectName().isEmpty())
            PhraseBookBox->setObjectName(QString::fromUtf8("PhraseBookBox"));
        PhraseBookBox->resize(596, 454);
        unnamed = new QHBoxLayout(PhraseBookBox);
        unnamed->setSpacing(6);
        unnamed->setContentsMargins(11, 11, 11, 11);
        unnamed->setObjectName(QString::fromUtf8("unnamed"));
        inputsLayout = new QVBoxLayout();
        inputsLayout->setSpacing(6);
        inputsLayout->setObjectName(QString::fromUtf8("inputsLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        target = new QLabel(PhraseBookBox);
        target->setObjectName(QString::fromUtf8("target"));

        gridLayout->addWidget(target, 1, 0, 1, 1);

        targetLed = new QLineEdit(PhraseBookBox);
        targetLed->setObjectName(QString::fromUtf8("targetLed"));

        gridLayout->addWidget(targetLed, 1, 1, 1, 1);

        source = new QLabel(PhraseBookBox);
        source->setObjectName(QString::fromUtf8("source"));

        gridLayout->addWidget(source, 0, 0, 1, 1);

        definitionLed = new QLineEdit(PhraseBookBox);
        definitionLed->setObjectName(QString::fromUtf8("definitionLed"));

        gridLayout->addWidget(definitionLed, 2, 1, 1, 1);

        sourceLed = new QLineEdit(PhraseBookBox);
        sourceLed->setObjectName(QString::fromUtf8("sourceLed"));

        gridLayout->addWidget(sourceLed, 0, 1, 1, 1);

        definition = new QLabel(PhraseBookBox);
        definition->setObjectName(QString::fromUtf8("definition"));

        gridLayout->addWidget(definition, 2, 0, 1, 1);


        inputsLayout->addLayout(gridLayout);

        phraseList = new QTreeView(PhraseBookBox);
        phraseList->setObjectName(QString::fromUtf8("phraseList"));
        phraseList->setRootIsDecorated(false);
        phraseList->setUniformRowHeights(true);
        phraseList->setItemsExpandable(false);
        phraseList->setSortingEnabled(true);
        phraseList->setExpandsOnDoubleClick(false);

        inputsLayout->addWidget(phraseList);


        unnamed->addLayout(inputsLayout);

        buttonLayout = new QVBoxLayout();
        buttonLayout->setSpacing(6);
        buttonLayout->setObjectName(QString::fromUtf8("buttonLayout"));
        newBut = new QPushButton(PhraseBookBox);
        newBut->setObjectName(QString::fromUtf8("newBut"));

        buttonLayout->addWidget(newBut);

        removeBut = new QPushButton(PhraseBookBox);
        removeBut->setObjectName(QString::fromUtf8("removeBut"));

        buttonLayout->addWidget(removeBut);

        saveBut = new QPushButton(PhraseBookBox);
        saveBut->setObjectName(QString::fromUtf8("saveBut"));

        buttonLayout->addWidget(saveBut);

        closeBut = new QPushButton(PhraseBookBox);
        closeBut->setObjectName(QString::fromUtf8("closeBut"));

        buttonLayout->addWidget(closeBut);

        spacer1 = new QSpacerItem(20, 51, QSizePolicy::Minimum, QSizePolicy::Expanding);

        buttonLayout->addItem(spacer1);


        unnamed->addLayout(buttonLayout);

#ifndef QT_NO_SHORTCUT
        target->setBuddy(targetLed);
        source->setBuddy(sourceLed);
        definition->setBuddy(definitionLed);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(sourceLed, targetLed);
        QWidget::setTabOrder(targetLed, definitionLed);
        QWidget::setTabOrder(definitionLed, newBut);
        QWidget::setTabOrder(newBut, removeBut);
        QWidget::setTabOrder(removeBut, saveBut);
        QWidget::setTabOrder(saveBut, closeBut);

        retranslateUi(PhraseBookBox);

        QMetaObject::connectSlotsByName(PhraseBookBox);
    } // setupUi

    void retranslateUi(QDialog *PhraseBookBox)
    {
        PhraseBookBox->setWindowTitle(QApplication::translate("PhraseBookBox", "Edit Phrase Book", nullptr));
#ifndef QT_NO_WHATSTHIS
        PhraseBookBox->setWhatsThis(QApplication::translate("PhraseBookBox", "This window allows you to add, modify, or delete phrases in a phrase book.", nullptr));
#endif // QT_NO_WHATSTHIS
        target->setText(QApplication::translate("PhraseBookBox", "&Translation:", nullptr));
#ifndef QT_NO_WHATSTHIS
        targetLed->setWhatsThis(QApplication::translate("PhraseBookBox", "This is the phrase in the target language corresponding to the source phrase.", nullptr));
#endif // QT_NO_WHATSTHIS
        source->setText(QApplication::translate("PhraseBookBox", "S&ource phrase:", nullptr));
#ifndef QT_NO_WHATSTHIS
        definitionLed->setWhatsThis(QApplication::translate("PhraseBookBox", "This is a definition for the source phrase.", nullptr));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_WHATSTHIS
        sourceLed->setWhatsThis(QApplication::translate("PhraseBookBox", "This is the phrase in the source language.", nullptr));
#endif // QT_NO_WHATSTHIS
        definition->setText(QApplication::translate("PhraseBookBox", "&Definition:", nullptr));
#ifndef QT_NO_WHATSTHIS
        newBut->setWhatsThis(QApplication::translate("PhraseBookBox", "Click here to add the phrase to the phrase book.", nullptr));
#endif // QT_NO_WHATSTHIS
        newBut->setText(QApplication::translate("PhraseBookBox", "&New Phrase", nullptr));
#ifndef QT_NO_WHATSTHIS
        removeBut->setWhatsThis(QApplication::translate("PhraseBookBox", "Click here to remove the phrase from the phrase book.", nullptr));
#endif // QT_NO_WHATSTHIS
        removeBut->setText(QApplication::translate("PhraseBookBox", "&Remove Phrase", nullptr));
#ifndef QT_NO_WHATSTHIS
        saveBut->setWhatsThis(QApplication::translate("PhraseBookBox", "Click here to save the changes made.", nullptr));
#endif // QT_NO_WHATSTHIS
        saveBut->setText(QApplication::translate("PhraseBookBox", "&Save", nullptr));
#ifndef QT_NO_WHATSTHIS
        closeBut->setWhatsThis(QApplication::translate("PhraseBookBox", "Click here to close this window.", nullptr));
#endif // QT_NO_WHATSTHIS
        closeBut->setText(QApplication::translate("PhraseBookBox", "Close", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PhraseBookBox: public Ui_PhraseBookBox {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PHRASEBOOKBOX_H
