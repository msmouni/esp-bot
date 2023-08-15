#include "handler.h"

void setSocketNonBlocking(int socket_desc)
{
    /* NON-BLOCKING FLAG:
        1- Call the fcntl() API to retrieve the socket descriptor's current flag settings into a local variable.
        2- In our local variable, set the O_NONBLOCK (non-blocking) flag on. (being careful, of course, not to tamper with the other flags)
        3- Call the fcntl() API to set the flags for the descriptor to the value in our local variable.
    */
    int flags = fcntl(socket_desc, F_GETFL) | O_NONBLOCK;
    fcntl(socket_desc, F_SETFL, flags);
}

Socket::Socket(const char *log, WhichSocket s_type)
{
    M_LOG_TAG = log;
    m_type = s_type;
    m_state = SocketState::NotStarted;
    m_error = SocketError::None;
}

Socket::~Socket()
{
    close(m_socket);
}

void Socket::start(ServerSocketDesc &socket_desc)
{
    m_socket_desc = socket_desc;
    m_state = SocketState::Uninitialized;
}

SocketState Socket::getState()
{
    return m_state;
}

void Socket::stop()
{
    if ((m_state != SocketState::NotStarted) & (m_state != SocketState::Uninitialized))
    {
        // According to manual: Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
        close(m_socket);
    }

    m_state = SocketState::NotStarted;
    m_error = SocketError::None;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TcpSocket::TcpSocket(const char *log, uint8_t nb_allowed_clients, WhichSocket s_type) : Socket(log, s_type)
{
    m_nb_allowed_clients = nb_allowed_clients;
}

TcpSocket::~TcpSocket()
{
    close(m_socket);
}

SocketError TcpSocket::update()
{
    switch (m_state)
    {
    case SocketState::Uninitialized:
    {
        m_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (m_socket < 0)
        {
            ESP_LOGE(M_LOG_TAG, "Failed to create socket: %d", (uint8_t)m_type);
            m_error = SocketError::CannotCreate;
            m_state = SocketState::Error;
            return SocketError::CannotCreate;
        }

        setSocketNonBlocking(m_socket);

        ESP_LOGI(M_LOG_TAG, "Socket_%d Created", (uint8_t)m_type);

        m_state = SocketState::Created;

        break;
    }
    case SocketState::Created:
    {
        if (bind(m_socket, (struct sockaddr *)&m_socket_desc.addr, m_socket_addr_len) < 0)
        {
            ESP_LOGE(M_LOG_TAG, "Failed to bind socket_%d", (uint8_t)m_type);
            m_error = SocketError::CannotBind;
            m_state = SocketState::Error;
            return SocketError::CannotBind;
        }

        m_state = SocketState::Bound;

        break;
    }
    case SocketState::Bound:
    {
        if (listen(m_socket, m_nb_allowed_clients) < 0)
        {
            ESP_LOGE(M_LOG_TAG, "Cannot listen on socket_%d", (uint8_t)m_type);
            m_error = SocketError::CannotListen;
            m_state = SocketState::Error;
            return SocketError::CannotListen;
        }

        ESP_LOGI(M_LOG_TAG, "Socket_%d start listening", (uint8_t)m_type);

        m_state = SocketState::Ready;

        break;
    }
    default:
    {
        break;
    }
    }

    return SocketError::None;
}

Option<int> TcpSocket::tryToConnetClient(sockaddr_in *next_client_addr)
{
    /*
    On success, these system calls return a file descriptor for the
    accepted socket (a nonnegative integer).  On error, -1 is
    returned, errno is set to indicate the error, and addrlen is left
    unchanged.
    */
    int client_socket = accept(m_socket, (sockaddr *)next_client_addr, &m_socket_addr_len);

    if (client_socket >= 0)
    {
        setSocketNonBlocking(client_socket);

        ESP_LOGI(M_LOG_TAG, "Client connected to %d: IP: %s | Port: %d\n", (uint8_t)m_type, inet_ntoa(next_client_addr->sin_addr), (int)ntohs(next_client_addr->sin_port));

        return Option<int>(client_socket);
    }

    return Option<int>();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UdpSocket::UdpSocket(const char *log, WhichSocket s_type) : Socket(log, s_type)
{
}

UdpSocket::~UdpSocket()
{
    close(m_socket);
}

SocketError UdpSocket::update()
{
    switch (m_state)
    {
    case SocketState::Uninitialized:
    {
        m_socket = socket(AF_INET, SOCK_DGRAM, 0);

        if (m_socket < 0)
        {
            ESP_LOGE(M_LOG_TAG, "Failed to create socket: %d", (uint8_t)m_type);
            m_error = SocketError::CannotCreate;
            m_state = SocketState::Error;
            return SocketError::CannotCreate;
        }

        setSocketNonBlocking(m_socket);

        ESP_LOGI(M_LOG_TAG, "Socket_%d Created", (uint8_t)m_type);

        m_state = SocketState::Created;

        break;
    }
    case SocketState::Created:
    {
        if (bind(m_socket, (struct sockaddr *)&m_socket_desc.addr, m_socket_addr_len) < 0)
        {
            ESP_LOGE(M_LOG_TAG, "Failed to bind socket_%d", (uint8_t)m_type);
            m_error = SocketError::CannotBind;
            m_state = SocketState::Error;
            return SocketError::CannotBind;
        }

        m_state = SocketState::Ready;

        break;
    }
    default:
    {
        break;
    }
    }

    return SocketError::None;
}

int UdpSocket::getFd()
{
    return m_socket;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////

TcpSocketsHandler::TcpSocketsHandler(uint8_t nb_allowed_clients)
{
    m_ap_socket = new TcpSocket(M_LOG_TAG, nb_allowed_clients, WhichSocket::Ap);
    m_sta_socket = new TcpSocket(M_LOG_TAG, nb_allowed_clients, WhichSocket::Sta);

    m_state = HandlerState::NotStarted;
    m_error = SocketsHandlerError::None;

    m_nb_allowed_clients = nb_allowed_clients;
}

TcpSocketsHandler::~TcpSocketsHandler()
{
    m_ap_socket->stop();
    m_sta_socket->stop();

    delete m_ap_socket;
    delete m_sta_socket;
}

void TcpSocketsHandler::start(ApStaSocketsDesc &sockets_desc)
{
    bool has_ap_sock = sockets_desc.m_ap_socket_desc.isSome();
    bool has_sta_sock = sockets_desc.m_sta_socket_desc.isSome();

    if (has_ap_sock && has_sta_sock)
    {
        m_ap_socket->start(sockets_desc.m_ap_socket_desc.getData());
        m_sta_socket->start(sockets_desc.m_sta_socket_desc.getData());
        m_state = HandlerState::InitializingApSta;
    }
    else if (has_ap_sock)
    {
        m_ap_socket->start(sockets_desc.m_ap_socket_desc.getData());
        m_state = HandlerState::InitializingAp;
    }
    else if (has_sta_sock)
    {
        m_sta_socket->start(sockets_desc.m_sta_socket_desc.getData());
        m_state = HandlerState::InitializingSta;
    }
    else
    {
        m_state = HandlerState::ErrorOnApSta;
    }
}

void TcpSocketsHandler::stop()
{
    m_ap_socket->stop();
    m_sta_socket->stop();

    m_state = HandlerState::NotStarted;
    m_error = SocketsHandlerError::None;
}

SocketsHandlerError TcpSocketsHandler::update()
{
    switch (m_state)
    {
    case HandlerState::InitializingApSta:
    {
        SocketError res_ap = m_ap_socket->update();
        SocketError res_sta = m_sta_socket->update();

        if ((res_ap != SocketError::None) || (res_sta != SocketError::None))
        {
            m_state = HandlerState::ErrorOnApSta;
            return SocketsHandlerError::ErrorOnApSta;
        }
        else if (res_ap != SocketError::None)
        {
            if (m_sta_socket->getState() == SocketState::Ready)
            {
                m_state = HandlerState::ReadyOnSta;
            }
            return SocketsHandlerError::ErrorOnAp;
        }
        else if (res_sta != SocketError::None)
        {
            if (m_ap_socket->getState() == SocketState::Ready)
            {
                m_state = HandlerState::ReadyOnAp;
            }
            return SocketsHandlerError::ErrorOnSta;
        }
        else if ((m_ap_socket->getState() == SocketState::Ready) && (m_sta_socket->getState() == SocketState::Ready))
        {
            m_state = HandlerState::ReadyOnApSta;
        }

        break;
    }
    case HandlerState::InitializingAp:
    {
        SocketError res_ap = m_ap_socket->update();

        if (res_ap != SocketError::None)
        {
            m_state = HandlerState::ErrorOnAp;

            return SocketsHandlerError::ErrorOnApSta;
        }
        else if (m_ap_socket->getState() == SocketState::Ready)
        {
            m_state = HandlerState::ReadyOnAp;
        }

        break;
    }
    case HandlerState::InitializingSta:
    {
        SocketError res_sta = m_sta_socket->update();

        if (res_sta != SocketError::None)
        {
            m_state = HandlerState::ErrorOnSta;

            return SocketsHandlerError::ErrorOnApSta;
        }
        else if (m_sta_socket->getState() == SocketState::Ready)
        {
            m_state = HandlerState::ReadyOnSta;
        }

        break;
    }
    default:
    {
        break;
    }
    }

    return SocketsHandlerError::None;
}

HandlerState TcpSocketsHandler::getState()
{
    return m_state;
}

bool TcpSocketsHandler::isReady()
{
    return (m_state == HandlerState::ReadyOnApSta) || (m_state == HandlerState::ReadyOnAp) || (m_state == HandlerState::ReadyOnSta);
}

Option<ConnectedClient> TcpSocketsHandler::tryToConnetClient(sockaddr_in *next_client_addr)
{
    // Prioritizing AP over STA
    Option<int> res_ap = m_ap_socket->tryToConnetClient(next_client_addr);

    if (res_ap.isSome())
    {
        return Option<ConnectedClient>(ConnectedClient(res_ap.getData(), WhichSocket::Ap));
    }
    else
    {
        Option<int> res_sta = m_sta_socket->tryToConnetClient(next_client_addr);

        if (res_sta.isSome())
        {
            return Option<ConnectedClient>(ConnectedClient(res_sta.getData(), WhichSocket::Sta));
        }
    }

    return Option<ConnectedClient>();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////// functions are mostly implemented the same way as for TcpSocketsHandler (template functions ... OR Something like TransportSocketsHandler)

UdpSocketsHandler::UdpSocketsHandler()
{
    m_ap_socket = new UdpSocket(M_LOG_TAG, WhichSocket::Ap);
    m_sta_socket = new UdpSocket(M_LOG_TAG, WhichSocket::Sta);

    m_state = HandlerState::NotStarted;
    m_error = SocketsHandlerError::None;
}

UdpSocketsHandler::~UdpSocketsHandler()
{
    m_ap_socket->stop();
    m_sta_socket->stop();

    delete m_ap_socket;
    delete m_sta_socket;
}

void UdpSocketsHandler::start(ApStaSocketsDesc &sockets_desc)
{
    bool has_ap_sock = sockets_desc.m_ap_socket_desc.isSome();
    bool has_sta_sock = sockets_desc.m_sta_socket_desc.isSome();

    if (has_ap_sock && has_sta_sock)
    {
        m_ap_socket->start(sockets_desc.m_ap_socket_desc.getData());
        m_sta_socket->start(sockets_desc.m_sta_socket_desc.getData());
        m_state = HandlerState::InitializingApSta;
    }
    else if (has_ap_sock)
    {
        m_ap_socket->start(sockets_desc.m_ap_socket_desc.getData());
        m_state = HandlerState::InitializingAp;
    }
    else if (has_sta_sock)
    {
        m_sta_socket->start(sockets_desc.m_sta_socket_desc.getData());
        m_state = HandlerState::InitializingSta;
    }
    else
    {
        m_state = HandlerState::ErrorOnApSta;
    }
}

void UdpSocketsHandler::stop()
{
    m_ap_socket->stop();
    m_sta_socket->stop();

    m_state = HandlerState::NotStarted;
    m_error = SocketsHandlerError::None;
}

SocketsHandlerError UdpSocketsHandler::update()
{
    switch (m_state)
    {
    case HandlerState::InitializingApSta:
    {
        SocketError res_ap = m_ap_socket->update();
        SocketError res_sta = m_sta_socket->update();

        if ((res_ap != SocketError::None) || (res_sta != SocketError::None))
        {
            m_state = HandlerState::ErrorOnApSta;
            return SocketsHandlerError::ErrorOnApSta;
        }
        else if (res_ap != SocketError::None)
        {
            if (m_sta_socket->getState() == SocketState::Ready)
            {
                m_state = HandlerState::ReadyOnSta;
            }
            return SocketsHandlerError::ErrorOnAp;
        }
        else if (res_sta != SocketError::None)
        {
            if (m_ap_socket->getState() == SocketState::Ready)
            {
                m_state = HandlerState::ReadyOnAp;
            }
            return SocketsHandlerError::ErrorOnSta;
        }
        else if ((m_ap_socket->getState() == SocketState::Ready) && (m_sta_socket->getState() == SocketState::Ready))
        {
            m_state = HandlerState::ReadyOnApSta;
        }

        break;
    }
    case HandlerState::InitializingAp:
    {
        SocketError res_ap = m_ap_socket->update();

        if (res_ap != SocketError::None)
        {
            m_state = HandlerState::ErrorOnAp;

            return SocketsHandlerError::ErrorOnApSta;
        }
        else if (m_ap_socket->getState() == SocketState::Ready)
        {
            m_state = HandlerState::ReadyOnAp;
        }

        break;
    }
    case HandlerState::InitializingSta:
    {
        SocketError res_sta = m_sta_socket->update();

        if (res_sta != SocketError::None)
        {
            m_state = HandlerState::ErrorOnSta;

            return SocketsHandlerError::ErrorOnApSta;
        }
        else if (m_sta_socket->getState() == SocketState::Ready)
        {
            m_state = HandlerState::ReadyOnSta;
        }

        break;
    }
    default:
    {
        break;
    }
    }

    return SocketsHandlerError::None;
}

HandlerState UdpSocketsHandler::getState()
{
    return m_state;
}

bool UdpSocketsHandler::isReady()
{
    return (m_state == HandlerState::ReadyOnApSta) || (m_state == HandlerState::ReadyOnAp) || (m_state == HandlerState::ReadyOnSta);
}

Option<int> UdpSocketsHandler::getApSocketFd()
{
    if (isReady())
    {
        return Option<int>(m_ap_socket->getFd());
    }
    else
    {
        return Option<int>();
    }
}

Option<int> UdpSocketsHandler::getStaSocketFd()
{
    if (isReady())
    {
        return Option<int>(m_sta_socket->getFd());
    }
    else
    {
        return Option<int>();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

SocketsHandler::SocketsHandler(uint8_t nb_allowed_clients)
{
    m_udp_handler = new UdpSocketsHandler();
    m_tcp_handler = new TcpSocketsHandler(nb_allowed_clients);
}

SocketsHandler::~SocketsHandler()
{
    delete m_udp_handler;
    delete m_tcp_handler;
}

void SocketsHandler::start(ApStaSocketsDesc &sock_desc)
{
    // if (m_udp_handler != nullptr && m_tcp_handler != nullptr)
    // {
    m_udp_handler->start(sock_desc);
    m_tcp_handler->start(sock_desc);
    // }
}

void SocketsHandler::stop()
{

    m_udp_handler->stop();
    m_tcp_handler->stop();
}

SocketsHandlerError SocketsHandler::update()
{
    SocketsHandlerError tcp_res = m_tcp_handler->update();

    if (tcp_res != SocketsHandlerError::None)
    {
        return tcp_res;
    }
    else
    {
        return m_udp_handler->update();
    }
}

bool SocketsHandler::isReady()
{
    return m_tcp_handler->isReady() && m_udp_handler->isReady();
}

Option<ConnectedClient> SocketsHandler::tryToConnetClient(sockaddr_in *next_client_addr)
{
    return m_tcp_handler->tryToConnetClient(next_client_addr);
}

Option<int> SocketsHandler::getApUdpFd()
{
    return m_udp_handler->getApSocketFd();
}

Option<int> SocketsHandler::getStaUdpFd()
{
    return m_udp_handler->getStaSocketFd();
}
