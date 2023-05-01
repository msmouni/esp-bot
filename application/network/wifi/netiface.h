#include "esp_netif.h"
#include "setting.h"

struct NetworkIface
{
    esp_netif_t *m_ap_netif;
    esp_netif_t *m_sta_netif;
    const char *M_LOG_TAG;
    WifiSetting m_setting;

    NetworkIface(esp_netif_t *ap_netif = NULL,
                 esp_netif_t *sta_netif = NULL,
                 const char *log_tag = NULL,
                 WifiSetting setting = {}) : m_ap_netif(ap_netif), m_sta_netif(sta_netif), M_LOG_TAG(log_tag), m_setting(setting){};

    esp_err_t setDnsServerInfos(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
    {
        if (addr && (addr != IPADDR_NONE))
        {
            esp_netif_dns_info_t dns;
            dns.ip.u_addr.ip4.addr = addr;
            dns.ip.type = IPADDR_TYPE_V4;
            ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
        }
        return ESP_OK;
    }

    void setIp(esp_netif_t *netif, IpConfig *ip_config)
    {
        if (esp_netif_dhcpc_stop(netif) != ESP_OK)
        {
            ESP_LOGE(M_LOG_TAG, "Failed to stop dhcp client");
            return;
        }
        esp_netif_ip_info_t ip = {};
        memset(&ip, 0, sizeof(esp_netif_ip_info_t));
        ip.ip.addr = ipaddr_addr(ip_config->ip);
        ip.netmask.addr = ipaddr_addr(ip_config->mask);
        ip.gw.addr = ipaddr_addr(ip_config->gw);
        if (esp_netif_set_ip_info(netif, &ip) != ESP_OK)
        {
            ESP_LOGE(M_LOG_TAG, "Failed to set ip info");
            return;
        }

        // As DNS
        // Router Address: GateWay
        setDnsServerInfos(netif, ipaddr_addr(ip_config->gw), ESP_NETIF_DNS_MAIN);
        setDnsServerInfos(netif, ipaddr_addr(ip_config->gw), ESP_NETIF_DNS_BACKUP);
    }

    void setStaIp()
    {
        setIp(m_sta_netif, &m_setting.m_sta_setting.m_ip_config);
    }

    void setAPIp()
    {
        setIp(m_ap_netif, &m_setting.m_ap_setting.m_ip_config);
    }
};