#include "esp_mac.h"
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
        return wifiEventHandler(arg, event_base, event_id, event_data);
    }
    else if (IP_EVENT == event_base)
    {
        return ipEventHandler(arg, event_base, event_id, event_data);
    }
    else
    {
        ESP_LOGE(M_LOG_TAG, "Unexpected event: %s", event_base);
    }
}

// event handler for wifi events
void EvHandler::wifiEventHandler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    //     ESP_LOGI(M_LOG_TAG, "event_base:%s |event_id:%ld", event_base, event_id);

    //     if (event_id == WIFI_EVENT_AP_STACONNECTED)
    //     {
    //         wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    //         ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
    //                  MAC2STR(event->mac), event->aid);
    //     }
    //     else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    //     {
    //         wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    //         ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
    //                  MAC2STR(event->mac), event->aid);
    //     }
    // }

    if (WIFI_EVENT == event_base)
    {
        const wifi_event_t event_type{static_cast<wifi_event_t>(event_id)}; // cast int32_t -> wifi_event_t

        switch (event_type)
        {
        case WIFI_EVENT_STA_START:
        {
            ESP_LOGI(M_LOG_TAG, "Wifi STA started");
            m_state_handler->changeState(WifiState::ReadyToConnect);

            break;
        }

        case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI(M_LOG_TAG, "Wifi STA connected");

            NetworkIface *network_iface = static_cast<NetworkIface *>(arg);

            switch (network_iface->m_setting.m_sta_setting.m_ip_setting)
            {
            case IpSetting::StaticIp:
            {
                m_state_handler->changeState(WifiState::SettingIp);

                network_iface->setStaIp();

                // setStaticIp(network_iface->m_sta_netif, &network_iface->m_setting.m_sta_setting.m_ip_config);

                break;
            }
            case IpSetting::Dhcp:
            {
                m_state_handler->changeState(WifiState::WaitingForIp);
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
                    m_state_handler->changeState(WifiState::Connecting);

                    m_retry_num++;
                }
            }
            else
            {
                ESP_LOGE(M_LOG_TAG, "Unable to connect to AP after %i attemps", MAX_FAILURES);
                m_state_handler->changeState(WifiState::Error);
            }
            break;
        }
        case WIFI_EVENT_AP_START:
        {
            ESP_LOGI(M_LOG_TAG, "Wifi AP started");

            NetworkIface *network_iface = static_cast<NetworkIface *>(arg);

            network_iface->setAPIp();

            break;
        }
        case WIFI_EVENT_AP_STACONNECTED:
        {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(M_LOG_TAG, "station " MACSTR " join, AID=%d",
                     MAC2STR(event->mac), event->aid);
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
            ESP_LOGI(M_LOG_TAG, "station " MACSTR " leave, AID=%d",
                     MAC2STR(event->mac), event->aid);
            break;
        }

        default:
            // TODO STOP and DISCONNECTED, and others
            break;
        }
    }
}

void EvHandler::ipEventHandler(void *arg, esp_event_base_t event_base,
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

            m_state_handler->changeState(WifiState::Connected);
            break;
        }

        case IP_EVENT_STA_LOST_IP:
        {
            ESP_LOGI(M_LOG_TAG, "STA LOST IP");
            m_state_handler->changeState(WifiState::WaitingForIp);
            break;
        }

        default:
            // TODO IP6
            break;
        }
    }
}