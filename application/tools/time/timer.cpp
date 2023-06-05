#include "timer.h"

HighResTimer::HighResTimer(const char *name, const esp_timer_cb_t &callback_funtion, void *args)
{
    /*  This callback function is called from the esp_timer task each time the timer elapses. */
    m_args.callback = callback_funtion;
    m_args.arg = args;
    /* name is optional, but may help identify the timer when debugging */
    m_args.name = name;

    esp_err_t res = esp_timer_create(&m_args, &m_handler);

    if (res == ESP_OK)
    {
        m_state = TimerState::Created;
    }
    else
    {
        m_state = TimerState::Error;
        m_error = res;
    }
}
HighResTimer::~HighResTimer()
{
}

int64_t HighResTimer::get_time_us()
{
    return m_time.get_us();
}

int64_t HighResTimer::get_time_ms()
{
    return m_time.get_ms();
}

esp_err_t HighResTimer::stop()
{
    esp_err_t res = esp_timer_stop(m_handler);

    if (res == ESP_OK)
    {
        m_state = TimerState::Stopped;
    }
    else
    {
        m_state = TimerState::Error;
        m_error = res;
    }
    return res;
};

// //////////////////////////// PERIODIC_TIMER

PeriodicTimer::~PeriodicTimer()
{
    esp_timer_stop(m_handler);
    esp_timer_delete(m_handler);
}

esp_err_t PeriodicTimer::start()
{
    esp_err_t res = esp_timer_start_periodic(m_handler, m_period_us);

    if (res == ESP_OK)
    {
        m_state = TimerState::Started;
    }
    else
    {
        m_state = TimerState::Error;
        m_error = res;
    }

    return res;
};

Result<uint64_t, esp_err_t> PeriodicTimer::get_timeout_us()
{
    uint64_t timeout;
    esp_err_t res = esp_timer_get_period(m_handler, &timeout);

    if (res == ESP_OK)
    {
        return Result<uint64_t, esp_err_t>(timeout);
    }
    else
    {
        return Result<uint64_t, esp_err_t>(res);
    }
};

//////////////////////////// ONE_SHOT_TIMER

OneShotTimer::~OneShotTimer()
{
    esp_timer_stop(m_handler);
    esp_timer_delete(m_handler);
}

esp_err_t OneShotTimer::start()
{
    esp_err_t res = esp_timer_start_once(m_handler, m_timeout_us);

    if (res == ESP_OK)
    {
        m_state = TimerState::Started;
    }
    else
    {
        m_state = TimerState::Error;
        m_error = res;
    }

    return res;
};

Result<uint64_t, esp_err_t> OneShotTimer::get_timeout_us()
{
    uint64_t timeout;
    esp_err_t res = esp_timer_get_expiry_time(m_handler, &timeout);

    if (res == ESP_OK)
    {
        return Result<uint64_t, esp_err_t>(timeout);
    }
    else
    {
        return Result<uint64_t, esp_err_t>(res);
    }
};