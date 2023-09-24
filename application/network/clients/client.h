#ifndef CLIENT_H
#define CLIENT_H

#include "lwip/sockets.h"
#include "state.h"
#include "additional.h"
#include "error.h"
#include "frame.h"
#include <errno.h>
#include "handler.h"

using namespace additional::result;
using namespace additional::option;

template <uint16_t MaxFrameLen>
class Client
{
private:
    uint8_t m_id;
    int m_socket;              // Socket descriptor id
    struct sockaddr_in m_addr; // Address
    ClientState m_state;
    WhichSocket m_connected_at;

    /*
    Note: Now, using {0} to initialize an aggregate like this is basically a trick to 0 the entire thing.
    This is because when using aggregate initialization you don't have to specify all the members and the spec requires
    that all unspecified members be default initialized, which means set to 0 for simple types.
    */
    uint8_t m_bytes_buffer[MaxFrameLen] = {0};

public:
    Client() : m_id(0), m_socket(0), m_addr({}), m_state(ClientState::Uninitialized), m_connected_at(WhichSocket::Undefined){};

    struct sockaddr_in *getAddrPtr(void)
    {
        return &m_addr;
    };

    void connect(uint8_t id, ConnectedClient connected_socket)
    {
        m_id = id;
        m_socket = connected_socket.m_socket_fd;
        m_state = ClientState::Connected;
        m_connected_at = connected_socket.m_connected_at;
    };

    void disconnect()
    {
        close(m_socket);
        m_id = 0;
        m_socket = 0;
        m_state = ClientState::Uninitialized;
    };

    Result<Option<ServerFrame<MaxFrameLen>>, ClientError> tryToRecvTcpMsg()
    {
        // ends at b"\n"
        int r = read(m_socket, m_bytes_buffer, MaxFrameLen); // receive N_BYTES AT Once
        // int r = recv(m_socket, m_bytes_buffer, MaxFrameLen, 0); // Receive data from the socket. The return value is a bytes object representing the data received. The maximum amount of data to be received at once is specified by bufsize.

        if (r > 0)
        {
            Result<Option<ServerFrame<MaxFrameLen>>, ClientError> res =
                Result<Option<ServerFrame<MaxFrameLen>>, ClientError>(Option<ServerFrame<MaxFrameLen>>(ServerFrame<MaxFrameLen>(m_bytes_buffer)));
            bzero(m_bytes_buffer, r); // clear buffer
            return res;
        }
        else if (r == 0)
        {
            return Result<Option<ServerFrame<MaxFrameLen>>, ClientError>(ClientError::NoResponse);
            /* the client socket is interrupted */
        }
        else if (errno == SOCKET_ERR_TRY_AGAIN)
        {
            return Result<Option<ServerFrame<MaxFrameLen>>, ClientError>(Option<ServerFrame<MaxFrameLen>>());
        }
        else
        {
            printf("Client_%d exit with err %d\n", m_id, errno);
            return Result<Option<ServerFrame<MaxFrameLen>>, ClientError>(ClientError::SocketError);
        }
    }

    Result<int, ClientError> tryToSendTcpMsg(ServerFrame<MaxFrameLen> &frame)
    {
        // frame.toBytes(m_bytes_buffer);

        int r = send(m_socket, frame.getBufferRef(), MaxFrameLen, 0);
        // bzero(m_bytes_buffer, MaxFrameLen); // clear buffer
        // frame.clear();

        if (r == 0)
        {
            printf("Client_%d NoResponse\n", m_id);
            return Result<int, ClientError>(ClientError::NoResponse);
        }
        else if (r > 0)
        {
            return Result<int, ClientError>(r);
        }
        else if (errno == SOCKET_ERR_TRY_AGAIN)
        {
            return Result<int, ClientError>(0);
        }
        else
        {
            printf("Client_%d exit with err %d\n", m_id, errno);
            return Result<int, ClientError>(ClientError::SocketError);
        }
    }

    int getSocketDesc()
    {
        return m_socket;
    }

    uint8_t &getId()
    {
        return m_id;
    }

    ClientState &getState()
    {
        return m_state;
    }

    void login()
    {
        m_state = ClientState::Authenticated;
    }

    void takeControl()
    {
        m_state = ClientState::TakingControl;
    }

    WhichSocket &getSocketConnectedTo()
    {
        return m_connected_at;
    }

    void
    logout()
    {
        m_state = ClientState::Connected;
    }

    bool isAuthenticated()
    {
        return m_state == ClientState::Authenticated;
    }

    bool isTakingControl()
    {
        return m_state == ClientState::TakingControl;
    }
};

#endif // CLIENT_H