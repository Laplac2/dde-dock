#ifndef NETWORKITEM_H
#define NETWORKITEM_H

#include <DGuiApplicationHelper>
#include <DSwitchButton>
#include <dloadingindicator.h>

#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

class PluginState;
namespace Dock {
class TipsWidget;
}
class WiredItem;
class WirelessItem;
class HorizontalSeperator;
class NetworkItem : public QWidget
{
    Q_OBJECT

    enum PluginState {  // A 无线 B 有线
        Unknow = 0,
        Disabled,
        Connected,
        Disconnected,
        Connecting,
        Failed, //有线无线都失败
        ConnectNoInternet,
        // Aenabled,
        // Benabled,
        Adisabled,
        Bdisabled,
        Aconnected,
        Bconnected,
        Adisconnected,
        Bdisconnected,
        Aconnecting,
        Bconnecting,
        AconnectNoInternet,
        BconnectNoInternet,
        Afailed,
        Bfailed,
        Nocable // 没插网线
    };

public:
    explicit NetworkItem(QWidget *parent = nullptr);

    QWidget *itemApplet();
    QWidget *itemTips();

    void updateDeviceItems(QMap<QString, WiredItem *> &wiredItems, QMap<QString, WirelessItem*> &wirelessItems);

    const QString contextMenu() const;
    void invokeMenuItem(const QString &menuId, const bool checked);
    void refreshTips();
    bool isShowControlCenter();

public slots:
    void updateSelf();
    void refreshIcon();
    void wirelessScan();

protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    bool eventFilter(QObject *obj,QEvent *event) override;
    QString getStrengthStateString(int strength = 0);

private slots:
    void wiredsEnable(bool enable);
    void wirelessEnable(bool enable);
    void onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType);

private:
    void initUi();
    void getPluginState();
    void updateSwitchState();
    void updateView();
    int getStrongestAp();

private:
    Dock::TipsWidget *m_tipsWidget; // 鼠标放在网络Item上的弹窗
    QScrollArea *m_appletScrollArea;          // 点击任务栏网络Item的弹窗

    DSwitchButton *m_wiredSwitchButton;
    QWidget *m_wiredWidget;
    bool m_switchWiredBtnState;

    DLoadingIndicator *m_wirelessLoadingIndicator;
    DSwitchButton *m_switchWirelessBtn;
    QWidget *m_wirelessWidget;
    bool m_switchWirelessBtnState;

    bool m_switchWire;
    //判断定时的时间是否到,否则不重置计时器
    bool m_timeOut;

    QMap<QString, WiredItem *> m_wiredItems;
    QMap<QString, WirelessItem *> m_wirelessItems;
    QMap<QString, WirelessItem *> m_connectedWirelessDevice;
    QMap<QString, WiredItem *> m_connectedWiredDevice;

    QPixmap m_iconPixmap;
    PluginState m_pluginState;
    QTimer *m_timer; // refesh icons timer, 100ms
    QTimer *m_switchWireTimer;
};

#endif // NETWORKITEM_H
