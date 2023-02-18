#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "state.h"
// #include "netiface.h"

// TMP : move to netiface.h
struct IpConfig
{
    const char *ip;
    const char *mask;
    const char *gw;
};

enum class IpSetting
{
    StaticIp,
    /// Dynamic Host Configuration Protocol
    Dhcp
};

struct NetworkIface
{
    esp_netif_t *netif;
    IpSetting ip_setting;
    IpConfig ip_config;
};

class EvHandler
{
private:
    // retry tracker
    const int MAX_FAILURES = 10;
    int m_retry_num;

    const char *M_LOG_TAG;

    WifiStateHandler *m_state_handler = NULL;

    void set_static_ip(esp_netif_t *netif, IpConfig *ip_config);

    esp_err_t set_dns_server_infos(esp_netif_t *netif, uint32_t addr, esp_netif_dns_type_t type);

    void ip_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data);
    void wifi_event_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data);

public:
    EvHandler(WifiStateHandler *, const char *);

    void handle(void *arg, esp_event_base_t event_base,
                int32_t event_id, void *event_data);
};