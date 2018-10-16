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

#include "qjni_p.h"
#include "qjnihelpers_p.h"
#include <QtCore/qthreadstorage.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/QThread>
#include <QtCore/QReadWriteLock>

QT_BEGIN_NAMESPACE

static inline QString keyBase()
{
    return QStringLiteral("%1%2:%3");
}

static QString qt_convertJString(jstring string)
{
    QJNIEnvironmentPrivate env;
    int strLength = env->GetStringLength(string);
    QString res(strLength, Qt::Uninitialized);
    env->GetStringRegion(string, 0, strLength, reinterpret_cast<jchar *>(res.data()));
    return res;
}

static inline bool exceptionCheckAndClear(JNIEnv *env)
{
    if (Q_UNLIKELY(env->ExceptionCheck())) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        return true;
    }

    return false;
}

typedef QHash<QString, jclass> JClassHash;
Q_GLOBAL_STATIC(JClassHash, cachedClasses)
Q_GLOBAL_STATIC(QReadWriteLock, cachedClassesLock)

static QByteArray toBinaryEncClassName(const QByteArray &className)
{
    return QByteArray(className).replace('/', '.');
}

static jclass getCachedClass(const QByteArray &classBinEnc, bool *isCached = 0)
{
    QReadLocker locker(cachedClassesLock);
    const QHash<QString, jclass>::const_iterator &it = cachedClasses->constFind(QString::fromLatin1(classBinEnc));
    const bool found = (it != cachedClasses->constEnd());

    if (isCached != 0)
        *isCached = found;

    return found ? it.value() : 0;
}

inline static jclass loadClass(const QByteArray &className, JNIEnv *env, bool binEncoded = false)
{
    const QByteArray &binEncClassName = binEncoded ? className : toBinaryEncClassName(className);

    bool isCached = false;
    jclass clazz = getCachedClass(binEncClassName, &isCached);
    if (clazz != 0 || isCached)
        return clazz;

    QJNIObjectPrivate classLoader(QtAndroidPrivate::classLoader());
    if (!classLoader.isValid())
        return 0;

    QWriteLocker locker(cachedClassesLock);
    // did we lose the race?
    const QLatin1String key(binEncClassName);
    const QHash<QString, jclass>::const_iterator &it = cachedClasses->constFind(key);
    if (it != cachedClasses->constEnd())
        return it.value();

    QJNIObjectPrivate stringName = QJNIObjectPrivate::fromString(key);
    QJNIObjectPrivate classObject = classLoader.callObjectMethod("loadClass",
                                                                 "(Ljava/lang/String;)Ljava/lang/Class;",
                                                                 stringName.object());

    if (!exceptionCheckAndClear(env) && classObject.isValid())
        clazz = static_cast<jclass>(env->NewGlobalRef(classObject.object()));

    cachedClasses->insert(key, clazz);
    return clazz;
}

typedef QHash<QString, jmethodID> JMethodIDHash;
Q_GLOBAL_STATIC(JMethodIDHash, cachedMethodID)
Q_GLOBAL_STATIC(QReadWriteLock, cachedMethodIDLock)

static inline jmethodID getMethodID(JNIEnv *env,
                                    jclass clazz,
                                    const char *name,
                                    const char *sig,
                                    bool isStatic = false)
{
    jmethodID id = isStatic ? env->GetStaticMethodID(clazz, name, sig)
                            : env->GetMethodID(clazz, name, sig);

    if (exceptionCheckAndClear(env))
        return 0;

    return id;
}

static jmethodID getCachedMethodID(JNIEnv *env,
                                   jclass clazz,
                                   const QByteArray &className,
                                   const char *name,
                                   const char *sig,
                                   bool isStatic = false)
{
    if (className.isEmpty())
        return getMethodID(env, clazz, name, sig, isStatic);

    const QString key = keyBase().arg(QLatin1String(className)).arg(QLatin1String(name)).arg(QLatin1String(sig));
    QHash<QString, jmethodID>::const_iterator it;

    {
        QReadLocker locker(cachedMethodIDLock);
        it = cachedMethodID->constFind(key);
        if (it != cachedMethodID->constEnd())
            return it.value();
    }

    {
        QWriteLocker locker(cachedMethodIDLock);
        it = cachedMethodID->constFind(key);
        if (it != cachedMethodID->constEnd())
            return it.value();

        jmethodID id = getMethodID(env, clazz, name, sig, isStatic);

        cachedMethodID->insert(key, id);
        return id;
    }
}

typedef QHash<QString, jfieldID> JFieldIDHash;
Q_GLOBAL_STATIC(JFieldIDHash, cachedFieldID)
Q_GLOBAL_STATIC(QReadWriteLock, cachedFieldIDLock)

static inline jfieldID getFieldID(JNIEnv *env,
                                  jclass clazz,
                                  const char *name,
                                  const char *sig,
                                  bool isStatic = false)
{
    jfieldID id = isStatic ? env->GetStaticFieldID(clazz, name, sig)
                           : env->GetFieldID(clazz, name, sig);

    if (exceptionCheckAndClear(env))
        return 0;

    return id;
}

static jfieldID getCachedFieldID(JNIEnv *env,
                                 jclass clazz,
                                 const QByteArray &className,
                                 const char *name,
                                 const char *sig,
                                 bool isStatic = false)
{
    if (className.isNull())
        return getFieldID(env, clazz, name, sig, isStatic);

    const QString key = keyBase().arg(QLatin1String(className)).arg(QLatin1String(name)).arg(QLatin1String(sig));
    QHash<QString, jfieldID>::const_iterator it;

    {
        QReadLocker locker(cachedFieldIDLock);
        it = cachedFieldID->constFind(key);
        if (it != cachedFieldID->constEnd())
            return it.value();
    }

    {
        QWriteLocker locker(cachedFieldIDLock);
        it = cachedFieldID->constFind(key);
        if (it != cachedFieldID->constEnd())
            return it.value();

        jfieldID id = getFieldID(env, clazz, name, sig, isStatic);

        cachedFieldID->insert(key, id);
        return id;
    }
}

void QJNILocalRefDeleter::cleanup(jobject obj)
{
    if (obj == 0)
        return;

    QJNIEnvironmentPrivate env;
    env->DeleteLocalRef(obj);
}

class QJNIEnvironmentPrivateTLS
{
public:
    inline ~QJNIEnvironmentPrivateTLS()
    {
        QtAndroidPrivate::javaVM()->DetachCurrentThread();
    }
};

Q_GLOBAL_STATIC(QThreadStorage<QJNIEnvironmentPrivateTLS *>, jniEnvTLS)

static const char qJniThreadName[] = "QtThread";

QJNIEnvironmentPrivate::QJNIEnvironmentPrivate()
    : jniEnv(0)
{
    JavaVM *vm = QtAndroidPrivate::javaVM();
    const jint ret = vm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6);
    if (ret == JNI_OK) // Already attached
        return;

    if (ret == JNI_EDETACHED) { // We need to (re-)attach
        JavaVMAttachArgs args = { JNI_VERSION_1_6, qJniThreadName, nullptr };
        if (vm->AttachCurrentThread(&jniEnv, &args) != JNI_OK)
            return;

        if (!jniEnvTLS->hasLocalData()) // If we attached the thread we own it.
            jniEnvTLS->setLocalData(new QJNIEnvironmentPrivateTLS);
    }
}

JNIEnv *QJNIEnvironmentPrivate::operator->()
{
    return jniEnv;
}

jclass QJNIEnvironmentPrivate::findClass(const char *className, JNIEnv *env)
{
    const QByteArray &classDotEnc = toBinaryEncClassName(className);
    bool isCached = false;
    jclass clazz = getCachedClass(classDotEnc, &isCached);

    const bool found = (clazz != 0) || (clazz == 0 && isCached);

    if (found)
        return clazz;

    const QLatin1String key(classDotEnc);
    if (env != 0) { // We got an env. pointer (We expect this to be the right env. and call FindClass())
        QWriteLocker locker(cachedClassesLock);
        const QHash<QString, jclass>::const_iterator &it = cachedClasses->constFind(key);
        // Did we lose the race?
        if (it != cachedClasses->constEnd())
            return it.value();

        jclass fclazz = env->FindClass(className);
        if (!exceptionCheckAndClear(env)) {
            clazz = static_cast<jclass>(env->NewGlobalRef(fclazz));
            env->DeleteLocalRef(fclazz);
        }

        if (clazz != 0)
            cachedClasses->insert(key, clazz);
    }

    if (clazz == 0) // We didn't get an env. pointer or we got one with the WRONG class loader...
        clazz = loadClass(classDotEnc, QJNIEnvironmentPrivate(), true);

    return clazz;
}

QJNIEnvironmentPrivate::operator JNIEnv* () const
{
    return jniEnv;
}

QJNIEnvironmentPrivate::~QJNIEnvironmentPrivate()
{
}

QJNIObjectData::QJNIObjectData()
    : m_jobject(0),
      m_jclass(0),
      m_own_jclass(true)
{

}

QJNIObjectData::~QJNIObjectData()
{
    QJNIEnvironmentPrivate env;
    if (m_jobject)
        env->DeleteGlobalRef(m_jobject);
    if (m_jclass && m_own_jclass)
        env->DeleteGlobalRef(m_jclass);
}

QJNIObjectPrivate::QJNIObjectPrivate()
    : d(new QJNIObjectData())
{

}

QJNIObjectPrivate::QJNIObjectPrivate(const char *className)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    d->m_className = toBinaryEncClassName(className);
    d->m_jclass = loadClass(d->m_className, env, true);
    d->m_own_jclass = false;
    if (d->m_jclass) {
        // get default constructor
        jmethodID constructorId = getCachedMethodID(env, d->m_jclass, d->m_className, "<init>", "()V");
        if (constructorId) {
            jobject obj = env->NewObject(d->m_jclass, constructorId);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(const char *className, const char *sig, ...)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    d->m_className = toBinaryEncClassName(className);
    d->m_jclass = loadClass(d->m_className, env, true);
    d->m_own_jclass = false;
    if (d->m_jclass) {
        jmethodID constructorId = getCachedMethodID(env, d->m_jclass, d->m_className, "<init>", sig);
        if (constructorId) {
            va_list args;
            va_start(args, sig);
            jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
            va_end(args);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(const char *className, const char *sig, const QVaListPrivate &args)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    d->m_className = toBinaryEncClassName(className);
    d->m_jclass = loadClass(d->m_className, env, true);
    d->m_own_jclass = false;
    if (d->m_jclass) {
        jmethodID constructorId = getCachedMethodID(env, d->m_jclass, d->m_className, "<init>", sig);
        if (constructorId) {
            jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(jclass clazz)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    d->m_jclass = static_cast<jclass>(env->NewGlobalRef(clazz));
    if (d->m_jclass) {
        // get default constructor
        jmethodID constructorId = getMethodID(env, d->m_jclass, "<init>", "()V");
        if (constructorId) {
            jobject obj = env->NewObject(d->m_jclass, constructorId);
            if (obj) {
                d->m_jobject = env->NewGlobalRef(obj);
                env->DeleteLocalRef(obj);
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(jclass clazz, const char *sig, ...)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    if (clazz) {
        d->m_jclass = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (d->m_jclass) {
            jmethodID constructorId = getMethodID(env, d->m_jclass, "<init>", sig);
            if (constructorId) {
                va_list args;
                va_start(args, sig);
                jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
                va_end(args);
                if (obj) {
                    d->m_jobject = env->NewGlobalRef(obj);
                    env->DeleteLocalRef(obj);
                }
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(jclass clazz, const char *sig, const QVaListPrivate &args)
    : d(new QJNIObjectData())
{
    QJNIEnvironmentPrivate env;
    if (clazz) {
        d->m_jclass = static_cast<jclass>(env->NewGlobalRef(clazz));
        if (d->m_jclass) {
            jmethodID constructorId = getMethodID(env, d->m_jclass, "<init>", sig);
            if (constructorId) {
                jobject obj = env->NewObjectV(d->m_jclass, constructorId, args);
                if (obj) {
                    d->m_jobject = env->NewGlobalRef(obj);
                    env->DeleteLocalRef(obj);
                }
            }
        }
    }
}

QJNIObjectPrivate::QJNIObjectPrivate(jobject obj)
    : d(new QJNIObjectData())
{
    if (!obj)
        return;

    QJNIEnvironmentPrivate env;
    d->m_jobject = env->NewGlobalRef(obj);
    jclass cls = env->GetObjectClass(obj);
    d->m_jclass = static_cast<jclass>(env->NewGlobalRef(cls));
    env->DeleteLocalRef(cls);
}
template <>
Q_CORE_EXPORT void QJNIObjectPrivate::callMethodV<void>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jmethodID id = getCachedMethodID(env, d->m_jclass, d->m_className, methodName, sig);
    if (id) {
        env->CallVoidMethodV(d->m_jobject, id, args);
    }
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::callMethod<void>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    callMethodV<void>(methodName, sig, args);
    va_end(args);
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::callMethodV<jboolean>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jboolean res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, d->m_className, methodName, sig);
    if (id) {
        res = env->CallBooleanMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::callMethod<jboolean>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jboolean res = callMethodV<jboolean>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::callMethodV<jbyte>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jbyte res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, d->m_className, methodName, sig);
    if (id) {
        res = env->CallByteMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::callMethod<jbyte>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jbyte res = callMethodV<jbyte>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::callMethodV<jchar>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jchar res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, d->m_className, methodName, sig);
    if (id) {
        res = env->CallCharMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::callMethod<jchar>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jchar res = callMethodV<jchar>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::callMethodV<jshort>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jshort res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, d->m_className, methodName, sig);
    if (id) {
        res = env->CallShortMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::callMethod<jshort>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jshort res = callMethodV<jshort>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::callMethodV<jint>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jint res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, d->m_className, methodName, sig);
    if (id) {
        res = env->CallIntMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::callMethod<jint>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jint res = callMethodV<jint>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::callMethodV<jlong>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jlong res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, d->m_className, methodName, sig);
    if (id) {
        res = env->CallLongMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::callMethod<jlong>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jlong res = callMethodV<jlong>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::callMethodV<jfloat>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jfloat res = 0.f;
    jmethodID id = getCachedMethodID(env, d->m_jclass, d->m_className, methodName, sig);
    if (id) {
        res = env->CallFloatMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::callMethod<jfloat>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jfloat res = callMethodV<jfloat>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::callMethodV<jdouble>(const char *methodName, const char *sig, va_list args) const
{
    QJNIEnvironmentPrivate env;
    jdouble res = 0.;
    jmethodID id = getCachedMethodID(env, d->m_jclass, d->m_className, methodName, sig);
    if (id) {
        res = env->CallDoubleMethodV(d->m_jobject, id, args);
    }
    return res;
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::callMethod<jdouble>(const char *methodName, const char *sig, ...) const
{
    va_list args;
    va_start(args, sig);
    jdouble res = callMethodV<jdouble>(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::callMethod<void>(const char *methodName) const
{
    callMethod<void>(methodName, "()V");
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::callMethod<jboolean>(const char *methodName) const
{
    return callMethod<jboolean>(methodName, "()Z");
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::callMethod<jbyte>(const char *methodName) const
{
    return callMethod<jbyte>(methodName, "()B");
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::callMethod<jchar>(const char *methodName) const
{
    return callMethod<jchar>(methodName, "()C");
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::callMethod<jshort>(const char *methodName) const
{
    return callMethod<jshort>(methodName, "()S");
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::callMethod<jint>(const char *methodName) const
{
    return callMethod<jint>(methodName, "()I");
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::callMethod<jlong>(const char *methodName) const
{
    return callMethod<jlong>(methodName, "()J");
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::callMethod<jfloat>(const char *methodName) const
{
    return callMethod<jfloat>(methodName, "()F");
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::callMethod<jdouble>(const char *methodName) const
{
    return callMethod<jdouble>(methodName, "()D");
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::callStaticMethodV<void>(const char *className,
                                                const char *methodName,
                                                const char *sig,
                                                va_list args)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, toBinaryEncClassName(className), methodName, sig, true);
        if (id) {
            env->CallStaticVoidMethodV(clazz, id, args);
        }
    }
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::callStaticMethod<void>(const char *className,
                                               const char *methodName,
                                               const char *sig,
                                               ...)
{
    va_list args;
    va_start(args, sig);
    callStaticMethodV<void>(className, methodName, sig, args);
    va_end(args);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::callStaticMethodV<void>(jclass clazz,
                                                const char *methodName,
                                                const char *sig,
                                                va_list args)
{
    QJNIEnvironmentPrivate env;
    jmethodID id = getMethodID(env, clazz, methodName, sig, true);
    if (id) {
        env->CallStaticVoidMethodV(clazz, id, args);
    }
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::callStaticMethod<void>(jclass clazz,
                                               const char *methodName,
                                               const char *sig,
                                               ...)
{
    va_list args;
    va_start(args, sig);
    callStaticMethodV<void>(clazz, methodName, sig, args);
    va_end(args);
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::callStaticMethodV<jboolean>(const char *className,
                                                        const char *methodName,
                                                        const char *sig,
                                                        va_list args)
{
    QJNIEnvironmentPrivate env;
    jboolean res = 0;
    jclass clazz = loadClass(className, env);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, toBinaryEncClassName(className), methodName, sig, true);
        if (id) {
            res = env->CallStaticBooleanMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::callStaticMethod<jboolean>(const char *className,
                                                       const char *methodName,
                                                       const char *sig,
                                                       ...)
{
    va_list args;
    va_start(args, sig);
    jboolean res = callStaticMethodV<jboolean>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::callStaticMethodV<jboolean>(jclass clazz,
                                                        const char *methodName,
                                                        const char *sig,
                                                        va_list args)
{
    QJNIEnvironmentPrivate env;
    jboolean res = 0;
    jmethodID id = getMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticBooleanMethodV(clazz, id, args);
    }

    return res;
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::callStaticMethod<jboolean>(jclass clazz,
                                                       const char *methodName,
                                                       const char *sig,
                                                       ...)
{
    va_list args;
    va_start(args, sig);
    jboolean res = callStaticMethodV<jboolean>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::callStaticMethodV<jbyte>(const char *className,
                                                  const char *methodName,
                                                  const char *sig,
                                                  va_list args)
{
    QJNIEnvironmentPrivate env;
    jbyte res = 0;
    jclass clazz = loadClass(className, env);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, toBinaryEncClassName(className), methodName, sig, true);
        if (id) {
            res = env->CallStaticByteMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::callStaticMethod<jbyte>(const char *className,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jbyte res = callStaticMethodV<jbyte>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::callStaticMethodV<jbyte>(jclass clazz,
                                                  const char *methodName,
                                                  const char *sig,
                                                  va_list args)
{
    QJNIEnvironmentPrivate env;
    jbyte res = 0;
    jmethodID id = getMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticByteMethodV(clazz, id, args);
    }

    return res;
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::callStaticMethod<jbyte>(jclass clazz,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jbyte res = callStaticMethodV<jbyte>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::callStaticMethodV<jchar>(const char *className,
                                                  const char *methodName,
                                                  const char *sig,
                                                  va_list args)
{
    QJNIEnvironmentPrivate env;
    jchar res = 0;
    jclass clazz = loadClass(className, env);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, toBinaryEncClassName(className), methodName, sig, true);
        if (id) {
            res = env->CallStaticCharMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::callStaticMethod<jchar>(const char *className,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jchar res = callStaticMethodV<jchar>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::callStaticMethodV<jchar>(jclass clazz,
                                                  const char *methodName,
                                                  const char *sig,
                                                  va_list args)
{
    QJNIEnvironmentPrivate env;
    jchar res = 0;
    jmethodID id = getMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticCharMethodV(clazz, id, args);
    }

    return res;
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::callStaticMethod<jchar>(jclass clazz,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jchar res = callStaticMethodV<jchar>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::callStaticMethodV<jshort>(const char *className,
                                                    const char *methodName,
                                                    const char *sig,
                                                    va_list args)
{
    QJNIEnvironmentPrivate env;
    jshort res = 0;
    jclass clazz = loadClass(className, env);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, toBinaryEncClassName(className), methodName, sig, true);
        if (id) {
            res = env->CallStaticShortMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::callStaticMethod<jshort>(const char *className,
                                                   const char *methodName,
                                                   const char *sig,
                                                   ...)
{
    va_list args;
    va_start(args, sig);
    jshort res = callStaticMethodV<jshort>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::callStaticMethodV<jshort>(jclass clazz,
                                                    const char *methodName,
                                                    const char *sig,
                                                    va_list args)
{
    QJNIEnvironmentPrivate env;
    jshort res = 0;
    jmethodID id = getMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticShortMethodV(clazz, id, args);
    }

    return res;
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::callStaticMethod<jshort>(jclass clazz,
                                                   const char *methodName,
                                                   const char *sig,
                                                   ...)
{
    va_list args;
    va_start(args, sig);
    jshort res = callStaticMethodV<jshort>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::callStaticMethodV<jint>(const char *className,
                                                const char *methodName,
                                                const char *sig,
                                                va_list args)
{
    QJNIEnvironmentPrivate env;
    jint res = 0;
    jclass clazz = loadClass(className, env);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, toBinaryEncClassName(className), methodName, sig, true);
        if (id) {
            res = env->CallStaticIntMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::callStaticMethod<jint>(const char *className,
                                               const char *methodName,
                                               const char *sig,
                                               ...)
{
    va_list args;
    va_start(args, sig);
    jint res = callStaticMethodV<jint>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::callStaticMethodV<jint>(jclass clazz,
                                                const char *methodName,
                                                const char *sig,
                                                va_list args)
{
    QJNIEnvironmentPrivate env;
    jint res = 0;
    jmethodID id = getMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticIntMethodV(clazz, id, args);
    }

    return res;
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::callStaticMethod<jint>(jclass clazz,
                                               const char *methodName,
                                               const char *sig,
                                               ...)
{
    va_list args;
    va_start(args, sig);
    jint res = callStaticMethodV<jint>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::callStaticMethodV<jlong>(const char *className,
                                                  const char *methodName,
                                                  const char *sig,
                                                  va_list args)
{
    QJNIEnvironmentPrivate env;
    jlong res = 0;
    jclass clazz = loadClass(className, env);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, toBinaryEncClassName(className), methodName, sig, true);
        if (id) {
            res = env->CallStaticLongMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::callStaticMethod<jlong>(const char *className,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jlong res = callStaticMethodV<jlong>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::callStaticMethodV<jlong>(jclass clazz,
                                                  const char *methodName,
                                                  const char *sig,
                                                  va_list args)
{
    QJNIEnvironmentPrivate env;
    jlong res = 0;
    jmethodID id = getMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticLongMethodV(clazz, id, args);
    }

    return res;
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::callStaticMethod<jlong>(jclass clazz,
                                                 const char *methodName,
                                                 const char *sig,
                                                 ...)
{
    va_list args;
    va_start(args, sig);
    jlong res = callStaticMethodV<jlong>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::callStaticMethodV<jfloat>(const char *className,
                                                    const char *methodName,
                                                    const char *sig,
                                                    va_list args)
{
    QJNIEnvironmentPrivate env;
    jfloat res = 0.f;
    jclass clazz = loadClass(className, env);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, toBinaryEncClassName(className), methodName, sig, true);
        if (id) {
            res = env->CallStaticFloatMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::callStaticMethod<jfloat>(const char *className,
                                                   const char *methodName,
                                                   const char *sig,
                                                   ...)
{
    va_list args;
    va_start(args, sig);
    jfloat res = callStaticMethodV<jfloat>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::callStaticMethodV<jfloat>(jclass clazz,
                                                    const char *methodName,
                                                    const char *sig,
                                                    va_list args)
{
    QJNIEnvironmentPrivate env;
    jfloat res = 0.f;
    jmethodID id = getMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticFloatMethodV(clazz, id, args);
    }

    return res;
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::callStaticMethod<jfloat>(jclass clazz,
                                                   const char *methodName,
                                                   const char *sig,
                                                   ...)
{
    va_list args;
    va_start(args, sig);
    jfloat res = callStaticMethodV<jfloat>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::callStaticMethodV<jdouble>(const char *className,
                                                      const char *methodName,
                                                      const char *sig,
                                                      va_list args)
{
    QJNIEnvironmentPrivate env;
    jdouble res = 0.;
    jclass clazz = loadClass(className, env);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, toBinaryEncClassName(className), methodName, sig, true);
        if (id) {
            res = env->CallStaticDoubleMethodV(clazz, id, args);
        }
    }

    return res;
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::callStaticMethod<jdouble>(const char *className,
                                                     const char *methodName,
                                                     const char *sig,
                                                     ...)
{
    va_list args;
    va_start(args, sig);
    jdouble res = callStaticMethodV<jdouble>(className, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::callStaticMethodV<jdouble>(jclass clazz,
                                                      const char *methodName,
                                                      const char *sig,
                                                      va_list args)
{
    QJNIEnvironmentPrivate env;
    jdouble res = 0.;
    jmethodID id = getMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticDoubleMethodV(clazz, id, args);
    }

    return res;
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::callStaticMethod<jdouble>(jclass clazz,
                                                     const char *methodName,
                                                     const char *sig,
                                                     ...)
{
    va_list args;
    va_start(args, sig);
    jdouble res = callStaticMethodV<jdouble>(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::callStaticMethod<void>(const char *className, const char *methodName)
{
    callStaticMethod<void>(className, methodName, "()V");
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::callStaticMethod<void>(jclass clazz, const char *methodName)
{
    callStaticMethod<void>(clazz, methodName, "()V");
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::callStaticMethod<jboolean>(const char *className, const char *methodName)
{
    return callStaticMethod<jboolean>(className, methodName, "()Z");
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::callStaticMethod<jboolean>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jboolean>(clazz, methodName, "()Z");
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::callStaticMethod<jbyte>(const char *className, const char *methodName)
{
    return callStaticMethod<jbyte>(className, methodName, "()B");
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::callStaticMethod<jbyte>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jbyte>(clazz, methodName, "()B");
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::callStaticMethod<jchar>(const char *className, const char *methodName)
{
    return callStaticMethod<jchar>(className, methodName, "()C");
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::callStaticMethod<jchar>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jchar>(clazz, methodName, "()C");
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::callStaticMethod<jshort>(const char *className, const char *methodName)
{
    return callStaticMethod<jshort>(className, methodName, "()S");
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::callStaticMethod<jshort>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jshort>(clazz, methodName, "()S");
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::callStaticMethod<jint>(const char *className, const char *methodName)
{
    return callStaticMethod<jint>(className, methodName, "()I");
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::callStaticMethod<jint>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jint>(clazz, methodName, "()I");
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::callStaticMethod<jlong>(const char *className, const char *methodName)
{
    return callStaticMethod<jlong>(className, methodName, "()J");
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::callStaticMethod<jlong>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jlong>(clazz, methodName, "()J");
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::callStaticMethod<jfloat>(const char *className, const char *methodName)
{
    return callStaticMethod<jfloat>(className, methodName, "()F");
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::callStaticMethod<jfloat>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jfloat>(clazz, methodName, "()F");
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::callStaticMethod<jdouble>(const char *className, const char *methodName)
{
    return callStaticMethod<jdouble>(className, methodName, "()D");
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::callStaticMethod<jdouble>(jclass clazz, const char *methodName)
{
    return callStaticMethod<jdouble>(clazz, methodName, "()D");
}

QJNIObjectPrivate QJNIObjectPrivate::callObjectMethodV(const char *methodName,
                                                       const char *sig,
                                                       va_list args) const
{
    QJNIEnvironmentPrivate env;
    jobject res = 0;
    jmethodID id = getCachedMethodID(env, d->m_jclass, d->m_className, methodName, sig);
    if (id) {
        res = env->CallObjectMethodV(d->m_jobject, id, args);
        if (res && env->ExceptionCheck())
            res = 0;
    }

    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod(const char *methodName,
                                                      const char *sig,
                                                      ...) const
{
    va_list args;
    va_start(args, sig);
    QJNIObjectPrivate res = callObjectMethodV(methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jstring>(const char *methodName) const
{
    return callObjectMethod(methodName, "()Ljava/lang/String;");
}

template <>
Q_CORE_EXPORT QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jbooleanArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[Z");
}

template <>
Q_CORE_EXPORT QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jbyteArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[B");
}

template <>
Q_CORE_EXPORT QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jshortArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[S");
}

template <>
Q_CORE_EXPORT QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jintArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[I");
}

template <>
Q_CORE_EXPORT QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jlongArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[J");
}

template <>
Q_CORE_EXPORT QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jfloatArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[F");
}

template <>
Q_CORE_EXPORT QJNIObjectPrivate QJNIObjectPrivate::callObjectMethod<jdoubleArray>(const char *methodName) const
{
    return callObjectMethod(methodName, "()[D");
}

QJNIObjectPrivate QJNIObjectPrivate::callStaticObjectMethodV(const char *className,
                                                             const char *methodName,
                                                             const char *sig,
                                                             va_list args)
{
    QJNIEnvironmentPrivate env;
    jobject res = 0;
    jclass clazz = loadClass(className, env);
    if (clazz) {
        jmethodID id = getCachedMethodID(env, clazz, toBinaryEncClassName(className), methodName, sig, true);
        if (id) {
            res = env->CallStaticObjectMethodV(clazz, id, args);
            if (res && env->ExceptionCheck())
                res = 0;
        }
    }

    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

QJNIObjectPrivate QJNIObjectPrivate::callStaticObjectMethod(const char *className,
                                                            const char *methodName,
                                                            const char *sig,
                                                            ...)
{
    va_list args;
    va_start(args, sig);
    QJNIObjectPrivate res = callStaticObjectMethodV(className, methodName, sig, args);
    va_end(args);
    return res;
}

QJNIObjectPrivate QJNIObjectPrivate::callStaticObjectMethodV(jclass clazz,
                                                             const char *methodName,
                                                             const char *sig,
                                                             va_list args)
{
    QJNIEnvironmentPrivate env;
    jobject res = 0;
    jmethodID id = getMethodID(env, clazz, methodName, sig, true);
    if (id) {
        res = env->CallStaticObjectMethodV(clazz, id, args);
        if (res && env->ExceptionCheck())
            res = 0;
    }

    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

QJNIObjectPrivate QJNIObjectPrivate::callStaticObjectMethod(jclass clazz,
                                                            const char *methodName,
                                                            const char *sig,
                                                            ...)
{
    va_list args;
    va_start(args, sig);
    QJNIObjectPrivate res = callStaticObjectMethodV(clazz, methodName, sig, args);
    va_end(args);
    return res;
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::getField<jboolean>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jboolean res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "Z");
    if (id)
        res = env->GetBooleanField(d->m_jobject, id);

    return res;
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::getField<jbyte>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jbyte res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "B");
    if (id)
        res = env->GetByteField(d->m_jobject, id);

    return res;
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::getField<jchar>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jchar res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "C");
    if (id)
        res = env->GetCharField(d->m_jobject, id);

    return res;
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::getField<jshort>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jshort res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "S");
    if (id)
        res = env->GetShortField(d->m_jobject, id);

    return res;
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::getField<jint>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jint res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "I");
    if (id)
        res = env->GetIntField(d->m_jobject, id);

    return res;
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::getField<jlong>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jlong res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "J");
    if (id)
        res = env->GetLongField(d->m_jobject, id);

    return res;
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::getField<jfloat>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jfloat res = 0.f;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "F");
    if (id)
        res = env->GetFloatField(d->m_jobject, id);

    return res;
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::getField<jdouble>(const char *fieldName) const
{
    QJNIEnvironmentPrivate env;
    jdouble res = 0.;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "D");
    if (id)
        res = env->GetDoubleField(d->m_jobject, id);

    return res;
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::getStaticField<jboolean>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jboolean res = 0;
    jfieldID id = getFieldID(env, clazz, fieldName, "Z", true);
    if (id)
        res = env->GetStaticBooleanField(clazz, id);

    return res;
}

template <>
Q_CORE_EXPORT jboolean QJNIObjectPrivate::getStaticField<jboolean>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return 0;

    jfieldID id = getCachedFieldID(env, clazz, toBinaryEncClassName(className), fieldName, "Z", true);
    if (id == 0)
        return 0;

    return env->GetStaticBooleanField(clazz, id);
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::getStaticField<jbyte>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jbyte res = 0;
    jfieldID id = getFieldID(env, clazz, fieldName, "B", true);
    if (id)
        res = env->GetStaticByteField(clazz, id);

    return res;
}

template <>
Q_CORE_EXPORT jbyte QJNIObjectPrivate::getStaticField<jbyte>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return 0;

    jfieldID id = getCachedFieldID(env, clazz, toBinaryEncClassName(className), fieldName, "B", true);
    if (id == 0)
        return 0;

    return env->GetStaticByteField(clazz, id);
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::getStaticField<jchar>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jchar res = 0;
    jfieldID id = getFieldID(env, clazz, fieldName, "C", true);
    if (id)
        res = env->GetStaticCharField(clazz, id);

    return res;
}

template <>
Q_CORE_EXPORT jchar QJNIObjectPrivate::getStaticField<jchar>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return 0;

    jfieldID id = getCachedFieldID(env, clazz, toBinaryEncClassName(className), fieldName, "C", true);
    if (id == 0)
        return 0;

    return env->GetStaticCharField(clazz, id);
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::getStaticField<jshort>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jshort res = 0;
    jfieldID id = getFieldID(env, clazz, fieldName, "S", true);
    if (id)
        res = env->GetStaticShortField(clazz, id);

    return res;
}

template <>
Q_CORE_EXPORT jshort QJNIObjectPrivate::getStaticField<jshort>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return 0;

    jfieldID id = getCachedFieldID(env, clazz, toBinaryEncClassName(className), fieldName, "S", true);
    if (id == 0)
        return 0;

    return env->GetStaticShortField(clazz, id);
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::getStaticField<jint>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jint res = 0;
    jfieldID id = getFieldID(env, clazz, fieldName, "I", true);
    if (id)
        res = env->GetStaticIntField(clazz, id);

    return res;
}

template <>
Q_CORE_EXPORT jint QJNIObjectPrivate::getStaticField<jint>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return 0;

    jfieldID id = getCachedFieldID(env, clazz, toBinaryEncClassName(className), fieldName, "I", true);
    if (id == 0)
        return 0;

    return env->GetStaticIntField(clazz, id);
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::getStaticField<jlong>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jlong res = 0;
    jfieldID id = getFieldID(env, clazz, fieldName, "J", true);
    if (id)
        res = env->GetStaticLongField(clazz, id);

    return res;
}

template <>
Q_CORE_EXPORT jlong QJNIObjectPrivate::getStaticField<jlong>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return 0;

    jfieldID id = getCachedFieldID(env, clazz, toBinaryEncClassName(className), fieldName, "J", true);
    if (id == 0)
        return 0;

    return env->GetStaticLongField(clazz, id);
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::getStaticField<jfloat>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jfloat res = 0.f;
    jfieldID id = getFieldID(env, clazz, fieldName, "F", true);
    if (id)
        res = env->GetStaticFloatField(clazz, id);

    return res;
}

template <>
Q_CORE_EXPORT jfloat QJNIObjectPrivate::getStaticField<jfloat>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return 0.f;

    jfieldID id = getCachedFieldID(env, clazz, toBinaryEncClassName(className), fieldName, "F", true);
    if (id == 0)
        return 0.f;

    return env->GetStaticFloatField(clazz, id);
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::getStaticField<jdouble>(jclass clazz, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jdouble res = 0.;
    jfieldID id = getFieldID(env, clazz, fieldName, "D", true);
    if (id)
        res = env->GetStaticDoubleField(clazz, id);

    return res;
}

template <>
Q_CORE_EXPORT jdouble QJNIObjectPrivate::getStaticField<jdouble>(const char *className, const char *fieldName)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return 0.;

    jfieldID id = getCachedFieldID(env, clazz, toBinaryEncClassName(className), fieldName, "D", true);
    if (id == 0)
        return 0.;

    return env->GetStaticDoubleField(clazz, id);
}

QJNIObjectPrivate QJNIObjectPrivate::getObjectField(const char *fieldName,
                                                    const char *sig) const
{
    QJNIEnvironmentPrivate env;
    jobject res = 0;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, sig);
    if (id) {
        res = env->GetObjectField(d->m_jobject, id);
        if (res && env->ExceptionCheck())
            res = 0;
    }

    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

QJNIObjectPrivate QJNIObjectPrivate::getStaticObjectField(const char *className,
                                                          const char *fieldName,
                                                          const char *sig)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return QJNIObjectPrivate();

    jfieldID id = getCachedFieldID(env, clazz, toBinaryEncClassName(className), fieldName, sig, true);
    if (id == 0)
        return QJNIObjectPrivate();

    jobject res = env->GetStaticObjectField(clazz, id);
    if (res && env->ExceptionCheck())
        res = 0;

    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

QJNIObjectPrivate QJNIObjectPrivate::getStaticObjectField(jclass clazz,
                                                          const char *fieldName,
                                                          const char *sig)
{
    QJNIEnvironmentPrivate env;
    jobject res = 0;
    jfieldID id = getFieldID(env, clazz, fieldName, sig, true);
    if (id) {
        res = env->GetStaticObjectField(clazz, id);
        if (res && env->ExceptionCheck())
            res = 0;
    }

    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jboolean>(const char *fieldName, jboolean value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "Z");
    if (id)
        env->SetBooleanField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jbyte>(const char *fieldName, jbyte value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "B");
    if (id)
        env->SetByteField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jchar>(const char *fieldName, jchar value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "C");
    if (id)
        env->SetCharField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jshort>(const char *fieldName, jshort value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "S");
    if (id)
        env->SetShortField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jint>(const char *fieldName, jint value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "I");
    if (id)
        env->SetIntField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jlong>(const char *fieldName, jlong value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "J");
    if (id)
        env->SetLongField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jfloat>(const char *fieldName, jfloat value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "F");
    if (id)
        env->SetFloatField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jdouble>(const char *fieldName, jdouble value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "D");
    if (id)
        env->SetDoubleField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jbooleanArray>(const char *fieldName, jbooleanArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "[Z");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jbyteArray>(const char *fieldName, jbyteArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "[B");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jcharArray>(const char *fieldName, jcharArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "[C");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jshortArray>(const char *fieldName, jshortArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "[S");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jintArray>(const char *fieldName, jintArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "[I");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jlongArray>(const char *fieldName, jlongArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "[J");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jfloatArray>(const char *fieldName, jfloatArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "[F");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jdoubleArray>(const char *fieldName, jdoubleArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "[D");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jstring>(const char *fieldName, jstring value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, "Ljava/lang/String;");
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jobject>(const char *fieldName,
                                          const char *sig,
                                          jobject value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, sig);
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setField<jobjectArray>(const char *fieldName,
                                               const char *sig,
                                               jobjectArray value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getCachedFieldID(env, d->m_jclass, d->m_className, fieldName, sig);
    if (id)
        env->SetObjectField(d->m_jobject, id, value);

}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jboolean>(jclass clazz,
                                                 const char *fieldName,
                                                 jboolean value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getFieldID(env, clazz, fieldName, "Z", true);
    if (id)
        env->SetStaticBooleanField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jboolean>(const char *className,
                                                 const char *fieldName,
                                                 jboolean value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return;

    jfieldID id = getCachedFieldID(env, clazz, className, fieldName, "Z", true);
    if (id == 0)
        return;

    env->SetStaticBooleanField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jbyte>(jclass clazz,
                                              const char *fieldName,
                                              jbyte value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getFieldID(env, clazz, fieldName, "B", true);
    if (id)
        env->SetStaticByteField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jbyte>(const char *className,
                                              const char *fieldName,
                                              jbyte value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return;

    jfieldID id = getCachedFieldID(env, clazz, className, fieldName, "B", true);
    if (id == 0)
        return;

    env->SetStaticByteField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jchar>(jclass clazz,
                                              const char *fieldName,
                                              jchar value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getFieldID(env, clazz, fieldName, "C", true);
    if (id)
        env->SetStaticCharField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jchar>(const char *className,
                                              const char *fieldName,
                                              jchar value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return;

    jfieldID id = getCachedFieldID(env, clazz, className, fieldName, "C", true);
    if (id == 0)
        return;

    env->SetStaticCharField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jshort>(jclass clazz,
                                               const char *fieldName,
                                               jshort value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getFieldID(env, clazz, fieldName, "S", true);
    if (id)
        env->SetStaticShortField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jshort>(const char *className,
                                               const char *fieldName,
                                               jshort value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return;

    jfieldID id = getCachedFieldID(env, clazz, className, fieldName, "S", true);
    if (id == 0)
        return;

    env->SetStaticShortField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jint>(jclass clazz,
                                             const char *fieldName,
                                             jint value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getFieldID(env, clazz, fieldName, "I", true);
    if (id)
        env->SetStaticIntField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jint>(const char *className,
                                             const char *fieldName,
                                             jint value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return;

    jfieldID id = getCachedFieldID(env, clazz, className, fieldName, "I", true);
    if (id == 0)
        return;

    env->SetStaticIntField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jlong>(jclass clazz,
                                              const char *fieldName,
                                              jlong value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getFieldID(env, clazz, fieldName, "J", true);
    if (id)
        env->SetStaticLongField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jlong>(const char *className,
                                              const char *fieldName,
                                              jlong value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return;

    jfieldID id = getCachedFieldID(env, clazz, className, fieldName, "J", true);
    if (id == 0)
        return;

    env->SetStaticLongField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jfloat>(jclass clazz,
                                               const char *fieldName,
                                               jfloat value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getFieldID(env, clazz, fieldName, "F", true);
    if (id)
        env->SetStaticFloatField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jfloat>(const char *className,
                                               const char *fieldName,
                                               jfloat value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return;

    jfieldID id = getCachedFieldID(env, clazz, className, fieldName, "F", true);
    if (id == 0)
        return;

    env->SetStaticFloatField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jdouble>(jclass clazz,
                                                const char *fieldName,
                                                jdouble value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getFieldID(env, clazz, fieldName, "D", true);
    if (id)
        env->SetStaticDoubleField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jdouble>(const char *className,
                                                const char *fieldName,
                                                jdouble value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return;

    jfieldID id = getCachedFieldID(env, clazz, className, fieldName, "D", true);
    if (id == 0)
        return;

    env->SetStaticDoubleField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jobject>(jclass clazz,
                                                const char *fieldName,
                                                const char *sig,
                                                jobject value)
{
    QJNIEnvironmentPrivate env;
    jfieldID id = getFieldID(env, clazz, fieldName, sig, true);
    if (id)
        env->SetStaticObjectField(clazz, id, value);
}

template <>
Q_CORE_EXPORT void QJNIObjectPrivate::setStaticField<jobject>(const char *className,
                                                const char *fieldName,
                                                const char *sig,
                                                jobject value)
{
    QJNIEnvironmentPrivate env;
    jclass clazz = loadClass(className, env);
    if (clazz == 0)
        return;

    jfieldID id = getCachedFieldID(env, clazz, className, fieldName, sig, true);
    if (id == 0)
        return;

    env->SetStaticObjectField(clazz, id, value);
}

QJNIObjectPrivate QJNIObjectPrivate::fromString(const QString &string)
{
    QJNIEnvironmentPrivate env;
    jstring res = env->NewString(reinterpret_cast<const jchar*>(string.constData()),
                                        string.length());
    QJNIObjectPrivate obj(res);
    env->DeleteLocalRef(res);
    return obj;
}

QString QJNIObjectPrivate::toString() const
{
    if (!isValid())
        return QString();

    QJNIObjectPrivate string = callObjectMethod<jstring>("toString");
    return qt_convertJString(static_cast<jstring>(string.object()));
}

bool QJNIObjectPrivate::isClassAvailable(const char *className)
{
    QJNIEnvironmentPrivate env;

    if (!env)
        return false;

    jclass clazz = loadClass(className, env);
    return (clazz != 0);
}

bool QJNIObjectPrivate::isValid() const
{
    return d->m_jobject;
}

QJNIObjectPrivate QJNIObjectPrivate::fromLocalRef(jobject lref)
{
    QJNIObjectPrivate o(lref);
    QJNIEnvironmentPrivate()->DeleteLocalRef(lref);
    return o;
}

bool QJNIObjectPrivate::isSameObject(jobject obj) const
{
    QJNIEnvironmentPrivate env;
    return env->IsSameObject(d->m_jobject, obj);
}

bool QJNIObjectPrivate::isSameObject(const QJNIObjectPrivate &other) const
{
    return isSameObject(other.d->m_jobject);
}

QT_END_NAMESPACE
