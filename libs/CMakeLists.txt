cmake_minimum_required(VERSION 3.0.0)

###############################
# tinyfiledialogs integration #
###############################
set(TINYFD_SOURCES
    tinyfiledialogs/tinyfiledialogs.c
)

set(TINYFD_HEADERS
    tinyfiledialogs/tinyfiledialogs.h
)

add_library(tinyfiledialogs STATIC ${TINYFD_SOURCES} ${TINYFD_HEADERS})
set_property(
  TARGET tinyfiledialogs
  APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/tinyfiledialogs
)

set_target_properties(tinyfiledialogs PROPERTIES DEBUG_POSTFIX "-d")

#####################
# LibSL integration #
#####################
add_subdirectory(LibSL)
