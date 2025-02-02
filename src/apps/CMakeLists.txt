
# Add tiff library 
############################################################
if( OJPH_ENABLE_TIFF_SUPPORT AND (NOT EMSCRIPTEN))

  FIND_PACKAGE( TIFF )

  if( TIFF_FOUND )
    set(USE_TIFF TRUE CACHE BOOL "Add TIFF support")
    add_definitions(-DOJPH_ENABLE_TIFF_SUPPORT)
  else()
    message(WARNING "TIFF support has been requested but no path to the TIFF library "
    "has been specified; please configure with -DCMAKE_PREFIX_PATH=<TIFF library directory>, "
    "or disable TIFF support using -DOJPH_ENABLE_TIFF_SUPPORT=OFF.")
  endif( TIFF_FOUND )

endif() 
############################################################

if (EMSCRIPTEN)
  add_link_options(-sWASM=1 -sASSERTIONS=1 -sALLOW_MEMORY_GROWTH=1 -sNODERAWFS=1 -sENVIRONMENT=node -sEXIT_RUNTIME=1 -sEXCEPTION_CATCHING_ALLOWED=['fake'])
endif()

## Build executables
add_subdirectory(ojph_expand)
add_subdirectory(ojph_compress)
if (OJPH_BUILD_STREAM_EXPAND)
  add_subdirectory(ojph_stream_expand)
endif()