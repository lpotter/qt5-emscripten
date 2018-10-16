/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qunicodetools_p.h"

#include "qunicodetables_p.h"
#include "qvarlengtharray.h"

#include "qharfbuzz_p.h"

#define FLAG(x) (1 << (x))

QT_BEGIN_NAMESPACE

Q_AUTOTEST_EXPORT int qt_initcharattributes_default_algorithm_only = 0;

namespace QUnicodeTools {

// -----------------------------------------------------------------------------------------------------
//
// The text boundaries determination algorithm.
// See http://www.unicode.org/reports/tr29/tr29-31.html
//
// -----------------------------------------------------------------------------------------------------

namespace GB {

/*
 * Most grapheme break rules can be implemented table driven, but rules GB10, GB12 and GB13 need a bit
 * of special treatment.
 */
enum State : uchar {
    Break,
    Inside,
    GB10,
    GB10_2,
    GB10_3,
    GB13, // also covers GB12
};

static const State breakTable[QUnicodeTables::NumGraphemeBreakClasses][QUnicodeTables::NumGraphemeBreakClasses] = {
//    Any     CR      LF     Control  Extend  ZWJ     RI     Prepend  S-Mark  L       V       T       LV      LVT     E_B     E_M     GAZ     EBG
    { Break , Break , Break , Break , Inside, Inside, Break , Break , Inside, Break , Break , Break , Break , Break , Break , Break , Break , Break  }, // Any
    { Break , Break , Inside, Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break  }, // CR
    { Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break  }, // LF
    { Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break , Break  }, // Control
    { Break , Break , Break , Break , GB10_2, Inside, Break , Break , Inside, Break , Break , Break , Break , Break , Break , GB10_3, Break , Break  }, // Extend
    { Break , Break , Break , Break , Inside, Inside, Break , Break , Inside, Break , Break , Break , Break , Break , Break , Break , Inside, Inside }, // ZWJ
    { Break , Break , Break , Break , Inside, Inside, GB13  , Break , Inside, Break , Break , Break , Break , Break , Break , Break , Break , Break  }, // RegionalIndicator
    { Inside, Break , Break , Break , Inside, Inside, Inside, Inside, Inside, Inside, Inside, Inside, Inside, Inside, Inside, Inside, Inside, Inside }, // Prepend
    { Break , Break , Break , Break , Inside, Inside, Break , Break , Inside, Break , Break , Break , Break , Break , Break , Break , Break , Break  }, // SpacingMark
    { Break , Break , Break , Break , Inside, Inside, Break , Break , Inside, Inside, Inside, Break , Inside, Inside, Break , Break , Break , Break  }, // L
    { Break , Break , Break , Break , Inside, Inside, Break , Break , Inside, Break , Inside, Inside, Break , Break , Break , Break , Break , Break  }, // V
    { Break , Break , Break , Break , Inside, Inside, Break , Break , Inside, Break , Break , Inside, Break , Break , Break , Break , Break , Break  }, // T
    { Break , Break , Break , Break , Inside, Inside, Break , Break , Inside, Break , Inside, Inside, Break , Break , Break , Break , Break , Break  }, // LV
    { Break , Break , Break , Break , Inside, Inside, Break , Break , Inside, Break , Break , Inside, Break , Break , Break , Break , Break , Break  }, // LVT
    { Break , Break , Break , Break , GB10  , Inside, Break , Break , Inside, Break , Break , Break , Break , Break , Break , Inside, Break , Break  }, // E_B
    { Break , Break , Break , Break , Inside, Inside, Break , Break , Inside, Break , Break , Break , Break , Break , Break , Break , Break , Break  }, // E_M
    { Break , Break , Break , Break , Inside, Inside, Break , Break , Inside, Break , Break , Break , Break , Break , Break , Break , Break , Break  }, // GAZ
    { Break , Break , Break , Break , GB10  , Inside, Break , Break , Inside, Break , Break , Break , Break , Break , Break , Inside, Break , Break  }, // EBG
};

} // namespace GB

static void getGraphemeBreaks(const ushort *string, quint32 len, QCharAttributes *attributes)
{
    QUnicodeTables::GraphemeBreakClass lcls = QUnicodeTables::GraphemeBreak_LF; // to meet GB1
    GB::State state = GB::Break; // only required to track some of the rules
    for (quint32 i = 0; i != len; ++i) {
        quint32 pos = i;
        uint ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 != len) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ucs4);
        QUnicodeTables::GraphemeBreakClass cls = (QUnicodeTables::GraphemeBreakClass) prop->graphemeBreakClass;

        switch (GB::breakTable[lcls][cls]) {
        case GB::Break:
            attributes[pos].graphemeBoundary = true;
            state = GB::Break;
            break;
        case GB::Inside:
            state = GB::Break;
            break;
        case GB::GB10:
            state = GB::GB10;
            break;
        case GB::GB10_2:
            if (state == GB::GB10 || state == GB::GB10_2)
                state = GB::GB10_2;
            else
                state = GB::Break;
            break;
        case GB::GB10_3:
            if (state != GB::GB10 && state != GB::GB10_2)
                attributes[pos].graphemeBoundary = true;
            state = GB::Break;
            break;
        case GB::GB13:
            if (state != GB::GB13) {
                state = GB::GB13;
            } else {
                attributes[pos].graphemeBoundary = true;
                state = GB::Break;
            }
        }

        lcls = cls;
    }

    attributes[len].graphemeBoundary = true; // GB2
}


namespace WB {

enum Action {
    NoBreak,
    Break,
    Lookup,
    LookupW
};

static const uchar breakTable[QUnicodeTables::NumWordBreakClasses][QUnicodeTables::NumWordBreakClasses] = {
//    Any      CR       LF       Newline  Extend   ZWJ      Format    RI       Katakana HLetter  ALetter  SQuote   DQuote  MidNumLet MidLetter MidNum  Numeric ExtNumLet E_Base   E_Mod    GAZ      EBG
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // Any
    { Break  , Break  , NoBreak, Break  , Break  , Break  , Break  ,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // CR
    { Break  , Break  , Break  , Break  , Break  , Break  , Break  ,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // LF
    { Break  , Break  , Break  , Break  , Break  , Break  , Break  ,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // Newline
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // Extend
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , NoBreak, NoBreak }, // ZWJ
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // Format
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  NoBreak, Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // RegionalIndicator
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , NoBreak, Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , NoBreak, Break  , Break  , Break  , Break   }, // Katakana
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , NoBreak, NoBreak, LookupW, Lookup , LookupW, LookupW, Break  , NoBreak, NoBreak, Break  , Break  , Break  , Break   }, // HebrewLetter
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , NoBreak, NoBreak, LookupW, Break  , LookupW, LookupW, Break  , NoBreak, NoBreak, Break  , Break  , Break  , Break   }, // ALetter
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // SingleQuote
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // DoubleQuote
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // MidNumLet
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // MidLetter
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // MidNum
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , NoBreak, NoBreak, Lookup , Break  , Lookup , Break  , Lookup , NoBreak, NoBreak, Break  , Break  , Break  , Break   }, // Numeric
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , NoBreak, NoBreak, NoBreak, Break  , Break  , Break  , Break  , Break  , NoBreak, NoBreak, Break  , Break  , Break  , Break   }, // ExtendNumLet
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , NoBreak, Break  , Break   }, // E_Base
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // E_Mod
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // GAZ
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , NoBreak, Break  , Break   }, // EBG
};

} // namespace WB

static void getWordBreaks(const ushort *string, quint32 len, QCharAttributes *attributes)
{
    enum WordType {
        WordTypeNone, WordTypeAlphaNumeric, WordTypeHiraganaKatakana
    } currentWordType = WordTypeNone;

    QUnicodeTables::WordBreakClass cls = QUnicodeTables::WordBreak_LF; // to meet WB1
    for (quint32 i = 0; i != len; ++i) {
        quint32 pos = i;
        uint ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 != len) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ucs4);
        QUnicodeTables::WordBreakClass ncls = (QUnicodeTables::WordBreakClass) prop->wordBreakClass;
#ifdef QT_BUILD_INTERNAL
        if (qt_initcharattributes_default_algorithm_only) {
            // as of Unicode 5.1, some punctuation marks were mapped to MidLetter and MidNumLet
            // which caused "hi.there" to be treated like if it were just a single word;
            // we keep the pre-5.1 behavior by remapping these characters in the Unicode tables generator
            // and this code is needed to pass the coverage tests; remove once the issue is fixed.
            if (ucs4 == 0x002E) // FULL STOP
                ncls = QUnicodeTables::WordBreak_MidNumLet;
            else if (ucs4 == 0x003A) // COLON
                ncls = QUnicodeTables::WordBreak_MidLetter;
        }
#endif

        uchar action = WB::breakTable[cls][ncls];
        switch (action) {
        case WB::Break:
            break;
        case WB::NoBreak:
            if (Q_UNLIKELY(ncls == QUnicodeTables::WordBreak_Extend || ncls == QUnicodeTables::WordBreak_ZWJ || ncls == QUnicodeTables::WordBreak_Format)) {
                // WB4: X(Extend|Format)* -> X
                if (cls != QUnicodeTables::WordBreak_ZWJ) // WB3c
                    continue;
            }
            if (Q_UNLIKELY(cls == QUnicodeTables::WordBreak_RegionalIndicator)) {
                // WB15/WB16: break between pairs of Regional indicator
                ncls = QUnicodeTables::WordBreak_Any;
            }
            break;
        case WB::Lookup:
        case WB::LookupW:
            for (quint32 lookahead = i + 1; lookahead < len; ++lookahead) {
                ucs4 = string[lookahead];
                if (QChar::isHighSurrogate(ucs4) && lookahead + 1 != len) {
                    ushort low = string[lookahead + 1];
                    if (QChar::isLowSurrogate(low)) {
                        ucs4 = QChar::surrogateToUcs4(ucs4, low);
                        ++lookahead;
                    }
                }

                prop = QUnicodeTables::properties(ucs4);
                QUnicodeTables::WordBreakClass tcls = (QUnicodeTables::WordBreakClass) prop->wordBreakClass;

                if (Q_UNLIKELY(tcls == QUnicodeTables::WordBreak_Extend || tcls == QUnicodeTables::WordBreak_ZWJ || tcls == QUnicodeTables::WordBreak_Format)) {
                    // WB4: X(Extend|Format)* -> X
                    continue;
                }

                if (Q_LIKELY(tcls == cls || (action == WB::LookupW && (tcls == QUnicodeTables::WordBreak_HebrewLetter
                                                                       || tcls == QUnicodeTables::WordBreak_ALetter)))) {
                    i = lookahead;
                    ncls = tcls;
                    action = WB::NoBreak;
                }
                break;
            }
            if (action != WB::NoBreak) {
                action = WB::Break;
                if (Q_UNLIKELY(ncls == QUnicodeTables::WordBreak_SingleQuote && cls == QUnicodeTables::WordBreak_HebrewLetter))
                    action = WB::NoBreak; // WB7a
            }
            break;
        }

        cls = ncls;
        if (action == WB::Break) {
            attributes[pos].wordBreak = true;
            if (currentWordType != WordTypeNone)
                attributes[pos].wordEnd = true;
            switch (cls) {
            case QUnicodeTables::WordBreak_Katakana:
                currentWordType = WordTypeHiraganaKatakana;
                attributes[pos].wordStart = true;
                break;
            case QUnicodeTables::WordBreak_HebrewLetter:
            case QUnicodeTables::WordBreak_ALetter:
            case QUnicodeTables::WordBreak_Numeric:
                currentWordType = WordTypeAlphaNumeric;
                attributes[pos].wordStart = true;
                break;
            default:
                currentWordType = WordTypeNone;
                break;
            }
        }
    }

    if (currentWordType != WordTypeNone)
        attributes[len].wordEnd = true;
    attributes[len].wordBreak = true; // WB2
}


namespace SB {

enum State {
    Initial,
    Lower,
    Upper,
    LUATerm,
    ATerm,
    ATermC,
    ACS,
    STerm,
    STermC,
    SCS,
    BAfterC,
    BAfter,
    Break,
    Lookup
};

static const uchar breakTable[BAfter + 1][QUnicodeTables::NumSentenceBreakClasses] = {
//    Any      CR       LF       Sep      Extend   Sp       Lower    Upper    OLetter  Numeric  ATerm   SContinue STerm    Close
    { Initial, BAfterC, BAfter , BAfter , Initial, Initial, Lower  , Upper  , Initial, Initial, ATerm  , Initial, STerm  , Initial }, // Initial
    { Initial, BAfterC, BAfter , BAfter , Lower  , Initial, Initial, Initial, Initial, Initial, LUATerm, Initial, STerm  , Initial }, // Lower
    { Initial, BAfterC, BAfter , BAfter , Upper  , Initial, Initial, Upper  , Initial, Initial, LUATerm, STerm  , STerm  , Initial }, // Upper

    { Lookup , BAfterC, BAfter , BAfter , LUATerm, ACS    , Initial, Upper  , Break  , Initial, ATerm  , STerm  , STerm  , ATermC  }, // LUATerm
    { Lookup , BAfterC, BAfter , BAfter , ATerm  , ACS    , Initial, Break  , Break  , Initial, ATerm  , STerm  , STerm  , ATermC  }, // ATerm
    { Lookup , BAfterC, BAfter , BAfter , ATermC , ACS    , Initial, Break  , Break  , Lookup , ATerm  , STerm  , STerm  , ATermC  }, // ATermC
    { Lookup , BAfterC, BAfter , BAfter , ACS    , ACS    , Initial, Break  , Break  , Lookup , ATerm  , STerm  , STerm  , Lookup  }, // ACS

    { Break  , BAfterC, BAfter , BAfter , STerm  , SCS    , Break  , Break  , Break  , Break  , ATerm  , STerm  , STerm  , STermC  }, // STerm,
    { Break  , BAfterC, BAfter , BAfter , STermC , SCS    , Break  , Break  , Break  , Break  , ATerm  , STerm  , STerm  , STermC  }, // STermC
    { Break  , BAfterC, BAfter , BAfter , SCS    , SCS    , Break  , Break  , Break  , Break  , ATerm  , STerm  , STerm  , Break   }, // SCS
    { Break  , Break  , BAfter , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // BAfterC
    { Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // BAfter
};

} // namespace SB

static void getSentenceBreaks(const ushort *string, quint32 len, QCharAttributes *attributes)
{
    uchar state = SB::BAfter; // to meet SB1
    for (quint32 i = 0; i != len; ++i) {
        quint32 pos = i;
        uint ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 != len) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ucs4);
        QUnicodeTables::SentenceBreakClass ncls = (QUnicodeTables::SentenceBreakClass) prop->sentenceBreakClass;

        Q_ASSERT(state <= SB::BAfter);
        state = SB::breakTable[state][ncls];
        if (Q_UNLIKELY(state == SB::Lookup)) { // SB8
            state = SB::Break;
            for (quint32 lookahead = i + 1; lookahead < len; ++lookahead) {
                ucs4 = string[lookahead];
                if (QChar::isHighSurrogate(ucs4) && lookahead + 1 != len) {
                    ushort low = string[lookahead + 1];
                    if (QChar::isLowSurrogate(low)) {
                        ucs4 = QChar::surrogateToUcs4(ucs4, low);
                        ++lookahead;
                    }
                }

                prop = QUnicodeTables::properties(ucs4);
                QUnicodeTables::SentenceBreakClass tcls = (QUnicodeTables::SentenceBreakClass) prop->sentenceBreakClass;
                switch (tcls) {
                case QUnicodeTables::SentenceBreak_Any:
                case QUnicodeTables::SentenceBreak_Extend:
                case QUnicodeTables::SentenceBreak_Sp:
                case QUnicodeTables::SentenceBreak_Numeric:
                case QUnicodeTables::SentenceBreak_SContinue:
                case QUnicodeTables::SentenceBreak_Close:
                    continue;
                case QUnicodeTables::SentenceBreak_Lower:
                    i = lookahead;
                    state = SB::Initial;
                    break;
                default:
                    break;
                }
                break;
            }
        }
        if (Q_UNLIKELY(state == SB::Break)) {
            attributes[pos].sentenceBoundary = true;
            state = SB::breakTable[SB::Initial][ncls];
        }
    }

    attributes[len].sentenceBoundary = true; // SB2
}


// -----------------------------------------------------------------------------------------------------
//
// The line breaking algorithm.
// See http://www.unicode.org/reports/tr14/tr14-39.html
//
// -----------------------------------------------------------------------------------------------------

namespace LB {

namespace NS { // Number Sequence

// LB25 recommends to not break lines inside numbers of the form
// described by the following regular expression:
//  (PR|PO)?(OP|HY)?NU(NU|SY|IS)*(CL|CP)?(PR|PO)?

enum Action {
    None,
    Start,
    Continue,
    Break
};

enum Class {
    XX,
    PRPO,
    OPHY,
    NU,
    SYIS,
    CLCP
};

static const uchar actionTable[CLCP + 1][CLCP + 1] = {
//     XX       PRPO      OPHY       NU       SYIS      CLCP
    { None    , Start   , Start   , Start   , None    , None     }, // XX
    { None    , Start   , Continue, Continue, None    , None     }, // PRPO
    { None    , Start   , Start   , Continue, None    , None     }, // OPHY
    { Break   , Break   , Break   , Continue, Continue, Continue }, // NU
    { Break   , Break   , Break   , Continue, Continue, Continue }, // SYIS
    { Break   , Continue, Break   , Break   , Break   , Break    }, // CLCP
};

inline Class toClass(QUnicodeTables::LineBreakClass lbc, QChar::Category category)
{
    switch (lbc) {
    case QUnicodeTables::LineBreak_AL:// case QUnicodeTables::LineBreak_AI:
        // resolve AI math symbols in numerical context to IS
        if (category == QChar::Symbol_Math)
            return SYIS;
        break;
    case QUnicodeTables::LineBreak_PR: case QUnicodeTables::LineBreak_PO:
        return PRPO;
    case QUnicodeTables::LineBreak_OP: case QUnicodeTables::LineBreak_HY:
        return OPHY;
    case QUnicodeTables::LineBreak_NU:
        return NU;
    case QUnicodeTables::LineBreak_SY: case QUnicodeTables::LineBreak_IS:
        return SYIS;
    case QUnicodeTables::LineBreak_CL: case QUnicodeTables::LineBreak_CP:
        return CLCP;
    default:
        break;
    }
    return XX;
}

} // namespace NS

/* In order to support the tailored implementation of LB25 properly
   the following changes were made in the pair table to allow breaks
   where the numeric expression doesn't match the template (i.e. [^NU](IS|SY)NU):
   (CL)(PO) from IB to DB
   (CP)(PO) from IB to DB
   (CL)(PR) from IB to DB
   (CP)(PR) from IB to DB
   (PO)(OP) from IB to DB
   (PR)(OP) from IB to DB
   (IS)(NU) from IB to DB
   (SY)(NU) from IB to DB
*/

/* In order to implementat LB21a properly a special rule HH has been introduced and
   the following changes were made in the pair table to disallow breaks after Hebrew + Hyphen:
   (HL)(HY|BA) from IB to CI
   (HY|BA)(!CB) from DB to HH
*/

enum Action {
    ProhibitedBreak, PB = ProhibitedBreak,
    DirectBreak, DB = DirectBreak,
    IndirectBreak, IB = IndirectBreak,
    CombiningIndirectBreak, CI = CombiningIndirectBreak,
    CombiningProhibitedBreak, CP = CombiningProhibitedBreak,
    ProhibitedBreakAfterHebrewPlusHyphen, HH = ProhibitedBreakAfterHebrewPlusHyphen
};

static const uchar breakTable[QUnicodeTables::LineBreak_SA][QUnicodeTables::LineBreak_SA] = {
/*         OP  CL  CP  QU  GL  NS  EX  SY  IS  PR  PO  NU  AL  HL  ID  IN  HY  BA  BB  B2  ZW  CM  WJ  H2  H3  JL  JV  JT  RI  CB  EB  EM  ZWJ*/
/* OP */ { PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, CP, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB },
/* CL */ { DB, PB, PB, IB, IB, PB, PB, PB, PB, DB, DB, DB, DB, DB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* CP */ { DB, PB, PB, IB, IB, PB, PB, PB, PB, DB, DB, IB, IB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* QU */ { PB, PB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB },
/* GL */ { IB, PB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB },
/* NS */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* EX */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* SY */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* IS */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, IB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* PR */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, DB, IB, IB, IB, IB, DB, IB, IB, DB, DB, PB, CI, PB, IB, IB, IB, IB, IB, DB, DB, IB, IB, IB },
/* PO */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, DB, IB, IB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* NU */ { IB, PB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* AL */ { IB, PB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* HL */ { IB, PB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, DB, IB, CI, CI, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* ID */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* IN */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* HY */ { HH, PB, PB, IB, HH, IB, PB, PB, PB, HH, HH, IB, HH, HH, HH, HH, IB, IB, HH, HH, PB, CI, PB, HH, HH, HH, HH, HH, HH, DB, DB, DB, IB },
/* BA */ { HH, PB, PB, IB, HH, IB, PB, PB, PB, HH, HH, HH, HH, HH, HH, HH, IB, IB, HH, HH, PB, CI, PB, HH, HH, HH, HH, HH, HH, DB, DB, DB, IB },
/* BB */ { IB, PB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB, IB, DB, IB, IB, IB },
/* B2 */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, DB, IB, IB, DB, PB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* ZW */ { DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* CM */ { IB, PB, PB, IB, IB, IB, PB, PB, PB, DB, DB, IB, IB, IB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* WJ */ { IB, PB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB },
/* H2 */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, IB, IB, DB, DB, DB, DB, IB },
/* H3 */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, IB, DB, DB, DB, DB, IB },
/* JL */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, IB, IB, IB, IB, DB, DB, DB, DB, DB, IB },
/* JV */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, IB, IB, DB, DB, DB, DB, IB },
/* JT */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, IB, DB, DB, DB, DB, IB },
/* RI */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, IB, DB, DB, DB, IB },
/* CB */ { DB, PB, PB, IB, IB, DB, PB, PB, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* EB */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, IB, IB },
/* EM */ { DB, PB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB },
/* ZWJ*/ { IB, PB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, IB, IB, IB }
};

// The following line break classes are not treated by the pair table
// and must be resolved outside:
//  AI, BK, CB, CJ, CR, LF, NL, SA, SG, SP, XX

} // namespace LB

static void getLineBreaks(const ushort *string, quint32 len, QCharAttributes *attributes)
{
    quint32 nestart = 0;
    LB::NS::Class nelast = LB::NS::XX;

    QUnicodeTables::LineBreakClass lcls = QUnicodeTables::LineBreak_LF; // to meet LB10
    QUnicodeTables::LineBreakClass cls = lcls;
    for (quint32 i = 0; i != len; ++i) {
        quint32 pos = i;
        uint ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 != len) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ucs4);
        QUnicodeTables::LineBreakClass ncls = (QUnicodeTables::LineBreakClass) prop->lineBreakClass;
        QUnicodeTables::LineBreakClass tcls;

        if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_SA)) {
            // LB1: resolve SA to AL, except of those that have Category Mn or Mc be resolved to CM
            static const int test = FLAG(QChar::Mark_NonSpacing) | FLAG(QChar::Mark_SpacingCombining);
            if (FLAG(prop->category) & test)
                ncls = QUnicodeTables::LineBreak_CM;
        }

        if (Q_UNLIKELY(lcls >= QUnicodeTables::LineBreak_CR)) {
            // LB4: BK!, LB5: (CRxLF|CR|LF|NL)!
            if (lcls > QUnicodeTables::LineBreak_CR || ncls != QUnicodeTables::LineBreak_LF)
                attributes[pos].lineBreak = attributes[pos].mandatoryBreak = true;
            if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_CM || ncls == QUnicodeTables::LineBreak_ZWJ)) {
                cls = QUnicodeTables::LineBreak_AL;
                goto next_no_cls_update;
            }
            goto next;
        }

        if (Q_UNLIKELY(ncls >= QUnicodeTables::LineBreak_SP)) {
            if (ncls > QUnicodeTables::LineBreak_SP)
                goto next; // LB6: x(BK|CR|LF|NL)
            goto next_no_cls_update; // LB7: xSP
        }

        if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_CM || ncls == QUnicodeTables::LineBreak_ZWJ)) {
            // LB9: treat CM that don't follows SP, BK, CR, LF, NL, or ZW as X
            if (lcls != QUnicodeTables::LineBreak_ZW && lcls < QUnicodeTables::LineBreak_SP)
                // don't update anything
                goto next_no_cls_update;
        }

        if (Q_UNLIKELY(lcls == QUnicodeTables::LineBreak_ZWJ)) {
            // LB8a: ZWJ x (ID | EB | EM)
            if (ncls == QUnicodeTables::LineBreak_ID || ncls == QUnicodeTables::LineBreak_EB || ncls == QUnicodeTables::LineBreak_EM)
                goto next;
        }

        // LB25: do not break lines inside numbers
        {
            LB::NS::Class necur = LB::NS::toClass(ncls, (QChar::Category)prop->category);
            switch (LB::NS::actionTable[nelast][necur]) {
            case LB::NS::Break:
                // do not change breaks before and after the expression
                for (quint32 j = nestart + 1; j < pos; ++j)
                    attributes[j].lineBreak = false;
                Q_FALLTHROUGH();
            case LB::NS::None:
                nelast = LB::NS::XX; // reset state
                break;
            case LB::NS::Start:
                nestart = i;
                Q_FALLTHROUGH();
            default:
                nelast = necur;
                break;
            }
        }

        if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_RI && lcls == QUnicodeTables::LineBreak_RI)) {
            // LB30a
            ncls = QUnicodeTables::LineBreak_SP;
            goto next;
        }

        // for South East Asian chars that require a complex analysis, the Unicode
        // standard recommends to treat them as AL. tailoring that do dictionary analysis can override
        if (Q_UNLIKELY(cls >= QUnicodeTables::LineBreak_SA))
            cls = QUnicodeTables::LineBreak_AL;

        tcls = cls;
        if (tcls == QUnicodeTables::LineBreak_CM)
            // LB10
            tcls = QUnicodeTables::LineBreak_AL;
        switch (LB::breakTable[tcls][ncls < QUnicodeTables::LineBreak_SA ? ncls : QUnicodeTables::LineBreak_AL]) {
        case LB::DirectBreak:
            attributes[pos].lineBreak = true;
            break;
        case LB::IndirectBreak:
            if (lcls == QUnicodeTables::LineBreak_SP)
                attributes[pos].lineBreak = true;
            break;
        case LB::CombiningIndirectBreak:
            if (lcls != QUnicodeTables::LineBreak_SP)
                goto next_no_cls_update;
            attributes[pos].lineBreak = true;
            break;
        case LB::CombiningProhibitedBreak:
            if (lcls != QUnicodeTables::LineBreak_SP)
                goto next_no_cls_update;
            break;
        case LB::ProhibitedBreakAfterHebrewPlusHyphen:
            if (lcls != QUnicodeTables::LineBreak_HL)
                attributes[pos].lineBreak = true;
            break;
        case LB::ProhibitedBreak:
            // nothing to do
        default:
            break;
        }

    next:
        cls = ncls;
    next_no_cls_update:
        lcls = ncls;
    }

    if (Q_UNLIKELY(LB::NS::actionTable[nelast][LB::NS::XX] == LB::NS::Break)) {
        // LB25: do not break lines inside numbers
        for (quint32 j = nestart + 1; j < len; ++j)
            attributes[j].lineBreak = false;
    }

    attributes[0].lineBreak = attributes[0].mandatoryBreak = false; // LB2
    attributes[len].lineBreak = attributes[len].mandatoryBreak = true; // LB3
}


static void getWhiteSpaces(const ushort *string, quint32 len, QCharAttributes *attributes)
{
    for (quint32 i = 0; i != len; ++i) {
        uint ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 != len) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        if (Q_UNLIKELY(QChar::isSpace(ucs4)))
            attributes[i].whiteSpace = true;
    }
}


Q_CORE_EXPORT void initCharAttributes(const ushort *string, int length,
                                      const ScriptItem *items, int numItems,
                                      QCharAttributes *attributes, CharAttributeOptions options)
{
    if (length <= 0)
        return;

    if (!(options & DontClearAttributes))
        ::memset(attributes, 0, (length + 1) * sizeof(QCharAttributes));

    if (options & GraphemeBreaks)
        getGraphemeBreaks(string, length, attributes);
    if (options & WordBreaks)
        getWordBreaks(string, length, attributes);
    if (options & SentenceBreaks)
        getSentenceBreaks(string, length, attributes);
    if (options & LineBreaks)
        getLineBreaks(string, length, attributes);
    if (options & WhiteSpaces)
        getWhiteSpaces(string, length, attributes);

    if (!qt_initcharattributes_default_algorithm_only) {
        if (!items || numItems <= 0)
            return;

        QVarLengthArray<HB_ScriptItem, 64> scriptItems;
        scriptItems.reserve(numItems);
        int start = 0;
        HB_Script startScript = script_to_hbscript(items[start].script);
        if (Q_UNLIKELY(startScript == HB_Script_Inherited))
            startScript = HB_Script_Common;
        for (int i = start + 1; i < numItems; ++i) {
            HB_Script script = script_to_hbscript(items[i].script);
            if (Q_LIKELY(script == startScript || script == HB_Script_Inherited))
                continue;
            Q_ASSERT(items[i].position > items[start].position);
            HB_ScriptItem item;
            item.pos = items[start].position;
            item.length = items[i].position - items[start].position;
            item.script = startScript;
            item.bidiLevel = 0; // unused
            scriptItems.append(item);
            start = i;
            startScript = script;
        }
        if (items[start].position + 1 < length) {
            HB_ScriptItem item;
            item.pos = items[start].position;
            item.length = length - items[start].position;
            item.script = startScript;
            item.bidiLevel = 0; // unused
            scriptItems.append(item);
        }
        Q_STATIC_ASSERT(sizeof(QCharAttributes) == sizeof(HB_CharAttributes));
        HB_GetTailoredCharAttributes(string, length,
                                     scriptItems.constData(), scriptItems.size(),
                                     reinterpret_cast<HB_CharAttributes *>(attributes));
    }
}


// ----------------------------------------------------------------------------
//
// The Unicode script property. See http://www.unicode.org/reports/tr24/tr24-24.html
//
// ----------------------------------------------------------------------------

Q_CORE_EXPORT void initScripts(const ushort *string, int length, uchar *scripts)
{
    int sor = 0;
    int eor = 0;
    uchar script = QChar::Script_Common;

    for (int i = 0; i < length; ++i, eor = i) {
        uint ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 < length) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ucs4);

        uchar nscript = prop->script;

        if (Q_LIKELY(nscript == script || nscript <= QChar::Script_Common))
            continue;

        // inherit preceding Common-s
        if (Q_UNLIKELY(script <= QChar::Script_Common)) {
            // also covers a case where the base character of Common script followed
            // by one or more combining marks of non-Inherited, non-Common script
            script = nscript;
            continue;
        }

        // Never break between a combining mark (gc= Mc, Mn or Me) and its base character.
        // Thus, a combining mark - whatever its script property value is - should inherit
        // the script property value of its base character.
        static const int test = (FLAG(QChar::Mark_NonSpacing) | FLAG(QChar::Mark_SpacingCombining) | FLAG(QChar::Mark_Enclosing));
        if (Q_UNLIKELY(FLAG(prop->category) & test))
            continue;

        Q_ASSERT(script > QChar::Script_Common);
        Q_ASSERT(sor < eor);
        ::memset(scripts + sor, script, (eor - sor) * sizeof(uchar));
        sor = eor;

        script = nscript;
    }

    Q_ASSERT(script >= QChar::Script_Common);
    Q_ASSERT(eor == length);
    ::memset(scripts + sor, script, (eor - sor) * sizeof(uchar));
}

} // namespace QUnicodeTools

QT_END_NAMESPACE
