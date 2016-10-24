#!/usr/bin/env bash

mkdir -p test/mount
./fuse_xattrs -o nonempty test/mount/

pushd test

set +e
python3 -m unittest -v
RESULT=$?
set -e

popd

fusermount -zu test/mount
rm -d test/mount

exit ${RESULT}
