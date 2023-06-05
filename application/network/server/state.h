enum class ServerState
{
    NotStarted,
    Uninitialized,
    // SocketCreated,
    // SocketBound,
    SocketsListening,
    Running,
    Error,
};