idf_component_register(
    SRCS "video.c"
    INCLUDE_DIRS "."
    REQUIRES esp_cam_sensor esp_video waveshare_bsp
)