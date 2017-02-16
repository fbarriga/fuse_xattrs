/*
  fuse_xattrs - Add xattrs support using sidecar files

  Copyright (C) 2016  Felipe Barriga Richards <felipe {at} felipebarriga.cl>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "fuse_xattrs_config.h"

char *prepend_source_directory(const char *a, const char *b) {
    size_t len = strlen(a) + strlen(b) + 1;
    char *dst = (char*) malloc(sizeof(char) * len);
    sprintf(dst, "%s%s", a, b);

    return dst;
}

char *get_sidecar_path(const char *path)
{
    const size_t path_len = strlen(path);
    const size_t sidecar_ext_len = strlen(BINARY_SIDECAR_EXT); // this can be optimized
    const size_t sidecar_path_len = path_len + sidecar_ext_len + 1;
    char *sidecar_path = (char *) malloc(sidecar_path_len);
    memset(sidecar_path, '\0', sidecar_path_len);
    memcpy(sidecar_path, path, path_len);
    memcpy(sidecar_path + path_len, BINARY_SIDECAR_EXT, sidecar_ext_len);

    return sidecar_path;
}

// TODO: make it work for binary data
char *sanitize_value(const char *value, size_t value_size)
{
    char *sanitized = malloc(value_size + 1);
    memcpy(sanitized, value, value_size);
    sanitized[value_size] = '\0';
    return sanitized;
}

enum namespace get_namespace(const char *name) {
    if (strncmp(name, "user.", strlen("user.")) == 0) {
        return USER;
    }

    if (strncmp(name, "system.", strlen("system.")) == 0) {
        return SYSTEM;
    }

    if (strncmp(name, "security.", strlen("security.")) == 0) {
        return SECURITY;
    }

    if (strncmp(name, "trusted.", strlen("trusted.")) == 0) {
        return TRUSTED;
    }

    error_print("invalid namespace for key: %s\n", name);
    return ERROR;
}
