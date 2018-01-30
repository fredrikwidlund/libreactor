#!/bin/bash

for file in reactor_util reactor_user reactor_core reactor_descriptor reactor_pool reactor_timer reactor_resolver \
                         reactor_stream reactor_tcp reactor_http
do
    test=`gcov -b src/reactor/libreactor_test_a-$file | grep -A4 File.*$file`
    echo "$test"
    echo "$test" | grep '% of' | grep '100.00%' >/dev/null || exit 1
    echo "$test" | grep '% of' | grep -v '100.00%' >/dev/null && exit 1
done

exit 0
