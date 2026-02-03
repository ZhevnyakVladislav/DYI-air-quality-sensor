idf_component_register(
    SRC_DIRS "."  "drivers" "fonts" "icons"
    PRIV_INCLUDE_DIRS "." "drivers" "fonts" "icons" "${ESP_MATTER_PATH}/examples/common/utils"
    REQUIRES esp_matter driver nvs_flash esp_phy openthread
)