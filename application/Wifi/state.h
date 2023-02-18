#include <mutex>

/*
    Ref: https://www.geeksforgeeks.org/enum-classes-in-c-and-their-advantage-over-enum-datatype/

    C++11 has introduced enum classes (also called scoped enumerations),
    that makes enumerations both strongly typed and strongly scoped.
    Class enum doesn’t allow implicit conversion to int,
    and also doesn’t compare enumerators from different enumerations.

    Defines Wifi STA State

*/
enum class WifiState
{
    NotInitialised,
    Initialised,
    ReadyToConnect,
    Connecting,
    SettingIp,
    WaitingForIp,
    Connected,
    Disconnected,
    Error,
};

struct WifiStateHandler
{
    WifiState m_state;

    std::mutex m_state_mutex; /// Mutex on State

    WifiStateHandler() : m_state(WifiState::NotInitialised) {}

    void change_state(WifiState new_state)
    {
        std::lock_guard<std::mutex> state_guard(m_state_mutex);
        m_state = new_state;
    }

    // A Copy
    WifiState get_state()
    {
        std::lock_guard<std::mutex> state_guard(m_state_mutex);
        return m_state;
    }
};