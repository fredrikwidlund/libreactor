#!/bin/sh

command -v valgrind || exit 0

for file in data pointer string list buffer vector hash map mapi maps utility core descriptor stream timer notify http net server
do
    echo [$file]
    if ! valgrind --track-fds=yes --error-exitcode=1 --read-var-info=yes --leak-check=full --show-leak-kinds=all test/$file
    then
        exit 1
    fi
done
