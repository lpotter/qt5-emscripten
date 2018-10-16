/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the utils of the Qt Toolkit.
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
// auto generated. DO NOT EDIT.
class RegExpTokenizer
{
public:
    RegExpTokenizer(const QString &inp);

    inline QChar next() {
        return (pos < input.length()) ? input.at(pos++) : QChar();
    }
    int lex();

    QString input;
    int pos;
    int lexemStart;
    int lexemLength;
};

RegExpTokenizer::RegExpTokenizer(const QString &inp)
{
    input = inp;
    pos = 0;
    lexemStart = 0;
    lexemLength = 0;
}


int RegExpTokenizer::lex()
{
    lexemStart = pos;
    lexemLength = 0;
    int lastAcceptingPos = -1;
    int token = -1;
    QChar ch;
    
    // initial state
        ch = next();
        if (ch.unicode() >= 1 && ch.unicode() <= 33)
            goto state_1;
        if (ch.unicode() == 34)
            goto state_2;
        if (ch.unicode() >= 35 && ch.unicode() <= 39)
            goto state_1;
        if (ch.unicode() == 40) {
            token = RE2NFA::TOK_LPAREN;
            goto found;
        }
        if (ch.unicode() == 41) {
            token = RE2NFA::TOK_RPAREN;
            goto found;
        }
        if (ch.unicode() == 42) {
            token = RE2NFA::TOK_STAR;
            goto found;
        }
        if (ch.unicode() == 43) {
            token = RE2NFA::TOK_PLUS;
            goto found;
        }
        if (ch.unicode() == 44) {
            token = RE2NFA::TOK_COMMA;
            goto found;
        }
        if (ch.unicode() == 45)
            goto state_1;
        if (ch.unicode() == 46) {
            token = RE2NFA::TOK_DOT;
            goto found;
        }
        if (ch.unicode() >= 47 && ch.unicode() <= 62)
            goto state_1;
        if (ch.unicode() == 63) {
            token = RE2NFA::TOK_QUESTION;
            goto found;
        }
        if (ch.unicode() >= 64 && ch.unicode() <= 90)
            goto state_1;
        if (ch.unicode() == 91)
            goto state_10;
        if (ch.unicode() == 92)
            goto state_11;
        if (ch.unicode() >= 93 && ch.unicode() <= 122)
            goto state_1;
        if (ch.unicode() == 123) {
            token = RE2NFA::TOK_LBRACE;
            goto found;
        }
        if (ch.unicode() == 124) {
            token = RE2NFA::TOK_OR;
            goto found;
        }
        if (ch.unicode() == 125) {
            token = RE2NFA::TOK_RBRACE;
            goto found;
        }
        if (ch.unicode() >= 126)
            goto state_1;
        goto out;
    state_1:
        lastAcceptingPos = pos;
        token = RE2NFA::TOK_STRING;
        goto out;
    state_2:
        lastAcceptingPos = pos;
        token = RE2NFA::TOK_STRING;
        ch = next();
        if (ch.unicode() >= 1 && ch.unicode() <= 33)
            goto state_15;
        if (ch.unicode() == 34)
            goto state_16;
        if (ch.unicode() >= 35)
            goto state_15;
        goto out;
    state_10:
        ch = next();
        if (ch.unicode() >= 1 && ch.unicode() <= 91)
            goto state_17;
        if (ch.unicode() == 92)
            goto state_18;
        if (ch.unicode() == 93)
            goto state_19;
        if (ch.unicode() >= 94)
            goto state_17;
        goto out;
    state_11:
        lastAcceptingPos = pos;
        token = RE2NFA::TOK_STRING;
        ch = next();
        if (ch.unicode() >= 1)
            goto state_20;
        goto out;
    state_15:
        ch = next();
        if (ch.unicode() >= 1 && ch.unicode() <= 33)
            goto state_15;
        if (ch.unicode() == 34)
            goto state_16;
        if (ch.unicode() >= 35)
            goto state_15;
        goto out;
    state_16:
        lastAcceptingPos = pos;
        token = RE2NFA::TOK_QUOTED_STRING;
        goto out;
    state_17:
        ch = next();
        if (ch.unicode() >= 1 && ch.unicode() <= 91)
            goto state_17;
        if (ch.unicode() == 92)
            goto state_18;
        if (ch.unicode() == 93)
            goto state_19;
        if (ch.unicode() >= 94)
            goto state_17;
        goto out;
    state_18:
        ch = next();
        if (ch.unicode() >= 1 && ch.unicode() <= 91)
            goto state_17;
        if (ch.unicode() == 92)
            goto state_18;
        if (ch.unicode() == 93)
            goto state_21;
        if (ch.unicode() >= 94)
            goto state_17;
        goto out;
    state_19:
        lastAcceptingPos = pos;
        token = RE2NFA::TOK_SEQUENCE;
        goto out;
    state_20:
        lastAcceptingPos = pos;
        token = RE2NFA::TOK_STRING;
        goto out;
    state_21:
        lastAcceptingPos = pos;
        token = RE2NFA::TOK_SEQUENCE;
        ch = next();
        if (ch.unicode() >= 1 && ch.unicode() <= 91)
            goto state_17;
        if (ch.unicode() == 92)
            goto state_18;
        if (ch.unicode() == 93)
            goto state_19;
        if (ch.unicode() >= 94)
            goto state_17;
        goto out;
    found:
    lastAcceptingPos = pos;
    
    out:
    if (lastAcceptingPos != -1) {
        lexemLength = lastAcceptingPos - lexemStart;
        pos = lastAcceptingPos;
    }
    return token;
}

