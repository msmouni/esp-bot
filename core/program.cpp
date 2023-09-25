#include "program.h"

bool MainProgram::m_gpio_state = false;
Option<RobotControl> MainProgram::m_robot_control = Option<RobotControl>();

/*void MainProgram::gpioToggle1s(void *args)
{
    m_gpio_state = !m_gpio_state;
    // bool state = m_gpio_tst.state();
    // m_gpio_tst.set(!state);
}*/

void MainProgram::processRobotControl(void *args)
{
    if (m_robot_control.isSome())
    {
        RobotControl robot_control = m_robot_control.getData();
        m_gpio_state = robot_control.getY() >= 0.5;
    }
    else
    {
        m_gpio_state = false;
    }
    // m_gpio_state = !m_gpio_state;
    // bool state = m_gpio_tst.state();
    // m_gpio_tst.set(!state);
}

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

    ServerConfig server_config = ServerConfig(ServerLogin(Login("test_admin", "test_password"), Login("super_admin", "super_password")), 12345, 54321);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    m_wifi = new Wifi(wifi_setting, server_config);
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
#if ESP_CAMERA_SUPPORTED
    if (ESP_OK == status)
    {
        ESP_LOGI(LOG_TAG, "Initializing Camera");
        status = m_camera.init();
    }
#endif

    if (ESP_OK == status)
    {
        ESP_LOGI(LOG_TAG, "Initializing GPIO");
        status = m_gpio_tst.init();
        if (ESP_OK == status)
        {
            // m_timer_gpio_toggle = new PeriodicTimer("Toggle", gpioToggle1s, NULL, 1000000);
            m_robot_control_timer = new PeriodicTimer("Control", processRobotControl, NULL, 100000); // 100ms
            status = m_robot_control_timer->start();
        }
    }

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
        // int64_t t_start = esp_timer_get_time();

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
            // Create Tasks instead of global loop

            update();

            m_robot_control = m_wifi->getRobotControl();

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

        vTaskDelay(1); // pdMS_TO_TICKS(100)); // 100 ms
    }
}

#if ESP_CAMERA_SUPPORTED
void MainProgram::updateCam()
{
    if (m_camera.isPicAvailable())
    {
        bool done_sending = false;
        /*
        Note: when calling socket's sendto(void* buff) for example, buff is copied to the OS's network stack before beeing actually sent => Socket out of memory
        */

        while (!done_sending)
        {
            Option<BuffRefLen> opt_pic_frame_buff_len = m_camera.getNextFrameBuff();

            if (opt_pic_frame_buff_len.isNone())
            {
                done_sending = true;
            }
            else
            {

                BuffRefLen pic_frame_buff_len = opt_pic_frame_buff_len.getData();

                if (m_wifi->tryToSendUdpMsg(pic_frame_buff_len.m_buff_ref, pic_frame_buff_len.m_len))
                {
                    m_camera.setCurrentFrameProcessed();
                }
                else
                {
                    done_sending = true;
                }
            }
        }
    }
}
#endif

void MainProgram::update()
{
    if (m_state == MainState::Running)
    {
#if ESP_CAMERA_SUPPORTED
        updateCam();
#endif

        if (m_wifi->update() == WifiResult::Err)
        {
            m_state = MainState::Error;
        }

        m_gpio_tst.set(m_gpio_state);
    }
}