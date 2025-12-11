/*
 * Manual Mnemonic Input Page
 * Allows users to enter BIP39 mnemonic words one by one using a keyboard
 * with smart filtering that disables invalid letters
 */

#include "manual_input.h"
#include "../../../ui_components/flash_error.h"
#include "../../../ui_components/prompt_dialog.h"
#include "../../../ui_components/theme.h"
#include "../../../ui_components/ui_keyboard.h"
#include "../../../ui_components/ui_menu.h"
#include "../../../utils/memory_utils.h"
#include "esp_log.h"
#include "mnemonic_loading.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <wally_bip39.h>
#include <wally_core.h>

static const char *TAG = "MANUAL_INPUT";

// UI mode enum
typedef enum {
  MODE_WORD_COUNT_SELECT, // Selecting 12 or 24 words
  MODE_KEYBOARD_INPUT,    // Typing on keyboard
  MODE_WORD_SELECT        // Selecting from filtered word list
} input_mode_t;

// Maximum prefix length for filtering
#define MAX_PREFIX_LEN 8
// Maximum filtered words to show in menu
#define MAX_FILTERED_WORDS 8
// BIP39 wordlist size
#define BIP39_WORDLIST_SIZE 2048
// Maximum mnemonic length (24 words * 8 chars + spaces)
#define MAX_MNEMONIC_LEN 256

// Page state
static lv_obj_t *manual_input_screen = NULL;
static ui_menu_t *current_menu = NULL;
static ui_keyboard_t *keyboard = NULL;
static void (*return_callback)(void) = NULL;
static void (*success_callback)(void) = NULL;

// Mnemonic state
static int total_words = 0;        // 12 or 24
static int current_word_index = 0; // 0-based index of current word being entered
static char entered_words[24][16]; // Storage for entered words
static char current_prefix[MAX_PREFIX_LEN + 1]; // Current typing prefix
static int prefix_len = 0;

// Filtered words cache
static const char *filtered_words[MAX_FILTERED_WORDS];
static int filtered_count = 0;

// Current mode
static input_mode_t current_mode = MODE_WORD_COUNT_SELECT;

// BIP39 wordlist reference
static struct words *wordlist = NULL;

// Pending word awaiting confirmation
static char pending_word[16] = {0};

// Forward declarations
static void word_confirmation_cb(bool confirmed, void *user_data);
static void create_word_count_menu(void);
static void create_keyboard_input(void);
static void create_word_select_menu(void);
static void update_keyboard_state(void);
static uint32_t get_valid_letters_mask(void);
static void filter_words_by_prefix(void);
static void word_count_12_cb(void);
static void word_count_24_cb(void);
static void keyboard_callback(char key);
static void word_selected_cb(void);
static void back_to_keyboard_cb(void);
static void back_cb(void);
static void finish_mnemonic(void);
static void cleanup_ui(void);

// Initialize BIP39 wordlist
static bool init_wordlist(void) {
  if (wordlist) {
    return true;
  }
  int ret = bip39_get_wordlist(NULL, &wordlist); // NULL = English
  if (ret != WALLY_OK) {
    ESP_LOGE(TAG, "Failed to get BIP39 wordlist: %d", ret);
    return false;
  }
  return true;
}

// Get a bitmask of valid next letters for current prefix
// Bit N is set if adding letter 'a'+N to current_prefix would match at least one BIP39 word
static uint32_t get_valid_letters_mask(void) {
  uint32_t mask = 0;

  if (!wordlist) {
    return 0xFFFFFFFF; // All enabled if no wordlist
  }

  // For each letter a-z, check if prefix + letter matches any word
  for (int letter = 0; letter < 26; letter++) {
    char test_prefix[MAX_PREFIX_LEN + 2];
    snprintf(test_prefix, sizeof(test_prefix), "%s%c", current_prefix, 'a' + letter);
    size_t test_len = prefix_len + 1;

    // Check if any word starts with this prefix
    for (size_t i = 0; i < BIP39_WORDLIST_SIZE; i++) {
      const char *word = bip39_get_word_by_index(wordlist, i);
      if (word && strncmp(word, test_prefix, test_len) == 0) {
        mask |= (1u << letter);
        break; // Found at least one match for this letter
      }
    }
  }

  return mask;
}

// Filter BIP39 words by current prefix
static void filter_words_by_prefix(void) {
  filtered_count = 0;

  if (!wordlist || prefix_len == 0) {
    return;
  }

  for (size_t i = 0; i < BIP39_WORDLIST_SIZE && filtered_count < MAX_FILTERED_WORDS; i++) {
    const char *word = bip39_get_word_by_index(wordlist, i);
    if (word && strncmp(word, current_prefix, prefix_len) == 0) {
      filtered_words[filtered_count++] = word;
    }
  }

  ESP_LOGI(TAG, "Filtered %d words for prefix '%s'", filtered_count, current_prefix);
}

// Count total matching words (not just first MAX_FILTERED_WORDS)
static int count_matching_words(void) {
  int count = 0;

  if (!wordlist || prefix_len == 0) {
    return BIP39_WORDLIST_SIZE;
  }

  for (size_t i = 0; i < BIP39_WORDLIST_SIZE; i++) {
    const char *word = bip39_get_word_by_index(wordlist, i);
    if (word && strncmp(word, current_prefix, prefix_len) == 0) {
      count++;
    }
  }

  return count;
}

// Cleanup current UI elements
static void cleanup_ui(void) {
  if (current_menu) {
    ui_menu_destroy(current_menu);
    current_menu = NULL;
  }
  if (keyboard) {
    ui_keyboard_destroy(keyboard);
    keyboard = NULL;
  }
}

// Show word confirmation dialog
static void show_word_confirmation(const char *word) {
  // Store pending word
  strncpy(pending_word, word, sizeof(pending_word) - 1);
  pending_word[sizeof(pending_word) - 1] = '\0';

  // Build confirmation message
  char msg[64];
  snprintf(msg, sizeof(msg), "Word %d: %s", current_word_index + 1, word);

  // Hide keyboard while showing dialog
  if (keyboard) {
    ui_keyboard_hide(keyboard);
  }

  show_prompt_dialog(msg, word_confirmation_cb, NULL);
}

// Word confirmation callback
static void word_confirmation_cb(bool confirmed, void *user_data) {
  (void)user_data;

  if (confirmed) {
    // Store the confirmed word
    snprintf(entered_words[current_word_index], sizeof(entered_words[0]), "%s", pending_word);
    ESP_LOGI(TAG, "Confirmed word %d: %s", current_word_index + 1, pending_word);

    current_word_index++;
    prefix_len = 0;
    current_prefix[0] = '\0';
    pending_word[0] = '\0';

    // Cleanup any existing UI before proceeding
    cleanup_ui();

    if (current_word_index >= total_words) {
      finish_mnemonic();
    } else {
      // Create keyboard for next word
      create_keyboard_input();
    }
  } else {
    // User declined - go back to editing current word
    ESP_LOGI(TAG, "Word declined, returning to keyboard");
    pending_word[0] = '\0';

    // Cleanup menu if it exists (user came from word select)
    if (current_menu) {
      ui_menu_destroy(current_menu);
      current_menu = NULL;
    }

    // Show keyboard for editing (prefix is still there)
    if (keyboard) {
      ui_keyboard_show(keyboard);
      update_keyboard_state();
    } else {
      create_keyboard_input();
    }
  }
}

// Create word count selection menu (12 or 24 words)
static void create_word_count_menu(void) {
  cleanup_ui();
  current_mode = MODE_WORD_COUNT_SELECT;

  current_menu = ui_menu_create(manual_input_screen, "Mnemonic Length");
  if (!current_menu) {
    ESP_LOGE(TAG, "Failed to create word count menu");
    return;
  }

  ui_menu_add_entry(current_menu, "12 Words", word_count_12_cb);
  ui_menu_add_entry(current_menu, "24 Words", word_count_24_cb);
  ui_menu_add_entry(current_menu, "Back", back_cb);

  ui_menu_show(current_menu);
}

// Update keyboard state (title, input text, valid letters)
static void update_keyboard_state(void) {
  if (!keyboard) {
    return;
  }

  // Update title
  char title[32];
  snprintf(title, sizeof(title), "Word %d/%d", current_word_index + 1, total_words);
  ui_keyboard_set_title(keyboard, title);

  // Update input display
  ui_keyboard_set_input_text(keyboard, current_prefix);

  // Update valid letters
  uint32_t valid_mask = get_valid_letters_mask();
  ui_keyboard_set_letters_enabled(keyboard, valid_mask);

  // Backspace enabled if we have prefix
  ui_keyboard_set_key_enabled(keyboard, UI_KB_KEY_BACKSPACE, prefix_len > 0);

  // OK button enabled if we have exactly one matching word, or if we're showing word select
  int match_count = count_matching_words();
  ui_keyboard_set_ok_enabled(keyboard, match_count > 0 && match_count <= MAX_FILTERED_WORDS);

  ESP_LOGD(TAG, "Updated keyboard: prefix='%s', valid_mask=0x%08lx, matches=%d",
           current_prefix, (unsigned long)valid_mask, match_count);
}

// Create keyboard input screen
static void create_keyboard_input(void) {
  cleanup_ui();
  current_mode = MODE_KEYBOARD_INPUT;

  char title[32];
  snprintf(title, sizeof(title), "Word %d/%d", current_word_index + 1, total_words);

  keyboard = ui_keyboard_create(manual_input_screen, title, keyboard_callback);
  if (!keyboard) {
    ESP_LOGE(TAG, "Failed to create keyboard");
    return;
  }

  update_keyboard_state();
  ui_keyboard_show(keyboard);
}

// Create word selection menu from filtered words
static void create_word_select_menu(void) {
  cleanup_ui();
  current_mode = MODE_WORD_SELECT;

  filter_words_by_prefix();

  if (filtered_count == 0) {
    ESP_LOGW(TAG, "No words match prefix '%s'", current_prefix);
    create_keyboard_input();
    return;
  }

  char title[64];
  snprintf(title, sizeof(title), "Select: %s...", current_prefix);

  current_menu = ui_menu_create(manual_input_screen, title);
  if (!current_menu) {
    ESP_LOGE(TAG, "Failed to create word select menu");
    return;
  }

  // Add filtered words
  for (int i = 0; i < filtered_count; i++) {
    ui_menu_add_entry(current_menu, filtered_words[i], word_selected_cb);
  }

  ui_menu_add_entry(current_menu, "< Back", back_to_keyboard_cb);

  ui_menu_show(current_menu);
}

// Callbacks
static void word_count_12_cb(void) {
  ESP_LOGI(TAG, "Selected 12-word mnemonic");
  total_words = 12;
  current_word_index = 0;
  prefix_len = 0;
  current_prefix[0] = '\0';
  memset(entered_words, 0, sizeof(entered_words));
  create_keyboard_input();
}

static void word_count_24_cb(void) {
  ESP_LOGI(TAG, "Selected 24-word mnemonic");
  total_words = 24;
  current_word_index = 0;
  prefix_len = 0;
  current_prefix[0] = '\0';
  memset(entered_words, 0, sizeof(entered_words));
  create_keyboard_input();
}

// Keyboard key callback
static void keyboard_callback(char key) {
  ESP_LOGI(TAG, "Keyboard callback: key='%c' (0x%02x)", key >= 32 ? key : '?', key);

  if (key >= 'a' && key <= 'z') {
    // Add letter to prefix
    if (prefix_len < MAX_PREFIX_LEN) {
      current_prefix[prefix_len++] = key;
      current_prefix[prefix_len] = '\0';
      ESP_LOGI(TAG, "Added letter '%c', prefix now: '%s'", key, current_prefix);

      // Check if there's exactly one matching word
      filter_words_by_prefix();
      if (filtered_count == 1) {
        // Auto-select the only matching word - ask for confirmation
        ESP_LOGI(TAG, "Auto-selected word: %s", filtered_words[0]);
        show_word_confirmation(filtered_words[0]);
      } else {
        update_keyboard_state();
      }
    }
  } else if (key == UI_KB_BACKSPACE) {
    // Delete last letter
    if (prefix_len > 0) {
      prefix_len--;
      current_prefix[prefix_len] = '\0';
      ESP_LOGI(TAG, "Backspace, prefix now: '%s'", current_prefix);
      update_keyboard_state();
    } else if (current_word_index > 0) {
      // Go back to previous word
      current_word_index--;
      strncpy(current_prefix, entered_words[current_word_index], MAX_PREFIX_LEN);
      current_prefix[MAX_PREFIX_LEN] = '\0';
      prefix_len = strlen(current_prefix);
      entered_words[current_word_index][0] = '\0';
      ESP_LOGI(TAG, "Went back to word %d, prefix: '%s'", current_word_index + 1, current_prefix);
      update_keyboard_state();
    }
  } else if (key == UI_KB_OK) {
    // Show word selection menu
    filter_words_by_prefix();
    if (filtered_count > 0) {
      create_word_select_menu();
    }
  }
}

static void word_selected_cb(void) {
  if (!current_menu) {
    return;
  }

  int selected = ui_menu_get_selected(current_menu);
  if (selected < 0 || selected >= filtered_count) {
    return;
  }

  const char *word = filtered_words[selected];
  ESP_LOGI(TAG, "User selected word: %s", word);

  // Hide the menu before showing confirmation
  if (current_menu) {
    ui_menu_hide(current_menu);
  }

  // Show confirmation dialog
  show_word_confirmation(word);
}

static void back_to_keyboard_cb(void) {
  ESP_LOGI(TAG, "Returning to keyboard");
  create_keyboard_input();
}

static void back_cb(void) {
  ESP_LOGI(TAG, "Back requested from mode %d", current_mode);

  switch (current_mode) {
  case MODE_WORD_COUNT_SELECT:
    // Return to load menu
    if (return_callback) {
      return_callback();
    }
    break;

  case MODE_KEYBOARD_INPUT:
    if (prefix_len > 0) {
      // Clear prefix first
      prefix_len = 0;
      current_prefix[0] = '\0';
      update_keyboard_state();
    } else if (current_word_index > 0) {
      // Go back to previous word
      current_word_index--;
      strncpy(current_prefix, entered_words[current_word_index], MAX_PREFIX_LEN);
      current_prefix[MAX_PREFIX_LEN] = '\0';
      prefix_len = strlen(current_prefix);
      entered_words[current_word_index][0] = '\0';
      ESP_LOGI(TAG, "Went back to word %d", current_word_index + 1);
      update_keyboard_state();
    } else {
      // Go back to word count selection
      create_word_count_menu();
    }
    break;

  case MODE_WORD_SELECT:
    create_keyboard_input();
    break;
  }
}

// Build and validate the complete mnemonic
static void finish_mnemonic(void) {
  ESP_LOGI(TAG, "Building complete mnemonic from %d words", total_words);

  // Build mnemonic string
  char mnemonic[MAX_MNEMONIC_LEN];
  mnemonic[0] = '\0';

  for (int i = 0; i < total_words; i++) {
    if (i > 0) {
      strncat(mnemonic, " ", sizeof(mnemonic) - strlen(mnemonic) - 1);
    }
    strncat(mnemonic, entered_words[i], sizeof(mnemonic) - strlen(mnemonic) - 1);
  }

  ESP_LOGI(TAG, "Mnemonic built, validating...");

  // Validate the mnemonic
  if (bip39_mnemonic_validate(NULL, mnemonic) != WALLY_OK) {
    ESP_LOGE(TAG, "Invalid mnemonic checksum");
    show_flash_error("Invalid checksum", NULL, 0);
    // Go back to first word
    current_word_index = 0;
    prefix_len = 0;
    current_prefix[0] = '\0';
    memset(entered_words, 0, sizeof(entered_words));
    create_keyboard_input();
    return;
  }

  ESP_LOGI(TAG, "Mnemonic valid, transitioning to loading page");

  // Hide our page and transition to mnemonic loading page
  manual_input_page_hide();

  // Create mnemonic loading page with the entered mnemonic
  mnemonic_loading_page_create(lv_screen_active(), return_callback,
                               success_callback, mnemonic);
  mnemonic_loading_page_show();
}

// Public API implementation
void manual_input_page_create(lv_obj_t *parent, void (*return_cb)(void),
                              void (*success_cb)(void)) {
  if (!parent) {
    ESP_LOGE(TAG, "Invalid parent object");
    return;
  }

  ESP_LOGI(TAG, "Creating manual input page");

  return_callback = return_cb;
  success_callback = success_cb;

  // Initialize wordlist
  if (!init_wordlist()) {
    show_flash_error("Failed to load wordlist", return_cb, 0);
    return;
  }

  // Reset state
  total_words = 0;
  current_word_index = 0;
  prefix_len = 0;
  current_prefix[0] = '\0';
  filtered_count = 0;
  memset(entered_words, 0, sizeof(entered_words));

  // Create screen container
  manual_input_screen = lv_obj_create(parent);
  lv_obj_set_size(manual_input_screen, LV_PCT(100), LV_PCT(100));
  theme_apply_screen(manual_input_screen);

  // Start with word count selection
  create_word_count_menu();

  ESP_LOGI(TAG, "Manual input page created successfully");
}

void manual_input_page_show(void) {
  if (manual_input_screen) {
    lv_obj_clear_flag(manual_input_screen, LV_OBJ_FLAG_HIDDEN);
  }
  if (current_mode == MODE_KEYBOARD_INPUT && keyboard) {
    ui_keyboard_show(keyboard);
  } else if (current_menu) {
    ui_menu_show(current_menu);
  }
  ESP_LOGI(TAG, "Manual input page shown");
}

void manual_input_page_hide(void) {
  if (manual_input_screen) {
    lv_obj_add_flag(manual_input_screen, LV_OBJ_FLAG_HIDDEN);
  }
  if (keyboard) {
    ui_keyboard_hide(keyboard);
  }
  if (current_menu) {
    ui_menu_hide(current_menu);
  }
  ESP_LOGI(TAG, "Manual input page hidden");
}

void manual_input_page_destroy(void) {
  cleanup_ui();

  if (manual_input_screen) {
    lv_obj_del(manual_input_screen);
    manual_input_screen = NULL;
  }

  // Clear sensitive data
  memset(entered_words, 0, sizeof(entered_words));
  memset(current_prefix, 0, sizeof(current_prefix));
  memset(pending_word, 0, sizeof(pending_word));

  return_callback = NULL;
  success_callback = NULL;
  total_words = 0;
  current_word_index = 0;
  prefix_len = 0;
  filtered_count = 0;
  current_mode = MODE_WORD_COUNT_SELECT;

  // Note: wordlist is static from libwally, don't free it

  ESP_LOGI(TAG, "Manual input page destroyed");
}

bool manual_input_page_navigate_next(void) {
  if (current_mode == MODE_KEYBOARD_INPUT && keyboard) {
    return ui_keyboard_navigate_next(keyboard);
  } else if (current_menu) {
    return ui_menu_navigate_next(current_menu);
  }
  return false;
}

bool manual_input_page_navigate_prev(void) {
  if (current_mode == MODE_KEYBOARD_INPUT && keyboard) {
    return ui_keyboard_navigate_prev(keyboard);
  } else if (current_menu) {
    return ui_menu_navigate_prev(current_menu);
  }
  return false;
}

bool manual_input_page_execute_selected(void) {
  if (current_mode == MODE_KEYBOARD_INPUT && keyboard) {
    return ui_keyboard_press_selected(keyboard);
  } else if (current_menu) {
    return ui_menu_execute_selected(current_menu);
  }
  return false;
}

int manual_input_page_get_selected(void) {
  if (current_menu) {
    return ui_menu_get_selected(current_menu);
  }
  return -1;
}
