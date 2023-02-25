enum class ServerError
{
    None,
    NotReady,
    CannotCreateSocket,
    CannotBindSocket,
    CannotListenOnSocket,
    ErrorConnectingToClient
};