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

