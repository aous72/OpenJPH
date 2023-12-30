# Add tiff library 
############################################################
if( OJPH_ENABLE_TIFF_SUPPORT )

  if( WIN32 )

    set(TIFF_INCLUDE_DIR "C:\\Program Files\\tiff\\include" CACHE PATH "the directory containing the TIFF headers")
    set(TIFF_LIBRARY_DEBUG   "C:\\Program Files\\tiff\\lib\\tiffd.lib" CACHE FILEPATH "the path to the TIFF library for debug configurations")
    set(TIFF_LIBRARY_RELEASE "C:\\Program Files\\tiff\\lib\\tiff.lib"  CACHE FILEPATH "the path to the TIFF library for release configurations")
    set(TIFFXX_LIBRARY_DEBUG  "C:\\Program Files\\tiff\\lib\\tiffxxd.lib" CACHE FILEPATH "the path to the TIFFXX  library for debug configurations")
    set(TIFFXX_LIBRARY_RELEASE "C:\\Program Files\\tiff\\lib\\tiffxx.lib" CACHE FILEPATH "the path to the TIFFXX  library for release configurations")

    message( STATUS "WIN32 detected: Setting CMakeCache TIFF values as follows, use CMake-gui Advanced to modify them" )
    message( STATUS "   TIFF_INCLUDE_DIR : \"${TIFF_INCLUDE_DIR}\"  " )
    message( STATUS "   TIFF_LIBRARY_DEBUG : \"${TIFF_LIBRARY_DEBUG}\"  " )
    message( STATUS "   TIFF_LIBRARY_RELEASE : \"${TIFF_LIBRARY_RELEASE}\"  " )
    message( STATUS "   TIFFXX_LIBRARY_DEBUG : \"${TIFFXX_LIBRARY_DEBUG}\"  " )
    message( STATUS "   TIFFXX_LIBRARY_RELEASE : \"${TIFFXX_LIBRARY_RELEASE}\"  " )

  endif( WIN32 )

  FIND_PACKAGE( TIFF )

  if( TIFF_FOUND )
    set(USE_TIFF TRUE CACHE BOOL "Add TIFF support")
    include_directories( ${TIFF_INCLUDE_DIR} ) 
    if (MSVC)
		  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D \"OJPH_ENABLE_TIFF_SUPPORT\"")
	  else()
		  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOJPH_ENABLE_TIFF_SUPPORT")
	  endif()
    #include_directories(${CMAKE_BINARY_DIR}/libtiff) # for tiffconf.h on windows
  endif( TIFF_FOUND )

endif() 
############################################################

## Change folders
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/../lib)

## Build executables
add_subdirectory(ojph_expand)
add_subdirectory(ojph_compress)