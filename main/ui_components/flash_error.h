#ifndef FLASH_ERROR_H
#define FLASH_ERROR_H

/**
 * Callback function type for flash error return action
 */
typedef void (*flash_error_callback_t)(void);

/**
 * Show a flash error message that auto-dismisses and returns to previous page
 * @param error_message The error message to display
 * @param return_callback The callback function to call when returning
 * (optional, can be NULL)
 * @param timeout_ms Timeout in milliseconds before auto-return (default: 2000ms
 * if 0)
 */
void show_flash_error(const char *error_message,
                      flash_error_callback_t return_callback, int timeout_ms);

#endif // FLASH_ERROR_H
