@echo off

if not exist Build ( mkdir Build )
if not exist Build\Debug ( mkdir Build\Debug )
if not exist Build\Release ( mkdir Build\Release )

set "AudioControllerClass=Source\Audio\AudioController.cpp"
set "AudioDriverClass=Source\Audio\AudioDriver.cpp"
set "AudioClasses=%AudioControllerClass% %AudioDriverClass%"
set "AudioID=-ISource\Audio"

set "ThreadClass=Source\Thread\Thread.cpp"
set "MutexClass=Source\Thread\Mutex.cpp"
set "EventClass=Source\Thread\Event.cpp"
set "ThreadClasses=%ThreadClass% %MutexClass% %EventClass%"
set "ThreadID=-ISource\Thread"

set "AllClasses=%AudioClasses% %ThreadClasses%"

set "IncludeDirectories=-ISource\Lib %AudioID% %ThreadID%"
set "MainFileDirectory=Source\Main.cpp"

set "BuildType=Debug"

set "OutputDirectory=Build\%BuildType%"

g++ -o %OutputDirectory%/Main %MainFileDirectory% %AllClasses% -lglfw3 -lglew32 -lopengl32 -lwinmm %IncludeDirectories%

"%OutputDirectory%\Main.exe"