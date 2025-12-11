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
 * Show a fullscreen prompt dialog with Yes/No buttons
 */
void show_prompt_dialog(const char *prompt_text,
                        prompt_dialog_callback_t callback, void *user_data);

/**
 * Show an overlay prompt dialog with Yes/No buttons (semi-transparent,
 * centered)
 */
void show_prompt_dialog_overlay(const char *prompt_text,
                                prompt_dialog_callback_t callback,
                                void *user_data);

#endif // PROMPT_DIALOG_H
