/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#include "game.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QTextStream>

Character Game::player() const
{
    return mPlayer;
}

QVector<Level> Game::levels() const
{
    return mLevels;
}

//! [0]
void Game::newGame()
{
    mPlayer = Character();
    mPlayer.setName(QStringLiteral("Hero"));
    mPlayer.setClassType(Character::Archer);
    mPlayer.setLevel(QRandomGenerator::global()->bounded(15, 21));

    mLevels.clear();
    mLevels.reserve(2);

    Level village(QStringLiteral("Village"));
    QVector<Character> villageNpcs;
    villageNpcs.reserve(2);
    villageNpcs.append(Character(QStringLiteral("Barry the Blacksmith"),
                                 QRandomGenerator::global()->bounded(8, 11),
                                 Character::Warrior));
    villageNpcs.append(Character(QStringLiteral("Terry the Trader"),
                                 QRandomGenerator::global()->bounded(6, 8),
                                 Character::Warrior));
    village.setNpcs(villageNpcs);
    mLevels.append(village);

    Level dungeon(QStringLiteral("Dungeon"));
    QVector<Character> dungeonNpcs;
    dungeonNpcs.reserve(3);
    dungeonNpcs.append(Character(QStringLiteral("Eric the Evil"),
                                 QRandomGenerator::global()->bounded(18, 26),
                                 Character::Mage));
    dungeonNpcs.append(Character(QStringLiteral("Eric's Left Minion"),
                                 QRandomGenerator::global()->bounded(5, 7),
                                 Character::Warrior));
    dungeonNpcs.append(Character(QStringLiteral("Eric's Right Minion"),
                                 QRandomGenerator::global()->bounded(4, 9),
                                 Character::Warrior));
    dungeon.setNpcs(dungeonNpcs);
    mLevels.append(dungeon);
}
//! [0]

//! [3]
bool Game::loadGame(Game::SaveFormat saveFormat)
{
    QFile loadFile(saveFormat == Json
        ? QStringLiteral("save.json")
        : QStringLiteral("save.dat"));

    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QByteArray saveData = loadFile.readAll();

    QJsonDocument loadDoc(saveFormat == Json
        ? QJsonDocument::fromJson(saveData)
        : QJsonDocument::fromBinaryData(saveData));

    read(loadDoc.object());

    QTextStream(stdout) << "Loaded save for "
                        << loadDoc["player"]["name"].toString()
                        << " using "
                        << (saveFormat != Json ? "binary " : "") << "JSON...\n";
    return true;
}
//! [3]

//! [4]
bool Game::saveGame(Game::SaveFormat saveFormat) const
{
    QFile saveFile(saveFormat == Json
        ? QStringLiteral("save.json")
        : QStringLiteral("save.dat"));

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QJsonObject gameObject;
    write(gameObject);
    QJsonDocument saveDoc(gameObject);
    saveFile.write(saveFormat == Json
        ? saveDoc.toJson()
        : saveDoc.toBinaryData());

    return true;
}
//! [4]

//! [1]
void Game::read(const QJsonObject &json)
{
    if (json.contains("player") && json["player"].isObject())
        mPlayer.read(json["player"].toObject());

    if (json.contains("levels") && json["levels"].isArray()) {
        QJsonArray levelArray = json["levels"].toArray();
        mLevels.clear();
        mLevels.reserve(levelArray.size());
        for (int levelIndex = 0; levelIndex < levelArray.size(); ++levelIndex) {
            QJsonObject levelObject = levelArray[levelIndex].toObject();
            Level level;
            level.read(levelObject);
            mLevels.append(level);
        }
    }
}
//! [1]

//! [2]
void Game::write(QJsonObject &json) const
{
    QJsonObject playerObject;
    mPlayer.write(playerObject);
    json["player"] = playerObject;

    QJsonArray levelArray;
    foreach (const Level level, mLevels) {
        QJsonObject levelObject;
        level.write(levelObject);
        levelArray.append(levelObject);
    }
    json["levels"] = levelArray;
}
//! [2]

void Game::print(int indentation) const
{
    const QString indent(indentation * 2, ' ');
    QTextStream(stdout) << indent << "Player\n";
    mPlayer.print(indentation + 1);

    QTextStream(stdout) << indent << "Levels\n";
    for (Level level : mLevels)
        level.print(indentation + 1);
}
