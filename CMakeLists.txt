cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(fuse_xattrs C)

set(FUSE_XATTRS_VERSION_MAJOR 0)
set(FUSE_XATTRS_VERSION_MINOR 3)

set(BINARY_SIDECAR_EXT \".xattr\")

set(MAX_METADATA_SIZE "8*1024*1024")  # 8 MiB
set(XATTR_NAME_MAX 255)               # chars in an extended attribute name
set(XATTR_SIZE_MAX 65536)             # size of an extended attribute value (64k)
set(XATTR_LIST_MAX 65536)             # size of extended attribute namelist (64k)

configure_file (
        "${PROJECT_SOURCE_DIR}/fuse_xattrs_config.h.in"
        "${PROJECT_BINARY_DIR}/fuse_xattrs_config.h"
)
include_directories(
        "${PROJECT_BINARY_DIR}"
)

configure_file (
        "${PROJECT_SOURCE_DIR}/fuse_xattrs.1.in"
        "${PROJECT_BINARY_DIR}/fuse_xattrs.1"
)

# Check xattr headers
include (CheckIncludeFile)
check_include_file (sys/xattr.h HAVE_SYS_XATTR_H)
if(NOT HAVE_SYS_XATTR_H)
    message(FATAL_ERROR "sys/xattr.h not found")
endif()

include (CheckCSourceCompiles)
check_c_source_compiles ("
  #include <sys/types.h>
  #include <attr/xattr.h>
  int main() { return 1; }
  " HAVE_ATTR_XATTR_H)
if(NOT HAVE_ATTR_XATTR_H)
    message(FATAL_ERROR "attr/xattr.h not found")
endif()

check_c_source_compiles ("
  #include <unistd.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  int main() { utimensat(-1, NULL, NULL, 0); return 0; }
  " HAS_UTIMENSAT)
if(HAS_UTIMENSAT)
    add_definitions (-DHAS_UTIMENSAT)
endif()

# set required definitions
add_definitions (-D_FILE_OFFSET_BITS=64)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

set(CMAKE_C_FLAGS "-O3")

option(ENABLE_CODECOVERAGE "Enable code coverage testing support" )
if(ENABLE_CODECOVERAGE)
    include (CodeCoverage)
    set(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
    setup_target_for_coverage(
            fuse_xattrs_coverage
            ./run_tests.sh
            coverage
    )
endif(ENABLE_CODECOVERAGE)

set(SOURCE_FILES
        fuse_xattrs.c
        passthrough.c
        binary_storage.c
        utils.c
)

add_executable(fuse_xattrs ${SOURCE_FILES})

target_link_libraries (
        fuse_xattrs
        fuse
)

install (TARGETS fuse_xattrs DESTINATION bin)
install (
        FILES ${CMAKE_CURRENT_BINARY_DIR}/fuse_xattrs.1
        DESTINATION share/man/man1
        COMPONENT doc
)

enable_testing()
configure_file(run_tests.sh run_tests.sh COPYONLY)
configure_file(test/tests.py test/tests.py COPYONLY)
add_test(NAME integration
        COMMAND run_tests.sh)
