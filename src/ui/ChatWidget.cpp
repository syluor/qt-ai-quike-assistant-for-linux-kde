#include "ChatWidget.h"
#include <QRegularExpression>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTimer>
#include <QPainter>
#include "../core/ConfigManager.h"

QPixmap ChatWidget::createModelIcon(int size) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#e7ecff"));
    p.drawEllipse(0, 0, size, size);

    QPolygonF poly;
    poly << QPointF(size * 0.50, size * 0.16)
         << QPointF(size * 0.80, size * 0.38)
         << QPointF(size * 0.70, size * 0.77)
         << QPointF(size * 0.30, size * 0.77)
         << QPointF(size * 0.20, size * 0.38);
    p.setBrush(QColor("#4d78ff"));
    p.drawPolygon(poly);

    return pixmap;
}

ChatWidget::ChatWidget(QWidget *parent) : QWidget(parent) {
    m_llmClient = new LlmClient(this);

    setStyleSheet("QWidget { background: #1f2227; color: #edf1fa; font-size: 15px; }");

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(10);

    // Top Header for current conversation
    QWidget *topContainer = new QWidget(this);
    QVBoxLayout *topLayout = new QVBoxLayout(topContainer);
    topLayout->setContentsMargins(14, 14, 14, 0);
    topLayout->setSpacing(10);
    
    QHBoxLayout *topRow = new QHBoxLayout;
    topRow->setSpacing(10);

    QLabel *icon = new QLabel(topContainer);
    icon->setPixmap(createModelIcon(34));
    icon->setFixedSize(34, 34);

    m_input = new QLineEdit(topContainer);
    m_input->setPlaceholderText(QStringLiteral("问问 助手 获取帮助... (ESC取消/返回)"));
    m_input->setStyleSheet(
        "QLineEdit {"
        "  background: transparent;"
        "  border: none;"
        "  padding: 8px 2px;"
        "  color: #e6ebf7;"
        "}"
        "QLineEdit:focus { border: none; }"
    );

    topRow->addWidget(icon);
    topRow->addWidget(m_input, 1);
    topLayout->addLayout(topRow);

    QFrame *separator = new QFrame(topContainer);
    separator->setFrameShape(QFrame::HLine);
    separator->setFixedHeight(1);
    separator->setStyleSheet("QFrame { border: none; background: #1b1e22; }");
    topLayout->addWidget(separator);

    root->addWidget(topContainer);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: #1f2227; }"
        "QScrollBar:vertical { width: 4px; background: transparent; margin: 0; }"
        "QScrollBar::handle:vertical { background: #3e4451; border-radius: 2px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
    );

    m_messagesWidget = new QWidget(m_scrollArea);
    m_messagesWidget->setStyleSheet("QWidget { background: #1f2227; }");
    m_messagesLayout = new QVBoxLayout(m_messagesWidget);
    m_messagesLayout->setContentsMargins(14, 6, 14, 14);
    m_messagesLayout->setSpacing(10);
    m_messagesLayout->setAlignment(Qt::AlignTop);
    
    m_scrollArea->setWidget(m_messagesWidget);
    root->addWidget(m_scrollArea, 1);

    connect(m_input, &QLineEdit::returnPressed, this, &ChatWidget::onSubmit);
    connect(m_llmClient, &LlmClient::chunkReceived, this, &ChatWidget::onChunkReceived);
    connect(m_llmClient, &LlmClient::replyFinished, this, &ChatWidget::onReplyFinished);
    connect(m_llmClient, &LlmClient::errorOccurred, this, &ChatWidget::onErrorOccurred);

    m_input->installEventFilter(this);
    installEventFilter(this);
    m_scrollArea->viewport()->installEventFilter(this);
}

ChatWidget::~ChatWidget() {}

void ChatWidget::setConversation(Conversation *conv) {
    m_conversation = conv;
    m_isGenerating = false;
    m_currentAssistantBubble = nullptr;

    while (QLayoutItem *item = m_messagesLayout->takeAt(0)) {
        if (QWidget *w = item->widget()) w->deleteLater();
        delete item;
    }

    if (conv) {
        for (const auto &msg : conv->messages) {
            addMessageToUi(msg.content, msg.role == "user", msg.role == "error");
        }
    }
    QTimer::singleShot(50, this, &ChatWidget::scrollToBottom);
}

void ChatWidget::focusInput() {
    m_input->setFocus();
}

void ChatWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_isGenerating) {
            m_llmClient->stop();
            m_isGenerating = false;
            m_conversation->addMessage({"error", "对话已停止"});
            ConversationManager::instance().save();
            addMessageToUi("对话已停止", false, true);
        } else {
            emit backRequested();
        }
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool ChatWidget::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_input && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            keyPressEvent(keyEvent);
            return true;
        }
    }

    if (event->type() == QEvent::Resize) {
        updateBubbleWidths();
    }
    return QWidget::eventFilter(obj, event);
}

void ChatWidget::sendInitialMessage(const QString& text) {
    if (m_isGenerating || text.isEmpty()) return;
    
    addMessageToUi(text, true);

    if (m_conversation) {
        m_conversation->addMessage({"user", text});
        ConversationManager::instance().save();
    }
    
    emit messageSent();

    m_isGenerating = true;
    m_currentAssistantBubble = nullptr;

    auto cfg = ConfigManager::instance().currentModel();
    if (m_conversation) {
        // Add thinking status
        m_thinkingWidget = createBubble("AI 正在思考中...", false, false);
        m_thinkingWidget->findChild<QLabel*>()->setStyleSheet("QLabel { background: transparent; color: #666; font-size: 13px; font-style: italic; padding: 2px 2px; }");
        m_messagesLayout->addWidget(m_thinkingWidget);
        scrollToBottom();

        QList<ChatMessage> fullContext = m_conversation->messages;
        m_llmClient->sendMessage(fullContext, cfg);
    }
}

void ChatWidget::onSubmit() {
    QString text = m_input->text().trimmed();
    if (text.isEmpty() || m_isGenerating) return;

    m_input->clear();
    sendInitialMessage(text);
}

void ChatWidget::onChunkReceived(const QString &chunk) {
    if (m_thinkingWidget) {
        m_thinkingWidget->deleteLater();
        m_thinkingWidget = nullptr;
    }

    if (!m_currentAssistantBubble) {
        QWidget *container = createBubble("", false, false);
        m_currentAssistantBubble = container->findChild<QLabel*>();
        m_messagesLayout->addWidget(container);
    }

    QString current = m_currentAssistantBubble->property("rawMarkdown").toString();
    current += chunk;
    m_currentAssistantBubble->setProperty("rawMarkdown", current);

    QString displayMarkdown = current;
    displayMarkdown.replace(QRegularExpression("(?m)^#{1,3} "), "#### ");
    m_currentAssistantBubble->setText(displayMarkdown);

    updateBubbleWidths();
    scrollToBottom();
}

void ChatWidget::onReplyFinished(const QString &fullText) {
    if (!m_isGenerating) return; // aborted
    m_isGenerating = false;
    if (m_conversation) {
        m_conversation->addMessage({"assistant", fullText});
        ConversationManager::instance().save();
    }
    m_currentAssistantBubble = nullptr;
}

void ChatWidget::onErrorOccurred(const QString &errorMsg) {
    if (m_thinkingWidget) {
        m_thinkingWidget->deleteLater();
        m_thinkingWidget = nullptr;
    }
    if (!m_isGenerating) return; // already handled abort
    m_isGenerating = false;
    addMessageToUi(errorMsg, false, true);
    if (m_conversation) {
        m_conversation->addMessage({"error", errorMsg});
        ConversationManager::instance().save();
    }
    m_currentAssistantBubble = nullptr;
}

void ChatWidget::addMessageToUi(const QString &text, bool isUser, bool isError) {
    QWidget *bubble = createBubble(text, isUser, isError);
    m_messagesLayout->addWidget(bubble);
    updateBubbleWidths();
    // Use slightly longer delay to ensure layout is updated
    QTimer::singleShot(50, this, &ChatWidget::scrollToBottom);
}

QWidget* ChatWidget::createBubble(const QString &text, bool isUser, bool isError) {
    QWidget *container = new QWidget(m_messagesWidget);
    QHBoxLayout *h = new QHBoxLayout(container);
    h->setContentsMargins(0, 0, 0, 0);

    QLabel *label = new QLabel(container);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByMouse);
    label->setOpenExternalLinks(true);
    
    bool isSpecial = isError || (text == "对话已停止") || text.startsWith("错误:");

    if (!isUser) {
        label->setTextFormat(Qt::MarkdownText);
    }
    
    if (!isUser && !isSpecial) {
        label->setProperty("rawMarkdown", text);
        QString displayMarkdown = text;
        displayMarkdown.replace(QRegularExpression("(?m)^#{1,3} "), "#### ");
        label->setText(displayMarkdown);
    } else {
        label->setText(text);
    }
    
    if (isUser) {
        label->setStyleSheet("QLabel { background: #2a2e36; color: #edf1fa; font-size: 14px; padding: 11px 14px; border-radius: 14px; }");
        h->addStretch(1);
        h->addWidget(label, 0, Qt::AlignRight);
    } else {
        if (isSpecial) {
            label->setStyleSheet("QLabel { background: transparent; color: #ff4d4d; font-size: 11px; font-style: italic; padding: 2px 2px; }");
        } else {
            label->setStyleSheet("QLabel { background: transparent; color: #eef2fb; font-size: 13px; padding: 2px 2px; }");
        }
        h->addWidget(label, 0, Qt::AlignLeft);
        h->addStretch(1);
    }

    return container;
}

void ChatWidget::updateBubbleWidths() {
    if (!m_scrollArea) return;
    int bubbleWidth = qMax(80, m_scrollArea->viewport()->width() - 30);
    for (int i = 0; i < m_messagesLayout->count(); ++i) {
        if (QWidget* bubbleContainer = m_messagesLayout->itemAt(i)->widget()) {
            if (QLabel* lbl = bubbleContainer->findChild<QLabel*>()) {
                lbl->setMaximumWidth(bubbleWidth);
            }
        }
    }
}

void ChatWidget::scrollToBottom() {
    QScrollBar *bar = m_scrollArea->verticalScrollBar();
    // Connect rangeChanged to setValue(maximum) once to handle layout reflow
    connect(bar, &QScrollBar::rangeChanged, this, [bar, this]() {
        bar->setValue(bar->maximum());
        disconnect(bar, &QScrollBar::rangeChanged, this, nullptr);
    });
    // Also scroll immediately to the current known bottom
    bar->setValue(bar->maximum());
}
