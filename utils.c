/*
  fuse_xattrs - Add xattrs support using sidecar files

  Copyright (C) 2016  Felipe Barriga Richards <felipe {at} felipebarriga.cl>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "utils.h"
#include "fuse_xattrs_config.h"
#include "xattrs_config.h"

const size_t BINARY_SIDECAR_EXT_SIZE;

/* TODO: re-use memory to avoid calling malloc every time */
char *prepend_source_directory(const char *b) {
    const size_t b_size = strlen(b);
    const size_t dst_len = xattrs_config.source_dir_size + b_size + 1;
    char *dst = (char*) malloc(sizeof(char) * dst_len);

    memcpy(dst, xattrs_config.source_dir, xattrs_config.source_dir_size);
    memcpy(dst+xattrs_config.source_dir_size, b, b_size + 1); // include '\0'
    //sprintf(dst, "%s%s", a, b);

    return dst;
}

int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        fprintf(stderr, "cannot get source directory status: %s\n", path);
        return -1;
    }

    if (!S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "source directory must be a directory: %s\n", path);
        return -1;
    }

    return 1;
}

int is_regular_file(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return -1;
    }

    if (!S_ISREG(statbuf.st_mode)) {
        return -1;
    }

    return 1;
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


const size_t BINARY_SIDECAR_EXT_SIZE = strlen(BINARY_SIDECAR_EXT);

const int filename_is_sidecar(const char *string) {
    if(string == NULL)
        return 0;

    size_t size = strlen(string);
    if (size <= BINARY_SIDECAR_EXT_SIZE)
        return 0;

    if (memcmp(string+size-BINARY_SIDECAR_EXT_SIZE, BINARY_SIDECAR_EXT, BINARY_SIDECAR_EXT_SIZE) == 0) {
        return 1;
    }

    return 0;
}

#define USER_NAMESPACE "user."
#define SYSTEM_NAMESPACE "system."
#define SECURITY_NAMESPACE "security."
#define TRUSTED_NAMESPACE "trusted."

const size_t USER_NAMESPACE_SIZE     = strlen(USER_NAMESPACE);
const size_t SYSTEM_NAMESPACE_SIZE   = strlen(SYSTEM_NAMESPACE);
const size_t SECURITY_NAMESPACE_SIZE = strlen(SECURITY_NAMESPACE);
const size_t THRUSTED_NAMESPACE_SIZE = strlen(TRUSTED_NAMESPACE);

enum namespace get_namespace(const char *name) {
    size_t name_size = strlen(name);

    if (name_size > USER_NAMESPACE_SIZE && memcmp(name, USER_NAMESPACE, USER_NAMESPACE_SIZE) == 0) {
        return USER;
    }

    if (name_size > SYSTEM_NAMESPACE_SIZE && memcmp(name, SYSTEM_NAMESPACE, SYSTEM_NAMESPACE_SIZE) == 0) {
        return SYSTEM;
    }

    if (name_size > SECURITY_NAMESPACE_SIZE && memcmp(name, SECURITY_NAMESPACE, SECURITY_NAMESPACE_SIZE) == 0) {
        return SECURITY;
    }

    if (name_size > THRUSTED_NAMESPACE_SIZE && memcmp(name, TRUSTED_NAMESPACE, THRUSTED_NAMESPACE_SIZE) == 0) {
        return TRUSTED;
    }

    error_print("invalid namespace for key: %s\n", name);
    return ERROR;
}
