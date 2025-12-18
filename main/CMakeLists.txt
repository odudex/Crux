file(GLOB_RECURSE SOURCES "*.c")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS .
    PRIV_REQUIRES lvgl esp_lcd_touch_gt911 k_quirc esp_timer waveshare_bsp libwally-core cUR
)
