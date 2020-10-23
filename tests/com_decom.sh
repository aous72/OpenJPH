#!/bin/bash

export LD_LIBRARY_PATH=`pwd`/../:`\pwd`/../../bin
export PATH=$PATH:`pwd`/../:`\pwd`/../../bin

# compress and decompress the images
kdu_compress $1
ojph_expand $2

# test PSNR and maximum error

t1=$?

rm test.jph
rm test.ppm

if [ $t1 = "0" ]; then
  exit 0
else
  exit 1
fi