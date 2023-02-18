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
#include <algorithm>
#include <cstring>
#include "ev_handler.h"

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

/// Dynamic Host Configuration Protocol
struct DhcpSetting
{
    SsidPassword ssid_password;
};

struct StaticIpSetting
{
    SsidPassword ssid_password;
    IpConfig ip_config;
};

class Wifi
{
private:
    /*// since C++11: The constexpr specifier declares that it is possible to evaluate the value of the function or variable at compile time.
    constexpr static const char *ssid{"MyWifiSsid"};
    constexpr static const char *password{"MyWifiPassword"};*/

    SsidPassword m_ssid_password;

    // Debug Tag
    constexpr static const char *WIFI_TAG = "WIFI";

    static WifiStateHandler m_state_handler;
    static void change_state(WifiState);

    // Configuration
    static wifi_init_config_t m_wifi_init_config;
    wifi_config_t m_wifi_config = {}; // All element as Default : https://iq.opengenus.org/different-ways-to-initialize-array-in-cpp/

      // static char mac_addr_cstr[13]; // Buffer to hold MAC as cstring

    void create(SsidPassword);

    static EvHandler m_ev_handler;

    static void handle_event(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data);

    static esp_err_t get_mac(void);

    //////////////////////////////////////  MOVE TO IpServer class
    NetworkIface m_netiface = {};

public:
    /*
Refs: https://www.geeksforgeeks.org/rule-of-three-in-cpp/
      https://en.cppreference.com/w/cpp/language/rule_of_three
      https://en.cppreference.com/w/cpp/language/operators
      https://en.wikipedia.org/wiki/Assignment_operator_(C%2B%2B)

"Rule Of Three":
*/
    Wifi(DhcpSetting);
    Wifi(StaticIpSetting);
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
    ServerError start_tcp_server(int);
    WifiState get_state(void); // Copy
    static void log(const char *debug_msg);
};