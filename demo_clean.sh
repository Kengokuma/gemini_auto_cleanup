#!/bin/bash

# --- 設定 ---
PROTECTED_FILES="gemini_auto_cleanup.c demo_clean.sh demo_setup.sh"
PROTECTED_EXEC="a.out" 

# --- Cleanup Execution ---

echo "--- Cleanup started ---"
echo "Protected files: ${PROTECTED_FILES} ${PROTECTED_EXEC}"
echo "-----------------------------------"

find . -mindepth 1 -maxdepth 1 -print0 | while IFS= read -r -d $'\0' item; do
    BASENAME=$(basename "$item")
    IS_PROTECTED=0

    for file in $PROTECTED_FILES $PROTECTED_EXEC; do
        if [[ "$BASENAME" == "$file" ]]; then
            IS_PROTECTED=1
            break
        fi
    done

    if [[ $IS_PROTECTED -eq 0 ]]; then
        echo "delete: $item"
        rm -rf "$item"
    else
        echo "skip: $item (protected)"
    fi
done

echo "--- Cleanup completed ---"