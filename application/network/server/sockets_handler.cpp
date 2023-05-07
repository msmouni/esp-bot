#include "sockets_handler.h"

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

Socket::Socket(const char *log, uint8_t nb_allowed_clients, WhichSocket s_type)
{
    M_LOG_TAG = log;
    m_type = s_type;
    m_nb_allowed_clients = nb_allowed_clients;
    m_state = SocketState::NotStarted;
    m_error = SocketError::None;
}

Socket::~Socket()
{
    close(m_socket);
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

SocketError Socket::update()
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

SocketState Socket::getState()
{
    return m_state;
}

Option<int> Socket::tryToConnetClient(sockaddr_in *next_client_addr)
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

///////////////////////////////////////////////////////////////////

SocketsHandler::SocketsHandler(uint8_t nb_allowed_clients)
{
    m_ap_socket = new Socket(M_LOG_TAG, nb_allowed_clients, WhichSocket::Ap);
    m_sta_socket = new Socket(M_LOG_TAG, nb_allowed_clients, WhichSocket::Sta);

    m_state = ApStaSocketsState::NotStarted;
    m_error = SocketsHandlerError::None;

    m_nb_allowed_clients = nb_allowed_clients;
}

SocketsHandler::~SocketsHandler()
{
    m_ap_socket->stop();
    m_sta_socket->stop();
}

void SocketsHandler::start(ApStaSocketsDesc sockets_desc)
{
    bool has_ap_sock = sockets_desc.m_ap_socket_desc.isSome();
    bool has_sta_sock = sockets_desc.m_sta_socket_desc.isSome();

    if (has_ap_sock && has_sta_sock)
    {
        m_ap_socket->start(sockets_desc.m_ap_socket_desc.getData());
        m_sta_socket->start(sockets_desc.m_sta_socket_desc.getData());
        m_state = ApStaSocketsState::InitializingApSta;
    }
    else if (has_ap_sock)
    {
        m_ap_socket->start(sockets_desc.m_ap_socket_desc.getData());
        m_state = ApStaSocketsState::InitializingAp;
    }
    else if (has_sta_sock)
    {
        m_sta_socket->start(sockets_desc.m_sta_socket_desc.getData());
        m_state = ApStaSocketsState::InitializingSta;
    }
    else
    {
        m_state = ApStaSocketsState::ErrorOnApSta;
    }
}

void SocketsHandler::stop()
{
    m_ap_socket->stop();
    m_sta_socket->stop();

    m_state = ApStaSocketsState::NotStarted;
    m_error = SocketsHandlerError::None;
}

SocketsHandlerError SocketsHandler::update()
{
    switch (m_state)
    {
    case ApStaSocketsState::InitializingApSta:
    {
        SocketError res_ap = m_ap_socket->update();
        SocketError res_sta = m_sta_socket->update();

        if ((res_ap != SocketError::None) || (res_sta != SocketError::None))
        {
            m_state = ApStaSocketsState::ErrorOnApSta;
            return SocketsHandlerError::ErrorOnApSta;
        }
        else if (res_ap != SocketError::None)
        {
            if (m_sta_socket->getState() == SocketState::Listening)
            {
                m_state = ApStaSocketsState::ListeningOnSta;
            }
            return SocketsHandlerError::ErrorOnAp;
        }
        else if (res_sta != SocketError::None)
        {
            if (m_ap_socket->getState() == SocketState::Listening)
            {
                m_state = ApStaSocketsState::ListeningOnAp;
            }
            return SocketsHandlerError::ErrorOnSta;
        }
        else if ((m_ap_socket->getState() == SocketState::Listening) && (m_sta_socket->getState() == SocketState::Listening))
        {
            m_state = ApStaSocketsState::ListeningOnApSta;
        }

        break;
    }
    case ApStaSocketsState::InitializingAp:
    {
        SocketError res_ap = m_ap_socket->update();

        if (res_ap != SocketError::None)
        {
            m_state = ApStaSocketsState::ErrorOnAp;

            return SocketsHandlerError::ErrorOnApSta;
        }
        else if (m_ap_socket->getState() == SocketState::Listening)
        {
            m_state = ApStaSocketsState::ListeningOnAp;
        }

        break;
    }
    case ApStaSocketsState::InitializingSta:
    {
        SocketError res_sta = m_sta_socket->update();

        if (res_sta != SocketError::None)
        {
            m_state = ApStaSocketsState::ErrorOnSta;

            return SocketsHandlerError::ErrorOnApSta;
        }
        else if (m_sta_socket->getState() == SocketState::Listening)
        {
            m_state = ApStaSocketsState::ListeningOnSta;
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

ApStaSocketsState SocketsHandler::getState()
{
    return m_state;
}

bool SocketsHandler::isListening()
{
    return (m_state == ApStaSocketsState::ListeningOnApSta) || (m_state == ApStaSocketsState::ListeningOnAp) || (m_state == ApStaSocketsState::ListeningOnSta);
}

Option<int> SocketsHandler::tryToConnetClient(sockaddr_in *next_client_addr)
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
