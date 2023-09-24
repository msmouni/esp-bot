#include "io_pin.h"

esp_err_t IoPinBase::init(void)
{
    esp_err_t status{ESP_OK};

    status |= gpio_config(&m_cfg);

    return status;
}

///////////////////////////////////////////////////

esp_err_t IoPinOutput::init(void)
{
    esp_err_t status{IoPinBase::init()};

    if (ESP_OK == status)
    {
        status |= set(m_state);
    }

    return status;
}

esp_err_t IoPinOutput::set(const bool state)
{
    m_state = state;

    return gpio_set_level(m_pin_num, state);
}

esp_err_t IoPinOutput::toggle()
{
    return set(!m_state);
}

///////////////////////////////////////////////////

esp_err_t IoPinInput::init(void)
{
    return IoPinBase::init();
}

bool IoPinInput::state()
{
    return gpio_get_level(m_pin_num);
}
