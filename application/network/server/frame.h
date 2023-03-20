#ifndef SERVER_FRAME
#define SERVER_FRAME

// Note: https://www.geeksforgeeks.org/cc-preprocessors/

#include <stdint.h>
#include <algorithm>

enum class ServerFrameId : uint32_t
{
    Authentification = 0x01,
    Debug = 0xFF,
};

// ServerFrame as uint8_t frame[Length]: [uint8_t ID[4], uint8_t FRAME_LENGTH, uint8_t DATA[DATA_LEN]]
template <uint8_t MaxFrameLen>
class ServerFrame
{
private:
    ServerFrameId m_id;
    uint8_t m_len;
    uint8_t m_data[MaxFrameLen - 5] = {0};

public:
    ServerFrame(){};
    ServerFrame(ServerFrameId id, uint8_t len, uint8_t (&data)[MaxFrameLen - 5]) : m_id(id), m_len(len)
    {
        for (uint8_t i = 0; i < MaxFrameLen - 5; i++)
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

        m_len = std::min(bytes_frame[4], MaxFrameLen);

        for (uint8_t i = 0; i < m_len; i++)
        {
            m_data[i] = bytes_frame[i + 5];
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

        buffer[4] = m_len;

        for (uint8_t i = 5; i < MaxFrameLen; i++)
        {
            buffer[i] = m_data[i - 5];
        }
    }

    void debug()
    {
        printf("ID: %ld\nLEN: %d\nData: [", static_cast<uint32_t>(m_id), m_len);

        for (uint8_t i = 0; i < m_len; i++)
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

    uint8_t getLen()
    {
        return m_len;
    }

    uint8_t (&getData())[MaxFrameLen - 5]
    {
        return m_data;
    }
};

#endif // SERVER_FRAME