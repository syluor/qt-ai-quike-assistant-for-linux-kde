#include <QApplication>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <QDir>
#include "ui/MainWindow.h"
#include "ui/TrayIcon.h"
#include "core/ConfigManager.h"
#include "core/InstanceIpc.h"

int main(int argc, char *argv[]) {
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
