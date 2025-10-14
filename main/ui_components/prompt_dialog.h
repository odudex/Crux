#ifndef PROMPT_DIALOG_H
#define PROMPT_DIALOG_H

#include <stdbool.h>

/**
 * Callback function type for prompt dialog response
 * @param result true if user clicked "Yes", false if user clicked "No"
 * @param user_data Optional user data passed to the callback
 */
typedef void (*prompt_dialog_callback_t)(bool result, void *user_data);

/**
 * Show a prompt dialog with Yes/No buttons
 * @param prompt_text The prompt message to display
 * @param callback The callback function to call when user responds
 * @param user_data Optional user data to pass to the callback
 */
void show_prompt_dialog(const char *prompt_text,
                        prompt_dialog_callback_t callback, void *user_data);

#endif // PROMPT_DIALOG_H
