# How to compile to standalone WASM file

Similar to how java code is compiled and run using the java executable (java.exe) which creates the runtime environment (RTE), C++ can be compiled to WASM and run from a command line that creates an RTE for WASM.

To achieve this, we need to have the tools to compile the C++ code to wasm, and we need a runtime environment.  Here, we will focus on the wasi-sdk and wasmer.

## Compilation

The [wasi-sdk](https://github.com/WebAssembly/wasi-sdk) provides the tools to compile the code to WASM; in particular, it provides the clang compiler, necessary C++ header files, sysroot directory, etc.  You can download the latest release from Releases (as of this writing, it is [here](https://github.com/WebAssembly/wasi-sdk/releases/tag/wasi-sdk-15)).  A separate package, [wasienv](https://github.com/wasienv/wasienv), is supposed to make it easier to use wasi-sdk with cmake; as of this writing wasienv does not include the latest release of wasi-sdk, and it feels like the project is left is disrepair -- last update is more than a year ago, but it might worth giving it a crack, adapting some of its code.

We hope that wasienv is update in the coming months.

To compile, I am using 

    /home/aous/wasi-sdk-15.0/bin/clang --sysroot=/home/aous/wasi-sdk-15.0/share/wasi-sysroot/ ...


It is worth nothing that as of this writing wasi-sdk and wasmer do not support C++ exceptions.  This is a complication, because it requires the user to remove all try, throw, and catch statements. It also requires modifying ```new``` to ```new (std::nothrow)```, which requires the addition of ```#include <new>```.  This situation is expected be resolved in the coming months because there is a standard for WASM exceptions; these can be activated by passing the clang commandline flag ```-fwasm-exceptions```.

This folder has the ```build.sh``` script; put this script in the ```OpenJPH/bin``` folder, and use it to build ojph_expand.wasm, ojph_expand_simd.wasm, ojph_compress.wasm, and ojph_compress_simd.wasm, after removing the offending exception generating code (hopefully this is a temporary solution). Perhaps, it is useful to have a separate tree for this modified code.

## Runtime Environment (RTE)

For RTE, There are two executables that I am aware of, wasmtime and wasmer. The former is from a nonprofit organization while the latter from a startup.  The authors of wasmer claim that their implementation is faster -- I will go with this. The wasmer RTE, which installs by default to ```.wasmer```, can obtained from https://github.com/wasmerio/wasmer.  To install, use

    curl https://get.wasmer.io -sSfL | sh


To run I am using 

    ~/.wasmer/bin/wasmer run --dir=. --enable-all helloworld.wasm -- arguments

Of course, you can include this in the execution path.

## Testing the WASM code

To test the WASM subroutines, I would like to use the same test subroutines for the library.  To do so, move the com_decom.sh and com_decom_yuv.sh files in this folder to ```OpenJPH/tests``` folder, and build the library as usual using 

    cd build
    cmake -DBUILD_TESTING=yes ..
    make
    make test

This is a standard build without any modifications. The modified com_decom.sh and com_decom_yuv.sh files should invoke the WASM versions of the code.  You can modify these files to test with or without WASM SIMD.

There is a small complication with the test.  The test reads uncompressed images from ```..\```, and writes compressed images to ```.\```, while wasmer has access to ```..``` only with the command ```--dir ..```.  It would be convenient if I can specify two folders, but I do not know how to do that.   To overcome this, I also use ```--mapdir ./:./``` that maps the ```.\``` folder to the same location in wasmer, thus giving wasmer access to this folder.






