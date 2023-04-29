#ifndef TOOLS_TIMER
#define TOOLS_TIMER

#include "esp_timer.h"
#include "additional.h"
#include "gltime.h"

using namespace additional::result;

enum class TimerState
{
    Uninitialized,
    Created,
    Started,
    Stopped,
    Error,
};

// An abstract class
class HighResTimer
{
protected:
    const char *name;
    esp_timer_handle_t m_handler;
    esp_timer_create_args_t m_args = {};
    TimerState m_state = TimerState::Uninitialized;
    esp_err_t m_error = ESP_OK;
    GlTime m_time;

public:
    /// HighResTimer(name, callback_function, args_for_callback_function)
    HighResTimer(const char *, const esp_timer_cb_t &, void *);
    /*
    Deleting a derived class object using a pointer of base class type that has a non-virtual destructor results in undefined behavior.
    To correct this situation, the base class should be defined with a virtual destructor.

    https://www.geeksforgeeks.org/virtual-destructor/
    https://www.geeksforgeeks.org/pure-virtual-destructor-c/
    */
    virtual ~HighResTimer() = 0; // Pure virtual destructor

    int64_t get_time_us();

    int64_t get_time_ms();

    // Pure Virtual Functions
    virtual esp_err_t start() = 0;
    virtual Result<uint64_t, esp_err_t> get_timeout_us() = 0;

    esp_err_t stop();
};

//////////////////////////// PERIODIC_TIMER

class PeriodicTimer : public HighResTimer
{
    uint64_t m_period_us;

public:
    /// PeriodicTimer(name, callback_function, args_for_callback_function, period_us)
    PeriodicTimer(const char *name, const esp_timer_cb_t &callback_funtion, void *args, uint64_t period_us) : HighResTimer(name, callback_funtion, args), m_period_us(period_us){};
    ~PeriodicTimer();

    esp_err_t start();
    Result<uint64_t, esp_err_t> get_timeout_us();
};

//////////////////////////// ONE_SHOT_TIMER

class OneShotTimer : public HighResTimer
{
    uint64_t m_timeout_us;

public:
    /// OneShotTimer(name, callback_function, args_for_callback_function, timeout_us)
    OneShotTimer(const char *name, const esp_timer_cb_t &callback_funtion, void *args, uint64_t timeout_us) : HighResTimer(name, callback_funtion, args), m_timeout_us(timeout_us){};
    ~OneShotTimer();

    esp_err_t start();
    Result<uint64_t, esp_err_t> get_timeout_us();
};

#endif // TOOLS_TIMER