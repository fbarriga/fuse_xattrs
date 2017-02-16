cmake_minimum_required(VERSION 2.8)
project(fuse_xattrs C)

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

# set required definitions
add_definitions (-D_FILE_OFFSET_BITS=64)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

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
        compat/fuse_opt.c
        compat/fuse_opt.h
        fuse_xattrs.c
        passthrough.c
        passthrough.h
        binary_storage.c
        binary_storage.h
        utils.c
        utils.h
        const.h
        fuse_xattrs_config.h)

add_executable(fuse_xattrs ${SOURCE_FILES})

target_link_libraries (
        fuse_xattrs
        fuse
)

install (TARGETS fuse_xattrs DESTINATION bin)

enable_testing()
configure_file(run_tests.sh run_tests.sh COPYONLY)
configure_file(test/tests.py test/tests.py COPYONLY)
add_test(NAME integration
        COMMAND run_tests.sh)
