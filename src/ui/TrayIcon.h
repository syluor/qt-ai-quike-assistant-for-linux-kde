#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include "MainWindow.h"

class TrayIcon : public QObject {
    Q_OBJECT
public:
    explicit TrayIcon(MainWindow *mainWindow, QObject *parent = nullptr);
    ~TrayIcon();

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_toggleAction;
    QAction *m_quitAction;
    MainWindow *m_mainWindow;
};

#endif // TRAYICON_H
