#include "server.h"

// TcpIpServer::TcpIpServer(ServerSocketDesc socket_desc = {}, ServerLogin login = {})
// {
//     m_socket_desc = socket_desc;
//     m_login = login;
// }

ServerError TcpIpServer::update()
{
    switch (m_state)
    {
    case ServerState::Uninitialized:
    {
        m_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (m_socket < 0)
        {
            ESP_LOGE(SERVER_TAG, "Failed to create a socket...");
            m_error = ServerError::CannotCreateSocket;
            m_state = ServerState::Error;
            return ServerError::CannotCreateSocket;
        }

        /* NON-BLOCKING FLAG:
            1- Call the fcntl() API to retrieve the socket descriptor's current flag settings into a local variable.
            2- In our local variable, set the O_NONBLOCK (non-blocking) flag on. (being careful, of course, not to tamper with the other flags)
            3- Call the fcntl() API to set the flags for the descriptor to the value in our local variable.
        */
        int flags = fcntl(m_socket, F_GETFL) | O_NONBLOCK;
        fcntl(m_socket, F_SETFL, flags);

        ESP_LOGI(SERVER_TAG, "TCP/IP Server Created");

        m_state = ServerState::SocketCreated;

        break;
    }
    case ServerState::SocketCreated:
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

        ESP_LOGI(SERVER_TAG, "TCP/IP Server Started");

        m_state = ServerState::Running;

        break;
    }
    case ServerState::Running:
    {

        tryToConnetClient();

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
    if (m_nb_connected_clients < NB_ALLOWED_CLIENTS)
    {

        /*
        On success, these system calls return a file descriptor for the
        accepted socket (a nonnegative integer).  On error, -1 is
        returned, errno is set to indicate the error, and addrlen is left
        unchanged.
        */
        int client_socket = accept(m_socket, (struct sockaddr *)&m_clients_addr[m_nb_connected_clients], &m_socket_addr_len);

        if (client_socket >= 0)
        {
            m_clients_sockets[m_nb_connected_clients] = client_socket;

            ESP_LOGI(SERVER_TAG, "Client connected: IP: %s | Port: %d\n", inet_ntoa(m_clients_addr[m_nb_connected_clients].sin_addr), (int)ntohs(m_clients_addr[m_nb_connected_clients].sin_port));

            m_nb_connected_clients += 1;
        }
    }
}

// TMP STORE MSGs IN BUFFER
void TcpIpServer::tryToRecvMsg()
{
    for (int client_idx = 0; client_idx <= m_nb_connected_clients; client_idx++)
    {

        // ends at b"\n"
        int r = read(m_clients_sockets[client_idx], m_recv_buffer, MAX_MSG_SIZE); // receive N_BYTES AT Once
        // r = recv(serv_sock, readBuffer, sizeof(readBuffer) - 1, 0);  // Receive data from the socket. The return value is a bytes object representing the data received. The maximum amount of data to be received at once is specified by bufsize.

        if (r > 0)
        {
            /*
            You can either add a null character after your termination character, and your printf will work,
            or you can add a '.*' in your printf statement and provide the length*/
            // printf("%.*s", r, m_recv_buffer);
            ESP_LOGI(SERVER_TAG, "Client_%d: %.*s", client_idx, r, m_recv_buffer);

            // bzero(readBuffer, sizeof(readBuffer));
        }
        else if (r == 0)
        {
            /* the client socket is interrupted */
            ESP_LOGI(SERVER_TAG, "Connexion with Client_%d interrupted", client_idx);

            close(m_clients_sockets[client_idx]);

            // Clear the client  from the list
            for (int cmpt = client_idx; cmpt < m_nb_connected_clients; cmpt++)
            {
                // @todo: CLIENT ID
                m_clients_sockets[cmpt] = m_clients_sockets[cmpt + 1];
                m_clients_addr[cmpt] = m_clients_addr[cmpt + 1];
                m_nb_connected_clients -= 1;
            }
        }
    }
}