set(SOURCES "main.cpp" "program.cpp")

idf_component_register(SRCS ${SOURCES}
    INCLUDE_DIRS "."
    REQUIRES freertos
    esp_common
    network
    tools
    io_pin)