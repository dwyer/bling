#!/bin/sh -e
find .nora_test/*/valid -name \*.c | while read i; do
    echo building $i
    ./bazel-bin/cmd/blingc/blingc -c $i >/dev/null #2>/dev/null
done
