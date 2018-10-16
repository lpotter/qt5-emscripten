/********************************************************************************
** Form generated from reading UI file 'gridpanel.ui'
**
** Created by: Qt User Interface Compiler version 5.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef GRIDPANEL_H
#define GRIDPANEL_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class Ui_GridPanel
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *m_gridGroupBox;
    QGridLayout *gridLayout;
    QCheckBox *m_visibleCheckBox;
    QLabel *label;
    QSpinBox *m_deltaXSpinBox;
    QCheckBox *m_snapXCheckBox;
    QHBoxLayout *hboxLayout;
    QPushButton *m_resetButton;
    QSpacerItem *spacerItem;
    QLabel *label_2;
    QSpinBox *m_deltaYSpinBox;
    QCheckBox *m_snapYCheckBox;

    void setupUi(QWidget *qdesigner_internal__GridPanel)
    {
        if (qdesigner_internal__GridPanel->objectName().isEmpty())
            qdesigner_internal__GridPanel->setObjectName(QString::fromUtf8("qdesigner_internal__GridPanel"));
        qdesigner_internal__GridPanel->resize(393, 110);
        vboxLayout = new QVBoxLayout(qdesigner_internal__GridPanel);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        m_gridGroupBox = new QGroupBox(qdesigner_internal__GridPanel);
        m_gridGroupBox->setObjectName(QString::fromUtf8("m_gridGroupBox"));
        gridLayout = new QGridLayout(m_gridGroupBox);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        m_visibleCheckBox = new QCheckBox(m_gridGroupBox);
        m_visibleCheckBox->setObjectName(QString::fromUtf8("m_visibleCheckBox"));
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_visibleCheckBox->sizePolicy().hasHeightForWidth());
        m_visibleCheckBox->setSizePolicy(sizePolicy);

        gridLayout->addWidget(m_visibleCheckBox, 0, 0, 1, 1);

        label = new QLabel(m_gridGroupBox);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 1, 1, 1);

        m_deltaXSpinBox = new QSpinBox(m_gridGroupBox);
        m_deltaXSpinBox->setObjectName(QString::fromUtf8("m_deltaXSpinBox"));
        m_deltaXSpinBox->setMinimum(2);
        m_deltaXSpinBox->setMaximum(100);

        gridLayout->addWidget(m_deltaXSpinBox, 0, 2, 1, 1);

        m_snapXCheckBox = new QCheckBox(m_gridGroupBox);
        m_snapXCheckBox->setObjectName(QString::fromUtf8("m_snapXCheckBox"));
        sizePolicy.setHeightForWidth(m_snapXCheckBox->sizePolicy().hasHeightForWidth());
        m_snapXCheckBox->setSizePolicy(sizePolicy);

        gridLayout->addWidget(m_snapXCheckBox, 0, 3, 1, 1);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        m_resetButton = new QPushButton(m_gridGroupBox);
        m_resetButton->setObjectName(QString::fromUtf8("m_resetButton"));

        hboxLayout->addWidget(m_resetButton);

        spacerItem = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);


        gridLayout->addLayout(hboxLayout, 1, 0, 1, 1);

        label_2 = new QLabel(m_gridGroupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 1, 1, 1, 1);

        m_deltaYSpinBox = new QSpinBox(m_gridGroupBox);
        m_deltaYSpinBox->setObjectName(QString::fromUtf8("m_deltaYSpinBox"));
        m_deltaYSpinBox->setMinimum(2);
        m_deltaYSpinBox->setMaximum(100);

        gridLayout->addWidget(m_deltaYSpinBox, 1, 2, 1, 1);

        m_snapYCheckBox = new QCheckBox(m_gridGroupBox);
        m_snapYCheckBox->setObjectName(QString::fromUtf8("m_snapYCheckBox"));
        sizePolicy.setHeightForWidth(m_snapYCheckBox->sizePolicy().hasHeightForWidth());
        m_snapYCheckBox->setSizePolicy(sizePolicy);

        gridLayout->addWidget(m_snapYCheckBox, 1, 3, 1, 1);


        vboxLayout->addWidget(m_gridGroupBox);

#ifndef QT_NO_SHORTCUT
        label->setBuddy(m_deltaXSpinBox);
        label_2->setBuddy(m_deltaYSpinBox);
#endif // QT_NO_SHORTCUT

        retranslateUi(qdesigner_internal__GridPanel);

        QMetaObject::connectSlotsByName(qdesigner_internal__GridPanel);
    } // setupUi

    void retranslateUi(QWidget *qdesigner_internal__GridPanel)
    {
        qdesigner_internal__GridPanel->setWindowTitle(QApplication::translate("qdesigner_internal::GridPanel", "Form", nullptr));
        m_gridGroupBox->setTitle(QApplication::translate("qdesigner_internal::GridPanel", "Grid", nullptr));
        m_visibleCheckBox->setText(QApplication::translate("qdesigner_internal::GridPanel", "Visible", nullptr));
        label->setText(QApplication::translate("qdesigner_internal::GridPanel", "Grid &X", nullptr));
        m_snapXCheckBox->setText(QApplication::translate("qdesigner_internal::GridPanel", "Snap", nullptr));
        m_resetButton->setText(QApplication::translate("qdesigner_internal::GridPanel", "Reset", nullptr));
        label_2->setText(QApplication::translate("qdesigner_internal::GridPanel", "Grid &Y", nullptr));
        m_snapYCheckBox->setText(QApplication::translate("qdesigner_internal::GridPanel", "Snap", nullptr));
    } // retranslateUi

};

} // namespace qdesigner_internal

namespace qdesigner_internal {
namespace Ui {
    class GridPanel: public Ui_GridPanel {};
} // namespace Ui
} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // GRIDPANEL_H
