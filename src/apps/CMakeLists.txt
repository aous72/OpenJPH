
# Add tiff library 
############################################################
if( OJPH_ENABLE_TIFF_SUPPORT )

  if( MSVC )

    set(TIFF_INCLUDE_DIR "${TIFF_PATH}\\include" CACHE PATH "the directory containing the TIFF headers")
    set(TIFF_LIBRARY_DEBUG   "${TIFF_PATH}\\lib\\tiffd.lib" CACHE FILEPATH "the path to the TIFF library for debug configurations")
    set(TIFF_LIBRARY_RELEASE "${TIFF_PATH}\\lib\\tiff.lib"  CACHE FILEPATH "the path to the TIFF library for release configurations")
    set(TIFFXX_LIBRARY_DEBUG  "${TIFF_PATH}\\lib\\tiffxxd.lib" CACHE FILEPATH "the path to the TIFFXX  library for debug configurations")
    set(TIFFXX_LIBRARY_RELEASE "${TIFF_PATH}\\lib\\tiffxx.lib" CACHE FILEPATH "the path to the TIFFXX  library for release configurations")

    message( STATUS "WIN32 detected: Setting CMakeCache TIFF values as follows, use CMake-gui Advanced to modify them" )
    message( STATUS "   TIFF_INCLUDE_DIR : \"${TIFF_INCLUDE_DIR}\"  " )
    message( STATUS "   TIFF_LIBRARY_DEBUG : \"${TIFF_LIBRARY_DEBUG}\"  " )
    message( STATUS "   TIFF_LIBRARY_RELEASE : \"${TIFF_LIBRARY_RELEASE}\"  " )
    message( STATUS "   TIFFXX_LIBRARY_DEBUG : \"${TIFFXX_LIBRARY_DEBUG}\"  " )
    message( STATUS "   TIFFXX_LIBRARY_RELEASE : \"${TIFFXX_LIBRARY_RELEASE}\"  " )

  endif( MSVC )

  FIND_PACKAGE( TIFF )

  if( TIFF_FOUND )
    set(USE_TIFF TRUE CACHE BOOL "Add TIFF support")
    include_directories( ${TIFF_INCLUDE_DIR} ) 
    add_definitions(-DOJPH_ENABLE_TIFF_SUPPORT)
  endif( TIFF_FOUND )

endif() 
############################################################

## Build executables
add_subdirectory(ojph_expand)
add_subdirectory(ojph_compress)

if (MSVC AND OJPH_ENABLE_TIFF_SUPPORT)
  if (CMAKE_BUILD_TYPE MATCHES "Release")
    file(COPY "${TIFF_PATH}\\bin\\tiff.dll" DESTINATION "${OJPH_DESTINATION_BINARY}")
    file(COPY "${TIFF_PATH}\\bin\\tiffxx.dll" DESTINATION "${OJPH_DESTINATION_BINARY}")
  elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
    file(COPY "${TIFF_PATH}\\bin\\tiffd.dll" DESTINATION "${OJPH_DESTINATION_BINARY}")
    file(COPY "${TIFF_PATH}\\bin\\tiffxxd.dll" DESTINATION "${OJPH_DESTINATION_BINARY}")
  endif()
endif()
