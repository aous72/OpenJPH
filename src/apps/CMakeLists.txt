
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

  # FIND_PACKAGE( Imath )

  # if( Imath_FOUND )
  #   message(STATUS "Found Imath ${Imath_VERSION}")
  #   message(STATUS "      Imath_INCLUDE_DIR = ${Imath_INCLUDE_DIR}")
  #   message(STATUS "      Imath_INCLUDE_DIRS = ${Imath_INCLUDE_DIRS}")
  #   message(STATUS "      Imath_LIBRARY = ${Imath_LIBRARY}")
  #   message(STATUS "      Imath_LIBRARIES = ${Imath_LIBRARIES}")
  #   include_directories( ${Imath_INCLUDE_DIR} ) 
  # else()
  #   message(STATUS "Imath support has been enabled but no path to the Imath library "
  #   "has been specified; please configure with -DCMAKE_PREFIX_PATH=<Imath library directory>, "
  #   "or disable OpenEXR support using -DOJPH_ENABLE_OPENEXR_SUPPORT=OFF.")
  # endif( Imath_FOUND )

  # FIND_PACKAGE( OpenEXR )

  # if( OpenEXR_FOUND )
  #   message(STATUS "Found OpenEXR ${OpenEXR_VERSION}")
  #   message(STATUS "      OpenEXR_INCLUDE_DIR = ${OpenEXR_INCLUDE_DIR}")
  #   message(STATUS "      OpenEXR_INCLUDE_DIRS = ${OpenEXR_INCLUDE_DIRS}")
  #   set(USE_OPENEXR TRUE CACHE BOOL "Add OpenEXR support")
  #   include_directories( ${OpenEXR_INCLUDE_DIRS} ) 
  #   add_definitions(-DOJPH_ENABLE_OPENEXR_SUPPORT)
  # else()
  #   message(STATUS "OpenEXR support has been enabled but no path to the OpenEXR library "
  #   "has been specified; please configure with -DCMAKE_PREFIX_PATH=<OpenEXR library directory>, "
  #   "or disable OpenEXR support using -DOJPH_ENABLE_OPENEXR_SUPPORT=OFF.")
  # endif( OpenEXR_FOUND )

  find_package(OpenEXR 3 CONFIG QUIET)
  if(OpenEXR_FOUND)
    message(STATUS "Found OpenEXR ${OpenEXR_VERSION}")
    set(USE_OPENEXR TRUE CACHE BOOL "Add OpenEXR support")
    add_definitions(-DOJPH_ENABLE_OPENEXR_SUPPORT)
  else()
    find_package(IlmBase)
    if(IlmBase_FOUND)
      message(STATUS "Found IlmBase ${IlmBase_VERSION}")

      find_package(OpenEXR 2)
      if(OpenEXR_FOUND)
        message(STATUS "Found OpenEXR ${OpenEXR_VERSION}")
        set(USE_OPENEXR TRUE CACHE BOOL "Add OpenEXR support")
        add_definitions(-DOJPH_ENABLE_OPENEXR_SUPPORT)
      endif()

    endif()
    
  endif()

endif() 
############################################################

## Build executables
add_subdirectory(ojph_expand)
add_subdirectory(ojph_compress)
add_subdirectory(ojph_stream_expand)
