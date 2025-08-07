idf_component_register(
    SRCS "video.c"
    INCLUDE_DIRS "."
    REQUIRES esp_cam_sensor esp_video waveshare__esp32_p4_wifi6_touch_lcd_4b
)