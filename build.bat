@echo off

if not exist Debug ( mkdir Debug )

set "AudioControllerClass=AudioController.cpp"

set "AllClasses=%AudioControllerClass%"

nvcc -o ./Debug/Main Main.cu %AllClasses% -lcudart -lgdi32 -luser32 -lkernel32 -lwinmm

"Debug/Main.exe"