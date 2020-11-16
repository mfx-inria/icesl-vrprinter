cmake_minimum_required(VERSION 3.0.0)

project(icesl-vrprinter)

include(UseCXX17)

#if(EMSCRIPTEN)
#  set(CMAKE_EXE_LINKER_FLAGS "-s TOTAL_MEMORY=536870912 --bind -s FORCE_FILESYSTEM=1 -s ERROR_ON_UNDEFINED_SYMBOLS=0")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --bind -std=c++17 -s FORCE_FILESYSTEM=1")
#endif(EMSCRIPTEN)

if(LIBSL_USE_GLUX)
  add_definitions(-DUSE_GLUX)
  if(NOT WIN32)
    add_definitions(-DGL_ARB_multitexture=1)  # hack due to MESA headers including ARB_multitexture
  endif(NOT WIN32)
endif(LIBSL_USE_GLUX)

ADD_DEFINITIONS(-DOPENGL)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(icesl-vrprinter
  icesl-vrprinter.h
  icesl-vrprinter.cpp

  FileDialog.h
  FileDialog.cpp

  sphere_squash.h
  shapes.h
  shapes.cpp

  bed.h
  bed.cpp

  gcode.h
  gcode.cpp

  motion.cpp
  motion.h

  #shaders
  final.h
  final.fp
  final.vp
  
  simple.h
  simple.fp
  simple.vp
  
  deposition.h
  deposition.fp
  deposition.vp  
)

AUTO_BIND_SHADERS( simple final deposition )

target_link_libraries(icesl-vrprinter
  LibSL 
  LibSL_gl
  tinyfiledialogs
)
