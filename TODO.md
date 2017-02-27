TODO
----

- Check if it's thread-safe
- Add mutex to avoid issues when two processes are modifying the same file.
- Handle permission issues with .xattr files
- Code Quality
  - C unit tests
  - Valgrind report on travis
  - Upload travis generated code coverage somewhere

BUGS
----

- Fix error code when value size > 64KiB (libfuse bug)

FEATURES
--------

- binary_storage:
  - crc32
  - make it endian-independent:
    - http://www.ibm.com/developerworks/aix/library/au-endianc/index.html
  - add header to file to determine the format / version of the sidecar
- Be able to use a database instead of sidecar files ?
- Support multiple namespaces
- Make DEBUG a runtime option
- Test it on macOS

OPTIMIZATIONS
-------------

- Use passthrough_ll code from libfuse
- Add option to present all files as symbolic links to original files
