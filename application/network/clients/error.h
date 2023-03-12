#ifndef CLIENTS_ERROR
#define CLIENTS_ERROR

enum class ClientError
{
    NoResponse
};

enum class ClientsError
{
    None,
    FullTxBuffer
};

#endif