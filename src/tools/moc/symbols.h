/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "token.h"
#include <qstring.h>
#include <qhash.h>
#include <qvector.h>
#include <qstack.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

//#define USE_LEXEM_STORE

struct SubArray
{
    inline SubArray():from(0),len(-1){}
    inline SubArray(const QByteArray &a):array(a),from(0), len(a.size()){}
    inline SubArray(const char *s):array(s),from(0) { len = array.size(); }
    inline SubArray(const QByteArray &a, int from, int len):array(a), from(from), len(len){}
    QByteArray array;
    int from, len;
    inline bool operator==(const SubArray &other) const {
        if (len != other.len)
            return false;
        for (int i = 0; i < len; ++i)
            if (array.at(from + i) != other.array.at(other.from + i))
                return false;
        return true;
    }
};

inline uint qHash(const SubArray &key)
{
    return qHash(QLatin1String(key.array.constData() + key.from, key.len));
}


struct Symbol
{

#ifdef USE_LEXEM_STORE
    typedef QHash<SubArray, QHashDummyValue> LexemStore;
    static LexemStore lexemStore;

    inline Symbol() : lineNum(-1),token(NOTOKEN){}
    inline Symbol(int lineNum, Token token):
        lineNum(lineNum), token(token){}
    inline Symbol(int lineNum, Token token, const QByteArray &lexem):
        lineNum(lineNum), token(token),lex(lexem){}
    inline Symbol(int lineNum, Token token, const QByteArray &lexem, int from, int len):
        lineNum(lineNum), token(token){
        LexemStore::const_iterator it = lexemStore.constFind(SubArray(lexem, from, len));

        if (it != lexemStore.constEnd()) {
            lex = it.key().array;
        } else {
            lex = lexem.mid(from, len);
            lexemStore.insert(lex, QHashDummyValue());
        }
    }
    int lineNum;
    Token token;
    inline QByteArray unquotedLexem() const { return lex.mid(1, lex.length()-2); }
    inline QByteArray lexem() const { return lex; }
    inline operator QByteArray() const { return lex; }
    QByteArray lex;

#else

    inline Symbol() : lineNum(-1),token(NOTOKEN), from(0),len(-1) {}
    inline Symbol(int lineNum, Token token):
        lineNum(lineNum), token(token), from(0), len(-1) {}
    inline Symbol(int lineNum, Token token, const QByteArray &lexem):
        lineNum(lineNum), token(token), lex(lexem), from(0) { len = lex.size(); }
    inline Symbol(int lineNum, Token token, const QByteArray &lexem, int from, int len):
        lineNum(lineNum), token(token),lex(lexem),from(from), len(len){}
    int lineNum;
    Token token;
    inline QByteArray lexem() const { return lex.mid(from, len); }
    inline QByteArray unquotedLexem() const { return lex.mid(from+1, len-2); }
    inline operator SubArray() const { return SubArray(lex, from, len); }
    bool operator==(const Symbol& o) const
    {
        return SubArray(lex, from, len) == SubArray(o.lex, o.from, o.len);
    }
    QByteArray lex;
    int from, len;

#endif
};
Q_DECLARE_TYPEINFO(Symbol, Q_MOVABLE_TYPE);

typedef QVector<Symbol> Symbols;

struct SafeSymbols {
    Symbols symbols;
    QByteArray expandedMacro;
    QSet<QByteArray> excludedSymbols;
    int index;
};
Q_DECLARE_TYPEINFO(SafeSymbols, Q_MOVABLE_TYPE);

class SymbolStack : public QStack<SafeSymbols>
{
public:
    inline bool hasNext() {
        while (!isEmpty() && top().index >= top().symbols.size())
            pop();
        return !isEmpty();
    }
    inline Token next() {
        while (!isEmpty() && top().index >= top().symbols.size())
            pop();
        if (isEmpty())
            return NOTOKEN;
        return top().symbols.at(top().index++).token;
    }
    bool test(Token);
    inline const Symbol &symbol() const { return top().symbols.at(top().index-1); }
    inline Token token() { return symbol().token; }
    inline QByteArray lexem() const { return symbol().lexem(); }
    inline QByteArray unquotedLexem() { return symbol().unquotedLexem(); }

    bool dontReplaceSymbol(const QByteArray &name);
    QSet<QByteArray> excludeSymbols();
};

inline bool SymbolStack::test(Token token)
{
    int stackPos = size() - 1;
    while (stackPos >= 0 && at(stackPos).index >= at(stackPos).symbols.size())
        --stackPos;
    if (stackPos < 0)
        return false;
    if (at(stackPos).symbols.at(at(stackPos).index).token == token) {
        next();
        return true;
    }
    return false;
}

inline bool SymbolStack::dontReplaceSymbol(const QByteArray &name)
{
    for (int i = 0; i < size(); ++i) {
        if (name == at(i).expandedMacro || at(i).excludedSymbols.contains(name))
            return true;
    }
    return false;
}

inline QSet<QByteArray> SymbolStack::excludeSymbols()
{
    QSet<QByteArray> set;
    for (int i = 0; i < size(); ++i) {
        set << at(i).expandedMacro;
        set += at(i).excludedSymbols;
    }
    return set;
}

QT_END_NAMESPACE

#endif // SYMBOLS_H
