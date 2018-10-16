/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qdatawidgetmapper.h"

#include "qabstractitemmodel.h"
#include "qitemdelegate.h"
#include "qmetaobject.h"
#include "qwidget.h"
#include "private/qobject_p.h"
#include "private/qabstractitemmodel_p.h"

#include <iterator>

QT_BEGIN_NAMESPACE

class QDataWidgetMapperPrivate: public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QDataWidgetMapper)

    QDataWidgetMapperPrivate()
        : model(QAbstractItemModelPrivate::staticEmptyModel()), delegate(0),
          orientation(Qt::Horizontal), submitPolicy(QDataWidgetMapper::AutoSubmit)
    {
    }

    QAbstractItemModel *model;
    QAbstractItemDelegate *delegate;
    Qt::Orientation orientation;
    QDataWidgetMapper::SubmitPolicy submitPolicy;
    QPersistentModelIndex rootIndex;
    QPersistentModelIndex currentTopLeft;

    inline int itemCount()
    {
        return orientation == Qt::Horizontal
            ? model->rowCount(rootIndex)
            : model->columnCount(rootIndex);
    }

    inline int currentIdx() const
    {
        return orientation == Qt::Horizontal ? currentTopLeft.row() : currentTopLeft.column();
    }

    inline QModelIndex indexAt(int itemPos)
    {
        return orientation == Qt::Horizontal
            ? model->index(currentIdx(), itemPos, rootIndex)
            : model->index(itemPos, currentIdx(), rootIndex);
    }

    void flipEventFilters(QAbstractItemDelegate *oldDelegate,
                          QAbstractItemDelegate *newDelegate) const
    {
        for (const WidgetMapper &e : widgetMap) {
            QWidget *w = e.widget;
            if (!w)
                continue;
            w->removeEventFilter(oldDelegate);
            w->installEventFilter(newDelegate);
        }
    }

    void populate();

    // private slots
    void _q_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &);
    void _q_commitData(QWidget *);
    void _q_closeEditor(QWidget *, QAbstractItemDelegate::EndEditHint);
    void _q_modelDestroyed();

    struct WidgetMapper
    {
        QPointer<QWidget> widget;
        int section;
        QPersistentModelIndex currentIndex;
        QByteArray property;
    };

    void populate(WidgetMapper &m);
    int findWidget(QWidget *w) const;

    bool commit(const WidgetMapper &m);

    std::vector<WidgetMapper> widgetMap;
};
Q_DECLARE_TYPEINFO(QDataWidgetMapperPrivate::WidgetMapper, Q_MOVABLE_TYPE);

int QDataWidgetMapperPrivate::findWidget(QWidget *w) const
{
    for (const WidgetMapper &e : widgetMap) {
        if (e.widget == w)
            return int(&e - &widgetMap.front());
    }
    return -1;
}

bool QDataWidgetMapperPrivate::commit(const WidgetMapper &m)
{
    if (m.widget.isNull())
        return true; // just ignore

    if (!m.currentIndex.isValid())
        return false;

    // Create copy to avoid passing the widget mappers data
    QModelIndex idx = m.currentIndex;
    if (m.property.isEmpty())
        delegate->setModelData(m.widget, model, idx);
    else
        model->setData(idx, m.widget->property(m.property), Qt::EditRole);

    return true;
}

void QDataWidgetMapperPrivate::populate(WidgetMapper &m)
{
    if (m.widget.isNull())
        return;

    m.currentIndex = indexAt(m.section);
    if (m.property.isEmpty())
        delegate->setEditorData(m.widget, m.currentIndex);
    else
        m.widget->setProperty(m.property, m.currentIndex.data(Qt::EditRole));
}

void QDataWidgetMapperPrivate::populate()
{
    for (WidgetMapper &e : widgetMap)
        populate(e);
}

static bool qContainsIndex(const QModelIndex &idx, const QModelIndex &topLeft,
                           const QModelIndex &bottomRight)
{
    return idx.row() >= topLeft.row() && idx.row() <= bottomRight.row()
           && idx.column() >= topLeft.column() && idx.column() <= bottomRight.column();
}

void QDataWidgetMapperPrivate::_q_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &)
{
    if (topLeft.parent() != rootIndex)
        return; // not in our hierarchy

    for (WidgetMapper &e : widgetMap) {
        if (qContainsIndex(e.currentIndex, topLeft, bottomRight))
            populate(e);
    }
}

void QDataWidgetMapperPrivate::_q_commitData(QWidget *w)
{
    if (submitPolicy == QDataWidgetMapper::ManualSubmit)
        return;

    int idx = findWidget(w);
    if (idx == -1)
        return; // not our widget

    commit(widgetMap[idx]);
}

void QDataWidgetMapperPrivate::_q_closeEditor(QWidget *w, QAbstractItemDelegate::EndEditHint hint)
{
    int idx = findWidget(w);
    if (idx == -1)
        return; // not our widget

    switch (hint) {
    case QAbstractItemDelegate::RevertModelCache: {
        populate(widgetMap[idx]);
        break; }
    case QAbstractItemDelegate::EditNextItem:
        w->focusNextChild();
        break;
    case QAbstractItemDelegate::EditPreviousItem:
        w->focusPreviousChild();
        break;
    case QAbstractItemDelegate::SubmitModelCache:
    case QAbstractItemDelegate::NoHint:
        // nothing
        break;
    }
}

void QDataWidgetMapperPrivate::_q_modelDestroyed()
{
    Q_Q(QDataWidgetMapper);

    model = 0;
    q->setModel(QAbstractItemModelPrivate::staticEmptyModel());
}

/*!
    \class QDataWidgetMapper
    \brief The QDataWidgetMapper class provides mapping between a section
    of a data model to widgets.
    \since 4.2
    \ingroup model-view
    \ingroup advanced
    \inmodule QtWidgets

    QDataWidgetMapper can be used to create data-aware widgets by mapping
    them to sections of an item model. A section is a column of a model
    if the orientation is horizontal (the default), otherwise a row.

    Every time the current index changes, each widget is updated with data
    from the model via the property specified when its mapping was made.
    If the user edits the contents of a widget, the changes are read using
    the same property and written back to the model.
    By default, each widget's \l{Q_PROPERTY()}{user property} is used to
    transfer data between the model and the widget. Since Qt 4.3, an
    additional addMapping() function enables a named property to be used
    instead of the default user property.

    It is possible to set an item delegate to support custom widgets. By default,
    a QItemDelegate is used to synchronize the model with the widgets.

    Let us assume that we have an item model named \c{model} with the following contents:

    \table
    \row \li 1 \li Qt Norway       \li Oslo
    \row \li 2 \li Qt Australia    \li Brisbane
    \row \li 3 \li Qt USA          \li Palo Alto
    \row \li 4 \li Qt China        \li Beijing
    \row \li 5 \li Qt Germany      \li Berlin
    \endtable

    The following code will map the columns of the model to widgets called \c mySpinBox,
    \c myLineEdit and \c{myCountryChooser}:

    \snippet code/src_gui_itemviews_qdatawidgetmapper.cpp 0

    After the call to toFirst(), \c mySpinBox displays the value \c{1}, \c myLineEdit
    displays \c{Qt Norway} and \c myCountryChooser displays \c{Oslo}. The
    navigational functions toFirst(), toNext(), toPrevious(), toLast() and setCurrentIndex()
    can be used to navigate in the model and update the widgets with contents from
    the model.

    The setRootIndex() function enables a particular item in a model to be
    specified as the root index - children of this item will be mapped to
    the relevant widgets in the user interface.

    QDataWidgetMapper supports two submit policies, \c AutoSubmit and \c{ManualSubmit}.
    \c AutoSubmit will update the model as soon as the current widget loses focus,
    \c ManualSubmit will not update the model unless submit() is called. \c ManualSubmit
    is useful when displaying a dialog that lets the user cancel all modifications.
    Also, other views that display the model won't update until the user finishes
    all their modifications and submits.

    Note that QDataWidgetMapper keeps track of external modifications. If the contents
    of the model are updated in another module of the application, the widgets are
    updated as well.

    \sa QAbstractItemModel, QAbstractItemDelegate
 */

/*! \enum QDataWidgetMapper::SubmitPolicy

    This enum describes the possible submit policies a QDataWidgetMapper
    supports.

    \value AutoSubmit    Whenever a widget loses focus, the widget's current
                         value is set to the item model.
    \value ManualSubmit  The model is not updated until submit() is called.
 */

/*!
    \fn void QDataWidgetMapper::currentIndexChanged(int index)

    This signal is emitted after the current index has changed and
    all widgets were populated with new data. \a index is the new
    current index.

    \sa currentIndex(), setCurrentIndex()
 */

/*!
    Constructs a new QDataWidgetMapper with parent object \a parent.
    By default, the orientation is horizontal and the submit policy
    is \c{AutoSubmit}.

    \sa setOrientation(), setSubmitPolicy()
 */
QDataWidgetMapper::QDataWidgetMapper(QObject *parent)
    : QObject(*new QDataWidgetMapperPrivate, parent)
{
    setItemDelegate(new QItemDelegate(this));
}

/*!
    Destroys the object.
 */
QDataWidgetMapper::~QDataWidgetMapper()
{
}

/*!
     Sets the current model to \a model. If another model was set,
     all mappings to that old model are cleared.

     \sa model()
 */
void QDataWidgetMapper::setModel(QAbstractItemModel *model)
{
    Q_D(QDataWidgetMapper);

    if (d->model == model)
        return;

    if (d->model) {
        disconnect(d->model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this,
                   SLOT(_q_dataChanged(QModelIndex,QModelIndex,QVector<int>)));
        disconnect(d->model, SIGNAL(destroyed()), this,
                   SLOT(_q_modelDestroyed()));
    }
    clearMapping();
    d->rootIndex = QModelIndex();
    d->currentTopLeft = QModelIndex();

    d->model = model;

    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)),
            SLOT(_q_dataChanged(QModelIndex,QModelIndex,QVector<int>)));
    connect(model, SIGNAL(destroyed()), SLOT(_q_modelDestroyed()));
}

/*!
    Returns the current model.

    \sa setModel()
 */
QAbstractItemModel *QDataWidgetMapper::model() const
{
    Q_D(const QDataWidgetMapper);
    return d->model == QAbstractItemModelPrivate::staticEmptyModel()
            ? static_cast<QAbstractItemModel *>(0)
            : d->model;
}

/*!
    Sets the item delegate to \a delegate. The delegate will be used to write
    data from the model into the widget and from the widget to the model,
    using QAbstractItemDelegate::setEditorData() and QAbstractItemDelegate::setModelData().

    The delegate also decides when to apply data and when to change the editor,
    using QAbstractItemDelegate::commitData() and QAbstractItemDelegate::closeEditor().

    \warning You should not share the same instance of a delegate between widget mappers
    or views. Doing so can cause incorrect or unintuitive editing behavior since each
    view connected to a given delegate may receive the \l{QAbstractItemDelegate::}{closeEditor()}
    signal, and attempt to access, modify or close an editor that has already been closed.
 */
void QDataWidgetMapper::setItemDelegate(QAbstractItemDelegate *delegate)
{
    Q_D(QDataWidgetMapper);
    QAbstractItemDelegate *oldDelegate = d->delegate;
    if (oldDelegate) {
        disconnect(oldDelegate, SIGNAL(commitData(QWidget*)), this, SLOT(_q_commitData(QWidget*)));
        disconnect(oldDelegate, SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)),
                   this, SLOT(_q_closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)));
    }

    d->delegate = delegate;

    if (delegate) {
        connect(delegate, SIGNAL(commitData(QWidget*)), SLOT(_q_commitData(QWidget*)));
        connect(delegate, SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)),
                SLOT(_q_closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)));
    }

    d->flipEventFilters(oldDelegate, delegate);
}

/*!
    Returns the current item delegate.
 */
QAbstractItemDelegate *QDataWidgetMapper::itemDelegate() const
{
    Q_D(const QDataWidgetMapper);
    return d->delegate;
}

/*!
    Sets the root item to \a index. This can be used to display
    a branch of a tree. Pass an invalid model index to display
    the top-most branch.

    \sa rootIndex()
 */
void QDataWidgetMapper::setRootIndex(const QModelIndex &index)
{
    Q_D(QDataWidgetMapper);
    d->rootIndex = index;
}

/*!
    Returns the current root index.

    \sa setRootIndex()
*/
QModelIndex QDataWidgetMapper::rootIndex() const
{
    Q_D(const QDataWidgetMapper);
    return QModelIndex(d->rootIndex);
}

/*!
    Adds a mapping between a \a widget and a \a section from the model.
    The \a section is a column in the model if the orientation is
    horizontal (the default), otherwise a row.

    For the following example, we assume a model \c myModel that
    has two columns: the first one contains the names of people in a
    group, and the second column contains their ages. The first column
    is mapped to the QLineEdit \c nameLineEdit, and the second is
    mapped to the QSpinBox \c{ageSpinBox}:

    \snippet code/src_gui_itemviews_qdatawidgetmapper.cpp 1

    \b{Notes:}
    \list
    \li If the \a widget is already mapped to a section, the
    old mapping will be replaced by the new one.
    \li Only one-to-one mappings between sections and widgets are allowed.
    It is not possible to map a single section to multiple widgets, or to
    map a single widget to multiple sections.
    \endlist

    \sa removeMapping(), mappedSection(), clearMapping()
 */
void QDataWidgetMapper::addMapping(QWidget *widget, int section)
{
    Q_D(QDataWidgetMapper);

    removeMapping(widget);
    d->widgetMap.push_back({widget, section, d->indexAt(section), QByteArray()});
    widget->installEventFilter(d->delegate);
}

/*!
  \since 4.3

  Essentially the same as addMapping(), but adds the possibility to specify
  the property to use specifying \a propertyName.

  \sa addMapping()
*/

void QDataWidgetMapper::addMapping(QWidget *widget, int section, const QByteArray &propertyName)
{
    Q_D(QDataWidgetMapper);

    removeMapping(widget);
    d->widgetMap.push_back({widget, section, d->indexAt(section), propertyName});
    widget->installEventFilter(d->delegate);
}

/*!
    Removes the mapping for the given \a widget.

    \sa addMapping(), clearMapping()
 */
void QDataWidgetMapper::removeMapping(QWidget *widget)
{
    Q_D(QDataWidgetMapper);

    int idx = d->findWidget(widget);
    if (idx == -1)
        return;

    d->widgetMap.erase(d->widgetMap.begin() + idx);
    widget->removeEventFilter(d->delegate);
}

/*!
    Returns the section the \a widget is mapped to or -1
    if the widget is not mapped.

    \sa addMapping(), removeMapping()
 */
int QDataWidgetMapper::mappedSection(QWidget *widget) const
{
    Q_D(const QDataWidgetMapper);

    int idx = d->findWidget(widget);
    if (idx == -1)
        return -1;

    return d->widgetMap[idx].section;
}

/*!
  \since 4.3
  Returns the name of the property that is used when mapping
  data to the given \a widget.

  \sa mappedSection(), addMapping(), removeMapping()
*/

QByteArray QDataWidgetMapper::mappedPropertyName(QWidget *widget) const
{
    Q_D(const QDataWidgetMapper);

    int idx = d->findWidget(widget);
    if (idx == -1)
        return QByteArray();
    const auto &m = d->widgetMap[idx];
    if (m.property.isEmpty())
        return m.widget->metaObject()->userProperty().name();
    else
        return m.property;
}

/*!
    Returns the widget that is mapped at \a section, or
    0 if no widget is mapped at that section.

    \sa addMapping(), removeMapping()
 */
QWidget *QDataWidgetMapper::mappedWidgetAt(int section) const
{
    Q_D(const QDataWidgetMapper);

    for (auto &e : d->widgetMap) {
        if (e.section == section)
            return e.widget;
    }

    return 0;
}

/*!
    Repopulates all widgets with the current data of the model.
    All unsubmitted changes will be lost.

    \sa submit(), setSubmitPolicy()
 */
void QDataWidgetMapper::revert()
{
    Q_D(QDataWidgetMapper);

    d->populate();
}

/*!
    Submits all changes from the mapped widgets to the model.

    For every mapped section, the item delegate reads the current
    value from the widget and sets it in the model. Finally, the
    model's \l {QAbstractItemModel::}{submit()} method is invoked.

    Returns \c true if all the values were submitted, otherwise false.

    Note: For database models, QSqlQueryModel::lastError() can be
    used to retrieve the last error.

    \sa revert(), setSubmitPolicy()
 */
bool QDataWidgetMapper::submit()
{
    Q_D(QDataWidgetMapper);

    for (auto &e : d->widgetMap) {
        if (!d->commit(e))
            return false;
    }

    return d->model->submit();
}

/*!
    Populates the widgets with data from the first row of the model
    if the orientation is horizontal (the default), otherwise
    with data from the first column.

    This is equivalent to calling \c setCurrentIndex(0).

    \sa toLast(), setCurrentIndex()
 */
void QDataWidgetMapper::toFirst()
{
    setCurrentIndex(0);
}

/*!
    Populates the widgets with data from the last row of the model
    if the orientation is horizontal (the default), otherwise
    with data from the last column.

    Calls setCurrentIndex() internally.

    \sa toFirst(), setCurrentIndex()
 */
void QDataWidgetMapper::toLast()
{
    Q_D(QDataWidgetMapper);
    setCurrentIndex(d->itemCount() - 1);
}


/*!
    Populates the widgets with data from the next row of the model
    if the orientation is horizontal (the default), otherwise
    with data from the next column.

    Calls setCurrentIndex() internally. Does nothing if there is
    no next row in the model.

    \sa toPrevious(), setCurrentIndex()
 */
void QDataWidgetMapper::toNext()
{
    Q_D(QDataWidgetMapper);
    setCurrentIndex(d->currentIdx() + 1);
}

/*!
    Populates the widgets with data from the previous row of the model
    if the orientation is horizontal (the default), otherwise
    with data from the previous column.

    Calls setCurrentIndex() internally. Does nothing if there is
    no previous row in the model.

    \sa toNext(), setCurrentIndex()
 */
void QDataWidgetMapper::toPrevious()
{
    Q_D(QDataWidgetMapper);
    setCurrentIndex(d->currentIdx() - 1);
}

/*!
    \property QDataWidgetMapper::currentIndex
    \brief the current row or column

    The widgets are populated with with data from the row at \a index
    if the orientation is horizontal (the default), otherwise with
    data from the column at \a index.

    \sa setCurrentModelIndex(), toFirst(), toNext(), toPrevious(), toLast()
*/
void QDataWidgetMapper::setCurrentIndex(int index)
{
    Q_D(QDataWidgetMapper);

    if (index < 0 || index >= d->itemCount())
        return;
    d->currentTopLeft = d->orientation == Qt::Horizontal
                            ? d->model->index(index, 0, d->rootIndex)
                            : d->model->index(0, index, d->rootIndex);
    d->populate();

    emit currentIndexChanged(index);
}

int QDataWidgetMapper::currentIndex() const
{
    Q_D(const QDataWidgetMapper);
    return d->currentIdx();
}

/*!
    Sets the current index to the row of the \a index if the
    orientation is horizontal (the default), otherwise to the
    column of the \a index.

    Calls setCurrentIndex() internally. This convenience slot can be
    connected to the signal \l
    {QItemSelectionModel::}{currentRowChanged()} or \l
    {QItemSelectionModel::}{currentColumnChanged()} of another view's
    \l {QItemSelectionModel}{selection model}.

    The following example illustrates how to update all widgets
    with new data whenever the selection of a QTableView named
    \c myTableView changes:

    \snippet code/src_gui_itemviews_qdatawidgetmapper.cpp 2

    \sa currentIndex()
*/
void QDataWidgetMapper::setCurrentModelIndex(const QModelIndex &index)
{
    Q_D(QDataWidgetMapper);

    if (!index.isValid()
        || index.model() != d->model
        || index.parent() != d->rootIndex)
        return;

    setCurrentIndex(d->orientation == Qt::Horizontal ? index.row() : index.column());
}

/*!
    Clears all mappings.

    \sa addMapping(), removeMapping()
 */
void QDataWidgetMapper::clearMapping()
{
    Q_D(QDataWidgetMapper);

    decltype(d->widgetMap) copy;
    d->widgetMap.swap(copy); // a C++98 move
    for (auto it = copy.crbegin(), end = copy.crend(); it != end; ++it) {
        if (it->widget)
            it->widget->removeEventFilter(d->delegate);
    }
}

/*!
    \property QDataWidgetMapper::orientation
    \brief the orientation of the model

    If the orientation is Qt::Horizontal (the default), a widget is
    mapped to a column of a data model. The widget will be populated
    with the model's data from its mapped column and the row that
    currentIndex() points at.

    Use Qt::Horizontal for tabular data that looks like this:

    \table
    \row \li 1 \li Qt Norway       \li Oslo
    \row \li 2 \li Qt Australia    \li Brisbane
    \row \li 3 \li Qt USA          \li Silicon Valley
    \row \li 4 \li Qt China        \li Beijing
    \row \li 5 \li Qt Germany      \li Berlin
    \endtable

    If the orientation is set to Qt::Vertical, a widget is mapped to
    a row. Calling setCurrentIndex() will change the current column.
    The widget will be populates with the model's data from its
    mapped row and the column that currentIndex() points at.

    Use Qt::Vertical for tabular data that looks like this:

    \table
    \row \li 1 \li 2 \li 3 \li 4 \li 5
    \row \li Qt Norway \li Qt Australia \li Qt USA \li Qt China \li Qt Germany
    \row \li Oslo \li Brisbane \li Silicon Valley \li Beijing \li Berlin
    \endtable

    Changing the orientation clears all existing mappings.
*/
void QDataWidgetMapper::setOrientation(Qt::Orientation orientation)
{
    Q_D(QDataWidgetMapper);

    if (d->orientation == orientation)
        return;

    clearMapping();
    d->orientation = orientation;
}

Qt::Orientation QDataWidgetMapper::orientation() const
{
    Q_D(const QDataWidgetMapper);
    return d->orientation;
}

/*!
    \property QDataWidgetMapper::submitPolicy
    \brief the current submit policy

    Changing the current submit policy will revert all widgets
    to the current data from the model.
*/
void QDataWidgetMapper::setSubmitPolicy(SubmitPolicy policy)
{
    Q_D(QDataWidgetMapper);
    if (policy == d->submitPolicy)
        return;

    revert();
    d->submitPolicy = policy;
}

QDataWidgetMapper::SubmitPolicy QDataWidgetMapper::submitPolicy() const
{
    Q_D(const QDataWidgetMapper);
    return d->submitPolicy;
}

QT_END_NAMESPACE

#include "moc_qdatawidgetmapper.cpp"
