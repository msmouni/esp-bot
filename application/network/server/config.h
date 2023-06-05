#include "login.h"

struct ServerConfig
{
    ServerLogin m_login;
    int m_sta_socket_port;
    int m_ap_socket_port;

    ServerConfig(ServerLogin login = {}, int sta_socket_port = 555, int ap_socket_port = 777) : m_login(login),
                                                                                                m_sta_socket_port(sta_socket_port),
                                                                                                m_ap_socket_port(ap_socket_port){};
};