@echo off
setlocal
cd /d "%~dp0"
if not exist build mkdir build
set "KM_LAUNCHER_EXE=KnightModLauncher.exe"
if /i "%~1"=="KMSettings.exe" set "KM_LAUNCHER_EXE=KMSettings.exe"
rc /nologo /fo build\launcher.res launcher.rc
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /utf-8 /W4 /O2 /MT /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0601 /DWINVER=0x0601 /Fo:build\ /Fe:build\%KM_LAUNCHER_EXE% launcher.cpp settings_files.cpp build\launcher.res /link /SUBSYSTEM:WINDOWS,6.01 /MACHINE:X86 /MANIFEST:NO user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib uuid.lib
if errorlevel 1 exit /b 1
dumpbin /nologo /imports build\%KM_LAUNCHER_EXE% > build\launcher-imports.txt
if errorlevel 1 exit /b 1
findstr /r /c:" CreateFile2 *$" build\launcher-imports.txt > nul
if not errorlevel 1 (
    echo Windows 7 compatibility regression: %KM_LAUNCHER_EXE% imports CreateFile2.
    exit /b 1
)
cl /nologo /EHsc /std:c++20 /utf-8 /W4 /MT /Fo:build\ /Fe:build\settings-test.exe settings_test.cpp /link /MACHINE:X86
if errorlevel 1 exit /b 1
build\settings-test.exe
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /utf-8 /W4 /MT /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0601 /DWINVER=0x0601 /Fo:build\ /Fe:build\settings-files-test.exe settings_files_test.cpp settings_files.cpp /link /MACHINE:X86 shell32.lib ole32.lib
if errorlevel 1 exit /b 1
build\settings-files-test.exe
exit /b %ERRORLEVEL%
