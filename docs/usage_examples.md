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
* It came to my realization (See https://github.com/aous72/OpenJPH/issues/187) that there is an issue with files with `.raw` extension.  Kakadu and OpenJPEG use `.raw` for big-endian data and `.rawl` for little-endian data -- This is only meaningful for data samples that are more than 1 byte.  OpenJPH uses `.raw` for little-endian and there is no support for big-endian.  I need to transition to the convention adopted by Kakadu and OpenJPEG; the plan to is to support `.rawl` first, and warning that `.raw` is currently little-endian, but the plan is to move to big-endian.  Then, at a future point, the warning for `.raw` becomes that it is for big-endian. Then after a while this warning can be removed.

**Notes about byte order of files on disk:**
* Byte order on disk is independent of the machine's architecture; OpenJPH reads and writes the same bytes on little-endian and big-endian machines.
* JPEG 2000 codestreams use big-endian byte order for all markers and marker segments.
* Samples wider than one byte are little-endian in `.raw`/`.rawl` and `.yuv` files, and big-endian in 16-bit `.pgm`/`.ppm` files (per the netpbm specification).
* The `.pfm` file reader supports both little-endian and big-endian files, by default the `.pfm` writer writes little-endian files.
* DPX files declare their byte order through the magic number.
* TIFF byte order is handled by libtiff.

