#include "ConfigManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

ModelConfig ModelConfig::fromJson(const QJsonObject &json) {
    ModelConfig cfg;
    cfg.name = json.value("name").toString();
    cfg.apiUrl = json.value("apiUrl").toString();
    cfg.apiKey = json.value("apiKey").toString();
    cfg.modelId = json.value("modelId").toString();
    cfg.systemPrompt = json.value("systemPrompt").toString();
    cfg.extraParams = json.value("extraParams").toObject().toVariantMap();
    return cfg;
}

QJsonObject ModelConfig::toJson() const {
    QJsonObject json;
    json["name"] = name;
    json["apiUrl"] = apiUrl;
    json["apiKey"] = apiKey;
    json["modelId"] = modelId;
    json["systemPrompt"] = systemPrompt;
    json["extraParams"] = QJsonObject::fromVariantMap(extraParams);
    return json;
}

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager(QObject *parent) : QObject(parent) {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if(configDir.isEmpty()) {
        configDir = QDir::homePath() + "/.config/qt-ai-assistant";
    }
    
    QDir dir(configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    m_configPath = dir.filePath("config.json");
    loadConfig();
}

void ConfigManager::loadConfig() {
    QFile file(m_configPath);
    if (!file.exists()) {
        // Default config
        ModelConfig def;
        def.name = "GPT-4o Mini";
        def.apiUrl = "https://api.openai.com/v1/chat/completions";
        def.apiKey = "your-api-key-here";
        def.modelId = "gpt-4o-mini";
        def.systemPrompt = "You are a helpful assistant.";
        QVariantMap extra;
        extra["temperature"] = 0.7;
        def.extraParams = extra;
        m_models.append(def);
        m_currentModel = def.modelId;
        saveConfig();
        return;
    }
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject root = doc.object();
        m_currentModel = root.value("currentModel").toString();
        
        QJsonArray modelsArray = root.value("modelList").toArray();
        m_models.clear();
        for (const QJsonValue &v : modelsArray) {
            m_models.append(ModelConfig::fromJson(v.toObject()));
        }
    }
}

void ConfigManager::saveConfig() {
    QJsonObject root;
    root["currentModel"] = m_currentModel;
    QJsonArray modelsArray;
    for (const auto &m : m_models) {
        modelsArray.append(m.toJson());
    }
    root["modelList"] = modelsArray;
    
    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(root);
        file.write(doc.toJson(QJsonDocument::Indented));
    }
}

QString ConfigManager::currentModelId() const {
    return m_currentModel;
}

void ConfigManager::setCurrentModelId(const QString &id) {
    if (m_currentModel != id) {
        m_currentModel = id;
        saveConfig();
        emit configChanged();
    }
}

QList<ModelConfig> ConfigManager::modelList() const {
    return m_models;
}

ModelConfig ConfigManager::currentModel() const {
    for (const auto &cfg : m_models) {
        if (cfg.name == m_currentModel) {
            return cfg;
        }
    }
    return m_models.isEmpty() ? ModelConfig() : m_models.first();
}

void ConfigManager::setModelList(const QList<ModelConfig> &models) {
    m_models = models;
    saveConfig();
    emit configChanged();
}
