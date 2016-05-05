#!/bin/sh

if command -v valgrind; then
    for file in reactor_core reactor_user reactor_desc reactor_timer reactor_signal reactor_stream
    do
        echo [$file]
        if ! valgrind --error-exitcode=1 --track-fds=yes \
             --read-var-info=yes --leak-check=full --show-leak-kinds=all test/$file
        then
            exit 1
        fi
    done
fi 
