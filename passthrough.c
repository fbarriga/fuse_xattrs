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

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "xattrs_config.h"
#include "utils.h"

static int chown_new_file(const char *path, struct fuse_context *fc)
{
    return lchown(path, fc->uid, fc->gid);
}

int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;

    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);
    res = lstat(_path, stbuf);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_access(const char *path, int mask) {
    int res;
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);
    res = access(_path, mask);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_readlink(const char *path, char *buf, size_t size) {
    int res;
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);
    res = readlink(_path, buf, size - 1);
    free(_path);

    if (res == -1)
        return -errno;

    buf[res] = '\0';
    return 0;
}

int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;

    (void) offset;

    if (fi != NULL && fi->fh != 0) {
        dp = fdopendir(fi->fh);
    } else {
        char *_path = prepend_source_directory(path);
        dp = opendir(_path);
        free(_path);
    }

    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(de->d_name) == 1) {
            continue;
        }

        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

int xmp_mknod(const char *path, mode_t mode, dev_t rdev) {
    int res;
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);

    /* On Linux this could just be 'mknod(path, mode, rdev)' but this
       is more portable */
    if (S_ISREG(mode)) {
        res = open(_path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            res = close(res);
    } else if (S_ISFIFO(mode))
        res = mkfifo(_path, mode);
    else
        res = mknod(_path, mode, rdev);

    if (res == -1) {
        free(_path);
        return -errno;
    }

    struct fuse_context *fc = fuse_get_context();
    res = chown_new_file(_path, fc);

    free(_path);
    return res;
}

int xmp_mkdir(const char *path, mode_t mode) {
    int res;
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);
    res = mkdir(_path, mode);

    if (res == -1) {
        free(_path);
        return -errno;
    }

    struct fuse_context *fc = fuse_get_context();
    res = chown_new_file(_path, fc);

    free(_path);
    return res;
}

int xmp_unlink(const char *path) {
    int res;
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);
    res = unlink(_path);

    if (res == -1) {
        free(_path);
        return -errno;
    }

    char *sidecar_path = get_sidecar_path(_path);
    if (is_regular_file(sidecar_path)) {
        if (unlink(sidecar_path) == -1) {
            error_print("Error removing sidecar file: %s\n", sidecar_path);
        }
    }
    free(sidecar_path);
    free(_path);

    return 0;
}

// FIXME: remove sidecar
int xmp_rmdir(const char *path) {
    int res;
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);
    res = rmdir(_path);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_symlink(const char *from, const char *to) {
    int res;
    if (xattrs_config.show_sidecar == 0) {
        if (filename_is_sidecar(from) == 1 || filename_is_sidecar(to)) {
            return -ENOENT;
        }
    }

    char *_to = prepend_source_directory(to);
    res = symlink(from, _to);

    if (res == -1) {
        free(_to);
        return -errno;
    }

    struct fuse_context *fc = fuse_get_context();
    res = chown_new_file(_to, fc);

    free(_to);
    return res;
}

int xmp_rename(const char *from, const char *to) {
    int res;
    if (xattrs_config.show_sidecar == 0) {
        if (filename_is_sidecar(from) == 1 || filename_is_sidecar(to)) {
            return -ENOENT;
        }
    }

    char *_from = prepend_source_directory(from);
    char *_to = prepend_source_directory(to);
    res = rename(_from, _to);

    if (res == -1) {
        free(_from);
        free(_to);
        return -errno;
    }

    char *from_sidecar_path = get_sidecar_path(_from);
    char *to_sidecar_path = get_sidecar_path(_to);

    // FIXME: Remove to_sidecar_path if it exists ?
    if (is_regular_file(from_sidecar_path)) {
        if (rename(from_sidecar_path, to_sidecar_path) == -1) {
            error_print("Error renaming sidecar. from: %s to: %s\n", from_sidecar_path, to_sidecar_path);
        }
    }
    free(from_sidecar_path);
    free(to_sidecar_path);

    free(_from);
    free(_to);

    return 0;
}

// TODO: handle sidecar file ?
int xmp_link(const char *from, const char *to) {
    int res;
    if (xattrs_config.show_sidecar == 0) {
        if (filename_is_sidecar(from) == 1 || filename_is_sidecar(to)) {
            return -ENOENT;
        }
    }

    char *_from = prepend_source_directory(from);
    char *_to = prepend_source_directory(to);
    res = link(_from, _to);
    free(_from);
    free(_to);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_chmod(const char *path, mode_t mode) {
    int res;
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);
    res = chmod(_path, mode);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_chown(const char *path, uid_t uid, gid_t gid) {
    int res;
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);
    res = lchown(_path, uid, gid);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_truncate(const char *path, off_t size) {
    int res;
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);
    res = truncate(_path, size);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

#ifdef HAS_UTIMENSAT
int xmp_utimens(const char *path, const struct timespec ts[2],
struct fuse_file_info *fi)
{
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    (void) fi;
    int res;

    char *_path = prepend_source_directory(path);
    /* don't use utime/utimes since they follow symlinks */
    res = utimensat(0, _path, ts, AT_SYMLINK_NOFOLLOW);
    free(_path);
    if (res == -1)
        return -errno;

    return 0;
}
#endif

int xmp_open(const char *path, struct fuse_file_info *fi) {
    int fd;
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);
    fd = open(_path, fi->flags);
    free(_path);

    if (fd == -1)
        return -errno;

    fi->fh = fd;
    return 0;
}

int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    int res;

    char *_path = prepend_source_directory(path);
    int fd = open(_path, fi->flags, mode & 0777);
    if (fd == -1) {
        free(_path);
        return -errno;
    }

    struct fuse_context *fc = fuse_get_context();
    res = chown_new_file(_path, fc);

    fi->fh = fd;

    free(_path);
    return res;
}

int xmp_read(const char *path, char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi)
{
    (void) path;
    if (fi == NULL || fi->fh == 0) {
        return -1;
    }

    int res = pread(fi->fh, buf, size, offset);
    if (res == -1)
        res = -errno;

    return res;
}

int xmp_write(const char *path, const char *buf, size_t size,
              off_t offset, struct fuse_file_info *fi)
{
    (void) path;
    if (fi == NULL || fi->fh == 0) {
        return -1;
    }

    int res = pwrite(fi->fh, buf, size, offset);
    if (res == -1)
        res = -errno;

    return res;
}

int xmp_statfs(const char *path, struct statvfs *stbuf) {
    int res;
    if (xattrs_config.show_sidecar == 0 && filename_is_sidecar(path) == 1)  {
        return -ENOENT;
    }

    char *_path = prepend_source_directory(path);
    res = statvfs(_path, stbuf);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_release(const char *path, struct fuse_file_info *fi) {
    (void) path;
    return close(fi->fh);
}

int xmp_fsync(const char *path, int isdatasync,
              struct fuse_file_info *fi) {
    /* Just a stub.	 This method is optional and can safely be left
       unimplemented */

    (void) path;
    (void) isdatasync;
    (void) fi;
    return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
int xmp_fallocate(const char *path, int mode,
                  off_t offset, off_t length, struct fuse_file_info *fi)
{
    (void) path;
    if (fi == NULL || fi->fh == 0) {
        return -1;
    }

    int res;
    if (mode)
        return -EOPNOTSUPP;

    res = -posix_fallocate(fi->fh, offset, length);
    return res;
}
#endif
