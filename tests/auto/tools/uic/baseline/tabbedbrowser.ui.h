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
** Form generated from reading UI file 'tabbedbrowser.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TABBEDBROWSER_H
#define TABBEDBROWSER_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TabbedBrowser
{
public:
    QVBoxLayout *vboxLayout;
    QTabWidget *tab;
    QWidget *frontpage;
    QGridLayout *gridLayout;
    QFrame *frameFind;
    QHBoxLayout *hboxLayout;
    QToolButton *toolClose;
    QLineEdit *editFind;
    QToolButton *toolPrevious;
    QToolButton *toolNext;
    QCheckBox *checkCase;
    QCheckBox *checkWholeWords;
    QLabel *labelWrapped;
    QSpacerItem *spacerItem;

    void setupUi(QWidget *TabbedBrowser)
    {
        if (TabbedBrowser->objectName().isEmpty())
            TabbedBrowser->setObjectName(QString::fromUtf8("TabbedBrowser"));
        TabbedBrowser->resize(710, 664);
        vboxLayout = new QVBoxLayout(TabbedBrowser);
        vboxLayout->setSpacing(0);
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        tab = new QTabWidget(TabbedBrowser);
        tab->setObjectName(QString::fromUtf8("tab"));
        frontpage = new QWidget();
        frontpage->setObjectName(QString::fromUtf8("frontpage"));
        gridLayout = new QGridLayout(frontpage);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
        gridLayout->setContentsMargins(8, 8, 8, 8);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        tab->addTab(frontpage, QString());

        vboxLayout->addWidget(tab);

        frameFind = new QFrame(TabbedBrowser);
        frameFind->setObjectName(QString::fromUtf8("frameFind"));
        frameFind->setFrameShape(QFrame::StyledPanel);
        frameFind->setFrameShadow(QFrame::Raised);
        hboxLayout = new QHBoxLayout(frameFind);
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        toolClose = new QToolButton(frameFind);
        toolClose->setObjectName(QString::fromUtf8("toolClose"));
        const QIcon icon = QIcon(QString::fromUtf8(":/qt-project.org/assistant/images/close.png"));
        toolClose->setIcon(icon);
        toolClose->setAutoRaise(true);

        hboxLayout->addWidget(toolClose);

        editFind = new QLineEdit(frameFind);
        editFind->setObjectName(QString::fromUtf8("editFind"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(editFind->sizePolicy().hasHeightForWidth());
        editFind->setSizePolicy(sizePolicy);
        editFind->setMinimumSize(QSize(150, 0));

        hboxLayout->addWidget(editFind);

        toolPrevious = new QToolButton(frameFind);
        toolPrevious->setObjectName(QString::fromUtf8("toolPrevious"));
        const QIcon icon1 = QIcon(QString::fromUtf8(":/qt-project.org/assistant/images/win/previous.png"));
        toolPrevious->setIcon(icon1);
        toolPrevious->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toolPrevious->setAutoRaise(true);

        hboxLayout->addWidget(toolPrevious);

        toolNext = new QToolButton(frameFind);
        toolNext->setObjectName(QString::fromUtf8("toolNext"));
        toolNext->setMinimumSize(QSize(0, 0));
        const QIcon icon2 = QIcon(QString::fromUtf8(":/qt-project.org/assistant/images/win/next.png"));
        toolNext->setIcon(icon2);
        toolNext->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toolNext->setAutoRaise(true);
        toolNext->setArrowType(Qt::NoArrow);

        hboxLayout->addWidget(toolNext);

        checkCase = new QCheckBox(frameFind);
        checkCase->setObjectName(QString::fromUtf8("checkCase"));

        hboxLayout->addWidget(checkCase);

        checkWholeWords = new QCheckBox(frameFind);
        checkWholeWords->setObjectName(QString::fromUtf8("checkWholeWords"));

        hboxLayout->addWidget(checkWholeWords);

        labelWrapped = new QLabel(frameFind);
        labelWrapped->setObjectName(QString::fromUtf8("labelWrapped"));
        labelWrapped->setMinimumSize(QSize(0, 20));
        labelWrapped->setMaximumSize(QSize(105, 20));
        labelWrapped->setTextFormat(Qt::RichText);
        labelWrapped->setScaledContents(true);
        labelWrapped->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        hboxLayout->addWidget(labelWrapped);

        spacerItem = new QSpacerItem(81, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);


        vboxLayout->addWidget(frameFind);


        retranslateUi(TabbedBrowser);

        QMetaObject::connectSlotsByName(TabbedBrowser);
    } // setupUi

    void retranslateUi(QWidget *TabbedBrowser)
    {
        TabbedBrowser->setWindowTitle(QApplication::translate("TabbedBrowser", "TabbedBrowser", nullptr));
        tab->setTabText(tab->indexOf(frontpage), QApplication::translate("TabbedBrowser", "Untitled", nullptr));
        toolClose->setText(QString());
        toolPrevious->setText(QApplication::translate("TabbedBrowser", "Previous", nullptr));
        toolNext->setText(QApplication::translate("TabbedBrowser", "Next", nullptr));
        checkCase->setText(QApplication::translate("TabbedBrowser", "Case Sensitive", nullptr));
        checkWholeWords->setText(QApplication::translate("TabbedBrowser", "Whole words", nullptr));
        labelWrapped->setText(QApplication::translate("TabbedBrowser", "<img src=\":/qt-project.org/assistant/images/wrap.png\">&nbsp;Search wrapped", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TabbedBrowser: public Ui_TabbedBrowser {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TABBEDBROWSER_H
