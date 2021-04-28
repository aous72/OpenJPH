#!/bin/bash

export LD_LIBRARY_PATH=`pwd`/../:`\pwd`/../../bin
export PATH=$PATH:`pwd`/../:`\pwd`/../../bin

# compress and decompress the images
if [ "$1" = "-dec" -o "$1" = "-rdec"  ]; then
  if ! kdu_compress $2; then
    exit 1
  fi
elif [ "$1" = "-com" -o "$1" = "-rcom" ]; then
  if ! ojph_compress $2; then
    exit 1
  fi
else
  exit 1
fi
if ! kdu_expand $3; then
  exit 1
fi
if ! ojph_expand $4; then
  exit 1
fi

# test PSNR and maximum error
out1=$(../psnr_pae $5 $6)
if [ $? -ne 0 ]; then
  exit 1
fi
out2=$(../psnr_pae $5 $7)
if [ $? -ne 0 ]; then
  exit 1
fi

rm $6 $7

psnr1=$(echo $out1 | cut -f1 -d' ')
psnr2=$(echo $out2 | cut -f1 -d' ')
d1=$(bc -l <<< "$psnr1 - $psnr2")
d1=${d1##*[+-]}

pae1=$(echo $out1 | cut -f2 -d' ')
pae2=$(echo $out2 | cut -f2 -d' ')
d2=$(($pae1 - $pae2))
d2=${d2##*[+-]}

if [ "$1" = "-rcom" -o "$1" = "-rdec" ]; then
  if [ "$psnr1" = "inf" -a "$psnr2" = "inf" -a "$pae1" = "0" -a "$pae2" = "0" ]; then
    exit 0
  else
    exit 1
  fi
else
  exit $(bc -l <<< "$d1 > 0.01 || $d2 > 1")
fi