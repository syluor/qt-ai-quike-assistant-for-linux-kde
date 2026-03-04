#include "LlmClient.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

LlmClient::LlmClient(QObject *parent) : QObject(parent) {
}

LlmClient::~LlmClient() {
    stop();
}

void LlmClient::sendMessage(const QList<ChatMessage>& history, const ModelConfig& config) {
    stop(); // stop any ongoing request
    
    m_fullResponse.clear();
    m_buffer.clear();

    QNetworkRequest request(QUrl(config.apiUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + config.apiKey).toUtf8());
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);

    QJsonObject root;
    root["model"] = config.modelId;
    root["stream"] = true;

    for (auto it = config.extraParams.constBegin(); it != config.extraParams.constEnd(); ++it) {
        root[it.key()] = QJsonValue::fromVariant(it.value());
    }

    QJsonArray messages;
    if (!config.systemPrompt.isEmpty()) {
        QJsonObject sys;
        sys["role"] = "system";
        sys["content"] = config.systemPrompt;
        messages.append(sys);
    }
    
    for (const auto& msg : history) {
        if (msg.role != "error" && msg.role != "system") { // Only send user and assistant valid messages
            QJsonObject m;
            m["role"] = msg.role;
            m["content"] = msg.content;
            messages.append(m);
        }
    }
    root["messages"] = messages;

    QJsonDocument doc(root);
    m_currentReply = m_networkManager.post(request, doc.toJson());

    connect(m_currentReply, &QNetworkReply::readyRead, this, &LlmClient::onReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &LlmClient::onFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, &LlmClient::onErrorOccurred);
}

void LlmClient::stop() {
    if (m_currentReply) {
        disconnect(m_currentReply, nullptr, this, nullptr); // Prevent callbacks
        if (m_currentReply->isRunning()) {
            m_currentReply->abort();
        }
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void LlmClient::onReadyRead() {
    if (!m_currentReply) return;
    
    m_buffer.append(m_currentReply->readAll());
    
    int newlineIndex;
    while ((newlineIndex = m_buffer.indexOf('\n')) != -1) {
        QByteArray line = m_buffer.left(newlineIndex).trimmed();
        m_buffer.remove(0, newlineIndex + 1);

        if (line.startsWith("data: ")) {
            QByteArray data = line.mid(6);
            if (data == "[DONE]") continue;
            
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(data, &err);
            if (err.error == QJsonParseError::NoError) {
                QJsonObject root = doc.object();
                QJsonArray choices = root.value("choices").toArray();
                if (!choices.isEmpty()) {
                    QJsonObject delta = choices.first().toObject().value("delta").toObject();
                    if (delta.contains("content")) {
                        QString content = delta.value("content").toString();
                        m_fullResponse += content;
                        emit chunkReceived(content);
                    }
                }
            }
        }
    }
}

void LlmClient::onFinished() {
    if (!m_currentReply) return;
    
    if (m_currentReply->error() == QNetworkReply::NoError) {
        emit replyFinished(m_fullResponse);
    }
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

void LlmClient::onErrorOccurred(QNetworkReply::NetworkError code) {
    if (!m_currentReply) return;
    
    if (code == QNetworkReply::OperationCanceledError) {
        // Aborted by user
        emit errorOccurred("对话已停止");
    } else {
        emit errorOccurred(QString("网络错误: %1").arg(m_currentReply->errorString()));
    }
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}
