enum class ServerError
{
    None,
    NotReady,
    // CannotCreateSocket,
    // CannotBindSocket,
    // CannotListenOnSocket,
    // ErrorConnectingToClient,
    SocketError,
    ErrorStarting25msTimer
};