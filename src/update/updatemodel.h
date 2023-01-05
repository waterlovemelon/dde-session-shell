#ifndef UPDATEMODEL_H
#define UPDATEMODEL_H

#include <QObject>
#include <QMap>
#include <QStringList>

class UpdateModel : public QObject
{
    Q_OBJECT

public:

enum UpdateType {
    Invalid             = 0,        // 无效
    SystemUpdate        = 1 << 0,   // 系统
    AppStoreUpdate      = 1 << 1,   // 应用商店（1050版本弃用）
    SecurityUpdate      = 1 << 2,   // 安全
    UnknownUpdate       = 1 << 3,   // 未知来源
    OnlySecurityUpdate  = 1 << 4    // 仅安全更新（1060版本弃用）
};
Q_ENUM(UpdateType)

const QMap<UpdateType, QString> UPDATE_TYPE_NAME = {
    {SystemUpdate, "system_upgrade"},
    {AppStoreUpdate, "appstore_upgrade"},
    {SecurityUpdate, "security_upgrade"},
    {UnknownUpdate, "unknown_upgrade"}
};

enum UpdateStatus {
    Default = 0,
    Ready,
    BackingUp,
    BackupSuccess,
    BackupFailed,
    Installing,
    InstallSuccess,
    InstallFailed,
    InstallComplete,
    Unknown
};
Q_ENUM(UpdateStatus)

enum UpdateError {
    NoError,
    NoNetwork,
    NoSpace,
    CanNotBackup,
    DependenciesBrokenError,
    DpkgInterrupted,
    BackendInterfaceError,
    UnKnown
};
Q_ENUM(UpdateError)

enum UpdateAction {
    None,
    DoBackupAgain,
    ExitUpdating,
    ContinueUpdating,
    CancelUpdating
};
Q_ENUM(UpdateAction)

public:
    static UpdateModel *instance();

    void setUpdateAvailable(bool available);
    bool isUpdateAvailable() const { return m_updateAvailable; }

    void setUpdateMode(qulonglong updateMode);
    qulonglong updateMode() const { return m_updateMode; }

    void setDownloadSize(qlonglong downloadSize);
    void setPackages(QMap<QString, QStringList> packages);

    void setUpdateStatus(UpdateModel::UpdateStatus status);
    UpdateStatus updateStatus() const { return m_updateStatus; }

    void setDistUpgradeProgress(double progress);
    double distUpgradeProgress() const { return m_distUpgradeProgress; }

    void setUpdateError(UpdateError error);
    UpdateError updateError() const { return m_updateError; };
    QString updateErrorMessage() const;

    void setLastError(const QString &message);
    QString lastError() const { return m_lastErrorMessage; }

    void setBackupConfigValidation(bool valid);
    bool isBackupConfigValid() const { return m_isBackupConfigValid; }

    static QString updateActionText(UpdateAction action);

private:
    explicit UpdateModel(QObject *parent = nullptr);
    void updateAvailableState();

signals:
    void updateAvailableChange(bool available);
    void updateModeChanged(qulonglong updateMode);
    void updateStatusChanged(UpdateStatus status);
    void distUpgradeProgressChanged(double progress);

private slots:
    void onUpdatePropertiesChanged(const QString &, const QVariantMap &, const QStringList &);

private:
    static UpdateModel *m_instance;
    bool m_updateAvailable;
    qulonglong m_updateMode;
    qlonglong m_downloadSize;
    QMap<QString, QStringList> m_packages;
    UpdateStatus m_updateStatus;
    double m_distUpgradeProgress;
    UpdateError m_updateError;
    QString m_lastErrorMessage;
    bool m_isBackupConfigValid;
};

#endif // UPDATEMODEL_H
