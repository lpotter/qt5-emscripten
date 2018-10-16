/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qabstractprintdialog_p.h"
#include "qcoreapplication.h"
#include "qprintdialog.h"
#include "qprinter.h"
#include "private/qprinter_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractPrintDialog
    \brief The QAbstractPrintDialog class provides a base implementation for
    print dialogs used to configure printers.

    \ingroup printing
    \inmodule QtPrintSupport

    This class implements getter and setter functions that are used to
    customize settings shown in print dialogs, but it is not used directly.
    Use QPrintDialog to display a print dialog in your application.

    \sa QPrintDialog, QPrinter
*/

/*!
    \enum QAbstractPrintDialog::PrintRange

    Used to specify the print range selection option.

    \value AllPages All pages should be printed.
    \value Selection Only the selection should be printed.
    \value PageRange The specified page range should be printed.
    \value CurrentPage Only the currently visible page should be printed.

    \sa QPrinter::PrintRange
*/

/*!
    \enum QAbstractPrintDialog::PrintDialogOption

    Used to specify which parts of the print dialog should be visible.

    \value None None of the options are enabled.
    \value PrintToFile The print to file option is enabled.
    \value PrintSelection The print selection option is enabled.
    \value PrintPageRange The page range selection option is enabled.
    \value PrintShowPageSize  Show the page size + margins page only if this is enabled.
    \value PrintCollateCopies The collate copies option is enabled
    \value PrintCurrentPage The print current page option is enabled

    This value is obsolete and does nothing since Qt 4.5:

    \value DontUseSheet In previous versions of Qt, exec() the print dialog
    would create a sheet by default the dialog was given a parent.
    This is no longer supported in Qt 4.5.  If you want to use sheets, use
    QPrintDialog::open() instead.
*/

/*!
    Constructs an abstract print dialog for \a printer with \a parent
    as parent widget.
*/
QAbstractPrintDialog::QAbstractPrintDialog(QPrinter *printer, QWidget *parent)
    : QDialog(*(new QAbstractPrintDialogPrivate), parent)
{
    Q_D(QAbstractPrintDialog);
    setWindowTitle(QCoreApplication::translate("QPrintDialog", "Print"));
    d->setPrinter(printer);
    d->minPage = printer->fromPage();
    int to = printer->toPage();
    d->maxPage = to > 0 ? to : INT_MAX;
}

/*!
     \internal
*/
QAbstractPrintDialog::QAbstractPrintDialog(QAbstractPrintDialogPrivate &ptr,
                                           QPrinter *printer,
                                           QWidget *parent)
    : QDialog(ptr, parent)
{
    Q_D(QAbstractPrintDialog);
    setWindowTitle(QCoreApplication::translate("QPrintDialog", "Print"));
    d->setPrinter(printer);
}

/*!
    \internal
*/
QAbstractPrintDialog::~QAbstractPrintDialog()
{
    Q_D(QAbstractPrintDialog);
    if (d->ownsPrinter)
        delete d->printer;
}

/*!
    Sets the given \a option to be enabled if \a on is true;
    otherwise, clears the given \a option.

    \sa options, testOption()
*/
void QPrintDialog::setOption(PrintDialogOption option, bool on)
{
    auto *d = static_cast<QAbstractPrintDialogPrivate *>(d_ptr.data());
    if (!(d->options & option) != !on)
        setOptions(d->options ^ option);
}

/*!
    Returns \c true if the given \a option is enabled; otherwise, returns
    false.

    \sa options, setOption()
*/
bool QPrintDialog::testOption(PrintDialogOption option) const
{
    auto *d = static_cast<const QAbstractPrintDialogPrivate *>(d_ptr.data());
    return (d->options & option) != 0;
}

/*!
    \property QPrintDialog::options
    \brief the various options that affect the look and feel of the dialog
    \since 4.5

    By default, all options are disabled.

    Options should be set before showing the dialog. Setting them while the
    dialog is visible is not guaranteed to have an immediate effect on the
    dialog (depending on the option and on the platform).

    \sa setOption(), testOption()
*/
void QPrintDialog::setOptions(PrintDialogOptions options)
{
    auto *d = static_cast<QAbstractPrintDialogPrivate *>(d_ptr.data());

    PrintDialogOptions changed = (options ^ d->options);
    if (!changed)
        return;

    d->options = options;
}

QPrintDialog::PrintDialogOptions QPrintDialog::options() const
{
    auto *d = static_cast<const QAbstractPrintDialogPrivate *>(d_ptr.data());
    return d->options;
}

/*!
    \obsolete

    Use QPrintDialog::setOptions() instead.
*/
void QAbstractPrintDialog::setEnabledOptions(PrintDialogOptions options)
{
    Q_D(QAbstractPrintDialog);
    d->options = options;
}

/*!
    \obsolete

    Use QPrintDialog::setOption(\a option, true) instead.
*/
void QAbstractPrintDialog::addEnabledOption(PrintDialogOption option)
{
    Q_D(QAbstractPrintDialog);
    d->options |= option;
}

/*!
    \obsolete

    Use QPrintDialog::options() instead.
*/
QAbstractPrintDialog::PrintDialogOptions QAbstractPrintDialog::enabledOptions() const
{
    Q_D(const QAbstractPrintDialog);
    return d->options;
}

/*!
    \obsolete

    Use QPrintDialog::testOption(\a option) instead.
*/
bool QAbstractPrintDialog::isOptionEnabled(PrintDialogOption option) const
{
    Q_D(const QAbstractPrintDialog);
    return d->options & option;
}

/*!
    Sets the print range option in to be \a range.
 */
void QAbstractPrintDialog::setPrintRange(PrintRange range)
{
    Q_D(QAbstractPrintDialog);
    d->printer->setPrintRange(QPrinter::PrintRange(range));
}

/*!
    Returns the print range.
*/
QAbstractPrintDialog::PrintRange QAbstractPrintDialog::printRange() const
{
    Q_D(const QAbstractPrintDialog);
    return QAbstractPrintDialog::PrintRange(d->pd->printRange);
}

/*!
    Sets the page range in this dialog to be from \a min to \a max. This also
    enables the PrintPageRange option.
*/
void QAbstractPrintDialog::setMinMax(int min, int max)
{
    Q_D(QAbstractPrintDialog);
    Q_ASSERT_X(min <= max, "QAbstractPrintDialog::setMinMax",
               "'min' must be less than or equal to 'max'");
    d->minPage = min;
    d->maxPage = max;
    d->options |= PrintPageRange;
}

/*!
    Returns the minimum page in the page range.
    By default, this value is set to 1.
*/
int QAbstractPrintDialog::minPage() const
{
    Q_D(const QAbstractPrintDialog);
    return d->minPage;
}

/*!
    Returns the maximum page in the page range. As of Qt 4.4, this
    function returns INT_MAX by default. Previous versions returned 1
    by default.
*/
int QAbstractPrintDialog::maxPage() const
{
    Q_D(const QAbstractPrintDialog);
    return d->maxPage;
}

/*!
    Sets the range in the print dialog to be from \a from to \a to.
*/
void QAbstractPrintDialog::setFromTo(int from, int to)
{
    Q_D(QAbstractPrintDialog);
    Q_ASSERT_X(from <= to, "QAbstractPrintDialog::setFromTo",
               "'from' must be less than or equal to 'to'");
    d->printer->setFromTo(from, to);

    if (d->minPage == 0 && d->maxPage == 0)
        setMinMax(1, to);
}

/*!
    Returns the first page to be printed
    By default, this value is set to 0.
*/
int QAbstractPrintDialog::fromPage() const
{
    Q_D(const QAbstractPrintDialog);
    return d->printer->fromPage();
}

/*!
    Returns the last page to be printed.
    By default, this value is set to 0.
*/
int QAbstractPrintDialog::toPage() const
{
    Q_D(const QAbstractPrintDialog);
    return d->printer->toPage();
}


/*!
    Returns the printer that this printer dialog operates
    on.
*/
QPrinter *QAbstractPrintDialog::printer() const
{
    Q_D(const QAbstractPrintDialog);
    return d->printer;
}

void QAbstractPrintDialogPrivate::setPrinter(QPrinter *newPrinter)
{
    if (newPrinter) {
        printer = newPrinter;
        ownsPrinter = false;
        if (printer->fromPage() || printer->toPage())
            options |= QAbstractPrintDialog::PrintPageRange;
    } else {
        printer = new QPrinter;
        ownsPrinter = true;
    }
    pd = printer->d_func();
}

/*!
    \fn int QAbstractPrintDialog::exec()

    This virtual function is called to pop up the dialog. It must be
    reimplemented in subclasses.
*/

/*!
    \class QPrintDialog

    \brief The QPrintDialog class provides a dialog for specifying
    the printer's configuration.

    \ingroup standard-dialogs
    \ingroup printing
    \inmodule QtPrintSupport

    The dialog allows users to change document-related settings, such
    as the paper size and orientation, type of print (color or
    grayscale), range of pages, and number of copies to print.

    Controls are also provided to enable users to choose from the
    printers available, including any configured network printers.

    Typically, QPrintDialog objects are constructed with a QPrinter
    object, and executed using the exec() function.

    \snippet code/src_gui_dialogs_qabstractprintdialog.cpp 0

    If the dialog is accepted by the user, the QPrinter object is
    correctly configured for printing.

    \table
    \row
    \li \inlineimage plastique-printdialog.png
    \li \inlineimage plastique-printdialog-properties.png
    \endtable

    The printer dialog (shown above in Plastique style) enables access to common
    printing properties. On X11 platforms that use the CUPS printing system, the
    settings for each available printer can be modified via the dialog's
    \uicontrol{Properties} push button.

    On Windows and \macos, the native print dialog is used, which means that
    some QWidget and QDialog properties set on the dialog won't be respected.
    The native print dialog on \macos does not support setting printer options,
    i.e. setOptions() and setOption() have no effect.

    In Qt 4.4, it was possible to use the static functions to show a sheet on
    \macos. This is no longer supported in Qt 4.5. If you want this
    functionality, use QPrintDialog::open().

    \sa QPageSetupDialog, QPrinter
*/

/*!
    \fn QPrintDialog::QPrintDialog(QPrinter *printer, QWidget *parent)

    Constructs a new modal printer dialog for the given \a printer
    with the given \a parent.
*/

/*!
    \fn QPrintDialog::~QPrintDialog()

    Destroys the print dialog.
*/

/*!
    \fn int QPrintDialog::exec()
    \reimp
*/

/*!
    \since 4.4

    Set a list of widgets as \a tabs to be shown on the print dialog, if supported.

    Currently this option is only supported on X11.

    Setting the option tabs will transfer their ownership to the print dialog.
*/
void QAbstractPrintDialog::setOptionTabs(const QList<QWidget*> &tabs)
{
    Q_D(QAbstractPrintDialog);
    d->setTabs(tabs);
}

/*!

    \fn void QPrintDialog::accepted(QPrinter *printer)

    This signal is emitted when the user accepts the values set in the print dialog.
    The \a printer parameter includes the printer that the settings were applied to.
*/

/*!
    \fn QPrinter *QPrintDialog::printer()

    Returns the printer that this printer dialog operates
    on. This can be useful when using the QPrintDialog::open() method.
*/

/*!
  Closes the dialog and sets its result code to \a result. If this dialog
  is shown with exec(), done() causes the local event loop to finish,
  and exec() to return \a result.

  \note This function does not apply to the Native Print Dialog on the Mac
  \macos and Windows platforms, because the dialog is required to be modal
  and only the user can close it.

  \sa QDialog::done()
*/
void QPrintDialog::done(int result)
{
    auto *d = static_cast<QAbstractPrintDialogPrivate *>(d_ptr.data());
    QDialog::done(result);
    if (result == Accepted)
        emit accepted(printer());
    if (d->receiverToDisconnectOnClose) {
        disconnect(this, SIGNAL(accepted(QPrinter*)),
                   d->receiverToDisconnectOnClose, d->memberToDisconnectOnClose);
        d->receiverToDisconnectOnClose = nullptr;
    }
    d->memberToDisconnectOnClose.clear();
}

/*!
    \since 4.5
    \overload

    Opens the dialog and connects its accepted() signal to the slot specified
    by \a receiver and \a member.

    The signal will be disconnected from the slot when the dialog is closed.
*/
void QPrintDialog::open(QObject *receiver, const char *member)
{
    auto *d = static_cast<QAbstractPrintDialogPrivate *>(d_ptr.data());
    connect(this, SIGNAL(accepted(QPrinter*)), receiver, member);
    d->receiverToDisconnectOnClose = receiver;
    d->memberToDisconnectOnClose = member;
    QDialog::open();
}

QT_END_NAMESPACE
