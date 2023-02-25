#include "wifi.h"

/*
Wifi statics

Ref:https://cpp.developpez.com/faq/cpp/?page=Les-donnees-et-fonctions-membres-statiques

https://www.irif.fr/~carton/Enseignement/ObjetsAvances/Cours/simons.pdf
*/

wifi_init_config_t Wifi::m_wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
WifiStateHandler Wifi::m_state_handler = WifiStateHandler();
EvHandler Wifi::m_ev_handler = EvHandler(&Wifi::m_state_handler, Wifi::WIFI_TAG);
// std::mutex *Wifi::m_state_mutex{};

void Wifi::create(SsidPassword ssid_password) //, IpConfig ip_config)
{

    /*// Check if the MAC cstring currently begins with a
    //   nullptr, i.e. is default initialised, not set
    if (!get_mac_cstr()[0])
    {
        // Get the MAC and if this fails restart
        if (ESP_OK != get_mac())
            esp_restart();
    }*/

    // TODO: AP
    m_ssid_password = ssid_password;

    // Copy SSID to config
    const size_t ssid_len_to_copy = std::min(strlen(m_ssid_password.ssid),
                                             sizeof(m_wifi_config.sta.ssid));
    memcpy(m_wifi_config.sta.ssid, m_ssid_password.ssid, ssid_len_to_copy);

    // Copy password to config
    const size_t password_len_to_copy = std::min(strlen(m_ssid_password.password),
                                                 sizeof(m_wifi_config.sta.password));
    memcpy(m_wifi_config.sta.password, m_ssid_password.password, password_len_to_copy);

    m_wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    m_wifi_config.sta.pmf_cfg.capable = true;
    m_wifi_config.sta.pmf_cfg.required = false;

    ESP_LOGI(WIFI_TAG, "CONST  CONFIG SSID: %s | PASS: %s", m_wifi_config.sta.ssid, m_wifi_config.sta.password);
}

// Wifi DHCP Constructor
Wifi::Wifi(DhcpSetting dhcp_setting, ServerConfig server_config)
{

    create(dhcp_setting.ssid_password);

    m_server_config = server_config;

    m_netiface.ip_setting = IpSetting::Dhcp;
}

// Wifi Static Ip Constructor
Wifi::Wifi(StaticIpSetting static_ip_setting, ServerConfig server_config)
{

    create(static_ip_setting.ssid_password);

    m_server_config = server_config;

    m_netiface.ip_config = static_ip_setting.ip_config;

    m_netiface.ip_setting = IpSetting::StaticIp;
}

void Wifi::change_state(WifiState new_state)
{
    m_state_handler.change_state(new_state);
}

void Wifi::handle_event(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    m_ev_handler.handle(arg, event_base, event_id, event_data);
}

// initialize storage
esp_err_t Wifi::init_nvs()
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ret = nvs_flash_erase();

        if (ret == ESP_OK)
        {
            ret = nvs_flash_init();
        }
    }

    return ret;
}

esp_err_t Wifi::init()
{
    esp_err_t status = ESP_OK;

    if (WifiState::NotInitialised == get_state())
    {

        // initialize the esp network interface
        status = esp_netif_init();

        // initialize default esp event loop
        if (ESP_OK == status)
        {
            status = esp_event_loop_create_default();
        }

        // create wifi station in the wifi driver
        m_netiface.netif = esp_netif_create_default_wifi_sta();

        if (ESP_OK == status)
        {
            if (!m_netiface.netif)
                status = ESP_FAIL;
        }

        // setup wifi station with the default wifi configuration
        if (ESP_OK == status)
        {
            status = esp_wifi_init(&m_wifi_init_config);
        }

        // EVENT LOOP
        if (ESP_OK == status)
        {
            status = esp_event_handler_instance_register(WIFI_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &handle_event,
                                                         &m_netiface,
                                                         NULL);
        }

        if (ESP_OK == status)
        {
            status = esp_event_handler_instance_register(IP_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &handle_event,
                                                         m_netiface.netif,
                                                         NULL);
        }

        // set the wifi controller to be a station
        if (ESP_OK == status)
        {
            status = esp_wifi_set_mode(WIFI_MODE_STA); // TODO keep track of mode
        }

        // set the wifi config
        if (ESP_OK == status)
        {
            ESP_LOGI(WIFI_TAG, "STA configuration");

            status = esp_wifi_set_config(WIFI_IF_STA, &m_wifi_config); // TODO keep track of mode
            ESP_LOGI(WIFI_TAG, "STA configured: %d", status);
        }

        // start the wifi driver
        if (ESP_OK == status)
        {
            ESP_LOGI(WIFI_TAG, "Starting STA ...");
            status = esp_wifi_start();
        }

        if ((ESP_OK == status) && (WifiState::NotInitialised == get_state()))
        {
            // In case Event happens before
            change_state(WifiState::Initialised);
        }

        ESP_LOGI(WIFI_TAG, "STA initialization complete");
    }
    else if (WifiState::Error == get_state())
    {
        status = ESP_FAIL;
    }

    return status;
}

esp_err_t Wifi::connect()
{

    esp_err_t status = ESP_OK;

    switch (get_state())
    {
    case WifiState::ReadyToConnect:
        status = esp_wifi_connect();

        if (ESP_OK == status)
        {
            change_state(WifiState::Connecting);
        }
        break;

    default:
        break;
    }

    return status;
}

ServerError Wifi::start_tcp_server()
{
    if (get_state() == WifiState::Connected)
    {
        m_tcp_ip_server = TcpIpServer(ServerSocketDesc(m_netiface.ip_config.ip, m_server_config.socket_port), m_server_config.login);

        // TMP
        while (true)
        {

            m_tcp_ip_server.Update();

            vTaskDelay(100);
        }

        return ServerError::None;
    }
    else
    {
        return ServerError::NotReady;
    }
}

WifiState Wifi::get_state()
{
    // Copy
    return m_state_handler.get_state();
}

void Wifi::log(const char *debug_msg)
{
    ESP_LOGI(WIFI_TAG, "%s", debug_msg);
}
