idf_component_register(
    SRCS main.c pages/splash_screen.c pages/login_pages/login.c pages/login_pages/about.c pages/qr_scanner.c ui_components/ui_menu.c ui_components/dark_theme.c ui_components/tron_theme.c
    INCLUDE_DIRS .
    REQUIRES lvgl esp_lcd_touch_gt911 quirc esp_timer waveshare__esp32_p4_wifi6_touch_lcd_4b
)
