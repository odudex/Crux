file(GLOB_RECURSE SOURCES "*.c")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS .
    PRIV_REQUIRES lvgl esp_lcd_touch_gt911 k_quirc esp_timer waveshare__esp32_p4_wifi6_touch_lcd_4b libwally-core cUR
)
