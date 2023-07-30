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

//////////////////////////////////////////////////////////////////////////////////////////////////////

// Inherit From socket: Stream (TCP) | Datagram (UDP)
class Socket
{
protected:
    const char *M_LOG_TAG;

    int m_socket; // Socket descriptor id

    socklen_t m_socket_addr_len = sizeof(sockaddr_in);

    WhichSocket m_type;

    SocketError m_error;
    SocketState m_state;

    ServerSocketDesc m_socket_desc;

public:
    virtual ~Socket() = 0; // Pure virtual destructor
    void start(ServerSocketDesc);

    void stop();

    virtual SocketError update() = 0;

    SocketState getState();
};

class TcpSocket : public Socket
{

    uint8_t m_nb_allowed_clients;

public:
    TcpSocket(const char *, uint8_t, WhichSocket);
    ~TcpSocket();

    // void start(ServerSocketDesc);
    // void stop();

    SocketError update();
    // SocketState getState();
    Option<int> tryToConnetClient(sockaddr_in *);
};

class UdpSocket : public Socket
{

    SocketState m_state;

    uint8_t m_nb_allowed_clients;

public:
    UdpSocket(const char *, uint8_t, WhichSocket);
    ~UdpSocket();

    // void start(ServerSocketDesc);
    // void stop();

    SocketError update();
    // SocketState getState();
    // Option<int> tryToConnetClient(sockaddr_in *);
};
//////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////

/*
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
*/

enum class ApStaTcpSocketsState
{
    NotStarted,
    InitializingAp,
    InitializingSta,
    InitializingApSta,
    ListeningOnSta,
    ListeningOnAp,
    ListeningOnApSta,
    Error,
};

enum class TcpSocketsHandlerError
{
    None,
    ErrorOnSta,
    ErrorOnAp,
    ErrorOnApSta,
};

class TcpSocketsHandler
{
    const char *M_LOG_TAG = "TcpSocketsHandler";

    TcpSocket *m_ap_socket;
    TcpSocket *m_sta_socket;

    ApStaTcpSocketsState m_state;
    TcpSocketsHandlerError m_error;

    uint8_t m_nb_allowed_clients;

public:
    TcpSocketsHandler(uint8_t);
    ~TcpSocketsHandler();

    void start(ApStaSocketsDesc);
    void stop();

    TcpSocketsHandlerError update();
    ApStaTcpSocketsState getState();
    bool isListening();
    Option<int> tryToConnetClient(sockaddr_in *);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////

enum class SocketsState
{
    NotStarted,
    Initializing,
    RunningTcpOnAp,
    RunningTcpOnSta,
    RunningTcpOnApSta,
    RunningUdpOnAp,
    RunningUdpOnSta,
    RunningUdpOnApSta,
    RunningTcpUdpOnAp,
    RunningTcpUdpOnSta,
    RunningTcpUdpOnApSta,
    Error,
};

enum class SocketsHandlerError
{
    None,
    Error,
    // TcpErrorOnSta,
    // TcpErrorOnAp,
    // TcpErrorOnApSta,
    // UdpErrorOnSta,
    // UdpErrorOnAp,
    // UdpErrorOnApSta,
    // GlErrorOnSta,
    // GlErrorOnAp,
    // GlErrorOnApSta,
};

class SocketsHandler
{
    TcpSocketsHandler *m_tcp_handler;

    SocketsState m_state;

public:
    SocketsHandler(uint8_t);
    ~SocketsHandler() = default;

    void start(ApStaSocketsDesc);
    void stop();

    SocketsHandlerError update();
    SocketsState getState();
    bool isReady();
    Option<int> tryToConnetClient(sockaddr_in *);
};

#endif // SOCKETS_HANDLER_H