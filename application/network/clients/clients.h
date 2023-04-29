#ifndef TCP_IP_CLIENTS
#define TCP_IP_CLIENTS

#include "client.h"
#include "circular_buffer.h"
#include "error.h"
#include "login.h"

/// Only one client have state `TakingControl` and can send messages to server
/// The other clients can only receive messages from server
template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
class Clients
{
private:
    // Debug Tag
    constexpr static const char *CLIENTS_TAG = "CLIENTS";

    Option<ServerLogin> m_server_login;

    uint8_t m_nb_connected_clients; // number of connected clients
    uint32_t m_client_id_tracker;

    Client<MaxFrameLen> m_clients[NbAllowedClients] = {}; // Clients array

    Option<uint8_t> m_client_taking_control;

    // Later: Mutex on a struct that holds Rx and Tx buffer, to be shared between Network_Task and other_task
    static const uint8_t RxBufferSize = 50;
    CircularBuffer<ServerFrame<MaxFrameLen>, RxBufferSize> rx_frames_buffer; // From client taking control
    static const uint8_t TxBufferSize = 50;
    CircularBuffer<ServerFrame<MaxFrameLen>, TxBufferSize> tx_frames_buffer; // To all authentified clients

    Option<struct sockaddr_in *> getClientAddr(uint8_t);
    void deleteClient(uint8_t);

public:
    Clients() : m_nb_connected_clients(0), m_client_id_tracker(0)
    {
        m_server_login = Option<ServerLogin>();
        m_client_taking_control = Option<uint8_t>();
    };
    // ~Clients();
    // Clients(const Clients &) = delete;
    // Clients &operator=(const Clients &) = delete;
    void setServerLogin(ServerLogin login);
    Option<struct sockaddr_in *> getNextClientAddr(void);
    void addClient(int);
    void update();
    Option<ServerFrame<MaxFrameLen>> getRecvMsg();
    ClientsError sendMsg(ServerFrame<MaxFrameLen>);
    Option<uint8_t> getClientTakingControl();
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// https://isocpp.org/wiki/faq/templates#templates-defn-vs-decl

template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
Option<struct sockaddr_in *> Clients<NbAllowedClients, MaxFrameLen>::getClientAddr(uint8_t client_index)
{
    if (client_index < NbAllowedClients)
    {
        return Option<struct sockaddr_in *>(m_clients[client_index].getAddrRef());
    }
    else
    {
        return Option<struct sockaddr_in *>();
    }
}

template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
void Clients<NbAllowedClients, MaxFrameLen>::setServerLogin(ServerLogin login)
{
    m_server_login.setData(login);
}

template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
Option<struct sockaddr_in *> Clients<NbAllowedClients, MaxFrameLen>::getNextClientAddr(void)
{
    return getClientAddr(m_nb_connected_clients);
}

template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
void Clients<NbAllowedClients, MaxFrameLen>::addClient(int socket_desc)
{
    m_clients[m_nb_connected_clients].connect(m_client_id_tracker, socket_desc);

    m_nb_connected_clients += 1;
    m_client_id_tracker += 1;
}

template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
void Clients<NbAllowedClients, MaxFrameLen>::deleteClient(uint8_t client_index)
{
    if (client_index < m_nb_connected_clients)
    {
        /* the client socket is interrupted */
        ESP_LOGI(CLIENTS_TAG, "Connexion with Client_%d interrupted", m_clients[client_index].getId());

        if (m_clients[client_index].getState() == ClientState::TakingControl)
        {
            m_client_taking_control.removeData();
        }

        m_clients[client_index].disconnect();

        // Clear the client  from the list
        for (int cmpt = client_index; cmpt < m_nb_connected_clients; cmpt++)
        {
            m_clients[cmpt] = m_clients[cmpt + 1];
        }

        m_nb_connected_clients -= 1;
    }
}

template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
Option<ServerFrame<MaxFrameLen>> Clients<NbAllowedClients, MaxFrameLen>::getRecvMsg()
{
    return rx_frames_buffer.get();
}

template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
ClientsError Clients<NbAllowedClients, MaxFrameLen>::sendMsg(ServerFrame<MaxFrameLen> frame)
{
    // TODO: Broadcast or send to one client
    if (tx_frames_buffer.push(frame))
    {
        return ClientsError::None;
    }
    else
    {
        return ClientsError::FullTxBuffer;
    }
}

template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
void Clients<NbAllowedClients, MaxFrameLen>::update()
{

    Option<ServerFrame<MaxFrameLen>> opt_msg = tx_frames_buffer.get();

    for (int client_idx = 0; client_idx < m_nb_connected_clients; client_idx++)
    {
        ClientState client_state = m_clients[client_idx].getState();

        // Send Messages to all authentified clients
        if ((opt_msg.isSome()) & ((client_state == ClientState::Authenticated) | (client_state == ClientState::TakingControl)))
        {

            Result<int, ClientError> res = m_clients[client_idx].tryToSendMsg(opt_msg.getData());
            if (res
                    .isErr())
            {
                deleteClient(client_idx);
                return; // TODO: continue for others
            }
        }

        // Receive Msg from all clients
        Result<Option<ServerFrame<MaxFrameLen>>, ClientError> res = m_clients[client_idx].tryToRecvMsg();
        if (res
                .isErr())
        {
            deleteClient(client_idx);
            return; // TODO: continue for others
        }
        else
        {
            Option<ServerFrame<MaxFrameLen>> opt_msg = res.getData();

            if (opt_msg.isSome())
            {

                ServerFrame<MaxFrameLen> msg = opt_msg.getData();

                switch (client_state)
                {
                case ClientState::Connected:
                {
                    if (m_server_login.isSome())
                    {
                        if (msg.getId() == ServerFrameId::Authentification)
                        {
                            // Compare to login
                            // '{super_admin:super_password}\0': [123, 115, 117, 112, 101, 114, 95, 97, 100, 109, 105, 110, 58, 115, 117, 112, 101, 114, 95, 112, 97, 115, 115, 119, 111, 114, 100, 125, 0]
                            // -> [0, 0, 0, 1, 29, 123, 115, 117, 112, 101, 114, 95, 97, 100, 109, 105, 110, 58, 115, 117, 112, 101, 114, 95, 112, 97, 115, 115, 119, 111, 114, 100, 125, 0]

                            ServerLogin &server_login = m_server_login.getData();

                            // TO VERIFY: reinterpret_cast
                            char *msg_data = reinterpret_cast<char *>(msg.getData());

                            switch (server_login.check(msg_data))
                            {
                            case LoginResult::Client:
                            {
                                // ESP_LOGI(CLIENTS_TAG, "Client_%d(%s:%d) Logged as Client\n", client_idx, inet_ntoa(m_clients[0].getAddrRef()->sin_addr), (int)ntohs(m_clients[client_idx].getAddrRef()->sin_port));
                                printf("Client_%d(%s:%d) Logged as Client\n", m_clients[client_idx].getId(), inet_ntoa(m_clients[client_idx].getAddrRef()->sin_addr), (int)ntohs(m_clients[client_idx].getAddrRef()->sin_port));
                                m_clients[client_idx].login();
                                break;
                            }
                            case LoginResult::SuperClient:
                            {
                                if (m_client_taking_control.isNone())
                                {
                                    printf("Client_%d(%s:%d) Logged as SuperClient\n", m_clients[client_idx].getId(), inet_ntoa(m_clients[client_idx].getAddrRef()->sin_addr), (int)ntohs(m_clients[client_idx].getAddrRef()->sin_port));
                                    m_clients[client_idx].takeControl();

                                    m_client_taking_control.setData(client_idx);
                                }
                                break;
                            }
                            case LoginResult::WrongLogin:
                            {
                                printf("Client_%d(%s:%d) tried to Log using wrong Login\n", m_clients[client_idx].getId(), inet_ntoa(m_clients[client_idx].getAddrRef()->sin_addr), (int)ntohs(m_clients[client_idx].getAddrRef()->sin_port));

                                break;
                            }

                            default:
                                break;
                            }
                        }
                    }

                    break;
                }
                case ClientState::Authenticated:
                {
                    if ((msg.getId() == ServerFrameId::Authentification) & (msg.getLen() == 0))
                    {
                        m_clients[client_idx].logout();
                    }
                    break;
                }
                case ClientState::TakingControl:
                {
                    if (msg.getId() == ServerFrameId::Authentification)
                    {
                        if (msg.getLen() == 0)
                        {
                            m_client_taking_control.removeData();
                            m_clients[client_idx].logout();
                        }
                    }
                    else
                    {
                        // Store Messages from client taking control
                        rx_frames_buffer.push(msg);
                    }
                    break;
                }
                default:
                {
                    break;
                }
                }
            }
        }
    }
}

template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
Option<uint8_t> Clients<NbAllowedClients, MaxFrameLen>::getClientTakingControl()
{
    if (m_client_taking_control
            .isSome())
    {
        uint8_t client_index = m_client_taking_control.getData();

        return Option<uint8_t>(m_clients[client_index].getId());
    }
    else
    {
        return Option<uint8_t>();
    }
}

#endif // TCP_IP_CLIENT