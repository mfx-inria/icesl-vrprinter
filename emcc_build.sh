#!/bin/bash

build_path='build/emcc'
source_path=$PWD

if [ ! -d "$build_path" ]
then
	mkdir $build_path
	echo -e "build folder created at $source_path/$build_path"
fi

cd $build_path
emcmake cmake -DCMAKE_BUILD_TYPE=Release $source_path
emmake make icesl-webprinter
cd $source_path
