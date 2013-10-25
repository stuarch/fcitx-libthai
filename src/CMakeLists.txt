include_directories(${LIBTHAI_INCLUDE_DIRS})
link_directories(${LIBTHAI_LIBRARY_DIRS})
# Source files
set(fcitx_libthai_sources
  libthai-config.c
  libthai.c
  thaikb.c
  )

# See document in FcitxMacro.cmake and the example .fxaddon file
# doc/fcitx-example.fxaddon in the fcitx repo for more information.
fcitx_add_addon_full(libthai
  IM_CONFIG libthai.conf
  DESC # Use configdesc file
  SCAN # Generate api header
  SCAN_PRIV # Generate addfunction header
  # HEADER_DIR alternative_name # change dir name your header is installed
  # LIB_NAME lib_name # module file name (without extension)
  # HEADERS headers.h # extra public headers to install
  # EXTRA_PO extra_files # extra files to translate
  LINK_LIBS ${LIBTHAI_LIBRARIES} # external libraies to link
  SOURCES ${fcitx_libthai_sources})
