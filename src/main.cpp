#include <QApplication>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <QDir>
#include "ui/MainWindow.h"
#include "ui/TrayIcon.h"
#include "core/ConfigManager.h"
#include "core/InstanceIpc.h"
#include "core/GlobalShortcutPortal.h"

int main(int argc, char *argv[]) {
    // 清除启动通知ID，防止鼠标旁出现跳动图标
    qunsetenv("DESKTOP_STARTUP_ID");

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setApplicationName("io.github.ader.QtAiAssistant");
    app.setOrganizationName("io.github.ader");
    app.setDesktopFileName("io.github.ader.QtAiAssistant");

    QCommandLineParser parser;
    QCommandLineOption toggleOption("toggle", "Toggle window visibility");
    parser.addOption(toggleOption);
    parser.process(app);
    bool toggleMode = parser.isSet(toggleOption);

    const QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    const QString serverName = QDir(runtimeDir).filePath("io.github.ader.QtAiAssistant-rewrite.ipc");
    InstanceIpc ipc(serverName);
    if (!ipc.startServer()) {
        // Already running, send message to primary instance
        InstanceIpc::sendMessage(serverName, toggleMode ? "toggle" : "show");
        return 0;
    }

    // Initialize config
    ConfigManager::instance();

    MainWindow mainWindow;
    TrayIcon trayIcon(&mainWindow);

    // Register global shortcut via XDG Portal (instant, no process spawning)
    GlobalShortcutPortal *portalShortcut = new GlobalShortcutPortal(&mainWindow);
    QObject::connect(portalShortcut, &GlobalShortcutPortal::activated,
                     &mainWindow, [&mainWindow](const QString &id) {
        Q_UNUSED(id);
        mainWindow.toggleVisibility();
    });
    portalShortcut->registerShortcut(
        QStringLiteral("toggle-window"),
        QStringLiteral("Toggle AI Assistant"),
        QStringLiteral("super+space"));

    // IPC fallback: second instance --toggle still works
    QObject::connect(&ipc, &InstanceIpc::messageReceived, &mainWindow, [&mainWindow](const QString &msg) {
        if (msg == "toggle") {
            mainWindow.toggleVisibility();
        } else {
            mainWindow.showNormal();
            mainWindow.activateWindow();
        }
    });

    if (!toggleMode) {
        mainWindow.show();
    }
    
    return app.exec();
}
