#!/bin/bash

cd build
cmake ..
make
make test
cd ..

