#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "state.h"
#include <algorithm>
#include <cstring>

//////////////////////////////////////////////////////////  ADD MUTEX

struct SsidPassword
{
    const char *ssid;
    const char *password;
};

enum class ServerError
{
    None,
    CannotCreateSocket,
    CannotBindSocket,
    CannotListenOnSocket,
    ErrorConnectingToClient
};

class Wifi
{
private:
    /*// since C++11: The constexpr specifier declares that it is possible to evaluate the value of the function or variable at compile time.
    constexpr static const char *ssid{"MyWifiSsid"};
    constexpr static const char *password{"MyWifiPassword"};*/

    SsidPassword m_ssid_password;

    // IP + GW + NetMask
    static esp_netif_ip_info_t net_infos;

    // Debug Tag
    constexpr static const char *WIFI_TAG = "WIFI";

    // Strongly typed enum to define our state
    static WifiState m_state;

    // Configuration
    static wifi_init_config_t m_wifi_init_config;
    wifi_config_t m_wifi_config = {}; // All element as Default : https://iq.opengenus.org/different-ways-to-initialize-array-in-cpp/

    // retry tracker
    static const int MAX_FAILURES = 10;
    static int s_retry_num;

    // static char mac_addr_cstr[13]; // Buffer to hold MAC as cstring

    static void event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data);
    static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data);
    static void ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data);

    static esp_err_t get_mac(void);

public:
    /*
Refs: https://www.geeksforgeeks.org/rule-of-three-in-cpp/
      https://en.cppreference.com/w/cpp/language/rule_of_three
      https://en.cppreference.com/w/cpp/language/operators
      https://en.wikipedia.org/wiki/Assignment_operator_(C%2B%2B)

"Rule Of Three":
*/
    Wifi(SsidPassword);
    ~Wifi(void) = default;
    Wifi(const Wifi &) = default;
    Wifi &operator=(const Wifi &) = default;
    // Ref: https://www.invivoo.com/decouverte-du-cplusplus-et-stdmove/
    Wifi(Wifi &&) = default;
    Wifi &operator=(Wifi &&) = default;

    static esp_err_t init_nvs(void);
    /*constexpr static const char *get_mac_cstr(void)
    {
        return mac_addr_cstr;
    }*/
    esp_err_t init(void);
    esp_err_t connect(void);
    ServerError start_tcp_server(void);
    WifiState get_state(void); // Copy
    static void log(const char *debug_msg);
};