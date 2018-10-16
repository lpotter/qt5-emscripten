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

#include <QTest>

#include <QtAlgorithms>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <qplatformdefs.h>

#include <QDebug>

#include <cstdlib>
#include <cstdio>

#ifdef Q_OS_WIN
#  include <qt_windows.h>
#  include <io.h>
#  ifndef FSCTL_SET_SPARSE
// MinGW doesn't define this.
#    define FSCTL_SET_SPARSE (0x900C4)
#  endif
#endif // Q_OS_WIN

#include "emulationdetector.h"

class tst_LargeFile
    : public QObject
{
    Q_OBJECT

public:
    tst_LargeFile()
        : blockSize(1 << 12)
        , maxSizeBits()
        , fd_(-1)
        , stream_(0)
    {
    #if defined(QT_LARGEFILE_SUPPORT) && !defined(Q_OS_MAC) && !defined(Q_OS_WINRT)
        maxSizeBits = 36; // 64 GiB
    #elif defined(Q_OS_MAC)
        // HFS+ does not support sparse files, so we limit file size for the test
        // on Mac OS.
        maxSizeBits = 24; // 16 MiB
    #else
        maxSizeBits = 24; // 16 MiB
    #endif

        // QEMU only supports < 4GB files
        if (EmulationDetector::isRunningArmOnX86())
            maxSizeBits = qMin(maxSizeBits, 28);
    }

private:
    void sparseFileData();
    QByteArray const &getDataBlock(int index, qint64 position);

private slots:
    // The LargeFile test case was designed to be run in order as a single unit

    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    // Create and fill large file
    void createSparseFile();
    void fillFileSparsely();
    void closeSparseFile();

    // Verify file was created
    void fileCreated();

    // Positioning in large files
    void filePositioning();
    void fdPositioning();
    void streamPositioning();

    // Read data from file
    void openFileForReading();
    void readFile();

    // Map/unmap large file
    void mapFile();
    void mapOffsetOverflow();

    void closeFile() { largeFile.close(); }

    // Test data
    void fillFileSparsely_data() { sparseFileData(); }
    void filePositioning_data() { sparseFileData(); }
    void fdPositioning_data() { sparseFileData(); }
    void streamPositioning_data() { sparseFileData(); }
    void readFile_data() { sparseFileData(); }
    void mapFile_data() { sparseFileData(); }

private:
    const int blockSize;
    int maxSizeBits;

    QFile largeFile;

    QVector<QByteArray> generatedBlocks;

    int fd_;
    FILE *stream_;

    QSharedPointer<QTemporaryDir> m_tempDir;
    QString m_previousCurrent;
};

/*
    Convenience function to hide reinterpret_cast when copying a POD directly
    into a QByteArray.
 */
template <class T>
static inline void appendRaw(QByteArray &array, T data)
{
    array.append(reinterpret_cast<char *>(&data), sizeof(T));
}

/*
    Pad array with filler up to size. On return, array.size() returns size.
 */
static inline void topUpWith(QByteArray &array, QByteArray filler, int size)
{
    for (int i = (size - array.size()) / filler.size(); i > 0; --i)
        array.append(filler);

    if (array.size() < size) {
        array.append(filler.left(size - array.size()));
    }
}

/*
   Generate a unique data block containing identifiable data. Unaligned,
   overlapping and partial blocks should not compare equal.
 */
static inline QByteArray generateDataBlock(int blockSize, QString text, qint64 userBits = -1)
{
    QByteArray block;
    block.reserve(blockSize);

    // Use of counter and randomBits means content of block will be dependent
    // on the generation order. For (file-)systems that do not support sparse
    // files, these can be removed so the test file can be reused and doesn't
    // have to be generated for every run.

    static qint64 counter = 0;

    qint64 randomBits = QRandomGenerator::global()->generate64();

    appendRaw(block, randomBits);
    appendRaw(block, userBits);
    appendRaw(block, counter);
    appendRaw(block, (qint32)0xdeadbeef);
    appendRaw(block, blockSize);

    QByteArray userContent = text.toUtf8();
    appendRaw(block, userContent.size());
    block.append(userContent);
    appendRaw(block, (qint64)0);

    // size, so far
    appendRaw(block, block.size());

    QByteArray filler("0123456789");
    block.append(filler.right(10 - block.size() % 10));
    topUpWith(block, filler, blockSize - 3 * sizeof(qint64));

    appendRaw(block, counter);
    appendRaw(block, userBits);
    appendRaw(block, randomBits);

    ++counter;
    return block;
}

/*
   Generates data blocks the first time they are requested. Keeps copies for reuse.
 */
QByteArray const &tst_LargeFile::getDataBlock(int index, qint64 position)
{
    if (index >= generatedBlocks.size())
        generatedBlocks.resize(index + 1);

    if (generatedBlocks[index].isNull()) {
        QString text = QString("Current %1-byte block (index = %2) "
            "starts %3 bytes into the file '%4'.")
                .arg(blockSize)
                .arg(index)
                .arg(position)
                .arg("qt_largefile.tmp");

        generatedBlocks[index] = generateDataBlock(blockSize, text, (qint64)1 << index);
    }

    return generatedBlocks[index];
}

void tst_LargeFile::initTestCase()
{
    m_previousCurrent = QDir::currentPath();
    m_tempDir = QSharedPointer<QTemporaryDir>::create();
    QVERIFY2(!m_tempDir.isNull(), qPrintable("Could not create temporary directory."));
    QVERIFY2(QDir::setCurrent(m_tempDir->path()), qPrintable("Could not switch current directory"));

    QFile file("qt_largefile.tmp");
    QVERIFY( !file.exists() || file.remove() );
}

void tst_LargeFile::cleanupTestCase()
{
    if (largeFile.isOpen())
        largeFile.close();

    QFile file("qt_largefile.tmp");
    QVERIFY( !file.exists() || file.remove() );

    QDir::setCurrent(m_previousCurrent);
}

void tst_LargeFile::init()
{
    fd_ = -1;
    stream_ = 0;
}

void tst_LargeFile::cleanup()
{
    if (-1 != fd_)
        QT_CLOSE(fd_);
    if (stream_)
        ::fclose(stream_);
}

void tst_LargeFile::sparseFileData()
{
    QTest::addColumn<int>("index");
    QTest::addColumn<qint64>("position");
    QTest::addColumn<QByteArray>("block");

    QTest::newRow(QString("block[%1] @%2)")
            .arg(0).arg(0)
            .toLocal8Bit().constData())
        << 0 << (qint64)0 << getDataBlock(0, 0);

    // While on Linux sparse files scale well, on Windows, testing at every
    // power of 2 leads to very large files. i += 4 gives us a good coverage
    // without taxing too much on resources.
    for (int index = 12; index <= maxSizeBits; index += 4) {
        qint64 position = (qint64)1 << index;
        QByteArray block = getDataBlock(index, position);

        QTest::newRow(
            QString("block[%1] @%2)")
                .arg(index).arg(position)
                .toLocal8Bit().constData())
            << index << position << block;
    }
}

void tst_LargeFile::createSparseFile()
{
#if defined(Q_OS_WIN32)
    // On Windows platforms, we must explicitly set the file to be sparse,
    // so disk space is not allocated for the full file when writing to it.
    HANDLE handle = ::CreateFileA("qt_largefile.tmp",
        GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    QVERIFY( INVALID_HANDLE_VALUE != handle );

    DWORD bytes;
    if (!::DeviceIoControl(handle, FSCTL_SET_SPARSE, NULL, 0, NULL, 0,
            &bytes, NULL)) {
        QWARN("Unable to set test file as sparse. "
            "Limiting test file to 16MiB.");
        maxSizeBits = 24;
    }

    int fd = ::_open_osfhandle((intptr_t)handle, 0);
    QVERIFY( -1 != fd );
    QVERIFY( largeFile.open(fd, QIODevice::WriteOnly | QIODevice::Unbuffered) );
#else // !Q_OS_WIN32
    largeFile.setFileName("qt_largefile.tmp");
    QVERIFY( largeFile.open(QIODevice::WriteOnly | QIODevice::Unbuffered) );
#endif
}

void tst_LargeFile::closeSparseFile()
{
#if defined(Q_OS_WIN32)
    int fd = largeFile.handle();
#endif

    largeFile.close();

#if defined(Q_OS_WIN32)
    if (-1 != fd)
        ::_close(fd);
#endif
}

void tst_LargeFile::fillFileSparsely()
{
    QFETCH( qint64, position );
    QFETCH( QByteArray, block );
    QCOMPARE( block.size(), blockSize );

    static int lastKnownGoodIndex = 0;
    struct ScopeGuard {
        ScopeGuard(tst_LargeFile* test)
            : this_(test)
            , failed(true)
        {
            QFETCH( int, index );
            index_ = index;
        }

        ~ScopeGuard()
        {
            if (failed) {
                this_->maxSizeBits = lastKnownGoodIndex;
                QWARN( qPrintable(
                    QString("QFile::error %1: '%2'. Maximum size bits reset to %3.")
                        .arg(this_->largeFile.error())
                        .arg(this_->largeFile.errorString())
                        .arg(this_->maxSizeBits)) );
            } else
                lastKnownGoodIndex = qMax<int>(index_, lastKnownGoodIndex);
        }

    private:
        tst_LargeFile * const this_;
        int index_;

    public:
        bool failed;
    };

    ScopeGuard resetMaxSizeBitsOnFailure(this);

    QVERIFY( largeFile.seek(position) );
    QCOMPARE( largeFile.pos(), position );

    QCOMPARE( largeFile.write(block), (qint64)blockSize );
    QCOMPARE( largeFile.pos(), position + blockSize );
    QVERIFY( largeFile.flush() );

    resetMaxSizeBitsOnFailure.failed = false;
}

void tst_LargeFile::fileCreated()
{
    QFileInfo info("qt_largefile.tmp");

    QVERIFY( info.exists() );
    QVERIFY( info.isFile() );
    QVERIFY( info.size() >= ((qint64)1 << maxSizeBits) + blockSize );
}

void tst_LargeFile::filePositioning()
{
    QFETCH( qint64, position );

    QFile file("qt_largefile.tmp");
    QVERIFY( file.open(QIODevice::ReadOnly) );

    QVERIFY( file.seek(position) );
    QCOMPARE( file.pos(), position );
}

void tst_LargeFile::fdPositioning()
{
    QFETCH( qint64, position );

    fd_ = QT_OPEN("qt_largefile.tmp",
            QT_OPEN_RDONLY | QT_OPEN_LARGEFILE);
    QVERIFY( -1 != fd_ );

    QFile file;
    QVERIFY( file.open(fd_, QIODevice::ReadOnly) );
    QCOMPARE( file.pos(), (qint64)0 );
    QVERIFY( file.seek(position) );
    QCOMPARE( file.pos(), position );

    file.close();

    QCOMPARE( QT_OFF_T(QT_LSEEK(fd_, QT_OFF_T(0), SEEK_SET)), QT_OFF_T(0) );
    QCOMPARE( QT_OFF_T(QT_LSEEK(fd_, QT_OFF_T(position), SEEK_SET)), QT_OFF_T(position) );

    QVERIFY( file.open(fd_, QIODevice::ReadOnly) );
    QCOMPARE( QT_OFF_T(QT_LSEEK(fd_, QT_OFF_T(0), SEEK_CUR)), QT_OFF_T(position) );
    QCOMPARE( file.pos(), position );
    QVERIFY( file.seek(0) );
    QCOMPARE( file.pos(), (qint64)0 );

    file.close();

    QVERIFY( !QT_CLOSE(fd_) );
    fd_ = -1;
}

void tst_LargeFile::streamPositioning()
{
    QFETCH( qint64, position );

    stream_ = QT_FOPEN("qt_largefile.tmp", "rb");
    QVERIFY( 0 != stream_ );

    QFile file;
    QVERIFY( file.open(stream_, QIODevice::ReadOnly) );
    QCOMPARE( file.pos(), (qint64)0 );
    QVERIFY( file.seek(position) );
    QCOMPARE( file.pos(), position );

    file.close();

    QVERIFY( !QT_FSEEK(stream_, QT_OFF_T(0), SEEK_SET) );
    QCOMPARE( QT_OFF_T(QT_FTELL(stream_)), QT_OFF_T(0) );
    QVERIFY( !QT_FSEEK(stream_, QT_OFF_T(position), SEEK_SET) );
    QCOMPARE( QT_OFF_T(QT_FTELL(stream_)), QT_OFF_T(position) );

    QVERIFY( file.open(stream_, QIODevice::ReadOnly) );
    QCOMPARE( QT_OFF_T(QT_FTELL(stream_)), QT_OFF_T(position) );
    QCOMPARE( file.pos(), position );
    QVERIFY( file.seek(0) );
    QCOMPARE( file.pos(), (qint64)0 );

    file.close();

    QVERIFY( !::fclose(stream_) );
    stream_ = 0;
}

void tst_LargeFile::openFileForReading()
{
    largeFile.setFileName("qt_largefile.tmp");
    QVERIFY( largeFile.open(QIODevice::ReadOnly) );
}

void tst_LargeFile::readFile()
{
    QFETCH( qint64, position );
    QFETCH( QByteArray, block );
    QCOMPARE( block.size(), blockSize );

    QVERIFY( largeFile.size() >= position + blockSize );

    QVERIFY( largeFile.seek(position) );
    QCOMPARE( largeFile.pos(), position );

    QCOMPARE( largeFile.read(blockSize), block );
    QCOMPARE( largeFile.pos(), position + blockSize );
}

void tst_LargeFile::mapFile()
{
    QFETCH( qint64, position );
    QFETCH( QByteArray, block );
    QCOMPARE( block.size(), blockSize );

    // Keep full block mapped to facilitate OS and/or internal reuse by Qt.
    uchar *baseAddress = largeFile.map(position, blockSize);
    QVERIFY( baseAddress );
    QVERIFY( qEqual(block.begin(), block.end(), reinterpret_cast<char*>(baseAddress)) );

    for (int offset = 1; offset <  blockSize; ++offset) {
        uchar *address = largeFile.map(position + offset, blockSize - offset);

        QVERIFY( address );
        if ( !qEqual(block.begin() + offset, block.end(), reinterpret_cast<char*>(address)) ) {
            qDebug() << "Expected:" << block.toHex();
            qDebug() << "Actual  :" << QByteArray(reinterpret_cast<char*>(address), blockSize).toHex();
            QVERIFY(false);
        }

        QVERIFY( largeFile.unmap( address ) );
    }

    QVERIFY( largeFile.unmap( baseAddress ) );
}

//Mac: memory-mapping beyond EOF may succeed but it could generate bus error on access
//FreeBSD: same
//Linux: memory-mapping beyond EOF usually succeeds, but depends on the filesystem
//  32-bit: limited to 44-bit offsets (when sizeof(off_t) == 8)
//Windows: memory-mapping beyond EOF is not allowed
void tst_LargeFile::mapOffsetOverflow()
{
    enum {
#ifdef Q_OS_WIN
        Succeeds = false,
        MaxOffset = 63
#else
        Succeeds = true,
#  if (defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)) && Q_PROCESSOR_WORDSIZE == 4
        MaxOffset = sizeof(QT_OFF_T) > 4 ? 43 : 30
#  else
        MaxOffset = 8 * sizeof(QT_OFF_T) - 1
#  endif
#endif
    };

    QByteArray zeroPage(blockSize, '\0');
    for (int i = maxSizeBits + 1; i < 63; ++i) {
        bool succeeds = Succeeds && (i <= MaxOffset);
        uchar *address = 0;
        qint64 offset = Q_INT64_C(1) << i;

        if (succeeds)
            QTest::ignoreMessage(QtWarningMsg, "QFSFileEngine::map: Mapping a file beyond its size is not portable");
        address = largeFile.map(offset, blockSize);
        QCOMPARE(!!address, succeeds);

        if (succeeds)
            QTest::ignoreMessage(QtWarningMsg, "QFSFileEngine::map: Mapping a file beyond its size is not portable");
        address = largeFile.map(offset + blockSize, blockSize);
        QCOMPARE(!!address, succeeds);
    }
}

QTEST_APPLESS_MAIN(tst_LargeFile)
#include "tst_largefile.moc"

