#include "wifi.h"

/*
Wifi statics

Ref:https://cpp.developpez.com/faq/cpp/?page=Les-donnees-et-fonctions-membres-statiques

https://www.irif.fr/~carton/Enseignement/ObjetsAvances/Cours/simons.pdf
*/
WifiState Wifi::m_state = WifiState::NotInitialised;
wifi_init_config_t Wifi::m_wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
int Wifi::s_retry_num = 0;

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
Wifi::Wifi(DhcpSetting dhcp_setting)
{

    create(dhcp_setting.ssid_password);

    m_netiface.ip_setting = IpSetting::Dhcp;
}

// Wifi Static Ip Constructor
Wifi::Wifi(StaticIpSetting static_ip_setting)
{

    create(static_ip_setting.ssid_password);

    m_netiface.ip_config = static_ip_setting.ip_config;

    m_netiface.ip_setting = IpSetting::StaticIp;
}

void Wifi::event_handler(void *arg, esp_event_base_t event_base,
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
        ESP_LOGE(WIFI_TAG, "Unexpected event: %s", event_base);
    }
}

esp_err_t Wifi::set_dns_server_infos(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type)
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

void Wifi::set_static_ip(esp_netif_t *netif, IpConfig *ip_config)
{
    if (esp_netif_dhcpc_stop(netif) != ESP_OK)
    {
        ESP_LOGE(WIFI_TAG, "Failed to stop dhcp client");
        return;
    }
    esp_netif_ip_info_t ip = {};
    memset(&ip, 0, sizeof(esp_netif_ip_info_t));
    ip.ip.addr = ipaddr_addr(ip_config->ip);
    ip.netmask.addr = ipaddr_addr(ip_config->mask);
    ip.gw.addr = ipaddr_addr(ip_config->gw);
    if (esp_netif_set_ip_info(netif, &ip) != ESP_OK)
    {
        ESP_LOGE(WIFI_TAG, "Failed to set ip info");
        return;
    }

    // As DNS
    // Router Address: GateWay
    set_dns_server_infos(netif, ipaddr_addr(ip_config->gw), ESP_NETIF_DNS_MAIN);
    set_dns_server_infos(netif, ipaddr_addr(ip_config->gw), ESP_NETIF_DNS_BACKUP);
}

// event handler for wifi events
void Wifi::wifi_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{

    if (WIFI_EVENT == event_base)
    {
        const wifi_event_t event_type{static_cast<wifi_event_t>(event_id)}; // cast int32_t -> wifi_event_t

        switch (event_type)
        {
        case WIFI_EVENT_STA_START:
        {
            ESP_LOGI(WIFI_TAG, "Wifi STA started");
            m_state = WifiState::ReadyToConnect;
            break;
        }

        case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI(WIFI_TAG, "Wifi STA connected");

            NetworkIface *network_iface = static_cast<NetworkIface *>(arg);

            switch (network_iface->ip_setting)
            {
            case IpSetting::StaticIp:
            {
                m_state = WifiState::SettingIp;
                set_static_ip(network_iface->netif, &network_iface->ip_config);

                break;
            }
            case IpSetting::Dhcp:
            {
                m_state = WifiState::WaitingForIp;
            }

            default:
                break;
            }

            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED:
        {
            if (s_retry_num < MAX_FAILURES)
            {
                ESP_LOGI(WIFI_TAG, "Reconnecting to AP...");

                if (ESP_OK == esp_wifi_connect())
                {
                    m_state = WifiState::Connecting;

                    s_retry_num++;
                }
            }
            else
            {
                m_state = WifiState::Error;
            }
        }

        default:
            // TODO STOP and DISCONNECTED, and others
            break;
        }
    }
}

void Wifi::ip_event_handler(void *arg, esp_event_base_t event_base,
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

            ESP_LOGI(WIFI_TAG, "ESP_NETIF_DNS_MAIN %lu", dns.ip.u_addr.ip4.addr); // FE01A8C0: 254.1.168.192*/
            ////////////////////////////

            ESP_LOGI(WIFI_TAG, "STA IP: " IPSTR, IP2STR(&event->ip_info.ip));

            s_retry_num = 0;

            m_state = WifiState::Connected;
            break;
        }

        case IP_EVENT_STA_LOST_IP:
        {
            ESP_LOGI(WIFI_TAG, "STA LOST IP");
            m_state = WifiState::WaitingForIp;
            break;
        }

        default:
            // TODO IP6
            break;
        }
    }
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

    if (WifiState::NotInitialised == m_state)
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
                                                         &wifi_event_handler,
                                                         &m_netiface,
                                                         NULL);
        }

        if (ESP_OK == status)
        {
            status = esp_event_handler_instance_register(IP_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &ip_event_handler,
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

        if ((ESP_OK == status) && (WifiState::NotInitialised == m_state))
        {
            // In case Event happens before
            m_state = WifiState::Initialised;
        }

        ESP_LOGI(WIFI_TAG, "STA initialization complete");
    }
    else if (WifiState::Error == m_state)
    {
        status = ESP_FAIL;
    }

    return status;
}

esp_err_t Wifi::connect()
{

    esp_err_t status = ESP_OK;

    switch (m_state)
    {
    case WifiState::ReadyToConnect:
        status = esp_wifi_connect();

        if (ESP_OK == status)
        {
            m_state = WifiState::Connecting;
        }
        break;

    default:
        break;
    }

    return status;
}

ServerError Wifi::start_tcp_server(int port)
{
    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    /* NON-BLOCKING FLAG:
        1- Call the fcntl() API to retrieve the socket descriptor's current flag settings into a local variable.
        2- In our local variable, set the O_NONBLOCK (non-blocking) flag on. (being careful, of course, not to tamper with the other flags)
        3- Call the fcntl() API to set the flags for the descriptor to the value in our local variable.
    */
    int flags = fcntl(serv_sock, F_GETFL) | O_NONBLOCK;
    fcntl(serv_sock, F_SETFL, flags);

    /*
    Note: Now, using {0} to initialize an aggregate like this is basically a trick to 0 the entire thing.
    This is because when using aggregate initialization you don't have to specify all the members and the spec requires
    that all unspecified members be default initialized, which means set to 0 for simple types.
    */
    struct sockaddr_in server_socket_addr = {0};
    socklen_t socket_addr_len = sizeof(sockaddr_in);

    char readBuffer[1024] = {0};

    server_socket_addr.sin_family = AF_INET;

    inet_aton(m_netiface.ip_config.ip, &server_socket_addr.sin_addr.s_addr); // inet_aton() converts the Internet host address cp from the IPv4 numbers-and-dots notation into binary form (in network byte order)
    server_socket_addr.sin_port = htons(port);                               // The htons() function converts the unsigned short integer hostshort from host byte order to network byte order.

    if (serv_sock < 0)
    {
        ESP_LOGE(WIFI_TAG, "Failed to create a socket...");
        return ServerError::CannotCreateSocket;
    }

    if (bind(serv_sock, (struct sockaddr *)&server_socket_addr, socket_addr_len) < 0)
    {
        ESP_LOGE(WIFI_TAG, "Failed to bind socket...");
        return ServerError::CannotCreateSocket;
    }

    if (listen(serv_sock, 5) < 0)
    {
        ESP_LOGE(WIFI_TAG, "Cannot listen on socket...");
        return ServerError::CannotListenOnSocket;
    }

    ////////////////////////////////
    struct sockaddr_in client_socket_addr = {0};

    // TMP: Blocking
    ESP_LOGI(WIFI_TAG, "WAITING FOR CLIENT TO CONNECT");
    int client_socket = -1;
    while (client_socket < 0)
    {
        client_socket = accept(serv_sock, (struct sockaddr *)&client_socket_addr, &socket_addr_len);
        vTaskDelay(100);
    }
    ESP_LOGI(WIFI_TAG, "CLIENT CONNECTED");

    if (client_socket < 0)
    {
        ESP_LOGE(WIFI_TAG, "Error connecting to client: %d", client_socket);
        close(client_socket);
        return ServerError::ErrorConnectingToClient;
    }
    else
    {
        ESP_LOGI(WIFI_TAG, "Client connected: IP: %s | Port: %d\n", inet_ntoa(client_socket_addr.sin_addr), (int)ntohs(client_socket_addr.sin_port));
    }

    int r;
    // TMP
    while (strcmp(readBuffer, "quit\n") != 0)
    {

        // TMP BLOCKING

        bzero(readBuffer, sizeof(readBuffer));

        // ends at b"\n"
        r = read(client_socket, readBuffer, sizeof(readBuffer) - 1); // receive N_BYTES AT Once
        // r = recv(serv_sock, readBuffer, sizeof(readBuffer) - 1, 0);  // Receive data from the socket. The return value is a bytes object representing the data received. The maximum amount of data to be received at once is specified by bufsize.

        if (r > 0)
        {
            /*
            You can either add a null character after your termination character, and your printf will work,
            or you can add a '.*' in your printf statement and provide the length*/
            printf("%.*s", r, readBuffer);
        }

        vTaskDelay(100);
    }

    return ServerError::None;
}

WifiState Wifi::get_state()
{

    // Mutex on state
    return m_state; // Copy
}

void Wifi::log(const char *debug_msg)
{
    ESP_LOGI(WIFI_TAG, "%s", debug_msg);
}
