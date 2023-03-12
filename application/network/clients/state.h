

enum class ClientState
{
    Uninitialized,
    Connected,
    Authenticated,
    TakingControl, // Note only one client should take control, the others can only recv msgs from server
};