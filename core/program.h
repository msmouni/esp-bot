#include "nvs_flash.h"
#include "wifi.h"
#include "cam.h"
#include "state.h"
#include "io_pin.h"

// final specifies that this class may not appear in the base-specifier-list of another class definition (in other words, cannot be derived from).
class MainProgram final
{
private:
    // The constexpr specifier declares that it is possible to evaluate the value of the function or variable at compile time.
    constexpr static const char *LOG_TAG = "MAIN";

    MainState m_state;

    Wifi *m_wifi = NULL;
#if ESP_CAMERA_SUPPORTED
    Camera m_camera = Camera();
#endif
    static bool m_gpio_state;
    IoPinOutput m_gpio_tst = IoPinOutput(GPIO_NUM_13, false);
    PeriodicTimer *m_timer_gpio_toggle = NULL;
    static void gpioToggle1s(void *);

    esp_err_t initNvs();

    esp_err_t setup(void);

#if ESP_CAMERA_SUPPORTED
    void updateCam(void); // Maybe move elsewhere
#endif

    void update(void);

public:
    MainProgram(void);
    ~MainProgram(void);
    void run(void);
};