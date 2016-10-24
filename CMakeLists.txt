cmake_minimum_required(VERSION 3.6)
project(fuse_xattrs)

include (CheckIncludeFileCXX)
check_include_file_cxx (attr/xattr.h HAVE_ATTR_XATTR_H)
check_include_file_cxx (sys/xattr.h HAVE_SYS_XATTR_H)

# Check if xattr functions take extra argument.
include (CheckCXXSourceCompiles)
CHECK_CXX_SOURCE_COMPILES ("#include <sys/types.h>
  #include <sys/xattr.h>
  int main() { getxattr(0,0,0,0,0,0); return 1; } " XATTR_ADD_OPT)

add_definitions (-D_FILE_OFFSET_BITS=64)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

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
)

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
