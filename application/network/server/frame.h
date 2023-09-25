#ifndef SERVER_FRAME
#define SERVER_FRAME

// Note: https://www.geeksforgeeks.org/cc-preprocessors/

#include <stdint.h>
#include <algorithm>

// ServerFrame as uint8_t frame[Length]: uint8_t ID, uint8_t FRAME_LENGTH_MS, uint8_t FRAME_LENGTH_LS, uint8_t FRAME_NB, uint8_t DATA[DATA_LEN]]
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
    Control = 0x04,
    Debug = 0x0F,
    Unknown = UINT8_MAX,
};

// ServerFrame as uint8_t frame[Length]: uint8_t ID, uint8_t FRAME_LENGTH_MS, uint8_t FRAME_LENGTH_LS, uint8_t FRAME_NB, uint8_t DATA[DATA_LEN]]
template <uint16_t MaxFrameLen>
class ServerFrame
{
private:
    uint8_t m_buffer[MaxFrameLen] = {0};

public:
    ServerFrame(){};
    ~ServerFrame(){};

    // https://www.nextptr.com/question/a6212599/passing-cplusplus-arrays-to-function-by-reference
    // https://www.geeksforgeeks.org/reference-to-array-in-cpp/

    void fromBytes(uint8_t (&bytes_frame)[MaxFrameLen])
    {
        memcpy(m_buffer, bytes_frame, MaxFrameLen);
    }

    ServerFrame(uint8_t (&bytes_frame)[MaxFrameLen])
    {
        fromBytes(bytes_frame);
    };

    void setId(ServerFrameId id)
    {
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
    }

    uint8_t setData(uint8_t *data_buff, uint16_t data_len)
    {
        uint16_t len_to_copy = std::min(data_len, uint16_t(MaxFrameLen - FRAME_HEADER_LEN));
        memcpy(m_buffer + FRAME_DATA_OFFSET, data_buff, len_to_copy);

        return len_to_copy;
    }

    ServerFrameId getId()
    {
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

////////////////////////////////////////////////////////////
struct ControlFrameData
{
    float m_joystick_x; // -1..1
    float m_joystick_y; // -1..1

    ControlFrameData(char *bytes)
    {
        m_joystick_x = (((float)(*bytes)) - 127) / 127; // 0..2

        bytes++;

        m_joystick_y = (((float)(*bytes)) - 127) / 127; // 0..2
    }

    ControlFrameData(float joystick_x, float joystick_y)
    {
        m_joystick_x = joystick_x;
        m_joystick_y = joystick_y;
    }

    uint16_t toBytes(char *bytes)
    {

        *bytes = (uint8_t)((1 + m_joystick_x) * 127); // 0..254
        bytes++;
        *bytes = (uint8_t)((1 + m_joystick_y) * 127); // 0..254

        return 2;
    }
};

#endif // SERVER_FRAME