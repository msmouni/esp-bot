#include "esp_err.h"

enum class WifiResult
{
    Ok,
    Err,
};

struct WifiError
{
    esp_err_t esp_err;
    ServerError server_err;

    WifiError() : esp_err(ESP_OK), server_err(ServerError::None){};
};
