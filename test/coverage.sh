#!/bin/sh

for file in data pointer string list buffer vector hash map mapi maps utility core descriptor stream timer notify http net server
do
    echo [$file]
    test=`gcov -b src/reactor/libreactor_test_a-$file | grep -A4 File.*$file`
    echo "$test"
    echo "$test" | grep '% of' | grep '100.00%' >/dev/null || exit 1
    echo "$test" | grep '% of' | grep -v '100.00%' >/dev/null && exit 1
done
exit 0
