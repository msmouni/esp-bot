#ifndef ROBOT_CONTROL_H
#define ROBOT_CONTROL_H

struct RobotControl
{
    float m_x;
    float m_y;

    RobotControl() : m_x(0), m_y(0){};
    RobotControl(float x, float y) : m_x(x), m_y(y){};

    float getX()
    {
        return m_x;
    }

    float getY()
    {
        return m_y;
    }
};

#endif // ROBOT_CONTROL_H