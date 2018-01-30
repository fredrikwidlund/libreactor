#!/bin/bash

if ! command -v valgrind; then
    exit 0;
fi

for file in reactor_util reactor_user reactor_descriptor reactor_timer reactor_stream reactor_http
do
    echo [$file]
    if ! valgrind --error-exitcode=1 --read-var-info=yes --leak-check=full --show-leak-kinds=all test/$file; then
        exit 1
    fi
done
