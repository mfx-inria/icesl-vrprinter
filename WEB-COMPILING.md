1. Install Emscripten [from here](https://emscripten.org/docs/getting_started/downloads.html)

2. Make sure Emscripten is [working properly](https://emscripten.org/docs/building_from_source/verify_emscripten_environment.html#verifying-the-emscripten-environment) 
> On Windows, you may need to add the following locations to you PATH:
> - **[install_location]/emsdk**
> - **[install_location]/emsdk/upstream/emscripten**
> - [install_location]/emsdk/node/[version_number]/bin
> - [install_location]/emsdk/python/[version_number]
> - [install_location]/emsdk/java/[version_number]/bin
> 
> You will also need [MinGW](http://www.mingw.org/wiki/Getting_Started#toc1), and to [add it properly to the PATH](http://www.mingw.org/wiki/Getting_Started#toc7)

3. Compile VR-printer:
```shell
  mkdir [path/to/build]/emcc
  cd [path/to/build]/emcc
  emcmake cmake -DCMAKE_BUILD_TYPE=Release [path/to/source]
  emmake make IceSL-webprinter
  cd [path/to/source]/www
  ./js_package.sh #or js_package.bat on Windows
```
4. Deploying VR-printer:
Copy the following files to **$VRPRINTER_WEBSITE/**:
  - auth
  - Blob.js
  - bootstrap.js
  - FileSaver.js
  - fileselect.js
  - jquery-3.3.1.min.js
  - LICENSE.Blob.md
  - LICENSE.FileSave.md
  - files.data *
  - files.js *
  - IceSL-webprinter.js *
  - IceSL-webprinter.wasm *
> (*) mandatory when redeploying

5. FAST COMPILATION (at least one past succesfull compilation needed):
```shell
  cd [path/to/build]/emcc 
  emcmake cmake -DCMAKE_BUILD_TYPE=Release [path/to/source]
  emmake make IceSL-webprinter
  cd [path/to/source]/www
  ./js_package.sh # or js_package.bat on Windows
```
