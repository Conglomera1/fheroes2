@echo off
setlocal EnableExtensions DisableDelayedExpansion
title Knight Mod uninstaller

pushd "%~dp0" >nul 2>&1
if errorlevel 1 goto :unavailable

if exist "docs\Knight Mod\uninstall\unins*.exe" goto :setup_install

set "KM_FOUND="
call :mod_files find
if not defined KM_FOUND goto :not_found

echo This will remove the Knight Mod files in "%CD%".
echo The original fheroes2 files and saved games will be kept.
echo.
choice /c YN /n /m "Uninstall Knight Mod? [Y/N] "
if errorlevel 2 goto :cancelled
if not errorlevel 1 goto :cancelled

set "KM_FAILED="
call :mod_files remove
if defined KM_FAILED goto :files_failed

rem Remove only the empty documentation folder, leaving shared folders alone.
rd "docs\Knight Mod" 2>nul
popd

rem Use the same settings directory as Knight Mod Launcher.
set "KM_CONFIG_DIR="
if defined APPDATA set "KM_CONFIG_DIR=%APPDATA%\fheroes2"
if defined HOME set "KM_CONFIG_DIR=%HOME%\.fheroes2"
if not defined KM_CONFIG_DIR goto :finished

pushd "%KM_CONFIG_DIR%" >nul 2>&1
if errorlevel 1 goto :finished
if not exist "knight_mod.ini" if not exist "knight_mod_launcher.language" goto :keep_settings

echo.
echo Optional settings in "%CD%":
if exist "knight_mod.ini" echo   knight_mod.ini
if exist "knight_mod_launcher.language" echo   knight_mod_launcher.language
choice /c YN /n /m "Delete these Knight Mod settings too? [Y/N] "
if errorlevel 2 goto :keep_settings
if not errorlevel 1 goto :keep_settings

call :remove "knight_mod.ini"
call :remove "knight_mod_launcher.language"
if not defined KM_FAILED goto :keep_settings
echo Some settings could not be deleted. See Installation.txt for manual removal.

:keep_settings
popd

:finished
echo.
echo Knight Mod removed. Your saved games were preserved.
pause
rem Return from this batch before deleting the script file.
(goto) 2>nul & del /f /q "%~f0"

:files_failed
echo.
echo Close the game and launcher, then try again.
echo If the folder is protected, right-click this batch file and choose Run as administrator.
popd
pause
exit /b 1

:setup_install
echo Use Uninstall Knight Mod from the Start menu for this installation.
popd
pause
exit /b 1

:not_found
echo Knight Mod files were not found next to this batch file.
popd
pause
exit /b 1

:unavailable
echo Cannot access the installation folder.
pause
exit /b 1

:cancelled
popd
exit /b 0

:mod_files
for %%F in (
    "fheroes2KM.exe"
    "KnightModLauncher.exe"
    "knight_mod.ini.default"
    "files\data\resurrectionKM.h2d"
    "files\data\training_grounds.png"
    "files\data\build_training_grounds.png"
    "docs\Knight Mod\LICENCE.txt"
    "docs\Knight Mod\Readme.txt"
    "docs\Knight Mod\Readme_FR.txt"
    "docs\Knight Mod\Testing_and_Balance_Guide.pdf"
    "docs\Knight Mod\Guide_de_Test_et_Equilibrage.pdf"
) do call :%~1 "%%~F"

for %%L in (
    be bg cs da de el eo es fr hu it lt nb nl pl pt ro ru sk sv tr uk vi
) do call :%~1 "files\lang\km_%%L.mo"
exit /b 0

:find
if exist "%~1" set "KM_FOUND=1"
exit /b 0

:remove
if not exist "%~1" exit /b 0
rem DEL applied to a directory would delete its contents.
if not exist "%~1\" del /f /q "%~1" >nul 2>&1
if exist "%~1" (
    echo Could not remove: "%~1"
    set "KM_FAILED=1"
)
exit /b 0
