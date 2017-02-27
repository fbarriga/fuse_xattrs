[![Build Status](https://travis-ci.org/fbarriga/fuse_xattrs.svg?branch=master)](https://travis-ci.org/fbarriga/fuse_xattrs)

## Abstract

This filesystem provides xattr support using sidecar files.

## Latest version

The latest version and more information can be found on
http://github.com/fbarriga/fuse_xattrs


## How to mount a filesystem

Once fuse_xattrs is installed (see next section) running it is very simple:

    fuse_xattrs source_directory mountpoint

To unmount the filesystem:

    fusermount -u mountpoint

## Distribution packages

Archlinux (https://aur.archlinux.org/packages/fuse_xattrs/):

    yaourt -S fuse_xattrs


## Building

First you need to download FUSE 2.9 or later from
http://github.com/libfuse/libfuse.

    mkdir build && cd build
    cmake ..
    make

## Code Coverage

    mkdir build && cd build
    cmake -DENABLE_CODECOVERAGE=1 -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_BUILD_TYPE=Debug ..
    make
    make fuse_xattrs_coverage

## Installing

    make install

## Links

- http://man7.org/linux/man-pages/man2/setxattr.2.html
- http://man7.org/linux/man-pages/man2/listxattr.2.html
- http://man7.org/linux/man-pages/man2/getxattr.2.html
- http://man7.org/linux/man-pages/man3/errno.3.html
- https://www.freedesktop.org/wiki/CommonExtendedAttributes/
