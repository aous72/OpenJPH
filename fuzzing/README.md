# Fuzzer Target #

Fuzzer targets can be build using the `OJPH_BUILD_FUZZER` build option. The Dockerfile allows local testing:

```sh
podman build -t openjph-fuzz  -f fuzzing/Dockerfile
podman run -it --rm -v $(pwd):/app/ojph/ openjph-fuzz bash
image# mkdir /app/build/
image# cd /app/build/
image# cmake /app/ojph -DOJPH_BUILD_FUZZER=ON -DBUILD_SHARED_LIBS=OFF
image# make
image# ./fuzzing/j2c_expand_fuzz_target /app/jp2k_test_codestreams/openjph/*.j2c
```
