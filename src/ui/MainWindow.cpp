#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPainter>
#include <QSizeGrip>
#include <QWindow>
#include <QScrollBar>
#include "../core/ConfigManager.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    applyWindowStyle();

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *root = new QVBoxLayout(centralWidget);
    root->setContentsMargins(0, 0, 0, 0); // No margins at window edge
    root->setSpacing(0);

    // Content container with margins
    QWidget *content = new QWidget(centralWidget);
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(14, 14, 14, 0); // Bottom 0 for grip
    contentLayout->setSpacing(10);

    // Header Widget (Model Icon + Input)
    m_headerWidget = new QWidget(this);
    QVBoxLayout *headerLayout = new QVBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(10);

    QHBoxLayout *topRow = new QHBoxLayout;
    topRow->setSpacing(10);

    QLabel *icon = new QLabel(m_headerWidget);
    icon->setPixmap(createModelIcon(34));
    icon->setFixedSize(34, 34);

    m_input = new QLineEdit(m_headerWidget);
    m_input->setPlaceholderText(QStringLiteral("输入消息开始新对话..."));
    m_input->setStyleSheet(
        "QLineEdit {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 0px;"
        "  padding: 8px 2px;"
        "  color: #e6ebf7;"
        "}"
        "QLineEdit:focus { border: none; }");

    topRow->addWidget(icon);
    topRow->addWidget(m_input, 1);
    headerLayout->addLayout(topRow);

    QFrame *separator = new QFrame(m_headerWidget);
    separator->setFrameShape(QFrame::HLine);
    separator->setFixedHeight(1);
    separator->setStyleSheet("QFrame { border: none; background: #1b1e22; }");
    headerLayout->addWidget(separator);

    contentLayout->addWidget(m_headerWidget);

    // History View
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(
        "QScrollArea { border: none; background: #1f2227; }"
        "QScrollBar:vertical { width: 4px; background: transparent; margin: 0; }"
        "QScrollBar::handle:vertical { background: #3e4451; border-radius: 2px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");

    m_historyWidget = new QWidget(m_scrollArea);
    m_historyWidget->setStyleSheet("QWidget { background: #1f2227; }");
    m_historyLayout = new QVBoxLayout(m_historyWidget);
    m_historyLayout->setContentsMargins(2, 6, 2, 6);
    m_historyLayout->setSpacing(8);
    m_historyLayout->setAlignment(Qt::AlignTop);
    m_scrollArea->setWidget(m_historyWidget);

    contentLayout->addWidget(m_scrollArea, 1);

    // Chat View
    m_chatWidget = new ChatWidget(this);
    m_chatWidget->hide();
    contentLayout->addWidget(m_chatWidget, 1);

    root->addWidget(content, 1);

    // Size Grip (from old project logic)
    QHBoxLayout *bottomRow = new QHBoxLayout;
    bottomRow->setContentsMargins(0, 0, 0, 0);
    bottomRow->addStretch(1);
    QSizeGrip *sizeGrip = new QSizeGrip(this);
    sizeGrip->setFixedSize(16, 16);
    bottomRow->addWidget(sizeGrip, 0, Qt::AlignRight | Qt::AlignBottom);
    root->addLayout(bottomRow);

    setCentralWidget(centralWidget);

    connect(m_input, &QLineEdit::returnPressed, this, &MainWindow::onInputSubmit);
    connect(m_chatWidget, &ChatWidget::backRequested, this, &MainWindow::onBackToHistory);
    
    auto& cm = ConversationManager::instance();
    connect(&cm, &ConversationManager::conversationAdded, this, [this](Conversation*){ updateHistoryList(); });
    connect(&cm, &ConversationManager::conversationUpdated, this, [this](Conversation*){ updateHistoryList(); });
    connect(&cm, &ConversationManager::conversationDeleted, this, [this](const QString&){ updateHistoryList(); });

    // Inactivity timer: return to history view after 3 minutes
    m_inactivityTimer = new QTimer(this);
    m_inactivityTimer->setSingleShot(true);
    m_inactivityTimer->setInterval(3 * 60 * 1000); // 3 minutes
    connect(m_inactivityTimer, &QTimer::timeout, this, &MainWindow::onBackToHistory);

    updateHistoryList();
    m_input->installEventFilter(this);
    m_historyWidget->installEventFilter(this);
    installEventFilter(this);
}

MainWindow::~MainWindow() {}

void MainWindow::applyWindowStyle() {
    setWindowIcon(QIcon::fromTheme("io.github.ader.QtAiAssistant"));
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint);
    setAttribute(Qt::WA_X11NetWmWindowTypeUtility, true);
    setStyleSheet("QWidget { background: #1f2227; color: #edf1fa; font-size: 15px; border-radius: 10px; }"); // Dark mode global
    setMinimumSize(360, 260);
    resize(520, 560);
}

QPixmap MainWindow::createModelIcon(int size) {
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

void MainWindow::toggleVisibility() {
    if (isVisible() && !isMinimized() && isActiveWindow()) {
        hide();
        m_inactivityTimer->stop();
    } else {
        showNormal();
        raise();
        activateWindow();
        m_inactivityTimer->start(); // restart 3-min countdown on each show
        if (m_viewMode == History) {
            m_input->setFocus();
        } else {
            m_chatWidget->focusInput();
        }
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    QWidget::mouseReleaseEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QWidget *widget = qobject_cast<QWidget*>(watched);
            
            // Handle history item click
            if (widget && widget->property("convId").isValid()) {
                onHistoryItemClicked(widget->property("convId").toString());
                return true;
            }

            // Drag support (non-input areas)
            if (watched != m_input && watched != m_chatWidget) {
                if (windowHandle() && windowHandle()->startSystemMove()) {
                    return true;
                }
            }
            
            // Fallback to manual drag if startSystemMove not available/fails
            m_dragPosition = mouseEvent->globalPosition().toPoint() - frameGeometry().topLeft();
        }
    }
    

    
    if (m_viewMode == History && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Up) {
            navigateHistory(-1);
            return true;
        } else if (ke->key() == Qt::Key_Down) {
            navigateHistory(1);
            return true;
        } else if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            if (m_focusIndex > 0) {
                int count = 0;
                for (int i = 0; i < m_historyLayout->count(); ++i) {
                    QWidget* w = m_historyLayout->itemAt(i)->widget();
                    if (w && w->property("convId").isValid()) {
                        count++;
                        if (count == m_focusIndex) {
                            onHistoryItemClicked(w->property("convId").toString());
                            return true;
                        }
                    }
                }
            }
        } else if (ke->key() == Qt::Key_Escape) {
            hide();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::navigateHistory(int direction) {
    int countValid = 0;
    for (int i = 0; i < m_historyLayout->count(); ++i) {
        QWidget* w = m_historyLayout->itemAt(i)->widget();
        if (w && w->property("convId").isValid()) countValid++;
    }
    int maxItems = qMin(5, countValid);
    if (maxItems < 1) return;

    m_focusIndex += direction;
    if (m_focusIndex < 0) m_focusIndex = maxItems;
    if (m_focusIndex > maxItems) m_focusIndex = 0;

    if (m_focusIndex == 0) {
        m_input->setFocus();
        for (int j = 0; j < m_historyLayout->count(); ++j) {
             if (QWidget* w2 = m_historyLayout->itemAt(j)->widget())
                 w2->setStyleSheet("QWidget { background: #2a2e36; border-radius: 8px; } QWidget:hover { background: #323740; }");
        }
    } else {
        // Find the N-th history item widget
        int count = 0;
        for (int i = 0; i < m_historyLayout->count(); ++i) {
            QWidget* w = m_historyLayout->itemAt(i)->widget();
            if (w && w->property("convId").isValid()) {
                count++;
                if (count == m_focusIndex) {
                    // Visual feedback
                    for (int j = 0; j < m_historyLayout->count(); ++j) {
                         if (QWidget* w2 = m_historyLayout->itemAt(j)->widget())
                             w2->setStyleSheet("QWidget { background: #2a2e36; border-radius: 8px; } QWidget:hover { background: #323740; }");
                    }
                    w->setStyleSheet("QWidget { background: #4d78ff; border-radius: 8px; }");
                    m_input->clearFocus();
                    break;
                }
            }
        }
    }
}

void MainWindow::onInputSubmit() {
    m_focusIndex = 0; // Reset focus index on submit
    QString text = m_input->text().trimmed();
    if (text.isEmpty()) return;
    
    m_input->clear();
    QString currentModel = ConfigManager::instance().currentModelId();
    if (currentModel.isEmpty()) currentModel = "default";

    Conversation *conv = ConversationManager::instance().createConversation(text, currentModel);
    showChatView(conv);
    
    m_chatWidget->sendInitialMessage(text);
}

void MainWindow::onHistoryItemClicked(const QString &convId) {
    Conversation *conv = ConversationManager::instance().getConversation(convId);
    if (conv) {
        showChatView(conv);
    }
}

void MainWindow::showChatView(Conversation *conv) {
    m_viewMode = Chat;
    m_headerWidget->hide();
    m_scrollArea->hide();
    m_chatWidget->setConversation(conv);
    m_chatWidget->show();
    m_chatWidget->focusInput();
}

void MainWindow::onBackToHistory() {
    m_inactivityTimer->stop();
    m_viewMode = History;
    m_chatWidget->hide();
    m_headerWidget->show();
    m_scrollArea->show();
    updateHistoryList();
    m_input->setFocus();
}

void MainWindow::updateHistoryList() {
    m_focusIndex = 0;
    while (QLayoutItem *item = m_historyLayout->takeAt(0)) {
        if (QWidget *w = item->widget()) w->deleteLater();
        delete item;
    }

    auto &convs = ConversationManager::instance().conversations();

    if (convs.isEmpty()) {
        QLabel *emptyLabel = new QLabel("暂无对话历史");
        emptyLabel->setStyleSheet("QLabel { color: #666; padding: 20px; }");
        emptyLabel->setAlignment(Qt::AlignCenter);
        m_historyLayout->addWidget(emptyLabel);
    } else {
        int count = 0;
        for (const auto &conv : convs) {
            if (count >= 5) break; 
            m_historyLayout->addWidget(createHistoryItem(conv.title, conv.id));
            count++;
        }
    }
    m_historyLayout->addStretch();
}

QWidget* MainWindow::createHistoryItem(const QString &title, const QString &convId) {
    QWidget *widget = new QWidget(m_historyWidget);
    widget->setStyleSheet("QWidget { background: #2a2e36; border-radius: 8px; } QWidget:hover { background: #323740; }");
    widget->setProperty("convId", convId);
    widget->setCursor(Qt::PointingHandCursor);

    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(12, 12, 12, 12);

    QLabel *label = new QLabel(title);
    label->setStyleSheet("QLabel { color: #edf1fa; font-size: 14px; background: transparent; }");
    layout->addWidget(label, 1);

    widget->installEventFilter(this);
    return widget;
}
