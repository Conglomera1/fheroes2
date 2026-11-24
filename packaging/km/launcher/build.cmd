@echo off
setlocal
cd /d "%~dp0"
if not exist build mkdir build
rc /nologo /fo build\launcher.res launcher.rc
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /utf-8 /W4 /O2 /MT /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0601 /DWINVER=0x0601 /Fo:build\ /Fe:build\KnightModLauncher.exe launcher.cpp build\launcher.res /link /SUBSYSTEM:WINDOWS,6.01 /MACHINE:X86 /MANIFEST:NO user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib uuid.lib
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /utf-8 /W4 /MT /Fo:build\ /Fe:build\settings-test.exe settings_test.cpp /link /MACHINE:X86
if errorlevel 1 exit /b 1
build\settings-test.exe
exit /b %ERRORLEVEL%
