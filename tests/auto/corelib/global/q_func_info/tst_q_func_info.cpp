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


#include <QString>
#include <QtTest/QtTest>
#include <QtDebug>

class tst_q_func_info : public QObject
{
    Q_OBJECT

private slots:
    void callFunctions() const;
    void isOfTypeConstChar() const;
    void availableWithoutDebug() const;

private:

    static void staticMember();
    void regularMember() const;
    void memberWithArguments(const QString &string, int value, const int value2) const;
};

static void staticRegularFunction()
{
    qDebug() << Q_FUNC_INFO;
}

void regularFunction()
{
    qDebug() << Q_FUNC_INFO;
}

template<typename T>
void templateFunction()
{
    qDebug() << Q_FUNC_INFO;
}

template<typename T, const int value>
void valueTemplateFunction()
{
    qDebug() << Q_FUNC_INFO;
}

void tst_q_func_info::staticMember()
{
    qDebug() << Q_FUNC_INFO;
}

void tst_q_func_info::regularMember() const
{
    qDebug() << Q_FUNC_INFO;
}

void tst_q_func_info::memberWithArguments(const QString &, int, const int) const
{
    qDebug() << Q_FUNC_INFO;
}

/*! \internal
    We don't do much here. We call different kinds of
    functions to make sure we don't crash anything or that valgrind
    is unhappy.
 */
void tst_q_func_info::callFunctions() const
{
    staticRegularFunction();
    regularFunction();
    templateFunction<char>();
    valueTemplateFunction<int, 3>();

    staticMember();
    regularMember();
    memberWithArguments(QString(), 3, 4);
}

void tst_q_func_info::isOfTypeConstChar() const
{
#ifndef QT_NO_DEBUG
    QString::fromLatin1(Q_FUNC_INFO);
#endif
}

/* \internal
    Ensure that the macro is available even though QT_NO_DEBUG
    is defined.  If QT_NO_DEBUG is not defined, we define it
    just for this function then undef it again afterwards.
 */
void tst_q_func_info::availableWithoutDebug() const
{
#ifndef QT_NO_DEBUG
#   define UNDEF_NO_DEBUG
#   define QT_NO_DEBUG
#endif
    QVERIFY(!QString::fromLatin1(Q_FUNC_INFO).isEmpty());
#ifdef UNDEF_NO_DEBUG
#   undef QT_NO_DEBUG
#   undef UNDEF_NO_DEBUG
#endif
}

QTEST_MAIN(tst_q_func_info)

#include "tst_q_func_info.moc"
