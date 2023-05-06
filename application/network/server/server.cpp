#include "server.h"

CircularBuffer<ServerFrame<TcpIpServer::MAX_MSG_SIZE>, 50> TcpIpServer::m_pending_send_msg = {};

void TcpIpServer::tryToSendMsg_25ms(void *args)
{
    // TMP
    uint8_t data[MAX_MSG_SIZE - 5] = {0};
    data[0] = 1;
    data[1] = 2;
    data[2] = 3;
    data[3] = 4;
    data[4] = 5;
    ServerFrame<MAX_MSG_SIZE> status_frame = ServerFrame<MAX_MSG_SIZE>(ServerFrameId::Status, 5, data);

    m_pending_send_msg.push(status_frame);
}

TcpIpServer::TcpIpServer()
{
    m_state = ServerState::NotStarted;
}
TcpIpServer::~TcpIpServer()
{
    // close(m_socket);
    m_socket_handler.stop();
    delete m_timer_send_25ms;
}

void TcpIpServer::start(ServerSocketDesc socket_desc, ServerLogin login)
{
    // m_socket_desc = socket_desc;
    m_socket_handler.start(socket_desc);
    m_clients.setServerLogin(login);
    m_timer_send_25ms = new PeriodicTimer("TCP_IP_Server_25ms", tryToSendMsg_25ms, NULL, 25000);
    m_state = ServerState::Uninitialized;
}

void TcpIpServer::stop()
{
    if ((m_state != ServerState::NotStarted) & (m_state != ServerState::Uninitialized))
    {
        // // According to manual: Upon successful completion, 0 shall be returned; otherwise, -1 shall be returned and errno set to indicate the error.
        // close(m_socket);

        m_socket_handler.stop();
    }

    if (m_state == ServerState::Running)
    {
        m_timer_send_25ms->stop();
    }

    m_state = ServerState::NotStarted;
}

ServerError TcpIpServer::update()
{
    switch (m_state)
    {
    case ServerState::Uninitialized:
    {
        if (m_socket_handler.update() != SocketError::None)
        {
            m_error = ServerError::SocketError;
            m_state = ServerState::Error;
            return ServerError::SocketError;
        }
        else if (m_socket_handler.getState() == SocketState::Listening)
        {
            m_state = ServerState::SocketsListening;
        }

        break;
        /*m_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (m_socket < 0)
        {
            ESP_LOGE(SERVER_TAG, "Failed to create a socket...");
            m_error = ServerError::CannotCreateSocket;
            m_state = ServerState::Error;
            return ServerError::CannotCreateSocket;
        }

        setSocketNonBlocking(m_socket);

        ESP_LOGI(SERVER_TAG, "TCP/IP Server Created");

        m_state = ServerState::SocketCreated;

        break;*/
    }
    case ServerState::SocketsListening:
    {
        esp_err_t res_strt = m_timer_send_25ms->start();
        if (res_strt != ESP_OK)
        {

            m_error = ServerError::ErrorStarting25msTimer;
            m_state = ServerState::Error;
            return ServerError::ErrorStarting25msTimer;

            ESP_LOGE(SERVER_TAG, "Error while starting server's 25ms timer: %d", res_strt);
        }

        ESP_LOGI(SERVER_TAG, "TCP/IP Server Started");

        m_state = ServerState::Running;

        break;
    }
    /*case ServerState::SocketCreated:
    {
        if (bind(m_socket, (struct sockaddr *)&m_socket_desc.addr, m_socket_addr_len) < 0)
        {
            ESP_LOGE(SERVER_TAG, "Failed to bind socket...");
            m_error = ServerError::CannotBindSocket;
            m_state = ServerState::Error;
            return ServerError::CannotBindSocket;
        }

        m_state = ServerState::SocketBound;

        break;
    }
    case ServerState::SocketBound:
    {
        if (listen(m_socket, NB_ALLOWED_CLIENTS) < 0)
        {
            ESP_LOGE(SERVER_TAG, "Cannot listen on socket...");
            m_error = ServerError::CannotListenOnSocket;
            m_state = ServerState::Error;
            return ServerError::CannotListenOnSocket;
        }

        esp_err_t res_strt = m_timer_send_25ms->start();
        if (res_strt != ESP_OK)
        {

            m_error = ServerError::ErrorStarting25msTimer;
            m_state = ServerState::Error;
            return ServerError::ErrorStarting25msTimer;

            ESP_LOGE(SERVER_TAG, "Error while starting server's 25ms timer: %d", res_strt);
        }

        ESP_LOGI(SERVER_TAG, "TCP/IP Server Started");

        m_state = ServerState::Running;

        break;
    }*/
    case ServerState::Running:
    {

        tryToConnetClient();

        if (!m_pending_send_msg.isEmpty())
        {
            tryToSendMsg(m_pending_send_msg.get().getData());
        }

        m_clients.update();

        tryToRecvMsg();

        break;
    }
    default:
    {
        break;
    }
    }

    return ServerError::None;
}

void TcpIpServer::tryToConnetClient()
{
    Option<sockaddr_in *> opt_next_client_addr = m_clients.getNextClientAddr();

    if (opt_next_client_addr
            .isSome())
    {
        // (m_nb_connected_clients < NB_ALLOWED_CLIENTS)

        sockaddr_in *next_client_addr = opt_next_client_addr.getData();

        Option<int> res_connect_client = m_socket_handler.tryToConnetClient(next_client_addr);

        if (res_connect_client
                .isSome())
        {
            m_clients.addClient(res_connect_client.getData());

            ESP_LOGI(SERVER_TAG, "Client connected: IP: %s | Port: %d\n", inet_ntoa(next_client_addr->sin_addr), (int)ntohs(next_client_addr->sin_port));
        }

        // /*
        // On success, these system calls return a file descriptor for the
        // accepted socket (a nonnegative integer).  On error, -1 is
        // returned, errno is set to indicate the error, and addrlen is left
        // unchanged.
        // */
        // int client_socket = accept(m_socket, (sockaddr *)next_client_addr, &m_socket_addr_len);

        // if (client_socket >= 0)
        // {
        //     setSocketNonBlocking(client_socket);

        //     m_clients.addClient(client_socket);

        //     ESP_LOGI(SERVER_TAG, "Client connected: IP: %s | Port: %d\n", inet_ntoa(next_client_addr->sin_addr), (int)ntohs(next_client_addr->sin_port));
        // }
    }
}

// TODO: STORE MSGs IN BUFFER
void TcpIpServer::tryToRecvMsg()
{
    // TMP
    Option<ServerFrame<MAX_MSG_SIZE>> opt_msg = m_clients.getRecvMsg();

    if (opt_msg.isSome())
    {
        // TMP
        uint8_t m_recv_buffer[MAX_MSG_SIZE] = {0};

        ServerFrame<MAX_MSG_SIZE> msg = opt_msg.getData();

        msg.debug();

        msg.toBytes(m_recv_buffer);

        ESP_LOGI(SERVER_TAG, "Client_%d: %.*s", m_clients.getClientTakingControl().getData(), MAX_MSG_SIZE, m_recv_buffer);
    }
}

void TcpIpServer::tryToSendMsg(ServerFrame<MAX_MSG_SIZE> frame)
{
    m_clients.sendMsg(frame);
}
