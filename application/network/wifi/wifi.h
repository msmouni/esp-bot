#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <algorithm>
#include <cstring>
#include "ev_handler.h"
#include "server.h"
#include "result.h"

class Wifi
{
private:
    SsidPassword m_ssid_password;

    // The constexpr specifier declares that it is possible to evaluate the value of the function or variable at compile time.
    // Debug Tag
    constexpr static const char *WIFI_TAG = "WIFI";

    static WifiStateHandler m_state_handler;
    static void changeState(WifiState);

    // Configuration
    static wifi_init_config_t m_wifi_init_config;
    WifiConfig m_config = {};

    // static char mac_addr_cstr[13]; // Buffer to hold MAC as cstring

    // void create(SsidPassword);
    void configure_sta(StaSetting &);
    void configure_ap(ApSetting &);
    void configure(WifiSetting &);

    static EvHandler m_ev_handler;

    static void handleEvent(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data);

    static esp_err_t getMac(void);

    esp_err_t init(void);
    esp_err_t connect(void);
    ServerError startTcpServer();

    WifiError m_error = {};

    TcpIpServer m_tcp_ip_server;
    ServerConfig m_server_config;
    NetworkIface m_netiface = {};

public:
    /*
Refs: https://www.geeksforgeeks.org/rule-of-three-in-cpp/
      https://en.cppreference.com/w/cpp/language/rule_of_three
      https://en.cppreference.com/w/cpp/language/operators
      https://en.wikipedia.org/wiki/Assignment_operator_(C%2B%2B)

"Rule Of Three":
*/
    Wifi(WifiSetting, ServerConfig);
    ~Wifi(void) = default;
    Wifi(const Wifi &) = default;
    Wifi &operator=(const Wifi &) = default;
    // Ref: https://www.invivoo.com/decouverte-du-cplusplus-et-stdmove/
    Wifi(Wifi &&) = default;
    Wifi &operator=(Wifi &&) = default;

    /*constexpr static const char *get_mac_cstr(void)
    {
        return mac_addr_cstr;
    }*/

    WifiResult update(void);
    WifiState getState(void);
    static void log(const char *debug_msg);
};