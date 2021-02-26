/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             listenerri <listenerri@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "constants.h"
#include "wireditem.h"
#include "applet/horizontalseperator.h"
#include "../widgets/tipswidget.h"
#include "util/utils.h"
#include "util/statebutton.h"
#include "util/imageutil.h"

#include <DGuiApplicationHelper>

#include <QLabel>
#include <QVBoxLayout>

#include <NetworkModel>

DGUI_USE_NAMESPACE

const int ItemHeight = 30;
extern const QString DarkType = "_dark.svg";
extern const QString LightType = ".svg";
extern void initFontColor(QWidget *widget);

WiredItem::WiredItem(WiredDevice *device, const QString &deviceName, QWidget *parent)
    : DeviceItem(device, parent)
    , m_itemWidget(new QWidget)
    , m_deviceName(deviceName)
    , m_itemLabel(new QLabel(this))
    , m_itemIcon(new QLabel(this))
    , m_stateButton(new StateButton(this))
    , m_loadingStat(new DSpinner(this))
    , m_freshWiredIcon(new QTimer(this))
{
    /* init ui */
    QHBoxLayout *itemLayout = new QHBoxLayout(m_itemWidget);
    itemLayout->setContentsMargins(0, 0, 0, 0);
    itemLayout->setSpacing(0);

    itemLayout->addWidget(m_itemIcon, 0, Qt::AlignLeft | Qt::AlignVCenter);

    m_itemLabel->setText(m_deviceName);
    initFontColor(m_itemLabel);
    itemLayout->addWidget(m_itemLabel , 1, Qt::AlignLeft | Qt::AlignVCenter);

    m_stateButton->setFixedSize(PLUGIN_ICON_MAX_SIZE, PLUGIN_ICON_MAX_SIZE);
    m_stateButton->setType(StateButton::Check);
    itemLayout->addWidget(m_stateButton, 0, Qt::AlignRight | Qt::AlignVCenter);

    m_loadingStat->setFixedSize(PLUGIN_ICON_MAX_SIZE, PLUGIN_ICON_MAX_SIZE);
    itemLayout->addWidget(m_loadingStat, 0, Qt::AlignRight | Qt::AlignVCenter);
    setFixedHeight(30);

    /* init connection */
    connect(m_freshWiredIcon, &QTimer::timeout, this, &WiredItem::setWiredStateIcon);
    connect(m_device, static_cast<void (NetworkDevice::*)(const bool) const>(&NetworkDevice::enableChanged),
            this, &WiredItem::enableChanged);
    connect(m_device, static_cast<void (NetworkDevice::*)(NetworkDevice::DeviceStatus) const>(&NetworkDevice::statusChanged),
            this, &WiredItem::deviceStateChanged);
    connect(m_device, static_cast<void (NetworkDevice::*)(NetworkDevice::DeviceStatus) const>(&NetworkDevice::statusChanged),
            this, &WiredItem::setWiredStateIcon);
    connect(m_device, static_cast<void (NetworkDevice::*)(bool) const>(&NetworkDevice::enableChanged),
            this, &WiredItem::setWiredStateIcon);

    connect(static_cast<WiredDevice *>(m_device.data()), &WiredDevice::activeWiredConnectionInfoChanged,
            this, &WiredItem::changedActiveWiredConnectionInfo);

    connect(m_stateButton, &StateButton::click, this, [&] {
        auto enableState = m_device->enabled();
        emit requestSetDeviceEnable(path(), !enableState);
    });

    deviceStateChanged(m_device->status());
    setWiredStateIcon();
}

QWidget *WiredItem::itemApplet()
{
    return m_itemWidget;
}

void WiredItem::setTitle(const QString &name)
{
    if (m_device->status() != NetworkDevice::Activated)
        m_itemLabel->setText(name);
    m_deviceName = name;
}

bool WiredItem::deviceEabled()
{
    return m_device->enabled();
}

void WiredItem::setDeviceEnabled(bool enabled)
{
    emit requestSetDeviceEnable(path(), enabled);
}

WiredItem::WiredStatus WiredItem::getDeviceState()
{
    if (!m_device->enabled()) {
        return Disabled;
    }
    if (m_device->status() == NetworkDevice::Activated
            && NetworkModel::connectivity() != Connectivity::Full) {
        return ConnectNoInternet;
    }
    if (m_device->obtainIpFailed()) {
        return ObtainIpFailed;
    }

    switch (m_device->status()) {
        case NetworkDevice::Unknown:       return Unknown;
        case NetworkDevice::Unmanaged:
        case NetworkDevice::Unavailable:   return Nocable;
        case NetworkDevice::Disconnected:  return Disconnected;
        case NetworkDevice::Prepare:
        case NetworkDevice::Config:        return Connecting;
        case NetworkDevice::NeedAuth:      return Authenticating;
        case NetworkDevice::IpConfig:
        case NetworkDevice::IpCheck:
        case NetworkDevice::Secondaries:   return ObtainingIP;
        case NetworkDevice::Activated:     return Connected;
        case NetworkDevice::Deactivation:
        case NetworkDevice::Failed:        return Failed;
    }
    Q_UNREACHABLE();
}

QJsonObject WiredItem::getActiveWiredConnectionInfo()
{
    return static_cast<WiredDevice *>(m_device.data())->activeWiredConnectionInfo();
}

void WiredItem::setThemeType(DGuiApplicationHelper::ColorType themeType)
{
    bool isLight = (themeType == DGuiApplicationHelper::LightType);

    auto pixpath = QString(":/wired/resources/wired/network-wired-symbolic");
    pixpath = isLight ? pixpath + "-dark.svg" : pixpath +  LightType;
    auto iconPix = Utils::renderSVG(pixpath, QSize(PLUGIN_ICON_MAX_SIZE, PLUGIN_ICON_MAX_SIZE), devicePixelRatioF());
    m_itemIcon->setPixmap(iconPix);
}

void WiredItem::setWiredStateIcon()
{
    QPixmap pixmap;
    QString iconString;
    QString stateString;

    auto ratio =  devicePixelRatioF();

    switch (m_deviceState) {
        case NetworkDevice::Unknown:
        case NetworkDevice::Unmanaged:
        case NetworkDevice::Unavailable: {
            stateString = "error";
            iconString = QString("network-%1-symbolic").arg(stateString);
        }
            break;
        case NetworkDevice::Disconnected: {
            stateString = "none";
            iconString = QString("network-%1-symbolic").arg(stateString);
        }
            break;
        case NetworkDevice::Deactivation:
        case NetworkDevice::Failed: {
            stateString = "offline";
            iconString = QString("network-%1-symbolic").arg(stateString);
        }
            break;
        case NetworkDevice::Prepare:
        case NetworkDevice::Config:
        case NetworkDevice::NeedAuth:
        case NetworkDevice::IpConfig:
        case NetworkDevice::IpCheck:
        case NetworkDevice::Secondaries: {
            m_freshWiredIcon->start(200);
            const int index = QTime::currentTime().msec() / 200 % 10;
            const int num = index + 1;
            qDebug() << num;
            iconString = QString("network-wired-symbolic-connecting%1").arg(num);
            if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
                iconString.append(PLUGIN_MIN_ICON_NAME);
            pixmap = ImageUtil::loadSvg(iconString, ":/", PLUGIN_ICON_MAX_SIZE, ratio);
            m_itemIcon->setPixmap(pixmap);
            update();
            return;
        }
        case NetworkDevice::Activated: {
            stateString = "online";
            iconString = QString("network-%1-symbolic").arg(stateString);
        }
            break;
    }

    m_freshWiredIcon->stop();

    if (m_deviceState == NetworkDevice::Activated && NetworkModel::connectivity() != Connectivity::Full) {
        stateString = "warning";
        iconString = QString("network-%1-symbolic").arg(stateString);
    }

    if (!m_device->enabled()) {
        stateString = "disabled";
        iconString = QString("network-%1-symbolic").arg(stateString);
    }

    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
        iconString.append(PLUGIN_MIN_ICON_NAME);

    pixmap = ImageUtil::loadSvg(iconString, ":/", PLUGIN_ICON_MAX_SIZE, ratio);
    m_itemIcon->setPixmap(pixmap);
    update();
}

void WiredItem::refreshConnectivity()
{
    setWiredStateIcon();
}

void WiredItem::deviceStateChanged(NetworkDevice::DeviceStatus state)
{
    m_deviceState = state;
    switch (state) {
        case NetworkDevice::Unknown:
        case NetworkDevice::Unmanaged:
        case NetworkDevice::Unavailable:
        case NetworkDevice::Disconnected:
        case NetworkDevice::Deactivation:
        case NetworkDevice::Failed: {
            m_loadingStat->stop();
            m_loadingStat->hide();
            m_loadingStat->setVisible(false);
            if (!m_device->enabled())
                m_stateButton->setVisible(false);
        }
        break;
        case NetworkDevice::Prepare:
        case NetworkDevice::Config:
        case NetworkDevice::NeedAuth:
        case NetworkDevice::IpConfig:
        case NetworkDevice::IpCheck:
        case NetworkDevice::Secondaries: {
            m_stateButton->setVisible(false);
            m_loadingStat->setVisible(true);
            m_loadingStat->start();
            m_loadingStat->show();
        }
        break;
        case NetworkDevice::Activated: {
            m_loadingStat->stop();
            m_loadingStat->hide();
            m_loadingStat->setVisible(false);
            m_stateButton->setVisible(true);
        }
        break;
    }

    emit wiredStateChanged();
}

void WiredItem::changedActiveWiredConnectionInfo(const QJsonObject &connInfo)
{
    if (connInfo.isEmpty()) {
        m_stateButton->setVisible(false);
    } else {
        m_stateButton->setVisible(true);
    }

    auto strTitle = connInfo.value("ConnectionName").toString();
    m_itemLabel->setText(strTitle);
    QFontMetrics fontMetrics(m_itemLabel->font());
    if (fontMetrics.width(strTitle) > m_itemLabel->width()) {
        strTitle = QFontMetrics(m_itemLabel->font()).elidedText(strTitle, Qt::ElideRight, m_itemLabel->width());
    }

    if (strTitle.isEmpty())
        m_itemLabel->setText(m_deviceName);
    else
        m_itemLabel->setText(strTitle);

    emit activeConnectionChanged();
}
