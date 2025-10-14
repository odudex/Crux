#include "key.h"
#include <esp_log.h>
#include <stdio.h>
#include <string.h>
#include <wally_bip39.h>
#include <wally_crypto.h>

static const char *TAG = "KEY";

// Key storage
static struct ext_key *master_key = NULL;
static unsigned char fingerprint[BIP32_KEY_FINGERPRINT_LEN];
static bool key_loaded = false;

bool key_init(void) {
    ESP_LOGI(TAG, "Initializing key management system");
    key_loaded = false;
    master_key = NULL;
    memset(fingerprint, 0, sizeof(fingerprint));
    return true;
}

bool key_is_loaded(void) {
    return key_loaded;
}

bool key_load_from_mnemonic(const char *mnemonic, const char *passphrase) {
    if (!mnemonic) {
        ESP_LOGE(TAG, "Invalid mnemonic");
        return false;
    }

    // Unload any existing key first
    if (key_loaded) {
        key_unload();
    }

    int ret;
    unsigned char seed[BIP39_SEED_LEN_512];

    // Validate mnemonic
    ret = bip39_mnemonic_validate(NULL, mnemonic);
    if (ret != WALLY_OK) {
        ESP_LOGE(TAG, "Invalid mnemonic phrase");
        return false;
    }

    // Convert mnemonic to seed
    ret = bip39_mnemonic_to_seed512(mnemonic, passphrase, seed, sizeof(seed));
    if (ret != WALLY_OK) {
        ESP_LOGE(TAG, "Failed to convert mnemonic to seed: %d", ret);
        memset(seed, 0, sizeof(seed));
        return false;
    }

    // Create master extended key from seed
    ret = bip32_key_from_seed_alloc(seed, sizeof(seed), BIP32_VER_MAIN_PRIVATE,
                                     0, &master_key);
    if (ret != WALLY_OK) {
        ESP_LOGE(TAG, "Failed to create master key from seed: %d", ret);
        memset(seed, 0, sizeof(seed));
        return false;
    }

    // Get the fingerprint
    ret = bip32_key_get_fingerprint(master_key, fingerprint,
                                     BIP32_KEY_FINGERPRINT_LEN);
    if (ret != WALLY_OK) {
        ESP_LOGE(TAG, "Failed to get key fingerprint: %d", ret);
        bip32_key_free(master_key);
        master_key = NULL;
        memset(seed, 0, sizeof(seed));
        return false;
    }

    // Clear sensitive data
    memset(seed, 0, sizeof(seed));

    key_loaded = true;
    ESP_LOGI(TAG, "Key loaded successfully");

    return true;
}

void key_unload(void) {
    if (master_key) {
        bip32_key_free(master_key);
        master_key = NULL;
    }
    memset(fingerprint, 0, sizeof(fingerprint));
    key_loaded = false;
    ESP_LOGI(TAG, "Key unloaded and cleared");
}

bool key_get_fingerprint(unsigned char *fingerprint_out) {
    if (!key_loaded || !fingerprint_out) {
        ESP_LOGE(TAG, "Cannot get fingerprint: %s",
                 !key_loaded ? "no key loaded" : "invalid output buffer");
        return false;
    }

    memcpy(fingerprint_out, fingerprint, BIP32_KEY_FINGERPRINT_LEN);
    return true;
}

bool key_get_fingerprint_hex(char *hex_out) {
    if (!key_loaded || !hex_out) {
        ESP_LOGE(TAG, "Cannot get fingerprint hex: %s",
                 !key_loaded ? "no key loaded" : "invalid output buffer");
        return false;
    }

    // Convert to hex manually (2 chars per byte + null terminator)
    for (int i = 0; i < BIP32_KEY_FINGERPRINT_LEN; i++) {
        sprintf(hex_out + (i * 2), "%02x", fingerprint[i]);
    }
    hex_out[BIP32_KEY_FINGERPRINT_LEN * 2] = '\0';

    return true;
}

// Helper function to parse BIP32 derivation path string
// Parses paths like "m/84'/0'/0'" into uint32_t array
static bool parse_derivation_path(const char *path, uint32_t *indices_out, size_t *depth_out, size_t max_depth) {
    if (!path || !indices_out || !depth_out) {
        return false;
    }

    // Check for "m/" prefix
    if (path[0] != 'm' || path[1] != '/') {
        ESP_LOGE(TAG, "Invalid path format: must start with 'm/'");
        return false;
    }

    const char *p = path + 2; // Skip "m/"
    size_t depth = 0;

    while (*p && depth < max_depth) {
        // Parse the numeric value
        uint32_t value = 0;
        bool has_digits = false;

        while (*p >= '0' && *p <= '9') {
            value = value * 10 + (*p - '0');
            p++;
            has_digits = true;
        }

        if (!has_digits) {
            ESP_LOGE(TAG, "Invalid path: expected number");
            return false;
        }

        // Check for hardened derivation marker (')
        if (*p == '\'') {
            value |= BIP32_INITIAL_HARDENED_CHILD;
            p++;
        }

        indices_out[depth++] = value;

        // Check for path separator or end
        if (*p == '/') {
            p++;
        } else if (*p == '\0') {
            break;
        } else {
            ESP_LOGE(TAG, "Invalid path: unexpected character '%c'", *p);
            return false;
        }
    }

    if (*p != '\0') {
        ESP_LOGE(TAG, "Path too long (max depth: %zu)", max_depth);
        return false;
    }

    *depth_out = depth;
    return true;
}

bool key_get_xpub(const char *path, char **xpub_out) {
    if (!key_loaded) {
        ESP_LOGE(TAG, "No key loaded");
        return false;
    }

    if (!path || !xpub_out) {
        ESP_LOGE(TAG, "Invalid parameters");
        return false;
    }

    // Parse the derivation path
    uint32_t path_indices[10]; // Support up to 10 levels deep
    size_t path_depth = 0;

    if (!parse_derivation_path(path, path_indices, &path_depth, 10)) {
        ESP_LOGE(TAG, "Failed to parse derivation path: %s", path);
        return false;
    }

    // Derive the key at the specified path
    struct ext_key *derived_key = NULL;
    int ret = bip32_key_from_parent_path_alloc(master_key, path_indices, path_depth,
                                                BIP32_FLAG_KEY_PRIVATE, &derived_key);

    if (ret != WALLY_OK) {
        ESP_LOGE(TAG, "Failed to derive key at path %s: %d", path, ret);
        return false;
    }

    // Export as xpub (public only)
    ret = bip32_key_to_base58(derived_key, BIP32_FLAG_KEY_PUBLIC, xpub_out);

    // Clean up derived key
    bip32_key_free(derived_key);

    if (ret != WALLY_OK) {
        ESP_LOGE(TAG, "Failed to export xpub: %d", ret);
        return false;
    }

    ESP_LOGI(TAG, "Exported xpub for path: %s", path);
    return true;
}

bool key_get_master_xpub(char **xpub_out) {
    if (!key_loaded) {
        ESP_LOGE(TAG, "No key loaded");
        return false;
    }

    if (!xpub_out) {
        ESP_LOGE(TAG, "Invalid output parameter");
        return false;
    }

    // Export master key as xpub (public only) using base58
    int ret = bip32_key_to_base58(master_key, BIP32_FLAG_KEY_PUBLIC, xpub_out);

    if (ret != WALLY_OK) {
        ESP_LOGE(TAG, "Failed to export xpub: %d", ret);
        return false;
    }

    ESP_LOGI(TAG, "Exported master xpub");
    return true;
}

void key_cleanup(void) {
    key_unload();
    ESP_LOGI(TAG, "Key management system cleaned up");
}
