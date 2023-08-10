#ifndef CAM_H
#define CAM_H

#include "esp_camera.h"
#include "esp_log.h"
#include <stdint.h>
#include <algorithm>
#include <cstring>
#include "timer.h"
#include "picture.h"

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

enum class CameraState
{
    Created,
    Ready,
    Error,
};

class Camera
{
private:
    static CamPicture m_pic;

    camera_config_t m_config = {};
    CameraState m_state;
    esp_err_t m_error;

    PeriodicTimer *m_timer_send_100ms = NULL; // In order to create timer after the application has started

    static void takePic100ms(void *);

public:
    Camera();

    esp_err_t init();

    bool isPicAvailable();
    // Option<ServerFrame<TcpIpServer::MAX_MSG_SIZE>> getNextFrame();

    Option<void *> getNextFrameBuff();

    uint32_t getLen();

    // void setProcessed();
};

// // TODO: Impl class Camera to handle camera (this is done this way just to test)
// // static CamPicture take_pic()
// static void take_pic(CamPicture &cam_pic)
// // static void take_pic()
// {
//     // ESP_LOGI(TAG, "Taking picture...");
//     camera_fb_t *pic = esp_camera_fb_get();

//     cam_pic = CamPicture(pic);
//     // CamPicture cam_pic = CamPicture(pic);

//     int frmt = (int)pic->format;

//     // send(client_socket, pic->buf, pic->len, 0);
//     // use pic->buf to access the image
//     // ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes format: %d", pic->len, frmt);
//     esp_camera_fb_return(pic);

//     // return cam_pic;
// }
#endif

#endif // CAM_H