# Readme #

Open source implementation of High-throughput JPEG2000 (HTJ2K), also known as JPH, JPEG2000 Part 15, ISO/IEC 15444-15, and ITU-T.814. Here, we are interested in implementing the HTJ2K only, supporting features that are defined in JPEG2000 Part 1 (for example, for wavelet transform, only reversible 5/3 and irreversible 9/7 are supported).

The interested reader is refered to [HTJ2K white paper](https://kakadusoftware.com/wp-content/uploads/2019/09/HTJ2K-White-Paper.pdf) for more details about HTJ2K.

# Status #

The code is written entirely in C++, but it conceivable that at some point in the future, SIMD instructions are employed to improve coding speed.  As it stands, on the quad core i7-6700, the code can encode 4K 4:4:4 HDR images losslessly in around 0.4-0.5s; for lossy compression, performance depends on the quantization step size (qstep), and it is between 0.25-0.55s for a similar image. 

As it stands, the OpenJPH library needs some documentation, and a more propor handling of errors. The provided encoder ojph\_compress only generates HTJ2K codestreams, with the extension j2c; the generated files lack the .jph header.  Adding the .jph header is of little urgency, as the codestream contains all needed information to properly decode an image.  The .jph header will be added at a future point in time.  The provided decoder ojph\_expand ignores the .jph header if it is present.

The provided command line tools ojph\_compress and ojph\_expand accepts and generated .pgm, .ppm, and .yuv. See the usage examples below.

# Usage Example #
**Note**: in Kakadu, pairs of data in command line arguments represent columns,rows. Here, a pair represents x,y information.  On a different note, in reversible compression, quantization is not supported.

    ojph_compress -i input_file.ppm -o output_file.j2c -num_decomps 5 -block_size {64,64} -precincts {128,128},{256,256} -prog_order CPRL -colour_trans true -qstep 0.05
    ojph_compress -i input_file.yuv -o output_file.j2c -num_decomps 5 -reversible true -dims {3840,2160} -num_comps 3 -signed false -bit_depth 10 -downsamp {1,1},{2,2}

    ojph_expand -i input_file.j2c -o output_file.ppm
    ojph_expand -i input_file.j2c -o output_file.yuv


