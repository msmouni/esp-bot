#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include <algorithm>
#include "esp_log.h"
#include "config.h"
#include "state.h"
#include "error.h"
#include "frame.h"
#include "clients.h"
#include "timer.h"
#include "sockets_handler.h"

class TcpIpServer
{
public:
    static const int NB_ALLOWED_CLIENTS = 5; // number of allowed clients
private:
    // Debug Tag
    constexpr static const char *SERVER_TAG = "SERVER";

    static const int MAX_MSG_SIZE = 128; // To adjust later reg Msgs to send
    static CircularBuffer<ServerFrame<MAX_MSG_SIZE>, 50> m_pending_send_msg;

    // SocketsHandler<NB_ALLOWED_CLIENTS> m_sockets_handler; // TODO
    SocketsHandler m_socket_handler = SocketsHandler(NB_ALLOWED_CLIENTS);

    Clients<NB_ALLOWED_CLIENTS, MAX_MSG_SIZE> m_clients = {};

    ServerState m_state = ServerState::Uninitialized;
    ServerError m_error = ServerError::None;

    // In order to create timer after the application has started
    PeriodicTimer *m_timer_send_25ms = NULL;

    // Try to connet clients
    void tryToConnetClient();

    // Message receive of a given size
    void tryToRecvMsg();

    void tryToSendMsg(ServerFrame<MAX_MSG_SIZE>);

    static void tryToSendMsg_25ms(void *);

public:
    TcpIpServer();
    ~TcpIpServer();
    void start(ApStaSocketsDesc, ServerLogin);
    void stop();

    ServerError update();
};
