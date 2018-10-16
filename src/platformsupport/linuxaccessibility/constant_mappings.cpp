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


#include "constant_mappings_p.h"

#include <qobject.h>
#include <qdebug.h>

// FIXME the assignment of roles is quite arbitrary, at some point go through this list and sort and check that it makes sense
//  "calendar" "check menu item"  "color chooser" "column header"    "dateeditor"  "desktop icon"  "desktop frame"
//  "directory pane"  "drawing area"  "file chooser" "fontchooser"  "frame"  "glass pane"  "html container"  "icon"
//  "internal frame"  "option pane"  "password text" "radio menu item"  "root pane"  "row header"    "scroll pane"
//  "tear off menu item"  "terminal" "toggle button" "tree table"  "unknown"  "viewport" "header"  "footer"  "paragraph"
//  "ruler"    "autocomplete"  "edit bar" "embedded component"  "entry"    "caption"
//  "heading"  "page"  "section"  "redundant object"  "form"  "input method window"  "menu"

#ifndef QT_NO_ACCESSIBILITY
QT_BEGIN_NAMESPACE

QHash <QAccessible::Role, RoleNames> qSpiRoleMapping;

quint64 spiStatesFromQState(QAccessible::State state)
{
    quint64 spiState = 0;

    if (state.active)
        setSpiStateBit(&spiState, ATSPI_STATE_ACTIVE);
    if (state.editable)
        setSpiStateBit(&spiState, ATSPI_STATE_EDITABLE);
    if (!state.disabled) {
        setSpiStateBit(&spiState, ATSPI_STATE_ENABLED);
        setSpiStateBit(&spiState, ATSPI_STATE_SENSITIVE);
    }
    if (state.selected)
        setSpiStateBit(&spiState, ATSPI_STATE_SELECTED);
    if (state.focused)
        setSpiStateBit(&spiState, ATSPI_STATE_FOCUSED);
    if (state.pressed)
        setSpiStateBit(&spiState, ATSPI_STATE_PRESSED);
    if (state.checked)
        setSpiStateBit(&spiState, ATSPI_STATE_CHECKED);
    if (state.checkStateMixed)
        setSpiStateBit(&spiState, ATSPI_STATE_INDETERMINATE);
    if (state.readOnly)
        unsetSpiStateBit(&spiState, ATSPI_STATE_EDITABLE);
    //        if (state.HotTracked)
    if (state.defaultButton)
        setSpiStateBit(&spiState, ATSPI_STATE_IS_DEFAULT);
    if (state.expanded)
        setSpiStateBit(&spiState, ATSPI_STATE_EXPANDED);
    if (state.collapsed)
        setSpiStateBit(&spiState, ATSPI_STATE_COLLAPSED);
    if (state.busy)
        setSpiStateBit(&spiState, ATSPI_STATE_BUSY);
    if (state.marqueed || state.animated)
        setSpiStateBit(&spiState, ATSPI_STATE_ANIMATED);
    if (!state.invisible && !state.offscreen) {
        setSpiStateBit(&spiState, ATSPI_STATE_SHOWING);
        setSpiStateBit(&spiState, ATSPI_STATE_VISIBLE);
    }
    if (state.sizeable)
        setSpiStateBit(&spiState, ATSPI_STATE_RESIZABLE);
    //        if (state.Movable)
    //        if (state.SelfVoicing)
    if (state.focusable)
        setSpiStateBit(&spiState, ATSPI_STATE_FOCUSABLE);
    if (state.selectable)
        setSpiStateBit(&spiState, ATSPI_STATE_SELECTABLE);
    //        if (state.Linked)
    if (state.traversed)
        setSpiStateBit(&spiState, ATSPI_STATE_VISITED);
    if (state.multiSelectable)
        setSpiStateBit(&spiState, ATSPI_STATE_MULTISELECTABLE);
    if (state.extSelectable)
        setSpiStateBit(&spiState, ATSPI_STATE_SELECTABLE);
    //        if (state.Protected)
    //        if (state.HasPopup)
    if (state.modal)
        setSpiStateBit(&spiState, ATSPI_STATE_MODAL);
    if (state.multiLine)
        setSpiStateBit(&spiState, ATSPI_STATE_MULTI_LINE);

    return spiState;
}

QSpiUIntList spiStateSetFromSpiStates(quint64 states)
{
    uint low = states & 0xFFFFFFFF;
    uint high = (states >> 32) & 0xFFFFFFFF;

    QSpiUIntList stateList;
    stateList.append(low);
    stateList.append(high);
    return stateList;
}

AtspiRelationType qAccessibleRelationToAtSpiRelation(QAccessible::Relation relation)
{
    switch (relation) {
    case QAccessible::Label:
        return ATSPI_RELATION_LABELLED_BY;
    case QAccessible::Labelled:
        return ATSPI_RELATION_LABEL_FOR;
    case QAccessible::Controller:
        return ATSPI_RELATION_CONTROLLED_BY;
    case QAccessible::Controlled:
        return ATSPI_RELATION_CONTROLLER_FOR;
    default:
        qWarning() << "Cannot return AT-SPI relation for:" << relation;
    }
    return ATSPI_RELATION_NULL;
}

QT_END_NAMESPACE
#endif //QT_NO_ACCESSIBILITY
