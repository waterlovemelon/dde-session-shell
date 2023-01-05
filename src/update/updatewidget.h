#ifndef UPDATEWIDGET_H
#define UPDATEWIDGET_H

#include "sessionbasemodel.h"
#include "updatemodel.h"

#include <QFrame>
#include <QWidget>
#include <QPushButton>
#include <QStackedLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QTextEdit>
#include <QtWebEngineWidgets/qwebengineview.h>
#include <QVBoxLayout>

#include <DSpinner>
#include <DLabel>
#include <DIconButton>
#include <DFloatingButton>
#include <DProgressBar>

class UpdateWidgetTool{
public:
    //字体缩放
    static void scaleFontSize(QWidget * p,int pixelSize);
    //大小缩放
    static int scaleSize(int e);
    //static int scaleHeight(int height);
    static QSize scaleSize(QSize size);
    static void updateScale(QSize size);

private:
    static double m_scale;
};

class UpdateChooseTip:public QFrame
{
    Q_OBJECT
public:
    UpdateChooseTip(QWidget* parent = nullptr);
    void setChooseType(QPoint movePos,bool isReboot);

signals:
    void sigUpdate(bool isReboot,bool doUpdate);

private:
    QPushButton * pNeedUpdate=nullptr;
    QPushButton * pCancel =nullptr;
    bool m_isReboot;
};

class UpdateFailedWidget: public QFrame
{
    Q_OBJECT
public:
    explicit UpdateFailedWidget(QWidget *parent = nullptr);
    void adjustWidgetSize();

signals:
    void showLog();

private:
    QLabel * m_failedTitle;
    QPushButton * m_showlogBtn;
};

class UpdateShowLogWidget: public QFrame
{
    Q_OBJECT
public:
    explicit UpdateShowLogWidget(QWidget *parent = nullptr);
    void setPathname(const QString pathname);
    void adjustWidgetSize();

signals:
    void backToFailed();

private:
    QFrame * m_center;
    QLabel * m_logFrameTitle;
    QPushButton * m_backToFailedBtn;
    QTextEdit * m_log;
};

class UpdateResultWidget: public QFrame
{
    Q_OBJECT
public:
    explicit UpdateResultWidget(QWidget *parent = nullptr);
    void setPathname(const QString pathname);
    void adjustWidgetSize();

private:
    void setCenterContent(QWidget * const widget);

private:
    QVBoxLayout * m_centerLayout;
    UpdateShowLogWidget * m_logFrame;
    UpdateFailedWidget * m_failedFrame;

    QFrame * m_bottomBtnFrame;
    QPushButton * m_exitUpdate;
    QPushButton * m_shutdownOrReboot;
};



class UpdatePrepareWidget : public QFrame
{
    Q_OBJECT
public:
    explicit UpdatePrepareWidget(QWidget *parent = nullptr);
    void showPrepare(bool failed,QString title, QString tip);
    void adjustWidgetSize();

private:
    QLabel * m_title;
    QLabel * m_tip;
    QPushButton * m_ok;
    Dtk::Widget::DSpinner *m_spinner;
};

class UpdateProgressWidget : public QFrame
{
    Q_OBJECT
public:
    explicit UpdateProgressWidget(QWidget *parent = nullptr);
    void setSoupHtml(QString pathname);
    void setProgress(double progress);
    void setTip(QString tip);
    void setWhetherLowPower(bool isLowerPower);
    void adjustWidgetSize();

private:
    Dtk::Widget::DLabel *m_poetry;
    QLabel *m_tip;
    Dtk::Widget::DProgressBar *m_progressBar;
    QLabel *m_progressText;

    bool m_isLowerPower = false;
    QString m_soupPathname="";
};

class UpdateSuccessWidget : public QFrame
{
    Q_OBJECT
public:
    explicit UpdateSuccessWidget(QWidget *parent = nullptr);
    void showResult(bool success, UpdateModel::UpdateError error = UpdateModel::UpdateError::NoError);
    void adjustWidgetSize();

private:
    void showError(UpdateModel::UpdateError error);
    void createButtons(QList<UpdateModel::UpdateAction> actions);

private:
    Dtk::Widget::DIconButton *m_icon;
    QLabel *m_title;
    QLabel *m_tips;
    QVBoxLayout *m_mainLayout;
    QList<QPushButton *> m_actionButtons;
};

class UpdateWidget : public QFrame
{
    Q_OBJECT

public:

    explicit UpdateWidget(QWidget *parent = nullptr);
    void setModel(SessionBaseModel * const model);
    void showUpdate();
    void adjustWidgetSize();

private slots:
    void onUpdateProgress(qint64 totalNum, qint64 curNum, QString packageName, int finished, QString scode, QString messge);
    void onExitUpdating();
    void onUpdateStatusChanged(UpdateModel::UpdateStatus status);

private:
    void initUi();
    void initConnections();

    // 显示日志界面
    void showLog();
    // 显示“正在准备更新”
    void showChecking();
    // 显示启动更新失败界面
    void showPrepareFaild(QString tip);
    // 显示进度条界面
    void showProgress();

    QWidget *topParentWidget();
    void setMouseCursorVisible(bool visible);

private:
    UpdatePrepareWidget *m_prepareWidget;
    UpdateProgressWidget *m_progressWidget;
    UpdateSuccessWidget *m_successWidget;
    UpdateResultWidget *m_logWidget;
    QStackedWidget *m_stackedWidget;

    //true 更新过程中   false 检查更新中
    bool m_isUpdating;
    //true  就不在切换stacked/处理后端数据
    bool m_updateFailed;

    SessionBaseModel* m_model;
};








#endif // UPDATEWIDGET_H
