#include "esp_netif.h"
#include "setting.h"

struct NetworkIface
{
    esp_netif_t *netif;
    IpSetting ip_setting;
    IpConfig ip_config;

    NetworkIface(esp_netif_t *netif = NULL,
                 IpSetting ip_setting = {},
                 IpConfig ip_config = {}) : netif(netif), ip_setting(ip_setting), ip_config(ip_config){};
};