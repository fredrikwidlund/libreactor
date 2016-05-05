#!/bin/sh

sources=$(find src -name '*.c')
for file in $sources
do
    dir=$(dirname $file)
    name=$(basename $file)
    name=${name%%.c}
    echo [$name]
    test=`gcov -b src/reactor_core/libreactor_core_test_a-$name | grep -A4 File.*$name`
    echo "$test"
    echo "$test" | grep '% of' | grep '100.00%' >/dev/null || exit 1
    echo "$test" | grep '% of' | grep -v '100.00%' >/dev/null && exit 1
done
exit 0
