#!/usr/bin/env bash

mkdir -p test/mount
mkdir -p test/source
./fuse_xattrs -o nonempty test/source/ test/mount/

pushd test

set +e
python3 -m unittest -v
RESULT=$?
set -e

popd

fusermount -zu test/mount
rm -d test/source
rm -d test/mount

exit ${RESULT}
