/*
  fuse_xattrs - Add xattrs support using sidecar files

  Copyright (C) 2016-2017  Felipe Barriga Richards <felipe {at} felipebarriga.cl>

  Based on passthrough.c (libfuse example)

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#define FUSE_USE_VERSION 30

/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700

/* For get_current_dir_name */
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>

#include <fuse.h>
#include <sys/xattr.h>

#include "fuse_xattrs_config.h"

#include "xattrs_config.h"
#include "utils.h"
#include "passthrough.h"

#include "binary_storage.h"

static int xmp_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

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

    char *_path = prepend_source_directory(path);

#ifdef DEBUG
    char *sanitized_value = sanitize_value(value, size);
    debug_print("path=%s name=%s value=%s size=%zu XATTR_CREATE=%d XATTR_REPLACE=%d\n",
                _path, name, sanitized_value, size, flags & XATTR_CREATE, flags & XATTR_REPLACE);

    free(sanitized_value);
#endif

    int rtval = binary_storage_write_key(_path, name, value, size, flags);
    free(_path);

    return rtval;
}

static int xmp_getxattr(const char *path, const char *name, char *value, size_t size)
{
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    if (get_namespace(name) != USER) {
        debug_print("Only user namespace is supported. name=%s\n", name);
        return -ENOTSUP;
    }
    if (strlen(name) > XATTR_NAME_MAX) {
        debug_print("attribute name must be equal or smaller than %d bytes\n", XATTR_NAME_MAX);
        return -ERANGE;
    }

    char *_path = prepend_source_directory(path);
    debug_print("path=%s name=%s size=%zu\n", _path, name, size);
    int rtval = binary_storage_read_key(_path, name, value, size);
    free(_path);

    return rtval;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    if (size > XATTR_LIST_MAX) {
        debug_print("The size of the list of attribute names for this file exceeds the system-imposed limit.\n");
        return -E2BIG;
    }

    char *_path = prepend_source_directory(path);
    debug_print("path=%s size=%zu\n", _path, size);
    int rtval = binary_storage_list_keys(_path, list, size);
    free(_path);

    return rtval;
}

static int xmp_removexattr(const char *path, const char *name)
{
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    if (get_namespace(name) != USER) {
        debug_print("Only user namespace is supported. name=%s\n", name);
        return -ENOTSUP;
    }
    if (strlen(name) > XATTR_NAME_MAX) {
        debug_print("attribute name must be equal or smaller than %d bytes\n", XATTR_NAME_MAX);
        return -ERANGE;
    }

    char *_path = prepend_source_directory(path);
    debug_print("path=%s name=%s\n", _path, name);
    int rtval = binary_storage_remove_key(_path, name);
    free(_path);

    return rtval;
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
#ifdef HAS_UTIMENSAT
        .utimens     = xmp_utimens,
#endif
        .open        = xmp_open,
        .create      = xmp_create,
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

/**
 * Check if the path is valid. If it's a relative path,
 * prepend the working path.
 * @param path relative or absolute path to eval.
 * @return new string with absolute path
 */
const char *sanitized_source_directory(const char *path) {
    char *absolute_path;
    if (strlen(path) == 0) {
        return NULL;
    }

    /* absolute path, we don't do anything */
    if (path[0] == '/') {
        if (is_directory(path) == -1) {
            return NULL;
        }
        absolute_path = strdup(path);
        return absolute_path;
    }

    char *pwd = get_current_dir_name();
    size_t len = strlen(pwd) + 1 + strlen(path) + 1;
    int has_trailing_backslash = (path[strlen(path)-1] == '/');
    if (!has_trailing_backslash)
        len++;

    absolute_path = (char*) malloc(sizeof(char) * len);
    memset(absolute_path, '\0', len);
    sprintf(absolute_path, "%s/%s", pwd, path);

    if(!has_trailing_backslash)
        absolute_path[len-2] = '/';

    if (is_directory(absolute_path) == -1) {
        free(absolute_path);
        return NULL;
    }

    return absolute_path;
}

enum {
    KEY_HELP,
    KEY_VERSION,
};

#define FUSE_XATTRS_OPT(t, p, v) { t, offsetof(struct xattrs_config, p), v }

static struct fuse_opt xattrs_opts[] = {
        FUSE_XATTRS_OPT("show_sidecar",    show_sidecar, 1),

        FUSE_OPT_KEY("-V",                 KEY_VERSION),
        FUSE_OPT_KEY("--version",          KEY_VERSION),
        FUSE_OPT_KEY("-h",                 KEY_HELP),
        FUSE_OPT_KEY("--help",             KEY_HELP),
        FUSE_OPT_END
};

static int xattrs_opt_proc(void *data, const char *arg, int key,
                           struct fuse_args *outargs) {
    (void) data;
    switch (key) {
        case FUSE_OPT_KEY_NONOPT:
            if (!xattrs_config.source_dir) {
                xattrs_config.source_dir = sanitized_source_directory(arg);
                xattrs_config.source_dir_size = strlen(xattrs_config.source_dir);
                return 0;
            }
            break;

        case KEY_HELP:
            fprintf(stderr,
                    "usage: %s source_dir mountpoint [options]\n"
                            "\n"
                            "general options:\n"
                            "    -o opt,[opt...]  mount options\n"
                            "    -h   --help      print help\n"
                            "    -V   --version   print version\n"
                            "\n"
                            "FUSE XATTRS options:\n"
                            "    -o show_sidecar  don't hide sidecar files\n"
                            "\n", outargs->argv[0]);

            fuse_opt_add_arg(outargs, "-ho");
            fuse_main(outargs->argc, outargs->argv, &xmp_oper, NULL);
            exit(1);

        case KEY_VERSION:
            printf("FUSE_XATTRS version %d.%d\n", FUSE_XATTRS_VERSION_MAJOR, FUSE_XATTRS_VERSION_MINOR);
            fuse_opt_add_arg(outargs, "--version");
            fuse_main(outargs->argc, outargs->argv, &xmp_oper, NULL);
            exit(0);
    }
    return 1;
}



int main(int argc, char *argv[]) {
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, &xattrs_config, xattrs_opts, xattrs_opt_proc) == -1) {
        exit(1);
    }

    if (!xattrs_config.source_dir) {
        fprintf(stderr, "missing source directory\n");
        fprintf(stderr, "see `%s -h' for usage\n", argv[0]);
        exit(1);
    }

    umask(0);

    // disable multi-threading
    fuse_opt_add_arg(&args, "-s");
    return fuse_main(args.argc, args.argv, &xmp_oper, NULL);
}
