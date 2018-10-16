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


#include <qfont.h>
#include <private/qfont_p.h>
#include <qfontdatabase.h>
#include <qfontinfo.h>
#include <qstringlist.h>
#include <qguiapplication.h>
#ifndef QT_NO_WIDGETS
#include <qwidget.h>
#endif
#include <qlist.h>

class tst_QFont : public QObject
{
Q_OBJECT

private slots:
    void getSetCheck();
    void exactMatch();
    void compare();
    void resolve();
#ifndef QT_NO_WIDGETS
    void resetFont();
#endif
    void isCopyOf();
    void italicOblique();
    void insertAndRemoveSubstitutions();
    void serialize_data();
    void serialize();
    void lastResortFont();
    void styleName();
    void defaultFamily_data();
    void defaultFamily();
    void toAndFromString();
    void fromStringWithoutStyleName();

    void sharing();
};

// Testing get/set functions
void tst_QFont::getSetCheck()
{
    QFont obj1;
    // Style QFont::style()
    // void QFont::setStyle(Style)
    obj1.setStyle(QFont::Style(QFont::StyleNormal));
    QCOMPARE(QFont::Style(QFont::StyleNormal), obj1.style());
    obj1.setStyle(QFont::Style(QFont::StyleItalic));
    QCOMPARE(QFont::Style(QFont::StyleItalic), obj1.style());
    obj1.setStyle(QFont::Style(QFont::StyleOblique));
    QCOMPARE(QFont::Style(QFont::StyleOblique), obj1.style());

    // StyleStrategy QFont::styleStrategy()
    // void QFont::setStyleStrategy(StyleStrategy)
    obj1.setStyleStrategy(QFont::StyleStrategy(QFont::PreferDefault));
    QCOMPARE(QFont::StyleStrategy(QFont::PreferDefault), obj1.styleStrategy());
    obj1.setStyleStrategy(QFont::StyleStrategy(QFont::PreferBitmap));
    QCOMPARE(QFont::StyleStrategy(QFont::PreferBitmap), obj1.styleStrategy());
    obj1.setStyleStrategy(QFont::StyleStrategy(QFont::PreferDevice));
    QCOMPARE(QFont::StyleStrategy(QFont::PreferDevice), obj1.styleStrategy());
    obj1.setStyleStrategy(QFont::StyleStrategy(QFont::PreferOutline));
    QCOMPARE(QFont::StyleStrategy(QFont::PreferOutline), obj1.styleStrategy());
    obj1.setStyleStrategy(QFont::StyleStrategy(QFont::ForceOutline));
    QCOMPARE(QFont::StyleStrategy(QFont::ForceOutline), obj1.styleStrategy());
    obj1.setStyleStrategy(QFont::StyleStrategy(QFont::PreferMatch));
    QCOMPARE(QFont::StyleStrategy(QFont::PreferMatch), obj1.styleStrategy());
    obj1.setStyleStrategy(QFont::StyleStrategy(QFont::PreferQuality));
    QCOMPARE(QFont::StyleStrategy(QFont::PreferQuality), obj1.styleStrategy());
    obj1.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias));
    QCOMPARE(QFont::StyleStrategy(QFont::PreferAntialias), obj1.styleStrategy());
    obj1.setStyleStrategy(QFont::StyleStrategy(QFont::NoAntialias));
    QCOMPARE(QFont::StyleStrategy(QFont::NoAntialias), obj1.styleStrategy());
    obj1.setStyleStrategy(QFont::StyleStrategy(QFont::OpenGLCompatible));
    QCOMPARE(QFont::StyleStrategy(QFont::OpenGLCompatible), obj1.styleStrategy());
}

void tst_QFont::exactMatch()
{
    QFont font;

    // Check if a non-existing font hasn't an exact match
    font = QFont( "BogusFont", 33 );
    QVERIFY( !font.exactMatch() );
    QVERIFY(!QFont("sans").exactMatch());
    QVERIFY(!QFont("sans-serif").exactMatch());
    QVERIFY(!QFont("serif").exactMatch());
    QVERIFY(!QFont("monospace").exactMatch());
}

void tst_QFont::italicOblique()
{
    QFontDatabase fdb;

    QStringList families = fdb.families();
    if (families.isEmpty())
        return;

    QStringList::ConstIterator f_it, f_end = families.end();
    for (f_it = families.begin(); f_it != f_end; ++f_it) {

        QString family = *f_it;
        QStringList styles = fdb.styles(family);
        QStringList::ConstIterator s_it, s_end = styles.end();
        for (s_it = styles.begin(); s_it != s_end; ++s_it) {
            QString style = *s_it;

            if (fdb.isSmoothlyScalable(family, style)) {
                if (style.contains("Oblique")) {
                    style.replace("Oblique", "Italic");
                } else if (style.contains("Italic")) {
                    style.replace("Italic", "Oblique");
                } else {
                    continue;
                }
                QFont f = fdb.font(family, style, 12);
                QVERIFY(f.italic());
            }
        }
    }
}

void tst_QFont::compare()
{
    QFont font;
    {
        QFont font2 = font;
        font2.setPointSize(24);
        QVERIFY(font != font2);
        QCOMPARE(font < font2,!(font2 < font));
    }
    {
        QFont font2 = font;
        font2.setPixelSize(24);
        QVERIFY(font != font2);
        QCOMPARE(font < font2,!(font2 < font));
    }

    font.setPointSize(12);
    font.setItalic(false);
    font.setWeight(QFont::Normal);
    font.setUnderline(false);
    font.setStrikeOut(false);
    font.setOverline(false);
    {
        QFont font2 = font;
        font2.setPointSize(24);
        QVERIFY(font != font2);
        QCOMPARE(font < font2,!(font2 < font));
    }
    {
        QFont font2 = font;
        font2.setPixelSize(24);
        QVERIFY(font != font2);
        QCOMPARE(font < font2,!(font2 < font));
    }
    {
        QFont font2 = font;

        font2.setItalic(true);
        QVERIFY(font != font2);
        QCOMPARE(font < font2,!(font2 < font));
        font2.setItalic(false);
        QCOMPARE(font, font2);
        QVERIFY(!(font < font2));

        font2.setWeight(QFont::Bold);
        QVERIFY(font != font2);
        QCOMPARE(font < font2,!(font2 < font));
        font2.setWeight(QFont::Normal);
        QCOMPARE(font, font2);
        QVERIFY(!(font < font2));

        font.setUnderline(true);
        QVERIFY(font != font2);
        QCOMPARE(font < font2,!(font2 < font));
        font.setUnderline(false);
        QCOMPARE(font, font2);
        QVERIFY(!(font < font2));

        font.setStrikeOut(true);
        QVERIFY(font != font2);
        QCOMPARE(font < font2,!(font2 < font));
        font.setStrikeOut(false);
        QCOMPARE(font, font2);
        QVERIFY(!(font < font2));

        font.setOverline(true);
        QVERIFY(font != font2);
        QCOMPARE(font < font2,!(font2 < font));
        font.setOverline(false);
        QCOMPARE(font, font2);
        QVERIFY(!(font < font2));

        font.setCapitalization(QFont::SmallCaps);
        QVERIFY(font != font2);
        QCOMPARE(font < font2,!(font2 < font));
        font.setCapitalization(QFont::MixedCase);
        QCOMPARE(font, font2);
        QVERIFY(!(font < font2));
    }
}

void tst_QFont::resolve()
{
    QFont font;
    font.setPointSize(font.pointSize() * 2);
    font.setItalic(false);
    font.setWeight(QFont::Normal);
    font.setUnderline(false);
    font.setStrikeOut(false);
    font.setOverline(false);
    font.setStretch(QFont::Unstretched);

    QFont font1;
    font1.setWeight(QFont::Bold);
    QFont font2 = font1.resolve(font);

    QCOMPARE(font2.weight(), font1.weight());

    QCOMPARE(font2.pointSize(), font.pointSize());
    QCOMPARE(font2.italic(), font.italic());
    QCOMPARE(font2.underline(), font.underline());
    QCOMPARE(font2.overline(), font.overline());
    QCOMPARE(font2.strikeOut(), font.strikeOut());
    QCOMPARE(font2.stretch(), font.stretch());

    QFont font3;
    font3.setStretch(QFont::UltraCondensed);
    QFont font4 = font3.resolve(font1).resolve(font);

    QCOMPARE(font4.stretch(), font3.stretch());

    QCOMPARE(font4.weight(), font.weight());
    QCOMPARE(font4.pointSize(), font.pointSize());
    QCOMPARE(font4.italic(), font.italic());
    QCOMPARE(font4.underline(), font.underline());
    QCOMPARE(font4.overline(), font.overline());
    QCOMPARE(font4.strikeOut(), font.strikeOut());


    QFont f1,f2,f3;
    f2.setPointSize(45);
    f3.setPointSize(55);

    QFont f4 = f1.resolve(f2);
    QCOMPARE(f4.pointSize(), 45);
    f4 = f4.resolve(f3);
    QCOMPARE(f4.pointSize(), 55);
}

#ifndef QT_NO_WIDGETS
void tst_QFont::resetFont()
{
    QWidget parent;
    QFont parentFont = parent.font();
    parentFont.setPointSize(parentFont.pointSize() + 2);
    parent.setFont(parentFont);

    QWidget *child = new QWidget(&parent);

    QFont childFont = child->font();
    childFont.setBold(!childFont.bold());
    child->setFont(childFont);

    QVERIFY(parentFont.resolve() != 0);
    QVERIFY(childFont.resolve() != 0);
    QVERIFY(childFont != parentFont);

    child->setFont(QFont()); // reset font

    QCOMPARE(child->font().resolve(), uint(0));
#ifdef Q_OS_ANDROID
    QEXPECT_FAIL("", "QTBUG-69214", Continue);
#endif
    QCOMPARE(child->font().pointSize(), parent.font().pointSize());
    QVERIFY(parent.font().resolve() != 0);
}
#endif

void tst_QFont::isCopyOf()
{
    QFont font;
    QVERIFY(font.isCopyOf(QGuiApplication::font()));

    QFont font2("bogusfont",  23);
    QVERIFY(! font2.isCopyOf(QGuiApplication::font()));

    QFont font3 = font;
    QVERIFY(font3.isCopyOf(font));

    font3.setPointSize(256);
    QVERIFY(!font3.isCopyOf(font));
    font3.setPointSize(font.pointSize());
    QVERIFY(!font3.isCopyOf(font));
}

void tst_QFont::insertAndRemoveSubstitutions()
{
    QFont::removeSubstitutions("BogusFontFamily");
    // make sure it is empty before we start
    QVERIFY(QFont::substitutes("BogusFontFamily").isEmpty());
    QVERIFY(QFont::substitutes("bogusfontfamily").isEmpty());

    // inserting Foo
    QFont::insertSubstitution("BogusFontFamily", "Foo");
    QCOMPARE(QFont::substitutes("BogusFontFamily").count(), 1);
    QCOMPARE(QFont::substitutes("bogusfontfamily").count(), 1);

    // inserting Bar and Baz
    QStringList moreFonts;
    moreFonts << "Bar" << "Baz";
    QFont::insertSubstitutions("BogusFontFamily", moreFonts);
    QCOMPARE(QFont::substitutes("BogusFontFamily").count(), 3);
    QCOMPARE(QFont::substitutes("bogusfontfamily").count(), 3);

    QFont::removeSubstitutions("BogusFontFamily");
    // make sure it is empty again
    QVERIFY(QFont::substitutes("BogusFontFamily").isEmpty());
    QVERIFY(QFont::substitutes("bogusfontfamily").isEmpty());
}

Q_DECLARE_METATYPE(QDataStream::Version)

void tst_QFont::serialize_data()
{
    QTest::addColumn<QFont>("font");
    // The version in which the tested feature was added.
    QTest::addColumn<QDataStream::Version>("minimumStreamVersion");

    QFont basicFont;
    // Versions <= Qt 2.1 had broken point size serialization,
    // so we set an integer point size.
    basicFont.setPointSize(9);
    // Versions <= Qt 5.4 didn't serialize styleName, so clear it
    basicFont.setStyleName(QString());

    QFont font = basicFont;
    QTest::newRow("defaultConstructed") << font << QDataStream::Qt_1_0;

    font.setLetterSpacing(QFont::AbsoluteSpacing, 105);
    QTest::newRow("letterSpacing") << font << QDataStream::Qt_4_5;

    font = basicFont;
    font.setWordSpacing(50.0);
    QTest::newRow("wordSpacing") << font << QDataStream::Qt_4_5;

    font = basicFont;
    font.setPointSize(20);
    QTest::newRow("pointSize") << font << QDataStream::Qt_1_0;

    font = basicFont;
    font.setPixelSize(32);
    QTest::newRow("pixelSize") << font << QDataStream::Qt_3_0;

    font = basicFont;
    font.setStyleHint(QFont::Monospace);
    QTest::newRow("styleHint") << font << QDataStream::Qt_1_0;

    font = basicFont;
    font.setStretch(4000);
    QTest::newRow("stretch") << font << QDataStream::Qt_4_3;

    font = basicFont;
    font.setWeight(99);
    QTest::newRow("weight") << font << QDataStream::Qt_1_0;

    font = basicFont;
    font.setUnderline(true);
    QTest::newRow("underline") << font << QDataStream::Qt_1_0;

    font = basicFont;
    font.setStrikeOut(true);
    QTest::newRow("strikeOut") << font << QDataStream::Qt_1_0;

    font = basicFont;
    font.setFixedPitch(true);
    // This fails for versions less than this, as ignorePitch is set to false
    // whenever setFixedPitch() is called, but ignorePitch is considered an
    // extended bit, which were apparently not available until 4.4.
    QTest::newRow("fixedPitch") << font << QDataStream::Qt_4_4;

    font = basicFont;
    font.setLetterSpacing(QFont::AbsoluteSpacing, 10);
    // Fails for 4.4 because letterSpacing wasn't read until 4.5.
    QTest::newRow("letterSpacing") << font << QDataStream::Qt_4_5;

    font = basicFont;
    font.setKerning(false);
    QTest::newRow("kerning") << font << QDataStream::Qt_4_0;

    font = basicFont;
    font.setStyleStrategy(QFont::NoFontMerging);
    // This wasn't read properly until 5.4.
    QTest::newRow("styleStrategy") << font << QDataStream::Qt_5_4;

    font = basicFont;
    font.setHintingPreference(QFont::PreferFullHinting);
    // This wasn't read until 5.4.
    QTest::newRow("hintingPreference") << font << QDataStream::Qt_5_4;

    font = basicFont;
    font.setStyleName("Regular Black Condensed");
    // This wasn't read until 5.4.
    QTest::newRow("styleName") << font << QDataStream::Qt_5_4;

    font = basicFont;
    font.setCapitalization(QFont::AllUppercase);
    // This wasn't read until 5.6.
    QTest::newRow("capitalization") << font << QDataStream::Qt_5_6;
}

void tst_QFont::serialize()
{
    QFETCH(QFont, font);
    QFETCH(QDataStream::Version, minimumStreamVersion);

    QDataStream stream;
    const int thisVersion = stream.version();

    for (int version = minimumStreamVersion; version <= thisVersion; ++version) {
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        stream.setDevice(&buffer);
        stream.setVersion(version);
        stream << font;
        buffer.close();

        buffer.open(QIODevice::ReadOnly);
        QFont readFont;
        stream >> readFont;
        QVERIFY2(readFont == font, qPrintable(QString::fromLatin1("Fonts do not compare equal for QDataStream version ") +
            QString::fromLatin1("%1:\nactual: %2\nexpected: %3").arg(version).arg(readFont.toString()).arg(font.toString())));
    }
}

// QFont::lastResortFont() may abort with qFatal() on QWS/QPA
// if absolutely no font is found. Just as ducumented for QFont::lastResortFont().
// This happens on our CI machines which run QWS autotests.
// ### fixme: Check platforms
void tst_QFont::lastResortFont()
{
    QSKIP("QFont::lastResortFont() may abort with qFatal() on QPA, QTBUG-22325");
    QFont font;
    QVERIFY(!font.lastResortFont().isEmpty());
}

void tst_QFont::styleName()
{
#if !defined(Q_OS_MAC)
    QSKIP("Only tested on Mac");
#else
    QFont font("Helvetica Neue");
    font.setStyleName("UltraLight");

    QCOMPARE(QFontInfo(font).styleName(), QString("UltraLight"));
#endif
}

QString getPlatformGenericFont(const char* genericName)
{
#if defined(Q_OS_UNIX) && !defined(QT_NO_FONTCONFIG) && QT_CONFIG(process)
    QProcess p;
    p.start(QLatin1String("fc-match"), (QStringList() << "-f%{family}" << genericName));
    if (!p.waitForStarted())
        qWarning("fc-match cannot be started: %s", qPrintable(p.errorString()));
    if (p.waitForFinished())
        return QString::fromLatin1(p.readAllStandardOutput());
#endif
    return QLatin1String(genericName);
}

static inline QByteArray msgNotAcceptableFont(const QString &defaultFamily, const QStringList &acceptableFamilies)
{
    QString res = QString::fromLatin1("Font family '%1' is not one of the following acceptable results: ").arg(defaultFamily);
    Q_FOREACH (const QString &family, acceptableFamilies)
        res += QLatin1String("\n ") + family;
    return res.toLocal8Bit();
}

Q_DECLARE_METATYPE(QFont::StyleHint)
void tst_QFont::defaultFamily_data()
{
    QTest::addColumn<QFont::StyleHint>("styleHint");
    QTest::addColumn<QStringList>("acceptableFamilies");

    QTest::newRow("serif") << QFont::Serif << (QStringList() << "Times New Roman" << "Times" << "Droid Serif" << getPlatformGenericFont("serif").split(","));
    QTest::newRow("monospace") << QFont::Monospace << (QStringList() << "Courier New" << "Monaco" << "Droid Sans Mono" << getPlatformGenericFont("monospace").split(","));
    QTest::newRow("cursive") << QFont::Cursive << (QStringList() << "Comic Sans MS" << "Apple Chancery" << "Roboto" << "Droid Sans" << getPlatformGenericFont("cursive").split(","));
    QTest::newRow("fantasy") << QFont::Fantasy << (QStringList() << "Impact" << "Zapfino"  << "Roboto" << "Droid Sans" << getPlatformGenericFont("fantasy").split(","));
    QTest::newRow("sans-serif") << QFont::SansSerif << (QStringList() << "Arial" << "Lucida Grande" << "Roboto" << "Droid Sans" << "Segoe UI" << getPlatformGenericFont("sans-serif").split(","));
}

void tst_QFont::defaultFamily()
{
    QFETCH(QFont::StyleHint, styleHint);
    QFETCH(QStringList, acceptableFamilies);

    QFont f;
    QFontDatabase db;
    f.setStyleHint(styleHint);
    const QString familyForHint(f.defaultFamily());

    // it should at least return a family that is available.
    QVERIFY(db.hasFamily(familyForHint));

    bool isAcceptable = false;
    Q_FOREACH (const QString& family, acceptableFamilies) {
        if (!familyForHint.compare(family, Qt::CaseInsensitive)) {
            isAcceptable = true;
            break;
        }
    }

#ifdef Q_OS_ANDROID
    QEXPECT_FAIL("serif", "QTBUG-69215", Continue);
#endif
    QVERIFY2(isAcceptable, msgNotAcceptableFont(familyForHint, acceptableFamilies));
}

void tst_QFont::toAndFromString()
{
    QFont defaultFont = QGuiApplication::font();
    QString family = defaultFont.family();

    QFontDatabase fdb;
    const QStringList stylesList = fdb.styles(family);
    if (stylesList.size() == 0)
        QSKIP("Default font doesn't have any styles");

    for (const QString &style : stylesList) {
        QFont result;
        QFont initial = fdb.font(family, style, defaultFont.pointSize());

        result.fromString(initial.toString());

        QCOMPARE(result, initial);
    }
}

void tst_QFont::fromStringWithoutStyleName()
{
    QFont font1;
    font1.fromString("Noto Sans,12,-1,5,50,0,0,0,0,0,Regular");

    QFont font2 = font1;
    const QString str = "Times,16,-1,5,50,0,0,0,0,0";
    font2.fromString(str);

    QCOMPARE(font2.toString(), str);
}


void tst_QFont::sharing()
{
    // QFontCache references the engineData
    int refs_by_cache = 1;

    QFont f;
    f.setStyleHint(QFont::Serif);
    f.exactMatch(); // loads engine
    QCOMPARE(QFontPrivate::get(f)->ref.load(), 1);
    QVERIFY(QFontPrivate::get(f)->engineData);
    QCOMPARE(QFontPrivate::get(f)->engineData->ref.load(), 1 + refs_by_cache);

    QFont f2(f);
    QCOMPARE(QFontPrivate::get(f2), QFontPrivate::get(f));
    QCOMPARE(QFontPrivate::get(f2)->ref.load(), 2);
    QVERIFY(QFontPrivate::get(f2)->engineData);
    QCOMPARE(QFontPrivate::get(f2)->engineData, QFontPrivate::get(f)->engineData);
    QCOMPARE(QFontPrivate::get(f2)->engineData->ref.load(), 1 + refs_by_cache);

    f2.setKerning(!f.kerning());
    QVERIFY(QFontPrivate::get(f2) != QFontPrivate::get(f));
    QCOMPARE(QFontPrivate::get(f2)->ref.load(), 1);
    QVERIFY(QFontPrivate::get(f2)->engineData);
    QCOMPARE(QFontPrivate::get(f2)->engineData, QFontPrivate::get(f)->engineData);
    QCOMPARE(QFontPrivate::get(f2)->engineData->ref.load(), 2 + refs_by_cache);

    f2 = f;
    QCOMPARE(QFontPrivate::get(f2), QFontPrivate::get(f));
    QCOMPARE(QFontPrivate::get(f2)->ref.load(), 2);
    QVERIFY(QFontPrivate::get(f2)->engineData);
    QCOMPARE(QFontPrivate::get(f2)->engineData, QFontPrivate::get(f)->engineData);
    QCOMPARE(QFontPrivate::get(f2)->engineData->ref.load(), 1 + refs_by_cache);

    if (f.pointSize() > 0)
        f2.setPointSize(f.pointSize() * 2 / 3);
    else
        f2.setPixelSize(f.pixelSize() * 2 / 3);
    QVERIFY(QFontPrivate::get(f2) != QFontPrivate::get(f));
    QCOMPARE(QFontPrivate::get(f2)->ref.load(), 1);
    QVERIFY(!QFontPrivate::get(f2)->engineData);
    QVERIFY(QFontPrivate::get(f2)->engineData != QFontPrivate::get(f)->engineData);
}

QTEST_MAIN(tst_QFont)
#include "tst_qfont.moc"
