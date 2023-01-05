#ifndef UPDATECTRL_H
#define UPDATECTRL_H

#include "updatemodel.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QtDBus>

#include <com_deepin_abrecovery.h>
#include <com_deepin_lastore_job.h>
#include <com_deepin_lastore_jobmanager.h>
#include <com_deepin_lastore_updater.h>

using UpdateInter = com::deepin::lastore::Updater;
using JobInter = com::deepin::lastore::Job;
using ManagerInter = com::deepin::lastore::Manager;
using RecoveryInter = com::deepin::ABRecovery;

#define ServiceName "com.deepin.lastore"
#define ServicePath "/com/deepin/lastore"
#define ServiceInterface "com.deepin.lastore.Manager"

enum UpdateStatusCode {
  Unknown = 0, // 未知

  Success = 200, // 成功
  // 系统升级状态定义
  SystemUpgradeDownloading = 201, // 升级下载中
  SystemUpgradeDownloaded = 202,  // 升级下载完成
  SystemUpgradeChecking = 203,    // 升级检查准备中
  SystemUpgradeInstalling = 204,  // 升级安装中
  SystemUpgradeInstalled = 205,   // 升级安装完成
  AppstoreDownloading = 206,      // 商店正在下载
  AppstoreInstalling = 207,       // 商店正在安装
  SystemUpgradeBackup = 208,      // 系统备份

  Failed = 500,                        // 失败
  FailedSystemUpgradeChecked = 501,    // 系统更新检查失败
  FailedSystemUpgradeBackuped = 502,   // 系统更新备份失败
  FailedSystemUpgradeDownloaded = 503, // 系统更新下载失败
  FailedSystemUpgradeInstalled = 504,  // 系统更新安装失败

  NetworkError = 511, // 网络错误
  SourceError = 512,  // 源错误，软件包不在源
  DiskError = 513, // 磁盘空间不足，下载目录或系统安装目录分区容量不足
  PowerError = 514,   // 电量低，电池电量低于60%为电量低
  AptDpkgError = 515, // apt dpkg下载安装错误，apt修复失败
  BackupError = 516,  // 备份失败
  PackageError = 517, // 下载包不存在

  SystemError = 600 // 系统错误

};

// struct UpdateSystemInfo{
//     QString advertisement_zh;     //鸡汤文案 html路径
//     QString advertisement_en;
//     QString release_time;         //检查时间  字符串格式：年-月-日 时-分
//     UpdateErrorCode code=UpdateErrorCode::None;       //"200"//错误码 Code

//    void operator=(const UpdateSystemInfo &info){
//        advertisement_zh =info.advertisement_zh  ;
//        advertisement_en =info.advertisement_en  ;
//        release_time  =info.release_time   ;
//        code=info.code;
//    };
//};

// 系统升级状态
struct UpdateSystemUpgradeStatus {
  UpdateStatusCode code = UpdateStatusCode::Unknown;
  QString desc;
  void operator=(const UpdateSystemUpgradeStatus &info) {
    code = info.code;
    desc = info.desc;
  };
};

typedef QMap<QString, double> BatteryPercentageMap;
class PowerInterface : public QDBusAbstractInterface {
  Q_OBJECT

  Q_SLOT void __propertyChanged__(const QDBusMessage &msg) {
    QList<QVariant> arguments = msg.arguments();
    if (3 != arguments.count())
      return;
    QString interfaceName = msg.arguments().at(0).toString();
    if (interfaceName != "com.deepin.daemon.Power")
      return;
    QVariantMap changedProps =
        qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
    foreach (const QString &prop, changedProps.keys()) {
      const QMetaObject *self = metaObject();
      for (int i = self->propertyOffset(); i < self->propertyCount(); ++i) {
        QMetaProperty p = self->property(i);
        if (p.name() == prop) {
          Q_EMIT p.notifySignal().invoke(this);
        }
      }
    }
  }

public:
  PowerInterface(QObject *parent = 0);

  Q_PROPERTY(bool OnBattery READ onBattery NOTIFY OnBatteryChanged)
  inline bool onBattery() const {
    return qvariant_cast<bool>(property("OnBattery"));
  }

  Q_PROPERTY(BatteryPercentageMap BatteryPercentage READ batteryPercentage
                 NOTIFY BatteryPercentageChanged)
  inline BatteryPercentageMap batteryPercentage() const {
    return qvariant_cast<BatteryPercentageMap>(property("BatteryPercentage"));
  }

Q_SIGNALS: // SIGNALS
  void OnBatteryChanged();
  void BatteryPercentageChanged();
};

class UpdateCtrl : public QObject {
  Q_OBJECT

public:
  // 更新条件不满足
  static QString updateCode(UpdateStatusCode code, QString message = "");
  static UpdateCtrl *instance() {
    static UpdateCtrl *pIns = nullptr;
    if (pIns == nullptr) {
      pIns = new UpdateCtrl();
    }
    return pIns;
  };

  /* 和后端的交互 */
  //(超时检查)获取鸡汤路径
  void doLoop();
  // 开始更新
  void UpgradeSystem();
  // 获取更新日志路径
  QString GetSystemUpgradeLog();

  void doBackup();

  void doAction(UpdateModel::UpdateAction action);
  //
  void setIsReboot(bool isReboot) { m_isReboot = isReboot; }
  bool isReboot() { return m_isReboot; }

  bool isUpdating() { return m_isUpdating; }
  void setIsUpdating(bool isUpdating) { m_isUpdating = isUpdating; }

  QString getSoupPathname() { return m_soupPathname; };

private:
  explicit UpdateCtrl(QObject *parent = nullptr);

  void init();
  UpdateModel::UpdateError analyzeJobErrorMessage(QString jobDescription);

signals:
  void sigUpdateProgress(qint64 totalNum, qint64 curNum, QString packageName,
                         int finished, QString scode, QString messge);
  // 更新结束，关机或重启
  void sigRequirePowerAction(bool isReboot);
  // 后端挂掉
  void sigServiceNotRun();
  void sigWhetherLowPower(bool lowPower);
  void sigExitUpdating();

public slots:
  void onUpdateProgress(qint64 totalNum, qint64 curNum, QString packageName,
                        int finished, QString code, QString messge);

private slots:
  void onJobListChanged(const QList<QDBusObjectPath> &jobs);
  void onDistUpgradeStatusChanged(const QString &status);

private:
  QDBusInterface *m_inter;
  bool m_isReboot; // true 更新重启   false 更新并关机
  bool m_isUpdating = false;
  QString m_soupPathname;
  PowerInterface *m_powerInter;
  UpdateInter *m_updateInter;
  ManagerInter *m_managerInter;
  RecoveryInter *m_abRecoveryInter;
  JobInter *m_distUpgradeJob;
};

#endif // UPDATECTRL_H
