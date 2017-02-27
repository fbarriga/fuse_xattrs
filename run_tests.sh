#!/usr/bin/env bash

mkdir -p test/mount
mkdir -p test/source
./fuse_xattrs -o nonempty test/source/ test/mount/

if [ $? -ne 0 ]; then
    echo "Error mounting the filesystem."
    echo "Do you have permissions?"
    exit 1
fi

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
