[![Join the chat at https://gitter.im/OpenJPH](https://badges.gitter.im/OpenJPH.svg)](https://gitter.im/OpenJPH?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

# Readme #

Open source implementation of High-throughput JPEG2000 (HTJ2K), also known as JPH, JPEG2000 Part 15, ISO/IEC 15444-15, and ITU-T T.814. Here, we are interested in implementing the HTJ2K only, supporting features that are defined in JPEG2000 Part 1 (for example, for wavelet transform, only reversible 5/3 and irreversible 9/7 are supported).

The interested reader is referred to the [short HTJ2K white paper](http://ds.jpeg.org/whitepapers/jpeg-htj2k-whitepaper.pdf), or the [extended HTJ2K white paper](https://htj2k.com/wp-content/uploads/white-paper.pdf) for more details on HTJ2K. [This](https://kakadusoftware.com/wp-content/uploads/2019/09/icip2019.pdf) paper explores the attainable performance on CPU, and [this](https://kakadusoftware.com/wp-content/uploads/2019/09/ICIP2019_GPU.pdf) paper for decoding on a GPU.

# Status #

The code is written in C++; the color and wavelet transform steps can employ SIMD instructions on Intel platforms.  It conceivable that at some point in the future, SIMD instructions are employed to improve performance of the block (de)coder, and/or for platforms other than Intel.  As it stands, on Intel Skylake i7-6700, encoding 4K 4:4:4 HDR images losslessly takes around 0.5s, and decoding takes around 0.34s; for lossy compression, performance depends on the quantisation step size (qstep), but for a high-quality image at a bitrate of around 3 bits/pixel, encoding takes around 0.27s and decoding takes 0.22s.

As it stands, the OpenJPH library needs documentation. The provided encoder ojph\_compress only generates HTJ2K codestreams, with the extension j2c; the generated files lack the .jph header.  Adding the .jph header is of little urgency, as the codestream contains all needed information to properly decode an image.  The .jph header will be added at a future point in time.  The provided decoder ojph\_expand decodes .jph files, by ignoring the .jph header if it is present.

The provided command line tools ojph\_compress and ojph\_expand accepts and generated .pgm, .ppm, and .yuv. See the usage examples below.

# Web-based Demos #

The associate site [openjph.org](https://openjph.org) serves as a blog.  It currently host the [javascript](https://openjph.org/javascript/demo.html) demo of the decoder; the webpage demonstrates that the library can be compiled to javascript, and can run inside a web-browser.  Any browser supporting webassembly can be used to view this webpage; examples include Firefox, Chrome, Safari, and Edge, on a desktop, mobile, or tablet.

Another project of interest is the [openjphjs](https://github.com/chafey/openjphjs) project, developed by [Chris](https://github.com/chafey).  You can see [there](https://chafey.github.io/openjphjs/test/browser/index.html) a nice online demonstration of javascript-based HTJ2K encoding/decoding, with a wealth of features and user-selectable options.

# Compiling #

The code employs the *cmake* tool to generate a variety of build environments.  A visual studio code container is included for building using
the visual studio code remote containers add in (highly recommended)

## For Linux ##

    cd build
    cmake -DCMAKE_BUILD_TYPE=Release  ../
    make

The generated library and executables will be in the bin folder.

## For Windows ##

    cd build
    cmake ../ -G "Visual Studio 14 2015 Win64"

cmake support other visual studio versions.  This command generates a solution in the build folder, which can be build using visual studio.

## For macOS ##

You can use the "For Linux" approach above.  Alternatively, you can use the Xcode project in src/apps/apps.xcodeproj, which I use.  Another approach is to use cmake to generate an xcode project, in the build folder, using

    cd build
    cmake ../ -G Xcode
    make

The generated library and executables will be in the bin folder.

# Compiling to javascript/wasm #

The library can now be compiled to javascript/wasm.  For this purpose, a small wrapper file (ojph_wrapper.cpp) has been written to interface between javascript and C++; the wrapper currently supports decoding only.  A small demo page demonstrating the script can be accessed [here](https://openjph.org/javascript/demo.html).

Compilation needs the [emscripten](https://emscripten.org/) tools. One way of using these tools is to install them on your machine, and activate them using

    source emsdk_env.sh
  
before compilation.  Alternatively, if you are a docker user, the you can launch a docker session using script provided at ```subprojects/js/emscripten-docker.sh```; this script will download a third-party docker image that has the emscripten tools integrated in it -- Thanks to [Chris](https://github.com/chafey) for the suggesting and providing these tools.  

The javascript decoder can be compiled using

    cd subprojects/js/build
    emcmake cmake ..
    emmake make

The compilation creates libopenjph.js and libopenjph.wasm in subprojects/js/html folder.  That html folder also has the demo webpage index.html and a compressed image test.j2c which the script in index.html decodes.  To run the demo webpage on your machine, you need a webserver running on the machine -- Due to security reasons, javascript engines running in a browser cannot access local files on the machine.  You can use the ```emrun``` command, provided with the emscripten
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

To compile for platforms where x86_64 SIMD instructions are not supported, such as on ARM, we need to disable SIMD instructions; this can be achieved using

    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DOJPH_DISABLE_INTEL_SIMD=ON ../
    make

As I do not have an ARM board, I tested this using QEMU for aarch64 architecture, targeting a Cortex-A57 CPU. The code worked without issues, but because the ARM platform is emulated, the whole process was slow.

# Compiling and Running in Docker #

## Step 1 - clone repository   
`https://github.com/aous72/OpenJPH.git`

## Step 2 - build docker image  
`cd OpenJPH`   
`docker build --rm -f Dockerfile -t openjph:latest .`

## Step 3 - run docker image

### in isolated container   
`docker run -it --rm openjph:latest`

### mapping /usr/src/openjph/build directory in the container to local windows c:\temp
`docker run -it --rm -v C:\\temp:/usr/src/openjph/build openjph:latest`

# Usage Example #

Here are some usage examples:

    ojph_compress -i input_file.ppm -o output_file.j2c -num_decomps 5 -block_size {64,64} -precincts {128,128},{256,256} -prog_order CPRL -colour_trans true -qstep 0.05
    ojph_compress -i input_file.yuv -o output_file.j2c -num_decomps 5 -reversible true -dims {3840,2160} -num_comps 3 -signed false -bit_depth 10 -downsamp {1,1},{2,2}

    ojph_expand -i input_file.j2c -o output_file.ppm
    ojph_expand -i input_file.j2c -o output_file.yuv

**Notes**:

* Issuing ojph\_compress or ojph\_expand without arguments prints a short usage statement.
* In reversible compression, quantization is not supported.
* On Linux and MacOS, but NOT Windows, { and } need to be escaped; i.e, we need to write \\\{ and \\\}.  So, -block\_size {64,64} must be written as -block\_size \\\{64,64\\\}.
* When the source is a .yuv file, use -downsamp {1,1} for 4:4:4 sources. For 4:2:2 downsampling, specify -downsamp {1,1},{2,1}, and for 4:2:0 subsampling specify -downsamp {1,1},{2,2}. The source must have already been downsampled (i.e., OpenJPH does not downsample the source before compression, but can compress downsampled sources).
* In Kakadu, pairs of data in command line arguments represent columns,rows. Here, a pair represents x,y information.

# The standard #

The standard is available free of charge from [ITU website](https://www.itu.int/rec/T-REC-T.814/en). It can also be purchased from the [ISO website](https://www.iso.org/standard/76621.html). 

