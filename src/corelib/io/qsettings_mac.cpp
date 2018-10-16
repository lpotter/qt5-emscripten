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

#include "qsettings.h"
#ifndef QT_NO_SETTINGS

#include "qsettings_p.h"
#include "qdatetime.h"
#include "qdir.h"
#include "qvarlengtharray.h"
#include "private/qcore_mac_p.h"
#ifndef QT_NO_QOBJECT
#include "qcoreapplication.h"
#endif // QT_NO_QOBJECT

QT_BEGIN_NAMESPACE

static const CFStringRef hostNames[2] = { kCFPreferencesCurrentHost, kCFPreferencesAnyHost };
static const int numHostNames = 2;

/*
    On the Mac, it is more natural to use '.' as the key separator
    than '/'. Therefore, it makes sense to replace '/' with '.' in
    keys. Then we replace '.' with middle dots (which we can't show
    here) and middle dots with '/'. A key like "4.0/BrowserCommand"
    becomes "4<middot>0.BrowserCommand".
*/

enum RotateShift { Macify = 1, Qtify = 2 };

static QString rotateSlashesDotsAndMiddots(const QString &key, int shift)
{
    static const int NumKnights = 3;
    static const char knightsOfTheRoundTable[NumKnights] = { '/', '.', '\xb7' };
    QString result = key;

    for (int i = 0; i < result.size(); ++i) {
        for (int j = 0; j < NumKnights; ++j) {
            if (result.at(i) == QLatin1Char(knightsOfTheRoundTable[j])) {
                result[i] = QLatin1Char(knightsOfTheRoundTable[(j + shift) % NumKnights]).unicode();
                break;
            }
        }
    }
    return result;
}

static QCFType<CFStringRef> macKey(const QString &key)
{
    return rotateSlashesDotsAndMiddots(key, Macify).toCFString();
}

static QString qtKey(CFStringRef cfkey)
{
    return rotateSlashesDotsAndMiddots(QString::fromCFString(cfkey), Qtify);
}

static QCFType<CFPropertyListRef> macValue(const QVariant &value);

static CFArrayRef macList(const QList<QVariant> &list)
{
    int n = list.size();
    QVarLengthArray<QCFType<CFPropertyListRef> > cfvalues(n);
    for (int i = 0; i < n; ++i)
        cfvalues[i] = macValue(list.at(i));
    return CFArrayCreate(kCFAllocatorDefault, reinterpret_cast<const void **>(cfvalues.data()),
                         CFIndex(n), &kCFTypeArrayCallBacks);
}

static QCFType<CFPropertyListRef> macValue(const QVariant &value)
{
    CFPropertyListRef result = 0;

    switch (value.type()) {
    case QVariant::ByteArray:
        {
            QByteArray ba = value.toByteArray();
            result = CFDataCreate(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(ba.data()),
                                  CFIndex(ba.size()));
        }
        break;
    // should be same as below (look for LIST)
    case QVariant::List:
    case QVariant::StringList:
    case QVariant::Polygon:
        result = macList(value.toList());
        break;
    case QVariant::Map:
        {
            /*
                QMap<QString, QVariant> is potentially a multimap,
                whereas CFDictionary is a single-valued map. To allow
                for multiple values with the same key, we store
                multiple values in a CFArray. To avoid ambiguities,
                we also wrap lists in a CFArray singleton.
            */
            QMap<QString, QVariant> map = value.toMap();
            QMap<QString, QVariant>::const_iterator i = map.constBegin();

            int maxUniqueKeys = map.size();
            int numUniqueKeys = 0;
            QVarLengthArray<QCFType<CFPropertyListRef> > cfkeys(maxUniqueKeys);
            QVarLengthArray<QCFType<CFPropertyListRef> > cfvalues(maxUniqueKeys);

            while (i != map.constEnd()) {
                const QString &key = i.key();
                QList<QVariant> values;

                do {
                    values << i.value();
                    ++i;
                } while (i != map.constEnd() && i.key() == key);

                bool singleton = (values.count() == 1);
                if (singleton) {
                    switch (values.constFirst().type()) {
                    // should be same as above (look for LIST)
                    case QVariant::List:
                    case QVariant::StringList:
                    case QVariant::Polygon:
                        singleton = false;
                    default:
                        ;
                    }
                }

                cfkeys[numUniqueKeys] = key.toCFString();
                cfvalues[numUniqueKeys] = singleton ? macValue(values.constFirst()) : macList(values);
                ++numUniqueKeys;
            }

            result = CFDictionaryCreate(kCFAllocatorDefault,
                                        reinterpret_cast<const void **>(cfkeys.data()),
                                        reinterpret_cast<const void **>(cfvalues.data()),
                                        CFIndex(numUniqueKeys),
                                        &kCFTypeDictionaryKeyCallBacks,
                                        &kCFTypeDictionaryValueCallBacks);
        }
        break;
    case QVariant::DateTime:
        {
            QDateTime dateTime = value.toDateTime();
            // CFDate, unlike QDateTime, doesn't store timezone information
            if (dateTime.timeSpec() == Qt::LocalTime)
                result = dateTime.toCFDate();
            else
                goto string_case;
        }
        break;
    case QVariant::Bool:
        result = value.toBool() ? kCFBooleanTrue : kCFBooleanFalse;
        break;
    case QVariant::Int:
    case QVariant::UInt:
        {
            int n = value.toInt();
            result = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &n);
        }
        break;
    case QVariant::Double:
        {
            double n = value.toDouble();
            result = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &n);
        }
        break;
    case QVariant::LongLong:
    case QVariant::ULongLong:
        {
            qint64 n = value.toLongLong();
            result = CFNumberCreate(0, kCFNumberLongLongType, &n);
        }
        break;
    case QVariant::String:
    string_case:
    default:
        QString string = QSettingsPrivate::variantToString(value);
        if (string.contains(QChar::Null))
            result = std::move(string).toUtf8().toCFData();
        else
            result = string.toCFString();
    }
    return result;
}

static QVariant qtValue(CFPropertyListRef cfvalue)
{
    if (!cfvalue)
        return QVariant();

    CFTypeID typeId = CFGetTypeID(cfvalue);

    /*
        Sorted grossly from most to least frequent type.
    */
    if (typeId == CFStringGetTypeID()) {
        return QSettingsPrivate::stringToVariant(QString::fromCFString(static_cast<CFStringRef>(cfvalue)));
    } else if (typeId == CFNumberGetTypeID()) {
        CFNumberRef cfnumber = static_cast<CFNumberRef>(cfvalue);
        if (CFNumberIsFloatType(cfnumber)) {
            double d;
            CFNumberGetValue(cfnumber, kCFNumberDoubleType, &d);
            return d;
        } else {
            int i;
            qint64 ll;

            if (CFNumberGetType(cfnumber) == kCFNumberIntType) {
                CFNumberGetValue(cfnumber, kCFNumberIntType, &i);
                return i;
            }
            CFNumberGetValue(cfnumber, kCFNumberLongLongType, &ll);
            return ll;
        }
    } else if (typeId == CFArrayGetTypeID()) {
        CFArrayRef cfarray = static_cast<CFArrayRef>(cfvalue);
        QList<QVariant> list;
        CFIndex size = CFArrayGetCount(cfarray);
        bool metNonString = false;
        for (CFIndex i = 0; i < size; ++i) {
            QVariant value = qtValue(CFArrayGetValueAtIndex(cfarray, i));
            if (value.type() != QVariant::String)
                metNonString = true;
            list << value;
        }
        if (metNonString)
            return list;
        else
            return QVariant(list).toStringList();
    } else if (typeId == CFBooleanGetTypeID()) {
        return (bool)CFBooleanGetValue(static_cast<CFBooleanRef>(cfvalue));
    } else if (typeId == CFDataGetTypeID()) {
        QByteArray byteArray = QByteArray::fromRawCFData(static_cast<CFDataRef>(cfvalue));

        // Fast-path for QByteArray, so that we don't have to go
        // though the expensive and lossy conversion via UTF-8.
        if (!byteArray.startsWith('@')) {
            byteArray.detach();
            return byteArray;
        }

        const QString str = QString::fromUtf8(byteArray.constData(), byteArray.size());
        return QSettingsPrivate::stringToVariant(str);
    } else if (typeId == CFDictionaryGetTypeID()) {
        CFDictionaryRef cfdict = static_cast<CFDictionaryRef>(cfvalue);
        CFTypeID arrayTypeId = CFArrayGetTypeID();
        int size = (int)CFDictionaryGetCount(cfdict);
        QVarLengthArray<CFPropertyListRef> keys(size);
        QVarLengthArray<CFPropertyListRef> values(size);
        CFDictionaryGetKeysAndValues(cfdict, keys.data(), values.data());

        QMultiMap<QString, QVariant> map;
        for (int i = 0; i < size; ++i) {
            QString key = QString::fromCFString(static_cast<CFStringRef>(keys[i]));

            if (CFGetTypeID(values[i]) == arrayTypeId) {
                CFArrayRef cfarray = static_cast<CFArrayRef>(values[i]);
                CFIndex arraySize = CFArrayGetCount(cfarray);
                for (CFIndex j = arraySize - 1; j >= 0; --j)
                    map.insert(key, qtValue(CFArrayGetValueAtIndex(cfarray, j)));
            } else {
                map.insert(key, qtValue(values[i]));
            }
        }
        return map;
    } else if (typeId == CFDateGetTypeID()) {
        return QDateTime::fromCFDate(static_cast<CFDateRef>(cfvalue));
    }
    return QVariant();
}

static QString comify(const QString &organization)
{
    for (int i = organization.size() - 1; i >= 0; --i) {
        QChar ch = organization.at(i);
        if (ch == QLatin1Char('.') || ch == QChar(0x3002) || ch == QChar(0xff0e)
                || ch == QChar(0xff61)) {
            QString suffix = organization.mid(i + 1).toLower();
            if (suffix.size() == 2 || suffix == QLatin1String("com")
                    || suffix == QLatin1String("org") || suffix == QLatin1String("net")
                    || suffix == QLatin1String("edu") || suffix == QLatin1String("gov")
                    || suffix == QLatin1String("mil") || suffix == QLatin1String("biz")
                    || suffix == QLatin1String("info") || suffix == QLatin1String("name")
                    || suffix == QLatin1String("pro") || suffix == QLatin1String("aero")
                    || suffix == QLatin1String("coop") || suffix == QLatin1String("museum")) {
                QString result = organization;
                result.replace(QLatin1Char('/'), QLatin1Char(' '));
                return result;
            }
            break;
        }
        int uc = ch.unicode();
        if ((uc < 'a' || uc > 'z') && (uc < 'A' || uc > 'Z'))
            break;
    }

    QString domain;
    for (int i = 0; i < organization.size(); ++i) {
        QChar ch = organization.at(i);
        int uc = ch.unicode();
        if ((uc >= 'a' && uc <= 'z') || (uc >= '0' && uc <= '9')) {
            domain += ch;
        } else if (uc >= 'A' && uc <= 'Z') {
            domain += ch.toLower();
        } else {
           domain += QLatin1Char(' ');
        }
    }
    domain = domain.simplified();
    domain.replace(QLatin1Char(' '), QLatin1Char('-'));
    if (!domain.isEmpty())
        domain.append(QLatin1String(".com"));
    return domain;
}

class QMacSettingsPrivate : public QSettingsPrivate
{
public:
    QMacSettingsPrivate(QSettings::Scope scope, const QString &organization,
                        const QString &application);
    ~QMacSettingsPrivate();

    void remove(const QString &key) override;
    void set(const QString &key, const QVariant &value) override;
    bool get(const QString &key, QVariant *value) const override;
    QStringList children(const QString &prefix, ChildSpec spec) const override;
    void clear() override;
    void sync() override;
    void flush() override;
    bool isWritable() const override;
    QString fileName() const override;

private:
    struct SearchDomain
    {
        CFStringRef userName;
        CFStringRef applicationOrSuiteId;
    };

    QCFString applicationId;
    QCFString suiteId;
    QCFString hostName;
    SearchDomain domains[6];
    int numDomains;
};

QMacSettingsPrivate::QMacSettingsPrivate(QSettings::Scope scope, const QString &organization,
                                         const QString &application)
    : QSettingsPrivate(QSettings::NativeFormat, scope, organization, application)
{
    QString javaPackageName;
    int curPos = 0;
    int nextDot;

    // attempt to use the organization parameter
    QString domainName = comify(organization);
    // if not found, attempt to use the bundle identifier.
    if (domainName.isEmpty()) {
        CFBundleRef main_bundle = CFBundleGetMainBundle();
        if (main_bundle != NULL) {
            CFStringRef main_bundle_identifier = CFBundleGetIdentifier(main_bundle);
            if (main_bundle_identifier != NULL) {
                QString bundle_identifier(qtKey(main_bundle_identifier));
                // CFBundleGetIdentifier returns identifier separated by slashes rather than periods.
                QStringList bundle_identifier_components = bundle_identifier.split(QLatin1Char('/'));
                // pre-reverse them so that when they get reversed again below, they are in the com.company.product format.
                QStringList bundle_identifier_components_reversed;
                for (int i=0; i<bundle_identifier_components.size(); ++i) {
                    const QString &bundle_identifier_component = bundle_identifier_components.at(i);
                    bundle_identifier_components_reversed.push_front(bundle_identifier_component);
                }
                domainName = bundle_identifier_components_reversed.join(QLatin1Char('.'));
            }
        }
    }
    // if no bundle identifier yet. use a hard coded string.
    if (domainName.isEmpty()) {
        domainName = QLatin1String("unknown-organization.trolltech.com");
    }

    while ((nextDot = domainName.indexOf(QLatin1Char('.'), curPos)) != -1) {
        javaPackageName.prepend(domainName.midRef(curPos, nextDot - curPos));
        javaPackageName.prepend(QLatin1Char('.'));
        curPos = nextDot + 1;
    }
    javaPackageName.prepend(domainName.midRef(curPos));
    javaPackageName = std::move(javaPackageName).toLower();
    if (curPos == 0)
        javaPackageName.prepend(QLatin1String("com."));
    suiteId = javaPackageName;

    if (!application.isEmpty()) {
        javaPackageName += QLatin1Char('.') + application;
        applicationId = javaPackageName;
    }

    numDomains = 0;
    for (int i = (scope == QSettings::SystemScope) ? 1 : 0; i < 2; ++i) {
        for (int j = (application.isEmpty()) ? 1 : 0; j < 3; ++j) {
            SearchDomain &domain = domains[numDomains++];
            domain.userName = (i == 0) ? kCFPreferencesCurrentUser : kCFPreferencesAnyUser;
            if (j == 0)
                domain.applicationOrSuiteId = applicationId;
            else if (j == 1)
                domain.applicationOrSuiteId = suiteId;
            else
                domain.applicationOrSuiteId = kCFPreferencesAnyApplication;
        }
    }

    hostName = (scope == QSettings::SystemScope) ? kCFPreferencesCurrentHost : kCFPreferencesAnyHost;
    sync();
}

QMacSettingsPrivate::~QMacSettingsPrivate()
{
}

void QMacSettingsPrivate::remove(const QString &key)
{
    QStringList keys = children(key + QLatin1Char('/'), AllKeys);

    // If i == -1, then delete "key" itself.
    for (int i = -1; i < keys.size(); ++i) {
        QString subKey = key;
        if (i >= 0) {
            subKey += QLatin1Char('/');
            subKey += keys.at(i);
        }
        CFPreferencesSetValue(macKey(subKey), 0, domains[0].applicationOrSuiteId,
                              domains[0].userName, hostName);
    }
}

void QMacSettingsPrivate::set(const QString &key, const QVariant &value)
{
    CFPreferencesSetValue(macKey(key), macValue(value), domains[0].applicationOrSuiteId,
                          domains[0].userName, hostName);
}

bool QMacSettingsPrivate::get(const QString &key, QVariant *value) const
{
    QCFString k = macKey(key);
    for (int i = 0; i < numDomains; ++i) {
        for (int j = 0; j < numHostNames; ++j) {
            QCFType<CFPropertyListRef> ret =
                    CFPreferencesCopyValue(k, domains[i].applicationOrSuiteId, domains[i].userName,
                                           hostNames[j]);
            if (ret) {
                if (value)
                    *value = qtValue(ret);
                return true;
            }
        }

        if (!fallbacks)
            break;
    }
    return false;
}

QStringList QMacSettingsPrivate::children(const QString &prefix, ChildSpec spec) const
{
    QStringList result;
    int startPos = prefix.size();

    for (int i = 0; i < numDomains; ++i) {
        for (int j = 0; j < numHostNames; ++j) {
            QCFType<CFArrayRef> cfarray = CFPreferencesCopyKeyList(domains[i].applicationOrSuiteId,
                                                                   domains[i].userName,
                                                                   hostNames[j]);
            if (cfarray) {
                CFIndex size = CFArrayGetCount(cfarray);
                for (CFIndex k = 0; k < size; ++k) {
                    QString currentKey =
                            qtKey(static_cast<CFStringRef>(CFArrayGetValueAtIndex(cfarray, k)));
                    if (currentKey.startsWith(prefix))
                        processChild(currentKey.midRef(startPos), spec, result);
                }
            }
        }

        if (!fallbacks)
            break;
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()),
                 result.end());
    return result;
}

void QMacSettingsPrivate::clear()
{
    QCFType<CFArrayRef> cfarray = CFPreferencesCopyKeyList(domains[0].applicationOrSuiteId,
                                                           domains[0].userName, hostName);
    CFPreferencesSetMultiple(0, cfarray, domains[0].applicationOrSuiteId, domains[0].userName,
                             hostName);
}

void QMacSettingsPrivate::sync()
{
    for (int i = 0; i < numDomains; ++i) {
        for (int j = 0; j < numHostNames; ++j) {
            Boolean ok = CFPreferencesSynchronize(domains[i].applicationOrSuiteId,
                                                  domains[i].userName, hostNames[j]);
            // only report failures for the primary file (the one we write to)
            if (!ok && i == 0 && hostNames[j] == hostName && status == QSettings::NoError) {
                setStatus(QSettings::AccessError);
            }
        }
    }
}

void QMacSettingsPrivate::flush()
{
    sync();
}

bool QMacSettingsPrivate::isWritable() const
{
    QMacSettingsPrivate *that = const_cast<QMacSettingsPrivate *>(this);
    QString impossibleKey(QLatin1String("qt_internal/"));

    QSettings::Status oldStatus = that->status;
    that->status = QSettings::NoError;

    that->set(impossibleKey, QVariant());
    that->sync();
    bool writable = (status == QSettings::NoError) && that->get(impossibleKey, 0);
    that->remove(impossibleKey);
    that->sync();

    that->status = oldStatus;
    return writable;
}

QString QMacSettingsPrivate::fileName() const
{
    QString result;
    if (scope == QSettings::UserScope)
        result = QDir::homePath();
    result += QLatin1String("/Library/Preferences/");
    result += QString::fromCFString(domains[0].applicationOrSuiteId);
    result += QLatin1String(".plist");
    return result;
}

QSettingsPrivate *QSettingsPrivate::create(QSettings::Format format,
                                           QSettings::Scope scope,
                                           const QString &organization,
                                           const QString &application)
{
#ifndef QT_BOOTSTRAPPED
    if (organization == QLatin1String("Qt"))
    {
        QString organizationDomain = QCoreApplication::organizationDomain();
        QString applicationName = QCoreApplication::applicationName();

        QSettingsPrivate *newSettings;
        if (format == QSettings::NativeFormat) {
            newSettings = new QMacSettingsPrivate(scope, organizationDomain, applicationName);
        } else {
            newSettings = new QConfFileSettingsPrivate(format, scope, organizationDomain, applicationName);
        }

        newSettings->beginGroupOrArray(QSettingsGroup(normalizedKey(organization)));
        if (!application.isEmpty())
            newSettings->beginGroupOrArray(QSettingsGroup(normalizedKey(application)));

        return newSettings;
    }
#endif
    if (format == QSettings::NativeFormat) {
        return new QMacSettingsPrivate(scope, organization, application);
    } else {
        return new QConfFileSettingsPrivate(format, scope, organization, application);
    }
}

bool QConfFileSettingsPrivate::readPlistFile(const QByteArray &data, ParsedSettingsMap *map) const
{
    QCFType<CFDataRef> cfData = data.toRawCFData();
    QCFType<CFPropertyListRef> propertyList =
            CFPropertyListCreateWithData(kCFAllocatorDefault, cfData, kCFPropertyListImmutable, nullptr, nullptr);

    if (!propertyList)
        return true;
    if (CFGetTypeID(propertyList) != CFDictionaryGetTypeID())
        return false;

    CFDictionaryRef cfdict =
            static_cast<CFDictionaryRef>(static_cast<CFPropertyListRef>(propertyList));
    int size = (int)CFDictionaryGetCount(cfdict);
    QVarLengthArray<CFPropertyListRef> keys(size);
    QVarLengthArray<CFPropertyListRef> values(size);
    CFDictionaryGetKeysAndValues(cfdict, keys.data(), values.data());

    for (int i = 0; i < size; ++i) {
        QString key = qtKey(static_cast<CFStringRef>(keys[i]));
        map->insert(QSettingsKey(key, Qt::CaseSensitive), qtValue(values[i]));
    }
    return true;
}

bool QConfFileSettingsPrivate::writePlistFile(QIODevice &file, const ParsedSettingsMap &map) const
{
    QVarLengthArray<QCFType<CFStringRef> > cfkeys(map.size());
    QVarLengthArray<QCFType<CFPropertyListRef> > cfvalues(map.size());
    int i = 0;
    ParsedSettingsMap::const_iterator j;
    for (j = map.constBegin(); j != map.constEnd(); ++j) {
        cfkeys[i] = macKey(j.key());
        cfvalues[i] = macValue(j.value());
        ++i;
    }

    QCFType<CFDictionaryRef> propertyList =
            CFDictionaryCreate(kCFAllocatorDefault,
                               reinterpret_cast<const void **>(cfkeys.data()),
                               reinterpret_cast<const void **>(cfvalues.data()),
                               CFIndex(map.size()),
                               &kCFTypeDictionaryKeyCallBacks,
                               &kCFTypeDictionaryValueCallBacks);

    QCFType<CFDataRef> xmlData = CFPropertyListCreateData(
                 kCFAllocatorDefault, propertyList, kCFPropertyListXMLFormat_v1_0, 0, 0);

    return file.write(QByteArray::fromRawCFData(xmlData)) == CFDataGetLength(xmlData);
}

QT_END_NAMESPACE
#endif //QT_NO_SETTINGS
