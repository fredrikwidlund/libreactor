#!/bin/sh

if ! command -v valgrind 2>&1 >/dev/null; then
    exit 0
fi

for file in $(find src -type f -name '*.c' -exec basename {} \; | sed 's/\.c//g' | sort)
do
    echo [$file]
    if ! valgrind  --error-exitcode=1 --read-var-info=yes --leak-check=full --show-leak-kinds=all test/test_$file; then
        exit 1
    fi
done
