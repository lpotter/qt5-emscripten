/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore>
#include <QtTest/QtTest>

#ifdef Q_OS_LINUX
# include <pthread.h>
#endif

Q_DECLARE_METATYPE(QMetaType::Type)

class tst_QMetaType: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QVariant> prop READ prop WRITE setProp)

public:
    tst_QMetaType() { propList << 42 << "Hello"; }

    QList<QVariant> prop() const { return propList; }
    void setProp(const QList<QVariant> &list) { propList = list; }

private:
    QList<QVariant> propList;

private slots:
    void defined();
    void threadSafety();
    void namespaces();
    void qMetaTypeId();
    void properties();
    void normalizedTypes();
    void typeName_data();
    void typeName();
    void create_data();
    void create();
    void createCopy_data();
    void createCopy();
    void sizeOf_data();
    void sizeOf();
    void sizeOfStaticLess_data();
    void sizeOfStaticLess();
    void flags_data();
    void flags();
    void flagsStaticLess_data();
    void flagsStaticLess();
    void construct_data();
    void construct();
    void constructCopy_data();
    void constructCopy();
    void typedefs();
    void registerType();
    void isRegistered_data();
    void isRegistered();
    void isRegisteredStaticLess_data();
    void isRegisteredStaticLess();
    void registerStreamBuiltin();
    void automaticTemplateRegistration();
};

struct Foo { int i; };

void tst_QMetaType::defined()
{
    QCOMPARE(int(QMetaTypeId2<QString>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<Foo>::Defined), 0);
    QCOMPARE(int(QMetaTypeId2<void*>::Defined), 1);
    QCOMPARE(int(QMetaTypeId2<int*>::Defined), 0);
}

struct Bar
{
    Bar()
    {
        // check re-entrancy
        if (!QMetaType::isRegistered(qRegisterMetaType<Foo>("Foo"))) {
            qWarning("%s: re-entrancy test failed", Q_FUNC_INFO);
            ++failureCount;
        }
    }

public:
    static int failureCount;
};

int Bar::failureCount = 0;

class MetaTypeTorturer: public QThread
{
    Q_OBJECT
protected:
    void run()
    {
        Bar space[1];
        space[0].~Bar();

        for (int i = 0; i < 1000; ++i) {
            const QByteArray name = QString("Bar%1_%2").arg(i).arg((size_t)QThread::currentThreadId()).toLatin1();
            const char *nm = name.constData();
            int tp = qRegisterMetaType<Bar>(nm);
#ifdef Q_OS_LINUX
            pthread_yield();
#endif
            QMetaType info(tp);
            if (!info.isValid()) {
                ++failureCount;
                qWarning() << "Wrong typeInfo returned for" << tp;
            }
            if (!info.isRegistered()) {
                ++failureCount;
                qWarning() << name << "is not a registered metatype";
            }
            if (QMetaType::typeFlags(tp) != (QMetaType::NeedsConstruction | QMetaType::NeedsDestruction)) {
                ++failureCount;
                qWarning() << "Wrong typeInfo returned for" << tp;
            }
            if (!QMetaType::isRegistered(tp)) {
                ++failureCount;
                qWarning() << name << "is not a registered metatype";
            }
            if (QMetaType::type(nm) != tp) {
                ++failureCount;
                qWarning() << "Wrong metatype returned for" << name;
            }
            if (QMetaType::typeName(tp) != name) {
                ++failureCount;
                qWarning() << "Wrong typeName returned for" << tp;
            }
            void *buf1 = QMetaType::create(tp, 0);
            void *buf2 = QMetaType::create(tp, buf1);
            void *buf3 = info.create(tp, 0);
            void *buf4 = info.create(tp, buf1);

            QMetaType::construct(tp, space, 0);
            QMetaType::destruct(tp, space);
            QMetaType::construct(tp, space, buf1);
            QMetaType::destruct(tp, space);

            info.construct(space, 0);
            info.destruct(space);
            info.construct(space, buf1);
            info.destruct(space);

            if (!buf1) {
                ++failureCount;
                qWarning() << "Null buffer returned by QMetaType::create(tp, 0)";
            }
            if (!buf2) {
                ++failureCount;
                qWarning() << "Null buffer returned by QMetaType::create(tp, buf)";
            }
            if (!buf3) {
                ++failureCount;
                qWarning() << "Null buffer returned by info.create(tp, 0)";
            }
            if (!buf4) {
                ++failureCount;
                qWarning() << "Null buffer returned by infocreate(tp, buf)";
            }
            QMetaType::destroy(tp, buf1);
            QMetaType::destroy(tp, buf2);
            info.destroy(buf3);
            info.destroy(buf4);
        }
        new (space) Bar;
    }
public:
    MetaTypeTorturer() : failureCount(0) { }
    int failureCount;
};

void tst_QMetaType::threadSafety()
{
    MetaTypeTorturer t1;
    MetaTypeTorturer t2;
    MetaTypeTorturer t3;

    t1.start();
    t2.start();
    t3.start();

    QVERIFY(t1.wait());
    QVERIFY(t2.wait());
    QVERIFY(t3.wait());

    QCOMPARE(t1.failureCount, 0);
    QCOMPARE(t2.failureCount, 0);
    QCOMPARE(t3.failureCount, 0);
    QCOMPARE(Bar::failureCount, 0);
}

namespace TestSpace
{
    struct Foo { double d; };

}
Q_DECLARE_METATYPE(TestSpace::Foo)

void tst_QMetaType::namespaces()
{
    TestSpace::Foo nf = { 11.12 };
    QVariant v = qVariantFromValue(nf);
    QCOMPARE(qvariant_cast<TestSpace::Foo>(v).d, 11.12);
}

void tst_QMetaType::qMetaTypeId()
{
    QCOMPARE(::qMetaTypeId<QString>(), int(QMetaType::QString));
    QCOMPARE(::qMetaTypeId<int>(), int(QMetaType::Int));
    QCOMPARE(::qMetaTypeId<TestSpace::Foo>(), QMetaType::type("TestSpace::Foo"));

    QCOMPARE(::qMetaTypeId<char>(), QMetaType::type("char"));
    QCOMPARE(::qMetaTypeId<uchar>(), QMetaType::type("unsigned char"));
    QCOMPARE(::qMetaTypeId<signed char>(), QMetaType::type("signed char"));
    QCOMPARE(::qMetaTypeId<qint8>(), QMetaType::type("qint8"));
}

void tst_QMetaType::properties()
{
    qRegisterMetaType<QList<QVariant> >("QList<QVariant>");

    QVariant v = property("prop");

    QCOMPARE(v.typeName(), "QVariantList");

    QList<QVariant> values = v.toList();
    QCOMPARE(values.count(), 2);
    QCOMPARE(values.at(0).toInt(), 42);

    values << 43 << "world";

    QVERIFY(setProperty("prop", values));
    v = property("prop");
    QCOMPARE(v.toList().count(), 4);
}

template <typename T>
struct Whity { T t; };

Q_DECLARE_METATYPE( Whity < int > )
Q_DECLARE_METATYPE(Whity<double>)

void tst_QMetaType::normalizedTypes()
{
    int WhityIntId = ::qMetaTypeId<Whity<int> >();
    int WhityDoubleId = ::qMetaTypeId<Whity<double> >();

    QCOMPARE(QMetaType::type("Whity<int>"), WhityIntId);
    QCOMPARE(QMetaType::type(" Whity < int > "), WhityIntId);
    QCOMPARE(QMetaType::type("Whity<int >"), WhityIntId);

    QCOMPARE(QMetaType::type("Whity<double>"), WhityDoubleId);
    QCOMPARE(QMetaType::type(" Whity< double > "), WhityDoubleId);
    QCOMPARE(QMetaType::type("Whity<double >"), WhityDoubleId);

    QCOMPARE(qRegisterMetaType<Whity<int> >(" Whity < int > "), WhityIntId);
    QCOMPARE(qRegisterMetaType<Whity<int> >("Whity<int>"), WhityIntId);
    QCOMPARE(qRegisterMetaType<Whity<int> >("Whity<int > "), WhityIntId);

    QCOMPARE(qRegisterMetaType<Whity<double> >(" Whity < double > "), WhityDoubleId);
    QCOMPARE(qRegisterMetaType<Whity<double> >("Whity<double>"), WhityDoubleId);
    QCOMPARE(qRegisterMetaType<Whity<double> >("Whity<double > "), WhityDoubleId);
}

#define TYPENAME_DATA(MetaTypeName, MetaTypeId, RealType)\
    QTest::newRow(#RealType) << QMetaType::MetaTypeName << #RealType;

#define TYPENAME_DATA_ALIAS(MetaTypeName, MetaTypeId, AliasType, RealTypeString)\
    QTest::newRow(RealTypeString) << QMetaType::MetaTypeName << #AliasType;

void tst_QMetaType::typeName_data()
{
    QTest::addColumn<QMetaType::Type>("aType");
    QTest::addColumn<QString>("aTypeName");

    QT_FOR_EACH_STATIC_TYPE(TYPENAME_DATA)
    QT_FOR_EACH_STATIC_ALIAS_TYPE(TYPENAME_DATA_ALIAS)
}

void tst_QMetaType::typeName()
{
    QFETCH(QMetaType::Type, aType);
    QFETCH(QString, aTypeName);

    QCOMPARE(QString::fromLatin1(QMetaType::typeName(aType)), aTypeName);
}

#define FOR_EACH_PRIMITIVE_METATYPE(F) \
    QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(F) \
    QT_FOR_EACH_STATIC_CORE_POINTER(F) \

#define FOR_EACH_COMPLEX_CORE_METATYPE(F) \
    QT_FOR_EACH_STATIC_CORE_CLASS(F)

#define FOR_EACH_CORE_METATYPE(F) \
    FOR_EACH_PRIMITIVE_METATYPE(F) \
    FOR_EACH_COMPLEX_CORE_METATYPE(F) \

template <int ID>
struct MetaEnumToType {};

#define DEFINE_META_ENUM_TO_TYPE(MetaTypeName, MetaTypeId, RealType) \
template<> \
struct MetaEnumToType<QMetaType::MetaTypeName> { \
    typedef RealType Type; \
};
FOR_EACH_CORE_METATYPE(DEFINE_META_ENUM_TO_TYPE)
#undef DEFINE_META_ENUM_TO_TYPE

template <int ID>
struct DefaultValueFactory
{
    typedef typename MetaEnumToType<ID>::Type Type;
    static Type *create() { return new Type; }
};

template <>
struct DefaultValueFactory<QMetaType::Void>
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    static Type *create() { return 0; }
};

template <int ID>
struct DefaultValueTraits
{
    // By default we assume that a default-constructed value (new T) is
    // initialized; e.g. QCOMPARE(*(new T), *(new T)) should succeed
    enum { IsInitialized = true };
};

#define DEFINE_NON_INITIALIZED_DEFAULT_VALUE_TRAITS(MetaTypeName, MetaTypeId, RealType) \
template<> struct DefaultValueTraits<QMetaType::MetaTypeName> { \
    enum { IsInitialized = false }; \
};
// Primitive types (int et al) aren't initialized
FOR_EACH_PRIMITIVE_METATYPE(DEFINE_NON_INITIALIZED_DEFAULT_VALUE_TRAITS)
#undef DEFINE_NON_INITIALIZED_DEFAULT_VALUE_TRAITS

template <int ID>
struct TestValueFactory {};

template<> struct TestValueFactory<QMetaType::Void> {
    static void *create() { return 0; }
};

template<> struct TestValueFactory<QMetaType::QString> {
    static QString *create() { return new QString(QString::fromLatin1("QString")); }
};
template<> struct TestValueFactory<QMetaType::Int> {
    static int *create() { return new int(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::UInt> {
    static uint *create() { return new uint(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::Bool> {
    static bool *create() { return new bool(true); }
};
template<> struct TestValueFactory<QMetaType::Double> {
    static double *create() { return new double(3.14); }
};
template<> struct TestValueFactory<QMetaType::QByteArray> {
    static QByteArray *create() { return new QByteArray(QByteArray("QByteArray")); }
};
template<> struct TestValueFactory<QMetaType::QChar> {
    static QChar *create() { return new QChar(QChar('q')); }
};
template<> struct TestValueFactory<QMetaType::Long> {
    static long *create() { return new long(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::Short> {
    static short *create() { return new short(0x1234); }
};
template<> struct TestValueFactory<QMetaType::Char> {
    static char *create() { return new char('c'); }
};
template<> struct TestValueFactory<QMetaType::ULong> {
    static ulong *create() { return new ulong(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::UShort> {
    static ushort *create() { return new ushort(0x1234); }
};
template<> struct TestValueFactory<QMetaType::UChar> {
    static uchar *create() { return new uchar('u'); }
};
template<> struct TestValueFactory<QMetaType::Float> {
    static float *create() { return new float(3.14); }
};
template<> struct TestValueFactory<QMetaType::QObjectStar> {
    static QObject * *create() { return new QObject *(0); }
};
template<> struct TestValueFactory<QMetaType::QWidgetStar> {
    static QWidget * *create() { return new QWidget *(0); }
};
template<> struct TestValueFactory<QMetaType::VoidStar> {
    static void * *create() { return new void *(0); }
};
template<> struct TestValueFactory<QMetaType::LongLong> {
    static qlonglong *create() { return new qlonglong(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::ULongLong> {
    static qulonglong *create() { return new qulonglong(0x12345678); }
};
template<> struct TestValueFactory<QMetaType::QStringList> {
    static QStringList *create() { return new QStringList(QStringList() << "Q" << "t"); }
};
template<> struct TestValueFactory<QMetaType::QBitArray> {
    static QBitArray *create() { return new QBitArray(QBitArray(256, true)); }
};
template<> struct TestValueFactory<QMetaType::QDate> {
    static QDate *create() { return new QDate(QDate::currentDate()); }
};
template<> struct TestValueFactory<QMetaType::QTime> {
    static QTime *create() { return new QTime(QTime::currentTime()); }
};
template<> struct TestValueFactory<QMetaType::QDateTime> {
    static QDateTime *create() { return new QDateTime(QDateTime::currentDateTime()); }
};
template<> struct TestValueFactory<QMetaType::QUrl> {
    static QUrl *create() { return new QUrl("http://www.example.org"); }
};
template<> struct TestValueFactory<QMetaType::QLocale> {
    static QLocale *create() { return new QLocale(QLocale::c()); }
};
template<> struct TestValueFactory<QMetaType::QRect> {
    static QRect *create() { return new QRect(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QRectF> {
    static QRectF *create() { return new QRectF(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QSize> {
    static QSize *create() { return new QSize(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QSizeF> {
    static QSizeF *create() { return new QSizeF(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QLine> {
    static QLine *create() { return new QLine(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QLineF> {
    static QLineF *create() { return new QLineF(10, 20, 30, 40); }
};
template<> struct TestValueFactory<QMetaType::QPoint> {
    static QPoint *create() { return new QPoint(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QPointF> {
    static QPointF *create() { return new QPointF(10, 20); }
};
template<> struct TestValueFactory<QMetaType::QEasingCurve> {
    static QEasingCurve *create() { return new QEasingCurve(QEasingCurve::InOutElastic); }
};
template<> struct TestValueFactory<QMetaType::QUuid> {
    static QUuid *create() { return new QUuid(); }
};
template<> struct TestValueFactory<QMetaType::QModelIndex> {
    static QModelIndex *create() { return new QModelIndex(); }
};
template<> struct TestValueFactory<QMetaType::QRegExp> {
    static QRegExp *create()
    {
#ifndef QT_NO_REGEXP
        return new QRegExp("A*");
#else
        return 0;
#endif
    }
};
template<> struct TestValueFactory<QMetaType::QVariant> {
    static QVariant *create() { return new QVariant(QStringList(QStringList() << "Q" << "t")); }
};

void tst_QMetaType::create_data()
{
    QTest::addColumn<QMetaType::Type>("type");
#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(QMetaType::typeName(QMetaType::MetaTypeName)) << QMetaType::MetaTypeName;
FOR_EACH_CORE_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

template<int ID>
static void testCreateHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    QMetaType info(ID);
    void *actual1 = QMetaType::create(ID);
    void *actual2 = info.create();
    if (DefaultValueTraits<ID>::IsInitialized) {
        Type *expected = DefaultValueFactory<ID>::create();
        QCOMPARE(*static_cast<Type *>(actual1), *expected);
        QCOMPARE(*static_cast<Type *>(actual2), *expected);
        delete expected;
    }
    QMetaType::destroy(ID, actual1);
    info.destroy(actual2);
}

template<>
void testCreateHelper<QMetaType::Void>()
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    void *actual = QMetaType::create(QMetaType::Void);
    if (DefaultValueTraits<QMetaType::Void>::IsInitialized) {
        QVERIFY(DefaultValueFactory<QMetaType::Void>::create());
    }
    QMetaType::destroy(QMetaType::Void, actual);
}


typedef void (*TypeTestFunction)();

void tst_QMetaType::create()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CREATE_FUNCTION(MetaTypeName, MetaTypeId, RealType) \
            case QMetaType::MetaTypeName: \
            return testCreateHelper<QMetaType::MetaTypeName>;
FOR_EACH_CORE_METATYPE(RETURN_CREATE_FUNCTION)
#undef RETURN_CREATE_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

template<int ID>
static void testCreateCopyHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    Type *expected = TestValueFactory<ID>::create();
    QMetaType info(ID);
    void *actual1 = QMetaType::create(ID, expected);
    void *actual2 = info.create(expected);
    QCOMPARE(*static_cast<Type *>(actual1), *expected);
    QCOMPARE(*static_cast<Type *>(actual2), *expected);
    QMetaType::destroy(ID, actual1);
    info.destroy(actual2);
    delete expected;
}

template<>
void testCreateCopyHelper<QMetaType::Void>()
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    Type *expected = TestValueFactory<QMetaType::Void>::create();
    void *actual = QMetaType::create(QMetaType::Void, expected);
    QCOMPARE(static_cast<Type *>(actual), expected);
    QMetaType::destroy(QMetaType::Void, actual);
}

void tst_QMetaType::createCopy_data()
{
    create_data();
}

void tst_QMetaType::createCopy()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CREATE_COPY_FUNCTION(MetaTypeName, MetaTypeId, RealType) \
            case QMetaType::MetaTypeName: \
            return testCreateCopyHelper<QMetaType::MetaTypeName>;
FOR_EACH_CORE_METATYPE(RETURN_CREATE_COPY_FUNCTION)
#undef RETURN_CREATE_COPY_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

void tst_QMetaType::sizeOf_data()
{
    QTest::addColumn<QMetaType::Type>("type");
    QTest::addColumn<int>("size");
#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << QMetaType::MetaTypeName << int(QTypeInfo<RealType>::sizeOf);
FOR_EACH_CORE_METATYPE(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
}

void tst_QMetaType::sizeOf()
{
    QFETCH(QMetaType::Type, type);
    QFETCH(int, size);
    QCOMPARE(QMetaType::sizeOf(type), size);
}

void tst_QMetaType::sizeOfStaticLess_data()
{
    sizeOf_data();
}

void tst_QMetaType::sizeOfStaticLess()
{
    QFETCH(QMetaType::Type, type);
    QFETCH(int, size);
    QCOMPARE(QMetaType(type).sizeOf(), size);
}

struct CustomMovable {};
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(CustomMovable, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
Q_DECLARE_METATYPE(CustomMovable);

class CustomObject : public QObject
{
    Q_OBJECT
public:
    CustomObject(QObject *parent = 0)
      : QObject(parent)
    {

    }
};
Q_DECLARE_METATYPE(CustomObject*);

struct SecondBase {};

class CustomMultiInheritanceObject : public QObject, SecondBase
{
    Q_OBJECT
public:
    CustomMultiInheritanceObject(QObject *parent = 0)
      : QObject(parent)
    {

    }
};
Q_DECLARE_METATYPE(CustomMultiInheritanceObject*);

class C { char _[4]; };
class M { char _[4]; };
class P { char _[4]; };

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(M, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(P, Q_PRIMITIVE_TYPE);
QT_END_NAMESPACE

// avoid the comma:
typedef QPair<C,C> QPairCC;
typedef QPair<C,M> QPairCM;
typedef QPair<C,P> QPairCP;
typedef QPair<M,C> QPairMC;
typedef QPair<M,M> QPairMM;
typedef QPair<M,P> QPairMP;
typedef QPair<P,C> QPairPC;
typedef QPair<P,M> QPairPM;
typedef QPair<P,P> QPairPP;

Q_DECLARE_METATYPE(QPairCC)
Q_DECLARE_METATYPE(QPairCM)
Q_DECLARE_METATYPE(QPairCP)
Q_DECLARE_METATYPE(QPairMC)
Q_DECLARE_METATYPE(QPairMM)
Q_DECLARE_METATYPE(QPairMP)
Q_DECLARE_METATYPE(QPairPC)
Q_DECLARE_METATYPE(QPairPM)
Q_DECLARE_METATYPE(QPairPP)

void tst_QMetaType::flags_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<bool>("isMovable");
    QTest::addColumn<bool>("isComplex");
    QTest::addColumn<bool>("isPointerToQObject");

#define ADD_METATYPE_TEST_ROW(MetaTypeName, MetaTypeId, RealType) \
    QTest::newRow(#RealType) << MetaTypeId << bool(!QTypeInfo<RealType>::isStatic) << bool(QTypeInfo<RealType>::isComplex) << bool(QtPrivate::IsPointerToTypeDerivedFromQObject<RealType>::Value);
QT_FOR_EACH_STATIC_CORE_CLASS(ADD_METATYPE_TEST_ROW)
QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(ADD_METATYPE_TEST_ROW)
QT_FOR_EACH_STATIC_CORE_POINTER(ADD_METATYPE_TEST_ROW)
#undef ADD_METATYPE_TEST_ROW
    QTest::newRow("TestSpace::Foo") << ::qMetaTypeId<TestSpace::Foo>() << false << true << false;
    QTest::newRow("Whity<double>") << ::qMetaTypeId<Whity<double> >() << false << true << false;
    QTest::newRow("CustomMovable") << ::qMetaTypeId<CustomMovable>() << true << true << false;
    QTest::newRow("CustomObject*") << ::qMetaTypeId<CustomObject*>() << true << false << true;
    QTest::newRow("CustomMultiInheritanceObject*") << ::qMetaTypeId<CustomMultiInheritanceObject*>() << true << false << true;
    QTest::newRow("QPair<C,C>") << ::qMetaTypeId<QPair<C,C> >() << false << true  << false;
    QTest::newRow("QPair<C,M>") << ::qMetaTypeId<QPair<C,M> >() << false << true  << false;
    QTest::newRow("QPair<C,P>") << ::qMetaTypeId<QPair<C,P> >() << false << true  << false;
    QTest::newRow("QPair<M,C>") << ::qMetaTypeId<QPair<M,C> >() << false << true  << false;
    QTest::newRow("QPair<M,M>") << ::qMetaTypeId<QPair<M,M> >() << true  << true  << false;
    QTest::newRow("QPair<M,P>") << ::qMetaTypeId<QPair<M,P> >() << true  << true  << false;
    QTest::newRow("QPair<P,C>") << ::qMetaTypeId<QPair<P,C> >() << false << true  << false;
    QTest::newRow("QPair<P,M>") << ::qMetaTypeId<QPair<P,M> >() << true  << true  << false;
    QTest::newRow("QPair<P,P>") << ::qMetaTypeId<QPair<P,P> >() << true  << false << false;
}

void tst_QMetaType::flags()
{
    QFETCH(int, type);
    QFETCH(bool, isMovable);
    QFETCH(bool, isComplex);
    QFETCH(bool, isPointerToQObject);

    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::NeedsConstruction), isComplex);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::NeedsDestruction), isComplex);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::MovableType), isMovable);
    QCOMPARE(bool(QMetaType::typeFlags(type) & QMetaType::PointerToQObject), isPointerToQObject);
}

void tst_QMetaType::flagsStaticLess_data()
{
    flags_data();
}

void tst_QMetaType::flagsStaticLess()
{
    QFETCH(int, type);
    QFETCH(bool, isMovable);
    QFETCH(bool, isComplex);

    int flags = QMetaType(type).flags();
    QCOMPARE(bool(flags & QMetaType::NeedsConstruction), isComplex);
    QCOMPARE(bool(flags & QMetaType::NeedsDestruction), isComplex);
    QCOMPARE(bool(flags & QMetaType::MovableType), isMovable);
}

void tst_QMetaType::construct_data()
{
    create_data();
}

#ifndef Q_ALIGNOF
template<uint N>
struct RoundToNextHighestPowerOfTwo
{
private:
    enum { V1 = N-1 };
    enum { V2 = V1 | (V1 >> 1) };
    enum { V3 = V2 | (V2 >> 2) };
    enum { V4 = V3 | (V3 >> 4) };
    enum { V5 = V4 | (V4 >> 8) };
    enum { V6 = V5 | (V5 >> 16) };
public:
    enum { Value = V6 + 1 };
};
#endif

template<class T>
struct TypeAlignment
{
#ifdef Q_ALIGNOF
    enum { Value = Q_ALIGNOF(T) };
#else
    enum { Value = RoundToNextHighestPowerOfTwo<sizeof(T)>::Value };
#endif
};

template<int ID>
static void testConstructHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    QMetaType info(ID);
    int size = info.sizeOf();
    void *storage1 = qMallocAligned(size, TypeAlignment<Type>::Value);
    void *actual1 = QMetaType::construct(ID, storage1, /*copy=*/0);
    void *storage2 = qMallocAligned(size, TypeAlignment<Type>::Value);
    void *actual2 = info.construct(storage2, /*copy=*/0);
    QCOMPARE(actual1, storage1);
    QCOMPARE(actual2, storage2);
    if (DefaultValueTraits<ID>::IsInitialized) {
        Type *expected = DefaultValueFactory<ID>::create();
        QCOMPARE(*static_cast<Type *>(actual1), *expected);
        QCOMPARE(*static_cast<Type *>(actual2), *expected);
        delete expected;
    }
    QMetaType::destruct(ID, actual1);
    qFreeAligned(storage1);
    info.destruct(actual2);
    qFreeAligned(storage2);

    QVERIFY(QMetaType::construct(ID, 0, /*copy=*/0) == 0);
    QMetaType::destruct(ID, 0);

    QVERIFY(info.construct(0, /*copy=*/0) == 0);
    info.destruct(0);
}

template<>
void testConstructHelper<QMetaType::Void>()
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    /*int size = */ QMetaType::sizeOf(QMetaType::Void);
    void *storage = 0;
    void *actual = QMetaType::construct(QMetaType::Void, storage, /*copy=*/0);
    QCOMPARE(actual, storage);
    if (DefaultValueTraits<QMetaType::Void>::IsInitialized) {
        /*Type *expected = */ DefaultValueFactory<QMetaType::Void>::create();
    }
    QMetaType::destruct(QMetaType::Void, actual);
    qFreeAligned(storage);

    QVERIFY(QMetaType::construct(QMetaType::Void, 0, /*copy=*/0) == 0);
    QMetaType::destruct(QMetaType::Void, 0);
}

void tst_QMetaType::construct()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CONSTRUCT_FUNCTION(MetaTypeName, MetaTypeId, RealType) \
            case QMetaType::MetaTypeName: \
            return testConstructHelper<QMetaType::MetaTypeName>;
FOR_EACH_CORE_METATYPE(RETURN_CONSTRUCT_FUNCTION)
#undef RETURN_CONSTRUCT_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

template<int ID>
static void testConstructCopyHelper()
{
    typedef typename MetaEnumToType<ID>::Type Type;
    Type *expected = TestValueFactory<ID>::create();
    QMetaType info(ID);
    int size = QMetaType::sizeOf(ID);
    QCOMPARE(info.sizeOf(), size);
    void *storage1 = qMallocAligned(size, TypeAlignment<Type>::Value);
    void *actual1 = QMetaType::construct(ID, storage1, expected);
    void *storage2 = qMallocAligned(size, TypeAlignment<Type>::Value);
    void *actual2 = info.construct(storage2, expected);
    QCOMPARE(actual1, storage1);
    QCOMPARE(actual2, storage2);
    QCOMPARE(*static_cast<Type *>(actual1), *expected);
    QCOMPARE(*static_cast<Type *>(actual2), *expected);
    QMetaType::destruct(ID, actual1);
    qFreeAligned(storage1);
    info.destruct(actual2);
    qFreeAligned(storage2);

    QVERIFY(QMetaType::construct(ID, 0, expected) == 0);
    QVERIFY(info.construct(0, expected) == 0);

    delete expected;
}

template<>
void testConstructCopyHelper<QMetaType::Void>()
{
    typedef MetaEnumToType<QMetaType::Void>::Type Type;
    Type *expected = TestValueFactory<QMetaType::Void>::create();
    /* int size = */QMetaType::sizeOf(QMetaType::Void);
    void *storage = 0;
    void *actual = QMetaType::construct(QMetaType::Void, storage, expected);
    QCOMPARE(actual, storage);
    QMetaType::destruct(QMetaType::Void, actual);
    qFreeAligned(storage);

    QVERIFY(QMetaType::construct(QMetaType::Void, 0, expected) == 0);
}

void tst_QMetaType::constructCopy_data()
{
    create_data();
}

void tst_QMetaType::constructCopy()
{
    struct TypeTestFunctionGetter
    {
        static TypeTestFunction get(int type)
        {
            switch (type) {
#define RETURN_CONSTRUCT_COPY_FUNCTION(MetaTypeName, MetaTypeId, RealType) \
            case QMetaType::MetaTypeName: \
            return testConstructCopyHelper<QMetaType::MetaTypeName>;
FOR_EACH_CORE_METATYPE(RETURN_CONSTRUCT_COPY_FUNCTION)
#undef RETURN_CONSTRUCT_COPY_FUNCTION
            }
            return 0;
        }
    };

    QFETCH(QMetaType::Type, type);
    TypeTestFunctionGetter::get(type)();
}

typedef QString CustomString;
Q_DECLARE_METATYPE(CustomString) //this line is useless

void tst_QMetaType::typedefs()
{
    QCOMPARE(QMetaType::type("long long"), int(QMetaType::LongLong));
    QCOMPARE(QMetaType::type("unsigned long long"), int(QMetaType::ULongLong));
    QCOMPARE(QMetaType::type("qint8"), int(QMetaType::Char));
    QCOMPARE(QMetaType::type("quint8"), int(QMetaType::UChar));
    QCOMPARE(QMetaType::type("qint16"), int(QMetaType::Short));
    QCOMPARE(QMetaType::type("quint16"), int(QMetaType::UShort));
    QCOMPARE(QMetaType::type("qint32"), int(QMetaType::Int));
    QCOMPARE(QMetaType::type("quint32"), int(QMetaType::UInt));
    QCOMPARE(QMetaType::type("qint64"), int(QMetaType::LongLong));
    QCOMPARE(QMetaType::type("quint64"), int(QMetaType::ULongLong));

    // make sure the qreal typeId is the type id of the type it's defined to
    QCOMPARE(QMetaType::type("qreal"), ::qMetaTypeId<qreal>());

    qRegisterMetaType<CustomString>("CustomString");
    QCOMPARE(QMetaType::type("CustomString"), ::qMetaTypeId<CustomString>());

    typedef Whity<double> WhityDouble;
    qRegisterMetaType<WhityDouble>("WhityDouble");
    QCOMPARE(QMetaType::type("WhityDouble"), ::qMetaTypeId<WhityDouble>());
}

void tst_QMetaType::registerType()
{
    // Built-in
    QCOMPARE(qRegisterMetaType<QString>("QString"), int(QMetaType::QString));
    QCOMPARE(qRegisterMetaType<QString>("QString"), int(QMetaType::QString));

    // Custom
    int fooId = qRegisterMetaType<TestSpace::Foo>("TestSpace::Foo");
    QVERIFY(fooId >= int(QMetaType::User));
    QCOMPARE(qRegisterMetaType<TestSpace::Foo>("TestSpace::Foo"), fooId);

    int movableId = qRegisterMetaType<CustomMovable>("CustomMovable");
    QVERIFY(movableId >= int(QMetaType::User));
    QCOMPARE(qRegisterMetaType<CustomMovable>("CustomMovable"), movableId);

    // Alias to built-in
    typedef QString MyString;

    QCOMPARE(qRegisterMetaType<MyString>("MyString"), int(QMetaType::QString));
    QCOMPARE(qRegisterMetaType<MyString>("MyString"), int(QMetaType::QString));

    QCOMPARE(QMetaType::type("MyString"), int(QMetaType::QString));

    // Alias to custom type
    typedef CustomMovable MyMovable;
    typedef TestSpace::Foo MyFoo;

    QCOMPARE(qRegisterMetaType<MyMovable>("MyMovable"), movableId);
    QCOMPARE(qRegisterMetaType<MyMovable>("MyMovable"), movableId);

    QCOMPARE(QMetaType::type("MyMovable"), movableId);

    QCOMPARE(qRegisterMetaType<MyFoo>("MyFoo"), fooId);
    QCOMPARE(qRegisterMetaType<MyFoo>("MyFoo"), fooId);

    QCOMPARE(QMetaType::type("MyFoo"), fooId);
}

class IsRegisteredDummyType { };

void tst_QMetaType::isRegistered_data()
{
    QTest::addColumn<int>("typeId");
    QTest::addColumn<bool>("registered");

    // predefined/custom types
    QTest::newRow("QMetaType::Void") << int(QMetaType::Void) << true;
    QTest::newRow("QMetaType::Int") << int(QMetaType::Int) << true;

    int dummyTypeId = qRegisterMetaType<IsRegisteredDummyType>("IsRegisteredDummyType");

    QTest::newRow("IsRegisteredDummyType") << dummyTypeId << true;

    // unknown types
    QTest::newRow("-1") << -1 << false;
    QTest::newRow("-42") << -42 << false;
    QTest::newRow("IsRegisteredDummyType + 1") << (dummyTypeId + 1) << false;
}

void tst_QMetaType::isRegistered()
{
    QFETCH(int, typeId);
    QFETCH(bool, registered);
    QCOMPARE(QMetaType::isRegistered(typeId), registered);
}

void tst_QMetaType::isRegisteredStaticLess_data()
{
    isRegistered_data();
}

void tst_QMetaType::isRegisteredStaticLess()
{
    QFETCH(int, typeId);
    QFETCH(bool, registered);
    QCOMPARE(QMetaType(typeId).isRegistered(), registered);
}

void tst_QMetaType::registerStreamBuiltin()
{
    //should not crash;
    qRegisterMetaTypeStreamOperators<QString>("QString");
    qRegisterMetaTypeStreamOperators<QVariant>("QVariant");
}

Q_DECLARE_METATYPE(QSharedPointer<QObject>)

void tst_QMetaType::automaticTemplateRegistration()
{
  {
    QList<int> intList;
    intList << 42;
    QVERIFY(QVariant::fromValue(intList).value<QList<int> >().first() == 42);
    QVector<QList<int> > vectorList;
    vectorList << intList;
    QVERIFY(QVariant::fromValue(vectorList).value<QVector<QList<int> > >().first().first() == 42);
  }

  {
    QList<QByteArray> bytearrayList;
    bytearrayList << QByteArray("foo");
    QVERIFY(QVariant::fromValue(bytearrayList).value<QList<QByteArray> >().first() == QByteArray("foo"));
    QVector<QList<QByteArray> > vectorList;
    vectorList << bytearrayList;
    QVERIFY(QVariant::fromValue(vectorList).value<QVector<QList<QByteArray> > >().first().first() == QByteArray("foo"));
  }

  QCOMPARE(::qMetaTypeId<QVariantList>(), (int)QMetaType::QVariantList);
  QCOMPARE(::qMetaTypeId<QList<QVariant> >(), (int)QMetaType::QVariantList);

  {
    QList<QVariant> variantList;
    variantList << 42;
    QVERIFY(QVariant::fromValue(variantList).value<QList<QVariant> >().first() == 42);
    QVector<QList<QVariant> > vectorList;
    vectorList << variantList;
    QVERIFY(QVariant::fromValue(vectorList).value<QVector<QList<QVariant> > >().first().first() == 42);
  }

  {
    QList<QSharedPointer<QObject> > sharedPointerList;
    QObject *testObject = new QObject;
    sharedPointerList << QSharedPointer<QObject>(testObject);
    QVERIFY(QVariant::fromValue(sharedPointerList).value<QList<QSharedPointer<QObject> > >().first() == testObject);
    QVector<QList<QSharedPointer<QObject> > > vectorList;
    vectorList << sharedPointerList;
    QVERIFY(QVariant::fromValue(vectorList).value<QVector<QList<QSharedPointer<QObject> > > >().first().first() == testObject);
  }
}

// Compile-time test, it should be possible to register function pointer types
class Undefined;

typedef Undefined (*UndefinedFunction0)();
typedef Undefined (*UndefinedFunction1)(Undefined);
typedef Undefined (*UndefinedFunction2)(Undefined, Undefined);
typedef Undefined (*UndefinedFunction3)(Undefined, Undefined, Undefined);

Q_DECLARE_METATYPE(UndefinedFunction0);
Q_DECLARE_METATYPE(UndefinedFunction1);
Q_DECLARE_METATYPE(UndefinedFunction2);
Q_DECLARE_METATYPE(UndefinedFunction3);

QTEST_MAIN(tst_QMetaType)
#include "tst_qmetatype.moc"
