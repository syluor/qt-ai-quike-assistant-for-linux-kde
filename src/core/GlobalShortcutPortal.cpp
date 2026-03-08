#include "GlobalShortcutPortal.h"
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>

const QString GlobalShortcutPortal::PORTAL_SERVICE =
    QStringLiteral("org.freedesktop.portal.Desktop");
const QString GlobalShortcutPortal::PORTAL_PATH =
    QStringLiteral("/org/freedesktop/portal/desktop");
const QString GlobalShortcutPortal::SHORTCUT_IFACE =
    QStringLiteral("org.freedesktop.portal.GlobalShortcuts");
const QString GlobalShortcutPortal::REQUEST_IFACE =
    QStringLiteral("org.freedesktop.portal.Request");

GlobalShortcutPortal::GlobalShortcutPortal(QObject *parent)
    : QObject(parent) {}

QString GlobalShortcutPortal::senderToken() const {
    // D-Bus unique name ":1.42" -> "1_42"
    QString name = QDBusConnection::sessionBus().baseService();
    name.remove(0, 1); // remove leading ':'
    name.replace('.', '_');
    return name;
}

QString GlobalShortcutPortal::nextRequestToken() {
    return QStringLiteral("qtaia_%1").arg(m_requestCounter++);
}

void GlobalShortcutPortal::registerShortcut(const QString &id,
                                            const QString &description,
                                            const QString &preferredTrigger) {
    m_shortcutId = id;
    m_description = description;
    m_preferredTrigger = preferredTrigger;
    createSession();
}

// ── Step 1: CreateSession ───────────────────────────────────────────

void GlobalShortcutPortal::createSession() {
    const QString token = nextRequestToken();
    const QString sessionToken = QStringLiteral("qtaia_session");

    // Build request object path so we can listen for the Response signal
    const QString requestPath =
        QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2")
            .arg(senderToken(), token);

    QDBusConnection::sessionBus().connect(
        PORTAL_SERVICE, requestPath, REQUEST_IFACE,
        QStringLiteral("Response"), this,
        SLOT(onCreateSessionResponse(QDBusMessage)));

    QVariantMap options;
    options[QStringLiteral("handle_token")] = token;
    options[QStringLiteral("session_handle_token")] = sessionToken;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        PORTAL_SERVICE, PORTAL_PATH, SHORTCUT_IFACE,
        QStringLiteral("CreateSession"));
    msg << options;

    QDBusPendingCall pending =
        QDBusConnection::sessionBus().asyncCall(msg);

    // If the call fails synchronously (e.g. portal not available), log it
    QDBusPendingCallWatcher *watcher =
        new QDBusPendingCallWatcher(pending, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [watcher](QDBusPendingCallWatcher *w) {
                if (w->isError()) {
                    qWarning() << "GlobalShortcuts: Portal not available:"
                               << w->error().message();
                }
                w->deleteLater();
            });
}

void GlobalShortcutPortal::onCreateSessionResponse(const QDBusMessage &msg) {
    // Response(uint response, a{sv} results)
    const QList<QVariant> args = msg.arguments();
    if (args.size() < 2) return;

    uint response = args.at(0).toUInt();
    if (response != 0) {
        qWarning() << "GlobalShortcuts: CreateSession failed, response ="
                    << response;
        return;
    }

    // Extract session_handle from results (note: it's a string, not object path,
    // due to a portal spec erratum)
    const QVariantMap results =
        qdbus_cast<QVariantMap>(args.at(1).value<QDBusArgument>());
    m_sessionHandle = results.value(QStringLiteral("session_handle")).toString();

    qDebug() << "GlobalShortcuts: Session created:" << m_sessionHandle;

    // Now listen for the Activated signal on the portal object
    QDBusConnection::sessionBus().connect(
        PORTAL_SERVICE, PORTAL_PATH, SHORTCUT_IFACE,
        QStringLiteral("Activated"), this,
        SLOT(onActivated(QDBusMessage)));

    bindShortcuts();
}

// ── Step 2: BindShortcuts ───────────────────────────────────────────

void GlobalShortcutPortal::bindShortcuts() {
    const QString token = nextRequestToken();

    const QString requestPath =
        QStringLiteral("/org/freedesktop/portal/desktop/request/%1/%2")
            .arg(senderToken(), token);

    QDBusConnection::sessionBus().connect(
        PORTAL_SERVICE, requestPath, REQUEST_IFACE,
        QStringLiteral("Response"), this,
        SLOT(onBindShortcutsResponse(QDBusMessage)));

    // Build shortcuts: a(sa{sv})
    // Each shortcut is a struct (string id, dict of properties)
    QDBusArgument shortcuts;
    shortcuts.beginArray(
        qMetaTypeId<QDBusArgument>() != 0
            ? QMetaType(qMetaTypeId<QDBusArgument>())
            : QMetaType());
    shortcuts.beginStructure();
    shortcuts << m_shortcutId;
    // properties dict: a{sv}
    shortcuts.beginMap(QMetaType(QMetaType::QString),
                       QMetaType(qMetaTypeId<QDBusVariant>()));
    shortcuts.beginMapEntry();
    shortcuts << QStringLiteral("description")
              << QDBusVariant(m_description);
    shortcuts.endMapEntry();
    shortcuts.beginMapEntry();
    shortcuts << QStringLiteral("preferred_trigger")
              << QDBusVariant(m_preferredTrigger);
    shortcuts.endMapEntry();
    shortcuts.endMap();
    shortcuts.endStructure();
    shortcuts.endArray();

    QVariantMap options;
    options[QStringLiteral("handle_token")] = token;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        PORTAL_SERVICE, PORTAL_PATH, SHORTCUT_IFACE,
        QStringLiteral("BindShortcuts"));
    msg << QVariant::fromValue(QDBusObjectPath(m_sessionHandle))
        << QVariant::fromValue(shortcuts)
        << QString()  // parent_window (empty = no parent)
        << options;

    QDBusConnection::sessionBus().asyncCall(msg);
}

void GlobalShortcutPortal::onBindShortcutsResponse(const QDBusMessage &msg) {
    const QList<QVariant> args = msg.arguments();
    if (args.size() < 2) return;

    uint response = args.at(0).toUInt();
    if (response != 0) {
        qWarning() << "GlobalShortcuts: BindShortcuts failed, response ="
                    << response;
        return;
    }
    qDebug() << "GlobalShortcuts: Shortcut bound successfully";
}

// ── Activated signal ────────────────────────────────────────────────

void GlobalShortcutPortal::onActivated(const QDBusMessage &msg) {
    // Activated(o session_handle, s shortcut_id, t timestamp, a{sv} options)
    const QList<QVariant> args = msg.arguments();
    if (args.size() < 2) return;

    const QString shortcutId = args.at(1).toString();
    qDebug() << "GlobalShortcuts: Activated" << shortcutId;
    emit activated(shortcutId);
}
