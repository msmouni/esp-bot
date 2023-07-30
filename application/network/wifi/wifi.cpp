#include "wifi.h"

/*
Wifi statics

Ref:https://cpp.developpez.com/faq/cpp/?page=Les-donnees-et-fonctions-membres-statiques

https://www.irif.fr/~carton/Enseignement/ObjetsAvances/Cours/simons.pdf
*/

wifi_init_config_t Wifi::m_wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
WifiStateHandler Wifi::m_state_handler = WifiStateHandler();
EvHandler Wifi::m_ev_handler = EvHandler(&Wifi::m_state_handler, Wifi::WIFI_TAG);

void Wifi::configure_sta(StaSetting &sta_setting)
{

    // Copy SSID to config
    const size_t ssid_len_to_copy = std::min(strlen(sta_setting.m_ssid_password.ssid),
                                             sizeof(m_config.sta_config.sta.ssid));
    memcpy(m_config.sta_config.sta.ssid, sta_setting.m_ssid_password.ssid, ssid_len_to_copy);

    // Copy password to config
    const size_t password_len_to_copy = std::min(strlen(sta_setting.m_ssid_password.password),
                                                 sizeof(m_config.sta_config.sta.password));
    memcpy(m_config.sta_config.sta.password, sta_setting.m_ssid_password.password, password_len_to_copy);

    m_config.sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    m_config.sta_config.sta.pmf_cfg.capable = true;
    m_config.sta_config.sta.pmf_cfg.required = false;

    ESP_LOGI(WIFI_TAG, "STA  CONFIG SSID: %s | PASS: %s", m_config.sta_config.sta.ssid, m_config.sta_config.sta.password);
}

void Wifi::configure_ap(ApSetting &ap_setting)
{
    // Copy SSID to config
    const size_t ssid_len = std::min(strlen(ap_setting.m_ssid_password.ssid),
                                     sizeof(m_config.ap_config.ap.ssid));
    memcpy(m_config.ap_config.ap.ssid, ap_setting.m_ssid_password.ssid, ssid_len);

    // Copy password to config
    const size_t password_len = std::min(strlen(ap_setting.m_ssid_password.password),
                                         sizeof(m_config.ap_config.ap.password));
    memcpy(m_config.ap_config.ap.password, ap_setting.m_ssid_password.password, password_len);

    m_config.ap_config.ap.ssid_len = ssid_len;
    m_config.ap_config.ap.channel = 1; // range 1 13
    if (password_len == 0)
    {
        m_config.ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    else
    {

        m_config.ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }
    m_config.ap_config.ap.max_connection = TcpIpServer::NB_ALLOWED_CLIENTS; // Maximal STA connections
    m_config.ap_config.ap.pmf_cfg.required = false;

    ESP_LOGI(WIFI_TAG, "AP  CONFIG SSID: %s | PASS: %s", m_config.ap_config.ap.ssid, m_config.ap_config.ap.password);
}

void Wifi::configure(WifiSetting &setting) //, IpConfig ip_config)
{

    /*// Check if the MAC cstring currently begins with a
    //   nullptr, i.e. is default initialized, not set
    if (!get_mac_cstr()[0])
    {
        // Get the MAC and if this fails restart
        if (ESP_OK != getMac())
            esp_restart();
    }*/

    // configure_ap(setting.m_ap_setting);
    configure_sta(setting.m_sta_setting);
}
Wifi::Wifi(WifiSetting setting, ServerConfig server_config)
{
    configure(setting);

    m_server_config = server_config;

    m_netiface.m_setting = setting;
}

void Wifi::changeState(WifiState new_state)
{
    m_state_handler.changeState(new_state);
}

void Wifi::handleEvent(void *arg, esp_event_base_t event_base,
                       int32_t event_id, void *event_data)
{
    m_ev_handler.handle(arg, event_base, event_id, event_data);
}

esp_err_t Wifi::init()
{
    esp_err_t status = ESP_OK;

    if (WifiState::NotInitialized == getState())
    {

        // initialize the esp network interface
        status = esp_netif_init();

        /*// create wifi access point in the wifi driver
        m_netiface.m_ap_netif = esp_netif_create_default_wifi_ap();*/

        // create wifi station in the wifi driver
        m_netiface.m_sta_netif = esp_netif_create_default_wifi_sta();

        if (ESP_OK == status)
        {
            if (!m_netiface.m_sta_netif)
            //((!m_netiface.m_ap_netif) || (!m_netiface.m_sta_netif))
            {
                status = ESP_FAIL;
            }
        }

        // setup wifi STA_AP with the default wifi configuration
        if (ESP_OK == status)
        {
            status = esp_wifi_init(&m_wifi_init_config);
        }

        // EVENT LOOP
        if (ESP_OK == status)
        {
            status = esp_event_handler_instance_register(WIFI_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &handleEvent,
                                                         &m_netiface,
                                                         NULL);
        }

        if (ESP_OK == status)
        {
            status = esp_event_handler_instance_register(IP_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &handleEvent,
                                                         &m_netiface,
                                                         NULL);
        }

        // Set Wifi storage to RAM: The default value is WIFI_STORAGE_FLASH
        if (ESP_OK == status)
        {
            status = esp_wifi_set_storage(WIFI_STORAGE_RAM);
        }

        // set the wifi controller to be a station
        if (ESP_OK == status)
        {
            status = esp_wifi_set_mode(WIFI_MODE_STA); // APSTA);
        }

        // set the wifi AP config
        /*if (ESP_OK == status)
        {
            ESP_LOGI(WIFI_TAG, "AP configuration");
            status = esp_wifi_set_config(WIFI_IF_AP, &m_config.ap_config);
            ESP_LOGI(WIFI_TAG, "AP configured: %d", status);
        }*/

        // set the wifi STA config
        if (ESP_OK == status)
        {
            ESP_LOGI(WIFI_TAG, "STA configuration");
            status = esp_wifi_set_config(WIFI_IF_STA, &m_config.sta_config);
            ESP_LOGI(WIFI_TAG, "STA configured: %d", status);
        }

        // start the wifi driver
        if (ESP_OK == status)
        {
            ESP_LOGI(WIFI_TAG, "Starting STA ...");
            status = esp_wifi_start();
        }

        if ((ESP_OK == status) && (WifiState::NotInitialized == getState()))
        {
            // In case Event happens before
            changeState(WifiState::Initialized);
        }

        ESP_LOGI(WIFI_TAG, "STA initialization complete");
    }
    else if (WifiState::Error == getState())
    {
        status = ESP_FAIL;
    }

    return status;
}

esp_err_t Wifi::connect()
{

    esp_err_t status = ESP_OK;

    switch (getState())
    {
    case WifiState::ReadyToConnect:
        status = esp_wifi_connect();

        if (ESP_OK == status)
        {
            changeState(WifiState::Connecting);
        }
        break;

    default:
        break;
    }

    return status;
}

ServerError Wifi::startTcpServer()
{
    if (getState() == WifiState::Connected)
    {
        // TODO: check if STA/AP has started
        Option<ServerSocketDesc> ap_socket_desc = Option<ServerSocketDesc>(ServerSocketDesc(m_netiface.m_setting.m_ap_setting.m_ip_config.ip, m_server_config.m_ap_socket_port));
        Option<ServerSocketDesc> sta_socket_desc = Option<ServerSocketDesc>(ServerSocketDesc(m_netiface.m_setting.m_sta_setting.m_ip_config.ip, m_server_config.m_sta_socket_port));

        m_tcp_ip_server.start(ApStaSocketsDesc(ap_socket_desc, sta_socket_desc), m_server_config.m_login);

        return ServerError::None;
    }
    else
    {
        return ServerError::NotReady;
    }
}

WifiResult Wifi::update()
{
    switch (m_state_handler.getState())
    {
    case WifiState::NotInitialized:
    {
        m_error.esp_err = init();

        if (m_error.esp_err != ESP_OK)
        {
            ESP_LOGE(WIFI_TAG, "Initialization error");
            m_state_handler.changeState(WifiState::Error);
            return WifiResult::Err;
        }

        break;
    }
    case WifiState::ReadyToConnect:
    {
        m_error.esp_err = connect();

        if (m_error.esp_err != ESP_OK)
        {
            ESP_LOGE(WIFI_TAG, "Wifi Connexion error");
            m_state_handler.changeState(WifiState::Error);
            return WifiResult::Err;
        }

        break;
    }
    case WifiState::Connected:
    {
        m_error.server_err = startTcpServer();

        if ((m_error.esp_err != ESP_OK) | (m_error.server_err != ServerError::None))
        {
            ESP_LOGE(WIFI_TAG, "Error while starting TCP/IP server");
            m_state_handler.changeState(WifiState::Error);
            return WifiResult::Err;
        }
        else
        {
            m_state_handler.changeState(WifiState::TcpIpServerRunning);
        }

        break;
    }
    case WifiState::TcpIpServerRunning:
    {
        m_error.server_err = m_tcp_ip_server.update();

        if ((m_error.esp_err != ESP_OK) | (m_error.server_err != ServerError::None))
        {
            ESP_LOGE(WIFI_TAG, "Error while running TCP/IP server");
            m_state_handler.changeState(WifiState::Error);
            return WifiResult::Err;
        }

        break;
    }
    case WifiState::Error:
    {
        return WifiResult::Err;
    }
    default:
        break;
    }

    return WifiResult::Ok;
}

WifiState Wifi::getState()
{
    // Copy
    return m_state_handler.getState();
}

bool Wifi::tryToSendUdpMsg(void *data_ptr, size_t data_size)
{
    return m_tcp_ip_server.tryToSendUdpMsg(data_ptr, data_size);
}

void Wifi::log(const char *debug_msg)
{
    ESP_LOGI(WIFI_TAG, "%s", debug_msg);
}
