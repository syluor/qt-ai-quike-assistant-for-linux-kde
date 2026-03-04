#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QVariantMap>

struct ModelConfig {
    QString name;
    QString apiUrl;
    QString apiKey;
    QString modelId;
    QString systemPrompt;
    QVariantMap extraParams;

    static ModelConfig fromJson(const QJsonObject &json);
    QJsonObject toJson() const;
};

class ConfigManager : public QObject {
    Q_OBJECT
public:
    static ConfigManager& instance();

    void loadConfig();
    void saveConfig();

    QString currentModelId() const;
    void setCurrentModelId(const QString &id);

    QList<ModelConfig> modelList() const;
    ModelConfig currentModel() const;
    void setModelList(const QList<ModelConfig> &models);

signals:
    void configChanged();

private:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager() = default;
    Q_DISABLE_COPY(ConfigManager)

    QString m_currentModel;
    QList<ModelConfig> m_models;
    QString m_configPath;
};

#endif // CONFIGMANAGER_H
