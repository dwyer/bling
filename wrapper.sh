#!/bin/sh
bin=${1%.*}.out
./bazel-bin/cmd/blingc/blingc -o $bin $1
if [ $? -eq 0 ]; then
    mv $bin ${bin%.*}
fi
rm -f $ctmp
