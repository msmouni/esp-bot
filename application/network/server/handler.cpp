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

void Socket::start(ServerSocketDesc socket_desc)
{
    m_socket_desc = socket_desc;
    m_state = SocketState::Uninitialized;
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

SocketState Socket::getState()
{
    return m_state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TcpSocket::TcpSocket(const char *log, uint8_t nb_allowed_clients, WhichSocket s_type)
{
    M_LOG_TAG = log;
    m_type = s_type;
    m_nb_allowed_clients = nb_allowed_clients;
    m_state = SocketState::NotStarted;
    m_error = SocketError::None;
}

TcpSocket::~TcpSocket()
{
    close(m_socket);
}

// void TcpSocket::start(ServerSocketDesc socket_desc)
// {
//     m_socket_desc = socket_desc;
//     m_state = SocketState::Uninitialized;
// }

// void TcpSocket::stop()
// {
//     if ((m_state != SocketState::NotStarted) & (m_state != SocketState::Uninitialized))
//     {
//         // According to manual: Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
//         close(m_socket);
//     }

//     m_state = SocketState::NotStarted;
//     m_error = SocketError::None;
// }

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

        m_state = SocketState::Listening;

        break;
    }
    default:
    {
        break;
    }
    }

    return SocketError::None;
}

// SocketState TcpSocket::getState()
// {
//     return m_state;
// }

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
UdpSocket::UdpSocket(const char *log, uint8_t nb_allowed_clients, WhichSocket s_type)
{
    M_LOG_TAG = log;
    m_type = s_type;
    m_nb_allowed_clients = nb_allowed_clients;
    m_state = SocketState::NotStarted;
    m_error = SocketError::None;
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

        m_state = SocketState::Bound;

        break;
    }
    default:
    {
        break;
    }
    }

    return SocketError::None;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////

TcpSocketsHandler::TcpSocketsHandler(uint8_t nb_allowed_clients)
{
    m_ap_socket = new TcpSocket(M_LOG_TAG, nb_allowed_clients, WhichSocket::Ap);
    m_sta_socket = new TcpSocket(M_LOG_TAG, nb_allowed_clients, WhichSocket::Sta);

    m_state = ApStaTcpSocketsState::NotStarted;
    m_error = TcpSocketsHandlerError::None;

    m_nb_allowed_clients = nb_allowed_clients;
}

TcpSocketsHandler::~TcpSocketsHandler()
{
    m_ap_socket->stop();
    m_sta_socket->stop();

    delete m_ap_socket;
    delete m_sta_socket;
}

void TcpSocketsHandler::start(ApStaSocketsDesc sockets_desc)
{
    bool has_ap_sock = sockets_desc.m_ap_socket_desc.isSome();
    bool has_sta_sock = sockets_desc.m_sta_socket_desc.isSome();

    if (has_ap_sock && has_sta_sock)
    {
        m_ap_socket->start(sockets_desc.m_ap_socket_desc.getData());
        m_sta_socket->start(sockets_desc.m_sta_socket_desc.getData());
        m_state = ApStaTcpSocketsState::InitializingApSta;
    }
    else if (has_ap_sock)
    {
        m_ap_socket->start(sockets_desc.m_ap_socket_desc.getData());
        m_state = ApStaTcpSocketsState::InitializingAp;
    }
    else if (has_sta_sock)
    {
        m_sta_socket->start(sockets_desc.m_sta_socket_desc.getData());
        m_state = ApStaTcpSocketsState::InitializingSta;
    }
    else
    {
        m_state = ApStaTcpSocketsState::Error;
        m_error = TcpSocketsHandlerError::ErrorOnApSta;
    }
}

void TcpSocketsHandler::stop()
{
    m_ap_socket->stop();
    m_sta_socket->stop();

    m_state = ApStaTcpSocketsState::NotStarted;
    m_error = TcpSocketsHandlerError::None;
}

TcpSocketsHandlerError TcpSocketsHandler::update()
{
    switch (m_state)
    {
    case ApStaTcpSocketsState::InitializingApSta:
    {
        SocketError res_ap = m_ap_socket->update();
        SocketError res_sta = m_sta_socket->update();

        if ((res_ap != SocketError::None) && (res_sta != SocketError::None))
        {
            m_state = ApStaTcpSocketsState::Error;
            return TcpSocketsHandlerError::ErrorOnApSta;
        }
        else if (res_ap != SocketError::None)
        {
            if (m_sta_socket->getState() == SocketState::Listening)
            {
                m_state = ApStaTcpSocketsState::ListeningOnSta;
            }
            return TcpSocketsHandlerError::ErrorOnAp;
        }
        else if (res_sta != SocketError::None)
        {
            if (m_ap_socket->getState() == SocketState::Listening)
            {
                m_state = ApStaTcpSocketsState::ListeningOnAp;
            }
            return TcpSocketsHandlerError::ErrorOnSta;
        }
        else if ((m_ap_socket->getState() == SocketState::Listening) && (m_sta_socket->getState() == SocketState::Listening))
        {
            m_state = ApStaTcpSocketsState::ListeningOnApSta;
        }

        break;
    }
    case ApStaTcpSocketsState::InitializingAp:
    {
        SocketError res_ap = m_ap_socket->update();

        if (res_ap != SocketError::None)
        {
            m_state = ApStaTcpSocketsState::Error;

            return TcpSocketsHandlerError::ErrorOnAp;
        }
        else if (m_ap_socket->getState() == SocketState::Listening)
        {
            m_state = ApStaTcpSocketsState::ListeningOnAp;
        }

        break;
    }
    case ApStaTcpSocketsState::InitializingSta:
    {
        SocketError res_sta = m_sta_socket->update();

        if (res_sta != SocketError::None)
        {
            m_state = ApStaTcpSocketsState::Error;

            return TcpSocketsHandlerError::ErrorOnSta;
        }
        else if (m_sta_socket->getState() == SocketState::Listening)
        {
            m_state = ApStaTcpSocketsState::ListeningOnSta;
        }

        break;
    }
    default:
    {
        break;
    }
    }

    return TcpSocketsHandlerError::None;
}

ApStaTcpSocketsState TcpSocketsHandler::getState()
{
    return m_state;
}

bool TcpSocketsHandler::isListening()
{
    return (m_state == ApStaTcpSocketsState::ListeningOnApSta) || (m_state == ApStaTcpSocketsState::ListeningOnAp) || (m_state == ApStaTcpSocketsState::ListeningOnSta);
}

Option<int> TcpSocketsHandler::tryToConnetClient(sockaddr_in *next_client_addr)
{
    // Prioritizing AP over STA
    Option<int> res_ap = m_ap_socket->tryToConnetClient(next_client_addr);

    if (res_ap.isSome())
    {
        return res_ap;
    }
    else
    {
        Option<int> res_sta = m_sta_socket->tryToConnetClient(next_client_addr);

        if (res_sta.isSome())
        {
            return res_sta;
        }
    }

    return Option<int>();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SocketsHandler::SocketsHandler(uint8_t nb_allowed_clients)
{
    m_tcp_handler = new TcpSocketsHandler(nb_allowed_clients);

    m_state = SocketsState::NotStarted;
}

void SocketsHandler::start(ApStaSocketsDesc sockets_desc)
{
    m_tcp_handler->start(sockets_desc);
}

void SocketsHandler::stop()
{
    m_tcp_handler->stop();
}

SocketsState SocketsHandler::getState()
{
    return m_state;
}
bool SocketsHandler::isReady()
{
    return (m_state == SocketsState::RunningTcpOnAp) ||
           (m_state == SocketsState::RunningTcpOnSta) ||
           (m_state == SocketsState::RunningTcpOnApSta) ||
           (m_state == SocketsState::RunningUdpOnAp) ||
           (m_state == SocketsState::RunningUdpOnSta) ||
           (m_state == SocketsState::RunningUdpOnApSta) ||
           (m_state == SocketsState::RunningTcpUdpOnAp) ||
           (m_state == SocketsState::RunningTcpUdpOnSta) ||
           (m_state == SocketsState::RunningTcpUdpOnApSta);
}
Option<int> SocketsHandler::tryToConnetClient(sockaddr_in *next_client_addr)
{
    return m_tcp_handler->tryToConnetClient(next_client_addr);
}
// ~SocketsHandler() = default;

// void start(ApStaSocketsDesc);
// void stop();

// SocketsHandlerError update();
// SocketsState getState();
// bool isListening();
// Option<int> tryToConnetClient(sockaddr_in *);