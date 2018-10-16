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


#include <QtTest/QtTest>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>
#include <QtGui/QImage>
#include <QtGui/QColor>
#include "../../../shared/platformclipboard.h"

class tst_QClipboard : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
#if QT_CONFIG(clipboard)
    void init();
#if defined(Q_OS_WIN) || defined(Q_OS_MAC) || defined(Q_OS_QNX)
    void copy_exit_paste();
    void copyImage();
#endif
    void capabilityFunctions();
    void modes();
    void testSignals();
    void setMimeData();
    void clearBeforeSetText();
#endif
};

void tst_QClipboard::initTestCase()
{
#if !QT_CONFIG(clipboard)
    QSKIP("This test requires clipboard support");
#endif
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive))
        QSKIP("Wayland: Manipulating the clipboard requires real input events. Can't auto test.");
}

#if QT_CONFIG(clipboard)
void tst_QClipboard::init()
{
#if QT_CONFIG(process)
    const QString testdataDir = QFileInfo(QFINDTESTDATA("copier")).absolutePath();
    QVERIFY2(QDir::setCurrent(testdataDir), qPrintable("Could not chdir to " + testdataDir));
#endif
}

Q_DECLARE_METATYPE(QClipboard::Mode)

/*
    Tests that the capability functions are implemented on all
    platforms.
*/
void tst_QClipboard::capabilityFunctions()
{
    QClipboard * const clipboard =  QGuiApplication::clipboard();

    clipboard->supportsSelection();
    clipboard->supportsFindBuffer();
    clipboard->ownsSelection();
    clipboard->ownsClipboard();
    clipboard->ownsFindBuffer();
}

/*
    Test that text inserted into the clipboard in different modes is
    kept separate.
*/
void tst_QClipboard::modes()
{
    QClipboard * const clipboard =  QGuiApplication::clipboard();

    if (!PlatformClipboard::isAvailable())
        QSKIP("Native clipboard not working in this setup");

    const QString defaultMode = "default mode text;";
    clipboard->setText(defaultMode);
    QCOMPARE(clipboard->text(), defaultMode);

    if (clipboard->supportsSelection()) {
        const QString selectionMode = "selection mode text";
        clipboard->setText(selectionMode, QClipboard::Selection);
        QCOMPARE(clipboard->text(QClipboard::Selection), selectionMode);
        QCOMPARE(clipboard->text(), defaultMode);
    }

    if (clipboard->supportsFindBuffer()) {
        const QString searchMode = "find mode text";
        clipboard->setText(searchMode, QClipboard::FindBuffer);
        QCOMPARE(clipboard->text(QClipboard::FindBuffer), searchMode);
        QCOMPARE(clipboard->text(), defaultMode);
    }
}

// A predicate to be used with a QSignalSpy / QTRY_VERIFY to ensure all delayed
// notifications are eaten. It waits at least one cycle and returns true when
// no new signals arrive.
class EatSignalSpyNotificationsPredicate
{
public:
    explicit EatSignalSpyNotificationsPredicate(QSignalSpy &spy) : m_spy(spy) { reset(); }

    operator bool() const
    {
        if (m_timer.elapsed() && !m_spy.count())
            return true;
        m_spy.clear();
        return false;
    }

    inline void reset() { m_timer.start(); }

private:
    QSignalSpy &m_spy;
    QElapsedTimer m_timer;
};

/*
    Test that the appropriate signals are emitted when the clipboard
    contents is changed by calling the qt functions.
*/
void tst_QClipboard::testSignals()
{
    qRegisterMetaType<QClipboard::Mode>("QClipboard::Mode");

    if (!PlatformClipboard::isAvailable())
        QSKIP("Native clipboard not working in this setup");

    QClipboard * const clipboard =  QGuiApplication::clipboard();

    QSignalSpy changedSpy(clipboard, SIGNAL(changed(QClipboard::Mode)));
    QSignalSpy dataChangedSpy(clipboard, SIGNAL(dataChanged()));
    // Clipboard notifications are asynchronous with the new AddClipboardFormatListener
    // in Windows Vista (5.4). Eat away all signals to ensure they don't interfere
    // with the QTRY_COMPARE below.
    EatSignalSpyNotificationsPredicate noLeftOverDataChanges(dataChangedSpy);
    EatSignalSpyNotificationsPredicate noLeftOverChanges(changedSpy);
    QTRY_VERIFY(noLeftOverChanges && noLeftOverDataChanges);

    QSignalSpy searchChangedSpy(clipboard, SIGNAL(findBufferChanged()));
    QSignalSpy selectionChangedSpy(clipboard, SIGNAL(selectionChanged()));

    const QString text = "clipboard text;";

    // Test the default mode signal.
    clipboard->setText(text);
    QTRY_COMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(searchChangedSpy.count(), 0);
    QCOMPARE(selectionChangedSpy.count(), 0);
    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.at(0).count(), 1);
    QCOMPARE(qvariant_cast<QClipboard::Mode>(changedSpy.at(0).at(0)), QClipboard::Clipboard);

    changedSpy.clear();

    // Test the selection mode signal.
    if (clipboard->supportsSelection()) {
        clipboard->setText(text, QClipboard::Selection);
        QCOMPARE(selectionChangedSpy.count(), 1);
        QCOMPARE(changedSpy.count(), 1);
        QCOMPARE(changedSpy.at(0).count(), 1);
        QCOMPARE(qvariant_cast<QClipboard::Mode>(changedSpy.at(0).at(0)), QClipboard::Selection);
    } else {
        QCOMPARE(selectionChangedSpy.count(), 0);
    }
    QCOMPARE(dataChangedSpy.count(), 1);
    QCOMPARE(searchChangedSpy.count(), 0);

    changedSpy.clear();

    // Test the search mode signal.
    if (clipboard->supportsFindBuffer()) {
        clipboard->setText(text, QClipboard::FindBuffer);
        QCOMPARE(searchChangedSpy.count(), 1);
        QCOMPARE(changedSpy.count(), 1);
        QCOMPARE(changedSpy.at(0).count(), 1);
        QCOMPARE(qvariant_cast<QClipboard::Mode>(changedSpy.at(0).at(0)), QClipboard::FindBuffer);
    } else {
        QCOMPARE(searchChangedSpy.count(), 0);
    }
    QCOMPARE(dataChangedSpy.count(), 1);
}

#if defined(Q_OS_WIN) || defined(Q_OS_MAC) || defined(Q_OS_QNX)
static bool runHelper(const QString &program, const QStringList &arguments, QByteArray *errorMessage)
{
#if QT_CONFIG(process)
    QProcess process;
    process.setReadChannelMode(QProcess::ForwardedChannels);
    process.start(program, arguments);
    if (!process.waitForStarted()) {
        *errorMessage = "Unable to start '" + program.toLocal8Bit() + " ': "
                        + process.errorString().toLocal8Bit();
        return false;
    }

    // Windows: Due to implementation changes, the event loop needs
    // to be spun since we ourselves also need to answer the
    // WM_DRAWCLIPBOARD message as we are in the chain of clipboard
    // viewers. Check for running before waitForFinished() in case
    // the process terminated while processEvents() was executed.
    bool running = true;
    for (int i = 0; i < 60 && running; ++i) {
        QGuiApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        if (process.state() != QProcess::Running || process.waitForFinished(500))
            running = false;
    }
    if (running) {
        process.kill();
        *errorMessage = "Timeout running '" + program.toLocal8Bit() + '\'';
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        *errorMessage = "Process '" + program.toLocal8Bit() + "' crashed.";
        return false;
    }
    if (process.exitCode()) {
        *errorMessage = "Process '" + program.toLocal8Bit() + "' returns "
                        + QByteArray::number(process.exitCode());
        return false;
    }
    return true;
#else // QT_CONFIG(process)
    Q_UNUSED(program)
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)
    return false;
#endif // QT_CONFIG(process)
}

// Test that pasted text remains on the clipboard after a Qt application exits.
// This test does not make sense on X11 and embedded, copied data disappears from the clipboard when the application exits
void tst_QClipboard::copy_exit_paste()
{
#if QT_CONFIG(process)
    // ### It's still possible to test copy/paste - just keep the apps running
    if (!PlatformClipboard::isAvailable())
        QSKIP("Native clipboard not working in this setup");
    const QString stringArgument(QStringLiteral("Test string."));
    QByteArray errorMessage;
    QVERIFY2(runHelper(QStringLiteral("copier/copier"), QStringList(stringArgument), &errorMessage),
             errorMessage.constData());
#ifdef Q_OS_MAC
    // The Pasteboard needs a moment to breathe (at least on older Macs).
    QTest::qWait(100);
#endif // Q_OS_MAC
    QVERIFY2(runHelper(QStringLiteral("paster/paster"),
                       QStringList() << QStringLiteral("--text") << stringArgument,
                       &errorMessage),
             errorMessage.constData());
#endif // QT_CONFIG(process)
}

void tst_QClipboard::copyImage()
{
#if QT_CONFIG(process)
    if (!PlatformClipboard::isAvailable())
        QSKIP("Native clipboard not working in this setup");
    QImage image(100, 100, QImage::Format_ARGB32);
    image.fill(QColor(Qt::transparent));
    image.setPixel(QPoint(1, 0), QColor(Qt::blue).rgba());
    QGuiApplication::clipboard()->setImage(image);
#ifdef Q_OS_OSX
    // The Pasteboard needs a moment to breathe (at least on older Macs).
    QTest::qWait(100);
#endif // Q_OS_OSX
    // paster will perform hard-coded checks on the copied image.
    QByteArray errorMessage;
    QVERIFY2(runHelper(QStringLiteral("paster/paster"),
                       QStringList(QStringLiteral("--image")), &errorMessage),
             errorMessage.constData());
#endif // QT_CONFIG(process)
}

#endif // Q_OS_WIN || Q_OS_MAC || Q_OS_QNX

void tst_QClipboard::setMimeData()
{
    if (!PlatformClipboard::isAvailable())
        QSKIP("Native clipboard not working in this setup");
    QMimeData *mimeData = new QMimeData;
    const QString TestName(QLatin1String("tst_QClipboard::setMimeData() mimeData"));
    mimeData->setObjectName(TestName);

    QGuiApplication::clipboard()->setMimeData(mimeData);
    QCOMPARE(QGuiApplication::clipboard()->mimeData(), (const QMimeData *)mimeData);
    QCOMPARE(QGuiApplication::clipboard()->mimeData()->objectName(), TestName);

    // set it to the same data again, it shouldn't delete mimeData (and crash as a result)
    QGuiApplication::clipboard()->setMimeData(mimeData);
    QCOMPARE(QGuiApplication::clipboard()->mimeData(), (const QMimeData *)mimeData);
    QCOMPARE(QGuiApplication::clipboard()->mimeData()->objectName(), TestName);
    QGuiApplication::clipboard()->clear();
    const QMimeData *appMimeData = QGuiApplication::clipboard()->mimeData();
    QVERIFY(appMimeData != mimeData || appMimeData->objectName() != TestName);

    // check for crash when using the same mimedata object on several clipboards
    QMimeData *data = new QMimeData;
    data->setText("foo");

    QGuiApplication::clipboard()->setMimeData(data, QClipboard::Clipboard);
    if (QGuiApplication::clipboard()->supportsSelection())
        QGuiApplication::clipboard()->setMimeData(data, QClipboard::Selection);
    if (QGuiApplication::clipboard()->supportsFindBuffer())
        QGuiApplication::clipboard()->setMimeData(data, QClipboard::FindBuffer);

    QSignalSpy spySelection(QGuiApplication::clipboard(), SIGNAL(selectionChanged()));
    QSignalSpy spyData(QGuiApplication::clipboard(), SIGNAL(dataChanged()));
    // Clipboard notifications are asynchronous with the new AddClipboardFormatListener
    // in Windows Vista (5.4). Eat away all signals to ensure they don't interfere
    // with the QTRY_COMPARE below.
    EatSignalSpyNotificationsPredicate noLeftOverDataChanges(spyData);
    QTRY_VERIFY(noLeftOverDataChanges);
    QSignalSpy spyFindBuffer(QGuiApplication::clipboard(), SIGNAL(findBufferChanged()));

    QGuiApplication::clipboard()->clear(QClipboard::Clipboard);
    QGuiApplication::clipboard()->clear(QClipboard::Selection); // used to crash on X11
    QGuiApplication::clipboard()->clear(QClipboard::FindBuffer);

    if (QGuiApplication::clipboard()->supportsSelection())
        QCOMPARE(spySelection.count(), 1);
    else
        QCOMPARE(spySelection.count(), 0);

    if (QGuiApplication::clipboard()->supportsFindBuffer())
        QCOMPARE(spyFindBuffer.count(), 1);
    else
        QCOMPARE(spyFindBuffer.count(), 0);

    QTRY_COMPARE(spyData.count(), 1);

    // an other crash test
    data = new QMimeData;
    data->setText("foo");

    QGuiApplication::clipboard()->setMimeData(data, QClipboard::Clipboard);
    if (QGuiApplication::clipboard()->supportsSelection())
        QGuiApplication::clipboard()->setMimeData(data, QClipboard::Selection);
    if (QGuiApplication::clipboard()->supportsFindBuffer())
        QGuiApplication::clipboard()->setMimeData(data, QClipboard::FindBuffer);

    QMimeData *newData = new QMimeData;
    newData->setText("bar");

    spySelection.clear();
    noLeftOverDataChanges.reset();
    QTRY_VERIFY(noLeftOverDataChanges);
    spyFindBuffer.clear();

    QGuiApplication::clipboard()->setMimeData(newData, QClipboard::Clipboard);
    if (QGuiApplication::clipboard()->supportsSelection())
        QGuiApplication::clipboard()->setMimeData(newData, QClipboard::Selection); // used to crash on X11
    if (QGuiApplication::clipboard()->supportsFindBuffer())
        QGuiApplication::clipboard()->setMimeData(newData, QClipboard::FindBuffer);

    if (QGuiApplication::clipboard()->supportsSelection())
        QCOMPARE(spySelection.count(), 1);
    else
        QCOMPARE(spySelection.count(), 0);

    if (QGuiApplication::clipboard()->supportsFindBuffer())
        QCOMPARE(spyFindBuffer.count(), 1);
    else
        QCOMPARE(spyFindBuffer.count(), 0);

    QTRY_COMPARE(spyData.count(), 1);
}

void tst_QClipboard::clearBeforeSetText()
{
    QGuiApplication::processEvents();

    if (!PlatformClipboard::isAvailable())
        QSKIP("Native clipboard not working in this setup");

    const QString text = "tst_QClipboard::clearBeforeSetText()";

    // setText() should work after processEvents()
    QGuiApplication::clipboard()->setText(text);
    QCOMPARE(QGuiApplication::clipboard()->text(), text);
    QGuiApplication::processEvents();
    QCOMPARE(QGuiApplication::clipboard()->text(), text);

    // same with clear()
    QGuiApplication::clipboard()->clear();
    QVERIFY(QGuiApplication::clipboard()->text().isEmpty());
    QGuiApplication::processEvents();
    QVERIFY(QGuiApplication::clipboard()->text().isEmpty());

    // setText() again
    QGuiApplication::clipboard()->setText(text);
    QCOMPARE(QGuiApplication::clipboard()->text(), text);
    QGuiApplication::processEvents();
    QCOMPARE(QGuiApplication::clipboard()->text(), text);

    // clear() immediately followed by setText() should still return the text
    QGuiApplication::clipboard()->clear();
    QVERIFY(QGuiApplication::clipboard()->text().isEmpty());
    QGuiApplication::clipboard()->setText(text);
    QCOMPARE(QGuiApplication::clipboard()->text(), text);
    QGuiApplication::processEvents();
    QCOMPARE(QGuiApplication::clipboard()->text(), text);
}

#endif // QT_CONFIG(clipboard)

QTEST_MAIN(tst_QClipboard)

#include "tst_qclipboard.moc"
