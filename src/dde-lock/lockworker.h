#ifndef LOCKWORKER_H
#define LOCKWORKER_H

#include "src/global_util/dbus/dbuslogin1manager.h"
#include "src/global_util/dbus/dbushotzone.h"
#include "src/session-widgets/userinfo.h"
#include "../libdde-auth/interface/deepinauthinterface.h"
#include "../libdde-auth/deepinauthframework.h"
#include "src/session-widgets/authinterface.h"

#include <QObject>
#include <QWidget>

#include <com_deepin_daemon_logined.h>
#include <com_deepin_daemon_accounts.h>
#include <com_deepin_sessionmanager.h>
#include <com_deepin_dde_lockservice.h>

using LoginedInter = com::deepin::daemon::Logined;
using AccountsInter = com::deepin::daemon::Accounts;
using SessionManager = com::deepin::SessionManager;
using LockService = com::deepin::dde::LockService;

class SessionBaseModel;
class LockWorker : public Auth::AuthInterface, public DeepinAuthInterface
{
    Q_OBJECT
public:
    enum AuthFlag {
        Password = 1 << 0,
        Fingerprint = 1 << 1,
        Face = 1 << 2,
        ActiveDirectory = 1 << 3
    };

    enum EventType {
        PromptQuestion = 1,
        PromptSecret = 2,
        ErrorMsg = 3,
        TextInfo = 4,
        Failure = 5,
        Success = 6
    };

    explicit LockWorker(SessionBaseModel *const model, QObject *parent = nullptr);

    void switchToUser(std::shared_ptr<User> user) override;
    void authUser(const QString &password) override;

    void enableZoneDetected(bool disable);

    void onDisplayErrorMsg(const QString &msg) override;
    void onDisplayTextInfo(const QString &msg) override;
    void onPasswordResult(const QString &msg) override;

    void setEditReadOnly(bool isReadOnly) override;

private:
    void onUserAdded(const QString &user) override;
    void saveNumlockStatus(std::shared_ptr<User> user, const bool &on);
    void recoveryUserKBState(std::shared_ptr<User> user);

    // lock
    void lockServiceEvent(quint32 eventType, quint32 pid, const QString &username, const QString &message);
    void onUnlockFinished(bool unlocked, bool fromAgent);

    void onCurrentUserChanged(const QString &user);

private:
    bool m_authenticating;
    bool m_isThumbAuth;
    LockService *m_lockInter;
    DBusHotzone *m_hotZoneInter;
    DeepinAuthFramework *m_authFramework;
    QString m_password;
    QMap<std::shared_ptr<User>, bool> m_lockUser;
    SessionManager *m_sessionManager;
};

#endif // LOCKWORKER_H
