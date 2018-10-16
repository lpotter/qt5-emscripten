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

#include "qcombobox.h"

#include <qstylepainter.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatformmenu.h>
#include <qlineedit.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
#include <private/qdesktopwidget_p.h>
#include <qlistview.h>
#if QT_CONFIG(tableview)
#include <qtableview.h>
#endif
#include <qitemdelegate.h>
#include <qmap.h>
#if QT_CONFIG(menu)
#include <qmenu.h>
#endif
#include <qevent.h>
#include <qlayout.h>
#include <qscrollbar.h>
#if QT_CONFIG(treeview)
#include <qtreeview.h>
#endif
#include <qheaderview.h>
#include <qmath.h>
#include <qmetaobject.h>
#include <qabstractproxymodel.h>
#include <qstylehints.h>
#include <private/qguiapplication_p.h>
#include <private/qhighdpiscaling_p.h>
#include <private/qapplication_p.h>
#include <private/qcombobox_p.h>
#include <private/qabstractitemmodel_p.h>
#include <private/qabstractscrollarea_p.h>
#include <private/qlineedit_p.h>
#if QT_CONFIG(completer)
#include <private/qcompleter_p.h>
#endif
#include <qdebug.h>
#if QT_CONFIG(effects)
# include <private/qeffects_p.h>
#endif
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif

QT_BEGIN_NAMESPACE

QComboBoxPrivate::QComboBoxPrivate()
    : QWidgetPrivate(),
      model(0),
      lineEdit(0),
      container(0),
      insertPolicy(QComboBox::InsertAtBottom),
      sizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow),
      minimumContentsLength(0),
      shownOnce(false),
      autoCompletion(true),
      duplicatesEnabled(false),
      frame(true),
      maxVisibleItems(10),
      maxCount(INT_MAX),
      modelColumn(0),
      inserting(false),
      arrowState(QStyle::State_None),
      hoverControl(QStyle::SC_None),
      autoCompletionCaseSensitivity(Qt::CaseInsensitive),
      indexBeforeChange(-1)
#ifdef Q_OS_MAC
      , m_platformMenu(0)
#endif
#if QT_CONFIG(completer)
      , completer(0)
#endif
{
}

QComboBoxPrivate::~QComboBoxPrivate()
{
#ifdef Q_OS_MAC
    cleanupNativePopup();
#endif
}

QStyleOptionMenuItem QComboMenuDelegate::getStyleOption(const QStyleOptionViewItem &option,
                                                        const QModelIndex &index) const
{
    QStyleOptionMenuItem menuOption;

    QPalette resolvedpalette = option.palette.resolve(QApplication::palette("QMenu"));
    QVariant value = index.data(Qt::ForegroundRole);
    if (value.canConvert<QBrush>()) {
        resolvedpalette.setBrush(QPalette::WindowText, qvariant_cast<QBrush>(value));
        resolvedpalette.setBrush(QPalette::ButtonText, qvariant_cast<QBrush>(value));
        resolvedpalette.setBrush(QPalette::Text, qvariant_cast<QBrush>(value));
    }
    menuOption.palette = resolvedpalette;
    menuOption.state = QStyle::State_None;
    if (mCombo->window()->isActiveWindow())
        menuOption.state = QStyle::State_Active;
    if ((option.state & QStyle::State_Enabled) && (index.model()->flags(index) & Qt::ItemIsEnabled))
        menuOption.state |= QStyle::State_Enabled;
    else
        menuOption.palette.setCurrentColorGroup(QPalette::Disabled);
    if (option.state & QStyle::State_Selected)
        menuOption.state |= QStyle::State_Selected;
    menuOption.checkType = QStyleOptionMenuItem::NonExclusive;
    menuOption.checked = mCombo->currentIndex() == index.row();
    if (QComboBoxDelegate::isSeparator(index))
        menuOption.menuItemType = QStyleOptionMenuItem::Separator;
    else
        menuOption.menuItemType = QStyleOptionMenuItem::Normal;

    QVariant variant = index.model()->data(index, Qt::DecorationRole);
    switch (variant.type()) {
    case QVariant::Icon:
        menuOption.icon = qvariant_cast<QIcon>(variant);
        break;
    case QVariant::Color: {
        static QPixmap pixmap(option.decorationSize);
        pixmap.fill(qvariant_cast<QColor>(variant));
        menuOption.icon = pixmap;
        break; }
    default:
        menuOption.icon = qvariant_cast<QPixmap>(variant);
        break;
    }
    if (index.data(Qt::BackgroundRole).canConvert<QBrush>()) {
        menuOption.palette.setBrush(QPalette::All, QPalette::Background,
                                    qvariant_cast<QBrush>(index.data(Qt::BackgroundRole)));
    }
    menuOption.text = index.model()->data(index, Qt::DisplayRole).toString()
                           .replace(QLatin1Char('&'), QLatin1String("&&"));
    menuOption.tabWidth = 0;
    menuOption.maxIconWidth =  option.decorationSize.width() + 4;
    menuOption.menuRect = option.rect;
    menuOption.rect = option.rect;

    // Make sure fonts set on the model or on the combo box, in
    // that order, also override the font for the popup menu.
    QVariant fontRoleData = index.data(Qt::FontRole);
    if (fontRoleData.isValid()) {
        menuOption.font = fontRoleData.value<QFont>();
    } else if (mCombo->testAttribute(Qt::WA_SetFont)
            || mCombo->testAttribute(Qt::WA_MacSmallSize)
            || mCombo->testAttribute(Qt::WA_MacMiniSize)
            || mCombo->font() != qt_app_fonts_hash()->value("QComboBox", QFont())) {
        menuOption.font = mCombo->font();
    } else {
        menuOption.font = qt_app_fonts_hash()->value("QComboMenuItem", mCombo->font());
    }

    menuOption.fontMetrics = QFontMetrics(menuOption.font);

    return menuOption;
}

#if QT_CONFIG(completer)
void QComboBoxPrivate::_q_completerActivated(const QModelIndex &index)
{
    Q_Q(QComboBox);
    if (index.isValid() && q->completer()) {
        QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel *>(q->completer()->completionModel());
        if (proxy) {
            const QModelIndex &completerIndex = proxy->mapToSource(index);
            int row = -1;
            if (completerIndex.model() == model) {
                row = completerIndex.row();
            } else {
                // if QCompleter uses a proxy model to host widget's one - map again
                QAbstractProxyModel *completerProxy = qobject_cast<QAbstractProxyModel *>(q->completer()->model());
                if (completerProxy && completerProxy->sourceModel() == model) {
                    row = completerProxy->mapToSource(completerIndex).row();
                } else {
                    QString match = q->completer()->model()->data(completerIndex).toString();
                    row = q->findText(match, matchFlags());
                }
            }
            q->setCurrentIndex(row);
            emitActivated(currentIndex);
        }
    }

#  ifdef QT_KEYPAD_NAVIGATION
    if ( QApplication::keypadNavigationEnabled()
         && q->isEditable()
         && q->completer()
         && q->completer()->completionMode() == QCompleter::UnfilteredPopupCompletion ) {
        q->setEditFocus(false);
    }
#  endif // QT_KEYPAD_NAVIGATION
}
#endif // QT_CONFIG(completer)

void QComboBoxPrivate::updateArrow(QStyle::StateFlag state)
{
    Q_Q(QComboBox);
    if (arrowState == state)
        return;
    arrowState = state;
    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    q->update(q->rect());
}

void QComboBoxPrivate::_q_modelReset()
{
    Q_Q(QComboBox);
    if (lineEdit) {
        lineEdit->setText(QString());
        updateLineEditGeometry();
    }
    if (currentIndex.row() != indexBeforeChange)
        _q_emitCurrentIndexChanged(currentIndex);
    modelChanged();
    q->update();
}

void QComboBoxPrivate::_q_modelDestroyed()
{
    model = QAbstractItemModelPrivate::staticEmptyModel();
}


//Windows and KDE allows menus to cover the taskbar, while GNOME and Mac don't
QRect QComboBoxPrivate::popupGeometry(int screen) const
{
    bool useFullScreenForPopupMenu = false;
    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        useFullScreenForPopupMenu = theme->themeHint(QPlatformTheme::UseFullScreenForPopupMenu).toBool();
    return useFullScreenForPopupMenu ?
           QDesktopWidgetPrivate::screenGeometry(screen) :
           QDesktopWidgetPrivate::availableGeometry(screen);
}

bool QComboBoxPrivate::updateHoverControl(const QPoint &pos)
{

    Q_Q(QComboBox);
    QRect lastHoverRect = hoverRect;
    QStyle::SubControl lastHoverControl = hoverControl;
    bool doesHover = q->testAttribute(Qt::WA_Hover);
    if (lastHoverControl != newHoverControl(pos) && doesHover) {
        q->update(lastHoverRect);
        q->update(hoverRect);
        return true;
    }
    return !doesHover;
}

QStyle::SubControl QComboBoxPrivate::newHoverControl(const QPoint &pos)
{
    Q_Q(QComboBox);
    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    hoverControl = q->style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt, pos, q);
    hoverRect = (hoverControl != QStyle::SC_None)
                   ? q->style()->subControlRect(QStyle::CC_ComboBox, &opt, hoverControl, q)
                   : QRect();
    return hoverControl;
}

/*
    Computes a size hint based on the maximum width
    for the items in the combobox.
*/
int QComboBoxPrivate::computeWidthHint() const
{
    Q_Q(const QComboBox);

    int width = 0;
    const int count = q->count();
    const int iconWidth = q->iconSize().width() + 4;
    const QFontMetrics &fontMetrics = q->fontMetrics();

    for (int i = 0; i < count; ++i) {
        const int textWidth = fontMetrics.horizontalAdvance(q->itemText(i));
        if (q->itemIcon(i).isNull())
            width = (qMax(width, textWidth));
        else
            width = (qMax(width, textWidth + iconWidth));
    }

    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    QSize tmp(width, 0);
    tmp = q->style()->sizeFromContents(QStyle::CT_ComboBox, &opt, tmp, q);
    return tmp.width();
}

QSize QComboBoxPrivate::recomputeSizeHint(QSize &sh) const
{
    Q_Q(const QComboBox);
    if (!sh.isValid()) {
        bool hasIcon = sizeAdjustPolicy == QComboBox::AdjustToMinimumContentsLengthWithIcon;
        int count = q->count();
        QSize iconSize = q->iconSize();
        const QFontMetrics &fm = q->fontMetrics();

        // text width
        if (&sh == &sizeHint || minimumContentsLength == 0) {
            switch (sizeAdjustPolicy) {
            case QComboBox::AdjustToContents:
            case QComboBox::AdjustToContentsOnFirstShow:
                if (count == 0) {
                    sh.rwidth() = 7 * fm.horizontalAdvance(QLatin1Char('x'));
                } else {
                    for (int i = 0; i < count; ++i) {
                        if (!q->itemIcon(i).isNull()) {
                            hasIcon = true;
                            sh.setWidth(qMax(sh.width(), fm.boundingRect(q->itemText(i)).width() + iconSize.width() + 4));
                        } else {
                            sh.setWidth(qMax(sh.width(), fm.boundingRect(q->itemText(i)).width()));
                        }
                    }
                }
                break;
            case QComboBox::AdjustToMinimumContentsLength:
                for (int i = 0; i < count && !hasIcon; ++i)
                    hasIcon = !q->itemIcon(i).isNull();
            default:
                ;
            }
        } else {
            for (int i = 0; i < count && !hasIcon; ++i)
                hasIcon = !q->itemIcon(i).isNull();
        }
        if (minimumContentsLength > 0)
            sh.setWidth(qMax(sh.width(), minimumContentsLength * fm.horizontalAdvance(QLatin1Char('X')) + (hasIcon ? iconSize.width() + 4 : 0)));


        // height
        sh.setHeight(qMax(qCeil(QFontMetricsF(fm).height()), 14) + 2);
        if (hasIcon) {
            sh.setHeight(qMax(sh.height(), iconSize.height() + 2));
        }

        // add style and strut values
        QStyleOptionComboBox opt;
        q->initStyleOption(&opt);
        sh = q->style()->sizeFromContents(QStyle::CT_ComboBox, &opt, sh, q);
    }
    return sh.expandedTo(QApplication::globalStrut());
}

void QComboBoxPrivate::adjustComboBoxSize()
{
    viewContainer()->adjustSizeTimer.start(20, container);
}

void QComboBoxPrivate::updateLayoutDirection()
{
    Q_Q(const QComboBox);
    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    Qt::LayoutDirection dir = Qt::LayoutDirection(
        q->style()->styleHint(QStyle::SH_ComboBox_LayoutDirection, &opt, q));
    if (lineEdit)
        lineEdit->setLayoutDirection(dir);
    if (container)
        container->setLayoutDirection(dir);
}


void QComboBoxPrivateContainer::timerEvent(QTimerEvent *timerEvent)
{
    if (timerEvent->timerId() == adjustSizeTimer.timerId()) {
        adjustSizeTimer.stop();
        if (combo->sizeAdjustPolicy() == QComboBox::AdjustToContents) {
            combo->updateGeometry();
            combo->adjustSize();
            combo->update();
        }
    }
}

void QComboBoxPrivateContainer::resizeEvent(QResizeEvent *e)
{
    QStyleOptionComboBox opt = comboStyleOption();
    if (combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo)) {
        QStyleOption myOpt;
        myOpt.initFrom(this);
        QStyleHintReturnMask mask;
        if (combo->style()->styleHint(QStyle::SH_Menu_Mask, &myOpt, this, &mask)) {
            setMask(mask.region);
        }
    } else {
        clearMask();
    }
    QFrame::resizeEvent(e);
}

void QComboBoxPrivateContainer::paintEvent(QPaintEvent *e)
{
    QStyleOptionComboBox cbOpt = comboStyleOption();
    if (combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &cbOpt, combo)
            && mask().isEmpty()) {
        QStyleOption opt;
        opt.initFrom(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_PanelMenu, &opt, &p, this);
    }

    QFrame::paintEvent(e);
}

void QComboBoxPrivateContainer::leaveEvent(QEvent *)
{
// On Mac using the Mac style we want to clear the selection
// when the mouse moves outside the popup.
#if 0 // Used to be included in Qt4 for Q_WS_MAC
    QStyleOptionComboBox opt = comboStyleOption();
    if (combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo))
          view->clearSelection();
#endif
}

QComboBoxPrivateContainer::QComboBoxPrivateContainer(QAbstractItemView *itemView, QComboBox *parent)
    : QFrame(parent, Qt::Popup), combo(parent), view(0), top(0), bottom(0), maybeIgnoreMouseButtonRelease(false)
{
    // we need the combobox and itemview
    Q_ASSERT(parent);
    Q_ASSERT(itemView);

    setAttribute(Qt::WA_WindowPropagation);
    setAttribute(Qt::WA_X11NetWmWindowTypeCombo);

    // setup container
    blockMouseReleaseTimer.setSingleShot(true);

    // we need a vertical layout
    QBoxLayout *layout =  new QBoxLayout(QBoxLayout::TopToBottom, this);
    layout->setSpacing(0);
    layout->setMargin(0);

    // set item view
    setItemView(itemView);

    // add scroller arrows if style needs them
    QStyleOptionComboBox opt = comboStyleOption();
    const bool usePopup = combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo);
    if (usePopup) {
        top = new QComboBoxPrivateScroller(QAbstractSlider::SliderSingleStepSub, this);
        bottom = new QComboBoxPrivateScroller(QAbstractSlider::SliderSingleStepAdd, this);
        top->hide();
        bottom->hide();
    } else {
        setLineWidth(1);
    }

    setFrameStyle(combo->style()->styleHint(QStyle::SH_ComboBox_PopupFrameStyle, &opt, combo));

    if (top) {
        layout->insertWidget(0, top);
        connect(top, SIGNAL(doScroll(int)), this, SLOT(scrollItemView(int)));
    }
    if (bottom) {
        layout->addWidget(bottom);
        connect(bottom, SIGNAL(doScroll(int)), this, SLOT(scrollItemView(int)));
    }

    // Some styles (Mac) have a margin at the top and bottom of the popup.
    layout->insertSpacing(0, 0);
    layout->addSpacing(0);
    updateTopBottomMargin();
}

void QComboBoxPrivateContainer::scrollItemView(int action)
{
#if QT_CONFIG(scrollbar)
    if (view->verticalScrollBar())
        view->verticalScrollBar()->triggerAction(static_cast<QAbstractSlider::SliderAction>(action));
#endif
}

void QComboBoxPrivateContainer::hideScrollers()
{
    if (top)
        top->hide();
    if (bottom)
        bottom->hide();
}

/*
    Hides or shows the scrollers when we emulate a popupmenu
*/
void QComboBoxPrivateContainer::updateScrollers()
{
#if QT_CONFIG(scrollbar)
    if (!top || !bottom)
        return;

    if (isVisible() == false)
        return;

    QStyleOptionComboBox opt = comboStyleOption();
    if (combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo) &&
        view->verticalScrollBar()->minimum() < view->verticalScrollBar()->maximum()) {

        bool needTop = view->verticalScrollBar()->value()
                       > (view->verticalScrollBar()->minimum() + topMargin());
        bool needBottom = view->verticalScrollBar()->value()
                          < (view->verticalScrollBar()->maximum() - bottomMargin() - topMargin());
        if (needTop)
            top->show();
        else
            top->hide();
        if (needBottom)
            bottom->show();
        else
            bottom->hide();
    } else {
        top->hide();
        bottom->hide();
    }
#endif // QT_CONFIG(scrollbar)
}

/*
    Cleans up when the view is destroyed.
*/
void QComboBoxPrivateContainer::viewDestroyed()
{
    view = 0;
    setItemView(new QComboBoxListView());
}

/*
    Returns the item view used for the combobox popup.
*/
QAbstractItemView *QComboBoxPrivateContainer::itemView() const
{
    return view;
}

/*!
    Sets the item view to be used for the combobox popup.
*/
void QComboBoxPrivateContainer::setItemView(QAbstractItemView *itemView)
{
    Q_ASSERT(itemView);

    // clean up old one
    if (view) {
        view->removeEventFilter(this);
        view->viewport()->removeEventFilter(this);
#if QT_CONFIG(scrollbar)
        disconnect(view->verticalScrollBar(), SIGNAL(valueChanged(int)),
                   this, SLOT(updateScrollers()));
        disconnect(view->verticalScrollBar(), SIGNAL(rangeChanged(int,int)),
                   this, SLOT(updateScrollers()));
#endif
        disconnect(view, SIGNAL(destroyed()),
                   this, SLOT(viewDestroyed()));

        if (isAncestorOf(view))
            delete view;
        view = 0;
    }

    // setup the item view
    view = itemView;
    view->setParent(this);
    view->setAttribute(Qt::WA_MacShowFocusRect, false);
    qobject_cast<QBoxLayout*>(layout())->insertWidget(top ? 2 : 0, view);
    view->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    view->installEventFilter(this);
    view->viewport()->installEventFilter(this);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QStyleOptionComboBox opt = comboStyleOption();
    const bool usePopup = combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo);
#if QT_CONFIG(scrollbar)
    if (usePopup)
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#endif
    if (combo->style()->styleHint(QStyle::SH_ComboBox_ListMouseTracking, &opt, combo) ||
        usePopup) {
        view->setMouseTracking(true);
    }
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setFrameStyle(QFrame::NoFrame);
    view->setLineWidth(0);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
#if QT_CONFIG(scrollbar)
    connect(view->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(updateScrollers()));
    connect(view->verticalScrollBar(), SIGNAL(rangeChanged(int,int)),
            this, SLOT(updateScrollers()));
#endif
    connect(view, SIGNAL(destroyed()),
            this, SLOT(viewDestroyed()));
}

/*!
    Returns the top/bottom vertical margin of the view.
*/
int QComboBoxPrivateContainer::topMargin() const
{
    if (const QListView *lview = qobject_cast<const QListView*>(view))
        return lview->spacing();
#if QT_CONFIG(tableview)
    if (const QTableView *tview = qobject_cast<const QTableView*>(view))
        return tview->showGrid() ? 1 : 0;
#endif
    return 0;
}

/*!
    Returns the spacing between the items in the view.
*/
int QComboBoxPrivateContainer::spacing() const
{
    QListView *lview = qobject_cast<QListView*>(view);
    if (lview)
        return 2 * lview->spacing(); // QListView::spacing is the padding around the item.
#if QT_CONFIG(tableview)
    QTableView *tview = qobject_cast<QTableView*>(view);
    if (tview)
        return tview->showGrid() ? 1 : 0;
#endif
    return 0;
}

void QComboBoxPrivateContainer::updateTopBottomMargin()
{
    if (!layout() || layout()->count() < 1)
        return;

    QBoxLayout *boxLayout = qobject_cast<QBoxLayout *>(layout());
    if (!boxLayout)
        return;

    const QStyleOptionComboBox opt = comboStyleOption();
    const bool usePopup = combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo);
    const int margin = usePopup ? combo->style()->pixelMetric(QStyle::PM_MenuVMargin, &opt, combo) : 0;

    QSpacerItem *topSpacer = boxLayout->itemAt(0)->spacerItem();
    if (topSpacer)
        topSpacer->changeSize(0, margin, QSizePolicy::Minimum, QSizePolicy::Fixed);

    QSpacerItem *bottomSpacer = boxLayout->itemAt(boxLayout->count() - 1)->spacerItem();
    if (bottomSpacer && bottomSpacer != topSpacer)
        bottomSpacer->changeSize(0, margin, QSizePolicy::Minimum, QSizePolicy::Fixed);

    boxLayout->invalidate();
}

void QComboBoxPrivateContainer::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::StyleChange) {
        QStyleOptionComboBox opt = comboStyleOption();
        view->setMouseTracking(combo->style()->styleHint(QStyle::SH_ComboBox_ListMouseTracking, &opt, combo) ||
                               combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo));
        setFrameStyle(combo->style()->styleHint(QStyle::SH_ComboBox_PopupFrameStyle, &opt, combo));
    }

    QWidget::changeEvent(e);
}


bool QComboBoxPrivateContainer::eventFilter(QObject *o, QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride: {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(e);
        switch (keyEvent->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
#ifdef QT_KEYPAD_NAVIGATION
        case Qt::Key_Select:
#endif
            if (view->currentIndex().isValid() && (view->currentIndex().flags() & Qt::ItemIsEnabled) ) {
                combo->hidePopup();
                emit itemSelected(view->currentIndex());
            }
            return true;
        case Qt::Key_Down:
            if (!(keyEvent->modifiers() & Qt::AltModifier))
                break;
            Q_FALLTHROUGH();
        case Qt::Key_F4:
            combo->hidePopup();
            return true;
        default:
#if QT_CONFIG(shortcut)
            if (keyEvent->matches(QKeySequence::Cancel)) {
                combo->hidePopup();
                return true;
            }
#endif
            break;
        }
        break;
    }
    case QEvent::MouseMove:
        if (isVisible()) {
            QMouseEvent *m = static_cast<QMouseEvent *>(e);
            QWidget *widget = static_cast<QWidget *>(o);
            QPoint vector = widget->mapToGlobal(m->pos()) - initialClickPosition;
            if (vector.manhattanLength() > 9 && blockMouseReleaseTimer.isActive())
                blockMouseReleaseTimer.stop();
            QModelIndex indexUnderMouse = view->indexAt(m->pos());
            if (indexUnderMouse.isValid()
                     && !QComboBoxDelegate::isSeparator(indexUnderMouse)) {
                view->setCurrentIndex(indexUnderMouse);
            }
        }
        break;
    case QEvent::MouseButtonPress:
        maybeIgnoreMouseButtonRelease = false;
        break;
    case QEvent::MouseButtonRelease: {
        bool ignoreEvent = maybeIgnoreMouseButtonRelease && popupTimer.elapsed() < QApplication::doubleClickInterval();

        QMouseEvent *m = static_cast<QMouseEvent *>(e);
        if (isVisible() && view->rect().contains(m->pos()) && view->currentIndex().isValid()
            && !blockMouseReleaseTimer.isActive() && !ignoreEvent
            && (view->currentIndex().flags() & Qt::ItemIsEnabled)
            && (view->currentIndex().flags() & Qt::ItemIsSelectable)) {
            combo->hidePopup();
            emit itemSelected(view->currentIndex());
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QFrame::eventFilter(o, e);
}

void QComboBoxPrivateContainer::showEvent(QShowEvent *)
{
    combo->update();
}

void QComboBoxPrivateContainer::hideEvent(QHideEvent *)
{
    emit resetButton();
    combo->update();
#if QT_CONFIG(graphicsview)
    // QGraphicsScenePrivate::removePopup closes the combo box popup, it hides it non-explicitly.
    // Hiding/showing the QComboBox after this will unexpectedly show the popup as well.
    // Re-hiding the popup container makes sure it is explicitly hidden.
    if (QGraphicsProxyWidget *proxy = graphicsProxyWidget())
        proxy->hide();
#endif
}

void QComboBoxPrivateContainer::mousePressEvent(QMouseEvent *e)
{

    QStyleOptionComboBox opt = comboStyleOption();
    opt.subControls = QStyle::SC_All;
    opt.activeSubControls = QStyle::SC_ComboBoxArrow;
    QStyle::SubControl sc = combo->style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt,
                                                           combo->mapFromGlobal(e->globalPos()),
                                                           combo);
    if ((combo->isEditable() && sc == QStyle::SC_ComboBoxArrow)
        || (!combo->isEditable() && sc != QStyle::SC_None))
        setAttribute(Qt::WA_NoMouseReplay);
    combo->hidePopup();
}

void QComboBoxPrivateContainer::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    if (!blockMouseReleaseTimer.isActive()){
        combo->hidePopup();
        emit resetButton();
    }
}

QStyleOptionComboBox QComboBoxPrivateContainer::comboStyleOption() const
{
    // ### This should use QComboBox's initStyleOption(), but it's protected
    // perhaps, we could cheat by having the QCombo private instead?
    QStyleOptionComboBox opt;
    opt.initFrom(combo);
    opt.subControls = QStyle::SC_All;
    opt.activeSubControls = QStyle::SC_None;
    opt.editable = combo->isEditable();
    return opt;
}

/*!
    \enum QComboBox::InsertPolicy

    This enum specifies what the QComboBox should do when a new string is
    entered by the user.

    \value NoInsert             The string will not be inserted into the combobox.
    \value InsertAtTop          The string will be inserted as the first item in the combobox.
    \value InsertAtCurrent      The current item will be \e replaced by the string.
    \value InsertAtBottom       The string will be inserted after the last item in the combobox.
    \value InsertAfterCurrent   The string is inserted after the current item in the combobox.
    \value InsertBeforeCurrent  The string is inserted before the current item in the combobox.
    \value InsertAlphabetically The string is inserted in the alphabetic order in the combobox.
*/

/*!
    \enum QComboBox::SizeAdjustPolicy

    This enum specifies how the size hint of the QComboBox should
    adjust when new content is added or content changes.

    \value AdjustToContents              The combobox will always adjust to the contents
    \value AdjustToContentsOnFirstShow   The combobox will adjust to its contents the first time it is shown.
    \value AdjustToMinimumContentsLength Use AdjustToContents or AdjustToContentsOnFirstShow instead.
    \value AdjustToMinimumContentsLengthWithIcon The combobox will adjust to \l minimumContentsLength plus space for an icon. For performance reasons use this policy on large models.
*/

/*!
    \fn void QComboBox::activated(int index)

    This signal is sent when the user chooses an item in the combobox.
    The item's \a index is passed. Note that this signal is sent even
    when the choice is not changed. If you need to know when the
    choice actually changes, use signal currentIndexChanged().

*/

/*!
    \fn void QComboBox::activated(const QString &text)

    This signal is sent when the user chooses an item in the combobox.
    The item's \a text is passed. Note that this signal is sent even
    when the choice is not changed. If you need to know when the
    choice actually changes, use signal currentIndexChanged().

*/

/*!
    \fn void QComboBox::highlighted(int index)

    This signal is sent when an item in the combobox popup list is
    highlighted by the user. The item's \a index is passed.
*/

/*!
    \fn void QComboBox::highlighted(const QString &text)

    This signal is sent when an item in the combobox popup list is
    highlighted by the user. The item's \a text is passed.
*/

/*!
    \fn void QComboBox::currentIndexChanged(int index)
    \since 4.1

    This signal is sent whenever the currentIndex in the combobox
    changes either through user interaction or programmatically. The
    item's \a index is passed or -1 if the combobox becomes empty or the
    currentIndex was reset.
*/

/*!
    \fn void QComboBox::currentIndexChanged(const QString &text)
    \since 4.1

    This signal is sent whenever the currentIndex in the combobox
    changes either through user interaction or programmatically.  The
    item's \a text is passed.
*/

/*!
    \fn void QComboBox::currentTextChanged(const QString &text)
    \since 5.0

    This signal is sent whenever currentText changes. The new value
    is passed as \a text.
*/

/*!
    Constructs a combobox with the given \a parent, using the default
    model QStandardItemModel.
*/
QComboBox::QComboBox(QWidget *parent)
    : QWidget(*new QComboBoxPrivate(), parent, 0)
{
    Q_D(QComboBox);
    d->init();
}

/*!
  \internal
*/
QComboBox::QComboBox(QComboBoxPrivate &dd, QWidget *parent)
    : QWidget(dd, parent, 0)
{
    Q_D(QComboBox);
    d->init();
}


/*!
    \class QComboBox
    \brief The QComboBox widget is a combined button and popup list.

    \ingroup basicwidgets
    \inmodule QtWidgets

    \image windows-combobox.png

    A QComboBox provides a means of presenting a list of options to the user
    in a way that takes up the minimum amount of screen space.

    A combobox is a selection widget that displays the current item,
    and can pop up a list of selectable items. A combobox may be editable,
    allowing the user to modify each item in the list.

    Comboboxes can contain pixmaps as well as strings; the
    insertItem() and setItemText() functions are suitably overloaded.
    For editable comboboxes, the function clearEditText() is provided,
    to clear the displayed string without changing the combobox's
    contents.

    There are two signals emitted if the current item of a combobox
    changes, currentIndexChanged() and activated().
    currentIndexChanged() is always emitted regardless if the change
    was done programmatically or by user interaction, while
    activated() is only emitted when the change is caused by user
    interaction. The highlighted() signal is emitted when the user
    highlights an item in the combobox popup list. All three signals
    exist in two versions, one with a QString argument and one with an
    \c int argument. If the user selects or highlights a pixmap, only
    the \c int signals are emitted. Whenever the text of an editable
    combobox is changed the editTextChanged() signal is emitted.

    When the user enters a new string in an editable combobox, the
    widget may or may not insert it, and it can insert it in several
    locations. The default policy is \l InsertAtBottom but you can change
    this using setInsertPolicy().

    It is possible to constrain the input to an editable combobox
    using QValidator; see setValidator(). By default, any input is
    accepted.

    A combobox can be populated using the insert functions,
    insertItem() and insertItems() for example. Items can be
    changed with setItemText(). An item can be removed with
    removeItem() and all items can be removed with clear(). The text
    of the current item is returned by currentText(), and the text of
    a numbered item is returned with text(). The current item can be
    set with setCurrentIndex(). The number of items in the combobox is
    returned by count(); the maximum number of items can be set with
    setMaxCount(). You can allow editing using setEditable(). For
    editable comboboxes you can set auto-completion using
    setCompleter() and whether or not the user can add duplicates
    is set with setDuplicatesEnabled().

    QComboBox uses the \l{Model/View Programming}{model/view
    framework} for its popup list and to store its items.  By default
    a QStandardItemModel stores the items and a QListView subclass
    displays the popuplist. You can access the model and view directly
    (with model() and view()), but QComboBox also provides functions
    to set and get item data (e.g., setItemData() and itemText()). You
    can also set a new model and view (with setModel() and setView()).
    For the text and icon in the combobox label, the data in the model
    that has the Qt::DisplayRole and Qt::DecorationRole is used.  Note
    that you cannot alter the \l{QAbstractItemView::}{SelectionMode}
    of the view(), e.g., by using
    \l{QAbstractItemView::}{setSelectionMode()}.

    \sa QLineEdit, QSpinBox, QRadioButton, QButtonGroup,
        {fowler}{GUI Design Handbook: Combo Box, Drop-Down List Box}
*/

void QComboBoxPrivate::init()
{
    Q_Q(QComboBox);
#ifdef Q_OS_OSX
    // On OS X, only line edits and list views always get tab focus. It's only
    // when we enable full keyboard access that other controls can get tab focus.
    // When it's not editable, a combobox looks like a button, and it behaves as
    // such in this respect.
    if (!q->isEditable())
        q->setFocusPolicy(Qt::TabFocus);
    else
#endif
        q->setFocusPolicy(Qt::WheelFocus);

    q->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed,
                                 QSizePolicy::ComboBox));
    setLayoutItemMargins(QStyle::SE_ComboBoxLayoutItem);
    q->setModel(new QStandardItemModel(0, 1, q));
    if (!q->isEditable())
        q->setAttribute(Qt::WA_InputMethodEnabled, false);
    else
        q->setAttribute(Qt::WA_InputMethodEnabled);
}

QComboBoxPrivateContainer* QComboBoxPrivate::viewContainer()
{
    if (container)
        return container;

    Q_Q(QComboBox);
    container = new QComboBoxPrivateContainer(new QComboBoxListView(q), q);
    container->itemView()->setModel(model);
    container->itemView()->setTextElideMode(Qt::ElideMiddle);
    updateDelegate(true);
    updateLayoutDirection();
    updateViewContainerPaletteAndOpacity();
    QObject::connect(container, SIGNAL(itemSelected(QModelIndex)),
                     q, SLOT(_q_itemSelected(QModelIndex)));
    QObject::connect(container->itemView()->selectionModel(),
                     SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                     q, SLOT(_q_emitHighlighted(QModelIndex)));
    QObject::connect(container, SIGNAL(resetButton()), q, SLOT(_q_resetButton()));
    return container;
}


void QComboBoxPrivate::_q_resetButton()
{
    updateArrow(QStyle::State_None);
}

void QComboBoxPrivate::_q_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    Q_Q(QComboBox);
    if (inserting || topLeft.parent() != root)
        return;

    if (sizeAdjustPolicy == QComboBox::AdjustToContents) {
        sizeHint = QSize();
        adjustComboBoxSize();
        q->updateGeometry();
    }

    if (currentIndex.row() >= topLeft.row() && currentIndex.row() <= bottomRight.row()) {
        const QString text = q->itemText(currentIndex.row());
        if (lineEdit) {
            lineEdit->setText(text);
            updateLineEditGeometry();
        } else {
            emit q->currentTextChanged(text);
        }
        q->update();
#ifndef QT_NO_ACCESSIBILITY
        QAccessibleValueChangeEvent event(q, text);
        QAccessible::updateAccessibility(&event);
#endif
    }
}

void QComboBoxPrivate::_q_rowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_Q(QComboBox);
    if (inserting || parent != root)
        return;

    if (sizeAdjustPolicy == QComboBox::AdjustToContents) {
        sizeHint = QSize();
        adjustComboBoxSize();
        q->updateGeometry();
    }

    // set current index if combo was previously empty
    if (start == 0 && (end - start + 1) == q->count() && !currentIndex.isValid()) {
        q->setCurrentIndex(0);
        // need to emit changed if model updated index "silently"
    } else if (currentIndex.row() != indexBeforeChange) {
        q->update();
        _q_emitCurrentIndexChanged(currentIndex);
    }
}

void QComboBoxPrivate::_q_updateIndexBeforeChange()
{
    indexBeforeChange = currentIndex.row();
}

void QComboBoxPrivate::_q_rowsRemoved(const QModelIndex &parent, int /*start*/, int /*end*/)
{
    Q_Q(QComboBox);
    if (parent != root)
        return;

    if (sizeAdjustPolicy == QComboBox::AdjustToContents) {
        sizeHint = QSize();
        adjustComboBoxSize();
        q->updateGeometry();
    }

    // model has changed the currentIndex
    if (currentIndex.row() != indexBeforeChange) {
        if (!currentIndex.isValid() && q->count()) {
            q->setCurrentIndex(qMin(q->count() - 1, qMax(indexBeforeChange, 0)));
            return;
        }
        if (lineEdit) {
            lineEdit->setText(q->itemText(currentIndex.row()));
            updateLineEditGeometry();
        }
        q->update();
        _q_emitCurrentIndexChanged(currentIndex);
    }
}


void QComboBoxPrivate::updateViewContainerPaletteAndOpacity()
{
    if (!container)
        return;
    Q_Q(QComboBox);
    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
#if QT_CONFIG(menu)
    if (q->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, q)) {
        QMenu menu;
        menu.ensurePolished();
        container->setPalette(menu.palette());
        container->setWindowOpacity(menu.windowOpacity());
    } else
#endif
    {
        container->setPalette(q->palette());
        container->setWindowOpacity(1.0);
    }
    if (lineEdit)
        lineEdit->setPalette(q->palette());
}

void QComboBoxPrivate::updateFocusPolicy()
{
#ifdef Q_OS_OSX
    Q_Q(QComboBox);

    // See comment in QComboBoxPrivate::init()
    if (q->isEditable())
        q->setFocusPolicy(Qt::WheelFocus);
    else
        q->setFocusPolicy(Qt::TabFocus);
#endif
}

/*!
    Initialize \a option with the values from this QComboBox. This method
    is useful for subclasses when they need a QStyleOptionComboBox, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QComboBox::initStyleOption(QStyleOptionComboBox *option) const
{
    if (!option)
        return;

    Q_D(const QComboBox);
    option->initFrom(this);
    option->editable = isEditable();
    option->frame = d->frame;
    if (hasFocus() && !option->editable)
        option->state |= QStyle::State_Selected;
    option->subControls = QStyle::SC_All;
    if (d->arrowState == QStyle::State_Sunken) {
        option->activeSubControls = QStyle::SC_ComboBoxArrow;
        option->state |= d->arrowState;
    } else {
        option->activeSubControls = d->hoverControl;
    }
    if (d->currentIndex.isValid()) {
        option->currentText = currentText();
        option->currentIcon = d->itemIcon(d->currentIndex);
    }
    option->iconSize = iconSize();
    if (d->container && d->container->isVisible())
        option->state |= QStyle::State_On;
}

void QComboBoxPrivate::updateLineEditGeometry()
{
    if (!lineEdit)
        return;

    Q_Q(QComboBox);
    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    QRect editRect = q->style()->subControlRect(QStyle::CC_ComboBox, &opt,
                                                QStyle::SC_ComboBoxEditField, q);
    if (!q->itemIcon(q->currentIndex()).isNull()) {
        QRect comboRect(editRect);
        editRect.setWidth(editRect.width() - q->iconSize().width() - 4);
        editRect = QStyle::alignedRect(q->layoutDirection(), Qt::AlignRight,
                                       editRect.size(), comboRect);
    }
    lineEdit->setGeometry(editRect);
}

Qt::MatchFlags QComboBoxPrivate::matchFlags() const
{
    // Base how duplicates are determined on the autocompletion case sensitivity
    Qt::MatchFlags flags = Qt::MatchFixedString;
#if QT_CONFIG(completer)
    if (!lineEdit->completer() || lineEdit->completer()->caseSensitivity() == Qt::CaseSensitive)
#endif
        flags |= Qt::MatchCaseSensitive;
    return flags;
}


void QComboBoxPrivate::_q_editingFinished()
{
    Q_Q(QComboBox);
    if (!lineEdit)
        return;
    const auto leText = lineEdit->text();
    if (!leText.isEmpty() && itemText(currentIndex) != leText) {
#if QT_CONFIG(completer)
        const auto *leCompleter = lineEdit->completer();
        const auto *popup = leCompleter ? QCompleterPrivate::get(leCompleter)->popup : nullptr;
        if (popup && popup->isVisible()) {
            // QLineEdit::editingFinished() will be emitted before the code flow returns
            // to QCompleter::eventFilter(), where QCompleter::activated() may be emitted.
            // We know that the completer popup will still be visible at this point, and
            // that any selection should be valid.
            const QItemSelectionModel *selModel = popup->selectionModel();
            const QModelIndex curIndex = popup->currentIndex();
            const bool completerIsActive = selModel && selModel->selectedIndexes().contains(curIndex);

            if (completerIsActive)
                return;
        }
#endif
        const int index = q_func()->findText(leText, matchFlags());
        if (index != -1) {
            q->setCurrentIndex(index);
            emitActivated(currentIndex);
        }
    }

}

void QComboBoxPrivate::_q_returnPressed()
{
    Q_Q(QComboBox);

    // The insertion code below does not apply when the policy is QComboBox::NoInsert.
    // In case a completer is installed, item activation via the completer is handled
    // in _q_completerActivated(). Otherwise _q_editingFinished() updates the current
    // index as appropriate.
    if (insertPolicy == QComboBox::NoInsert)
        return;

    if (lineEdit && !lineEdit->text().isEmpty()) {
        if (q->count() >= maxCount && !(this->insertPolicy == QComboBox::InsertAtCurrent))
            return;
        lineEdit->deselect();
        lineEdit->end(false);
        QString text = lineEdit->text();
        // check for duplicates (if not enabled) and quit
        int index = -1;
        if (!duplicatesEnabled) {
            index = q->findText(text, matchFlags());
            if (index != -1) {
                q->setCurrentIndex(index);
                emitActivated(currentIndex);
                return;
            }
        }
        switch (insertPolicy) {
        case QComboBox::InsertAtTop:
            index = 0;
            break;
        case QComboBox::InsertAtBottom:
            index = q->count();
            break;
        case QComboBox::InsertAtCurrent:
        case QComboBox::InsertAfterCurrent:
        case QComboBox::InsertBeforeCurrent:
            if (!q->count() || !currentIndex.isValid())
                index = 0;
            else if (insertPolicy == QComboBox::InsertAtCurrent)
                q->setItemText(q->currentIndex(), text);
            else if (insertPolicy == QComboBox::InsertAfterCurrent)
                index = q->currentIndex() + 1;
            else if (insertPolicy == QComboBox::InsertBeforeCurrent)
                index = q->currentIndex();
            break;
        case QComboBox::InsertAlphabetically:
            index = 0;
            for (int i=0; i< q->count(); i++, index++ ) {
                if (text.toLower() < q->itemText(i).toLower())
                    break;
            }
            break;
        default:
            break;
        }
        if (index >= 0) {
            q->insertItem(index, text);
            q->setCurrentIndex(index);
            emitActivated(currentIndex);
        }
    }
}

void QComboBoxPrivate::_q_itemSelected(const QModelIndex &item)
{
    Q_Q(QComboBox);
    if (item != currentIndex) {
        setCurrentIndex(item);
    } else if (lineEdit) {
        lineEdit->selectAll();
        lineEdit->setText(q->itemText(currentIndex.row()));
    }
    emitActivated(currentIndex);
}

void QComboBoxPrivate::emitActivated(const QModelIndex &index)
{
    Q_Q(QComboBox);
    if (!index.isValid())
        return;
    QString text(itemText(index));
    emit q->activated(index.row());
    emit q->activated(text);
}

void QComboBoxPrivate::_q_emitHighlighted(const QModelIndex &index)
{
    Q_Q(QComboBox);
    if (!index.isValid())
        return;
    QString text(itemText(index));
    emit q->highlighted(index.row());
    emit q->highlighted(text);
}

void QComboBoxPrivate::_q_emitCurrentIndexChanged(const QModelIndex &index)
{
    Q_Q(QComboBox);
    const QString text = itemText(index);
    emit q->currentIndexChanged(index.row());
    emit q->currentIndexChanged(text);
    // signal lineEdit.textChanged already connected to signal currentTextChanged, so don't emit double here
    if (!lineEdit)
        emit q->currentTextChanged(text);
#ifndef QT_NO_ACCESSIBILITY
    QAccessibleValueChangeEvent event(q, text);
    QAccessible::updateAccessibility(&event);
#endif
}

QString QComboBoxPrivate::itemText(const QModelIndex &index) const
{
    return index.isValid() ? model->data(index, itemRole()).toString() : QString();
}

int QComboBoxPrivate::itemRole() const
{
    return q_func()->isEditable() ? Qt::EditRole : Qt::DisplayRole;
}

/*!
    Destroys the combobox.
*/
QComboBox::~QComboBox()
{
    // ### check delegateparent and delete delegate if us?
    Q_D(QComboBox);

    QT_TRY {
        disconnect(d->model, SIGNAL(destroyed()),
                this, SLOT(_q_modelDestroyed()));
    } QT_CATCH(...) {
        ; // objects can't throw in destructor
    }
}

/*!
    \property QComboBox::maxVisibleItems
    \brief the maximum allowed size on screen of the combo box, measured in items

    By default, this property has a value of 10.

    \note This property is ignored for non-editable comboboxes in styles that returns
    true for QStyle::SH_ComboBox_Popup such as the Mac style or the Gtk+ Style.
*/
int QComboBox::maxVisibleItems() const
{
    Q_D(const QComboBox);
    return d->maxVisibleItems;
}

void QComboBox::setMaxVisibleItems(int maxItems)
{
    Q_D(QComboBox);
    if (Q_UNLIKELY(maxItems < 0)) {
        qWarning("QComboBox::setMaxVisibleItems: "
                 "Invalid max visible items (%d) must be >= 0", maxItems);
        return;
    }
    d->maxVisibleItems = maxItems;
}

/*!
    \property QComboBox::count
    \brief the number of items in the combobox

    By default, for an empty combo box, this property has a value of 0.
*/
int QComboBox::count() const
{
    Q_D(const QComboBox);
    return d->model->rowCount(d->root);
}

/*!
    \property QComboBox::maxCount
    \brief the maximum number of items allowed in the combobox

    \note If you set the maximum number to be less then the current
    amount of items in the combobox, the extra items will be
    truncated. This also applies if you have set an external model on
    the combobox.

    By default, this property's value is derived from the highest
    signed integer available (typically 2147483647).
*/
void QComboBox::setMaxCount(int max)
{
    Q_D(QComboBox);
    if (Q_UNLIKELY(max < 0)) {
        qWarning("QComboBox::setMaxCount: Invalid count (%d) must be >= 0", max);
        return;
    }

    const int rowCount = count();
    if (rowCount > max)
        d->model->removeRows(max, rowCount - max, d->root);

    d->maxCount = max;
}

int QComboBox::maxCount() const
{
    Q_D(const QComboBox);
    return d->maxCount;
}

#if QT_CONFIG(completer)

/*!
    \property QComboBox::autoCompletion
    \brief whether the combobox provides auto-completion for editable items
    \since 4.1
    \obsolete

    Use setCompleter() instead.

    By default, this property is \c true.

    \sa editable
*/

/*!
    \obsolete

    Use setCompleter() instead.
*/
bool QComboBox::autoCompletion() const
{
    Q_D(const QComboBox);
    return d->autoCompletion;
}

/*!
    \obsolete

    Use setCompleter() instead.
*/
void QComboBox::setAutoCompletion(bool enable)
{
    Q_D(QComboBox);

#ifdef QT_KEYPAD_NAVIGATION
    if (Q_UNLIKELY(QApplication::keypadNavigationEnabled() && !enable && isEditable()))
        qWarning("QComboBox::setAutoCompletion: auto completion is mandatory when combo box editable");
#endif

    d->autoCompletion = enable;
    if (!d->lineEdit)
        return;
    if (enable) {
        if (d->lineEdit->completer())
            return;
        d->completer = new QCompleter(d->model, d->lineEdit);
        connect(d->completer, SIGNAL(activated(QModelIndex)), this, SLOT(_q_completerActivated(QModelIndex)));
        d->completer->setCaseSensitivity(d->autoCompletionCaseSensitivity);
        d->completer->setCompletionMode(QCompleter::InlineCompletion);
        d->completer->setCompletionColumn(d->modelColumn);
        d->lineEdit->setCompleter(d->completer);
        d->completer->setWidget(this);
    } else {
        d->lineEdit->setCompleter(0);
    }
}

/*!
    \property QComboBox::autoCompletionCaseSensitivity
    \brief whether string comparisons are case-sensitive or case-insensitive for auto-completion
    \obsolete

    By default, this property is Qt::CaseInsensitive.

    Use setCompleter() instead. Case sensitivity of the auto completion can be
    changed using QCompleter::setCaseSensitivity().

    \sa autoCompletion
*/

/*!
    \obsolete

    Use setCompleter() and QCompleter::setCaseSensitivity() instead.
*/
Qt::CaseSensitivity QComboBox::autoCompletionCaseSensitivity() const
{
    Q_D(const QComboBox);
    return d->autoCompletionCaseSensitivity;
}

/*!
    \obsolete

    Use setCompleter() and QCompleter::setCaseSensitivity() instead.
*/
void QComboBox::setAutoCompletionCaseSensitivity(Qt::CaseSensitivity sensitivity)
{
    Q_D(QComboBox);
    d->autoCompletionCaseSensitivity = sensitivity;
    if (d->lineEdit && d->lineEdit->completer())
        d->lineEdit->completer()->setCaseSensitivity(sensitivity);
}

#endif // QT_CONFIG(completer)

/*!
    \property QComboBox::duplicatesEnabled
    \brief whether the user can enter duplicate items into the combobox

    Note that it is always possible to programmatically insert duplicate items into the
    combobox.

    By default, this property is \c false (duplicates are not allowed).
*/
bool QComboBox::duplicatesEnabled() const
{
    Q_D(const QComboBox);
    return d->duplicatesEnabled;
}

void QComboBox::setDuplicatesEnabled(bool enable)
{
    Q_D(QComboBox);
    d->duplicatesEnabled = enable;
}

/*!  \fn int QComboBox::findText(const QString &text, Qt::MatchFlags flags = Qt::MatchExactly|Qt::MatchCaseSensitive) const

  Returns the index of the item containing the given \a text; otherwise
  returns -1.

  The \a flags specify how the items in the combobox are searched.
*/

/*!
  Returns the index of the item containing the given \a data for the
  given \a role; otherwise returns -1.

  The \a flags specify how the items in the combobox are searched.
*/
int QComboBox::findData(const QVariant &data, int role, Qt::MatchFlags flags) const
{
    Q_D(const QComboBox);
    QModelIndex start = d->model->index(0, d->modelColumn, d->root);
    const QModelIndexList result = d->model->match(start, role, data, 1, flags);
    if (result.isEmpty())
        return -1;
    return result.first().row();
}

/*!
    \property QComboBox::insertPolicy
    \brief the policy used to determine where user-inserted items should
    appear in the combobox

    The default value is \l InsertAtBottom, indicating that new items will appear
    at the bottom of the list of items.

    \sa InsertPolicy
*/

QComboBox::InsertPolicy QComboBox::insertPolicy() const
{
    Q_D(const QComboBox);
    return d->insertPolicy;
}

void QComboBox::setInsertPolicy(InsertPolicy policy)
{
    Q_D(QComboBox);
    d->insertPolicy = policy;
}

/*!
    \property QComboBox::sizeAdjustPolicy
    \brief the policy describing how the size of the combobox changes
    when the content changes

    The default value is \l AdjustToContentsOnFirstShow.

    \sa SizeAdjustPolicy
*/

QComboBox::SizeAdjustPolicy QComboBox::sizeAdjustPolicy() const
{
    Q_D(const QComboBox);
    return d->sizeAdjustPolicy;
}

void QComboBox::setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy policy)
{
    Q_D(QComboBox);
    if (policy == d->sizeAdjustPolicy)
        return;

    d->sizeAdjustPolicy = policy;
    d->sizeHint = QSize();
    d->adjustComboBoxSize();
    updateGeometry();
}

/*!
    \property QComboBox::minimumContentsLength
    \brief the minimum number of characters that should fit into the combobox.

    The default value is 0.

    If this property is set to a positive value, the
    minimumSizeHint() and sizeHint() take it into account.

    \sa sizeAdjustPolicy
*/
int QComboBox::minimumContentsLength() const
{
    Q_D(const QComboBox);
    return d->minimumContentsLength;
}

void QComboBox::setMinimumContentsLength(int characters)
{
    Q_D(QComboBox);
    if (characters == d->minimumContentsLength || characters < 0)
        return;

    d->minimumContentsLength = characters;

    if (d->sizeAdjustPolicy == AdjustToContents
            || d->sizeAdjustPolicy == AdjustToMinimumContentsLength
            || d->sizeAdjustPolicy == AdjustToMinimumContentsLengthWithIcon) {
        d->sizeHint = QSize();
        d->adjustComboBoxSize();
        updateGeometry();
    }
}

/*!
    \property QComboBox::iconSize
    \brief the size of the icons shown in the combobox.

    Unless explicitly set this returns the default value of the
    current style.  This size is the maximum size that icons can have;
    icons of smaller size are not scaled up.
*/

QSize QComboBox::iconSize() const
{
    Q_D(const QComboBox);
    if (d->iconSize.isValid())
        return d->iconSize;

    int iconWidth = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
    return QSize(iconWidth, iconWidth);
}

void QComboBox::setIconSize(const QSize &size)
{
    Q_D(QComboBox);
    if (size == d->iconSize)
        return;

    view()->setIconSize(size);
    d->iconSize = size;
    d->sizeHint = QSize();
    updateGeometry();
}

/*!
    \property QComboBox::editable
    \brief whether the combo box can be edited by the user

    By default, this property is \c false. The effect of editing depends
    on the insert policy.

    \note When disabling the \a editable state, the validator and
    completer are removed.

    \sa InsertPolicy
*/
bool QComboBox::isEditable() const
{
    Q_D(const QComboBox);
    return d->lineEdit != 0;
}

/*! \internal
    update the default delegate
    depending on the style's SH_ComboBox_Popup hint, we use a different default delegate.

    but we do not change the delegate is the combobox use a custom delegate,
    unless \a force is set to true.
 */
void QComboBoxPrivate::updateDelegate(bool force)
{
    Q_Q(QComboBox);
    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    if (q->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, q)) {
        if (force || qobject_cast<QComboBoxDelegate *>(q->itemDelegate()))
            q->setItemDelegate(new QComboMenuDelegate(q->view(), q));
    } else {
        if (force || qobject_cast<QComboMenuDelegate *>(q->itemDelegate()))
            q->setItemDelegate(new QComboBoxDelegate(q->view(), q));
    }
}

QIcon QComboBoxPrivate::itemIcon(const QModelIndex &index) const
{
    QVariant decoration = model->data(index, Qt::DecorationRole);
    if (decoration.type() == QVariant::Pixmap)
        return QIcon(qvariant_cast<QPixmap>(decoration));
    else
        return qvariant_cast<QIcon>(decoration);
}

void QComboBox::setEditable(bool editable)
{
    Q_D(QComboBox);
    if (isEditable() == editable)
        return;

    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    if (editable) {
        if (style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, this)) {
            d->viewContainer()->updateScrollers();
            view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        QLineEdit *le = new QLineEdit(this);
        setLineEdit(le);
    } else {
        if (style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, this)) {
            d->viewContainer()->updateScrollers();
            view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        setAttribute(Qt::WA_InputMethodEnabled, false);
        d->lineEdit->hide();
        d->lineEdit->deleteLater();
        d->lineEdit = 0;
    }

    d->updateDelegate();
    d->updateFocusPolicy();

    d->viewContainer()->updateTopBottomMargin();
    if (!testAttribute(Qt::WA_Resized))
        adjustSize();
}

/*!
    Sets the line \a edit to use instead of the current line edit widget.

    The combo box takes ownership of the line edit.
*/
void QComboBox::setLineEdit(QLineEdit *edit)
{
    Q_D(QComboBox);
    if (Q_UNLIKELY(!edit)) {
        qWarning("QComboBox::setLineEdit: cannot set a 0 line edit");
        return;
    }

    if (edit == d->lineEdit)
        return;

    edit->setText(currentText());
    delete d->lineEdit;

    d->lineEdit = edit;
#ifndef QT_NO_IM
    qt_widget_private(d->lineEdit)->inheritsInputMethodHints = 1;
#endif
    if (d->lineEdit->parent() != this)
        d->lineEdit->setParent(this);
    connect(d->lineEdit, SIGNAL(returnPressed()), this, SLOT(_q_returnPressed()));
    connect(d->lineEdit, SIGNAL(editingFinished()), this, SLOT(_q_editingFinished()));
    connect(d->lineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(editTextChanged(QString)));
    connect(d->lineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(currentTextChanged(QString)));
    connect(d->lineEdit, SIGNAL(cursorPositionChanged(int,int)), this, SLOT(updateMicroFocus()));
    connect(d->lineEdit, SIGNAL(selectionChanged()), this, SLOT(updateMicroFocus()));
    connect(d->lineEdit->d_func()->control, SIGNAL(updateMicroFocus()), this, SLOT(updateMicroFocus()));
    d->lineEdit->setFrame(false);
    d->lineEdit->setContextMenuPolicy(Qt::NoContextMenu);
    d->updateFocusPolicy();
    d->lineEdit->setFocusProxy(this);
    d->lineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
#if QT_CONFIG(completer)
    setAutoCompletion(d->autoCompletion);
#endif

#ifdef QT_KEYPAD_NAVIGATION
#if QT_CONFIG(completer)
    if (QApplication::keypadNavigationEnabled()) {
        // Editable combo boxes will have a completer that is set to UnfilteredPopupCompletion.
        // This means that when the user enters edit mode they are immediately presented with a
        // list of possible completions.
        setAutoCompletion(true);
        if (d->completer) {
            d->completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
            connect(d->completer, SIGNAL(activated(QModelIndex)), this, SLOT(_q_completerActivated()));
        }
    }
#endif
#endif

    setAttribute(Qt::WA_InputMethodEnabled);
    d->updateLayoutDirection();
    d->updateLineEditGeometry();
    if (isVisible())
        d->lineEdit->show();

    update();
}

/*!
    Returns the line edit used to edit items in the combobox, or 0 if there
    is no line edit.

    Only editable combo boxes have a line edit.
*/
QLineEdit *QComboBox::lineEdit() const
{
    Q_D(const QComboBox);
    return d->lineEdit;
}

#ifndef QT_NO_VALIDATOR
/*!
    \fn void QComboBox::setValidator(const QValidator *validator)

    Sets the \a validator to use instead of the current validator.

    \note The validator is removed when the \l editable property becomes \c false.
*/

void QComboBox::setValidator(const QValidator *v)
{
    Q_D(QComboBox);
    if (d->lineEdit)
        d->lineEdit->setValidator(v);
}

/*!
    Returns the validator that is used to constrain text input for the
    combobox.

    \sa editable
*/
const QValidator *QComboBox::validator() const
{
    Q_D(const QComboBox);
    return d->lineEdit ? d->lineEdit->validator() : 0;
}
#endif // QT_NO_VALIDATOR

#if QT_CONFIG(completer)

/*!
    \fn void QComboBox::setCompleter(QCompleter *completer)
    \since 4.2

    Sets the \a completer to use instead of the current completer.
    If \a completer is 0, auto completion is disabled.

    By default, for an editable combo box, a QCompleter that
    performs case insensitive inline completion is automatically created.

    \note The completer is removed when the \l editable property becomes \c false.
*/
void QComboBox::setCompleter(QCompleter *c)
{
    Q_D(QComboBox);
    if (!d->lineEdit)
        return;
    d->lineEdit->setCompleter(c);
    if (c) {
        connect(c, SIGNAL(activated(QModelIndex)), this, SLOT(_q_completerActivated(QModelIndex)));
        c->setWidget(this);
    }
}

/*!
    \since 4.2

    Returns the completer that is used to auto complete text input for the
    combobox.

    \sa editable
*/
QCompleter *QComboBox::completer() const
{
    Q_D(const QComboBox);
    return d->lineEdit ? d->lineEdit->completer() : 0;
}

#endif // QT_CONFIG(completer)

/*!
    Returns the item delegate used by the popup list view.

    \sa setItemDelegate()
*/
QAbstractItemDelegate *QComboBox::itemDelegate() const
{
    return view()->itemDelegate();
}

/*!
    Sets the item \a delegate for the popup list view.
    The combobox takes ownership of the delegate.

    \warning You should not share the same instance of a delegate between comboboxes,
    widget mappers or views. Doing so can cause incorrect or unintuitive editing behavior
    since each view connected to a given delegate may receive the
    \l{QAbstractItemDelegate::}{closeEditor()} signal, and attempt to access, modify or
    close an editor that has already been closed.

    \sa itemDelegate()
*/
void QComboBox::setItemDelegate(QAbstractItemDelegate *delegate)
{
    if (Q_UNLIKELY(!delegate)) {
        qWarning("QComboBox::setItemDelegate: cannot set a 0 delegate");
        return;
    }
    delete view()->itemDelegate();
    view()->setItemDelegate(delegate);
}

/*!
    Returns the model used by the combobox.
*/

QAbstractItemModel *QComboBox::model() const
{
    Q_D(const QComboBox);
    if (d->model == QAbstractItemModelPrivate::staticEmptyModel()) {
        QComboBox *that = const_cast<QComboBox*>(this);
        that->setModel(new QStandardItemModel(0, 1, that));
    }
    return d->model;
}

/*!
    Sets the model to be \a model. \a model must not be 0.
    If you want to clear the contents of a model, call clear().

    \sa clear()
*/
void QComboBox::setModel(QAbstractItemModel *model)
{
    Q_D(QComboBox);

    if (Q_UNLIKELY(!model)) {
        qWarning("QComboBox::setModel: cannot set a 0 model");
        return;
    }

    if (model == d->model)
        return;

#if QT_CONFIG(completer)
    if (d->lineEdit && d->lineEdit->completer()
        && d->lineEdit->completer() == d->completer)
        d->lineEdit->completer()->setModel(model);
#endif
    if (d->model) {
        disconnect(d->model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                   this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
        disconnect(d->model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                   this, SLOT(_q_updateIndexBeforeChange()));
        disconnect(d->model, SIGNAL(rowsInserted(QModelIndex,int,int)),
                   this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
        disconnect(d->model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                   this, SLOT(_q_updateIndexBeforeChange()));
        disconnect(d->model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SLOT(_q_rowsRemoved(QModelIndex,int,int)));
        disconnect(d->model, SIGNAL(destroyed()),
                   this, SLOT(_q_modelDestroyed()));
        disconnect(d->model, SIGNAL(modelAboutToBeReset()),
                   this, SLOT(_q_updateIndexBeforeChange()));
        disconnect(d->model, SIGNAL(modelReset()),
                   this, SLOT(_q_modelReset()));
        if (d->model->QObject::parent() == this)
            delete d->model;
    }

    d->model = model;

    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(_q_dataChanged(QModelIndex,QModelIndex)));
    connect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
            this, SLOT(_q_updateIndexBeforeChange()));
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(_q_rowsInserted(QModelIndex,int,int)));
    connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(_q_updateIndexBeforeChange()));
    connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(_q_rowsRemoved(QModelIndex,int,int)));
    connect(model, SIGNAL(destroyed()),
            this, SLOT(_q_modelDestroyed()));
    connect(model, SIGNAL(modelAboutToBeReset()),
            this, SLOT(_q_updateIndexBeforeChange()));
    connect(model, SIGNAL(modelReset()),
            this, SLOT(_q_modelReset()));

    if (d->container) {
        d->container->itemView()->setModel(model);
        connect(d->container->itemView()->selectionModel(),
                SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                this, SLOT(_q_emitHighlighted(QModelIndex)), Qt::UniqueConnection);
    }

    setRootModelIndex(QModelIndex());

    bool currentReset = false;

    const int rowCount = count();
    for (int pos=0; pos < rowCount; pos++) {
        if (d->model->index(pos, d->modelColumn, d->root).flags() & Qt::ItemIsEnabled) {
            setCurrentIndex(pos);
            currentReset = true;
            break;
        }
    }

    if (!currentReset)
        setCurrentIndex(-1);

    d->modelChanged();
}

/*!
    Returns the root model item index for the items in the combobox.

    \sa setRootModelIndex()
*/

QModelIndex QComboBox::rootModelIndex() const
{
    Q_D(const QComboBox);
    return QModelIndex(d->root);
}

/*!
    Sets the root model item \a index for the items in the combobox.

    \sa rootModelIndex()
*/
void QComboBox::setRootModelIndex(const QModelIndex &index)
{
    Q_D(QComboBox);
    if (d->root == index)
        return;
    d->root = QPersistentModelIndex(index);
    view()->setRootIndex(index);
    update();
}

/*!
    \property QComboBox::currentIndex
    \brief the index of the current item in the combobox.

    The current index can change when inserting or removing items.

    By default, for an empty combo box or a combo box in which no current
    item is set, this property has a value of -1.
*/
int QComboBox::currentIndex() const
{
    Q_D(const QComboBox);
    return d->currentIndex.row();
}

void QComboBox::setCurrentIndex(int index)
{
    Q_D(QComboBox);
    QModelIndex mi = d->model->index(index, d->modelColumn, d->root);
    d->setCurrentIndex(mi);
}

void QComboBox::setCurrentText(const QString &text)
{
    if (isEditable()) {
        setEditText(text);
    } else {
        const int i = findText(text);
        if (i > -1)
            setCurrentIndex(i);
    }
}

void QComboBoxPrivate::setCurrentIndex(const QModelIndex &mi)
{
    Q_Q(QComboBox);

    QModelIndex normalized = mi.sibling(mi.row(), modelColumn); // no-op if mi.column() == modelColumn
    if (!normalized.isValid())
        normalized = mi;    // Fallback to passed index.

    bool indexChanged = (normalized != currentIndex);
    if (indexChanged)
        currentIndex = QPersistentModelIndex(normalized);
    if (lineEdit) {
        const QString newText = itemText(normalized);
        if (lineEdit->text() != newText) {
            lineEdit->setText(newText); // may cause lineEdit -> nullptr (QTBUG-54191)
#if QT_CONFIG(completer)
            if (lineEdit && lineEdit->completer())
                lineEdit->completer()->setCompletionPrefix(newText);
#endif
        }
        updateLineEditGeometry();
    }
    if (indexChanged) {
        q->update();
        _q_emitCurrentIndexChanged(currentIndex);
    }
}

/*!
    \property QComboBox::currentText
    \brief the current text

    If the combo box is editable, the current text is the value displayed
    by the line edit. Otherwise, it is the value of the current item or
    an empty string if the combo box is empty or no current item is set.

    The setter setCurrentText() simply calls setEditText() if the combo box is editable.
    Otherwise, if there is a matching text in the list, currentIndex is set to the
    corresponding index.

    \sa editable, setEditText()
*/
QString QComboBox::currentText() const
{
    Q_D(const QComboBox);
    if (d->lineEdit)
        return d->lineEdit->text();
    else if (d->currentIndex.isValid())
        return d->itemText(d->currentIndex);
    else
        return QString();
}

/*!
    \property QComboBox::currentData
    \brief the data for the current item
    \since 5.2

    By default, for an empty combo box or a combo box in which no current
    item is set, this property contains an invalid QVariant.
*/
QVariant QComboBox::currentData(int role) const
{
    Q_D(const QComboBox);
    return d->currentIndex.data(role);
}

/*!
    Returns the text for the given \a index in the combobox.
*/
QString QComboBox::itemText(int index) const
{
    Q_D(const QComboBox);
    QModelIndex mi = d->model->index(index, d->modelColumn, d->root);
    return d->itemText(mi);
}

/*!
    Returns the icon for the given \a index in the combobox.
*/
QIcon QComboBox::itemIcon(int index) const
{
    Q_D(const QComboBox);
    QModelIndex mi = d->model->index(index, d->modelColumn, d->root);
    return d->itemIcon(mi);
}

/*!
   Returns the data for the given \a role in the given \a index in the
   combobox, or QVariant::Invalid if there is no data for this role.
*/
QVariant QComboBox::itemData(int index, int role) const
{
    Q_D(const QComboBox);
    QModelIndex mi = d->model->index(index, d->modelColumn, d->root);
    return d->model->data(mi, role);
}

/*!
  \fn void QComboBox::insertItem(int index, const QString &text, const QVariant &userData)

    Inserts the \a text and \a userData (stored in the Qt::UserRole)
    into the combobox at the given \a index.

    If the index is equal to or higher than the total number of items,
    the new item is appended to the list of existing items. If the
    index is zero or negative, the new item is prepended to the list
    of existing items.

  \sa insertItems()
*/

/*!

    Inserts the \a icon, \a text and \a userData (stored in the
    Qt::UserRole) into the combobox at the given \a index.

    If the index is equal to or higher than the total number of items,
    the new item is appended to the list of existing items. If the
    index is zero or negative, the new item is prepended to the list
    of existing items.

    \sa insertItems()
*/
void QComboBox::insertItem(int index, const QIcon &icon, const QString &text, const QVariant &userData)
{
    Q_D(QComboBox);
    int itemCount = count();
    index = qBound(0, index, itemCount);
    if (index >= d->maxCount)
        return;

    // For the common case where we are using the built in QStandardItemModel
    // construct a QStandardItem, reducing the number of expensive signals from the model
    if (QStandardItemModel *m = qobject_cast<QStandardItemModel*>(d->model)) {
        QStandardItem *item = new QStandardItem(text);
        if (!icon.isNull()) item->setData(icon, Qt::DecorationRole);
        if (userData.isValid()) item->setData(userData, Qt::UserRole);
        m->insertRow(index, item);
        ++itemCount;
    } else {
        d->inserting = true;
        if (d->model->insertRows(index, 1, d->root)) {
            QModelIndex item = d->model->index(index, d->modelColumn, d->root);
            if (icon.isNull() && !userData.isValid()) {
                d->model->setData(item, text, Qt::EditRole);
            } else {
                QMap<int, QVariant> values;
                if (!text.isNull()) values.insert(Qt::EditRole, text);
                if (!icon.isNull()) values.insert(Qt::DecorationRole, icon);
                if (userData.isValid()) values.insert(Qt::UserRole, userData);
                if (!values.isEmpty()) d->model->setItemData(item, values);
            }
            d->inserting = false;
            d->_q_rowsInserted(d->root, index, index);
            ++itemCount;
        } else {
            d->inserting = false;
        }
    }

    if (itemCount > d->maxCount)
        d->model->removeRows(itemCount - 1, itemCount - d->maxCount, d->root);
}

/*!
    Inserts the strings from the \a list into the combobox as separate items,
    starting at the \a index specified.

    If the index is equal to or higher than the total number of items, the new items
    are appended to the list of existing items. If the index is zero or negative, the
    new items are prepended to the list of existing items.

    \sa insertItem()
    */
void QComboBox::insertItems(int index, const QStringList &list)
{
    Q_D(QComboBox);
    if (list.isEmpty())
        return;
    index = qBound(0, index, count());
    int insertCount = qMin(d->maxCount - index, list.count());
    if (insertCount <= 0)
        return;
    // For the common case where we are using the built in QStandardItemModel
    // construct a QStandardItem, reducing the number of expensive signals from the model
    if (QStandardItemModel *m = qobject_cast<QStandardItemModel*>(d->model)) {
        QList<QStandardItem *> items;
        items.reserve(insertCount);
        QStandardItem *hiddenRoot = m->invisibleRootItem();
        for (int i = 0; i < insertCount; ++i)
            items.append(new QStandardItem(list.at(i)));
        hiddenRoot->insertRows(index, items);
    } else {
        d->inserting = true;
        if (d->model->insertRows(index, insertCount, d->root)) {
            QModelIndex item;
            for (int i = 0; i < insertCount; ++i) {
                item = d->model->index(i+index, d->modelColumn, d->root);
                d->model->setData(item, list.at(i), Qt::EditRole);
            }
            d->inserting = false;
            d->_q_rowsInserted(d->root, index, index + insertCount - 1);
        } else {
            d->inserting = false;
        }
    }

    int mc = count();
    if (mc > d->maxCount)
        d->model->removeRows(d->maxCount, mc - d->maxCount, d->root);
}

/*!
    \since 4.4

    Inserts a separator item into the combobox at the given \a index.

    If the index is equal to or higher than the total number of items, the new item
    is appended to the list of existing items. If the index is zero or negative, the
    new item is prepended to the list of existing items.

    \sa insertItem()
*/
void QComboBox::insertSeparator(int index)
{
    Q_D(QComboBox);
    int itemCount = count();
    index = qBound(0, index, itemCount);
    if (index >= d->maxCount)
        return;
    insertItem(index, QIcon(), QString());
    QComboBoxDelegate::setSeparator(d->model, d->model->index(index, 0, d->root));
}

/*!
    Removes the item at the given \a index from the combobox.
    This will update the current index if the index is removed.

    This function does nothing if \a index is out of range.
*/
void QComboBox::removeItem(int index)
{
    Q_D(QComboBox);
    if (index < 0 || index >= count())
        return;
    d->model->removeRows(index, 1, d->root);
}

/*!
    Sets the \a text for the item on the given \a index in the combobox.
*/
void QComboBox::setItemText(int index, const QString &text)
{
    Q_D(const QComboBox);
    QModelIndex item = d->model->index(index, d->modelColumn, d->root);
    if (item.isValid()) {
        d->model->setData(item, text, Qt::EditRole);
    }
}

/*!
    Sets the \a icon for the item on the given \a index in the combobox.
*/
void QComboBox::setItemIcon(int index, const QIcon &icon)
{
    Q_D(const QComboBox);
    QModelIndex item = d->model->index(index, d->modelColumn, d->root);
    if (item.isValid()) {
        d->model->setData(item, icon, Qt::DecorationRole);
    }
}

/*!
    Sets the data \a role for the item on the given \a index in the combobox
    to the specified \a value.
*/
void QComboBox::setItemData(int index, const QVariant &value, int role)
{
    Q_D(const QComboBox);
    QModelIndex item = d->model->index(index, d->modelColumn, d->root);
    if (item.isValid()) {
        d->model->setData(item, value, role);
    }
}

/*!
    Returns the list view used for the combobox popup.
*/
QAbstractItemView *QComboBox::view() const
{
    Q_D(const QComboBox);
    return const_cast<QComboBoxPrivate*>(d)->viewContainer()->itemView();
}

/*!
  Sets the view to be used in the combobox popup to the given \a
  itemView. The combobox takes ownership of the view.

  Note: If you want to use the convenience views (like QListWidget,
  QTableWidget or QTreeWidget), make sure to call setModel() on the
  combobox with the convenience widgets model before calling this
  function.
*/
void QComboBox::setView(QAbstractItemView *itemView)
{
    Q_D(QComboBox);
    if (Q_UNLIKELY(!itemView)) {
        qWarning("QComboBox::setView: cannot set a 0 view");
        return;
    }

    if (itemView->model() != d->model)
        itemView->setModel(d->model);
    d->viewContainer()->setItemView(itemView);
}

/*!
    \reimp
*/
QSize QComboBox::minimumSizeHint() const
{
    Q_D(const QComboBox);
    return d->recomputeSizeHint(d->minimumSizeHint);
}

/*!
    \reimp

    This implementation caches the size hint to avoid resizing when
    the contents change dynamically. To invalidate the cached value
    change the \l sizeAdjustPolicy.
*/
QSize QComboBox::sizeHint() const
{
    Q_D(const QComboBox);
    return d->recomputeSizeHint(d->sizeHint);
}

#ifdef Q_OS_MAC

namespace {
struct IndexSetter {
    int index;
    QComboBox *cb;

    void operator()(void)
    {
        cb->setCurrentIndex(index);
        emit cb->activated(index);
        emit cb->activated(cb->itemText(index));
    }
};
}

void QComboBoxPrivate::cleanupNativePopup()
{
    if (!m_platformMenu)
        return;

    int count = int(m_platformMenu->tag());
    for (int i = 0; i < count; ++i)
        m_platformMenu->menuItemAt(i)->deleteLater();

    delete m_platformMenu;
    m_platformMenu = 0;
}

/*!
 * \internal
 *
 * Tries to show a native popup. Returns true if it could, false otherwise.
 *
 */
bool QComboBoxPrivate::showNativePopup()
{
    Q_Q(QComboBox);

    cleanupNativePopup();

    QPlatformTheme *theme = QGuiApplicationPrivate::instance()->platformTheme();
    m_platformMenu = theme->createPlatformMenu();
    if (!m_platformMenu)
        return false;

    int itemsCount = q->count();
    m_platformMenu->setTag(quintptr(itemsCount));

    QPlatformMenuItem *currentItem = 0;
    int currentIndex = q->currentIndex();

    for (int i = 0; i < itemsCount; ++i) {
        QPlatformMenuItem *item = theme->createPlatformMenuItem();
        QModelIndex rowIndex = model->index(i, modelColumn, root);
        QVariant textVariant = model->data(rowIndex, Qt::EditRole);
        item->setText(textVariant.toString());
        QVariant iconVariant = model->data(rowIndex, Qt::DecorationRole);
        if (iconVariant.canConvert<QIcon>())
            item->setIcon(iconVariant.value<QIcon>());
        item->setCheckable(true);
        item->setChecked(i == currentIndex);
        if (!currentItem || i == currentIndex)
            currentItem = item;

        IndexSetter setter = { i, q };
        QObject::connect(item, &QPlatformMenuItem::activated, setter);

        m_platformMenu->insertMenuItem(item, 0);
        m_platformMenu->syncMenuItem(item);
    }

    QWindow *tlw = q->window()->windowHandle();
    m_platformMenu->setFont(q->font());
    m_platformMenu->setMinimumWidth(q->rect().width());
    QPoint offset = QPoint(0, 7);
    if (q->testAttribute(Qt::WA_MacSmallSize))
        offset = QPoint(-1, 7);
    else if (q->testAttribute(Qt::WA_MacMiniSize))
        offset = QPoint(-2, 6);

    const QRect targetRect = QRect(tlw->mapFromGlobal(q->mapToGlobal(offset)), QSize());
    m_platformMenu->showPopup(tlw, QHighDpi::toNativePixels(targetRect, tlw), currentItem);

#ifdef Q_OS_OSX
    // The Cocoa popup will swallow any mouse release event.
    // We need to fake one here to un-press the button.
    QMouseEvent mouseReleased(QEvent::MouseButtonRelease, q->pos(), Qt::LeftButton,
                              Qt::MouseButtons(Qt::LeftButton), Qt::KeyboardModifiers());
    qApp->sendEvent(q, &mouseReleased);
#endif

    return true;
}

#endif // Q_OS_MAC

/*!
    Displays the list of items in the combobox. If the list is empty
    then the no items will be shown.

    If you reimplement this function to show a custom pop-up, make
    sure you call hidePopup() to reset the internal state.

    \sa hidePopup()
*/
void QComboBox::showPopup()
{
    Q_D(QComboBox);
    if (count() <= 0)
        return;

    QStyle * const style = this->style();
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    const bool usePopup = style->styleHint(QStyle::SH_ComboBox_Popup, &opt, this);

#ifdef Q_OS_MAC
    if (usePopup
        && (!d->container
            || (view()->metaObject()->className() == QByteArray("QComboBoxListView")
                && view()->itemDelegate()->metaObject()->className() == QByteArray("QComboMenuDelegate")))
        && style->styleHint(QStyle::SH_ComboBox_UseNativePopup, &opt, this)
        && d->showNativePopup())
        return;
#endif // Q_OS_MAC

#ifdef QT_KEYPAD_NAVIGATION
#if QT_CONFIG(completer)
    if (QApplication::keypadNavigationEnabled() && d->completer) {
        // editable combo box is line edit plus completer
        setEditFocus(true);
        d->completer->complete(); // show popup
        return;
    }
#endif
#endif

    // set current item and select it
    view()->selectionModel()->setCurrentIndex(d->currentIndex,
                                              QItemSelectionModel::ClearAndSelect);
    QComboBoxPrivateContainer* container = d->viewContainer();
    QRect listRect(style->subControlRect(QStyle::CC_ComboBox, &opt,
                                         QStyle::SC_ComboBoxListBoxPopup, this));
    QRect screen = d->popupGeometry(QDesktopWidgetPrivate::screenNumber(this));

    QPoint below = mapToGlobal(listRect.bottomLeft());
    int belowHeight = screen.bottom() - below.y();
    QPoint above = mapToGlobal(listRect.topLeft());
    int aboveHeight = above.y() - screen.y();
    bool boundToScreen = !window()->testAttribute(Qt::WA_DontShowOnScreen);

    {
        int listHeight = 0;
        int count = 0;
        QStack<QModelIndex> toCheck;
        toCheck.push(view()->rootIndex());
#if QT_CONFIG(treeview)
        QTreeView *treeView = qobject_cast<QTreeView*>(view());
        if (treeView && treeView->header() && !treeView->header()->isHidden())
            listHeight += treeView->header()->height();
#endif
        while (!toCheck.isEmpty()) {
            QModelIndex parent = toCheck.pop();
            for (int i = 0, end = d->model->rowCount(parent); i < end; ++i) {
                QModelIndex idx = d->model->index(i, d->modelColumn, parent);
                if (!idx.isValid())
                    continue;
                listHeight += view()->visualRect(idx).height();
#if QT_CONFIG(treeview)
                if (d->model->hasChildren(idx) && treeView && treeView->isExpanded(idx))
                    toCheck.push(idx);
#endif
                ++count;
                if (!usePopup && count >= d->maxVisibleItems) {
                    toCheck.clear();
                    break;
                }
            }
        }
        if (count > 1)
            listHeight += (count - 1) * container->spacing();
        listRect.setHeight(listHeight);
    }

    {
        // add the spacing for the grid on the top and the bottom;
        int heightMargin = container->topMargin()  + container->bottomMargin();

        // add the frame of the container
        int marginTop, marginBottom;
        container->getContentsMargins(0, &marginTop, 0, &marginBottom);
        heightMargin += marginTop + marginBottom;

        //add the frame of the view
        view()->getContentsMargins(0, &marginTop, 0, &marginBottom);
        marginTop += static_cast<QAbstractScrollAreaPrivate *>(QObjectPrivate::get(view()))->top;
        marginBottom += static_cast<QAbstractScrollAreaPrivate *>(QObjectPrivate::get(view()))->bottom;
        heightMargin += marginTop + marginBottom;

        listRect.setHeight(listRect.height() + heightMargin);
    }

    // Add space for margin at top and bottom if the style wants it.
    if (usePopup)
        listRect.setHeight(listRect.height() + style->pixelMetric(QStyle::PM_MenuVMargin, &opt, this) * 2);

    // Make sure the popup is wide enough to display its contents.
    if (usePopup) {
        const int diff = d->computeWidthHint() - width();
        if (diff > 0)
            listRect.setWidth(listRect.width() + diff);
    }

    //we need to activate the layout to make sure the min/maximum size are set when the widget was not yet show
    container->layout()->activate();
    //takes account of the minimum/maximum size of the container
    listRect.setSize( listRect.size().expandedTo(container->minimumSize())
                      .boundedTo(container->maximumSize()));

    // make sure the widget fits on screen
    if (boundToScreen) {
        if (listRect.width() > screen.width() )
            listRect.setWidth(screen.width());
        if (mapToGlobal(listRect.bottomRight()).x() > screen.right()) {
            below.setX(screen.x() + screen.width() - listRect.width());
            above.setX(screen.x() + screen.width() - listRect.width());
        }
        if (mapToGlobal(listRect.topLeft()).x() < screen.x() ) {
            below.setX(screen.x());
            above.setX(screen.x());
        }
    }

    if (usePopup) {
        // Position horizontally.
        listRect.moveLeft(above.x());

        // Position vertically so the curently selected item lines up
        // with the combo box.
        const QRect currentItemRect = view()->visualRect(view()->currentIndex());
        const int offset = listRect.top() - currentItemRect.top();
        listRect.moveTop(above.y() + offset - listRect.top());

        // Clamp the listRect height and vertical position so we don't expand outside the
        // available screen geometry.This may override the vertical position, but it is more
        // important to show as much as possible of the popup.
        const int height = !boundToScreen ? listRect.height() : qMin(listRect.height(), screen.height());
        listRect.setHeight(height);

        if (boundToScreen) {
            if (listRect.top() < screen.top())
                listRect.moveTop(screen.top());
            if (listRect.bottom() > screen.bottom())
                listRect.moveBottom(screen.bottom());
        }
    } else if (!boundToScreen || listRect.height() <= belowHeight) {
        listRect.moveTopLeft(below);
    } else if (listRect.height() <= aboveHeight) {
        listRect.moveBottomLeft(above);
    } else if (belowHeight >= aboveHeight) {
        listRect.setHeight(belowHeight);
        listRect.moveTopLeft(below);
    } else {
        listRect.setHeight(aboveHeight);
        listRect.moveBottomLeft(above);
    }

    if (qApp) {
        QGuiApplication::inputMethod()->reset();
    }

    QScrollBar *sb = view()->horizontalScrollBar();
    Qt::ScrollBarPolicy policy = view()->horizontalScrollBarPolicy();
    bool needHorizontalScrollBar = (policy == Qt::ScrollBarAsNeeded || policy == Qt::ScrollBarAlwaysOn)
                                   && sb->minimum() < sb->maximum();
    if (needHorizontalScrollBar) {
        listRect.adjust(0, 0, 0, sb->height());
    }

    // Hide the scrollers here, so that the listrect gets the full height of the container
    // If the scrollers are truly needed, the later call to container->updateScrollers()
    // will make them visible again.
    container->hideScrollers();
    container->setGeometry(listRect);

#ifndef Q_OS_MAC
    const bool updatesEnabled = container->updatesEnabled();
#endif

#if QT_CONFIG(effects)
    bool scrollDown = (listRect.topLeft() == below);
    if (QApplication::isEffectEnabled(Qt::UI_AnimateCombo)
        && !style->styleHint(QStyle::SH_ComboBox_Popup, &opt, this) && !window()->testAttribute(Qt::WA_DontShowOnScreen))
        qScrollEffect(container, scrollDown ? QEffects::DownScroll : QEffects::UpScroll, 150);
#endif

// Don't disable updates on OS X. Windows are displayed immediately on this platform,
// which means that the window will be visible before the call to container->show() returns.
// If updates are disabled at this point we'll miss our chance at painting the popup
// menu before it's shown, causing flicker since the window then displays the standard gray
// background.
#ifndef Q_OS_MAC
    container->setUpdatesEnabled(false);
#endif

    bool startTimer = !container->isVisible();
    container->raise();
    container->create();
    QWindow *containerWindow = container->window()->windowHandle();
    if (containerWindow) {
        QWindow *win = window()->windowHandle();
        if (win) {
            QScreen *currentScreen = win->screen();
            if (currentScreen && !currentScreen->virtualSiblings().contains(containerWindow->screen())) {
                containerWindow->setScreen(currentScreen);

                // This seems to workaround an issue in xcb+multi GPU+multiscreen
                // environment where the window might not always show up when screen
                // is changed.
                container->hide();
            }
        }
    }
    container->show();
    container->updateScrollers();
    view()->setFocus();

    view()->scrollTo(view()->currentIndex(),
                     style->styleHint(QStyle::SH_ComboBox_Popup, &opt, this)
                             ? QAbstractItemView::PositionAtCenter
                             : QAbstractItemView::EnsureVisible);

#ifndef Q_OS_MAC
    container->setUpdatesEnabled(updatesEnabled);
#endif

    container->update();
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled())
        view()->setEditFocus(true);
#endif
    if (startTimer) {
        container->popupTimer.start();
        container->maybeIgnoreMouseButtonRelease = true;
    }
}

/*!
    Hides the list of items in the combobox if it is currently visible
    and resets the internal state, so that if the custom pop-up was
    shown inside the reimplemented showPopup(), then you also need to
    reimplement the hidePopup() function to hide your custom pop-up
    and call the base class implementation to reset the internal state
    whenever your custom pop-up widget is hidden.

    \sa showPopup()
*/
void QComboBox::hidePopup()
{
    Q_D(QComboBox);
    if (d->container && d->container->isVisible()) {
#if QT_CONFIG(effects)
        QSignalBlocker modelBlocker(d->model);
        QSignalBlocker viewBlocker(d->container->itemView());
        QSignalBlocker containerBlocker(d->container);
        // Flash selected/triggered item (if any).
        if (style()->styleHint(QStyle::SH_Menu_FlashTriggeredItem)) {
            QItemSelectionModel *selectionModel = view() ? view()->selectionModel() : 0;
            if (selectionModel && selectionModel->hasSelection()) {
                QEventLoop eventLoop;
                const QItemSelection selection = selectionModel->selection();

                // Deselect item and wait 60 ms.
                selectionModel->select(selection, QItemSelectionModel::Toggle);
                QTimer::singleShot(60, &eventLoop, SLOT(quit()));
                eventLoop.exec();

                // Select item and wait 20 ms.
                selectionModel->select(selection, QItemSelectionModel::Toggle);
                QTimer::singleShot(20, &eventLoop, SLOT(quit()));
                eventLoop.exec();
            }
        }

        // Fade out.
        bool needFade = style()->styleHint(QStyle::SH_Menu_FadeOutOnHide);
        bool didFade = false;
        if (needFade) {
#if defined(Q_OS_MAC)
            QPlatformNativeInterface *platformNativeInterface = qApp->platformNativeInterface();
            int at = platformNativeInterface->metaObject()->indexOfMethod("fadeWindow()");
            if (at != -1) {
                QMetaMethod windowFade = platformNativeInterface->metaObject()->method(at);
                windowFade.invoke(platformNativeInterface, Q_ARG(QWindow *, d->container->windowHandle()));
                didFade = true;
            }

#endif // Q_OS_MAC
            // Other platform implementations welcome :-)
        }
        containerBlocker.unblock();
        viewBlocker.unblock();
        modelBlocker.unblock();

        if (!didFade)
#endif // QT_CONFIG(effects)
            // Fade should implicitly hide as well ;-)
            d->container->hide();
    }
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled() && isEditable() && hasFocus())
        setEditFocus(true);
#endif
    d->_q_resetButton();
}

/*!
    Clears the combobox, removing all items.

    Note: If you have set an external model on the combobox this model
    will still be cleared when calling this function.
*/
void QComboBox::clear()
{
    Q_D(QComboBox);
    d->model->removeRows(0, d->model->rowCount(d->root), d->root);
#ifndef QT_NO_ACCESSIBILITY
    QAccessibleValueChangeEvent event(this, QString());
    QAccessible::updateAccessibility(&event);
#endif
}

/*!
    Clears the contents of the line edit used for editing in the combobox.
*/
void QComboBox::clearEditText()
{
    Q_D(QComboBox);
    if (d->lineEdit)
        d->lineEdit->clear();
#ifndef QT_NO_ACCESSIBILITY
    QAccessibleValueChangeEvent event(this, QString());
    QAccessible::updateAccessibility(&event);
#endif
}

/*!
    Sets the \a text in the combobox's text edit.
*/
void QComboBox::setEditText(const QString &text)
{
    Q_D(QComboBox);
    if (d->lineEdit)
        d->lineEdit->setText(text);
#ifndef QT_NO_ACCESSIBILITY
    QAccessibleValueChangeEvent event(this, text);
    QAccessible::updateAccessibility(&event);
#endif
}

/*!
    \reimp
*/
void QComboBox::focusInEvent(QFocusEvent *e)
{
    Q_D(QComboBox);
    update();
    if (d->lineEdit) {
        d->lineEdit->event(e);
#if QT_CONFIG(completer)
        if (d->lineEdit->completer())
            d->lineEdit->completer()->setWidget(this);
#endif
    }
}

/*!
    \reimp
*/
void QComboBox::focusOutEvent(QFocusEvent *e)
{
    Q_D(QComboBox);
    update();
    if (d->lineEdit)
        d->lineEdit->event(e);
}

/*! \reimp */
void QComboBox::changeEvent(QEvent *e)
{
    Q_D(QComboBox);
    switch (e->type()) {
    case QEvent::StyleChange:
        d->updateDelegate();
#ifdef Q_OS_MAC
    case QEvent::MacSizeChange:
#endif
        d->sizeHint = QSize(); // invalidate size hint
        d->minimumSizeHint = QSize();
        d->updateLayoutDirection();
        if (d->lineEdit)
            d->updateLineEditGeometry();
        d->setLayoutItemMargins(QStyle::SE_ComboBoxLayoutItem);

        if (e->type() == QEvent::MacSizeChange){
            QPlatformTheme::Font f = QPlatformTheme::SystemFont;
            if (testAttribute(Qt::WA_MacSmallSize))
                f = QPlatformTheme::SmallFont;
            else if (testAttribute(Qt::WA_MacMiniSize))
                f = QPlatformTheme::MiniFont;
            if (const QFont *platformFont = QApplicationPrivate::platformTheme()->font(f)) {
                QFont f = font();
                f.setPointSizeF(platformFont->pointSizeF());
                setFont(f);
            }
        }
        // ### need to update scrollers etc. as well here
        break;
    case QEvent::EnabledChange:
        if (!isEnabled())
            hidePopup();
        break;
    case QEvent::PaletteChange: {
        d->updateViewContainerPaletteAndOpacity();
        break;
    }
    case QEvent::FontChange:
        d->sizeHint = QSize(); // invalidate size hint
        d->viewContainer()->setFont(font());
        if (d->lineEdit)
            d->updateLineEditGeometry();
        break;
    default:
        break;
    }
    QWidget::changeEvent(e);
}

/*!
    \reimp
*/
void QComboBox::resizeEvent(QResizeEvent *)
{
    Q_D(QComboBox);
    d->updateLineEditGeometry();
}

/*!
    \reimp
*/
void QComboBox::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    // draw the combobox frame, focusrect and selected etc.
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // draw the icon and text
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

/*!
    \reimp
*/
void QComboBox::showEvent(QShowEvent *e)
{
    Q_D(QComboBox);
    if (!d->shownOnce && d->sizeAdjustPolicy == QComboBox::AdjustToContentsOnFirstShow) {
        d->sizeHint = QSize();
        updateGeometry();
    }
    d->shownOnce = true;
    QWidget::showEvent(e);
}

/*!
    \reimp
*/
void QComboBox::hideEvent(QHideEvent *)
{
    hidePopup();
}

/*!
    \reimp
*/
bool QComboBox::event(QEvent *event)
{
    Q_D(QComboBox);
    switch(event->type()) {
    case QEvent::LayoutDirectionChange:
    case QEvent::ApplicationLayoutDirectionChange:
        d->updateLayoutDirection();
        d->updateLineEditGeometry();
        break;
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
        if (const QHoverEvent *he = static_cast<const QHoverEvent *>(event))
            d->updateHoverControl(he->pos());
        break;
    case QEvent::ShortcutOverride:
        if (d->lineEdit)
            return d->lineEdit->event(event);
        break;
#ifdef QT_KEYPAD_NAVIGATION
    case QEvent::EnterEditFocus:
        if (!d->lineEdit)
            setEditFocus(false); // We never want edit focus if we are not editable
        else
            d->lineEdit->event(event);  //so cursor starts
        break;
    case QEvent::LeaveEditFocus:
        if (d->lineEdit)
            d->lineEdit->event(event);  //so cursor stops
        break;
#endif
    default:
        break;
    }
    return QWidget::event(event);
}

/*!
    \reimp
*/
void QComboBox::mousePressEvent(QMouseEvent *e)
{
    Q_D(QComboBox);
    if (!QGuiApplication::styleHints()->setFocusOnTouchRelease())
        d->showPopupFromMouseEvent(e);
}

void QComboBoxPrivate::showPopupFromMouseEvent(QMouseEvent *e)
{
    Q_Q(QComboBox);
    QStyleOptionComboBox opt;
    q->initStyleOption(&opt);
    QStyle::SubControl sc = q->style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt, e->pos(), q);

    if (e->button() == Qt::LeftButton
            && !(sc == QStyle::SC_None && e->type() == QEvent::MouseButtonRelease)
            && (sc == QStyle::SC_ComboBoxArrow || !q->isEditable())
            && !viewContainer()->isVisible()) {
        if (sc == QStyle::SC_ComboBoxArrow)
            updateArrow(QStyle::State_Sunken);
#ifdef QT_KEYPAD_NAVIGATION
        //if the container already exists, then d->viewContainer() is safe to call
        if (container) {
#endif
            // We've restricted the next couple of lines, because by not calling
            // viewContainer(), we avoid creating the QComboBoxPrivateContainer.
            viewContainer()->initialClickPosition = q->mapToGlobal(e->pos());
#ifdef QT_KEYPAD_NAVIGATION
        }
#endif
        q->showPopup();
        // The code below ensures that regular mousepress and pick item still works
        // If it was not called the viewContainer would ignore event since it didn't have
        // a mousePressEvent first.
        if (viewContainer()) {
            viewContainer()->blockMouseReleaseTimer.start(QApplication::doubleClickInterval());
            viewContainer()->maybeIgnoreMouseButtonRelease = false;
        }
    } else {
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled() && sc == QStyle::SC_ComboBoxEditField && lineEdit) {
            lineEdit->event(e);  //so lineedit can move cursor, etc
            return;
        }
#endif
        e->ignore();
    }
}

/*!
    \reimp
*/
void QComboBox::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QComboBox);
    d->updateArrow(QStyle::State_None);
    if (QGuiApplication::styleHints()->setFocusOnTouchRelease() && hasFocus())
        d->showPopupFromMouseEvent(e);
}

/*!
    \reimp
*/
void QComboBox::keyPressEvent(QKeyEvent *e)
{
    Q_D(QComboBox);

#if QT_CONFIG(completer)
    if (const auto *cmpltr = completer()) {
        const auto *popup = QCompleterPrivate::get(cmpltr)->popup;
        if (popup && popup->isVisible()) {
            // provide same autocompletion support as line edit
            d->lineEdit->event(e);
            return;
        }
    }
#endif

    enum Move { NoMove=0 , MoveUp , MoveDown , MoveFirst , MoveLast};

    Move move = NoMove;
    int newIndex = currentIndex();
    switch (e->key()) {
    case Qt::Key_Up:
        if (e->modifiers() & Qt::ControlModifier)
            break; // pass to line edit for auto completion
        Q_FALLTHROUGH();
    case Qt::Key_PageUp:
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled())
            e->ignore();
        else
#endif
        move = MoveUp;
        break;
    case Qt::Key_Down:
        if (e->modifiers() & Qt::AltModifier) {
            showPopup();
            return;
        } else if (e->modifiers() & Qt::ControlModifier)
            break; // pass to line edit for auto completion
        Q_FALLTHROUGH();
    case Qt::Key_PageDown:
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled())
            e->ignore();
        else
#endif
        move = MoveDown;
        break;
    case Qt::Key_Home:
        if (!d->lineEdit)
            move = MoveFirst;
        break;
    case Qt::Key_End:
        if (!d->lineEdit)
            move = MoveLast;
        break;
    case Qt::Key_F4:
        if (!e->modifiers()) {
            showPopup();
            return;
        }
        break;
    case Qt::Key_Space:
        if (!d->lineEdit) {
            showPopup();
            return;
        }
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Escape:
        if (!d->lineEdit)
            e->ignore();
        break;
#ifdef QT_KEYPAD_NAVIGATION
    case Qt::Key_Select:
        if (QApplication::keypadNavigationEnabled()
                && (!hasEditFocus() || !d->lineEdit)) {
            showPopup();
            return;
        }
        break;
    case Qt::Key_Left:
    case Qt::Key_Right:
        if (QApplication::keypadNavigationEnabled() && !hasEditFocus())
            e->ignore();
        break;
    case Qt::Key_Back:
        if (QApplication::keypadNavigationEnabled()) {
            if (!hasEditFocus() || !d->lineEdit)
                e->ignore();
        } else {
            e->ignore(); // let the surounding dialog have it
        }
        break;
#endif
    default:
        if (!d->lineEdit) {
            if (!e->text().isEmpty())
                d->keyboardSearchString(e->text());
            else
                e->ignore();
        }
    }

    const int rowCount = count();

    if (move != NoMove) {
        e->accept();
        switch (move) {
        case MoveFirst:
            newIndex = -1;
            Q_FALLTHROUGH();
        case MoveDown:
            newIndex++;
            while (newIndex < rowCount && !(d->model->index(newIndex, d->modelColumn, d->root).flags() & Qt::ItemIsEnabled))
                newIndex++;
            break;
        case MoveLast:
            newIndex = rowCount;
            Q_FALLTHROUGH();
        case MoveUp:
            newIndex--;
            while ((newIndex >= 0) && !(d->model->flags(d->model->index(newIndex,d->modelColumn,d->root)) & Qt::ItemIsEnabled))
                newIndex--;
            break;
        default:
            e->ignore();
            break;
        }

        if (newIndex >= 0 && newIndex < rowCount && newIndex != currentIndex()) {
            setCurrentIndex(newIndex);
            d->emitActivated(d->currentIndex);
        }
    } else if (d->lineEdit) {
        d->lineEdit->event(e);
    }
}


/*!
    \reimp
*/
void QComboBox::keyReleaseEvent(QKeyEvent *e)
{
    Q_D(QComboBox);
    if (d->lineEdit)
        d->lineEdit->event(e);
    else
        QWidget::keyReleaseEvent(e);
}

/*!
    \reimp
*/
#if QT_CONFIG(wheelevent)
void QComboBox::wheelEvent(QWheelEvent *e)
{
    Q_D(QComboBox);
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    if (style()->styleHint(QStyle::SH_ComboBox_AllowWheelScrolling, &opt, this) &&
        !d->viewContainer()->isVisible()) {
        const int rowCount = count();
        int newIndex = currentIndex();

        if (e->delta() > 0) {
            newIndex--;
            while ((newIndex >= 0) && !(d->model->flags(d->model->index(newIndex,d->modelColumn,d->root)) & Qt::ItemIsEnabled))
                newIndex--;
        } else if (e->delta() < 0) {
            newIndex++;
            while (newIndex < rowCount && !(d->model->index(newIndex, d->modelColumn, d->root).flags() & Qt::ItemIsEnabled))
                newIndex++;
        }

        if (newIndex >= 0 && newIndex < rowCount && newIndex != currentIndex()) {
            setCurrentIndex(newIndex);
            d->emitActivated(d->currentIndex);
        }
        e->accept();
    }
}
#endif

#ifndef QT_NO_CONTEXTMENU
/*!
    \reimp
*/
void QComboBox::contextMenuEvent(QContextMenuEvent *e)
{
    Q_D(QComboBox);
    if (d->lineEdit) {
        Qt::ContextMenuPolicy p = d->lineEdit->contextMenuPolicy();
        d->lineEdit->setContextMenuPolicy(Qt::DefaultContextMenu);
        d->lineEdit->event(e);
        d->lineEdit->setContextMenuPolicy(p);
    }
}
#endif // QT_NO_CONTEXTMENU

void QComboBoxPrivate::keyboardSearchString(const QString &text)
{
    // use keyboardSearch from the listView so we do not duplicate code
    QAbstractItemView *view = viewContainer()->itemView();
    view->setCurrentIndex(currentIndex);
    int currentRow = view->currentIndex().row();
    view->keyboardSearch(text);
    if (currentRow != view->currentIndex().row()) {
        setCurrentIndex(view->currentIndex());
        emitActivated(currentIndex);
    }
}

void QComboBoxPrivate::modelChanged()
{
    Q_Q(QComboBox);

    if (sizeAdjustPolicy == QComboBox::AdjustToContents) {
        sizeHint = QSize();
        adjustComboBoxSize();
        q->updateGeometry();
    }
}

/*!
    \reimp
*/
void QComboBox::inputMethodEvent(QInputMethodEvent *e)
{
    Q_D(QComboBox);
    if (d->lineEdit) {
        d->lineEdit->event(e);
    } else {
        if (!e->commitString().isEmpty())
            d->keyboardSearchString(e->commitString());
        else
            e->ignore();
    }
}

/*!
    \reimp
*/
QVariant QComboBox::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const QComboBox);
    if (d->lineEdit)
        return d->lineEdit->inputMethodQuery(query);
    return QWidget::inputMethodQuery(query);
}

/*!\internal
*/
QVariant QComboBox::inputMethodQuery(Qt::InputMethodQuery query, const QVariant &argument) const
{
    Q_D(const QComboBox);
    if (d->lineEdit)
        return d->lineEdit->inputMethodQuery(query, argument);
    return QWidget::inputMethodQuery(query);
}

/*!
    \fn void QComboBox::addItem(const QString &text, const QVariant &userData)

    Adds an item to the combobox with the given \a text, and
    containing the specified \a userData (stored in the Qt::UserRole).
    The item is appended to the list of existing items.
*/

/*!
    \fn void QComboBox::addItem(const QIcon &icon, const QString &text,
                                const QVariant &userData)

    Adds an item to the combobox with the given \a icon and \a text,
    and containing the specified \a userData (stored in the
    Qt::UserRole). The item is appended to the list of existing items.
*/

/*!
    \fn void QComboBox::addItems(const QStringList &texts)

    Adds each of the strings in the given \a texts to the combobox. Each item
    is appended to the list of existing items in turn.
*/

/*!
    \fn void QComboBox::editTextChanged(const QString &text)

    This signal is emitted when the text in the combobox's line edit
    widget is changed. The new text is specified by \a text.
*/

/*!
    \property QComboBox::frame
    \brief whether the combo box draws itself with a frame


    If enabled (the default) the combo box draws itself inside a
    frame, otherwise the combo box draws itself without any frame.
*/
bool QComboBox::hasFrame() const
{
    Q_D(const QComboBox);
    return d->frame;
}


void QComboBox::setFrame(bool enable)
{
    Q_D(QComboBox);
    d->frame = enable;
    update();
    updateGeometry();
}

/*!
    \property QComboBox::modelColumn
    \brief the column in the model that is visible.

    If set prior to populating the combo box, the pop-up view will
    not be affected and will show the first column (using this property's
    default value).

    By default, this property has a value of 0.
*/
int QComboBox::modelColumn() const
{
    Q_D(const QComboBox);
    return d->modelColumn;
}

void QComboBox::setModelColumn(int visibleColumn)
{
    Q_D(QComboBox);
    d->modelColumn = visibleColumn;
    QListView *lv = qobject_cast<QListView *>(d->viewContainer()->itemView());
    if (lv)
        lv->setModelColumn(visibleColumn);
#if QT_CONFIG(completer)
    if (d->lineEdit && d->lineEdit->completer()
        && d->lineEdit->completer() == d->completer)
        d->lineEdit->completer()->setCompletionColumn(visibleColumn);
#endif
    setCurrentIndex(currentIndex()); //update the text to the text of the new column;
}

QT_END_NAMESPACE

#include "moc_qcombobox.cpp"
#include "moc_qcombobox_p.cpp"
