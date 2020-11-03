################################################################################
CMake_Minimum_Required(VERSION 2.6.3)
################################################################################

# For compiling without warnings from stupid TPL
Message(STATUS "Silence warnings from now on")

# This is to avoid Warning D9025 (see http://vtk.1045678.n5.nabble.com/A-fix-for-MSVC-warning-D9025-overriding-W1-with-w-td5734028.html)
if(MSVC)
	Set(MY_FLAGS ${MY_FLAGS} /W0)
else()
	Set(MY_FLAGS ${MY_FLAGS} -w)
endif()



Include (CheckCXXCompilerFlag)

Foreach (FLAG ${MY_FLAGS})
	Check_CXX_Compiler_Flag("${FLAG}" IS_SUPPORTED_${FLAG})
	If (IS_SUPPORTED_${FLAG})
		Set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
		Set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
	Endif ()
Endforeach ()
