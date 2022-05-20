/home/aous/wasi-sdk-15.0/bin/clang++ -msimd128 --sysroot=/home/aous/wasi-sdk-15.0/share/wasi-sysroot/ \
  -D_WASI_EMULATED_PROCESS_CLOCKS -lwasi-emulated-process-clocks \
  -D OJPH_DISABLE_INTEL_SIMD -D OJPH_EMSCRIPTEN -D OJPH_ENABLE_WASM_SIMD \
  -I../src/core/common -I../src/apps/common\
  ../src/core/codestream/ojph_codestream.cpp ../src/core/codestream/ojph_codestream_wasm.cpp ../src/core/codestream/ojph_params.cpp \
  ../src/core/coding/ojph_block_common.cpp ../src/core/coding/ojph_block_encoder.cpp ../src/core/coding/ojph_block_decoder.cpp ../src/core/coding/ojph_block_decoder_wasm.cpp \
  ../src/core/others/ojph_arch.cpp ../src/core/others/ojph_file.cpp ../src/core/others/ojph_mem.cpp ../src/core/others/ojph_message.cpp \
  ../src/core/transform/ojph_colour.cpp ../src/core/transform/ojph_colour_wasm.cpp ../src/core/transform/ojph_transform.cpp ../src/core/transform/ojph_transform_wasm.cpp \
  ../src/apps/others/ojph_img_io.cpp \
  ../src/apps/ojph_compress/ojph_compress.cpp \
  -o ojph_compress_simd.wasm
  
# ~/.wasmer/bin/wasmer create-exe --llvm --enable-simd ojph_compress_simd.wasm -o wcompress_simd
  
/home/aous/wasi-sdk-15.0/bin/clang++ -msimd128 --sysroot=/home/aous/wasi-sdk-15.0/share/wasi-sysroot/ \
  -D_WASI_EMULATED_PROCESS_CLOCKS -lwasi-emulated-process-clocks \
  -D OJPH_DISABLE_INTEL_SIMD -D OJPH_EMSCRIPTEN -D OJPH_ENABLE_WASM_SIMD \
  -I../src/core/common -I../src/apps/common\
  ../src/core/codestream/ojph_codestream.cpp ../src/core/codestream/ojph_codestream_wasm.cpp ../src/core/codestream/ojph_params.cpp \
  ../src/core/coding/ojph_block_common.cpp ../src/core/coding/ojph_block_encoder.cpp ../src/core/coding/ojph_block_decoder.cpp ../src/core/coding/ojph_block_decoder_wasm.cpp \
  ../src/core/others/ojph_arch.cpp ../src/core/others/ojph_file.cpp ../src/core/others/ojph_mem.cpp ../src/core/others/ojph_message.cpp \
  ../src/core/transform/ojph_colour.cpp ../src/core/transform/ojph_colour_wasm.cpp ../src/core/transform/ojph_transform.cpp ../src/core/transform/ojph_transform_wasm.cpp \
  ../src/apps/others/ojph_img_io.cpp \
  ../src/apps/ojph_expand/ojph_expand.cpp \
  -o ojph_expand_simd.wasm

# ~/.wasmer/bin/wasmer create-exe --llvm --enable-simd ojph_expand_simd.wasm -o wexpand_simd

/home/aous/wasi-sdk-15.0/bin/clang++ --sysroot=/home/aous/wasi-sdk-15.0/share/wasi-sysroot/ \
  -D_WASI_EMULATED_PROCESS_CLOCKS -lwasi-emulated-process-clocks \
  -D OJPH_DISABLE_INTEL_SIMD -DOJPH_EMSCRIPTEN \
  -I../src/core/common -I../src/apps/common\
  ../src/core/codestream/ojph_codestream.cpp ../src/core/codestream/ojph_params.cpp \
  ../src/core/coding/ojph_block_common.cpp ../src/core/coding/ojph_block_encoder.cpp ../src/core/coding/ojph_block_decoder.cpp \
  ../src/core/others/ojph_arch.cpp ../src/core/others/ojph_file.cpp ../src/core/others/ojph_mem.cpp ../src/core/others/ojph_message.cpp \
  ../src/core/transform/ojph_colour.cpp ../src/core/transform/ojph_transform.cpp \
  ../src/apps/others/ojph_img_io.cpp \
  ../src/apps/ojph_compress/ojph_compress.cpp \
  -o ojph_compress.wasm
  
# ~/.wasmer/bin/wasmer create-exe --llvm ojph_compress.wasm -o wcompress
  
/home/aous/wasi-sdk-15.0/bin/clang++ --sysroot=/home/aous/wasi-sdk-15.0/share/wasi-sysroot/ \
  -D_WASI_EMULATED_PROCESS_CLOCKS -lwasi-emulated-process-clocks \
  -D OJPH_DISABLE_INTEL_SIMD -DOJPH_EMSCRIPTEN \
  -I../src/core/common -I../src/apps/common\
  ../src/core/codestream/ojph_codestream.cpp ../src/core/codestream/ojph_params.cpp \
  ../src/core/coding/ojph_block_common.cpp ../src/core/coding/ojph_block_encoder.cpp ../src/core/coding/ojph_block_decoder.cpp \
  ../src/core/others/ojph_arch.cpp ../src/core/others/ojph_file.cpp ../src/core/others/ojph_mem.cpp ../src/core/others/ojph_message.cpp \
  ../src/core/transform/ojph_colour.cpp ../src/core/transform/ojph_transform.cpp \
  ../src/apps/others/ojph_img_io.cpp \
  ../src/apps/ojph_expand/ojph_expand.cpp \
  -o ojph_expand.wasm

# ~/.wasmer/bin/wasmer create-exe --llvm ojph_expand.wasm -o wexpand
