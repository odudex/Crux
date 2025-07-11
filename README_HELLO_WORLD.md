# LVGL Hello World Demo for ESP32-P4

This is a minimal LVGL example that displays "Hello World!" on the ESP32-P4 Function EV Board's screen.

## Features

- Minimal dependencies (only essential LVGL and ESP32-P4 board support)
- Simple "Hello World" text display
- Clean project structure

## Dependencies

The project uses only the following minimal components:
- `espressif/esp32_p4_function_ev_board` - Board support package
- `espressif/esp_lvgl_port` - LVGL port for ESP-IDF
- `lvgl/lvgl` - LVGL graphics library (version 9.2.x)

## Hardware Setup

- ESP32-P4-Function-EV-Board
- 7-inch 1024 x 600 LCD screen with EK79007 driver IC
- USB-C cable for power and programming

### Connection Steps

1. Connect the pins on the back of the screen adapter board to the development board:
   | Screen Adapter Board | ESP32-P4-Function-EV-Board |
   | -------------------- | -------------------------- |
   | 5V (any one)         | 5V (any one)               |
   | GND (any one)        | GND (any one)              |
   | PWM                  | GPIO26                     |
   | LCD_RST              | GPIO27                     |

2. Connect the LCD FPC through the `MIPI_DSI` interface
3. Connect USB-C cable to the `USB-UART` port
4. Turn on the power switch

## Building and Running

1. Set the target:
   ```bash
   idf.py set-target esp32p4
   ```

2. Build the project:
   ```bash
   idf.py build
   ```

3. Flash and monitor:
   ```bash
   idf.py flash monitor
   ```

## What it does

The application:
1. Initializes the display with minimal configuration
2. Creates a simple label with "Hello World!" text
3. Sets the text to use Montserrat 48pt font in white color
4. Centers the text on the screen

## Technical Support

For technical queries and feedback:
- [ESP32 Forum](https://esp32.com/viewforum.php?f=22)
- [GitHub Issues](https://github.com/espressif/esp-dev-kits/issues)
