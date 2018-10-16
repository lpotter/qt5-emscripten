/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qloggingcategory.h"
#include "qloggingregistry_p.h"

QT_BEGIN_NAMESPACE

const char qtDefaultCategoryName[] = "default";

Q_GLOBAL_STATIC_WITH_ARGS(QLoggingCategory, qtDefaultCategory,
                          (qtDefaultCategoryName))

#ifndef Q_ATOMIC_INT8_IS_SUPPORTED
static void setBoolLane(QBasicAtomicInt *atomic, bool enable, int shift)
{
    const int bit = 1 << shift;

    if (enable)
        atomic->fetchAndOrRelaxed(bit);
    else
        atomic->fetchAndAndRelaxed(~bit);
}
#endif

/*!
    \class QLoggingCategory
    \inmodule QtCore
    \since 5.2
    \threadsafe

    \brief The QLoggingCategory class represents a category, or 'area' in the
    logging infrastructure.

    QLoggingCategory represents a certain logging category - identified by a
    string - at runtime. A category can be configured to enable or disable
    logging of messages per message type. Whether a message type is enabled or
    not can be checked with the \l isDebugEnabled(), \l isInfoEnabled(),
    \l isWarningEnabled(), and \l isCriticalEnabled() methods.

    All objects are meant to be configured by a common registry (see also
    \l{Configuring Categories}). Different objects can also represent the same
    category. It is therefore not recommended to export objects across module
    boundaries, nor to manipulate the objects directly, nor to inherit from
    QLoggingCategory.

    \section1 Creating Category Objects

    The Q_DECLARE_LOGGING_CATEGORY() and Q_LOGGING_CATEGORY() macros
    conveniently declare and create QLoggingCategory objects:

    \snippet qloggingcategory/main.cpp 1

    \note Category names are free text. However, to allow easy configuration
    of the categories using \l{Logging Rules} the names should follow some rules:
    \list
       \li Use letters and numbers only.
       \li Further structure categories into common areas by using dots.
       \li Avoid the category names \c{debug}, \c{info}, \c{warning}, and \c{critical}.
       \li Category names starting with \c{qt} are reserved for Qt modules.
    \endlist

    QLoggingCategory objects implicitly defined by Q_LOGGING_CATEGORY()
    are created on first use in a thread-safe manner.

    \section1 Checking Category Configuration

    QLoggingCategory provides \l isDebugEnabled(), \l isInfoEnabled(),
    \l isWarningEnabled(), \l isCriticalEnabled(), as well as \l isEnabled()
    to check whether messages for the given message type should be logged.

    \note The qCDebug(), qCWarning(), qCCritical() macros prevent arguments
    from being evaluated if the respective message types are not enabled for the
    category, so explicit checking is not needed:

    \snippet qloggingcategory/main.cpp 4

    \section1 Default Category Configuration

    Both the QLoggingCategory constructor and the Q_LOGGING_CATEGORY() macro
    accept an optional QtMsgType argument, which disables all message types with
    a lower severity. That is, a category declared with

    \snippet qloggingcategory/main.cpp 5

    will log messages of type \c QtWarningMsg, \c QtCriticalMsg, \c QtFatalMsg, but will
    ignore messages of type \c QtDebugMsg and \c QtInfoMsg.

    If no argument is passed, all messages will be logged.

    \section1 Configuring Categories

    The default configuration of categories can be overridden either by setting logging
    rules, or by installing a custom filter.

    \section2 Logging Rules

    Logging rules allow logging for categories to be enabled or disabled in a
    flexible way. Rules are specified in text, where every line must have the
    format

    \code
    <category>[.<type>] = true|false
    \endcode

    \c <category> is the name of the category, potentially with \c{*} as a
    wildcard symbol as the first or last character (or at both positions).
    The optional \c <type> must be either \c debug, \c info, \c warning, or \c critical.
    Lines that do not fit this scheme are ignored.

    Rules are evaluated in text order, from first to last. That is, if two rules
    apply to a category/type, the rule that comes later is applied.

    Rules can be set via \l setFilterRules():

    \code
    QLoggingCategory::setFilterRules("*.debug=false\n"
                                     "driver.usb.debug=true");
    \endcode

    Since Qt 5.3, logging rules are also
    automatically loaded from the \c [Rules] section of a logging
    configuration file. Such configuration files are looked up in the QtProject
    configuration directory, or explicitly set in a \c QT_LOGGING_CONF
    environment variable:

    \code
    [Rules]
    *.debug=false
    driver.usb.debug=true
    \endcode

    Since Qt 5.3, logging rules can also be specified in a \c QT_LOGGING_RULES
    environment variable. And since Qt 5.6, multiple rules can also be
    separated by semicolons:

    \code
    QT_LOGGING_RULES="*.debug=false;driver.usb.debug=true"
    \endcode

    Rules set by \l setFilterRules() take precedence over rules specified
    in the QtProject configuration directory, and can, in turn, be
    overwritten by rules from the configuration file specified by
    \c QT_LOGGING_CONF, and rules set by \c QT_LOGGING_RULES.

    Order of evaluation:
    \list
    \li [QLibraryInfo::DataPath]/qtlogging.ini
    \li QtProject/qtlogging.ini
    \li \l setFilterRules()
    \li \c QT_LOGGING_CONF
    \li \c QT_LOGGING_RULES
    \endlist

    The \c QtProject/qtlogging.ini file is looked up in all directories returned
    by QStandardPaths::GenericConfigLocation, e.g.

    \list
    \li on \macos and iOS: \c ~/Library/Preferences
    \li on Unix: \c ~/.config, \c /etc/xdg
    \li on Windows: \c %LOCALAPPDATA%, \c %ProgramData%,
        \l QCoreApplication::applicationDirPath(),
        QCoreApplication::applicationDirPath() + \c "/data"
    \endlist

    Set the \c QT_LOGGING_DEBUG environment variable to see from where
    logging rules are loaded.

    \section2 Installing a Custom Filter

    As a lower-level alternative to the text rules, you can also implement a
    custom filter via \l installFilter(). All filter rules are ignored in this
    case.

    \section1 Printing the Category

    Use the \c %{category} placeholder to print the category in the default
    message handler:

    \snippet qloggingcategory/main.cpp 3
*/

/*!
    Constructs a QLoggingCategory object with the provided \a category name.
    All message types for this category are enabled by default.

    If \a category is \c{0}, the category name is changed to \c "default".

    Note that \a category must be kept valid during the lifetime of this object.
*/
QLoggingCategory::QLoggingCategory(const char *category)
    : d(0),
      name(0)
{
    init(category, QtDebugMsg);
}

/*!
    Constructs a QLoggingCategory object with the provided \a category name,
    and enables all messages with types more severe or equal than \a enableForLevel.

    If \a category is \c{0}, the category name is changed to \c "default".

    Note that \a category must be kept valid during the lifetime of this object.

    \since 5.4
*/
QLoggingCategory::QLoggingCategory(const char *category, QtMsgType enableForLevel)
    : d(0),
      name(0)
{
    init(category, enableForLevel);
}

void QLoggingCategory::init(const char *category, QtMsgType severityLevel)
{
    enabled.store(0x01010101);   // enabledDebug = enabledWarning = enabledCritical = true;

    if (category)
        name = category;
    else
        name = qtDefaultCategoryName;

    if (QLoggingRegistry *reg = QLoggingRegistry::instance())
        reg->registerCategory(this, severityLevel);
}

/*!
    Destructs a QLoggingCategory object.
*/
QLoggingCategory::~QLoggingCategory()
{
    if (QLoggingRegistry *reg = QLoggingRegistry::instance())
        reg->unregisterCategory(this);
}

/*!
   \fn const char *QLoggingCategory::categoryName() const

    Returns the name of the category.
*/

/*!
    \fn bool QLoggingCategory::isDebugEnabled() const

    Returns \c true if debug messages should be shown for this category.
    Returns \c false otherwise.

    \note The \l qCDebug() macro already does this check before executing any
    code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.
*/


/*!
    \fn bool QLoggingCategory::isInfoEnabled() const

    Returns \c true if informational messages should be shown for this category.
    Returns \c false otherwise.

    \note The \l qCInfo() macro already does this check before executing any
    code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.

    \since 5.5
*/


/*!
    \fn bool QLoggingCategory::isWarningEnabled() const

    Returns \c true if warning messages should be shown for this category.
    Returns \c false otherwise.

    \note The \l qCWarning() macro already does this check before executing any
    code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.
*/

/*!
    \fn bool QLoggingCategory::isCriticalEnabled() const

    Returns \c true if critical messages should be shown for this category.
    Returns \c false otherwise.

    \note The \l qCCritical() macro already does this check before executing any
    code. However, calling this method may be useful to avoid
    expensive generation of data that is only used for debug output.
*/

/*!
    Returns \c true if a message of type \a msgtype for the category should be
    shown. Returns \c false otherwise.
*/
bool QLoggingCategory::isEnabled(QtMsgType msgtype) const
{
    switch (msgtype) {
    case QtDebugMsg: return isDebugEnabled();
    case QtInfoMsg: return isInfoEnabled();
    case QtWarningMsg: return isWarningEnabled();
    case QtCriticalMsg: return isCriticalEnabled();
    case QtFatalMsg: return true;
    }
    return false;
}

/*!
    Changes the message type \a type for the category to \a enable.

    This method is meant to be used only from inside a filter
    installed by \l installFilter(). See \l {Configuring Categories} for
    an overview on how to configure categories globally.

    \note \c QtFatalMsg cannot be changed. It will always remain \c true.
*/
void QLoggingCategory::setEnabled(QtMsgType type, bool enable)
{
    switch (type) {
#ifdef Q_ATOMIC_INT8_IS_SUPPORTED
    case QtDebugMsg: bools.enabledDebug.store(enable); break;
    case QtInfoMsg: bools.enabledInfo.store(enable); break;
    case QtWarningMsg: bools.enabledWarning.store(enable); break;
    case QtCriticalMsg: bools.enabledCritical.store(enable); break;
#else
    case QtDebugMsg: setBoolLane(&enabled, enable, DebugShift); break;
    case QtInfoMsg: setBoolLane(&enabled, enable, InfoShift); break;
    case QtWarningMsg: setBoolLane(&enabled, enable, WarningShift); break;
    case QtCriticalMsg: setBoolLane(&enabled, enable, CriticalShift); break;
#endif
    case QtFatalMsg: break;
    }
}

/*!
    \fn QLoggingCategory &QLoggingCategory::operator()()

    Returns the object itself. This allows both a QLoggingCategory variable, and
    a factory method returning a QLoggingCategory, to be used in \l qCDebug(),
    \l qCWarning(), \l qCCritical() macros.
 */

/*!
    \fn const QLoggingCategory &QLoggingCategory::operator()() const

    Returns the object itself. This allows both a QLoggingCategory variable, and
    a factory method returning a QLoggingCategory, to be used in \l qCDebug(),
    \l qCWarning(), \l qCCritical() macros.
 */

/*!
    Returns a pointer to the global category \c "default" that
    is used e.g. by qDebug(), qInfo(), qWarning(), qCritical(), qFatal().

    \note The returned pointer may be null during destruction of
    static objects.

    \note Ownership of the category is not transferred, do not
    \c delete the returned pointer.

 */
QLoggingCategory *QLoggingCategory::defaultCategory()
{
    return qtDefaultCategory();
}

/*!
    \typedef QLoggingCategory::CategoryFilter

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet qloggingcategory/main.cpp 20

    A function with this signature can be installed with \l installFilter().
*/

/*!
    Installs a function \a filter that is used to determine which categories
    and message types should be enabled. Returns a pointer to the previous
    installed filter.

    Every QLoggingCategory object created is passed to the filter, and the
    filter is free to change the respective category configuration with
    \l setEnabled().

    The filter might be called from different threads, but never concurrently.
    The filter shall not call any static functions of QLoggingCategory.

    Example:
    \snippet qloggingcategory/main.cpp 21

    An alternative way of configuring the default filter is via
    \l setFilterRules().
 */
QLoggingCategory::CategoryFilter
QLoggingCategory::installFilter(QLoggingCategory::CategoryFilter filter)
{
    return QLoggingRegistry::instance()->installFilter(filter);
}

/*!
    Configures which categories and message types should be enabled through a
    a set of \a rules.

    Example:

    \snippet qloggingcategory/main.cpp 2

    \note The rules might be ignored if a custom category filter is installed
    with \l installFilter(), or if the user defined \c QT_LOGGING_CONF or \c QT_LOGGING_RULES
    environment variable.
*/
void QLoggingCategory::setFilterRules(const QString &rules)
{
    QLoggingRegistry::instance()->setApiRules(rules);
}

/*!
    \macro qCDebug(category)
    \relates QLoggingCategory
    \threadsafe
    \since 5.2

    Returns an output stream for debug messages in the logging category
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isDebugEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 10

    \note Arguments are not processed if debug output for the category is not
    enabled, so do not rely on any side effects.

    \sa qDebug()
*/

/*!
    \macro qCDebug(category, const char *message, ...)
    \relates QLoggingCategory
    \threadsafe
    \since 5.3

    Logs a debug message \a message in the logging category \a category.
    \a message might contain place holders that are replaced by additional
    arguments, similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp 13

    \note Arguments might not be processed if debug output for the category is
    not enabled, so do not rely on any side effects.

    \sa qDebug()
*/

/*!
    \macro qCInfo(category)
    \relates QLoggingCategory
    \threadsafe
    \since 5.5

    Returns an output stream for informational messages in the logging category
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isInfoEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp qcinfo_stream

    \note Arguments are not processed if debug output for the category is not
    enabled, so do not rely on any side effects.

    \sa qInfo()
*/

/*!
    \macro qCInfo(category, const char *message, ...)
    \relates QLoggingCategory
    \threadsafe
    \since 5.5

    Logs an informational message \a message in the logging category \a category.
    \a message might contain place holders that are replaced by additional
    arguments, similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp qcinfo_printf

    \note Arguments might not be processed if debug output for the category is
    not enabled, so do not rely on any side effects.

    \sa qInfo()
*/

/*!
    \macro qCWarning(category)
    \relates QLoggingCategory
    \threadsafe
    \since 5.2

    Returns an output stream for warning messages in the logging category
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isWarningEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 11

    \note Arguments are not processed if warning output for the category is not
    enabled, so do not rely on any side effects.

    \sa qWarning()
*/

/*!
    \macro qCWarning(category, const char *message, ...)
    \relates QLoggingCategory
    \threadsafe
    \since 5.3

    Logs a warning message \a message in the logging category \a category.
    \a message might contain place holders that are replaced by additional
    arguments, similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp 14

    \note Arguments might not be processed if warning output for the category is
    not enabled, so do not rely on any side effects.

    \sa qWarning()
*/

/*!
    \macro qCCritical(category)
    \relates QLoggingCategory
    \threadsafe
    \since 5.2

    Returns an output stream for critical messages in the logging category
    \a category.

    The macro expands to code that checks whether
    \l QLoggingCategory::isCriticalEnabled() evaluates to \c true.
    If so, the stream arguments are processed and sent to the message handler.

    Example:

    \snippet qloggingcategory/main.cpp 12

    \note Arguments are not processed if critical output for the category is not
    enabled, so do not rely on any side effects.

    \sa qCritical()
*/

/*!
    \macro qCCritical(category, const char *message, ...)
    \relates QLoggingCategory
    \threadsafe
    \since 5.3

    Logs a critical message \a message in the logging category \a category.
    \a message might contain place holders that are replaced by additional
    arguments, similar to the C printf() function.

    Example:

    \snippet qloggingcategory/main.cpp 15

    \note Arguments might not be processed if critical output for the category
    is not enabled, so do not rely on any side effects.

    \sa qCritical()
*/
/*!
    \macro Q_DECLARE_LOGGING_CATEGORY(name)
    \sa Q_LOGGING_CATEGORY()
    \relates QLoggingCategory
    \since 5.2

    Declares a logging category \a name. The macro can be used to declare
    a common logging category shared in different parts of the program.

    This macro must be used outside of a class or method.
*/

/*!
    \macro Q_LOGGING_CATEGORY(name, string)
    \sa Q_DECLARE_LOGGING_CATEGORY()
    \relates QLoggingCategory
    \since 5.2

    Defines a logging category \a name, and makes it configurable under the
    \a string identifier. By default, all message types are enabled.

    Only one translation unit in a library or executable can define a category
    with a specific name. The implicitly defined QLoggingCategory object is
    created on first use, in a thread-safe manner.

    This macro must be used outside of a class or method.
*/

/*!
    \macro Q_LOGGING_CATEGORY(name, string, msgType)
    \sa Q_DECLARE_LOGGING_CATEGORY()
    \relates QLoggingCategory
    \since 5.4

    Defines a logging category \a name, and makes it configurable under the
    \a string identifier. By default, messages of QtMsgType \a msgType
    and more severe are enabled, types with a lower severity are disabled.

    Only one translation unit in a library or executable can define a category
    with a specific name. The implicitly defined QLoggingCategory object is
    created on first use, in a thread-safe manner.

    This macro must be used outside of a class or method. It is only defined
    if variadic macros are supported.
*/

QT_END_NAMESPACE
