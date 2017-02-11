#!/bin/bash

for file in src/reactor/*.c
do
    file=${file##*/}
    file=${file%.c}
    test=`gcov -b src/reactor/libreactor_test_a-$file | grep -A4 File.*$file`
    echo "$test"
    echo "$test" | grep '% of' | grep '100.00%' >/dev/null || exit 1
    echo "$test" | grep '% of' | grep -v '100.00%' >/dev/null && exit 1
done

exit 0
