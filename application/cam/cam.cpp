#include "cam.h"

CamPicture Camera::m_pic = {};

void Camera::takePic100ms(void *args)
{
    // ESP_LOGI(TAG, "Taking picture...");
    camera_fb_t *pic = esp_camera_fb_get();

    m_pic.update(pic);

    int frmt = (int)pic->format;

    // ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes format: %d", pic->len, frmt);
    esp_camera_fb_return(pic);
}

Camera::Camera()
{

    m_config.pin_pwdn = CAM_PIN_PWDN;
    m_config.pin_reset = CAM_PIN_RESET;
    m_config.pin_xclk = CAM_PIN_XCLK;
    m_config.pin_sccb_sda = CAM_PIN_SIOD;
    m_config.pin_sccb_scl = CAM_PIN_SIOC;

    m_config.pin_d7 = CAM_PIN_D7;
    m_config.pin_d6 = CAM_PIN_D6;
    m_config.pin_d5 = CAM_PIN_D5;
    m_config.pin_d4 = CAM_PIN_D4;
    m_config.pin_d3 = CAM_PIN_D3;
    m_config.pin_d2 = CAM_PIN_D2;
    m_config.pin_d1 = CAM_PIN_D1;
    m_config.pin_d0 = CAM_PIN_D0;
    m_config.pin_vsync = CAM_PIN_VSYNC;
    m_config.pin_href = CAM_PIN_HREF;
    m_config.pin_pclk = CAM_PIN_PCLK;

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    m_config.xclk_freq_hz = 20000000;
    m_config.ledc_timer = LEDC_TIMER_0;
    m_config.ledc_channel = LEDC_CHANNEL_0;

    m_config.pixel_format = PIXFORMAT_JPEG; // PIXFORMAT_RGB565, // YUV422,GRAYSCALE,RGB565,JPEG
    m_config.frame_size = FRAMESIZE_QVGA;   // QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    m_config.jpeg_quality = 12; // 0-63, for OV series camera sensors, lower number means higher quality
    m_config.fb_count = 1;      // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    m_config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    m_error = ESP_OK;
    m_state = CameraState::Created;
}

esp_err_t Camera::init()
{
    // initialize the camera
    esp_err_t err = esp_camera_init(&m_config);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        m_error = err;
        return err;
    }

    m_timer_send_100ms = new PeriodicTimer("Camera_Take_Pic_100ms", takePic100ms, NULL, 100000);
    err = m_timer_send_100ms->start();

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Timer Start Failed");
        m_error = err;
        return err;
    }

    ESP_LOGI(TAG, "Camera Timer Started");
    return ESP_OK;
}

bool Camera::isPicAvailable()
{
    return m_pic.isAvailable();
}

// Option<ServerFrame<TcpIpServer::MAX_MSG_SIZE>> Camera::getNextFrame()
// {
//     return m_pic.getNextFrame();
// }

void *Camera::getFrameRef()
{
    return m_pic.getFrameRef();
}

uint32_t Camera::getLen()
{
    return m_pic.getLen();
}

void Camera::setProcessed()
{
    m_pic.setProcessed();
}