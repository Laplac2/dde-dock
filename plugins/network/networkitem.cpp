#include "networkitem.h"
#include "item/wireditem.h"
#include "item/wirelessitem.h"
#include "../../widgets/tipswidget.h"
#include "../frame/util/imageutil.h"

#include <DHiDPIHelper>
#include <DApplicationHelper>
#include <DDBusSender>

#include <QVBoxLayout>
#include <QJsonDocument>

const int DeviceItemHeight = 36;
const int ItemWidth = 300;
const int TitleHeight = 46;
const int TitleSpace = 2;
const int MaxDeviceCount = 8;
extern const int ItemMargin;
extern const int ItemHeight;
const int ControlItemHeight = 35;
const QString MenueEnable = "enable";
const QString MenueWiredEnable = "wireEnable";
const QString MenueWirelessEnable = "wirelessEnable";
const QString MenueSettings = "settings";

extern void initFontColor(QWidget *widget)
{
    if (!widget)
        return;

    auto fontChange = [&](QWidget * widget) {
        QPalette defaultPalette = widget->palette();
        defaultPalette.setBrush(QPalette::WindowText, defaultPalette.brightText());
        widget->setPalette(defaultPalette);
    };

    fontChange(widget);

    QObject::connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, widget, [ = ] {
        fontChange(widget);
    });
}

NetworkItem::NetworkItem(QWidget *parent)
    : QWidget(parent)
    , m_tipsWidget(new Dock::TipsWidget(this))
    , m_appletScrollArea(new QScrollArea(this))
    , m_switchWire(true)
    , m_timeOut(true)
    , m_timer(new QTimer(this))
    , m_switchWireTimer(new QTimer(this))
{
    m_timer->setInterval(100);

    m_tipsWidget->setVisible(false);

    m_switchWirelessBtnState = false;
    m_switchWiredBtnState = false;

    initUi();

    m_wirelessLoadingIndicator->installEventFilter(this);

    connect(m_switchWireTimer, &QTimer::timeout, [ = ] {
        m_switchWire = !m_switchWire;
        m_timeOut = true;
    });
    connect(m_timer, &QTimer::timeout, this, &NetworkItem::refreshIcon);
    connect(m_wiredSwitchButton, &DSwitchButton::toggled, this, &NetworkItem::wiredsEnable);
    connect(m_switchWirelessBtn, &DSwitchButton::toggled, this, &NetworkItem::wirelessEnable);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &NetworkItem::onThemeTypeChanged);
}

void NetworkItem::initUi()
{
    QFont labelFont = QFont(font().family(), font().pointSize() + 2);
    QPixmap pixmap = DHiDPIHelper::loadNxPixmap(":/wireless/resources/wireless/refresh.svg");

    QWidget *appletContentWidget = new QWidget(this); // contents
    appletContentWidget->setMinimumSize(250, 35);
    QVBoxLayout *appletContentLayout = new QVBoxLayout(appletContentWidget);
    appletContentLayout->setContentsMargins(0, 0, 0, 0);
    appletContentLayout->setSpacing(0);

    /* wireless */
    m_wirelessWidget = new QWidget(this); // wireless widget
    m_wirelessWidget->setMinimumHeight(35);
    QVBoxLayout *wirelessLayout = new QVBoxLayout(m_wirelessWidget); // wireless layout
    wirelessLayout->setContentsMargins(0, 0, 0, 0);
    wirelessLayout->setSpacing(1);
    QWidget *wirelessSwitchWidget = new QWidget(m_wirelessWidget); // wireless switch widget
    wirelessSwitchWidget->setMinimumHeight(35);
    QHBoxLayout *wirelessSwitchLayout = new QHBoxLayout(wirelessSwitchWidget); // wireless title and switch button loyout
    wirelessSwitchLayout->setContentsMargins(0, 0, 0, 0);
    wirelessSwitchLayout->setSpacing(2);

    QLabel *wirelessLabel = new QLabel(m_wirelessWidget); // wireless title
    wirelessLabel->setText(tr("Wireless Network"));
    wirelessLabel->setFont(labelFont);
    initFontColor(wirelessLabel);
    wirelessSwitchLayout->addWidget(wirelessLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);

    m_wirelessLoadingIndicator = new DLoadingIndicator(m_wirelessWidget); // wireless loading indicator
    m_wirelessLoadingIndicator->setLoading(false);
    m_wirelessLoadingIndicator->setSmooth(true);
    m_wirelessLoadingIndicator->setAniDuration(1000);
    m_wirelessLoadingIndicator->setAniEasingCurve(QEasingCurve::InOutCirc);
    m_wirelessLoadingIndicator->setFixedSize(pixmap.size() / devicePixelRatioF());
    m_wirelessLoadingIndicator->viewport()->setAutoFillBackground(false);
    m_wirelessLoadingIndicator->setFrameShape(QFrame::NoFrame);
    wirelessSwitchLayout->addWidget(m_wirelessLoadingIndicator, 0, Qt::AlignRight | Qt::AlignVCenter);

    m_switchWirelessBtn = new DSwitchButton(m_wirelessWidget); // wireless switch button
    wirelessSwitchLayout->addWidget(m_switchWirelessBtn, 0, Qt::AlignRight | Qt::AlignVCenter);

    wirelessLayout->addWidget(wirelessSwitchWidget, 0, Qt::AlignVCenter);
    m_wirelessWidget->hide();
    appletContentLayout->addWidget(m_wirelessWidget);

    /* wired */
    m_wiredWidget = new QWidget(this); // wired widget
    m_wiredWidget->resize(250, 100);
    m_wiredWidget->setMinimumHeight(35);
    QVBoxLayout *wiredLayout = new QVBoxLayout(m_wiredWidget);
    wiredLayout->setContentsMargins(0, 0, 0, 0);
    wiredLayout->setSpacing(10);

    QWidget *wiredSwitchWidget = new QWidget(m_wiredWidget); // wired tittle and switch button widget
    wiredSwitchWidget->setMinimumHeight(35);
    QHBoxLayout *wiredSwitchLayout = new QHBoxLayout(wiredSwitchWidget);
    wiredSwitchLayout->setContentsMargins(0, 0, 0, 0);
    wiredSwitchLayout->setSpacing(0);

    QLabel *wiredLabel = new QLabel(m_wiredWidget); // wired label
    wiredLabel->setText(tr("Wired Network"));
    wiredLabel->setFont(labelFont);
    initFontColor(wiredLabel);
    wiredSwitchLayout->addWidget(wiredLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);

    m_wiredSwitchButton = new DSwitchButton(m_wiredWidget); // wired switch button
    wiredSwitchLayout->addWidget(m_wiredSwitchButton, 0, Qt::AlignRight | Qt::AlignVCenter);

    wiredLayout->addWidget(wiredSwitchWidget, 0, Qt::AlignVCenter); // add wired switch widget to wired widget
    appletContentLayout->addWidget(m_wiredWidget); // add wired widget to scroll area content widget

    m_appletScrollArea->setWidget(appletContentWidget);
    appletContentWidget->setAutoFillBackground(false);
    m_appletScrollArea->viewport()->setAutoFillBackground(false);
    m_appletScrollArea->resize(250, 130);
    m_appletScrollArea->setMaximumSize(250, 370);
    m_appletScrollArea->setWidgetResizable(true);
    m_appletScrollArea->setFrameShape(QFrame::NoFrame);
    m_appletScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_appletScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_appletScrollArea->setVisible(false);

}

QWidget *NetworkItem::itemApplet()
{
    m_appletScrollArea->setVisible(true);
    return m_appletScrollArea;
}

QWidget *NetworkItem::itemTips()
{
    return m_tipsWidget;
}

/**
 * @brief 根据返回的网卡设备列表，更新现有的列表
 *
 * @param wiredItems 有线设备
 * @param wirelessItems 无线设备
 */
void NetworkItem::updateDeviceItems(QMap<QString, WiredItem *> &wiredItems, QMap<QString, WirelessItem *> &wirelessItems)
{
    /* 猜测传过来的是当前完整的设备列表，下面根据之前的列表与现有列表比较，剔除没用了的设备 */
    auto tempWiredItems = m_wiredItems;
    auto tempWirelessItems = m_wirelessItems;

    for (auto wirelessItem : wirelessItems) {
        if (wirelessItem) {
            auto path = wirelessItem->path();
            if (m_wirelessItems.contains(path)) {
                m_wirelessItems.value(path)->setDeviceInfo(wirelessItem->deviceInfo());
                tempWirelessItems.remove(path);
                delete wirelessItem;
            } else {
                wirelessItem->setParent(this);
                m_wirelessItems.insert(path, wirelessItem);
            }
        }
    }

    for (auto wirelessItem : tempWirelessItems) {
        if (wirelessItem) {
            auto path = wirelessItem->device()->path();
            m_wirelessItems.remove(path);
            m_connectedWirelessDevice.remove(path); // 从已连接成功的列表中去除没用了的设备
            wirelessItem->itemApplet()->setVisible(false);
            m_wirelessWidget->layout()->removeWidget(wirelessItem->itemApplet());
            delete wirelessItem;
        }
    }

    for (auto wiredItem : wiredItems) {
        if (wiredItem) {
            auto path = wiredItem->path();
            if (m_wiredItems.contains(path)) {
                m_wiredItems.value(path)->setTitle(wiredItem->deviceName());
                tempWiredItems.remove(path);
                delete wiredItem;
            } else {
                wiredItem->setParent(this);
                m_wiredItems.insert(path, wiredItem);
                wiredItem->setVisible(true);
                m_wiredWidget->layout()->addWidget(wiredItem->itemApplet());
            }
        }
    }

    for (auto wiredItem : tempWiredItems) {
        if (wiredItem) {
            auto path = wiredItem->device()->path();
            m_wiredItems.remove(path);
            m_connectedWiredDevice.remove(path);
            wiredItem->setVisible(false);
            m_wiredWidget->layout()->removeWidget(wiredItem->itemApplet());
            delete wiredItem;
        }
    }

    updateSelf();
}

const QString NetworkItem::contextMenu() const
{
    QList<QVariant> items;

    if (m_wirelessItems.size() && m_wiredItems.size()) {
        items.reserve(3);
        QMap<QString, QVariant> wireEnable;
        wireEnable["itemId"] = MenueWiredEnable;
        if (m_switchWiredBtnState)
            wireEnable["itemText"] = tr("Disable wired connection");
        else
            wireEnable["itemText"] = tr("Enable wired connection");
        wireEnable["isActive"] = true;
        items.push_back(wireEnable);

        QMap<QString, QVariant> wirelessEnable;
        wirelessEnable["itemId"] = MenueWirelessEnable;
        if (m_switchWirelessBtnState)
            wirelessEnable["itemText"] = tr("Disable wireless connection");
        else
            wirelessEnable["itemText"] = tr("Enable wireless connection");
        wirelessEnable["isActive"] = true;
        items.push_back(wirelessEnable);
    } else {
        items.reserve(2);
        QMap<QString, QVariant> enable;
        enable["itemId"] = MenueEnable;
        if (m_switchWiredBtnState || m_switchWirelessBtnState)
            enable["itemText"] = tr("Disable network");
        else
            enable["itemText"] = tr("Enable network");
        enable["isActive"] = true;
        items.push_back(enable);
    }

    QMap<QString, QVariant> settings;
    settings["itemId"] = MenueSettings;
    settings["itemText"] = tr("Network settings");
    settings["isActive"] = true;
    items.push_back(settings);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;

    return QJsonDocument::fromVariant(menu).toJson();
}

/**
 * @brief 调用菜单项（暂时没被调用）
 *
 * @param menuId
 * @param checked
 */
void NetworkItem::invokeMenuItem(const QString &menuId, const bool checked)
{
    Q_UNUSED(checked);

    if (menuId == MenueEnable) {
        wiredsEnable(!m_switchWiredBtnState);
        wirelessEnable(!m_switchWirelessBtnState);
    } else if (menuId == MenueWiredEnable)
        wiredsEnable(!m_switchWiredBtnState);
    else if (menuId == MenueWirelessEnable)
        wirelessEnable(!m_switchWirelessBtnState);
    else if (menuId == MenueSettings)
        DDBusSender()
        .service("com.deepin.dde.ControlCenter")
        .interface("com.deepin.dde.ControlCenter")
        .path("/com/deepin/dde/ControlCenter")
        .method(QString("ShowModule"))
        .arg(QString("network"))
        .call();
}

/**
 * @brief 根据网络连接状态显示图标
 *
 */
void NetworkItem::refreshIcon()
{
    QPixmap pixmap; // 刷新按钮
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
        pixmap = DHiDPIHelper::loadNxPixmap(":/wireless/resources/wireless/refresh_dark.svg");
    else
        pixmap = DHiDPIHelper::loadNxPixmap(":/wireless/resources/wireless/refresh.svg");
    m_wirelessLoadingIndicator->setImageSource(pixmap); // 加载指示器

    QString stateString;
    QString iconString;
    const auto ratio = devicePixelRatioF();
    int iconSize = PLUGIN_ICON_MAX_SIZE;
    int strength = 0;

    switch (m_pluginState) {
    case Disabled:
    case Adisabled:
        stateString = "disabled";
        iconString = QString("wireless-%1-symbolic").arg(stateString);
        break;
    case Bdisabled:
        stateString = "disabled";
        iconString = QString("network-%1-symbolic").arg(stateString);
        break;
    case Connected:
    case Aconnected:
        strength = getStrongestAp();
        stateString = getStrengthStateString(strength);
        iconString = QString("wireless-%1-symbolic").arg(stateString);
        break;
    case Bconnected:
        stateString = "online";
        iconString = QString("network-%1-symbolic").arg(stateString);
        break;
    case Disconnected:
    case Adisconnected:
        stateString = "0";
        iconString = QString("wireless-%1-symbolic").arg(stateString);
        break;
    case Bdisconnected:
        stateString = "none";
        iconString = QString("network-%1-symbolic").arg(stateString);
        break;
    case Connecting: {
        m_timer->start();
        if (m_switchWire) {
            strength = QTime::currentTime().msec() / 10 % 100;
            stateString = getStrengthStateString(strength);
            iconString = QString("wireless-%1-symbolic").arg(stateString);
            if (height() <= PLUGIN_BACKGROUND_MIN_SIZE
                    && DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
                iconString.append(PLUGIN_MIN_ICON_NAME);
            m_iconPixmap = ImageUtil::loadSvg(iconString, ":/", iconSize, ratio);
            update();
            return;
        } else {
            m_timer->start(200);
            const int index = QTime::currentTime().msec() / 200 % 10;
            const int num = index + 1;
            iconString = QString("network-wired-symbolic-connecting%1").arg(num);
            if (height() <= PLUGIN_BACKGROUND_MIN_SIZE
                    && DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
                iconString.append(PLUGIN_MIN_ICON_NAME);
            m_iconPixmap = ImageUtil::loadSvg(iconString, ":/", iconSize, ratio);
            update();
            return;
        }
    }
    case Aconnecting: {
        m_timer->start();
        strength = QTime::currentTime().msec() / 10 % 100;
        stateString = getStrengthStateString(strength);
        iconString = QString("wireless-%1-symbolic").arg(stateString);
        if (height() <= PLUGIN_BACKGROUND_MIN_SIZE
                && DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
            iconString.append(PLUGIN_MIN_ICON_NAME);
        m_iconPixmap = ImageUtil::loadSvg(iconString, ":/", iconSize, ratio);
        update();
        return;
    }
    case Bconnecting: {
        m_timer->start(200);
        const int index = QTime::currentTime().msec() / 200 % 10;
        const int num = index + 1;
        iconString = QString("network-wired-symbolic-connecting%1").arg(num);
        if (height() <= PLUGIN_BACKGROUND_MIN_SIZE
                && DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
            iconString.append(PLUGIN_MIN_ICON_NAME);
        m_iconPixmap = ImageUtil::loadSvg(iconString, ":/", iconSize, ratio);
        update();
        return;
    }
    case ConnectNoInternet:  // 有线、无线已连接，但是无法访问互联网
    case AconnectNoInternet: // 无线已连接但无法访问互联网 offline
        stateString = "offline";
        iconString = QString("network-wireless-%1-symbolic").arg(stateString);
        break;
    case BconnectNoInternet: // 有线已连接，但是无法访问互联网
        stateString = "warning";
        iconString = QString("network-%1-symbolic").arg(stateString);
        break;
    case Bfailed: // 有线连接失败none变为offline
        stateString = "offline";
        iconString = QString("network-%1-symbolic").arg(stateString);
        break;
    case Unknow:
    case Nocable:             // 没插网线
        stateString = "error";//待图标 暂用错误图标
        iconString = QString("network-%1-symbolic").arg(stateString);
        break;
    case Afailed: // 无线连接失败
    case Failed:  // 无线连接失败改为 disconnect
        stateString = "disconnect";
        iconString = QString("wireless-%1").arg(stateString);
    }

    m_timer->stop();

    if (height() <= PLUGIN_BACKGROUND_MIN_SIZE && DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
        iconString.append(PLUGIN_MIN_ICON_NAME);

    m_iconPixmap = ImageUtil::loadSvg(iconString, ":/", iconSize, ratio);

    update();
}

void NetworkItem::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();
    // 保持横纵比
    if (position == Dock::Bottom || position == Dock::Top) {
        setMaximumWidth(height());
        setMaximumHeight(QWIDGETSIZE_MAX);
    } else {
        setMaximumHeight(width());
        setMaximumWidth(QWIDGETSIZE_MAX);
    }

    refreshIcon();
}

void NetworkItem::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter painter(this);
    const QRectF &rf = rect();
    const QRectF &rfp = QRectF(m_iconPixmap.rect());
    painter.drawPixmap(rf.center() - rfp.center() / m_iconPixmap.devicePixelRatioF(),
                       m_iconPixmap);
}

bool NetworkItem::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_wirelessLoadingIndicator) {
        if (event->type() == QEvent::MouseButtonPress) {
            for (auto wirelessItem : m_wirelessItems) {
                if (wirelessItem) {
                    wirelessItem->requestWirelessScan();
                }
            }
            wirelessScan();
        }
    }
    return false;
}

QString NetworkItem::getStrengthStateString(int strength)
{
    if (5 >= strength)
        return "0";
    else if (5 < strength && 30 >= strength)
        return "20";
    else if (30 < strength && 55 >= strength)
        return "40";
    else if (55 < strength && 65 >= strength)
        return "60";
    else if (65 < strength)
        return "80";
    else
        return "0";
}

void NetworkItem::wiredsEnable(bool enable)
{
    for (auto wiredItem : m_wiredItems) {
        if (wiredItem) {
            wiredItem->setDeviceEnabled(enable);
            wiredItem->setVisible(enable);
        }
    }
}

void NetworkItem::wirelessEnable(bool enable)
{
    for (auto wirelessItem : m_wirelessItems) {
        if (wirelessItem) {
            wirelessItem->setDeviceEnabled(enable);
            enable ? m_wirelessWidget->layout()->addWidget(wirelessItem->itemApplet())
            : m_wirelessWidget->layout()->removeWidget(wirelessItem->itemApplet());
            wirelessItem->itemApplet()->setVisible(enable);
        }
    }
}

void NetworkItem::onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType)
{
    for (auto wiredItem : m_wiredItems) {
        wiredItem->setThemeType(themeType);
    }
    refreshIcon();
}

void NetworkItem::getPluginState()
{
    int wiredState = 0;
    int wirelessState = 0;
    int state = 0; // 只要有一个设备连上了，这个标志为就会置1
    int temp = 0;  // 存的是设备的连接状态

    /* 所有设备状态叠加，最后确定总连接状态，并将连接了的设备存起来 */
    QMapIterator<QString, WirelessItem *> iwireless(m_wirelessItems);
    while (iwireless.hasNext()) {
        iwireless.next();
        auto wirelessItem = iwireless.value();
        if (wirelessItem) {
            temp = wirelessItem->getDeviceState();
            state |= temp;
            if ((temp & WirelessItem::Connected) >> 18) {
                m_connectedWirelessDevice.insert(iwireless.key(), wirelessItem);
            } else {
                m_connectedWirelessDevice.remove(iwireless.key());
            }
        }
    }

    /* 按如下顺序解析所有设备的状态，并将最有进展的状态存起来 */
    temp = state;
    if (!temp)
        wirelessState = WirelessItem::Unknown;
    temp = state;
    if ((temp & WirelessItem::Disabled) >> 17)
        wirelessState = WirelessItem::Disabled;
    temp = state;
    if ((temp & WirelessItem::Disconnected) >> 19)
        wirelessState = WirelessItem::Disconnected;
    temp = state;
    if ((temp & WirelessItem::Connecting) >> 20)
        wirelessState = WirelessItem::Connecting;
    temp = state;
    if ((temp & WirelessItem::ConnectNoInternet) >> 24)
        wirelessState = WirelessItem::ConnectNoInternet;
    temp = state;
    if ((temp & WirelessItem::Connected) >> 18) {
        wirelessState = WirelessItem::Connected;
    }
    //将无线获取地址状态中显示为连接中状态
    temp = state;
    if ((temp & WirelessItem::ObtainingIP) >> 22) {
        wirelessState = WirelessItem::ObtainingIP;
    }
    //无线正在认证
    temp = state;
    if ((temp & WirelessItem::Authenticating) >> 17) {
        wirelessState = WirelessItem::Authenticating;
    }

    state = 0;
    temp = 0;
    QMapIterator<QString, WiredItem *> iwired(m_wiredItems);
    while (iwired.hasNext()) {
        iwired.next();
        auto wiredItem = iwired.value();
        if (wiredItem) {
            temp = wiredItem->getDeviceState();
            state |= temp;
            if ((temp & WiredItem::Connected) >> 2) {
                m_connectedWiredDevice.insert(iwired.key(), wiredItem);
            } else {
                m_connectedWiredDevice.remove(iwired.key());
            }
        }
    }
    temp = state;
    if (!temp)
        wiredState = WiredItem::Unknown;
    temp = state;
    if ((temp & WiredItem::Nocable) >> 9)
        wiredState = WiredItem::Nocable;
    temp = state;
    if ((temp & WiredItem::Disabled) >> 1)
        wiredState = WiredItem::Disabled;
    temp = state;
    if ((temp & WiredItem::Disconnected) >> 3)
        wiredState = WiredItem::Disconnected;
    temp = state;
    if ((temp & WiredItem::Connecting) >> 4)
        wiredState = WiredItem::Connecting;
    temp = state;
    if ((temp & WiredItem::ConnectNoInternet) >> 8)
        wiredState = WiredItem::ConnectNoInternet;
    temp = state;
    if ((temp & WiredItem::Connected) >> 2) {
        wiredState = WiredItem::Connected;
    }
    //将有线获取地址状态中显示为连接中状态
    temp = state;
    if ((temp & WiredItem::ObtainingIP) >> 6) {
        wiredState = WiredItem::ObtainingIP;
    }
    //有线正在认证
    temp = state;
    if ((temp & WiredItem::Authenticating) >> 5) {
        wiredState = WiredItem::Authenticating;
    }

    /* 将无线和有线网络的连接状态取或，最终获取网络连接状态 */
    switch (wirelessState | wiredState) {
    case 0:
        m_pluginState = Unknow;
        break;
    case 0x00000001:
        m_pluginState = Bdisconnected;
        break;
    case 0x00000002:
        m_pluginState = Bdisabled;
        break;
    case 0x00000004:
        m_pluginState = Bconnected;
        break;
    case 0x00000008:
        m_pluginState = Bdisconnected;
        break;
    case 0x00000010: //有线正在连接
        m_pluginState = Bconnecting;
        break;
    case 0x00000020: //有线正在认证
        m_pluginState = Bconnecting;
        break;
    case 0x00000040: //有线正在获取ip转换为正在连接
        m_pluginState = Bconnecting;
        break;
    case 0x00000080:
        m_pluginState = Bdisconnected;
        break;
    case 0x00000100:
        m_pluginState = BconnectNoInternet;
        break;
    case 0x00000200:
        m_pluginState = Nocable;
        break;
    case 0x00000400: //只有有线,有线失败
        m_pluginState = Bfailed;
        break;
    case 0x00010000:
        m_pluginState = Adisconnected;
        break;
    case 0x00020000:
        m_pluginState = Adisabled;
        break;
    case 0x00040000:
        m_pluginState = Aconnected;
        break;
    case 0x00080000:
        m_pluginState = Adisconnected;
        break;
    case 0x00100000: //无线正在连接
        m_pluginState = Aconnecting;
        break;
    case 0x00200000: //无线正在认证
        m_pluginState = Aconnecting;
        break;
    case 0x00400000: //无线正在获取ip转换为正在连接
        m_pluginState = Aconnecting;
        break;
    case 0x00800000: //无线获取ip失败
        m_pluginState = Adisconnected;
        break;
    case 0x01000000:
        m_pluginState = AconnectNoInternet;
        break;
    case 0x02000000: // 只有无线 Adisconnected(无线未连接) 改为 Afailed(无线连接失败)
        m_pluginState = Afailed;
        break;
    case 0x00010001:
        m_pluginState = Disconnected;
        break;
    case 0x00020001:
        m_pluginState = Bdisconnected;//
        break;
    case 0x00040001:
        m_pluginState = Aconnected;
        break;
    case 0x00080001:
        m_pluginState = Disconnected;
        break;
    case 0x00100001: //无线正在连接,有线启用
        m_pluginState = Aconnecting;
        break;
    case 0x00200001: //无线正在认证, 有线启用
        m_pluginState = Aconnecting;
        break;
    case 0x00400001: //无线正在获取ip,有线启用
        m_pluginState = Aconnecting;
        break;
    case 0x00800001:
        m_pluginState = Disconnected;
        break;
    case 0x01000001:
        m_pluginState = AconnectNoInternet;
        break;
    case 0x02000001:
        m_pluginState = Disconnected;
        break;
    case 0x00010002:
        m_pluginState = Adisconnected;
        break;
    case 0x00020002: //有线无线都禁用
        m_pluginState = Disabled;
        break;
    case 0x00040002:
        m_pluginState = Aconnected;
        break;
    case 0x00080002:
        m_pluginState = Adisconnected;
        break;
    case 0x00100002: //无线正在连接,有线禁用
        m_pluginState = Aconnecting;
        break;
    case 0x00200002: //无线正在认证,有线禁用
        m_pluginState = Aconnecting;
        break;
    case 0x00400002: //无线正在获取ip,有线禁用
        m_pluginState = Aconnecting;
        break;
    case 0x00800002: //有线禁用,无线未连接
        m_pluginState = Adisconnected;
        break;
    case 0x01000002:
        m_pluginState = AconnectNoInternet;
        break;
    case 0x02000002: //有线禁用,无线连接失败,设为无线连接失败  Adisconnected换成Afailed
        m_pluginState = Afailed;
        break;
    case 0x00010004:
        m_pluginState = Bconnected;
        break;
    case 0x00020004:
        m_pluginState = Bconnected;
        break;
    case 0x00040004: //无线已连接,有线已连接
        m_pluginState = Connected;
        break;
    case 0x00080004: //无线断开连接,有线已连接,状态改为有线已连接
        m_pluginState = Bconnected;
        break;
    case 0x00100004: //无线正在连接,有线已连接
        m_pluginState = Aconnecting;
        break;
    case 0x00200004: // 无线认证中.有线已连接,
        m_pluginState = Aconnecting;
        break;
    case 0x00400004: //无线正在获取ip,有线已连接
        m_pluginState = Aconnecting;
        break;
    case 0x00800004:
        m_pluginState = Bconnected;
        break;
    case 0x01000004:
        m_pluginState = Bconnected;
        break;
    case 0x02000004:
        m_pluginState = Bconnected;
        break;
    case 0x00010008:
        m_pluginState = Disconnected;
        break;
    case 0x00020008:
        m_pluginState = Bdisconnected;
        break;
    case 0x00040008: //无线已连接,有线连接失败
        m_pluginState = Aconnected;
        break;
    case 0x00080008:
        m_pluginState = Disconnected;
        break;
    case 0x00100008: //无线正在连接,有线断开连接
        m_pluginState = Aconnecting;
        break;
    case 0x00200008: //无线正在认证,有线断开连接
        m_pluginState = Aconnecting;
        break;
    case 0x00400008:  //无线正在获取ip,有线断开连接
        m_pluginState = Aconnecting;
        break;
    case 0x00800008:
        m_pluginState = Disconnected;
        break;
    case 0x01000008:
        m_pluginState = AconnectNoInternet;
        break;
    case 0x02000008:
        m_pluginState = Disconnected;
        break;
    case 0x00010010:
        m_pluginState = Bconnecting;
        break;
    case 0x00020010:
        m_pluginState = Bconnecting;
        break;
    case 0x00040010: //有线正在连接, 无线已连接
        m_pluginState = Bconnecting;
        break;
    case 0x00080010:
        m_pluginState = Bconnecting;
        break;
    case 0x00100010: //无线正在连接,有线正在连接
        m_pluginState = Connecting;
        break;
    case 0x00200010: //无线正在认证, 有线正在连接
        m_pluginState = Connecting;
        break;
    case 0x00400010:  //无线正在获取ip,有线正在连接
        m_pluginState = Connecting;
        break;
    case 0x00800010:
        m_pluginState = Bconnecting;
        break;
    case 0x01000010:
        m_pluginState = Bconnecting;
        break;
    case 0x02000010:
        m_pluginState = Bconnecting;
        break;
    case 0x00010020: //有线正在连接认证 ,无线其余操作
        m_pluginState = Bconnecting;
        break;
    case 0x00020020:
        m_pluginState = Bconnecting;
        break;
    case 0x00040020: //有线正在认证, 无线已连接
        m_pluginState = Bconnecting;
        break;
    case 0x00080020://无线断开连接,有线正在认证
        m_pluginState = Bconnecting;
        break;
    case 0x00100020: //无线正在连接,有线正在认证
        m_pluginState = Connecting;
        break;
    case 0x00200020: //无线正在认证,有线正在认证
        m_pluginState = Connecting;
        break;
    case 0x00400020: // 无线正在获取ip ,有线正在认证
        m_pluginState = Connecting;
        break;
    case 0x00800020: // 无线获取ip失败 ,有线正在认证(仅有线正在连接,显示有线状态)
        m_pluginState = Bconnecting;
        break;
    case 0x01000020: // 无线连上但无法访问 ,有线正在认证
        m_pluginState = Bconnecting;
        break;
    case 0x02000020: // 无线连接失败 ,有线正在认证
        m_pluginState = Bconnecting;
        break;
    case 0x00010040: //有线正在获取ip,无线启用
        m_pluginState = Bconnecting;
        break;
    case 0x00020040:
        m_pluginState = Bconnecting;
        break;
    case 0x00040040: //有线正在获取ip,无线已连接
        m_pluginState = Bconnecting;
        break;
    case 0x00080040:
        m_pluginState = Bconnecting;
        break;
    case 0x00100040: //无线正在连接,有线正在获取ip
        m_pluginState = Connecting;
        break;
    case 0x00200040: //无线正在认证.有线正在获取ip
        m_pluginState = Connecting;
        break;
    case 0x00400040: // 无线正在获取ip ,有线正在获取ip
        m_pluginState = Connecting;
        break;
    case 0x00800040: //有线正在获取ip,无线获取ip失败
        m_pluginState = Bconnecting;
        break;
    case 0x01000040:
        m_pluginState = Bconnecting;
        break;
    case 0x02000040:
        m_pluginState = Bconnecting;
        break;
    case 0x00010080:
        m_pluginState = Disconnected;
        break;
    case 0x00020080:
        m_pluginState = Bdisconnected;
        break;
    case 0x00040080:
        m_pluginState = Aconnected;
        break;
    case 0x00080080:
        m_pluginState = Disconnected;
        break;
    case 0x00100080: //无线正在连接,有线获取ip失败
        m_pluginState = Aconnecting;
        break;
    case 0x00200080: //无线正在认证,有线获取ip失败
        m_pluginState = Aconnecting;
        break;
    case 0x00400080: // 无线正在获取ip ,有线获取ip失败
        m_pluginState = Aconnecting;
        break;
    case 0x00800080:
        m_pluginState = Disconnected;
        break;
    case 0x01000080:
        m_pluginState = AconnectNoInternet;
        break;
    case 0x02000080:
        m_pluginState = Disconnected;
        break;
    case 0x00010100:
        m_pluginState = BconnectNoInternet;
        break;
    case 0x00020100:
        m_pluginState = BconnectNoInternet;
        break;
    case 0x00040100:
        m_pluginState = Aconnected;
        break;
    case 0x00080100:
        m_pluginState = BconnectNoInternet;
        break;
    case 0x00100100: //无线正在连接,有线已连接但无法访问
        m_pluginState = Aconnecting;
        break;
    case 0x00200100: //无线正在认证,有线连接但无法访问
        m_pluginState = Aconnecting;
        break;
    case 0x00400100: // 无线正在获取ip ,有线已连接但无法访问
        m_pluginState = Aconnecting;
        break;
    case 0x00800100:
        m_pluginState = BconnectNoInternet;
        break;
    case 0x01000100:
        m_pluginState = ConnectNoInternet;
        break;
    case 0x02000100:
        m_pluginState = BconnectNoInternet;
        break;
    case 0x00010200:
        m_pluginState = Adisconnected;
        break;
    case 0x00020200:
        m_pluginState = Nocable;
        break;
    case 0x00040200:
        m_pluginState = Aconnected;
        break;
    case 0x00080200:
        m_pluginState = Adisconnected;
        break;
    case 0x00100200: //无线正在连接,未插入网线
        m_pluginState = Aconnecting;
        break;
    case 0x00200200: //无线正在认证,有线未插入网线
        m_pluginState = Aconnecting;
        break;
    case 0x00400200: // 无线正在获取ip ,有线未插入网线
        m_pluginState = Aconnecting;
        break;
    case 0x00800200:
        m_pluginState = Adisconnected;
        break;
    case 0x01000200:
        m_pluginState = AconnectNoInternet;
        break;
    case 0x02000200:
        m_pluginState = Adisconnected;
        break;
    case 0x00010400:
        m_pluginState = Adisconnected;
        break;
    case 0x00020400: //有线失败,无线禁用
        m_pluginState = Bfailed;
        break;
    case 0x00040400:
        m_pluginState = Aconnected;
        break;
    case 0x00080400:
        m_pluginState = Adisconnected;
        break;
    case 0x00100400: //无线正在连接,有线连接失败
        m_pluginState = Aconnecting;
        break;
    case 0x00200400: //无线正在认证,有线连接失败
        m_pluginState = Aconnecting;
        break;
    case 0x00400400: // 无线正在获取ip ,有线连接失败
        m_pluginState = Aconnecting;
        break;
    case 0x00800400:
        m_pluginState = Adisconnected;
        break;
    case 0x01000400:
        m_pluginState = AconnectNoInternet;
        break;
    case 0x02000400: //有线,无线都连接失败,改为无线连接失败
        m_pluginState = Failed;
        break;
    }

    switch (m_pluginState) {
    case Unknow:
    case Disabled:
    case Connected:
    case Disconnected:
    case ConnectNoInternet:
    case Adisabled:
    case Bdisabled:
    case Aconnected:
    case Bconnected:
    case Adisconnected:
    case Bdisconnected:
    case Aconnecting:
    case Bconnecting:
    case AconnectNoInternet:
    case BconnectNoInternet:
    case Bfailed:
    case Nocable:
        m_switchWireTimer->stop();
        m_timeOut = true;
        break;
    case Connecting:
        // 启动2s切换计时,只有当计时器记满则重新计数
        if (m_timeOut) {
            m_switchWireTimer->start(2000);
            m_timeOut = false;
        }
        break;
    }
}

void NetworkItem::updateView()
{
    m_appletScrollArea->widget()->resize(250, 50);
    m_appletScrollArea->resize(250, 30);
    return;

    // 固定显示高度即为固定示项目数
    const int constDisplayItemCnt = 5;
    int contentHeight = 0;
    int itemCount = 0;

    auto wirelessCnt = m_wirelessItems.size();
    if (m_switchWirelessBtnState) {
        for (auto wirelessItem : m_wirelessItems) {
            if (wirelessItem) {
                if (wirelessItem->device()->enabled())
                    itemCount += wirelessItem->APcount();
                // 单个设备开关控制项
                if (wirelessCnt == 1) {
                    wirelessItem->setControlPanelVisible(false);
                    continue;
                } else {
                    wirelessItem->setControlPanelVisible(true);
                }
                itemCount++;
            }
        }
    }
    // 设备总控开关只与是否有设备相关
    auto wirelessDeviceCnt = m_wirelessItems.size();
    if (wirelessDeviceCnt)
        contentHeight += m_wirelessWidget->height();
    m_wirelessWidget->setVisible(wirelessDeviceCnt);

    auto wiredDeviceCnt = m_wiredItems.size();
    if (wiredDeviceCnt)
        contentHeight += m_wiredWidget->height();
    m_wiredWidget->setVisible(wiredDeviceCnt);

    itemCount += wiredDeviceCnt;

    // 分割线 都有设备时才有
//    auto hasDevice = wirelessDeviceCnt && wiredDeviceCnt;
//    m_line->setVisible(hasDevice);

    auto centralWidget = m_appletScrollArea->widget();
    if (itemCount <= constDisplayItemCnt) {
        contentHeight += (itemCount - wiredDeviceCnt) * ItemHeight;
        contentHeight += wiredDeviceCnt * ItemHeight;
//        centralWidget->setFixedHeight(contentHeight);
//        m_appletScrollArea->setFixedHeight(contentHeight);
        centralWidget->resize(250, contentHeight);
        m_appletScrollArea->resize(250, contentHeight);
    } else {
        contentHeight += (itemCount - wiredDeviceCnt) * ItemHeight;
        contentHeight += wiredDeviceCnt * ItemHeight;
//        centralWidget->setFixedHeight(contentHeight);
//        m_appletScrollArea->setFixedHeight(constDisplayItemCnt * ItemHeight);
//        m_appletScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    }
}

void NetworkItem::updateSelf()
{
    getPluginState();
    updateSwitchState();
    refreshIcon();
    refreshTips();
    updateView();
}

int NetworkItem::getStrongestAp()
{
    int retStrength = -1;
    for (auto wirelessItem : m_connectedWirelessDevice) {
        auto apInfo = wirelessItem->getConnectedApInfo();
        if (apInfo.isEmpty())
            continue;
        auto strength = apInfo.value("Strength").toInt();
        if (retStrength < strength)
            retStrength = strength;
    }
    return retStrength;
}

/**
 * @brief 更新有线（无线）适配器的开关状态，并根据开关状态显示设备列表。
 */
void NetworkItem::updateSwitchState()
{
    m_switchWiredBtnState = false;
    m_switchWirelessBtnState = false;

    /* 获取有线适配器启用状态 */
    for (WiredItem *wiredItem : m_wiredItems) {
        if (wiredItem && wiredItem->deviceEabled()) {
            m_switchWiredBtnState = wiredItem->deviceEabled();
            break;
        }
    }
    /* 更新有线适配器总开关状态（阻塞信号是为了防止重复设置适配器启用状态）*/
    m_wiredSwitchButton->blockSignals(true);
    m_wiredSwitchButton->setChecked(m_switchWiredBtnState);
    m_wiredSwitchButton->blockSignals(false);
    /* 根据有线适配器启用状态增/删布局中的组件 */
    for (WiredItem *wiredItem : m_wiredItems) {
        if (!wiredItem) {
            continue;
        }
        if (m_switchWiredBtnState) {
             m_wiredWidget->layout()->addWidget(wiredItem->itemApplet());
        } else {
             m_wiredWidget->layout()->removeWidget(wiredItem->itemApplet());
        }
        wiredItem->itemApplet()->setVisible(m_switchWiredBtnState); // TODO
        wiredItem->setVisible(m_switchWiredBtnState);
    }

    /* 获取无线适配器启用状态 */
    for (auto wirelessItem : m_wirelessItems) {
        if (wirelessItem && wirelessItem->deviceEanbled()) {
            m_switchWirelessBtnState = wirelessItem->deviceEanbled();
            break;
        }
    }
    /* 更新无线适配器总开关状态（阻塞信号是为了防止重复设置适配器启用状态） */
    m_switchWirelessBtn->blockSignals(true);
    m_switchWirelessBtn->setChecked(m_switchWirelessBtnState);
    m_switchWirelessBtn->blockSignals(false);
    /* 根据无线适配器启用状态增/删布局中的组件 */
    for (WirelessItem *wirelessItem : m_wirelessItems) {
        if (!wirelessItem) {
            continue;
        }
        if (m_switchWirelessBtnState) {
            m_wirelessWidget->layout()->addWidget(wirelessItem->itemApplet());
        } else {
            m_wirelessWidget->layout()->removeWidget(wirelessItem->itemApplet());
        }
        wirelessItem->itemApplet()->setVisible(m_switchWirelessBtnState);
        wirelessItem->setVisible(m_switchWirelessBtnState);
    }

    m_wirelessLoadingIndicator->setVisible(m_switchWirelessBtnState || m_switchWiredBtnState);
}

void NetworkItem::refreshTips()
{
    switch (m_pluginState) {
    case Disabled:
    case Adisabled:
    case Bdisabled:
        m_tipsWidget->setText(tr("Device disabled"));
        break;
    case Connected: {
        QString strTips;
        QStringList textList;
        int wirelessIndex = 1;
        int wireIndex = 1;
        for (auto wirelessItem : m_connectedWirelessDevice) {
            if (wirelessItem) {
                auto info = wirelessItem->getActiveWirelessConnectionInfo();
                if (!info.contains("Ip4"))
                    continue;
                const QJsonObject ipv4 = info.value("Ip4").toObject();
                if (!ipv4.contains("Address"))
                    continue;
                if (m_connectedWirelessDevice.size() == 1) {
                    strTips = tr("Wireless connection: %1").arg(ipv4.value("Address").toString()) + '\n';
                } else {
                    strTips = tr("Wireless Network").append(QString("%1").arg(wirelessIndex++)).append(":"+ipv4.value("Address").toString()) + '\n';
                }
                strTips.chop(1);
                textList << strTips;
            }
        }
        for (auto wiredItem : m_connectedWiredDevice) {
            if (wiredItem) {
                auto info = wiredItem->getActiveWiredConnectionInfo();
                if (!info.contains("Ip4"))
                    continue;
                const QJsonObject ipv4 = info.value("Ip4").toObject();
                if (!ipv4.contains("Address"))
                    continue;
                if (m_connectedWiredDevice.size() == 1) {
                    strTips = tr("Wired connection: %1").arg(ipv4.value("Address").toString()) + '\n';
                } else {
                    strTips = tr("Wired Network").append(QString("%1").arg(wireIndex++)).append(":"+ipv4.value("Address").toString()) + '\n';
                }
                strTips.chop(1);
                textList << strTips;
            }
        }
        m_tipsWidget->setTextList(textList);
    }
    break;
    case Aconnected: {
        QString strTips;
        int wirelessIndex=1;
        QStringList textList;
        for (auto wirelessItem : m_connectedWirelessDevice) {
            if (wirelessItem) {
                auto info = wirelessItem->getActiveWirelessConnectionInfo();
                if (!info.contains("Ip4"))
                    continue;
                const QJsonObject ipv4 = info.value("Ip4").toObject();
                if (!ipv4.contains("Address"))
                    continue;
                if (m_connectedWirelessDevice.size() == 1) {
                    strTips = tr("Wireless connection: %1").arg(ipv4.value("Address").toString()) + '\n';
                } else {
                    strTips = tr("Wireless Network").append(QString("%1").arg(wirelessIndex++)).append(":"+ipv4.value("Address").toString()) + '\n';
                }
                strTips.chop(1);
                textList << strTips;
            }
        }
        m_tipsWidget->setTextList(textList);
    }
    break;
    case Bconnected: {
        QString strTips;
        QStringList textList;
        int wireIndex = 1;
        for (auto wiredItem : m_connectedWiredDevice) {
            if (wiredItem) {
                auto info = wiredItem->getActiveWiredConnectionInfo();
                if (!info.contains("Ip4")) {
                    textList << "BConnected, but don't contains IPv4.\n";
                    continue;
                }
                const QJsonObject ipv4 = info.value("Ip4").toObject();
                if (!ipv4.contains("Address")) {
                    textList << "BConnected, contains IPv4, but don't contains Address.\n";
                    continue;
                }
                if (m_connectedWiredDevice.size() == 1) {
                    strTips = tr("Wired connection: %1").arg(ipv4.value("Address").toString()) + '\n';
                } else {
                    strTips = tr("Wired Network").append(QString("%1").arg(wireIndex++)).append(":"+ipv4.value("Address").toString()) + '\n';
                }
                strTips.chop(1);
                textList << strTips;
            }
        }
        m_tipsWidget->setTextList(textList);
    }
    break;
    case Disconnected:
    case Adisconnected:
    case Bdisconnected:
        m_tipsWidget->setText(tr("Not connected"));
        break;
    case Connecting:
    case Aconnecting:
    case Bconnecting: {
        m_tipsWidget->setText(tr("Connecting"));
        return;
    }
    case ConnectNoInternet:
    case AconnectNoInternet:
    case BconnectNoInternet:
        m_tipsWidget->setText(tr("Connected but no Internet access"));
        break;
    case Failed:
    case Afailed:
    case Bfailed:
        m_tipsWidget->setText(tr("Connection failed"));
        break;
    case Unknow:
    case Nocable:
        m_tipsWidget->setText(tr("Network cable unplugged"));
        break;
    }
}

bool NetworkItem::isShowControlCenter()
{
    bool onlyOneTypeDevice = false;
    if ((m_wiredItems.size() == 0 && m_wirelessItems.size() > 0)
            || (m_wiredItems.size() > 0 && m_wirelessItems.size() == 0))
        onlyOneTypeDevice = true;

    if (onlyOneTypeDevice) {
        switch (m_pluginState) {
        case Unknow:
        case Nocable:
        case Bfailed:
        case AconnectNoInternet:
        case BconnectNoInternet:
        case Adisconnected:
        case Bdisconnected:
        case Adisabled:
        case Bdisabled:
            return true;
        default:
            break;
        }
    } else {
        switch (m_pluginState) {
        case Unknow:
        case Nocable:
        case Bfailed:
        case ConnectNoInternet:
        case Disconnected:
        case Disabled:
            return true;
        default:
            break;
        }
    }

    return false;
}

void NetworkItem::wirelessScan()
{
    if (m_wirelessLoadingIndicator->loading())
        return;
    m_wirelessLoadingIndicator->setLoading(true);
    QTimer::singleShot(1000, this, [ = ] {
        m_wirelessLoadingIndicator->setLoading(false);
    });
}
