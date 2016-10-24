/*
  fuse_xattrs - Add xattrs support using sidecar files

  Copyright (C) 2016  Felipe Barriga Richards <felipe {at} felipebarriga.cl>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#ifndef FUSE_XATTRS_CONST_H
#define FUSE_XATTRS_CONST_H

#define SIDECAR_EXT ".xattr"
// TODO: This is just arbitrary...
#define MAX_METADATA_SIZE 8 * 1024 * 1024 // 8 MiB

#define XATTR_NAME_MAX   255    /* # chars in an extended attribute name */
#define XATTR_SIZE_MAX 65536    /* size of an extended attribute value (64k) */
#define XATTR_LIST_MAX 65536    /* size of extended attribute namelist (64k) */

#endif //FUSE_XATTRS_CONST_H
