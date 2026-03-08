cmake_minimum_required(VERSION 3.16)
project(qt-ai-assistant VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network DBus)

add_executable(qt-ai-assistant
    src/main.cpp
    src/core/ConfigManager.cpp
    src/core/Conversation.cpp
    src/core/ConversationManager.cpp
    src/core/LlmClient.cpp
    src/core/InstanceIpc.cpp
    src/core/GlobalShortcutPortal.cpp
    src/ui/MainWindow.cpp
    src/ui/ChatWidget.cpp
    src/ui/TrayIcon.cpp
)

target_link_libraries(qt-ai-assistant PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::DBus
)

include(GNUInstallDirs)
install(TARGETS qt-ai-assistant DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES io.github.ader.QtAiAssistant.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES resources/icons/io.github.ader.QtAiAssistant.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
