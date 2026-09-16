# Status #

The code is written in C++; the color and wavelet transform steps can employ SIMD instructions on Intel platforms.  SIMD instructions are also available for the block decoder (SSE3) and for the block encoder (AVX512). Other parts of the library may include SIMD in the future, for Intel and ARM; existing implementations can also be improved as there is still decent performance improvements on the table. SIMD instructions are also employed for WebAssembly (Emscripten-based), which is now widely supported in most browsers.

The encoder supports lossless and quantization-based lossy encoding.  There is currently no implementation for rate-control-based encoding.

The non-linearity point transformation (NLT) marker segment is supported for the three nonlinearities of the standard that this library implements: type 2 (LUT style), type 3 (binary complement, sometimes called SMAG), and type 4 (binary complement followed by a LUT).  Types 2 and 4 are implemented for the irreversible (9/7) wavelet only; asking for them together with the reversible (5/3) wavelet is refused when the codestream is written, and a codestream that carries that combination, which another encoder may have produced, is reported when its headers are read as an error because the decoder does not support this mode yet.

As it stands, the OpenJPH library needs documentation. The provided encoder ojph\_compress only generates HTJ2K codestreams, with the extension j2c; the generated files lack the .jph header.  Adding the .jph header is of little urgency, as the codestream contains all needed information to properly decode an image.  The .jph header will be added at a future point in time.  The provided decoder ojph\_expand decodes .jph files, by ignoring the .jph header if it is present.

The provided command line tools ojph\_compress and ojph\_expand accepts and generates .pgm, .ppm, .yuv, .raw, and .dpx. See the usage examples below.