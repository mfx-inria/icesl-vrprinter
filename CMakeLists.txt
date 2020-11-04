cmake_minimum_required(VERSION 3.0.0)

project(icesl-webprinter)

##################################
#### Clang external libraries ####
##################################
add_definitions(-DENABLE_SERVICE_MODE)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  set(CLANG_BASE_WARNINGS "-Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-private-field")
  set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CLANG_BASE_WARNINGS}" )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wpedantic")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")  
endif()

########################
#### Cmake includes ####
########################

# Enable local module file
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

# Enable gcc output coloring if available
include(UseColors)

#################
#### Options ####
#################

# GLFW
if(NOT WIN32)
  option(LIBSL_USE_GLFW "Use GLFW" ON)
#  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

option(OPENGL_DEBUG "Use GL_KHR_DEBUG for OpenGL error checking" OFF)

###################################
#### Environement preparations ####
###################################

# Emscripten preparation
if(EMSCRIPTEN)
  set(CMAKE_EXE_LINKER_FLAGS "--bind -s TOTAL_MEMORY=384MB -s FORCE_FILESYSTEM=1 -s ERROR_ON_UNDEFINED_SYMBOLS=0 -s USE_WEBGL2=1 -s WASM=1")
  set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --bind -std=c++17 -s FORCE_FILESYSTEM=1")
  set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
  if(LIBSL_USE_GLFW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3")
  endif(LIBSL_USE_GLFW)
endif(EMSCRIPTEN)

# OpenGL (Debug) preparation
if(OPENGL_DEBUG)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DOPENGL_DEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DOPENGL_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DOPENGL_DEBUG")
endif()


########################
#### Code inclusion ####
########################

# Include libraries
add_subdirectory(libs)

# Import back LibSL targets if not already present
if(NOT TARGET LibSL)
	set(ENABLE_LIBARCHIVE TRUE)
	find_package(LibSL)
endif()
add_definitions(-DASIO_STANDALONE)

# include source files
add_subdirectory(src)


#######################################
#### Compilation output management ####
#######################################

if(EMSCRIPTEN)
	set_target_properties(icesl-webprinter PROPERTIES DEBUG_POSTFIX "-d")
	set_target_properties(icesl-webprinter PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/www/")
	set_target_properties(icesl-webprinter PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/www/")
	set_target_properties(icesl-webprinter PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/www/")
	set_target_properties(icesl-webprinter PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/www/")
else(EMSCRIPTEN)
	set_target_properties(icesl-webprinter PROPERTIES DEBUG_POSTFIX "-d")
	set_target_properties(icesl-webprinter PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
	set_target_properties(icesl-webprinter PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/bin")
	set_target_properties(icesl-webprinter PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin")
	set_target_properties(icesl-webprinter PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/bin")
endif(EMSCRIPTEN)
