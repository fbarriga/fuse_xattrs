/*
  fuse_xattrs - Add xattrs support using sidecar files

  Copyright (C) 2016  Felipe Barriga Richards <felipe {at} felipebarriga.cl>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "binary_storage.h"
#include "utils.h"
#include "fuse_xattrs_config.h"

#include <attr/xattr.h>

struct on_memory_attr {
    u_int16_t name_size;
    size_t value_size;

    char *name;
    char *value;
};

void __print_on_memory_attr(struct on_memory_attr *attr)
{
#ifdef DEBUG
    char *sanitized_value = sanitize_value(attr->value, attr->value_size);
    debug_print("--------------\n");
    debug_print("name size: %hu\n", attr->name_size);
    debug_print("name: '%s'\n", attr->name);
    debug_print("value size: %zu\n", attr->value_size);
    debug_print("sanitized_value: '%s'\n", sanitized_value);
    debug_print("--------------\n");
    free(sanitized_value);
#endif
}

void __free_on_memory_attr(struct on_memory_attr *attr)
{
    if(attr->name != NULL)
        free(attr->name);

    if(attr->value != NULL)
        free(attr->value);

    free(attr);
}

char *__read_file(const char *path, int *buffer_size)
{
    FILE *file = fopen(path, "r");
    char *buffer = NULL;

    if (file == NULL) {
        debug_print("file not found: %s\n", path);
        *buffer_size = -ENOENT;
        return NULL;
    }

    debug_print("file found, reading it: %s\n", path);

    fseek(file, 0, SEEK_END);
    *buffer_size = (int)ftell(file);

    if (*buffer_size == -1) {
        error_print("error: path: %s, size: %d, errno=%d\n", path, *buffer_size, errno);
        *buffer_size = errno;
        fclose(file);
        return NULL;
    }

    if (*buffer_size > MAX_METADATA_SIZE) {
        error_print("metadata file too big. path: %s, size: %d\n", path, *buffer_size);
        *buffer_size = -ENOSPC;
        fclose(file);
        return NULL;
    }

    if (*buffer_size == 0) {
        debug_print("empty file.\n");
        *buffer_size = -ENOENT;
        fclose(file);
        return NULL;
    }
    assert(*buffer_size > 0);
    size_t _buffer_size = (size_t) *buffer_size;

    fseek(file, 0, SEEK_SET);
    buffer = malloc(_buffer_size);
    if (buffer == NULL) {
        *buffer_size = -ENOMEM;
        error_print("cannot allocate memory.\n");
        fclose(file);
        return NULL;
    }

    memset(buffer, '\0', _buffer_size);
    fread(buffer, 1, _buffer_size, file);
    fclose(file);

    return buffer;
}

char *__read_file_sidecar(const char *path, int *buffer_size)
{
    char *sidecar_path = get_sidecar_path(path);
    debug_print("path=%s sidecar_path=%s\n", path, sidecar_path);

    char *buffer = __read_file(sidecar_path, buffer_size);
    free (sidecar_path);
    return buffer;
}

int __cmp_name(const char *name, size_t name_length, struct on_memory_attr *attr)
{
    if (attr->name_size == name_length && memcmp(attr->name, name, name_length) == 0) {
        debug_print("match: name=%s, name_length=%zu\n", name, name_length);
        __print_on_memory_attr(attr);
        return 1;
    }

    debug_print("doesn't match: name=%s, name_length=%zu\n", name, name_length);
    __print_on_memory_attr(attr);

    return 0;
}

struct on_memory_attr *__read_on_memory_attr(size_t *offset, char *buffer, size_t buffer_size)
{
    debug_print("offset=%zu\n", *offset);
    struct on_memory_attr *attr = malloc(sizeof(struct on_memory_attr));
    attr->name = NULL;
    attr->value = NULL;

    ////////////////////////////////
    // Read name size
    size_t data_size = sizeof(u_int16_t);
    if (*offset + data_size > buffer_size) {
        error_print("Error, sizes doesn't match.\n");
        __free_on_memory_attr(attr);
        return NULL;
    }
    memcpy(&attr->name_size, buffer + *offset, data_size);
    *offset += data_size;
    debug_print("attr->name_size=%hu\n", attr->name_size);

    ////////////////////////////////
    // Read name data
    data_size = attr->name_size;
    if (*offset + data_size > buffer_size) {
        error_print("Error, sizes doesn't match.\n");
        __free_on_memory_attr(attr);
        return NULL;
    }
    attr->name = malloc(data_size);
    memcpy(attr->name, buffer + *offset, data_size);
    *offset += data_size;

    ////////////////////////////////
    // Read value size
    data_size = sizeof(size_t);
    if (*offset + data_size > buffer_size) {
        error_print("Error, sizes doesn't match.\n");
        __free_on_memory_attr(attr);
        return NULL;
    }
    memcpy(&attr->value_size, buffer + *offset, data_size);
    *offset += data_size;
    debug_print("attr->value_size=%zu\n", attr->value_size);

    ////////////////////////////////
    // Read value data
    data_size = attr->value_size;
    if (*offset + data_size > buffer_size) {
        error_print("Error, sizes doesn't match. data_size=%zu buffer_size=%zu\n",
            data_size, buffer_size);

        __free_on_memory_attr(attr);
        return NULL;
    }
    attr->value = malloc(data_size);
    memcpy(attr->value, buffer + *offset, data_size);
    *offset += data_size;

    return attr;
}

int __write_to_file(FILE *file, const char *name, const char *value, const size_t value_size)
{
    const u_int16_t name_size = (int) strlen(name) + 1;

#ifdef DEBUG
    char *sanitized_value = sanitize_value(value, value_size);
    debug_print("name='%s' name_size=%zu sanitized_value='%s' value_size=%zu\n", name, name_size, sanitized_value, value_size);
    free(sanitized_value);
#endif

    // write name
    if (fwrite(&name_size, sizeof(u_int16_t), 1, file) != 1) {
        return -1;
    }
    if (fwrite(name, name_size, 1, file) != 1) {
        return -1;
    }

    // write value
    if (fwrite(&value_size, sizeof(size_t), 1, file) != 1) {
        return -1;
    }
    // write value content only if we have something to write.
    if (value_size > 0) {
        if (fwrite(value, value_size, 1, file) != 1) {
            return -1;
        }
    }

    return 0;
}

/**
 *
 * @param path - path to file.
 * @param name - attribute name. string null terminated. size <= XATTR_NAME_MAX bytes
 * @param value - attribute value. size < XATTR_SIZE_MAX
 * @param size
 * @param flags - XATTR_CREATE and/or XATTR_REPLACE
 * @return On success, zero is returned.  On failure, -errno is returnted.
 */
int binary_storage_write_key(const char *path, const char *name, const char *value, size_t size, int flags)
{
#ifdef DEBUG
    char *sanitized_value = sanitize_value(value, size);
    debug_print("path=%s name=%s sanitized_value=%s size=%zu flags=%d\n", path, name, sanitized_value, size, flags);
    free(sanitized_value);
#endif

    int buffer_size;
    char *buffer = __read_file_sidecar(path, &buffer_size);

    if (buffer == NULL && buffer_size == -ENOENT && flags & XATTR_REPLACE) {
        error_print("No xattr. (flag XATTR_REPLACE)");
        return -ENODATA;
    }

    if (buffer == NULL && buffer_size != -ENOENT) {
        return buffer_size;
    }

    int status;
    char *sidecar_path = get_sidecar_path(path);
    FILE *file = fopen(sidecar_path, "w");
    free(sidecar_path);

    if (buffer == NULL) {
        debug_print("new file, writing directly...\n");
        status = __write_to_file(file, name, value, size);
        assert(status == 0);
        fclose(file);
        free(buffer);
        return 0;
    }
    assert(buffer_size >= 0);
    size_t _buffer_size = (size_t)buffer_size;

    int res = 0;
    size_t offset = 0;
    size_t name_len = strlen(name) + 1; // null byte
    int replaced = 0;
    while(offset < _buffer_size)
    {
        debug_print("replaced=%d offset=%zu buffer_size=%zu\n", replaced, offset, _buffer_size);
        struct on_memory_attr *attr = __read_on_memory_attr(&offset, buffer, _buffer_size);

        // FIXME: handle attr == NULL
        assert(attr != NULL);

        if (memcmp(attr->name, name, name_len) == 0) {
            assert(replaced == 0);
            if (flags & XATTR_CREATE) {
                error_print("Key already exists. (flag XATTR_CREATE)");
                status = __write_to_file(file, attr->name, attr->value, attr->value_size);
                assert(status == 0);
                res = -EEXIST;
            } else {
                status = __write_to_file(file, name, value, size);
                assert(status == 0);
                replaced = 1;
            }
        } else {
            status = __write_to_file(file, attr->name, attr->value, attr->value_size);
            assert(status == 0);
        }
        __free_on_memory_attr(attr);
    }

    if (replaced == 0 && res == 0) {
        if (flags & XATTR_REPLACE) {
            error_print("Key doesn't exists. (flag XATTR_REPLACE)");
            res = -ENODATA;
        } else {
            status = __write_to_file(file, name, value, size);
            assert(status == 0);
        }
    }

    fclose(file);
    free(buffer);
    return res;
}

int binary_storage_read_key(const char *path, const char *name, char *value, size_t size)
{
    int buffer_size;
    char *buffer = __read_file_sidecar(path, &buffer_size);
    
    if (buffer == NULL) {
        if (buffer_size == -ENOENT) {
            return -ENOATTR;
        }
        return buffer_size;
    }
    assert(buffer_size >= 0);
    size_t _buffer_size = (size_t)buffer_size;

    if (size > 0) {
        memset(value, '\0', size);
    }

    size_t name_len = strlen(name) + 1; // null byte \0
    size_t offset = 0;
    while(offset < _buffer_size)
    {
        struct on_memory_attr *attr = __read_on_memory_attr(&offset, buffer, _buffer_size);
        if (attr == NULL) {
            free(buffer);
            return -EILSEQ;
        }

        if (__cmp_name(name, name_len, attr) == 1) {
            int value_size = (int)attr->value_size;
            int res;
            if (size == 0) {
                res = value_size;
            } else if (attr->value_size <= size) {
                memcpy(value, attr->value, attr->value_size);
                res = value_size;
            } else {
                error_print("error, attr->value_size=%zu > size=%zu\n", attr->value_size, size);
                res = -ERANGE;
            }
            free(buffer);
            __free_on_memory_attr(attr);
            return res;
        }
        __free_on_memory_attr(attr);
    }
    free(buffer);

    return -ENOATTR;
}

int binary_storage_list_keys(const char *path, char *list, size_t size)
{
    int buffer_size;
    char *buffer = __read_file_sidecar(path, &buffer_size);

    if (buffer == NULL) {
        debug_print("buffer == NULL buffer_size=%d\n", buffer_size);
        if (buffer_size == -ENOENT) {
            return 0;
        }
        return buffer_size;
    }

    assert(buffer_size > 0);
    size_t _buffer_size = (size_t)buffer_size;

    if (size > 0) {
        memset(list, '\0', size);
    }

    size_t res = 0;
    size_t offset = 0;
    while(offset < _buffer_size)
    {
        struct on_memory_attr *attr = __read_on_memory_attr(&offset, buffer, _buffer_size);
        if (attr == NULL) {
            free(buffer);
            return -EILSEQ;
        }

        if (size > 0) {
            if (attr->name_size + res > size) {
                error_print("Not enough memory allocated. allocated=%zu required=%ld\n",
                            size, attr->name_size + res);
                __free_on_memory_attr(attr);
                free(buffer);
                return -ERANGE;
            } else {
                memcpy(list + res, attr->name, attr->name_size);
                res += attr->name_size;
            }
        } else {
            res += attr->name_size;
        }
        __free_on_memory_attr(attr);
    }
    free(buffer);

    if (size == 0 && res > XATTR_LIST_MAX) {
        // FIXME: we should return the size or an error ?
        return -E2BIG;
    }

    return (int)res;
}

int binary_storage_remove_key(const char *path, const char *name)
{
    debug_print("path=%s name=%s\n", path, name);
    int buffer_size;
    char *buffer = __read_file_sidecar(path, &buffer_size);

    if (buffer == NULL) {
        return buffer_size;
    }
    assert(buffer_size > 0);
    size_t _buffer_size = (size_t) buffer_size;

    char *sidecar_path = get_sidecar_path(path);
    FILE *file = fopen(sidecar_path, "w");
    free(sidecar_path);

    size_t offset = 0;
    size_t name_len = strlen(name) + 1; // null byte \0

    int removed = 0;
    while(offset < _buffer_size)
    {
        debug_print("removed=%d offset=%zu buffer_size=%zu\n", removed, offset, _buffer_size);

        struct on_memory_attr *attr = __read_on_memory_attr(&offset, buffer, _buffer_size);
        if (attr == NULL) {
            error_print("error reading file. corrupted ?");
            break;
        }

        if (memcmp(attr->name, name, name_len) == 0) {
            removed++;
        } else {
            int status = __write_to_file(file, attr->name, attr->value, attr->value_size);
            assert(status == 0);
        }
        __free_on_memory_attr(attr);
    }

    int res = 0;
    if (removed == 1) {
        debug_print("key removed successfully.\n");
    } else if (removed == 0) {
        error_print("key not found.\n");
        res = -ENOATTR;
    } else {
        debug_print("removed %d keys (was duplicated)\n", removed);
        res = -EILSEQ;
    }

    fclose(file);
    free(buffer);
    return res;
}