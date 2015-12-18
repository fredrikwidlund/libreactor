#!/bin/sh

for file in reactor_core reactor_user reactor_desc reactor_timer reactor_signal reactor_stream
do
    echo [$file]
    test=`gcov -b src/reactor_core/libreactor_core_test_a-$file | grep -A4 File.*$file`
    echo "$test"
    echo "$test" | grep '% of' | grep '100.00%' >/dev/null || exit 1
    echo "$test" | grep '% of' | grep -v '100.00%' >/dev/null && exit 1
done
exit 0
