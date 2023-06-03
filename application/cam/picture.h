#ifndef CAM_PIC_H
#define CAM_PIC_H

#include "esp_camera.h"
#include "esp_log.h"
#include <stdint.h>
#include <algorithm>
#include <cstring>
#include "timer.h"
#include "server.h"
#include "option.h"

// Later: Endianness
void u32ToBytes(uint32_t val, char *p_buffer);

uint32_t bytesToU32(char *p_buffer);

#if ESP_CAMERA_SUPPORTED

enum class PictureState
{
    Unavailable,
    Available,
    Processing,
    Processed,
};

struct CamPicture
{
    static const uint32_t MAX_BUFFER_LEN = 4096;
    uint8_t m_pic_buff[MAX_BUFFER_LEN]; /*!< The pixel data */
    uint32_t m_len;                     /*!< Length of the buffer in bytes */
    uint32_t m_width;                   /*!< Width of the buffer in pixels */
    uint32_t m_height;                  /*!< Height of the buffer in pixels */
    PictureState m_state = PictureState::Unavailable;

    static const uint32_t MAX_FRAME_DATA_LEN = TcpIpServer::MAX_MSG_SIZE - 7; // See Frame
    char m_frame_buff[MAX_FRAME_DATA_LEN];
    uint8_t m_nb_frames_to_send;
    uint8_t m_nb_processed_frames;

    void set_nb_frames(uint32_t buff_len)
    {
        m_nb_frames_to_send = buff_len / MAX_FRAME_DATA_LEN;
        if (m_nb_frames_to_send * MAX_FRAME_DATA_LEN < buff_len)
        {
            m_nb_frames_to_send += 1;
        }
    }

    CamPicture()
    {
        m_nb_processed_frames = 0;
        set_nb_frames(MAX_BUFFER_LEN);
    }
    CamPicture(camera_fb_t *pic)
    {
        uint32_t len_to_copy = std::min((uint32_t)pic->len, (uint32_t)MAX_BUFFER_LEN);
        memcpy(m_pic_buff, pic->buf, len_to_copy);

        m_len = len_to_copy;
        m_width = pic->width;
        m_height = pic->height;

        m_nb_processed_frames = 0;
        set_nb_frames(m_len);
        m_state = PictureState::Available;
    }

    CamPicture(char *bytes)
    {
        m_len = bytesToU32(bytes);
        m_width = bytesToU32(bytes);
        m_height = bytesToU32(bytes);

        memcpy(m_pic_buff, bytes, MAX_BUFFER_LEN);

        m_nb_processed_frames = 0;
        set_nb_frames(m_len);
        m_state = PictureState::Available;
    }

    void update(camera_fb_t *pic)
    {
        // printf("Update: %d\n", (uint16_t)m_state);
        if (m_state == PictureState::Processed || m_state == PictureState::Unavailable)
        {
            uint32_t len_to_copy = std::min((uint32_t)pic->len, (uint32_t)MAX_BUFFER_LEN);
            memcpy(m_pic_buff, pic->buf, len_to_copy);

            m_len = len_to_copy;
            m_width = pic->width;
            m_height = pic->height;

            m_nb_processed_frames = 0;
            set_nb_frames(m_len);
            m_state = PictureState::Available;
            // printf("PictureState::Available\n");
        }
    }

    uint16_t toBytes(char *bytes)
    {
        u32ToBytes(m_len, bytes);
        u32ToBytes(m_width, bytes);
        u32ToBytes(m_height, bytes);

        // memcpy(bytes, m_pic_buff, MAX_BUFFER_LEN); // TMP

        m_nb_processed_frames = m_nb_processed_frames;
        m_state = PictureState::Processed;
        return MAX_BUFFER_LEN + 4 * 3;
    }

    bool isAvailable()
    {
        return m_state == PictureState::Available;
    }

    Option<ServerFrame<TcpIpServer::MAX_MSG_SIZE>> getNextFrame()
    {
        if ((m_state == PictureState::Available || m_state == PictureState::Processing) && (m_nb_processed_frames < m_nb_frames_to_send))
        {
            m_state = PictureState::Processing;
            uint32_t len_to_copy = std::min((uint32_t)(m_len - (m_nb_processed_frames * MAX_FRAME_DATA_LEN)), (uint32_t)MAX_FRAME_DATA_LEN);

            memcpy(m_frame_buff, m_pic_buff + (m_nb_processed_frames * MAX_FRAME_DATA_LEN), len_to_copy);

            Option<ServerFrame<TcpIpServer::MAX_MSG_SIZE>> res = Option<ServerFrame<TcpIpServer::MAX_MSG_SIZE>>(ServerFrame<TcpIpServer::MAX_MSG_SIZE>(ServerFrameId::CamPic, (uint16_t)len_to_copy, (uint8_t)m_nb_processed_frames, m_frame_buff));

            m_nb_processed_frames += 1;

            if (m_nb_processed_frames == m_nb_frames_to_send)
            {
                // printf("PictureState::Processed\n");
                m_state = PictureState::Processed;
            }
            else
            {
                // printf("PictureState::Processing\n");
            }
            return res;
        }
        else
        {
            return Option<ServerFrame<TcpIpServer::MAX_MSG_SIZE>>();
        }
    }

    // ServerFrame(ServerFrameId id, uint16_t len, uint8_t number, char (&data)[MaxFrameLen - 7])

    // template <uint16_t MaxFrameLen>
    // void fill
};

#endif // ESP_CAMERA_SUPPORTED

#endif // CAM_PIC_H