/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

#include "ui_form.h"

#define FLIGHTVIEW_URL "http://mobile.flightview.com/TrackByFlight.aspx"
#define FLIGHTVIEW_RANDOM "http://mobile.flightview.com/TrackSampleFlight.aspx"

// strips all invalid constructs that might trip QXmlStreamReader
static QString sanitized(const QString &xml)
{
    QString data = xml;

    // anything up to the html tag
    int i = data.indexOf("<html");
    if (i > 0)
        data.remove(0, i - 1);

    // everything inside the head tag
    i = data.indexOf("<head");
    if (i > 0)
        data.remove(i, data.indexOf("</head>") - i + 7);

    // invalid link for JavaScript code
    while (true) {
        i  = data.indexOf("onclick=\"gotoUrl(");
        if (i < 0)
            break;
        data.remove(i, data.indexOf('\"', i + 9) - i + 1);
    }

    // all inline frames
    while (true) {
        i  = data.indexOf("<iframe");
        if (i < 0)
            break;
        data.remove(i, data.indexOf("</iframe>") - i + 8);
    }

    // entities
    data.remove("&nbsp;");
    data.remove("&copy;");

    return data;
}

class FlightInfo : public QMainWindow
{
    Q_OBJECT

private:

    Ui_Form ui;
    QUrl m_url;
    QDate m_searchDate;
    QPixmap m_map;
    QNetworkAccessManager m_manager;
    QList<QNetworkReply *> mapReplies;

public:

    FlightInfo(QMainWindow *parent = 0): QMainWindow(parent) {

        QWidget *w = new QWidget(this);
        ui.setupUi(w);
        setCentralWidget(w);

        ui.searchBar->hide();
        ui.infoBox->hide();
        connect(ui.searchButton, SIGNAL(clicked()), SLOT(startSearch()));
        connect(ui.flightEdit, SIGNAL(returnPressed()), SLOT(startSearch()));

        setWindowTitle("Flight Info");

        // Rendered from the public-domain vectorized aircraft
        // http://openclipart.org/media/people/Jarno
        m_map = QPixmap(":/aircraft.png");

        QAction *searchTodayAction = new QAction("Today's Flight", this);
        QAction *searchYesterdayAction = new QAction("Yesterday's Flight", this);
        QAction *randomAction = new QAction("Random Flight", this);
        connect(searchTodayAction, SIGNAL(triggered()), SLOT(today()));
        connect(searchYesterdayAction, SIGNAL(triggered()), SLOT(yesterday()));
        connect(randomAction, SIGNAL(triggered()), SLOT(randomFlight()));
        connect(&m_manager, SIGNAL(finished(QNetworkReply*)),
                this, SLOT(handleNetworkData(QNetworkReply*)));
        addAction(searchTodayAction);
        addAction(searchYesterdayAction);
        addAction(randomAction);
        setContextMenuPolicy(Qt::ActionsContextMenu);
    }

private slots:

    void handleNetworkData(QNetworkReply *networkReply) {
        if (!networkReply->error()) {
            if (!mapReplies.contains(networkReply)) {
                // Assume UTF-8 encoded
                QByteArray data = networkReply->readAll();
                QString xml = QString::fromUtf8(data);
                digest(xml);
            } else {
                mapReplies.removeOne(networkReply);
                m_map.loadFromData(networkReply->readAll());
                update();
            }
        }
        networkReply->deleteLater();
    }

    void today() {
        QDateTime timestamp = QDateTime::currentDateTime();
        m_searchDate = timestamp.date();
        searchFlight();
    }

    void yesterday() {
        QDateTime timestamp = QDateTime::currentDateTime();
        timestamp = timestamp.addDays(-1);
        m_searchDate = timestamp.date();
        searchFlight();
    }

    void searchFlight() {
        ui.searchBar->show();
        ui.infoBox->hide();
        ui.flightStatus->hide();
        ui.flightName->setText("Enter flight number");
        ui.flightEdit->setFocus();
#ifdef QT_KEYPAD_NAVIGATION
        ui.flightEdit->setEditFocus(true);
#endif
        m_map = QPixmap();
        update();
    }

    void startSearch() {
        ui.searchBar->hide();
        QString flight = ui.flightEdit->text().simplified();
        if (!flight.isEmpty())
            request(flight, m_searchDate);
    }

    void randomFlight() {
        request(QString(), QDate::currentDate());
    }

public slots:

    void request(const QString &flightCode, const QDate &date) {

        setWindowTitle("Loading...");

        QString code = flightCode.simplified();
        QString airlineCode = code.left(2).toUpper();
        QString flightNumber = code.mid(2, code.length());

        ui.flightName->setText("Searching for " + code);

        QUrlQuery query;
        query.addQueryItem("view", "detail");
        query.addQueryItem("al", airlineCode);
        query.addQueryItem("fn", flightNumber);
        query.addQueryItem("dpdat", date.toString("yyyyMMdd"));
        m_url = QUrl(FLIGHTVIEW_URL);
        m_url.setQuery(query);

        if (code.isEmpty()) {
            // random flight as sample
            m_url = QUrl(FLIGHTVIEW_RANDOM);
            ui.flightName->setText("Getting a random flight...");
        }

        m_manager.get(QNetworkRequest(m_url));
    }


private:

    void digest(const QString &content) {

        setWindowTitle("Flight Info");
        QString data = sanitized(content);

        // do we only get the flight list?
        // we grab the first leg in the flight list
        // then fetch another URL for the real flight info
        int i = data.indexOf("a href=\"?view=detail");
        if (i > 0) {
            QString href = data.mid(i, data.indexOf('\"', i + 8) - i + 1);
            QRegularExpression regex("dpap=([A-Za-z0-9]+)");
            QRegularExpressionMatch match = regex.match(href);
            QString airport = match.captured(1);
            QUrlQuery query(m_url);
            query.addQueryItem("dpap", airport);
            m_url.setQuery(query);
            m_manager.get(QNetworkRequest(m_url));
            return;
        }

        QXmlStreamReader xml(data);
        bool inFlightName = false;
        bool inFlightStatus = false;
        bool inFlightMap = false;
        bool inFieldName = false;
        bool inFieldValue = false;

        QString flightName;
        QString flightStatus;
        QStringList fieldNames;
        QStringList fieldValues;

        while (!xml.atEnd()) {
            xml.readNext();

            if (xml.tokenType() == QXmlStreamReader::StartElement) {
                QStringRef className = xml.attributes().value("class");
                inFlightName |= xml.name() == "h1";
                inFlightStatus |= className == "FlightDetailHeaderStatus";
                inFlightMap |= className == "flightMap";
                if (xml.name() == "td" && !className.isEmpty()) {
                    if (className.contains("fieldTitle")) {
                        inFieldName = true;
                        fieldNames += QString();
                        fieldValues += QString();
                    }
                    if (className.contains("fieldValue"))
                        inFieldValue = true;
                }
                if (xml.name() == "img" && inFlightMap) {
                    const QByteArray encoded
                        = ("http://mobile.flightview.com/" % xml.attributes().value("src")).toLatin1();
                    QUrl url = QUrl::fromPercentEncoding(encoded);
                    mapReplies.append(m_manager.get(QNetworkRequest(url)));
                }
            }

            if (xml.tokenType() == QXmlStreamReader::EndElement) {
                inFlightName &= xml.name() != "h1";
                inFlightStatus &= xml.name() != "div";
                inFlightMap &= xml.name() != "div";
                inFieldName &= xml.name() != "td";
                inFieldValue &= xml.name() != "td";
            }

            if (xml.tokenType() == QXmlStreamReader::Characters) {
                if (inFlightName)
                    flightName += xml.text();
                if (inFlightStatus)
                    flightStatus += xml.text();
                if (inFieldName)
                    fieldNames.last() += xml.text();
                if (inFieldValue)
                    fieldValues.last() += xml.text();
            }
        }

        if (fieldNames.isEmpty()) {
            QString code = ui.flightEdit->text().simplified().left(10);
            QString msg = QString("Flight %1 is not found").arg(code);
            ui.flightName->setText(msg);
            return;
        }

        ui.flightName->setText(flightName);
        flightStatus.remove("Status: ");
        ui.flightStatus->setText(flightStatus);
        ui.flightStatus->show();

        QStringList whiteList;
        whiteList << "Departure";
        whiteList << "Arrival";
        whiteList << "Scheduled";
        whiteList << "Takeoff";
        whiteList << "Estimated";
        whiteList << "Term-Gate";

        QString text;
        text = QString("<table width=%1>").arg(width() - 25);
        for (int i = 0; i < fieldNames.count(); i++) {
            QString fn = fieldNames[i].simplified();
            if (fn.endsWith(':'))
                fn = fn.left(fn.length() - 1);
            if (!whiteList.contains(fn))
                continue;

            QString fv = fieldValues[i].simplified();
            bool special = false;
            special |= fn.startsWith("Departure");
            special |= fn.startsWith("Arrival");
            text += "<tr>";
            if (special) {
                text += "<td align=center colspan=2>";
                text += "<b><font size=+1>" + fv + "</font></b>";
                text += "</td>";
            } else {
                text += "<td align=right>";
                text += fn;
                text += " : ";
                text += "&nbsp;";
                text += "</td>";
                text += "<td>";
                text += fv;
                text += "</td>";
            }
            text += "</tr>";
        }
        text += "</table>";
        ui.detailedInfo->setText(text);
        ui.infoBox->show();
    }

    void resizeEvent(QResizeEvent *event) {
        Q_UNUSED(event);
        ui.detailedInfo->setMaximumWidth(width() - 25);
    }

    void paintEvent(QPaintEvent *event) {
        QMainWindow::paintEvent(event);
        QPainter p(this);
        p.fillRect(rect(), QColor(131, 171, 210));
        if (!m_map.isNull()) {
            int x = (width() - m_map.width()) / 2;
            int space = ui.infoBox->pos().y();
            if (!ui.infoBox->isVisible())
                space = height();
            int top = ui.titleBox->height();
            int y = qMax(top, (space - m_map.height()) / 2);
            p.drawPixmap(x, y, m_map);
        }
        p.end();
    }

};


#include "flightinfo.moc"

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(flightinfo);

    QApplication app(argc, argv);

    FlightInfo w;
    w.resize(360, 504);
    w.show();

    return app.exec();
}
