# Readme #

Open source implementation of High-throughput JPEG2000 (HTJ2K), also known as JPH, JPEG2000 Part 15, ISO/IEC 15444-15, and ITU-T T.814. Here, we are interested in implementing the HTJ2K only, supporting features that are defined in JPEG2000 Part 1 (for example, for wavelet transform, only reversible 5/3 and irreversible 9/7 are supported).

The interested reader is refered to [HTJ2K white paper](https://kakadusoftware.com/wp-content/uploads/2019/09/HTJ2K-White-Paper.pdf) for more details on HTJ2K, [this](https://kakadusoftware.com/wp-content/uploads/2019/09/icip2019.pdf) paper for the attainable performance on CPU, and [this](https://kakadusoftware.com/wp-content/uploads/2019/09/ICIP2019_GPU.pdf) paper for decoding on a GPU.


# Status #

The code is written in C++; the color and wavelet transform steps can employ SIMD instructions on Intel platforms.  It conceivable that at some point in the future, SIMD instructions are employed to improve performance of the block (de)coder, and/or for platforms other than Intel.  As it stands, on Intel Skylake i7-6700, encoding 4K 4:4:4 HDR images losslessly takes around 0.5s, and decoding takes around 0.34s; for lossy compression, performance depends on the quantisation step size (qstep), but for a high-quality image at a bitrate of around 3 bits/pixel, encoding takes around 0.27s and decoding takes 0.22s.

As it stands, the OpenJPH library needs documentation. The provided encoder ojph\_compress only generates HTJ2K codestreams, with the extension j2c; the generated files lack the .jph header.  Adding the .jph header is of little urgency, as the codestream contains all needed information to properly decode an image.  The .jph header will be added at a future point in time.  The provided decoder ojph\_expand decodes .jph files, by ignoring the .jph header if it is present.

The provided command line tools ojph\_compress and ojph\_expand accepts and generated .pgm, .ppm, and .yuv. See the usage examples below.


# Compiling #

The code employs the *cmake* tool to generate a variety of build enviroments.  A visual studio code container is included for building using
the visual studio code remote conatiners add in (highly recommended)

**For Linux**

    cd build
    cmake ../
    make

The generated library and executables will be in the bin folder.

**For Windows**

    cd build
    cmake ../ -G "Visual Studio 14 2015 Win64"

cmake support other visual studio versions.  This command generates a solution in the build folder, which can be build using visual studio

**For macOS**

You can use the "For Linux" approach above.  Alternatively, you can use the Xcode project in src/apps/apps.xcodeproj, which I use.  Another approach is to use cmake to generate an xcode project, in the build folder, using

    cd build
    cmake ../ -G Xcode
    make

The generated library and executables will be in the bin folder.

# Compiling to javascript/wasm #

The library can now be compiled to javascript/wasm.  For this purpose, a small wrapper file (ojph_wrapper.cpp) has been written to interface between javascript and C++; the wrapper currently supports decoding only.  A small demo page demonstrating the script can be accessed [here](https://openjph.org/javascript/demo.html).

Compilation needs the [emscripten](https://emscripten.org/) tools. The script subprojects/js/emscripten-docker.sh will create a shell in a docker image
with emscripten already installed if you don't want to install it locally.  The javascript decoder can be compiled using
```bash
cd subprojects/js/build
emmake cmake ..
make
```

This creates libopenjph.js and libopenjph.wasm in subprojects/js/html folder.  That html folder also has the demo webpage index.html and a compressed image test.j2c which the script in index.html decodes.  To run the demo webpage on your machine, you need a webserver running on the machine -- Due to security reasons, javascript engines running in a browser cannot access local files on the machine.  A simple python webserver can be run 
```python
python -m SimpleHTTPServer 8000
```  
from inside the html folder.  Here, 8000 is the port number at which the webserver will be listening.  The webpage can then be accessed by open 127.0.0.1:8000 in you browser.   Any browser supporting webassembly can be used to view this webpage; examples include Firefox, Chrome, Safari, and Edge, on a desktop, mobile, or tablet.

# Usage Example #

**Note**: in Kakadu, pairs of data in command line arguments represent columns,rows. Here, a pair represents x,y information.  On a different note, in reversible compression, quantization is not supported.

    ojph_compress -i input_file.ppm -o output_file.j2c -num_decomps 5 -block_size {64,64} -precincts {128,128},{256,256} -prog_order CPRL -colour_trans true -qstep 0.05
    ojph_compress -i input_file.yuv -o output_file.j2c -num_decomps 5 -reversible true -dims {3840,2160} -num_comps 3 -signed false -bit_depth 10 -downsamp {1,1},{2,2}

    ojph_expand -i input_file.j2c -o output_file.ppm
    ojph_expand -i input_file.j2c -o output_file.yuv

# Related #

The standard is available [here](https://www.itu.int/rec/T-REC-T.814/en).  It is currently free of charge; I do not know if this is temporary or permanent.

The associate site [openjph.org](https://openjph.org) serves as a blog.  It currently host the [javascript](https://openjph.org/javascript/demo.html) demo of the decoder; the webpage demonstrates that the library can be compiled to javascript, and can run inside a web-browser.  Any browser supporting webassembly can be used to view this webpage; examples include Firefox, Chrome, Safari, and Edge, on a desktop, mobile, or tablet.

