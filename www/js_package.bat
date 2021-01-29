SET tool="C:\emsdk\upstream\emscripten\tools\file_packager.py"

python %tool% files.data --preload icesl.gcode default.icss --js-output=files.js
