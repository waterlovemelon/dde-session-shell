#include "updatewidget.h"

#include "logowidget.h"
#include "../src/global_util/constants.h"
#include "dsysinfo.h"
#include "updatectrl.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>

#include <DFontSizeManager>

using namespace DDESESSIONCC;
DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

double UpdateWidgetTool::m_scale=1.0;
void UpdateWidgetTool::scaleFontSize(QWidget *p, int pixelSize)
{
    QFont ft = p->font();
    int ps = pixelSize * m_scale;
    ft.setPixelSize(ps);
    p->setFont(ft);
}

int UpdateWidgetTool::scaleSize(int e)
{
    return e*m_scale;
}

QSize UpdateWidgetTool::scaleSize(QSize size)
{
    return size*m_scale;
}

void UpdateWidgetTool::updateScale(QSize size)
{
    m_scale=size.height()/1080.0;
}


UpdateChooseTip::UpdateChooseTip(QWidget *parent): QFrame(parent)
{
    setObjectName("UpdateChooseTip");
    setFixedSize(94,72);
    pNeedUpdate =new QPushButton(this);
    pCancel =new QPushButton(this);
    pNeedUpdate->setFixedSize(86,32);
    pCancel->setFixedSize(86,32);

    QVBoxLayout * pLayout=new QVBoxLayout(this);
    pLayout->setContentsMargins(4,4,4,4);
    pLayout->addWidget(pNeedUpdate);
    pLayout->addWidget(pCancel);

    connect(pNeedUpdate,&QPushButton::clicked,[=]{
        emit this->sigUpdate(m_isReboot,true);
        setVisible(false);
    });
    connect(pCancel,&QPushButton::clicked,[=]{
        emit this->sigUpdate(m_isReboot,false);
        setVisible(false);
    });
}

void UpdateChooseTip::setChooseType(QPoint movePos,bool isReboot)
{
    move(movePos);
    setVisible(true);
    raise();

    m_isReboot=isReboot;
    if (m_isReboot){
        pNeedUpdate->setText(tr("Update and Reboot"));//更新并重启
        pCancel->setText(tr("Reboot"));
    } else {
        pNeedUpdate->setText(tr("Update and Shutdown"));                   //更新并关机
        pCancel->setText(tr("Shutdown"));
    }

    auto textSize=[](QPushButton *p)->QSize{
        QFontMetrics metrics(p->font());
        return metrics.boundingRect(p->text()).size();
    };
    int btnWidth=textSize(pNeedUpdate).width() +2*8;
    pNeedUpdate->setFixedWidth(btnWidth);
    pCancel->setFixedWidth(btnWidth);
    setFixedWidth(qMax(btnWidth+2*4,width()));
}


UpdateFailedWidget::UpdateFailedWidget(QWidget *parent)
{
    m_failedTitle=new QLabel(tr("Update failed"),this);     // 更新失败
    m_showlogBtn=new QPushButton(tr("View update logs"),this); //查看更新日志
    m_showlogBtn->setObjectName("logFailedFrame_showlogBtn");
    m_showlogBtn->setIcon(QIcon("://img/update/arrow_r.png"));

    QVBoxLayout * pFailedLayout=new QVBoxLayout(this);
    pFailedLayout->setMargin(0);
    pFailedLayout->setSpacing(0);
    pFailedLayout->addStretch();
    pFailedLayout->addWidget(m_failedTitle,0,Qt::AlignCenter);
    pFailedLayout->addWidget(m_showlogBtn,0,Qt::AlignCenter);
    pFailedLayout->addStretch();

    // 按钮图标显示在右侧
    m_showlogBtn->setLayoutDirection(Qt::RightToLeft);

    connect(m_showlogBtn,&QPushButton::clicked,this,&UpdateFailedWidget::showLog);
}

void UpdateFailedWidget::adjustWidgetSize()
{
    UpdateWidgetTool::scaleFontSize(m_failedTitle,50);
    UpdateWidgetTool::scaleFontSize(m_showlogBtn,22);

    m_failedTitle->setFixedHeight(UpdateWidgetTool::scaleSize(74));
    m_showlogBtn->setFixedHeight(UpdateWidgetTool::scaleSize(33));
    m_showlogBtn->setIconSize(UpdateWidgetTool::scaleSize(QSize(24,24)));

    layout()->setSpacing(UpdateWidgetTool::scaleSize(24));
}

UpdateShowLogWidget::UpdateShowLogWidget(QWidget *parent)
{
    m_center=new QFrame(this);
    m_center->setObjectName("logFrame");
    m_logFrameTitle =new QLabel(tr("Update logs"),this);    //更新日志
    m_log=new QTextEdit(this);
    m_log->setObjectName("logFrame_logLabel");
    m_backToFailedBtn=new QPushButton(tr("Close"),this);     //关闭
    m_backToFailedBtn->setObjectName("logFrame_closelogBtn");
    QVBoxLayout * pLogFrameLayout=new QVBoxLayout(m_center);
    pLogFrameLayout->setMargin(0);
    pLogFrameLayout->setSpacing(0);
    pLogFrameLayout->addWidget(m_logFrameTitle,0,Qt::AlignLeft);
    pLogFrameLayout->addSpacing(16);
    pLogFrameLayout->addWidget(m_log);
    pLogFrameLayout->addSpacing(24);
    pLogFrameLayout->addWidget(m_backToFailedBtn,0,Qt::AlignRight);

    QVBoxLayout * pLayout=new QVBoxLayout(this);
    pLayout->setMargin(0);
    //pLayout->addStretch();
    pLayout->addWidget(m_center,0,Qt::AlignCenter);
    //pLayout->addStretch();

    connect(m_backToFailedBtn,&QPushButton::clicked,this,&UpdateShowLogWidget::backToFailed);
}

void UpdateShowLogWidget::setPathname(const QString pathname)
{
    QFile file(pathname);
    if (file.exists()){
        if(file.open(QIODevice::ReadOnly)){
            m_log->setText(file.readAll());
            file.close();
        }
    }
    else {
        m_log->setText(pathname);
    }
}

void UpdateShowLogWidget::adjustWidgetSize()
{
    UpdateWidgetTool::scaleFontSize(m_logFrameTitle,16);
    UpdateWidgetTool::scaleFontSize(m_backToFailedBtn,14);
    m_logFrameTitle->setFixedSize(UpdateWidgetTool::scaleSize(QSize(100,24)));   //100 否则英文会超出
    m_backToFailedBtn->setFixedSize(UpdateWidgetTool::scaleSize(QSize(72,28)));
    m_center->setFixedSize(UpdateWidgetTool::scaleSize(QSize(600,443)));
    m_center->layout()->setMargin(UpdateWidgetTool::scaleSize(24));
}


UpdateResultWidget::UpdateResultWidget(QWidget *parent): QFrame(parent)
{
    m_logFrame=new UpdateShowLogWidget(this);
    m_failedFrame=new UpdateFailedWidget(this);
    m_centerLayout=new QVBoxLayout;
    setCenterContent(m_failedFrame);

    //bottom
    m_bottomBtnFrame=new QFrame(this);
    m_exitUpdate=new QPushButton(tr("Exit updates"),this); //退出更新
    m_shutdownOrReboot = new QPushButton(this);
    QHBoxLayout * pBottomLayout=new QHBoxLayout(m_bottomBtnFrame);
    pBottomLayout->setMargin(0);
    pBottomLayout->setSpacing(20);
    pBottomLayout->addStretch();
    pBottomLayout->addWidget(m_exitUpdate,0,Qt::AlignTop);
    pBottomLayout->addWidget(m_shutdownOrReboot,0,Qt::AlignTop);
    pBottomLayout->addStretch();

    //布局
    QVBoxLayout * pLayout =new  QVBoxLayout(this);
    pLayout->setMargin(0);
    pLayout->setSpacing(0);
    pLayout->addLayout(m_centerLayout);
    pLayout->addWidget(m_bottomBtnFrame);

    connect(m_logFrame,&UpdateShowLogWidget::backToFailed,[=]{
        setCenterContent(m_failedFrame);
    });
    connect(m_failedFrame,&UpdateFailedWidget::showLog,[=]{
        setCenterContent(m_logFrame);
    });
    connect(m_exitUpdate,&QPushButton::clicked,this,[=]{
        emit UpdateCtrl::instance()->sigExitUpdating();
    });
    connect(m_shutdownOrReboot,&QPushButton::clicked,this,[=]{
        UpdateCtrl::instance()->sigRequirePowerAction(
                    UpdateCtrl::instance()->isReboot()
                    );
    });
}

void UpdateResultWidget::setPathname(const QString pathname)
{
    m_shutdownOrReboot->setText(UpdateCtrl::instance()->isReboot()
                                ?tr("Reboot"):tr("Shutdown"));
    m_logFrame->setPathname(pathname);
    setCenterContent(m_failedFrame);
}

void UpdateResultWidget::adjustWidgetSize()
{
    //字体
    UpdateWidgetTool::scaleFontSize(m_exitUpdate,22);
    UpdateWidgetTool::scaleFontSize(m_shutdownOrReboot,22);

    //大小
    m_exitUpdate->setFixedSize(UpdateWidgetTool::scaleSize(QSize(168,56)));
    m_shutdownOrReboot->setFixedSize(UpdateWidgetTool::scaleSize(QSize(168,56)));

    //窗口大小
   // m_stckedWidget->setFixedSize(UpdateWidgetTool::scaleSize(QSize(680,443)));
    m_logFrame->setFixedHeight(UpdateWidgetTool::scaleSize(700));
    m_failedFrame->setFixedHeight(UpdateWidgetTool::scaleSize(700));
    m_bottomBtnFrame->setFixedHeight(UpdateWidgetTool::scaleSize(150));

    m_logFrame->adjustWidgetSize();
    m_failedFrame->adjustWidgetSize();
}

void UpdateResultWidget::setCenterContent(QWidget * const widget)
{
    if (!widget) {
        return;
    }
    QLayoutItem * layoutItem;
    do{
        layoutItem = m_centerLayout->takeAt(0);
        if(layoutItem && layoutItem->widget())
            layoutItem->widget()->setVisible(false);
    }while(layoutItem);

    m_centerLayout->addWidget(widget, 0, Qt::AlignCenter);
    widget->show();
}

UpdatePrepareWidget::UpdatePrepareWidget(QWidget *parent):QFrame(parent)
{
    m_tip = new QLabel(this);
    m_tip->setText(tr("Preparing update..."));

    m_ok = new QPushButton(tr("OK"),this); //确定
    m_ok->setObjectName("update_okBtn");
    m_spinner = new Dtk::Widget::DSpinner(this);
    m_spinner->setFixedSize(72, 72);

    QVBoxLayout *pLayout=new QVBoxLayout(this);
    pLayout->setMargin(0);
    pLayout->addWidget(m_spinner, 0, Qt::AlignCenter);
    pLayout->addSpacing(30);
    pLayout->addWidget(m_tip);
    pLayout->addStretch();
    pLayout->addWidget(m_ok, 0, Qt::AlignCenter);

    connect(m_ok,&QPushButton::clicked,this,[=]{
        emit UpdateCtrl::instance()->sigExitUpdating();
    });
}

void UpdatePrepareWidget::showPrepare(bool failed, QString title, QString tip)
{
    // m_tip->setText(tip);
    m_spinner->setVisible(true);
    m_spinner->start();

    bool visible = failed;
    m_tip->setVisible(true);
    m_ok->setVisible(visible);
}

void UpdatePrepareWidget::adjustWidgetSize()
{
    //字体
    UpdateWidgetTool::scaleFontSize(m_tip,22);
    UpdateWidgetTool::scaleFontSize(m_ok,22);

    //大小
    m_tip->setFixedHeight(UpdateWidgetTool::scaleSize(33));
    m_ok->setFixedSize(UpdateWidgetTool::scaleSize(QSize(124,56)));

    //布局
    int marginTop = UpdateWidgetTool::scaleSize(280);
    int marginBottom = UpdateWidgetTool::scaleSize(95);

    layout()->setContentsMargins(0,marginTop,0,marginBottom);
    layout()->setSpacing(UpdateWidgetTool::scaleSize(32));
}

UpdateProgressWidget::UpdateProgressWidget(QWidget *parent)
{
    m_poetry = new DLabel(this);
    m_poetry->setAlignment(Qt::AlignCenter);
    m_poetry->setText("远上寒山石径斜，白云深处有人家");
    QPalette palette = m_poetry->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    m_poetry->setPalette(palette);
    DFontSizeManager::instance()->bind(m_poetry, DFontSizeManager::T3);

    m_tip = new QLabel(this);
    palette = m_tip->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    m_tip->setPalette(palette);
    DFontSizeManager::instance()->bind(m_tip, DFontSizeManager::T6);

    m_progressBar = new DProgressBar(this);
    m_progressBar->setFixedWidth(500);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setRange(0, 100);
    m_progressBar->setAlignment(Qt::AlignRight);
    m_progressBar->setAccessibleName("ProgressBar");

    m_progressText = new QLabel(this);
    m_progressText->setText("0%");
    DFontSizeManager::instance()->bind(m_progressText, DFontSizeManager::T6);

    QHBoxLayout *pProgressLayout = new QHBoxLayout;
    pProgressLayout->addStretch();
    pProgressLayout->addWidget(m_progressBar, 0, Qt::AlignCenter);
    pProgressLayout->addSpacing(10);
    pProgressLayout->addWidget(m_progressText, 0, Qt::AlignCenter);
    pProgressLayout->addStretch();

    QVBoxLayout *pLayout = new QVBoxLayout(this);
    pLayout->addStretch();
    pLayout->addWidget(m_poetry, 0, Qt::AlignCenter);
    pLayout->addSpacing(100);
    pLayout->addLayout(pProgressLayout, 0);
    pLayout->addSpacing(10);
    pLayout->addWidget(m_tip, 0, Qt::AlignCenter);
    pLayout->addStretch();
}

void UpdateProgressWidget::setSoupHtml(QString pathname)
{
    if (m_soupPathname == pathname)
        return;

    m_soupPathname=pathname;
}

void UpdateProgressWidget::setProgress(double progress)
{
    qInfo() << Q_FUNC_INFO << progress;
    int iProgress = static_cast<int>(progress * 100);
    // 不用在意精度
    if (iProgress > 100 || iProgress < 0)
        return;

    qInfo() << Q_FUNC_INFO << iProgress;
    m_progressBar->setValue(iProgress);
    m_progressText->setText(QString::number(iProgress) + "%");
}

void UpdateProgressWidget::setTip(QString tip)
{
    m_tip->setText(m_isLowerPower?tr("Battery low, please charge now"):tip);
}

void UpdateProgressWidget::setWhetherLowPower(bool isLowerPower)
{
    m_isLowerPower=isLowerPower;
    if (m_isLowerPower){
        setTip(tr("Battery low, please charge now"));
    }
}

void UpdateProgressWidget::adjustWidgetSize()
{

}

UpdateSuccessWidget::UpdateSuccessWidget(QWidget *parent)
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    setPalette(palette);

    m_icon = new Dtk::Widget::DIconButton(this);
    m_icon->setIconSize(QSize(128, 128));
    m_icon->setFlat(true);
    m_icon->setIcon(DStyle::SP_MessageBoxCritical);

    m_title = new QLabel(this);
    m_title->setAlignment(Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(m_title, DFontSizeManager::T4);

    m_tips = new QLabel(this);
    m_tips->setAlignment(Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(m_tips, DFontSizeManager::T6);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setMargin(0);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_icon, 0, Qt::AlignCenter);
    m_mainLayout->addWidget(m_title, 0, Qt::AlignCenter);
    m_mainLayout->addWidget(m_tips,0 , Qt::AlignCenter);

    m_title->setVisible(true);
    m_tips->setVisible(true);
}

void UpdateSuccessWidget::showResult(bool success, UpdateModel::UpdateError error)
{
    qInfo() << Q_FUNC_INFO << success << ", error: " << error;
    if (success) {
        m_title->setText(UpdateCtrl::instance()->isReboot()
                        ?tr("Finished updating, your PC will restart now")  // TODO 翻译 更新已完成，系统即将重启
                        :tr("Finished updating, your PC will shutdown now"));// TODO 翻译 更新已完成，系统即将关机
        return;
    }

    showError(error);
}

void UpdateSuccessWidget::showError(UpdateModel::UpdateError error)
{
    qInfo() << Q_FUNC_INFO << error;
    static const QMap<UpdateModel::UpdateError, QList<UpdateModel::UpdateAction>> ErrorActions = {
        {UpdateModel::CanNotBackup, {UpdateModel::ContinueUpdating, UpdateModel::CancelUpdating}}
    };
    const auto actions = ErrorActions.value(error);
    switch(error) {
        case UpdateModel::UpdateError::CanNotBackup:
            m_title->setText(tr("Can not backup")); // TODO 翻译
            createButtons(actions);
            break;
        default:
            break;
    };
}

void UpdateSuccessWidget::createButtons(QList<UpdateModel::UpdateAction> actions)
{
    qDeleteAll(m_actionButtons);
    m_actionButtons.clear();

    for (auto action : actions) {
        qInfo() << Q_FUNC_INFO << action;
        auto button = new QPushButton(UpdateModel::updateActionText(action), this);
        m_mainLayout->addWidget(button);
        m_actionButtons.append(button);
        connect(button, &QPushButton::clicked, this, [action] {
            UpdateCtrl::instance()->doAction(action);
        });
    }
}

void UpdateSuccessWidget::adjustWidgetSize()
{

}

UpdateWidget::UpdateWidget(QWidget *parent) : QFrame(parent)
{
    initUi();
    initConnections();
}

void UpdateWidget::setModel(SessionBaseModel * const model)
{
    m_model = model;
}

void UpdateWidget::showUpdate()
{
    m_isUpdating=false;
    m_updateFailed = false;
    m_progressWidget->setSoupHtml(UpdateCtrl::instance()->getSoupPathname());

    //显示准备界面
    showChecking();
}

void UpdateWidget::adjustWidgetSize()
{
    /*
     *  LOCK_CONTENT_TOP_WIDGET_HEIGHT = 132; // 顶部控件（日期）的高度  (现在为0)
     *  LOCK_CONTENT_CENTER_LAYOUT_MARGIN = 33; // SessionBaseWindow 中mainlayout的上下间隔
     */
    //总体布局
    m_prepareWidget->adjustWidgetSize();
    m_progressWidget->adjustWidgetSize();
    m_successWidget->adjustWidgetSize();
    m_logWidget->adjustWidgetSize();
}

void UpdateWidget::onUpdateProgress(qint64 totalNum, qint64 curNum, QString packageName, int finished, QString scode, QString messge)
{
    return;
    if (m_updateFailed)
        return;

    qInfo()<<"update: recv:"<<totalNum<<totalNum<<packageName<<finished<<scode<<messge <<" m_isUpdating "<<m_isUpdating;
    UpdateStatusCode code = UpdateStatusCode(scode.toInt());

    switch (code) {
    case Success     :
    case SystemUpgradeInstalled   :
        // 升级成功
        m_stackedWidget->setCurrentWidget(m_successWidget);
        // m_successWidget->showResult();
        setMouseCursorVisible(false);
        emit UpdateCtrl::instance()->sigRequirePowerAction(UpdateCtrl::instance()->isReboot());
        break;

    case SystemUpgradeChecking    :
        showChecking();
        break;

    case SystemUpgradeInstalling  :
        m_isUpdating=true;
        // showProgress(totalNum,curNum,UpdateCtrl::updateCode(code));
        //进度条
        break;

    case NetworkError:
        m_isUpdating ? showLog() : showPrepareFaild(UpdateCtrl::updateCode(code));
        break;
    case DiskError   :
        m_isUpdating ? showLog() : showPrepareFaild(UpdateCtrl::updateCode(code,messge));
        break;
    case PowerError  :
        m_isUpdating ? showLog() : showPrepareFaild(UpdateCtrl::updateCode(code));
        break;

    case Failed      :
    case FailedSystemUpgradeChecked:
    case FailedSystemUpgradeBackuped:
    case FailedSystemUpgradeDownloaded:
    case FailedSystemUpgradeInstalled:
    case SourceError :
    case AptDpkgError:
    case BackupError :
    case SystemError :
    case PackageError:
        showLog();
        break;
    default:
        break;
    }
}

void UpdateWidget::onUpdateStatusChanged(UpdateModel::UpdateStatus status)
{
    qInfo() << Q_FUNC_INFO << status;
    switch (status) {
        case UpdateModel::UpdateStatus::Ready:
            showChecking();
            UpdateCtrl::instance()->doBackup();
            break;
        case UpdateModel::UpdateStatus::BackingUp:
            m_stackedWidget->setCurrentWidget(m_progressWidget);
            break;
        case UpdateModel::UpdateStatus::BackupFailed:
        case UpdateModel::UpdateStatus::BackupSuccess:
            setMouseCursorVisible(true);
            m_stackedWidget->setCurrentWidget(m_successWidget);
            m_successWidget->showResult(status == UpdateModel::UpdateStatus::BackupSuccess, UpdateModel::instance()->updateError());
            break;
        case UpdateModel::UpdateStatus::Installing:
            setMouseCursorVisible(false);
            m_stackedWidget->setCurrentWidget(m_progressWidget);
            m_progressWidget->setTip(tr("Updating, please do not unplug the power supply!"));   // TODO 翻译
            break;
        case UpdateModel::UpdateStatus::InstallSuccess:
            // 升级成功
            m_stackedWidget->setCurrentWidget(m_successWidget);
            m_successWidget->showResult(true);
            setMouseCursorVisible(false);
            break;
        case UpdateModel::UpdateStatus::InstallFailed:
            setMouseCursorVisible(true);
            showLog();
        default:
            break;
    }
}


void UpdateWidget::onExitUpdating()
{
    qInfo() << Q_FUNC_INFO;
    setMouseCursorVisible(true);
    UpdateCtrl::instance()->setIsUpdating(false);

    QWidgetList widgets = qApp->topLevelWidgets();
    for (QWidget *widget : widgets) {
        if (widget->isVisible()) {
            widget->hide();
        }
    }

    m_model->setCurrentModeState(SessionBaseModel::ModeStatus::PasswordMode);
}

void UpdateWidget::initUi()
{
    m_prepareWidget = new UpdatePrepareWidget(this);
    m_progressWidget = new UpdateProgressWidget(this);
    m_successWidget = new UpdateSuccessWidget(this);
    m_logWidget = new UpdateResultWidget(this);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(m_prepareWidget);
    m_stackedWidget->addWidget(m_progressWidget);
    m_stackedWidget->addWidget(m_successWidget);
    m_stackedWidget->addWidget(m_logWidget);

    //布局
    QVBoxLayout * pLayout=new QVBoxLayout(this);
    pLayout->setMargin(0);
    pLayout->setSpacing(0);
    pLayout->addWidget(m_stackedWidget, 0, Qt::AlignCenter);

    for (auto p:findChildren<QLabel*>()){
        p->setAlignment(Qt::AlignCenter);
    }
}

void UpdateWidget::initConnections()
{
    // connect(UpdateCtrl::instance(),&UpdateCtrl::sigUpdateProgress,this,&UpdateWidget::onUpdateProgress);
    connect(UpdateCtrl::instance(),&UpdateCtrl::sigExitUpdating,this,&UpdateWidget::onExitUpdating);

    connect(UpdateCtrl::instance(),&UpdateCtrl::sigServiceNotRun,[=]{showLog();});
    //安装过程中 报告低电
    connect(UpdateCtrl::instance(),&UpdateCtrl::sigWhetherLowPower,[=](bool lowPower){
        m_progressWidget->setWhetherLowPower(lowPower);
    });

    connect(UpdateModel::instance(), &UpdateModel::distUpgradeProgressChanged, m_progressWidget, &UpdateProgressWidget::setProgress);
    connect(UpdateModel::instance(), &UpdateModel::updateStatusChanged, this, &UpdateWidget::onUpdateStatusChanged);
}

void UpdateWidget::showLog()
{
    m_updateFailed=true;
    setMouseCursorVisible(true);

    m_stackedWidget->setCurrentWidget(m_logWidget);
    m_logWidget->setPathname(UpdateCtrl::instance()->GetSystemUpgradeLog());
    //update();
}

void UpdateWidget::showChecking()
{
    setMouseCursorVisible(false);
    m_stackedWidget->setCurrentWidget(m_prepareWidget);
    m_prepareWidget->showPrepare(false, UpdateCtrl::updateCode(SystemUpgradeChecking), "");
}

void UpdateWidget::showPrepareFaild(QString tip)
{
    setMouseCursorVisible(true);
    m_stackedWidget->setCurrentWidget(m_prepareWidget);
    m_prepareWidget->showPrepare(true,tr("Failed to connect to the update service, please check and retry")
                                 ,tip);//无法启动更新服务，请检查后重试
}

void UpdateWidget::showProgress()
{
    setMouseCursorVisible(false);
    m_stackedWidget->setCurrentWidget(m_progressWidget);
    // m_progressWidget->setTip(tip);
}

void UpdateWidget::setMouseCursorVisible( bool visible)
{
    static bool mouseVisible=true;
    if(mouseVisible == visible)
        return;

    mouseVisible= visible;
    qInfo()<<"update: MouseCursorVisble ,"<<mouseVisible;

    //TODO debug
    //return;
    //隐藏鼠标
    //topParentWidget()->setCursor(visible?Qt::ArrowCursor:Qt::BlankCursor);
    QApplication::setOverrideCursor(mouseVisible?Qt::ArrowCursor:Qt::BlankCursor);
}
