#include "server.h"

CircularBuffer<StatusFrameData, 10> TcpIpServer::m_pending_status_data = {};
// CamPicture TcpIpServer::m_cam_pic = {};

void TcpIpServer::tryToSendMsg_25ms(void *args)
{
    // // TMP
    // uint8_t data[MAX_MSG_SIZE - 5] = {0};
    // data[0] = 1;
    // data[1] = 2;
    // data[2] = 3;
    // data[3] = 4;
    // data[4] = 5;
    // ServerFrame<MAX_MSG_SIZE> status_frame = ServerFrame<MAX_MSG_SIZE>(ServerFrameId::Status, 5, data);

    // StatusFrameData status_data = StatusFrameData(m_cam_pic);

    // m_pending_status_data.push(status_data);
}

TcpIpServer::TcpIpServer()
{
    // init_camera();
    m_state = ServerState::NotStarted;
}
TcpIpServer::~TcpIpServer()
{
    m_socket_handler.stop();
    delete m_timer_send_25ms;
}

void TcpIpServer::start(ApStaSocketsDesc sockets_desc, ServerLogin login)
{
    m_socket_handler.start(sockets_desc);
    m_clients.setServerLogin(login);
    m_timer_send_25ms = new PeriodicTimer("TCP_IP_Server_25ms", tryToSendMsg_25ms, NULL, 25000);
    m_state = ServerState::Uninitialized;
}

void TcpIpServer::stop()
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

ServerError TcpIpServer::update()
{
    // TMP
    // m_cam_pic =
    // take_pic();

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
        else if (m_socket_handler.isListening())
        {
            m_state = ServerState::SocketsListening;
        }

        break;
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
    case ServerState::Running:
    {

        tryToConnetClient();

        // if (!m_pending_status_data.isEmpty())
        // {
        //     tryToSendMsg(m_pending_status_data.get().getData());
        // }

        if (!m_pending_status_data.isEmpty())
        {
            tryToSendStatus(m_pending_status_data.get().getData());
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
        // (m_nb_connected_clients < NB_ALLOWED_CLIENTS) : handled in Clients

        sockaddr_in *next_client_addr = opt_next_client_addr.getData();

        Option<int> res_connect_client = m_socket_handler.tryToConnetClient(next_client_addr);

        if (res_connect_client
                .isSome())
        {
            m_clients.addClient(res_connect_client.getData());

            ESP_LOGI(SERVER_TAG, "Client connected: IP: %s | Port: %d\n", inet_ntoa(next_client_addr->sin_addr), (int)ntohs(next_client_addr->sin_port));
        }
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

void TcpIpServer::tryToSendStatus(StatusFrameData status_data)
{
    m_clients.sendStatus(status_data);
}
