@echo off
setlocal
cd /d "%~dp0"
echo This removes PTP and its configuration from this installation.
echo Your original fheroes2 files and saves stay in place.
set /p "answer=Continue? [Y/N]: "
if /i not "%answer%"=="Y" exit /b 0
del /q "fheroes2TPx86.exe" "fheroes2TPx64.exe" "files\lang\*_TP.mo" "files\data\resurrection_TP.h2d" "%APPDATA%\fheroes2\knight_mod.ini" 2>nul
if exist "fheroes2TPx86.exe" goto failed
if exist "fheroes2TPx64.exe" goto failed
if exist "files\lang\*_TP.mo" goto failed
if exist "files\data\resurrection_TP.h2d" goto failed
if exist "%APPDATA%\fheroes2\knight_mod.ini" goto failed
echo PTP uninstalled.
pause
endlocal & del /q "%~f0" & exit /b 0
:failed
echo Some files could not be deleted. Close PTP and try again.
pause
exit /b 1
