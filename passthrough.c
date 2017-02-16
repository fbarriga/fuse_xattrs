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

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "fuse_xattrs_config.h"
#include "utils.h"

int xmp_getattr(const char *path, struct stat *stbuf) {
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    res = lstat(_path, stbuf);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_access(const char *path, int mask) {
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    res = access(_path, mask);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_readlink(const char *path, char *buf, size_t size) {
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
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
    (void) fi;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    dp = opendir(_path);
    free(_path);

    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
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
    char *_path = prepend_source_directory(xattrs_config.source_dir, path);

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

    free(_path);
    if (res == -1)
        return -errno;

    return 0;
}

int xmp_mkdir(const char *path, mode_t mode) {
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    res = mkdir(_path, mode);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_unlink(const char *path) {
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    res = unlink(_path);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_rmdir(const char *path) {
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    res = rmdir(_path);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_symlink(const char *from, const char *to) {
    int res;

    char *_to = prepend_source_directory(xattrs_config.source_dir, to);
    res = symlink(from, _to);
    free(_to);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_rename(const char *from, const char *to) {
    int res;

    char *_from = prepend_source_directory(xattrs_config.source_dir, from);
    char *_to = prepend_source_directory(xattrs_config.source_dir, to);
    res = rename(_from, _to);
    free(_from);
    free(_to);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_link(const char *from, const char *to) {
    int res;

    char *_from = prepend_source_directory(xattrs_config.source_dir, from);
    char *_to = prepend_source_directory(xattrs_config.source_dir, to);
    res = link(_from, _to);
    free(_from);
    free(_to);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_chmod(const char *path, mode_t mode) {
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    res = chmod(_path, mode);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_chown(const char *path, uid_t uid, gid_t gid) {
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    res = lchown(_path, uid, gid);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_truncate(const char *path, off_t size) {
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    res = truncate(_path, size);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

#ifdef HAVE_UTIMENSAT
int xmp_utimens(const char *path, const struct timespec ts[2],
struct fuse_file_info *fi)
{
    (void) fi;
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    /* don't use utime/utimes since they follow symlinks */
    res = utimensat(0, _path, ts, AT_SYMLINK_NOFOLLOW);
    free(_path);
    if (res == -1)
        return -errno;

    return 0;
}
#endif

int xmp_open(const char *path, struct fuse_file_info *fi) {
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    res = open(_path, fi->flags);
    free(_path);

    if (res == -1)
        return -errno;

    close(res);
    return 0;
}

int xmp_read(const char *path, char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi) {
    int fd;
    int res;

    (void) fi;
    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    fd = open(_path, O_RDONLY);
    free(_path);

    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

int xmp_write(const char *path, const char *buf, size_t size,
              off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;

    (void) fi;
    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    fd = open(_path, O_WRONLY);
    free(_path);

    if (fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

int xmp_statfs(const char *path, struct statvfs *stbuf) {
    int res;

    char *_path = prepend_source_directory(xattrs_config.source_dir, path);
    res = statvfs(_path, stbuf);
    free(_path);

    if (res == -1)
        return -errno;

    return 0;
}

int xmp_release(const char *path, struct fuse_file_info *fi) {
    /* Just a stub.	 This method is optional and can safely be left
       unimplemented */

    (void) path;
    (void) fi;
    return 0;
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
    int fd;
    int res;

    (void) fi;

    if (mode)
        return -EOPNOTSUPP;

    char *_path = concat(xattrs_config.source_dir, path);
    fd = open(_path, O_WRONLY);
    free(_path);

    if (fd == -1)
        return -errno;

    res = -posix_fallocate(fd, offset, length);

    close(fd);
    return res;
}
#endif

