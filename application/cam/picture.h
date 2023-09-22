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

#if ESP_CAMERA_SUPPORTED

struct BuffRefLen
{
    void *m_buff_ref;
    int m_len;

    BuffRefLen(void *buff_ref = NULL, int len = 0) : m_buff_ref(buff_ref), m_len(len){};
};

enum class PictureState
{
    Unavailable,
    Available,
    Processed,
};

struct CamPicture
{
    static const uint16_t MAX_PIC_LEN = 8192; // 4096;
    // Note: https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32s2/api-reference/kconfig.html#config-esp32-wifi-tx-buffer
    static const uint16_t MAX_MSG_SIZE = 1024; // The size of each static TX buffer is fixed to about 1.6KB
    static const uint16_t FRAME_DATA_SIZE = MAX_MSG_SIZE - FRAME_DATA_OFFSET;
    static const uint8_t MAX_FRAMES_NB = uint8_t(MAX_PIC_LEN / FRAME_DATA_SIZE);

    ServerFrame<MAX_MSG_SIZE> m_pic_frames[MAX_FRAMES_NB]; /*!< The pixel data */
    uint8_t m_stored_frames;
    uint8_t m_processed_frames;
    uint32_t m_len;    /*!< Length of the buffer in bytes */
    uint32_t m_width;  /*!< Width of the buffer in pixels */
    uint32_t m_height; /*!< Height of the buffer in pixels */
    PictureState m_state = PictureState::Unavailable;

    CamPicture()
    {
        m_stored_frames = 0;
        m_processed_frames = 0;
    }

    void update(camera_fb_t *pic)
    {
        if (m_state == PictureState::Processed || m_state == PictureState::Unavailable)
        {
            uint8_t pic_frames_nb = (uint32_t)pic->len / (uint32_t)(FRAME_DATA_SIZE);
            if ((uint32_t)pic_frames_nb * (uint32_t)(FRAME_DATA_SIZE) < (uint32_t)pic->len)
            {
                pic_frames_nb++;
            }

            m_stored_frames = std::min(pic_frames_nb, (uint8_t)MAX_FRAMES_NB);

            uint32_t remaining_bytes = (uint32_t)pic->len;

            for (int i = 0; i < m_stored_frames; i++)
            {

                if (remaining_bytes < uint32_t(FRAME_DATA_SIZE))
                {
                    m_pic_frames[i].setHeader(ServerFrameId::CamPic, uint16_t(remaining_bytes), m_stored_frames - i - 1);
                    m_pic_frames[i].setData(pic->buf + ((uint32_t)pic->len - remaining_bytes), uint16_t(remaining_bytes));
                    remaining_bytes = 0;
                }
                else
                {
                    m_pic_frames[i].setHeader(ServerFrameId::CamPic, FRAME_DATA_SIZE, m_stored_frames - i - 1);
                    m_pic_frames[i].setData(pic->buf + ((uint32_t)pic->len - remaining_bytes), FRAME_DATA_SIZE);
                    remaining_bytes -= FRAME_DATA_SIZE;
                }
            }

            m_len = pic->len;

            m_width = pic->width;
            m_height = pic->height;

            m_state = PictureState::Available;
        }
    }

    bool isAvailable()
    {

        return m_state == PictureState::Available;
    }

    Option<BuffRefLen> getNextFrameBuff()
    {
        if (isAvailable())
        {
            return Option<BuffRefLen>(BuffRefLen(m_pic_frames[m_processed_frames].getBufferRef(), m_pic_frames[m_processed_frames].getLen() + FRAME_HEADER_LEN));
        }
        else
        {
            return Option<BuffRefLen>();
        }
    }

    void setCurrentFrameProcessed()
    {
        if (isAvailable())
        {
            m_processed_frames += 1;

            if (m_processed_frames == m_stored_frames)
            {
                m_processed_frames = 0;
                m_state = PictureState::Processed;
            }
        }
    }

    uint32_t getLen()
    {
        return m_len;
    }
};

#endif // ESP_CAMERA_SUPPORTED

#endif // CAM_PIC_H