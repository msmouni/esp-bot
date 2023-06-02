#include "program.h"

MainProgram::MainProgram()
{
    m_state = MainState::Uninitialized;

    // TODO: Move all config to a json file for example                     ////////////////////////////////
    // LATER: Will be stored at NVS (Non-volatile Storage)
    SsidPassword wifi_ssid_password = SsidPassword("My-SSID", "My-Password");

    // Router Address: GateWay
    IpConfig ip_config = IpConfig("xxx.xxx.xxx.xxx", "xxx.xxx.xxx.xxx", "xxx.xxx.xxx.xxx");
    StaticIpSetting static_ip_setting = StaticIpSetting(wifi_ssid_password, ip_config);
    StaSetting wifi_sta_setting = StaSetting(static_ip_setting);

    ApSetting wifi_ap_setting = ApSetting(SsidPassword("AP-SSID", "AP-Password"), IpConfig("xxx.xxx.xxx.xxx", "xxx.xxx.xxx.xxx", "xxx.xxx.xxx.xxx"));

    WifiSetting wifi_setting = WifiSetting(wifi_ap_setting, wifi_sta_setting);

    ServerConfig tcp_ip_server_config = ServerConfig(ServerLogin(Login("test_admin", "test_password"), Login("super_admin", "super_password")), 12345, 54321);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    m_wifi = new Wifi(wifi_setting, tcp_ip_server_config);
}

MainProgram::~MainProgram()
{
    delete m_wifi;
}

esp_err_t MainProgram::setup()
{
    esp_err_t status = ESP_OK;

    ESP_LOGI(LOG_TAG, "Creating default event loop");
    status = esp_event_loop_create_default();

    if (ESP_OK == status)
    {
        ESP_LOGI(LOG_TAG, "Initializing NVS");
        status = initNvs();
    }

    init_camera();

    return status;
};

// initialize storage
esp_err_t MainProgram::initNvs()
{
    esp_err_t res = nvs_flash_init();

    if (res == ESP_ERR_NVS_NO_FREE_PAGES || res == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        res = nvs_flash_erase();

        if (res == ESP_OK)
        {
            res = nvs_flash_init();
        }
    }

    return res;
}

void MainProgram::run(void)
{
    while (true)
    {
        switch (m_state)
        {
        case MainState::Uninitialized:
        {
            if (ESP_OK == setup())
            {
                m_state = MainState::Running;
            }
            break;
        }
        case MainState::Running:
        {

            update();

            break;
        }

        case MainState::Error:
        {

            ESP_LOGE(LOG_TAG, "Error occured");

            break;
        }

        default:
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // 100 ms
    }
}

void MainProgram::update()
{
    if (m_state == MainState::Running)
    {
        if (m_wifi->update() == WifiResult::Err)
        {
            m_state = MainState::Error;
        }
    }
}