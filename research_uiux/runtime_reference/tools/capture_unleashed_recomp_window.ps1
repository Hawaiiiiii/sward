# Phase 288: capture the UnleashedRecomp window's client area to a PNG.
#
# Used by the Phase 288 side-by-side+diff harness as the "reference"
# input. The harness compares this capture against a render produced
# by `render_csd_scene.py` to measure pixel parity between the
# UnleashedRecomp runtime and the human-readable port.
#
# Usage:
#   pwsh -ExecutionPolicy Bypass `
#        -File research_uiux/research_uiux/runtime_reference/tools/capture_unleashed_recomp_window.ps1 `
#        -OutputPath out/phase288/unleashed_recomp_capture.png `
#        [-WindowTitle "UnleashedRecomp"] `
#        [-CropLeft 0 -CropTop 0 -CropWidth 0 -CropHeight 0]
#
# CropLeft/Top/Width/Height (all optional; pass 0 to skip cropping)
# isolate a HUD region. Coordinates are in client-area pixels.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)] [string] $OutputPath,
    [string] $WindowTitle = 'UnleashedRecomp',
    [int] $CropLeft = 0,
    [int] $CropTop = 0,
    [int] $CropWidth = 0,
    [int] $CropHeight = 0
)

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

Add-Type @'
using System;
using System.Runtime.InteropServices;

public static class Win32 {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

    [DllImport("user32.dll")]
    public static extern bool IsWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool ClientToScreen(IntPtr hWnd, ref POINT lpPoint);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }

    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X; public int Y; }
}
'@

function Find-WindowByTitleSubstring {
    param([string] $needle)
    $procs = Get-Process | Where-Object { $_.MainWindowTitle -and $_.MainWindowTitle -match $needle }
    foreach ($p in $procs) {
        if ([Win32]::IsWindow($p.MainWindowHandle)) {
            return $p.MainWindowHandle
        }
    }
    return [IntPtr]::Zero
}

$hWnd = Find-WindowByTitleSubstring -needle $WindowTitle
if ($hWnd -eq [IntPtr]::Zero) {
    Write-Error "Could not locate any open window matching title '$WindowTitle'. Launch UnleashedRecomp first."
    exit 1
}

[Win32]::SetForegroundWindow($hWnd) | Out-Null
Start-Sleep -Milliseconds 250

$rect = New-Object Win32+RECT
[Win32]::GetClientRect($hWnd, [ref] $rect) | Out-Null
$clientW = $rect.Right - $rect.Left
$clientH = $rect.Bottom - $rect.Top
if ($clientW -le 0 -or $clientH -le 0) {
    Write-Error "Window client area is empty ($clientW x $clientH). Is the window minimized?"
    exit 1
}

$pt = New-Object Win32+POINT
$pt.X = 0; $pt.Y = 0
[Win32]::ClientToScreen($hWnd, [ref] $pt) | Out-Null

Write-Host "[capture] window '$WindowTitle' client=$clientW x $clientH at screen ($($pt.X), $($pt.Y))"

$bmp = New-Object System.Drawing.Bitmap($clientW, $clientH)
$gfx = [System.Drawing.Graphics]::FromImage($bmp)
$gfx.CopyFromScreen($pt.X, $pt.Y, 0, 0, [System.Drawing.Size]::new($clientW, $clientH))
$gfx.Dispose()

if ($CropWidth -gt 0 -and $CropHeight -gt 0) {
    $cropRect = [System.Drawing.Rectangle]::new($CropLeft, $CropTop, $CropWidth, $CropHeight)
    $cropped = $bmp.Clone($cropRect, $bmp.PixelFormat)
    $bmp.Dispose()
    $bmp = $cropped
    Write-Host "[capture] cropped to $CropWidth x $CropHeight at ($CropLeft, $CropTop)"
}

$dir = Split-Path -Parent $OutputPath
if ($dir -and -not (Test-Path $dir)) {
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
}
$bmp.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

Write-Host "[capture] wrote $OutputPath"
