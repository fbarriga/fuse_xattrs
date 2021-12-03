/*
  fuse_xattrs - Add xattrs support using sidecar files

  Copyright (C) 2016  Felipe Barriga Richards <felipe {at} felipebarriga.cl>

  Based on passthrough.c (libfuse example)

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#include <fuse.h>

int xmp_getattr(const char *path, struct stat *stbuf);
int xmp_access(const char *path, int mask);
int xmp_readlink(const char *path, char *buf, size_t size);
int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int xmp_mknod(const char *path, mode_t mode, dev_t rdev);
int xmp_mkdir(const char *path, mode_t mode);
int xmp_unlink(const char *path);
int xmp_rmdir(const char *path);
int xmp_symlink(const char *from, const char *to);
int xmp_rename(const char *from, const char *to);
int xmp_link(const char *from, const char *to);
int xmp_chmod(const char *path, mode_t mode);
int xmp_chown(const char *path, uid_t uid, gid_t gid);
int xmp_truncate(const char *path, off_t size);
int xmp_utimens(const char *path, const struct timespec ts[2]);
int xmp_open(const char *path, struct fuse_file_info *fi);
int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int xmp_statfs(const char *path, struct statvfs *stbuf);
int xmp_release(const char *path, struct fuse_file_info *fi);
int xmp_fsync(const char *path, int isdatasync, struct fuse_file_info *fi);
int xmp_fallocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi);

