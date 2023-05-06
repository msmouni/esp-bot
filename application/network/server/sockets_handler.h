#ifndef SOCKETS_HANDLER_H
#define SOCKETS_HANDLER_H

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include <algorithm>
#include "esp_log.h"
#include "socket_desc.h"

void setSocketNonBlocking(int socket_desc);
// {
//     /* NON-BLOCKING FLAG:
//         1- Call the fcntl() API to retrieve the socket descriptor's current flag settings into a local variable.
//         2- In our local variable, set the O_NONBLOCK (non-blocking) flag on. (being careful, of course, not to tamper with the other flags)
//         3- Call the fcntl() API to set the flags for the descriptor to the value in our local variable.
//     */
//     int flags = fcntl(socket_desc, F_GETFL) | O_NONBLOCK;
//     fcntl(socket_desc, F_SETFL, flags);
// }

enum class WhichSocket : uint8_t
{
    Ap,
    Sta,
};

enum class SocketState
{
    NotStarted,
    Uninitialized,
    Created,
    Bound,
    Listening,
    Error,
};

enum class SocketError
{
    None,
    CannotCreate,
    CannotBind,
    CannotListen,
    ErrorConnectingToClient,
};

class Socket
{
    const char *M_LOG_TAG;

    int m_socket; // Socket descriptor id

    // WhichSocket m_type;
    uint8_t m_type;

    SocketState m_state;
    SocketError m_error;

    ServerSocketDesc m_socket_desc;

    socklen_t m_socket_addr_len = sizeof(sockaddr_in);

    uint8_t m_nb_allowed_clients;

public:
    Socket(const char *, uint8_t, WhichSocket);
    ~Socket();

    void start(ServerSocketDesc);
    void stop();

    SocketError update();
    SocketState getState();
    Option<int> tryToConnetClient(sockaddr_in *);
};

template <uint8_t NbAllowedClients>
class SocketsHandler
{
    Socket m_ap_socket;
    Socket m_sta_socket;
};

#endif // SOCKETS_HANDLER_H