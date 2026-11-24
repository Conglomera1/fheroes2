param(
    [Parameter(Mandatory)] [string] $Launcher,
    [Parameter(Mandatory)] [string] $GameProbe,
    [string] $ScreenshotFolder
)
$ErrorActionPreference = 'Stop'
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class Native {
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll", SetLastError=true)]
    public static extern bool PostMessage(IntPtr window, uint message, IntPtr command, IntPtr parameter);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)]
    public static extern IntPtr FindWindow(string windowClass, string title);
    [DllImport("user32.dll")]
    public static extern IntPtr GetDlgItem(IntPtr window, int id);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)]
    public static extern IntPtr SendMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr window, IntPtr after, int x, int y, int width, int height, uint flags);
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
function Save-LauncherCapture([IntPtr]$Window, [string]$Path) {
    Add-Type -AssemblyName System.Drawing
    $rect = [Native+Rect]::new()
    if (-not [Native]::GetWindowRect($Window, [ref]$rect)) { throw 'Cannot locate the launcher window.' }
    [void][Native]::SetWindowPos($Window, [IntPtr](-1), 0, 0, 0, 0, 0x43)
    Start-Sleep -Milliseconds 250
    $bitmap = [System.Drawing.Bitmap]::new(($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top))
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
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
$process = Start-Process -FilePath $exe -ArgumentList "--language=en" -PassThru
try {
    Wait-Until {
        $process.Refresh()
        if ($process.HasExited) { throw "The launcher exited during startup with code $($process.ExitCode)." }
        if ([Native]::FindWindow('#32770', 'Knight Mod Launcher') -ne [IntPtr]::Zero) { throw 'The launcher displayed a startup error.' }
        $main = [Native]::FindWindow('KnightModLauncher', 'Knight Mod Launcher')
        $main -ne [IntPtr]::Zero -and [Native]::IsWindowVisible($main)
    } 'The launcher did not create its main window.'
    $window = [Native]::FindWindow('KnightModLauncher', 'Knight Mod Launcher')
    Write-Output 'The native launcher window opened.'
    $panel = [Native]::GetDlgItem($window, 318)
    $readme = [Native]::GetDlgItem($window, 315)
    $conversion = [Native]::GetDlgItem($panel, 316)
    $requirements = [Native]::GetDlgItem($panel, 317)
    $goldCost = [Native]::GetDlgItem($panel, 214)
    $captain = [Native]::GetDlgItem($panel, 220)
    if (@($panel, $readme, $conversion, $requirements, $goldCost, $captain) -contains [IntPtr]::Zero) {
        throw "The launcher controls are incomplete. Panel=$panel Readme=$readme Conversion=$conversion Requirements=$requirements Gold=$goldCost Captain=$captain"
    }
    if (-not [Native]::IsWindowVisible($readme)) { throw 'The Readme button is hidden.' }
    [void][Native]::SendMessage($window, 0x111, [IntPtr]313, [IntPtr]::Zero)
    if (-not ([Native]::IsWindowVisible($conversion) -and [Native]::IsWindowVisible($requirements))) { throw 'Advanced sections are missing.' }
    if ([Native]::IsWindowVisible($goldCost) -or [Native]::IsWindowVisible($captain)) { throw 'Advanced sections should start collapsed.' }
    [void][Native]::SendMessage($panel, 0x111, [IntPtr]316, $conversion)
    if (-not [Native]::IsWindowVisible($goldCost)) { throw 'Conversion costs did not expand.' }
    [void][Native]::SendMessage($panel, 0x111, [IntPtr]316, $conversion)
    [void][Native]::SendMessage($panel, 0x111, [IntPtr]317, $requirements)
    if ([Native]::IsWindowVisible($goldCost) -or -not [Native]::IsWindowVisible($captain)) { throw 'Advanced sections do not expand independently.' }
    [void][Native]::SendMessage($panel, 0x111, [IntPtr]317, $requirements)
    [void][Native]::SendMessage($window, 0x111, [IntPtr]310, [IntPtr]::Zero)
    Write-Output 'The Readme control and Advanced panels passed.'
    if (-not [Native]::PostMessage($window, 0x111, [IntPtr]303, [IntPtr]::Zero)) { throw 'Cannot click Save.' }
    Wait-Until {
        if ([Native]::FindWindow('#32770', 'Invalid value') -ne [IntPtr]::Zero) { throw 'The launcher rejected a setting from the fixture on Save.' }
        (Get-Content -LiteralPath $ini -Raw) -match 'training_grounds.show_artwork = 1'
    } 'Saving did not update the actual INI.'
    $settings = Get-Content -LiteralPath $ini -Raw
    foreach ($line in @('# keep me', 'external.setting = 47', 'conscription.gold_per_peasant = 65')) {
        if (-not $settings.Contains($line)) { throw "Saving lost an existing value: $line" }
    }
    Write-Output 'The existing INI and mode presets passed.'
    $mode = [Native]::GetDlgItem($panel, 202)
    foreach ($preset in @(@(1,100), @(2,50))) {
        [void][Native]::SendMessage($mode, 0x14E, [IntPtr]($preset[0]), [IntPtr]::Zero)
        [void][Native]::SendMessage($panel, 0x111, [IntPtr]65738, $mode)
        [void][Native]::SendMessage($window, 0x111, [IntPtr]303, [IntPtr]::Zero)
        $settings = Get-Content -LiteralPath $ini -Raw
        if ($settings -notmatch "(?m)^conscription.mode = $($preset[0])\r?$" -or
            $settings -notmatch "(?m)^conscription.gold_per_peasant = $($preset[1])\r?$") { throw 'Changing the mode did not restore its base price.' }
    }
    foreach ($entry in @(@(307,'Save As'), @(308,'Load Profile'))) {
        $command = [int]$entry[0]
        $title = [string]$entry[1]
        if (-not [Native]::PostMessage($window, 0x111, [IntPtr]$command, [IntPtr]::Zero)) { throw "Cannot open $title." }
        Wait-Until { [Native]::FindWindow('#32770', $title) -ne [IntPtr]::Zero } "The native dialog $title did not open."
        if (-not [Native]::PostMessage([Native]::FindWindow('#32770', $title), 0x10, [IntPtr]::Zero, [IntPtr]::Zero)) {
            throw "Cannot close $title."
        }
        Wait-Until { [Native]::FindWindow('#32770', $title) -eq [IntPtr]::Zero } "The native dialog $title did not close."
        Write-Output "The $title dialog passed."
    }
    if (-not [Native]::PostMessage($window, 0x111, [IntPtr]304, [IntPtr]::Zero)) { throw 'Cannot click Play.' }
    $marker = Join-Path $game.FullName 'launcher-probe.txt'
    Wait-Until { Test-Path -LiteralPath $marker } 'The launcher did not start the x64 probe in the game folder.'
    $process.Refresh()
    if ($process.HasExited -or $process.MainWindowHandle -eq [IntPtr]::Zero) { throw 'Play closed the launcher.' }
    if (-not [Native]::PostMessage($window, 0x111, [IntPtr]305, [IntPtr]::Zero)) { throw 'Cannot click Quit.' }
    if (-not $process.WaitForExit(5000) -or $process.ExitCode -ne 0) { throw 'Quit did not close the launcher cleanly.' }
    Write-Output 'x86 GUI, collapsible sections, Readme button, mode presets, real INI write, native profile dialogs, x64 child process, Play and Quit passed.'
    if ($ScreenshotFolder) {
        $screenshots = New-Item -ItemType Directory -Path $ScreenshotFolder -Force
        $process = Start-Process -FilePath $exe -ArgumentList "--language=fr" -PassThru
        Wait-Until { $process.Refresh(); $process.MainWindowHandle -ne [IntPtr]::Zero } 'The French launcher did not open.'
        $window = $process.MainWindowHandle
        Save-LauncherCapture $window (Join-Path $screenshots.FullName 'Launcher-Reglages.png')
        [void][Native]::SendMessage($window, 0x111, [IntPtr]313, [IntPtr]::Zero)
        Save-LauncherCapture $window (Join-Path $screenshots.FullName 'Launcher-Avance.png')
        [void][Native]::SendMessage($window, 0x111, [IntPtr]305, [IntPtr]::Zero)
        if (-not $process.WaitForExit(5000)) { throw 'The French launcher did not close.' }
    }
} finally {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        [void]$process.WaitForExit(5000)
    }
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction Continue
}
