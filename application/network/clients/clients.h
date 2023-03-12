#include "client.h"
#include "circular_buffer.h"
#include "error.h"

/// Only one client have state `TakingControl` and can send messages to server
/// The other clients can only receive messages from server
template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
class Clients
{
private:
    // Debug Tag
    constexpr static const char *CLIENTS_TAG = "CLIENTS";
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
        m_client_taking_control = Option<uint8_t>();
    };
    // ~Clients();
    // Clients(const Clients &) = delete;
    // Clients &operator=(const Clients &) = delete;
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
Option<struct sockaddr_in *> Clients<NbAllowedClients, MaxFrameLen>::getNextClientAddr(void)
{
    return getClientAddr(m_nb_connected_clients);
}

template <uint8_t NbAllowedClients, uint8_t MaxFrameLen>
void Clients<NbAllowedClients, MaxFrameLen>::addClient(int socket_desc)
{
    m_clients[m_nb_connected_clients].connect(m_client_id_tracker, socket_desc);

    // TMP
    m_client_taking_control = Option<uint8_t>(m_nb_connected_clients); // TMP: Last one

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
    if (opt_msg.isSome())
    {
        // Send Messages to all authentified clients
        for (int client_idx = 0; client_idx < m_nb_connected_clients; client_idx++)
        {
            Result<int, ClientError> res = m_clients[client_idx].tryToSendMsg(opt_msg.getData());
            if (res
                    .isErr())
            {
                deleteClient(client_idx);
            }
        }
    }

    // Receive Messages from client taking control
    if (m_client_taking_control
            .isSome())
    {
        uint8_t client_index = m_client_taking_control.getData();

        Result<Option<ServerFrame<MaxFrameLen>>, ClientError> res = m_clients[client_index].tryToRecvMsg();

        if (res
                .isErr())
        {
            deleteClient(client_index);
            m_client_taking_control = Option<uint8_t>();
        }
        else
        {
            Option<ServerFrame<MaxFrameLen>> opt_msg = res.getData();

            if (opt_msg.isSome())
            {
                rx_frames_buffer.push(opt_msg.getData());
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