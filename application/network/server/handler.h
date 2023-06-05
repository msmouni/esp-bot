#ifndef SOCKETS_HANDLER_H
#define SOCKETS_HANDLER_H

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include <algorithm>
#include "esp_log.h"
#include "additional.h"
#include "socket_desc.h"

using namespace additional::option;

void setSocketNonBlocking(int socket_desc);

enum class WhichSocket : uint8_t
{
    Undefined,
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

    WhichSocket m_type;

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

// struct ApStaSocketsError
// {
//     SocketError m_ap;
//     SocketError m_sta;

//     ApStaSocketsError() : m_ap(SocketError::None), m_sta(SocketError::None){};

//     bool hasApError()
//     {
//         return (m_ap != SocketError::None);
//     }
//     bool hasStaError()
//     {
//         return (m_sta != SocketError::None);
//     }
//     bool hasApStaError()
//     {
//         return hasApError() || hasStaError();
//     }
// };

enum class ApStaSocketsState
{
    NotStarted,
    InitializingAp,
    InitializingSta,
    InitializingApSta,
    ListeningOnSta,
    ListeningOnAp,
    ListeningOnApSta,
    ErrorOnAp,
    ErrorOnSta,
    ErrorOnApSta,
};

enum class SocketsHandlerError
{
    None,
    ErrorOnSta,
    ErrorOnAp,
    ErrorOnApSta
};

class SocketsHandler
{
    const char *M_LOG_TAG = "SocketsHandler";

    Socket *m_ap_socket;
    Socket *m_sta_socket;

    ApStaSocketsState m_state;
    SocketsHandlerError m_error;

    uint8_t m_nb_allowed_clients;

public:
    /// SocketsHandler(NbAllowedClients)
    SocketsHandler(uint8_t);
    ~SocketsHandler();

    void start(ApStaSocketsDesc);
    void stop();

    SocketsHandlerError update();
    ApStaSocketsState getState();
    bool isListening();
    Option<int> tryToConnetClient(sockaddr_in *);
};

#endif // SOCKETS_HANDLER_H