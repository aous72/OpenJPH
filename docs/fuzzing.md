# Fuzzer Target #

Fuzzer targets can be build using the `OJPH_BUILD_FUZZER` build option. The Dockerfile in the `fuzzing directory` allows local testing:

```sh
podman build -t openjph-fuzz  -f fuzzing/Dockerfile
podman run -it --rm -v $(pwd):/app/ojph/ openjph-fuzz bash
image# mkdir /app/build/
image# cd /app/build/
image# cmake /app/ojph -DCMAKE_CXX_FLAGS="-fsanitize=fuzzer,address" -DOJPH_BUILD_FUZZER=ON -DBUILD_SHARED_LIBS=OFF
image# make
image# ./fuzzing/ojph_expand_fuzz_target /app/jp2k_test_codestreams/openjph/*.j2c
```
