#ifndef ROBOT_H
#define ROBOT_H

#include "timer.h"
#include "io_pin.h"
#include "control.h"
#include "additional.h"
#include <mutex>

using namespace additional::option;

class EspBot
{
private:
    // TODO: Pins as constructor params or as template
    IoPinOutput m_move_forward_pin = IoPinOutput(GPIO_NUM_13, false);
    IoPinOutput m_move_backward_pin = IoPinOutput(GPIO_NUM_14, false);
    IoPinOutput m_turn_right_pin = IoPinOutput(GPIO_NUM_32, false);
    IoPinOutput m_turn_left_pin = IoPinOutput(GPIO_NUM_33, false);

    static bool m_move_forward_state;
    static bool m_move_backward_state;
    static bool m_turn_right_state;
    static bool m_turn_left_state;

    static std::mutex m_mutex;

    static Option<RobotControl> m_robot_control;

    PeriodicTimer *m_control_timer = NULL;
    static void processControlReq(void *);

public:
    EspBot();
    esp_err_t start();

    esp_err_t stop();

    void setControlReq(Option<RobotControl> opt_robot_control);

    void update();
};

#endif // ROBOT_H