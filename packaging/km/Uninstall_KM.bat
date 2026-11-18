@echo off
setlocal EnableExtensions DisableDelayedExpansion
title Knight Mod Public Beta uninstaller
echo This removes only the public Knight Mod files from this folder.
echo Original game files, saved games and your active knight_mod.ini are kept.
echo.
if not exist "%~dp0fheroes2KM.exe" (
    echo Place this batch in the Knight Mod installation folder.
    pause
    exit /b 1
)
choice /c YN /n /m "Uninstall Knight Mod? [Y/N] "
if errorlevel 2 exit /b 0
set "KM_FAILED=0"
for %%F in ("fheroes2KM.exe" "files\data\resurrectionKM.h2d" "files\data\training_grounds.png" "files\data\build_training_grounds.png" "knight_mod.ini.default" "KM_SOURCE_COMMIT.txt" "KM_BUILD.txt") do call :remove "%~dp0%%~F"
for %%L in (be bg cs da de el eo es fr hu it lt nb nl pl pt ro ru sk sv tr uk vi) do call :remove "%~dp0files\lang\km_%%L.mo"
if "%KM_FAILED%"=="1" (
    echo Some mod files could not be removed. Close the game and retry.
    pause
    exit /b 1
)
echo Knight Mod removed. Your configuration and saves were preserved.
pause
endlocal & del /q "%~f0" & exit /b 0
:remove
if exist "%~1" del /f /q "%~1"
if exist "%~1" set "KM_FAILED=1"
exit /b 0
