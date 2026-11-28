param(
    [Parameter(Mandatory)][string]$Launcher,
    [Parameter(Mandatory)][string]$GameProbe,
    [Parameter(Mandatory)][string]$Screenshot
)

$ErrorActionPreference = 'Stop'
Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class KMWindow {
    private delegate bool WindowCallback(IntPtr child, IntPtr ignored);
    [DllImport("user32.dll")] private static extern bool EnumChildWindows(IntPtr parent, WindowCallback callback, IntPtr ignored);
    [DllImport("user32.dll")] private static extern int GetDlgCtrlID(IntPtr child);
    [DllImport("user32.dll")] private static extern IntPtr GetParent(IntPtr child);
    [DllImport("user32.dll")] private static extern IntPtr SendMessage(IntPtr window, int message, IntPtr param, IntPtr data);
    [DllImport("user32.dll", EntryPoint = "SendMessageW", CharSet = CharSet.Unicode)]
    private static extern IntPtr TextMessage(IntPtr window, int message, IntPtr size, StringBuilder text);
    [DllImport("user32.dll")] private static extern bool PostMessage(IntPtr window, int message, IntPtr param, IntPtr data);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr window, out Rect rect);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern IntPtr FindWindow(string type, string title);
    [StructLayout(LayoutKind.Sequential)]
    public struct Rect { public int Left, Top, Right, Bottom; }
    public static IntPtr Find(IntPtr parent, int id) {
        IntPtr result = IntPtr.Zero;
        EnumChildWindows(parent, (child, _) => {
            if (GetDlgCtrlID(child) != id) return true;
            result = child;
            return false;
        }, IntPtr.Zero);
        return result;
    }
    public static string Read(IntPtr window) {
        var text = new StringBuilder(256);
        TextMessage(window, 0xD, (IntPtr)text.Capacity, text);
        return text.ToString();
    }
    public static bool Click(IntPtr window) {
        return window != IntPtr.Zero && PostMessage(window, 0xf5, IntPtr.Zero, IntPtr.Zero);
    }
    public static void Select(IntPtr combo, int value) {
        SendMessage(combo, 0x014e, (IntPtr)value, IntPtr.Zero);
        SendMessage(GetParent(combo), 0x0111, (IntPtr)((9 << 16) | GetDlgCtrlID(combo)), combo);
    }
    public static void Cancel(IntPtr window) {
        PostMessage(window, 0x10, IntPtr.Zero, IntPtr.Zero);
    }
}
'@

function Wait-For([scriptblock]$Condition, [string]$Failure) {
    $deadline = [DateTime]::UtcNow.AddSeconds(20)
    do {
        if (& $Condition) { return }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)
    throw $Failure
}

function Click-Control([int]$Id) {
    if (![KMWindow]::Click([KMWindow]::Find($window, $Id))) { throw "Could not click control $Id" }
}

function Save-And-Read {
    Click-Control 303
    Wait-For {
        if ([KMWindow]::FindWindow('#32770', 'Knight Mod Launcher') -ne [IntPtr]::Zero) {
            throw 'The launcher reported a Save error'
        }
        [KMWindow]::Read([KMWindow]::Find($window, 319)) -eq 'Settings saved.'
    } 'The launcher did not confirm the save'
    Get-Content -LiteralPath $ini -Raw
}

$root = Join-Path $env:RUNNER_TEMP ("Knight Mod wxWidgets " + [guid]::NewGuid())
$game = New-Item -ItemType Directory (Join-Path $root 'Game folder') -Force
$env:HOME = (New-Item -ItemType Directory (Join-Path $root 'User profile')).FullName
$config = New-Item -ItemType Directory (Join-Path $env:HOME '.fheroes2')
$ini = Join-Path $config.FullName 'knight_mod.ini'
$exe = Join-Path $game.FullName 'KnightModLauncher.exe'
Copy-Item -LiteralPath $Launcher -Destination $exe
Copy-Item -LiteralPath $GameProbe -Destination (Join-Path $game.FullName 'fheroes2KM.exe')
Set-Content -LiteralPath $ini -Encoding ascii -Value @('# preserve this', 'external.setting = 47', 'conscription.mode = 2', 'conscription.gold_per_peasant = 65')
$process = Start-Process -FilePath $exe -ArgumentList '--language=en' -PassThru
try {
    Wait-For {
        $process.Refresh()
        if ($process.HasExited) { throw "KMSettings exited with status $($process.ExitCode)" }
        $process.MainWindowHandle -ne [IntPtr]::Zero -and $process.MainWindowTitle -eq 'Knight Mod Launcher'
    } 'The wxWidgets launcher did not open'
    $window = $process.MainWindowHandle
    $settings = Save-And-Read
    foreach ($line in @('# preserve this', 'external.setting = 47', 'conscription.gold_per_peasant = 65', 'training_grounds.show_artwork = 1')) {
        if (!$settings.Contains($line)) { throw "Unexpected INI after Save: missing $line" }
    }
    $mode = [KMWindow]::Find($window, 202)
    if ($mode -eq [IntPtr]::Zero) { throw 'Mode control is missing' }
    foreach ($preset in @(@(1, 100), @(2, 50))) {
        [KMWindow]::Select($mode, $preset[0])
        $settings = Save-And-Read
        if ($settings -notmatch "(?m)^conscription.mode = $($preset[0])\r?$" -or
            $settings -notmatch "(?m)^conscription.gold_per_peasant = $($preset[1])\r?$") {
            throw 'The mode did not restore its base price'
        }
    }

    $baseReadme = Join-Path $game.FullName 'Readme.txt'
    Set-Content -LiteralPath $baseReadme -Value 'Base game documentation'
    Click-Control 315
    Wait-For { [KMWindow]::FindWindow('#32770', 'Knight Mod Launcher') -ne [IntPtr]::Zero } 'Expected a missing-mod-document message'
    [KMWindow]::Cancel([KMWindow]::FindWindow('#32770', 'Knight Mod Launcher'))
    Wait-For { [KMWindow]::FindWindow('#32770', 'Knight Mod Launcher') -eq [IntPtr]::Zero } 'The Readme message stayed open'
    if ((Get-Content -LiteralPath $baseReadme) -ne 'Base game documentation') { throw 'The base game Readme changed' }

    Click-Control 311
    Wait-For { [KMWindow]::IsWindowVisible([KMWindow]::Find($window, 210)) } 'Advanced is missing training settings'
    Click-Control 312
    Wait-For { [KMWindow]::IsWindowVisible([KMWindow]::Find($window, 307)) } 'Miscellaneous is missing profiles'
    foreach ($dialog in @(@(307, 'Save As'), @(308, 'Load Profile'))) {
        Click-Control $dialog[0]
        Wait-For { [KMWindow]::FindWindow('#32770', $dialog[1]) -ne [IntPtr]::Zero } "Profile dialog $($dialog[1]) did not open"
        [KMWindow]::Cancel([KMWindow]::FindWindow('#32770', $dialog[1]))
        Wait-For { [KMWindow]::FindWindow('#32770', $dialog[1]) -eq [IntPtr]::Zero } "Profile dialog $($dialog[1]) did not close"
    }
    Click-Control 304
    Wait-For { Test-Path -LiteralPath (Join-Path $game.FullName 'launcher-probe.txt') } 'Play did not start the adjacent x64 game'
    Click-Control 305
    if (!$process.WaitForExit(5000) -or $process.ExitCode -ne 0) { throw 'The launcher failed to close' }

    Remove-Item -LiteralPath $ini
    $process = Start-Process -FilePath $exe -ArgumentList '--language=fr' -PassThru
    Wait-For {
        $process.Refresh()
        [KMWindow]::Read([KMWindow]::Find($process.MainWindowHandle, 304)) -eq 'Jouer'
    } 'The French interface did not open'
    $window = $process.MainWindowHandle
    Click-Control 303
    Wait-For { [KMWindow]::Read([KMWindow]::Find($window, 319)) -eq 'Réglages enregistrés.' } 'The first save did not complete'
    $expectedIni = (Get-Content -LiteralPath "$PSScriptRoot/game_default.ini" -Raw) -replace '\r?\n', "`r`n"
    if ((Get-Content -LiteralPath $ini -Raw) -cne $expectedIni) { throw 'The first save differs from the INI generated by the game' }
    Add-Type -AssemblyName System.Drawing
    $rect = [KMWindow+Rect]::new()
    if (![KMWindow]::GetWindowRect($window, [ref]$rect)) { throw 'Cannot locate the launcher window' }
    $bitmap = [Drawing.Bitmap]::new(($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top))
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Screenshot, [Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
    Click-Control 305
    if (!$process.WaitForExit(5000)) { throw 'The French launcher failed to close' }
    Write-Output 'PASS wxWidgets interface in English and French, INI, mode presets, documentation, profile dialogs, and launch of the adjacent x64 game'
} finally {
    if (!$process.HasExited) {
        Stop-Process -Id $process.Id -Force
        [void]$process.WaitForExit(5000)
    }
    Remove-Item -LiteralPath $root -Recurse -Force
}
