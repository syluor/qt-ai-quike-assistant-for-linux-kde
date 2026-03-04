#include "TrayIcon.h"
#include <QApplication>
#include <QIcon>
#include <QStyle>

TrayIcon::TrayIcon(MainWindow *mainWindow, QObject *parent)
    : QObject(parent), m_mainWindow(mainWindow) {
    
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
    m_trayIcon->setToolTip("Qt AI Assistant");

    m_trayMenu = new QMenu();
    m_toggleAction = new QAction("显示/隐藏", this);
    m_quitAction = new QAction("退出", this);

    m_trayMenu->addAction(m_toggleAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_quitAction);

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_toggleAction, &QAction::triggered, m_mainWindow, &MainWindow::toggleVisibility);
    connect(m_quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayIcon::onTrayIconActivated);

    m_trayIcon->show();
}

TrayIcon::~TrayIcon() {
    m_trayIcon->hide();
}

void TrayIcon::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        m_mainWindow->toggleVisibility();
    }
}
