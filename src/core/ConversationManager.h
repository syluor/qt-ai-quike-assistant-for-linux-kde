#ifndef CONVERSATIONMANAGER_H
#define CONVERSATIONMANAGER_H

#include <QObject>
#include <QList>
#include "Conversation.h"

class ConversationManager : public QObject {
    Q_OBJECT
public:
    static ConversationManager& instance();

    void load();
    void save();

    QList<Conversation>& conversations();
    Conversation* createConversation(const QString &firstMessage, const QString &modelId);
    Conversation* getConversation(const QString &id);
    void deleteConversation(const QString &id);

signals:
    void conversationAdded(Conversation* conv);
    void conversationUpdated(Conversation* conv);
    void conversationDeleted(const QString &id);

private:
    explicit ConversationManager(QObject *parent = nullptr);
    ~ConversationManager() = default;
    Q_DISABLE_COPY(ConversationManager)

    QString m_filePath;
    QList<Conversation> m_conversations;
};

#endif // CONVERSATIONMANAGER_H
