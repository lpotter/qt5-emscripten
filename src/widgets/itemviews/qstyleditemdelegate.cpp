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

#include "qstyleditemdelegate.h"

#include <qabstractitemmodel.h>
#include <qapplication.h>
#include <qbrush.h>
#if QT_CONFIG(lineedit)
#include <qlineedit.h>
#endif
#if QT_CONFIG(textedit)
#include <qtextedit.h>
#include <qplaintextedit.h>
#endif
#include <qpainter.h>
#include <qpalette.h>
#include <qpoint.h>
#include <qrect.h>
#include <qsize.h>
#include <qstyle.h>
#include <qdatetime.h>
#include <qstyleoption.h>
#include <qevent.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qpixmapcache.h>
#include <qitemeditorfactory.h>
#include <private/qitemeditorfactory_p.h>
#include <qmetaobject.h>
#include <qtextlayout.h>
#include <private/qabstractitemdelegate_p.h>
#include <private/qtextengine_p.h>
#include <private/qlayoutengine_p.h>
#include <qdebug.h>
#include <qlocale.h>
#if QT_CONFIG(tableview)
#include <qtableview.h>
#endif

#include <limits.h>

QT_BEGIN_NAMESPACE

class QStyledItemDelegatePrivate : public QAbstractItemDelegatePrivate
{
    Q_DECLARE_PUBLIC(QStyledItemDelegate)

public:
    QStyledItemDelegatePrivate() : factory(0) { }

    static const QWidget *widget(const QStyleOptionViewItem &option)
    {
        return option.widget;
    }

    const QItemEditorFactory *editorFactory() const
    {
        return factory ? factory : QItemEditorFactory::defaultFactory();
    }

    QItemEditorFactory *factory;
};

/*!
    \class QStyledItemDelegate

    \brief The QStyledItemDelegate class provides display and editing facilities for
    data items from a model.

    \ingroup model-view
    \inmodule QtWidgets

    \since 4.4

    When displaying data from models in Qt item views, e.g., a
    QTableView, the individual items are drawn by a delegate. Also,
    when an item is edited, it provides an editor widget, which is
    placed on top of the item view while editing takes place.
    QStyledItemDelegate is the default delegate for all Qt item
    views, and is installed upon them when they are created.

    The QStyledItemDelegate class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view
    framework}. The delegate allows the display and editing of items
    to be developed independently from the model and view.

    The data of items in models are assigned an
    \l{Qt::}{ItemDataRole}; each item can store a QVariant for each
    role. QStyledItemDelegate implements display and editing for the
    most common datatypes expected by users, including booleans,
    integers, and strings.

    The data will be drawn differently depending on which role they
    have in the model. The following table describes the roles and the
    data types the delegate can handle for each of them. It is often
    sufficient to ensure that the model returns appropriate data for
    each of the roles to determine the appearance of items in views.

    \table
    \header \li Role \li Accepted Types
    \omit
    \row    \li \l Qt::AccessibleDescriptionRole \li QString
    \row    \li \l Qt::AccessibleTextRole \li QString
    \endomit
    \row    \li \l Qt::BackgroundRole \li QBrush
    \row    \li \l Qt::BackgroundColorRole \li QColor (obsolete; use Qt::BackgroundRole instead)
    \row    \li \l Qt::CheckStateRole \li Qt::CheckState
    \row    \li \l Qt::DecorationRole \li QIcon, QPixmap, QImage and QColor
    \row    \li \l Qt::DisplayRole \li QString and types with a string representation
    \row    \li \l Qt::EditRole \li See QItemEditorFactory for details
    \row    \li \l Qt::FontRole \li QFont
    \row    \li \l Qt::SizeHintRole \li QSize
    \omit
    \row    \li \l Qt::StatusTipRole \li
    \endomit
    \row    \li \l Qt::TextAlignmentRole \li Qt::Alignment
    \row    \li \l Qt::ForegroundRole \li QBrush
    \row    \li \l Qt::TextColorRole \li QColor (obsolete; use Qt::ForegroundRole instead)
    \omit
    \row    \li \l Qt::ToolTipRole
    \row    \li \l Qt::WhatsThisRole
    \endomit
    \endtable

    Editors are created with a QItemEditorFactory; a default static
    instance provided by QItemEditorFactory is installed on all item
    delegates. You can set a custom factory using
    setItemEditorFactory() or set a new default factory with
    QItemEditorFactory::setDefaultFactory(). It is the data stored in
    the item model with the \l{Qt::}{EditRole} that is edited. See the
    QItemEditorFactory class for a more high-level introduction to
    item editor factories. The \l{Color Editor Factory Example}{Color
    Editor Factory} example shows how to create custom editors with a
    factory.

    \section1 Subclassing QStyledItemDelegate

    If the delegate does not support painting of the data types you
    need or you want to customize the drawing of items, you need to
    subclass QStyledItemDelegate, and reimplement paint() and possibly
    sizeHint(). The paint() function is called individually for each
    item, and with sizeHint(), you can specify the hint for each
    of them.

    When reimplementing paint(), one would typically handle the
    datatypes one would like to draw and use the superclass
    implementation for other types.

    The painting of check box indicators are performed by the current
    style. The style also specifies the size and the bounding
    rectangles in which to draw the data for the different data roles.
    The bounding rectangle of the item itself is also calculated by
    the style. When drawing already supported datatypes, it is
    therefore a good idea to ask the style for these bounding
    rectangles. The QStyle class description describes this in
    more detail.

    If you wish to change any of the bounding rectangles calculated by
    the style or the painting of check box indicators, you can
    subclass QStyle. Note, however, that the size of the items can
    also be affected by reimplementing sizeHint().

    It is possible for a custom delegate to provide editors
    without the use of an editor item factory. In this case, the
    following virtual functions must be reimplemented:

    \list
        \li createEditor() returns the widget used to change data from the model
           and can be reimplemented to customize editing behavior.
        \li setEditorData() provides the widget with data to manipulate.
        \li updateEditorGeometry() ensures that the editor is displayed correctly
           with respect to the item view.
        \li setModelData() returns updated data to the model.
    \endlist

    The \l{Star Delegate Example}{Star Delegate} example creates
    editors by reimplementing these methods.

    \section1 QStyledItemDelegate vs. QItemDelegate

    Since Qt 4.4, there are two delegate classes: QItemDelegate and
    QStyledItemDelegate. However, the default delegate is QStyledItemDelegate.
    These two classes are independent alternatives to painting and providing
    editors for items in views. The difference between them is that
    QStyledItemDelegate uses the current style to paint its items. We therefore
    recommend using QStyledItemDelegate as the base class when implementing
    custom delegates or when working with Qt style sheets. The code required
    for either class should be equal unless the custom delegate needs to use
    the style for drawing.

    If you wish to customize the painting of item views, you should
    implement a custom style. Please see the QStyle class
    documentation for details.

    \sa {Delegate Classes}, QItemDelegate, QAbstractItemDelegate, QStyle,
        {Spin Box Delegate Example}, {Star Delegate Example}, {Color
         Editor Factory Example}
*/


/*!
    Constructs an item delegate with the given \a parent.
*/
QStyledItemDelegate::QStyledItemDelegate(QObject *parent)
    : QAbstractItemDelegate(*new QStyledItemDelegatePrivate(), parent)
{
}

/*!
    Destroys the item delegate.
*/
QStyledItemDelegate::~QStyledItemDelegate()
{
}

/*!
    This function returns the string that the delegate will use to display the
    Qt::DisplayRole of the model in \a locale. \a value is the value of the Qt::DisplayRole
    provided by the model.

    The default implementation uses the QLocale::toString to convert \a value into
    a QString.

    This function is not called for empty model indices, i.e., indices for which
    the model returns an invalid QVariant.

    \sa QAbstractItemModel::data()
*/
QString QStyledItemDelegate::displayText(const QVariant &value, const QLocale& locale) const
{
    return d_func()->textForRole(Qt::DisplayRole, value, locale);
}

/*!
    Initialize \a option with the values using the index \a index. This method
    is useful for subclasses when they need a QStyleOptionViewItem, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QStyledItemDelegate::initStyleOption(QStyleOptionViewItem *option,
                                         const QModelIndex &index) const
{
    QVariant value = index.data(Qt::FontRole);
    if (value.isValid() && !value.isNull()) {
        option->font = qvariant_cast<QFont>(value).resolve(option->font);
        option->fontMetrics = QFontMetrics(option->font);
    }

    value = index.data(Qt::TextAlignmentRole);
    if (value.isValid() && !value.isNull())
        option->displayAlignment = Qt::Alignment(value.toInt());

    value = index.data(Qt::ForegroundRole);
    if (value.canConvert<QBrush>())
        option->palette.setBrush(QPalette::Text, qvariant_cast<QBrush>(value));

    option->index = index;
    value = index.data(Qt::CheckStateRole);
    if (value.isValid() && !value.isNull()) {
        option->features |= QStyleOptionViewItem::HasCheckIndicator;
        option->checkState = static_cast<Qt::CheckState>(value.toInt());
    }

    value = index.data(Qt::DecorationRole);
    if (value.isValid() && !value.isNull()) {
        option->features |= QStyleOptionViewItem::HasDecoration;
        switch (value.type()) {
        case QVariant::Icon: {
            option->icon = qvariant_cast<QIcon>(value);
            QIcon::Mode mode;
            if (!(option->state & QStyle::State_Enabled))
                mode = QIcon::Disabled;
            else if (option->state & QStyle::State_Selected)
                mode = QIcon::Selected;
            else
                mode = QIcon::Normal;
            QIcon::State state = option->state & QStyle::State_Open ? QIcon::On : QIcon::Off;
            QSize actualSize = option->icon.actualSize(option->decorationSize, mode, state);
            // For highdpi icons actualSize might be larger than decorationSize, which we don't want. Clamp it to decorationSize.
            option->decorationSize = QSize(qMin(option->decorationSize.width(), actualSize.width()),
                                           qMin(option->decorationSize.height(), actualSize.height()));
            break;
        }
        case QVariant::Color: {
            QPixmap pixmap(option->decorationSize);
            pixmap.fill(qvariant_cast<QColor>(value));
            option->icon = QIcon(pixmap);
            break;
        }
        case QVariant::Image: {
            QImage image = qvariant_cast<QImage>(value);
            option->icon = QIcon(QPixmap::fromImage(image));
            option->decorationSize = image.size() / image.devicePixelRatio();
            break;
        }
        case QVariant::Pixmap: {
            QPixmap pixmap = qvariant_cast<QPixmap>(value);
            option->icon = QIcon(pixmap);
            option->decorationSize = pixmap.size() / pixmap.devicePixelRatio();
            break;
        }
        default:
            break;
        }
    }

    value = index.data(Qt::DisplayRole);
    if (value.isValid() && !value.isNull()) {
        option->features |= QStyleOptionViewItem::HasDisplay;
        option->text = displayText(value, option->locale);
    }

    option->backgroundBrush = qvariant_cast<QBrush>(index.data(Qt::BackgroundRole));

    // disable style animations for checkboxes etc. within itemviews (QTBUG-30146)
    option->styleObject = 0;
}

/*!
    Renders the delegate using the given \a painter and style \a option for
    the item specified by \a index.

    This function paints the item using the view's QStyle.

    When reimplementing paint in a subclass. Use the initStyleOption()
    to set up the \a option in the same way as the
    QStyledItemDelegate.

    Whenever possible, use the \a option while painting.
    Especially its \l{QStyleOption::}{rect} variable to decide
    where to draw and its \l{QStyleOption::}{state} to determine
    if it is enabled or selected.

    After painting, you should ensure that the painter is returned to
    the state it was supplied in when this function was called.
    For example, it may be useful to call QPainter::save() before
    painting and QPainter::restore() afterwards.

    \sa QItemDelegate::paint(), QStyle::drawControl(), QStyle::CE_ItemViewItem
*/
void QStyledItemDelegate::paint(QPainter *painter,
        const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QWidget *widget = QStyledItemDelegatePrivate::widget(option);
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
}

/*!
    Returns the size needed by the delegate to display the item
    specified by \a index, taking into account the style information
    provided by \a option.

    This function uses the view's QStyle to determine the size of the
    item.

    \sa QStyle::sizeFromContents(), QStyle::CT_ItemViewItem
*/
QSize QStyledItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    QVariant value = index.data(Qt::SizeHintRole);
    if (value.isValid())
        return qvariant_cast<QSize>(value);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const QWidget *widget = QStyledItemDelegatePrivate::widget(option);
    QStyle *style = widget ? widget->style() : QApplication::style();
    return style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), widget);
}

/*!
    Returns the widget used to edit the item specified by \a index
    for editing. The \a parent widget and style \a option are used to
    control how the editor widget appears.

    \sa QAbstractItemDelegate::createEditor()
*/
QWidget *QStyledItemDelegate::createEditor(QWidget *parent,
                                     const QStyleOptionViewItem &,
                                     const QModelIndex &index) const
{
    Q_D(const QStyledItemDelegate);
    if (!index.isValid())
        return 0;
    return d->editorFactory()->createEditor(index.data(Qt::EditRole).userType(), parent);
}

/*!
    Sets the data to be displayed and edited by the \a editor from the
    data model item specified by the model \a index.

    The default implementation stores the data in the \a editor
    widget's \l {Qt's Property System} {user property}.

    \sa QMetaProperty::isUser()
*/
void QStyledItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
#ifdef QT_NO_PROPERTIES
    Q_UNUSED(editor);
    Q_UNUSED(index);
#else
    QVariant v = index.data(Qt::EditRole);
    QByteArray n = editor->metaObject()->userProperty().name();

    if (!n.isEmpty()) {
        if (!v.isValid())
            v = QVariant(editor->property(n).userType(), (const void *)0);
        editor->setProperty(n, v);
    }
#endif
}

/*!
    Gets data from the \a editor widget and stores it in the specified
    \a model at the item \a index.

    The default implementation gets the value to be stored in the data
    model from the \a editor widget's \l {Qt's Property System} {user
    property}.

    \sa QMetaProperty::isUser()
*/
void QStyledItemDelegate::setModelData(QWidget *editor,
                                 QAbstractItemModel *model,
                                 const QModelIndex &index) const
{
#ifdef QT_NO_PROPERTIES
    Q_UNUSED(model);
    Q_UNUSED(editor);
    Q_UNUSED(index);
#else
    Q_D(const QStyledItemDelegate);
    Q_ASSERT(model);
    Q_ASSERT(editor);
    QByteArray n = editor->metaObject()->userProperty().name();
    if (n.isEmpty())
        n = d->editorFactory()->valuePropertyName(
            model->data(index, Qt::EditRole).userType());
    if (!n.isEmpty())
        model->setData(index, editor->property(n), Qt::EditRole);
#endif
}

/*!
    Updates the \a editor for the item specified by \a index
    according to the style \a option given.
*/
void QStyledItemDelegate::updateEditorGeometry(QWidget *editor,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    if (!editor)
        return;
    Q_ASSERT(index.isValid());
    const QWidget *widget = QStyledItemDelegatePrivate::widget(option);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    // let the editor take up all available space
    //if the editor is not a QLineEdit
    //or it is in a QTableView
#if QT_CONFIG(tableview) && QT_CONFIG(lineedit)
    if (qobject_cast<QExpandingLineEdit*>(editor) && !qobject_cast<const QTableView*>(widget))
        opt.showDecorationSelected = editor->style()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, 0, editor);
    else
#endif
        opt.showDecorationSelected = true;

    QStyle *style = widget ? widget->style() : QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);
    const int delta = qSmartMinSize(editor).width() - geom.width();
    if (delta > 0) {
        //we need to widen the geometry
        if (editor->layoutDirection() == Qt::RightToLeft)
            geom.adjust(-delta, 0, 0, 0);
        else
            geom.adjust(0, 0, delta, 0);
    }

    editor->setGeometry(geom);
}

/*!
  Returns the editor factory used by the item delegate.
  If no editor factory is set, the function will return null.

  \sa setItemEditorFactory()
*/
QItemEditorFactory *QStyledItemDelegate::itemEditorFactory() const
{
    Q_D(const QStyledItemDelegate);
    return d->factory;
}

/*!
  Sets the editor factory to be used by the item delegate to be the \a factory
  specified. If no editor factory is set, the item delegate will use the
  default editor factory.

  \sa itemEditorFactory()
*/
void QStyledItemDelegate::setItemEditorFactory(QItemEditorFactory *factory)
{
    Q_D(QStyledItemDelegate);
    d->factory = factory;
}


/*!
    \fn bool QStyledItemDelegate::eventFilter(QObject *editor, QEvent *event)

    Returns \c true if the given \a editor is a valid QWidget and the
    given \a event is handled; otherwise returns \c false. The following
    key press events are handled by default:

    \list
        \li \uicontrol Tab
        \li \uicontrol Backtab
        \li \uicontrol Enter
        \li \uicontrol Return
        \li \uicontrol Esc
    \endlist

    If the \a editor's type is QTextEdit or QPlainTextEdit then \uicontrol Enter and
    \uicontrol Return keys are \e not handled.

    In the case of \uicontrol Tab, \uicontrol Backtab, \uicontrol Enter and \uicontrol Return
    key press events, the \a editor's data is committed to the model
    and the editor is closed. If the \a event is a \uicontrol Tab key press
    the view will open an editor on the next item in the
    view. Likewise, if the \a event is a \uicontrol Backtab key press the
    view will open an editor on the \e previous item in the view.

    If the event is a \uicontrol Esc key press event, the \a editor is
    closed \e without committing its data.

    \sa commitData(), closeEditor()
*/
bool QStyledItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QStyledItemDelegate);
    return d->editorEventFilter(object, event);
}

/*!
  \reimp
*/
bool QStyledItemDelegate::editorEvent(QEvent *event,
                                QAbstractItemModel *model,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index)
{
    Q_ASSERT(event);
    Q_ASSERT(model);

    // make sure that the item is checkable
    Qt::ItemFlags flags = model->flags(index);
    if (!(flags & Qt::ItemIsUserCheckable) || !(option.state & QStyle::State_Enabled)
        || !(flags & Qt::ItemIsEnabled))
        return false;

    // make sure that we have a check state
    QVariant value = index.data(Qt::CheckStateRole);
    if (!value.isValid())
        return false;

    const QWidget *widget = QStyledItemDelegatePrivate::widget(option);
    QStyle *style = widget ? widget->style() : QApplication::style();

    // make sure that we have the right event type
    if ((event->type() == QEvent::MouseButtonRelease)
        || (event->type() == QEvent::MouseButtonDblClick)
        || (event->type() == QEvent::MouseButtonPress)) {
        QStyleOptionViewItem viewOpt(option);
        initStyleOption(&viewOpt, index);
        QRect checkRect = style->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &viewOpt, widget);
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton || !checkRect.contains(me->pos()))
            return false;

        if ((event->type() == QEvent::MouseButtonPress)
            || (event->type() == QEvent::MouseButtonDblClick))
            return true;

    } else if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space
         && static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select)
            return false;
    } else {
        return false;
    }

    Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
    if (flags & Qt::ItemIsUserTristate)
        state = ((Qt::CheckState)((state + 1) % 3));
    else
        state = (state == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
    return model->setData(index, state, Qt::CheckStateRole);
}

QT_END_NAMESPACE

#include "moc_qstyleditemdelegate.cpp"
