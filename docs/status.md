# Status #

The code is written in C++; the color and wavelet transform steps can employ SIMD instructions on Intel platforms.  SIMD instructions are also available for the block decoder (SSE3) and for the block encoder (AVX512). Other parts of the library may include SIMD in the future, for Intel and ARM; existing implementations can also be improved as there is still decent performance improvements on the table. SIMD instructions are also employed for WebAssembly (Emscripten-based), which is now widely supported in most browsers.

The encoder supports lossless and quantization-based lossy encoding.  There is currently no implementation for rate-control-based encoding.

As it stands, the OpenJPH library needs documentation. The provided encoder ojph\_compress only generates HTJ2K codestreams, with the extension j2c; the generated files lack the .jph header.  Adding the .jph header is of little urgency, as the codestream contains all needed information to properly decode an image.  The .jph header will be added at a future point in time.  The provided decoder ojph\_expand decodes .jph files, by ignoring the .jph header if it is present.

The provided command line tools ojph\_compress and ojph\_expand accepts and generates .pgm, .ppm, .yuv, .raw, and .dpx. See the usage examples below.