#include "config.h"

struct SsidPassword
{
    const char *ssid;
    const char *password;

    SsidPassword(const char *ssid = NULL,
                 const char *password = NULL) : ssid(ssid), password(password){};
};

enum class IpSetting
{
    StaticIp,
    /// Dynamic Host Configuration Protocol
    Dhcp
};

/// Dynamic Host Configuration Protocol
struct DhcpSetting
{
    SsidPassword ssid_password;

    DhcpSetting(SsidPassword ssid_password = {}) : ssid_password(ssid_password){};
};

struct StaticIpSetting
{
    SsidPassword ssid_password;
    IpConfig ip_config;

    StaticIpSetting(SsidPassword ssid_password = {},
                    IpConfig ip_config = {}) : ssid_password(ssid_password), ip_config(ip_config){};
};

struct StaSetting
{
    SsidPassword m_ssid_password;
    IpSetting m_ip_setting;
    IpConfig m_ip_config;

    StaSetting(StaticIpSetting static_ip_setting) : m_ssid_password(static_ip_setting.ssid_password),
                                                    m_ip_setting(IpSetting::StaticIp),
                                                    m_ip_config(static_ip_setting.ip_config){};
    StaSetting(DhcpSetting dhcp_setting = {}) : m_ssid_password(dhcp_setting.ssid_password),
                                                m_ip_setting(IpSetting::Dhcp){};
};

struct ApSetting
{
    SsidPassword m_ssid_password;
    IpConfig m_ip_config;

    ApSetting(SsidPassword ssid_password = {},
              IpConfig ip_config = {}) : m_ssid_password(ssid_password), m_ip_config(ip_config){};
};

struct WifiSetting
{
    ApSetting m_ap_setting;
    StaSetting m_sta_setting;

    WifiSetting(ApSetting ap_setting = {}, StaSetting sta_setting = {}) : m_ap_setting(ap_setting), m_sta_setting(sta_setting){};
};