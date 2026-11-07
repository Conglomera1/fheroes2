@echo off
setlocal EnableExtensions
title Knight Mod uninstaller
echo This removes Knight Mod files from this folder and its settings.
echo Your original fheroes2 files and saved games will be kept.
echo knight_mod.ini is shared with other Knight Mod / PTP versions.
echo.
if not exist "%~dp0fheroes2KM.exe" if not exist "%~dp0files\data\resurrectionKM.h2d" (
    echo Place this batch file in the folder where Knight Mod is installed.
    pause
    exit /b 1
)
choice /c YN /n /m "Uninstall Knight Mod? [Y/N] "
if errorlevel 2 exit /b 0
set "KM_FAILED=0"
for %%F in ("%~dp0fheroes2KM.exe" "%~dp0files\data\resurrectionKM.h2d" "%~dp0files\data\training_grounds.png" "%~dp0files\data\build_training_grounds.png" "%APPDATA%\fheroes2\knight_mod.ini") do (
    if exist "%%~F" del /f /q "%%~F"
    if exist "%%~F" (
        echo Could not delete: %%~F
        set "KM_FAILED=1"
    )
)
if "%KM_FAILED%"=="1" (
    echo Close Knight Mod and retry. Administrator rights may be needed.
    pause
    exit /b 1
)
echo Knight Mod has been uninstalled. This batch file will now remove itself.
pause
endlocal & del /f /q "%~f0" & exit /b 0
