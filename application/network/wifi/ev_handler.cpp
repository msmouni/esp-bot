#include "ev_handler.h"

EvHandler::EvHandler(WifiStateHandler *state_handler, const char *log_tag)
{
    m_retry_num = 0;

    m_state_handler = state_handler;
    M_LOG_TAG = log_tag;
}

void EvHandler::handle(void *arg, esp_event_base_t event_base,
                       int32_t event_id, void *event_data)
{
    if (WIFI_EVENT == event_base)
    {
        return wifi_event_handler(arg, event_base, event_id, event_data);
    }
    else if (IP_EVENT == event_base)
    {
        return ip_event_handler(arg, event_base, event_id, event_data);
    }
    else
    {
        ESP_LOGE(M_LOG_TAG, "Unexpected event: %s", event_base);
    }
}

esp_err_t EvHandler::set_dns_server_infos(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
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

void EvHandler::set_static_ip(esp_netif_t *netif, IpConfig *ip_config)
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
    set_dns_server_infos(netif, ipaddr_addr(ip_config->gw), ESP_NETIF_DNS_MAIN);
    set_dns_server_infos(netif, ipaddr_addr(ip_config->gw), ESP_NETIF_DNS_BACKUP);
}

// event handler for wifi events
void EvHandler::wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{

    if (WIFI_EVENT == event_base)
    {
        const wifi_event_t event_type{static_cast<wifi_event_t>(event_id)}; // cast int32_t -> wifi_event_t

        switch (event_type)
        {
        case WIFI_EVENT_STA_START:
        {
            ESP_LOGI(M_LOG_TAG, "Wifi STA started");
            m_state_handler->change_state(WifiState::ReadyToConnect);

            break;
        }

        case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI(M_LOG_TAG, "Wifi STA connected");

            NetworkIface *network_iface = static_cast<NetworkIface *>(arg);

            switch (network_iface->ip_setting)
            {
            case IpSetting::StaticIp:
            {
                m_state_handler->change_state(WifiState::SettingIp);

                set_static_ip(network_iface->netif, &network_iface->ip_config);

                break;
            }
            case IpSetting::Dhcp:
            {
                m_state_handler->change_state(WifiState::WaitingForIp);
            }

            default:
                break;
            }

            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED:
        {
            if (m_retry_num < MAX_FAILURES)
            {
                ESP_LOGI(M_LOG_TAG, "Reconnecting to AP...");

                if (ESP_OK == esp_wifi_connect())
                {
                    m_state_handler->change_state(WifiState::Connecting);

                    m_retry_num++;
                }
            }
            else
            {
                ESP_LOGE(M_LOG_TAG, "Unable to connect to AP after %i attemps", MAX_FAILURES);
                m_state_handler->change_state(WifiState::Error);
            }
        }

        default:
            // TODO STOP and DISCONNECTED, and others
            break;
        }
    }
}

void EvHandler::ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{

    if (IP_EVENT == event_base)
    {
        const ip_event_t event_type{static_cast<ip_event_t>(event_id)};

        switch (event_type)
        {
        case IP_EVENT_STA_GOT_IP:
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

            ////////////////////////////
            /*esp_netif_t *netif = static_cast<esp_netif_t *>(arg);

            esp_netif_dns_info_t dns;
            esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns);

            ESP_LOGI(M_LOG_TAG, "ESP_NETIF_DNS_MAIN %lu", dns.ip.u_addr.ip4.addr); // FE01A8C0: 254.1.168.192*/
            ////////////////////////////

            ESP_LOGI(M_LOG_TAG, "STA IP: " IPSTR, IP2STR(&event->ip_info.ip));

            m_retry_num = 0;

            m_state_handler->change_state(WifiState::Connected);
            break;
        }

        case IP_EVENT_STA_LOST_IP:
        {
            ESP_LOGI(M_LOG_TAG, "STA LOST IP");
            m_state_handler->change_state(WifiState::WaitingForIp);
            break;
        }

        default:
            // TODO IP6
            break;
        }
    }
}