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
    Ready,
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

// Inherit From socket: Stream (TCP) | Datagram (UDP)
class Socket
{
protected:
    const char *M_LOG_TAG;

    int m_socket; // Socket descriptor id

    WhichSocket m_type;

    SocketState m_state;
    SocketError m_error;

    ServerSocketDesc m_socket_desc;

    socklen_t m_socket_addr_len = sizeof(sockaddr_in);

public:
    Socket(const char *, WhichSocket);
    virtual ~Socket() = 0;

    void start(ServerSocketDesc &);
    void stop();

    virtual SocketError update() = 0;
    SocketState getState();
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ConnectedClient
{
    int m_socket_fd;
    WhichSocket m_connected_at;

    ConnectedClient(){};
    ConnectedClient(int socket_fd, WhichSocket connected_at) : m_socket_fd(socket_fd), m_connected_at(connected_at) {}
};

class TcpSocket : public Socket
{

    uint8_t m_nb_allowed_clients;

public:
    TcpSocket(const char *, uint8_t, WhichSocket);
    ~TcpSocket();

    SocketError update();
    Option<int> tryToConnetClient(sockaddr_in *);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// enum class DatagramError{

// }

class UdpSocket : public Socket
{
public:
    UdpSocket(const char *, WhichSocket);
    ~UdpSocket();

    SocketError update();
    int getFd();
};

/*
Result<int, ClientError> tryToSendBytes(void *dataptr, size_t size)
    {
        int r = send(m_socket, dataptr, size, 0);

        if (r == 0)
        {
            printf("Client_%d NoResponse\n", m_id);
            return Result<int, ClientError>(ClientError::NoResponse);
        }
        else if (r > 0)
        {
            return Result<int, ClientError>(r);
        }
        else if (errno == SOCKET_ERR_TRY_AGAIN)
        {
            return Result<int, ClientError>(0);
        }
        else
        {
            printf("Client_%d exit with err %d\n", m_id, errno);
            return Result<int, ClientError>(ClientError::SocketError);
        }
    }
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class SocketsHandlerError
{
    None,
    ErrorOnSta,
    ErrorOnAp,
    ErrorOnApSta
};

enum class HandlerState
{
    NotStarted,
    InitializingAp,
    InitializingSta,
    InitializingApSta,
    ReadyOnSta,
    ReadyOnAp,
    ReadyOnApSta,
    ErrorOnAp,
    ErrorOnSta,
    ErrorOnApSta,
};

// /// TO TEST LATER
// class TransportSocketsHandler
// {
// protected:
//     Socket *m_ap_socket;
//     Socket *m_sta_socket;

//     HandlerState m_state;
//     SocketsHandlerError m_error;

// public:
//     TransportSocketsHandler();
//     virtual ~TransportSocketsHandler() = 0;

//     void start(ApStaSocketsDesc &);
//     void stop();

//     SocketsHandlerError update();
//     HandlerState getState();
//     bool isReady();
//     Option<ConnectedClient> tryToConnetClient(sockaddr_in *);
// };
// class TcpSocketsHandler: public TransportSocketsHandler

class TcpSocketsHandler
{
    const char *M_LOG_TAG = "TcpSocketsHandler";

    TcpSocket *m_ap_socket;
    TcpSocket *m_sta_socket;

    HandlerState m_state;
    SocketsHandlerError m_error;

    uint8_t m_nb_allowed_clients;

public:
    TcpSocketsHandler(uint8_t);
    ~TcpSocketsHandler();

    void start(ApStaSocketsDesc &);
    void stop();

    SocketsHandlerError update();
    HandlerState getState();
    bool isReady();
    Option<ConnectedClient> tryToConnetClient(sockaddr_in *);
};

class UdpSocketsHandler
{
    const char *M_LOG_TAG = "UdpSocketsHandler";

    UdpSocket *m_ap_socket;
    UdpSocket *m_sta_socket;

    HandlerState m_state;
    SocketsHandlerError m_error;

public:
    UdpSocketsHandler();
    ~UdpSocketsHandler();

    void start(ApStaSocketsDesc &);
    void stop();

    SocketsHandlerError update();
    HandlerState getState();
    bool isReady();
    Option<int> getApSocketFd();
    Option<int> getStaSocketFd();
};

class SocketsHandler
{
    UdpSocketsHandler *m_udp_handler;
    TcpSocketsHandler *m_tcp_handler;

public:
    SocketsHandler(uint8_t);
    ~SocketsHandler();

    void start(ApStaSocketsDesc &);
    void stop();

    SocketsHandlerError update();
    bool isReady();
    Option<ConnectedClient> tryToConnetClient(sockaddr_in *);
    Option<int> getApUdpFd();
    Option<int> getStaUdpFd();
};

#endif // SOCKETS_HANDLER_H