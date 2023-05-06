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
            ESP_LOGE(M_LOG_TAG, "Failed to create socket: %d", m_type);
            m_error = SocketError::CannotCreate;
            m_state = SocketState::Error;
            return SocketError::CannotCreate;
        }

        setSocketNonBlocking(m_socket);

        ESP_LOGI(M_LOG_TAG, "Socket_%d Created", m_type);

        m_state = SocketState::Created;

        break;
    }
    case SocketState::Created:
    {
        if (bind(m_socket, (struct sockaddr *)&m_socket_desc.addr, m_socket_addr_len) < 0)
        {
            ESP_LOGE(M_LOG_TAG, "Failed to bind socket_%d", m_type);
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
            ESP_LOGE(M_LOG_TAG, "Cannot listen on socket_%d", m_type);
            m_error = SocketError::CannotListen;
            m_state = SocketState::Error;
            return SocketError::CannotListen;
        }

        ESP_LOGI(M_LOG_TAG, "Socket_%d start listening", m_type);

        m_state = SocketState::Listening;

        break;
    }
    // TODO
    // case SocketState::Listening:
    // {

    //     tryToConnetClient();

    //     if (!m_pending_send_msg.isEmpty())
    //     {
    //         tryToSendMsg(m_pending_send_msg.get().getData());
    //     }

    //     m_clients.update();

    //     tryToRecvMsg();

    //     break;
    // }
    default:
    {
        break;
    }
    }

    return SocketError::None;
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

        ESP_LOGI(M_LOG_TAG, "Client connected to %d: IP: %s | Port: %d\n", m_type, inet_ntoa(next_client_addr->sin_addr), (int)ntohs(next_client_addr->sin_port));

        return Option<int>(client_socket);
    }

    return Option<int>();
}