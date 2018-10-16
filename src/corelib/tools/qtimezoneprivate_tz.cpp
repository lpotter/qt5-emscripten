/****************************************************************************
**
** Copyright (C) 2013 John Layt <jlayt@kde.org>
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

#include "qtimezone.h"
#include "qtimezoneprivate_p.h"

#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QDataStream>
#include <QtCore/QDateTime>

#include <qdebug.h>

#include "qlocale_tools_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

/*
    Private

    tz file implementation
*/

struct QTzTimeZone {
    QLocale::Country country;
    QByteArray comment;
};

// Define as a type as Q_GLOBAL_STATIC doesn't like it
typedef QHash<QByteArray, QTzTimeZone> QTzTimeZoneHash;

// Parse zone.tab table, assume lists all installed zones, if not will need to read directories
static QTzTimeZoneHash loadTzTimeZones()
{
    QString path = QStringLiteral("/usr/share/zoneinfo/zone.tab");
    if (!QFile::exists(path))
        path = QStringLiteral("/usr/lib/zoneinfo/zone.tab");

    QFile tzif(path);
    if (!tzif.open(QIODevice::ReadOnly))
        return QTzTimeZoneHash();

    QTzTimeZoneHash zonesHash;
    // TODO QTextStream inefficient, replace later
    QTextStream ts(&tzif);
    while (!ts.atEnd()) {
        const QString line = ts.readLine();
        // Comment lines are prefixed with a #
        if (!line.isEmpty() && line.at(0) != '#') {
            // Data rows are tab-separated columns Region, Coordinates, ID, Optional Comments
            const auto parts = line.splitRef(QLatin1Char('\t'));
            QTzTimeZone zone;
            zone.country = QLocalePrivate::codeToCountry(parts.at(0));
            if (parts.size() > 3)
                zone.comment = parts.at(3).toUtf8();
            zonesHash.insert(parts.at(2).toUtf8(), zone);
        }
    }
    return zonesHash;
}

// Hash of available system tz files as loaded by loadTzTimeZones()
Q_GLOBAL_STATIC_WITH_ARGS(const QTzTimeZoneHash, tzZones, (loadTzTimeZones()));

/*
    The following is copied and modified from tzfile.h which is in the public domain.
    Copied as no compatibility guarantee and is never system installed.
    See https://github.com/eggert/tz/blob/master/tzfile.h
*/

#define TZ_MAGIC      "TZif"
#define TZ_MAX_TIMES  1200
#define TZ_MAX_TYPES   256  // Limited by what (unsigned char)'s can hold
#define TZ_MAX_CHARS    50  // Maximum number of abbreviation characters
#define TZ_MAX_LEAPS    50  // Maximum number of leap second corrections

struct QTzHeader {
    char       tzh_magic[4];        // TZ_MAGIC
    char       tzh_version;         // '\0' or '2' as of 2005
    char       tzh_reserved[15];    // reserved--must be zero
    quint32    tzh_ttisgmtcnt;      // number of trans. time flags
    quint32    tzh_ttisstdcnt;      // number of trans. time flags
    quint32    tzh_leapcnt;         // number of leap seconds
    quint32    tzh_timecnt;         // number of transition times
    quint32    tzh_typecnt;         // number of local time types
    quint32    tzh_charcnt;         // number of abbr. chars
};

struct QTzTransition {
    qint64 tz_time;     // Transition time
    quint8 tz_typeind;  // Type Index
};
Q_DECLARE_TYPEINFO(QTzTransition, Q_PRIMITIVE_TYPE);

struct QTzType {
    int tz_gmtoff;  // UTC offset in seconds
    bool   tz_isdst;   // Is DST
    quint8 tz_abbrind; // abbreviation list index
};
Q_DECLARE_TYPEINFO(QTzType, Q_PRIMITIVE_TYPE);


// TZ File parsing

static QTzHeader parseTzHeader(QDataStream &ds, bool *ok)
{
    QTzHeader hdr;
    quint8 ch;
    *ok = false;

    // Parse Magic, 4 bytes
    ds.readRawData(hdr.tzh_magic, 4);

    if (memcmp(hdr.tzh_magic, TZ_MAGIC, 4) != 0 || ds.status() != QDataStream::Ok)
        return hdr;

    // Parse Version, 1 byte, before 2005 was '\0', since 2005 a '2', since 2013 a '3'
    ds >> ch;
    hdr.tzh_version = ch;
    if (ds.status() != QDataStream::Ok
        || (hdr.tzh_version != '2' && hdr.tzh_version != '\0' && hdr.tzh_version != '3')) {
        return hdr;
    }

    // Parse reserved space, 15 bytes
    ds.readRawData(hdr.tzh_reserved, 15);
    if (ds.status() != QDataStream::Ok)
        return hdr;

    // Parse rest of header, 6 x 4-byte transition counts
    ds >> hdr.tzh_ttisgmtcnt >> hdr.tzh_ttisstdcnt >> hdr.tzh_leapcnt >> hdr.tzh_timecnt
       >> hdr.tzh_typecnt >> hdr.tzh_charcnt;

    // Check defined maximums
    if (ds.status() != QDataStream::Ok
        || hdr.tzh_timecnt > TZ_MAX_TIMES
        || hdr.tzh_typecnt > TZ_MAX_TYPES
        || hdr.tzh_charcnt > TZ_MAX_CHARS
        || hdr.tzh_leapcnt > TZ_MAX_LEAPS
        || hdr.tzh_ttisgmtcnt > hdr.tzh_typecnt
        || hdr.tzh_ttisstdcnt > hdr.tzh_typecnt) {
        return hdr;
    }

    *ok = true;
    return hdr;
}

static QVector<QTzTransition> parseTzTransitions(QDataStream &ds, int tzh_timecnt, bool longTran)
{
    QVector<QTzTransition> transitions(tzh_timecnt);

    if (longTran) {
        // Parse tzh_timecnt x 8-byte transition times
        for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
            ds >> transitions[i].tz_time;
            if (ds.status() != QDataStream::Ok)
                transitions.resize(i);
        }
    } else {
        // Parse tzh_timecnt x 4-byte transition times
        qint32 val;
        for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
            ds >> val;
            transitions[i].tz_time = val;
            if (ds.status() != QDataStream::Ok)
                transitions.resize(i);
        }
    }

    // Parse tzh_timecnt x 1-byte transition type index
    for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
        quint8 typeind;
        ds >> typeind;
        if (ds.status() == QDataStream::Ok)
            transitions[i].tz_typeind = typeind;
    }

    return transitions;
}

static QVector<QTzType> parseTzTypes(QDataStream &ds, int tzh_typecnt)
{
    QVector<QTzType> types(tzh_typecnt);

    // Parse tzh_typecnt x transition types
    for (int i = 0; i < tzh_typecnt && ds.status() == QDataStream::Ok; ++i) {
        QTzType &type = types[i];
        // Parse UTC Offset, 4 bytes
        ds >> type.tz_gmtoff;
        // Parse Is DST flag, 1 byte
        if (ds.status() == QDataStream::Ok)
            ds >> type.tz_isdst;
        // Parse Abbreviation Array Index, 1 byte
        if (ds.status() == QDataStream::Ok)
            ds >> type.tz_abbrind;
        if (ds.status() != QDataStream::Ok)
            types.resize(i);
    }

    return types;
}

static QMap<int, QByteArray> parseTzAbbreviations(QDataStream &ds, int tzh_charcnt, const QVector<QTzType> &types)
{
    // Parse the abbreviation list which is tzh_charcnt long with '\0' separated strings. The
    // QTzType.tz_abbrind index points to the first char of the abbreviation in the array, not the
    // occurrence in the list. It can also point to a partial string so we need to use the actual typeList
    // index values when parsing.  By using a map with tz_abbrind as ordered key we get both index
    // methods in one data structure and can convert the types afterwards.
    QMap<int, QByteArray> map;
    quint8 ch;
    QByteArray input;
    // First parse the full abbrev string
    for (int i = 0; i < tzh_charcnt && ds.status() == QDataStream::Ok; ++i) {
        ds >> ch;
        if (ds.status() == QDataStream::Ok)
            input.append(char(ch));
        else
            return map;
    }
    // Then extract all the substrings pointed to by types
    for (const QTzType &type : types) {
        QByteArray abbrev;
        for (int i = type.tz_abbrind; input.at(i) != '\0'; ++i)
            abbrev.append(input.at(i));
        // Have reached end of an abbreviation, so add to map
        map[type.tz_abbrind] = abbrev;
    }
    return map;
}

static void parseTzLeapSeconds(QDataStream &ds, int tzh_leapcnt, bool longTran)
{
    // Parse tzh_leapcnt x pairs of leap seconds
    // We don't use leap seconds, so only read and don't store
    qint32 val;
    if (longTran) {
        // v2 file format, each entry is 12 bytes long
        qint64 time;
        for (int i = 0; i < tzh_leapcnt && ds.status() == QDataStream::Ok; ++i) {
            // Parse Leap Occurrence Time, 8 bytes
            ds >> time;
            // Parse Leap Seconds To Apply, 4 bytes
            if (ds.status() == QDataStream::Ok)
                ds >> val;
        }
    } else {
        // v0 file format, each entry is 8 bytes long
        for (int i = 0; i < tzh_leapcnt && ds.status() == QDataStream::Ok; ++i) {
            // Parse Leap Occurrence Time, 4 bytes
            ds >> val;
            // Parse Leap Seconds To Apply, 4 bytes
            if (ds.status() == QDataStream::Ok)
                ds >> val;
        }
    }
}

static QVector<QTzType> parseTzIndicators(QDataStream &ds, const QVector<QTzType> &types, int tzh_ttisstdcnt, int tzh_ttisgmtcnt)
{
    QVector<QTzType> result = types;
    bool temp;
    /*
      Scan and discard indicators.

      These indicators are only of use (by the date program) when "handling
      POSIX-style time zone environment variables".  The flags here say whether
      the *specification* of the zone gave the time in UTC, local standard time
      or local wall time; but whatever was specified has been digested for us,
      already, by the zone-info compiler (zic), so that the tz_time values read
      from the file (by parseTzTransitions) are all in UTC.
     */

    // Scan tzh_ttisstdcnt x 1-byte standard/wall indicators
    for (int i = 0; i < tzh_ttisstdcnt && ds.status() == QDataStream::Ok; ++i)
        ds >> temp;

    // Scan tzh_ttisgmtcnt x 1-byte UTC/local indicators
    for (int i = 0; i < tzh_ttisgmtcnt && ds.status() == QDataStream::Ok; ++i)
        ds >> temp;

    return result;
}

static QByteArray parseTzPosixRule(QDataStream &ds)
{
    // Parse POSIX rule, variable length '\n' enclosed
    QByteArray rule;

    quint8 ch;
    ds >> ch;
    if (ch != '\n' || ds.status() != QDataStream::Ok)
        return rule;
    ds >> ch;
    while (ch != '\n' && ds.status() == QDataStream::Ok) {
        rule.append((char)ch);
        ds >> ch;
    }

    return rule;
}

static QDate calculateDowDate(int year, int month, int dayOfWeek, int week)
{
    QDate date(year, month, 1);
    int startDow = date.dayOfWeek();
    if (startDow <= dayOfWeek)
        date = date.addDays(dayOfWeek - startDow - 7);
    else
        date = date.addDays(dayOfWeek - startDow);
    date = date.addDays(week * 7);
    while (date.month() != month)
        date = date.addDays(-7);
    return date;
}

static QDate calculatePosixDate(const QByteArray &dateRule, int year)
{
    // Can start with M, J, or a digit
    if (dateRule.at(0) == 'M') {
        // nth week in month format "Mmonth.week.dow"
        QList<QByteArray> dateParts = dateRule.split('.');
        int month = dateParts.at(0).mid(1).toInt();
        int week = dateParts.at(1).toInt();
        int dow = dateParts.at(2).toInt();
        if (dow == 0)
            ++dow;
        return calculateDowDate(year, month, dow, week);
    } else if (dateRule.at(0) == 'J') {
        // Day of Year ignores Feb 29
        int doy = dateRule.mid(1).toInt();
        QDate date = QDate(year, 1, 1).addDays(doy - 1);
        if (QDate::isLeapYear(date.year()))
            date = date.addDays(-1);
        return date;
    } else {
        // Day of Year includes Feb 29
        int doy = dateRule.toInt();
        return QDate(year, 1, 1).addDays(doy - 1);
    }
}

// returns the time in seconds, INT_MIN if we failed to parse
static int parsePosixTime(const char *begin, const char *end)
{
    // Format "hh[:mm[:ss]]"
    int hour, min = 0, sec = 0;

    // Note that the calls to qstrtoll do *not* check the end pointer, which
    // means they proceed until they find a non-digit. We check that we're
    // still in range at the end, but we may have read from past end. It's the
    // caller's responsibility to ensure that begin is part of a
    // null-terminated string.

    bool ok = false;
    hour = qstrtoll(begin, &begin, 10, &ok);
    if (!ok || hour < 0)
        return INT_MIN;
    if (begin < end && *begin == ':') {
        // minutes
        ++begin;
        min = qstrtoll(begin, &begin, 10, &ok);
        if (!ok || min < 0)
            return INT_MIN;

        if (begin < end && *begin == ':') {
            // seconds
            ++begin;
            sec = qstrtoll(begin, &begin, 10, &ok);
            if (!ok || sec < 0)
                return INT_MIN;
        }
    }

    // we must have consumed everything
    if (begin != end)
        return INT_MIN;

    return (hour * 60 + min) * 60 + sec;
}

static QTime parsePosixTransitionTime(const QByteArray &timeRule)
{
    // Format "hh[:mm[:ss]]"
    int value = parsePosixTime(timeRule.constBegin(), timeRule.constEnd());
    if (value == INT_MIN) {
        // if we failed to parse, return 02:00
        return QTime(2, 0, 0);
    }
    return QTime::fromMSecsSinceStartOfDay(value * 1000);
}

static int parsePosixOffset(const char *begin, const char *end)
{
    // Format "[+|-]hh[:mm[:ss]]"
    // note that the sign is inverted because POSIX counts in hours West of GMT
    bool negate = true;
    if (*begin == '+') {
        ++begin;
    } else if (*begin == '-') {
        negate = false;
        ++begin;
    }

    int value = parsePosixTime(begin, end);
    if (value == INT_MIN)
        return value;
    return negate ? -value : value;
}

static inline bool asciiIsLetter(char ch)
{
    ch |= 0x20; // lowercases if it is a letter, otherwise just corrupts ch
    return ch >= 'a' && ch <= 'z';
}

namespace {

struct PosixZone
{
    enum {
        InvalidOffset = INT_MIN,
    };

    QString name;
    int offset;

    static PosixZone invalid() { return {QString(), InvalidOffset}; }
    static PosixZone parse(const char *&pos, const char *end);

    bool hasValidOffset() const Q_DECL_NOTHROW { return offset != InvalidOffset; }
};

} // unnamed namespace

// Returns the zone name, the offset (in seconds) and advances \a begin to
// where the parsing ended. Returns a zone of INT_MIN in case an offset
// couldn't be read.
PosixZone PosixZone::parse(const char *&pos, const char *end)
{
    static const char offsetChars[] = "0123456789:";

    const char *nameBegin = pos;
    const char *nameEnd;
    Q_ASSERT(pos < end);

    if (*pos == '<') {
        nameBegin = pos + 1;    // skip the '<'
        nameEnd = nameBegin;
        while (nameEnd < end && *nameEnd != '>') {
            // POSIX says only alphanumeric, but we allow anything
            ++nameEnd;
        }
        pos = nameEnd + 1;      // skip the '>'
    } else {
        nameBegin = pos;
        nameEnd = nameBegin;
        while (nameEnd < end && asciiIsLetter(*nameEnd))
            ++nameEnd;
        pos = nameEnd;
    }
    if (nameEnd - nameBegin < 3)
        return invalid();  // name must be at least 3 characters long

    // zone offset, form [+-]hh:mm:ss
    const char *zoneBegin = pos;
    const char *zoneEnd = pos;
    if (zoneEnd < end && (zoneEnd[0] == '+' || zoneEnd[0] == '-'))
        ++zoneEnd;
    while (zoneEnd < end) {
        if (strchr(offsetChars, char(*zoneEnd)) == NULL)
            break;
        ++zoneEnd;
    }

    QString name = QString::fromUtf8(nameBegin, nameEnd - nameBegin);
    const int offset = zoneEnd > zoneBegin ? parsePosixOffset(zoneBegin, zoneEnd) : InvalidOffset;
    pos = zoneEnd;
    return {std::move(name), offset};
}

static QVector<QTimeZonePrivate::Data> calculatePosixTransitions(const QByteArray &posixRule,
                                                                 int startYear, int endYear,
                                                                 int lastTranMSecs)
{
    QVector<QTimeZonePrivate::Data> result;

    // Limit year by qint64 max size for msecs
    if (startYear > 292278994)
        startYear = 292278994;
    if (endYear > 292278994)
        endYear = 292278994;

    // POSIX Format is like "TZ=CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00"
    // i.e. "std offset dst [offset],start[/time],end[/time]"
    // See the section about TZ at http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html
    QList<QByteArray> parts = posixRule.split(',');

    PosixZone stdZone, dstZone = PosixZone::invalid();
    {
        const QByteArray &zoneinfo = parts.at(0);
        const char *begin = zoneinfo.constBegin();

        stdZone = PosixZone::parse(begin, zoneinfo.constEnd());
        if (!stdZone.hasValidOffset()) {
            stdZone.offset = 0;     // reset to UTC if we failed to parse
        } else if (begin < zoneinfo.constEnd()) {
            dstZone = PosixZone::parse(begin, zoneinfo.constEnd());
            if (!dstZone.hasValidOffset()) {
                // if the dst offset isn't provided, it is 1 hour ahead of the standard offset
                dstZone.offset = stdZone.offset + (60 * 60);
            }
        }
    }

    // If only the name part then no transitions
    if (parts.count() == 1) {
        QTimeZonePrivate::Data data;
        data.atMSecsSinceEpoch = lastTranMSecs;
        data.offsetFromUtc = stdZone.offset;
        data.standardTimeOffset = stdZone.offset;
        data.daylightTimeOffset = 0;
        data.abbreviation = stdZone.name;
        result << data;
        return result;
    }


    // Get the std to dst transtion details
    QList<QByteArray> dstParts = parts.at(1).split('/');
    QByteArray dstDateRule = dstParts.at(0);
    QTime dstTime;
    if (dstParts.count() > 1)
        dstTime = parsePosixTransitionTime(dstParts.at(1));
    else
        dstTime = QTime(2, 0, 0);

    // Get the dst to std transtion details
    QList<QByteArray> stdParts = parts.at(2).split('/');
    QByteArray stdDateRule = stdParts.at(0);
    QTime stdTime;
    if (stdParts.count() > 1)
        stdTime = parsePosixTransitionTime(stdParts.at(1));
    else
        stdTime = QTime(2, 0, 0);

    for (int year = startYear; year <= endYear; ++year) {
        QTimeZonePrivate::Data dstData;
        QDateTime dst(calculatePosixDate(dstDateRule, year), dstTime, Qt::UTC);
        dstData.atMSecsSinceEpoch = dst.toMSecsSinceEpoch() - (stdZone.offset * 1000);
        dstData.offsetFromUtc = dstZone.offset;
        dstData.standardTimeOffset = stdZone.offset;
        dstData.daylightTimeOffset = dstZone.offset - stdZone.offset;
        dstData.abbreviation = dstZone.name;
        QTimeZonePrivate::Data stdData;
        QDateTime std(calculatePosixDate(stdDateRule, year), stdTime, Qt::UTC);
        stdData.atMSecsSinceEpoch = std.toMSecsSinceEpoch() - (dstZone.offset * 1000);
        stdData.offsetFromUtc = stdZone.offset;
        stdData.standardTimeOffset = stdZone.offset;
        stdData.daylightTimeOffset = 0;
        stdData.abbreviation = stdZone.name;
        // Part of the high year will overflow
        if (year == 292278994 && (dstData.atMSecsSinceEpoch < 0 || stdData.atMSecsSinceEpoch < 0)) {
            if (dstData.atMSecsSinceEpoch > 0) {
                result << dstData;
            } else if (stdData.atMSecsSinceEpoch > 0) {
                result << stdData;
            }
        } else if (dst < std) {
            result << dstData << stdData;
        } else {
            result << stdData << dstData;
        }
    }
    return result;
}

// Create the system default time zone
QTzTimeZonePrivate::QTzTimeZonePrivate()
{
    init(systemTimeZoneId());
}

// Create a named time zone
QTzTimeZonePrivate::QTzTimeZonePrivate(const QByteArray &ianaId)
{
    init(ianaId);
}

QTzTimeZonePrivate::~QTzTimeZonePrivate()
{
}

QTzTimeZonePrivate *QTzTimeZonePrivate::clone() const
{
    return new QTzTimeZonePrivate(*this);
}

void QTzTimeZonePrivate::init(const QByteArray &ianaId)
{
    QFile tzif;
    if (ianaId.isEmpty()) {
        // Open system tz
        tzif.setFileName(QStringLiteral("/etc/localtime"));
        if (!tzif.open(QIODevice::ReadOnly))
            return;
    } else {
        // Open named tz, try modern path first, if fails try legacy path
        tzif.setFileName(QLatin1String("/usr/share/zoneinfo/") + QString::fromLocal8Bit(ianaId));
        if (!tzif.open(QIODevice::ReadOnly)) {
            tzif.setFileName(QLatin1String("/usr/lib/zoneinfo/") + QString::fromLocal8Bit(ianaId));
            if (!tzif.open(QIODevice::ReadOnly))
                return;
        }
    }

    QDataStream ds(&tzif);

    // Parse the old version block of data
    bool ok = false;
    QTzHeader hdr = parseTzHeader(ds, &ok);
    if (!ok || ds.status() != QDataStream::Ok)
        return;
    QVector<QTzTransition> tranList = parseTzTransitions(ds, hdr.tzh_timecnt, false);
    if (ds.status() != QDataStream::Ok)
        return;
    QVector<QTzType> typeList = parseTzTypes(ds, hdr.tzh_typecnt);
    if (ds.status() != QDataStream::Ok)
        return;
    QMap<int, QByteArray> abbrevMap = parseTzAbbreviations(ds, hdr.tzh_charcnt, typeList);
    if (ds.status() != QDataStream::Ok)
        return;
    parseTzLeapSeconds(ds, hdr.tzh_leapcnt, false);
    if (ds.status() != QDataStream::Ok)
        return;
    typeList = parseTzIndicators(ds, typeList, hdr.tzh_ttisstdcnt, hdr.tzh_ttisgmtcnt);
    if (ds.status() != QDataStream::Ok)
        return;

    // If version 2 then parse the second block of data
    if (hdr.tzh_version == '2' || hdr.tzh_version == '3') {
        ok = false;
        QTzHeader hdr2 = parseTzHeader(ds, &ok);
        if (!ok || ds.status() != QDataStream::Ok)
            return;
        tranList = parseTzTransitions(ds, hdr2.tzh_timecnt, true);
        if (ds.status() != QDataStream::Ok)
            return;
        typeList = parseTzTypes(ds, hdr2.tzh_typecnt);
        if (ds.status() != QDataStream::Ok)
            return;
        abbrevMap = parseTzAbbreviations(ds, hdr2.tzh_charcnt, typeList);
        if (ds.status() != QDataStream::Ok)
            return;
        parseTzLeapSeconds(ds, hdr2.tzh_leapcnt, true);
        if (ds.status() != QDataStream::Ok)
            return;
        typeList = parseTzIndicators(ds, typeList, hdr2.tzh_ttisstdcnt, hdr2.tzh_ttisgmtcnt);
        if (ds.status() != QDataStream::Ok)
            return;
        m_posixRule = parseTzPosixRule(ds);
        if (ds.status() != QDataStream::Ok)
            return;
    }

    // Translate the TZ file into internal format

    // Translate the array index based tz_abbrind into list index
    const int size = abbrevMap.size();
    m_abbreviations.clear();
    m_abbreviations.reserve(size);
    QVector<int> abbrindList;
    abbrindList.reserve(size);
    for (auto it = abbrevMap.cbegin(), end = abbrevMap.cend(); it != end; ++it) {
        m_abbreviations.append(it.value());
        abbrindList.append(it.key());
    }
    for (int i = 0; i < typeList.size(); ++i)
        typeList[i].tz_abbrind = abbrindList.indexOf(typeList.at(i).tz_abbrind);

    // Offsets are stored as total offset, want to know separate UTC and DST offsets
    // so find the first non-dst transition to use as base UTC Offset
    int utcOffset = 0;
    for (const QTzTransition &tran : qAsConst(tranList)) {
        if (!typeList.at(tran.tz_typeind).tz_isdst) {
            utcOffset = typeList.at(tran.tz_typeind).tz_gmtoff;
            break;
        }
    }

    // Now for each transition time calculate and store our rule:
    const int tranCount = tranList.count();;
    m_tranTimes.reserve(tranCount);
    // The DST offset when in effect: usually stable, usually an hour:
    int lastDstOff = 3600;
    for (int i = 0; i < tranCount; i++) {
        const QTzTransition &tz_tran = tranList.at(i);
        QTzTransitionTime tran;
        QTzTransitionRule rule;
        const QTzType tz_type = typeList.at(tz_tran.tz_typeind);

        // Calculate the associated Rule
        if (!tz_type.tz_isdst) {
            utcOffset = tz_type.tz_gmtoff;
        } else if (Q_UNLIKELY(tz_type.tz_gmtoff != utcOffset + lastDstOff)) {
            /*
              This might be a genuine change in DST offset, but could also be
              DST starting at the same time as the standard offset changed.  See
              if DST's end gives a more plausible utcOffset (i.e. one closer to
              the last we saw, or a simple whole hour):
            */
            // Standard offset inferred from net offset and expected DST offset:
            const int inferStd = tz_type.tz_gmtoff - lastDstOff; // != utcOffset
            for (int j = i + 1; j < tranCount; j++) {
                const QTzType new_type = typeList.at(tranList.at(j).tz_typeind);
                if (!new_type.tz_isdst) {
                    const int newUtc = new_type.tz_gmtoff;
                    if (newUtc == utcOffset) {
                        // DST-end can't help us, avoid lots of messy checks.
                    // else: See if the end matches the familiar DST offset:
                    } else if (newUtc == inferStd) {
                        utcOffset = newUtc;
                    // else: let either end shift us to one hour as DST offset:
                    } else if (tz_type.tz_gmtoff - 3600 == utcOffset) {
                        // Start does it
                    } else if (tz_type.tz_gmtoff - 3600 == newUtc) {
                        utcOffset = newUtc; // End does it
                    // else: prefer whichever end gives DST offset closer to
                    // last, but consider any offset > 0 "closer" than any <= 0:
                    } else if (newUtc < tz_type.tz_gmtoff
                               ? (utcOffset >= tz_type.tz_gmtoff
                                  || qAbs(newUtc - inferStd) < qAbs(utcOffset - inferStd))
                               : (utcOffset >= tz_type.tz_gmtoff
                                  && qAbs(newUtc - inferStd) < qAbs(utcOffset - inferStd))) {
                        utcOffset = newUtc;
                    }
                    break;
                }
            }
            lastDstOff = tz_type.tz_gmtoff - utcOffset;
        }
        rule.stdOffset = utcOffset;
        rule.dstOffset = tz_type.tz_gmtoff - utcOffset;
        rule.abbreviationIndex = tz_type.tz_abbrind;

        // If the rule already exist then use that, otherwise add it
        int ruleIndex = m_tranRules.indexOf(rule);
        if (ruleIndex == -1) {
            m_tranRules.append(rule);
            tran.ruleIndex = m_tranRules.size() - 1;
        } else {
            tran.ruleIndex = ruleIndex;
        }

        tran.atMSecsSinceEpoch = tz_tran.tz_time * 1000;
        m_tranTimes.append(tran);
    }

    if (ianaId.isEmpty())
        m_id = systemTimeZoneId();
    else
        m_id = ianaId;
}

QLocale::Country QTzTimeZonePrivate::country() const
{
    return tzZones->value(m_id).country;
}

QString QTzTimeZonePrivate::comment() const
{
    return QString::fromUtf8(tzZones->value(m_id).comment);
}

QString QTzTimeZonePrivate::displayName(qint64 atMSecsSinceEpoch,
                                        QTimeZone::NameType nameType,
                                        const QLocale &locale) const
{
#if QT_CONFIG(icu)
    if (!m_icu)
        m_icu = new QIcuTimeZonePrivate(m_id);
    // TODO small risk may not match if tran times differ due to outdated files
    // TODO Some valid TZ names are not valid ICU names, use translation table?
    if (m_icu->isValid())
        return m_icu->displayName(atMSecsSinceEpoch, nameType, locale);
#else
    Q_UNUSED(nameType)
    Q_UNUSED(locale)
#endif
    return abbreviation(atMSecsSinceEpoch);
}

QString QTzTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                        QTimeZone::NameType nameType,
                                        const QLocale &locale) const
{
#if QT_CONFIG(icu)
    if (!m_icu)
        m_icu = new QIcuTimeZonePrivate(m_id);
    // TODO small risk may not match if tran times differ due to outdated files
    // TODO Some valid TZ names are not valid ICU names, use translation table?
    if (m_icu->isValid())
        return m_icu->displayName(timeType, nameType, locale);
#else
    Q_UNUSED(timeType)
    Q_UNUSED(nameType)
    Q_UNUSED(locale)
#endif
    // If no ICU available then have to use abbreviations instead
    // Abbreviations don't have GenericTime
    if (timeType == QTimeZone::GenericTime)
        timeType = QTimeZone::StandardTime;

    // Get current tran, if valid and is what we want, then use it
    const qint64 currentMSecs = QDateTime::currentMSecsSinceEpoch();
    QTimeZonePrivate::Data tran = data(currentMSecs);
    if (tran.atMSecsSinceEpoch != invalidMSecs()
        && ((timeType == QTimeZone::DaylightTime && tran.daylightTimeOffset != 0)
        || (timeType == QTimeZone::StandardTime && tran.daylightTimeOffset == 0))) {
        return tran.abbreviation;
    }

    // Otherwise get next tran and if valid and is what we want, then use it
    tran = nextTransition(currentMSecs);
    if (tran.atMSecsSinceEpoch != invalidMSecs()
        && ((timeType == QTimeZone::DaylightTime && tran.daylightTimeOffset != 0)
        || (timeType == QTimeZone::StandardTime && tran.daylightTimeOffset == 0))) {
        return tran.abbreviation;
    }

    // Otherwise get prev tran and if valid and is what we want, then use it
    tran = previousTransition(currentMSecs);
    if (tran.atMSecsSinceEpoch != invalidMSecs())
        tran = previousTransition(tran.atMSecsSinceEpoch);
    if (tran.atMSecsSinceEpoch != invalidMSecs()
        && ((timeType == QTimeZone::DaylightTime && tran.daylightTimeOffset != 0)
        || (timeType == QTimeZone::StandardTime && tran.daylightTimeOffset == 0))) {
        return tran.abbreviation;
    }

    // Otherwise is strange sequence, so work backwards through trans looking for first match, if any
    auto it = std::partition_point(m_tranTimes.cbegin(), m_tranTimes.cend(),
                                   [currentMSecs](const QTzTransitionTime &at) {
                                       return at.atMSecsSinceEpoch <= currentMSecs;
                                   });

    while (it != m_tranTimes.cbegin()) {
        --it;
        tran = dataForTzTransition(*it);
        int offset = tran.daylightTimeOffset;
        if ((timeType == QTimeZone::DaylightTime) != (offset == 0))
            return tran.abbreviation;
    }

    // Otherwise if no match use current data
    return data(currentMSecs).abbreviation;
}

QString QTzTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).abbreviation;
}

int QTzTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    const QTimeZonePrivate::Data tran = data(atMSecsSinceEpoch);
    return tran.offsetFromUtc; // == tran.standardTimeOffset + tran.daylightTimeOffset
}

int QTzTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).standardTimeOffset;
}

int QTzTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).daylightTimeOffset;
}

bool QTzTimeZonePrivate::hasDaylightTime() const
{
    // TODO Perhaps cache as frequently accessed?
    for (const QTzTransitionRule &rule : m_tranRules) {
        if (rule.dstOffset != 0)
            return true;
    }
    return false;
}

bool QTzTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    return (daylightTimeOffset(atMSecsSinceEpoch) != 0);
}

QTimeZonePrivate::Data QTzTimeZonePrivate::dataForTzTransition(QTzTransitionTime tran) const
{
    QTimeZonePrivate::Data data;
    data.atMSecsSinceEpoch = tran.atMSecsSinceEpoch;
    QTzTransitionRule rule = m_tranRules.at(tran.ruleIndex);
    data.standardTimeOffset = rule.stdOffset;
    data.daylightTimeOffset = rule.dstOffset;
    data.offsetFromUtc = rule.stdOffset + rule.dstOffset;
    data.abbreviation = QString::fromUtf8(m_abbreviations.at(rule.abbreviationIndex));
    return data;
}

QTimeZonePrivate::Data QTzTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    // If we have no rules (so probably an invalid tz), return invalid data:
    if (!m_tranTimes.size())
        return invalidData();

    // If the required time is after the last transition and we have a POSIX rule then use it
    if (m_tranTimes.last().atMSecsSinceEpoch < forMSecsSinceEpoch
        && !m_posixRule.isEmpty() && forMSecsSinceEpoch >= 0) {
        const int year = QDateTime::fromMSecsSinceEpoch(forMSecsSinceEpoch, Qt::UTC).date().year();
        QVector<QTimeZonePrivate::Data> posixTrans =
            calculatePosixTransitions(m_posixRule, year - 1, year + 1,
                                      m_tranTimes.last().atMSecsSinceEpoch);
        auto it = std::partition_point(posixTrans.cbegin(), posixTrans.cend(),
                                       [forMSecsSinceEpoch] (const QTimeZonePrivate::Data &at) {
                                           return at.atMSecsSinceEpoch <= forMSecsSinceEpoch;
                                       });
        if (it > posixTrans.cbegin()) {
            QTimeZonePrivate::Data data = *--it;
            data.atMSecsSinceEpoch = forMSecsSinceEpoch;
            return data;
        }
    }

    // Otherwise, if we can find a valid tran, then use its rule:
    auto last = std::partition_point(m_tranTimes.cbegin(), m_tranTimes.cend(),
                                     [forMSecsSinceEpoch] (const QTzTransitionTime &at) {
                                         return at.atMSecsSinceEpoch <= forMSecsSinceEpoch;
                                     });
    if (last > m_tranTimes.cbegin())
        --last;
    Data data = dataForTzTransition(*last);
    data.atMSecsSinceEpoch = forMSecsSinceEpoch;
    return data;
}

bool QTzTimeZonePrivate::hasTransitions() const
{
    return true;
}

QTimeZonePrivate::Data QTzTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    // If the required time is after the last transition and we have a POSIX rule then use it
    if (m_tranTimes.size() > 0 && m_tranTimes.last().atMSecsSinceEpoch < afterMSecsSinceEpoch
        && !m_posixRule.isEmpty() && afterMSecsSinceEpoch >= 0) {
        const int year = QDateTime::fromMSecsSinceEpoch(afterMSecsSinceEpoch, Qt::UTC).date().year();
        QVector<QTimeZonePrivate::Data> posixTrans =
            calculatePosixTransitions(m_posixRule, year - 1, year + 1,
                                      m_tranTimes.last().atMSecsSinceEpoch);
        auto it = std::partition_point(posixTrans.cbegin(), posixTrans.cend(),
                                       [afterMSecsSinceEpoch] (const QTimeZonePrivate::Data &at) {
                                           return at.atMSecsSinceEpoch <= afterMSecsSinceEpoch;
                                       });

        return it == posixTrans.cend() ? invalidData() : *it;
    }

    // Otherwise, if we can find a valid tran, use its rule:
    auto last = std::partition_point(m_tranTimes.cbegin(), m_tranTimes.cend(),
                                     [afterMSecsSinceEpoch] (const QTzTransitionTime &at) {
                                         return at.atMSecsSinceEpoch <= afterMSecsSinceEpoch;
                                     });
    return last != m_tranTimes.cend() ? dataForTzTransition(*last) : invalidData();
}

QTimeZonePrivate::Data QTzTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    // If the required time is after the last transition and we have a POSIX rule then use it
    if (m_tranTimes.size() > 0 && m_tranTimes.last().atMSecsSinceEpoch < beforeMSecsSinceEpoch
        && !m_posixRule.isEmpty() && beforeMSecsSinceEpoch > 0) {
        const int year = QDateTime::fromMSecsSinceEpoch(beforeMSecsSinceEpoch, Qt::UTC).date().year();
        QVector<QTimeZonePrivate::Data> posixTrans =
            calculatePosixTransitions(m_posixRule, year - 1, year + 1,
                                      m_tranTimes.last().atMSecsSinceEpoch);
        auto it = std::partition_point(posixTrans.cbegin(), posixTrans.cend(),
                                       [beforeMSecsSinceEpoch] (const QTimeZonePrivate::Data &at) {
                                           return at.atMSecsSinceEpoch < beforeMSecsSinceEpoch;
                                       });
        Q_ASSERT(it > posixTrans.cbegin());
        return *--it;
    }

    // Otherwise if we can find a valid tran then use its rule
    auto last = std::partition_point(m_tranTimes.cbegin(), m_tranTimes.cend(),
                                     [beforeMSecsSinceEpoch] (const QTzTransitionTime &at) {
                                         return at.atMSecsSinceEpoch < beforeMSecsSinceEpoch;
                                     });
    return last > m_tranTimes.cbegin() ? dataForTzTransition(*--last) : invalidData();
}

// TODO Could cache the value and monitor the required files for any changes
QByteArray QTzTimeZonePrivate::systemTimeZoneId() const
{
    // Check TZ env var first, if not populated try find it
    QByteArray ianaId = qgetenv("TZ");
    if (!ianaId.isEmpty() && ianaId.at(0) == ':')
        ianaId = ianaId.mid(1);

    // The TZ value can be ":/etc/localtime" which libc considers
    // to be a "default timezone", in which case it will be read
    // by one of the blocks below, so unset it here so it is not
    // considered as a valid/found ianaId
    if (ianaId == "/etc/localtime")
        ianaId.clear();

    // On most distros /etc/localtime is a symlink to a real file so extract name from the path
    if (ianaId.isEmpty()) {
        const QString path = QFile::symLinkTarget(QStringLiteral("/etc/localtime"));
        if (!path.isEmpty()) {
            // /etc/localtime is a symlink to the current TZ file, so extract from path
            int index = path.indexOf(QLatin1String("/zoneinfo/"));
            if (index != -1)
                ianaId = path.mid(index + 10).toUtf8();
        }
    }

    // On Debian Etch up to Jessie, /etc/localtime is a regular file while the actual name is in /etc/timezone
    if (ianaId.isEmpty()) {
        QFile tzif(QStringLiteral("/etc/timezone"));
        if (tzif.open(QIODevice::ReadOnly)) {
            // TODO QTextStream inefficient, replace later
            QTextStream ts(&tzif);
            if (!ts.atEnd())
                ianaId = ts.readLine().toUtf8();
        }
    }

    // On some Red Hat distros /etc/localtime is real file with name held in /etc/sysconfig/clock
    // in a line like ZONE="Europe/Oslo" or TIMEZONE="Europe/Oslo"
    if (ianaId.isEmpty()) {
        QFile tzif(QStringLiteral("/etc/sysconfig/clock"));
        if (tzif.open(QIODevice::ReadOnly)) {
            // TODO QTextStream inefficient, replace later
            QTextStream ts(&tzif);
            QString line;
            while (ianaId.isEmpty() && !ts.atEnd() && ts.status() == QTextStream::Ok) {
                line = ts.readLine();
                if (line.startsWith(QLatin1String("ZONE="))) {
                    ianaId = line.mid(6, line.size() - 7).toUtf8();
                } else if (line.startsWith(QLatin1String("TIMEZONE="))) {
                    ianaId = line.mid(10, line.size() - 11).toUtf8();
                }
            }
        }
    }

    // Give up for now and return UTC
    if (ianaId.isEmpty())
        ianaId = utcQByteArray();

    return ianaId;
}

bool QTzTimeZonePrivate::isTimeZoneIdAvailable(const QByteArray &ianaId) const
{
    return tzZones->contains(ianaId);
}

QList<QByteArray> QTzTimeZonePrivate::availableTimeZoneIds() const
{
    QList<QByteArray> result = tzZones->keys();
    std::sort(result.begin(), result.end());
    return result;
}

QList<QByteArray> QTzTimeZonePrivate::availableTimeZoneIds(QLocale::Country country) const
{
    // TODO AnyCountry
    QList<QByteArray> result;
    for (auto it = tzZones->cbegin(), end = tzZones->cend(); it != end; ++it) {
        if (it.value().country == country)
            result << it.key();
    }
    std::sort(result.begin(), result.end());
    return result;
}

QT_END_NAMESPACE
