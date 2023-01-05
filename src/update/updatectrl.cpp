#include "updatectrl.h"

#include <QTimer>
#include <QDebug>
#include <QDir>

#include <QDBusPendingReply>
#include <QDBusMetaType>
#include <QMap>

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>

#include <QFont>
#include <QWidget>
#include <QApplication>

Q_DECLARE_METATYPE(BatteryPercentageMap)


PowerInterface::PowerInterface(QObject *parent)
    :    QDBusAbstractInterface("com.deepin.daemon.Power","/com/deepin/daemon/Power"
                                ,"com.deepin.daemon.Power"
                                ,QDBusConnection::sessionBus(),parent)
{
    qRegisterMetaType<BatteryPercentageMap>("BatteryPercentageMap");
    qDBusRegisterMetaType<BatteryPercentageMap>();

    QDBusConnection::sessionBus().connect(this->service(), this->path(), "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged","sa{sv}as", this, SLOT(__propertyChanged__(QDBusMessage)));
}


QString UpdateCtrl::updateCode(UpdateStatusCode code, QString message)
{
    switch (code) {
    case SystemUpgradeChecking:
        return tr("Preparing update...");           //正在准备更新...
    case SystemUpgradeInstalling:
        return tr("Working on updates. Don't turn off your PC");//正在安装更新，请不要关闭电脑

    case NetworkError :
        return tr("Network error, please check your network and retry");//网络异常，请检查您的网络连接后重试
    case DiskError    :{
        double size=message.toInt()/1024.0;
        char sss[32];
        sprintf(sss, "%.2f", size);
        //系统盘剩余空间不足%1G，请清理系统盘
        return tr("The free space on the system disk is less than %1G, please free up space on your system disk and retry").arg(sss);
    }
    case PowerError   :
        return tr("The battery level is lower than 60%, please charge it and retry.");//电池电量低于60%，\n请接通电脑电源后重试;
    default:
        break;
    }

    return "";
}

UpdateCtrl::UpdateCtrl(QObject *parent)
    : QObject(parent)
    , m_inter(new QDBusInterface(ServiceName, ServicePath,ServiceInterface,QDBusConnection::systemBus()))
    , m_powerInter(new PowerInterface(this))
    , m_updateInter(nullptr)
    , m_managerInter(nullptr)
    , m_abRecoveryInter(nullptr)
    , m_distUpgradeJob(nullptr)
{
    init();
}

void UpdateCtrl::init()
{
    connect(m_inter,SIGNAL(sigSystemUpgradeProgress(qint64 ,qint64 ,QString , int ,QString ,QString ))
            ,this, SLOT(onUpdateProgress(qint64 ,qint64 ,QString , int ,QString ,QString )));

    QDBusInterface langInter( "com.deepin.daemon.LangSelector",
                              "/com/deepin/daemon/LangSelector",
                              "com.deepin.daemon.LangSelector",
                              QDBusConnection::sessionBus() );
    bool langIsChinese = langInter.property("CurrentLocale").toString().split('_').last() == "zh";

    //鸡汤路径
    QDBusPendingReply<QString>  reply =
            m_inter->asyncCall(QLatin1String("CheckSystemLatestVersion"),QVariant(true));
    reply.waitForFinished();
    QString res=reply;
    qInfo()<<"update: CheckSystemLatestVersion "<<res;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(res.toLocal8Bit().data());
    if(! jsonDocument.isNull() ){
        QJsonObject js= jsonDocument.object();
        m_soupPathname=langIsChinese
                    ?js.value("advertisement_zh").toString()
                   :js.value("advertisement_en").toString();
    }
    qInfo() << "update : soup path"<<m_soupPathname;

    /* power */
    auto checkLowPower=[=]{
        bool onBattery=m_powerInter->onBattery();
        BatteryPercentageMap data=m_powerInter->batteryPercentage();
        int batteryPercentage = uint(qMin(100.0, qMax(0.0, data.value("Display"))));
        bool lowPower= batteryPercentage<60 && onBattery;
        qInfo()<<"update power: onBattery "<<onBattery<<", batteryPercentage "<<batteryPercentage;
        emit sigWhetherLowPower(lowPower);
    };

    connect(m_powerInter,&PowerInterface::BatteryPercentageChanged,[=]{
        qInfo()<<"update: BatteryPercentageChanged";
        checkLowPower();
    });
    connect(m_powerInter,&PowerInterface::onBattery,[=] {
        qInfo()<<"update: onBattery";
        checkLowPower();
    });

    /* update */
    // 传入空列表表示获取需要下载的所有包的大小，监听NeedDownloadSize即可
    m_managerInter = new ManagerInter("com.deepin.lastore", "/com/deepin/lastore", QDBusConnection::systemBus(), this);
    m_managerInter->setSync(false);
    m_updateInter = new UpdateInter("com.deepin.lastore", "/com/deepin/lastore", QDBusConnection::systemBus(), this);
    m_managerInter->setSync(false);
    m_abRecoveryInter = new RecoveryInter("com.deepin.ABRecovery", "/com/deepin/ABRecovery", QDBusConnection::systemBus(), this);

    m_managerInter->PackagesDownloadSize(QStringList());
    connect(m_managerInter, &ManagerInter::UpdateModeChanged, UpdateModel::instance(), &UpdateModel::setUpdateMode);
    QDBusConnection::systemBus().connect("com.deepin.lastore",
                                          "/com/deepin/lastore",
                                          "org.freedesktop.DBus.Properties",
                                          "PropertiesChanged",
                                          UpdateModel::instance(),
                                          SLOT(onUpdatePropertiesChanged(QString,QVariantMap,QStringList)));
    connect(m_managerInter, &ManagerInter::JobListChanged, this, &UpdateCtrl::onJobListChanged);

    connect(m_abRecoveryInter, &RecoveryInter::JobEnd, this, [] (const QString &kind, bool success, const QString &errMsg) {
        qInfo() << "Backup job end, kind: " << kind << ", success: " << success << ", error message: " << errMsg;
        if ("backup" != kind) {
            qWarning() << "Kind error: " << kind;
            return;
        }

        UpdateModel::instance()->setUpdateStatus(success ? UpdateModel::UpdateStatus::BackupSuccess : UpdateModel::UpdateStatus::BackupFailed);
        if (!success)
            UpdateModel::instance()->setLastError(errMsg);
    });
    connect(m_abRecoveryInter, &RecoveryInter::BackingUpChanged, this, [] (bool value) {
        qInfo() << "Backing up changed: " << value;
        if (value) {
            UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::BackingUp);
        }
    });
    connect(m_abRecoveryInter, &RecoveryInter::ConfigValidChanged, this, [] (bool value) {
        UpdateModel::instance()->setBackupConfigValidation(value);
    });

    UpdateModel::instance()->setUpdateMode(m_managerInter->updateMode());
    UpdateModel::instance()->setPackages(m_updateInter->classifiedUpdatablePackages());
}

// TODO 防呆机制，有什么办法可以判断更新卡住了，超时后判定更新失败，重启电脑，避免卡死在更新界面
void UpdateCtrl::doLoop()
{
    return;
    QTimer * pLoopTimer = new QTimer(this);
    pLoopTimer->setInterval(30*1000);

    static int errorCount=0;
    connect(pLoopTimer,&QTimer::timeout,this,[=]{
        pLoopTimer->stop();
        QDBusPendingReply<QString>  reply =
                m_inter->asyncCall(QLatin1String("GetSystemUpgradeStatus"));
        reply.waitForFinished();

        reply.error().type()==QDBusError::NoError ? errorCount=0 : errorCount++;
        if (errorCount>10){
            // 5min未响应 判断更新失败
            emit sigServiceNotRun();
        }
        qInfo()<<"update: doloop, error count :"<<errorCount<<reply.error();

        pLoopTimer->start();
    });

    pLoopTimer->start();
}

void UpdateCtrl::UpgradeSystem()
{
    qInfo() << Q_FUNC_INFO;
    m_managerInter->DistUpgrade();
}

QString UpdateCtrl::GetSystemUpgradeLog()
{
    QDBusPendingReply<QString> reply =
            m_inter->asyncCall(QLatin1String("GetSystemUpgradeLog"));
    reply.waitForFinished();
    qInfo()<<"update: GetSystemUpgradeLog,"<<reply;
    return reply;
}

void UpdateCtrl::onUpdateProgress(qint64 totalNum, qint64 curNum, QString packageName, int finished, QString scode, QString messge)
{
    emit sigUpdateProgress(totalNum,curNum,packageName,finished,scode,messge);
}

void UpdateCtrl::onJobListChanged(const QList<QDBusObjectPath> &jobs)
{
    qInfo() << Q_FUNC_INFO;

    for (const auto &job : jobs) {
        const QString &jobPath = job.path();
        qInfo() << "path : " << jobPath;
        JobInter jobInter("com.deepin.lastore", jobPath, QDBusConnection::systemBus());
        if (!jobInter.isValid()) {
            qWarning() << "Job is not valid";
            continue;
        }

        // id maybe scrapped
        const QString &id = jobInter.id();
        qDebug() << "Job id : " << id;
        if (id == "dist_upgrade" && m_distUpgradeJob == nullptr) {
            m_distUpgradeJob = new JobInter("com.deepin.lastore", jobPath, QDBusConnection::systemBus(), this);
            connect(m_distUpgradeJob, &__Job::ProgressChanged, UpdateModel::instance(), &UpdateModel::setDistUpgradeProgress);
            connect(m_distUpgradeJob, &__Job::StatusChanged, this, &UpdateCtrl::onDistUpgradeStatusChanged);
        }
    }
}

void UpdateCtrl::onDistUpgradeStatusChanged(const QString &status)
{
    static const QMap<QString, UpdateModel::UpdateStatus> DIST_UPGRADE_STATUS_MAP = {
        {"ready", UpdateModel::UpdateStatus::Ready},
        {"running", UpdateModel::UpdateStatus::Installing},
        {"failed", UpdateModel::UpdateStatus::InstallFailed},
        {"succeed", UpdateModel::UpdateStatus::InstallSuccess},
        {"end", UpdateModel::UpdateStatus::InstallComplete}
    };

    qInfo() << "Dist upgrade status changed " << status;
    if (DIST_UPGRADE_STATUS_MAP.contains(status)) {
        const UpdateModel::UpdateStatus updateStatus = DIST_UPGRADE_STATUS_MAP.value(status);
        if (updateStatus == UpdateModel::UpdateStatus::InstallComplete) {
            m_managerInter->CleanJob(m_distUpgradeJob->id());
            delete m_distUpgradeJob;
            m_distUpgradeJob = nullptr;

            if (UpdateModel::InstallSuccess == UpdateModel::instance()->updateStatus()) {
                Q_EMIT sigRequirePowerAction(UpdateCtrl::instance()->isReboot());
            }
        } else {
            if (updateStatus == UpdateModel::UpdateStatus::InstallFailed && m_distUpgradeJob) {
                UpdateModel::instance()->setUpdateError(analyzeJobErrorMessage(m_distUpgradeJob->description()));
            }
            UpdateModel::instance()->setUpdateStatus(updateStatus);
        }
    } else {
        qWarning() << "Unknown dist upgrade status";
        UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::Unknown);
    }
}

UpdateModel::UpdateError UpdateCtrl::analyzeJobErrorMessage(QString jobDescription)
{
    qInfo() << Q_FUNC_INFO << jobDescription;
    QJsonParseError err_rpt;
    QJsonDocument jobErrorMessage = QJsonDocument::fromJson(jobDescription.toUtf8(), &err_rpt);

    if (err_rpt.error != QJsonParseError::NoError) {
        qWarning() << "更新失败JSON格式错误";
        return UpdateModel::UpdateError::UnKnown;
    }
    const QJsonObject &object = jobErrorMessage.object();
    QString errorType =  object.value("ErrType").toString();

    if (errorType.contains("fetchFailed", Qt::CaseInsensitive) || errorType.contains("IndexDownloadFailed", Qt::CaseInsensitive)) {
        return UpdateModel::UpdateError::NoNetwork;
    }
    if (errorType.contains("unmetDependencies", Qt::CaseInsensitive) || errorType.contains("dependenciesBroken", Qt::CaseInsensitive)) {
        return UpdateModel::UpdateError::DependenciesBrokenError;
    }
    if (errorType.contains("insufficientSpace", Qt::CaseInsensitive)) {
        return UpdateModel::UpdateError::NoSpace;
    }
    if (errorType.contains("interrupted", Qt::CaseInsensitive)) {
        return UpdateModel::UpdateError::DpkgInterrupted;
    }

    return UpdateModel::UpdateError::UnKnown;
}

void UpdateCtrl::doBackup()
{
    qInfo() << Q_FUNC_INFO;

    QDBusPendingCall call = m_abRecoveryInter->CanBackup();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, call] {
        if (!call.isError()) {
            QDBusReply<bool> reply = call.reply();
            const bool value = reply.value();
            qInfo() << "Can backup: " << value;
            if (value) {
                UpdateModel::instance()->setUpdateStatus(UpdateModel::BackingUp);
                m_abRecoveryInter->StartBackup();
            } else {
                UpdateModel::instance()->setUpdateError(UpdateModel::CanNotBackup);
                UpdateModel::instance()->setUpdateStatus(UpdateModel::BackupFailed);
            }
        } else {
            qWarning() << "Call `CanBackup` interface failed";
            UpdateModel::instance()->setUpdateError(UpdateModel::BackendInterfaceError);
            UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::BackupFailed);
        }
    });
}

void UpdateCtrl::doAction(UpdateModel::UpdateAction action)
{
    qInfo() << Q_FUNC_INFO << action;
    switch (action) {
        case UpdateModel::ContinueUpdating:
            m_managerInter->DistUpgrade();
            UpdateModel::instance()->setUpdateStatus(UpdateModel::Installing);
            break;
        case UpdateModel::ExitUpdating:
        case UpdateModel::CancelUpdating:
            Q_EMIT sigExitUpdating();
            break;
        default:
            break;
    }
}

