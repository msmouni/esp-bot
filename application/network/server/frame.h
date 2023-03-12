#ifndef SERVER_FRAME
#define SERVER_FRAME

// Note: https://www.geeksforgeeks.org/cc-preprocessors/

#include <stdint.h>
#include <algorithm>

// ServerFrame as uint8_t frame[Length]: [uint8_t ID[4], uint8_t FRAME_LENGTH, uint8_t DATA[DATA_LEN]]
template <uint8_t MaxFrameLen>
class ServerFrame
{
private:
    uint32_t m_id;
    uint8_t m_len;
    uint8_t m_data[MaxFrameLen - 5];

public:
    ServerFrame(){};
    ServerFrame(uint32_t id, uint8_t len, uint8_t (&data)[MaxFrameLen - 5]) : m_id(id), m_len(len)
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
        m_id = static_cast<uint32_t>((uint32_t(bytes_frame[0]) << 24) | (uint32_t(bytes_frame[1]) << 16) | (uint32_t(bytes_frame[2]) << 8) | (uint32_t(bytes_frame[3])));

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
        // Endianness ...
        buffer[0] = static_cast<uint8_t>(m_id >> 24);
        buffer[1] = static_cast<uint8_t>(m_id >> 16);
        buffer[2] = static_cast<uint8_t>(m_id >> 8);
        buffer[3] = static_cast<uint8_t>(m_id);

        buffer[4] = m_len;

        for (uint8_t i = 5; i < MaxFrameLen; i++)
        {
            buffer[i] = m_data[i - 5];
        }
    }

    void debug()
    {
        printf("ID: %ld\nLEN: %d\nData: [", m_id, m_len);

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
};

#endif // SERVER_FRAME