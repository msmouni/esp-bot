#include "login.h"

struct ServerConfig
{
    ServerLogin login;
    int socket_port;

    /// (login: "super_admin", password: "super_password", int server_socket_port)
    ServerConfig(const char *login = "super_admin", const char *password = "super_password", int socket_port = 555) : login(ServerLogin(login, password)), socket_port(socket_port){};
};