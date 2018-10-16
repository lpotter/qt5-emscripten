/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef OPTIONSWIDGET_H
#define OPTIONSWIDGET_H

#include <QtGui>
#include <QtWidgets>

class OptionsWidget : public QWidget
{
Q_OBJECT
public:
    OptionsWidget()
        :QWidget()
    {
        QVBoxLayout *m_layout = new QVBoxLayout;

        m_refresh = new QPushButton(this);
        m_refresh->setText(QLatin1String("Refresh"));
        m_layout->addWidget(m_refresh);
        connect(m_refresh, SIGNAL(clicked()), SIGNAL(refreshClicked()));

        m_hideInvisibleItems = new QCheckBox(this);
        m_layout->addWidget(m_hideInvisibleItems);
        m_hideInvisibleItems->setText("Hide Invisible Items");
        m_hideInvisibleItems->setChecked(true);
        connect(m_hideInvisibleItems, SIGNAL(toggled(bool)), SIGNAL(optionsChanged()));

        m_hideOffscreenItems = new QCheckBox(this);
        m_layout->addWidget(m_hideOffscreenItems);
        m_hideOffscreenItems->setText("Hide Offscreen Items");
        m_hideOffscreenItems->setChecked(true);
        connect(m_hideOffscreenItems, SIGNAL(toggled(bool)), SIGNAL(optionsChanged()));

        m_hidePaneItems = new QCheckBox(this);
        m_layout->addWidget(m_hidePaneItems);
        m_hidePaneItems->setText("Hide Items with the Pane role");
        m_hidePaneItems->setChecked(true);
        connect(m_hidePaneItems, SIGNAL(toggled(bool)), SIGNAL(optionsChanged()));

        m_hideNullObjectItems = new QCheckBox(this);
        m_layout->addWidget(m_hideNullObjectItems);
        m_hideNullObjectItems->setText("Hide Items with a null QObject pointer");
        m_hideNullObjectItems->setChecked(true);
        connect(m_hideNullObjectItems, SIGNAL(toggled(bool)), SIGNAL(optionsChanged()));

        m_hideNullRectItems = new QCheckBox(this);
        m_layout->addWidget(m_hideNullRectItems);
        m_hideNullRectItems->setText("Hide Items with a null rect");
        m_hideNullRectItems->setChecked(true);
        connect(m_hideNullRectItems, SIGNAL(toggled(bool)), SIGNAL(optionsChanged()));

        m_enableTextToSpeach = new QCheckBox(this);
        m_layout->addWidget(m_enableTextToSpeach);
        m_enableTextToSpeach->setText("Enable Text To Speech");
        m_enableTextToSpeach->setChecked(false);
        connect(m_enableTextToSpeach, SIGNAL(toggled(bool)), SIGNAL(optionsChanged()));


        m_scale = new QSlider(Qt::Horizontal);
//        m_layout->addWidget(m_scale);
        m_scale->setRange(5, 30);
        m_scale->setValue(1);
        connect(m_scale, SIGNAL(valueChanged(int)), SIGNAL(scaleChanged(int)));

        this->setLayout(m_layout);
    }

    bool hideInvisibleItems() { return m_hideInvisibleItems->isChecked(); }
    bool hideOffscreenItems() { return m_hideOffscreenItems->isChecked(); }
    bool hidePaneItems() { return m_hidePaneItems->isChecked(); }
    bool hideNullObjectItems() { return m_hideNullObjectItems->isChecked(); }
    bool hideNullRectItems() { return m_hideNullRectItems->isChecked(); }
    bool enableTextToSpeach() { return m_enableTextToSpeach->isChecked(); }
signals:
    void optionsChanged();
    void refreshClicked();
    void scaleChanged(int);

private:
    QVBoxLayout *m_layout;

    QPushButton *m_refresh;
    QCheckBox *m_hideInvisibleItems;
    QCheckBox *m_hideOffscreenItems;
    QCheckBox *m_hidePaneItems;
    QCheckBox *m_hideNullObjectItems;
    QCheckBox *m_hideNullRectItems;
    QCheckBox *m_enableTextToSpeach;
    QSlider *m_scale;
};


#endif // OPTIONSWIDGET_H
