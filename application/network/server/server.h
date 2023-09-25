#ifndef SERVER_H
#define SERVER_H

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
#include "handler.h"
#include "control.h"
#include "watchdog.h"
#include "additional.h"

using namespace additional::option;

class NetServer
{
public:
    static const int NB_ALLOWED_CLIENTS = 5; // number of allowed clients
    static const uint8_t MAX_MSG_SIZE = 128; //  To adjust later reg Msgs to send
private:
    // Debug Tag
    constexpr static const char *SERVER_TAG = "SERVER";

    static CircularBuffer<StatusFrameData, 10> m_pending_status_data;

    SocketsHandler m_socket_handler = SocketsHandler(NB_ALLOWED_CLIENTS);

    Clients<NB_ALLOWED_CLIENTS, MAX_MSG_SIZE> m_clients = {};

    ServerState m_state = ServerState::Uninitialized;
    ServerError m_error = ServerError::None;

    // In order to create timer after the application has started
    PeriodicTimer *m_timer_send_25ms = NULL;

    WatchDog<RobotControl> m_robot_control = WatchDog<RobotControl>(250); // Timeout = 2.5 *100 ms

    // Try to connet clients
    void tryToConnetClient();

    // Message receive of a given size
    void tryToRecvTcpMsg();

    void tryToSendTcpMsg(ServerFrame<MAX_MSG_SIZE>);
    void tryToSendStatus(StatusFrameData);

    static void tryToSendMsg25ms(void *);

public:
    NetServer();
    ~NetServer();
    void start(ApStaSocketsDesc, ServerLogin);
    void stop();

    bool tryToSendUdpMsg(void *, size_t);

    ServerError update();

    Option<RobotControl> getRobotControl();
};

#endif // SERVER_H