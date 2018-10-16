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

#include "qtoolbox.h"

#include <qapplication.h>
#include <qeventloop.h>
#include <qlayout.h>
#include <qlist.h>
#include <qpainter.h>
#include <qscrollarea.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qtooltip.h>
#include <qabstractbutton.h>

#include "qframe_p.h"

QT_BEGIN_NAMESPACE

class QToolBoxButton : public QAbstractButton
{
    Q_OBJECT
public:
    QToolBoxButton(QWidget *parent)
        : QAbstractButton(parent), selected(false), indexInPage(-1)
    {
        setBackgroundRole(QPalette::Window);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        setFocusPolicy(Qt::NoFocus);
    }

    inline void setSelected(bool b) { selected = b; update(); }
    inline void setIndex(int newIndex) { indexInPage = newIndex; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void initStyleOption(QStyleOptionToolBox *opt) const;
    void paintEvent(QPaintEvent *) override;

private:
    bool selected;
    int indexInPage;
};


class QToolBoxPrivate : public QFramePrivate
{
    Q_DECLARE_PUBLIC(QToolBox)
public:
    struct Page
    {
        QToolBoxButton *button;
        QScrollArea *sv;
        QWidget *widget;

        inline void setText(const QString &text) { button->setText(text); }
        inline void setIcon(const QIcon &is) { button->setIcon(is); }
#ifndef QT_NO_TOOLTIP
        inline void setToolTip(const QString &tip) { button->setToolTip(tip); }
        inline QString toolTip() const { return button->toolTip(); }
#endif
        inline QString text() const { return button->text(); }
        inline QIcon icon() const { return button->icon(); }

        inline bool operator==(const Page& other) const
        {
            return widget == other.widget;
        }
    };
    typedef QList<Page> PageList;

    inline QToolBoxPrivate()
        : currentPage(0)
    {
    }
    void _q_buttonClicked();
    void _q_widgetDestroyed(QObject*);

    const Page *page(const QObject *widget) const;
    const Page *page(int index) const;
    Page *page(int index);

    void updateTabs();
    void relayout();

    PageList pageList;
    QVBoxLayout *layout;
    Page *currentPage;
};

const QToolBoxPrivate::Page *QToolBoxPrivate::page(const QObject *widget) const
{
    if (!widget)
        return 0;

    for (PageList::ConstIterator i = pageList.constBegin(); i != pageList.constEnd(); ++i)
        if ((*i).widget == widget)
            return (const Page*) &(*i);
    return 0;
}

QToolBoxPrivate::Page *QToolBoxPrivate::page(int index)
{
    if (index >= 0 && index < pageList.size())
        return &pageList[index];
    return 0;
}

const QToolBoxPrivate::Page *QToolBoxPrivate::page(int index) const
{
    if (index >= 0 && index < pageList.size())
        return &pageList.at(index);
    return 0;
}

void QToolBoxPrivate::updateTabs()
{
    QToolBoxButton *lastButton = currentPage ? currentPage->button : 0;
    bool after = false;
    int index = 0;
    for (index = 0; index < pageList.count(); ++index) {
        const Page &page = pageList.at(index);
        QToolBoxButton *tB = page.button;
        // update indexes, since the updates are delayed, the indexes will be correct
        // when we actually paint.
        tB->setIndex(index);
        QWidget *tW = page.widget;
        if (after) {
            QPalette p = tB->palette();
            p.setColor(tB->backgroundRole(), tW->palette().color(tW->backgroundRole()));
            tB->setPalette(p);
            tB->update();
        } else if (tB->backgroundRole() != QPalette::Window) {
            tB->setBackgroundRole(QPalette::Window);
            tB->update();
        }
        after = tB == lastButton;
    }
}

QSize QToolBoxButton::sizeHint() const
{
    QSize iconSize(8, 8);
    if (!icon().isNull()) {
        int icone = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, parentWidget() /* QToolBox */);
        iconSize += QSize(icone + 2, icone);
    }
    QSize textSize = fontMetrics().size(Qt::TextShowMnemonic, text()) + QSize(0, 8);

    QSize total(iconSize.width() + textSize.width(), qMax(iconSize.height(), textSize.height()));
    return total.expandedTo(QApplication::globalStrut());
}

QSize QToolBoxButton::minimumSizeHint() const
{
    if (icon().isNull())
        return QSize();
    int icone = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, parentWidget() /* QToolBox */);
    return QSize(icone + 8, icone + 8);
}

void QToolBoxButton::initStyleOption(QStyleOptionToolBox *option) const
{
    if (!option)
        return;
    option->initFrom(this);
    if (selected)
        option->state |= QStyle::State_Selected;
    if (isDown())
        option->state |= QStyle::State_Sunken;
    option->text = text();
    option->icon = icon();

    QToolBox *toolBox = static_cast<QToolBox *>(parentWidget()); // I know I'm in a tool box.
    const int widgetCount = toolBox->count();
    const int currIndex = toolBox->currentIndex();
    if (widgetCount == 1) {
        option->position = QStyleOptionToolBox::OnlyOneTab;
    } else if (indexInPage == 0) {
        option->position = QStyleOptionToolBox::Beginning;
    } else if (indexInPage == widgetCount - 1) {
        option->position = QStyleOptionToolBox::End;
    } else {
        option->position = QStyleOptionToolBox::Middle;
    }
    if (currIndex == indexInPage - 1) {
        option->selectedPosition = QStyleOptionToolBox::PreviousIsSelected;
    } else if (currIndex == indexInPage + 1) {
        option->selectedPosition = QStyleOptionToolBox::NextIsSelected;
    } else {
        option->selectedPosition = QStyleOptionToolBox::NotAdjacent;
    }
}

void QToolBoxButton::paintEvent(QPaintEvent *)
{
    QPainter paint(this);
    QPainter *p = &paint;
    QStyleOptionToolBox opt;
    initStyleOption(&opt);
    style()->drawControl(QStyle::CE_ToolBoxTab, &opt, p, parentWidget());
}

/*!
    \class QToolBox

    \brief The QToolBox class provides a column of tabbed widget items.


    \ingroup basicwidgets
    \inmodule QtWidgets

    A toolbox is a widget that displays a column of tabs one above the
    other, with the current item displayed below the current tab.
    Every tab has an index position within the column of tabs. A tab's
    item is a QWidget.

    Each item has an itemText(), an optional itemIcon(), an optional
    itemToolTip(), and a widget(). The item's attributes can be
    changed with setItemText(), setItemIcon(), and
    setItemToolTip(). Each item can be enabled or disabled
    individually with setItemEnabled().

    Items are added using addItem(), or inserted at particular
    positions using insertItem(). The total number of items is given
    by count(). Items can be deleted with delete, or removed from the
    toolbox with removeItem(). Combining removeItem() and insertItem()
    allows you to move items to different positions.

    The index of the current item widget is returned by currentIndex(),
    and set with setCurrentIndex(). The index of a particular item can
    be found using indexOf(), and the item at a given index is returned
    by item().

    The currentChanged() signal is emitted when the current item is
    changed.

    \sa QTabWidget
*/

/*!
    \fn void QToolBox::currentChanged(int index)

    This signal is emitted when the current item is changed. The new
    current item's index is passed in \a index, or -1 if there is no
    current item.
*/


/*!
    Constructs a new toolbox with the given \a parent and the flags, \a f.
*/
QToolBox::QToolBox(QWidget *parent, Qt::WindowFlags f)
    :  QFrame(*new QToolBoxPrivate, parent, f)
{
    Q_D(QToolBox);
    d->layout = new QVBoxLayout(this);
    d->layout->setMargin(0);
    setBackgroundRole(QPalette::Button);
}

/*!
    Destroys the toolbox.
*/

QToolBox::~QToolBox()
{
}

/*!
    \fn int QToolBox::addItem(QWidget *w, const QString &text)
    \overload

    Adds the widget \a w in a new tab at bottom of the toolbox. The
    new tab's text is set to \a text. Returns the new tab's index.
*/

/*!
    \fn int QToolBox::addItem(QWidget *widget, const QIcon &iconSet,const QString &text)
    Adds the \a widget in a new tab at bottom of the toolbox. The
    new tab's text is set to \a text, and the \a iconSet is
    displayed to the left of the \a text.  Returns the new tab's index.
*/

/*!
    \fn int QToolBox::insertItem(int index, QWidget *widget, const QString &text)
    \overload

    Inserts the \a widget at position \a index, or at the bottom
    of the toolbox if \a index is out of range. The new item's text is
    set to \a text. Returns the new item's index.
*/

/*!
    Inserts the \a widget at position \a index, or at the bottom
    of the toolbox if \a index is out of range. The new item's text
    is set to \a text, and the \a icon is displayed to the left of
    the \a text. Returns the new item's index.
*/

int QToolBox::insertItem(int index, QWidget *widget, const QIcon &icon, const QString &text)
{
    if (!widget)
        return -1;

    Q_D(QToolBox);
    connect(widget, SIGNAL(destroyed(QObject*)), this, SLOT(_q_widgetDestroyed(QObject*)));

    QToolBoxPrivate::Page c;
    c.widget = widget;
    c.button = new QToolBoxButton(this);
    c.button->setObjectName(QLatin1String("qt_toolbox_toolboxbutton"));
    connect(c.button, SIGNAL(clicked()), this, SLOT(_q_buttonClicked()));

    c.sv = new QScrollArea(this);
    c.sv->setWidget(widget);
    c.sv->setWidgetResizable(true);
    c.sv->hide();
    c.sv->setFrameStyle(QFrame::NoFrame);

    c.setText(text);
    c.setIcon(icon);

    if (index < 0 || index >= (int)d->pageList.count()) {
        index = d->pageList.count();
        d->pageList.append(c);
        d->layout->addWidget(c.button);
        d->layout->addWidget(c.sv);
        if (index == 0)
            setCurrentIndex(index);
    } else {
        d->pageList.insert(index, c);
        d->relayout();
        if (d->currentPage) {
            QWidget *current = d->currentPage->widget;
            int oldindex = indexOf(current);
            if (index <= oldindex) {
                d->currentPage = 0; // trigger change
                setCurrentIndex(oldindex);
            }
        }
    }

    c.button->show();

    d->updateTabs();
    itemInserted(index);
    return index;
}

void QToolBoxPrivate::_q_buttonClicked()
{
    Q_Q(QToolBox);
    QToolBoxButton *tb = qobject_cast<QToolBoxButton*>(q->sender());
    QWidget* item = 0;
    for (QToolBoxPrivate::PageList::ConstIterator i = pageList.constBegin(); i != pageList.constEnd(); ++i)
        if ((*i).button == tb) {
            item = (*i).widget;
            break;
        }

    q->setCurrentIndex(q->indexOf(item));
}

/*!
    \property QToolBox::count
    \brief The number of items contained in the toolbox.

    By default, this property has a value of 0.
*/

int QToolBox::count() const
{
    Q_D(const QToolBox);
    return d->pageList.count();
}

void QToolBox::setCurrentIndex(int index)
{
    Q_D(QToolBox);
    QToolBoxPrivate::Page *c = d->page(index);
    if (!c || d->currentPage == c)
        return;

    c->button->setSelected(true);
    if (d->currentPage) {
        d->currentPage->sv->hide();
        d->currentPage->button->setSelected(false);
    }
    d->currentPage = c;
    d->currentPage->sv->show();
    d->updateTabs();
    emit currentChanged(index);
}

void QToolBoxPrivate::relayout()
{
    Q_Q(QToolBox);
    delete layout;
    layout = new QVBoxLayout(q);
    layout->setMargin(0);
    for (QToolBoxPrivate::PageList::ConstIterator i = pageList.constBegin(); i != pageList.constEnd(); ++i) {
        layout->addWidget((*i).button);
        layout->addWidget((*i).sv);
    }
}

void QToolBoxPrivate::_q_widgetDestroyed(QObject *object)
{
    Q_Q(QToolBox);

    const QToolBoxPrivate::Page * const c = page(object);
    if (!c)
        return;

    layout->removeWidget(c->sv);
    layout->removeWidget(c->button);
    c->sv->deleteLater(); // page might still be a child of sv
    delete c->button;

    bool removeCurrent = c == currentPage;
    pageList.removeAll(*c);

    if (!pageList.count()) {
        currentPage = 0;
        emit q->currentChanged(-1);
    } else if (removeCurrent) {
        currentPage = 0;
        q->setCurrentIndex(0);
    }
}

/*!
    Removes the item at position \a index from the toolbox. Note that
    the widget is \e not deleted.
*/

void QToolBox::removeItem(int index)
{
    Q_D(QToolBox);
    if (QWidget *w = widget(index)) {
        disconnect(w, SIGNAL(destroyed(QObject*)), this, SLOT(_q_widgetDestroyed(QObject*)));
        w->setParent(this);
        // destroy internal data
        d->_q_widgetDestroyed(w);
        itemRemoved(index);
    }
}


/*!
    \property QToolBox::currentIndex
    \brief the index of the current item

    By default, for an empty toolbox, this property has a value of -1.

    \sa indexOf(), widget()
*/


int QToolBox::currentIndex() const
{
    Q_D(const QToolBox);
    return d->currentPage ? indexOf(d->currentPage->widget) : -1;
}

/*!
    Returns a pointer to the current widget, or 0 if there is no such item.

    \sa currentIndex(), setCurrentWidget()
*/

QWidget * QToolBox::currentWidget() const
{
    Q_D(const QToolBox);
    return d->currentPage ? d->currentPage->widget : 0;
}

/*!
  Makes\a widget the current widget. The \a widget must be an item in this tool box.

  \sa addItem(), setCurrentIndex(), currentWidget()
 */
void QToolBox::setCurrentWidget(QWidget *widget)
{
    int i = indexOf(widget);
    if (Q_UNLIKELY(i < 0))
        qWarning("QToolBox::setCurrentWidget: widget not contained in tool box");
    else
        setCurrentIndex(i);
}

/*!
    Returns the widget at position \a index, or 0 if there is no such
    item.
*/

QWidget *QToolBox::widget(int index) const
{
    Q_D(const QToolBox);
    if (index < 0 || index >= (int) d->pageList.size())
        return 0;
    return d->pageList.at(index).widget;
}

/*!
    Returns the index of \a widget, or -1 if the item does not
    exist.
*/

int QToolBox::indexOf(QWidget *widget) const
{
    Q_D(const QToolBox);
    const QToolBoxPrivate::Page *c = (widget ? d->page(widget) : 0);
    return c ? d->pageList.indexOf(*c) : -1;
}

/*!
    If \a enabled is true then the item at position \a index is enabled; otherwise
    the item at position \a index is disabled.
*/

void QToolBox::setItemEnabled(int index, bool enabled)
{
    Q_D(QToolBox);
    QToolBoxPrivate::Page *c = d->page(index);
    if (!c)
        return;

    c->button->setEnabled(enabled);
    if (!enabled && c == d->currentPage) {
        int curIndexUp = index;
        int curIndexDown = curIndexUp;
        const int count = d->pageList.count();
        while (curIndexUp > 0 || curIndexDown < count-1) {
            if (curIndexDown < count-1) {
                if (d->page(++curIndexDown)->button->isEnabled()) {
                    index = curIndexDown;
                    break;
                }
            }
            if (curIndexUp > 0) {
                if (d->page(--curIndexUp)->button->isEnabled()) {
                    index = curIndexUp;
                    break;
                }
            }
        }
        setCurrentIndex(index);
    }
}


/*!
    Sets the text of the item at position \a index to \a text.

    If the provided text contains an ampersand character ('&'), a
    mnemonic is automatically created for it. The character that
    follows the '&' will be used as the shortcut key. Any previous
    mnemonic will be overwritten, or cleared if no mnemonic is defined
    by the text. See the \l {QShortcut#mnemonic}{QShortcut}
    documentation for details (to display an actual ampersand, use
    '&&').
*/

void QToolBox::setItemText(int index, const QString &text)
{
    Q_D(QToolBox);
    QToolBoxPrivate::Page *c = d->page(index);
    if (c)
        c->setText(text);
}

/*!
    Sets the icon of the item at position \a index to \a icon.
*/

void QToolBox::setItemIcon(int index, const QIcon &icon)
{
    Q_D(QToolBox);
    QToolBoxPrivate::Page *c = d->page(index);
    if (c)
        c->setIcon(icon);
}

#ifndef QT_NO_TOOLTIP
/*!
    Sets the tooltip of the item at position \a index to \a toolTip.
*/

void QToolBox::setItemToolTip(int index, const QString &toolTip)
{
    Q_D(QToolBox);
    QToolBoxPrivate::Page *c = d->page(index);
    if (c)
        c->setToolTip(toolTip);
}
#endif // QT_NO_TOOLTIP

/*!
    Returns \c true if the item at position \a index is enabled; otherwise returns \c false.
*/

bool QToolBox::isItemEnabled(int index) const
{
    Q_D(const QToolBox);
    const QToolBoxPrivate::Page *c = d->page(index);
    return c && c->button->isEnabled();
}

/*!
    Returns the text of the item at position \a index, or an empty string if
    \a index is out of range.
*/

QString QToolBox::itemText(int index) const
{
    Q_D(const QToolBox);
    const QToolBoxPrivate::Page *c = d->page(index);
    return (c ? c->text() : QString());
}

/*!
    Returns the icon of the item at position \a index, or a null
    icon if \a index is out of range.
*/

QIcon QToolBox::itemIcon(int index) const
{
    Q_D(const QToolBox);
    const QToolBoxPrivate::Page *c = d->page(index);
    return (c ? c->icon() : QIcon());
}

#ifndef QT_NO_TOOLTIP
/*!
    Returns the tooltip of the item at position \a index, or an
    empty string if \a index is out of range.
*/

QString QToolBox::itemToolTip(int index) const
{
    Q_D(const QToolBox);
    const QToolBoxPrivate::Page *c = d->page(index);
    return (c ? c->toolTip() : QString());
}
#endif // QT_NO_TOOLTIP

/*! \reimp */
void QToolBox::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
}

/*! \reimp */
void QToolBox::changeEvent(QEvent *ev)
{
    Q_D(QToolBox);
    if(ev->type() == QEvent::StyleChange)
        d->updateTabs();
    QFrame::changeEvent(ev);
}

/*!
  This virtual handler is called after a new item was added or
  inserted at position \a index.

  \sa itemRemoved()
 */
void QToolBox::itemInserted(int index)
{
    Q_UNUSED(index)
}

/*!
  This virtual handler is called after an item was removed from
  position \a index.

  \sa itemInserted()
 */
void QToolBox::itemRemoved(int index)
{
    Q_UNUSED(index)
}

/*! \reimp */
bool QToolBox::event(QEvent *e)
{
    return QFrame::event(e);
}

QT_END_NAMESPACE

#include "moc_qtoolbox.cpp"
#include "qtoolbox.moc"
