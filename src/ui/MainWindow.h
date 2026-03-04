#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPoint>
#include <QMouseEvent>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLineEdit>
#include "ChatWidget.h"
#include "../core/ConversationManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void toggleVisibility();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onInputSubmit();
    void onHistoryItemClicked(const QString &convId);
    void onBackToHistory();
    void updateHistoryList();

private:
    void applyWindowStyle();
    QWidget* createHistoryItem(const QString &title, const QString &convId);
    void showChatView(Conversation *conv);
    QPixmap createModelIcon(int size);
    void navigateHistory(int direction);

    enum ViewMode { History, Chat };
    ViewMode m_viewMode = History;

    QWidget *m_headerWidget;
    QLineEdit *m_input;
    
    QScrollArea *m_scrollArea;
    QWidget *m_historyWidget;
    QVBoxLayout *m_historyLayout;

    ChatWidget *m_chatWidget;
    QPoint m_dragPosition;
    int m_focusIndex = 0; // 0 for input, 1-5 for history items
};

#endif // MAINWINDOW_H
