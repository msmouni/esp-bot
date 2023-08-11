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
// #include "cam.h"

class TcpIpServer
{
public:
    static const int NB_ALLOWED_CLIENTS = 5;  // number of allowed clients
    static const uint16_t MAX_MSG_SIZE = 128; // 1024; // 8192; // 128; // To adjust later reg Msgs to send
private:
    // Debug Tag
    constexpr static const char *SERVER_TAG = "SERVER";

    // static CircularBuffer<ServerFrame<MAX_MSG_SIZE>, 50> m_pending_send_msg;
    static CircularBuffer<StatusFrameData, 10> m_pending_status_data;
    // static CamPicture m_cam_pic; // TODO: Maybe move elsewhere

    SocketsHandler m_socket_handler = SocketsHandler(NB_ALLOWED_CLIENTS);
    // TcpSocketsHandler m_socket_handler = TcpSocketsHandler(NB_ALLOWED_CLIENTS);

    Clients<NB_ALLOWED_CLIENTS, MAX_MSG_SIZE> m_clients = {};

    ServerState m_state = ServerState::Uninitialized;
    ServerError m_error = ServerError::None;

    // In order to create timer after the application has started
    PeriodicTimer *m_timer_send_25ms = NULL;

    // Try to connet clients
    void tryToConnetClient();

    // Message receive of a given size
    void tryToRecvTcpMsg();

    void tryToSendTcpMsg(ServerFrame<MAX_MSG_SIZE>);
    void tryToSendStatus(StatusFrameData);

    static void tryToSendMsg25ms(void *);

public:
    TcpIpServer();
    ~TcpIpServer();
    void start(ApStaSocketsDesc, ServerLogin);
    void stop();

    bool tryToSendUdpMsg(void *, size_t);

    ServerError update();
};

#endif // SERVER_H