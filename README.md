# Test Mangler

A Flang plugin designed to iteratively turn a single successful 
OpenMP test into many failing tests.

## Build instructions

```console
$ mkdir build
$ cd build
$ cmake -DSHARED_INSTALL_DIR=$INSTALL_DIR ../TestMangler/
$ cd ../Scripts
$ pip install -r requirements.txt
$ python3 main.py
```

To invoke the plugin from flang directly:
```console
$ MANGLE_OFFSET=1 flang-new -fc1 -load build/libtestMangler.so \
> -plugin test-mangle BaseTests/allocators.f90
```
