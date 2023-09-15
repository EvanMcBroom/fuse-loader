# Fuse Loader

[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat)](LICENSE.txt)

A reference implementation of loading a dynamic library from an application's own memory using a fuse mount.
Although Linux 3.17 and above provides an alternative solution using `memfd_create`, fuse provides a solution that is available for Linux 2.6.14 and above and other operating systems.
Access to the fuse mount may also be restricted to an application's own process which may be desirable.

## Building

Fuse loader uses [CMake](https://cmake.org/) to generate and run the build system files for your platform.
The project requires the `libfuse3-dev` library to build which is available through most package managers.

```
git clone https://github.com/EvanMcBroom/fuse-loader.git
cd fuse-loader/builds
cmake ..
cmake --build .
```

By default CMake will build the `libfuse-loader` static library, a utility named `example`, and a `libtest` dynamic library to demonstrate its use.

Other CMake projects may use fuse loader by calling `include` on this directory from an overarching project's `CMakeLists.txt` files.
Doing so will add the static library as a CMake target in the overarching project but will not add the `example` utility or the `libtest` library.
