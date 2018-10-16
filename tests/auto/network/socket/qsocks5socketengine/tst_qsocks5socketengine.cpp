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


#include <QtTest/QTest>
#include <QtTest/QTestEventLoop>

#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QCoreApplication>
#include <QtCore/QMetaType>
#include <QtCore/QTimer>

#include <private/qsocks5socketengine_p.h>
#include <qhostinfo.h>
#include <qhostaddress.h>
#include <qtcpsocket.h>
#include <qauthenticator.h>
#include <qdebug.h>
#include <qtcpserver.h>
#include <qmetatype.h>
#include <qdebug.h>

#include "../../../network-settings.h"

class tst_QSocks5SocketEngine : public QObject, public QAbstractSocketEngineReceiver
{
    Q_OBJECT

private slots:
    void initTestCase();
    void construction();
    void errorTest_data();
    void errorTest();
    void simpleConnectToIMAP();
    void simpleErrorsAndStates();
    void udpTest();
    void serverTest();
    void tcpSocketBlockingTest();
    void tcpSocketNonBlockingTest();
    void downloadBigFile();
   // void tcpLoopbackPerformance();
    void passwordAuth();
    void passwordAuth2();
    void fragmentation_data();
    void fragmentation();
    void incomplete_data();
    void incomplete();

protected slots:
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth);

private:
    void readNotification() { }
    void writeNotification() { }
    void closeNotification() { }
    void exceptionNotification() { }
    void connectionNotification() { }
};

class MiniSocks5ResponseHandler : public QObject
{
    Q_OBJECT
public:
    QQueue<QByteArray> responses;
    QTcpSocket *client;

    MiniSocks5ResponseHandler(QQueue<QByteArray> r, QTcpSocket *c, int autoResponseTime)
        : responses(r), client(c)
    {
        client->setParent(this);
        connect(client, SIGNAL(disconnected()), SLOT(deleteLater()));
        connect(client, SIGNAL(readyRead()), SLOT(sendNextResponse()));
        if (autoResponseTime)
            QTimer::singleShot(autoResponseTime, this, SLOT(sendNextResponse()));
    }

private slots:
    void sendNextResponse()
    {
        // WARNING
        // this assumes that the client command is received in its entirety
        // should be ok, since SOCKSv5 commands are rather small
        if (responses.isEmpty())
            client->disconnectFromHost();
        else
            client->write(responses.dequeue());
    }
};

class MiniSocks5Server: public QTcpServer
{
    Q_OBJECT
public:
    QQueue<QByteArray> responses;
    int autoResponseTime;

    MiniSocks5Server(const QQueue<QByteArray> r, int t = 0)
        : responses(r), autoResponseTime(t)
    {
        listen();
        connect(this, SIGNAL(newConnection()), SLOT(handleNewConnection()));
    }

private slots:
    void handleNewConnection()
    {
        QTcpSocket *client = nextPendingConnection();
        new MiniSocks5ResponseHandler(responses, client, autoResponseTime);
    }
};

void tst_QSocks5SocketEngine::initTestCase()
{
    QVERIFY(QtNetworkSettings::verifyTestNetworkSettings());
}

//---------------------------------------------------------------------------
void tst_QSocks5SocketEngine::construction()
{
    QSocks5SocketEngine socketDevice;

    QVERIFY(!socketDevice.isValid());

    // Initialize device
    QVERIFY(socketDevice.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QVERIFY(socketDevice.isValid());
    QCOMPARE(socketDevice.protocol(), QAbstractSocket::IPv4Protocol);
    QCOMPARE(socketDevice.socketType(), QAbstractSocket::TcpSocket);
    QCOMPARE(socketDevice.state(), QAbstractSocket::UnconnectedState);
   // QVERIFY(socketDevice.socketDescriptor() != -1);
    QCOMPARE(socketDevice.localAddress(), QHostAddress());
    QCOMPARE(socketDevice.localPort(), quint16(0));
    QCOMPARE(socketDevice.peerAddress(), QHostAddress());
    QCOMPARE(socketDevice.peerPort(), quint16(0));
    QCOMPARE(socketDevice.error(), QAbstractSocket::UnknownSocketError);

    //QTest::ignoreMessage(QtWarningMsg, "QSocketLayer::bytesAvailable() was called in QAbstractSocket::UnconnectedState");
    QCOMPARE(socketDevice.bytesAvailable(), 0);

    //QTest::ignoreMessage(QtWarningMsg, "QSocketLayer::hasPendingDatagrams() was called in QAbstractSocket::UnconnectedState");
    QVERIFY(!socketDevice.hasPendingDatagrams());
}

//---------------------------------------------------------------------------
void tst_QSocks5SocketEngine::errorTest_data()
{
    QTest::addColumn<QString>("hostname");
    QTest::addColumn<int>("port");
    QTest::addColumn<QString>("username");
    QTest::addColumn<QQueue<QByteArray> >("responses");
    QTest::addColumn<int>("expectedError");

    QQueue<QByteArray> responses;
    QTest::newRow("proxy-host-not-found") << "this-host-does-not-exist." << 1080 << QString()
                                          << responses
                                          << int(QAbstractSocket::ProxyNotFoundError);
    QTest::newRow("proxy-connection-refused") << "127.0.0.1" << 2 << QString()
                                              << responses
                                              << int(QAbstractSocket::ProxyConnectionRefusedError);

#define REPLY(name, contents)                                           \
    static const char raw_ ## name [] = contents;                       \
    const QByteArray name = QByteArray::fromRawData(raw_ ## name, sizeof raw_ ## name - 1)

    REPLY(garbage, "\4\4\4\4");
    // authentication method replies
    REPLY(noAuthentication, "\5\0");
    REPLY(passwordAuthentication, "\5\2");
    REPLY(garbageAuthentication, "\5\177");
    REPLY(noAcceptableAuthentication, "\5\377");
    // authentication replies
    REPLY(authenticationAccepted, "\5\0");
    REPLY(authenticationNotAccepted, "\5\1");
    // connection replies
    REPLY(connectionAccepted, "\5\0\0\4\177\0\0\1\0\100");
    REPLY(connectionNotAllowed, "\5\2\0");
    REPLY(networkUnreachable, "\5\3\0");
    REPLY(hostUnreachable, "\5\4\0");
    REPLY(connectionRefused, "\5\5\0");

#undef REPLY

    responses << garbage;
    QTest::newRow("garbage1") << QString() << 0 << QString() << responses
                              << int(QAbstractSocket::ProxyProtocolError);

    responses.clear();
    responses << noAuthentication << garbage;
    QTest::newRow("garbage2") << QString() << 0 << QString() << responses
                              << int(QAbstractSocket::ProxyProtocolError);

    responses.clear();
    responses << garbageAuthentication;
    QTest::newRow("unknown-auth-method") << QString() << 0 << QString()
                                         << responses
                                         << int(QAbstractSocket::SocketAccessError);

    responses.clear();
    responses << noAcceptableAuthentication;
    QTest::newRow("no-acceptable-authentication") << QString() << 0 << QString()
                                                  << responses
                                                  << int(QAbstractSocket::ProxyAuthenticationRequiredError);

    responses.clear();
    responses << passwordAuthentication << authenticationNotAccepted;
    QTest::newRow("authentication-required") << QString() << 0 << "foo"
                                             << responses
                                             << int(QAbstractSocket::ProxyAuthenticationRequiredError);

    responses.clear();
    responses << noAuthentication << connectionNotAllowed;
    QTest::newRow("connection-not-allowed") << QString() << 0 << QString()
                                            << responses
                                            << int(QAbstractSocket::SocketAccessError);

    responses.clear();
    responses << noAuthentication << networkUnreachable;
    QTest::newRow("network-unreachable") << QString() << 0 << QString()
                                         << responses
                                         << int(QAbstractSocket::NetworkError);

    responses.clear();
    responses << noAuthentication << hostUnreachable;
    QTest::newRow("host-unreachable") << QString() << 0 << QString()
                                      << responses
                                      << int(QAbstractSocket::HostNotFoundError);

    responses.clear();
    responses << noAuthentication << connectionRefused;
    QTest::newRow("connection-refused") << QString() << 0 << QString()
                                        << responses
                                        << int(QAbstractSocket::ConnectionRefusedError);
}

void tst_QSocks5SocketEngine::errorTest()
{
    QFETCH(QString, hostname);
    QFETCH(int, port);
    QFETCH(QString, username);
    QFETCH(QQueue<QByteArray>, responses);
    QFETCH(int, expectedError);

    MiniSocks5Server server(responses);

    if (hostname.isEmpty()) {
        hostname = "127.0.0.1";
        port = server.serverPort();
    }
    QTcpSocket socket;
    socket.setProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, hostname, port, username, username));
    socket.connectToHost("0.1.2.3", 12345);

    connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(int(socket.error()), expectedError);
}

//---------------------------------------------------------------------------
void tst_QSocks5SocketEngine::simpleConnectToIMAP()
{
    QSocks5SocketEngine socketDevice;

    // Initialize device
    QVERIFY(socketDevice.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QCOMPARE(socketDevice.state(), QAbstractSocket::UnconnectedState);

    socketDevice.setProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080));

    QVERIFY(!socketDevice.connectToHost(QtNetworkSettings::serverIP(), 143));
    QCOMPARE(socketDevice.state(), QAbstractSocket::ConnectingState);
    QVERIFY(socketDevice.waitForWrite());
    QCOMPARE(socketDevice.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socketDevice.peerAddress(), QtNetworkSettings::serverIP());

    // Wait for the greeting
    QVERIFY(socketDevice.waitForRead());

    // Read the greeting
    qint64 available = socketDevice.bytesAvailable();
    QVERIFY(available > 0);
    QByteArray array;
    array.resize(available);
    QVERIFY(socketDevice.read(array.data(), array.size()) == available);

    // Check that the greeting is what we expect it to be
    QVERIFY2(QtNetworkSettings::compareReplyIMAP(array), array.constData());

    // Write a logout message
    QByteArray array2 = "XXXX LOGOUT\r\n";
    QVERIFY(socketDevice.write(array2.data(),
                              array2.size()) == array2.size());

    // Wait for the response
    QVERIFY(socketDevice.waitForRead());

    available = socketDevice.bytesAvailable();
    QVERIFY(available > 0);
    array.resize(available);
    QVERIFY(socketDevice.read(array.data(), array.size()) == available);

    // Check that the greeting is what we expect it to be
    QCOMPARE(array.constData(), "* BYE LOGOUT received\r\nXXXX OK Completed\r\n");

    // Wait for the response
    QVERIFY(socketDevice.waitForRead());
    char c;
    QVERIFY(socketDevice.read(&c, sizeof(c)) == -1);
    QCOMPARE(socketDevice.error(), QAbstractSocket::RemoteHostClosedError);
    QCOMPARE(socketDevice.state(), QAbstractSocket::UnconnectedState);
}

//---------------------------------------------------------------------------
void tst_QSocks5SocketEngine::simpleErrorsAndStates()
{
    {
        QSocks5SocketEngine socketDevice;

        // Initialize device
        QVERIFY(socketDevice.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));

        socketDevice.setProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080));

        QCOMPARE(socketDevice.state(), QAbstractSocket::UnconnectedState);
        QVERIFY(!socketDevice.connectToHost(QHostInfo::fromName(QtNetworkSettings::serverName()).addresses().first(), 8088));
        QCOMPARE(socketDevice.state(), QAbstractSocket::ConnectingState);
        if (socketDevice.waitForWrite(15000)) {
            QVERIFY(socketDevice.state() == QAbstractSocket::UnconnectedState ||
                    socketDevice.state() == QAbstractSocket::ConnectedState);
        } else {
            QCOMPARE(socketDevice.error(), QAbstractSocket::SocketTimeoutError);
        }
    }

}

/*
//---------------------------------------------------------------------------
void tst_QSocks5SocketEngine::tcpLoopbackPerformance()
{
    QTcpServer server;

    // Bind to any port on all interfaces
    QVERIFY(server.bind(QHostAddress("0.0.0.0"), 0));
    QCOMPARE(server.state(), QAbstractSocket::BoundState);
    quint16 port = server.localPort();

    // Listen for incoming connections
    QVERIFY(server.listen());
    QCOMPARE(server.state(), QAbstractSocket::ListeningState);

    // Initialize a Tcp socket
    QSocks5SocketEngine client;
    QVERIFY(client.initialize(QAbstractSocket::TcpSocket));

    client.setProxy(QHostAddress("80.232.37.158"), 1081);

    // Connect to our server
    if (!client.connectToHost(QHostAddress("127.0.0.1"), port)) {
        QVERIFY(client.waitForWrite());
        QVERIFY(client.connectToHost(QHostAddress("127.0.0.1"), port));
    }

    // The server accepts the connectio
    int socketDescriptor = server.accept();
    QVERIFY(socketDescriptor > 0);

    // A socket device is initialized on the server side, passing the
    // socket descriptor from accept(). It's pre-connected.
    QSocketLayer serverSocket;
    QVERIFY(serverSocket.initialize(socketDescriptor));
    QCOMPARE(serverSocket.state(), QAbstractSocket::ConnectedState);

    const int messageSize = 1024 * 256;
    QByteArray message1(messageSize, '@');
    QByteArray answer(messageSize, '@');

    QTime timer;
    timer.start();
    qlonglong readBytes = 0;
    while (timer.elapsed() < 5000) {
        qlonglong written = serverSocket.write(message1.data(), message1.size());
        while (written > 0) {
            client.waitForRead();
            if (client.bytesAvailable() > 0) {
                qlonglong readNow = client.read(answer.data(), answer.size());
                written -= readNow;
                readBytes += readNow;
            }
        }
    }

    qDebug("\t\t%.1fMB/%.1fs: %.1fMB/s",
           readBytes / (1024.0 * 1024.0),
           timer.elapsed() / 1024.0,
           (readBytes / (timer.elapsed() / 1000.0)) / (1024 * 1024));
}
*/

//---------------------------------------------------------------------------
void tst_QSocks5SocketEngine::serverTest()
{
    QSocks5SocketEngine server;

    // Initialize a Tcp socket
    QVERIFY(server.initialize(QAbstractSocket::TcpSocket));

    QNetworkProxy proxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080);

    server.setProxy(proxy);

    // Bind to any port on all interfaces
    QVERIFY(server.bind(QHostAddress("0.0.0.0"), 0));
    QCOMPARE(server.state(), QAbstractSocket::BoundState);

    // Listen for incoming connections
    QVERIFY(server.listen());
    QCOMPARE(server.state(), QAbstractSocket::ListeningState);

    // Initialize a Tcp socket
    QSocks5SocketEngine client;
    QVERIFY(client.initialize(QAbstractSocket::TcpSocket));

    client.setProxy(proxy);

   // QTest::wait(100000); // ### timing problem on win32


    // Connect to our server
    if (!client.connectToHost(server.localAddress(), server.localPort())) {
        QVERIFY(client.waitForWrite());
       // QTest::wait(100); // ### timing problem on win32
        QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
        //QTest::wait(100);
    }

    QVERIFY(server.waitForRead());

    // The server accepts the connection
    int socketDescriptor = server.accept();
    QVERIFY(socketDescriptor > 0);

    // A socket device is initialized on the server side, passing the
    // socket descriptor from accept(). It's pre-connected.

    QSocks5SocketEngine serverSocket;
    QVERIFY(serverSocket.initialize(socketDescriptor));
    QCOMPARE(serverSocket.state(), QAbstractSocket::ConnectedState);

    QCOMPARE(serverSocket.localAddress(), client.peerAddress());
    QCOMPARE(serverSocket.localPort(), client.peerPort());
    // this seems depends on the socks server implementation, especially
    // when connecting /to/ the socks server /through/ the same socks server
    //QCOMPARE(serverSocket.peerAddress(), client.localAddress());
    //QCOMPARE(serverSocket.peerPort(), client.localPort());

    // The server socket sends a greeting to the client
    QByteArray greeting = "Greetings!";
    QVERIFY(serverSocket.write(greeting.data(),
                              greeting.size()) == greeting.size());

    // The client waits for the greeting to arrive
    QVERIFY(client.waitForRead());
    qint64 available = client.bytesAvailable();
    QVERIFY(available > 0);

    // The client reads the greeting and checks that it's correct
    QByteArray response;
    response.resize(available);
    QVERIFY(client.read(response.data(),
                       response.size()) == response.size());
    QCOMPARE(response, greeting);
}


//---------------------------------------------------------------------------
void tst_QSocks5SocketEngine::udpTest()
{
    QSocks5SocketEngine udpSocket;

    // Initialize device #1
    QVERIFY(udpSocket.initialize(QAbstractSocket::UdpSocket));
    QVERIFY(udpSocket.isValid());

    QNetworkProxy proxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080);

    udpSocket.setProxy(proxy);

    QCOMPARE(udpSocket.protocol(), QAbstractSocket::IPv4Protocol);
    QCOMPARE(udpSocket.socketType(), QAbstractSocket::UdpSocket);
    QCOMPARE(udpSocket.state(), QAbstractSocket::UnconnectedState);

    // Bind #1
    bool bindSuccessful = udpSocket.bind(QHostAddress("0.0.0.0"), 0);
    if (!bindSuccessful)
        QEXPECT_FAIL("", "QTBUG-23380 / QTBUG-35490: Fails on some Ubuntu 11.10 x64 configurations and on new network test server", Abort);
    QVERIFY(bindSuccessful);
    QCOMPARE(udpSocket.state(), QAbstractSocket::BoundState);
    QVERIFY(udpSocket.localPort() != 0);

    // Initialize device #2
    QSocks5SocketEngine udpSocket2;
    QVERIFY(udpSocket2.initialize(QAbstractSocket::UdpSocket));

    udpSocket2.setProxy(proxy);

    // Connect device #2 to #1
    QVERIFY(udpSocket2.connectToHost(udpSocket.localAddress(), udpSocket.localPort()));
    QCOMPARE(udpSocket2.state(), QAbstractSocket::ConnectedState);

    // Write a message to #1
    QByteArray message1 = "hei der";
    QVERIFY(udpSocket2.write(message1.data(),
                            message1.size()) == message1.size());

    // Read the message from #2
    QVERIFY(udpSocket.waitForRead());
    QVERIFY(udpSocket.hasPendingDatagrams());
    qint64 available = udpSocket.pendingDatagramSize();
    QVERIFY(available > 0);
    QByteArray answer;
    answer.resize(available);
    QIpPacketHeader header;
    QCOMPARE(udpSocket.readDatagram(answer.data(), answer.size(),
                                    &header, QAbstractSocketEngine::WantDatagramSender),
             qint64(message1.size()));
    QVERIFY(header.senderAddress == udpSocket2.localAddress());
    QCOMPARE(header.senderAddress, udpSocket2.localAddress());
    QCOMPARE(header.senderPort, udpSocket2.localPort());
}

void tst_QSocks5SocketEngine::tcpSocketBlockingTest()
{
    QSocks5SocketEngineHandler socks5;

    QTcpSocket socket;

    // Connect
    socket.connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(socket.waitForConnected());
    QCOMPARE(socket.state(), QTcpSocket::ConnectedState);

    // Read greeting
    QVERIFY(socket.waitForReadyRead(5000));
    QByteArray s = socket.readLine();
    QVERIFY2(QtNetworkSettings::compareReplyIMAP(s), s.constData());

    // Write NOOP
    QCOMPARE((int) socket.write("1 NOOP\r\n", 8), 8);

    if (!socket.canReadLine())
        QVERIFY(socket.waitForReadyRead(5000));

    // Read response
    s = socket.readLine();
    QCOMPARE(s, QByteArrayLiteral("1 OK Completed\r\n"));

    // Write LOGOUT
    QCOMPARE((int) socket.write("2 LOGOUT\r\n", 10), 10);

    if (!socket.canReadLine())
        QVERIFY(socket.waitForReadyRead(5000));

    // Read two lines of respose
    s = socket.readLine();
    QCOMPARE(s, QByteArrayLiteral("* BYE LOGOUT received\r\n"));

    if (!socket.canReadLine())
        QVERIFY(socket.waitForReadyRead(5000));

    s = socket.readLine();
    QCOMPARE(s, QByteArrayLiteral("2 OK Completed\r\n"));

    // Close the socket
    socket.close();

    // Check that it's closed
    QCOMPARE(socket.state(), QTcpSocket::UnconnectedState);
}

//----------------------------------------------------------------------------------

void tst_QSocks5SocketEngine::tcpSocketNonBlockingTest()
{
    QSocks5SocketEngineHandler socks5;

    qint64 tcpSocketNonBlocking_totalWritten  = 0;
    QStringList tcpSocketNonBlocking_data;
    QTcpSocket socket;
    connect(&socket, &QAbstractSocket::hostFound,
            &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
    connect(&socket, &QAbstractSocket::connected,
            &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
    connect(&socket, &QIODevice::bytesWritten,
            [&tcpSocketNonBlocking_totalWritten] (qint64 written)
            {
                tcpSocketNonBlocking_totalWritten += written;
                QTestEventLoop::instance().exitLoop();
            });

    connect(&socket, &QIODevice::readyRead,
            [&tcpSocketNonBlocking_data, &socket] ()
            {
                while (socket.canReadLine())
                    tcpSocketNonBlocking_data.append(socket.readLine());
                QTestEventLoop::instance().exitLoop();
            });

    // Connect
    socket.connectToHost(QtNetworkSettings::serverName(), 143);
    QVERIFY(socket.state() == QTcpSocket::HostLookupState ||
            socket.state() == QTcpSocket::ConnectingState);

    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout()) {
        QFAIL("Timed out");
    }

    if (socket.state() == QTcpSocket::ConnectingState) {
        QTestEventLoop::instance().enterLoop(30);
        if (QTestEventLoop::instance().timeout()) {
            QFAIL("Timed out");
        }
    }

    QCOMPARE(socket.state(), QTcpSocket::ConnectedState);

    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout()) {
        QFAIL("Timed out");
    }

    // Read greeting
    QVERIFY(!tcpSocketNonBlocking_data.isEmpty());
    QByteArray data = tcpSocketNonBlocking_data.at(0).toLatin1();
    QVERIFY2(QtNetworkSettings::compareReplyIMAP(data), data.constData());

    tcpSocketNonBlocking_data.clear();

    tcpSocketNonBlocking_totalWritten = 0;

    // Write NOOP
    QCOMPARE((int) socket.write("1 NOOP\r\n", 8), 8);


    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout()) {
        QFAIL("Timed out");
    }

    QCOMPARE(tcpSocketNonBlocking_totalWritten, 8);


    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout()) {
        QFAIL("Timed out");
    }

    // Read response
    QVERIFY(!tcpSocketNonBlocking_data.isEmpty());
    QCOMPARE(tcpSocketNonBlocking_data.at(0), QLatin1String("1 OK Completed\r\n"));
    tcpSocketNonBlocking_data.clear();


    tcpSocketNonBlocking_totalWritten = 0;

    // Write LOGOUT
    QCOMPARE((int) socket.write("2 LOGOUT\r\n", 10), 10);

    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout()) {
        QFAIL("Timed out");
    }

    QCOMPARE(tcpSocketNonBlocking_totalWritten, 10);

    // Wait for greeting
    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout()) {
        QFAIL("Timed out");
    }

    // Read two lines of respose
    QCOMPARE(tcpSocketNonBlocking_data.at(0).toLatin1().constData(), "* BYE LOGOUT received\r\n");
    QCOMPARE(tcpSocketNonBlocking_data.at(1).toLatin1().constData(), "2 OK Completed\r\n");
    tcpSocketNonBlocking_data.clear();

    // Close the socket
    socket.close();

    // Check that it's closed
    QCOMPARE(socket.state(), QTcpSocket::UnconnectedState);
}

//----------------------------------------------------------------------------------

void tst_QSocks5SocketEngine::downloadBigFile()
{
    QSocks5SocketEngineHandler socks5;

    QTcpSocket socket;
    qint64 bytesAvailable = 0;

    QElapsedTimer stopWatch;
    stopWatch.start();

    connect(&socket, &QAbstractSocket::connected,
            &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
    connect(&socket, &QIODevice::readyRead,
            [&socket, &bytesAvailable] ()
            {
                const QByteArray tmp = socket.readAll();
                int correction = tmp.indexOf(char(0), 0); //skip header
                if (correction == -1)
                    correction = 0;
                bytesAvailable += (tmp.size() - correction);
                if (bytesAvailable >= 10000000)
                    QTestEventLoop::instance().exitLoop();
            });

    connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            [&socket, &stopWatch] (QAbstractSocket::SocketError errorCode)
            {
                qWarning().noquote().nospace() << QTest::currentTestFunction()
                    << ": error " << errorCode << ": " << socket.errorString()
                    << " (" << stopWatch.elapsed() << "ms)";
            });

    socket.connectToHost(QtNetworkSettings::serverName(), 80);

    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout())
        QFAIL("Network operation timed out");

    QByteArray hostName = QtNetworkSettings::serverName().toLatin1();
    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
    QVERIFY(socket.write("GET /qtest/mediumfile HTTP/1.0\r\n") > 0);
    QVERIFY(socket.write("HOST: ") > 0);
    QVERIFY(socket.write(hostName.data()) > 0);
    QVERIFY(socket.write("\r\n") > 0);
    QVERIFY(socket.write("\r\n") > 0);

    stopWatch.restart();
    QTestEventLoop::instance().enterLoop(60);
    if (QTestEventLoop::instance().timeout())
        QFAIL("Network operation timed out");

    QCOMPARE(bytesAvailable, qint64(10000000));

    QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

    /*qDebug("\t\t%.1fMB/%.1fs: %.1fMB/s",
           bytesAvailable / (1024.0 * 1024.0),
           stopWatch.elapsed() / 1024.0,
           (bytesAvailable / (stopWatch.elapsed() / 1000.0)) / (1024 * 1024));*/
}

void tst_QSocks5SocketEngine::passwordAuth()
{
    QSocks5SocketEngine socketDevice;

    // Initialize device
    QVERIFY(socketDevice.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QCOMPARE(socketDevice.state(), QAbstractSocket::UnconnectedState);

    socketDevice.setProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1080, "qsockstest", "password"));

    // Connect to imap.trolltech.com's IP
    QVERIFY(!socketDevice.connectToHost(QtNetworkSettings::serverIP(), 143));
    QCOMPARE(socketDevice.state(), QAbstractSocket::ConnectingState);
    QVERIFY(socketDevice.waitForWrite());
    if (!socketDevice.connectToHost(QtNetworkSettings::serverIP(), 143)) {
        qDebug("%d, %s", socketDevice.error(), socketDevice.errorString().toLatin1().constData());
    }
    QCOMPARE(socketDevice.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socketDevice.peerAddress(), QtNetworkSettings::serverIP());

    // Wait for the greeting
    QVERIFY(socketDevice.waitForRead());

    // Read the greeting
    qint64 available = socketDevice.bytesAvailable();
    QVERIFY(available > 0);
    QByteArray array;
    array.resize(available);
    QVERIFY(socketDevice.read(array.data(), array.size()) == available);

    // Check that the greeting is what we expect it to be
    QVERIFY2(QtNetworkSettings::compareReplyIMAP(array), array.constData());

    // Write a logout message
    QByteArray array2 = "XXXX LOGOUT\r\n";
    QVERIFY(socketDevice.write(array2.data(),
                              array2.size()) == array2.size());

    // Wait for the response
    QVERIFY(socketDevice.waitForRead());

    available = socketDevice.bytesAvailable();
    QVERIFY(available > 0);
    array.resize(available);
    QVERIFY(socketDevice.read(array.data(), array.size()) == available);

    // Check that the greeting is what we expect it to be
    QCOMPARE(array.constData(), "* BYE LOGOUT received\r\nXXXX OK Completed\r\n");

    // Wait for the response
    QVERIFY(socketDevice.waitForRead());
    char c;
    QVERIFY(socketDevice.read(&c, sizeof(c)) == -1);
    QCOMPARE(socketDevice.error(), QAbstractSocket::RemoteHostClosedError);
    QCOMPARE(socketDevice.state(), QAbstractSocket::UnconnectedState);
}

//----------------------------------------------------------------------------------

void tst_QSocks5SocketEngine::proxyAuthenticationRequired(const QNetworkProxy &,
                                                          QAuthenticator *auth)
{
    auth->setUser("qsockstest");
    auth->setPassword("password");
}

void tst_QSocks5SocketEngine::passwordAuth2()
{
    QSocks5SocketEngine socketDevice;

    // Initialize device
    QVERIFY(socketDevice.initialize(QAbstractSocket::TcpSocket, QAbstractSocket::IPv4Protocol));
    QCOMPARE(socketDevice.state(), QAbstractSocket::UnconnectedState);

    socketDevice.setProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, QtNetworkSettings::serverName(), 1081));
    socketDevice.setReceiver(this);

    QVERIFY(!socketDevice.connectToHost(QtNetworkSettings::serverIP(), 143));
    QCOMPARE(socketDevice.state(), QAbstractSocket::ConnectingState);
    while (socketDevice.state() == QAbstractSocket::ConnectingState) {
        QVERIFY(socketDevice.waitForWrite());
        socketDevice.connectToHost(QtNetworkSettings::serverIP(), 143);
    }
    if (socketDevice.state() != QAbstractSocket::ConnectedState)
        qDebug("%d, %s", socketDevice.error(), socketDevice.errorString().toLatin1().constData());
    QCOMPARE(socketDevice.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(socketDevice.peerAddress(), QtNetworkSettings::serverIP());

    // Wait for the greeting
    QVERIFY(socketDevice.waitForRead());

    // Read the greeting
    qint64 available = socketDevice.bytesAvailable();
    QVERIFY(available > 0);
    QByteArray array;
    array.resize(available);
    QVERIFY(socketDevice.read(array.data(), array.size()) == available);

    // Check that the greeting is what we expect it to be
    QVERIFY2(QtNetworkSettings::compareReplyIMAP(array), array.constData());

    // Write a logout message
    QByteArray array2 = "XXXX LOGOUT\r\n";
    QVERIFY(socketDevice.write(array2.data(),
                              array2.size()) == array2.size());

    // Wait for the response
    QVERIFY(socketDevice.waitForRead());

    available = socketDevice.bytesAvailable();
    QVERIFY(available > 0);
    array.resize(available);
    QVERIFY(socketDevice.read(array.data(), array.size()) == available);

    // Check that the greeting is what we expect it to be
    QCOMPARE(array.constData(), "* BYE LOGOUT received\r\nXXXX OK Completed\r\n");

    // Wait for the response
    QVERIFY(socketDevice.waitForRead());
    char c;
    QVERIFY(socketDevice.read(&c, sizeof(c)) == -1);
    QCOMPARE(socketDevice.error(), QAbstractSocket::RemoteHostClosedError);
    QCOMPARE(socketDevice.state(), QAbstractSocket::UnconnectedState);
}

void tst_QSocks5SocketEngine::fragmentation_data()
{
    QTest::addColumn<QQueue<QByteArray> >("responses");

    QByteArray authMethodNone = QByteArray::fromRawData("\5\0", 2);
    QByteArray authMethodBasic = QByteArray::fromRawData("\5\2", 2);
    QByteArray authSuccess = QByteArray::fromRawData("\1\0", 2);
    QByteArray connectResponseIPv4 = QByteArray::fromRawData("\5\0\0\1\1\2\3\4\5\6", 10);
    QByteArray connectResponseIPv6 = QByteArray::fromRawData("\5\0\0\4\x01\x23\x45\x67\x89\xab\xcd\xef\x01\x23\x45\x67\x89\xab\xcd\xef\5\6", 22);

    QQueue<QByteArray> responses;
    responses << authMethodNone.left(1) << authMethodNone.mid(1) << connectResponseIPv4;
    QTest::newRow("auth-method") << responses;

    responses.clear();
    responses << authMethodBasic << authSuccess.left(1) << authSuccess.mid(1) << connectResponseIPv4;
    QTest::newRow("auth-response") << responses;

    for (int i = 1; i < connectResponseIPv4.length() - 1; i++) {
        responses.clear();
        responses << authMethodNone << connectResponseIPv4.left(i) << connectResponseIPv4.mid(i);
        QTest::newRow(qPrintable(QString("connect-response-ipv4-") + QString::number(i))) << responses;
    }

    for (int i = 1; i < connectResponseIPv6.length() - 1; i++) {
        responses.clear();
        responses << authMethodNone << connectResponseIPv6.left(i) << connectResponseIPv6.mid(i);
        QTest::newRow(qPrintable(QString("connect-response-ipv6-") + QString::number(i))) << responses;
    }
}

void tst_QSocks5SocketEngine::fragmentation()
{
    QFETCH(QQueue<QByteArray>, responses);
    MiniSocks5Server server(responses, 500);

    QTcpSocket socket;
    socket.setProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, "localhost", server.serverPort(), "user", "password"));
    socket.connectToHost("0.1.2.3", 12345);

    connect(&socket, SIGNAL(connected()),
            &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QVERIFY(socket.localAddress() == QHostAddress("1.2.3.4") || socket.localAddress() == QHostAddress("0123:4567:89ab:cdef:0123:4567:89ab:cdef"));
    QCOMPARE(socket.localPort(), quint16(0x0506));
}

void tst_QSocks5SocketEngine::incomplete_data()
{
    QTest::addColumn<QQueue<QByteArray> >("responses");

    QByteArray authMethodNone = QByteArray::fromRawData("\5\0", 2);
    QByteArray authMethodBasic = QByteArray::fromRawData("\5\2", 2);
    QByteArray authSuccess = QByteArray::fromRawData("\1\0", 2);
    QByteArray connectResponseIPv4 = QByteArray::fromRawData("\5\0\0\1\1\2\3\4\5\6", 10);
    QByteArray connectResponseIPv6 = QByteArray::fromRawData("\5\0\0\4\x01\x23\x45\x67\x89\xab\xcd\xef\x01\x23\x45\x67\x89\xab\xcd\xef\5\6", 22);

    QQueue<QByteArray> responses;
    responses << authMethodNone.left(1);
    QTest::newRow("auth-method") << responses;

    responses.clear();
    responses << authMethodBasic << authSuccess.left(1);
    QTest::newRow("auth-response") << responses;

    for (int i = 1; i < connectResponseIPv4.length() - 1; i++) {
        responses.clear();
        responses << authMethodNone << connectResponseIPv4.left(i);
        QTest::newRow(qPrintable(QString("connect-response-ipv4-") + QString::number(i))) << responses;
    }

    for (int i = 1; i < connectResponseIPv6.length() - 1; i++) {
        responses.clear();
        responses << authMethodNone << connectResponseIPv6.left(i);
        QTest::newRow(qPrintable(QString("connect-response-ipv6-") + QString::number(i))) << responses;
    }
}

void tst_QSocks5SocketEngine::incomplete()
{
    QFETCH(QQueue<QByteArray>, responses);
    MiniSocks5Server server(responses, 500);

    QTcpSocket socket;
    socket.setProxy(QNetworkProxy(QNetworkProxy::Socks5Proxy, "127.0.0.1", server.serverPort(), "user", "password"));
    socket.connectToHost("0.1.2.3", 12345);

    connect(&socket, SIGNAL(connected()),
            &QTestEventLoop::instance(), SLOT(exitLoop()));
    connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(70);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(socket.error(), QAbstractSocket::ProxyConnectionClosedError);
}

//----------------------------------------------------------------------------------

QTEST_MAIN(tst_QSocks5SocketEngine)
#include "tst_qsocks5socketengine.moc"
