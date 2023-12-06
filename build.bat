@echo off

REM Change this to your visual studio's 'vcvars64.bat' script path
set MSVC_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build"

set CXXFLAGS=/std:c++17 /EHsc /W4 /WX /FC /MT /wd4996 /wd4201 /nologo %*
set INCLUDES=/I"deps\include"
set LIBS="deps\lib\raylib\raylib.lib" opengl32.lib kernel32.lib user32.lib gdi32.lib winmm.lib shell32.lib

call %MSVC_PATH%\vcvars64.bat

pushd %~dp0
if not exist .\build mkdir build
cl %CXXFLAGS% %INCLUDES% "code\main.cpp" /Fo:build\ /Fe:build\maidai.exe /link %LIBS% /SUBSYSTEM:CONSOLE /NODEFAULTLIB:libcmt.lib

cd build
del *.obj
cd ..
popd