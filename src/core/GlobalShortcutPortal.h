#ifndef GLOBALSHORTCUTPORTAL_H
#define GLOBALSHORTCUTPORTAL_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QString>
#include <QVariantMap>

class GlobalShortcutPortal : public QObject {
    Q_OBJECT
public:
    explicit GlobalShortcutPortal(QObject *parent = nullptr);

    /// Start the portal session and register shortcuts.
    /// Call once at startup; will silently no-op if portal is unavailable.
    void registerShortcut(const QString &id,
                          const QString &description,
                          const QString &preferredTrigger);

signals:
    void activated(const QString &shortcutId);

private:
    void createSession();
    void bindShortcuts();

    void onCreateSessionResponse(const QDBusMessage &msg);
    void onBindShortcutsResponse(const QDBusMessage &msg);
    void onActivated(const QDBusMessage &msg);

    QString senderToken() const;
    QString nextRequestToken();

    // Shortcut details (we only register one shortcut, but could extend)
    QString m_shortcutId;
    QString m_description;
    QString m_preferredTrigger;

    // Portal session
    QString m_sessionHandle;
    uint m_requestCounter = 0;

    static const QString PORTAL_SERVICE;
    static const QString PORTAL_PATH;
    static const QString SHORTCUT_IFACE;
    static const QString REQUEST_IFACE;
};

#endif // GLOBALSHORTCUTPORTAL_H
