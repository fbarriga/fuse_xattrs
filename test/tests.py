#!/usr/bin/env python3


# fuse_xattrs - Add xattrs support using sidecar files
#
# Copyright (C) 2016  Felipe Barriga Richards <felipe {at} felipebarriga.cl>
#
# This program can be distributed under the terms of the GNU GPL.
# See the file COPYING.


import unittest
import xattr
from pathlib import Path
import os

if xattr.__version__ != '0.9.1':
    print("WARNING, only tested with xattr version 0.9.1")

# TODO
# - listxattr: list too long
# - sidecar file permissions
# - corrupt metadata files


class TestXAttrs(unittest.TestCase):
    def setUp(self):
        self.sourceDir = "./source/"
        self.mountDir = "./mount/"
        self.randomFilename = "foo.txt"

        self.randomFile = self.mountDir + self.randomFilename
        self.randomFileSidecar = self.randomFile + ".xattr"

        self.randomSourceFile = self.sourceDir + self.randomFilename
        self.randomSourceFileSidecar = self.randomSourceFile + ".xattr"

        if os.path.isfile(self.randomFile):
            os.remove(self.randomFile)

        if os.path.isfile(self.randomFileSidecar):
            os.remove(self.randomFileSidecar)

        Path(self.randomFile).touch()
        self.assertTrue(os.path.isfile(self.randomFile))
        self.assertFalse(os.path.isfile(self.randomFileSidecar))

    def tearDown(self):
        if os.path.isfile(self.randomFile):
            os.remove(self.randomFile)

        if os.path.isfile(self.randomFileSidecar):
            os.remove(self.randomFileSidecar)

    def test_xattr_set(self):
        xattr.setxattr(self.randomFile, "user.foo", bytes("bar", "utf-8"))

    def test_xattr_set_name_max_length(self):
        enc = "utf-8"
        key = "user." + "x" * 250
        value = "x"
        self.assertEqual(len(key), 255)
        xattr.setxattr(self.randomFile, key, bytes(value, enc))

        key = "user." + "x" * 251
        self.assertEqual(len(key), 256)
        with self.assertRaises(OSError) as ex:
            xattr.setxattr(self.randomFile, key, bytes(value, enc))
        self.assertEqual(ex.exception.errno, 34)
        self.assertEqual(ex.exception.strerror, "Numerical result out of range")

    @unittest.expectedFailure
    def test_xattr_set_value_max_size(self):
        enc = "utf-8"
        key = "user.foo"
        value = "x" * (64 * 1024 + 1)  # we want max 64KiB of data
        with self.assertRaises(OSError) as ex:
            xattr.setxattr(self.randomFile, key, bytes(value, enc))

        # on btrfs we get "no space left on device"
        self.assertEqual(ex.exception.errno, 28)
        self.assertEqual(ex.exception.strerror, "No space left on device")

        # on fuse_xattr we get "Argument list too long"
        # the error is thrown by fuse, not by fuse_xattr code

    def test_xattr_set_namespaces(self):
        with self.assertRaises(OSError) as ex:
            xattr.setxattr(self.randomFile, "system.foo", bytes("bar", "utf-8"))
        self.assertEqual(ex.exception.errno, 95)
        self.assertEqual(ex.exception.strerror, "Operation not supported")

        with self.assertRaises(OSError) as ex:
            xattr.setxattr(self.randomFile, "trust.foo", bytes("bar", "utf-8"))
        self.assertEqual(ex.exception.errno, 95)
        self.assertEqual(ex.exception.strerror, "Operation not supported")

        with self.assertRaises(OSError) as ex:
            xattr.setxattr(self.randomFile, "foo.foo", bytes("bar", "utf-8"))
        self.assertEqual(ex.exception.errno, 95)
        self.assertEqual(ex.exception.strerror, "Operation not supported")

        with self.assertRaises(PermissionError) as ex:
            xattr.setxattr(self.randomFile, "security.foo", bytes("bar", "utf-8"))
        self.assertEqual(ex.exception.errno, 1)
        self.assertEqual(ex.exception.strerror, "Operation not permitted")

    def test_xattr_get_non_existent(self):
        key = "user.foo"
        with self.assertRaises(OSError) as ex:
            xattr.getxattr(self.randomFile, key)
        self.assertEqual(ex.exception.errno, 61)
        self.assertEqual(ex.exception.strerror, "No data available")

    def test_xattr_get(self):
        enc = "utf-8"
        key = "user.foo"
        value = "bar"
        xattr.setxattr(self.randomFile, key, bytes(value, enc))
        read_value = xattr.getxattr(self.randomFile, key)
        self.assertEqual(value, read_value.decode(enc))

    def test_xattr_set_empty(self):
        enc = "utf-8"
        key = "user.foo"
        value = ""
        xattr.setxattr(self.randomFile, key, bytes(value, enc))
        read_value = xattr.getxattr(self.randomFile, key)
        self.assertEqual(value, read_value.decode(enc))

    def test_xattr_set_override(self):
        enc = "utf-8"
        key = "user.foo"
        value1 = "bar"
        value2 = "rab"
        xattr.setxattr(self.randomFile, key, bytes(value1, enc))
        xattr.setxattr(self.randomFile, key, bytes(value2, enc))
        read_value = xattr.getxattr(self.randomFile, key)
        self.assertEqual(value2, read_value.decode(enc))

    def test_xattr_set_create(self):
        enc = "utf-8"
        key = "user.foo"
        value1 = "bar"
        value2 = "rab"
        xattr.setxattr(self.randomFile, key, bytes(value1, enc), xattr.XATTR_CREATE)
        with self.assertRaises(FileExistsError) as ex:
            xattr.setxattr(self.randomFile, key, bytes(value2, enc), xattr.XATTR_CREATE)
        self.assertEqual(ex.exception.errno, 17)
        self.assertEqual(ex.exception.strerror, "File exists")

        read_value = xattr.getxattr(self.randomFile, key)
        self.assertEqual(value1, read_value.decode(enc))

    def test_xattr_set_replace(self):
        enc = "utf-8"
        key = "user.foo"
        value = "bar"
        with self.assertRaises(OSError) as ex:
            xattr.setxattr(self.randomFile, key, bytes(value, enc), xattr.XATTR_REPLACE)
        self.assertEqual(ex.exception.errno, 61)
        self.assertEqual(ex.exception.strerror, "No data available")

        with self.assertRaises(OSError) as ex:
            xattr.getxattr(self.randomFile, key)
        self.assertEqual(ex.exception.errno, 61)
        self.assertEqual(ex.exception.strerror, "No data available")

    def test_xattr_list_empty(self):
        attrs = xattr.listxattr(self.randomFile)
        self.assertEqual(len(attrs), 0)

    def test_xattr_list(self):
        enc = "utf-8"
        key1 = "user.foo"
        key2 = "user.foo2"
        key3 = "user.foo3"
        value = "bar"

        # set 3 keys
        xattr.setxattr(self.randomFile, key1, bytes(value, enc))
        xattr.setxattr(self.randomFile, key2, bytes(value, enc))
        xattr.setxattr(self.randomFile, key3, bytes(value, enc))

        # get and check the list
        attrs = xattr.listxattr(self.randomFile)

        self.assertEqual(len(attrs), 3)
        self.assertTrue(key1 in attrs)
        self.assertTrue(key2 in attrs)
        self.assertTrue(key3 in attrs)

    def test_xattr_unicode(self):
        enc = "utf-8"
        key = "user.fooｼüßてЙĘ𝄠✠"
        value = "bar"

        # set
        xattr.setxattr(self.randomFile, key, bytes(value, enc))

        # list
        attrs = xattr.listxattr(self.randomFile)
        self.assertEqual(len(attrs), 1)
        self.assertEqual(attrs[0], key)

        # read
        read_value = xattr.getxattr(self.randomFile, key)
        self.assertEqual(value, read_value.decode(enc))

        # remove
        xattr.removexattr(self.randomFile, key)

    def test_xattr_remove(self):
        enc = "utf-8"
        key = "user.foo"
        value = "bar"

        # set
        xattr.setxattr(self.randomFile, key, bytes(value, enc))

        # remove
        xattr.removexattr(self.randomFile, key)

        # list should be empty
        attrs = xattr.listxattr(self.randomFile)
        self.assertEqual(len(attrs), 0)

        # should fail when trying to read it
        with self.assertRaises(OSError) as ex:
            xattr.getxattr(self.randomFile, key)
        self.assertEqual(ex.exception.errno, 61)
        self.assertEqual(ex.exception.strerror, "No data available")

        # removing twice should fail
        with self.assertRaises(OSError) as ex:
            xattr.getxattr(self.randomFile, key)
        self.assertEqual(ex.exception.errno, 61)
        self.assertEqual(ex.exception.strerror, "No data available")

    def test_hide_sidecar(self):
        xattr.setxattr(self.randomFile, "user.foo", bytes("bar", "utf-8"))
        self.assertTrue(os.path.isfile(self.randomFile))
        self.assertFalse(os.path.isfile(self.randomFileSidecar))

        sidecarFilename = self.randomFilename + ".xattr"
        files_mount = os.listdir(self.mountDir)
        self.assertTrue(self.randomFilename in files_mount)
        self.assertTrue(sidecarFilename not in files_mount)

        files_source = os.listdir(self.sourceDir)
        self.assertTrue(self.randomFilename in files_source)
        self.assertTrue(sidecarFilename in files_source)

    def test_create_new_file(self):
        test_filename = "test_create_new_file"
        self.assertFalse(os.path.isfile(self.sourceDir + test_filename))
        self.assertFalse(os.path.isfile(self.mountDir + test_filename))

        open(self.mountDir + test_filename, "a").close()
        self.assertTrue(os.path.isfile(self.sourceDir + test_filename))
        self.assertTrue(os.path.isfile(self.mountDir + test_filename))
        # FIXME: if one assert fails, the file isn't going to be deleted
        os.remove(self.mountDir + test_filename)

    def test_remove_file_with_sidecar(self):
        xattr.setxattr(self.randomFile, "user.foo", bytes("bar", "utf-8"))
        self.assertTrue(os.path.isfile(self.randomFile))
        self.assertTrue(os.path.isfile(self.randomSourceFile))
        self.assertTrue(os.path.isfile(self.randomSourceFileSidecar))

        os.remove(self.randomFile)
        self.assertFalse(os.path.isfile(self.randomFile))
        self.assertFalse(os.path.isfile(self.randomSourceFile))
        self.assertFalse(os.path.isfile(self.randomSourceFileSidecar))

    def test_remove_file_without_sidecar(self):
        self.assertTrue(os.path.isfile(self.randomFile))
        self.assertTrue(os.path.isfile(self.randomSourceFile))
        self.assertFalse(os.path.isfile(self.randomSourceFileSidecar))

        os.remove(self.randomFile)
        self.assertFalse(os.path.isfile(self.randomFile))
        self.assertFalse(os.path.isfile(self.randomSourceFile))
        self.assertFalse(os.path.isfile(self.randomSourceFileSidecar))

if __name__ == '__main__':
    unittest.main()
