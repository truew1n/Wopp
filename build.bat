@echo off

if not exist Build ( mkdir Build )
if not exist Build\Debug ( mkdir Build\Debug )
if not exist Build\Release ( mkdir Build\Release )

set "AudioControllerClass=Source\Audio\AudioController.cpp"
set "AudioClasses=%AudioControllerClass%"
set "AudioIncludeDirectory=-ISource\Audio"

set "WindowClass=Source\Window\Window.cpp"
set "WindowClasses=%WindowClass%"
set "WindowIncludeDirectory=-ISource\Window"

set "DisplayComponentClass=Source\Component\DisplayComponent.cpp"
set "ComponentClasses=%DisplayComponentClass%"
set "ComponentIncludeDirectory=-ISource\Component"

set "MutexVariableClass=Source\Utility\MutexVariable.cpp"
set "UtilityClasses=%MutexVariableClass%"
set "UtilityIncludeDirectory=-ISource\Utility"

set "AllClasses=%AudioClasses% %WindowClasses% %ComponentClasses% %UtilityClasses%"

set "IncludeDirectories=-ISource\Lib %AudioIncludeDirectory% %WindowIncludeDirectory% %ComponentIncludeDirectory% %UtilityIncludeDirectory%"
set "MainFileDirectory=Source\Main.cu"

set "BuildType=Debug"

set "OutputDirectory=Build\%BuildType%"

nvcc -o %OutputDirectory%/Main %MainFileDirectory% %AllClasses% -lcudart -lgdi32 -luser32 -lkernel32 -lwinmm %IncludeDirectories%

"%OutputDirectory%\Main.exe"