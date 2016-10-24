/*
  fuse_xattrs - Add xattrs support using sidecar files

  Copyright (C) 2016  Felipe Barriga Richards <felipe {at} felipebarriga.cl>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/
#include <stdio.h>

#ifndef FUSE_XATTRS_BINARY_STORAGE_STRUCT_H
#define FUSE_XATTRS_BINARY_STORAGE_STRUCT_H

int binary_storage_write_key(const char *path, const char *name, const char *value, size_t size, int flags);
int binary_storage_read_key(const char *path, const char *name, char *value, size_t size);
int binary_storage_list_keys(const char *path, char *list, size_t size);
int binary_storage_remove_key(const char *path, const char *name);

#endif //FUSE_XATTRS_BINARY_STORAGE_STRUCT_H
