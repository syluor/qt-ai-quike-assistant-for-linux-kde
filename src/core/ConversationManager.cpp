#include "ConversationManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ConversationManager& ConversationManager::instance() {
    static ConversationManager inst;
    return inst;
}

ConversationManager::ConversationManager(QObject *parent) : QObject(parent) {
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if(dataDir.isEmpty()) {
        dataDir = QDir::homePath() + "/.local/share/qt-ai-assistant";
    }
    
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    m_filePath = dir.filePath("conversations.json");
    load();
}

void ConversationManager::load() {
    QFile file(m_filePath);
    if (!file.exists()) return;

    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray arr = doc.array();
        m_conversations.clear();
        for (const QJsonValue &v : arr) {
            m_conversations.append(Conversation::fromJson(v.toObject()));
        }
    }
}

void ConversationManager::save() {
    QJsonArray arr;
    for (const auto &c : m_conversations) {
        arr.append(c.toJson());
    }
    
    QFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(arr);
        file.write(doc.toJson());
    }
}

QList<Conversation>& ConversationManager::conversations() {
    return m_conversations;
}

Conversation* ConversationManager::createConversation(const QString &firstMessage, const QString &modelId) {
    // Determine title from first message (truncate if too long)
    QString title = firstMessage;
    if (title.length() > 20) {
        title = title.left(20) + "...";
    }

    Conversation conv(title);
    conv.modelId = modelId;
    m_conversations.prepend(conv); // Add to front
    save();
    
    emit conversationAdded(&m_conversations.first());
    return &m_conversations.first();
}

Conversation* ConversationManager::getConversation(const QString &id) {
    for (auto &c : m_conversations) {
        if (c.id == id) {
            return &c;
        }
    }
    return nullptr;
}

void ConversationManager::deleteConversation(const QString &id) {
    for (int i = 0; i < m_conversations.size(); ++i) {
        if (m_conversations[i].id == id) {
            m_conversations.removeAt(i);
            save();
            emit conversationDeleted(id);
            break;
        }
    }
}
