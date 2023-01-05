#ifndef __UPDATEMODEL_H__
#define __UPDATEMODEL_H__

#include "updatemodel.h"

#include <QDebug>
#include <QMetaEnum>
#include <QList>
#include <QMap>

UpdateModel * UpdateModel::m_instance = nullptr;

UpdateModel::UpdateModel(QObject *parent)
    : QObject{parent}
    , m_updateAvailable(false)
    , m_updateMode(UpdateModel::UpdateType::Invalid)
    , m_downloadSize(1)
    , m_updateStatus(UpdateStatus::Default)
{

}

UpdateModel* UpdateModel::instance()
{
    // 对于单线程来讲，安全性足够
    if (m_instance == nullptr) {
        m_instance = new UpdateModel();
    }

    return m_instance;
}

void UpdateModel::setUpdateAvailable(bool available)
{
    qInfo() << Q_FUNC_INFO << available << ", current: " << m_updateAvailable;
    if (m_updateAvailable == available)
        return;

    m_updateAvailable = available;
    Q_EMIT updateAvailableChange(m_updateAvailable);
}


void UpdateModel::setUpdateMode(qulonglong updateMode)
{
    qInfo() << Q_FUNC_INFO << updateMode << ", current: " << m_updateMode;
    if (m_updateMode == updateMode)
        return;

    m_updateMode = updateMode;
    updateAvailableState();
    Q_EMIT updateModeChanged(m_updateMode);
}

void UpdateModel::setDownloadSize(qlonglong downloadSize)
{
    m_downloadSize = downloadSize;
    updateAvailableState();
}

void UpdateModel::setPackages(QMap<QString, QStringList> packages)
{
    m_packages = packages;
    updateAvailableState();
}

void UpdateModel::updateAvailableState()
{
    bool isPackageEmpty = true;
    QMetaEnum metaEnum = QMetaEnum::fromType<UpdateModel::UpdateType>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        const UpdateModel::UpdateType type = static_cast<UpdateModel::UpdateType>(metaEnum.value(i));
        if ((m_updateMode & type) && UPDATE_TYPE_NAME.contains(type)) {
            const QStringList &packages = m_packages.value(UPDATE_TYPE_NAME.value(type));
            if (!packages.isEmpty()) {
                isPackageEmpty = false;
                break;
            }
        }
    }

    if (isPackageEmpty || m_downloadSize > 0)
        setUpdateAvailable(false);

    setUpdateAvailable(true);
}

void UpdateModel::onUpdatePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName)
    Q_UNUSED(invalidatedProperties)

    if (changedProperties.contains("NeedDownloadSize")) {
        bool ok;
        double size = changedProperties.value("NeedDownloadSize").toDouble(&ok);
        if (!ok) size = 1.0;
        setDownloadSize(size);
    }
}

void UpdateModel::setUpdateStatus(UpdateModel::UpdateStatus status)
{
    qInfo() << Q_FUNC_INFO << status << ", current: " << m_updateStatus;
    if (m_updateStatus == status)
        return;

    m_updateStatus = status;
    Q_EMIT updateStatusChanged(m_updateStatus);
}

void UpdateModel::setDistUpgradeProgress(double progress)
{
    qInfo() << Q_FUNC_INFO << progress << ", current: " << m_distUpgradeProgress;
    if (qFuzzyCompare(progress, m_distUpgradeProgress))
        return;

    m_distUpgradeProgress = progress;
    Q_EMIT distUpgradeProgressChanged(m_distUpgradeProgress);
}

void UpdateModel::setUpdateError(UpdateError error)
{
    qInfo() << Q_FUNC_INFO << error << ", current: " << m_updateError;
    if (m_updateError == error)
        return;

    m_updateError = error;
}

QString UpdateModel::updateErrorMessage() const
{
    // TODO 翻译
    static const QMap<UpdateError, QString> ErrorMessage = {
        {UpdateError::NoError, ""},
        {UpdateError::NoNetwork, tr("Dependency error, failed to detect the updates")},
        {UpdateError::NoSpace, tr("Update failed: insufficient disk space.")},
        {UpdateError::DependenciesBrokenError, tr("Unmet dependencies.")},
        {UpdateError::DpkgInterrupted, tr("Dpkg interrupt.")},
        {UpdateError::UnKnown, tr("Unknown error.")}
    };

    return ErrorMessage.value(m_updateError);
}

void UpdateModel::setLastError(const QString &message)
{
    m_lastErrorMessage = message;
}

void UpdateModel::setBackupConfigValidation(bool valid)
{
    m_isBackupConfigValid = valid;
}

QString UpdateModel::updateActionText(UpdateAction action)
{
    static const QMap<UpdateAction, QString> ActionsText = {
        {None, tr("")},
        {DoBackupAgain, tr("Backup again")},
        {ExitUpdating, tr("Exit updating")},
        {ContinueUpdating, tr("Continue updating")},
        {CancelUpdating, tr("Cancel updating")},
    };

    return ActionsText.value(action);
}




#endif // __UPDATEMODEL_H__