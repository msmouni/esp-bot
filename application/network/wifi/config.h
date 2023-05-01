struct IpConfig
{
    const char *ip;
    const char *mask;
    const char *gw;

    IpConfig(const char *ip = NULL,
             const char *mask = NULL,
             const char *gw = NULL) : ip(ip), mask(mask), gw(gw){};
};

struct WifiConfig
{
    wifi_config_t sta_config = {};
    wifi_config_t ap_config = {};
};