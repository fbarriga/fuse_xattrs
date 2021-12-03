/*
  fuse_xattrs - Add xattrs support using sidecar files

  Copyright (C) 2017  Felipe Barriga Richards <felipe {at} felipebarriga.cl>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#include <stdio.h>


struct xattrs_config {
    const int show_sidecar;
    const char *source_dir;
    size_t source_dir_size;
} xattrs_config;
