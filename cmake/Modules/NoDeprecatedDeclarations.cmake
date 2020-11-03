################################################################################
CMake_Minimum_Required(VERSION 2.6.3)
################################################################################

# For compiling without warnings from stupid TPL
Message(STATUS "Silence warnings about deprecated declarations")

Set(MY_FLAGS ${MY_FLAGS} -Wno-deprecated-declarations)

Include (CheckCXXCompilerFlag)

Foreach (FLAG ${MY_FLAGS})
	Check_CXX_Compiler_Flag("${FLAG}" IS_SUPPORTED_${FLAG})
	If (IS_SUPPORTED_${FLAG})
		Set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")
		Set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
	Endif ()
Endforeach ()
