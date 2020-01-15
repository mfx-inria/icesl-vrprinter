CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
Project(icesl-vrprint)

include(UseCXX11)

IF(EMSCRIPTEN)
SET(CMAKE_EXE_LINKER_FLAGS "-s TOTAL_MEMORY=536870912 --bind -s FORCE_FILESYSTEM=1")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --bind -std=c++11 -s FORCE_FILESYSTEM=1")
ENDIF(EMSCRIPTEN)

IF (LIBSL_USE_GLUX)
ADD_DEFINITIONS(-DUSE_GLUX)
IF(NOT WIN32)
ADD_DEFINITIONS(-DGL_ARB_multitexture=1)  # hack due to MESA headers including ARB_multitexture
ENDIF(NOT WIN32)
ENDIF (LIBSL_USE_GLUX)

ADD_DEFINITIONS(-DOPENGL)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
  )

ADD_EXECUTABLE(icesl-vrprint
  icesl-vrprint.cpp
  icesl-vrprint.h
  sphere_squash.h
  shapes.cpp
  shapes.h
  bed.cpp
  bed.h
  gcode.cpp
  gcode.h
  motion.cpp
  motion.h
  final.fp
  final.vp
  final.h
  simple.fp
  simple.vp
  simple.h
  deposition.fp
  deposition.vp
  deposition.h  
)

AUTO_BIND_SHADERS( simple final deposition )

target_link_libraries(icesl-vrprint
  LibSL 
  LibSL_gl
  libcore
  )
 