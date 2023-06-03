#ifndef CAM_H
#define CAM_H

#include "esp_camera.h"
#include "esp_log.h"
#include <stdint.h>
#include <algorithm>
#include <cstring>

static const char *TAG = "CAMERA";

// support IDF 5.x
#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

// #include "esp_camera.h"

#define BOARD_WROVER_KIT 1

// WROVER-KIT PIN Map
#ifdef BOARD_WROVER_KIT

#define CAM_PIN_PWDN -1  // power down is not used
#define CAM_PIN_RESET -1 // software reset will be performed
#define CAM_PIN_XCLK 21
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 19
#define CAM_PIN_D2 18
#define CAM_PIN_D1 5
#define CAM_PIN_D0 4
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif

// ESP32Cam (AiThinker) PIN Map
#ifdef BOARD_ESP32CAM_AITHINKER

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 // software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif

#if ESP_CAMERA_SUPPORTED
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, // PIXFORMAT_RGB565, // YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,   // QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, // 0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,      // When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

static esp_err_t init_camera(void)
{
    // initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

// Later: Endianness
void u32ToBytes(uint32_t val, char *p_buffer);
// {
//     *p_buffer = static_cast<uint8_t>(val >> 24);
//     p_buffer++;
//     *p_buffer = static_cast<uint8_t>(val >> 16);
//     p_buffer++;
//     *p_buffer = static_cast<uint8_t>(val >> 8);
//     p_buffer++;
//     *p_buffer = static_cast<uint8_t>(val);
//     p_buffer++;
// }
uint32_t bytesToU32(char *p_buffer);
// {
//     uint8_t byte_4 = static_cast<uint8_t>(*p_buffer);
//     p_buffer++;
//     uint8_t byte_3 = static_cast<uint8_t>(*p_buffer);
//     p_buffer++;
//     uint8_t byte_2 = static_cast<uint8_t>(*p_buffer);
//     p_buffer++;
//     uint8_t byte_1 = static_cast<uint8_t>(*p_buffer);
//     p_buffer++;

//     return ((uint32_t)byte_4 << 24) | ((uint32_t)byte_3 << 16) | ((uint32_t)byte_2 << 8) | ((uint32_t)byte_1);
// }

struct CamPicture
{
    static const uint32_t MAX_BUFFER_LEN = 4096;
    uint8_t m_pic_buff[MAX_BUFFER_LEN]; /*!< The pixel data */
    uint32_t m_len;                     /*!< Length of the buffer in bytes */
    uint32_t m_width;                   /*!< Width of the buffer in pixels */
    uint32_t m_height;                  /*!< Height of the buffer in pixels */

    CamPicture()
    {
    }
    CamPicture(camera_fb_t *pic)
    {
        uint32_t len_to_copy = MAX_BUFFER_LEN;
        // std::min((uint32_t)pic->len,MAX_BUFFER_LEN);
        memcpy(m_pic_buff, pic->buf, len_to_copy);

        m_len = len_to_copy;
        m_width = pic->width;
        m_height = pic->height;
    }

    CamPicture(char *bytes)
    {
        m_len = bytesToU32(bytes);
        m_width = bytesToU32(bytes);
        m_height = bytesToU32(bytes);

        memcpy(m_pic_buff, bytes, MAX_BUFFER_LEN);
    }

    uint16_t toBytes(char *bytes)
    {
        u32ToBytes(m_len, bytes);
        u32ToBytes(m_width, bytes);
        u32ToBytes(m_height, bytes);

        // memcpy(bytes, m_pic_buff, MAX_BUFFER_LEN); // TMP
        return MAX_BUFFER_LEN + 4 * 3;
    }

    // TMP: MOVE to CamHandler
    void take_pic()
    {
        // ESP_LOGI(TAG, "Taking picture...");
        camera_fb_t *pic = esp_camera_fb_get();

        uint32_t len_to_copy = MAX_BUFFER_LEN;
        // std::min((uint32_t)pic->len,MAX_BUFFER_LEN);
        memcpy(m_pic_buff, pic->buf, len_to_copy);

        m_len = len_to_copy;
        m_width = pic->width;
        m_height = pic->height;

        int frmt = (int)pic->format;

        // send(client_socket, pic->buf, pic->len, 0);
        // use pic->buf to access the image
        ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes format: %d", pic->len, frmt);
        esp_camera_fb_return(pic);

        // return cam_pic;
    }
};

// TODO: Impl class Camera to handle camera (this is done this way just to test)
// static CamPicture take_pic()
static void take_pic(CamPicture &cam_pic)
// static void take_pic()
{
    // ESP_LOGI(TAG, "Taking picture...");
    camera_fb_t *pic = esp_camera_fb_get();

    cam_pic = CamPicture(pic);
    // CamPicture cam_pic = CamPicture(pic);

    int frmt = (int)pic->format;

    // send(client_socket, pic->buf, pic->len, 0);
    // use pic->buf to access the image
    // ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes format: %d", pic->len, frmt);
    esp_camera_fb_return(pic);

    // return cam_pic;
}
#endif

#endif // CAM_H