#pragma once

#include <string>

/* SP 2019-03-11: Any reason for this? Won't compile with empscripten on Linux.
#ifdef EMSCRIPTEN
#define OFD_FILTER_MODELS             ""
#define OFD_FILTER_MODELS_AND_SCRIPTS ""
#define OFD_FILTER_SETTINGS           ""
#define OFD_FILTER_GCODE              ""
#define OFD_FILTER_NONE               ""

std::string openFileDialog(const char* filter);
std::string openFileDialog(const char* directory, const char* filter);
std::string saveFileDialog(const char* proposedFileNameFullPath, const char* filter);
std::string openFolderDialog(const char* proposedFolderNameFullPath);
#else
*/

#ifdef WIN32
#define OFD_FILTER_MODELS             "3D models (*.stl, *.obj, *.3ds)\0*.stl;*.obj;*.3ds\0All (*.*)\0*.*\0"
#define OFD_FILTER_MODELS_AND_SCRIPTS "Scripts & models (*.lua, *.stl, *.obj, *.3ds)\0*.lua;*.stl;*.obj;*.3ds\0All (*.*)\0*.*\0"
#define OFD_FILTER_SETTINGS           "Settings (*.xml)\0*.xml\0All (*.*)\0*.*\0"
#define OFD_FILTER_GCODE              "G-Code (*.gcode)\0*.gcode\0All (*.*)\0*.*\0"
#define OFD_FILTER_NONE               "All (*.*)\0*.*\0"
#else
#define OFD_FILTER_MODELS             std::vector<const char*>({"*.stl","*.obj","*.3ds"})
#define OFD_FILTER_MODELS_AND_SCRIPTS std::vector<const char*>({"*.lua","*.stl","*.obj","*.3ds"})
#define OFD_FILTER_SETTINGS           std::vector<const char*>({"*.xml"})
#define OFD_FILTER_GCODE              std::vector<const char*>({"*.gcode"})
#define OFD_FILTER_NONE               std::vector<const char*>({"*.*"})
#endif

#ifdef WIN32
std::string openFileDialog(const char* filter, bool changedir = true);
std::string openFileDialog(const char* directory, const char* filter, bool changedir = true);
std::string saveFileDialog(const char* proposedFileNameFullPath, const char* filter);
#else
std::string openFileDialog(std::vector<const char*> filter);
std::string openFileDialog(const char* directory, std::vector<const char*> filter);
std::string saveFileDialog(const char* proposedFileNameFullPath, std::vector<const char*> filter);
#endif

std::string openFolderDialog(const char* proposedFolderNameFullPath);

//#endif
