# Qt AI Quike Assistant

## 项目用途

A software to solve Chinese input problem.

我经常用CherryStudio的ai快捷助手向ai查询一些简单的问题，但是这个软件在fedora kde下存在偶尔切换不了输入法的问题，遇到时必须重开软件，非常搞心态。我想这是因为CherryStudio是用electron开发的，electron在wayland下存在输入法焦点切换的问题。于是我vibe coding了一个基于qt flatpak 的 ai快捷助手。我完全不会qt开发，这个软件不保证能用。

---

## 使用方法

### 安装方法

**方式一：通过 Flatpak 编译/安装 (推荐)**
项目提供了 Flatpak manifest，你可以直接构建出独立的 flatpak 环境应用：

```bash
flatpak-builder build-dir io.github.ader.QtAiAssistant.json --force-clean --user --install
```

如果项目目录下已有预先打包好的 `QtAiAssistant.flatpak`，也可以通过以下命令直接安装：

```bash
flatpak install --user QtAiAssistant.flatpak
```

**方式二：通过源码编译所需环境**
你需要系统中安装有 Qt6 相关的开发包。

```bash
mkdir build && cd build
cmake ..
cmake --build .
sudo make install
```

### 使用方法

1. **正常启动**：启动应用后，它可能会停留在后台并在系统托盘显示对应的图标。你可以点击托盘图标来呼出或隐藏主界面。
2. **全局快捷键呼出**：你可以利用 KDE 的“自定义快捷键”设置，将应用的显示/隐藏绑定到快捷键（如 `Alt + Space` 或 `Meta + A`）。

### 配置方法

配置文件在 `~/.var/app/io.github.ader.QtAiAssistant/config/io.github.ader/io.github.ader.QtAiAssistant/config.json`。`extraParams`中的内容会传递给api的请求体。参考配置示例：

```json
{
  "currentModel": "m/minimax-2.5",
  "modelList": [
    {
      "name": "m/minimax-2.5",
      "apiUrl": "https://api.minimaxi.com/v1/text/chatcompletion_v2",
      "apiKey": "sk-api-PD19hskSHoetzdsfsfsfuZfPZrnfmLf8D6ovslSYQU9T_S9orRtc9J6lJC3NJiXY6ZAAcpFncTsZ6DIqurW5CudhV7qaVpCM",
      "modelId": "MiniMax-M2.5",
      "systemPrompt": "你是用户的快捷助手，用于查询、处理简单问题。回复规则：中文，纯文本，简短",
      "extraParams": {
        "stream": true
      }
    },
    {
      "name": "op/minimax-2.5",
      "apiUrl": "https://openrouter.ai/api/v1/chat/completions",
      "apiKey": "sk-or-v1-784f366wtwrwrwrf0b0e09877905c967f7647a3a8a10886",
      "modelId": "minimax/minimax-m2.5",
      "systemPrompt": "xxx",
      "extraParams": {
        "provider": {
          "sort": "throughput"
        },
        "reasoning": {
          "effort": "low"
        }
      }
    }
  ]
}
```

---

## 开发指南

### 项目架构介绍

本项目基于 **C++17** 和 **Qt6** 进行开发，采用前后端逻辑分离的设计架构。核心源码集中在 `src` 目录：

- **`src/core/` (核心逻辑层)**
  - `ConfigManager`: 单例模式，负责读取和保存用户模型配置（JSON格式）。
  - `LlmClient`: 封装网络请求流程，处理与大语言模型 API 端点 (如 OpenAI 格式) 的 HTTP 通信与流式/非流式响应。
  - `Conversation` & `ConversationManager`: 会话和上下文管理，用于维护聊天内容的发送及清理流程。
  - `InstanceIpc`: 进程间通信 (IPC) 管理模块。当重新执行应用时，由于存在单例限制，IPC 负责捕捉 `--toggle` 等指令并转发给目前正在运行的主进程，实现窗口的极速隐藏与呼出。
- **`src/ui/` (用户界面层)**
  - `MainWindow`: 主窗口框架，集成托盘逻辑、窗口无边框或是悬浮特征等。
  - `ChatWidget`: 处理消息列表、支持 Markdown 渲染（如适用），响应用户的消息发送指令。
  - `TrayIcon`: 负责系统状态栏 (System Tray) 图标及其右键菜单逻辑。

### 构建方法与依赖

**开发依赖条件：**

- C++ 17 编译器 (GCC/Clang)
- CMake >= 3.16
- Qt6 库 (所用模块：`Core`, `Gui`, `Widgets`, `Network`, `DBus`)

**开发构建命令：**

```bash
# 生成 CMake 缓存配置
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 进行编译
cmake --build build -j$(nproc)

# 运行调试程序
./build/qt-ai-assistant
```

如果要添加新的 Qt 模块，只需在 `CMakeLists.txt` 中的 `find_package(Qt6 ...)` 中追加即可。
