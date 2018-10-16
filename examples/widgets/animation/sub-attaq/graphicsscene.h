/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef __GRAPHICSSCENE__H__
#define __GRAPHICSSCENE__H__

//Qt
#include <QtWidgets/QGraphicsScene>
#include <QtCore/QSet>
#include <QtCore/QState>


class Boat;
class SubMarine;
class Torpedo;
class Bomb;
class PixmapItem;
class ProgressItem;
class TextInformationItem;
QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

class GraphicsScene : public QGraphicsScene
{
Q_OBJECT
public:
    enum Mode {
        Big = 0,
        Small
    };

    struct SubmarineDescription {
        int type;
        int points;
        QString name;
    };

    struct LevelDescription {
        int id;
        QString name;
        QList<QPair<int,int> > submarines;
    };

    GraphicsScene(int x, int y, int width, int height, Mode mode = Big);
    qreal sealLevel() const;
    void setupScene(QAction *newAction, QAction *quitAction);
    void addItem(Bomb *bomb);
    void addItem(Torpedo *torpedo);
    void addItem(SubMarine *submarine);
    void addItem(QGraphicsItem *item);
    void clearScene();

signals:
    void subMarineDestroyed(int);
    void allSubMarineDestroyed(int);

private slots:
    void onBombExecutionFinished();
    void onTorpedoExecutionFinished();
    void onSubMarineExecutionFinished();

private:
    Mode mode;
    ProgressItem *progressItem;
    TextInformationItem *textInformationItem;
    Boat *boat;
    QSet<SubMarine *> submarines;
    QSet<Bomb *> bombs;
    QSet<Torpedo *> torpedos;
    QVector<SubmarineDescription> submarinesData;
    QHash<int, LevelDescription> levelsData;

    friend class PauseState;
    friend class PlayState;
    friend class LevelState;
    friend class LostState;
    friend class WinState;
    friend class WinTransition;
    friend class UpdateScoreTransition;
};

#endif //__GRAPHICSSCENE__H__

