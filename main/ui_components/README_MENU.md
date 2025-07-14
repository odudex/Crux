# UI Menu Component

A generic, reusable menu system for ESP32 LVGL applications. This component allows you to easily create menus with customizable entries and callbacks.

## Features

- **Reusable**: Create multiple menu instances throughout your application
- **Configurable**: Add up to 10 menu entries with custom names and callbacks
- **Interactive**: Support for touch and navigation controls
- **Visual**: Modern LVGL-based UI with proper styling
- **Memory efficient**: Proper resource management and cleanup

## Usage

### Basic Example

```c
#include "ui_components/ui_menu.h"

// Define callback functions
void option1_callback(void) {
    // Handle option 1 selection
}

void option2_callback(void) {
    // Handle option 2 selection
}

// Create and configure menu
lv_obj_t *parent = lv_screen_active();
ui_menu_t *menu = ui_menu_create(parent, "Main Menu");

// Add menu entries
ui_menu_add_entry(menu, "Option 1", option1_callback);
ui_menu_add_entry(menu, "Option 2", option2_callback);

// Show the menu
ui_menu_show(menu);
```

### Advanced Usage

```c
// Create menu with multiple options
ui_menu_t *settings_menu = ui_menu_create(parent, "Settings");

ui_menu_add_entry(settings_menu, "Display", display_settings_cb);
ui_menu_add_entry(settings_menu, "Audio", audio_settings_cb);
ui_menu_add_entry(settings_menu, "Network", network_settings_cb);
ui_menu_add_entry(settings_menu, "About", about_cb);

// Disable a specific entry
ui_menu_set_entry_enabled(settings_menu, 2, false); // Disable network

// Navigate programmatically
ui_menu_set_selected(settings_menu, 1); // Select audio option

// Execute selected entry
ui_menu_execute_selected(settings_menu);

// Clean up when done
ui_menu_destroy(settings_menu);
```

## API Reference

### Core Functions

- `ui_menu_create()` - Create a new menu instance
- `ui_menu_add_entry()` - Add a menu entry with callback
- `ui_menu_destroy()` - Clean up and free resources

### Navigation Functions

- `ui_menu_navigate_next()` - Move to next entry
- `ui_menu_navigate_prev()` - Move to previous entry
- `ui_menu_set_selected()` - Set specific entry as selected
- `ui_menu_get_selected()` - Get currently selected entry index

### Control Functions

- `ui_menu_show()` - Make menu visible
- `ui_menu_hide()` - Hide menu
- `ui_menu_execute_selected()` - Execute callback of selected entry
- `ui_menu_set_entry_enabled()` - Enable/disable specific entry

## Configuration

### Limits

- Maximum entries per menu: `UI_MENU_MAX_ENTRIES` (default: 10)
- Maximum entry name length: `UI_MENU_ENTRY_NAME_MAX_LEN` (default: 32)

### Styling

The menu uses LVGL's default styling but can be customized by modifying the styles applied to:
- `menu->container` - Main menu container
- `menu->title_label` - Menu title
- `menu->list` - List container for entries
- `menu->buttons[i]` - Individual menu buttons

## Example Implementation

See `pages/login.c` for a complete example implementation demonstrating:
- Menu creation with multiple entries
- Callback function implementations
- Integration with page management
- Error handling and logging

## Memory Management

The component automatically handles:
- LVGL object creation and destruction
- Memory allocation and cleanup
- Input group management

Always call `ui_menu_destroy()` when the menu is no longer needed to prevent memory leaks.

## Dependencies

- LVGL (Light and Versatile Graphics Library)
- ESP-IDF logging system
- FreeRTOS (for task delays in examples)

## Integration

To integrate the UI menu component into your project:

1. Add the source files to your CMakeLists.txt:
   ```cmake
   idf_component_register(
       SRCS your_sources.c ui_components/ui_menu.c
       INCLUDE_DIRS .
   )
   ```

2. Include the header in your source files:
   ```c
   #include "ui_components/ui_menu.h"
   ```

3. Create and use menus as shown in the examples above.
