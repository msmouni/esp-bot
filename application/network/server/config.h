#include "login.h"

struct ServerConfig
{
    ServerLogin m_login;
    int m_socket_port;

    ServerConfig(ServerLogin login = {}, int socket_port = 555) : m_login(login), m_socket_port(socket_port){};
};