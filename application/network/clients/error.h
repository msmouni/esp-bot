#ifndef CLIENTS_ERROR
#define CLIENTS_ERROR

/*
    https://www.thegeekstuff.com/2010/10/linux-error-codes/
    https://manpages.ubuntu.com/manpages/bionic/man3/errno.3.html

    EAGAIN 11 : Try again
*/
const int SOCKET_ERR_TRY_AGAIN = 11;

enum class ClientError
{
    NoResponse,
    SocketError,
};

enum class ClientsError
{
    None,
    FullTxBuffer
};

#endif