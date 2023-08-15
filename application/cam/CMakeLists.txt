set(SRC_DIRECTORIES ".")

idf_component_register(SRC_DIRS ${SRC_DIRECTORIES}
    INCLUDE_DIRS ${SRC_DIRECTORIES}
    REQUIRES esp_event tools network esp32-camera)