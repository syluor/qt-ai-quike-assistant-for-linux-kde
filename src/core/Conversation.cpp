#include "Conversation.h"
#include <QJsonArray>

ChatMessage ChatMessage::fromJson(const QJsonObject &json) {
    ChatMessage msg;
    msg.role = json.value("role").toString();
    msg.content = json.value("content").toString();
    return msg;
}

QJsonObject ChatMessage::toJson() const {
    QJsonObject json;
    json["role"] = role;
    json["content"] = content;
    return json;
}

Conversation::Conversation(const QString& title) 
    : id(QUuid::createUuid().toString(QUuid::WithoutBraces)), title(title) 
{
}

void Conversation::addMessage(const ChatMessage& msg) {
    messages.append(msg);
}

Conversation Conversation::fromJson(const QJsonObject &json) {
    Conversation conv;
    conv.id = json.value("id").toString();
    conv.title = json.value("title").toString();
    conv.modelId = json.value("modelId").toString();
    
    QJsonArray msgArr = json.value("messages").toArray();
    for (const QJsonValue& val : msgArr) {
        conv.messages.append(ChatMessage::fromJson(val.toObject()));
    }
    return conv;
}

QJsonObject Conversation::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["title"] = title;
    json["modelId"] = modelId;
    
    QJsonArray msgArr;
    for (const auto& m : messages) {
        msgArr.append(m.toJson());
    }
    json["messages"] = msgArr;
    return json;
}
