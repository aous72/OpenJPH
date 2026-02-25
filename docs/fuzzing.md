# Fuzzer Target #

Fuzzer targets intended for use with [oss-fuzz](https://oss-fuzz.com/) can be build using the `OJPH_BUILD_FUZZER` build option.

The targets can be run locally as follows:

```sh
cd build
cmake .. -DOJPH_BUILD_FUZZER=ON
./fuzzing/ojph_expand_fuzz_target <test case>
```
