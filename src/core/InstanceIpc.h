#ifndef INSTANCEIPC_H
#define INSTANCEIPC_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>

class InstanceIpc : public QObject {
    Q_OBJECT
public:
    explicit InstanceIpc(const QString &serverName, QObject *parent = nullptr);
    bool startServer();
    static bool sendMessage(const QString &serverName, const QString &message);

signals:
    void messageReceived(const QString &message);

private slots:
    void onNewConnection();
    void onReadyRead();

private:
    QString m_serverName;
    QLocalServer *m_server;
};

#endif // INSTANCEIPC_H
