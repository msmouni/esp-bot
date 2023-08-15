#include "server.h"

CircularBuffer<StatusFrameData, 10> NetServer::m_pending_status_data = {};

void NetServer::tryToSendMsg25ms(void *args)
{
    StatusFrameData status_data = StatusFrameData();

    m_pending_status_data.push(status_data);
}

NetServer::NetServer()
{
    m_state = ServerState::NotStarted;
}
NetServer::~NetServer()
{
    m_socket_handler.stop();
    delete m_timer_send_25ms;
}

void NetServer::start(ApStaSocketsDesc sockets_desc, ServerLogin login)
{
    m_socket_handler.start(sockets_desc);

    m_clients.setServerLogin(login);

    m_timer_send_25ms = new PeriodicTimer("Server_25ms", tryToSendMsg25ms, NULL, 25000);
    m_state = ServerState::Uninitialized;
}

void NetServer::stop()
{
    if ((m_state != ServerState::NotStarted) & (m_state != ServerState::Uninitialized))
    {
        m_socket_handler.stop();
    }

    if (m_state == ServerState::Running)
    {
        m_timer_send_25ms->stop();
    }

    m_state = ServerState::NotStarted;
}

ServerError NetServer::update()
{
    switch (m_state)
    {
    case ServerState::Uninitialized:
    {
        if (m_socket_handler.update() == SocketsHandlerError::ErrorOnApSta)
        {
            m_error = ServerError::SocketError;
            m_state = ServerState::Error;
            return ServerError::SocketError;
        }
        else if (m_socket_handler.isReady())
        {
            m_state = ServerState::SocketsReady;
        }

        break;
    }
    case ServerState::SocketsReady:
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
    case ServerState::Running:
    {
        if (m_socket_handler.isReady())
        {
            if (!m_clients.hasApUdpFd())
            {
                m_clients.setApUdpFd(m_socket_handler.getApUdpFd());
            }
            if (!m_clients.hasStaUdpFd())
            {
                m_clients.setStaUdpFd(m_socket_handler.getStaUdpFd());
            }
        }

        tryToConnetClient();

        if (!m_pending_status_data.isEmpty())
        {
            tryToSendStatus(m_pending_status_data.get().getData());
        }

        m_clients.update();

        tryToRecvTcpMsg();

        break;
    }
    default:
    {
        break;
    }
    }

    return ServerError::None;
}

void NetServer::tryToConnetClient()
{
    Option<sockaddr_in *> opt_next_client_addr = m_clients.getNextClientAddr();

    if (opt_next_client_addr
            .isSome())
    {
        // (m_nb_connected_clients < NB_ALLOWED_CLIENTS) : handled in Clients

        sockaddr_in *next_client_addr = opt_next_client_addr.getData();

        Option<ConnectedClient> res_connect_client = m_socket_handler.tryToConnetClient(next_client_addr);

        if (res_connect_client
                .isSome())
        {
            m_clients.addClient(res_connect_client.getData());

            ESP_LOGI(SERVER_TAG, "Client connected: IP: %s | Port: %d\n", inet_ntoa(next_client_addr->sin_addr), (int)ntohs(next_client_addr->sin_port));
        }
    }
}

// TODO: STORE MSGs IN BUFFER
void NetServer::tryToRecvTcpMsg()
{
    // CONTROL messages ...
    // TMP
    Option<ServerFrame<MAX_MSG_SIZE>> opt_msg = m_clients.getRecvTcpMsg();

    if (opt_msg.isSome())
    {
        // TMP
        // uint8_t m_recv_buffer[MAX_MSG_SIZE] = {0};

        ServerFrame<MAX_MSG_SIZE> msg = opt_msg.getData();

        ESP_LOGI(SERVER_TAG, "Client_%d:", m_clients.getClientTakingControl().getData());

        msg.debug();

        // msg.toBytes(m_recv_buffer);

        // ESP_LOGI(SERVER_TAG, "Client_%d: %.*s", m_clients.getClientTakingControl().getData(), MAX_MSG_SIZE, m_recv_buffer);
    }
}

void NetServer::tryToSendTcpMsg(ServerFrame<MAX_MSG_SIZE> frame)
{
    m_clients.sendTcpMsg(frame);
}

void NetServer::tryToSendStatus(StatusFrameData status_data)
{
    m_clients.sendStatus(status_data);
}

bool NetServer::tryToSendUdpMsg(void *data_ptr, size_t data_size)
{

    return m_clients.sendUdpMsgToAll(data_ptr, data_size).isOk();
}