set(SRC_DIRECTORIES buffer time additional)

idf_component_register(SRC_DIRS ${SRC_DIRECTORIES}
    INCLUDE_DIRS ${SRC_DIRECTORIES}
    REQUIRES esp_event esp_timer)