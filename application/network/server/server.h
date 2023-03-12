#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include <algorithm>
#include "esp_log.h"
#include "socket_desc.h"
#include "config.h"
#include "state.h"
#include "error.h"
#include "frame.h"
#include "clients.h"

class TcpIpServer
{
private:
    // Debug Tag
    constexpr static const char *SERVER_TAG = "SERVER";

    ServerLogin m_login;

    static const int MAX_MSG_SIZE = 128; // To adjust later reg Msgs to send

    int m_socket; // Socket descriptor id
    ServerSocketDesc m_socket_desc;
    socklen_t m_socket_addr_len = sizeof(sockaddr_in);

    static const int NB_ALLOWED_CLIENTS = 5; // number of allowed clients
    Clients<NB_ALLOWED_CLIENTS, MAX_MSG_SIZE> m_clients = {};

    ServerState m_state = ServerState::Uninitialized;
    ServerError m_error = ServerError::None;

    // Try to connet clients
    void tryToConnetClient();

    // Message receive of a given size
    void tryToRecvMsg();

    void tryToSendMsg();

public:
    TcpIpServer() : m_state(ServerState::NotStarted){};
    void start(ServerSocketDesc, ServerLogin);

    ServerError update();
};
