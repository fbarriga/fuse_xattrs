/*
  fuse_xattrs - Add xattrs support using sidecar files

  Copyright (C) 2016  Felipe Barriga Richards <felipe {at} felipebarriga.cl>

  Based on passthrough.c (libfuse example)

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#define FUSE_USE_VERSION 30

/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <fuse.h>
#include <sys/xattr.h>

#include "utils.h"
#include "passthrough.h"

#include "binary_storage.h"
#include "const.h"

static int xmp_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    if (get_namespace(name) != USER) {
        debug_print("Only user namespace is supported. name=%s\n", name);
        return -ENOTSUP;
    }
    if (strlen(name) > XATTR_NAME_MAX) {
        debug_print("attribute name must be equal or smaller than %d bytes\n", XATTR_NAME_MAX);
        return -ERANGE;
    }
    if (size > XATTR_SIZE_MAX) {
        debug_print("attribute value cannot be bigger than %d bytes\n", XATTR_SIZE_MAX);
        return -ENOSPC;
    }

#ifdef DEBUG
    char *sanitized_value = sanitize_value(value, size);
    debug_print("path=%s name=%s value=%s size=%zu XATTR_CREATE=%d XATTR_REPLACE=%d\n",
                path, name, sanitized_value, size, flags & XATTR_CREATE, flags & XATTR_REPLACE);

    free(sanitized_value);
#endif
     return binary_storage_write_key(path, name, value, size, flags);
}

static int xmp_getxattr(const char *path, const char *name, char *value, size_t size)
{
    if (get_namespace(name) != USER) {
        debug_print("Only user namespace is supported. name=%s\n", name);
        return -ENOTSUP;
    }
    if (strlen(name) > XATTR_NAME_MAX) {
        debug_print("attribute name must be equal or smaller than %d bytes\n", XATTR_NAME_MAX);
        return -ERANGE;
    }

    debug_print("path=%s name=%s size=%zu\n", path, name, size);
    return binary_storage_read_key(path, name, value, size);
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
    if (size > XATTR_LIST_MAX) {
        debug_print("The size of the list of attribute names for this file exceeds the system-imposed limit.\n");
        return -E2BIG;
    }

    debug_print("path=%s size=%zu\n", path, size);
    return binary_storage_list_keys(path, list, size);
}

static int xmp_removexattr(const char *path, const char *name)
{
    if (get_namespace(name) != USER) {
        debug_print("Only user namespace is supported. name=%s\n", name);
        return -ENOTSUP;
    }
    if (strlen(name) > XATTR_NAME_MAX) {
        debug_print("attribute name must be equal or smaller than %d bytes\n", XATTR_NAME_MAX);
        return -ERANGE;
    }

    debug_print("path=%s name=%s\n", path, name);
    return binary_storage_remove_key(path, name);
}

static struct fuse_operations xmp_oper = {
        .getattr     = xmp_getattr,
        .access      = xmp_access,
        .readlink    = xmp_readlink,
        .readdir     = xmp_readdir,
        .mknod       = xmp_mknod,
        .mkdir       = xmp_mkdir,
        .symlink     = xmp_symlink,
        .unlink      = xmp_unlink,
        .rmdir       = xmp_rmdir,
        .rename      = xmp_rename,
        .link        = xmp_link,
        .chmod       = xmp_chmod,
        .chown       = xmp_chown,
        .truncate    = xmp_truncate,
#ifdef HAVE_UTIMENSAT
        .utimens     = xmp_utimens,
#endif
        .open        = xmp_open,
        .read        = xmp_read,
        .write       = xmp_write,
        .statfs      = xmp_statfs,
        .release     = xmp_release,
        .fsync       = xmp_fsync,
#ifdef HAVE_POSIX_FALLOCATE
        .fallocate   = xmp_fallocate,
#endif
        .setxattr    = xmp_setxattr,
        .getxattr    = xmp_getxattr,
        .listxattr   = xmp_listxattr,
        .removexattr = xmp_removexattr,
};

int main(int argc, char *argv[])
{
    // TODO: parse options...
    umask(0);
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
