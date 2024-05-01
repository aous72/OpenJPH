# Compiling #

The code employs the *cmake* tool to generate a variety of build environments.  A visual studio code container is included for building using
the visual studio code remote containers add in (highly recommended)

## For Linux ##

You may need to install libtiff; then,

    cd build
    cmake -DCMAKE_BUILD_TYPE=Release  ../
    make 
    sudo make install

## For Windows ##

Compilation depends on libtiff. A pre-compiled library with all the library features for Windows is not available; I am using [this](https://github.com/aous72/OpenJPH/files/14060335/tiff.zip), but I think I have only the basic library. 

    cd build
    cmake .. -G "Visual Studio 17 2022 Win64" -DCMAKE_PREFIX_PATH=<tiff library path>

`cmake` supports other visual studio versions.  This command generates a solution in the build folder, which can be build using visual studio.

To compile from the command line, use

    cmake --build . --config Release

To install either use 

    cmake --install . --prefix <your folder>

to install the library to your desired folder, or, if you want to install to C:\Program Files, you need a PowerShell/CMD running as administrator, and 

    cmake --install .


## For macOS ##

You can use the "For Linux" approach above.  Alternatively, you can use the Xcode project in src/apps/apps.xcodeproj, which I use.  Another approach is to use cmake to generate an xcode project, in the build folder, using

    cd build
    cmake ../ -G Xcode
    make
    sudo make install

I have not tested this in a long time, but you get the picture.

## Building Tests ##

When you invoke `cmake` add `-DOJPH_BUILD_TESTS=ON`, then, for Windows

    cd tests
    ctest -C Release

For other platforms 

    cd tests
    ctest

The test setup is a bit finicky, and may sometimes fail for silly reasons.

# Compiling to Node.js #

The library can be compiled to run with Node.js.  Compilation needs the [emscripten](https://emscripten.org/) tools. One way of using these tools is to install them on your machine, and activate them using, assuming running on platform other than Windows,

    source emsdk_env.sh
  
before compilation.  Then, 
    emcmake cmake ..
    emmake make

Compilation will generate two version of the library and executables, one with WebAssembly SIMD instructions and one without.


# Compiling to javascript/wasm #

The library can now be compiled to javascript/wasm.  For this purpose, a small wrapper file (ojph_wrapper.cpp) has been written to interface between javascript and C++; the wrapper currently supports decoding only.  A small demo page demonstrating the script can be accessed [here](https://openjph.org/javascript/demo.html).

Compilation needs the [emscripten](https://emscripten.org/) tools. One way of using these tools is to install them on your machine, and activate them using

    source emsdk_env.sh
  
before compilation.  Alternatively, if you are a docker user, the you can launch a docker session using script provided at ```subprojects/js/emscripten-docker.sh```; this script will download a third-party docker image that has the emscripten tools integrated in it -- Thanks to [Chris](https://github.com/chafey) for the suggesting and providing these tools.  

The javascript decoder can be compiled using

    cd subprojects/js/build
    emcmake cmake ..
    emmake make

The compilation creates libopenjph.js and libopenjph.wasm in subprojects/js/html folder; it also creates libopenjphsimd.js and libopenjphsimd.wasm.  That html folder also has the demo webpage index.html and a compressed image test.j2c which the script in index.html decodes. The index.html detects if the browser supports WebAssembly SIMD instructions, and loads the correct library accordingly.  

To run the demo webpage on your machine, you need a webserver running on the machine -- Due to security reasons, javascript engines running in a browser cannot access local files on the machine.  You can use the ```emrun``` command, provided with the emscripten
tools, by issuing the command

    emrun index.html

from inside the html folder; the default port is 6931.
Alternatively, a simple python webserver can be run using

    python -m http.server 8000
  
also from inside the html folder.  Here, 8000 is the port number at which the webserver will be listening.  The webpage can then be accessed by open localhost:8000 in you browser.   Any browser supporting webassembly can be used to view this webpage; examples include Firefox, Chrome, Safari, and Edge, on a desktop, mobile, or tablet.

# Visual Studio Code Remote Containers #

Visual Studio Code Remote Containers are now available with OpenJPH.  These scripts/configuration files are provided by [Chris](https://github.com/chafey) -- Thank you Chris, and I must say I am not familiar with them.
The scripts, in the ```.devcontainer``` folder, will build a docker image that can be used with visual studio code as a development environment.

# Compiling for ARM and other platforms #

Compilation should simply work now.  The simple test code I have passes when run on MacOS ARM on GitHub.

# Disabling SIMD instructions #

The code now employs the architecture-agnostic option `OJPH_DISABLE_SIMD`, which should include SIMD instructions wherever they are supported.  This can be achieved with `-DOJPH_DISABLE_SIMD=ON` option during CMake configuration.  Individual instruction sets can be disabled; see the options in the main CMakeLists.txt file.
