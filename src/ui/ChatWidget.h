#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QLabel>
#include "../core/ConversationManager.h"
#include "../core/LlmClient.h"

class ChatWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget();

    void setConversation(Conversation *conv);
    void focusInput();
    void sendInitialMessage(const QString& text);

signals:
    void backRequested();
    void messageSent();

private slots:
    void onSubmit();
    void onChunkReceived(const QString &chunk);
    void onReplyFinished(const QString &fullText);
    void onErrorOccurred(const QString &errorMsg);
    void scrollToBottom();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void addMessageToUi(const QString &text, bool isUser, bool isError = false);
    QWidget* createBubble(const QString &text, bool isUser, bool isError = false);
    void updateBubbleWidths();
    QPixmap createModelIcon(int size);

    QScrollArea *m_scrollArea;
    QWidget *m_messagesWidget;
    QVBoxLayout *m_messagesLayout;
    
    QLineEdit *m_input;
    
    LlmClient *m_llmClient;
    Conversation *m_conversation = nullptr;

    bool m_isGenerating = false;
    QLabel* m_currentAssistantBubble = nullptr;
    QWidget* m_thinkingWidget = nullptr;
};

#endif // CHATWIDGET_H
