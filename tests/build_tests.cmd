@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d "%~dp0"
cl /nologo /std:c++17 /EHsc /W4 /MT "..\source\algorithms.cpp" /Fe:milestone3.exe /Fo:milestone3.obj
