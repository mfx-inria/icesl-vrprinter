# IceSL-vrprinter

IceSL-vrprinter is a Gcode vizualizer and simulator.

[![IceSL-vrprinter main view][sim]][vrprint]

Created by members of the team behind IceSL, IceSL-vrprinter aims to provide a way to preview, simulate and create statitics on any provided Gcode.
In addition to these principles, the web version is designed to feature a text editor, to follow in real time the simulation advancement, and to edit it.

The web version of this tool is [currently deployed][vrprint] and in use on [IceSL's website][icesl].

## Features
- Automatic Gcode flavor detection, to support most of Gcodes.
- Multi-extrusion and extruders offsets support.
- Basic statistics visualization and export (layers overhang and overlap).
- Real-time Gcode editing (web version).

## Building and compiling IceSL-vrprinter
To build IceSL-vrprinter, you will need the following components installed on your computer:
- git
- CMake
- C++ standard libraries
- Any IDE supporting C++ (on Windows, [Visual Studio][vs] is recommended)

To build IceSL-vrprinter on Linux, you will also need glfw3.
```Shell
sudo apt-get install libglfw3-dev
```

Once the required components installed, you can fetch the source code by cloning or [downloading a .zip][zip] of the source files.
```Shell
git clone https://github.com/mfx-inria/icesl-vrprinter.git
```

Initialize and fetch the submodules dependencies.
```Shell
git submodule update --init --recursive

git submodule update --recursive --remote	# with git 1.8.2 or above
# or
git submodule update --recursive	# with git 1.7.3 or above
```

Then you can, if you want, fetch the latest changes of each submodule.
```Shell
git pull --recurse-submodules
```

You can now build the project for your IDE of choice using CMAKE, and then compile.

## Emscriten build
With the Emscriten tools properly installed on your system, you can use these helper scripts:
- `emcc_build.bat` for Windows.
- `emcc_build.sh` for Linux.

>**Note:**
>These helper scripts assumes that your `build` folder is directly at the root of the project.
>If your `build` folder is located elsewhere, modify the helper script accordingly.
>For the Windows script, it is also assumed that Emscripten is available in the PATH.

Once the Emscriten build and compilation done, you have to execute the packaging script:
- `www/js_package.bat` for Windows.
- `www/js_package.sh` for Linux.

This script will bundle up `default.icss` (theming file) and `icesl.gcode` (the example/preview gcode) into `files.data`.

You can then copy the `www` folder in you webserver of choice

>**Note:**
>You can copy all of the `www` folder content, exept the packaging scripts and the packaged files.

A more detailed procedure can be found in [WEB-COMPILING.md][webcomp]

## Contribute to IceSL-vrprinter
### New features or reworking the project
Create your own fork of the project, and submit a [pull request][pr]. 
>**Note:**
>When contributing using a pull request, don't hesitate to be precise in the description of your contribution, and don't forget to document what you did.
>It will greatly help us to integrate your changes! 

### Bug reports and feature requests
If you encounter a bug, or have an idea for a new feature, please fill an [issue][issues] so we can discuss about it.
>**Note:**
>Please be as precise as possible when filling an issue. For bug reports, please join some scripts/files that reproduce the problem.
>Using a label on your Issue will greatly help us tracking and processing them.

### C++ Style Guide
You can follow the [Google C++ Style Guide][codestyle] for new code.

# Credits
## Developpers:
- Sylvain Lefebvre [[@sylefeb](https://github.com/sylefeb)] 
- Pierre Bedell [[@Phazon54](https://github.com/Phazon54)] 
- Salim Perchy [[@ysperchy](https://github.com/ysperchy)]


## External libraries and tools used:
- [LibSL](https://github.com/sylefeb/LibSL)
- [dear imgui](https://github.com/ocornut/imgui)
- [tinyfiledialogs](https://github.com/native-toolkit/tinyfiledialogs)
- [Emscripten](https://emscripten.org/index.html)
- [ace](https://ace.c9.io/)
- [bootstrap](https://getbootstrap.com/)
- [FileSaver.js](https://github.com/eligrey/FileSaver.js/)

## License
[Affero GPL 3.0](https://www.gnu.org/licenses/agpl-3.0.html)

[//]: # (Links)
[vrprint]: https://icesl.loria.fr/webprinter/
[icesl]: https://icesl.loria.fr/
[vs]: https://visualstudio.microsoft.com
[codestyle]: https://google.github.io/styleguide/cppguide.html
[zip]: https://github.com/mfx-inria/icesl-vrprinter/archive/refs/heads/master.zip
[pr]: https://github.com/mfx-inria/icesl-vrprinter/pulls
[issues]: https://github.com/mfx-inria/icesl-vrprinter/issues

[//]: # (Ressources)
[sim]: ressources/vrprinter.gif
[webcomp]: WEB-COMPILING.md
