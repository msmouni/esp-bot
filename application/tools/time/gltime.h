#ifndef TOOLS_GL_TIME
#define TOOLS_GL_TIME

#include "esp_timer.h"

struct GlTime
{
    int64_t get_us()
    {
        return esp_timer_get_time();
    }

    int64_t get_ms()
    {
        return get_us() / 1000;
    }
};

#endif // TOOLS_GL_TIME