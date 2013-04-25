/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Intel Corporation.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QURL_H
#define QURL_H

#include <QtCore/qbytearray.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE


class QUrlQuery;
class QUrlPrivate;
class QDataStream;

template <typename E1, typename E2>
class QUrlTwoFlags
{
    int i;
    typedef int QUrlTwoFlags:: *Zero;
public:
    Q_DECL_CONSTEXPR inline QUrlTwoFlags(E1 f) : i(f) {}
    Q_DECL_CONSTEXPR inline QUrlTwoFlags(E2 f) : i(f) {}
    Q_DECL_CONSTEXPR inline QUrlTwoFlags(QFlag f) : i(f) {}
    Q_DECL_CONSTEXPR inline QUrlTwoFlags(QFlags<E1> f) : i(f.operator int()) {}
    Q_DECL_CONSTEXPR inline QUrlTwoFlags(QFlags<E2> f) : i(f.operator int()) {}
    Q_DECL_CONSTEXPR inline QUrlTwoFlags(Zero = 0) : i(0) {}

    inline QUrlTwoFlags &operator&=(int mask) { i &= mask; return *this; }
    inline QUrlTwoFlags &operator&=(uint mask) { i &= mask; return *this; }
    inline QUrlTwoFlags &operator|=(QUrlTwoFlags f) { i |= f.i; return *this; }
    inline QUrlTwoFlags &operator|=(E1 f) { i |= f; return *this; }
    inline QUrlTwoFlags &operator|=(E2 f) { i |= f; return *this; }
    inline QUrlTwoFlags &operator^=(QUrlTwoFlags f) { i ^= f.i; return *this; }
    inline QUrlTwoFlags &operator^=(E1 f) { i ^= f; return *this; }
    inline QUrlTwoFlags &operator^=(E2 f) { i ^= f; return *this; }

    Q_DECL_CONSTEXPR inline operator QFlags<E1>() const { return E1(i); }
    Q_DECL_CONSTEXPR inline operator QFlags<E2>() const { return E2(i); }
    Q_DECL_CONSTEXPR inline operator int() const { return i; }
    Q_DECL_CONSTEXPR inline bool operator!() const { return !i; }

    Q_DECL_CONSTEXPR inline QUrlTwoFlags operator|(QUrlTwoFlags f) const
    { return QUrlTwoFlags(E1(i | f.i)); }
    Q_DECL_CONSTEXPR inline QUrlTwoFlags operator|(E1 f) const
    { return QUrlTwoFlags(E1(i | f)); }
    Q_DECL_CONSTEXPR inline QUrlTwoFlags operator|(E2 f) const
    { return QUrlTwoFlags(E2(i | f)); }
    Q_DECL_CONSTEXPR inline QUrlTwoFlags operator^(QUrlTwoFlags f) const
    { return QUrlTwoFlags(E1(i ^ f.i)); }
    Q_DECL_CONSTEXPR inline QUrlTwoFlags operator^(E1 f) const
    { return QUrlTwoFlags(E1(i ^ f)); }
    Q_DECL_CONSTEXPR inline QUrlTwoFlags operator^(E2 f) const
    { return QUrlTwoFlags(E2(i ^ f)); }
    Q_DECL_CONSTEXPR inline QUrlTwoFlags operator&(int mask) const
    { return QUrlTwoFlags(E1(i & mask)); }
    Q_DECL_CONSTEXPR inline QUrlTwoFlags operator&(uint mask) const
    { return QUrlTwoFlags(E1(i & mask)); }
    Q_DECL_CONSTEXPR inline QUrlTwoFlags operator&(E1 f) const
    { return QUrlTwoFlags(E1(i & f)); }
    Q_DECL_CONSTEXPR inline QUrlTwoFlags operator&(E2 f) const
    { return QUrlTwoFlags(E2(i & f)); }
    Q_DECL_CONSTEXPR inline QUrlTwoFlags operator~() const
    { return QUrlTwoFlags(E1(~i)); }

    inline bool testFlag(E1 f) const { return (i & f) == f && (f != 0 || i == int(f)); }
    inline bool testFlag(E2 f) const { return (i & f) == f && (f != 0 || i == int(f)); }
};

template<typename E1, typename E2>
class QTypeInfo<QUrlTwoFlags<E1, E2> > : public QTypeInfoMerger<QUrlTwoFlags<E1, E2>, E1, E2> {};

class QUrl;
// qHash is a friend, but we can't use default arguments for friends (§8.3.6.4)
Q_CORE_EXPORT uint qHash(const QUrl &url, uint seed = 0) Q_DECL_NOTHROW;

class Q_CORE_EXPORT QUrl
{
public:
    enum ParsingMode {
        TolerantMode,
        StrictMode,
        DecodedMode
    };

    // encoding / toString values
    enum UrlFormattingOption {
        None = 0x0,
        RemoveScheme = 0x1,
        RemovePassword = 0x2,
        RemoveUserInfo = RemovePassword | 0x4,
        RemovePort = 0x8,
        RemoveAuthority = RemoveUserInfo | RemovePort | 0x10,
        RemovePath = 0x20,
        RemoveQuery = 0x40,
        RemoveFragment = 0x80,
        // 0x100 was a private code in Qt 4, keep unused for a while
        PreferLocalFile = 0x200,
        StripTrailingSlash = 0x400,
        RemoveFilename = 0x800,
        NormalizePathSegments = 0x1000
    };

    enum ComponentFormattingOption {
        PrettyDecoded = 0x000000,
        EncodeSpaces = 0x100000,
        EncodeUnicode = 0x200000,
        EncodeDelimiters = 0x400000 | 0x800000,
        EncodeReserved = 0x1000000,
        DecodeReserved = 0x2000000,
        // 0x4000000 used to indicate full-decode mode

        FullyEncoded = EncodeSpaces | EncodeUnicode | EncodeDelimiters | EncodeReserved,
        FullyDecoded = FullyEncoded | DecodeReserved | 0x4000000
    };
    Q_DECLARE_FLAGS(ComponentFormattingOptions, ComponentFormattingOption)
#ifdef Q_QDOC
    Q_DECLARE_FLAGS(FormattingOptions, UrlFormattingOption)
#else
    typedef QUrlTwoFlags<UrlFormattingOption, ComponentFormattingOption> FormattingOptions;
#endif

    QUrl();
    QUrl(const QUrl &copy);
    QUrl &operator =(const QUrl &copy);
#ifdef QT_NO_URL_CAST_FROM_STRING
    explicit QUrl(const QString &url, ParsingMode mode = TolerantMode);
#else
    QUrl(const QString &url, ParsingMode mode = TolerantMode);
    QUrl &operator=(const QString &url);
#endif
#ifdef Q_COMPILER_RVALUE_REFS
    QUrl(QUrl &&other) : d(0)
    { qSwap(d, other.d); }
    inline QUrl &operator=(QUrl &&other)
    { qSwap(d, other.d); return *this; }
#endif
    ~QUrl();

    inline void swap(QUrl &other) { qSwap(d, other.d); }

    void setUrl(const QString &url, ParsingMode mode = TolerantMode);
    QString url(FormattingOptions options = FormattingOptions(PrettyDecoded)) const;
    QString toString(FormattingOptions options = FormattingOptions(PrettyDecoded)) const;
    QString toDisplayString(FormattingOptions options = FormattingOptions(PrettyDecoded)) const;
    QUrl adjusted(FormattingOptions options) const;

    QByteArray toEncoded(FormattingOptions options = FullyEncoded) const;
    static QUrl fromEncoded(const QByteArray &url, ParsingMode mode = TolerantMode);

    static QUrl fromUserInput(const QString &userInput);

    bool isValid() const;
    QString errorString() const;

    bool isEmpty() const;
    void clear();

    void setScheme(const QString &scheme);
    QString scheme() const;

    void setAuthority(const QString &authority, ParsingMode mode = TolerantMode);
    QString authority(ComponentFormattingOptions options = PrettyDecoded) const;

    void setUserInfo(const QString &userInfo, ParsingMode mode = TolerantMode);
    QString userInfo(ComponentFormattingOptions options = PrettyDecoded) const;

    void setUserName(const QString &userName, ParsingMode mode = TolerantMode);
    QString userName(ComponentFormattingOptions options = PrettyDecoded) const;

    void setPassword(const QString &password, ParsingMode mode = TolerantMode);
    QString password(ComponentFormattingOptions = PrettyDecoded) const;

    void setHost(const QString &host, ParsingMode mode = TolerantMode);
    QString host(ComponentFormattingOptions = PrettyDecoded) const;
    QString topLevelDomain(ComponentFormattingOptions options = PrettyDecoded) const;

    void setPort(int port);
    int port(int defaultPort = -1) const;

    void setPath(const QString &path, ParsingMode mode = TolerantMode);
    QString path(ComponentFormattingOptions options = PrettyDecoded) const;

    bool hasQuery() const;
    void setQuery(const QString &query, ParsingMode mode = TolerantMode);
    void setQuery(const QUrlQuery &query);
    QString query(ComponentFormattingOptions = PrettyDecoded) const;

    bool hasFragment() const;
    QString fragment(ComponentFormattingOptions options = PrettyDecoded) const;
    void setFragment(const QString &fragment, ParsingMode mode = TolerantMode);

    QUrl resolved(const QUrl &relative) const;

    bool isRelative() const;
    bool isParentOf(const QUrl &url) const;

    bool isLocalFile() const;
    static QUrl fromLocalFile(const QString &localfile);
    QString toLocalFile() const;

    void detach();
    bool isDetached() const;

    bool operator <(const QUrl &url) const;
    bool operator ==(const QUrl &url) const;
    bool operator !=(const QUrl &url) const;

    static QString fromPercentEncoding(const QByteArray &);
    static QByteArray toPercentEncoding(const QString &,
                                        const QByteArray &exclude = QByteArray(),
                                        const QByteArray &include = QByteArray());
#if QT_DEPRECATED_SINCE(5,0)
    QT_DEPRECATED static QString fromPunycode(const QByteArray &punycode)
    { return fromAce(punycode); }
    QT_DEPRECATED static QByteArray toPunycode(const QString &string)
    { return toAce(string); }

    QT_DEPRECATED inline void setQueryItems(const QList<QPair<QString, QString> > &qry);
    QT_DEPRECATED inline void addQueryItem(const QString &key, const QString &value);
    QT_DEPRECATED inline QList<QPair<QString, QString> > queryItems() const;
    QT_DEPRECATED inline bool hasQueryItem(const QString &key) const;
    QT_DEPRECATED inline QString queryItemValue(const QString &key) const;
    QT_DEPRECATED inline QStringList allQueryItemValues(const QString &key) const;
    QT_DEPRECATED inline void removeQueryItem(const QString &key);
    QT_DEPRECATED inline void removeAllQueryItems(const QString &key);

    QT_DEPRECATED inline void setEncodedQueryItems(const QList<QPair<QByteArray, QByteArray> > &query);
    QT_DEPRECATED inline void addEncodedQueryItem(const QByteArray &key, const QByteArray &value);
    QT_DEPRECATED inline QList<QPair<QByteArray, QByteArray> > encodedQueryItems() const;
    QT_DEPRECATED inline bool hasEncodedQueryItem(const QByteArray &key) const;
    QT_DEPRECATED inline QByteArray encodedQueryItemValue(const QByteArray &key) const;
    QT_DEPRECATED inline QList<QByteArray> allEncodedQueryItemValues(const QByteArray &key) const;
    QT_DEPRECATED inline void removeEncodedQueryItem(const QByteArray &key);
    QT_DEPRECATED inline void removeAllEncodedQueryItems(const QByteArray &key);

    QT_DEPRECATED void setEncodedUrl(const QByteArray &u, ParsingMode mode = TolerantMode)
    { setUrl(fromEncodedComponent_helper(u), mode); }

    QT_DEPRECATED QByteArray encodedUserName() const
    { return userName(FullyEncoded).toLatin1(); }
    QT_DEPRECATED void setEncodedUserName(const QByteArray &value)
    { setUserName(fromEncodedComponent_helper(value)); }

    QT_DEPRECATED QByteArray encodedPassword() const
    { return password(FullyEncoded).toLatin1(); }
    QT_DEPRECATED void setEncodedPassword(const QByteArray &value)
    { setPassword(fromEncodedComponent_helper(value)); }

    QT_DEPRECATED QByteArray encodedHost() const
    { return host(FullyEncoded).toLatin1(); }
    QT_DEPRECATED void setEncodedHost(const QByteArray &value)
    { setHost(fromEncodedComponent_helper(value)); }

    QT_DEPRECATED QByteArray encodedPath() const
    { return path(FullyEncoded).toLatin1(); }
    QT_DEPRECATED void setEncodedPath(const QByteArray &value)
    { setPath(fromEncodedComponent_helper(value)); }

    QT_DEPRECATED QByteArray encodedQuery() const
    { return toLatin1_helper(query(FullyEncoded)); }
    QT_DEPRECATED void setEncodedQuery(const QByteArray &value)
    { setQuery(fromEncodedComponent_helper(value)); }

    QT_DEPRECATED QByteArray encodedFragment() const
    { return toLatin1_helper(fragment(FullyEncoded)); }
    QT_DEPRECATED void setEncodedFragment(const QByteArray &value)
    { setFragment(fromEncodedComponent_helper(value)); }

private:
    // helper function for the encodedQuery and encodedFragment functions
    static QByteArray toLatin1_helper(const QString &string)
    {
        if (string.isEmpty())
            return string.isNull() ? QByteArray() : QByteArray("");
        return string.toLatin1();
    }
#endif
private:
    static QString fromEncodedComponent_helper(const QByteArray &ba);

public:
    static QString fromAce(const QByteArray &);
    static QByteArray toAce(const QString &);
    static QStringList idnWhitelist();
    static QStringList toStringList(const QList<QUrl> &uris, FormattingOptions options = FormattingOptions(PrettyDecoded));
    static QList<QUrl> fromStringList(const QStringList &uris, ParsingMode mode = TolerantMode);

    static void setIdnWhitelist(const QStringList &);
    friend Q_CORE_EXPORT uint qHash(const QUrl &url, uint seed) Q_DECL_NOTHROW;

private:
    QUrlPrivate *d;
    friend class QUrlQuery;

public:
    typedef QUrlPrivate * DataPtr;
    inline DataPtr &data_ptr() { return d; }
};

Q_DECLARE_SHARED(QUrl)
Q_DECLARE_OPERATORS_FOR_FLAGS(QUrl::ComponentFormattingOptions)
//Q_DECLARE_OPERATORS_FOR_FLAGS(QUrl::FormattingOptions)

Q_DECL_CONSTEXPR inline QUrl::FormattingOptions operator|(QUrl::UrlFormattingOption f1, QUrl::UrlFormattingOption f2)
{ return QUrl::FormattingOptions(f1) | f2; }
Q_DECL_CONSTEXPR inline QUrl::FormattingOptions operator|(QUrl::UrlFormattingOption f1, QUrl::FormattingOptions f2)
{ return f2 | f1; }
Q_DECL_CONSTEXPR inline QIncompatibleFlag operator|(QUrl::UrlFormattingOption f1, int f2)
{ return QIncompatibleFlag(int(f1) | f2); }

// add operators for OR'ing the two types of flags
inline QUrl::FormattingOptions &operator|=(QUrl::FormattingOptions &i, QUrl::ComponentFormattingOptions f)
{ i |= QUrl::UrlFormattingOption(int(f)); return i; }
Q_DECL_CONSTEXPR inline QUrl::FormattingOptions operator|(QUrl::UrlFormattingOption i, QUrl::ComponentFormattingOption f)
{ return i | QUrl::UrlFormattingOption(int(f)); }
Q_DECL_CONSTEXPR inline QUrl::FormattingOptions operator|(QUrl::UrlFormattingOption i, QUrl::ComponentFormattingOptions f)
{ return i | QUrl::UrlFormattingOption(int(f)); }
Q_DECL_CONSTEXPR inline QUrl::FormattingOptions operator|(QUrl::ComponentFormattingOption f, QUrl::UrlFormattingOption i)
{ return i | QUrl::UrlFormattingOption(int(f)); }
Q_DECL_CONSTEXPR inline QUrl::FormattingOptions operator|(QUrl::ComponentFormattingOptions f, QUrl::UrlFormattingOption i)
{ return i | QUrl::UrlFormattingOption(int(f)); }
Q_DECL_CONSTEXPR inline QUrl::FormattingOptions operator|(QUrl::FormattingOptions i, QUrl::ComponentFormattingOptions f)
{ return i | QUrl::UrlFormattingOption(int(f)); }
Q_DECL_CONSTEXPR inline QUrl::FormattingOptions operator|(QUrl::ComponentFormattingOption f, QUrl::FormattingOptions i)
{ return i | QUrl::UrlFormattingOption(int(f)); }
Q_DECL_CONSTEXPR inline QUrl::FormattingOptions operator|(QUrl::ComponentFormattingOptions f, QUrl::FormattingOptions i)
{ return i | QUrl::UrlFormattingOption(int(f)); }

//inline QUrl::UrlFormattingOption &operator=(const QUrl::UrlFormattingOption &i, QUrl::ComponentFormattingOptions f)
//{ i = int(f); f; }

#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QUrl &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QUrl &);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QUrl &);
#endif

QT_END_NAMESPACE

#if QT_DEPRECATED_SINCE(5,0)
# include <QtCore/qurlquery.h>
#endif

#endif // QURL_H
