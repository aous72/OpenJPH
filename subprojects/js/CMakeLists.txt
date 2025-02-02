cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_SYSTEM_NAME Generic)

project (OpenJPH_WASM DESCRIPTION "Open source implementation of JPH" LANGUAGES CXX)

add_subdirectory("../.." openjph EXCLUDE_FROM_ALL)
add_executable(libopenjph "src/ojph_wrapper.cpp")
if (OJPH_DISABLE_SIMD)
else()
  target_compile_options(libopenjph PRIVATE -DOJPH_ENABLE_WASM_SIMD -msimd128)
endif()
set_target_properties(libopenjph PROPERTIES SUFFIX ".js")
target_link_options(libopenjph PRIVATE
  -fexceptions
  -sWASM=1
  -sEXPORT_ES6=1
  -sMODULARIZE=1
  -sENVIRONMENT=web
  -sEXPORTED_FUNCTIONS=[_free,_malloc]
  -sEXPORTED_RUNTIME_METHODS=[ccall,cwrap,writeArrayToMemory]
  -sNO_EXIT_RUNTIME=1
  -sALLOW_MEMORY_GROWTH=1
  -sINITIAL_MEMORY=134217728
)
target_link_libraries(libopenjph PRIVATE openjph)

