#include "InstanceIpc.h"
#include <QLocalSocket>

InstanceIpc::InstanceIpc(const QString &serverName, QObject *parent)
    : QObject(parent), m_serverName(serverName), m_server(new QLocalServer(this)) {
}

bool InstanceIpc::startServer() {
    // Try to listen first
    if (m_server->listen(m_serverName)) {
        connect(m_server, &QLocalServer::newConnection, this, &InstanceIpc::onNewConnection);
        return true;
    }

    // Check if another instance is really running
    QLocalSocket probe;
    probe.connectToServer(m_serverName);
    if (probe.waitForConnected(100)) {
        probe.disconnectFromServer();
        return false; // Another instance is definitely running
    }

    // If we're here, it's a stale socket file. Remove and try again.
    QLocalServer::removeServer(m_serverName);
    if (m_server->listen(m_serverName)) {
        connect(m_server, &QLocalServer::newConnection, this, &InstanceIpc::onNewConnection);
        return true;
    }
    return false;
}

bool InstanceIpc::sendMessage(const QString &serverName, const QString &message) {
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        socket.write(message.toUtf8());
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
        return true;
    }
    return false;
}

void InstanceIpc::onNewConnection() {
    QLocalSocket *socket = m_server->nextPendingConnection();
    if (socket) {
        connect(socket, &QLocalSocket::readyRead, this, &InstanceIpc::onReadyRead);
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
    }
}

void InstanceIpc::onReadyRead() {
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (socket) {
        QString message = QString::fromUtf8(socket->readAll());
        emit messageReceived(message);
    }
}
