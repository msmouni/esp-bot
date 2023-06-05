# set(SOURCES wifi/wifi.cpp wifi/ev_handler.cpp)

# idf_component_register(SRCS ${SOURCES}
# INCLUDE_DIRS . wifi
# REQUIRES esp_wifi nvs_flash esp_event esp_netif lwip)

set(SRC_DIRECTORIES wifi server clients)

idf_component_register(SRC_DIRS ${SRC_DIRECTORIES}
    INCLUDE_DIRS ${SRC_DIRECTORIES}
    REQUIRES esp_wifi nvs_flash esp_event esp_netif lwip tools)