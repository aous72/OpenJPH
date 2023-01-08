# Test Helpers

This folder has some code that helps with generating tests.

Currently it has

# convert_mse_pae_to_test.cpp
This file can be compiled using

``` g++ -g3 convert_mse_pae_to_tests.cpp -o convert_mse_pae_to_tests ```

The generated executable uses ```mse_pae.txt``` in the
```jp2k_test_streams/openjph``` folder and ```ht_cmdlines.txt``` in this folder
to generate the ```openjph_tests.cpp``` file. The ```openjph_tests.cpp``` file 
contains all the googletest tests from those original files, together with some 
comments.  The content of ```openjph_tests.cpp``` needs to be copied and pasted 
in ```../test_executables.cpp```, because googletest does not support 
included files.

# Other files can added in the future.  