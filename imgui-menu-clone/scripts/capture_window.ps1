<#
.SYNOPSIS
  Launches the built menu, screenshots its window, then closes it.
  This is the feedback half of the clone loop: render -> capture -> compare.

  ASCII-only on purpose (Windows PowerShell 5.1 reads .ps1 as ANSI).

.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File capture_window.ps1 -Exe .\build\Release\menu.exe -Out render.png
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string] $Exe,
    [Parameter(Mandatory = $true)][string] $Out,
    [int] $WaitMs = 2500,
    [switch] $KeepOpen
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinCap {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

if (-not (Test-Path $Exe)) { throw "Executable not found: $Exe" }

$proc = Start-Process -FilePath (Resolve-Path $Exe) -PassThru `
                      -WorkingDirectory (Split-Path (Resolve-Path $Exe) -Parent)
try {
    Start-Sleep -Milliseconds $WaitMs
    $proc.Refresh()
    if ($proc.HasExited) { throw "Process exited immediately (code $($proc.ExitCode))." }

    $hwnd = $proc.MainWindowHandle
    if ($hwnd -eq [IntPtr]::Zero) { throw 'No main window handle; the app may have failed to create its window.' }

    [void][WinCap]::SetForegroundWindow($hwnd)
    Start-Sleep -Milliseconds 400

    $r = New-Object WinCap+RECT
    [void][WinCap]::GetWindowRect($hwnd, [ref]$r)
    $w = $r.Right - $r.Left
    $h = $r.Bottom - $r.Top
    if ($w -le 0 -or $h -le 0) { throw "Bad window rect ($w x $h)." }

    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g   = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r.Left, $r.Top, 0, 0, $bmp.Size)
    $g.Dispose()

    $dir = Split-Path $Out -Parent
    if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
    $bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()

    Write-Host ("[capture] " + $w + "x" + $h + " -> " + $Out)
} finally {
    if (-not $KeepOpen -and $proc -and -not $proc.HasExited) {
        $proc.CloseMainWindow() | Out-Null
        Start-Sleep -Milliseconds 300
        if (-not $proc.HasExited) { $proc.Kill() }
    }
}
