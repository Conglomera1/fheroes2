@echo off
setlocal EnableExtensions DisableDelayedExpansion
title Knight Mod uninstaller

set "KM_SELF=%~f0"
set "KM_DIR=%~dp0"

rem Resolve the original user's active settings before requesting elevation.
set "KM_CONFIG="
if defined APPDATA set "KM_CONFIG=%APPDATA%\fheroes2\knight_mod.ini"
if defined HOME set "KM_CONFIG=%HOME%\.fheroes2\knight_mod.ini"
if /i "%~1"=="--km-elevated" (
    if /i "%~nx2"=="knight_mod.ini" set "KM_CONFIG=%~2"
)

rem Check whether this process already has administrator privileges.
powershell.exe -NoProfile -NonInteractive -Command "$id=[System.Security.Principal.WindowsIdentity]::GetCurrent(); $p=New-Object -TypeName System.Security.Principal.WindowsPrincipal -ArgumentList $id; if ($p.IsInRole([System.Security.Principal.WindowsBuiltInRole]::Administrator)) { exit 0 }; exit 1" >nul 2>&1

if not errorlevel 1 goto :administrator

rem Prevent repeated elevation attempts if the elevated launch failed.
if /i "%~1"=="--km-elevated" (
    echo Administrator privileges could not be obtained.
    pause
    exit /b 1
)

echo Windows needs your permission to remove Knight Mod from this folder. Please approve the next Windows prompt to continue.

rem Pass the script path through the environment and quote it for cmd.exe.
powershell.exe -NoProfile -NonInteractive -Command "try { $q=[char]34; $a='/d /s /c '+$q+$q+$env:KM_SELF+$q+' --km-elevated '+$q+$env:KM_CONFIG+$q+$q; Start-Process -FilePath $env:ComSpec -Verb RunAs -ArgumentList $a -WorkingDirectory $env:KM_DIR -ErrorAction Stop; exit 0 } catch { exit 1 }"

if errorlevel 1 (
    cls
    echo Uninstallation could not continue because administrator permission was not obtained. Please try again and approve the Windows permission prompt or refer to the README for instructions on removing Knight Mod manually.
    pause
    exit /b 1
)

exit /b 0

:administrator
pushd "%KM_DIR%" >nul 2>&1
if errorlevel 1 (
    echo Cannot access the Knight Mod installation folder. Please refer to the README for instructions on removing Knight Mod manually.
    pause
    exit /b 1
)

rem Allow another attempt when only some mod files remain.
for %%F in (
    "fheroes2KM.exe"
    "files\data\resurrectionKM.h2d"
    "files\data\training_grounds.png"
    "files\data\build_training_grounds.png"
    "knight_mod.ini.default"
    "KM_SOURCE_COMMIT.txt"
    "KM_BUILD.txt"
) do if exist "%%~F" goto :confirmation

for %%L in (
    be bg cs da de el eo es fr hu it lt nb nl
    pl pt ro ru sk sv tr uk vi
) do if exist "files\lang\km_%%L.mo" goto :confirmation

echo Knight Mod files were not found in this folder.
popd
pause
exit /b 1

:confirmation
echo This removes only the Knight Mod files from this folder.
echo Original game files and saved games are kept.
echo You can choose whether to remove your Knight Mod settings afterwards.
echo.
choice /c YN /n /m "Uninstall Knight Mod? [Y/N] "
if errorlevel 2 (
    popd
    exit /b 0
)
if errorlevel 1 goto :uninstall

popd
exit /b 1

:uninstall
set "KM_FAILED=0"

for %%F in (
    "fheroes2KM.exe"
    "files\data\resurrectionKM.h2d"
    "files\data\training_grounds.png"
    "files\data\build_training_grounds.png"
    "knight_mod.ini.default"
    "KM_SOURCE_COMMIT.txt"
    "KM_BUILD.txt"
) do call :remove "%%~F"

for %%L in (
    be bg cs da de el eo es fr hu it lt nb nl
    pl pt ro ru sk sv tr uk vi
) do call :remove "files\lang\km_%%L.mo"

if "%KM_FAILED%"=="1" (
    echo.
    echo Some Knight Mod files could not be removed.
    echo Close the game and retry.
    echo If access is still denied, check the permissions of the files listed above.
    popd
    pause
    exit /b 1
)

rem Offer to remove only the active settings file, after the mod files.
if not defined KM_CONFIG goto :finished
if not exist "%KM_CONFIG%" goto :finished

cls
echo knight_mod.ini stores your Knight Mod settings, including taxation,
echo conscription and building costs.
echo.
echo Keep this file if you want to reuse your settings later.
echo If deleted, it will be recreated with default settings the next time
echo you run Knight Mod.
echo.
echo File: "%KM_CONFIG%"
echo.
choice /c YN /n /m "Do you also want to delete knight_mod.ini? [Y/N] "
if errorlevel 2 goto :finished
if not errorlevel 1 goto :finished

del /f /q "%KM_CONFIG%"
if exist "%KM_CONFIG%" (
    echo.
    echo Knight Mod was removed, but knight_mod.ini could not be deleted.
    echo File: "%KM_CONFIG%"
    echo Please refer to the README for instructions on removing it manually.
    popd
    pause
    exit /b 1
)
echo.
echo knight_mod.ini was removed.

:finished
echo.
echo Knight Mod removed. Your saved games were preserved.
popd
pause
endlocal & del /q "%KM_SELF%" & exit /b 0

:remove
if not exist "%~1" exit /b 0
del /f /q "%~1"
if exist "%~1" (
    echo Could not remove: "%~1"
    set "KM_FAILED=1"
)
exit /b 0