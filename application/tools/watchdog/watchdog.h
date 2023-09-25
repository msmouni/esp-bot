#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdint.h>
#include <mutex>
#include <memory>
#include "esp_timer.h"
#include "additional.h"

using namespace additional::option;

template <class T>
class WatchDog
{
private:
    Option<T> m_value;
    std::mutex m_mutex = {};
    int64_t m_timeout_ms;
    int64_t m_t_set_ms = -1;

public:
    WatchDog(int64_t);
    void set(T);
    Option<T> get();
};

template <class T>
WatchDog<T>::WatchDog(int64_t timeout_ms)
{
    m_value = Option<T>();
    m_timeout_ms = timeout_ms;
}

template <class T>
void WatchDog<T>::set(T value)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_value = value;
    m_t_set_ms = esp_timer_get_time() / 1000;
}

template <class T>
Option<T> WatchDog<T>::get()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_value.isSome() && (m_t_set_ms >= 0))
    {
        if (((esp_timer_get_time() / 1000) - m_t_set_ms) <= m_timeout_ms)
        {
            return m_value;
        }
        else
        {
            return Option<T>();
        }
    }
    else
    {
        return Option<T>();
    }
}

#endif // WATCHDOG_H
