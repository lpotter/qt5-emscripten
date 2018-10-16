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

#ifndef TST_QVARIANT_COMMON
#define TST_QVARIANT_COMMON

#include <QString>

class MessageHandler {
public:
    MessageHandler(const int typeId, QtMessageHandler msgHandler = handler)
        : oldMsgHandler(qInstallMessageHandler(msgHandler))
    {
        currentId = typeId;
    }

    ~MessageHandler()
    {
        qInstallMessageHandler(oldMsgHandler);
    }

    bool testPassed() const
    {
        return ok;
    }
protected:
    static void handler(QtMsgType, const QMessageLogContext &, const QString &msg)
    {
        // Format itself is not important, but basic data as a type name should be included in the output
        ok = msg.startsWith("QVariant(");
        QVERIFY2(ok, (QString::fromLatin1("Message is not started correctly: '") + msg + '\'').toLatin1().constData());
        ok &= (currentId == QMetaType::UnknownType
             ? msg.contains("Invalid")
             : msg.contains(QMetaType::typeName(currentId)));
        QVERIFY2(ok, (QString::fromLatin1("Message doesn't contain type name: '") + msg + '\'').toLatin1().constData());
        if (currentId == QMetaType::Char || currentId == QMetaType::QChar) {
            // Chars insert '\0' into the qdebug stream, it is not possible to find a real string length
            return;
        }
        if (QMetaType::typeFlags(currentId) & QMetaType::PointerToQObject) {
            QByteArray currentName = QMetaType::typeName(currentId);
            currentName.chop(1);
            ok &= (msg.contains(", " + currentName) || msg.contains(", 0x0"));
        }
        ok &= msg.endsWith(QLatin1Char(')'));
        QVERIFY2(ok, (QString::fromLatin1("Message is not correctly finished: '") + msg + '\'').toLatin1().constData());

    }

    QtMessageHandler oldMsgHandler;
    static int currentId;
    static bool ok;
};
bool MessageHandler::ok;
int MessageHandler::currentId;

#define TST_QVARIANT_CANCONVERT_DATATABLE_HEADERS \
    QTest::addColumn<QVariant>("val"); \
    QTest::addColumn<bool>("BitArrayCast"); \
    QTest::addColumn<bool>("BitmapCast"); \
    QTest::addColumn<bool>("BoolCast"); \
    QTest::addColumn<bool>("BrushCast"); \
    QTest::addColumn<bool>("ByteArrayCast"); \
    QTest::addColumn<bool>("ColorCast"); \
    QTest::addColumn<bool>("CursorCast"); \
    QTest::addColumn<bool>("DateCast"); \
    QTest::addColumn<bool>("DateTimeCast"); \
    QTest::addColumn<bool>("DoubleCast"); \
    QTest::addColumn<bool>("FontCast"); \
    QTest::addColumn<bool>("ImageCast"); \
    QTest::addColumn<bool>("IntCast"); \
    QTest::addColumn<bool>("InvalidCast"); \
    QTest::addColumn<bool>("KeySequenceCast"); \
    QTest::addColumn<bool>("ListCast"); \
    QTest::addColumn<bool>("LongLongCast"); \
    QTest::addColumn<bool>("MapCast"); \
    QTest::addColumn<bool>("PaletteCast"); \
    QTest::addColumn<bool>("PenCast"); \
    QTest::addColumn<bool>("PixmapCast"); \
    QTest::addColumn<bool>("PointCast"); \
    QTest::addColumn<bool>("RectCast"); \
    QTest::addColumn<bool>("RegionCast"); \
    QTest::addColumn<bool>("SizeCast"); \
    QTest::addColumn<bool>("SizePolicyCast"); \
    QTest::addColumn<bool>("StringCast"); \
    QTest::addColumn<bool>("StringListCast"); \
    QTest::addColumn<bool>("TimeCast"); \
    QTest::addColumn<bool>("UIntCast"); \
    QTest::addColumn<bool>("ULongLongCast");

#define TST_QVARIANT_CANCONVERT_FETCH_DATA \
    QFETCH(QVariant, val); \
    QFETCH(bool, BitArrayCast); \
    QFETCH(bool, BitmapCast); \
    QFETCH(bool, BoolCast); \
    QFETCH(bool, BrushCast); \
    QFETCH(bool, ByteArrayCast); \
    QFETCH(bool, ColorCast); \
    QFETCH(bool, CursorCast); \
    QFETCH(bool, DateCast); \
    QFETCH(bool, DateTimeCast); \
    QFETCH(bool, DoubleCast); \
    QFETCH(bool, FontCast); \
    QFETCH(bool, ImageCast); \
    QFETCH(bool, IntCast); \
    QFETCH(bool, InvalidCast); \
    QFETCH(bool, KeySequenceCast); \
    QFETCH(bool, ListCast); \
    QFETCH(bool, LongLongCast); \
    QFETCH(bool, MapCast); \
    QFETCH(bool, PaletteCast); \
    QFETCH(bool, PenCast); \
    QFETCH(bool, PixmapCast); \
    QFETCH(bool, PointCast); \
    QFETCH(bool, RectCast); \
    QFETCH(bool, RegionCast); \
    QFETCH(bool, SizeCast); \
    QFETCH(bool, SizePolicyCast); \
    QFETCH(bool, StringCast); \
    QFETCH(bool, StringListCast); \
    QFETCH(bool, TimeCast); \
    QFETCH(bool, UIntCast); \
    QFETCH(bool, ULongLongCast);

#define TST_QVARIANT_CANCONVERT_COMPARE_DATA \
    QCOMPARE(val.canConvert(QVariant::BitArray), BitArrayCast); \
    QCOMPARE(val.canConvert(QVariant::Bitmap), BitmapCast); \
    QCOMPARE(val.canConvert(QVariant::Bool), BoolCast); \
    QCOMPARE(val.canConvert(QVariant::Brush), BrushCast); \
    QCOMPARE(val.canConvert(QVariant::ByteArray), ByteArrayCast); \
    QCOMPARE(val.canConvert(QVariant::Color), ColorCast); \
    QCOMPARE(val.canConvert(QVariant::Cursor), CursorCast); \
    QCOMPARE(val.canConvert(QVariant::Date), DateCast); \
    QCOMPARE(val.canConvert(QVariant::DateTime), DateTimeCast); \
    QCOMPARE(val.canConvert(QVariant::Double), DoubleCast); \
    QCOMPARE(val.canConvert(QVariant::Type(QMetaType::Float)), DoubleCast); \
    QCOMPARE(val.canConvert(QVariant::Font), FontCast); \
    QCOMPARE(val.canConvert(QVariant::Image), ImageCast); \
    QCOMPARE(val.canConvert(QVariant::Int), IntCast); \
    QCOMPARE(val.canConvert(QVariant::Invalid), InvalidCast); \
    QCOMPARE(val.canConvert(QVariant::KeySequence), KeySequenceCast); \
    QCOMPARE(val.canConvert(QVariant::List), ListCast); \
    QCOMPARE(val.canConvert(QVariant::LongLong), LongLongCast); \
    QCOMPARE(val.canConvert(QVariant::Map), MapCast); \
    QCOMPARE(val.canConvert(QVariant::Palette), PaletteCast); \
    QCOMPARE(val.canConvert(QVariant::Pen), PenCast); \
    QCOMPARE(val.canConvert(QVariant::Pixmap), PixmapCast); \
    QCOMPARE(val.canConvert(QVariant::Point), PointCast); \
    QCOMPARE(val.canConvert(QVariant::Rect), RectCast); \
    QCOMPARE(val.canConvert(QVariant::Region), RegionCast); \
    QCOMPARE(val.canConvert(QVariant::Size), SizeCast); \
    QCOMPARE(val.canConvert(QVariant::SizePolicy), SizePolicyCast); \
    QCOMPARE(val.canConvert(QVariant::String), StringCast); \
    QCOMPARE(val.canConvert(QVariant::StringList), StringListCast); \
    QCOMPARE(val.canConvert(QVariant::Time), TimeCast); \
    QCOMPARE(val.canConvert(QVariant::UInt), UIntCast); \
    QCOMPARE(val.canConvert(QVariant::ULongLong), ULongLongCast);


#endif
