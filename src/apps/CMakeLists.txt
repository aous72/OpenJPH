
# Add tiff library 
############################################################
if( OJPH_ENABLE_TIFF_SUPPORT )

  FIND_PACKAGE( TIFF )

  if( TIFF_FOUND )
    set(USE_TIFF TRUE CACHE BOOL "Add TIFF support")
    include_directories( ${TIFF_INCLUDE_DIR} ) 
    add_definitions(-DOJPH_ENABLE_TIFF_SUPPORT)
  elseif(MSVC)
    message(STATUS "TIFF support has been enabled but no path to the TIFF library "
    "has been specified; please configure with -DCMAKE_PREFIX_PATH=<TIFF library directory>, "
    "or disable TIFF support using -DOJPH_ENABLE_TIFF_SUPPORT=OFF.")
  endif( TIFF_FOUND )

endif() 
############################################################

# Add OPENEXR library 
############################################################
if( OJPH_ENABLE_OPENEXR_SUPPORT )

  FIND_PACKAGE( OpenEXR )

  if( OpenEXR_FOUND )
    message(STATUS "Found OpenEXR ${OpenEXR_VERSION}")
    message(STATUS "      OPENEXR_INCLUDE_DIR = ${OPENEXR_INCLUDE_DIR}")
    message(STATUS "      OpenEXR_INCLUDE_DIRS = ${OpenEXR_INCLUDE_DIRS}")
    set(USE_OPENEXR TRUE CACHE BOOL "Add OpenEXR support")
    include_directories( ${OpenEXR_INCLUDE_DIRS} ) 
    add_definitions(-DOJPH_ENABLE_OPENEXR_SUPPORT)
  else()
    message(STATUS "OpenEXR support has been enabled but no path to the OpenEXR library "
    "has been specified; please configure with -DCMAKE_PREFIX_PATH=<OpenEXR library directory>, "
    "or disable OpenEXR support using -DOJPH_ENABLE_OPENEXR_SUPPORT=OFF.")
  endif( OpenEXR_FOUND )

endif() 
############################################################

## Build executables
add_subdirectory(ojph_expand)
add_subdirectory(ojph_compress)
add_subdirectory(ojph_stream_expand)
