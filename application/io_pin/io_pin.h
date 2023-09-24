#ifndef IO_PIN_H
#define IO_PIN_H

#include "driver/gpio.h"

class IoPinBase
{
protected:
    const gpio_num_t m_pin_num;
    const gpio_config_t m_cfg;

public:
    constexpr IoPinBase(const gpio_num_t pin_num,
                        const gpio_config_t &config) : m_pin_num(pin_num),
                                                       m_cfg(config)
    {
    }

    virtual bool state(void) = 0;
    virtual esp_err_t set(const bool state) = 0;

    esp_err_t init(void);
};

///////////////////////////////////////////////////

class IoPinOutput : public IoPinBase
{
    bool m_state = false;

public:
    constexpr IoPinOutput(const gpio_num_t pin, bool state) : IoPinBase(pin,
                                                                        gpio_config_t{
                                                                            .pin_bit_mask = static_cast<uint64_t>(1) << pin,
                                                                            .mode = GPIO_MODE_OUTPUT,
                                                                            .pull_up_en = GPIO_PULLUP_DISABLE,
                                                                            .pull_down_en = GPIO_PULLDOWN_ENABLE,
                                                                            .intr_type = GPIO_INTR_DISABLE}),
                                                              m_state(state)
    {
    }

    esp_err_t init(void);

    esp_err_t set(const bool state);
    esp_err_t toggle(void);
    bool state(void) { return m_state; }
};

///////////////////////////////////////////////////

class IoPinInput : public IoPinBase
{

public:
    constexpr IoPinInput(const gpio_num_t pin) : IoPinBase(pin,
                                                           gpio_config_t{
                                                               .pin_bit_mask = static_cast<uint64_t>(1) << pin,
                                                               .mode = GPIO_MODE_INPUT,
                                                               .pull_up_en = GPIO_PULLUP_DISABLE,
                                                               .pull_down_en = GPIO_PULLDOWN_ENABLE, // GPIO_PULLDOWN_DISABLE,
                                                               .intr_type = GPIO_INTR_DISABLE})
    {
    }

    esp_err_t init(void);

    bool state(void);
};

#endif // IO_PIN_H