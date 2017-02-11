#!/bin/bash

if ! command -v valgrind; then
    exit 0;
fi

for file in src/reactor/*.c
do
    file=${file##*/}
    file=${file%.c}
    echo [$file]
    if ! valgrind  --error-exitcode=1 --read-var-info=yes --leak-check=full --show-leak-kinds=all test/$file; then
        exit 1
    fi
done
