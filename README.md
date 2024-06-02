![Continuous Integration](https://github.com/msmouni/esp-bot/actions/workflows/esp-idf.yml/badge.svg?branch=main) 
# esp32-robot

# Project Overview:

This project involved developing firmware for real-time control of a mobile robot, managing WIFI connectivity, and integrating an onboard camera module. A cross-platform client application was created for both desktop and Android, supporting user authentication in visitor and administrator modes:

- Firmware development for real-time control of a 4 wheel mobile robot.
- Management of WIFI connectivity on STA and AP, and the on-board camera module.
- Development of a cross-platform client application (Desktop + Android).
- Client authentication management in both visitor and administrator modes.

# Tools:

- ESP-IDF and FreeRTOS: Used to develop the firmware, drivers, and real-time control algorithms in C++ for the ESP32 microcontroller.
- ESP32 and lwIP: Employed for WIFI connectivity, configuring the microcontroller as both Station (STA) and Access Point (AP) to handle network communication.
- Qt and QtCreator: Utilized to develop a responsive, cross-platform client application for desktop and Android, enabling the user’s authentication and his interaction with the robot and video reception. (https://github.com/msmouni/esp-bot-client)


# Notes:

If you are using the camera: Enable PSRAM in menuconfig (also set Flash and PSRAM frequiencies to 80MHz)

ESP-Idf Wifi configuration:
https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32s2/api-reference/kconfig.html#config-esp32-wifi-tx-buffer

https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32s2/api-reference/kconfig.html#config-spiram-try-allocate-wifi-lwip
