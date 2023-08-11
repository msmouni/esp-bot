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
#include "frame.h"

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
    static const uint32_t MAX_FRAME_BUFFER_LEN = 4096;
    uint8_t m_pic_frame_buff[MAX_FRAME_BUFFER_LEN]; /*!< The pixel data */
    uint32_t m_len;                                 /*!< Length of the buffer in bytes */
    uint32_t m_width;                               /*!< Width of the buffer in pixels */
    uint32_t m_height;                              /*!< Height of the buffer in pixels */
    PictureState m_state = PictureState::Unavailable;

    // static const uint32_t MAX_FRAME_DATA_LEN = TcpIpServer::MAX_MSG_SIZE - FRAME_HEADER_LEN; // See Frame
    // char m_frame_buff[MAX_FRAME_BUFFER_LEN];
    // ServerFrame<MAX_FRAME_DATA_LEN> m_pic_frame = ServerFrame<MAX_FRAME_DATA_LEN>(ServerFrameId::CamPic);
    // uint8_t m_nb_frames_to_send;
    // uint8_t m_nb_processed_frames;

    // void set_nb_frames(uint32_t buff_len)
    // {
    //     m_nb_frames_to_send = buff_len / MAX_FRAME_DATA_LEN;
    //     if (m_nb_frames_to_send * MAX_FRAME_DATA_LEN < buff_len)
    //     {
    //         m_nb_frames_to_send += 1;
    //     }
    // }

    void setFrameHeader()
    {
        uint8_t header_buff[FRAME_HEADER_LEN] = {0};
        ServerFrame<FRAME_HEADER_LEN>::setHeader(header_buff, ServerFrameId::CamPic, MAX_FRAME_BUFFER_LEN);

        memcpy(m_pic_frame_buff, header_buff, FRAME_HEADER_LEN);
    }
    CamPicture()
    {
        setFrameHeader();
        // m_nb_processed_frames = 0;
        // set_nb_frames(MAX_BUFFER_LEN);
    }
    CamPicture(camera_fb_t *pic)
    {
        setFrameHeader();

        uint32_t len_to_copy = std::min((uint32_t)pic->len, (uint32_t)(MAX_FRAME_BUFFER_LEN - FRAME_HEADER_LEN));
        memcpy(&m_pic_frame_buff + FRAME_HEADER_LEN, pic->buf, len_to_copy);

        m_len = len_to_copy;
        // m_pic_frame.setData(pic->buf, (uint16_t)pic->len);

        m_width = pic->width;
        m_height = pic->height;

        // m_nb_processed_frames = 0;
        // set_nb_frames(m_len);
        m_state = PictureState::Available;
    }

    CamPicture(char *bytes)
    {
        setFrameHeader();

        m_len = bytesToU32(bytes);
        m_width = bytesToU32(bytes);
        m_height = bytesToU32(bytes);

        // m_len = m_pic_frame.setData((uint8_t *)bytes, m_len);

        memcpy(m_pic_frame_buff, bytes, (uint32_t)(MAX_FRAME_BUFFER_LEN - FRAME_HEADER_LEN));

        // m_nb_processed_frames = 0;
        // set_nb_frames(m_len);
        m_state = PictureState::Available;
    }

    void update(camera_fb_t *pic)
    {
        // printf("Update: %d\n", (uint16_t)m_state);
        if (m_state == PictureState::Processed || m_state == PictureState::Unavailable)
        {
            // uint32_t len_to_copy = std::min((uint32_t)pic->len, (uint32_t)MAX_BUFFER_LEN);
            // memcpy(m_pic_buff, pic->buf, len_to_copy);

            // m_len = len_to_copy;
            // m_width = pic->width;
            // m_height = pic->height;

            // m_nb_processed_frames = 0;
            // set_nb_frames(m_len);

            uint32_t len_to_copy = std::min((uint32_t)pic->len, (uint32_t)(MAX_FRAME_BUFFER_LEN - FRAME_HEADER_LEN));

            // printf("len_to_copy:%ld\n", len_to_copy);
            memcpy(m_pic_frame_buff + FRAME_HEADER_LEN, pic->buf, len_to_copy);

            m_len = len_to_copy;
            // m_pic_frame.setData(pic->buf, (uint16_t)pic->len);

            m_width = pic->width;
            m_height = pic->height;

            // m_len = m_pic_frame.setData(pic->buf, (uint16_t)pic->len);

            // m_width = pic->width;
            // m_height = pic->height;

            m_state = PictureState::Available;
            // printf("PictureState::Available\n");
        }
    }

    /*uint16_t toBytes(char *bytes)
    {
        u32ToBytes(m_len, bytes);
        u32ToBytes(m_width, bytes);
        u32ToBytes(m_height, bytes);

        // memcpy(bytes, m_pic_buff, MAX_BUFFER_LEN); // TMP

        m_nb_processed_frames = m_nb_processed_frames;
        m_state = PictureState::Processed;
        return MAX_BUFFER_LEN + 4 * 3;
    }*/

    bool isAvailable()
    {

        return m_state == PictureState::Available;
    }

    void *getFrameRef()
    {
        return &m_pic_frame_buff;
    }

    uint32_t getLen()
    {
        // return m_len;
        return MAX_FRAME_BUFFER_LEN - 7; // TMP: Fixed for Stream (TCP)
    }

    void setProcessed()
    {
        m_state = PictureState::Processed; // TMP: TODO: USE WATCHDOG
    }

    /*Option<ServerFrame<TcpIpServer::MAX_MSG_SIZE>> getNextFrame()
    {
        if ((m_state == PictureState::Available || m_state == PictureState::Processing) && (m_nb_processed_frames < m_nb_frames_to_send))
        {
            m_state = PictureState::Processing;
            uint32_t len_to_copy = std::min((uint32_t)(m_len - (m_nb_processed_frames * MAX_FRAME_DATA_LEN)), (uint32_t)MAX_FRAME_DATA_LEN);

            memcpy(m_frame_buff, m_pic_buff + (m_nb_processed_frames * MAX_FRAME_DATA_LEN), len_to_copy);

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

            ServerFrame<TcpIpServer::MAX_MSG_SIZE> frame = ServerFrame<TcpIpServer::MAX_MSG_SIZE>(ServerFrameId::CamPic, (uint16_t)len_to_copy, (uint8_t)(m_nb_frames_to_send - m_nb_processed_frames), m_frame_buff);
            // frame.debug();

            Option<ServerFrame<TcpIpServer::MAX_MSG_SIZE>> res = Option<ServerFrame<TcpIpServer::MAX_MSG_SIZE>>(frame);

            return res;
        }
        else
        {
            return Option<ServerFrame<TcpIpServer::MAX_MSG_SIZE>>();
        }
    }*/

    // ServerFrame(ServerFrameId id, uint16_t len, uint8_t number, char (&data)[MaxFrameLen - 7])

    // template <uint16_t MaxFrameLen>
    // void fill
};

#endif // ESP_CAMERA_SUPPORTED

#endif // CAM_PIC_H