/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/private/qtestresult_p.h>
#include <QtTest/qtestassert.h>
#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qplaintestlogger_p.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>

#include <QtCore/private/qlogging_p.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef min // windows.h without NOMINMAX is included by the benchmark headers.
#  undef min
#endif
#ifdef max
#  undef max
#endif

#include <QtCore/QByteArray>
#include <QtCore/qmath.h>
#include <QtCore/QLibraryInfo>

#ifdef Q_OS_ANDROID
#  include <android/log.h>
#endif

#ifdef Q_OS_WIN
#  include <qt_windows.h>
#endif

QT_BEGIN_NAMESPACE

namespace QTest {

    static const char *incidentType2String(QAbstractTestLogger::IncidentTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::Pass:
            return "PASS   ";
        case QAbstractTestLogger::XFail:
            return "XFAIL  ";
        case QAbstractTestLogger::Fail:
            return "FAIL!  ";
        case QAbstractTestLogger::XPass:
            return "XPASS  ";
        case QAbstractTestLogger::BlacklistedPass:
            return "BPASS  ";
        case QAbstractTestLogger::BlacklistedFail:
            return "BFAIL  ";
        }
        return "??????";
    }

    static const char *benchmarkResult2String()
    {
        return "RESULT ";
    }

    static const char *messageType2String(QAbstractTestLogger::MessageTypes type)
    {
        switch (type) {
        case QAbstractTestLogger::Skip:
            return "SKIP   ";
        case QAbstractTestLogger::Warn:
            return "WARNING";
        case QAbstractTestLogger::QWarning:
            return "QWARN  ";
        case QAbstractTestLogger::QDebug:
            return "QDEBUG ";
        case QAbstractTestLogger::QInfo:
            return "QINFO  ";
        case QAbstractTestLogger::QSystem:
            return "QSYSTEM";
        case QAbstractTestLogger::QFatal:
            return "QFATAL ";
        case QAbstractTestLogger::Info:
            return "INFO   ";
        }
        return "??????";
    }

    template <typename T>
    static int countSignificantDigits(T num)
    {
        if (num <= 0)
            return 0;

        int digits = 0;
        qreal divisor = 1;

        while (num / divisor >= 1) {
            divisor *= 10;
            ++digits;
        }

        return digits;
    }

    // Pretty-prints a benchmark result using the given number of digits.
    template <typename T> QString formatResult(T number, int significantDigits)
    {
        if (number < T(0))
            return QLatin1String("NAN");
        if (number == T(0))
            return QLatin1String("0");

        QString beforeDecimalPoint = QString::number(qint64(number), 'f', 0);
        QString afterDecimalPoint = QString::number(number, 'f', 20);
        afterDecimalPoint.remove(0, beforeDecimalPoint.count() + 1);

        int beforeUse = qMin(beforeDecimalPoint.count(), significantDigits);
        int beforeRemove = beforeDecimalPoint.count() - beforeUse;

        // Replace insignificant digits before the decimal point with zeros.
        beforeDecimalPoint.chop(beforeRemove);
        for (int i = 0; i < beforeRemove; ++i) {
            beforeDecimalPoint.append(QLatin1Char('0'));
        }

        int afterUse = significantDigits - beforeUse;

        // leading zeroes after the decimal point does not count towards the digit use.
        if (beforeDecimalPoint == QLatin1String("0") && afterDecimalPoint.isEmpty() == false) {
            ++afterUse;

            int i = 0;
            while (i < afterDecimalPoint.count() && afterDecimalPoint.at(i) == QLatin1Char('0')) {
                ++i;
            }

            afterUse += i;
        }

        int afterRemove = afterDecimalPoint.count() - afterUse;
        afterDecimalPoint.chop(afterRemove);

        QChar separator = QLatin1Char(',');
        QChar decimalPoint = QLatin1Char('.');

        // insert thousands separators
        int length = beforeDecimalPoint.length();
        for (int i = beforeDecimalPoint.length() -1; i >= 1; --i) {
            if ((length - i) % 3 == 0)
                beforeDecimalPoint.insert(i, separator);
        }

        QString print;
        print = beforeDecimalPoint;
        if (afterUse > 0)
            print.append(decimalPoint);

        print += afterDecimalPoint;


        return print;
    }

    template <typename T>
    int formatResult(char * buffer, int bufferSize, T number, int significantDigits)
    {
        QString result = formatResult(number, significantDigits);
        int size = result.count();
        qstrncpy(buffer, std::move(result).toLatin1().constData(), bufferSize);
        return size;
    }
}

void QPlainTestLogger::outputMessage(const char *str)
{
#if defined(Q_OS_WIN)
    // Log to system log only if output is not redirected and stderr not preferred
    if (stream == stdout && !QtPrivate::shouldLogToStderr()) {
        OutputDebugStringA(str);
        return;
    }
#elif defined(Q_OS_ANDROID)
    __android_log_write(ANDROID_LOG_INFO, "QTestLib", str);
#endif
    outputString(str);
}

void QPlainTestLogger::printMessage(const char *type, const char *msg, const char *file, int line)
{
    QTEST_ASSERT(type);
    QTEST_ASSERT(msg);

    QTestCharBuffer messagePrefix;

    QTestCharBuffer failureLocation;
    if (file) {
#ifdef Q_OS_WIN
#define FAILURE_LOCATION_STR "\n%s(%d) : failure location"
#else
#define FAILURE_LOCATION_STR "\n   Loc: [%s(%d)]"
#endif
        QTest::qt_asprintf(&failureLocation, FAILURE_LOCATION_STR, file, line);
    }

    const char *msgFiller = msg[0] ? " " : "";
    QTestCharBuffer testIdentifier;
    QTestPrivate::generateTestIdentifier(&testIdentifier);
    QTest::qt_asprintf(&messagePrefix, "%s: %s%s%s%s\n",
                       type, testIdentifier.data(), msgFiller, msg, failureLocation.data());

    // In colored mode, printf above stripped our nonprintable control characters.
    // Put them back.
    memcpy(messagePrefix.data(), type, strlen(type));

    outputMessage(messagePrefix.data());
}

void QPlainTestLogger::printBenchmarkResult(const QBenchmarkResult &result)
{
    const char *bmtag = QTest::benchmarkResult2String();

    char buf1[1024];
    qsnprintf(
        buf1, sizeof(buf1), "%s: %s::%s",
        bmtag,
        QTestResult::currentTestObjectName(),
        result.context.slotName.toLatin1().data());

    char bufTag[1024];
    bufTag[0] = 0;
    QByteArray tag = result.context.tag.toLocal8Bit();
    if (tag.isEmpty() == false) {
        qsnprintf(bufTag, sizeof(bufTag), ":\"%s\"", tag.data());
    }


    char fillFormat[8];
    int fillLength = 5;
    qsnprintf(fillFormat, sizeof(fillFormat), ":\n%%%ds", fillLength);
    char fill[1024];
    qsnprintf(fill, sizeof(fill), fillFormat, "");

    const char * unitText = QTest::benchmarkMetricUnit(result.metric);

    qreal valuePerIteration = qreal(result.value) / qreal(result.iterations);
    char resultBuffer[100] = "";
    QTest::formatResult(resultBuffer, 100, valuePerIteration, QTest::countSignificantDigits(result.value));

    char buf2[1024];
    qsnprintf(buf2, sizeof(buf2), "%s %s", resultBuffer, unitText);

    char buf2_[1024];
    QByteArray iterationText = " per iteration";
    Q_ASSERT(result.iterations > 0);
    qsnprintf(buf2_, sizeof(buf2_), "%s", iterationText.data());

    char buf3[1024];
    Q_ASSERT(result.iterations > 0);
    QTest::formatResult(resultBuffer, 100, result.value, QTest::countSignificantDigits(result.value));
    qsnprintf(buf3, sizeof(buf3), " (total: %s, iterations: %d)", resultBuffer, result.iterations);

    char buf[1024];

    if (result.setByMacro) {
        qsnprintf(buf, sizeof(buf), "%s%s%s%s%s%s\n", buf1, bufTag, fill, buf2, buf2_, buf3);
    } else {
        qsnprintf(buf, sizeof(buf), "%s%s%s%s\n", buf1, bufTag, fill, buf2);
    }

    memcpy(buf, bmtag, strlen(bmtag));
    outputMessage(buf);
}

QPlainTestLogger::QPlainTestLogger(const char *filename)
    : QAbstractTestLogger(filename)
{
}

QPlainTestLogger::~QPlainTestLogger()
{
}

void QPlainTestLogger::startLogging()
{
    QAbstractTestLogger::startLogging();

    char buf[1024];
    if (QTestLog::verboseLevel() < 0) {
        qsnprintf(buf, sizeof(buf), "Testing %s\n", QTestResult::currentTestObjectName());
    } else {
        qsnprintf(buf, sizeof(buf),
                  "********* Start testing of %s *********\n"
                  "Config: Using QtTest library " QTEST_VERSION_STR
                  ", %s\n", QTestResult::currentTestObjectName(), QLibraryInfo::build());
    }
    outputMessage(buf);
}

void QPlainTestLogger::stopLogging()
{
    char buf[1024];
    const int timeMs = qRound(QTestLog::msecsTotalTime());
    if (QTestLog::verboseLevel() < 0) {
        qsnprintf(buf, sizeof(buf), "Totals: %d passed, %d failed, %d skipped, %d blacklisted, %dms\n",
                  QTestLog::passCount(), QTestLog::failCount(),
                  QTestLog::skipCount(), QTestLog::blacklistCount(), timeMs);
    } else {
        qsnprintf(buf, sizeof(buf),
                  "Totals: %d passed, %d failed, %d skipped, %d blacklisted, %dms\n"
                  "********* Finished testing of %s *********\n",
                  QTestLog::passCount(), QTestLog::failCount(),
                  QTestLog::skipCount(), QTestLog::blacklistCount(), timeMs,
                  QTestResult::currentTestObjectName());
    }
    outputMessage(buf);

    QAbstractTestLogger::stopLogging();
}


void QPlainTestLogger::enterTestFunction(const char * /*function*/)
{
    if (QTestLog::verboseLevel() >= 1)
        printMessage(QTest::messageType2String(Info), "entering");
}

void QPlainTestLogger::leaveTestFunction()
{
}

void QPlainTestLogger::addIncident(IncidentTypes type, const char *description,
                                   const char *file, int line)
{
    // suppress PASS and XFAIL in silent mode
    if ((type == QAbstractTestLogger::Pass || type == QAbstractTestLogger::XFail)
        && QTestLog::verboseLevel() < 0)
        return;

    printMessage(QTest::incidentType2String(type), description, file, line);
}

void QPlainTestLogger::addBenchmarkResult(const QBenchmarkResult &result)
{
    // suppress benchmark results in silent mode
    if (QTestLog::verboseLevel() < 0)
        return;

    printBenchmarkResult(result);
}

void QPlainTestLogger::addMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QAbstractTestLogger::addMessage(type, context, message);
}

void QPlainTestLogger::addMessage(MessageTypes type, const QString &message,
                                  const char *file, int line)
{
    // suppress non-fatal messages in silent mode
    if (type != QAbstractTestLogger::QFatal && QTestLog::verboseLevel() < 0)
        return;

    printMessage(QTest::messageType2String(type), qPrintable(message), file, line);
}

QT_END_NAMESPACE
