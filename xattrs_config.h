/*
  fuse_xattrs - Add xattrs support using sidecar files

  Copyright (C) 2017  Felipe Barriga Richards <felipe {at} felipebarriga.cl>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#ifndef FUSE_XATTRS_CONFIG_H
#define FUSE_XATTRS_CONFIG_H

struct xattrs_config {
    int show_sidecar;
    const char *source_dir;
} xattrs_config;


#endif //FUSE_XATTRS_CONFIG_H
