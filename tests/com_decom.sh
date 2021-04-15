#!/bin/bash

export LD_LIBRARY_PATH=`pwd`/../:`\pwd`/../../bin
export PATH=$PATH:`pwd`/../:`\pwd`/../../bin

# compress and decompress the images
kdu_compress $1
kdu_expand $2
ojph_expand $3

# test PSNR and maximum error
pwd
out1=$(../psnr_pae $4 $5)
out2=$(../psnr_pae $4 $6)

psnr1=$(echo $out1 | cut -f1 -d' ')
psnr2=$(echo $out2 | cut -f1 -d' ')
d1=$(bc -l <<< "$psnr1 - $psnr2")
d1=${d1##*[+-]}
pae1=$(echo $out1 | cut -f2 -d' ')
pae2=$(echo $out2 | cut -f2 -d' ')
d2=$(($pae1 - $pae2))
d2=${d2##*[+-]}

rm $5 $6

# echo $d1 $d2

exit `bc -l <<< "$d1 > 0.01 || $d2 > 1"`