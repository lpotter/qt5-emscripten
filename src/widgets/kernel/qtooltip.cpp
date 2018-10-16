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
#if 0 // Used to be included in Qt4 for Q_WS_MAC
# include <private/qcore_mac_p.h>
#endif

#include <QtWidgets/private/qtwidgetsglobal_p.h>

#include <qapplication.h>
#include <qdesktopwidget.h>
#include <private/qdesktopwidget_p.h>
#include <qevent.h>
#include <qpointer.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qstylepainter.h>
#include <qtimer.h>
#if QT_CONFIG(effects)
#include <private/qeffects_p.h>
#endif
#include <qtextdocument.h>
#include <qdebug.h>
#include <private/qstylesheetstyle_p.h>

#ifndef QT_NO_TOOLTIP
#include <qlabel.h>
#include <qtooltip.h>

#if 0 // Used to be included in Qt4 for Q_WS_MAC
# include <private/qcore_mac_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QToolTip

    \brief The QToolTip class provides tool tips (balloon help) for any
    widget.

    \ingroup helpsystem
    \inmodule QtWidgets

    The tip is a short piece of text reminding the user of the
    widget's function. It is drawn immediately below the given
    position in a distinctive black-on-yellow color combination. The
    tip can be any \l{QTextEdit}{rich text} formatted string.

    Rich text displayed in a tool tip is implicitly word-wrapped unless
    specified differently with \c{<p style='white-space:pre'>}.

    The simplest and most common way to set a widget's tool tip is by
    calling its QWidget::setToolTip() function.

    It is also possible to show different tool tips for different
    regions of a widget, by using a QHelpEvent of type
    QEvent::ToolTip. Intercept the help event in your widget's \l
    {QWidget::}{event()} function and call QToolTip::showText() with
    the text you want to display. The \l{widgets/tooltips}{Tooltips}
    example illustrates this technique.

    If you are calling QToolTip::hideText(), or QToolTip::showText()
    with an empty string, as a result of a \l{QEvent::}{ToolTip}-event you
    should also call \l{QEvent::}{ignore()} on the event, to signal
    that you don't want to start any tooltip specific modes.

    Note that, if you want to show tooltips in an item view, the
    model/view architecture provides functionality to set an item's
    tool tip; e.g., the QTableWidgetItem::setToolTip() function.
    However, if you want to provide custom tool tips in an item view,
    you must intercept the help event in the
    QAbstractItemView::viewportEvent() function and handle it yourself.

    The default tool tip color and font can be customized with
    setPalette() and setFont(). When a tooltip is currently on
    display, isVisible() returns \c true and text() the currently visible
    text.

    \note Tool tips use the inactive color group of QPalette, because tool
    tips are not active windows.

    \sa QWidget::toolTip, QAction::toolTip, {Tool Tips Example}
*/

class QTipLabel : public QLabel
{
    Q_OBJECT
public:
    QTipLabel(const QString &text, const QPoint &pos, QWidget *w, int msecDisplayTime);
    ~QTipLabel();
    static QTipLabel *instance;

    void updateSize(const QPoint &pos);

    bool eventFilter(QObject *, QEvent *) override;

    QBasicTimer hideTimer, expireTimer;

    bool fadingOut;

    void reuseTip(const QString &text, int msecDisplayTime, const QPoint &pos);
    void hideTip();
    void hideTipImmediately();
    void setTipRect(QWidget *w, const QRect &r);
    void restartExpireTimer(int msecDisplayTime);
    bool tipChanged(const QPoint &pos, const QString &text, QObject *o);
    void placeTip(const QPoint &pos, QWidget *w);

    static int getTipScreen(const QPoint &pos, QWidget *w);
protected:
    void timerEvent(QTimerEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

#ifndef QT_NO_STYLE_STYLESHEET
public slots:
    /** \internal
      Cleanup the _q_stylesheet_parent propery.
     */
    void styleSheetParentDestroyed() {
        setProperty("_q_stylesheet_parent", QVariant());
        styleSheetParent = 0;
    }

private:
    QWidget *styleSheetParent;
#endif

private:
    QWidget *widget;
    QRect rect;
};

QTipLabel *QTipLabel::instance = 0;

QTipLabel::QTipLabel(const QString &text, const QPoint &pos, QWidget *w, int msecDisplayTime)
#ifndef QT_NO_STYLE_STYLESHEET
    : QLabel(w, Qt::ToolTip | Qt::BypassGraphicsProxyWidget), styleSheetParent(0), widget(0)
#else
    : QLabel(w, Qt::ToolTip | Qt::BypassGraphicsProxyWidget), widget(0)
#endif
{
    delete instance;
    instance = this;
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    setPalette(QToolTip::palette());
    ensurePolished();
    setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, 0, this));
    setFrameStyle(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft);
    setIndent(1);
    qApp->installEventFilter(this);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, 0, this) / 255.0);
    setMouseTracking(true);
    fadingOut = false;
    reuseTip(text, msecDisplayTime, pos);
}

void QTipLabel::restartExpireTimer(int msecDisplayTime)
{
    int time = 10000 + 40 * qMax(0, text().length()-100);
    if (msecDisplayTime > 0)
        time = msecDisplayTime;
    expireTimer.start(time, this);
    hideTimer.stop();
}

void QTipLabel::reuseTip(const QString &text, int msecDisplayTime, const QPoint &pos)
{
#ifndef QT_NO_STYLE_STYLESHEET
    if (styleSheetParent){
        disconnect(styleSheetParent, SIGNAL(destroyed()),
                   QTipLabel::instance, SLOT(styleSheetParentDestroyed()));
        styleSheetParent = 0;
    }
#endif

    setWordWrap(true);
    setText(text);
    updateSize(pos);
    restartExpireTimer(msecDisplayTime);
}

void  QTipLabel::updateSize(const QPoint &pos)
{
    QFontMetrics fm(font());
    QSize extra(1, 0);
    // Make it look good with the default ToolTip font on Mac, which has a small descent.
    if (fm.descent() == 2 && fm.ascent() >= 11)
        ++extra.rheight();
    QSize sh = sizeHint();
    if (wordWrap()) {
        const QRect screenRect = QGuiApplication::screenAt(pos)->geometry();
        if (sh.width() > screenRect.width()) {
            // Try to use widely accepted 75chars max length or 80% of the screen width else.
            // See https://en.wikipedia.org/wiki/Line_length
            sh.setWidth(qMin(fm.averageCharWidth() * 75, static_cast<int>(screenRect.width() * .8)));
            sh.setHeight(heightForWidth(sh.width()));
        }
    }
    resize(sh + extra);
}

void QTipLabel::paintEvent(QPaintEvent *ev)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.init(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();

    QLabel::paintEvent(ev);
}

void QTipLabel::resizeEvent(QResizeEvent *e)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.init(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);

    QLabel::resizeEvent(e);
}

void QTipLabel::mouseMoveEvent(QMouseEvent *e)
{
    if (!rect.isNull()) {
        QPoint pos = e->globalPos();
        if (widget)
            pos = widget->mapFromGlobal(pos);
        if (!rect.contains(pos))
            hideTip();
    }
    QLabel::mouseMoveEvent(e);
}

QTipLabel::~QTipLabel()
{
    instance = 0;
}

void QTipLabel::hideTip()
{
    if (!hideTimer.isActive())
        hideTimer.start(300, this);
}

void QTipLabel::hideTipImmediately()
{
    close(); // to trigger QEvent::Close which stops the animation
    deleteLater();
}

void QTipLabel::setTipRect(QWidget *w, const QRect &r)
{
    if (Q_UNLIKELY(!r.isNull() && !w)) {
        qWarning("QToolTip::setTipRect: Cannot pass null widget if rect is set");
        return;
    }
    widget = w;
    rect = r;
}

void QTipLabel::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == hideTimer.timerId()
        || e->timerId() == expireTimer.timerId()){
        hideTimer.stop();
        expireTimer.stop();
#if 0 /* Used to be included in Qt4 for Q_WS_MAC */ && QT_CONFIG(effects)
        if (QApplication::isEffectEnabled(Qt::UI_FadeTooltip)){
            // Fade out tip on mac (makes it invisible).
            // The tip will not be deleted until a new tip is shown.

                        // DRSWAT - Cocoa
                        macWindowFade(qt_mac_window_for(this));
            QTipLabel::instance->fadingOut = true; // will never be false again.
        }
        else
            hideTipImmediately();
#else
        hideTipImmediately();
#endif
    }
}

bool QTipLabel::eventFilter(QObject *o, QEvent *e)
{
    switch (e->type()) {
#ifdef Q_OS_MACOS
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        const int key = static_cast<QKeyEvent *>(e)->key();
        // Anything except key modifiers or caps-lock, etc.
        if (key < Qt::Key_Shift || key > Qt::Key_ScrollLock)
            hideTipImmediately();
        break;
    }
#endif
    case QEvent::Leave:
        hideTip();
        break;


#if defined (Q_OS_QNX) // On QNX the window activate and focus events are delayed and will appear
                       // after the window is shown.
    case QEvent::WindowActivate:
    case QEvent::FocusIn:
        return false;
    case QEvent::WindowDeactivate:
        if (o != this)
            return false;
        hideTipImmediately();
        break;
    case QEvent::FocusOut:
        if (reinterpret_cast<QWindow*>(o) != windowHandle())
            return false;
        hideTipImmediately();
        break;
#else
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
#endif
    case QEvent::Close: // For QTBUG-55523 (QQC) specifically: Hide tooltip when windows are closed
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        hideTipImmediately();
        break;

    case QEvent::MouseMove:
        if (o == widget && !rect.isNull() && !rect.contains(static_cast<QMouseEvent*>(e)->pos()))
            hideTip();
    default:
        break;
    }
    return false;
}

int QTipLabel::getTipScreen(const QPoint &pos, QWidget *w)
{
    if (QDesktopWidgetPrivate::isVirtualDesktop())
        return QDesktopWidgetPrivate::screenNumber(pos);
    else
        return QDesktopWidgetPrivate::screenNumber(w);
}

void QTipLabel::placeTip(const QPoint &pos, QWidget *w)
{
#ifndef QT_NO_STYLE_STYLESHEET
    if (testAttribute(Qt::WA_StyleSheet) || (w && qt_styleSheet(w->style()))) {
        //the stylesheet need to know the real parent
        QTipLabel::instance->setProperty("_q_stylesheet_parent", QVariant::fromValue(w));
        //we force the style to be the QStyleSheetStyle, and force to clear the cache as well.
        QTipLabel::instance->setStyleSheet(QLatin1String("/* */"));

        // Set up for cleaning up this later...
        QTipLabel::instance->styleSheetParent = w;
        if (w) {
            connect(w, SIGNAL(destroyed()),
                QTipLabel::instance, SLOT(styleSheetParentDestroyed()));
            // QTBUG-64550: A font inherited by the style sheet might change the size,
            // particular on Windows, where the tip is not parented on a window.
            QTipLabel::instance->updateSize(pos);
        }
    }
#endif //QT_NO_STYLE_STYLESHEET


#if 0 // Used to be included in Qt4 for Q_WS_MAC
    // When in full screen mode, there is no Dock nor Menu so we can use
    // the whole screen for displaying the tooltip. However when not in
    // full screen mode we need to save space for the dock, so we use
    // availableGeometry instead.
    extern bool qt_mac_app_fullscreen; //qapplication_mac.mm
    QRect screen;
    if(qt_mac_app_fullscreen)
        screen = QDesktopWidgetPrivate::screenGeometry(getTipScreen(pos, w));
    else
        screen = QDesktopWidgetPrivate::availableGeometry(getTipScreen(pos, w));
#else
    QRect screen = QDesktopWidgetPrivate::screenGeometry(getTipScreen(pos, w));
#endif

    QPoint p = pos;
    p += QPoint(2,
#if 0 // Used to be included in Qt4 for Q_WS_WIN
                21
#else
                16
#endif
        );
    if (p.x() + this->width() > screen.x() + screen.width())
        p.rx() -= 4 + this->width();
    if (p.y() + this->height() > screen.y() + screen.height())
        p.ry() -= 24 + this->height();
    if (p.y() < screen.y())
        p.setY(screen.y());
    if (p.x() + this->width() > screen.x() + screen.width())
        p.setX(screen.x() + screen.width() - this->width());
    if (p.x() < screen.x())
        p.setX(screen.x());
    if (p.y() + this->height() > screen.y() + screen.height())
        p.setY(screen.y() + screen.height() - this->height());
    this->move(p);
}

bool QTipLabel::tipChanged(const QPoint &pos, const QString &text, QObject *o)
{
    if (QTipLabel::instance->text() != text)
        return true;

    if (o != widget)
        return true;

    if (!rect.isNull())
        return !rect.contains(pos);
    else
       return false;
}

/*!
    Shows \a text as a tool tip, with the global position \a pos as
    the point of interest. The tool tip will be shown with a platform
    specific offset from this point of interest.

    If you specify a non-empty rect the tip will be hidden as soon
    as you move your cursor out of this area.

    The \a rect is in the coordinates of the widget you specify with
    \a w. If the \a rect is not empty you must specify a widget.
    Otherwise this argument can be 0 but it is used to determine the
    appropriate screen on multi-head systems.

    If \a text is empty the tool tip is hidden. If the text is the
    same as the currently shown tooltip, the tip will \e not move.
    You can force moving by first hiding the tip with an empty text,
    and then showing the new tip at the new position.
*/

void QToolTip::showText(const QPoint &pos, const QString &text, QWidget *w, const QRect &rect)
{
    showText(pos, text, w, rect, -1);
}

/*!
   \since 5.2
   \overload
   This is similar to QToolTip::showText(\a pos, \a text, \a w, \a rect) but with an extra parameter \a msecDisplayTime
   that specifies how long the tool tip will be displayed, in milliseconds.
*/

void QToolTip::showText(const QPoint &pos, const QString &text, QWidget *w, const QRect &rect, int msecDisplayTime)
{
    if (QTipLabel::instance && QTipLabel::instance->isVisible()){ // a tip does already exist
        if (text.isEmpty()){ // empty text means hide current tip
            QTipLabel::instance->hideTip();
            return;
        }
        else if (!QTipLabel::instance->fadingOut){
            // If the tip has changed, reuse the one
            // that is showing (removes flickering)
            QPoint localPos = pos;
            if (w)
                localPos = w->mapFromGlobal(pos);
            if (QTipLabel::instance->tipChanged(localPos, text, w)){
                QTipLabel::instance->reuseTip(text, msecDisplayTime, pos);
                QTipLabel::instance->setTipRect(w, rect);
                QTipLabel::instance->placeTip(pos, w);
            }
            return;
        }
    }

    if (!text.isEmpty()){ // no tip can be reused, create new tip:
#ifdef Q_OS_WIN32
        // On windows, we can't use the widget as parent otherwise the window will be
        // raised when the tooltip will be shown
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        new QTipLabel(text, pos, QApplication::desktop()->screen(QTipLabel::getTipScreen(pos, w)), msecDisplayTime);
QT_WARNING_POP
#else
        new QTipLabel(text, pos, w, msecDisplayTime); // sets QTipLabel::instance to itself
#endif
        QTipLabel::instance->setTipRect(w, rect);
        QTipLabel::instance->placeTip(pos, w);
        QTipLabel::instance->setObjectName(QLatin1String("qtooltip_label"));


#if QT_CONFIG(effects) && !0 /* Used to be included in Qt4 for Q_WS_MAC */
        if (QApplication::isEffectEnabled(Qt::UI_FadeTooltip))
            qFadeEffect(QTipLabel::instance);
        else if (QApplication::isEffectEnabled(Qt::UI_AnimateTooltip))
            qScrollEffect(QTipLabel::instance);
        else
            QTipLabel::instance->showNormal();
#else
        QTipLabel::instance->showNormal();
#endif
    }
}

/*!
    \overload

    This is analogous to calling QToolTip::showText(\a pos, \a text, \a w, QRect())
*/

void QToolTip::showText(const QPoint &pos, const QString &text, QWidget *w)
{
    QToolTip::showText(pos, text, w, QRect());
}


/*!
    \fn void QToolTip::hideText()
    \since 4.2

    Hides the tool tip. This is the same as calling showText() with an
    empty string.

    \sa showText()
*/


/*!
  \since 4.4

  Returns \c true if this tooltip is currently shown.

  \sa showText()
 */
bool QToolTip::isVisible()
{
    return (QTipLabel::instance != 0 && QTipLabel::instance->isVisible());
}

/*!
  \since 4.4

  Returns the tooltip text, if a tooltip is visible, or an
  empty string if a tooltip is not visible.
 */
QString QToolTip::text()
{
    if (QTipLabel::instance)
        return QTipLabel::instance->text();
    return QString();
}


Q_GLOBAL_STATIC(QPalette, tooltip_palette)

/*!
    Returns the palette used to render tooltips.

    \note Tool tips use the inactive color group of QPalette, because tool
    tips are not active windows.
*/
QPalette QToolTip::palette()
{
    return *tooltip_palette();
}

/*!
    \since 4.2

    Returns the font used to render tooltips.
*/
QFont QToolTip::font()
{
    return QApplication::font("QTipLabel");
}

/*!
    \since 4.2

    Sets the \a palette used to render tooltips.

    \note Tool tips use the inactive color group of QPalette, because tool
    tips are not active windows.
*/
void QToolTip::setPalette(const QPalette &palette)
{
    *tooltip_palette() = palette;
    if (QTipLabel::instance)
        QTipLabel::instance->setPalette(palette);
}

/*!
    \since 4.2

    Sets the \a font used to render tooltips.
*/
void QToolTip::setFont(const QFont &font)
{
    QApplication::setFont(font, "QTipLabel");
}

QT_END_NAMESPACE

#include "qtooltip.moc"
#endif // QT_NO_TOOLTIP
