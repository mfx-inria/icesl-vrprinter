cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

project(icesl-webprinter)

add_subdirectory(libs)
add_subdirectory(src)

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
