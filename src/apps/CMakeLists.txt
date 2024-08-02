
# Add tiff library 
############################################################
if( OJPH_ENABLE_TIFF_SUPPORT )

  FIND_PACKAGE( TIFF )

  if( TIFF_FOUND )
    set(USE_TIFF TRUE CACHE BOOL "Add TIFF support")
    include_directories( ${TIFF_INCLUDE_DIR} ) 
    add_definitions(-DOJPH_ENABLE_TIFF_SUPPORT)
  elseif(MSVC)
    message(STATUS "TIFF support has been enabled by no path to the TIFF library "
    "has been specified; please configure with -DCMAKE_PREFIX_PATH=<TIFF library directory>, "
    "or disable TIFF support using -DOJPH_ENABLE_TIFF_SUPPORT=OFF.")
  endif( TIFF_FOUND )

endif() 
############################################################

## Build executables
add_subdirectory(ojph_expand)
add_subdirectory(ojph_compress)
add_subdirectory(ojph_stream_expand)
