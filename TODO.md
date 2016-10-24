TODO
----

- Add mutex to avoid issues when two processes are modifying the same file.
- Handle permission issues with .xattr files
- Add options to:
  - Change extension name of sidecar files
  - Hide sidecar files
- CI: travis ?
  - Valgrind support
  - Code coverage
  - C unit tests

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
