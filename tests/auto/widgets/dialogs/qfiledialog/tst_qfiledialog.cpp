/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qfile.h>
#include <qdebug.h>
#include <qsharedpointer.h>
#include <qfiledialog.h>
#include <qabstractitemdelegate.h>
#include <qdirmodel.h>
#include <qitemdelegate.h>
#include <qlistview.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qtreeview.h>
#include <qheaderview.h>
#include <qcompleter.h>
#include <qaction.h>
#include <qdialogbuttonbox.h>
#include <qsortfilterproxymodel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qtemporarydir.h>
#include <private/qfiledialog_p.h>
#if defined QT_BUILD_INTERNAL
#include <private/qsidebar_p.h>
#include <private/qfilesystemmodel_p.h>
#endif
#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>
#include <QFileDialog>
#include <QFileSystemModel>

#if defined(Q_OS_UNIX)
#include <unistd.h> // for pathconf() on OS X
#ifdef QT_BUILD_INTERNAL
QT_BEGIN_NAMESPACE
extern Q_GUI_EXPORT QString qt_tildeExpansion(const QString &path);
QT_END_NAMESPACE
#endif
#endif

static inline bool isCaseSensitiveFileSystem(const QString &path)
{
    Q_UNUSED(path)
#if defined(Q_OS_MAC)
    return pathconf(QFile::encodeName(path).constData(), _PC_CASE_SENSITIVE);
#elif defined(Q_OS_WIN)
    return false;
#else
    return true;
#endif
}

class tst_QFiledialog : public QObject
{
Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void currentChangedSignal();
#ifdef QT_BUILD_INTERNAL
    void directoryEnteredSignal();
#endif
    void filesSelectedSignal_data();
    void filesSelectedSignal();
    void filterSelectedSignal();

    void args();
    void directory();
    void completer_data();
    void completer();
    void completer_up();
    void acceptMode();
    void confirmOverwrite();
    void defaultSuffix();
    void fileMode();
    void filters();
    void history();
    void iconProvider();
    void isReadOnly();
    void itemDelegate();
    void labelText();
    void resolveSymlinks();
    void selectFile_data();
    void selectFile();
    void selectFiles();
    void selectFileWrongCaseSaveAs();
    void selectFilter();
    void viewMode();
    void proxymodel();
    void setMimeTypeFilters_data();
    void setMimeTypeFilters();
    void setNameFilter_data();
    void setNameFilter();
    void setEmptyNameFilter();
    void focus();
    void caption();
    void historyBack();
    void historyForward();
    void disableSaveButton_data();
    void disableSaveButton();
    void saveButtonText_data();
    void saveButtonText();
    void clearLineEdit();
    void enableChooseButton();
    void selectedFilesWithoutWidgets();
    void trailingDotsAndSpaces();
#ifdef Q_OS_UNIX
#ifdef QT_BUILD_INTERNAL
    void tildeExpansion_data();
    void tildeExpansion();
#endif // QT_BUILD_INTERNAL
#endif
    void rejectModalDialogs();
    void QTBUG49600_nativeIconProviderCrash();
    void focusObjectDuringDestruction();

    // NOTE: Please keep widgetlessNativeDialog() as the LAST test!
    //
    // widgetlessNativeDialog() is the only test function that creates
    // a native file dialog instance. GTK+ versions prior 3.15.5 have
    // a nasty bug (https://bugzilla.gnome.org/show_bug.cgi?id=725164)
    // in GtkFileChooserWidget, which makes it leak its folder change
    // callback, causing a crash "at some point later". Running the
    // native test last is enough to avoid spinning the event loop after
    // the test, and that way circumvent the crash.
    //
    // The crash has been fixed in GTK+ 3.15.5, but the RHEL 7.2 CI has
    // GTK+ 3.14.13 installed (QTBUG-55276).
    void widgetlessNativeDialog();

private:
    void cleanupSettingsFile();
};

void tst_QFiledialog::cleanupSettingsFile()
{
    // clean up the sidebar between each test
    QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
    settings.beginGroup(QLatin1String("FileDialog"));
    settings.remove(QString());
    settings.endGroup();
    settings.beginGroup(QLatin1String("Qt")); // Compatibility settings
    settings.remove(QLatin1String("filedialog"));
    settings.endGroup();
}

void tst_QFiledialog::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    cleanupSettingsFile();
}

void tst_QFiledialog::init()
{
    // all tests, except widgetlessNativeDialog, use non-native dialogs
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QFileDialogPrivate::setLastVisitedDirectory(QUrl());
    // populate the sidebar with some default settings
    QFileDialog fd;
}

void tst_QFiledialog::cleanup()
{
    cleanupSettingsFile();
}

class MyAbstractItemDelegate : public QAbstractItemDelegate
{
public:
    MyAbstractItemDelegate() : QAbstractItemDelegate() {};
    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const {}
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const { return QSize(); }
};

// emitted any time the selection model emits current changed
void tst_QFiledialog::currentChangedSignal()
{
    QFileDialog fd;
    fd.setViewMode(QFileDialog::List);
    QSignalSpy spyCurrentChanged(&fd, SIGNAL(currentChanged(QString)));

    QListView* listView = fd.findChild<QListView*>("listView");
    QVERIFY(listView);
    fd.setDirectory(QDir::root());
    QModelIndex root = listView->rootIndex();
    QTRY_COMPARE(listView->model()->rowCount(root) > 0, true);

    QModelIndex folder;
    for (int i = 0; i < listView->model()->rowCount(root); ++i) {
        folder = listView->model()->index(i, 0, root);
        if (listView->model()->hasChildren(folder))
            break;
    }
    QVERIFY(listView->model()->hasChildren(folder));
    listView->setCurrentIndex(folder);

    QCOMPARE(spyCurrentChanged.count(), 1);
}

// only emitted from the views, sidebar, or lookin combo
#if defined QT_BUILD_INTERNAL
void tst_QFiledialog::directoryEnteredSignal()
{
    QFileDialog fd(0, "", QDir::root().path());
    QSidebar *sidebar = fd.findChild<QSidebar*>("sidebar");
    QVERIFY(sidebar);
    if (sidebar->model()->rowCount() < 2)
        QSKIP("This test requires at least 2 side bar entries.");

    fd.show();
    QTRY_COMPARE(fd.isVisible(), true);
    QSignalSpy spyDirectoryEntered(&fd, SIGNAL(directoryEntered(QString)));

    // sidebar
    QModelIndex secondItem = sidebar->model()->index(1, 0);
    QVERIFY(secondItem.isValid());
    sidebar->setCurrentIndex(secondItem);
    QTest::keyPress(sidebar->viewport(), Qt::Key_Return);
    QCOMPARE(spyDirectoryEntered.count(), 1);
    spyDirectoryEntered.clear();

    // lookInCombo
    QComboBox *comboBox = fd.findChild<QComboBox*>("lookInCombo");
    comboBox->showPopup();
    QVERIFY(comboBox->view()->model()->index(1, 0).isValid());
    comboBox->view()->setCurrentIndex(comboBox->view()->model()->index(1, 0));
    QTest::keyPress(comboBox->view()->viewport(), Qt::Key_Return);
    QCOMPARE(spyDirectoryEntered.count(), 1);
    spyDirectoryEntered.clear();

    // view
    /*
    // platform specific
    fd.setViewMode(QFileDialog::ViewMode(QFileDialog::List));
    QListView* listView = fd.findChild<QListView*>("listView");
    QVERIFY(listView);
    QModelIndex root = listView->rootIndex();
    QTRY_COMPARE(listView->model()->rowCount(root) > 0, true);

    QModelIndex folder;
    for (int i = 0; i < listView->model()->rowCount(root); ++i) {
        folder = listView->model()->index(i, 0, root);
        if (listView->model()->hasChildren(folder))
            break;
    }
    QVERIFY(listView->model()->hasChildren(folder));
    listView->setCurrentIndex(folder);
    QTRY_COMPARE((listView->indexAt(listView->visualRect(folder).center())), folder);
    QTest::mouseDClick(listView->viewport(), Qt::LeftButton, 0, listView->visualRect(folder).center());
    QTRY_COMPARE(spyDirectoryEntered.count(), 1);
    */
}
#endif

Q_DECLARE_METATYPE(QFileDialog::FileMode)
void tst_QFiledialog::filesSelectedSignal_data()
{
    QTest::addColumn<QFileDialog::FileMode>("fileMode");
    QTest::newRow("any") << QFileDialog::AnyFile;
    QTest::newRow("existing") << QFileDialog::ExistingFile;
    QTest::newRow("directory") << QFileDialog::Directory;
    QTest::newRow("directoryOnly") << QFileDialog::DirectoryOnly;
    QTest::newRow("existingFiles") << QFileDialog::ExistingFiles;
}

// emitted when the dialog closes with the selected files
void tst_QFiledialog::filesSelectedSignal()
{
    QFileDialog fd;
    fd.setViewMode(QFileDialog::List);
    QDir testDir(SRCDIR);
    fd.setDirectory(testDir);
    QFETCH(QFileDialog::FileMode, fileMode);
    fd.setFileMode(fileMode);
    QSignalSpy spyFilesSelected(&fd, SIGNAL(filesSelected(QStringList)));

    fd.show();
    QVERIFY(QTest::qWaitForWindowExposed(&fd));
    QListView *listView = fd.findChild<QListView*>("listView");
    QVERIFY(listView);

    QModelIndex root = listView->rootIndex();
    QTRY_COMPARE(listView->model()->rowCount(root) > 0, true);
    QModelIndex file;
    for (int i = 0; i < listView->model()->rowCount(root); ++i) {
        file = listView->model()->index(i, 0, root);
        if (fileMode == QFileDialog::Directory || fileMode == QFileDialog::DirectoryOnly) {
            if (listView->model()->hasChildren(file))
                break;
        } else {
            if (!listView->model()->hasChildren(file))
                break;
        }
        file = QModelIndex();
    }
    QVERIFY(file.isValid());
    listView->selectionModel()->select(file, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    listView->setCurrentIndex(file);

    QDialogButtonBox *buttonBox = fd.findChild<QDialogButtonBox*>("buttonBox");
    QPushButton *button = buttonBox->button(QDialogButtonBox::Open);
    QVERIFY(button);
    QVERIFY(button->isEnabled());
    button->animateClick();
    QTRY_COMPARE(fd.isVisible(), false);
    QCOMPARE(spyFilesSelected.count(), 1);
}

// only emitted when the combo box is activated
void tst_QFiledialog::filterSelectedSignal()
{
    QFileDialog fd;
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.show();
    QSignalSpy spyFilterSelected(&fd, SIGNAL(filterSelected(QString)));

    QStringList filterChoices;
    filterChoices << "Image files (*.png *.xpm *.jpg)"
                  << "Text files (*.txt)"
                  << "Any files (*.*)";
    fd.setNameFilters(filterChoices);
    QCOMPARE(fd.nameFilters(), filterChoices);

    QComboBox *filters = fd.findChild<QComboBox*>("fileTypeCombo");
    QVERIFY(filters);
    QVERIFY(filters->view());
    QCOMPARE(filters->isVisible(), true);

    QTest::keyPress(filters, Qt::Key_Down);

    QCOMPARE(spyFilterSelected.count(), 1);
}

void tst_QFiledialog::args()
{
    QWidget *parent = 0;
    QString caption = "caption";
    QString directory = QDir::tempPath();
    QString filter = "*.mp3";
    QFileDialog fd(parent, caption, directory, filter);
    QCOMPARE(fd.parent(), (QObject *)parent);
    QCOMPARE(fd.windowTitle(), caption);
#ifndef Q_OS_WIN
    QCOMPARE(fd.directory(), QDir(directory));
#endif
    QCOMPARE(fd.nameFilters(), QStringList(filter));
}

void tst_QFiledialog::directory()
{
    QFileDialog fd;
    fd.setViewMode(QFileDialog::List);
    QFileSystemModel *model = fd.findChild<QFileSystemModel*>("qt_filesystem_model");
    QVERIFY(model);
    fd.setDirectory(QDir::currentPath());
    QSignalSpy spyCurrentChanged(&fd, SIGNAL(currentChanged(QString)));
    QSignalSpy spyDirectoryEntered(&fd, SIGNAL(directoryEntered(QString)));
    QSignalSpy spyFilesSelected(&fd, SIGNAL(filesSelected(QStringList)));
    QSignalSpy spyFilterSelected(&fd, SIGNAL(filterSelected(QString)));

    QCOMPARE(QDir::current().absolutePath(), fd.directory().absolutePath());
    QDir temp = QDir::temp();
    QString tempPath = temp.absolutePath();
#ifdef Q_OS_WIN
    // since the user can have lowercase temp dir, check that we are actually case-insensitive.
    tempPath = tempPath.toLower();
#endif
    fd.setDirectory(tempPath);
#ifndef Q_OS_WIN
    QCOMPARE(tempPath, fd.directory().absolutePath());
#endif
    QCOMPARE(spyCurrentChanged.count(), 0);
    QCOMPARE(spyDirectoryEntered.count(), 0);
    QCOMPARE(spyFilesSelected.count(), 0);
    QCOMPARE(spyFilterSelected.count(), 0);

    // Check my way
    QList<QListView*> list = fd.findChildren<QListView*>("listView");
    QVERIFY(list.count() > 0);
#ifdef Q_OS_WIN
    QCOMPARE(list.at(0)->rootIndex().data().toString().toLower(), temp.dirName().toLower());
#else
    QCOMPARE(list.at(0)->rootIndex().data().toString(), temp.dirName());
#endif
    QFileDialog *dlg = new QFileDialog(0, "", tempPath);
    QCOMPARE(model->index(tempPath), model->index(dlg->directory().absolutePath()));
    QCOMPARE(model->index(tempPath).data(QFileSystemModel::FileNameRole).toString(),
             model->index(dlg->directory().absolutePath()).data(QFileSystemModel::FileNameRole).toString());
    delete dlg;
    dlg = new QFileDialog();
    QCOMPARE(model->index(tempPath), model->index(dlg->directory().absolutePath()));
    delete dlg;
}

void tst_QFiledialog::completer_data()
{
    QTest::addColumn<QString>("startPath");
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("expected");

    const QString rootPath = QDir::rootPath();

    QTest::newRow("r, 10")   << QString() << "r"   << 10;
    QTest::newRow("x, 0")    << QString() << "x"   << 0;
    QTest::newRow("../, -1") << QString() << "../" << -1;

    QTest::newRow("goto root")     << QString()        << rootPath << -1;
    QTest::newRow("start at root") << rootPath << QString()        << -1;

    QFileInfoList list = QDir::root().entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    QVERIFY(!list.isEmpty());
    const QString folder = list.first().absoluteFilePath();
    QTest::newRow("start at one below root r") << folder << "r" << -1;
    QTest::newRow("start at one below root ../") << folder << "../" << -1;
}

void tst_QFiledialog::completer()
{
    typedef QSharedPointer<QTemporaryFile> TemporaryFilePtr;

#ifdef Q_OS_WIN
    static const Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
#else
    static const Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
#endif

    QFETCH(QString, input);
    QFETCH(QString, startPath);
    QFETCH(int, expected);

    // make temp dir and files
    QScopedPointer<QTemporaryDir> tempDir;
    QList<TemporaryFilePtr> files;

    if (startPath.isEmpty()) {
        tempDir.reset(new QTemporaryDir);
        QVERIFY2(tempDir->isValid(), qPrintable(tempDir->errorString()));
        startPath = tempDir->path();
        for (int i = 0; i < 10; ++i) {
            TemporaryFilePtr file(new QTemporaryFile(startPath + QStringLiteral("/rXXXXXX")));
            QVERIFY2(file->open(), qPrintable(file->errorString()));
            // Force the temporary file to materialize with the requested name
            (void) file->fileName();
            QVERIFY(file->exists());
            files.append(file);
        }
    }

    // ### flesh this out more
    QFileDialog fd(0, QLatin1String(QTest::currentTestFunction())
                            + QStringLiteral(" \"") + QLatin1String(QTest::currentDataTag())
                            + QLatin1Char('"'), startPath);
    fd.show();
    QVERIFY(QTest::qWaitForWindowExposed(&fd));
    QVERIFY(fd.isVisible());
    QFileSystemModel *model = fd.findChild<QFileSystemModel*>("qt_filesystem_model");
    QVERIFY(model);
    QLineEdit *lineEdit = fd.findChild<QLineEdit*>("fileNameEdit");
    QVERIFY(lineEdit);
    QCompleter *completer = lineEdit->completer();
    QVERIFY(completer);
    QAbstractItemModel *cModel = completer->completionModel();
    QVERIFY(cModel);

    // path C:\depot\qt\examples\dialogs\standarddialogs
    // files
    //       [debug] [release] [tmp] dialog dialog main makefile makefile.debug makefile.release standarddialgos
    //
    // d      -> D:\ debug dialog.cpp dialog.h
    // ..\    -> ..\classwizard ..\configdialog ..\dialogs.pro
    // c      -> C:\ control panel
    // c:     -> C:\ (nothing more)
    // C:\    -> C:\, C:\_viminfo, ...
    // \      -> \_viminfo
    // c:\depot  -> 'nothing'
    // c:\depot\ -> C:\depot\devtools, C:\depot\dteske
    QTRY_COMPARE(model->index(fd.directory().path()), model->index(startPath));

    if (input.isEmpty()) {
        // Try to find a suitable directory under root that does not
        // start with 'C' to avoid issues with completing to "C:" drives on Windows.
        const QString rootPath = model->rootPath();
        const QChar rootPathFirstChar = rootPath.at(0).toLower();
        QModelIndex rootIndex = model->index(rootPath);
        const int rowCount = model->rowCount(rootIndex);
        QVERIFY(rowCount > 0);
        for (int row = 0; row < rowCount && input.isEmpty(); ++row) {
            const QString name = model->index(row, 0, rootIndex).data().toString();
            const QChar firstChar = name.at(0);
            if (firstChar.isLetter() && firstChar.toLower() != rootPathFirstChar)
                input = firstChar;
        }
        QVERIFY2(!input.isEmpty(), "Unable to find a suitable input directory");
    }

    // press 'keys' for the input
    for (int i = 0; i < input.count(); ++i)
        QTest::keyPress(lineEdit, input[i].toLatin1());

    QStringList expectedFiles;
    if (expected == -1) {
        QString fullPath = startPath;
        if (!fullPath.endsWith(QLatin1Char('/')))
            fullPath.append(QLatin1Char('/'));
        fullPath.append(input);
        if (input.startsWith(QDir::rootPath(), caseSensitivity)) {
            fullPath = input;
            input.clear();
        }

        QFileInfo fi(fullPath);
        QDir x(fi.absolutePath());
        expectedFiles = x.entryList(model->filter());
        expected = 0;
        if (input.startsWith(".."))
            input.clear();
        foreach (const QString &expectedFile, expectedFiles) {
            if (expectedFile.startsWith(input, caseSensitivity))
                ++expected;
        }
        // The temporary dir may create a node in QFileSystemModel
        // which will bypass filters. If the path to the temporary
        // dir contains an element which should be a subdirectory
        // of x dir, but which is not listed, then take it into
        // accont.
        if (!tempDir.isNull()) {
            QString xPath = x.absolutePath();
            if (!xPath.endsWith(QLatin1Char('/')))
                xPath.append(QLatin1Char('/'));
            QString tmpPath = tempDir->path();
            if (tmpPath.startsWith(xPath)) {
                QString bypassedDirName = tmpPath.mid(xPath.size()).section(QLatin1Char('/'), 0, 0);
                if (!expectedFiles.contains(bypassedDirName))
                    ++expected;
            }
        }
    }

    QTRY_COMPARE(cModel->rowCount(), expected);
}

void tst_QFiledialog::completer_up()
{
    QFileDialog fd;
    QSignalSpy spyCurrentChanged(&fd, SIGNAL(currentChanged(QString)));
    QSignalSpy spyDirectoryEntered(&fd, SIGNAL(directoryEntered(QString)));
    QSignalSpy spyFilesSelected(&fd, SIGNAL(filesSelected(QStringList)));
    QSignalSpy spyFilterSelected(&fd, SIGNAL(filterSelected(QString)));

    fd.show();
    QLineEdit *lineEdit = fd.findChild<QLineEdit*>("fileNameEdit");
    QVERIFY(lineEdit);
    QCOMPARE(spyFilesSelected.count(), 0);
    int depth = QDir::currentPath().split('/').count();
    for (int i = 0; i <= depth * 3 + 1; ++i) {
        lineEdit->insert("../");
        qApp->processEvents();
    }
    QCOMPARE(spyCurrentChanged.count(), 0);
    QCOMPARE(spyDirectoryEntered.count(), 0);
    QCOMPARE(spyFilesSelected.count(), 0);
    QCOMPARE(spyFilterSelected.count(), 0);
}

void tst_QFiledialog::acceptMode()
{
    QFileDialog fd;
    fd.show();

    QToolButton* newButton = fd.findChild<QToolButton*>("newFolderButton");
    QVERIFY(newButton);

    // default
    QCOMPARE(fd.acceptMode(), QFileDialog::AcceptOpen);
    QCOMPARE(newButton && newButton->isVisible(), true);

    //fd.setDetailsExpanded(true);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    QCOMPARE(fd.acceptMode(), QFileDialog::AcceptSave);
    QCOMPARE(newButton->isVisible(), true);

    fd.setAcceptMode(QFileDialog::AcceptOpen);
    QCOMPARE(fd.acceptMode(), QFileDialog::AcceptOpen);
    QCOMPARE(newButton->isVisible(), true);
}

void tst_QFiledialog::confirmOverwrite()
{
    QFileDialog fd;
    QCOMPARE(fd.confirmOverwrite(), true);
    fd.setConfirmOverwrite(true);
    QCOMPARE(fd.confirmOverwrite(), true);
    fd.setConfirmOverwrite(false);
    QCOMPARE(fd.confirmOverwrite(), false);
    fd.setConfirmOverwrite(true);
    QCOMPARE(fd.confirmOverwrite(), true);
}

void tst_QFiledialog::defaultSuffix()
{
    QFileDialog fd;
    QCOMPARE(fd.defaultSuffix(), QString());
    fd.setDefaultSuffix("txt");
    QCOMPARE(fd.defaultSuffix(), QString("txt"));
    fd.setDefaultSuffix(".txt");
    QCOMPARE(fd.defaultSuffix(), QString("txt"));
    fd.setDefaultSuffix(QString());
    QCOMPARE(fd.defaultSuffix(), QString());
}

void tst_QFiledialog::fileMode()
{
    QFileDialog fd;
    QCOMPARE(fd.fileMode(), QFileDialog::AnyFile);
    fd.setFileMode(QFileDialog::ExistingFile);
    QCOMPARE(fd.fileMode(), QFileDialog::ExistingFile);
    fd.setFileMode(QFileDialog::Directory);
    QCOMPARE(fd.fileMode(), QFileDialog::Directory);
    fd.setFileMode(QFileDialog::DirectoryOnly);
    QCOMPARE(fd.fileMode(), QFileDialog::DirectoryOnly);
    fd.setFileMode(QFileDialog::ExistingFiles);
    QCOMPARE(fd.fileMode(), QFileDialog::ExistingFiles);
}

void tst_QFiledialog::caption()
{
    QFileDialog fd;
    fd.setWindowTitle("testing");
    fd.setFileMode(QFileDialog::Directory);
    QCOMPARE(fd.windowTitle(), QString("testing"));
}

void tst_QFiledialog::filters()
{
    QFileDialog fd;
    QSignalSpy spyCurrentChanged(&fd, SIGNAL(currentChanged(QString)));
    QSignalSpy spyDirectoryEntered(&fd, SIGNAL(directoryEntered(QString)));
    QSignalSpy spyFilesSelected(&fd, SIGNAL(filesSelected(QStringList)));
    QSignalSpy spyFilterSelected(&fd, SIGNAL(filterSelected(QString)));
    QCOMPARE(fd.nameFilters(), QStringList("All Files (*)"));

    // effects
    QList<QComboBox*> views = fd.findChildren<QComboBox*>("fileTypeCombo");
    QCOMPARE(views.count(), 1);
    QCOMPARE(views.at(0)->isVisible(), false);

    QStringList filters;
    filters << "Image files (*.png *.xpm *.jpg)"
         << "Text files (*.txt)"
         << "Any files (*.*)";
    fd.setNameFilters(filters);
    QCOMPARE(views.at(0)->isVisible(), false);
    fd.show();
    fd.setAcceptMode(QFileDialog::AcceptSave);
    QCOMPARE(views.at(0)->isVisible(), true);
    QCOMPARE(fd.nameFilters(), filters);
    fd.setNameFilter("Image files (*.png *.xpm *.jpg);;Text files (*.txt);;Any files (*.*)");
    QCOMPARE(fd.nameFilters(), filters);
    QCOMPARE(spyCurrentChanged.count(), 0);
    QCOMPARE(spyDirectoryEntered.count(), 0);
    QCOMPARE(spyFilesSelected.count(), 0);
    QCOMPARE(spyFilterSelected.count(), 0);

    // setting shouldn't emit any signals
    for (int i = views.at(0)->currentIndex(); i < views.at(0)->count(); ++i)
        views.at(0)->setCurrentIndex(i);
    QCOMPARE(spyFilterSelected.count(), 0);

    //Let check if filters with whitespaces
    QFileDialog fd2;
    QStringList expected;
    expected << "C++ Source Files(*.cpp)";
    expected << "Any(*.*)";
    fd2.setNameFilter("C++ Source Files(*.cpp);;Any(*.*)");
    QCOMPARE(expected, fd2.nameFilters());
    fd2.setNameFilter("C++ Source Files(*.cpp) ;;Any(*.*)");
    QCOMPARE(expected, fd2.nameFilters());
    fd2.setNameFilter("C++ Source Files(*.cpp);; Any(*.*)");
    QCOMPARE(expected, fd2.nameFilters());
    fd2.setNameFilter(" C++ Source Files(*.cpp);; Any(*.*)");
    QCOMPARE(expected, fd2.nameFilters());
    fd2.setNameFilter("C++ Source Files(*.cpp) ;; Any(*.*)");
    QCOMPARE(expected, fd2.nameFilters());
}

void tst_QFiledialog::selectFilter()
{
    QFileDialog fd;
    QSignalSpy spyFilterSelected(&fd, SIGNAL(filterSelected(QString)));
    QCOMPARE(fd.selectedNameFilter(), QString("All Files (*)"));
    QStringList filters;
    filters << "Image files (*.png *.xpm *.jpg)"
         << "Text files (*.txt)"
         << "Any files (*.*)";
    fd.setNameFilters(filters);
    QCOMPARE(fd.selectedNameFilter(), filters.at(0));
    fd.selectNameFilter(filters.at(1));
    QCOMPARE(fd.selectedNameFilter(), filters.at(1));
    fd.selectNameFilter(filters.at(2));
    QCOMPARE(fd.selectedNameFilter(), filters.at(2));

    fd.selectNameFilter("bob");
    QCOMPARE(fd.selectedNameFilter(), filters.at(2));
    fd.selectNameFilter("");
    QCOMPARE(fd.selectedNameFilter(), filters.at(2));
    QCOMPARE(spyFilterSelected.count(), 0);
}

void tst_QFiledialog::history()
{
    QFileDialog fd;
    fd.setViewMode(QFileDialog::List);
    QFileSystemModel *model = fd.findChild<QFileSystemModel*>("qt_filesystem_model");
    QVERIFY(model);
    QSignalSpy spyCurrentChanged(&fd, SIGNAL(currentChanged(QString)));
    QSignalSpy spyDirectoryEntered(&fd, SIGNAL(directoryEntered(QString)));
    QSignalSpy spyFilesSelected(&fd, SIGNAL(filesSelected(QStringList)));
    QSignalSpy spyFilterSelected(&fd, SIGNAL(filterSelected(QString)));
    QCOMPARE(model->index(fd.history().first()), model->index(QDir::toNativeSeparators(fd.directory().absolutePath())));
    fd.setDirectory(QDir::current().absolutePath());
    QStringList history;
    history << QDir::toNativeSeparators(QDir::current().absolutePath())
            << QDir::toNativeSeparators(QDir::home().absolutePath())
            << QDir::toNativeSeparators(QDir::temp().absolutePath());
    fd.setHistory(history);
    if (fd.history() != history) {
        qDebug() << fd.history() << history;
        // quick and dirty output for windows failure.
        QListView* list = fd.findChild<QListView*>("listView");
        QVERIFY(list);
        QModelIndex root = list->rootIndex();
        while (root.isValid()) {
            qDebug() << root.data();
            root = root.parent();
        }
    }
    QCOMPARE(fd.history(), history);

    QStringList badHistory;
    badHistory << "junk";
    fd.setHistory(badHistory);
    badHistory << QDir::toNativeSeparators(QDir::current().absolutePath());
    QCOMPARE(fd.history(), badHistory);

    QCOMPARE(spyCurrentChanged.count(), 0);
    QCOMPARE(spyDirectoryEntered.count(), 0);
    QCOMPARE(spyFilesSelected.count(), 0);
    QCOMPARE(spyFilterSelected.count(), 0);
}

void tst_QFiledialog::iconProvider()
{
    QFileDialog *fd = new QFileDialog();
    QVERIFY(fd->iconProvider() != 0);
    QFileIconProvider *ip = new QFileIconProvider();
    fd->setIconProvider(ip);
    QCOMPARE(fd->iconProvider(), ip);
    delete fd;
    delete ip;
}

void tst_QFiledialog::isReadOnly()
{
    QFileDialog fd;

    QPushButton* newButton = fd.findChild<QPushButton*>("newFolderButton");
    QAction* renameAction = fd.findChild<QAction*>("qt_rename_action");
    QAction* deleteAction = fd.findChild<QAction*>("qt_delete_action");

    QCOMPARE(fd.isReadOnly(), false);

    // This is dependent upon the file/dir, find cross platform way to test
    //fd.setDirectory(QDir::home());
    //QCOMPARE(newButton && newButton->isEnabled(), true);
    //QCOMPARE(renameAction && renameAction->isEnabled(), true);
    //QCOMPARE(deleteAction && deleteAction->isEnabled(), true);

    fd.setReadOnly(true);
    QCOMPARE(fd.isReadOnly(), true);

    QCOMPARE(newButton && newButton->isEnabled(), false);
    QCOMPARE(renameAction && renameAction->isEnabled(), false);
    QCOMPARE(deleteAction && deleteAction->isEnabled(), false);
}

void tst_QFiledialog::itemDelegate()
{
    QFileDialog fd;
    QVERIFY(fd.itemDelegate() != 0);
    QItemDelegate *id = new QItemDelegate(&fd);
    fd.setItemDelegate(id);
    QCOMPARE(fd.itemDelegate(), (QAbstractItemDelegate *)id);
}

void tst_QFiledialog::labelText()
{
    QFileDialog fd;
    QDialogButtonBox buttonBox;
    QPushButton *cancelButton = buttonBox.addButton(QDialogButtonBox::Cancel);
    QCOMPARE(fd.labelText(QFileDialog::LookIn), QString("Look in:"));
    QCOMPARE(fd.labelText(QFileDialog::FileName), QString("File &name:"));
    QCOMPARE(fd.labelText(QFileDialog::FileType), QString("Files of type:"));
    QCOMPARE(fd.labelText(QFileDialog::Accept), QString("&Open")); ///### see task 241462
    QCOMPARE(fd.labelText(QFileDialog::Reject), cancelButton->text());

    fd.setLabelText(QFileDialog::LookIn, "1");
    QCOMPARE(fd.labelText(QFileDialog::LookIn), QString("1"));
    fd.setLabelText(QFileDialog::FileName, "2");
    QCOMPARE(fd.labelText(QFileDialog::FileName), QString("2"));
    fd.setLabelText(QFileDialog::FileType, "3");
    QCOMPARE(fd.labelText(QFileDialog::FileType), QString("3"));
    fd.setLabelText(QFileDialog::Accept, "4");
    QCOMPARE(fd.labelText(QFileDialog::Accept), QString("4"));
    fd.setLabelText(QFileDialog::Reject, "5");
    QCOMPARE(fd.labelText(QFileDialog::Reject), QString("5"));
}

void tst_QFiledialog::resolveSymlinks()
{
    QFileDialog fd;

    // default
    QCOMPARE(fd.resolveSymlinks(), true);
    fd.setResolveSymlinks(false);
    QCOMPARE(fd.resolveSymlinks(), false);
    fd.setResolveSymlinks(true);
    QCOMPARE(fd.resolveSymlinks(), true);

    // the file dialog doesn't do anything based upon this, just passes it to the model
    // the model should fully test it, don't test it here
}

void tst_QFiledialog::selectFile_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<int>("count");
    QTest::newRow("null") << QString() << 1;
    QTest::newRow("file") << "foo" << 1;
    QTest::newRow("tmp") << "temp" << 1;
}

void tst_QFiledialog::selectFile()
{
    QFETCH(QString, file);
    QFETCH(int, count);
    QScopedPointer<QFileDialog> fd(new QFileDialog);
    QFileSystemModel *model = fd->findChild<QFileSystemModel*>("qt_filesystem_model");
    QVERIFY(model);
    fd->setDirectory(QDir::currentPath());
    // default value
    QCOMPARE(fd->selectedFiles().count(), 1);

    QScopedPointer<QTemporaryFile> tempFile;
    if (file == QLatin1String("temp")) {
        tempFile.reset(new QTemporaryFile(QDir::tempPath() + QStringLiteral("/aXXXXXX")));
        QVERIFY2(tempFile->open(), qPrintable(tempFile->errorString()));
        file = tempFile->fileName();
    }

    fd->selectFile(file);
    QCOMPARE(fd->selectedFiles().count(), count);
    if (tempFile.isNull()) {
        QCOMPARE(model->index(fd->directory().path()), model->index(QDir::currentPath()));
    } else {
        QCOMPARE(model->index(fd->directory().path()), model->index(QDir::tempPath()));
    }
    fd.reset(); // Ensure the file dialog let's go of the temporary file for "temp".
}

void tst_QFiledialog::selectFileWrongCaseSaveAs()
{
    const QString home = QDir::homePath();
    if (isCaseSensitiveFileSystem(home))
        QSKIP("This test is intended for case-insensitive file systems only.");
    // QTBUG-38162: when passing a wrongly capitalized path to selectFile()
    // on a case-insensitive file system, the line edit should only
    // contain the file name ("c:\PRogram files\foo.txt" -> "foo.txt").
    const QString fileName = QStringLiteral("foo.txt");
    const QString path = home + QLatin1Char('/') + fileName;
    QString wrongCasePath = path;
    for (int c = 0; c < wrongCasePath.size(); c += 2)
        wrongCasePath[c] = wrongCasePath.at(c).isLower() ? wrongCasePath.at(c).toUpper() : wrongCasePath.at(c).toLower();
    QFileDialog fd(0, "QTBUG-38162", wrongCasePath);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.selectFile(wrongCasePath);
    const QLineEdit *lineEdit = fd.findChild<QLineEdit*>("fileNameEdit");
    QVERIFY(lineEdit);
    QCOMPARE(lineEdit->text().compare(fileName, Qt::CaseInsensitive), 0);
}

void tst_QFiledialog::selectFiles()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), qPrintable(tempDir.errorString()));
    const QString tempPath = tempDir.path();
    {
    QFileDialog fd;
    fd.setViewMode(QFileDialog::List);
    fd.setDirectory(tempPath);
    QSignalSpy spyCurrentChanged(&fd, SIGNAL(currentChanged(QString)));
    QSignalSpy spyDirectoryEntered(&fd, SIGNAL(directoryEntered(QString)));
    QSignalSpy spyFilesSelected(&fd, SIGNAL(filesSelected(QStringList)));
    QSignalSpy spyFilterSelected(&fd, SIGNAL(filterSelected(QString)));
    fd.setFileMode(QFileDialog::ExistingFiles);

    QString filesPath = fd.directory().absolutePath();
    for (int i=0; i < 5; ++i) {
        QFile file(filesPath + QLatin1String("/qfiledialog_auto_test_not_pres_") + QString::number(i));
        file.open(QIODevice::WriteOnly);
        file.resize(1024);
        file.flush();
        file.close();
    }

    // Get a list of files in the view and then get the corresponding index's
    QStringList list = fd.directory().entryList(QDir::Files);
    QModelIndexList toSelect;
    QVERIFY(list.count() > 1);
    QListView* listView = fd.findChild<QListView*>("listView");
    QVERIFY(listView);
    for (int i = 0; i < list.count(); ++i) {
        fd.selectFile(fd.directory().path() + QLatin1Char('/') + list.at(i));
        QTRY_VERIFY(!listView->selectionModel()->selectedRows().isEmpty());
        toSelect.append(listView->selectionModel()->selectedRows().last());
    }
    QCOMPARE(spyFilesSelected.count(), 0);

    listView->selectionModel()->clear();
    QCOMPARE(spyFilesSelected.count(), 0);

    // select the indexes
    for (int i = 0; i < toSelect.count(); ++i) {
        listView->selectionModel()->select(toSelect.at(i),
                QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
    QCOMPARE(fd.selectedFiles().count(), toSelect.count());
    QCOMPARE(spyCurrentChanged.count(), 0);
    QCOMPARE(spyDirectoryEntered.count(), 0);
    QCOMPARE(spyFilesSelected.count(), 0);
    QCOMPARE(spyFilterSelected.count(), 0);

    }

    {
        //If the selection is invalid then we fill the line edit but without the /
        QFileDialog dialog( 0, "Save" );
        dialog.setFileMode( QFileDialog::AnyFile );
        dialog.setAcceptMode( QFileDialog::AcceptSave );
        dialog.selectFile(tempPath + QStringLiteral("/blah"));
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        QLineEdit *lineEdit = dialog.findChild<QLineEdit*>("fileNameEdit");
        QVERIFY(lineEdit);
        QCOMPARE(lineEdit->text(),QLatin1String("blah"));
    }
}

void tst_QFiledialog::viewMode()
{
    QFileDialog fd;
    fd.setViewMode(QFileDialog::List);
    fd.show();

    // find widgets
    QList<QTreeView*> treeView = fd.findChildren<QTreeView*>("treeView");
    QCOMPARE(treeView.count(), 1);
    QList<QListView*> listView = fd.findChildren<QListView*>("listView");
    QCOMPARE(listView.count(), 1);
    QList<QToolButton*> listButton = fd.findChildren<QToolButton*>("listModeButton");
    QCOMPARE(listButton.count(), 1);
    QList<QToolButton*> treeButton = fd.findChildren<QToolButton*>("detailModeButton");
    QCOMPARE(treeButton.count(), 1);

    // default value
    QCOMPARE(fd.viewMode(), QFileDialog::List);

    // detail
    fd.setViewMode(QFileDialog::ViewMode(QFileDialog::Detail));

    QCOMPARE(QFileDialog::ViewMode(QFileDialog::Detail), fd.viewMode());
    QCOMPARE(listView.at(0)->isVisible(), false);
    QCOMPARE(listButton.at(0)->isDown(), false);
    QCOMPARE(treeView.at(0)->isVisible(), true);
    QCOMPARE(treeButton.at(0)->isDown(), true);

    // list
    fd.setViewMode(QFileDialog::ViewMode(QFileDialog::List));

    QCOMPARE(QFileDialog::ViewMode(QFileDialog::List), fd.viewMode());
    QCOMPARE(treeView.at(0)->isVisible(), false);
    QCOMPARE(treeButton.at(0)->isDown(), false);
    QCOMPARE(listView.at(0)->isVisible(), true);
    QCOMPARE(listButton.at(0)->isDown(), true);
}

void tst_QFiledialog::proxymodel()
{
    QFileDialog fd;
    QCOMPARE(fd.proxyModel(), nullptr);

    fd.setProxyModel(0);
    QCOMPARE(fd.proxyModel(), nullptr);

    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(&fd);
    fd.setProxyModel(proxyModel);
    QCOMPARE(fd.proxyModel(), (QAbstractProxyModel *)proxyModel);

    fd.setProxyModel(0);
    QCOMPARE(fd.proxyModel(), nullptr);
}

void tst_QFiledialog::setMimeTypeFilters_data()
{
    QTest::addColumn<QStringList>("mimeTypeFilters");
    QTest::addColumn<QString>("targetMimeTypeFilter");
    QTest::addColumn<QString>("expectedSelectedMimeTypeFilter");

    const auto headerMime = QStringLiteral("text/x-chdr");
    const auto pdfMime = QStringLiteral("application/pdf");
    const auto zipMime = QStringLiteral("application/zip");

    QTest::newRow("single mime filter (C header file)") << QStringList {headerMime} << headerMime << headerMime;
    QTest::newRow("single mime filter (JSON file)") << QStringList {pdfMime} << pdfMime << pdfMime;
    QTest::newRow("multiple mime filters") << QStringList {pdfMime, zipMime} << pdfMime << pdfMime;
}

void tst_QFiledialog::setMimeTypeFilters()
{
    QFETCH(QStringList, mimeTypeFilters);
    QFETCH(QString, targetMimeTypeFilter);
    QFETCH(QString, expectedSelectedMimeTypeFilter);

    QFileDialog fd;
    fd.setMimeTypeFilters(mimeTypeFilters);
    fd.selectMimeTypeFilter(targetMimeTypeFilter);

    QCOMPARE(fd.selectedMimeTypeFilter(), expectedSelectedMimeTypeFilter);

    auto *filters = fd.findChild<QComboBox*>("fileTypeCombo");
    filters->setCurrentIndex(filters->count() - 1);
    QCOMPARE(fd.selectedMimeTypeFilter(), mimeTypeFilters.last());
}

void tst_QFiledialog::setEmptyNameFilter()
{
    QFileDialog fd;
    fd.setNameFilter(QString());
    fd.setNameFilters(QStringList());
}

void tst_QFiledialog::setNameFilter_data()
{
    QTest::addColumn<bool>("nameFilterDetailsVisible");
    QTest::addColumn<QStringList>("filters");
    QTest::addColumn<QString>("selectFilter");
    QTest::addColumn<QString>("expectedSelectedFilter");

    QTest::newRow("namedetailsvisible-empty") << true << QStringList() << QString() << QString();
    QTest::newRow("namedetailsinvisible-empty") << false << QStringList() << QString() << QString();

    const QString anyFileNoDetails = QLatin1String("Any files");
    const QString anyFile = anyFileNoDetails + QLatin1String(" (*)");
    const QString imageFilesNoDetails = QLatin1String("Image files");
    const QString imageFiles = imageFilesNoDetails + QLatin1String(" (*.png *.xpm *.jpg)");
    const QString textFileNoDetails = QLatin1String("Text files");
    const QString textFile = textFileNoDetails + QLatin1String(" (*.txt)");

    QStringList filters;
    filters << anyFile << imageFiles << textFile;

    QTest::newRow("namedetailsvisible-images") << true << filters << imageFiles << imageFiles;
    QTest::newRow("namedetailsinvisible-images") << false << filters << imageFiles << imageFilesNoDetails;

    const QString invalid = "foo";
    QTest::newRow("namedetailsvisible-invalid") << true << filters << invalid << anyFile;
    // Potential crash when trying to convert the invalid filter into a list and stripping it, resulting in an empty list.
    QTest::newRow("namedetailsinvisible-invalid") << false << filters << invalid << anyFileNoDetails;
}

void tst_QFiledialog::setNameFilter()
{
    QFETCH(bool, nameFilterDetailsVisible);
    QFETCH(QStringList, filters);
    QFETCH(QString, selectFilter);
    QFETCH(QString, expectedSelectedFilter);

    QFileDialog fd;
    fd.setNameFilters(filters);
    fd.setNameFilterDetailsVisible(nameFilterDetailsVisible);
    fd.selectNameFilter(selectFilter);
    QCOMPARE(fd.selectedNameFilter(), expectedSelectedFilter);
}

void tst_QFiledialog::focus()
{
    QFileDialog fd;
    fd.setDirectory(QDir::currentPath());
    fd.show();
    QApplication::setActiveWindow(&fd);
    QVERIFY(QTest::qWaitForWindowActive(&fd));
    QCOMPARE(fd.isVisible(), true);
    QCOMPARE(QApplication::activeWindow(), static_cast<QWidget*>(&fd));
    qApp->processEvents();

    // make sure the tests work with focus follows mouse
    QCursor::setPos(fd.geometry().center());

    QList<QWidget*> treeView = fd.findChildren<QWidget*>("fileNameEdit");
    QCOMPARE(treeView.count(), 1);
    QVERIFY(treeView.at(0));
    QTRY_COMPARE(treeView.at(0)->hasFocus(), true);
    QCOMPARE(treeView.at(0)->hasFocus(), true);
}


void tst_QFiledialog::historyBack()
{
    QFileDialog fd;
    QFileSystemModel *model = fd.findChild<QFileSystemModel*>("qt_filesystem_model");
    QVERIFY(model);
    QToolButton *backButton = fd.findChild<QToolButton*>("backButton");
    QVERIFY(backButton);
    QToolButton *forwardButton = fd.findChild<QToolButton*>("forwardButton");
    QVERIFY(forwardButton);

    QSignalSpy spy(model, SIGNAL(rootPathChanged(QString)));

    QString home = fd.directory().absolutePath();
    QString desktop = QDir::homePath();
    QString temp = QDir::tempPath();

    QCOMPARE(backButton->isEnabled(), false);
    QCOMPARE(forwardButton->isEnabled(), false);
    fd.setDirectory(temp);
    qApp->processEvents();
    QCOMPARE(backButton->isEnabled(), true);
    QCOMPARE(forwardButton->isEnabled(), false);
    fd.setDirectory(desktop);
    QCOMPARE(spy.count(), 2);

    backButton->click();
    qApp->processEvents();
    QCOMPARE(backButton->isEnabled(), true);
    QCOMPARE(forwardButton->isEnabled(), true);
    QCOMPARE(spy.count(), 3);
    QString currentPath = qvariant_cast<QString>(spy.last().first());
    QCOMPARE(model->index(currentPath), model->index(temp));

    backButton->click();
    currentPath = qvariant_cast<QString>(spy.last().first());
    QCOMPARE(currentPath, home);
    QCOMPARE(backButton->isEnabled(), false);
    QCOMPARE(forwardButton->isEnabled(), true);
    QCOMPARE(spy.count(), 4);

    // nothing should change at this point
    backButton->click();
    QCOMPARE(spy.count(), 4);
    QCOMPARE(backButton->isEnabled(), false);
    QCOMPARE(forwardButton->isEnabled(), true);
}

void tst_QFiledialog::historyForward()
{
    QFileDialog fd;
    fd.setDirectory(QDir::currentPath());
    QToolButton *backButton = fd.findChild<QToolButton*>("backButton");
    QVERIFY(backButton);
    QToolButton *forwardButton = fd.findChild<QToolButton*>("forwardButton");
    QVERIFY(forwardButton);

    QFileSystemModel *model = fd.findChild<QFileSystemModel*>("qt_filesystem_model");
    QVERIFY(model);
    QSignalSpy spy(model, SIGNAL(rootPathChanged(QString)));

    QString home = fd.directory().absolutePath();
    QString desktop = QDir::homePath();
    QString temp = QDir::tempPath();

    fd.setDirectory(home);
    fd.setDirectory(temp);
    fd.setDirectory(desktop);

    backButton->click();
    QCOMPARE(forwardButton->isEnabled(), true);
    QCOMPARE(model->index(qvariant_cast<QString>(spy.last().first())), model->index(temp));

    forwardButton->click();
    QCOMPARE(model->index(qvariant_cast<QString>(spy.last().first())), model->index(desktop));
    QCOMPARE(backButton->isEnabled(), true);
    QCOMPARE(forwardButton->isEnabled(), false);
    QCOMPARE(spy.count(), 4);

    backButton->click();
    QCOMPARE(model->index(qvariant_cast<QString>(spy.last().first())), model->index(temp));
    QCOMPARE(backButton->isEnabled(), true);

    backButton->click();
    QCOMPARE(model->index(qvariant_cast<QString>(spy.last().first())), model->index(home));
    QCOMPARE(backButton->isEnabled(), false);
    QCOMPARE(forwardButton->isEnabled(), true);
    QCOMPARE(spy.count(), 6);

    forwardButton->click();
    QCOMPARE(model->index(qvariant_cast<QString>(spy.last().first())), model->index(temp));
    backButton->click();
    QCOMPARE(model->index(qvariant_cast<QString>(spy.last().first())), model->index(home));
    QCOMPARE(spy.count(), 8);

    forwardButton->click();
    QCOMPARE(model->index(qvariant_cast<QString>(spy.last().first())), model->index(temp));
    forwardButton->click();
    QCOMPARE(model->index(qvariant_cast<QString>(spy.last().first())), model->index(desktop));

    backButton->click();
    QCOMPARE(model->index(qvariant_cast<QString>(spy.last().first())), model->index(temp));
    backButton->click();
    QCOMPARE(model->index(qvariant_cast<QString>(spy.last().first())), model->index(home));
    fd.setDirectory(desktop);
    QCOMPARE(forwardButton->isEnabled(), false);
}

void tst_QFiledialog::disableSaveButton_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("isEnabled");

    QTest::newRow("valid path") << QDir::temp().absolutePath() + QDir::separator() + "qfiledialog.new_file" << true;
    QTest::newRow("no path") << "" << false;
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_OPENBSD)
    QTest::newRow("too long path") << "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii" << false;
#endif
    QTest::newRow("file") << "foo.html" << true;
}

void tst_QFiledialog::disableSaveButton()
{
    QFETCH(QString, path);
    QFETCH(bool, isEnabled);

    QFileDialog fd(0, "caption", path);
    fd.setAcceptMode(QFileDialog::AcceptSave);
    QDialogButtonBox *buttonBox = fd.findChild<QDialogButtonBox*>("buttonBox");
    QPushButton *button = buttonBox->button(QDialogButtonBox::Save);
    QVERIFY(button);
    QCOMPARE(button->isEnabled(), isEnabled);
}

void tst_QFiledialog::saveButtonText_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("label");
    QTest::addColumn<QString>("caption");

    QTest::newRow("empty path") << "" << QString() << QFileDialog::tr("&Save");
    QTest::newRow("file path") << "qfiledialog.new_file" << QString() << QFileDialog::tr("&Save");
    QTest::newRow("dir") << QDir::temp().absolutePath() << QString() << QFileDialog::tr("&Open");
    QTest::newRow("setTextLabel") << "qfiledialog.new_file" << "Mooo" << "Mooo";
    QTest::newRow("dir & label") << QDir::temp().absolutePath() << "Poo" << QFileDialog::tr("&Open");
}

void tst_QFiledialog::saveButtonText()
{
    QFETCH(QString, path);
    QFETCH(QString, label);
    QFETCH(QString, caption);

    QFileDialog fd(0, "auto test", QDir::temp().absolutePath());
    fd.setAcceptMode(QFileDialog::AcceptSave);
    if (!label.isNull())
        fd.setLabelText(QFileDialog::Accept, label);
    fd.setDirectory(QDir::temp());
    fd.selectFile(path);
    QDialogButtonBox *buttonBox = fd.findChild<QDialogButtonBox*>("buttonBox");
    QVERIFY(buttonBox);
    QPushButton *button = buttonBox->button(QDialogButtonBox::Save);
    QVERIFY(button);
    QCOMPARE(button->text(), caption);
}

// Predicate for use with QTRY_VERIFY() that checks whether the file dialog list
// has been populated (contains an entry).
class DirPopulatedPredicate
{
public:
    explicit DirPopulatedPredicate(QListView *list, const QString &needle) :
        m_list(list), m_needle(needle) {}

    operator bool() const
    {
        const auto model = m_list->model();
        const auto root = m_list->rootIndex();
        for (int r = 0, count = model->rowCount(root); r < count; ++r) {
            if (m_needle == model->index(r, 0, root).data(Qt::DisplayRole).toString())
                return true;
        }
        return false;
    }

private:
    QListView *m_list;
    QString m_needle;
};

// A predicate for use with QTRY_VERIFY() that ensures an entry of the file dialog
// list is selected by pressing cursor down.
class SelectDirTestPredicate
{
public:
    explicit SelectDirTestPredicate(QListView *list, const QString &needle) :
        m_list(list), m_needle(needle) {}

    operator bool() const
    {
        if (m_needle == m_list->currentIndex().data(Qt::DisplayRole).toString())
            return true;
        QCoreApplication::processEvents();
        QTest::keyClick(m_list, Qt::Key_Down);
        return false;
    }

private:
    QListView *m_list;
    QString m_needle;
};

void tst_QFiledialog::clearLineEdit()
{
    // Play it really safe by creating a directory which should show first in
    // a temporary dir
    QTemporaryDir workDir(QDir::tempPath() + QLatin1String("/tst_qfd_clearXXXXXX"));
    QVERIFY2(workDir.isValid(), qPrintable(workDir.errorString()));
    const QString workDirPath = workDir.path();
    const QString dirName = QLatin1String("aaaaa");
    QVERIFY(QDir(workDirPath).mkdir(dirName));

    QFileDialog fd(nullptr,
                   QLatin1String(QTest::currentTestFunction()) + QLatin1String(" AnyFile"),
                   "foo");
    fd.setViewMode(QFileDialog::List);
    fd.setFileMode(QFileDialog::AnyFile);
    fd.show();

    QLineEdit *lineEdit = fd.findChild<QLineEdit*>("fileNameEdit");
    QVERIFY(lineEdit);
    QCOMPARE(lineEdit->text(), QLatin1String("foo"));

    QListView* list = fd.findChild<QListView*>("listView");
    QVERIFY(list);

    // When in AnyFile mode, lineEdit's text shouldn't be cleared when entering
    // a directory by activating one in the list
    fd.setDirectory(workDirPath);
    DirPopulatedPredicate dirPopulated(list, dirName);
    QTRY_VERIFY(dirPopulated);

#ifdef QT_KEYPAD_NAVIGATION
    list->setEditFocus(true);
#endif

    SelectDirTestPredicate selectTestDir(list, dirName);
    QTRY_VERIFY(selectTestDir);

#ifndef Q_OS_MAC
    QTest::keyClick(list, Qt::Key_Return);
#else
    QTest::keyClick(list, Qt::Key_O, Qt::ControlModifier);
#endif

    QTRY_VERIFY(fd.directory().absolutePath() != workDirPath);
    QVERIFY(!lineEdit->text().isEmpty());

    // When in Directory mode, lineEdit's text should be cleared when entering
    // a directory by activating one in the list so one can just hit ok
    // and it selects that directory
    fd.setFileMode(QFileDialog::Directory);
    fd.setWindowTitle(QLatin1String(QTest::currentTestFunction()) + QLatin1String(" Directory"));
    fd.setDirectory(workDirPath);
    QTRY_VERIFY(dirPopulated);

    QTRY_VERIFY(selectTestDir);

#ifndef Q_OS_MAC
    QTest::keyClick(list, Qt::Key_Return);
#else
    QTest::keyClick(list, Qt::Key_O, Qt::ControlModifier);
#endif

    QTRY_VERIFY(fd.directory().absolutePath() != workDirPath);
    QVERIFY(lineEdit->text().isEmpty());
}

void tst_QFiledialog::enableChooseButton()
{
    QFileDialog fd;
    fd.setFileMode(QFileDialog::Directory);
    fd.show();
    QDialogButtonBox *buttonBox = fd.findChild<QDialogButtonBox*>("buttonBox");
    QPushButton *button = buttonBox->button(QDialogButtonBox::Open);
    QVERIFY(button);
    QCOMPARE(button->isEnabled(), true);
}

void tst_QFiledialog::widgetlessNativeDialog()
{
    if (!QGuiApplicationPrivate::platformTheme()->usePlatformNativeDialog(QPlatformTheme::FileDialog))
        QSKIP("This platform always uses widgets to realize its QFileDialog, instead of the native file dialog.");
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, false);
    QFileDialog fd;
    fd.setWindowModality(Qt::ApplicationModal);
    fd.show();
    QTRY_VERIFY(fd.isVisible());
    QFileSystemModel *model = fd.findChild<QFileSystemModel*>("qt_filesystem_model");
    QVERIFY(!model);
    QPushButton *button = fd.findChild<QPushButton*>();
    QVERIFY(!button);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, true);
}

void tst_QFiledialog::selectedFilesWithoutWidgets()
{
    // Test for a crash when widgets are not instantiated yet.
    QFileDialog fd;
    fd.setAcceptMode(QFileDialog::AcceptOpen);
    QVERIFY(fd.selectedFiles().size() >= 0);
}

void tst_QFiledialog::trailingDotsAndSpaces()
{
#ifndef Q_OS_WIN
    QSKIP("This is only tested on Windows");
#endif
    QFileDialog fd;
    fd.setViewMode(QFileDialog::List);
    fd.setFileMode(QFileDialog::ExistingFile);
    fd.show();
    QLineEdit *lineEdit = fd.findChild<QLineEdit *>("fileNameEdit");
    QVERIFY(lineEdit);
    QListView *list = fd.findChild<QListView *>("listView");
    QVERIFY(list);
    QTest::qWait(1000);
    int currentChildrenCount = list->model()->rowCount(list->rootIndex());
    QTest::keyClick(lineEdit, Qt::Key_Space);
    QTest::keyClick(lineEdit, Qt::Key_Period);
    QTest::qWait(1000);
    QCOMPARE(currentChildrenCount, list->model()->rowCount(list->rootIndex()));
    lineEdit->clear();
    QTest::keyClick(lineEdit, Qt::Key_Period);
    QTest::keyClick(lineEdit, Qt::Key_Space);
    QTest::qWait(1000);
    QCOMPARE(currentChildrenCount, list->model()->rowCount(list->rootIndex()));
}

#ifdef Q_OS_UNIX
#ifdef QT_BUILD_INTERNAL
void tst_QFiledialog::tildeExpansion_data()
{
    QTest::addColumn<QString>("tildePath");
    QTest::addColumn<QString>("expandedPath");

    const QString tilde = QStringLiteral("~");
    const QString tildeUser = tilde + QString(qgetenv("USER"));
    const QLatin1String someSubDir("/some/sub/dir");
    const QString homePath = QDir::homePath();
    const QString invalid = QStringLiteral("~thisIsNotAValidUserName");

    QTest::newRow("empty path") << QString() << QString();
    QTest::newRow("~")                    << tilde                  << homePath;
    QTest::newRow("~/some/sub/dir/")      << tilde     + someSubDir << homePath + someSubDir;
    QTest::newRow("~<user>")              << tildeUser              << homePath;
    QTest::newRow("~<user>/some/sub/dir") << tildeUser + someSubDir << homePath + someSubDir;
    QTest::newRow("invalid user name")    << invalid                << invalid;
}
#endif // QT_BUILD_INTERNAL

#ifdef QT_BUILD_INTERNAL
void tst_QFiledialog::tildeExpansion()
{
    QFETCH(QString, tildePath);
    QFETCH(QString, expandedPath);

    QCOMPARE(qt_tildeExpansion(tildePath), expandedPath);
}
#endif // QT_BUILD_INTERNAL
#endif

class DialogRejecter : public QObject
{
    Q_OBJECT
public:
    DialogRejecter()
    {
        connect(qApp, &QApplication::focusChanged, this, &DialogRejecter::rejectFileDialog);
    }

public slots:
    virtual void rejectFileDialog()
    {
        if (QWidget *w = QApplication::activeModalWidget())
            if (QDialog *d = qobject_cast<QDialog *>(w))
                QTest::keyClick(d, Qt::Key_Escape);
    }
};

void tst_QFiledialog::rejectModalDialogs()
{
    // QTBUG-38672 , static functions should return empty Urls
    DialogRejecter dr;

    QUrl url = QFileDialog::getOpenFileUrl(0, QStringLiteral("getOpenFileUrl"));
    QVERIFY(url.isEmpty());
    QVERIFY(!url.isValid());

    url = QFileDialog::getExistingDirectoryUrl(0, QStringLiteral("getExistingDirectoryUrl"));
    QVERIFY(url.isEmpty());
    QVERIFY(!url.isValid());

    url = QFileDialog::getSaveFileUrl(0, QStringLiteral("getSaveFileUrl"));
    QVERIFY(url.isEmpty());
    QVERIFY(!url.isValid());

    // Same test with local files
    QString file = QFileDialog::getOpenFileName(0, QStringLiteral("getOpenFileName"));
    QVERIFY(file.isEmpty());

    file = QFileDialog::getExistingDirectory(0, QStringLiteral("getExistingDirectory"));
    QVERIFY(file.isEmpty());

    file = QFileDialog::getSaveFileName(0, QStringLiteral("getSaveFileName"));
    QVERIFY(file.isEmpty());
}

void tst_QFiledialog::QTBUG49600_nativeIconProviderCrash()
{
    if (!QGuiApplicationPrivate::platformTheme()->usePlatformNativeDialog(QPlatformTheme::FileDialog))
        QSKIP("This platform always uses widgets to realize its QFileDialog, instead of the native file dialog.");
    QFileDialog fd;
    fd.iconProvider();
}

class qtbug57193DialogRejecter : public DialogRejecter
{
public:
    void rejectFileDialog() override
    {
        QCOMPARE(QGuiApplication::topLevelWindows().size(), 1);
        const QWindow *window = QGuiApplication::topLevelWindows().constFirst();

        const QFileDialog *fileDialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
        if (!fileDialog)
            return;

        // The problem in QTBUG-57193 was from a platform input context plugin that was
        // connected to QWindow::focusObjectChanged(), and consequently accessed the focus
        // object (the QFileDialog) that was in the process of being destroyed. This test
        // checks that the QFileDialog is never set as the focus object after its destruction process begins.
        connect(window, &QWindow::focusObjectChanged, [=](QObject *focusObject) {
            QVERIFY(focusObject != fileDialog);
        });
        DialogRejecter::rejectFileDialog();
    }
};

void tst_QFiledialog::focusObjectDuringDestruction()
{
    QTRY_VERIFY(QGuiApplication::topLevelWindows().isEmpty());

    qtbug57193DialogRejecter dialogRejecter;

    QFileDialog::getOpenFileName(nullptr, QString(), QString(), QString(), nullptr);
}

QTEST_MAIN(tst_QFiledialog)
#include "tst_qfiledialog.moc"
