#ifndef WALLET_H
#define WALLET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  WALLET_TYPE_NATIVE_SEGWIT = 0,
} wallet_type_t;

typedef enum {
  WALLET_NETWORK_MAINNET = 0,
  WALLET_NETWORK_TESTNET = 1,
} wallet_network_t;

bool wallet_init(wallet_network_t network);
bool wallet_is_initialized(void);
wallet_type_t wallet_get_type(void);
wallet_network_t wallet_get_network(void);
bool wallet_get_account_xpub(char **xpub_out);
bool wallet_get_receive_address(uint32_t index, char **address_out);
bool wallet_get_change_address(uint32_t index, char **address_out);
void wallet_cleanup(void);

#endif // WALLET_H
