/********************************************************************************
** Form generated from reading UI file 'controller.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Controller
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QPushButton *decelerate;
    QPushButton *accelerate;
    QPushButton *right;
    QPushButton *left;

    void setupUi(QWidget *Controller)
    {
        if (Controller->objectName().isEmpty())
            Controller->setObjectName(QString::fromUtf8("Controller"));
        Controller->resize(255, 111);
        gridLayout = new QGridLayout(Controller);
#ifndef Q_OS_MAC
        gridLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        gridLayout->setContentsMargins(9, 9, 9, 9);
#endif
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(Controller);
        label->setObjectName(QString::fromUtf8("label"));
        label->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(label, 1, 1, 1, 1);

        decelerate = new QPushButton(Controller);
        decelerate->setObjectName(QString::fromUtf8("decelerate"));

        gridLayout->addWidget(decelerate, 2, 1, 1, 1);

        accelerate = new QPushButton(Controller);
        accelerate->setObjectName(QString::fromUtf8("accelerate"));

        gridLayout->addWidget(accelerate, 0, 1, 1, 1);

        right = new QPushButton(Controller);
        right->setObjectName(QString::fromUtf8("right"));

        gridLayout->addWidget(right, 1, 2, 1, 1);

        left = new QPushButton(Controller);
        left->setObjectName(QString::fromUtf8("left"));

        gridLayout->addWidget(left, 1, 0, 1, 1);


        retranslateUi(Controller);

        QMetaObject::connectSlotsByName(Controller);
    } // setupUi

    void retranslateUi(QWidget *Controller)
    {
        Controller->setWindowTitle(QApplication::translate("Controller", "Controller", nullptr));
        label->setText(QApplication::translate("Controller", "Controller", nullptr));
        decelerate->setText(QApplication::translate("Controller", "Decelerate", nullptr));
        accelerate->setText(QApplication::translate("Controller", "Accelerate", nullptr));
        right->setText(QApplication::translate("Controller", "Right", nullptr));
        left->setText(QApplication::translate("Controller", "Left", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Controller: public Ui_Controller {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CONTROLLER_H
