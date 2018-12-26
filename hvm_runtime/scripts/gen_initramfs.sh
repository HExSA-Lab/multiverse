#!/bin/sh
orig=$PWD

echo "Generating $2"

pushd $1
find . -print0 | cpio --null -ov --format=newc | gzip -9 > $orig/$2
popd
