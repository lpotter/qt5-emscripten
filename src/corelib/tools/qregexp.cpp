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

#include "qregexp.h"

#include "qalgorithms.h"
#include "qbitarray.h"
#include "qcache.h"
#include "qdatastream.h"
#include "qdebug.h"
#include "qhashfunctions.h"
#include "qlist.h"
#include "qmap.h"
#include "qmutex.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qstringmatcher.h"
#include "qvector.h"

#include <limits.h>
#include <algorithm>

QT_BEGIN_NAMESPACE

int qFindString(const QChar *haystack, int haystackLen, int from,
    const QChar *needle, int needleLen, Qt::CaseSensitivity cs);

// error strings for the regexp parser
#define RXERR_OK         QT_TRANSLATE_NOOP("QRegExp", "no error occurred")
#define RXERR_DISABLED   QT_TRANSLATE_NOOP("QRegExp", "disabled feature used")
#define RXERR_CHARCLASS  QT_TRANSLATE_NOOP("QRegExp", "bad char class syntax")
#define RXERR_LOOKAHEAD  QT_TRANSLATE_NOOP("QRegExp", "bad lookahead syntax")
#define RXERR_LOOKBEHIND QT_TRANSLATE_NOOP("QRegExp", "lookbehinds not supported, see QTBUG-2371")
#define RXERR_REPETITION QT_TRANSLATE_NOOP("QRegExp", "bad repetition syntax")
#define RXERR_OCTAL      QT_TRANSLATE_NOOP("QRegExp", "invalid octal value")
#define RXERR_LEFTDELIM  QT_TRANSLATE_NOOP("QRegExp", "missing left delim")
#define RXERR_END        QT_TRANSLATE_NOOP("QRegExp", "unexpected end")
#define RXERR_LIMIT      QT_TRANSLATE_NOOP("QRegExp", "met internal limit")
#define RXERR_INTERVAL   QT_TRANSLATE_NOOP("QRegExp", "invalid interval")
#define RXERR_CATEGORY   QT_TRANSLATE_NOOP("QRegExp", "invalid category")

/*!
    \class QRegExp
    \inmodule QtCore
    \reentrant
    \brief The QRegExp class provides pattern matching using regular expressions.

    \ingroup tools
    \ingroup shared

    \keyword regular expression

    A regular expression, or "regexp", is a pattern for matching
    substrings in a text. This is useful in many contexts, e.g.,

    \table
    \row \li Validation
         \li A regexp can test whether a substring meets some criteria,
         e.g. is an integer or contains no whitespace.
    \row \li Searching
         \li A regexp provides more powerful pattern matching than
         simple substring matching, e.g., match one of the words
         \e{mail}, \e{letter} or \e{correspondence}, but none of the
         words \e{email}, \e{mailman}, \e{mailer}, \e{letterbox}, etc.
     \row \li Search and Replace
         \li A regexp can replace all occurrences of a substring with a
         different substring, e.g., replace all occurrences of \e{&}
         with \e{\&amp;} except where the \e{&} is already followed by
         an \e{amp;}.
    \row \li String Splitting
         \li A regexp can be used to identify where a string should be
         split apart, e.g. splitting tab-delimited strings.
    \endtable

    A brief introduction to regexps is presented, a description of
    Qt's regexp language, some examples, and the function
    documentation itself. QRegExp is modeled on Perl's regexp
    language. It fully supports Unicode. QRegExp can also be used in a
    simpler, \e{wildcard mode} that is similar to the functionality
    found in command shells. The syntax rules used by QRegExp can be
    changed with setPatternSyntax(). In particular, the pattern syntax
    can be set to QRegExp::FixedString, which means the pattern to be
    matched is interpreted as a plain string, i.e., special characters
    (e.g., backslash) are not escaped.

    A good text on regexps is \e {Mastering Regular Expressions}
    (Third Edition) by Jeffrey E. F.  Friedl, ISBN 0-596-52812-4.

    \note In Qt 5, the new QRegularExpression class provides a Perl
    compatible implementation of regular expressions and is recommended
    in place of QRegExp.

    \tableofcontents

    \section1 Introduction

    Regexps are built up from expressions, quantifiers, and
    assertions. The simplest expression is a character, e.g. \b{x}
    or \b{5}. An expression can also be a set of characters
    enclosed in square brackets. \b{[ABCD]} will match an \b{A}
    or a \b{B} or a \b{C} or a \b{D}. We can write this same
    expression as \b{[A-D]}, and an expression to match any
    capital letter in the English alphabet is written as
    \b{[A-Z]}.

    A quantifier specifies the number of occurrences of an expression
    that must be matched. \b{x{1,1}} means match one and only one
    \b{x}. \b{x{1,5}} means match a sequence of \b{x}
    characters that contains at least one \b{x} but no more than
    five.

    Note that in general regexps cannot be used to check for balanced
    brackets or tags. For example, a regexp can be written to match an
    opening html \c{<b>} and its closing \c{</b>}, if the \c{<b>} tags
    are not nested, but if the \c{<b>} tags are nested, that same
    regexp will match an opening \c{<b>} tag with the wrong closing
    \c{</b>}.  For the fragment \c{<b>bold <b>bolder</b></b>}, the
    first \c{<b>} would be matched with the first \c{</b>}, which is
    not correct. However, it is possible to write a regexp that will
    match nested brackets or tags correctly, but only if the number of
    nesting levels is fixed and known. If the number of nesting levels
    is not fixed and known, it is impossible to write a regexp that
    will not fail.

    Suppose we want a regexp to match integers in the range 0 to 99.
    At least one digit is required, so we start with the expression
    \b{[0-9]{1,1}}, which matches a single digit exactly once. This
    regexp matches integers in the range 0 to 9. To match integers up
    to 99, increase the maximum number of occurrences to 2, so the
    regexp becomes \b{[0-9]{1,2}}. This regexp satisfies the
    original requirement to match integers from 0 to 99, but it will
    also match integers that occur in the middle of strings. If we
    want the matched integer to be the whole string, we must use the
    anchor assertions, \b{^} (caret) and \b{$} (dollar). When
    \b{^} is the first character in a regexp, it means the regexp
    must match from the beginning of the string. When \b{$} is the
    last character of the regexp, it means the regexp must match to
    the end of the string. The regexp becomes \b{^[0-9]{1,2}$}.
    Note that assertions, e.g. \b{^} and \b{$}, do not match
    characters but locations in the string.

    If you have seen regexps described elsewhere, they may have looked
    different from the ones shown here. This is because some sets of
    characters and some quantifiers are so common that they have been
    given special symbols to represent them. \b{[0-9]} can be
    replaced with the symbol \b{\\d}. The quantifier to match
    exactly one occurrence, \b{{1,1}}, can be replaced with the
    expression itself, i.e. \b{x{1,1}} is the same as \b{x}. So
    our 0 to 99 matcher could be written as \b{^\\d{1,2}$}. It can
    also be written \b{^\\d\\d{0,1}$}, i.e. \e{From the start of
    the string, match a digit, followed immediately by 0 or 1 digits}.
    In practice, it would be written as \b{^\\d\\d?$}. The \b{?}
    is shorthand for the quantifier \b{{0,1}}, i.e. 0 or 1
    occurrences. \b{?} makes an expression optional. The regexp
    \b{^\\d\\d?$} means \e{From the beginning of the string, match
    one digit, followed immediately by 0 or 1 more digit, followed
    immediately by end of string}.

    To write a regexp that matches one of the words 'mail' \e or
    'letter' \e or 'correspondence' but does not match words that
    contain these words, e.g., 'email', 'mailman', 'mailer', and
    'letterbox', start with a regexp that matches 'mail'. Expressed
    fully, the regexp is \b{m{1,1}a{1,1}i{1,1}l{1,1}}, but because
    a character expression is automatically quantified by
    \b{{1,1}}, we can simplify the regexp to \b{mail}, i.e., an
    'm' followed by an 'a' followed by an 'i' followed by an 'l'. Now
    we can use the vertical bar \b{|}, which means \b{or}, to
    include the other two words, so our regexp for matching any of the
    three words becomes \b{mail|letter|correspondence}. Match
    'mail' \b{or} 'letter' \b{or} 'correspondence'. While this
    regexp will match one of the three words we want to match, it will
    also match words we don't want to match, e.g., 'email'.  To
    prevent the regexp from matching unwanted words, we must tell it
    to begin and end the match at word boundaries. First we enclose
    our regexp in parentheses, \b{(mail|letter|correspondence)}.
    Parentheses group expressions together, and they identify a part
    of the regexp that we wish to \l{capturing text}{capture}.
    Enclosing the expression in parentheses allows us to use it as a
    component in more complex regexps. It also allows us to examine
    which of the three words was actually matched. To force the match
    to begin and end on word boundaries, we enclose the regexp in
    \b{\\b} \e{word boundary} assertions:
    \b{\\b(mail|letter|correspondence)\\b}.  Now the regexp means:
    \e{Match a word boundary, followed by the regexp in parentheses,
    followed by a word boundary}. The \b{\\b} assertion matches a
    \e position in the regexp, not a \e character. A word boundary is
    any non-word character, e.g., a space, newline, or the beginning
    or ending of a string.

    If we want to replace ampersand characters with the HTML entity
    \b{\&amp;}, the regexp to match is simply \b{\&}. But this
    regexp will also match ampersands that have already been converted
    to HTML entities. We want to replace only ampersands that are not
    already followed by \b{amp;}. For this, we need the negative
    lookahead assertion, \b{(?!}__\b{)}. The regexp can then be
    written as \b{\&(?!amp;)}, i.e. \e{Match an ampersand that is}
    \b{not} \e{followed by} \b{amp;}.

    If we want to count all the occurrences of 'Eric' and 'Eirik' in a
    string, two valid solutions are \b{\\b(Eric|Eirik)\\b} and
    \b{\\bEi?ri[ck]\\b}. The word boundary assertion '\\b' is
    required to avoid matching words that contain either name,
    e.g. 'Ericsson'. Note that the second regexp matches more
    spellings than we want: 'Eric', 'Erik', 'Eiric' and 'Eirik'.

    Some of the examples discussed above are implemented in the
    \l{#code-examples}{code examples} section.

    \target characters-and-abbreviations-for-sets-of-characters
    \section1 Characters and Abbreviations for Sets of Characters

    \table
    \header \li Element \li Meaning
    \row \li \b{c}
         \li A character represents itself unless it has a special
         regexp meaning. e.g. \b{c} matches the character \e c.
    \row \li \b{\\c}
         \li A character that follows a backslash matches the character
         itself, except as specified below. e.g., To match a literal
         caret at the beginning of a string, write \b{\\^}.
    \row \li \b{\\a}
         \li Matches the ASCII bell (BEL, 0x07).
    \row \li \b{\\f}
         \li Matches the ASCII form feed (FF, 0x0C).
    \row \li \b{\\n}
         \li Matches the ASCII line feed (LF, 0x0A, Unix newline).
    \row \li \b{\\r}
         \li Matches the ASCII carriage return (CR, 0x0D).
    \row \li \b{\\t}
         \li Matches the ASCII horizontal tab (HT, 0x09).
    \row \li \b{\\v}
         \li Matches the ASCII vertical tab (VT, 0x0B).
    \row \li \b{\\x\e{hhhh}}
         \li Matches the Unicode character corresponding to the
         hexadecimal number \e{hhhh} (between 0x0000 and 0xFFFF).
    \row \li \b{\\0\e{ooo}} (i.e., \\zero \e{ooo})
         \li matches the ASCII/Latin1 character for the octal number
         \e{ooo} (between 0 and 0377).
    \row \li \b{. (dot)}
         \li Matches any character (including newline).
    \row \li \b{\\d}
         \li Matches a digit (QChar::isDigit()).
    \row \li \b{\\D}
         \li Matches a non-digit.
    \row \li \b{\\s}
         \li Matches a whitespace character (QChar::isSpace()).
    \row \li \b{\\S}
         \li Matches a non-whitespace character.
    \row \li \b{\\w}
         \li Matches a word character (QChar::isLetterOrNumber(), QChar::isMark(), or '_').
    \row \li \b{\\W}
         \li Matches a non-word character.
    \row \li \b{\\\e{n}}
         \li The \e{n}-th backreference, e.g. \\1, \\2, etc.
    \endtable

    \b{Note:} The C++ compiler transforms backslashes in strings.
    To include a \b{\\} in a regexp, enter it twice, i.e. \c{\\}.
    To match the backslash character itself, enter it four times, i.e.
    \c{\\\\}.

    \target sets-of-characters
    \section1 Sets of Characters

    Square brackets mean match any character contained in the square
    brackets. The character set abbreviations described above can
    appear in a character set in square brackets. Except for the
    character set abbreviations and the following two exceptions,
    characters do not have special meanings in square brackets.

    \table
    \row \li \b{^}

         \li The caret negates the character set if it occurs as the
         first character (i.e. immediately after the opening square
         bracket). \b{[abc]} matches 'a' or 'b' or 'c', but
         \b{[^abc]} matches anything \e but 'a' or 'b' or 'c'.

    \row \li \b{-}

         \li The dash indicates a range of characters. \b{[W-Z]}
         matches 'W' or 'X' or 'Y' or 'Z'.

    \endtable

    Using the predefined character set abbreviations is more portable
    than using character ranges across platforms and languages. For
    example, \b{[0-9]} matches a digit in Western alphabets but
    \b{\\d} matches a digit in \e any alphabet.

    Note: In other regexp documentation, sets of characters are often
    called "character classes".

    \target quantifiers
    \section1 Quantifiers

    By default, an expression is automatically quantified by
    \b{{1,1}}, i.e. it should occur exactly once. In the following
    list, \b{\e {E}} stands for expression. An expression is a
    character, or an abbreviation for a set of characters, or a set of
    characters in square brackets, or an expression in parentheses.

    \table
    \row \li \b{\e {E}?}

         \li Matches zero or one occurrences of \e E. This quantifier
         means \e{The previous expression is optional}, because it
         will match whether or not the expression is found. \b{\e
         {E}?} is the same as \b{\e {E}{0,1}}. e.g., \b{dents?}
         matches 'dent' or 'dents'.

    \row \li \b{\e {E}+}

         \li Matches one or more occurrences of \e E. \b{\e {E}+} is
         the same as \b{\e {E}{1,}}. e.g., \b{0+} matches '0',
         '00', '000', etc.

    \row \li \b{\e {E}*}

         \li Matches zero or more occurrences of \e E. It is the same
         as \b{\e {E}{0,}}. The \b{*} quantifier is often used
         in error where \b{+} should be used. For example, if
         \b{\\s*$} is used in an expression to match strings that
         end in whitespace, it will match every string because
         \b{\\s*$} means \e{Match zero or more whitespaces followed
         by end of string}. The correct regexp to match strings that
         have at least one trailing whitespace character is
         \b{\\s+$}.

    \row \li \b{\e {E}{n}}

         \li Matches exactly \e n occurrences of \e E. \b{\e {E}{n}}
         is the same as repeating \e E \e n times. For example,
         \b{x{5}} is the same as \b{xxxxx}. It is also the same
         as \b{\e {E}{n,n}}, e.g. \b{x{5,5}}.

    \row \li \b{\e {E}{n,}}
         \li Matches at least \e n occurrences of \e E.

    \row \li \b{\e {E}{,m}}
         \li Matches at most \e m occurrences of \e E. \b{\e {E}{,m}}
         is the same as \b{\e {E}{0,m}}.

    \row \li \b{\e {E}{n,m}}
         \li Matches at least \e n and at most \e m occurrences of \e E.
    \endtable

    To apply a quantifier to more than just the preceding character,
    use parentheses to group characters together in an expression. For
    example, \b{tag+} matches a 't' followed by an 'a' followed by
    at least one 'g', whereas \b{(tag)+} matches at least one
    occurrence of 'tag'.

    Note: Quantifiers are normally "greedy". They always match as much
    text as they can. For example, \b{0+} matches the first zero it
    finds and all the consecutive zeros after the first zero. Applied
    to '20005', it matches '2\underline{000}5'. Quantifiers can be made
    non-greedy, see setMinimal().

    \target capturing parentheses
    \target backreferences
    \section1 Capturing Text

    Parentheses allow us to group elements together so that we can
    quantify and capture them. For example if we have the expression
    \b{mail|letter|correspondence} that matches a string we know
    that \e one of the words matched but not which one. Using
    parentheses allows us to "capture" whatever is matched within
    their bounds, so if we used \b{(mail|letter|correspondence)}
    and matched this regexp against the string "I sent you some email"
    we can use the cap() or capturedTexts() functions to extract the
    matched characters, in this case 'mail'.

    We can use captured text within the regexp itself. To refer to the
    captured text we use \e backreferences which are indexed from 1,
    the same as for cap(). For example we could search for duplicate
    words in a string using \b{\\b(\\w+)\\W+\\1\\b} which means match a
    word boundary followed by one or more word characters followed by
    one or more non-word characters followed by the same text as the
    first parenthesized expression followed by a word boundary.

    If we want to use parentheses purely for grouping and not for
    capturing we can use the non-capturing syntax, e.g.
    \b{(?:green|blue)}. Non-capturing parentheses begin '(?:' and
    end ')'. In this example we match either 'green' or 'blue' but we
    do not capture the match so we only know whether or not we matched
    but not which color we actually found. Using non-capturing
    parentheses is more efficient than using capturing parentheses
    since the regexp engine has to do less book-keeping.

    Both capturing and non-capturing parentheses may be nested.

    \target greedy quantifiers

    For historical reasons, quantifiers (e.g. \b{*}) that apply to
    capturing parentheses are more "greedy" than other quantifiers.
    For example, \b{a*(a*)} will match "aaa" with cap(1) == "aaa".
    This behavior is different from what other regexp engines do
    (notably, Perl). To obtain a more intuitive capturing behavior,
    specify QRegExp::RegExp2 to the QRegExp constructor or call
    setPatternSyntax(QRegExp::RegExp2).

    \target cap_in_a_loop

    When the number of matches cannot be determined in advance, a
    common idiom is to use cap() in a loop. For example:

    \snippet code/src_corelib_tools_qregexp.cpp 0

    \target assertions
    \section1 Assertions

    Assertions make some statement about the text at the point where
    they occur in the regexp but they do not match any characters. In
    the following list \b{\e {E}} stands for any expression.

    \table
    \row \li \b{^}
         \li The caret signifies the beginning of the string. If you
         wish to match a literal \c{^} you must escape it by
         writing \c{\\^}. For example, \b{^#include} will only
         match strings which \e begin with the characters '#include'.
         (When the caret is the first character of a character set it
         has a special meaning, see \l{#sets-of-characters}{Sets of Characters}.)

    \row \li \b{$}
         \li The dollar signifies the end of the string. For example
         \b{\\d\\s*$} will match strings which end with a digit
         optionally followed by whitespace. If you wish to match a
         literal \c{$} you must escape it by writing
         \c{\\$}.

    \row \li \b{\\b}
         \li A word boundary. For example the regexp
         \b{\\bOK\\b} means match immediately after a word
         boundary (e.g. start of string or whitespace) the letter 'O'
         then the letter 'K' immediately before another word boundary
         (e.g. end of string or whitespace). But note that the
         assertion does not actually match any whitespace so if we
         write \b{(\\bOK\\b)} and we have a match it will only
         contain 'OK' even if the string is "It's \underline{OK} now".

    \row \li \b{\\B}
         \li A non-word boundary. This assertion is true wherever
         \b{\\b} is false. For example if we searched for
         \b{\\Bon\\B} in "Left on" the match would fail (space
         and end of string aren't non-word boundaries), but it would
         match in "t\underline{on}ne".

    \row \li \b{(?=\e E)}
         \li Positive lookahead. This assertion is true if the
         expression matches at this point in the regexp. For example,
         \b{const(?=\\s+char)} matches 'const' whenever it is
         followed by 'char', as in 'static \underline{const} char *'.
         (Compare with \b{const\\s+char}, which matches 'static
         \underline{const char} *'.)

    \row \li \b{(?!\e E)}
         \li Negative lookahead. This assertion is true if the
         expression does not match at this point in the regexp. For
         example, \b{const(?!\\s+char)} matches 'const' \e except
         when it is followed by 'char'.
    \endtable

    \target QRegExp wildcard matching
    \section1 Wildcard Matching

    Most command shells such as \e bash or \e cmd.exe support "file
    globbing", the ability to identify a group of files by using
    wildcards. The setPatternSyntax() function is used to switch
    between regexp and wildcard mode. Wildcard matching is much
    simpler than full regexps and has only four features:

    \table
    \row \li \b{c}
         \li Any character represents itself apart from those mentioned
         below. Thus \b{c} matches the character \e c.
    \row \li \b{?}
         \li Matches any single character. It is the same as
         \b{.} in full regexps.
    \row \li \b{*}
         \li Matches zero or more of any characters. It is the
         same as \b{.*} in full regexps.
    \row \li \b{[...]}
         \li Sets of characters can be represented in square brackets,
         similar to full regexps. Within the character class, like
         outside, backslash has no special meaning.
    \endtable

    In the mode Wildcard, the wildcard characters cannot be
    escaped. In the mode WildcardUnix, the character '\\' escapes the
    wildcard.

    For example if we are in wildcard mode and have strings which
    contain filenames we could identify HTML files with \b{*.html}.
    This will match zero or more characters followed by a dot followed
    by 'h', 't', 'm' and 'l'.

    To test a string against a wildcard expression, use exactMatch().
    For example:

    \snippet code/src_corelib_tools_qregexp.cpp 1

    \target perl-users
    \section1 Notes for Perl Users

    Most of the character class abbreviations supported by Perl are
    supported by QRegExp, see \l{#characters-and-abbreviations-for-sets-of-characters}
    {characters and abbreviations for sets of characters}.

    In QRegExp, apart from within character classes, \c{^} always
    signifies the start of the string, so carets must always be
    escaped unless used for that purpose. In Perl the meaning of caret
    varies automagically depending on where it occurs so escaping it
    is rarely necessary. The same applies to \c{$} which in
    QRegExp always signifies the end of the string.

    QRegExp's quantifiers are the same as Perl's greedy quantifiers
    (but see the \l{greedy quantifiers}{note above}). Non-greedy
    matching cannot be applied to individual quantifiers, but can be
    applied to all the quantifiers in the pattern. For example, to
    match the Perl regexp \b{ro+?m} requires:

    \snippet code/src_corelib_tools_qregexp.cpp 2

    The equivalent of Perl's \c{/i} option is
    setCaseSensitivity(Qt::CaseInsensitive).

    Perl's \c{/g} option can be emulated using a \l{#cap_in_a_loop}{loop}.

    In QRegExp \b{.} matches any character, therefore all QRegExp
    regexps have the equivalent of Perl's \c{/s} option. QRegExp
    does not have an equivalent to Perl's \c{/m} option, but this
    can be emulated in various ways for example by splitting the input
    into lines or by looping with a regexp that searches for newlines.

    Because QRegExp is string oriented, there are no \\A, \\Z, or \\z
    assertions. The \\G assertion is not supported but can be emulated
    in a loop.

    Perl's $& is cap(0) or capturedTexts()[0]. There are no QRegExp
    equivalents for $`, $' or $+. Perl's capturing variables, $1, $2,
    ... correspond to cap(1) or capturedTexts()[1], cap(2) or
    capturedTexts()[2], etc.

    To substitute a pattern use QString::replace().

    Perl's extended \c{/x} syntax is not supported, nor are
    directives, e.g. (?i), or regexp comments, e.g. (?#comment). On
    the other hand, C++'s rules for literal strings can be used to
    achieve the same:

    \snippet code/src_corelib_tools_qregexp.cpp 3

    Both zero-width positive and zero-width negative lookahead
    assertions (?=pattern) and (?!pattern) are supported with the same
    syntax as Perl. Perl's lookbehind assertions, "independent"
    subexpressions and conditional expressions are not supported.

    Non-capturing parentheses are also supported, with the same
    (?:pattern) syntax.

    See QString::split() and QStringList::join() for equivalents
    to Perl's split and join functions.

    Note: because C++ transforms \\'s they must be written \e twice in
    code, e.g. \b{\\b} must be written \b{\\\\b}.

    \target code-examples
    \section1 Code Examples

    \snippet code/src_corelib_tools_qregexp.cpp 4

    The third string matches '\underline{6}'. This is a simple validation
    regexp for integers in the range 0 to 99.

    \snippet code/src_corelib_tools_qregexp.cpp 5

    The second string matches '\underline{This_is-OK}'. We've used the
    character set abbreviation '\\S' (non-whitespace) and the anchors
    to match strings which contain no whitespace.

    In the following example we match strings containing 'mail' or
    'letter' or 'correspondence' but only match whole words i.e. not
    'email'

    \snippet code/src_corelib_tools_qregexp.cpp 6

    The second string matches "Please write the \underline{letter}". The
    word 'letter' is also captured (because of the parentheses). We
    can see what text we've captured like this:

    \snippet code/src_corelib_tools_qregexp.cpp 7

    This will capture the text from the first set of capturing
    parentheses (counting capturing left parentheses from left to
    right). The parentheses are counted from 1 since cap(0) is the
    whole matched regexp (equivalent to '&' in most regexp engines).

    \snippet code/src_corelib_tools_qregexp.cpp 8

    Here we've passed the QRegExp to QString's replace() function to
    replace the matched text with new text.

    \snippet code/src_corelib_tools_qregexp.cpp 9

    We've used the indexIn() function to repeatedly match the regexp in
    the string. Note that instead of moving forward by one character
    at a time \c pos++ we could have written \c {pos +=
    rx.matchedLength()} to skip over the already matched string. The
    count will equal 3, matching 'One \underline{Eric} another
    \underline{Eirik}, and an Ericsson. How many Eiriks, \underline{Eric}?'; it
    doesn't match 'Ericsson' or 'Eiriks' because they are not bounded
    by non-word boundaries.

    One common use of regexps is to split lines of delimited data into
    their component fields.

    \snippet code/src_corelib_tools_qregexp.cpp 10

    In this example our input lines have the format company name, web
    address and country. Unfortunately the regexp is rather long and
    not very versatile -- the code will break if we add any more
    fields. A simpler and better solution is to look for the
    separator, '\\t' in this case, and take the surrounding text. The
    QString::split() function can take a separator string or regexp
    as an argument and split a string accordingly.

    \snippet code/src_corelib_tools_qregexp.cpp 11

    Here field[0] is the company, field[1] the web address and so on.

    To imitate the matching of a shell we can use wildcard mode.

    \snippet code/src_corelib_tools_qregexp.cpp 12

    Wildcard matching can be convenient because of its simplicity, but
    any wildcard regexp can be defined using full regexps, e.g.
    \b{.*\\.html$}. Notice that we can't match both \c .html and \c
    .htm files with a wildcard unless we use \b{*.htm*} which will
    also match 'test.html.bak'. A full regexp gives us the precision
    we need, \b{.*\\.html?$}.

    QRegExp can match case insensitively using setCaseSensitivity(),
    and can use non-greedy matching, see setMinimal(). By
    default QRegExp uses full regexps but this can be changed with
    setPatternSyntax(). Searching can be done forward with indexIn() or backward
    with lastIndexIn(). Captured text can be accessed using
    capturedTexts() which returns a string list of all captured
    strings, or using cap() which returns the captured string for the
    given index. The pos() function takes a match index and returns
    the position in the string where the match was made (or -1 if
    there was no match).

    \sa QString, QStringList, QRegExpValidator, QSortFilterProxyModel,
        {tools/regexp}{Regular Expression Example}
*/

#if defined(Q_OS_VXWORKS) && defined(EOS)
#  undef EOS
#endif

const int NumBadChars = 64;
#define BadChar(ch) ((ch).unicode() % NumBadChars)

const int NoOccurrence = INT_MAX;
const int EmptyCapture = INT_MAX;
const int InftyLen = INT_MAX;
const int InftyRep = 1025;
const int EOS = -1;

static bool isWord(QChar ch)
{
    return ch.isLetterOrNumber() || ch.isMark() || ch == QLatin1Char('_');
}

/*
  Merges two vectors of ints and puts the result into the first
  one.
*/
static void mergeInto(QVector<int> *a, const QVector<int> &b)
{
    int asize = a->size();
    int bsize = b.size();
    if (asize == 0) {
        *a = b;
#ifndef QT_NO_REGEXP_OPTIM
    } else if (bsize == 1 && a->at(asize - 1) < b.at(0)) {
        a->resize(asize + 1);
        (*a)[asize] = b.at(0);
#endif
    } else if (bsize >= 1) {
        int csize = asize + bsize;
        QVector<int> c(csize);
        int i = 0, j = 0, k = 0;
        while (i < asize) {
            if (j < bsize) {
                if (a->at(i) == b.at(j)) {
                    ++i;
                    --csize;
                } else if (a->at(i) < b.at(j)) {
                    c[k++] = a->at(i++);
                } else {
                    c[k++] = b.at(j++);
                }
            } else {
                memcpy(c.data() + k, a->constData() + i, (asize - i) * sizeof(int));
                break;
            }
        }
        c.resize(csize);
        if (j < bsize)
            memcpy(c.data() + k, b.constData() + j, (bsize - j) * sizeof(int));
        *a = c;
    }
}

#ifndef QT_NO_REGEXP_WILDCARD
/*
  Translates a wildcard pattern to an equivalent regular expression
  pattern (e.g., *.cpp to .*\.cpp).

  If enableEscaping is true, it is possible to escape the wildcard
  characters with \
*/
static QString wc2rx(const QString &wc_str, const bool enableEscaping)
{
    const int wclen = wc_str.length();
    QString rx;
    int i = 0;
    bool isEscaping = false; // the previous character is '\'
    const QChar *wc = wc_str.unicode();

    while (i < wclen) {
        const QChar c = wc[i++];
        switch (c.unicode()) {
        case '\\':
            if (enableEscaping) {
                if (isEscaping) {
                    rx += QLatin1String("\\\\");
                } // we insert the \\ later if necessary
                if (i == wclen) { // the end
                    rx += QLatin1String("\\\\");
                }
            } else {
                rx += QLatin1String("\\\\");
            }
            isEscaping = true;
            break;
        case '*':
            if (isEscaping) {
                rx += QLatin1String("\\*");
                isEscaping = false;
            } else {
                rx += QLatin1String(".*");
            }
            break;
        case '?':
            if (isEscaping) {
                rx += QLatin1String("\\?");
                isEscaping = false;
            } else {
                rx += QLatin1Char('.');
            }

            break;
        case '$':
        case '(':
        case ')':
        case '+':
        case '.':
        case '^':
        case '{':
        case '|':
        case '}':
            if (isEscaping) {
                isEscaping = false;
                rx += QLatin1String("\\\\");
            }
            rx += QLatin1Char('\\');
            rx += c;
            break;
         case '[':
            if (isEscaping) {
                isEscaping = false;
                rx += QLatin1String("\\[");
            } else {
                rx += c;
                if (wc[i] == QLatin1Char('^'))
                    rx += wc[i++];
                if (i < wclen) {
                    if (rx[i] == QLatin1Char(']'))
                        rx += wc[i++];
                    while (i < wclen && wc[i] != QLatin1Char(']')) {
                        if (wc[i] == QLatin1Char('\\'))
                            rx += QLatin1Char('\\');
                        rx += wc[i++];
                    }
                }
            }
             break;

        case ']':
            if(isEscaping){
                isEscaping = false;
                rx += QLatin1String("\\");
            }
            rx += c;
            break;

        default:
            if(isEscaping){
                isEscaping = false;
                rx += QLatin1String("\\\\");
            }
            rx += c;
        }
    }
    return rx;
}
#endif

static int caretIndex(int offset, QRegExp::CaretMode caretMode)
{
    if (caretMode == QRegExp::CaretAtZero) {
        return 0;
    } else if (caretMode == QRegExp::CaretAtOffset) {
        return offset;
    } else { // QRegExp::CaretWontMatch
        return -1;
    }
}

/*
    The QRegExpEngineKey struct uniquely identifies an engine.
*/
struct QRegExpEngineKey
{
    QString pattern;
    QRegExp::PatternSyntax patternSyntax;
    Qt::CaseSensitivity cs;

    inline QRegExpEngineKey(const QString &pattern, QRegExp::PatternSyntax patternSyntax,
                            Qt::CaseSensitivity cs)
        : pattern(pattern), patternSyntax(patternSyntax), cs(cs) {}

    inline void clear() {
        pattern.clear();
        patternSyntax = QRegExp::RegExp;
        cs = Qt::CaseSensitive;
    }
};

static bool operator==(const QRegExpEngineKey &key1, const QRegExpEngineKey &key2)
{
    return key1.pattern == key2.pattern && key1.patternSyntax == key2.patternSyntax
           && key1.cs == key2.cs;
}

static uint qHash(const QRegExpEngineKey &key, uint seed = 0) Q_DECL_NOTHROW
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.pattern);
    seed = hash(seed, key.patternSyntax);
    seed = hash(seed, key.cs);
    return seed;
}

class QRegExpEngine;

//Q_DECLARE_TYPEINFO(QVector<int>, Q_MOVABLE_TYPE);

/*
  This is the engine state during matching.
*/
struct QRegExpMatchState
{
    const QChar *in; // a pointer to the input string data
    int pos; // the current position in the string
    int caretPos;
    int len; // the length of the input string
    bool minimal; // minimal matching?
    int *bigArray; // big array holding the data for the next pointers
    int *inNextStack; // is state is nextStack?
    int *curStack; // stack of current states
    int *nextStack; // stack of next states
    int *curCapBegin; // start of current states' captures
    int *nextCapBegin; // start of next states' captures
    int *curCapEnd; // end of current states' captures
    int *nextCapEnd; // end of next states' captures
    int *tempCapBegin; // start of temporary captures
    int *tempCapEnd; // end of temporary captures
    int *capBegin; // start of captures for a next state
    int *capEnd; // end of captures for a next state
    int *slideTab; // bump-along slide table for bad-character heuristic
    int *captured; // what match() returned last
    int slideTabSize; // size of slide table
    int capturedSize;
#ifndef QT_NO_REGEXP_BACKREF
    QList<QVector<int> > sleeping; // list of back-reference sleepers
#endif
    int matchLen; // length of match
    int oneTestMatchedLen; // length of partial match

    const QRegExpEngine *eng;

    inline QRegExpMatchState() : bigArray(0), captured(0) {}
    inline ~QRegExpMatchState() { free(bigArray); }

    void drain() { free(bigArray); bigArray = 0; captured = 0; } // to save memory
    void prepareForMatch(QRegExpEngine *eng);
    void match(const QChar *str, int len, int pos, bool minimal,
        bool oneTest, int caretIndex);
    bool matchHere();
    bool testAnchor(int i, int a, const int *capBegin);
};

/*
  The struct QRegExpAutomatonState represents one state in a modified NFA. The
  input characters matched are stored in the state instead of on
  the transitions, something possible for an automaton
  constructed from a regular expression.
*/
struct QRegExpAutomatonState
{
#ifndef QT_NO_REGEXP_CAPTURE
    int atom; // which atom does this state belong to?
#endif
    int match; // what does it match? (see CharClassBit and BackRefBit)
    QVector<int> outs; // out-transitions
    QMap<int, int> reenter; // atoms reentered when transiting out
    QMap<int, int> anchors; // anchors met when transiting out

    inline QRegExpAutomatonState() { }
#ifndef QT_NO_REGEXP_CAPTURE
    inline QRegExpAutomatonState(int a, int m)
        : atom(a), match(m) { }
#else
    inline QRegExpAutomatonState(int m)
        : match(m) { }
#endif
};

Q_DECLARE_TYPEINFO(QRegExpAutomatonState, Q_MOVABLE_TYPE);

/*
  The struct QRegExpCharClassRange represents a range of characters (e.g.,
  [0-9] denotes range 48 to 57).
*/
struct QRegExpCharClassRange
{
    ushort from; // 48
    ushort len; // 10
};

Q_DECLARE_TYPEINFO(QRegExpCharClassRange, Q_PRIMITIVE_TYPE);

#ifndef QT_NO_REGEXP_CAPTURE
/*
  The struct QRegExpAtom represents one node in the hierarchy of regular
  expression atoms.
*/
struct QRegExpAtom
{
    enum { NoCapture = -1, OfficialCapture = -2, UnofficialCapture = -3 };

    int parent; // index of parent in array of atoms
    int capture; // index of capture, from 1 to ncap - 1
};

Q_DECLARE_TYPEINFO(QRegExpAtom, Q_PRIMITIVE_TYPE);
#endif

struct QRegExpLookahead;

#ifndef QT_NO_REGEXP_ANCHOR_ALT
/*
  The struct QRegExpAnchorAlternation represents a pair of anchors with
  OR semantics.
*/
struct QRegExpAnchorAlternation
{
    int a; // this anchor...
    int b; // ...or this one
};

Q_DECLARE_TYPEINFO(QRegExpAnchorAlternation, Q_PRIMITIVE_TYPE);
#endif

#ifndef QT_NO_REGEXP_CCLASS

#define FLAG(x) (1 << (x))
/*
  The class QRegExpCharClass represents a set of characters, such as can
  be found in regular expressions (e.g., [a-z] denotes the set
  {a, b, ..., z}).
*/
class QRegExpCharClass
{
public:
    QRegExpCharClass();

    void clear();
    bool negative() const { return n; }
    void setNegative(bool negative);
    void addCategories(uint cats);
    void addRange(ushort from, ushort to);
    void addSingleton(ushort ch) { addRange(ch, ch); }

    bool in(QChar ch) const;
#ifndef QT_NO_REGEXP_OPTIM
    const QVector<int> &firstOccurrence() const { return occ1; }
#endif

#if defined(QT_DEBUG)
    void dump() const;
#endif

private:
    QVector<QRegExpCharClassRange> r; // character ranges
#ifndef QT_NO_REGEXP_OPTIM
    QVector<int> occ1; // first-occurrence array
#endif
    uint c; // character classes
    bool n; // negative?
};
#else
struct QRegExpCharClass
{
    int dummy;

#ifndef QT_NO_REGEXP_OPTIM
    QRegExpCharClass() { occ1.fill(0, NumBadChars); }

    const QVector<int> &firstOccurrence() const { return occ1; }
    QVector<int> occ1;
#endif
};
#endif

Q_DECLARE_TYPEINFO(QRegExpCharClass, Q_MOVABLE_TYPE);

/*
  The QRegExpEngine class encapsulates a modified nondeterministic
  finite automaton (NFA).
*/
class QRegExpEngine
{
public:
    QRegExpEngine(Qt::CaseSensitivity cs, bool greedyQuantifiers)
        : cs(cs), greedyQuantifiers(greedyQuantifiers) { setup(); }

    QRegExpEngine(const QRegExpEngineKey &key);
    ~QRegExpEngine();

    bool isValid() const { return valid; }
    const QString &errorString() const { return yyError; }
    int captureCount() const { return officialncap; }

    int createState(QChar ch);
    int createState(const QRegExpCharClass &cc);
#ifndef QT_NO_REGEXP_BACKREF
    int createState(int bref);
#endif

    void addCatTransitions(const QVector<int> &from, const QVector<int> &to);
#ifndef QT_NO_REGEXP_CAPTURE
    void addPlusTransitions(const QVector<int> &from, const QVector<int> &to, int atom);
#endif

#ifndef QT_NO_REGEXP_ANCHOR_ALT
    int anchorAlternation(int a, int b);
    int anchorConcatenation(int a, int b);
#else
    int anchorAlternation(int a, int b) { return a & b; }
    int anchorConcatenation(int a, int b) { return a | b; }
#endif
    void addAnchors(int from, int to, int a);

#ifndef QT_NO_REGEXP_OPTIM
    void heuristicallyChooseHeuristic();
#endif

#if defined(QT_DEBUG)
    void dump() const;
#endif

    QAtomicInt ref;

private:
    enum { CharClassBit = 0x10000, BackRefBit = 0x20000 };
    enum { InitialState = 0, FinalState = 1 };

    void setup();
    int setupState(int match);

    /*
      Let's hope that 13 lookaheads and 14 back-references are
      enough.
     */
    enum { MaxLookaheads = 13, MaxBackRefs = 14 };
    enum { Anchor_Dollar = 0x00000001, Anchor_Caret = 0x00000002, Anchor_Word = 0x00000004,
           Anchor_NonWord = 0x00000008, Anchor_FirstLookahead = 0x00000010,
           Anchor_BackRef1Empty = Anchor_FirstLookahead << MaxLookaheads,
           Anchor_BackRef0Empty = Anchor_BackRef1Empty >> 1,
           Anchor_Alternation = unsigned(Anchor_BackRef1Empty) << MaxBackRefs,

           Anchor_LookaheadMask = (Anchor_FirstLookahead - 1) ^
                   ((Anchor_FirstLookahead << MaxLookaheads) - 1) };
#ifndef QT_NO_REGEXP_CAPTURE
    int startAtom(bool officialCapture);
    void finishAtom(int atom, bool needCapture);
#endif

#ifndef QT_NO_REGEXP_LOOKAHEAD
    int addLookahead(QRegExpEngine *eng, bool negative);
#endif

#ifndef QT_NO_REGEXP_OPTIM
    bool goodStringMatch(QRegExpMatchState &matchState) const;
    bool badCharMatch(QRegExpMatchState &matchState) const;
#else
    bool bruteMatch(QRegExpMatchState &matchState) const;
#endif

    QVector<QRegExpAutomatonState> s; // array of states
#ifndef QT_NO_REGEXP_CAPTURE
    QVector<QRegExpAtom> f; // atom hierarchy
    int nf; // number of atoms
    int cf; // current atom
    QVector<int> captureForOfficialCapture;
#endif
    int officialncap; // number of captures, seen from the outside
    int ncap; // number of captures, seen from the inside
#ifndef QT_NO_REGEXP_CCLASS
    QVector<QRegExpCharClass> cl; // array of character classes
#endif
#ifndef QT_NO_REGEXP_LOOKAHEAD
    QVector<QRegExpLookahead *> ahead; // array of lookaheads
#endif
#ifndef QT_NO_REGEXP_ANCHOR_ALT
    QVector<QRegExpAnchorAlternation> aa; // array of (a, b) pairs of anchors
#endif
#ifndef QT_NO_REGEXP_OPTIM
    bool caretAnchored; // does the regexp start with ^?
    bool trivial; // is the good-string all that needs to match?
#endif
    bool valid; // is the regular expression valid?
    Qt::CaseSensitivity cs; // case sensitive?
    bool greedyQuantifiers; // RegExp2?
    bool xmlSchemaExtensions;
#ifndef QT_NO_REGEXP_BACKREF
    int nbrefs; // number of back-references
#endif

#ifndef QT_NO_REGEXP_OPTIM
    bool useGoodStringHeuristic; // use goodStringMatch? otherwise badCharMatch

    int goodEarlyStart; // the index where goodStr can first occur in a match
    int goodLateStart; // the index where goodStr can last occur in a match
    QString goodStr; // the string that any match has to contain

    int minl; // the minimum length of a match
    QVector<int> occ1; // first-occurrence array
#endif

    /*
      The class Box is an abstraction for a regular expression
      fragment. It can also be seen as one node in the syntax tree of
      a regular expression with synthetized attributes.

      Its interface is ugly for performance reasons.
    */
    class Box
    {
    public:
        Box(QRegExpEngine *engine);
        Box(const Box &b) { operator=(b); }

        Box &operator=(const Box &b);

        void clear() { operator=(Box(eng)); }
        void set(QChar ch);
        void set(const QRegExpCharClass &cc);
#ifndef QT_NO_REGEXP_BACKREF
        void set(int bref);
#endif

        void cat(const Box &b);
        void orx(const Box &b);
        void plus(int atom);
        void opt();
        void catAnchor(int a);
#ifndef QT_NO_REGEXP_OPTIM
        void setupHeuristics();
#endif

#if defined(QT_DEBUG)
        void dump() const;
#endif

    private:
        void addAnchorsToEngine(const Box &to) const;

        QRegExpEngine *eng; // the automaton under construction
        QVector<int> ls; // the left states (firstpos)
        QVector<int> rs; // the right states (lastpos)
        QMap<int, int> lanchors; // the left anchors
        QMap<int, int> ranchors; // the right anchors
        int skipanchors; // the anchors to match if the box is skipped

#ifndef QT_NO_REGEXP_OPTIM
        int earlyStart; // the index where str can first occur
        int lateStart; // the index where str can last occur
        QString str; // a string that has to occur in any match
        QString leftStr; // a string occurring at the left of this box
        QString rightStr; // a string occurring at the right of this box
        int maxl; // the maximum length of this box (possibly InftyLen)
#endif

        int minl; // the minimum length of this box
#ifndef QT_NO_REGEXP_OPTIM
        QVector<int> occ1; // first-occurrence array
#endif
    };

    friend class Box;

    /*
      This is the lexical analyzer for regular expressions.
    */
    enum { Tok_Eos, Tok_Dollar, Tok_LeftParen, Tok_MagicLeftParen, Tok_PosLookahead,
           Tok_NegLookahead, Tok_RightParen, Tok_CharClass, Tok_Caret, Tok_Quantifier, Tok_Bar,
           Tok_Word, Tok_NonWord, Tok_Char = 0x10000, Tok_BackRef = 0x20000 };
    int getChar();
    int getEscape();
#ifndef QT_NO_REGEXP_INTERVAL
    int getRep(int def);
#endif
#ifndef QT_NO_REGEXP_LOOKAHEAD
    void skipChars(int n);
#endif
    void error(const char *msg);
    void startTokenizer(const QChar *rx, int len);
    int getToken();

    const QChar *yyIn; // a pointer to the input regular expression pattern
    int yyPos0; // the position of yyTok in the input pattern
    int yyPos; // the position of the next character to read
    int yyLen; // the length of yyIn
    int yyCh; // the last character read
    QScopedPointer<QRegExpCharClass> yyCharClass; // attribute for Tok_CharClass tokens
    int yyMinRep; // attribute for Tok_Quantifier
    int yyMaxRep; // ditto
    QString yyError; // syntax error or overflow during parsing?

    /*
      This is the syntactic analyzer for regular expressions.
    */
    int parse(const QChar *rx, int len);
    void parseAtom(Box *box);
    void parseFactor(Box *box);
    void parseTerm(Box *box);
    void parseExpression(Box *box);

    int yyTok; // the last token read
    bool yyMayCapture; // set this to false to disable capturing

    friend struct QRegExpMatchState;
};

#ifndef QT_NO_REGEXP_LOOKAHEAD
/*
  The struct QRegExpLookahead represents a lookahead a la Perl (e.g.,
  (?=foo) and (?!bar)).
*/
struct QRegExpLookahead
{
    QRegExpEngine *eng; // NFA representing the embedded regular expression
    bool neg; // negative lookahead?

    inline QRegExpLookahead(QRegExpEngine *eng0, bool neg0)
        : eng(eng0), neg(neg0) { }
    inline ~QRegExpLookahead() { delete eng; }
};
#endif

/*!
    \internal
    convert the pattern string to the RegExp syntax.

    This is also used by QScriptEngine::newRegExp to convert to a pattern that JavaScriptCore can understan
 */
Q_CORE_EXPORT QString qt_regexp_toCanonical(const QString &pattern, QRegExp::PatternSyntax patternSyntax)
{
    switch (patternSyntax) {
#ifndef QT_NO_REGEXP_WILDCARD
    case QRegExp::Wildcard:
        return wc2rx(pattern, false);
    case QRegExp::WildcardUnix:
        return wc2rx(pattern, true);
#endif
    case QRegExp::FixedString:
        return QRegExp::escape(pattern);
    case QRegExp::W3CXmlSchema11:
    default:
        return pattern;
    }
}

QRegExpEngine::QRegExpEngine(const QRegExpEngineKey &key)
    : cs(key.cs), greedyQuantifiers(key.patternSyntax == QRegExp::RegExp2),
      xmlSchemaExtensions(key.patternSyntax == QRegExp::W3CXmlSchema11)
{
    setup();

    QString rx = qt_regexp_toCanonical(key.pattern, key.patternSyntax);

    valid = (parse(rx.unicode(), rx.length()) == rx.length());
    if (!valid) {
#ifndef QT_NO_REGEXP_OPTIM
        trivial = false;
#endif
        error(RXERR_LEFTDELIM);
    }
}

QRegExpEngine::~QRegExpEngine()
{
#ifndef QT_NO_REGEXP_LOOKAHEAD
    qDeleteAll(ahead);
#endif
}

void QRegExpMatchState::prepareForMatch(QRegExpEngine *eng)
{
    /*
      We use one QVector<int> for all the big data used a lot in
      matchHere() and friends.
    */
    int ns = eng->s.size(); // number of states
    int ncap = eng->ncap;
#ifndef QT_NO_REGEXP_OPTIM
    int newSlideTabSize = qMax(eng->minl + 1, 16);
#else
    int newSlideTabSize = 0;
#endif
    int numCaptures = eng->captureCount();
    int newCapturedSize = 2 + 2 * numCaptures;
    bigArray = q_check_ptr((int *)realloc(bigArray, ((3 + 4 * ncap) * ns + 4 * ncap + newSlideTabSize + newCapturedSize)*sizeof(int)));

    // set all internal variables only _after_ bigArray is realloc'ed
    // to prevent a broken regexp in oom case

    slideTabSize = newSlideTabSize;
    capturedSize = newCapturedSize;
    inNextStack = bigArray;
    memset(inNextStack, -1, ns * sizeof(int));
    curStack = inNextStack + ns;
    nextStack = inNextStack + 2 * ns;

    curCapBegin = inNextStack + 3 * ns;
    nextCapBegin = curCapBegin + ncap * ns;
    curCapEnd = curCapBegin + 2 * ncap * ns;
    nextCapEnd = curCapBegin + 3 * ncap * ns;

    tempCapBegin = curCapBegin + 4 * ncap * ns;
    tempCapEnd = tempCapBegin + ncap;
    capBegin = tempCapBegin + 2 * ncap;
    capEnd = tempCapBegin + 3 * ncap;

    slideTab = tempCapBegin + 4 * ncap;
    captured = slideTab + slideTabSize;
    memset(captured, -1, capturedSize*sizeof(int));
    this->eng = eng;
}

/*
  Tries to match in str and returns an array of (begin, length) pairs
  for captured text. If there is no match, all pairs are (-1, -1).
*/
void QRegExpMatchState::match(const QChar *str0, int len0, int pos0,
    bool minimal0, bool oneTest, int caretIndex)
{
    bool matched = false;
    QChar char_null;

#ifndef QT_NO_REGEXP_OPTIM
    if (eng->trivial && !oneTest) {
        pos = qFindString(str0, len0, pos0, eng->goodStr.unicode(), eng->goodStr.length(), eng->cs);
        matchLen = eng->goodStr.length();
        matched = (pos != -1);
    } else
#endif
    {
        in = str0;
        if (in == 0)
            in = &char_null;
        pos = pos0;
        caretPos = caretIndex;
        len = len0;
        minimal = minimal0;
        matchLen = 0;
        oneTestMatchedLen = 0;

        if (eng->valid && pos >= 0 && pos <= len) {
#ifndef QT_NO_REGEXP_OPTIM
            if (oneTest) {
                matched = matchHere();
            } else {
                if (pos <= len - eng->minl) {
                    if (eng->caretAnchored) {
                        matched = matchHere();
                    } else if (eng->useGoodStringHeuristic) {
                        matched = eng->goodStringMatch(*this);
                    } else {
                        matched = eng->badCharMatch(*this);
                    }
                }
            }
#else
            matched = oneTest ? matchHere() : eng->bruteMatch(*this);
#endif
        }
    }

    if (matched) {
        int *c = captured;
        *c++ = pos;
        *c++ = matchLen;

        int numCaptures = (capturedSize - 2) >> 1;
#ifndef QT_NO_REGEXP_CAPTURE
        for (int i = 0; i < numCaptures; ++i) {
            int j = eng->captureForOfficialCapture.at(i);
            if (capBegin[j] != EmptyCapture) {
                int len = capEnd[j] - capBegin[j];
                *c++ = (len > 0) ? pos + capBegin[j] : 0;
                *c++ = len;
            } else {
                *c++ = -1;
                *c++ = -1;
            }
        }
#endif
    } else {
        // we rely on 2's complement here
        memset(captured, -1, capturedSize * sizeof(int));
    }
}

/*
  The three following functions add one state to the automaton and
  return the number of the state.
*/

int QRegExpEngine::createState(QChar ch)
{
    return setupState(ch.unicode());
}

int QRegExpEngine::createState(const QRegExpCharClass &cc)
{
#ifndef QT_NO_REGEXP_CCLASS
    int n = cl.size();
    cl += QRegExpCharClass(cc);
    return setupState(CharClassBit | n);
#else
    Q_UNUSED(cc);
    return setupState(CharClassBit);
#endif
}

#ifndef QT_NO_REGEXP_BACKREF
int QRegExpEngine::createState(int bref)
{
    if (bref > nbrefs) {
        nbrefs = bref;
        if (nbrefs > MaxBackRefs) {
            error(RXERR_LIMIT);
            return 0;
        }
    }
    return setupState(BackRefBit | bref);
}
#endif

/*
  The two following functions add a transition between all pairs of
  states (i, j) where i is found in from, and j is found in to.

  Cat-transitions are distinguished from plus-transitions for
  capturing.
*/

void QRegExpEngine::addCatTransitions(const QVector<int> &from, const QVector<int> &to)
{
    for (int i = 0; i < from.size(); i++)
        mergeInto(&s[from.at(i)].outs, to);
}

#ifndef QT_NO_REGEXP_CAPTURE
void QRegExpEngine::addPlusTransitions(const QVector<int> &from, const QVector<int> &to, int atom)
{
    for (int i = 0; i < from.size(); i++) {
        QRegExpAutomatonState &st = s[from.at(i)];
        const QVector<int> oldOuts = st.outs;
        mergeInto(&st.outs, to);
        if (f.at(atom).capture != QRegExpAtom::NoCapture) {
            for (int j = 0; j < to.size(); j++) {
                // ### st.reenter.contains(to.at(j)) check looks suspicious
                if (!st.reenter.contains(to.at(j)) &&
                     !std::binary_search(oldOuts.constBegin(), oldOuts.constEnd(), to.at(j)))
                    st.reenter.insert(to.at(j), atom);
            }
        }
    }
}
#endif

#ifndef QT_NO_REGEXP_ANCHOR_ALT
/*
  Returns an anchor that means a OR b.
*/
int QRegExpEngine::anchorAlternation(int a, int b)
{
    if (((a & b) == a || (a & b) == b) && ((a | b) & Anchor_Alternation) == 0)
        return a & b;

    int n = aa.size();
#ifndef QT_NO_REGEXP_OPTIM
    if (n > 0 && aa.at(n - 1).a == a && aa.at(n - 1).b == b)
        return Anchor_Alternation | (n - 1);
#endif

    QRegExpAnchorAlternation element = {a, b};
    aa.append(element);
    return Anchor_Alternation | n;
}

/*
  Returns an anchor that means a AND b.
*/
int QRegExpEngine::anchorConcatenation(int a, int b)
{
    if (((a | b) & Anchor_Alternation) == 0)
        return a | b;
    if ((b & Anchor_Alternation) != 0)
        qSwap(a, b);

    int aprime = anchorConcatenation(aa.at(a ^ Anchor_Alternation).a, b);
    int bprime = anchorConcatenation(aa.at(a ^ Anchor_Alternation).b, b);
    return anchorAlternation(aprime, bprime);
}
#endif

/*
  Adds anchor a on a transition caracterised by its from state and
  its to state.
*/
void QRegExpEngine::addAnchors(int from, int to, int a)
{
    QRegExpAutomatonState &st = s[from];
    if (st.anchors.contains(to))
        a = anchorAlternation(st.anchors.value(to), a);
    st.anchors.insert(to, a);
}

#ifndef QT_NO_REGEXP_OPTIM
/*
  This function chooses between the good-string and the bad-character
  heuristics. It computes two scores and chooses the heuristic with
  the highest score.

  Here are some common-sense constraints on the scores that should be
  respected if the formulas are ever modified: (1) If goodStr is
  empty, the good-string heuristic scores 0. (2) If the regular
  expression is trivial, the good-string heuristic should be used.
  (3) If the search is case insensitive, the good-string heuristic
  should be used, unless it scores 0. (Case insensitivity turns all
  entries of occ1 to 0.) (4) If (goodLateStart - goodEarlyStart) is
  big, the good-string heuristic should score less.
*/
void QRegExpEngine::heuristicallyChooseHeuristic()
{
    if (minl == 0) {
        useGoodStringHeuristic = false;
    } else if (trivial) {
        useGoodStringHeuristic = true;
    } else {
        /*
          Magic formula: The good string has to constitute a good
          proportion of the minimum-length string, and appear at a
          more-or-less known index.
        */
        int goodStringScore = (64 * goodStr.length() / minl) -
                              (goodLateStart - goodEarlyStart);
        /*
          Less magic formula: We pick some characters at random, and
          check whether they are good or bad.
        */
        int badCharScore = 0;
        int step = qMax(1, NumBadChars / 32);
        for (int i = 1; i < NumBadChars; i += step) {
            if (occ1.at(i) == NoOccurrence)
                badCharScore += minl;
            else
                badCharScore += occ1.at(i);
        }
        badCharScore /= minl;
        useGoodStringHeuristic = (goodStringScore > badCharScore);
    }
}
#endif

#if defined(QT_DEBUG)
void QRegExpEngine::dump() const
{
    int i, j;
    qDebug("Case %ssensitive engine", cs ? "" : "in");
    qDebug("  States");
    for (i = 0; i < s.size(); i++) {
        qDebug("  %d%s", i, i == InitialState ? " (initial)" : i == FinalState ? " (final)" : "");
#ifndef QT_NO_REGEXP_CAPTURE
        if (nf > 0)
            qDebug("    in atom %d", s[i].atom);
#endif
        int m = s[i].match;
        if ((m & CharClassBit) != 0) {
            qDebug("    match character class %d", m ^ CharClassBit);
#ifndef QT_NO_REGEXP_CCLASS
            cl[m ^ CharClassBit].dump();
#else
            qDebug("    negative character class");
#endif
        } else if ((m & BackRefBit) != 0) {
            qDebug("    match back-reference %d", m ^ BackRefBit);
        } else if (m >= 0x20 && m <= 0x7e) {
            qDebug("    match 0x%.4x (%c)", m, m);
        } else {
            qDebug("    match 0x%.4x", m);
        }
        for (j = 0; j < s[i].outs.size(); j++) {
            int next = s[i].outs[j];
            qDebug("    -> %d", next);
            if (s[i].reenter.contains(next))
                qDebug("       [reenter %d]", s[i].reenter[next]);
            if (s[i].anchors.value(next) != 0)
                qDebug("       [anchors 0x%.8x]", s[i].anchors[next]);
        }
    }
#ifndef QT_NO_REGEXP_CAPTURE
    if (nf > 0) {
        qDebug("  Atom    Parent  Capture");
        for (i = 0; i < nf; i++) {
            if (f[i].capture == QRegExpAtom::NoCapture) {
                qDebug("  %6d  %6d     nil", i, f[i].parent);
            } else {
                int cap = f[i].capture;
                bool official = captureForOfficialCapture.contains(cap);
                qDebug("  %6d  %6d  %6d  %s", i, f[i].parent, f[i].capture,
                       official ? "official" : "");
            }
        }
    }
#endif
#ifndef QT_NO_REGEXP_ANCHOR_ALT
    for (i = 0; i < aa.size(); i++)
        qDebug("  Anchor alternation 0x%.8x: 0x%.8x 0x%.9x", i, aa[i].a, aa[i].b);
#endif
}
#endif

void QRegExpEngine::setup()
{
    ref.store(1);
#ifndef QT_NO_REGEXP_CAPTURE
    f.resize(32);
    nf = 0;
    cf = -1;
#endif
    officialncap = 0;
    ncap = 0;
#ifndef QT_NO_REGEXP_OPTIM
    caretAnchored = true;
    trivial = true;
#endif
    valid = false;
#ifndef QT_NO_REGEXP_BACKREF
    nbrefs = 0;
#endif
#ifndef QT_NO_REGEXP_OPTIM
    useGoodStringHeuristic = true;
    minl = 0;
    occ1.fill(0, NumBadChars);
#endif
}

int QRegExpEngine::setupState(int match)
{
#ifndef QT_NO_REGEXP_CAPTURE
    s += QRegExpAutomatonState(cf, match);
#else
    s += QRegExpAutomatonState(match);
#endif
    return s.size() - 1;
}

#ifndef QT_NO_REGEXP_CAPTURE
/*
  Functions startAtom() and finishAtom() should be called to delimit
  atoms. When a state is created, it is assigned to the current atom.
  The information is later used for capturing.
*/
int QRegExpEngine::startAtom(bool officialCapture)
{
    if ((nf & (nf + 1)) == 0 && nf + 1 >= f.size())
        f.resize((nf + 1) << 1);
    f[nf].parent = cf;
    cf = nf++;
    f[cf].capture = officialCapture ? QRegExpAtom::OfficialCapture : QRegExpAtom::NoCapture;
    return cf;
}

void QRegExpEngine::finishAtom(int atom, bool needCapture)
{
    if (greedyQuantifiers && needCapture && f[atom].capture == QRegExpAtom::NoCapture)
        f[atom].capture = QRegExpAtom::UnofficialCapture;
    cf = f.at(atom).parent;
}
#endif

#ifndef QT_NO_REGEXP_LOOKAHEAD
/*
  Creates a lookahead anchor.
*/
int QRegExpEngine::addLookahead(QRegExpEngine *eng, bool negative)
{
    int n = ahead.size();
    if (n == MaxLookaheads) {
        error(RXERR_LIMIT);
        return 0;
    }
    ahead += new QRegExpLookahead(eng, negative);
    return Anchor_FirstLookahead << n;
}
#endif

#ifndef QT_NO_REGEXP_CAPTURE
/*
  We want the longest leftmost captures.
*/
static bool isBetterCapture(int ncap, const int *begin1, const int *end1, const int *begin2,
                            const int *end2)
{
    for (int i = 0; i < ncap; i++) {
        int delta = begin2[i] - begin1[i]; // it has to start early...
        if (delta == 0)
            delta = end1[i] - end2[i]; // ...and end late

        if (delta != 0)
            return delta > 0;
    }
    return false;
}
#endif

/*
  Returns \c true if anchor a matches at position pos + i in the input
  string, otherwise false.
*/
bool QRegExpMatchState::testAnchor(int i, int a, const int *capBegin)
{
    int j;

#ifndef QT_NO_REGEXP_ANCHOR_ALT
    if ((a & QRegExpEngine::Anchor_Alternation) != 0)
        return testAnchor(i, eng->aa.at(a ^ QRegExpEngine::Anchor_Alternation).a, capBegin)
               || testAnchor(i, eng->aa.at(a ^ QRegExpEngine::Anchor_Alternation).b, capBegin);
#endif

    if ((a & QRegExpEngine::Anchor_Caret) != 0) {
        if (pos + i != caretPos)
            return false;
    }
    if ((a & QRegExpEngine::Anchor_Dollar) != 0) {
        if (pos + i != len)
            return false;
    }
#ifndef QT_NO_REGEXP_ESCAPE
    if ((a & (QRegExpEngine::Anchor_Word | QRegExpEngine::Anchor_NonWord)) != 0) {
        bool before = false;
        bool after = false;
        if (pos + i != 0)
            before = isWord(in[pos + i - 1]);
        if (pos + i != len)
            after = isWord(in[pos + i]);
        if ((a & QRegExpEngine::Anchor_Word) != 0 && (before == after))
            return false;
        if ((a & QRegExpEngine::Anchor_NonWord) != 0 && (before != after))
            return false;
    }
#endif
#ifndef QT_NO_REGEXP_LOOKAHEAD
    if ((a & QRegExpEngine::Anchor_LookaheadMask) != 0) {
        const QVector<QRegExpLookahead *> &ahead = eng->ahead;
        for (j = 0; j < ahead.size(); j++) {
            if ((a & (QRegExpEngine::Anchor_FirstLookahead << j)) != 0) {
                QRegExpMatchState matchState;
                matchState.prepareForMatch(ahead[j]->eng);
                matchState.match(in + pos + i, len - pos - i, 0,
                    true, true, caretPos - pos - i);
                if ((matchState.captured[0] == 0) == ahead[j]->neg)
                    return false;
            }
        }
    }
#endif
#ifndef QT_NO_REGEXP_CAPTURE
#ifndef QT_NO_REGEXP_BACKREF
    for (j = 0; j < eng->nbrefs; j++) {
        if ((a & (QRegExpEngine::Anchor_BackRef1Empty << j)) != 0) {
            int i = eng->captureForOfficialCapture.at(j);
            if (capBegin[i] != EmptyCapture)
                return false;
        }
    }
#endif
#endif
    return true;
}

#ifndef QT_NO_REGEXP_OPTIM
/*
  The three following functions are what Jeffrey Friedl would call
  transmissions (or bump-alongs). Using one or the other should make
  no difference except in performance.
*/

bool QRegExpEngine::goodStringMatch(QRegExpMatchState &matchState) const
{
    int k = matchState.pos + goodEarlyStart;
    QStringMatcher matcher(goodStr.unicode(), goodStr.length(), cs);
    while ((k = matcher.indexIn(matchState.in, matchState.len, k)) != -1) {
        int from = k - goodLateStart;
        int to = k - goodEarlyStart;
        if (from > matchState.pos)
            matchState.pos = from;

        while (matchState.pos <= to) {
            if (matchState.matchHere())
                return true;
            ++matchState.pos;
        }
        ++k;
    }
    return false;
}

bool QRegExpEngine::badCharMatch(QRegExpMatchState &matchState) const
{
    int slideHead = 0;
    int slideNext = 0;
    int i;
    int lastPos = matchState.len - minl;
    memset(matchState.slideTab, 0, matchState.slideTabSize * sizeof(int));

    /*
      Set up the slide table, used for the bad-character heuristic,
      using the table of first occurrence of each character.
    */
    for (i = 0; i < minl; i++) {
        int sk = occ1[BadChar(matchState.in[matchState.pos + i])];
        if (sk == NoOccurrence)
            sk = i + 1;
        if (sk > 0) {
            int k = i + 1 - sk;
            if (k < 0) {
                sk = i + 1;
                k = 0;
            }
            if (sk > matchState.slideTab[k])
                matchState.slideTab[k] = sk;
        }
    }

    if (matchState.pos > lastPos)
        return false;

    for (;;) {
        if (++slideNext >= matchState.slideTabSize)
            slideNext = 0;
        if (matchState.slideTab[slideHead] > 0) {
            if (matchState.slideTab[slideHead] - 1 > matchState.slideTab[slideNext])
                matchState.slideTab[slideNext] = matchState.slideTab[slideHead] - 1;
            matchState.slideTab[slideHead] = 0;
        } else {
            if (matchState.matchHere())
                return true;
        }

        if (matchState.pos == lastPos)
            break;

        /*
          Update the slide table. This code has much in common with
          the initialization code.
        */
        int sk = occ1[BadChar(matchState.in[matchState.pos + minl])];
        if (sk == NoOccurrence) {
            matchState.slideTab[slideNext] = minl;
        } else if (sk > 0) {
            int k = slideNext + minl - sk;
            if (k >= matchState.slideTabSize)
                k -= matchState.slideTabSize;
            if (sk > matchState.slideTab[k])
                matchState.slideTab[k] = sk;
        }
        slideHead = slideNext;
        ++matchState.pos;
    }
    return false;
}
#else
bool QRegExpEngine::bruteMatch(QRegExpMatchState &matchState) const
{
    while (matchState.pos <= matchState.len) {
        if (matchState.matchHere())
            return true;
        ++matchState.pos;
    }
    return false;
}
#endif

/*
  Here's the core of the engine. It tries to do a match here and now.
*/
bool QRegExpMatchState::matchHere()
{
    int ncur = 1, nnext = 0;
    int i = 0, j, k, m;
    bool stop = false;

    matchLen = -1;
    oneTestMatchedLen = -1;
    curStack[0] = QRegExpEngine::InitialState;

    int ncap = eng->ncap;
#ifndef QT_NO_REGEXP_CAPTURE
    if (ncap > 0) {
        for (j = 0; j < ncap; j++) {
            curCapBegin[j] = EmptyCapture;
            curCapEnd[j] = EmptyCapture;
        }
    }
#endif

#ifndef QT_NO_REGEXP_BACKREF
    while ((ncur > 0 || !sleeping.isEmpty()) && i <= len - pos && !stop)
#else
    while (ncur > 0 && i <= len - pos && !stop)
#endif
    {
        int ch = (i < len - pos) ? in[pos + i].unicode() : 0;
        for (j = 0; j < ncur; j++) {
            int cur = curStack[j];
            const QRegExpAutomatonState &scur = eng->s.at(cur);
            const QVector<int> &outs = scur.outs;
            for (k = 0; k < outs.size(); k++) {
                int next = outs.at(k);
                const QRegExpAutomatonState &snext = eng->s.at(next);
                bool inside = true;
#if !defined(QT_NO_REGEXP_BACKREF) && !defined(QT_NO_REGEXP_CAPTURE)
                int needSomeSleep = 0;
#endif

                /*
                  First, check if the anchors are anchored properly.
                */
                int a = scur.anchors.value(next);
                if (a != 0 && !testAnchor(i, a, curCapBegin + j * ncap))
                    inside = false;

                /*
                  If indeed they are, check if the input character is
                  correct for this transition.
                */
                if (inside) {
                    m = snext.match;
                    if ((m & (QRegExpEngine::CharClassBit | QRegExpEngine::BackRefBit)) == 0) {
                        if (eng->cs)
                            inside = (m == ch);
                        else
                            inside = (QChar(m).toLower() == QChar(ch).toLower());
                    } else if (next == QRegExpEngine::FinalState) {
                        matchLen = i;
                        stop = minimal;
                        inside = true;
                    } else if ((m & QRegExpEngine::CharClassBit) != 0) {
#ifndef QT_NO_REGEXP_CCLASS
                        const QRegExpCharClass &cc = eng->cl.at(m ^ QRegExpEngine::CharClassBit);
                        if (eng->cs)
                            inside = cc.in(ch);
                        else if (cc.negative())
                            inside = cc.in(QChar(ch).toLower()) &&
                                     cc.in(QChar(ch).toUpper());
                        else
                            inside = cc.in(QChar(ch).toLower()) ||
                                     cc.in(QChar(ch).toUpper());
#endif
#if !defined(QT_NO_REGEXP_BACKREF) && !defined(QT_NO_REGEXP_CAPTURE)
                    } else { /* ((m & QRegExpEngine::BackRefBit) != 0) */
                        int bref = m ^ QRegExpEngine::BackRefBit;
                        int ell = j * ncap + eng->captureForOfficialCapture.at(bref - 1);

                        inside = bref <= ncap && curCapBegin[ell] != EmptyCapture;
                        if (inside) {
                            if (eng->cs)
                                inside = (in[pos + curCapBegin[ell]] == QChar(ch));
                            else
                                inside = (in[pos + curCapBegin[ell]].toLower()
                                       == QChar(ch).toLower());
                        }

                        if (inside) {
                            int delta;
                            if (curCapEnd[ell] == EmptyCapture)
                                delta = i - curCapBegin[ell];
                            else
                                delta = curCapEnd[ell] - curCapBegin[ell];

                            inside = (delta <= len - (pos + i));
                            if (inside && delta > 1) {
                                int n = 1;
                                if (eng->cs) {
                                    while (n < delta) {
                                        if (in[pos + curCapBegin[ell] + n]
                                            != in[pos + i + n])
                                            break;
                                        ++n;
                                    }
                                } else {
                                    while (n < delta) {
                                        QChar a = in[pos + curCapBegin[ell] + n];
                                        QChar b = in[pos + i + n];
                                        if (a.toLower() != b.toLower())
                                            break;
                                        ++n;
                                    }
                                }
                                inside = (n == delta);
                                if (inside)
                                    needSomeSleep = delta - 1;
                            }
                        }
#endif
                    }
                }

                /*
                  We must now update our data structures.
                */
                if (inside) {
#ifndef QT_NO_REGEXP_CAPTURE
                    int *capBegin, *capEnd;
#endif
                    /*
                      If the next state was not encountered yet, all
                      is fine.
                    */
                    if ((m = inNextStack[next]) == -1) {
                        m = nnext++;
                        nextStack[m] = next;
                        inNextStack[next] = m;
#ifndef QT_NO_REGEXP_CAPTURE
                        capBegin = nextCapBegin + m * ncap;
                        capEnd = nextCapEnd + m * ncap;

                    /*
                      Otherwise, we'll first maintain captures in
                      temporary arrays, and decide at the end whether
                      it's best to keep the previous capture zones or
                      the new ones.
                    */
                    } else {
                        capBegin = tempCapBegin;
                        capEnd = tempCapEnd;
#endif
                    }

#ifndef QT_NO_REGEXP_CAPTURE
                    /*
                      Updating the capture zones is much of a task.
                    */
                    if (ncap > 0) {
                        memcpy(capBegin, curCapBegin + j * ncap, ncap * sizeof(int));
                        memcpy(capEnd, curCapEnd + j * ncap, ncap * sizeof(int));
                        int c = scur.atom, n = snext.atom;
                        int p = -1, q = -1;
                        int cap;

                        /*
                          Lemma 1. For any x in the range [0..nf), we
                          have f[x].parent < x.

                          Proof. By looking at startAtom(), it is
                          clear that cf < nf holds all the time, and
                          thus that f[nf].parent < nf.
                        */

                        /*
                          If we are reentering an atom, we empty all
                          capture zones inside it.
                        */
                        if ((q = scur.reenter.value(next)) != 0) {
                            QBitArray b(eng->nf, false);
                            b.setBit(q, true);
                            for (int ell = q + 1; ell < eng->nf; ell++) {
                                if (b.testBit(eng->f.at(ell).parent)) {
                                    b.setBit(ell, true);
                                    cap = eng->f.at(ell).capture;
                                    if (cap >= 0) {
                                        capBegin[cap] = EmptyCapture;
                                        capEnd[cap] = EmptyCapture;
                                    }
                                }
                            }
                            p = eng->f.at(q).parent;

                        /*
                          Otherwise, close the capture zones we are
                          leaving. We are leaving f[c].capture,
                          f[f[c].parent].capture,
                          f[f[f[c].parent].parent].capture, ...,
                          until f[x].capture, with x such that
                          f[x].parent is the youngest common ancestor
                          for c and n.

                          We go up along c's and n's ancestry until
                          we find x.
                        */
                        } else {
                            p = c;
                            q = n;
                            while (p != q) {
                                if (p > q) {
                                    cap = eng->f.at(p).capture;
                                    if (cap >= 0) {
                                        if (capBegin[cap] == i) {
                                            capBegin[cap] = EmptyCapture;
                                            capEnd[cap] = EmptyCapture;
                                        } else {
                                            capEnd[cap] = i;
                                        }
                                    }
                                    p = eng->f.at(p).parent;
                                } else {
                                    q = eng->f.at(q).parent;
                                }
                            }
                        }

                        /*
                          In any case, we now open the capture zones
                          we are entering. We work upwards from n
                          until we reach p (the parent of the atom we
                          reenter or the youngest common ancestor).
                        */
                        while (n > p) {
                            cap = eng->f.at(n).capture;
                            if (cap >= 0) {
                                capBegin[cap] = i;
                                capEnd[cap] = EmptyCapture;
                            }
                            n = eng->f.at(n).parent;
                        }
                        /*
                          If the next state was already in
                          nextStack, we must choose carefully which
                          capture zones we want to keep.
                        */
                        if (capBegin == tempCapBegin &&
                                isBetterCapture(ncap, capBegin, capEnd, nextCapBegin + m * ncap,
                                                nextCapEnd + m * ncap)) {
                            memcpy(nextCapBegin + m * ncap, capBegin, ncap * sizeof(int));
                            memcpy(nextCapEnd + m * ncap, capEnd, ncap * sizeof(int));
                        }
                    }
#ifndef QT_NO_REGEXP_BACKREF
                    /*
                      We are done with updating the capture zones.
                      It's now time to put the next state to sleep,
                      if it needs to, and to remove it from
                      nextStack.
                    */
                    if (needSomeSleep > 0) {
                        QVector<int> zzZ(2 + 2 * ncap);
                        zzZ[0] = i + needSomeSleep;
                        zzZ[1] = next;
                        if (ncap > 0) {
                            memcpy(zzZ.data() + 2, capBegin, ncap * sizeof(int));
                            memcpy(zzZ.data() + 2 + ncap, capEnd, ncap * sizeof(int));
                        }
                        inNextStack[nextStack[--nnext]] = -1;
                        sleeping.append(zzZ);
                    }
#endif
#endif
                }
            }
        }
#ifndef QT_NO_REGEXP_CAPTURE
        /*
          If we reached the final state, hurray! Copy the captured
          zone.
        */
        if (ncap > 0 && (m = inNextStack[QRegExpEngine::FinalState]) != -1) {
            memcpy(capBegin, nextCapBegin + m * ncap, ncap * sizeof(int));
            memcpy(capEnd, nextCapEnd + m * ncap, ncap * sizeof(int));
        }
#ifndef QT_NO_REGEXP_BACKREF
        /*
          It's time to wake up the sleepers.
        */
        j = 0;
        while (j < sleeping.count()) {
            if (sleeping.at(j)[0] == i) {
                const QVector<int> &zzZ = sleeping.at(j);
                int next = zzZ[1];
                const int *capBegin = zzZ.data() + 2;
                const int *capEnd = zzZ.data() + 2 + ncap;
                bool copyOver = true;

                if ((m = inNextStack[next]) == -1) {
                    m = nnext++;
                    nextStack[m] = next;
                    inNextStack[next] = m;
                } else {
                    copyOver = isBetterCapture(ncap, nextCapBegin + m * ncap, nextCapEnd + m * ncap,
                                               capBegin, capEnd);
                }
                if (copyOver) {
                    memcpy(nextCapBegin + m * ncap, capBegin, ncap * sizeof(int));
                    memcpy(nextCapEnd + m * ncap, capEnd, ncap * sizeof(int));
                }

                sleeping.removeAt(j);
            } else {
                ++j;
            }
        }
#endif
#endif
        for (j = 0; j < nnext; j++)
            inNextStack[nextStack[j]] = -1;

        // avoid needless iteration that confuses oneTestMatchedLen
        if (nnext == 1 && nextStack[0] == QRegExpEngine::FinalState
#ifndef QT_NO_REGEXP_BACKREF
             && sleeping.isEmpty()
#endif
           )
            stop = true;

        qSwap(curStack, nextStack);
#ifndef QT_NO_REGEXP_CAPTURE
        qSwap(curCapBegin, nextCapBegin);
        qSwap(curCapEnd, nextCapEnd);
#endif
        ncur = nnext;
        nnext = 0;
        ++i;
    }

#ifndef QT_NO_REGEXP_BACKREF
    /*
      If minimal matching is enabled, we might have some sleepers
      left.
    */
    if (!sleeping.isEmpty())
        sleeping.clear();
#endif

    oneTestMatchedLen = i - 1;
    return (matchLen >= 0);
}

#ifndef QT_NO_REGEXP_CCLASS

QRegExpCharClass::QRegExpCharClass()
    : c(0), n(false)
{
#ifndef QT_NO_REGEXP_OPTIM
    occ1.fill(NoOccurrence, NumBadChars);
#endif
}

void QRegExpCharClass::clear()
{
    c = 0;
    r.clear();
    n = false;
}

void QRegExpCharClass::setNegative(bool negative)
{
    n = negative;
#ifndef QT_NO_REGEXP_OPTIM
    occ1.fill(0, NumBadChars);
#endif
}

void QRegExpCharClass::addCategories(uint cats)
{
    static const int all_cats = FLAG(QChar::Mark_NonSpacing) |
                                FLAG(QChar::Mark_SpacingCombining) |
                                FLAG(QChar::Mark_Enclosing) |
                                FLAG(QChar::Number_DecimalDigit) |
                                FLAG(QChar::Number_Letter) |
                                FLAG(QChar::Number_Other) |
                                FLAG(QChar::Separator_Space) |
                                FLAG(QChar::Separator_Line) |
                                FLAG(QChar::Separator_Paragraph) |
                                FLAG(QChar::Other_Control) |
                                FLAG(QChar::Other_Format) |
                                FLAG(QChar::Other_Surrogate) |
                                FLAG(QChar::Other_PrivateUse) |
                                FLAG(QChar::Other_NotAssigned) |
                                FLAG(QChar::Letter_Uppercase) |
                                FLAG(QChar::Letter_Lowercase) |
                                FLAG(QChar::Letter_Titlecase) |
                                FLAG(QChar::Letter_Modifier) |
                                FLAG(QChar::Letter_Other) |
                                FLAG(QChar::Punctuation_Connector) |
                                FLAG(QChar::Punctuation_Dash) |
                                FLAG(QChar::Punctuation_Open) |
                                FLAG(QChar::Punctuation_Close) |
                                FLAG(QChar::Punctuation_InitialQuote) |
                                FLAG(QChar::Punctuation_FinalQuote) |
                                FLAG(QChar::Punctuation_Other) |
                                FLAG(QChar::Symbol_Math) |
                                FLAG(QChar::Symbol_Currency) |
                                FLAG(QChar::Symbol_Modifier) |
                                FLAG(QChar::Symbol_Other);
    c |= (all_cats & cats);
#ifndef QT_NO_REGEXP_OPTIM
    occ1.fill(0, NumBadChars);
#endif
}

void QRegExpCharClass::addRange(ushort from, ushort to)
{
    if (from > to)
        qSwap(from, to);
    int m = r.size();
    r.resize(m + 1);
    r[m].from = from;
    r[m].len = to - from + 1;

#ifndef QT_NO_REGEXP_OPTIM
    int i;

    if (to - from < NumBadChars) {
        if (from % NumBadChars <= to % NumBadChars) {
            for (i = from % NumBadChars; i <= to % NumBadChars; i++)
                occ1[i] = 0;
        } else {
            for (i = 0; i <= to % NumBadChars; i++)
                occ1[i] = 0;
            for (i = from % NumBadChars; i < NumBadChars; i++)
                occ1[i] = 0;
        }
    } else {
        occ1.fill(0, NumBadChars);
    }
#endif
}

bool QRegExpCharClass::in(QChar ch) const
{
#ifndef QT_NO_REGEXP_OPTIM
    if (occ1.at(BadChar(ch)) == NoOccurrence)
        return n;
#endif

    if (c != 0 && (c & FLAG(ch.category())) != 0)
        return !n;

    const int uc = ch.unicode();
    int size = r.size();

    for (int i = 0; i < size; ++i) {
        const QRegExpCharClassRange &range = r.at(i);
        if (uint(uc - range.from) < uint(r.at(i).len))
            return !n;
    }
    return n;
}

#if defined(QT_DEBUG)
void QRegExpCharClass::dump() const
{
    int i;
    qDebug("    %stive character class", n ? "nega" : "posi");
#ifndef QT_NO_REGEXP_CCLASS
    if (c != 0)
        qDebug("      categories 0x%.8x", c);
#endif
    for (i = 0; i < r.size(); i++)
        qDebug("      0x%.4x through 0x%.4x", r[i].from, r[i].from + r[i].len - 1);
}
#endif
#endif

QRegExpEngine::Box::Box(QRegExpEngine *engine)
    : eng(engine), skipanchors(0)
#ifndef QT_NO_REGEXP_OPTIM
      , earlyStart(0), lateStart(0), maxl(0)
#endif
{
#ifndef QT_NO_REGEXP_OPTIM
    occ1.fill(NoOccurrence, NumBadChars);
#endif
    minl = 0;
}

QRegExpEngine::Box &QRegExpEngine::Box::operator=(const Box &b)
{
    eng = b.eng;
    ls = b.ls;
    rs = b.rs;
    lanchors = b.lanchors;
    ranchors = b.ranchors;
    skipanchors = b.skipanchors;
#ifndef QT_NO_REGEXP_OPTIM
    earlyStart = b.earlyStart;
    lateStart = b.lateStart;
    str = b.str;
    leftStr = b.leftStr;
    rightStr = b.rightStr;
    maxl = b.maxl;
    occ1 = b.occ1;
#endif
    minl = b.minl;
    return *this;
}

void QRegExpEngine::Box::set(QChar ch)
{
    ls.resize(1);
    ls[0] = eng->createState(ch);
    rs = ls;
#ifndef QT_NO_REGEXP_OPTIM
    str = ch;
    leftStr = ch;
    rightStr = ch;
    maxl = 1;
    occ1[BadChar(ch)] = 0;
#endif
    minl = 1;
}

void QRegExpEngine::Box::set(const QRegExpCharClass &cc)
{
    ls.resize(1);
    ls[0] = eng->createState(cc);
    rs = ls;
#ifndef QT_NO_REGEXP_OPTIM
    maxl = 1;
    occ1 = cc.firstOccurrence();
#endif
    minl = 1;
}

#ifndef QT_NO_REGEXP_BACKREF
void QRegExpEngine::Box::set(int bref)
{
    ls.resize(1);
    ls[0] = eng->createState(bref);
    rs = ls;
    if (bref >= 1 && bref <= MaxBackRefs)
        skipanchors = Anchor_BackRef0Empty << bref;
#ifndef QT_NO_REGEXP_OPTIM
    maxl = InftyLen;
#endif
    minl = 0;
}
#endif

void QRegExpEngine::Box::cat(const Box &b)
{
    eng->addCatTransitions(rs, b.ls);
    addAnchorsToEngine(b);
    if (minl == 0) {
        lanchors.unite(b.lanchors);
        if (skipanchors != 0) {
            for (int i = 0; i < b.ls.size(); i++) {
                int a = eng->anchorConcatenation(lanchors.value(b.ls.at(i), 0), skipanchors);
                lanchors.insert(b.ls.at(i), a);
            }
        }
        mergeInto(&ls, b.ls);
    }
    if (b.minl == 0) {
        ranchors.unite(b.ranchors);
        if (b.skipanchors != 0) {
            for (int i = 0; i < rs.size(); i++) {
                int a = eng->anchorConcatenation(ranchors.value(rs.at(i), 0), b.skipanchors);
                ranchors.insert(rs.at(i), a);
            }
        }
        mergeInto(&rs, b.rs);
    } else {
        ranchors = b.ranchors;
        rs = b.rs;
    }

#ifndef QT_NO_REGEXP_OPTIM
    if (maxl != InftyLen) {
        if (rightStr.length() + b.leftStr.length() >
             qMax(str.length(), b.str.length())) {
            earlyStart = minl - rightStr.length();
            lateStart = maxl - rightStr.length();
            str = rightStr + b.leftStr;
        } else if (b.str.length() > str.length()) {
            earlyStart = minl + b.earlyStart;
            lateStart = maxl + b.lateStart;
            str = b.str;
        }
    }

    if (leftStr.length() == maxl)
        leftStr += b.leftStr;

    if (b.rightStr.length() == b.maxl) {
        rightStr += b.rightStr;
    } else {
        rightStr = b.rightStr;
    }

    if (maxl == InftyLen || b.maxl == InftyLen) {
        maxl = InftyLen;
    } else {
        maxl += b.maxl;
    }

    for (int i = 0; i < NumBadChars; i++) {
        if (b.occ1.at(i) != NoOccurrence && minl + b.occ1.at(i) < occ1.at(i))
            occ1[i] = minl + b.occ1.at(i);
    }
#endif

    minl += b.minl;
    if (minl == 0)
        skipanchors = eng->anchorConcatenation(skipanchors, b.skipanchors);
    else
        skipanchors = 0;
}

void QRegExpEngine::Box::orx(const Box &b)
{
    mergeInto(&ls, b.ls);
    lanchors.unite(b.lanchors);
    mergeInto(&rs, b.rs);
    ranchors.unite(b.ranchors);

    if (b.minl == 0) {
        if (minl == 0)
            skipanchors = eng->anchorAlternation(skipanchors, b.skipanchors);
        else
            skipanchors = b.skipanchors;
    }

#ifndef QT_NO_REGEXP_OPTIM
    for (int i = 0; i < NumBadChars; i++) {
        if (occ1.at(i) > b.occ1.at(i))
            occ1[i] = b.occ1.at(i);
    }
    earlyStart = 0;
    lateStart = 0;
    str = QString();
    leftStr = QString();
    rightStr = QString();
    if (b.maxl > maxl)
        maxl = b.maxl;
#endif
    if (b.minl < minl)
        minl = b.minl;
}

void QRegExpEngine::Box::plus(int atom)
{
#ifndef QT_NO_REGEXP_CAPTURE
    eng->addPlusTransitions(rs, ls, atom);
#else
    Q_UNUSED(atom);
    eng->addCatTransitions(rs, ls);
#endif
    addAnchorsToEngine(*this);
#ifndef QT_NO_REGEXP_OPTIM
    maxl = InftyLen;
#endif
}

void QRegExpEngine::Box::opt()
{
#ifndef QT_NO_REGEXP_OPTIM
    earlyStart = 0;
    lateStart = 0;
    str = QString();
    leftStr = QString();
    rightStr = QString();
#endif
    skipanchors = 0;
    minl = 0;
}

void QRegExpEngine::Box::catAnchor(int a)
{
    if (a != 0) {
        for (int i = 0; i < rs.size(); i++) {
            a = eng->anchorConcatenation(ranchors.value(rs.at(i), 0), a);
            ranchors.insert(rs.at(i), a);
        }
        if (minl == 0)
            skipanchors = eng->anchorConcatenation(skipanchors, a);
    }
}

#ifndef QT_NO_REGEXP_OPTIM
void QRegExpEngine::Box::setupHeuristics()
{
    eng->goodEarlyStart = earlyStart;
    eng->goodLateStart = lateStart;
    eng->goodStr = eng->cs ? str : str.toLower();

    eng->minl = minl;
    if (eng->cs) {
        /*
          A regular expression such as 112|1 has occ1['2'] = 2 and minl =
          1 at this point. An entry of occ1 has to be at most minl or
          infinity for the rest of the algorithm to go well.

          We waited until here before normalizing these cases (instead of
          doing it in Box::orx()) because sometimes things improve by
          themselves. Consider for example (112|1)34.
        */
        for (int i = 0; i < NumBadChars; i++) {
            if (occ1.at(i) != NoOccurrence && occ1.at(i) >= minl)
                occ1[i] = minl;
        }
        eng->occ1 = occ1;
    } else {
        eng->occ1.fill(0, NumBadChars);
    }

    eng->heuristicallyChooseHeuristic();
}
#endif

#if defined(QT_DEBUG)
void QRegExpEngine::Box::dump() const
{
    int i;
    qDebug("Box of at least %d character%s", minl, minl == 1 ? "" : "s");
    qDebug("  Left states:");
    for (i = 0; i < ls.size(); i++) {
        if (lanchors.value(ls[i], 0) == 0)
            qDebug("    %d", ls[i]);
        else
            qDebug("    %d [anchors 0x%.8x]", ls[i], lanchors[ls[i]]);
    }
    qDebug("  Right states:");
    for (i = 0; i < rs.size(); i++) {
        if (ranchors.value(rs[i], 0) == 0)
            qDebug("    %d", rs[i]);
        else
            qDebug("    %d [anchors 0x%.8x]", rs[i], ranchors[rs[i]]);
    }
    qDebug("  Skip anchors: 0x%.8x", skipanchors);
}
#endif

void QRegExpEngine::Box::addAnchorsToEngine(const Box &to) const
{
    for (int i = 0; i < to.ls.size(); i++) {
        for (int j = 0; j < rs.size(); j++) {
            int a = eng->anchorConcatenation(ranchors.value(rs.at(j), 0),
                                             to.lanchors.value(to.ls.at(i), 0));
            eng->addAnchors(rs[j], to.ls[i], a);
        }
    }
}

#ifndef QT_NO_REGEXP_CCLASS
// fast lookup hash for xml schema extensions
// sorted by name for b-search
static const struct CategoriesRangeMapEntry {
    const char name[40];
    uint first, second;
} categoriesRangeMap[] = {
    { "AegeanNumbers",                        0x10100, 0x1013F },
    { "AlphabeticPresentationForms",          0xFB00, 0xFB4F },
    { "AncientGreekMusicalNotation",          0x1D200, 0x1D24F },
    { "AncientGreekNumbers",                  0x10140, 0x1018F },
    { "Arabic",                               0x0600, 0x06FF },
    { "ArabicPresentationForms-A",            0xFB50, 0xFDFF },
    { "ArabicPresentationForms-B",            0xFE70, 0xFEFF },
    { "ArabicSupplement",                     0x0750, 0x077F },
    { "Armenian",                             0x0530, 0x058F },
    { "Arrows",                               0x2190, 0x21FF },
    { "BasicLatin",                           0x0000, 0x007F },
    { "Bengali",                              0x0980, 0x09FF },
    { "BlockElements",                        0x2580, 0x259F },
    { "Bopomofo",                             0x3100, 0x312F },
    { "BopomofoExtended",                     0x31A0, 0x31BF },
    { "BoxDrawing",                           0x2500, 0x257F },
    { "BraillePatterns",                      0x2800, 0x28FF },
    { "Buginese",                             0x1A00, 0x1A1F },
    { "Buhid",                                0x1740, 0x175F },
    { "ByzantineMusicalSymbols",              0x1D000, 0x1D0FF },
    { "CJKCompatibility",                     0x3300, 0x33FF },
    { "CJKCompatibilityForms",                0xFE30, 0xFE4F },
    { "CJKCompatibilityIdeographs",           0xF900, 0xFAFF },
    { "CJKCompatibilityIdeographsSupplement", 0x2F800, 0x2FA1F },
    { "CJKRadicalsSupplement",                0x2E80, 0x2EFF },
    { "CJKStrokes",                           0x31C0, 0x31EF },
    { "CJKSymbolsandPunctuation",             0x3000, 0x303F },
    { "CJKUnifiedIdeographs",                 0x4E00, 0x9FFF },
    { "CJKUnifiedIdeographsExtensionA",       0x3400, 0x4DB5 },
    { "CJKUnifiedIdeographsExtensionB",       0x20000, 0x2A6DF },
    { "Cherokee",                             0x13A0, 0x13FF },
    { "CombiningDiacriticalMarks",            0x0300, 0x036F },
    { "CombiningDiacriticalMarksSupplement",  0x1DC0, 0x1DFF },
    { "CombiningHalfMarks",                   0xFE20, 0xFE2F },
    { "CombiningMarksforSymbols",             0x20D0, 0x20FF },
    { "ControlPictures",                      0x2400, 0x243F },
    { "Coptic",                               0x2C80, 0x2CFF },
    { "CurrencySymbols",                      0x20A0, 0x20CF },
    { "CypriotSyllabary",                     0x10800, 0x1083F },
    { "Cyrillic",                             0x0400, 0x04FF },
    { "CyrillicSupplement",                   0x0500, 0x052F },
    { "Deseret",                              0x10400, 0x1044F },
    { "Devanagari",                           0x0900, 0x097F },
    { "Dingbats",                             0x2700, 0x27BF },
    { "EnclosedAlphanumerics",                0x2460, 0x24FF },
    { "EnclosedCJKLettersandMonths",          0x3200, 0x32FF },
    { "Ethiopic",                             0x1200, 0x137F },
    { "EthiopicExtended",                     0x2D80, 0x2DDF },
    { "EthiopicSupplement",                   0x1380, 0x139F },
    { "GeneralPunctuation",                   0x2000, 0x206F },
    { "GeometricShapes",                      0x25A0, 0x25FF },
    { "Georgian",                             0x10A0, 0x10FF },
    { "GeorgianSupplement",                   0x2D00, 0x2D2F },
    { "Glagolitic",                           0x2C00, 0x2C5F },
    { "Gothic",                               0x10330, 0x1034F },
    { "Greek",                                0x0370, 0x03FF },
    { "GreekExtended",                        0x1F00, 0x1FFF },
    { "Gujarati",                             0x0A80, 0x0AFF },
    { "Gurmukhi",                             0x0A00, 0x0A7F },
    { "HalfwidthandFullwidthForms",           0xFF00, 0xFFEF },
    { "HangulCompatibilityJamo",              0x3130, 0x318F },
    { "HangulJamo",                           0x1100, 0x11FF },
    { "HangulSyllables",                      0xAC00, 0xD7A3 },
    { "Hanunoo",                              0x1720, 0x173F },
    { "Hebrew",                               0x0590, 0x05FF },
    { "Hiragana",                             0x3040, 0x309F },
    { "IPAExtensions",                        0x0250, 0x02AF },
    { "IdeographicDescriptionCharacters",     0x2FF0, 0x2FFF },
    { "Kanbun",                               0x3190, 0x319F },
    { "KangxiRadicals",                       0x2F00, 0x2FDF },
    { "Kannada",                              0x0C80, 0x0CFF },
    { "Katakana",                             0x30A0, 0x30FF },
    { "KatakanaPhoneticExtensions",           0x31F0, 0x31FF },
    { "Kharoshthi",                           0x10A00, 0x10A5F },
    { "Khmer",                                0x1780, 0x17FF },
    { "KhmerSymbols",                         0x19E0, 0x19FF },
    { "Lao",                                  0x0E80, 0x0EFF },
    { "Latin-1Supplement",                    0x0080, 0x00FF },
    { "LatinExtended-A",                      0x0100, 0x017F },
    { "LatinExtended-B",                      0x0180, 0x024F },
    { "LatinExtendedAdditional",              0x1E00, 0x1EFF },
    { "LetterlikeSymbols",                    0x2100, 0x214F },
    { "Limbu",                                0x1900, 0x194F },
    { "LinearBIdeograms",                     0x10080, 0x100FF },
    { "LinearBSyllabary",                     0x10000, 0x1007F },
    { "Malayalam",                            0x0D00, 0x0D7F },
    { "MathematicalAlphanumericSymbols",      0x1D400, 0x1D7FF },
    { "MathematicalOperators",                0x2200, 0x22FF },
    { "MiscellaneousMathematicalSymbols-A",   0x27C0, 0x27EF },
    { "MiscellaneousMathematicalSymbols-B",   0x2980, 0x29FF },
    { "MiscellaneousSymbols",                 0x2600, 0x26FF },
    { "MiscellaneousSymbolsandArrows",        0x2B00, 0x2BFF },
    { "MiscellaneousTechnical",               0x2300, 0x23FF },
    { "ModifierToneLetters",                  0xA700, 0xA71F },
    { "Mongolian",                            0x1800, 0x18AF },
    { "MusicalSymbols",                       0x1D100, 0x1D1FF },
    { "Myanmar",                              0x1000, 0x109F },
    { "NewTaiLue",                            0x1980, 0x19DF },
    { "NumberForms",                          0x2150, 0x218F },
    { "Ogham",                                0x1680, 0x169F },
    { "OldItalic",                            0x10300, 0x1032F },
    { "OldPersian",                           0x103A0, 0x103DF },
    { "OpticalCharacterRecognition",          0x2440, 0x245F },
    { "Oriya",                                0x0B00, 0x0B7F },
    { "Osmanya",                              0x10480, 0x104AF },
    { "PhoneticExtensions",                   0x1D00, 0x1D7F },
    { "PhoneticExtensionsSupplement",         0x1D80, 0x1DBF },
    { "PrivateUse",                           0xE000, 0xF8FF },
    { "Runic",                                0x16A0, 0x16FF },
    { "Shavian",                              0x10450, 0x1047F },
    { "Sinhala",                              0x0D80, 0x0DFF },
    { "SmallFormVariants",                    0xFE50, 0xFE6F },
    { "SpacingModifierLetters",               0x02B0, 0x02FF },
    { "Specials",                             0xFFF0, 0xFFFF },
    { "SuperscriptsandSubscripts",            0x2070, 0x209F },
    { "SupplementalArrows-A",                 0x27F0, 0x27FF },
    { "SupplementalArrows-B",                 0x2900, 0x297F },
    { "SupplementalMathematicalOperators",    0x2A00, 0x2AFF },
    { "SupplementalPunctuation",              0x2E00, 0x2E7F },
    { "SupplementaryPrivateUseArea-A",        0xF0000, 0xFFFFF },
    { "SupplementaryPrivateUseArea-B",        0x100000, 0x10FFFF },
    { "SylotiNagri",                          0xA800, 0xA82F },
    { "Syriac",                               0x0700, 0x074F },
    { "Tagalog",                              0x1700, 0x171F },
    { "Tagbanwa",                             0x1760, 0x177F },
    { "Tags",                                 0xE0000, 0xE007F },
    { "TaiLe",                                0x1950, 0x197F },
    { "TaiXuanJingSymbols",                   0x1D300, 0x1D35F },
    { "Tamil",                                0x0B80, 0x0BFF },
    { "Telugu",                               0x0C00, 0x0C7F },
    { "Thaana",                               0x0780, 0x07BF },
    { "Thai",                                 0x0E00, 0x0E7F },
    { "Tibetan",                              0x0F00, 0x0FFF },
    { "Tifinagh",                             0x2D30, 0x2D7F },
    { "Ugaritic",                             0x10380, 0x1039F },
    { "UnifiedCanadianAboriginalSyllabics",   0x1400, 0x167F },
    { "VariationSelectors",                   0xFE00, 0xFE0F },
    { "VariationSelectorsSupplement",         0xE0100, 0xE01EF },
    { "VerticalForms",                        0xFE10, 0xFE1F },
    { "YiRadicals",                           0xA490, 0xA4CF },
    { "YiSyllables",                          0xA000, 0xA48F },
    { "YijingHexagramSymbols",                0x4DC0, 0x4DFF }
};

inline bool operator<(const CategoriesRangeMapEntry &entry1, const CategoriesRangeMapEntry &entry2)
{ return qstrcmp(entry1.name, entry2.name) < 0; }
inline bool operator<(const char *name, const CategoriesRangeMapEntry &entry)
{ return qstrcmp(name, entry.name) < 0; }
inline bool operator<(const CategoriesRangeMapEntry &entry, const char *name)
{ return qstrcmp(entry.name, name) < 0; }
#endif // QT_NO_REGEXP_CCLASS

int QRegExpEngine::getChar()
{
    return (yyPos == yyLen) ? EOS : yyIn[yyPos++].unicode();
}

int QRegExpEngine::getEscape()
{
#ifndef QT_NO_REGEXP_ESCAPE
    const char tab[] = "afnrtv"; // no b, as \b means word boundary
    const char backTab[] = "\a\f\n\r\t\v";
    ushort low;
    int i;
#endif
    ushort val;
    int prevCh = yyCh;

    if (prevCh == EOS) {
        error(RXERR_END);
        return Tok_Char | '\\';
    }
    yyCh = getChar();
#ifndef QT_NO_REGEXP_ESCAPE
    if ((prevCh & ~0xff) == 0) {
        const char *p = strchr(tab, prevCh);
        if (p != 0)
            return Tok_Char | backTab[p - tab];
    }
#endif

    switch (prevCh) {
#ifndef QT_NO_REGEXP_ESCAPE
    case '0':
        val = 0;
        for (i = 0; i < 3; i++) {
            if (yyCh >= '0' && yyCh <= '7')
                val = (val << 3) | (yyCh - '0');
            else
                break;
            yyCh = getChar();
        }
        if ((val & ~0377) != 0)
            error(RXERR_OCTAL);
        return Tok_Char | val;
#endif
#ifndef QT_NO_REGEXP_ESCAPE
    case 'B':
        return Tok_NonWord;
#endif
#ifndef QT_NO_REGEXP_CCLASS
    case 'D':
        // see QChar::isDigit()
        yyCharClass->addCategories(uint(-1) ^ FLAG(QChar::Number_DecimalDigit));
        return Tok_CharClass;
    case 'S':
        // see QChar::isSpace()
        yyCharClass->addCategories(uint(-1) ^ (FLAG(QChar::Separator_Space) |
                                               FLAG(QChar::Separator_Line) |
                                               FLAG(QChar::Separator_Paragraph) |
                                               FLAG(QChar::Other_Control)));
        yyCharClass->addRange(0x0000, 0x0008);
        yyCharClass->addRange(0x000e, 0x001f);
        yyCharClass->addRange(0x007f, 0x0084);
        yyCharClass->addRange(0x0086, 0x009f);
        return Tok_CharClass;
    case 'W':
        // see QChar::isLetterOrNumber() and QChar::isMark()
        yyCharClass->addCategories(uint(-1) ^ (FLAG(QChar::Mark_NonSpacing) |
                                               FLAG(QChar::Mark_SpacingCombining) |
                                               FLAG(QChar::Mark_Enclosing) |
                                               FLAG(QChar::Number_DecimalDigit) |
                                               FLAG(QChar::Number_Letter) |
                                               FLAG(QChar::Number_Other) |
                                               FLAG(QChar::Letter_Uppercase) |
                                               FLAG(QChar::Letter_Lowercase) |
                                               FLAG(QChar::Letter_Titlecase) |
                                               FLAG(QChar::Letter_Modifier) |
                                               FLAG(QChar::Letter_Other) |
                                               FLAG(QChar::Punctuation_Connector)));
        yyCharClass->addRange(0x203f, 0x2040);
        yyCharClass->addSingleton(0x2040);
        yyCharClass->addSingleton(0x2054);
        yyCharClass->addSingleton(0x30fb);
        yyCharClass->addRange(0xfe33, 0xfe34);
        yyCharClass->addRange(0xfe4d, 0xfe4f);
        yyCharClass->addSingleton(0xff3f);
        yyCharClass->addSingleton(0xff65);
        return Tok_CharClass;
#endif
#ifndef QT_NO_REGEXP_ESCAPE
    case 'b':
        return Tok_Word;
#endif
#ifndef QT_NO_REGEXP_CCLASS
    case 'd':
        // see QChar::isDigit()
        yyCharClass->addCategories(FLAG(QChar::Number_DecimalDigit));
        return Tok_CharClass;
    case 's':
        // see QChar::isSpace()
        yyCharClass->addCategories(FLAG(QChar::Separator_Space) |
                                   FLAG(QChar::Separator_Line) |
                                   FLAG(QChar::Separator_Paragraph));
        yyCharClass->addRange(0x0009, 0x000d);
        yyCharClass->addSingleton(0x0085);
        return Tok_CharClass;
    case 'w':
        // see QChar::isLetterOrNumber() and QChar::isMark()
        yyCharClass->addCategories(FLAG(QChar::Mark_NonSpacing) |
                                   FLAG(QChar::Mark_SpacingCombining) |
                                   FLAG(QChar::Mark_Enclosing) |
                                   FLAG(QChar::Number_DecimalDigit) |
                                   FLAG(QChar::Number_Letter) |
                                   FLAG(QChar::Number_Other) |
                                   FLAG(QChar::Letter_Uppercase) |
                                   FLAG(QChar::Letter_Lowercase) |
                                   FLAG(QChar::Letter_Titlecase) |
                                   FLAG(QChar::Letter_Modifier) |
                                   FLAG(QChar::Letter_Other));
        yyCharClass->addSingleton(0x005f); // '_'
        return Tok_CharClass;
    case 'I':
        if (xmlSchemaExtensions) {
            yyCharClass->setNegative(!yyCharClass->negative());
            Q_FALLTHROUGH();
        } else {
            break;
        }
    case 'i':
        if (xmlSchemaExtensions) {
            yyCharClass->addCategories(FLAG(QChar::Mark_NonSpacing) |
                                       FLAG(QChar::Mark_SpacingCombining) |
                                       FLAG(QChar::Mark_Enclosing) |
                                       FLAG(QChar::Number_DecimalDigit) |
                                       FLAG(QChar::Number_Letter) |
                                       FLAG(QChar::Number_Other) |
                                       FLAG(QChar::Letter_Uppercase) |
                                       FLAG(QChar::Letter_Lowercase) |
                                       FLAG(QChar::Letter_Titlecase) |
                                       FLAG(QChar::Letter_Modifier) |
                                       FLAG(QChar::Letter_Other));
            yyCharClass->addSingleton(0x003a); // ':'
            yyCharClass->addSingleton(0x005f); // '_'
            yyCharClass->addRange(0x0041, 0x005a); // [A-Z]
            yyCharClass->addRange(0x0061, 0x007a); // [a-z]
            yyCharClass->addRange(0xc0, 0xd6);
            yyCharClass->addRange(0xd8, 0xf6);
            yyCharClass->addRange(0xf8, 0x2ff);
            yyCharClass->addRange(0x370, 0x37d);
            yyCharClass->addRange(0x37f, 0x1fff);
            yyCharClass->addRange(0x200c, 0x200d);
            yyCharClass->addRange(0x2070, 0x218f);
            yyCharClass->addRange(0x2c00, 0x2fef);
            yyCharClass->addRange(0x3001, 0xd7ff);
            yyCharClass->addRange(0xf900, 0xfdcf);
            yyCharClass->addRange(0xfdf0, 0xfffd);
            yyCharClass->addRange((ushort)0x10000, (ushort)0xeffff);
            return Tok_CharClass;
        } else {
            break;
        }
    case 'C':
        if (xmlSchemaExtensions) {
            yyCharClass->setNegative(!yyCharClass->negative());
            Q_FALLTHROUGH();
        } else {
            break;
        }
    case 'c':
        if (xmlSchemaExtensions) {
            yyCharClass->addCategories(FLAG(QChar::Mark_NonSpacing) |
                                       FLAG(QChar::Mark_SpacingCombining) |
                                       FLAG(QChar::Mark_Enclosing) |
                                       FLAG(QChar::Number_DecimalDigit) |
                                       FLAG(QChar::Number_Letter) |
                                       FLAG(QChar::Number_Other) |
                                       FLAG(QChar::Letter_Uppercase) |
                                       FLAG(QChar::Letter_Lowercase) |
                                       FLAG(QChar::Letter_Titlecase) |
                                       FLAG(QChar::Letter_Modifier) |
                                       FLAG(QChar::Letter_Other));
            yyCharClass->addSingleton(0x002d); // '-'
            yyCharClass->addSingleton(0x002e); // '.'
            yyCharClass->addSingleton(0x003a); // ':'
            yyCharClass->addSingleton(0x005f); // '_'
            yyCharClass->addSingleton(0xb7);
            yyCharClass->addRange(0x0030, 0x0039); // [0-9]
            yyCharClass->addRange(0x0041, 0x005a); // [A-Z]
            yyCharClass->addRange(0x0061, 0x007a); // [a-z]
            yyCharClass->addRange(0xc0, 0xd6);
            yyCharClass->addRange(0xd8, 0xf6);
            yyCharClass->addRange(0xf8, 0x2ff);
            yyCharClass->addRange(0x370, 0x37d);
            yyCharClass->addRange(0x37f, 0x1fff);
            yyCharClass->addRange(0x200c, 0x200d);
            yyCharClass->addRange(0x2070, 0x218f);
            yyCharClass->addRange(0x2c00, 0x2fef);
            yyCharClass->addRange(0x3001, 0xd7ff);
            yyCharClass->addRange(0xf900, 0xfdcf);
            yyCharClass->addRange(0xfdf0, 0xfffd);
            yyCharClass->addRange((ushort)0x10000, (ushort)0xeffff);
            yyCharClass->addRange(0x0300, 0x036f);
            yyCharClass->addRange(0x203f, 0x2040);
            return Tok_CharClass;
        } else {
            break;
        }
    case 'P':
        if (xmlSchemaExtensions) {
            yyCharClass->setNegative(!yyCharClass->negative());
            Q_FALLTHROUGH();
        } else {
            break;
        }
    case 'p':
        if (xmlSchemaExtensions) {
            if (yyCh != '{') {
                error(RXERR_CHARCLASS);
                return Tok_CharClass;
            }

            QByteArray category;
            yyCh = getChar();
            while (yyCh != '}') {
                if (yyCh == EOS) {
                    error(RXERR_END);
                    return Tok_CharClass;
                }
                category.append(yyCh);
                yyCh = getChar();
            }
            yyCh = getChar(); // skip closing '}'

            int catlen = category.length();
            if (catlen == 1 || catlen == 2) {
                switch (category.at(0)) {
                case 'M':
                    if (catlen == 1) {
                        yyCharClass->addCategories(FLAG(QChar::Mark_NonSpacing) |
                                                   FLAG(QChar::Mark_SpacingCombining) |
                                                   FLAG(QChar::Mark_Enclosing));
                    } else {
                        switch (category.at(1)) {
                        case 'n': yyCharClass->addCategories(FLAG(QChar::Mark_NonSpacing)); break; // Mn
                        case 'c': yyCharClass->addCategories(FLAG(QChar::Mark_SpacingCombining)); break; // Mc
                        case 'e': yyCharClass->addCategories(FLAG(QChar::Mark_Enclosing)); break; // Me
                        default: error(RXERR_CATEGORY); break;
                        }
                    }
                    break;
                case 'N':
                    if (catlen == 1) {
                        yyCharClass->addCategories(FLAG(QChar::Number_DecimalDigit) |
                                                   FLAG(QChar::Number_Letter) |
                                                   FLAG(QChar::Number_Other));
                    } else {
                        switch (category.at(1)) {
                        case 'd': yyCharClass->addCategories(FLAG(QChar::Number_DecimalDigit)); break; // Nd
                        case 'l': yyCharClass->addCategories(FLAG(QChar::Number_Letter)); break; // Hl
                        case 'o': yyCharClass->addCategories(FLAG(QChar::Number_Other)); break; // No
                        default: error(RXERR_CATEGORY); break;
                        }
                    }
                    break;
                case 'Z':
                    if (catlen == 1) {
                        yyCharClass->addCategories(FLAG(QChar::Separator_Space) |
                                                   FLAG(QChar::Separator_Line) |
                                                   FLAG(QChar::Separator_Paragraph));
                    } else {
                        switch (category.at(1)) {
                        case 's': yyCharClass->addCategories(FLAG(QChar::Separator_Space)); break; // Zs
                        case 'l': yyCharClass->addCategories(FLAG(QChar::Separator_Line)); break; // Zl
                        case 'p': yyCharClass->addCategories(FLAG(QChar::Separator_Paragraph)); break; // Zp
                        default: error(RXERR_CATEGORY); break;
                        }
                    }
                    break;
                case 'C':
                    if (catlen == 1) {
                        yyCharClass->addCategories(FLAG(QChar::Other_Control) |
                                                   FLAG(QChar::Other_Format) |
                                                   FLAG(QChar::Other_Surrogate) |
                                                   FLAG(QChar::Other_PrivateUse) |
                                                   FLAG(QChar::Other_NotAssigned));
                    } else {
                        switch (category.at(1)) {
                        case 'c': yyCharClass->addCategories(FLAG(QChar::Other_Control)); break; // Cc
                        case 'f': yyCharClass->addCategories(FLAG(QChar::Other_Format)); break; // Cf
                        case 's': yyCharClass->addCategories(FLAG(QChar::Other_Surrogate)); break; // Cs
                        case 'o': yyCharClass->addCategories(FLAG(QChar::Other_PrivateUse)); break; // Co
                        case 'n': yyCharClass->addCategories(FLAG(QChar::Other_NotAssigned)); break; // Cn
                        default: error(RXERR_CATEGORY); break;
                        }
                    }
                    break;
                case 'L':
                    if (catlen == 1) {
                        yyCharClass->addCategories(FLAG(QChar::Letter_Uppercase) |
                                                   FLAG(QChar::Letter_Lowercase) |
                                                   FLAG(QChar::Letter_Titlecase) |
                                                   FLAG(QChar::Letter_Modifier) |
                                                   FLAG(QChar::Letter_Other));
                    } else {
                        switch (category.at(1)) {
                        case 'u': yyCharClass->addCategories(FLAG(QChar::Letter_Uppercase)); break; // Lu
                        case 'l': yyCharClass->addCategories(FLAG(QChar::Letter_Lowercase)); break; // Ll
                        case 't': yyCharClass->addCategories(FLAG(QChar::Letter_Titlecase)); break; // Lt
                        case 'm': yyCharClass->addCategories(FLAG(QChar::Letter_Modifier)); break; // Lm
                        case 'o': yyCharClass->addCategories(FLAG(QChar::Letter_Other)); break; // Lo
                        default: error(RXERR_CATEGORY); break;
                        }
                    }
                    break;
                case 'P':
                    if (catlen == 1) {
                        yyCharClass->addCategories(FLAG(QChar::Punctuation_Connector) |
                                                   FLAG(QChar::Punctuation_Dash) |
                                                   FLAG(QChar::Punctuation_Open) |
                                                   FLAG(QChar::Punctuation_Close) |
                                                   FLAG(QChar::Punctuation_InitialQuote) |
                                                   FLAG(QChar::Punctuation_FinalQuote) |
                                                   FLAG(QChar::Punctuation_Other));
                    } else {
                        switch (category.at(1)) {
                        case 'c': yyCharClass->addCategories(FLAG(QChar::Punctuation_Connector)); break; // Pc
                        case 'd': yyCharClass->addCategories(FLAG(QChar::Punctuation_Dash)); break; // Pd
                        case 's': yyCharClass->addCategories(FLAG(QChar::Punctuation_Open)); break; // Ps
                        case 'e': yyCharClass->addCategories(FLAG(QChar::Punctuation_Close)); break; // Pe
                        case 'i': yyCharClass->addCategories(FLAG(QChar::Punctuation_InitialQuote)); break; // Pi
                        case 'f': yyCharClass->addCategories(FLAG(QChar::Punctuation_FinalQuote)); break; // Pf
                        case 'o': yyCharClass->addCategories(FLAG(QChar::Punctuation_Other)); break; // Po
                        default: error(RXERR_CATEGORY); break;
                        }
                    }
                    break;
                case 'S':
                    if (catlen == 1) {
                        yyCharClass->addCategories(FLAG(QChar::Symbol_Math) |
                                                   FLAG(QChar::Symbol_Currency) |
                                                   FLAG(QChar::Symbol_Modifier) |
                                                   FLAG(QChar::Symbol_Other));
                    } else {
                        switch (category.at(1)) {
                        case 'm': yyCharClass->addCategories(FLAG(QChar::Symbol_Math)); break; // Sm
                        case 'c': yyCharClass->addCategories(FLAG(QChar::Symbol_Currency)); break; // Sc
                        case 'k': yyCharClass->addCategories(FLAG(QChar::Symbol_Modifier)); break; // Sk
                        case 'o': yyCharClass->addCategories(FLAG(QChar::Symbol_Other)); break; // So
                        default: error(RXERR_CATEGORY); break;
                        }
                    }
                    break;
                default:
                    error(RXERR_CATEGORY);
                    break;
                }
            } else if (catlen > 2 && category.at(0) == 'I' && category.at(1) == 's') {
                static const int N = sizeof(categoriesRangeMap) / sizeof(categoriesRangeMap[0]);
                const char * const categoryFamily = category.constData() + 2;
                const CategoriesRangeMapEntry *r = std::lower_bound(categoriesRangeMap, categoriesRangeMap + N, categoryFamily);
                if (r != categoriesRangeMap + N && qstrcmp(r->name, categoryFamily) == 0)
                    yyCharClass->addRange(r->first, r->second);
                else
                    error(RXERR_CATEGORY);
            } else {
                error(RXERR_CATEGORY);
            }
            return Tok_CharClass;
        } else {
            break;
        }
#endif
#ifndef QT_NO_REGEXP_ESCAPE
    case 'x':
        val = 0;
        for (i = 0; i < 4; i++) {
            low = QChar(yyCh).toLower().unicode();
            if (low >= '0' && low <= '9')
                val = (val << 4) | (low - '0');
            else if (low >= 'a' && low <= 'f')
                val = (val << 4) | (low - 'a' + 10);
            else
                break;
            yyCh = getChar();
        }
        return Tok_Char | val;
#endif
    default:
        break;
    }
    if (prevCh >= '1' && prevCh <= '9') {
#ifndef QT_NO_REGEXP_BACKREF
        val = prevCh - '0';
        while (yyCh >= '0' && yyCh <= '9') {
            val = (val * 10) + (yyCh - '0');
            yyCh = getChar();
        }
        return Tok_BackRef | val;
#else
        error(RXERR_DISABLED);
#endif
    }
    return Tok_Char | prevCh;
}

#ifndef QT_NO_REGEXP_INTERVAL
int QRegExpEngine::getRep(int def)
{
    if (yyCh >= '0' && yyCh <= '9') {
        int rep = 0;
        do {
            rep = 10 * rep + yyCh - '0';
            if (rep >= InftyRep) {
                error(RXERR_REPETITION);
                rep = def;
            }
            yyCh = getChar();
        } while (yyCh >= '0' && yyCh <= '9');
        return rep;
    } else {
        return def;
    }
}
#endif

#ifndef QT_NO_REGEXP_LOOKAHEAD
void QRegExpEngine::skipChars(int n)
{
    if (n > 0) {
        yyPos += n - 1;
        yyCh = getChar();
    }
}
#endif

void QRegExpEngine::error(const char *msg)
{
    if (yyError.isEmpty())
        yyError = QLatin1String(msg);
}

void QRegExpEngine::startTokenizer(const QChar *rx, int len)
{
    yyIn = rx;
    yyPos0 = 0;
    yyPos = 0;
    yyLen = len;
    yyCh = getChar();
    yyCharClass.reset(new QRegExpCharClass);
    yyMinRep = 0;
    yyMaxRep = 0;
    yyError = QString();
}

int QRegExpEngine::getToken()
{
#ifndef QT_NO_REGEXP_CCLASS
    ushort pendingCh = 0;
    bool charPending;
    bool rangePending;
    int tok;
#endif
    int prevCh = yyCh;

    yyPos0 = yyPos - 1;
#ifndef QT_NO_REGEXP_CCLASS
    yyCharClass->clear();
#endif
    yyMinRep = 0;
    yyMaxRep = 0;
    yyCh = getChar();

    switch (prevCh) {
    case EOS:
        yyPos0 = yyPos;
        return Tok_Eos;
    case '$':
        return Tok_Dollar;
    case '(':
        if (yyCh == '?') {
            prevCh = getChar();
            yyCh = getChar();
            switch (prevCh) {
#ifndef QT_NO_REGEXP_LOOKAHEAD
            case '!':
                return Tok_NegLookahead;
            case '=':
                return Tok_PosLookahead;
#endif
            case ':':
                return Tok_MagicLeftParen;
            case '<':
                error(RXERR_LOOKBEHIND);
                return Tok_MagicLeftParen;
            default:
                error(RXERR_LOOKAHEAD);
                return Tok_MagicLeftParen;
            }
        } else {
            return Tok_LeftParen;
        }
    case ')':
        return Tok_RightParen;
    case '*':
        yyMinRep = 0;
        yyMaxRep = InftyRep;
        return Tok_Quantifier;
    case '+':
        yyMinRep = 1;
        yyMaxRep = InftyRep;
        return Tok_Quantifier;
    case '.':
#ifndef QT_NO_REGEXP_CCLASS
        yyCharClass->setNegative(true);
#endif
        return Tok_CharClass;
    case '?':
        yyMinRep = 0;
        yyMaxRep = 1;
        return Tok_Quantifier;
    case '[':
#ifndef QT_NO_REGEXP_CCLASS
        if (yyCh == '^') {
            yyCharClass->setNegative(true);
            yyCh = getChar();
        }
        charPending = false;
        rangePending = false;
        do {
            if (yyCh == '-' && charPending && !rangePending) {
                rangePending = true;
                yyCh = getChar();
            } else {
                if (charPending && !rangePending) {
                    yyCharClass->addSingleton(pendingCh);
                    charPending = false;
                }
                if (yyCh == '\\') {
                    yyCh = getChar();
                    tok = getEscape();
                    if (tok == Tok_Word)
                        tok = '\b';
                } else {
                    tok = Tok_Char | yyCh;
                    yyCh = getChar();
                }
                if (tok == Tok_CharClass) {
                    if (rangePending) {
                        yyCharClass->addSingleton('-');
                        yyCharClass->addSingleton(pendingCh);
                        charPending = false;
                        rangePending = false;
                    }
                } else if ((tok & Tok_Char) != 0) {
                    if (rangePending) {
                        yyCharClass->addRange(pendingCh, tok ^ Tok_Char);
                        charPending = false;
                        rangePending = false;
                    } else {
                        pendingCh = tok ^ Tok_Char;
                        charPending = true;
                    }
                } else {
                    error(RXERR_CHARCLASS);
                }
            }
        }  while (yyCh != ']' && yyCh != EOS);
        if (rangePending)
            yyCharClass->addSingleton('-');
        if (charPending)
            yyCharClass->addSingleton(pendingCh);
        if (yyCh == EOS)
            error(RXERR_END);
        else
            yyCh = getChar();
        return Tok_CharClass;
#else
        error(RXERR_END);
        return Tok_Char | '[';
#endif
    case '\\':
        return getEscape();
    case ']':
        error(RXERR_LEFTDELIM);
        return Tok_Char | ']';
    case '^':
        return Tok_Caret;
    case '{':
#ifndef QT_NO_REGEXP_INTERVAL
        yyMinRep = getRep(0);
        yyMaxRep = yyMinRep;
        if (yyCh == ',') {
            yyCh = getChar();
            yyMaxRep = getRep(InftyRep);
        }
        if (yyMaxRep < yyMinRep)
            error(RXERR_INTERVAL);
        if (yyCh != '}')
            error(RXERR_REPETITION);
        yyCh = getChar();
        return Tok_Quantifier;
#else
        error(RXERR_DISABLED);
        return Tok_Char | '{';
#endif
    case '|':
        return Tok_Bar;
    case '}':
        error(RXERR_LEFTDELIM);
        return Tok_Char | '}';
    default:
        return Tok_Char | prevCh;
    }
}

int QRegExpEngine::parse(const QChar *pattern, int len)
{
    valid = true;
    startTokenizer(pattern, len);
    yyTok = getToken();
#ifndef QT_NO_REGEXP_CAPTURE
    yyMayCapture = true;
#else
    yyMayCapture = false;
#endif

#ifndef QT_NO_REGEXP_CAPTURE
    int atom = startAtom(false);
#endif
    QRegExpCharClass anything;
    Box box(this); // create InitialState
    box.set(anything);
    Box rightBox(this); // create FinalState
    rightBox.set(anything);

    Box middleBox(this);
    parseExpression(&middleBox);
#ifndef QT_NO_REGEXP_CAPTURE
    finishAtom(atom, false);
#endif
#ifndef QT_NO_REGEXP_OPTIM
    middleBox.setupHeuristics();
#endif
    box.cat(middleBox);
    box.cat(rightBox);
    yyCharClass.reset(0);

#ifndef QT_NO_REGEXP_CAPTURE
    for (int i = 0; i < nf; ++i) {
        switch (f[i].capture) {
        case QRegExpAtom::NoCapture:
            break;
        case QRegExpAtom::OfficialCapture:
            f[i].capture = ncap;
            captureForOfficialCapture.append(ncap);
            ++ncap;
            ++officialncap;
            break;
        case QRegExpAtom::UnofficialCapture:
            f[i].capture = greedyQuantifiers ? ncap++ : QRegExpAtom::NoCapture;
        }
    }

#ifndef QT_NO_REGEXP_BACKREF
#ifndef QT_NO_REGEXP_OPTIM
    if (officialncap == 0 && nbrefs == 0) {
        ncap = nf = 0;
        f.clear();
    }
#endif
    // handle the case where there's a \5 with no corresponding capture
    // (captureForOfficialCapture.size() != officialncap)
    for (int i = 0; i < nbrefs - officialncap; ++i) {
        captureForOfficialCapture.append(ncap);
        ++ncap;
    }
#endif
#endif

    if (!yyError.isEmpty())
        return -1;

#ifndef QT_NO_REGEXP_OPTIM
    const QRegExpAutomatonState &sinit = s.at(InitialState);
    caretAnchored = !sinit.anchors.isEmpty();
    if (caretAnchored) {
        const QMap<int, int> &anchors = sinit.anchors;
        QMap<int, int>::const_iterator a;
        for (a = anchors.constBegin(); a != anchors.constEnd(); ++a) {
            if (
#ifndef QT_NO_REGEXP_ANCHOR_ALT
                (*a & Anchor_Alternation) != 0 ||
#endif
                (*a & Anchor_Caret) == 0)
            {
                caretAnchored = false;
                break;
            }
        }
    }
#endif

    // cleanup anchors
    int numStates = s.count();
    for (int i = 0; i < numStates; ++i) {
        QRegExpAutomatonState &state = s[i];
        if (!state.anchors.isEmpty()) {
            QMap<int, int>::iterator a = state.anchors.begin();
            while (a != state.anchors.end()) {
                if (a.value() == 0)
                    a = state.anchors.erase(a);
                else
                    ++a;
            }
        }
    }

    return yyPos0;
}

void QRegExpEngine::parseAtom(Box *box)
{
#ifndef QT_NO_REGEXP_LOOKAHEAD
    QRegExpEngine *eng = 0;
    bool neg;
    int len;
#endif

    if ((yyTok & Tok_Char) != 0) {
        box->set(QChar(yyTok ^ Tok_Char));
    } else {
#ifndef QT_NO_REGEXP_OPTIM
        trivial = false;
#endif
        switch (yyTok) {
        case Tok_Dollar:
            box->catAnchor(Anchor_Dollar);
            break;
        case Tok_Caret:
            box->catAnchor(Anchor_Caret);
            break;
#ifndef QT_NO_REGEXP_LOOKAHEAD
        case Tok_PosLookahead:
        case Tok_NegLookahead:
            neg = (yyTok == Tok_NegLookahead);
            eng = new QRegExpEngine(cs, greedyQuantifiers);
            len = eng->parse(yyIn + yyPos - 1, yyLen - yyPos + 1);
            if (len >= 0)
                skipChars(len);
            else
                error(RXERR_LOOKAHEAD);
            box->catAnchor(addLookahead(eng, neg));
            yyTok = getToken();
            if (yyTok != Tok_RightParen)
                error(RXERR_LOOKAHEAD);
            break;
#endif
#ifndef QT_NO_REGEXP_ESCAPE
        case Tok_Word:
            box->catAnchor(Anchor_Word);
            break;
        case Tok_NonWord:
            box->catAnchor(Anchor_NonWord);
            break;
#endif
        case Tok_LeftParen:
        case Tok_MagicLeftParen:
            yyTok = getToken();
            parseExpression(box);
            if (yyTok != Tok_RightParen)
                error(RXERR_END);
            break;
        case Tok_CharClass:
            box->set(*yyCharClass);
            break;
        case Tok_Quantifier:
            error(RXERR_REPETITION);
            break;
        default:
#ifndef QT_NO_REGEXP_BACKREF
            if ((yyTok & Tok_BackRef) != 0)
                box->set(yyTok ^ Tok_BackRef);
            else
#endif
                error(RXERR_DISABLED);
        }
    }
    yyTok = getToken();
}

void QRegExpEngine::parseFactor(Box *box)
{
#ifndef QT_NO_REGEXP_CAPTURE
    int outerAtom = greedyQuantifiers ? startAtom(false) : -1;
    int innerAtom = startAtom(yyMayCapture && yyTok == Tok_LeftParen);
    bool magicLeftParen = (yyTok == Tok_MagicLeftParen);
#else
    const int innerAtom = -1;
#endif

#ifndef QT_NO_REGEXP_INTERVAL
#define YYREDO() \
        yyIn = in, yyPos0 = pos0, yyPos = pos, yyLen = len, yyCh = ch, \
        *yyCharClass = charClass, yyMinRep = 0, yyMaxRep = 0, yyTok = tok

    const QChar *in = yyIn;
    int pos0 = yyPos0;
    int pos = yyPos;
    int len = yyLen;
    int ch = yyCh;
    QRegExpCharClass charClass;
    if (yyTok == Tok_CharClass)
        charClass = *yyCharClass;
    int tok = yyTok;
    bool mayCapture = yyMayCapture;
#endif

    parseAtom(box);
#ifndef QT_NO_REGEXP_CAPTURE
    finishAtom(innerAtom, magicLeftParen);
#endif

    bool hasQuantifier = (yyTok == Tok_Quantifier);
    if (hasQuantifier) {
#ifndef QT_NO_REGEXP_OPTIM
        trivial = false;
#endif
        if (yyMaxRep == InftyRep) {
            box->plus(innerAtom);
#ifndef QT_NO_REGEXP_INTERVAL
        } else if (yyMaxRep == 0) {
            box->clear();
#endif
        }
        if (yyMinRep == 0)
            box->opt();

#ifndef QT_NO_REGEXP_INTERVAL
        yyMayCapture = false;
        int alpha = (yyMinRep == 0) ? 0 : yyMinRep - 1;
        int beta = (yyMaxRep == InftyRep) ? 0 : yyMaxRep - (alpha + 1);

        Box rightBox(this);
        int i;

        for (i = 0; i < beta; i++) {
            YYREDO();
            Box leftBox(this);
            parseAtom(&leftBox);
            leftBox.cat(rightBox);
            leftBox.opt();
            rightBox = leftBox;
        }
        for (i = 0; i < alpha; i++) {
            YYREDO();
            Box leftBox(this);
            parseAtom(&leftBox);
            leftBox.cat(rightBox);
            rightBox = leftBox;
        }
        rightBox.cat(*box);
        *box = rightBox;
#endif
        yyTok = getToken();
#ifndef QT_NO_REGEXP_INTERVAL
        yyMayCapture = mayCapture;
#endif
    }
#undef YYREDO
#ifndef QT_NO_REGEXP_CAPTURE
    if (greedyQuantifiers)
        finishAtom(outerAtom, hasQuantifier);
#endif
}

void QRegExpEngine::parseTerm(Box *box)
{
#ifndef QT_NO_REGEXP_OPTIM
    if (yyTok != Tok_Eos && yyTok != Tok_RightParen && yyTok != Tok_Bar)
        parseFactor(box);
#endif
    while (yyTok != Tok_Eos && yyTok != Tok_RightParen && yyTok != Tok_Bar) {
        Box rightBox(this);
        parseFactor(&rightBox);
        box->cat(rightBox);
    }
}

void QRegExpEngine::parseExpression(Box *box)
{
    parseTerm(box);
    while (yyTok == Tok_Bar) {
#ifndef QT_NO_REGEXP_OPTIM
        trivial = false;
#endif
        Box rightBox(this);
        yyTok = getToken();
        parseTerm(&rightBox);
        box->orx(rightBox);
    }
}

/*
  The struct QRegExpPrivate contains the private data of a regular
  expression other than the automaton. It makes it possible for many
  QRegExp objects to use the same QRegExpEngine object with different
  QRegExpPrivate objects.
*/
struct QRegExpPrivate
{
    QRegExpEngine *eng;
    QRegExpEngineKey engineKey;
    bool minimal;
#ifndef QT_NO_REGEXP_CAPTURE
    QString t; // last string passed to QRegExp::indexIn() or lastIndexIn()
    QStringList capturedCache; // what QRegExp::capturedTexts() returned last
#endif
    QRegExpMatchState matchState;

    inline QRegExpPrivate()
        : eng(0), engineKey(QString(), QRegExp::RegExp, Qt::CaseSensitive), minimal(false) { }
    inline QRegExpPrivate(const QRegExpEngineKey &key)
        : eng(0), engineKey(key), minimal(false) {}
};

#if !defined(QT_NO_REGEXP_OPTIM)
struct QRECache
{
    typedef QHash<QRegExpEngineKey, QRegExpEngine *> EngineCache;
    typedef QCache<QRegExpEngineKey, QRegExpEngine> UnusedEngineCache;
    EngineCache usedEngines;
    UnusedEngineCache unusedEngines;
};
Q_GLOBAL_STATIC(QRECache, engineCache)
static QBasicMutex engineCacheMutex;
#endif // QT_NO_REGEXP_OPTIM

static void derefEngine(QRegExpEngine *eng, const QRegExpEngineKey &key)
{
#if !defined(QT_NO_REGEXP_OPTIM)
    QMutexLocker locker(&engineCacheMutex);
    if (!eng->ref.deref()) {
        if (QRECache *c = engineCache()) {
            c->unusedEngines.insert(key, eng, 4 + key.pattern.length() / 4);
            c->usedEngines.remove(key);
        } else {
            delete eng;
        }
    }
#else
    Q_UNUSED(key);
    if (!eng->ref.deref())
        delete eng;
#endif
}

static void prepareEngine_helper(QRegExpPrivate *priv)
{
    Q_ASSERT(!priv->eng);

#if !defined(QT_NO_REGEXP_OPTIM)
    QMutexLocker locker(&engineCacheMutex);
    if (QRECache *c = engineCache()) {
        priv->eng = c->unusedEngines.take(priv->engineKey);
        if (!priv->eng)
            priv->eng = c->usedEngines.value(priv->engineKey);
        if (!priv->eng)
            priv->eng = new QRegExpEngine(priv->engineKey);
        else
            priv->eng->ref.ref();

        c->usedEngines.insert(priv->engineKey, priv->eng);
        return;
    }
#endif // QT_NO_REGEXP_OPTIM

    priv->eng = new QRegExpEngine(priv->engineKey);
}

inline static void prepareEngine(QRegExpPrivate *priv)
{
    if (priv->eng)
        return;
    prepareEngine_helper(priv);
    priv->matchState.prepareForMatch(priv->eng);
}

static void prepareEngineForMatch(QRegExpPrivate *priv, const QString &str)
{
    prepareEngine(priv);
    priv->matchState.prepareForMatch(priv->eng);
#ifndef QT_NO_REGEXP_CAPTURE
    priv->t = str;
    priv->capturedCache.clear();
#else
    Q_UNUSED(str);
#endif
}

static void invalidateEngine(QRegExpPrivate *priv)
{
    if (priv->eng != 0) {
        derefEngine(priv->eng, priv->engineKey);
        priv->eng = 0;
        priv->matchState.drain();
    }
}

/*!
    \enum QRegExp::CaretMode

    The CaretMode enum defines the different meanings of the caret
    (\b{^}) in a regular expression. The possible values are:

    \value CaretAtZero
           The caret corresponds to index 0 in the searched string.

    \value CaretAtOffset
           The caret corresponds to the start offset of the search.

    \value CaretWontMatch
           The caret never matches.
*/

/*!
    \enum QRegExp::PatternSyntax

    The syntax used to interpret the meaning of the pattern.

    \value RegExp A rich Perl-like pattern matching syntax. This is
    the default.

    \value RegExp2 Like RegExp, but with \l{greedy quantifiers}.
    (Introduced in Qt 4.2.)

    \value Wildcard This provides a simple pattern matching syntax
    similar to that used by shells (command interpreters) for "file
    globbing". See \l{QRegExp wildcard matching}.

    \value WildcardUnix This is similar to Wildcard but with the
    behavior of a Unix shell. The wildcard characters can be escaped
    with the character "\\".

    \value FixedString The pattern is a fixed string. This is
    equivalent to using the RegExp pattern on a string in
    which all metacharacters are escaped using escape().

    \value W3CXmlSchema11 The pattern is a regular expression as
    defined by the W3C XML Schema 1.1 specification.

    \sa setPatternSyntax()
*/

/*!
    Constructs an empty regexp.

    \sa isValid(), errorString()
*/
QRegExp::QRegExp()
{
    priv = new QRegExpPrivate;
    prepareEngine(priv);
}

/*!
    Constructs a regular expression object for the given \a pattern
    string. The pattern must be given using wildcard notation if \a
    syntax is \l Wildcard; the default is \l RegExp. The pattern is
    case sensitive, unless \a cs is Qt::CaseInsensitive. Matching is
    greedy (maximal), but can be changed by calling
    setMinimal().

    \sa setPattern(), setCaseSensitivity(), setPatternSyntax()
*/
QRegExp::QRegExp(const QString &pattern, Qt::CaseSensitivity cs, PatternSyntax syntax)
{
    priv = new QRegExpPrivate(QRegExpEngineKey(pattern, syntax, cs));
    prepareEngine(priv);
}

/*!
    Constructs a regular expression as a copy of \a rx.

    \sa operator=()
*/
QRegExp::QRegExp(const QRegExp &rx)
{
    priv = new QRegExpPrivate;
    operator=(rx);
}

/*!
    Destroys the regular expression and cleans up its internal data.
*/
QRegExp::~QRegExp()
{
    invalidateEngine(priv);
    delete priv;
}

/*!
    Copies the regular expression \a rx and returns a reference to the
    copy. The case sensitivity, wildcard, and minimal matching options
    are also copied.
*/
QRegExp &QRegExp::operator=(const QRegExp &rx)
{
    prepareEngine(rx.priv); // to allow sharing
    QRegExpEngine *otherEng = rx.priv->eng;
    if (otherEng)
        otherEng->ref.ref();
    invalidateEngine(priv);
    priv->eng = otherEng;
    priv->engineKey = rx.priv->engineKey;
    priv->minimal = rx.priv->minimal;
#ifndef QT_NO_REGEXP_CAPTURE
    priv->t = rx.priv->t;
    priv->capturedCache = rx.priv->capturedCache;
#endif
    if (priv->eng)
        priv->matchState.prepareForMatch(priv->eng);
    priv->matchState.captured = rx.priv->matchState.captured;
    return *this;
}

/*!
    \fn QRegExp &QRegExp::operator=(QRegExp &&other)

    Move-assigns \a other to this QRegExp instance.

    \since 5.2
*/

/*!
    \fn void QRegExp::swap(QRegExp &other)
    \since 4.8

    Swaps regular expression \a other with this regular
    expression. This operation is very fast and never fails.
*/

/*!
    Returns \c true if this regular expression is equal to \a rx;
    otherwise returns \c false.

    Two QRegExp objects are equal if they have the same pattern
    strings and the same settings for case sensitivity, wildcard and
    minimal matching.
*/
bool QRegExp::operator==(const QRegExp &rx) const
{
    return priv->engineKey == rx.priv->engineKey && priv->minimal == rx.priv->minimal;
}

/*!
    \since 5.6
    \relates QRegExp

    Returns the hash value for \a key, using
    \a seed to seed the calculation.
*/
uint qHash(const QRegExp &key, uint seed) Q_DECL_NOTHROW
{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.priv->engineKey);
    seed = hash(seed, key.priv->minimal);
    return seed;
}

/*!
    \fn bool QRegExp::operator!=(const QRegExp &rx) const

    Returns \c true if this regular expression is not equal to \a rx;
    otherwise returns \c false.

    \sa operator==()
*/

/*!
    Returns \c true if the pattern string is empty; otherwise returns
    false.

    If you call exactMatch() with an empty pattern on an empty string
    it will return true; otherwise it returns \c false since it operates
    over the whole string. If you call indexIn() with an empty pattern
    on \e any string it will return the start offset (0 by default)
    because the empty pattern matches the 'emptiness' at the start of
    the string. In this case the length of the match returned by
    matchedLength() will be 0.

    See QString::isEmpty().
*/

bool QRegExp::isEmpty() const
{
    return priv->engineKey.pattern.isEmpty();
}

/*!
    Returns \c true if the regular expression is valid; otherwise returns
    false. An invalid regular expression never matches.

    The pattern \b{[a-z} is an example of an invalid pattern, since
    it lacks a closing square bracket.

    Note that the validity of a regexp may also depend on the setting
    of the wildcard flag, for example \b{*.html} is a valid
    wildcard regexp but an invalid full regexp.

    \sa errorString()
*/
bool QRegExp::isValid() const
{
    if (priv->engineKey.pattern.isEmpty()) {
        return true;
    } else {
        prepareEngine(priv);
        return priv->eng->isValid();
    }
}

/*!
    Returns the pattern string of the regular expression. The pattern
    has either regular expression syntax or wildcard syntax, depending
    on patternSyntax().

    \sa patternSyntax(), caseSensitivity()
*/
QString QRegExp::pattern() const
{
    return priv->engineKey.pattern;
}

/*!
    Sets the pattern string to \a pattern. The case sensitivity,
    wildcard, and minimal matching options are not changed.

    \sa setPatternSyntax(), setCaseSensitivity()
*/
void QRegExp::setPattern(const QString &pattern)
{
    if (priv->engineKey.pattern != pattern) {
        invalidateEngine(priv);
        priv->engineKey.pattern = pattern;
    }
}

/*!
    Returns Qt::CaseSensitive if the regexp is matched case
    sensitively; otherwise returns Qt::CaseInsensitive.

    \sa patternSyntax(), pattern(), isMinimal()
*/
Qt::CaseSensitivity QRegExp::caseSensitivity() const
{
    return priv->engineKey.cs;
}

/*!
    Sets case sensitive matching to \a cs.

    If \a cs is Qt::CaseSensitive, \b{\\.txt$} matches
    \c{readme.txt} but not \c{README.TXT}.

    \sa setPatternSyntax(), setPattern(), setMinimal()
*/
void QRegExp::setCaseSensitivity(Qt::CaseSensitivity cs)
{
    if ((bool)cs != (bool)priv->engineKey.cs) {
        invalidateEngine(priv);
        priv->engineKey.cs = cs;
    }
}

/*!
    Returns the syntax used by the regular expression. The default is
    QRegExp::RegExp.

    \sa pattern(), caseSensitivity()
*/
QRegExp::PatternSyntax QRegExp::patternSyntax() const
{
    return priv->engineKey.patternSyntax;
}

/*!
    Sets the syntax mode for the regular expression. The default is
    QRegExp::RegExp.

    Setting \a syntax to QRegExp::Wildcard enables simple shell-like
    \l{QRegExp wildcard matching}. For example, \b{r*.txt} matches the
    string \c{readme.txt} in wildcard mode, but does not match
    \c{readme}.

    Setting \a syntax to QRegExp::FixedString means that the pattern
    is interpreted as a plain string. Special characters (e.g.,
    backslash) don't need to be escaped then.

    \sa setPattern(), setCaseSensitivity(), escape()
*/
void QRegExp::setPatternSyntax(PatternSyntax syntax)
{
    if (syntax != priv->engineKey.patternSyntax) {
        invalidateEngine(priv);
        priv->engineKey.patternSyntax = syntax;
    }
}

/*!
    Returns \c true if minimal (non-greedy) matching is enabled;
    otherwise returns \c false.

    \sa caseSensitivity(), setMinimal()
*/
bool QRegExp::isMinimal() const
{
    return priv->minimal;
}

/*!
    Enables or disables minimal matching. If \a minimal is false,
    matching is greedy (maximal) which is the default.

    For example, suppose we have the input string "We must be
    <b>bold</b>, very <b>bold</b>!" and the pattern
    \b{<b>.*</b>}. With the default greedy (maximal) matching,
    the match is "We must be \underline{<b>bold</b>, very
    <b>bold</b>}!". But with minimal (non-greedy) matching, the
    first match is: "We must be \underline{<b>bold</b>}, very
    <b>bold</b>!" and the second match is "We must be <b>bold</b>,
    very \underline{<b>bold</b>}!". In practice we might use the pattern
    \b{<b>[^<]*\</b>} instead, although this will still fail for
    nested tags.

    \sa setCaseSensitivity()
*/
void QRegExp::setMinimal(bool minimal)
{
    priv->minimal = minimal;
}

// ### Qt 5: make non-const
/*!
    Returns \c true if \a str is matched exactly by this regular
    expression; otherwise returns \c false. You can determine how much of
    the string was matched by calling matchedLength().

    For a given regexp string R, exactMatch("R") is the equivalent of
    indexIn("^R$") since exactMatch() effectively encloses the regexp
    in the start of string and end of string anchors, except that it
    sets matchedLength() differently.

    For example, if the regular expression is \b{blue}, then
    exactMatch() returns \c true only for input \c blue. For inputs \c
    bluebell, \c blutak and \c lightblue, exactMatch() returns \c false
    and matchedLength() will return 4, 3 and 0 respectively.

    Although const, this function sets matchedLength(),
    capturedTexts(), and pos().

    \sa indexIn(), lastIndexIn()
*/
bool QRegExp::exactMatch(const QString &str) const
{
    prepareEngineForMatch(priv, str);
    priv->matchState.match(str.unicode(), str.length(), 0, priv->minimal, true, 0);
    if (priv->matchState.captured[1] == str.length()) {
        return true;
    } else {
        priv->matchState.captured[0] = 0;
        priv->matchState.captured[1] = priv->matchState.oneTestMatchedLen;
        return false;
    }
}

// ### Qt 5: make non-const
/*!
    Attempts to find a match in \a str from position \a offset (0 by
    default). If \a offset is -1, the search starts at the last
    character; if -2, at the next to last character; etc.

    Returns the position of the first match, or -1 if there was no
    match.

    The \a caretMode parameter can be used to instruct whether \b{^}
    should match at index 0 or at \a offset.

    You might prefer to use QString::indexOf(), QString::contains(),
    or even QStringList::filter(). To replace matches use
    QString::replace().

    Example:
    \snippet code/src_corelib_tools_qregexp.cpp 13

    Although const, this function sets matchedLength(),
    capturedTexts() and pos().

    If the QRegExp is a wildcard expression (see setPatternSyntax())
    and want to test a string against the whole wildcard expression,
    use exactMatch() instead of this function.

    \sa lastIndexIn(), exactMatch()
*/

int QRegExp::indexIn(const QString &str, int offset, CaretMode caretMode) const
{
    prepareEngineForMatch(priv, str);
    if (offset < 0)
        offset += str.length();
    priv->matchState.match(str.unicode(), str.length(), offset,
        priv->minimal, false, caretIndex(offset, caretMode));
    return priv->matchState.captured[0];
}

// ### Qt 5: make non-const
/*!
    Attempts to find a match backwards in \a str from position \a
    offset. If \a offset is -1 (the default), the search starts at the
    last character; if -2, at the next to last character; etc.

    Returns the position of the first match, or -1 if there was no
    match.

    The \a caretMode parameter can be used to instruct whether \b{^}
    should match at index 0 or at \a offset.

    Although const, this function sets matchedLength(),
    capturedTexts() and pos().

    \warning Searching backwards is much slower than searching
    forwards.

    \sa indexIn(), exactMatch()
*/

int QRegExp::lastIndexIn(const QString &str, int offset, CaretMode caretMode) const
{
    prepareEngineForMatch(priv, str);
    if (offset < 0)
        offset += str.length();
    if (offset < 0 || offset > str.length()) {
        memset(priv->matchState.captured, -1, priv->matchState.capturedSize*sizeof(int));
        return -1;
    }

    while (offset >= 0) {
        priv->matchState.match(str.unicode(), str.length(), offset,
            priv->minimal, true, caretIndex(offset, caretMode));
        if (priv->matchState.captured[0] == offset)
            return offset;
        --offset;
    }
    return -1;
}

/*!
    Returns the length of the last matched string, or -1 if there was
    no match.

    \sa exactMatch(), indexIn(), lastIndexIn()
*/
int QRegExp::matchedLength() const
{
    return priv->matchState.captured[1];
}

#ifndef QT_NO_REGEXP_CAPTURE

/*!
  \since 4.6
  Returns the number of captures contained in the regular expression.
 */
int QRegExp::captureCount() const
{
    prepareEngine(priv);
    return priv->eng->captureCount();
}

/*!
    Returns a list of the captured text strings.

    The first string in the list is the entire matched string. Each
    subsequent list element contains a string that matched a
    (capturing) subexpression of the regexp.

    For example:
    \snippet code/src_corelib_tools_qregexp.cpp 14

    The above example also captures elements that may be present but
    which we have no interest in. This problem can be solved by using
    non-capturing parentheses:

    \snippet code/src_corelib_tools_qregexp.cpp 15

    Note that if you want to iterate over the list, you should iterate
    over a copy, e.g.
    \snippet code/src_corelib_tools_qregexp.cpp 16

    Some regexps can match an indeterminate number of times. For
    example if the input string is "Offsets: 12 14 99 231 7" and the
    regexp, \c{rx}, is \b{(\\d+)+}, we would hope to get a list of
    all the numbers matched. However, after calling
    \c{rx.indexIn(str)}, capturedTexts() will return the list ("12",
    "12"), i.e. the entire match was "12" and the first subexpression
    matched was "12". The correct approach is to use cap() in a
    \l{QRegExp#cap_in_a_loop}{loop}.

    The order of elements in the string list is as follows. The first
    element is the entire matching string. Each subsequent element
    corresponds to the next capturing open left parentheses. Thus
    capturedTexts()[1] is the text of the first capturing parentheses,
    capturedTexts()[2] is the text of the second and so on
    (corresponding to $1, $2, etc., in some other regexp languages).

    \sa cap(), pos()
*/
QStringList QRegExp::capturedTexts() const
{
    if (priv->capturedCache.isEmpty()) {
        prepareEngine(priv);
        const int *captured = priv->matchState.captured;
        int n = priv->matchState.capturedSize;

        for (int i = 0; i < n; i += 2) {
            QString m;
            if (captured[i + 1] == 0)
                m = QLatin1String(""); // ### Qt 5: don't distinguish between null and empty
            else if (captured[i] >= 0)
                m = priv->t.mid(captured[i], captured[i + 1]);
            priv->capturedCache.append(m);
        }
        priv->t.clear();
    }
    return priv->capturedCache;
}

/*!
    \internal
*/
QStringList QRegExp::capturedTexts()
{
    return const_cast<const QRegExp *>(this)->capturedTexts();
}

/*!
    Returns the text captured by the \a nth subexpression. The entire
    match has index 0 and the parenthesized subexpressions have
    indexes starting from 1 (excluding non-capturing parentheses).

    \snippet code/src_corelib_tools_qregexp.cpp 17

    The order of elements matched by cap() is as follows. The first
    element, cap(0), is the entire matching string. Each subsequent
    element corresponds to the next capturing open left parentheses.
    Thus cap(1) is the text of the first capturing parentheses, cap(2)
    is the text of the second, and so on.

    \sa capturedTexts(), pos()
*/
QString QRegExp::cap(int nth) const
{
    return capturedTexts().value(nth);
}

/*!
    \internal
*/
QString QRegExp::cap(int nth)
{
    return const_cast<const QRegExp *>(this)->cap(nth);
}

/*!
    Returns the position of the \a nth captured text in the searched
    string. If \a nth is 0 (the default), pos() returns the position
    of the whole match.

    Example:
    \snippet code/src_corelib_tools_qregexp.cpp 18

    For zero-length matches, pos() always returns -1. (For example, if
    cap(4) would return an empty string, pos(4) returns -1.) This is
    a feature of the implementation.

    \sa cap(), capturedTexts()
*/
int QRegExp::pos(int nth) const
{
    if (nth < 0 || nth >= priv->matchState.capturedSize / 2)
        return -1;
    else
        return priv->matchState.captured[2 * nth];
}

/*!
    \internal
*/
int QRegExp::pos(int nth)
{
    return const_cast<const QRegExp *>(this)->pos(nth);
}

/*!
  Returns a text string that explains why a regexp pattern is
  invalid the case being; otherwise returns "no error occurred".

  \sa isValid()
*/
QString QRegExp::errorString() const
{
    if (isValid()) {
        return QString::fromLatin1(RXERR_OK);
    } else {
        return priv->eng->errorString();
    }
}

/*!
    \internal
*/
QString QRegExp::errorString()
{
    return const_cast<const QRegExp *>(this)->errorString();
}
#endif

/*!
    Returns the string \a str with every regexp special character
    escaped with a backslash. The special characters are $, (,), *, +,
    ., ?, [, \,], ^, {, | and }.

    Example:

    \snippet code/src_corelib_tools_qregexp.cpp 19

    This function is useful to construct regexp patterns dynamically:

    \snippet code/src_corelib_tools_qregexp.cpp 20

    \sa setPatternSyntax()
*/
QString QRegExp::escape(const QString &str)
{
    QString quoted;
    const int count = str.count();
    quoted.reserve(count * 2);
    const QLatin1Char backslash('\\');
    for (int i = 0; i < count; i++) {
        switch (str.at(i).toLatin1()) {
        case '$':
        case '(':
        case ')':
        case '*':
        case '+':
        case '.':
        case '?':
        case '[':
        case '\\':
        case ']':
        case '^':
        case '{':
        case '|':
        case '}':
            quoted.append(backslash);
        }
        quoted.append(str.at(i));
    }
    return quoted;
}


#ifndef QT_NO_DATASTREAM
/*!
    \relates QRegExp

    Writes the regular expression \a regExp to stream \a out.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator<<(QDataStream &out, const QRegExp &regExp)
{
    return out << regExp.pattern() << (quint8)regExp.caseSensitivity()
               << (quint8)regExp.patternSyntax()
               << (quint8)!!regExp.isMinimal();
}

/*!
    \relates QRegExp

    Reads a regular expression from stream \a in into \a regExp.

    \sa {Serializing Qt Data Types}
*/
QDataStream &operator>>(QDataStream &in, QRegExp &regExp)
{
    QString pattern;
    quint8 cs;
    quint8 patternSyntax;
    quint8 isMinimal;

    in >> pattern >> cs >> patternSyntax >> isMinimal;

    QRegExp newRegExp(pattern, Qt::CaseSensitivity(cs),
                      QRegExp::PatternSyntax(patternSyntax));

    newRegExp.setMinimal(isMinimal);
    regExp = newRegExp;
    return in;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QRegExp &r)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QRegExp(patternSyntax=" << r.patternSyntax()
                  << ", pattern='"<< r.pattern() << "')";
    return dbg;
}
#endif

QT_END_NAMESPACE
