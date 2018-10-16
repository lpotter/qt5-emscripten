/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "rcc.h"

#include <qbytearray.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qfile.h>
#include <qiodevice.h>
#include <qlocale.h>
#include <qstack.h>
#include <qxmlstream.h>

#include <algorithm>

// Note: A copy of this file is used in Qt Designer (qttools/src/designer/src/lib/shared/rcc.cpp)

QT_BEGIN_NAMESPACE

enum {
    CONSTANT_USENAMESPACE = 1,
    CONSTANT_COMPRESSLEVEL_DEFAULT = -1,
    CONSTANT_COMPRESSTHRESHOLD_DEFAULT = 70
};


#define writeString(s) write(s, sizeof(s))

void RCCResourceLibrary::write(const char *str, int len)
{
    --len; // trailing \0 on string literals...
    int n = m_out.size();
    m_out.resize(n + len);
    memcpy(m_out.data() + n, str, len);
}

void RCCResourceLibrary::writeByteArray(const QByteArray &other)
{
    if (m_format == Pass2) {
        m_outDevice->write(other);
    } else {
        m_out.append(other);
    }
}

static inline QString msgOpenReadFailed(const QString &fname, const QString &why)
{
    return QString::fromLatin1("Unable to open %1 for reading: %2\n").arg(fname, why);
}


///////////////////////////////////////////////////////////
//
// RCCFileInfo
//
///////////////////////////////////////////////////////////

class RCCFileInfo
{
public:
    enum Flags
    {
        NoFlags = 0x00,
        Compressed = 0x01,
        Directory = 0x02
    };

    RCCFileInfo(const QString &name = QString(), const QFileInfo &fileInfo = QFileInfo(),
                QLocale::Language language = QLocale::C,
                QLocale::Country country = QLocale::AnyCountry,
                uint flags = NoFlags,
                int compressLevel = CONSTANT_COMPRESSLEVEL_DEFAULT,
                int compressThreshold = CONSTANT_COMPRESSTHRESHOLD_DEFAULT);
    ~RCCFileInfo();

    QString resourceName() const;

public:
    qint64 writeDataBlob(RCCResourceLibrary &lib, qint64 offset, QString *errorMessage);
    qint64 writeDataName(RCCResourceLibrary &, qint64 offset);
    void writeDataInfo(RCCResourceLibrary &lib);

    int m_flags;
    QString m_name;
    QLocale::Language m_language;
    QLocale::Country m_country;
    QFileInfo m_fileInfo;
    RCCFileInfo *m_parent;
    QHash<QString, RCCFileInfo*> m_children;
    int m_compressLevel;
    int m_compressThreshold;

    qint64 m_nameOffset;
    qint64 m_dataOffset;
    qint64 m_childOffset;
};

RCCFileInfo::RCCFileInfo(const QString &name, const QFileInfo &fileInfo,
    QLocale::Language language, QLocale::Country country, uint flags,
    int compressLevel, int compressThreshold)
{
    m_name = name;
    m_fileInfo = fileInfo;
    m_language = language;
    m_country = country;
    m_flags = flags;
    m_parent = 0;
    m_nameOffset = 0;
    m_dataOffset = 0;
    m_childOffset = 0;
    m_compressLevel = compressLevel;
    m_compressThreshold = compressThreshold;
}

RCCFileInfo::~RCCFileInfo()
{
    qDeleteAll(m_children);
}

QString RCCFileInfo::resourceName() const
{
    QString resource = m_name;
    for (RCCFileInfo *p = m_parent; p; p = p->m_parent)
        resource = resource.prepend(p->m_name + QLatin1Char('/'));
    return QLatin1Char(':') + resource;
}

void RCCFileInfo::writeDataInfo(RCCResourceLibrary &lib)
{
    const bool text = lib.m_format == RCCResourceLibrary::C_Code;
    const bool pass1 = lib.m_format == RCCResourceLibrary::Pass1;
    //some info
    if (text || pass1) {
        if (m_language != QLocale::C) {
            lib.writeString("  // ");
            lib.writeByteArray(resourceName().toLocal8Bit());
            lib.writeString(" [");
            lib.writeByteArray(QByteArray::number(m_country));
            lib.writeString("::");
            lib.writeByteArray(QByteArray::number(m_language));
            lib.writeString("[\n  ");
        } else {
            lib.writeString("  // ");
            lib.writeByteArray(resourceName().toLocal8Bit());
            lib.writeString("\n  ");
        }
    }

    //pointer data
    if (m_flags & RCCFileInfo::Directory) {
        // name offset
        lib.writeNumber4(m_nameOffset);

        // flags
        lib.writeNumber2(m_flags);

        // child count
        lib.writeNumber4(m_children.size());

        // first child offset
        lib.writeNumber4(m_childOffset);
    } else {
        // name offset
        lib.writeNumber4(m_nameOffset);

        // flags
        lib.writeNumber2(m_flags);

        // locale
        lib.writeNumber2(m_country);
        lib.writeNumber2(m_language);

        //data offset
        lib.writeNumber4(m_dataOffset);
    }
    if (text || pass1)
        lib.writeChar('\n');

    if (lib.formatVersion() >= 2) {
        // last modified time stamp
        const QDateTime lastModified = m_fileInfo.lastModified();
        quint64 lastmod = quint64(lastModified.isValid() ? lastModified.toMSecsSinceEpoch() : 0);
        static const quint64 sourceDate = 1000 * qgetenv("QT_RCC_SOURCE_DATE_OVERRIDE").toULongLong();
        if (sourceDate != 0)
            lastmod = sourceDate;
        lib.writeNumber8(lastmod);
        if (text || pass1)
            lib.writeChar('\n');
    }
}

qint64 RCCFileInfo::writeDataBlob(RCCResourceLibrary &lib, qint64 offset,
    QString *errorMessage)
{
    const bool text = lib.m_format == RCCResourceLibrary::C_Code;
    const bool pass1 = lib.m_format == RCCResourceLibrary::Pass1;
    const bool pass2 = lib.m_format == RCCResourceLibrary::Pass2;
    const bool binary = lib.m_format == RCCResourceLibrary::Binary;

    //capture the offset
    m_dataOffset = offset;

    //find the data to be written
    QFile file(m_fileInfo.absoluteFilePath());
    if (!file.open(QFile::ReadOnly)) {
        *errorMessage = msgOpenReadFailed(m_fileInfo.absoluteFilePath(), file.errorString());
        return 0;
    }
    QByteArray data = file.readAll();

#ifndef QT_NO_COMPRESS
    // Check if compression is useful for this file
    if (m_compressLevel != 0 && data.size() != 0) {
        QByteArray compressed =
            qCompress(reinterpret_cast<uchar *>(data.data()), data.size(), m_compressLevel);

        int compressRatio = int(100.0 * (data.size() - compressed.size()) / data.size());
        if (compressRatio >= m_compressThreshold) {
            data = compressed;
            m_flags |= Compressed;
        }
    }
#endif // QT_NO_COMPRESS

    // some info
    if (text || pass1) {
        lib.writeString("  // ");
        lib.writeByteArray(m_fileInfo.absoluteFilePath().toLocal8Bit());
        lib.writeString("\n  ");
    }

    // write the length

    if (text || binary || pass2)
        lib.writeNumber4(data.size());
    if (text || pass1)
        lib.writeString("\n  ");
    offset += 4;

    // write the payload
    const char *p = data.constData();
    if (text) {
        for (int i = data.size(), j = 0; --i >= 0; --j) {
            lib.writeHex(*p++);
            if (j == 0) {
                lib.writeString("\n  ");
                j = 16;
            }
        }
    } else if (binary || pass2) {
        lib.writeByteArray(data);
    }
    offset += data.size();

    // done
    if (text || pass1)
        lib.writeString("\n  ");
    return offset;
}

qint64 RCCFileInfo::writeDataName(RCCResourceLibrary &lib, qint64 offset)
{
    const bool text = lib.m_format == RCCResourceLibrary::C_Code;
    const bool pass1 = lib.m_format == RCCResourceLibrary::Pass1;

    // capture the offset
    m_nameOffset = offset;

    // some info
    if (text || pass1) {
        lib.writeString("  // ");
        lib.writeByteArray(m_name.toLocal8Bit());
        lib.writeString("\n  ");
    }

    // write the length
    lib.writeNumber2(m_name.length());
    if (text || pass1)
        lib.writeString("\n  ");
    offset += 2;

    // write the hash
    lib.writeNumber4(qt_hash(m_name));
    if (text || pass1)
        lib.writeString("\n  ");
    offset += 4;

    // write the m_name
    const QChar *unicode = m_name.unicode();
    for (int i = 0; i < m_name.length(); ++i) {
        lib.writeNumber2(unicode[i].unicode());
        if ((text || pass1) && i % 16 == 0)
            lib.writeString("\n  ");
    }
    offset += m_name.length()*2;

    // done
    if (text || pass1)
        lib.writeString("\n  ");
    return offset;
}


///////////////////////////////////////////////////////////
//
// RCCResourceLibrary
//
///////////////////////////////////////////////////////////

RCCResourceLibrary::Strings::Strings() :
   TAG_RCC(QLatin1String("RCC")),
   TAG_RESOURCE(QLatin1String("qresource")),
   TAG_FILE(QLatin1String("file")),
   ATTRIBUTE_LANG(QLatin1String("lang")),
   ATTRIBUTE_PREFIX(QLatin1String("prefix")),
   ATTRIBUTE_ALIAS(QLatin1String("alias")),
   ATTRIBUTE_THRESHOLD(QLatin1String("threshold")),
   ATTRIBUTE_COMPRESS(QLatin1String("compress"))
{
}

RCCResourceLibrary::RCCResourceLibrary(quint8 formatVersion)
  : m_root(0),
    m_format(C_Code),
    m_verbose(false),
    m_compressLevel(CONSTANT_COMPRESSLEVEL_DEFAULT),
    m_compressThreshold(CONSTANT_COMPRESSTHRESHOLD_DEFAULT),
    m_treeOffset(0),
    m_namesOffset(0),
    m_dataOffset(0),
    m_useNameSpace(CONSTANT_USENAMESPACE),
    m_errorDevice(0),
    m_outDevice(0),
    m_formatVersion(formatVersion)
{
    m_out.reserve(30 * 1000 * 1000);
}

RCCResourceLibrary::~RCCResourceLibrary()
{
    delete m_root;
}

enum RCCXmlTag {
    RccTag,
    ResourceTag,
    FileTag
};
Q_DECLARE_TYPEINFO(RCCXmlTag, Q_PRIMITIVE_TYPE);

bool RCCResourceLibrary::interpretResourceFile(QIODevice *inputDevice,
    const QString &fname, QString currentPath, bool listMode)
{
    Q_ASSERT(m_errorDevice);
    const QChar slash = QLatin1Char('/');
    if (!currentPath.isEmpty() && !currentPath.endsWith(slash))
        currentPath += slash;

    QXmlStreamReader reader(inputDevice);
    QStack<RCCXmlTag> tokens;

    QString prefix;
    QLocale::Language language = QLocale::c().language();
    QLocale::Country country = QLocale::c().country();
    QString alias;
    int compressLevel = m_compressLevel;
    int compressThreshold = m_compressThreshold;

    while (!reader.atEnd()) {
        QXmlStreamReader::TokenType t = reader.readNext();
        switch (t) {
        case QXmlStreamReader::StartElement:
            if (reader.name() == m_strings.TAG_RCC) {
                if (!tokens.isEmpty())
                    reader.raiseError(QLatin1String("expected <RCC> tag"));
                else
                    tokens.push(RccTag);
            } else if (reader.name() == m_strings.TAG_RESOURCE) {
                if (tokens.isEmpty() || tokens.top() != RccTag) {
                    reader.raiseError(QLatin1String("unexpected <RESOURCE> tag"));
                } else {
                    tokens.push(ResourceTag);

                    QXmlStreamAttributes attributes = reader.attributes();
                    language = QLocale::c().language();
                    country = QLocale::c().country();

                    if (attributes.hasAttribute(m_strings.ATTRIBUTE_LANG)) {
                        QString attribute = attributes.value(m_strings.ATTRIBUTE_LANG).toString();
                        QLocale lang = QLocale(attribute);
                        language = lang.language();
                        if (2 == attribute.length()) {
                            // Language only
                            country = QLocale::AnyCountry;
                        } else {
                            country = lang.country();
                        }
                    }

                    prefix.clear();
                    if (attributes.hasAttribute(m_strings.ATTRIBUTE_PREFIX))
                        prefix = attributes.value(m_strings.ATTRIBUTE_PREFIX).toString();
                    if (!prefix.startsWith(slash))
                        prefix.prepend(slash);
                    if (!prefix.endsWith(slash))
                        prefix += slash;
                }
            } else if (reader.name() == m_strings.TAG_FILE) {
                if (tokens.isEmpty() || tokens.top() != ResourceTag) {
                    reader.raiseError(QLatin1String("unexpected <FILE> tag"));
                } else {
                    tokens.push(FileTag);

                    QXmlStreamAttributes attributes = reader.attributes();
                    alias.clear();
                    if (attributes.hasAttribute(m_strings.ATTRIBUTE_ALIAS))
                        alias = attributes.value(m_strings.ATTRIBUTE_ALIAS).toString();

                    compressLevel = m_compressLevel;
                    if (attributes.hasAttribute(m_strings.ATTRIBUTE_COMPRESS))
                        compressLevel = attributes.value(m_strings.ATTRIBUTE_COMPRESS).toString().toInt();

                    compressThreshold = m_compressThreshold;
                    if (attributes.hasAttribute(m_strings.ATTRIBUTE_THRESHOLD))
                        compressThreshold = attributes.value(m_strings.ATTRIBUTE_THRESHOLD).toString().toInt();

                    // Special case for -no-compress. Overrides all other settings.
                    if (m_compressLevel == -2)
                        compressLevel = 0;
                }
            } else {
                reader.raiseError(QString(QLatin1String("unexpected tag: %1")).arg(reader.name().toString()));
            }
            break;

        case QXmlStreamReader::EndElement:
            if (reader.name() == m_strings.TAG_RCC) {
                if (!tokens.isEmpty() && tokens.top() == RccTag)
                    tokens.pop();
                else
                    reader.raiseError(QLatin1String("unexpected closing tag"));
            } else if (reader.name() == m_strings.TAG_RESOURCE) {
                if (!tokens.isEmpty() && tokens.top() == ResourceTag)
                    tokens.pop();
                else
                    reader.raiseError(QLatin1String("unexpected closing tag"));
            } else if (reader.name() == m_strings.TAG_FILE) {
                if (!tokens.isEmpty() && tokens.top() == FileTag)
                    tokens.pop();
                else
                    reader.raiseError(QLatin1String("unexpected closing tag"));
            }
            break;

        case QXmlStreamReader::Characters:
            if (reader.isWhitespace())
                break;
            if (tokens.isEmpty() || tokens.top() != FileTag) {
                reader.raiseError(QLatin1String("unexpected text"));
            } else {
                QString fileName = reader.text().toString();
                if (fileName.isEmpty()) {
                    const QString msg = QString::fromLatin1("RCC: Warning: Null node in XML of '%1'\n").arg(fname);
                    m_errorDevice->write(msg.toUtf8());
                }

                if (alias.isNull())
                    alias = fileName;

                alias = QDir::cleanPath(alias);
                while (alias.startsWith(QLatin1String("../")))
                    alias.remove(0, 3);
                alias = QDir::cleanPath(m_resourceRoot) + prefix + alias;

                QString absFileName = fileName;
                if (QDir::isRelativePath(absFileName))
                    absFileName.prepend(currentPath);
                QFileInfo file(absFileName);
                if (file.isDir()) {
                    QDir dir(file.filePath());
                    if (!alias.endsWith(slash))
                        alias += slash;
                    QDirIterator it(dir, QDirIterator::FollowSymlinks|QDirIterator::Subdirectories);
                    while (it.hasNext()) {
                        it.next();
                        QFileInfo child(it.fileInfo());
                        if (child.fileName() != QLatin1String(".") && child.fileName() != QLatin1String("..")) {
                            const bool arc =
                                addFile(alias + child.fileName(),
                                        RCCFileInfo(child.fileName(),
                                                    child,
                                                    language,
                                                    country,
                                                    child.isDir() ? RCCFileInfo::Directory : RCCFileInfo::NoFlags,
                                                    compressLevel,
                                                    compressThreshold)
                                        );
                            if (!arc)
                                m_failedResources.push_back(child.fileName());
                        }
                    }
                } else if (listMode || file.isFile()) {
                    const bool arc =
                        addFile(alias,
                                RCCFileInfo(alias.section(slash, -1),
                                            file,
                                            language,
                                            country,
                                            RCCFileInfo::NoFlags,
                                            compressLevel,
                                            compressThreshold)
                                );
                    if (!arc)
                        m_failedResources.push_back(absFileName);
                } else if (file.exists()) {
                    m_failedResources.push_back(absFileName);
                    const QString msg = QString::fromLatin1("RCC: Error in '%1': Entry '%2' is neither a file nor a directory\n")
                                        .arg(fname, fileName);
                    m_errorDevice->write(msg.toUtf8());
                    return false;
                } else {
                    m_failedResources.push_back(absFileName);
                    const QString msg = QString::fromLatin1("RCC: Error in '%1': Cannot find file '%2'\n")
                                        .arg(fname, fileName);
                    m_errorDevice->write(msg.toUtf8());
                    return false;
                }
            }
            break;

        default:
            break;
        }
    }

    if (reader.hasError()) {
        int errorLine = reader.lineNumber();
        int errorColumn = reader.columnNumber();
        QString errorMessage = reader.errorString();
        QString msg = QString::fromLatin1("RCC Parse Error: '%1' Line: %2 Column: %3 [%4]\n").arg(fname).arg(errorLine).arg(errorColumn).arg(errorMessage);
        m_errorDevice->write(msg.toUtf8());
        return false;
    }

    if (m_root == 0) {
        const QString msg = QString::fromLatin1("RCC: Warning: No resources in '%1'.\n").arg(fname);
        m_errorDevice->write(msg.toUtf8());
        if (!listMode && m_format == Binary) {
            // create dummy entry, otherwise loading with QResource will crash
            m_root = new RCCFileInfo(QString(), QFileInfo(),
                    QLocale::C, QLocale::AnyCountry, RCCFileInfo::Directory);
        }
    }

    return true;
}

bool RCCResourceLibrary::addFile(const QString &alias, const RCCFileInfo &file)
{
    Q_ASSERT(m_errorDevice);
    if (file.m_fileInfo.size() > 0xffffffff) {
        const QString msg = QString::fromLatin1("File too big: %1\n").arg(file.m_fileInfo.absoluteFilePath());
        m_errorDevice->write(msg.toUtf8());
        return false;
    }
    if (!m_root)
        m_root = new RCCFileInfo(QString(), QFileInfo(), QLocale::C, QLocale::AnyCountry, RCCFileInfo::Directory);

    RCCFileInfo *parent = m_root;
    const QStringList nodes = alias.split(QLatin1Char('/'));
    for (int i = 1; i < nodes.size()-1; ++i) {
        const QString node = nodes.at(i);
        if (node.isEmpty())
            continue;
        if (!parent->m_children.contains(node)) {
            RCCFileInfo *s = new RCCFileInfo(node, QFileInfo(), QLocale::C, QLocale::AnyCountry, RCCFileInfo::Directory);
            s->m_parent = parent;
            parent->m_children.insert(node, s);
            parent = s;
        } else {
            parent = parent->m_children[node];
        }
    }

    const QString filename = nodes.at(nodes.size()-1);
    RCCFileInfo *s = new RCCFileInfo(file);
    s->m_parent = parent;
    typedef QHash<QString, RCCFileInfo*>::const_iterator ChildConstIterator;
    const ChildConstIterator cbegin = parent->m_children.constFind(filename);
    const ChildConstIterator cend = parent->m_children.constEnd();
    for (ChildConstIterator it = cbegin; it != cend; ++it) {
        if (it.key() == filename && it.value()->m_language == s->m_language &&
            it.value()->m_country == s->m_country) {
            for (const QString &name : qAsConst(m_fileNames)) {
                qWarning("%s: Warning: potential duplicate alias detected: '%s'",
                qPrintable(name), qPrintable(filename));
            }
            break;
        }
    }
    parent->m_children.insertMulti(filename, s);
    return true;
}

void RCCResourceLibrary::reset()
{
     if (m_root) {
        delete m_root;
        m_root = 0;
    }
    m_errorDevice = 0;
    m_failedResources.clear();
}


bool RCCResourceLibrary::readFiles(bool listMode, QIODevice &errorDevice)
{
    reset();
    m_errorDevice = &errorDevice;
    //read in data
    if (m_verbose) {
        const QString msg = QString::fromLatin1("Processing %1 files [listMode=%2]\n")
            .arg(m_fileNames.size()).arg(static_cast<int>(listMode));
        m_errorDevice->write(msg.toUtf8());
    }
    for (int i = 0; i < m_fileNames.size(); ++i) {
        QFile fileIn;
        QString fname = m_fileNames.at(i);
        QString pwd;
        if (fname == QLatin1String("-")) {
            fname = QLatin1String("(stdin)");
            pwd = QDir::currentPath();
            fileIn.setFileName(fname);
            if (!fileIn.open(stdin, QIODevice::ReadOnly)) {
                m_errorDevice->write(msgOpenReadFailed(fname, fileIn.errorString()).toUtf8());
                return false;
            }
        } else {
            pwd = QFileInfo(fname).path();
            fileIn.setFileName(fname);
            if (!fileIn.open(QIODevice::ReadOnly)) {
                m_errorDevice->write(msgOpenReadFailed(fname, fileIn.errorString()).toUtf8());
                return false;
            }
        }
        if (m_verbose) {
            const QString msg = QString::fromLatin1("Interpreting %1\n").arg(fname);
            m_errorDevice->write(msg.toUtf8());
        }

        if (!interpretResourceFile(&fileIn, fname, pwd, listMode))
            return false;
    }
    return true;
}

QStringList RCCResourceLibrary::dataFiles() const
{
    QStringList ret;
    QStack<RCCFileInfo*> pending;

    if (!m_root)
        return ret;
    pending.push(m_root);
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for (QHash<QString, RCCFileInfo*>::iterator it = file->m_children.begin();
            it != file->m_children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
            else
                ret.append(child->m_fileInfo.filePath());
        }
    }
    return ret;
}

// Determine map of resource identifier (':/newPrefix/images/p1.png') to file via recursion
static void resourceDataFileMapRecursion(const RCCFileInfo *m_root, const QString &path, RCCResourceLibrary::ResourceDataFileMap &m)
{
    typedef QHash<QString, RCCFileInfo*>::const_iterator ChildConstIterator;
    const QChar slash = QLatin1Char('/');
    const ChildConstIterator cend = m_root->m_children.constEnd();
    for (ChildConstIterator it = m_root->m_children.constBegin(); it != cend; ++it) {
        const RCCFileInfo *child = it.value();
        const QString childName = path + slash + child->m_name;
        if (child->m_flags & RCCFileInfo::Directory) {
            resourceDataFileMapRecursion(child, childName, m);
        } else {
            m.insert(childName, child->m_fileInfo.filePath());
        }
    }
}

RCCResourceLibrary::ResourceDataFileMap RCCResourceLibrary::resourceDataFileMap() const
{
    ResourceDataFileMap rc;
    if (m_root)
        resourceDataFileMapRecursion(m_root, QString(QLatin1Char(':')),  rc);
    return rc;
}

bool RCCResourceLibrary::output(QIODevice &outDevice, QIODevice &tempDevice, QIODevice &errorDevice)
{
    m_errorDevice = &errorDevice;

    if (m_format == Pass2) {
        const char pattern[] = { 'Q', 'R', 'C', '_', 'D', 'A', 'T', 'A' };
        bool foundSignature = false;

        while (true) {
            char c;
            for (int i = 0; i < 8; ) {
                if (!tempDevice.getChar(&c)) {
                    if (foundSignature)
                        return true;
                    m_errorDevice->write("No data signature found\n");
                    return false;
                }
                if (c == pattern[i]) {
                    ++i;
                } else {
                    for (int k = 0; k < i; ++k)
                        outDevice.putChar(pattern[k]);
                    outDevice.putChar(c);
                    i = 0;
                }
            }

            m_outDevice = &outDevice;
            quint64 start = outDevice.pos();
            writeDataBlobs();
            quint64 len = outDevice.pos() - start;

            tempDevice.seek(tempDevice.pos() + len - 8);
            foundSignature = true;
        }
    }

    //write out
    if (m_verbose)
        m_errorDevice->write("Outputting code\n");
    if (!writeHeader()) {
        m_errorDevice->write("Could not write header\n");
        return false;
    }
    if (m_root) {
        if (!writeDataBlobs()) {
            m_errorDevice->write("Could not write data blobs.\n");
            return false;
        }
        if (!writeDataNames()) {
            m_errorDevice->write("Could not write file names\n");
            return false;
        }
        if (!writeDataStructure()) {
            m_errorDevice->write("Could not write data tree\n");
            return false;
        }
    }
    if (!writeInitializer()) {
        m_errorDevice->write("Could not write footer\n");
        return false;
    }
    outDevice.write(m_out.constData(), m_out.size());
    return true;
}

void RCCResourceLibrary::writeHex(quint8 tmp)
{
    const char digits[] = "0123456789abcdef";
    writeChar('0');
    writeChar('x');
    if (tmp < 16) {
        writeChar(digits[tmp]);
    } else {
        writeChar(digits[tmp >> 4]);
        writeChar(digits[tmp & 0xf]);
    }
    writeChar(',');
}

void RCCResourceLibrary::writeNumber2(quint16 number)
{
    if (m_format == RCCResourceLibrary::Binary) {
        writeChar(number >> 8);
        writeChar(number);
    } else {
        writeHex(number >> 8);
        writeHex(number);
    }
}

void RCCResourceLibrary::writeNumber4(quint32 number)
{
    if (m_format == RCCResourceLibrary::Pass2) {
        m_outDevice->putChar(char(number >> 24));
        m_outDevice->putChar(char(number >> 16));
        m_outDevice->putChar(char(number >> 8));
        m_outDevice->putChar(char(number));
    } else if (m_format == RCCResourceLibrary::Binary) {
        writeChar(number >> 24);
        writeChar(number >> 16);
        writeChar(number >> 8);
        writeChar(number);
    } else {
        writeHex(number >> 24);
        writeHex(number >> 16);
        writeHex(number >> 8);
        writeHex(number);
    }
}

void RCCResourceLibrary::writeNumber8(quint64 number)
{
    if (m_format == RCCResourceLibrary::Pass2) {
        m_outDevice->putChar(char(number >> 56));
        m_outDevice->putChar(char(number >> 48));
        m_outDevice->putChar(char(number >> 40));
        m_outDevice->putChar(char(number >> 32));
        m_outDevice->putChar(char(number >> 24));
        m_outDevice->putChar(char(number >> 16));
        m_outDevice->putChar(char(number >> 8));
        m_outDevice->putChar(char(number));
    } else if (m_format == RCCResourceLibrary::Binary) {
        writeChar(number >> 56);
        writeChar(number >> 48);
        writeChar(number >> 40);
        writeChar(number >> 32);
        writeChar(number >> 24);
        writeChar(number >> 16);
        writeChar(number >> 8);
        writeChar(number);
    } else {
        writeHex(number >> 56);
        writeHex(number >> 48);
        writeHex(number >> 40);
        writeHex(number >> 32);
        writeHex(number >> 24);
        writeHex(number >> 16);
        writeHex(number >> 8);
        writeHex(number);
    }
}

bool RCCResourceLibrary::writeHeader()
{
    if (m_format == C_Code || m_format == Pass1) {
        writeString("/****************************************************************************\n");
        writeString("** Resource object code\n");
        writeString("**\n");
        writeString("** Created by: The Resource Compiler for Qt version ");
        writeByteArray(QT_VERSION_STR);
        writeString("\n**\n");
        writeString("** WARNING! All changes made in this file will be lost!\n");
        writeString( "*****************************************************************************/\n\n");
    } else if (m_format == Binary) {
        writeString("qres");
        writeNumber4(0);
        writeNumber4(0);
        writeNumber4(0);
        writeNumber4(0);
    }
    return true;
}

bool RCCResourceLibrary::writeDataBlobs()
{
    Q_ASSERT(m_errorDevice);
    if (m_format == C_Code) {
        writeString("static const unsigned char qt_resource_data[] = {\n");
    } else if (m_format == Binary) {
        m_dataOffset = m_out.size();
    }

    if (!m_root)
        return false;

    QStack<RCCFileInfo*> pending;
    pending.push(m_root);
    qint64 offset = 0;
    QString errorMessage;
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for (QHash<QString, RCCFileInfo*>::iterator it = file->m_children.begin();
            it != file->m_children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
            else {
                offset = child->writeDataBlob(*this, offset, &errorMessage);
                if (offset == 0) {
                    m_errorDevice->write(errorMessage.toUtf8());
                    return false;
                }
            }
        }
    }
    if (m_format == C_Code)
        writeString("\n};\n\n");
    else if (m_format == Pass1) {
        if (offset < 8)
            offset = 8;
        writeString("\nstatic const unsigned char qt_resource_data[");
        writeByteArray(QByteArray::number(offset));
        writeString("] = { 'Q', 'R', 'C', '_', 'D', 'A', 'T', 'A' };\n\n");
    }
    return true;
}

bool RCCResourceLibrary::writeDataNames()
{
    if (m_format == C_Code || m_format == Pass1)
        writeString("static const unsigned char qt_resource_name[] = {\n");
    else if (m_format == Binary)
        m_namesOffset = m_out.size();

    QHash<QString, int> names;
    QStack<RCCFileInfo*> pending;

    if (!m_root)
        return false;

    pending.push(m_root);
    qint64 offset = 0;
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        for (QHash<QString, RCCFileInfo*>::iterator it = file->m_children.begin();
            it != file->m_children.end(); ++it) {
            RCCFileInfo *child = it.value();
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
            if (names.contains(child->m_name)) {
                child->m_nameOffset = names.value(child->m_name);
            } else {
                names.insert(child->m_name, offset);
                offset = child->writeDataName(*this, offset);
            }
        }
    }
    if (m_format == C_Code || m_format == Pass1)
        writeString("\n};\n\n");
    return true;
}

struct qt_rcc_compare_hash
{
    typedef bool result_type;
    result_type operator()(const RCCFileInfo *left, const RCCFileInfo *right) const
    {
        return qt_hash(left->m_name) < qt_hash(right->m_name);
    }
};

bool RCCResourceLibrary::writeDataStructure()
{
    if (m_format == C_Code || m_format == Pass1)
        writeString("static const unsigned char qt_resource_struct[] = {\n");
    else if (m_format == Binary)
        m_treeOffset = m_out.size();
    QStack<RCCFileInfo*> pending;

    if (!m_root)
        return false;

    //calculate the child offsets (flat)
    pending.push(m_root);
    int offset = 1;
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();
        file->m_childOffset = offset;

        //sort by hash value for binary lookup
        QList<RCCFileInfo*> m_children = file->m_children.values();
        std::sort(m_children.begin(), m_children.end(), qt_rcc_compare_hash());

        //write out the actual data now
        for (int i = 0; i < m_children.size(); ++i) {
            RCCFileInfo *child = m_children.at(i);
            ++offset;
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
        }
    }

    //write out the structure (ie iterate again!)
    pending.push(m_root);
    m_root->writeDataInfo(*this);
    while (!pending.isEmpty()) {
        RCCFileInfo *file = pending.pop();

        //sort by hash value for binary lookup
        QList<RCCFileInfo*> m_children = file->m_children.values();
        std::sort(m_children.begin(), m_children.end(), qt_rcc_compare_hash());

        //write out the actual data now
        for (int i = 0; i < m_children.size(); ++i) {
            RCCFileInfo *child = m_children.at(i);
            child->writeDataInfo(*this);
            if (child->m_flags & RCCFileInfo::Directory)
                pending.push(child);
        }
    }
    if (m_format == C_Code || m_format == Pass1)
        writeString("\n};\n\n");

    return true;
}

void RCCResourceLibrary::writeMangleNamespaceFunction(const QByteArray &name)
{
    if (m_useNameSpace) {
        writeString("QT_RCC_MANGLE_NAMESPACE(");
        writeByteArray(name);
        writeChar(')');
    } else {
        writeByteArray(name);
    }
}

void RCCResourceLibrary::writeAddNamespaceFunction(const QByteArray &name)
{
    if (m_useNameSpace) {
        writeString("QT_RCC_PREPEND_NAMESPACE(");
        writeByteArray(name);
        writeChar(')');
    } else {
        writeByteArray(name);
    }
}

bool RCCResourceLibrary::writeInitializer()
{
    if (m_format == C_Code || m_format == Pass1) {
        //write("\nQT_BEGIN_NAMESPACE\n");
        QString initNameStr = m_initName;
        if (!initNameStr.isEmpty()) {
            initNameStr.prepend(QLatin1Char('_'));
            initNameStr.replace(QRegExp(QLatin1String("[^a-zA-Z0-9_]")), QLatin1String("_"));
        }
        QByteArray initName = initNameStr.toLatin1();

        //init
        if (m_useNameSpace) {
            writeString("#ifdef QT_NAMESPACE\n"
                        "#  define QT_RCC_PREPEND_NAMESPACE(name) ::QT_NAMESPACE::name\n"
                        "#  define QT_RCC_MANGLE_NAMESPACE0(x) x\n"
                        "#  define QT_RCC_MANGLE_NAMESPACE1(a, b) a##_##b\n"
                        "#  define QT_RCC_MANGLE_NAMESPACE2(a, b) QT_RCC_MANGLE_NAMESPACE1(a,b)\n"
                        "#  define QT_RCC_MANGLE_NAMESPACE(name) QT_RCC_MANGLE_NAMESPACE2( \\\n"
                        "        QT_RCC_MANGLE_NAMESPACE0(name), QT_RCC_MANGLE_NAMESPACE0(QT_NAMESPACE))\n"
                        "#else\n"
                        "#   define QT_RCC_PREPEND_NAMESPACE(name) name\n"
                        "#   define QT_RCC_MANGLE_NAMESPACE(name) name\n"
                        "#endif\n\n");

            writeString("#ifdef QT_NAMESPACE\n"
                        "namespace QT_NAMESPACE {\n"
                        "#endif\n\n");
        }

        if (m_root) {
            writeString("bool qRegisterResourceData"
                "(int, const unsigned char *, "
                "const unsigned char *, const unsigned char *);\n\n");
            writeString("bool qUnregisterResourceData"
                "(int, const unsigned char *, "
                "const unsigned char *, const unsigned char *);\n\n");
        }

        if (m_useNameSpace)
            writeString("#ifdef QT_NAMESPACE\n}\n#endif\n\n");

        QByteArray initResources = "qInitResources";
        initResources += initName;

        // Work around -Wmissing-declarations warnings.
        writeString("int ");
        writeMangleNamespaceFunction(initResources);
        writeString("();\n");

        writeString("int ");
        writeMangleNamespaceFunction(initResources);
        writeString("()\n{\n");

        if (m_root) {
            writeString("    ");
            writeAddNamespaceFunction("qRegisterResourceData");
            writeString("\n        (");
            writeHex(m_formatVersion);
            writeString(" qt_resource_struct, "
                       "qt_resource_name, qt_resource_data);\n");
        }
        writeString("    return 1;\n");
        writeString("}\n\n");

        //cleanup
        QByteArray cleanResources = "qCleanupResources";
        cleanResources += initName;

        // Work around -Wmissing-declarations warnings.
        writeString("int ");
        writeMangleNamespaceFunction(cleanResources);
        writeString("();\n");

        writeString("int ");
        writeMangleNamespaceFunction(cleanResources);
        writeString("()\n{\n");
        if (m_root) {
            writeString("    ");
            writeAddNamespaceFunction("qUnregisterResourceData");
            writeString("\n       (");
            writeHex(m_formatVersion);
            writeString(" qt_resource_struct, "
                      "qt_resource_name, qt_resource_data);\n");
        }
        writeString("    return 1;\n");
        writeString("}\n\n");

        writeByteArray(
                    "namespace {\n"
                  "   struct initializer {\n"
                  "       initializer() { QT_RCC_MANGLE_NAMESPACE(" + initResources + ")(); }\n"
                  "       ~initializer() { QT_RCC_MANGLE_NAMESPACE(" + cleanResources + ")(); }\n"
                  "   } dummy;\n"
                  "}\n");

    } else if (m_format == Binary) {
        int i = 4;
        char *p = m_out.data();
        p[i++] = 0;
        p[i++] = 0;
        p[i++] = 0;
        p[i++] = m_formatVersion;

        p[i++] = (m_treeOffset >> 24) & 0xff;
        p[i++] = (m_treeOffset >> 16) & 0xff;
        p[i++] = (m_treeOffset >>  8) & 0xff;
        p[i++] = (m_treeOffset >>  0) & 0xff;

        p[i++] = (m_dataOffset >> 24) & 0xff;
        p[i++] = (m_dataOffset >> 16) & 0xff;
        p[i++] = (m_dataOffset >>  8) & 0xff;
        p[i++] = (m_dataOffset >>  0) & 0xff;

        p[i++] = (m_namesOffset >> 24) & 0xff;
        p[i++] = (m_namesOffset >> 16) & 0xff;
        p[i++] = (m_namesOffset >>  8) & 0xff;
        p[i++] = (m_namesOffset >>  0) & 0xff;
    }
    return true;
}

QT_END_NAMESPACE
