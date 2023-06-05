#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "state.h"
#include "netiface.h"

class EvHandler
{
private:
    // retry tracker
    const int MAX_FAILURES = 10;
    int m_retry_num;

    const char *M_LOG_TAG;

    WifiStateHandler *m_state_handler = NULL;

    void ipEventHandler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data);
    void wifiEventHandler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data);

public:
    EvHandler(WifiStateHandler *, const char *);

    void handle(void *arg, esp_event_base_t event_base,
                int32_t event_id, void *event_data);
};