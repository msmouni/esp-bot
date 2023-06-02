#ifndef SERVER_FRAME
#define SERVER_FRAME

// Note: https://www.geeksforgeeks.org/cc-preprocessors/

#include <stdint.h>
#include <algorithm>
#include "cam.h"

// To Maybe adjust later : 32bits ...
enum class ServerFrameId : uint32_t
{
    Authentification = 0x01,
    Status = 0x02,
    Debug = 0xFF,
};

// ServerFrame as uint8_t frame[Length]: [uint8_t ID[4], uint8_t FRAME_LENGTH[2], uint8_t DATA[DATA_LEN]]
template <uint16_t MaxFrameLen>
class ServerFrame
{
private:
    ServerFrameId m_id;
    uint16_t m_len;
    uint8_t m_data[MaxFrameLen - 6] = {0};

public:
    ServerFrame(){};
    ServerFrame(ServerFrameId id, uint16_t len, char (&data)[MaxFrameLen - 6]) : m_id(id), m_len(len)
    {
        for (uint8_t i = 0; i < MaxFrameLen - 6; i++)
        {
            m_data[i] = data[i];
        }
    };
    ~ServerFrame(){};

    // https://www.nextptr.com/question/a6212599/passing-cplusplus-arrays-to-function-by-reference
    // https://www.geeksforgeeks.org/reference-to-array-in-cpp/

    void fromBytes(uint8_t (&bytes_frame)[MaxFrameLen])
    {

        // Endianness ...
        m_id = static_cast<ServerFrameId>((uint32_t(bytes_frame[0]) << 24) | (uint32_t(bytes_frame[1]) << 16) | (uint32_t(bytes_frame[2]) << 8) | (uint32_t(bytes_frame[3])));

        uint16_t frame_len = ((uint16_t)(bytes_frame[4] << 8) | (uint16_t)(bytes_frame[5]));
        m_len = std::min(frame_len, MaxFrameLen);

        for (uint8_t i = 0; i < m_len; i++)
        {
            m_data[i] = bytes_frame[i + 6];
        }
    }

    ServerFrame(uint8_t (&bytes_frame)[MaxFrameLen])
    {
        fromBytes(bytes_frame);
    };

    void toBytes(uint8_t (&buffer)[MaxFrameLen])
    {
        uint32_t id_uint32 = static_cast<uint32_t>(m_id);
        // Endianness ...
        buffer[0] = static_cast<uint8_t>(id_uint32 >> 24);
        buffer[1] = static_cast<uint8_t>(id_uint32 >> 16);
        buffer[2] = static_cast<uint8_t>(id_uint32 >> 8);
        buffer[3] = static_cast<uint8_t>(id_uint32);

        buffer[4] = static_cast<uint8_t>(m_len >> 8);
        buffer[5] = static_cast<uint8_t>(m_len);

        for (uint8_t i = 6; i < MaxFrameLen; i++)
        {
            buffer[i] = m_data[i - 6];
        }
    }

    void debug()
    {
        printf("ID: %ld\nLEN: %d\nData: [", static_cast<uint32_t>(m_id), m_len);

        for (uint16_t i = 0; i < m_len; i++)
        {
            if (i == m_len - 1)
            {
                printf("%d", m_data[i]);
            }
            else
            {
                printf("%d ,", m_data[i]);
            }
        }

        printf("]\n");
    }

    ServerFrameId &getId()
    {
        return m_id;
    }

    uint16_t getLen()
    {
        return m_len;
    }

    uint8_t (&getData())[MaxFrameLen - 6]
    {
        return m_data;
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

        return (uint16_t)(MAX_LOGIN_PASS_LEN + 1);
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
    CamPicture m_cam_pic;

    StatusFrameData(AuthentificationType auth_type = AuthentificationType::Undefined, CamPicture cam_pic = {})
    {
        m_auth_type = auth_type;
        m_cam_pic = cam_pic;
    }
    StatusFrameData(char *bytes)
    {
        m_auth_type = static_cast<AuthentificationType>(*bytes);

        bytes++;

        m_cam_pic = CamPicture(bytes);
    }

    uint16_t toBytes(char *bytes)
    {
        uint16_t bytes_size = 0;

        *bytes = (uint8_t)m_auth_type;

        bytes_size += 1;

        bytes++;

        bytes_size += m_cam_pic.toBytes(bytes);

        return bytes_size;
    }
};

#endif // SERVER_FRAME