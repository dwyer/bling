#!/bin/sh
ctmp=$(mktemp -u).c
otmp=$(mktemp -u).out
./bazel-bin/cmd/blingc/blingc -w -o $ctmp $1
if [ $? -eq 0 ]; then
    cc -o $otmp $ctmp
    mv $otmp ${1%.*}
fi
rm -f $ctmp
