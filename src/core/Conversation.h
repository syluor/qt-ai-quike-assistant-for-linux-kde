#ifndef CONVERSATION_H
#define CONVERSATION_H

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QUuid>

struct ChatMessage {
    QString role; // "user", "assistant", "system", "error"
    QString content;

    static ChatMessage fromJson(const QJsonObject &json);
    QJsonObject toJson() const;
};

class Conversation {
public:
    Conversation(const QString& title = "新对话");
    
    QString id;
    QString title;
    QString modelId;
    QList<ChatMessage> messages;

    void addMessage(const ChatMessage& msg);
    
    static Conversation fromJson(const QJsonObject &json);
    QJsonObject toJson() const;
};

#endif // CONVERSATION_H
