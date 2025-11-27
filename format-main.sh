#!/bin/bash

# Script to run clang-format on all .c and .h files in the main folder

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MAIN_DIR="$SCRIPT_DIR/main"

if [ ! -d "$MAIN_DIR" ]; then
    echo "Error: main directory not found at $MAIN_DIR"
    exit 1
fi

echo "Running clang-format on all .c and .h files in main folder..."

# Find and format all .c files
find "$MAIN_DIR" -type f -name "*.c" -not -path "*/build/*" -exec clang-format -i {} \;

# Find and format all .h files
find "$MAIN_DIR" -type f -name "*.h" -not -path "*/build/*" -exec clang-format -i {} \;

echo "Formatting complete!"
