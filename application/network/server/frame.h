#ifndef SERVER_FRAME
#define SERVER_FRAME

// Note: https://www.geeksforgeeks.org/cc-preprocessors/

#include <stdint.h>
#include <algorithm>

// ServerFrame as uint16_t frame[Length]: uint8_t ID, uint8_t FRAME_LENGTH_MS, uint8_t FRAME_LENGTH_LS, uint8_t FRAME_NB, uint8_t DATA[DATA_LEN]]
// Frame definition
const uint8_t FRAME_ID_OFFSET = 0;
const uint8_t FRAME_LEN_OFFSET = 1;
const uint8_t FRAME_DATA_OFFSET = 4;
const uint16_t FRAME_HEADER_LEN = FRAME_DATA_OFFSET;

enum class ServerFrameId : uint8_t
{
    NotDefined = 0x00,
    Authentification = 0x01,
    Status = 0x02,
    CamPic = 0x03,
    Debug = 0xFF,
    Unknown = UINT8_MAX,
};

/*
Note:
////////////////////////////  WHEN SENDING
To create frame
ServerFrame<MaxFrameLen> frame; //ServerFrame(){};

// Or if already created: frame.clear()
frame.setId(ServerFrameId id);
uint8_t len = someDataFillFunct(frame.getDataPtr()); // The ref will be filled directly (no use to create another buffer for data)
frame.setLen(len);

////////////////////////////  WHEN RECEIVING
int r = read(socket, bytes_buffer, MaxFrameLen);
ServerFrame<MaxFrameLen> frame(bytes_buffer);

OR
int r = read(socket, frame.getBufferRef(), MaxFrameLen);

*/

// ServerFrame as uint8_t frame[Length]: [uint8_t ID[4], uint8_t FRAME_LENGTH, uint8_t FRAME_NB, uint8_t DATA[DATA_LEN]]
template <uint16_t MaxFrameLen>
class ServerFrame
{
private:
    // ServerFrameId m_id;
    // uint8_t m_len;
    // uint8_t m_data[MaxFrameLen - 5] = {0};
    uint8_t m_buffer[MaxFrameLen] = {0};

public:
    ServerFrame(){};
    /*ServerFrame(ServerFrameId id, uint8_t len, char (&data)[MaxFrameLen - FRAME_HEADER_LEN]) : m_id(id), m_len(len)
    {
        // // To test later
        // memcpy(m_data, data, len);
        for (uint8_t i = 0; i < MaxFrameLen - 5; i++)
        {
            m_data[i] = data[i];
        }
    };*/
    ~ServerFrame(){};

    // https://www.nextptr.com/question/a6212599/passing-cplusplus-arrays-to-function-by-reference
    // https://www.geeksforgeeks.org/reference-to-array-in-cpp/

    void fromBytes(uint8_t (&bytes_frame)[MaxFrameLen])
    {
        memcpy(m_buffer, bytes_frame, MaxFrameLen);

        // Endianness ...
        /*m_id = static_cast<ServerFrameId>((uint32_t(bytes_frame[0]) << 24) | (uint32_t(bytes_frame[1]) << 16) | (uint32_t(bytes_frame[2]) << 8) | (uint32_t(bytes_frame[3])));

        uint16_t frame_len = ((uint16_t)(bytes_frame[4]) << 8) | (uint16_t)(bytes_frame[5]);
        m_len = std::min(frame_len, (uint16_t)(MaxFrameLen - SERVER_FRAME_HEADER_SIZE));

        m_number = bytes_frame[6];

        for (uint8_t i = 0; i < m_len; i++)
        {
            m_data[i] = bytes_frame[i + 5];
        }*/
    }

    ServerFrame(uint8_t (&bytes_frame)[MaxFrameLen])
    {
        fromBytes(bytes_frame);
    };

    /*void toBytes(uint8_t (&buffer)[MaxFrameLen])
    {
        uint32_t id_uint32 = static_cast<uint32_t>(m_id);
        // Endianness ...
        buffer[0] = static_cast<uint8_t>(id_uint32 >> 24);
        buffer[1] = static_cast<uint8_t>(id_uint32 >> 16);
        buffer[2] = static_cast<uint8_t>(id_uint32 >> 8);
        buffer[3] = static_cast<uint8_t>(id_uint32);

        buffer[4] = static_cast<uint8_t>(m_len >> 8);
        buffer[5] = static_cast<uint8_t>(m_len);

        buffer[6] = m_number;

        for (uint8_t i = SERVER_FRAME_HEADER_SIZE; i < MaxFrameLen; i++)
        {
            buffer[i] = m_data[i - SERVER_FRAME_HEADER_SIZE];
        }
    }*/

    void setId(ServerFrameId id)
    {
        // uint32_t id_uint32 = static_cast<uint32_t>(id);
        // // Endianness ...
        // m_buffer[0] = static_cast<uint8_t>(id_uint32 >> 24);
        // m_buffer[1] = static_cast<uint8_t>(id_uint32 >> 16);
        // m_buffer[2] = static_cast<uint8_t>(id_uint32 >> 8);
        // m_buffer[3] = static_cast<uint8_t>(id_uint32);
        m_buffer[0] = static_cast<uint8_t>(id);
    }

    void setLen(uint16_t len)
    {
        m_buffer[1] = static_cast<uint8_t>(len >> 8);
        m_buffer[2] = static_cast<uint8_t>(len);
    }

    void setNumber(uint8_t number)
    {
        m_buffer[3] = number;
    }

    void setHeader(ServerFrameId id, uint16_t len, uint8_t number)
    {

        setId(id);
        setLen(len);
        setNumber(number);

        // uint8_t[SERVER_FRAME_HEADER_SIZE] buffer;

        /*uint32_t id_uint32 = static_cast<uint32_t>(id);
        // Endianness ...
        m_buffer[0] = static_cast<uint8_t>(id_uint32 >> 24);
        m_buffer[1] = static_cast<uint8_t>(id_uint32 >> 16);
        m_buffer[2] = static_cast<uint8_t>(id_uint32 >> 8);
        m_buffer[3] = static_cast<uint8_t>(id_uint32);

        m_buffer[4] = static_cast<uint8_t>(len);
        m_buffer[5] = static_cast<uint8_t>(number);*/

        // return buffer;
    }

    uint8_t setData(uint8_t *data_buff, uint16_t data_len)
    {
        uint16_t len_to_copy = std::min(data_len, uint16_t(MaxFrameLen - FRAME_HEADER_LEN));
        memcpy(m_buffer + FRAME_DATA_OFFSET, data_buff, len_to_copy);

        return len_to_copy;
    }

    ServerFrameId getId()
    {
        // return static_cast<ServerFrameId>((uint32_t(m_buffer[0]) << 24) | (uint32_t(m_buffer[1]) << 16) | (uint32_t(m_buffer[2]) << 8) | (uint32_t(m_buffer[3])));
        return static_cast<ServerFrameId>(m_buffer[0]);
    }

    uint16_t getLen()
    {
        return (uint16_t(m_buffer[1]) << 8 | uint16_t(m_buffer[2]));
    }

    uint8_t getNumber()
    {
        return uint16_t(m_buffer[3]);
    }

    uint8_t *getDataPtr()
    {
        // return m_data;
        return m_buffer + FRAME_DATA_OFFSET;
    }

    uint8_t (&getBufferRef())[MaxFrameLen]
    {
        return m_buffer;
    }

    void clear()
    {
        bzero(m_buffer, MaxFrameLen);
    }

    void debug()
    {
        uint8_t len = getLen();
        ServerFrameId id = getId();
        uint8_t nb = getNumber();

        printf("ID: %ld\nLEN: %d\nNB: %d\nData: [", static_cast<uint32_t>(id), len, nb);

        for (uint8_t i = 0; i < len; i++)
        {
            if (i == len - 1)
            {
                printf("%d", m_buffer[FRAME_DATA_OFFSET + i]);
            }
            else
            {
                printf("%d ,", m_buffer[FRAME_DATA_OFFSET + i]);
            }
        }

        printf("]\n");
    }
};

//////////////////////////////////////////////////////////////////////
// TODO: Move later to a shared submodule or use xml file parsing
enum class AuthentificationRequest : uint8_t
{
    LogIn = 1,
    LogOut = 2,
};

struct AuthFrameData
{
    static const uint8_t MAX_LOGIN_PASS_LEN = 126;
    AuthentificationRequest m_auth_req;
    char m_login_password[MAX_LOGIN_PASS_LEN];

    AuthFrameData(char *bytes)
    {
        m_auth_req = static_cast<AuthentificationRequest>((uint8_t)*bytes);

        bytes++;

        // Note: https://stackoverflow.com/questions/26456813/will-a-char-array-differ-in-ordering-in-a-little-endian-or-big-endian-system
        // Copy
        uint8_t login_pass_len = std::min(strlen(bytes) + 1,
                                          (unsigned int)MAX_LOGIN_PASS_LEN); // +1 for '\0'
        memcpy(m_login_password, bytes, login_pass_len);
    }

    AuthFrameData(AuthentificationRequest auth_req, char *login_password)
    {
        m_auth_req = auth_req;

        // Copy
        uint8_t login_pass_len = std::min(strlen(login_password) + 1,
                                          (unsigned int)MAX_LOGIN_PASS_LEN); // +1 for '\0'
        memcpy(m_login_password, login_password, login_pass_len);
    }

    uint16_t toBytes(char *bytes)
    {

        *bytes = (uint8_t)m_auth_req;

        bytes++;

        memcpy(bytes, m_login_password, MAX_LOGIN_PASS_LEN);

        return MAX_LOGIN_PASS_LEN + 1;
    }
};

///////////////////////////////:

enum class AuthentificationType : uint8_t
{
    Undefined,
    AsClient,
    AsSuperClient,
};

// To complete later
struct StatusFrameData
{
    AuthentificationType m_auth_type;

    StatusFrameData()
    {
        m_auth_type = AuthentificationType::Undefined;
    }

    StatusFrameData(char *bytes)
    {
        m_auth_type = static_cast<AuthentificationType>(*bytes);

        // bytes++;
    }

    StatusFrameData(AuthentificationType auth_type)
    {
        m_auth_type = auth_type;
    }

    uint16_t toBytes(char *bytes)
    {
        *bytes = (uint8_t)m_auth_type;

        return 1;
    }
};

#endif // SERVER_FRAME