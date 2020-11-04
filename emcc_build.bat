@ECHO OFF

SET build_path=build\emcc

REM Get current path as source path
SET source_path=%~dp0

IF NOT EXIST "%build_path%" (
	MKDIR "%build_path%" 
	ECHO build folder created at "%source_path%%build_path%"
)

CD %build_path%

CALL emcmake cmake -DCMAKE_BUILD_TYPE=Release "%source_path%"
CALL emmake make icesl-webprinter

CD %source_path%
