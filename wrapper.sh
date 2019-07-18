#!/bin/sh
ctmp=$(mktemp -u).c
otmp=$(mktemp -u).out
./bazel-bin/cmd/blingc/blingc -o $ctmp $1
cc -o $otmp $ctmp
cp $otmp ${1%.*}
rm -f $ctmp
