#include "lwip/sockets.h"
#include "state.h"
#include "additional.h"
#include "error.h"
#include "frame.h"

using namespace additional::result;
using namespace additional::option;

template <uint8_t MaxFrameLen>
class Client
{
private:
    uint8_t m_id;
    int m_socket;              // Socket descriptor id
    struct sockaddr_in m_addr; // Address
    ClientState m_state;

    /*
    Note: Now, using {0} to initialize an aggregate like this is basically a trick to 0 the entire thing.
    This is because when using aggregate initialization you don't have to specify all the members and the spec requires
    that all unspecified members be default initialized, which means set to 0 for simple types.
    */
    uint8_t m_bytes_buffer[MaxFrameLen] = {0};

public:
    Client() : m_id(0), m_socket(0), m_addr({}), m_state(ClientState::Uninitialized){};

    struct sockaddr_in *getAddrRef(void)
    {
        return &m_addr;
    };

    void connect(uint8_t id, int socket)
    {
        m_id = id;
        m_socket = socket;
        m_state = ClientState::Connected;
    };

    void disconnect()
    {
        close(m_socket);
        m_id = 0;
        m_socket = 0;
        m_state = ClientState::Uninitialized;
    };

    Result<Option<ServerFrame<MaxFrameLen>>, ClientError> tryToRecvMsg()
    {
        // ends at b"\n"
        int r = read(m_socket, m_bytes_buffer, MaxFrameLen); // receive N_BYTES AT Once
        // r = recv(serv_sock, readBuffer, sizeof(readBuffer) - 1, 0);  // Receive data from the socket. The return value is a bytes object representing the data received. The maximum amount of data to be received at once is specified by bufsize.

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
        else
        {
            return Result<Option<ServerFrame<MaxFrameLen>>, ClientError>(Option<ServerFrame<MaxFrameLen>>());
        }
    }

    Result<int, ClientError> tryToSendMsg(ServerFrame<MaxFrameLen> frame)
    {
        frame.toBytes(m_bytes_buffer);

        int r = send(m_socket, m_bytes_buffer, MaxFrameLen, 0);
        bzero(m_bytes_buffer, r); // clear buffer

        if (r == 0)
        {
            return Result<int, ClientError>(ClientError::NoResponse);
        }
        else
        {
            return Result<int, ClientError>(r);
        }
    }

    int getSocketDesc()
    {
        return m_socket;
    }

    uint8_t getId()
    {
        return m_id;
    }
};