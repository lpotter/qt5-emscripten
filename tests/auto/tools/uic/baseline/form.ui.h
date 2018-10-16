/********************************************************************************
** Form generated from reading UI file 'form.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef FORM_H
#define FORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTimeEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "worldtimeclock.h"

QT_BEGIN_NAMESPACE

class Ui_WorldTimeForm
{
public:
    QHBoxLayout *hboxLayout;
    WorldTimeClock *worldTimeClock;
    QVBoxLayout *vboxLayout;
    QSpacerItem *spacerItem;
    QHBoxLayout *hboxLayout1;
    QLabel *label;
    QTimeEdit *timeEdit;
    QHBoxLayout *hboxLayout2;
    QLabel *label_2;
    QSpinBox *spinBox;
    QSpacerItem *spacerItem1;

    void setupUi(QWidget *WorldTimeForm)
    {
        if (WorldTimeForm->objectName().isEmpty())
            WorldTimeForm->setObjectName(QString::fromUtf8("WorldTimeForm"));
        WorldTimeForm->resize(400, 300);
        hboxLayout = new QHBoxLayout(WorldTimeForm);
#ifndef Q_OS_MAC
        hboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        worldTimeClock = new WorldTimeClock(WorldTimeForm);
        worldTimeClock->setObjectName(QString::fromUtf8("worldTimeClock"));

        hboxLayout->addWidget(worldTimeClock);

        vboxLayout = new QVBoxLayout();
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
        vboxLayout->setContentsMargins(1, 1, 1, 1);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem);

        hboxLayout1 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout1->setSpacing(6);
#endif
        hboxLayout1->setContentsMargins(1, 1, 1, 1);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        label = new QLabel(WorldTimeForm);
        label->setObjectName(QString::fromUtf8("label"));

        hboxLayout1->addWidget(label);

        timeEdit = new QTimeEdit(WorldTimeForm);
        timeEdit->setObjectName(QString::fromUtf8("timeEdit"));
        timeEdit->setReadOnly(true);

        hboxLayout1->addWidget(timeEdit);


        vboxLayout->addLayout(hboxLayout1);

        hboxLayout2 = new QHBoxLayout();
#ifndef Q_OS_MAC
        hboxLayout2->setSpacing(6);
#endif
        hboxLayout2->setContentsMargins(1, 1, 1, 1);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        label_2 = new QLabel(WorldTimeForm);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        hboxLayout2->addWidget(label_2);

        spinBox = new QSpinBox(WorldTimeForm);
        spinBox->setObjectName(QString::fromUtf8("spinBox"));
        spinBox->setMaximum(12);
        spinBox->setMinimum(-12);

        hboxLayout2->addWidget(spinBox);


        vboxLayout->addLayout(hboxLayout2);

        spacerItem1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem1);


        hboxLayout->addLayout(vboxLayout);


        retranslateUi(WorldTimeForm);
        QObject::connect(spinBox, SIGNAL(valueChanged(int)), worldTimeClock, SLOT(setTimeZone(int)));
        QObject::connect(worldTimeClock, SIGNAL(updated(QTime)), timeEdit, SLOT(setTime(QTime)));

        QMetaObject::connectSlotsByName(WorldTimeForm);
    } // setupUi

    void retranslateUi(QWidget *WorldTimeForm)
    {
        WorldTimeForm->setWindowTitle(QApplication::translate("WorldTimeForm", "World Time Clock", nullptr));
        label->setText(QApplication::translate("WorldTimeForm", "Current time:", nullptr));
        label_2->setText(QApplication::translate("WorldTimeForm", "Set time zone:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class WorldTimeForm: public Ui_WorldTimeForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // FORM_H
