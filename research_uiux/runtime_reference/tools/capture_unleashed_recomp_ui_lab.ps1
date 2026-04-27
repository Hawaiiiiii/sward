param(
    [string[]]$Targets = @("title-loop", "title-menu", "loading", "sonic-hud"),
    [string]$ExePath = "W:\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe",
    [string]$InstallRoot = "Unleashed Recomp - Windows (Complete Installation) 1.0.3",
    [string]$OutputRoot = "out\ui_lab_runtime_evidence",
    [int]$InitialWaitSeconds = 8,
    [int]$SecondCaptureDelaySeconds = 3,
    [int]$AutoExitSeconds = 16,
    [int]$ObserveSeconds = 0,
    [int]$SnapshotIntervalSeconds = 10,
    [ValidateSet("input", "direct-context")]
    [string]$RoutePolicy = "input",
    [bool]$NormalizeWindow = $true,
    [switch]$Observer,
    [switch]$HideOverlay,
    [switch]$KeepRunning,
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..\..")

function Resolve-InRepo([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    return Join-Path $repoRoot.Path $Path
}

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential)]
public struct UiLabRect
{
    public int Left;
    public int Top;
    public int Right;
    public int Bottom;
}

public static class UiLabNative
{
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out UiLabRect rect);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    public static extern bool MoveWindow(IntPtr hWnd, int x, int y, int width, int height, bool repaint);

    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);
}
"@

function Wait-UiLabWindow([System.Diagnostics.Process]$Process, [int]$TimeoutSeconds) {
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)

    while ((Get-Date) -lt $deadline) {
        $Process.Refresh()

        if ($Process.HasExited) {
            return [IntPtr]::Zero
        }

        if ($Process.MainWindowHandle -ne [IntPtr]::Zero) {
            return $Process.MainWindowHandle
        }

        Start-Sleep -Milliseconds 250
    }

    return [IntPtr]::Zero
}

function Test-BitmapHasSignal([System.Drawing.Bitmap]$Bitmap) {
    $unique = New-Object "System.Collections.Generic.HashSet[int]"
    $nonDark = 0
    $xStep = [Math]::Max(1, [int]($Bitmap.Width / 16))
    $yStep = [Math]::Max(1, [int]($Bitmap.Height / 9))

    for ($y = 0; $y -lt $Bitmap.Height; $y += $yStep) {
        for ($x = 0; $x -lt $Bitmap.Width; $x += $xStep) {
            $pixel = $Bitmap.GetPixel($x, $y)
            $packed = ($pixel.R -shl 16) -bor ($pixel.G -shl 8) -bor $pixel.B
            [void]$unique.Add($packed)

            if ($pixel.R -gt 8 -or $pixel.G -gt 8 -or $pixel.B -gt 8) {
                $nonDark += 1
            }
        }
    }

    return $unique.Count -gt 4 -or $nonDark -gt 2
}

function Capture-Window([IntPtr]$Handle, [string]$Path) {
    if ($Handle -eq [IntPtr]::Zero) {
        throw "Cannot capture screenshot because the process has no window handle."
    }

    [UiLabNative]::ShowWindow($Handle, 9) | Out-Null

    if ($NormalizeWindow) {
        [UiLabNative]::MoveWindow($Handle, 64, 64, 1280, 720, $true) | Out-Null
    }

    [UiLabNative]::SetForegroundWindow($Handle) | Out-Null
    Start-Sleep -Milliseconds 500

    $rect = New-Object UiLabRect
    if (-not [UiLabNative]::GetWindowRect($Handle, [ref]$rect)) {
        throw "GetWindowRect failed for UI Lab window."
    }

    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top

    if ($width -le 0 -or $height -le 0) {
        throw "UI Lab window has invalid capture bounds: ${width}x${height}."
    }

    $bitmap = [System.Drawing.Bitmap]::new($width, $height)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)

    try {
        $printed = $false
        $hdc = $graphics.GetHdc()

        try {
            $printed = [UiLabNative]::PrintWindow($Handle, $hdc, 2)
        }
        finally {
            $graphics.ReleaseHdc($hdc)
        }

        if (-not $printed -or -not (Test-BitmapHasSignal $bitmap)) {
            $graphics.Clear([System.Drawing.Color]::Black)
            $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        }

        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Join-ProcessArguments([string[]]$Arguments) {
    $quoted = foreach ($argument in $Arguments) {
        if ($argument -notmatch '[\s"]') {
            $argument
        }
        else {
            '"' + ($argument -replace '"', '\"') + '"'
        }
    }

    return $quoted -join " "
}

function Start-UiLabProcess([string]$FilePath, [string[]]$Arguments, [string]$WorkingDirectory) {
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false

    if ($null -ne $startInfo.ArgumentList) {
        foreach ($argument in $Arguments) {
            [void]$startInfo.ArgumentList.Add($argument)
        }
    }
    else {
        $startInfo.Arguments = Join-ProcessArguments $Arguments
    }

    return [System.Diagnostics.Process]::Start($startInfo)
}

$exe = Resolve-InRepo $ExePath
$install = Resolve-InRepo $InstallRoot
$output = Resolve-InRepo $OutputRoot

if (-not (Test-Path -LiteralPath $exe)) {
    if ($NoBuild) {
        throw "UI Lab executable not found: $exe"
    }

    & (Join-Path $PSScriptRoot "build_unleashed_recomp_ui_lab.ps1")
}

if (-not (Test-Path -LiteralPath $exe)) {
    throw "UI Lab executable not found after build: $exe"
}

if (-not (Test-Path -LiteralPath $install)) {
    throw "Complete installation root not found: $install"
}

$sessionDir = Join-Path $output (Get-Date -Format "yyyyMMdd_HHmmss")
New-Item -ItemType Directory -Force -Path $sessionDir | Out-Null

$stageTargets = @("sonic-hud", "extra-stage-hud", "tutorial", "result")
$records = @()
$expandedTargets = @()

if ($Observer) {
    $expandedTargets = @("manual-observer")
}
else {
    foreach ($target in $Targets) {
        $expandedTargets += $target -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ }
    }
}

foreach ($target in $expandedTargets) {
    $safeTarget = $target -replace "[^A-Za-z0-9_.-]", "_"
    $targetDir = Join-Path $sessionDir $safeTarget
    New-Item -ItemType Directory -Force -Path $targetDir | Out-Null

    $args = @(
        "--use-cwd",
        "--ui-lab-evidence-dir", $targetDir
    )

    if (-not $Observer) {
        $args += @("--ui-lab-screen", $target)
        $args += @("--ui-lab-route-policy", $RoutePolicy)
    }
    else {
        $args += "--ui-lab-observer"
    }

    if ($HideOverlay) {
        $args += @("--ui-lab-overlay", "off")
    }

    if (-not $KeepRunning -and $AutoExitSeconds -gt 0) {
        $args += @("--ui-lab-auto-exit", "$AutoExitSeconds")
    }

    if (-not $Observer -and $stageTargets -contains $target) {
        $args += @("--ui-lab-stage", "auto")
    }

    $process = Start-UiLabProcess $exe $args $install
    $handle = Wait-UiLabWindow $process 12

    Start-Sleep -Seconds $InitialWaitSeconds

    $earlyShot = Join-Path $targetDir "screen_early.png"
    $lateShot = Join-Path $targetDir "screen_late.png"
    $snapshots = @()
    $captureError = $null

    try {
        if (-not $process.HasExited) {
            Capture-Window $handle $earlyShot
        }

        if ($ObserveSeconds -gt 0) {
            $observeStart = Get-Date
            $snapshotIndex = 0

            while (-not $process.HasExited -and ((Get-Date) - $observeStart).TotalSeconds -lt $ObserveSeconds) {
                Start-Sleep -Seconds ([Math]::Max(1, $SnapshotIntervalSeconds))
                $snapshotIndex += 1
                $snapshotPath = Join-Path $targetDir ("screen_observe_{0:D3}.png" -f $snapshotIndex)
                Capture-Window $handle $snapshotPath
                $snapshots += $snapshotPath
            }
        }
        else {
            Start-Sleep -Seconds $SecondCaptureDelaySeconds
        }

        if (-not $process.HasExited) {
            Capture-Window $handle $lateShot
        }
    }
    catch {
        $captureError = $_.Exception.Message
    }

    $waitSeconds = if ($AutoExitSeconds -gt 0) { [Math]::Max(2, $AutoExitSeconds - $InitialWaitSeconds) } else { 2 }
    $deadline = (Get-Date).AddSeconds($waitSeconds)
    while (-not $process.HasExited -and (Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 250
        $process.Refresh()
    }

    $stopped = $false
    if (-not $KeepRunning -and -not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
        $process.WaitForExit()
        $stopped = $true
    }

    $eventsPath = Join-Path $targetDir "ui_lab_events.jsonl"
    $records += [ordered]@{
        target = $target
        args = $args
        directory = $targetDir
        earlyScreenshot = if (Test-Path -LiteralPath $earlyShot) { $earlyShot } else { $null }
        lateScreenshot = if (Test-Path -LiteralPath $lateShot) { $lateShot } else { $null }
        snapshots = $snapshots
        events = if (Test-Path -LiteralPath $eventsPath) { $eventsPath } else { $null }
        stoppedByHarness = $stopped
        stillRunning = -not $process.HasExited
        processId = $process.Id
        exitCode = if ($process.HasExited) { $process.ExitCode } else { $null }
        captureError = $captureError
    }
}

$manifest = Join-Path $sessionDir "capture_manifest.json"
$records | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifest -Encoding UTF8

Write-Host "UI Lab runtime evidence: $sessionDir"
