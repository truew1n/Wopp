@echo off

if not exist Build ( mkdir Build )
if not exist Build\Debug ( mkdir Build\Debug )
if not exist Build\Release ( mkdir Build\Release )

set "IncludeDirectories=-ISource\Lib -ISource\Audio"

set "AudioControllerClass=Source\Audio\AudioController.cpp"
set "AllClasses=%AudioControllerClass%"


set "MainFileDirectory=Source\Main.cu"
set "OutputDirectory=Build\Debug"

nvcc -o %OutputDirectory%/Main %MainFileDirectory% %AllClasses% -lcudart -lgdi32 -luser32 -lkernel32 -lwinmm %IncludeDirectories%

"%OutputDirectory%\Main.exe"