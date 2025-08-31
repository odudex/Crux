/*
 * Memory Management Utilities
 * Common memory management macros and functions
 */

#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <stdlib.h>
#include <string.h>

/**
 * Safely free a static/global pointer variable and set it to NULL
 * Use this for static variables where the address is never NULL
 *
 * @param ptr The pointer variable itself (not its address)
 */
#define SAFE_FREE_STATIC(ptr)                                                  \
  do {                                                                         \
    if (ptr) {                                                                 \
      free(ptr);                                                               \
      ptr = NULL;                                                              \
    }                                                                          \
  } while (0)

/**
 * Safely duplicate a string (like strdup but handles NULL input)
 * Returns NULL if input is NULL or allocation fails
 *
 * @param str The string to duplicate (can be NULL)
 * @return Newly allocated copy of the string, or NULL
 */
#define SAFE_STRDUP(str)                                                       \
  ((str) ? strcpy((char *)malloc(strlen(str) + 1), (str)) : NULL)

#endif // MEMORY_UTILS_H