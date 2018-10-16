/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwindowsfontdatabase_p.h"
#include "qwindowsfontdatabase_ft_p.h" // for default font
#include "qwindowsfontengine_p.h"
#include "qwindowsfontenginedirectwrite_p.h"
#include <QtCore/qt_windows.h>

#include <QtGui/QFont>
#include <QtGui/QGuiApplication>
#include <QtGui/private/qhighdpiscaling_p.h>

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QtEndian>
#include <QtCore/QThreadStorage>
#include <QtCore/private/qsystemlibrary_p.h>

#include <wchar.h>

#if !defined(QT_NO_DIRECTWRITE)
#  if defined(QT_USE_DIRECTWRITE2)
#    include <dwrite_2.h>
#  else
#    include <dwrite.h>
#  endif
#  include <d2d1.h>
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaFonts, "qt.qpa.fonts")

#ifndef QT_NO_DIRECTWRITE
// ### fixme: Consider direct linking of dwrite.dll once Windows Vista pre SP2 is dropped (QTBUG-49711)

typedef HRESULT (WINAPI *DWriteCreateFactoryType)(DWRITE_FACTORY_TYPE, const IID &, IUnknown **);

static inline DWriteCreateFactoryType resolveDWriteCreateFactory()
{
    QSystemLibrary library(QStringLiteral("dwrite"));
    QFunctionPointer result = library.resolve("DWriteCreateFactory");
    if (Q_UNLIKELY(!result)) {
        qWarning("Unable to load dwrite.dll");
        return nullptr;
    }
    return reinterpret_cast<DWriteCreateFactoryType>(result);
}

static void createDirectWriteFactory(IDWriteFactory **factory)
{
    *factory = nullptr;

    static const DWriteCreateFactoryType dWriteCreateFactory = resolveDWriteCreateFactory();
    if (!dWriteCreateFactory)
        return;

    IUnknown *result = NULL;
#if defined(QT_USE_DIRECTWRITE2)
    dWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory2), &result);
#endif

    if (result == NULL) {
        if (FAILED(dWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &result))) {
            qErrnoWarning("DWriteCreateFactory failed");
            return;
        }
    }

    *factory = static_cast<IDWriteFactory *>(result);
}

static inline bool useDirectWrite(QFont::HintingPreference hintingPreference,
                                  const QString &familyName = QString(),
                                  bool isColorFont = false)
{
    const unsigned options = QWindowsFontDatabase::fontOptions();
    if (Q_UNLIKELY(options & QWindowsFontDatabase::DontUseDirectWriteFonts))
        return false;

    // At some scales, GDI will misrender the MingLiU font, so we force use of
    // DirectWrite to work around the issue.
    if (Q_UNLIKELY(familyName.startsWith(QLatin1String("MingLiU"))))
        return true;

    if (isColorFont)
        return (options & QWindowsFontDatabase::DontUseColorFonts) == 0;

    return hintingPreference == QFont::PreferNoHinting
        || hintingPreference == QFont::PreferVerticalHinting
        || (QHighDpiScaling::isActive() && hintingPreference == QFont::PreferDefaultHinting);
}
#endif // !QT_NO_DIRECTWRITE

// Helper classes for creating font engines directly from font data
namespace {

#   pragma pack(1)

    // Common structure for all formats of the "name" table
    struct NameTable
    {
        quint16 format;
        quint16 count;
        quint16 stringOffset;
    };

    struct NameRecord
    {
        quint16 platformID;
        quint16 encodingID;
        quint16 languageID;
        quint16 nameID;
        quint16 length;
        quint16 offset;
    };

    struct OffsetSubTable
    {
        quint32 scalerType;
        quint16 numTables;
        quint16 searchRange;
        quint16 entrySelector;
        quint16 rangeShift;
    };

    struct TableDirectory
    {
        quint32 identifier;
        quint32 checkSum;
        quint32 offset;
        quint32 length;
    };

    struct OS2Table
    {
        quint16 version;
        qint16  avgCharWidth;
        quint16 weightClass;
        quint16 widthClass;
        quint16 type;
        qint16  subscriptXSize;
        qint16  subscriptYSize;
        qint16  subscriptXOffset;
        qint16  subscriptYOffset;
        qint16  superscriptXSize;
        qint16  superscriptYSize;
        qint16  superscriptXOffset;
        qint16  superscriptYOffset;
        qint16  strikeOutSize;
        qint16  strikeOutPosition;
        qint16  familyClass;
        quint8  panose[10];
        quint32 unicodeRanges[4];
        quint8  vendorID[4];
        quint16 selection;
        quint16 firstCharIndex;
        quint16 lastCharIndex;
        qint16  typoAscender;
        qint16  typoDescender;
        qint16  typoLineGap;
        quint16 winAscent;
        quint16 winDescent;
        quint32 codepageRanges[2];
        qint16  height;
        qint16  capHeight;
        quint16 defaultChar;
        quint16 breakChar;
        quint16 maxContext;
    };

#   pragma pack()

    class EmbeddedFont
    {
    public:
        EmbeddedFont(const QByteArray &fontData) : m_fontData(fontData) {}

        QString changeFamilyName(const QString &newFamilyName);
        QByteArray data() const { return m_fontData; }
        TableDirectory *tableDirectoryEntry(const QByteArray &tagName);
        QString familyName(TableDirectory *nameTableDirectory = 0);

    private:
        QByteArray m_fontData;
    };

    TableDirectory *EmbeddedFont::tableDirectoryEntry(const QByteArray &tagName)
    {
        Q_ASSERT(tagName.size() == 4);
        quint32 tagId = *(reinterpret_cast<const quint32 *>(tagName.constData()));
        const size_t fontDataSize = m_fontData.size();
        if (Q_UNLIKELY(fontDataSize < sizeof(OffsetSubTable)))
            return 0;

        OffsetSubTable *offsetSubTable = reinterpret_cast<OffsetSubTable *>(m_fontData.data());
        TableDirectory *tableDirectory = reinterpret_cast<TableDirectory *>(offsetSubTable + 1);

        const size_t tableCount = qFromBigEndian<quint16>(offsetSubTable->numTables);
        if (Q_UNLIKELY(fontDataSize < sizeof(OffsetSubTable) + sizeof(TableDirectory) * tableCount))
            return 0;

        TableDirectory *tableDirectoryEnd = tableDirectory + tableCount;
        for (TableDirectory *entry = tableDirectory; entry < tableDirectoryEnd; ++entry) {
            if (entry->identifier == tagId)
                return entry;
        }

        return 0;
    }

    QString EmbeddedFont::familyName(TableDirectory *nameTableDirectoryEntry)
    {
        QString name;

        if (nameTableDirectoryEntry == 0)
            nameTableDirectoryEntry = tableDirectoryEntry("name");

        if (nameTableDirectoryEntry != 0) {
            quint32 offset = qFromBigEndian<quint32>(nameTableDirectoryEntry->offset);
            if (Q_UNLIKELY(quint32(m_fontData.size()) < offset + sizeof(NameTable)))
                return QString();

            NameTable *nameTable = reinterpret_cast<NameTable *>(m_fontData.data() + offset);
            NameRecord *nameRecord = reinterpret_cast<NameRecord *>(nameTable + 1);

            quint16 nameTableCount = qFromBigEndian<quint16>(nameTable->count);
            if (Q_UNLIKELY(quint32(m_fontData.size()) < offset + sizeof(NameRecord) * nameTableCount))
                return QString();

            for (int i = 0; i < nameTableCount; ++i, ++nameRecord) {
                if (qFromBigEndian<quint16>(nameRecord->nameID) == 1
                 && qFromBigEndian<quint16>(nameRecord->platformID) == 3 // Windows
                 && qFromBigEndian<quint16>(nameRecord->languageID) == 0x0409) { // US English
                    quint16 stringOffset = qFromBigEndian<quint16>(nameTable->stringOffset);
                    quint16 nameOffset = qFromBigEndian<quint16>(nameRecord->offset);
                    quint16 nameLength = qFromBigEndian<quint16>(nameRecord->length);

                    if (Q_UNLIKELY(quint32(m_fontData.size()) < offset + stringOffset + nameOffset + nameLength))
                        return QString();

                    const void *ptr = reinterpret_cast<const quint8 *>(nameTable)
                                                        + stringOffset
                                                        + nameOffset;

                    const quint16 *s = reinterpret_cast<const quint16 *>(ptr);
                    const quint16 *e = s + nameLength / sizeof(quint16);
                    while (s != e)
                        name += QChar( qFromBigEndian<quint16>(*s++));
                    break;
                }
            }
        }

        return name;
    }

    QString EmbeddedFont::changeFamilyName(const QString &newFamilyName)
    {
        TableDirectory *nameTableDirectoryEntry = tableDirectoryEntry("name");
        if (nameTableDirectoryEntry == 0)
            return QString();

        QString oldFamilyName = familyName(nameTableDirectoryEntry);

        // Reserve size for name table header, five required name records and string
        const int requiredRecordCount = 5;
        quint16 nameIds[requiredRecordCount] = { 1, 2, 3, 4, 6 };

        int sizeOfHeader = sizeof(NameTable) + sizeof(NameRecord) * requiredRecordCount;
        int newFamilyNameSize = newFamilyName.size() * int(sizeof(quint16));

        const QString regularString = QString::fromLatin1("Regular");
        int regularStringSize = regularString.size() * int(sizeof(quint16));

        // Align table size of table to 32 bits (pad with 0)
        int fullSize = ((sizeOfHeader + newFamilyNameSize + regularStringSize) & ~3) + 4;

        QByteArray newNameTable(fullSize, char(0));

        {
            NameTable *nameTable = reinterpret_cast<NameTable *>(newNameTable.data());
            nameTable->count = qbswap<quint16>(requiredRecordCount);
            nameTable->stringOffset = qbswap<quint16>(sizeOfHeader);

            NameRecord *nameRecord = reinterpret_cast<NameRecord *>(nameTable + 1);
            for (int i = 0; i < requiredRecordCount; ++i, nameRecord++) {
                nameRecord->nameID = qbswap<quint16>(nameIds[i]);
                nameRecord->encodingID = qbswap<quint16>(1);
                nameRecord->languageID = qbswap<quint16>(0x0409);
                nameRecord->platformID = qbswap<quint16>(3);
                nameRecord->length = qbswap<quint16>(newFamilyNameSize);

                // Special case for sub-family
                if (nameIds[i] == 4) {
                    nameRecord->offset = qbswap<quint16>(newFamilyNameSize);
                    nameRecord->length = qbswap<quint16>(regularStringSize);
                }
            }

            // nameRecord now points to string data
            quint16 *stringStorage = reinterpret_cast<quint16 *>(nameRecord);
            const quint16 *sourceString = newFamilyName.utf16();
            for (int i = 0; i < newFamilyName.size(); ++i)
                stringStorage[i] = qbswap<quint16>(sourceString[i]);
            stringStorage += newFamilyName.size();

            sourceString = regularString.utf16();
            for (int i = 0; i < regularString.size(); ++i)
                stringStorage[i] = qbswap<quint16>(sourceString[i]);
        }

        quint32 *p = reinterpret_cast<quint32 *>(newNameTable.data());
        quint32 *tableEnd = reinterpret_cast<quint32 *>(newNameTable.data() + fullSize);

        quint32 checkSum = 0;
        while (p < tableEnd)
            checkSum +=  qFromBigEndian<quint32>(*(p++));

        nameTableDirectoryEntry->checkSum = qbswap<quint32>(checkSum);
        nameTableDirectoryEntry->offset = qbswap<quint32>(m_fontData.size());
        nameTableDirectoryEntry->length = qbswap<quint32>(fullSize);

        m_fontData.append(newNameTable);

        return oldFamilyName;
    }

#if !defined(QT_NO_DIRECTWRITE)

    class DirectWriteFontFileStream: public IDWriteFontFileStream
    {
        Q_DISABLE_COPY(DirectWriteFontFileStream)
    public:
        DirectWriteFontFileStream(const QByteArray &fontData)
            : m_fontData(fontData)
            , m_referenceCount(0)
        {
        }
        virtual ~DirectWriteFontFileStream()
        {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **object);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

        HRESULT STDMETHODCALLTYPE ReadFileFragment(const void **fragmentStart, UINT64 fileOffset,
                                                   UINT64 fragmentSize, OUT void **fragmentContext);
        void STDMETHODCALLTYPE ReleaseFileFragment(void *fragmentContext);
        HRESULT STDMETHODCALLTYPE GetFileSize(OUT UINT64 *fileSize);
        HRESULT STDMETHODCALLTYPE GetLastWriteTime(OUT UINT64 *lastWriteTime);

    private:
        QByteArray m_fontData;
        ULONG m_referenceCount;
    };

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::QueryInterface(REFIID iid, void **object)
    {
        if (iid == IID_IUnknown || iid == __uuidof(IDWriteFontFileStream)) {
            *object = this;
            AddRef();
            return S_OK;
        } else {
            *object = NULL;
            return E_NOINTERFACE;
        }
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileStream::AddRef()
    {
        return InterlockedIncrement(&m_referenceCount);
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileStream::Release()
    {
        ULONG newCount = InterlockedDecrement(&m_referenceCount);
        if (newCount == 0)
            delete this;
        return newCount;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::ReadFileFragment(
        const void **fragmentStart,
        UINT64 fileOffset,
        UINT64 fragmentSize,
        OUT void **fragmentContext)
    {
        *fragmentContext = NULL;
        if (fileOffset + fragmentSize <= quint64(m_fontData.size())) {
            *fragmentStart = m_fontData.data() + fileOffset;
            return S_OK;
        } else {
            *fragmentStart = NULL;
            return E_FAIL;
        }
    }

    void STDMETHODCALLTYPE DirectWriteFontFileStream::ReleaseFileFragment(void *)
    {
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::GetFileSize(UINT64 *fileSize)
    {
        *fileSize = m_fontData.size();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileStream::GetLastWriteTime(UINT64 *lastWriteTime)
    {
        *lastWriteTime = 0;
        return E_NOTIMPL;
    }

    class DirectWriteFontFileLoader: public IDWriteFontFileLoader
    {
    public:
        DirectWriteFontFileLoader() : m_referenceCount(0) {}
        virtual ~DirectWriteFontFileLoader()
        {
        }

        inline void addKey(const void *key, const QByteArray &fontData)
        {
            Q_ASSERT(!m_fontDatas.contains(key));
            m_fontDatas.insert(key, fontData);
        }

        inline void removeKey(const void *key)
        {
            m_fontDatas.remove(key);
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **object);
        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();

        HRESULT STDMETHODCALLTYPE CreateStreamFromKey(void const *fontFileReferenceKey,
                                                      UINT32 fontFileReferenceKeySize,
                                                      OUT IDWriteFontFileStream **fontFileStream);

    private:
        ULONG m_referenceCount;
        QHash<const void *, QByteArray> m_fontDatas;
    };

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileLoader::QueryInterface(const IID &iid,
                                                                        void **object)
    {
        if (iid == IID_IUnknown || iid == __uuidof(IDWriteFontFileLoader)) {
            *object = this;
            AddRef();
            return S_OK;
        } else {
            *object = NULL;
            return E_NOINTERFACE;
        }
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileLoader::AddRef()
    {
        return InterlockedIncrement(&m_referenceCount);
    }

    ULONG STDMETHODCALLTYPE DirectWriteFontFileLoader::Release()
    {
        ULONG newCount = InterlockedDecrement(&m_referenceCount);
        if (newCount == 0)
            delete this;
        return newCount;
    }

    HRESULT STDMETHODCALLTYPE DirectWriteFontFileLoader::CreateStreamFromKey(
        void const *fontFileReferenceKey,
        UINT32 fontFileReferenceKeySize,
        IDWriteFontFileStream **fontFileStream)
    {
        Q_UNUSED(fontFileReferenceKeySize);

        if (fontFileReferenceKeySize != sizeof(const void *)) {
            qWarning("%s: Wrong key size", __FUNCTION__);
            return E_FAIL;
        }

        const void *key = *reinterpret_cast<void * const *>(fontFileReferenceKey);
        *fontFileStream = NULL;
        auto it = m_fontDatas.constFind(key);
        if (it == m_fontDatas.constEnd())
            return E_FAIL;

        QByteArray fontData = it.value();
        DirectWriteFontFileStream *stream = new DirectWriteFontFileStream(fontData);
        stream->AddRef();
        *fontFileStream = stream;

        return S_OK;
    }

    class CustomFontFileLoader
    {
    public:
        CustomFontFileLoader() : m_directWriteFontFileLoader(nullptr)
        {
            createDirectWriteFactory(&m_directWriteFactory);

            if (m_directWriteFactory) {
                m_directWriteFontFileLoader = new DirectWriteFontFileLoader();
                m_directWriteFactory->RegisterFontFileLoader(m_directWriteFontFileLoader);
            }
        }

        ~CustomFontFileLoader()
        {
            if (m_directWriteFactory != 0 && m_directWriteFontFileLoader != 0)
                m_directWriteFactory->UnregisterFontFileLoader(m_directWriteFontFileLoader);

            if (m_directWriteFactory != 0)
                m_directWriteFactory->Release();
        }

        void addKey(const void *key, const QByteArray &fontData)
        {
            if (m_directWriteFontFileLoader != 0)
                m_directWriteFontFileLoader->addKey(key, fontData);
        }

        void removeKey(const void *key)
        {
            if (m_directWriteFontFileLoader != 0)
                m_directWriteFontFileLoader->removeKey(key);
        }

        IDWriteFontFileLoader *loader() const
        {
            return m_directWriteFontFileLoader;
        }

    private:
        IDWriteFactory *m_directWriteFactory;
        DirectWriteFontFileLoader *m_directWriteFontFileLoader;
    };

#endif

} // Anonymous namespace

/*!
    \struct QWindowsFontEngineData
    \brief Static constant data shared by the font engines.
    \ingroup qt-lighthouse-win
*/

QWindowsFontEngineData::QWindowsFontEngineData()
    : fontSmoothingGamma(QWindowsFontDatabase::fontSmoothingGamma())
{
    // from qapplication_win.cpp
    UINT result = 0;
    if (SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &result, 0))
        clearTypeEnabled = (result == FE_FONTSMOOTHINGCLEARTYPE);

    const qreal gray_gamma = 2.31;
    for (int i=0; i<256; ++i)
        pow_gamma[i] = uint(qRound(qPow(i / qreal(255.), gray_gamma) * 2047));

    HDC displayDC = GetDC(0);
    hdc = CreateCompatibleDC(displayDC);
    ReleaseDC(0, displayDC);
}

unsigned QWindowsFontDatabase::m_fontOptions = 0;

void QWindowsFontDatabase::setFontOptions(unsigned options)
{
    m_fontOptions = options & (QWindowsFontDatabase::DontUseDirectWriteFonts |
                               QWindowsFontDatabase::DontUseColorFonts);
}

unsigned QWindowsFontDatabase::fontOptions()
{
    return m_fontOptions;
}

QWindowsFontEngineData::~QWindowsFontEngineData()
{
    if (hdc)
        DeleteDC(hdc);
#if !defined(QT_NO_DIRECTWRITE)
    if (directWriteGdiInterop)
        directWriteGdiInterop->Release();
    if (directWriteFactory)
        directWriteFactory->Release();
#endif
}

qreal QWindowsFontDatabase::fontSmoothingGamma()
{
    int winSmooth;
    qreal result = 1;
    if (SystemParametersInfo(0x200C /* SPI_GETFONTSMOOTHINGCONTRAST */, 0, &winSmooth, 0))
        result = qreal(winSmooth) / qreal(1000.0);

    // Safeguard ourselves against corrupt registry values...
    if (result > 5 || result < 1)
        result = qreal(1.4);
    return result;
}

#if !defined(QT_NO_DIRECTWRITE)
static inline bool initDirectWrite(QWindowsFontEngineData *d)
{
    if (!d->directWriteFactory) {
        createDirectWriteFactory(&d->directWriteFactory);
        if (!d->directWriteFactory)
            return false;
    }
    if (!d->directWriteGdiInterop) {
        const HRESULT  hr = d->directWriteFactory->GetGdiInterop(&d->directWriteGdiInterop);
        if (FAILED(hr)) {
            qErrnoWarning("%s: GetGdiInterop failed", __FUNCTION__);
            return false;
        }
    }
    return true;
}

#endif // !defined(QT_NO_DIRECTWRITE)

/*!
    \class QWindowsFontDatabase
    \brief Font database for Windows

    \note The Qt 4.8 WIndows font database employed a mechanism of
    delayed population of the database again passing a font name
    to EnumFontFamiliesEx(), working around the fact that
    EnumFontFamiliesEx() does not list all fonts by default.
    This should be introduced to Lighthouse as well?

    \internal
    \ingroup qt-lighthouse-win
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QFontDef &def)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d << "QFontDef(Family=\"" << def.family << '"';
    if (!def.styleName.isEmpty())
        d << ", stylename=" << def.styleName;
    d << ", pointsize=" << def.pointSize << ", pixelsize=" << def.pixelSize
        << ", styleHint=" << def.styleHint << ", weight=" << def.weight
        << ", stretch=" << def.stretch << ", hintingPreference="
        << def.hintingPreference << ')';
    return d;
}

QDebug operator<<(QDebug d, const LOGFONT &lf)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d << "LOGFONT(\"" << QString::fromWCharArray(lf.lfFaceName)
        << "\", lfWidth=" << lf.lfWidth << ", lfHeight=" << lf.lfHeight << ')';
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

static inline QFontDatabase::WritingSystem writingSystemFromCharSet(uchar charSet)
{
    switch (charSet) {
    case ANSI_CHARSET:
    case EASTEUROPE_CHARSET:
    case BALTIC_CHARSET:
    case TURKISH_CHARSET:
        return QFontDatabase::Latin;
    case GREEK_CHARSET:
        return QFontDatabase::Greek;
    case RUSSIAN_CHARSET:
        return QFontDatabase::Cyrillic;
    case HEBREW_CHARSET:
        return QFontDatabase::Hebrew;
    case ARABIC_CHARSET:
        return QFontDatabase::Arabic;
    case THAI_CHARSET:
        return QFontDatabase::Thai;
    case GB2312_CHARSET:
        return QFontDatabase::SimplifiedChinese;
    case CHINESEBIG5_CHARSET:
        return QFontDatabase::TraditionalChinese;
    case SHIFTJIS_CHARSET:
        return QFontDatabase::Japanese;
    case HANGUL_CHARSET:
    case JOHAB_CHARSET:
        return QFontDatabase::Korean;
    case VIETNAMESE_CHARSET:
        return QFontDatabase::Vietnamese;
    case SYMBOL_CHARSET:
        return QFontDatabase::Symbol;
    default:
        break;
    }
    return QFontDatabase::Any;
}

#ifdef MAKE_TAG
#undef MAKE_TAG
#endif
// GetFontData expects the tags in little endian ;(
#define MAKE_TAG(ch1, ch2, ch3, ch4) (\
    (((quint32)(ch4)) << 24) | \
    (((quint32)(ch3)) << 16) | \
    (((quint32)(ch2)) << 8) | \
    ((quint32)(ch1)) \
    )

bool qt_localizedName(const QString &name)
{
    const QChar *c = name.unicode();
    for (int i = 0; i < name.length(); ++i) {
        if (c[i].unicode() >= 0x100)
            return true;
    }
    return false;
}

namespace {

static QString readName(bool unicode, const uchar *string, int length)
{
    QString out;
    if (unicode) {
        // utf16

        length /= 2;
        out.resize(length);
        QChar *uc = out.data();
        for (int i = 0; i < length; ++i)
            uc[i] = qt_getUShort(string + 2*i);
    } else {
        // Apple Roman

        out.resize(length);
        QChar *uc = out.data();
        for (int i = 0; i < length; ++i)
            uc[i] = QLatin1Char(char(string[i]));
    }
    return out;
}

enum FieldTypeValue {
    FamilyId = 1,
    StyleId = 2,
    PreferredFamilyId = 16,
    PreferredStyleId = 17,
};

enum PlatformFieldValue {
    PlatformId_Unicode = 0,
    PlatformId_Apple = 1,
    PlatformId_Microsoft = 3
};

QFontNames qt_getCanonicalFontNames(const uchar *table, quint32 bytes)
{
    QFontNames out;
    const int NameRecordSize = 12;
    const int MS_LangIdEnglish = 0x009;

    // get the name table
    quint16 count;
    quint16 string_offset;
    const unsigned char *names;

    if (bytes < 8)
        return out;

    if (qt_getUShort(table) != 0)
        return out;

    count = qt_getUShort(table + 2);
    string_offset = qt_getUShort(table + 4);
    names = table + 6;

    if (string_offset >= bytes || 6 + count*NameRecordSize > string_offset)
        return out;

    enum PlatformIdType {
        NotFound = 0,
        Unicode = 1,
        Apple = 2,
        Microsoft = 3
    };

    PlatformIdType idStatus[4] = { NotFound, NotFound, NotFound, NotFound };
    int ids[4] = { -1, -1, -1, -1 };

    for (int i = 0; i < count; ++i) {
        // search for the correct name entries

        quint16 platform_id = qt_getUShort(names + i*NameRecordSize);
        quint16 encoding_id = qt_getUShort(names + 2 + i*NameRecordSize);
        quint16 language_id = qt_getUShort(names + 4 + i*NameRecordSize);
        quint16 name_id = qt_getUShort(names + 6 + i*NameRecordSize);

        PlatformIdType *idType = nullptr;
        int *id = nullptr;

        switch (name_id) {
        case FamilyId:
            idType = &idStatus[0];
            id = &ids[0];
            break;
        case StyleId:
            idType = &idStatus[1];
            id = &ids[1];
            break;
        case PreferredFamilyId:
            idType = &idStatus[2];
            id = &ids[2];
            break;
        case PreferredStyleId:
            idType = &idStatus[3];
            id = &ids[3];
            break;
        default:
            continue;
        }

        quint16 length = qt_getUShort(names + 8 + i*NameRecordSize);
        quint16 offset = qt_getUShort(names + 10 + i*NameRecordSize);
        if (DWORD(string_offset + offset + length) > bytes)
            continue;

        if ((platform_id == PlatformId_Microsoft
            && (encoding_id == 0 || encoding_id == 1))
            && ((language_id & 0x3ff) == MS_LangIdEnglish
                || *idType < Microsoft)) {
            *id = i;
            *idType = Microsoft;
        }
        // not sure if encoding id 4 for Unicode is utf16 or ucs4...
        else if (platform_id == PlatformId_Unicode && encoding_id < 4 && *idType < Unicode) {
            *id = i;
            *idType = Unicode;
        }
        else if (platform_id == PlatformId_Apple && encoding_id == 0 && language_id == 0 && *idType < Apple) {
            *id = i;
            *idType = Apple;
        }
    }

    QString strings[4];
    for (int i = 0; i < 4; ++i) {
        if (idStatus[i] == NotFound)
            continue;
        int id = ids[i];
        quint16 length = qt_getUShort(names +  8 + id * NameRecordSize);
        quint16 offset = qt_getUShort(names + 10 + id * NameRecordSize);
        const unsigned char *string = table + string_offset + offset;
        strings[i] = readName(idStatus[i] != Apple, string, length);
    }

    out.name = strings[0];
    out.style = strings[1];
    out.preferredName = strings[2];
    out.preferredStyle = strings[3];
    return out;
}

} // namespace

QString qt_getEnglishName(const QString &familyName, bool includeStyle)
{
    QString i18n_name;
    QString faceName = familyName;
    faceName.truncate(LF_FACESIZE - 1);

    HDC hdc = GetDC( 0 );
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));
    faceName.toWCharArray(lf.lfFaceName);
    lf.lfFaceName[faceName.size()] = 0;
    lf.lfCharSet = DEFAULT_CHARSET;
    HFONT hfont = CreateFontIndirect(&lf);

    if (!hfont) {
        ReleaseDC(0, hdc);
        return QString();
    }

    HGDIOBJ oldobj = SelectObject( hdc, hfont );

    const DWORD name_tag = MAKE_TAG( 'n', 'a', 'm', 'e' );

    // get the name table
    unsigned char *table = 0;

    DWORD bytes = GetFontData( hdc, name_tag, 0, 0, 0 );
    if ( bytes == GDI_ERROR ) {
        // ### Unused variable
        // int err = GetLastError();
        goto error;
    }

    table = new unsigned char[bytes];
    GetFontData(hdc, name_tag, 0, table, bytes);
    if ( bytes == GDI_ERROR )
        goto error;

    {
        const QFontNames names = qt_getCanonicalFontNames(table, bytes);
        i18n_name = names.name;
        if (includeStyle)
            i18n_name += QLatin1Char(' ') + names.style;
    }
error:
    delete [] table;
    SelectObject( hdc, oldobj );
    DeleteObject( hfont );
    ReleaseDC( 0, hdc );

    //qDebug("got i18n name of '%s' for font '%s'", i18n_name.latin1(), familyName.toLocal8Bit().data());
    return i18n_name;
}

// Note this duplicates parts of qt_getEnglishName, we should try to unify the two functions.
QFontNames qt_getCanonicalFontNames(const LOGFONT &lf)
{
    QFontNames fontNames;
    HDC hdc = GetDC(0);
    HFONT hfont = CreateFontIndirect(&lf);

    if (!hfont) {
        ReleaseDC(0, hdc);
        return fontNames;
    }

    HGDIOBJ oldobj = SelectObject(hdc, hfont);

    // get the name table
    QByteArray table;
    const DWORD name_tag = MAKE_TAG('n', 'a', 'm', 'e');
    DWORD bytes = GetFontData(hdc, name_tag, 0, 0, 0);
    if (bytes != GDI_ERROR) {
        table.resize(bytes);

        if (GetFontData(hdc, name_tag, 0, table.data(), bytes) != GDI_ERROR)
            fontNames = qt_getCanonicalFontNames(reinterpret_cast<const uchar*>(table.constData()), bytes);
    }

    SelectObject(hdc, oldobj);
    DeleteObject(hfont);
    ReleaseDC(0, hdc);

    return fontNames;
}

static QChar *createFontFile(const QString &faceName)
{
    QChar *faceNamePtr = nullptr;
    if (!faceName.isEmpty()) {
        const int nameLength = qMin(faceName.length(), LF_FACESIZE - 1);
        faceNamePtr = new QChar[nameLength + 1];
        memcpy(static_cast<void *>(faceNamePtr), faceName.utf16(), sizeof(wchar_t) * nameLength);
        faceNamePtr[nameLength] = 0;
    }
    return faceNamePtr;
}

static bool addFontToDatabase(QString familyName,
                              QString styleName,
                              const LOGFONT &logFont,
                              const TEXTMETRIC *textmetric,
                              const FONTSIGNATURE *signature,
                              int type)
{
    // the "@family" fonts are just the same as "family". Ignore them.
    if (familyName.isEmpty() || familyName.at(0) == QLatin1Char('@') || familyName.startsWith(QLatin1String("WST_")))
        return false;

    uchar charSet = logFont.lfCharSet;

    static const int SMOOTH_SCALABLE = 0xffff;
    const QString foundryName; // No such concept.
    const bool fixed = !(textmetric->tmPitchAndFamily & TMPF_FIXED_PITCH);
    const bool ttf = (textmetric->tmPitchAndFamily & TMPF_TRUETYPE);
    const bool scalable = textmetric->tmPitchAndFamily & (TMPF_VECTOR|TMPF_TRUETYPE);
    const int size = scalable ? SMOOTH_SCALABLE : textmetric->tmHeight;
    const QFont::Style style = textmetric->tmItalic ? QFont::StyleItalic : QFont::StyleNormal;
    const bool antialias = false;
    const QFont::Weight weight = QPlatformFontDatabase::weightFromInteger(textmetric->tmWeight);
    const QFont::Stretch stretch = QFont::Unstretched;

#ifndef QT_NO_DEBUG_OUTPUT
    if (lcQpaFonts().isDebugEnabled()) {
        QString message;
        QTextStream str(&message);
        str << __FUNCTION__ << ' ' << familyName << ' ' << charSet << " TTF=" << ttf;
        if (type & DEVICE_FONTTYPE)
            str << " DEVICE";
        if (type & RASTER_FONTTYPE)
            str << " RASTER";
        if (type & TRUETYPE_FONTTYPE)
            str << " TRUETYPE";
        str << " scalable=" << scalable << " Size=" << size
                << " Style=" << style << " Weight=" << weight
                << " stretch=" << stretch;
        qCDebug(lcQpaFonts) << message;
    }
#endif
    QString englishName;
    QString faceName;

    QString subFamilyName;
    QString subFamilyStyle;
    if (ttf) {
        // Look-up names registered in the font
        QFontNames canonicalNames = qt_getCanonicalFontNames(logFont);
        if (qt_localizedName(familyName) && !canonicalNames.name.isEmpty())
            englishName = canonicalNames.name;
        if (!canonicalNames.preferredName.isEmpty()) {
            subFamilyName = familyName;
            subFamilyStyle = styleName;
            faceName = familyName; // Remember the original name for later lookups
            familyName = canonicalNames.preferredName;
            styleName = canonicalNames.preferredStyle;
        }
    }

    QSupportedWritingSystems writingSystems;
    if (type & TRUETYPE_FONTTYPE) {
        Q_ASSERT(signature);
        quint32 unicodeRange[4] = {
            signature->fsUsb[0], signature->fsUsb[1],
            signature->fsUsb[2], signature->fsUsb[3]
        };
        quint32 codePageRange[2] = {
            signature->fsCsb[0], signature->fsCsb[1]
        };
        writingSystems = QPlatformFontDatabase::writingSystemsFromTrueTypeBits(unicodeRange, codePageRange);
        // ### Hack to work around problem with Thai text on Windows 7. Segoe UI contains
        // the symbol for Baht, and Windows thus reports that it supports the Thai script.
        // Since it's the default UI font on this platform, most widgets will be unable to
        // display Thai text by default. As a temporary work around, we special case Segoe UI
        // and remove the Thai script from its list of supported writing systems.
        if (writingSystems.supported(QFontDatabase::Thai) &&
                familyName == QLatin1String("Segoe UI"))
            writingSystems.setSupported(QFontDatabase::Thai, false);
    } else {
        const QFontDatabase::WritingSystem ws = writingSystemFromCharSet(charSet);
        if (ws != QFontDatabase::Any)
            writingSystems.setSupported(ws);
    }

    QPlatformFontDatabase::registerFont(familyName, styleName, foundryName, weight,
                                        style, stretch, antialias, scalable, size, fixed, writingSystems, createFontFile(faceName));

    // add fonts windows can generate for us:
    if (weight <= QFont::DemiBold && styleName.isEmpty())
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, QFont::Bold,
                                            style, stretch, antialias, scalable, size, fixed, writingSystems, createFontFile(faceName));
    if (style != QFont::StyleItalic && styleName.isEmpty())
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, weight,
                                            QFont::StyleItalic, stretch, antialias, scalable, size, fixed, writingSystems, createFontFile(faceName));
    if (weight <= QFont::DemiBold && style != QFont::StyleItalic && styleName.isEmpty())
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, QFont::Bold,
                                            QFont::StyleItalic, stretch, antialias, scalable, size, fixed, writingSystems, createFontFile(faceName));

    if (!subFamilyName.isEmpty() && familyName != subFamilyName) {
        QPlatformFontDatabase::registerFont(subFamilyName, subFamilyStyle, foundryName, weight,
                                            style, stretch, antialias, scalable, size, fixed, writingSystems, createFontFile(faceName));
    }

    if (!englishName.isEmpty() && englishName != familyName)
        QPlatformFontDatabase::registerAliasToFontFamily(familyName, englishName);

    return true;
}

static int QT_WIN_CALLBACK storeFont(const LOGFONT *logFont, const TEXTMETRIC *textmetric,
                                     DWORD type, LPARAM)
{
    const ENUMLOGFONTEX *f = reinterpret_cast<const ENUMLOGFONTEX *>(logFont);
    const QString familyName = QString::fromWCharArray(f->elfLogFont.lfFaceName);
    const QString styleName = QString::fromWCharArray(f->elfStyle);

    // NEWTEXTMETRICEX (passed for TT fonts) is a NEWTEXTMETRIC, which according
    // to the documentation is identical to a TEXTMETRIC except for the last four
    // members, which we don't use anyway
    const FONTSIGNATURE *signature = nullptr;
    if (type & TRUETYPE_FONTTYPE)
        signature = &reinterpret_cast<const NEWTEXTMETRICEX *>(textmetric)->ntmFontSig;
    addFontToDatabase(familyName, styleName, *logFont, textmetric, signature, type);

    // keep on enumerating
    return 1;
}

void QWindowsFontDatabase::populateFamily(const QString &familyName)
{
    qCDebug(lcQpaFonts) << familyName;
    if (familyName.size() >= LF_FACESIZE) {
        qCWarning(lcQpaFonts) << "Unable to enumerate family '" << familyName << '\'';
        return;
    }
    HDC dummy = GetDC(0);
    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET;
    familyName.toWCharArray(lf.lfFaceName);
    lf.lfFaceName[familyName.size()] = 0;
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(dummy, &lf, storeFont, 0, 0);
    ReleaseDC(0, dummy);
}

static int QT_WIN_CALLBACK populateFontFamilies(const LOGFONT *logFont, const TEXTMETRIC *textmetric,
                                                DWORD, LPARAM)
{
    // the "@family" fonts are just the same as "family". Ignore them.
    const ENUMLOGFONTEX *f = reinterpret_cast<const ENUMLOGFONTEX *>(logFont);
    const wchar_t *faceNameW = f->elfLogFont.lfFaceName;
    if (faceNameW[0] && faceNameW[0] != L'@' && wcsncmp(faceNameW, L"WST_", 4)) {
        const QString faceName = QString::fromWCharArray(faceNameW);
        QPlatformFontDatabase::registerFontFamily(faceName);
        // Register current font's english name as alias
        const bool ttf = (textmetric->tmPitchAndFamily & TMPF_TRUETYPE);
        if (ttf && qt_localizedName(faceName)) {
            const QString englishName = qt_getEnglishName(faceName);
            if (!englishName.isEmpty())
                QPlatformFontDatabase::registerAliasToFontFamily(faceName, englishName);
        }
    }
    return 1; // continue
}

void QWindowsFontDatabase::addDefaultEUDCFont()
{
    QString path;
    {
        HKEY key;
        if (RegOpenKeyEx(HKEY_CURRENT_USER,
                         L"EUDC\\1252",
                         0,
                         KEY_READ,
                         &key) != ERROR_SUCCESS) {
            return;
        }

        WCHAR value[MAX_PATH];
        DWORD bufferSize = sizeof(value);
        ZeroMemory(value, bufferSize);

        if (RegQueryValueEx(key,
                            L"SystemDefaultEUDCFont",
                            nullptr,
                            nullptr,
                            reinterpret_cast<LPBYTE>(value),
                            &bufferSize) == ERROR_SUCCESS) {
            path = QString::fromWCharArray(value);
        }

        RegCloseKey(key);
    }

    if (!path.isEmpty()) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qCWarning(lcQpaFonts) << "Unable to open default EUDC font:" << path;
            return;
        }

        m_eudcFonts = addApplicationFont(file.readAll(), path);
    }
}

void QWindowsFontDatabase::populateFontDatabase()
{
    removeApplicationFonts();
    HDC dummy = GetDC(0);
    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfFaceName[0] = 0;
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(dummy, &lf, populateFontFamilies, 0, 0);
    ReleaseDC(0, dummy);
    // Work around EnumFontFamiliesEx() not listing the system font.
    QString systemDefaultFamily = QWindowsFontDatabase::systemDefaultFont().family();
    if (QPlatformFontDatabase::resolveFontFamilyAlias(systemDefaultFamily) == systemDefaultFamily)
        QPlatformFontDatabase::registerFontFamily(systemDefaultFamily);
    addDefaultEUDCFont();
}

typedef QSharedPointer<QWindowsFontEngineData> QWindowsFontEngineDataPtr;

typedef QThreadStorage<QWindowsFontEngineDataPtr> FontEngineThreadLocalData;

Q_GLOBAL_STATIC(FontEngineThreadLocalData, fontEngineThreadLocalData)

QSharedPointer<QWindowsFontEngineData> sharedFontData()
{
    FontEngineThreadLocalData *data = fontEngineThreadLocalData();
    if (!data->hasLocalData())
        data->setLocalData(QSharedPointer<QWindowsFontEngineData>::create());
    return data->localData();
}

QWindowsFontDatabase::QWindowsFontDatabase()
{
    // Properties accessed by QWin32PrintEngine (Qt Print Support)
    static const int hfontMetaTypeId = qRegisterMetaType<HFONT>();
    static const int logFontMetaTypeId = qRegisterMetaType<LOGFONT>();
    Q_UNUSED(hfontMetaTypeId)
    Q_UNUSED(logFontMetaTypeId)

    if (lcQpaFonts().isDebugEnabled()) {
        const QWindowsFontEngineDataPtr data = sharedFontData();
        qCDebug(lcQpaFonts) << __FUNCTION__ << "Clear type: "
            << data->clearTypeEnabled << "gamma: " << data->fontSmoothingGamma;
    }
}

QWindowsFontDatabase::~QWindowsFontDatabase()
{
    removeApplicationFonts();
}

QFontEngineMulti *QWindowsFontDatabase::fontEngineMulti(QFontEngine *fontEngine, QChar::Script script)
{
    return new QWindowsMultiFontEngine(fontEngine, script);
}

QFontEngine * QWindowsFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    const QString faceName(static_cast<const QChar*>(handle));
    QFontEngine *fe = QWindowsFontDatabase::createEngine(fontDef, faceName,
                                                         defaultVerticalDPI(),
                                                         sharedFontData());
    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDEF" << fontDef << fe << handle;
    return fe;
}

QFontEngine *QWindowsFontDatabase::fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
    EmbeddedFont font(fontData);
    QFontEngine *fontEngine = 0;

#if !defined(QT_NO_DIRECTWRITE)
    if (!useDirectWrite(hintingPreference))
#endif
    {
        GUID guid;
        CoCreateGuid(&guid);

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wstrict-aliasing")
        QString uniqueFamilyName = QLatin1Char('f')
                + QString::number(guid.Data1, 36) + QLatin1Char('-')
                + QString::number(guid.Data2, 36) + QLatin1Char('-')
                + QString::number(guid.Data3, 36) + QLatin1Char('-')
                + QString::number(*reinterpret_cast<quint64 *>(guid.Data4), 36);
QT_WARNING_POP

        QString actualFontName = font.changeFamilyName(uniqueFamilyName);
        if (actualFontName.isEmpty()) {
            qWarning("%s: Can't change family name of font", __FUNCTION__);
            return 0;
        }

        DWORD count = 0;
        QByteArray newFontData = font.data();
        HANDLE fontHandle =
            AddFontMemResourceEx(const_cast<char *>(newFontData.constData()),
                                 DWORD(newFontData.size()), 0, &count);
        if (count == 0 && fontHandle != 0) {
            RemoveFontMemResourceEx(fontHandle);
            fontHandle = 0;
        }

        if (fontHandle == 0) {
            qWarning("%s: AddFontMemResourceEx failed", __FUNCTION__);
        } else {
            QFontDef request;
            request.family = uniqueFamilyName;
            request.pixelSize = pixelSize;
            request.styleStrategy = QFont::PreferMatch;
            request.hintingPreference = hintingPreference;
            request.stretch = QFont::Unstretched;

            fontEngine = QWindowsFontDatabase::createEngine(request, QString(),
                                                            defaultVerticalDPI(),
                                                            sharedFontData());

            if (fontEngine) {
                if (request.family != fontEngine->fontDef.family) {
                    qWarning("%s: Failed to load font. Got fallback instead: %s",
                             __FUNCTION__, qPrintable(fontEngine->fontDef.family));
                    if (fontEngine->ref.load() == 0)
                        delete fontEngine;
                    fontEngine = 0;
                } else {
                    Q_ASSERT(fontEngine->ref.load() == 0);

                    // Override the generated font name
                    switch (fontEngine->type()) {
                    case QFontEngine::Win:
                        static_cast<QWindowsFontEngine *>(fontEngine)->setUniqueFamilyName(uniqueFamilyName);
                        fontEngine->fontDef.family = actualFontName;
                        break;

#if !defined(QT_NO_DIRECTWRITE)
                    case QFontEngine::DirectWrite:
                        static_cast<QWindowsFontEngineDirectWrite *>(fontEngine)->setUniqueFamilyName(uniqueFamilyName);
                        fontEngine->fontDef.family = actualFontName;
                        break;
#endif // !QT_NO_DIRECTWRITE

                    default:
                        Q_ASSERT_X(false, Q_FUNC_INFO, "Unhandled font engine.");
                    }

                    UniqueFontData uniqueData;
                    uniqueData.handle = fontHandle;
                    uniqueData.refCount.ref();
                    m_uniqueFontData[uniqueFamilyName] = uniqueData;
                }
            } else {
                RemoveFontMemResourceEx(fontHandle);
            }
        }
    }
#if !defined(QT_NO_DIRECTWRITE)
    else {
        CustomFontFileLoader fontFileLoader;
        fontFileLoader.addKey(this, fontData);

        QSharedPointer<QWindowsFontEngineData> fontEngineData = sharedFontData();
        if (!initDirectWrite(fontEngineData.data()))
            return 0;

        IDWriteFontFile *fontFile = 0;
        void *key = this;

        HRESULT hres = fontEngineData->directWriteFactory->CreateCustomFontFileReference(&key,
                                                                                         sizeof(void *),
                                                                                         fontFileLoader.loader(),
                                                                                         &fontFile);
        if (FAILED(hres)) {
            qErrnoWarning(hres, "%s: CreateCustomFontFileReference failed", __FUNCTION__);
            return 0;
        }

        BOOL isSupportedFontType;
        DWRITE_FONT_FILE_TYPE fontFileType;
        DWRITE_FONT_FACE_TYPE fontFaceType;
        UINT32 numberOfFaces;
        fontFile->Analyze(&isSupportedFontType, &fontFileType, &fontFaceType, &numberOfFaces);
        if (!isSupportedFontType) {
            fontFile->Release();
            return 0;
        }

        IDWriteFontFace *directWriteFontFace = 0;
        hres = fontEngineData->directWriteFactory->CreateFontFace(fontFaceType,
                                                                  1,
                                                                  &fontFile,
                                                                  0,
                                                                  DWRITE_FONT_SIMULATIONS_NONE,
                                                                  &directWriteFontFace);
        if (FAILED(hres)) {
            qErrnoWarning(hres, "%s: CreateFontFace failed", __FUNCTION__);
            fontFile->Release();
            return 0;
        }

        fontFile->Release();

        fontEngine = new QWindowsFontEngineDirectWrite(directWriteFontFace,
                                                       pixelSize,
                                                       fontEngineData);

        // Get font family from font data
        fontEngine->fontDef.family = font.familyName();
        fontEngine->fontDef.hintingPreference = hintingPreference;

        directWriteFontFace->Release();
    }
#endif

    // Get style and weight info
    if (fontEngine != 0) {
        TableDirectory *os2TableEntry = font.tableDirectoryEntry("OS/2");
        if (os2TableEntry != 0) {
            const OS2Table *os2Table =
                    reinterpret_cast<const OS2Table *>(fontData.constData()
                                                       + qFromBigEndian<quint32>(os2TableEntry->offset));

            bool italic = qFromBigEndian<quint16>(os2Table->selection) & 1;
            bool oblique = qFromBigEndian<quint16>(os2Table->selection) & 128;

            if (italic)
                fontEngine->fontDef.style = QFont::StyleItalic;
            else if (oblique)
                fontEngine->fontDef.style = QFont::StyleOblique;
            else
                fontEngine->fontDef.style = QFont::StyleNormal;

            fontEngine->fontDef.weight = QPlatformFontDatabase::weightFromInteger(qFromBigEndian<quint16>(os2Table->weightClass));
        }
    }

    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDATA" << fontData << pixelSize << hintingPreference << fontEngine;
    return fontEngine;
}

static QList<quint32> getTrueTypeFontOffsets(const uchar *fontData)
{
    QList<quint32> offsets;
    const quint32 headerTag = *reinterpret_cast<const quint32 *>(fontData);
    if (headerTag != MAKE_TAG('t', 't', 'c', 'f')) {
        if (headerTag != MAKE_TAG(0, 1, 0, 0)
            && headerTag != MAKE_TAG('O', 'T', 'T', 'O')
            && headerTag != MAKE_TAG('t', 'r', 'u', 'e')
            && headerTag != MAKE_TAG('t', 'y', 'p', '1'))
            return offsets;
        offsets << 0;
        return offsets;
    }
    const quint32 numFonts = qFromBigEndian<quint32>(fontData + 8);
    for (uint i = 0; i < numFonts; ++i) {
        offsets << qFromBigEndian<quint32>(fontData + 12 + i * 4);
    }
    return offsets;
}

static void getFontTable(const uchar *fileBegin, const uchar *data, quint32 tag, const uchar **table, quint32 *length)
{
    const quint16 numTables = qFromBigEndian<quint16>(data + 4);
    for (uint i = 0; i < numTables; ++i) {
        const quint32 offset = 12 + 16 * i;
        if (*reinterpret_cast<const quint32 *>(data + offset) == tag) {
            *table = fileBegin + qFromBigEndian<quint32>(data + offset + 8);
            *length = qFromBigEndian<quint32>(data + offset + 12);
            return;
        }
    }
    *table = 0;
    *length = 0;
    return;
}

static void getFamiliesAndSignatures(const QByteArray &fontData,
                                     QList<QFontNames> *families,
                                     QVector<FONTSIGNATURE> *signatures)
{
    const uchar *data = reinterpret_cast<const uchar *>(fontData.constData());

    QList<quint32> offsets = getTrueTypeFontOffsets(data);
    if (offsets.isEmpty())
        return;

    for (int i = 0; i < offsets.count(); ++i) {
        const uchar *font = data + offsets.at(i);
        const uchar *table;
        quint32 length;
        getFontTable(data, font, MAKE_TAG('n', 'a', 'm', 'e'), &table, &length);
        if (!table)
            continue;
        QFontNames names = qt_getCanonicalFontNames(table, length);
        if (names.name.isEmpty())
            continue;

        families->append(qMove(names));

        if (signatures) {
            FONTSIGNATURE signature;
            getFontTable(data, font, MAKE_TAG('O', 'S', '/', '2'), &table, &length);
            if (table && length >= 86) {
                // Offsets taken from OS/2 table in the TrueType spec
                signature.fsUsb[0] = qFromBigEndian<quint32>(table + 42);
                signature.fsUsb[1] = qFromBigEndian<quint32>(table + 46);
                signature.fsUsb[2] = qFromBigEndian<quint32>(table + 50);
                signature.fsUsb[3] = qFromBigEndian<quint32>(table + 54);

                signature.fsCsb[0] = qFromBigEndian<quint32>(table + 78);
                signature.fsCsb[1] = qFromBigEndian<quint32>(table + 82);
            } else {
                memset(&signature, 0, sizeof(signature));
            }
            signatures->append(signature);
        }
    }
}

QStringList QWindowsFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName)
{
    WinApplicationFont font;
    font.fileName = fileName;
    QVector<FONTSIGNATURE> signatures;
    QList<QFontNames> families;
    QStringList familyNames;

    if (!fontData.isEmpty()) {
        getFamiliesAndSignatures(fontData, &families, &signatures);
        if (families.isEmpty())
            return familyNames;

        DWORD dummy = 0;
        font.handle =
            AddFontMemResourceEx(const_cast<char *>(fontData.constData()),
                                 DWORD(fontData.size()), 0, &dummy);
        if (font.handle == 0)
            return QStringList();

        // Memory fonts won't show up in enumeration, so do add them the hard way.
        for (int j = 0; j < families.count(); ++j) {
            const QString familyName = families.at(j).name;
            const QString styleName = families.at(j).style;
            familyNames << familyName;
            HDC hdc = GetDC(0);
            LOGFONT lf;
            memset(&lf, 0, sizeof(LOGFONT));
            memcpy(lf.lfFaceName, familyName.utf16(), sizeof(wchar_t) * qMin(LF_FACESIZE - 1, familyName.size()));
            lf.lfCharSet = DEFAULT_CHARSET;
            HFONT hfont = CreateFontIndirect(&lf);
            HGDIOBJ oldobj = SelectObject(hdc, hfont);

            TEXTMETRIC textMetrics;
            GetTextMetrics(hdc, &textMetrics);

            addFontToDatabase(familyName, styleName, lf, &textMetrics, &signatures.at(j),
                              TRUETYPE_FONTTYPE);

            SelectObject(hdc, oldobj);
            DeleteObject(hfont);
            ReleaseDC(0, hdc);
        }
    } else {
        QFile f(fileName);
        if (!f.open(QIODevice::ReadOnly))
            return QStringList();
        QByteArray data = f.readAll();
        f.close();

        getFamiliesAndSignatures(data, &families, 0);
        if (families.isEmpty())
            return QStringList();

        if (AddFontResourceExW((wchar_t*)fileName.utf16(), FR_PRIVATE, 0) == 0)
            return QStringList();

        font.handle = 0;

        // Fonts based on files are added via populate, as they will show up in font enumeration.
        for (int j = 0; j < families.count(); ++j) {
            const QString familyName = families.at(j).name;
            familyNames << familyName;
            populateFamily(familyName);
        }
    }

    m_applicationFonts << font;

    return familyNames;
}

void QWindowsFontDatabase::removeApplicationFonts()
{
    for (const WinApplicationFont &font : qAsConst(m_applicationFonts)) {
        if (font.handle) {
            RemoveFontMemResourceEx(font.handle);
        } else {
            RemoveFontResourceExW(reinterpret_cast<LPCWSTR>(font.fileName.utf16()),
                                  FR_PRIVATE, nullptr);
        }
    }
    m_applicationFonts.clear();
    m_eudcFonts.clear();
}

void QWindowsFontDatabase::releaseHandle(void *handle)
{
    const QChar *faceName = reinterpret_cast<const QChar *>(handle);
    delete[] faceName;
}

QString QWindowsFontDatabase::fontDir() const
{
    const QString result = QPlatformFontDatabase::fontDir();
    qCDebug(lcQpaFonts) << __FUNCTION__ << result;
    return result;
}

bool QWindowsFontDatabase::fontsAlwaysScalable() const
{
    return false;
}

void QWindowsFontDatabase::derefUniqueFont(const QString &uniqueFont)
{
    if (m_uniqueFontData.contains(uniqueFont)) {
        if (!m_uniqueFontData[uniqueFont].refCount.deref()) {
            RemoveFontMemResourceEx(m_uniqueFontData[uniqueFont].handle);
            m_uniqueFontData.remove(uniqueFont);
        }
    }
}

void QWindowsFontDatabase::refUniqueFont(const QString &uniqueFont)
{
    if (m_uniqueFontData.contains(uniqueFont))
        m_uniqueFontData[uniqueFont].refCount.ref();
}

// ### fixme Qt 6 (QTBUG-58610): See comment at QWindowsFontDatabase::systemDefaultFont()
HFONT QWindowsFontDatabase::systemFont()
{
    static const auto stock_sysfont =
        reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    return stock_sysfont;
}

// Creation functions

static const char *other_tryFonts[] = {
    "Arial",
    "MS UI Gothic",
    "Gulim",
    "SimSun",
    "PMingLiU",
    "Arial Unicode MS",
    0
};

static const char *jp_tryFonts [] = {
    "MS UI Gothic",
    "Arial",
    "Gulim",
    "SimSun",
    "PMingLiU",
    "Arial Unicode MS",
    0
};

static const char *ch_CN_tryFonts [] = {
    "SimSun",
    "Arial",
    "PMingLiU",
    "Gulim",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char *ch_TW_tryFonts [] = {
    "PMingLiU",
    "Arial",
    "SimSun",
    "Gulim",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char *kr_tryFonts[] = {
    "Gulim",
    "Arial",
    "PMingLiU",
    "SimSun",
    "MS UI Gothic",
    "Arial Unicode MS",
    0
};

static const char **tryFonts = 0;

LOGFONT QWindowsFontDatabase::fontDefToLOGFONT(const QFontDef &request, const QString &faceName)
{
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));

    lf.lfHeight = -qRound(request.pixelSize);
    lf.lfWidth                = 0;
    lf.lfEscapement        = 0;
    lf.lfOrientation        = 0;
    if (request.weight == 50)
        lf.lfWeight = FW_DONTCARE;
    else
        lf.lfWeight = (request.weight*900)/99;
    lf.lfItalic         = request.style != QFont::StyleNormal;
    lf.lfCharSet        = DEFAULT_CHARSET;

    int strat = OUT_DEFAULT_PRECIS;
    if (request.styleStrategy & QFont::PreferBitmap) {
        strat = OUT_RASTER_PRECIS;
    } else if (request.styleStrategy & QFont::PreferDevice) {
        strat = OUT_DEVICE_PRECIS;
    } else if (request.styleStrategy & QFont::PreferOutline) {
        strat = OUT_OUTLINE_PRECIS;
    } else if (request.styleStrategy & QFont::ForceOutline) {
        strat = OUT_TT_ONLY_PRECIS;
    }

    lf.lfOutPrecision   = strat;

    int qual = DEFAULT_QUALITY;

    if (request.styleStrategy & QFont::PreferMatch)
        qual = DRAFT_QUALITY;
    else if (request.styleStrategy & QFont::PreferQuality)
        qual = PROOF_QUALITY;

    if (request.styleStrategy & QFont::PreferAntialias) {
        qual = (request.styleStrategy & QFont::NoSubpixelAntialias) == 0
            ? CLEARTYPE_QUALITY : ANTIALIASED_QUALITY;
    } else if (request.styleStrategy & QFont::NoAntialias) {
        qual = NONANTIALIASED_QUALITY;
    } else if ((request.styleStrategy & QFont::NoSubpixelAntialias) && sharedFontData()->clearTypeEnabled) {
        qual = ANTIALIASED_QUALITY;
    }

    lf.lfQuality        = qual;

    lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;

    int hint = FF_DONTCARE;
    switch (request.styleHint) {
        case QFont::Helvetica:
            hint = FF_SWISS;
            break;
        case QFont::Times:
            hint = FF_ROMAN;
            break;
        case QFont::Courier:
            hint = FF_MODERN;
            break;
        case QFont::OldEnglish:
            hint = FF_DECORATIVE;
            break;
        case QFont::System:
            hint = FF_MODERN;
            break;
        default:
            break;
    }

    lf.lfPitchAndFamily = DEFAULT_PITCH | hint;

    QString fam = faceName;
    if (fam.isEmpty())
        fam = request.family;
    if (Q_UNLIKELY(fam.size() >= LF_FACESIZE)) {
        qCritical("%s: Family name '%s' is too long.", __FUNCTION__, qPrintable(fam));
        fam.truncate(LF_FACESIZE - 1);
    }

    if (fam.isEmpty())
        fam = QStringLiteral("MS Sans Serif");

    if (fam == QLatin1String("MS Sans Serif")
        && (request.style == QFont::StyleItalic || (-lf.lfHeight > 18 && -lf.lfHeight != 24))) {
        fam = QStringLiteral("Arial"); // MS Sans Serif has bearing problems in italic, and does not scale
    }
    if (fam == QLatin1String("Courier") && !(request.styleStrategy & QFont::PreferBitmap))
        fam = QStringLiteral("Courier New");

    memcpy(lf.lfFaceName, fam.utf16(), fam.size() * sizeof(wchar_t));

    return lf;
}

QStringList QWindowsFontDatabase::extraTryFontsForFamily(const QString &family)
{
    QStringList result;
    QFontDatabase db;
    if (!db.writingSystems(family).contains(QFontDatabase::Symbol)) {
        if (!tryFonts) {
            LANGID lid = GetUserDefaultLangID();
            switch (lid&0xff) {
            case LANG_CHINESE: // Chinese
                if ( lid == 0x0804 || lid == 0x1004) // China mainland and Singapore
                    tryFonts = ch_CN_tryFonts;
                else
                    tryFonts = ch_TW_tryFonts; // Taiwan, Hong Kong and Macau
                break;
            case LANG_JAPANESE:
                tryFonts = jp_tryFonts;
                break;
            case LANG_KOREAN:
                tryFonts = kr_tryFonts;
                break;
            default:
                tryFonts = other_tryFonts;
                break;
            }
        }
        QFontDatabase db;
        const QStringList families = db.families();
        const char **tf = tryFonts;
        while (tf && *tf) {
            // QTBUG-31689, family might be an English alias for a localized font name.
            const QString family = QString::fromLatin1(*tf);
            if (families.contains(family) || db.hasFamily(family))
                result << family;
            ++tf;
        }
    }
    result.append(QStringLiteral("Segoe UI Emoji"));
    result.append(QStringLiteral("Segoe UI Symbol"));
    return result;
}

QString QWindowsFontDatabase::familyForStyleHint(QFont::StyleHint styleHint)
{
    switch (styleHint) {
    case QFont::Times:
        return QStringLiteral("Times New Roman");
    case QFont::Courier:
        return QStringLiteral("Courier New");
    case QFont::Monospace:
        return QStringLiteral("Courier New");
    case QFont::Cursive:
        return QStringLiteral("Comic Sans MS");
    case QFont::Fantasy:
        return QStringLiteral("Impact");
    case QFont::Decorative:
        return QStringLiteral("Old English");
    case QFont::Helvetica:
        return QStringLiteral("Arial");
    case QFont::System:
    default:
        break;
    }
    return QStringLiteral("MS Shell Dlg 2");
}

QStringList QWindowsFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    QStringList result;
    result.append(QWindowsFontDatabase::familyForStyleHint(styleHint));
    result.append(m_eudcFonts);
    result.append(QWindowsFontDatabase::extraTryFontsForFamily(family));
    result.append(QPlatformFontDatabase::fallbacksForFamily(family, style, styleHint, script));

    qCDebug(lcQpaFonts) << __FUNCTION__ << family << style << styleHint
        << script << result;
    return result;
}


QFontEngine *QWindowsFontDatabase::createEngine(const QFontDef &request, const QString &faceName,
                                                int dpi,
                                                const QSharedPointer<QWindowsFontEngineData> &data)
{
    QFontEngine *fe = 0;

    LOGFONT lf = fontDefToLOGFONT(request, faceName);
    const bool preferClearTypeAA = lf.lfQuality == CLEARTYPE_QUALITY;

    if (request.stretch != 100) {
        HFONT hfont = CreateFontIndirect(&lf);
        if (!hfont) {
            qErrnoWarning("%s: CreateFontIndirect failed", __FUNCTION__);
            hfont = QWindowsFontDatabase::systemFont();
        }

        HGDIOBJ oldObj = SelectObject(data->hdc, hfont);
        TEXTMETRIC tm;
        if (!GetTextMetrics(data->hdc, &tm))
            qErrnoWarning("%s: GetTextMetrics failed", __FUNCTION__);
        else
            lf.lfWidth = tm.tmAveCharWidth * request.stretch / 100;
        SelectObject(data->hdc, oldObj);

        DeleteObject(hfont);
    }

#if !defined(QT_NO_DIRECTWRITE)
    if (initDirectWrite(data.data())) {
        const QString fam = QString::fromWCharArray(lf.lfFaceName);
        const QString nameSubstitute = QWindowsFontEngineDirectWrite::fontNameSubstitute(fam);
        if (nameSubstitute != fam) {
            const int nameSubstituteLength = qMin(nameSubstitute.length(), LF_FACESIZE - 1);
            memcpy(lf.lfFaceName, nameSubstitute.utf16(), nameSubstituteLength * sizeof(wchar_t));
            lf.lfFaceName[nameSubstituteLength] = 0;
        }

        HFONT hfont = CreateFontIndirect(&lf);
        if (!hfont) {
            qErrnoWarning("%s: CreateFontIndirect failed", __FUNCTION__);
        } else {
            HGDIOBJ oldFont = SelectObject(data->hdc, hfont);

            IDWriteFontFace *directWriteFontFace = NULL;
            HRESULT hr = data->directWriteGdiInterop->CreateFontFaceFromHdc(data->hdc, &directWriteFontFace);
            if (FAILED(hr)) {
                const QString errorString = qt_error_string(int(hr));
                qWarning().noquote().nospace() << "DirectWrite: CreateFontFaceFromHDC() failed ("
                    << errorString << ") for " << request << ' ' << lf << " dpi=" << dpi;
            } else {
                bool isColorFont = false;
#if defined(QT_USE_DIRECTWRITE2)
                IDWriteFontFace2 *directWriteFontFace2 = nullptr;
                if (SUCCEEDED(directWriteFontFace->QueryInterface(__uuidof(IDWriteFontFace2),
                                                                  reinterpret_cast<void **>(&directWriteFontFace2)))) {
                    if (directWriteFontFace2->IsColorFont())
                        isColorFont = directWriteFontFace2->GetPaletteEntryCount() > 0;
                }
#endif
                const QFont::HintingPreference hintingPreference =
                    static_cast<QFont::HintingPreference>(request.hintingPreference);
                const bool useDw = useDirectWrite(hintingPreference, fam, isColorFont);
                qCDebug(lcQpaFonts) << __FUNCTION__ << request.family << request.pointSize
                    << "pt" << "hintingPreference=" << hintingPreference << "color=" << isColorFont
                    << dpi << "dpi" << "useDirectWrite=" << useDw;
                if (useDw) {
                    QWindowsFontEngineDirectWrite *fedw = new QWindowsFontEngineDirectWrite(directWriteFontFace,
                                                                                            request.pixelSize,
                                                                                            data);

                    wchar_t n[64];
                    GetTextFace(data->hdc, 64, n);

                    QFontDef fontDef = request;
                    fontDef.family = QString::fromWCharArray(n);

                    if (isColorFont)
                        fedw->glyphFormat = QFontEngine::Format_ARGB;
                    fedw->initFontInfo(fontDef, dpi);
                    fe = fedw;
                } else {
                    directWriteFontFace->Release();
                }
            }

            SelectObject(data->hdc, oldFont);
            DeleteObject(hfont);
        }
    }
#endif // QT_NO_DIRECTWRITE

    if (!fe) {
        QWindowsFontEngine *few = new QWindowsFontEngine(request.family, lf, data);
        if (preferClearTypeAA)
            few->glyphFormat = QFontEngine::Format_A32;
        few->initFontInfo(request, dpi);
        fe = few;
    }

    return fe;
}

QFont QWindowsFontDatabase::systemDefaultFont()
{
#if QT_VERSION >= 0x060000
    // Qt 6: Obtain default GUI font (typically "Segoe UI, 9pt", see QTBUG-58610)
    NONCLIENTMETRICS ncm;
    ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICS, lfMessageFont) + sizeof(LOGFONT);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize , &ncm, 0);
    const QFont systemFont = QWindowsFontDatabase::LOGFONT_to_QFont(ncm.lfMessageFont);
#else
    LOGFONT lf;
    GetObject(QWindowsFontDatabase::systemFont(), sizeof(lf), &lf);
    QFont systemFont =  QWindowsFontDatabase::LOGFONT_to_QFont(lf);
    // "MS Shell Dlg 2" is the correct system font >= Win2k
    if (systemFont.family() == QLatin1String("MS Shell Dlg"))
        systemFont.setFamily(QStringLiteral("MS Shell Dlg 2"));
    // Qt 5 by (Qt 4) legacy uses GetStockObject(DEFAULT_GUI_FONT) to
    // obtain the default GUI font (typically "MS Shell Dlg 2, 8pt"). This has been
    // long deprecated; the message font of the NONCLIENTMETRICS structure obtained by
    // SystemParametersInfo(SPI_GETNONCLIENTMETRICS) should be used instead (see
    // QWindowsTheme::refreshFonts(), typically "Segoe UI, 9pt"), which is larger.
#endif // Qt 5
    qCDebug(lcQpaFonts) << __FUNCTION__ << systemFont;
    return systemFont;
}

QFont QWindowsFontDatabase::LOGFONT_to_QFont(const LOGFONT& logFont, int verticalDPI_In)
{
    if (verticalDPI_In <= 0)
        verticalDPI_In = defaultVerticalDPI();
    QFont qFont(QString::fromWCharArray(logFont.lfFaceName));
    qFont.setItalic(logFont.lfItalic);
    if (logFont.lfWeight != FW_DONTCARE)
        qFont.setWeight(QPlatformFontDatabase::weightFromInteger(logFont.lfWeight));
    const qreal logFontHeight = qAbs(logFont.lfHeight);
    qFont.setPointSizeF(logFontHeight * 72.0 / qreal(verticalDPI_In));
    qFont.setUnderline(logFont.lfUnderline);
    qFont.setOverline(false);
    qFont.setStrikeOut(logFont.lfStrikeOut);
    return qFont;
}

int QWindowsFontDatabase::defaultVerticalDPI()
{
    static int vDPI = -1;
    if (vDPI == -1) {
        if (HDC defaultDC = GetDC(0)) {
            vDPI = GetDeviceCaps(defaultDC, LOGPIXELSY);
            ReleaseDC(0, defaultDC);
        } else {
            // FIXME: Resolve now or return 96 and keep unresolved?
            vDPI = 96;
        }
    }
    return vDPI;
}

QString QWindowsFontDatabase::readRegistryString(HKEY parentHandle, const wchar_t *keyPath, const wchar_t *keyName)
{
    QString result;
    HKEY handle = 0;
    if (RegOpenKeyEx(parentHandle, keyPath, 0, KEY_READ, &handle) == ERROR_SUCCESS) {
        // get the size and type of the value
        DWORD dataType;
        DWORD dataSize;
        if (RegQueryValueEx(handle, keyName, 0, &dataType, 0, &dataSize) == ERROR_SUCCESS) {
            if (dataType == REG_SZ || dataType == REG_EXPAND_SZ) {
                dataSize += 2; // '\0' missing?
                QVarLengthArray<unsigned char> data(dataSize);
                data[dataSize - 2] = data[dataSize - 1] = '\0';
                if (RegQueryValueEx(handle, keyName, 0, 0, data.data(), &dataSize) == ERROR_SUCCESS)
                    result = QString::fromWCharArray(reinterpret_cast<const wchar_t *>(data.data()));
            }
        }
        RegCloseKey(handle);
    }
    return result;
}

bool QWindowsFontDatabase::isPrivateFontFamily(const QString &family) const
{
    return m_eudcFonts.contains(family) || QPlatformFontDatabase::isPrivateFontFamily(family);
}

QT_END_NAMESPACE
