param(
    [Parameter(Mandatory)] [string] $Launcher,
    [Parameter(Mandatory)] [string] $GameProbe
)
$ErrorActionPreference = 'Stop'
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Native {
    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool PostMessage(IntPtr window, uint message, IntPtr command, IntPtr parameter);
}
'@
function Wait-Until([scriptblock]$Condition, [string]$Failure) {
    $limit = [DateTime]::UtcNow.AddSeconds(20)
    do {
        if (& $Condition) { return }
        Start-Sleep -Milliseconds 150
    } while ([DateTime]::UtcNow -lt $limit)
    throw $Failure
}
$temp = Join-Path ([IO.Path]::GetTempPath()) ("Knight Mod launcher " + [guid]::NewGuid())
$game = New-Item -ItemType Directory -Path (Join-Path $temp 'game install') -Force
$homeFolder = New-Item -ItemType Directory -Path (Join-Path $temp 'profile with spaces') -Force
$env:HOME = $homeFolder.FullName
$config = New-Item -ItemType Directory -Path (Join-Path $homeFolder.FullName '.fheroes2')
$ini = Join-Path $config.FullName 'knight_mod.ini'
$exe = Join-Path $game.FullName 'KnightModLauncher.exe'
Copy-Item $Launcher $exe
Copy-Item $GameProbe (Join-Path $game.FullName 'fheroes2KM.exe')
Set-Content -LiteralPath $ini -Encoding ASCII -Value @('# keep me', 'external.setting = 47', 'conscription.mode = 2', 'conscription.gold_per_peasant = 65')
$process = Start-Process -FilePath $exe -PassThru
try {
    Wait-Until { $process.Refresh(); $process.MainWindowHandle -ne [IntPtr]::Zero } 'The launcher did not create its main window.'
    $window = $process.MainWindowHandle
    if (-not [Native]::PostMessage($window, 0x111, [IntPtr]303, [IntPtr]::Zero)) { throw 'Cannot click Save.' }
    Wait-Until { (Get-Content -LiteralPath $ini -Raw) -match 'training_grounds.show_artwork = 1' } 'Saving did not update the actual INI.'
    $settings = Get-Content -LiteralPath $ini -Raw
    foreach ($line in @('# keep me', 'external.setting = 47', 'conscription.gold_per_peasant = 65')) {
        if (-not $settings.Contains($line)) { throw "Saving lost an existing value: $line" }
    }
    if (-not [Native]::PostMessage($window, 0x111, [IntPtr]304, [IntPtr]::Zero)) { throw 'Cannot click Play.' }
    $marker = Join-Path $game.FullName 'launcher-probe.txt'
    Wait-Until { Test-Path -LiteralPath $marker } 'The launcher did not start the x64 probe in the game folder.'
    $process.Refresh()
    if ($process.HasExited -or $process.MainWindowHandle -eq [IntPtr]::Zero) { throw 'Play closed the launcher.' }
    if (-not [Native]::PostMessage($window, 0x111, [IntPtr]305, [IntPtr]::Zero)) { throw 'Cannot click Quit.' }
    if (-not $process.WaitForExit(5000) -or $process.ExitCode -ne 0) { throw 'Quit did not close the launcher cleanly.' }
    Write-Output 'x86 GUI, real INI write, x64 child process, working directory, Play and Quit passed.'
} finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
    Remove-Item -LiteralPath $temp -Recurse -Force
}
