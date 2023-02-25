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

struct IpConfig
{
    const char *ip;
    const char *mask;
    const char *gw;

    IpConfig(const char *ip = NULL,
             const char *mask = NULL,
             const char *gw = NULL) : ip(ip), mask(mask), gw(gw){};
};

/// Dynamic Host Configuration Protocol
struct DhcpSetting
{
    SsidPassword ssid_password;

    DhcpSetting(SsidPassword ssid_password) : ssid_password(ssid_password){};
};

struct StaticIpSetting
{
    SsidPassword ssid_password;
    IpConfig ip_config;

    StaticIpSetting(SsidPassword ssid_password,
                    IpConfig ip_config) : ssid_password(ssid_password), ip_config(ip_config){};
};