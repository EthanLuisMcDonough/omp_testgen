# Test Mangler

A Flang plugin designed to iteratively turn a single successful 
OpenMP test into many failing tests.

## Build instructions

```sh
$ mkdir build
$ cd build
$ cmake -DSHARED_INSTALL_DIR=$BUILD_DIR ../TestMangler/
$ cd ..
$ MANGLE_OFFSET=1 flang-new -fc1 -load build/libtestMangler.so \
> -plugin test-mangle BaseTests/allocators.f90
```
