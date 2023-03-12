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

class TcpIpServer
{
private:
    // Debug Tag
    constexpr static const char *SERVER_TAG = "SERVER";

    ServerLogin m_login;

    static const int MAX_MSG_SIZE = 128; // To adjust later reg Msgs to send
    /*
    Note: Now, using {0} to initialize an aggregate like this is basically a trick to 0 the entire thing.
    This is because when using aggregate initialization you don't have to specify all the members and the spec requires
    that all unspecified members be default initialized, which means set to 0 for simple types.
    */
    char m_recv_buffer[MAX_MSG_SIZE] = {0};

    int m_socket; // Socket descriptor id
    ServerSocketDesc m_socket_desc;
    socklen_t m_socket_addr_len = sizeof(sockaddr_in);

    // TODO: Class Clients
    static const int NB_ALLOWED_CLIENTS = 5;               // number of allowed clients
    int m_nb_connected_clients = 0;                        // number of connected clients
    int m_clients_sockets[NB_ALLOWED_CLIENTS];             // Clients sockets id
    struct sockaddr_in m_clients_addr[NB_ALLOWED_CLIENTS]; // list : Socket address of the clients (@IP, port)

    ServerState m_state = ServerState::Uninitialized;
    ServerError m_error = ServerError::None;

    // Try to connet clients
    void tryToConnetClient();

    // Message receive of a given size
    void tryToRecvMsg();

public:
    // (ServerSocketId, ServerLogin)
    TcpIpServer(ServerSocketDesc socket_desc = {}, ServerLogin login = {}) : m_login(login), m_socket_desc(socket_desc){};

    ServerError update();
};
