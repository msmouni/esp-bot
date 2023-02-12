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