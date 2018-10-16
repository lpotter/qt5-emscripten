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

#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include <QDialog>
#include <QRasterWindow>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

class PreviewWindow : public QRasterWindow
{
    void paintEvent(QPaintEvent *event);
};

class PreviewWidget : public QWidget
{
    Q_OBJECT

public:
    PreviewWidget(QWidget *parent = 0);

    void setWindowFlags(Qt::WindowFlags flags);

public slots:
    void updateInfo();

protected:
    bool event(QEvent *) override;

private:
    QPlainTextEdit *textEdit;
};

class PreviewDialog : public QDialog
{
    Q_OBJECT

public:
    PreviewDialog(QWidget *parent = 0);

    void setWindowFlags(Qt::WindowFlags flags);

public slots:
    void updateInfo();

protected:
    bool event(QEvent *) override;

private:
    QPlainTextEdit *textEdit;
};

#endif
