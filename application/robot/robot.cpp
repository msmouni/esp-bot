#include "robot.h"

bool EspBot::m_move_forward_state = false;
bool EspBot::m_move_backward_state = false;
bool EspBot::m_turn_right_state = false;
bool EspBot::m_turn_left_state = false;

std::mutex EspBot::m_mutex = {};

Option<RobotControl> EspBot::m_robot_control = Option<RobotControl>();

void EspBot::processControlReq(void *args)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_robot_control.isSome())
    {
        RobotControl robot_control = m_robot_control.getData();

        m_move_forward_state = robot_control.getY() >= 0.5;
        m_move_backward_state = robot_control.getY() <= -0.5;

        m_turn_right_state = robot_control.getX() >= 0.5;
        m_turn_left_state = robot_control.getX() <= -0.5;
    }
    else
    {
        m_move_forward_state = false;
        m_move_backward_state = false;
        m_turn_right_state = false;
        m_turn_left_state = false;
    }
}

EspBot::EspBot(){};

esp_err_t EspBot::start()
{
    esp_err_t status = m_move_forward_pin.init();
    if (status == ESP_OK)
    {
        status = m_move_backward_pin.init();
    }

    if (status == ESP_OK)
    {
        status = m_turn_right_pin.init();
    }

    if (status == ESP_OK)
    {
        status = m_turn_left_pin.init();
    }

    if (status == ESP_OK)
    {
        if (m_control_timer == NULL)
        {
            m_control_timer = new PeriodicTimer("Robot Control", processControlReq, NULL, 100000); // 100ms
        }
        status = m_control_timer->start();
    }

    return status;
};

esp_err_t EspBot::stop()
{
    esp_err_t res = m_control_timer->stop();

    m_move_forward_state = false;
    m_move_backward_state = false;
    m_turn_right_state = false;
    m_turn_left_state = false;

    return res;
};

void EspBot::setControlReq(Option<RobotControl> opt_robot_control)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_robot_control = opt_robot_control;
};

void EspBot::update()
{
    if (m_move_forward_pin.state() != m_move_forward_state)
    {
        m_move_forward_pin.set(m_move_forward_state);
    }
    if (m_move_backward_pin.state() != m_move_backward_state)
    {
        m_move_backward_pin.set(m_move_backward_state);
    }
    if (m_turn_right_pin.state() != m_turn_right_state)
    {
        m_turn_right_pin.set(m_turn_right_state);
    }
    if (m_turn_left_pin.state() != m_turn_left_state)
    {
        m_turn_left_pin.set(m_turn_left_state);
    }
}