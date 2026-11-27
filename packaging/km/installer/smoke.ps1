param(
    [Parameter(Mandatory)][string]$Installer,
    [Parameter(Mandatory)][string]$Payload
)

$ErrorActionPreference = 'Stop'
$Installer = (Resolve-Path $Installer).Path
$Payload = (Resolve-Path $Payload).Path
$work = Join-Path $env:RUNNER_TEMP 'Knight Mod installer test'
New-Item -ItemType Directory $work -Force | Out-Null

function SameBytes($First, $Second) {
    if ((Get-FileHash -LiteralPath $First).Hash -ne (Get-FileHash -LiteralPath $Second).Hash) {
        throw "Unexpected contents in $First"
    }
}

function RunSetup($Executable, $Name, $Options, [switch]$Reject) {
    $log = Join-Path $work "$Name.log"
    $process = Start-Process -FilePath $Executable -ArgumentList (
        "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG=`"$log`" $Options"
    ) -Wait -PassThru
    if (($process.ExitCode -ne 0) -ne $Reject.IsPresent) {
        Get-Content -LiteralPath $log -ErrorAction SilentlyContinue | Select-Object -Last 60
        throw "$Name returned exit code $($process.ExitCode)"
    }
}

function Snapshot($Folder) {
    $map = @{}
    foreach ($file in Get-ChildItem -LiteralPath $Folder -Recurse -File -Force) {
        $relative = [IO.Path]::GetRelativePath($Folder, $file.FullName)
        $map[$relative] = (Get-FileHash -LiteralPath $file.FullName).Hash
    }
    return $map
}

function AssertPreserved($Folder, $Original) {
    foreach ($name in $Original.Keys) {
        $file = Join-Path $Folder $name
        if (!(Test-Path -LiteralPath $file -PathType Leaf) -or (Get-FileHash -LiteralPath $file).Hash -ne $Original[$name]) {
            throw "Original fheroes2 file changed: $file"
        }
    }
}

$exe = [IO.File]::ReadAllBytes($Installer)
$header = [BitConverter]::ToInt32($exe, 0x3c)
if ([BitConverter]::ToUInt16($exe, 0) -ne 0x5a4d -or [BitConverter]::ToUInt32($exe, $header) -ne 0x4550 -or
    [BitConverter]::ToUInt16($exe, $header + 4) -ne 0x14c -or [BitConverter]::ToUInt16($exe, $header + 24) -ne 0x10b) {
    throw 'The installer is not an x86 executable'
}

$fixtures = @{
    x86 = 'dcfe6f3937e2c53061984696f8af10b69676fa3c90ab60b470c344eaa9059603'
    x64 = 'aec387da1acd52cd422b49a6cf463829f790574dd2a970b7929a3d56f1096746'
}

foreach ($arch in @('x86', 'x64')) {
    $name = "fheroes2_windows_${arch}_SDL2.zip"
    $zip = Join-Path $work $name
    Invoke-WebRequest "https://github.com/ihhub/fheroes2/releases/download/1.1.17/$name" -OutFile $zip
    if ((Get-FileHash $zip).Hash -ne $fixtures[$arch]) { throw "Unexpected fheroes2 $arch archive" }
    Expand-Archive -LiteralPath $zip -DestinationPath (Join-Path $work "fheroes2 $arch")
}

$invalid = Join-Path $work 'Incomplete fheroes2'
New-Item -ItemType Directory $invalid | Out-Null
Set-Content -LiteralPath "$invalid/fheroes2.exe" -Value 'not a PE file'
RunSetup $Installer 'malformed-exe' "/DIR=`"$invalid`" /LANG=english" -Reject
if ((Snapshot $invalid).Count -ne 1) { throw 'Setup wrote into an invalid folder' }

$mixed = Join-Path $work 'Mixed fheroes2'
Copy-Item "$work/fheroes2 x64" $mixed -Recurse
Copy-Item "$work/fheroes2 x86/SDL2.dll" "$mixed/SDL2.dll" -Force
$original = Snapshot $mixed
RunSetup $Installer 'mixed-dll' "/DIR=`"$mixed`" /LANG=french" -Reject
AssertPreserved $mixed $original
if ((Snapshot $mixed).Count -ne $original.Count) { throw 'Setup wrote into a folder with mismatched DLLs' }

$reg = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\fheroes2_is1'
New-Item -Path $reg -Force | Out-Null
New-ItemProperty -Path $reg -Name 'Inno Setup: App Path' -Value "$work/fheroes2 x86" -PropertyType String -Force | Out-Null
$userSettings = Join-Path $env:APPDATA 'fheroes2'
New-Item -ItemType Directory $userSettings -Force | Out-Null
Set-Content -LiteralPath "$userSettings/knight_mod.ini" -Value 'test-existing-settings'
$userIniHash = (Get-FileHash -LiteralPath "$userSettings/knight_mod.ini").Hash

foreach ($arch in @('x86', 'x64')) {
    $directory = Join-Path $work "fheroes2 $arch"
    $original = Snapshot $directory

    if ($arch -eq 'x86') {
        $options = '/LANG=french /TASKS='
        $locale = 'fr'
        $readme = 'Readme_FR.txt'
    } else {
        $options = "/DIR=`"$directory`" /LANG=english /TASKS="
        $locale = 'en'
        $readme = 'Readme.txt'
    }
    RunSetup $Installer "install-$arch" $options
    AssertPreserved $directory $original
    SameBytes "$directory/fheroes2KM.exe" "$Payload/$arch/fheroes2KM.exe"
    SameBytes "$directory/KnightModLauncher.exe" "$Payload/common/KnightModLauncher.exe"
    foreach ($file in Get-ChildItem "$Payload/common/Knight Mod/docs" -File) {
        SameBytes "$directory/Knight Mod/docs/$($file.Name)" $file.FullName
    }
    SameBytes "$directory/Knight Mod/knight_mod.ini.default" "$Payload/common/Knight Mod/knight_mod.ini.default"
    foreach ($file in Get-ChildItem "$Payload/common/Knight Mod/files/data" -File) {
        SameBytes "$directory/Knight Mod/files/data/$($file.Name)" $file.FullName
        if (Test-Path -LiteralPath "$directory/files/data/$($file.Name)") { throw 'Mod assets leaked into the original resource folder' }
    }
    if ((Get-ChildItem "$directory/Knight Mod/files/lang" -Filter 'km_*.mo').Count -ne 23) { throw 'Missing mod catalogs' }
    if ((Get-Content -LiteralPath "$directory/Knight Mod/KnightModLauncher.language" -Raw).Trim() -ne $locale) {
        throw 'The launcher language does not match the installer'
    }

    $startMenu = Join-Path ([Environment]::GetFolderPath('CommonPrograms')) 'Knight Mod'
    $shortcuts = New-Object -ComObject WScript.Shell
    $readmeLink = $shortcuts.CreateShortcut("$startMenu/Readme.lnk").TargetPath
    if ([IO.Path]::GetFullPath($readmeLink) -ne [IO.Path]::GetFullPath("$directory/Knight Mod/docs/$readme")) {
        throw 'The Start menu Readme shortcut points to the wrong language or folder'
    }

    $uninstallers = @(Get-ChildItem "$directory/Knight Mod/uninstall" -Filter 'unins*.exe')
    if ($uninstallers.Count -ne 1) { throw 'Missing standalone Knight Mod uninstaller' }
    RunSetup $uninstallers[0].FullName "uninstall-$arch" ''
    AssertPreserved $directory $original
    $remaining = @((Snapshot $directory).Keys | Where-Object { !$original.ContainsKey($_) })
    if ($remaining.Count) {
        Get-Content -LiteralPath "$work/uninstall-$arch.log" | Select-Object -Last 60
        throw "Unexpected files left in the fheroes2 installation: $($remaining -join ', ')"
    }
    if ((Get-FileHash -LiteralPath "$userSettings/knight_mod.ini").Hash -ne $userIniHash) { throw 'User settings changed' }
    Write-Host "PASS $arch install ($locale), selection, shortcuts, uninstall, original files and INI"
}
