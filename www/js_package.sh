#!/bin/bash

#tool="$EMSDK/fastcomp/emscripten/tools/file_packager.py" # warning: fastcomp is deprecated for newer version of emsk
tool="$EMSDK/upstream/emscripten/tools/file_packager.py"


python3 $tool files.data --preload icesl.gcode --js-output=files.js
