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

#include <QtTest/qtestassert.h>

#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qabstracttestlogger_p.h>
#include <QtTest/private/qplaintestlogger_p.h>
#include <QtTest/private/qcsvbenchmarklogger_p.h>
#include <QtTest/private/qxunittestlogger_p.h>
#include <QtTest/private/qxmltestlogger_p.h>
#include <QtTest/private/qteamcitylogger_p.h>
#include <QtTest/private/qtaptestlogger_p.h>
#if defined(HAVE_XCTEST)
#include <QtTest/private/qxctestlogger_p.h>
#endif

#if defined(Q_OS_DARWIN)
#include <QtTest/private/qappletestlogger_p.h>
#endif

#include <QtCore/qatomic.h>
#include <QtCore/qbytearray.h>
#include <QtCore/QElapsedTimer>
#include <QtCore/QVariant>
#if QT_CONFIG(regularexpression)
#include <QtCore/QRegularExpression>
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

static void saveCoverageTool(const char * appname, bool testfailed, bool installedTestCoverage)
{
#ifdef __COVERAGESCANNER__
    if (!installedTestCoverage)
        return;
    // install again to make sure the filename is correct.
    // without this, a plugin or similar may have changed the filename.
    __coveragescanner_install(appname);
    __coveragescanner_teststate(testfailed ? "FAILED" : "PASSED");
    __coveragescanner_save();
    __coveragescanner_testname("");
    __coveragescanner_clear();
    unsetenv("QT_TESTCOCOON_ACTIVE");
#else
    Q_UNUSED(appname);
    Q_UNUSED(testfailed);
    Q_UNUSED(installedTestCoverage);
#endif
}

static QElapsedTimer elapsedFunctionTime;
static QElapsedTimer elapsedTotalTime;

namespace QTest {

    int fails = 0;
    int passes = 0;
    int skips = 0;
    int blacklists = 0;

    struct IgnoreResultList
    {
        inline IgnoreResultList(QtMsgType tp, const QVariant &patternIn)
            : type(tp), pattern(patternIn), next(0) {}

        static inline void clearList(IgnoreResultList *&list)
        {
            while (list) {
                IgnoreResultList *current = list;
                list = list->next;
                delete current;
            }
        }

        static void append(IgnoreResultList *&list, QtMsgType type, const QVariant &patternIn)
        {
            QTest::IgnoreResultList *item = new QTest::IgnoreResultList(type, patternIn);

            if (!list) {
                list = item;
                return;
            }
            IgnoreResultList *last = list;
            for ( ; last->next; last = last->next) ;
            last->next = item;
        }

        static bool stringsMatch(const QString &expected, const QString &actual)
        {
            if (expected == actual)
                return true;

            // ignore an optional whitespace at the end of str
            // (the space was added automatically by ~QDebug() until Qt 5.3,
            //  so autotests still might expect it)
            if (expected.endsWith(QLatin1Char(' ')))
                return actual == expected.leftRef(expected.length() - 1);

            return false;
        }

        inline bool matches(QtMsgType tp, const QString &message) const
        {
            return tp == type
                   && (pattern.type() == QVariant::String ?
                       stringsMatch(pattern.toString(), message) :
#if QT_CONFIG(regularexpression)
                       pattern.toRegularExpression().match(message).hasMatch());
#else
                       false);
#endif
        }

        QtMsgType type;
        QVariant pattern;
        IgnoreResultList *next;
    };

    static IgnoreResultList *ignoreResultList = 0;

    struct LoggerList
    {
        QAbstractTestLogger *logger;
        LoggerList *next;
    };

    class TestLoggers
    {
    public:
        static void addLogger(QAbstractTestLogger *logger)
        {
            LoggerList *l = new LoggerList;
            l->logger = logger;
            l->next = loggers;
            loggers = l;
        }

        static void destroyLoggers()
        {
            while (loggers) {
                LoggerList *l = loggers;
                loggers = loggers->next;
                delete l->logger;
                delete l;
            }
        }

#define FOREACH_LOGGER(operation) \
        LoggerList *l = loggers; \
        while (l) { \
            QAbstractTestLogger *logger = l->logger; \
            Q_UNUSED(logger); \
            operation; \
            l = l->next; \
        }

        static void startLogging()
        {
            FOREACH_LOGGER(logger->startLogging());
        }

        static void stopLogging()
        {
            FOREACH_LOGGER(logger->stopLogging());
        }

        static void enterTestFunction(const char *function)
        {
            FOREACH_LOGGER(logger->enterTestFunction(function));
        }

        static void leaveTestFunction()
        {
            FOREACH_LOGGER(logger->leaveTestFunction());
        }

        static void enterTestData(QTestData *data)
        {
            FOREACH_LOGGER(logger->enterTestData(data));
        }

        static void addIncident(QAbstractTestLogger::IncidentTypes type, const char *description,
                                const char *file = 0, int line = 0)
        {
            FOREACH_LOGGER(logger->addIncident(type, description, file, line));
        }

        static void addBenchmarkResult(const QBenchmarkResult &result)
        {
            FOREACH_LOGGER(logger->addBenchmarkResult(result));
        }

        static void addMessage(QtMsgType type, const QMessageLogContext &context,
                               const QString &message)
        {
            FOREACH_LOGGER(logger->addMessage(type, context, message));
        }

        static void addMessage(QAbstractTestLogger::MessageTypes type, const QString &message,
                               const char *file = 0, int line = 0)
        {
            FOREACH_LOGGER(logger->addMessage(type, message, file, line));
        }

        static void outputString(const char *msg)
        {
            FOREACH_LOGGER(logger->outputString(msg));
        }

        static int loggerCount()
        {
            int count = 0;
            FOREACH_LOGGER(++count);
            return count;
        }

    private:
        static LoggerList *loggers;
    };

#undef FOREACH_LOGGER

    LoggerList *TestLoggers::loggers = 0;
    static bool loggerUsingStdout = false;

    static int verbosity = 0;
    static int maxWarnings = 2002;
    static bool installedTestCoverage = true;

    static QtMessageHandler oldMessageHandler;

    static bool handleIgnoredMessage(QtMsgType type, const QString &message)
    {
        if (!ignoreResultList)
            return false;
        IgnoreResultList *last = 0;
        IgnoreResultList *list = ignoreResultList;
        while (list) {
            if (list->matches(type, message)) {
                // remove the item from the list
                if (last)
                    last->next = list->next;
                else if (list->next)
                    ignoreResultList = list->next;
                else
                    ignoreResultList = 0;

                delete list;
                return true;
            }

            last = list;
            list = list->next;
        }
        return false;
    }

    static void messageHandler(QtMsgType type, const QMessageLogContext & context, const QString &message)
    {
        static QBasicAtomicInt counter = Q_BASIC_ATOMIC_INITIALIZER(QTest::maxWarnings);

        if (QTest::TestLoggers::loggerCount() == 0) {
            // if this goes wrong, something is seriously broken.
            qInstallMessageHandler(oldMessageHandler);
            QTEST_ASSERT(QTest::TestLoggers::loggerCount() != 0);
        }

        if (handleIgnoredMessage(type, message)) {
            // the message is expected, so just swallow it.
            return;
        }

        if (type != QtFatalMsg) {
            if (counter.load() <= 0)
                return;

            if (!counter.deref()) {
                QTest::TestLoggers::addMessage(QAbstractTestLogger::QSystem,
                        QStringLiteral("Maximum amount of warnings exceeded. Use -maxwarnings to override."));
                return;
            }
        }

        QTest::TestLoggers::addMessage(type, context, message);

        if (type == QtFatalMsg) {
             /* Right now, we're inside the custom message handler and we're
             * being qt_message_output in qglobal.cpp. After we return from
             * this function, it will proceed with calling exit() and abort()
             * and hence crash. Therefore, we call these logging functions such
             * that we wrap up nicely, and in particular produce well-formed XML. */
            QTestResult::addFailure("Received a fatal error.", "Unknown file", 0);
            QTestLog::leaveTestFunction();
            QTestLog::stopLogging();
        }
    }
}

void QTestLog::enterTestFunction(const char* function)
{
    elapsedFunctionTime.restart();
    if (printAvailableTags)
        return;

    QTEST_ASSERT(function);

    QTest::TestLoggers::enterTestFunction(function);
}

void QTestLog::enterTestData(QTestData *data)
{
    QTEST_ASSERT(data);
    QTest::TestLoggers::enterTestData(data);
}

int QTestLog::unhandledIgnoreMessages()
{
    int i = 0;
    QTest::IgnoreResultList *list = QTest::ignoreResultList;
    while (list) {
        ++i;
        list = list->next;
    }
    return i;
}

void QTestLog::leaveTestFunction()
{
    if (printAvailableTags)
        return;

    QTest::TestLoggers::leaveTestFunction();
}

void QTestLog::printUnhandledIgnoreMessages()
{
    QString message;
    QTest::IgnoreResultList *list = QTest::ignoreResultList;
    while (list) {
        if (list->pattern.type() == QVariant::String) {
            message = QStringLiteral("Did not receive message: \"") + list->pattern.toString() + QLatin1Char('"');
        } else {
#if QT_CONFIG(regularexpression)
            message = QStringLiteral("Did not receive any message matching: \"") + list->pattern.toRegularExpression().pattern() + QLatin1Char('"');
#endif
        }
        QTest::TestLoggers::addMessage(QAbstractTestLogger::Info, message);

        list = list->next;
    }
}

void QTestLog::clearIgnoreMessages()
{
    QTest::IgnoreResultList::clearList(QTest::ignoreResultList);
}

void QTestLog::addPass(const char *msg)
{
    if (printAvailableTags)
        return;

    QTEST_ASSERT(msg);

    ++QTest::passes;

    QTest::TestLoggers::addIncident(QAbstractTestLogger::Pass, msg);
}

void QTestLog::addFail(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    ++QTest::fails;

    QTest::TestLoggers::addIncident(QAbstractTestLogger::Fail, msg, file, line);
}

void QTestLog::addXFail(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);
    QTEST_ASSERT(file);

    QTest::TestLoggers::addIncident(QAbstractTestLogger::XFail, msg, file, line);
}

void QTestLog::addXPass(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);
    QTEST_ASSERT(file);

    ++QTest::fails;

    QTest::TestLoggers::addIncident(QAbstractTestLogger::XPass, msg, file, line);
}

void QTestLog::addBPass(const char *msg)
{
    QTEST_ASSERT(msg);

    ++QTest::blacklists;

    QTest::TestLoggers::addIncident(QAbstractTestLogger::BlacklistedPass, msg);
}

void QTestLog::addBFail(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);
    QTEST_ASSERT(file);

    ++QTest::blacklists;

    QTest::TestLoggers::addIncident(QAbstractTestLogger::BlacklistedFail, msg, file, line);
}

void QTestLog::addSkip(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);
    QTEST_ASSERT(file);

    ++QTest::skips;

    QTest::TestLoggers::addMessage(QAbstractTestLogger::Skip, QString::fromUtf8(msg), file, line);
}

void QTestLog::addBenchmarkResult(const QBenchmarkResult &result)
{
    QTest::TestLoggers::addBenchmarkResult(result);
}

void QTestLog::startLogging()
{
    elapsedTotalTime.start();
    elapsedFunctionTime.start();
    QTest::TestLoggers::startLogging();
    QTest::oldMessageHandler = qInstallMessageHandler(QTest::messageHandler);
}

void QTestLog::stopLogging()
{
    qInstallMessageHandler(QTest::oldMessageHandler);
    QTest::TestLoggers::stopLogging();
    QTest::TestLoggers::destroyLoggers();
    QTest::loggerUsingStdout = false;
    saveCoverageTool(QTestResult::currentAppName(), failCount() != 0, QTestLog::installedTestCoverage());
}

void QTestLog::addLogger(LogMode mode, const char *filename)
{
    if (filename && strcmp(filename, "-") == 0)
        filename = 0;
    if (!filename)
        QTest::loggerUsingStdout = true;

    QAbstractTestLogger *logger = 0;
    switch (mode) {
    case QTestLog::Plain:
        logger = new QPlainTestLogger(filename);
        break;
    case QTestLog::CSV:
        logger = new QCsvBenchmarkLogger(filename);
        break;
    case QTestLog::XML:
        logger = new QXmlTestLogger(QXmlTestLogger::Complete, filename);
        break;
    case QTestLog::LightXML:
        logger = new QXmlTestLogger(QXmlTestLogger::Light, filename);
        break;
    case QTestLog::XunitXML:
        logger = new QXunitTestLogger(filename);
        break;
    case QTestLog::TeamCity:
        logger = new QTeamCityLogger(filename);
        break;
    case QTestLog::TAP:
        logger = new QTapTestLogger(filename);
        break;
#if defined(HAVE_XCTEST)
    case QTestLog::XCTest:
        logger = new QXcodeTestLogger;
        break;
#endif
    }

#if defined(QT_USE_APPLE_UNIFIED_LOGGING)
    // Logger that also feeds messages to AUL. It needs to wrap the existing
    // logger, as it needs to be able to short circuit the existing logger
    // in case AUL prints to stderr.
    if (QAppleTestLogger::debugLoggingEnabled())
        logger = new QAppleTestLogger(logger);
#endif

    QTEST_ASSERT(logger);
    QTest::TestLoggers::addLogger(logger);
}

int QTestLog::loggerCount()
{
    return QTest::TestLoggers::loggerCount();
}

bool QTestLog::loggerUsingStdout()
{
    return QTest::loggerUsingStdout;
}

void QTestLog::warn(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    if (QTest::TestLoggers::loggerCount() > 0)
        QTest::TestLoggers::addMessage(QAbstractTestLogger::Warn, QString::fromUtf8(msg), file, line);
}

void QTestLog::info(const char *msg, const char *file, int line)
{
    QTEST_ASSERT(msg);

    QTest::TestLoggers::addMessage(QAbstractTestLogger::Info, QString::fromUtf8(msg), file, line);
}

void QTestLog::setVerboseLevel(int level)
{
    QTest::verbosity = level;
}

int QTestLog::verboseLevel()
{
    return QTest::verbosity;
}

void QTestLog::ignoreMessage(QtMsgType type, const char *msg)
{
    QTEST_ASSERT(msg);

    QTest::IgnoreResultList::append(QTest::ignoreResultList, type, QString::fromLocal8Bit(msg));
}

#if QT_CONFIG(regularexpression)
void QTestLog::ignoreMessage(QtMsgType type, const QRegularExpression &expression)
{
    QTEST_ASSERT(expression.isValid());

    QTest::IgnoreResultList::append(QTest::ignoreResultList, type, QVariant(expression));
}
#endif // QT_CONFIG(regularexpression)

void QTestLog::setMaxWarnings(int m)
{
    QTest::maxWarnings = m <= 0 ? INT_MAX : m + 2;
}

bool QTestLog::printAvailableTags = false;

void QTestLog::setPrintAvailableTagsMode()
{
    printAvailableTags = true;
}

int QTestLog::passCount()
{
    return QTest::passes;
}

int QTestLog::failCount()
{
    return QTest::fails;
}

int QTestLog::skipCount()
{
    return QTest::skips;
}

int QTestLog::blacklistCount()
{
    return QTest::blacklists;
}

int QTestLog::totalCount()
{
    return passCount() + failCount() + skipCount() + blacklistCount();
}

void QTestLog::resetCounters()
{
    QTest::passes = 0;
    QTest::fails = 0;
    QTest::skips = 0;
}

void QTestLog::setInstalledTestCoverage(bool installed)
{
    QTest::installedTestCoverage = installed;
}

bool QTestLog::installedTestCoverage()
{
    return QTest::installedTestCoverage;
}

qint64 QTestLog::nsecsTotalTime()
{
    return elapsedTotalTime.nsecsElapsed();
}

qint64 QTestLog::nsecsFunctionTime()
{
    return elapsedFunctionTime.nsecsElapsed();
}

QT_END_NAMESPACE
