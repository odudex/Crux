# Dark Theme System

This dark theme system provides a centralized and efficient way to apply consistent dark mode styling throughout your LVGL application.

## Features

- **Centralized Color Palette**: All dark theme colors are defined in one place
- **Consistent Styling**: Uniform appearance across all UI components
- **Easy to Use**: Simple functions to apply themes to different component types
- **Customizable**: Easy to modify colors and styling properties
- **Performance Optimized**: Efficient application of styles

## Quick Start

### 1. Initialize the Theme System

```c
#include "ui_components/dark_theme.h"

void app_main(void) {
    // Initialize LVGL and display...
    
    // Initialize dark theme
    dark_theme_init();
    
    // Apply to main screen
    lv_obj_t *screen = lv_screen_active();
    dark_theme_apply_screen(screen);
}
```

### 2. Create UI Components with Dark Theme

#### Buttons
```c
// Regular button
lv_obj_t *btn = dark_theme_create_button(parent, "Click Me", false);

// Primary button (with accent color)
lv_obj_t *primary_btn = dark_theme_create_button(parent, "Submit", true);

// Or apply theme to existing button
lv_obj_t *existing_btn = lv_btn_create(parent);
dark_theme_apply_button(existing_btn, false);
```

#### Labels
```c
// Primary text
lv_obj_t *title = dark_theme_create_label(parent, "Main Title", false);

// Secondary text
lv_obj_t *subtitle = dark_theme_create_label(parent, "Subtitle", true);

// Or apply theme to existing label
lv_obj_t *existing_label = lv_label_create(parent);
dark_theme_apply_label(existing_label, false);
```

#### Containers
```c
// Create themed container
lv_obj_t *container = dark_theme_create_container(parent);

// Or apply theme to existing container
lv_obj_t *existing_container = lv_obj_create(parent);
dark_theme_apply_container(existing_container);
```

#### Modals/Dialogs
```c
lv_obj_t *modal = lv_obj_create(lv_screen_active());
lv_obj_set_size(modal, 300, 200);
lv_obj_center(modal);
dark_theme_apply_modal(modal);
```

#### Input Fields
```c
lv_obj_t *textarea = lv_textarea_create(parent);
dark_theme_apply_input(textarea);
```

## Color Palette

The dark theme uses the following color scheme:

| Purpose | Color Code | Description |
|---------|------------|-------------|
| Primary Background | `#1e1e1e` | Main application background |
| Secondary Background | `#2e2e2e` | Cards, modals, containers |
| Tertiary Background | `#3e3e3e` | Buttons, interactive elements |
| Border Color | `#555555` | Borders and separators |
| Primary Text | `#ffffff` | Main text content |
| Secondary Text | `#cccccc` | Less important text |
| Disabled Text | `#888888` | Disabled state text |
| Accent Color | `#007acc` | Primary buttons, highlights |
| Success | `#28a745` | Success messages |
| Warning | `#ffc107` | Warning messages |
| Error | `#dc3545` | Error messages |

## Advanced Usage

### Creating Complex UI Components

```c
lv_obj_t* create_custom_card(lv_obj_t *parent, const char *title, const char *content) {
    // Create card container
    lv_obj_t *card = dark_theme_create_container(parent);
    lv_obj_set_size(card, 250, 150);
    
    // Title with primary text
    lv_obj_t *title_label = dark_theme_create_label(card, title, false);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 10, 10);
    
    // Content with secondary text
    lv_obj_t *content_label = dark_theme_create_label(card, content, true);
    lv_obj_align(content_label, LV_ALIGN_TOP_LEFT, 10, 40);
    
    // Action button
    lv_obj_t *action_btn = dark_theme_create_button(card, "Action", true);
    lv_obj_set_size(action_btn, 80, 30);
    lv_obj_align(action_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    
    return card;
}
```

### Customizing Colors

To modify the color scheme, edit the `dark_theme_config` in `dark_theme.c`:

```c
static dark_theme_config_t dark_theme_config = {
    .bg_primary = {.red = 0x1e, .green = 0x1e, .blue = 0x1e},
    .accent_color = {.red = 0x00, .green = 0x7a, .blue = 0xcc},
    // ... modify other colors as needed
};
```

### Getting Theme Configuration

```c
const dark_theme_config_t *config = dark_theme_get_config();
lv_obj_set_style_bg_color(my_object, config->bg_primary, 0);
```

## Integration with Existing Components

### UI Menu Component

The UI menu component has been updated to automatically use dark theme:

```c
ui_menu_t *menu = ui_menu_create(parent, "Main Menu");
ui_menu_add_entry(menu, "Option 1", callback1);
ui_menu_add_entry(menu, "Option 2", callback2);
// Dark theme is automatically applied
```

### Custom Styling

For components that need special styling beyond the standard theme:

```c
lv_obj_t *special_button = dark_theme_create_button(parent, "Special", false);

// Add custom styling on top of dark theme
lv_obj_set_style_bg_color(special_button, lv_color_hex(0xff6b35), 0); // Orange
lv_obj_set_style_shadow_width(special_button, 10, 0);
```

## Touch Interface Optimization

The dark theme system is optimized for touch interfaces by default:

- **No Focus Styling**: Buttons don't show focus states (blue contours) after being pressed
- **Touch-Friendly Buttons**: The `dark_theme_create_button()` function creates buttons optimized for touch
- **Clean Visual Feedback**: Only pressed state is shown during touch interaction

### Touch vs Keyboard Navigation

```c
// For touch interfaces (default, no focus styling)
lv_obj_t *touch_btn = dark_theme_create_button(parent, "Touch", false);

// For keyboard navigation (with focus styling)
lv_obj_t *kbd_btn = lv_btn_create(parent);
dark_theme_apply_button(kbd_btn, false); // Includes focus styling
lv_group_add_obj(input_group, kbd_btn);  // Add to input group for navigation
```

### Disabling Focus for Existing Buttons

```c
lv_obj_t *existing_btn = lv_btn_create(parent);
dark_theme_apply_button(existing_btn, false);
dark_theme_disable_focus(existing_btn); // Remove focus styling
```

## Best Practices

1. **Initialize Early**: Call `dark_theme_init()` right after LVGL initialization
2. **Use Theme Functions**: Prefer `dark_theme_create_*()` functions over manual styling
3. **Consistent Application**: Apply theme to all UI components for consistency
4. **Color Hierarchy**: Use primary text for important content, secondary for less important
5. **Button Types**: Use primary buttons sparingly for main actions
6. **Test on Hardware**: Always test the dark theme on your actual display hardware

## Examples

See `dark_theme_examples.c` for complete examples of:
- Settings page with navigation
- Main menu with multiple options
- Forms with input validation
- Status panels with indicators

## Migration from Manual Styling

To migrate existing code that uses manual dark styling:

**Before:**
```c
lv_obj_set_style_bg_color(obj, lv_color_hex(0x2e2e2e), 0);
lv_obj_set_style_text_color(label, lv_color_white(), 0);
lv_obj_set_style_border_color(obj, lv_color_hex(0x555555), 0);
```

**After:**
```c
dark_theme_apply_container(obj);
dark_theme_apply_label(label, false);
```

This approach is more maintainable and ensures consistency across your application.
