#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>

#include "ConfigManager.h"
#include "Conversation.h"

class LlmClient : public QObject {
    Q_OBJECT
public:
    explicit LlmClient(QObject *parent = nullptr);
    ~LlmClient();

    void sendMessage(const QList<ChatMessage>& history, const ModelConfig& config);
    void stop();

signals:
    void chunkReceived(const QString& chunk);
    void replyFinished(const QString& fullText);
    void errorOccurred(const QString& errorMsg);

private slots:
    void onReadyRead();
    void onFinished();
    void onErrorOccurred(QNetworkReply::NetworkError code);

private:
    QNetworkAccessManager m_networkManager;
    QNetworkReply *m_currentReply = nullptr;
    QString m_fullResponse;
    QByteArray m_buffer;
};

#endif // LLMCLIENT_H
