struct ServerSocketDesc
{

    struct sockaddr_in addr; // Server @
    int port;                // Server port number

    /// IP: "xxx.xxx.xxx.xxx", int port
    ServerSocketDesc(const char *ip = "", int port = 0) : port(port)
    {
        addr.sin_family = AF_INET;

        inet_aton(ip, &addr.sin_addr.s_addr); // inet_aton() converts the Internet host address cp from the IPv4 numbers-and-dots notation into binary form (in network byte order)
        addr.sin_port = htons(port);          // The htons() function converts the unsigned short integer hostshort from host byte order to network byte order.
    };
};