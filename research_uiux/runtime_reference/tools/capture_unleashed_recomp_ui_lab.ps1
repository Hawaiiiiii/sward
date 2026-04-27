param(
    [string[]]$Targets = @(),
    [ValidateSet("early-game", "frontend", "all", "custom")]
    [string]$TargetSet = "early-game",
    [string]$ExePath = "W:\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe",
    [string]$InstallRoot = "Unleashed Recomp - Windows (Complete Installation) 1.0.3",
    [string]$OutputRoot = "out\ui_lab_runtime_evidence",
    [int]$InitialWaitSeconds = 8,
    [int]$SecondCaptureDelaySeconds = 3,
    [int]$AutoExitSeconds = 45,
    [int]$PostEvidenceDelaySeconds = 1,
    [int]$StagePostEvidenceDelaySeconds = 8,
    [int]$ObserveSeconds = 0,
    [int]$SnapshotIntervalSeconds = 10,
    [ValidateSet("input", "direct-context")]
    [string]$RoutePolicy = "direct-context",
    [bool]$NativeCapture = $true,
    [int]$NativeCaptureCount = 1,
    [int]$NativeCaptureIntervalFrames = 120,
    [bool]$NormalizeWindow = $true,
    [switch]$Observer,
    [switch]$HideOverlay,
    [switch]$SkipWindowScreenshots,
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
    public static extern IntPtr GetForegroundWindow();

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

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

function Test-ForegroundBelongsToProcess([int]$ProcessId) {
    $foregroundHandle = [UiLabNative]::GetForegroundWindow()

    if ($foregroundHandle -eq [IntPtr]::Zero) {
        return $false
    }

    $foregroundProcessId = 0
    [UiLabNative]::GetWindowThreadProcessId($foregroundHandle, [ref]$foregroundProcessId) | Out-Null
    return $foregroundProcessId -eq $ProcessId
}

function Capture-Window([IntPtr]$Handle, [string]$Path, [int]$ProcessId) {
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
            if (-not (Test-ForegroundBelongsToProcess $ProcessId)) {
                throw "PrintWindow did not produce a usable frame and the UI Lab window is not foreground; refusing desktop fallback."
            }

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

function Get-UiLabTargetSet([string]$Name) {
    switch ($Name) {
        "early-game" {
            return @("title-loop", "title-menu", "title-options", "loading", "sonic-hud")
        }
        "frontend" {
            return @("title-loop", "title-menu", "title-options", "loading")
        }
        "all" {
            return @("title-loop", "title-menu", "title-options", "loading", "sonic-hud", "status", "world-map", "tutorial", "result", "extra-stage-hud")
        }
        default {
            return @()
        }
    }
}

function Get-UiLabRequiredEvents([string]$Target) {
    switch ($Target) {
        "title-loop" {
            return @("target-csd-project-made")
        }
        "title-menu" {
            return @("target-csd-project-made", "title-menu-reached")
        }
        "title-options" {
            return @("target-csd-project-made", "title-options-accept-injected")
        }
        "loading" {
            return @("target-csd-project-made", "loading-requested", "loading-display-active")
        }
        "sonic-hud" {
            return @("stage-context-observed", "target-csd-project-made", "stage-target-csd-bound")
        }
        default {
            return @("first-presented-frame")
        }
    }
}

function Test-UiLabEvidenceEvents([string]$Target, [string]$EventsPath) {
    $requiredEvents = @(Get-UiLabRequiredEvents $Target)
    $observedEvents = New-Object "System.Collections.Generic.HashSet[string]"

    if (Test-Path -LiteralPath $EventsPath) {
        foreach ($line in Get-Content -LiteralPath $EventsPath) {
            if ([string]::IsNullOrWhiteSpace($line)) {
                continue
            }

            try {
                $event = $line | ConvertFrom-Json
                if ($event.event) {
                    [void]$observedEvents.Add([string]$event.event)
                }
            }
            catch {
                [void]$observedEvents.Add("unparseable-jsonl-line")
            }
        }
    }

    $missingEvents = @($requiredEvents | Where-Object { -not $observedEvents.Contains($_) })

    return [ordered]@{
        requiredEvents = $requiredEvents
        observedEvents = @($observedEvents | Sort-Object)
        missingEvents = $missingEvents
        passed = $missingEvents.Count -eq 0
    }
}

function Wait-UiLabEvidenceEvents([string]$Target, [string]$EventsPath, [int]$TimeoutSeconds, [System.Diagnostics.Process]$Process = $null) {
    $timeout = [Math]::Max(0, $TimeoutSeconds)
    $deadline = (Get-Date).AddSeconds($timeout)
    $checks = Test-UiLabEvidenceEvents $Target $EventsPath

    while (-not $checks.passed -and (Get-Date) -lt $deadline) {
        if ($null -ne $Process) {
            $Process.Refresh()

            if ($Process.HasExited) {
                break
            }
        }

        Start-Sleep -Milliseconds 500
        $checks = Test-UiLabEvidenceEvents $Target $EventsPath
    }

    return $checks
}

function Get-UiLabNativeFrameCaptures([string]$EventsPath) {
    if (-not (Test-Path -LiteralPath $EventsPath)) {
        return @()
    }

    $nativeCaptures = @()

    foreach ($line in Get-Content -LiteralPath $EventsPath) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }

        try {
            $event = $line | ConvertFrom-Json
        }
        catch {
            continue
        }

        if ($event.event -ne "native-frame-captured") {
            continue
        }

        $detail = [string]$event.detail
        $path = $null
        $width = $null
        $height = $null
        $index = $null
        $max = $null

        if ($detail -match "path=(.*?) width=") {
            $path = $Matches[1]
        }

        if ($detail -match "width=([0-9]+)") {
            $width = [int]$Matches[1]
        }

        if ($detail -match "height=([0-9]+)") {
            $height = [int]$Matches[1]
        }

        if ($detail -match "index=([0-9]+)") {
            $index = [int]$Matches[1]
        }

        if ($detail -match "max=([0-9]+)") {
            $max = [int]$Matches[1]
        }

        $nativeCaptures += [ordered]@{
            path = $path
            width = $width
            height = $height
            index = $index
            max = $max
            exists = if ($path) { Test-Path -LiteralPath $path } else { $false }
            detail = $detail
        }
    }

    return $nativeCaptures
}

function Get-UiLabNativeFrameCapture([string]$EventsPath) {
    $nativeCaptures = @(Get-UiLabNativeFrameCaptures $EventsPath)

    if ($nativeCaptures.Count -eq 0) {
        return $null
    }

    return $nativeCaptures[$nativeCaptures.Count - 1]
}

function Wait-UiLabNativeFrameCapture([string]$EventsPath, [int]$TimeoutSeconds, [System.Diagnostics.Process]$Process = $null) {
    $timeout = [Math]::Max(0, $TimeoutSeconds)
    $deadline = (Get-Date).AddSeconds($timeout)
    $nativeCapture = Get-UiLabNativeFrameCapture $EventsPath

    while (($null -eq $nativeCapture -or -not $nativeCapture.exists) -and (Get-Date) -lt $deadline) {
        if ($null -ne $Process) {
            $Process.Refresh()

            if ($Process.HasExited) {
                break
            }
        }

        Start-Sleep -Milliseconds 500
        $nativeCapture = Get-UiLabNativeFrameCapture $EventsPath
    }

    return $nativeCapture
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
    if ($Targets.Count -eq 0) {
        $Targets = Get-UiLabTargetSet $TargetSet
    }

    foreach ($target in $Targets) {
        $expandedTargets += $target -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ }
    }
}

foreach ($target in $expandedTargets) {
    $safeTarget = $target -replace "[^A-Za-z0-9_.-]", "_"
    $targetDir = Join-Path $sessionDir $safeTarget
    New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
    $eventsPath = Join-Path $targetDir "ui_lab_events.jsonl"

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

    if ($NativeCapture) {
        $args += @("--ui-lab-native-capture")
        $args += @("--ui-lab-native-capture-dir", $targetDir)
        $args += @("--ui-lab-native-capture-count", "$([Math]::Max(1, $NativeCaptureCount))")
        $args += @("--ui-lab-native-capture-interval-frames", "$([Math]::Max(1, $NativeCaptureIntervalFrames))")
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
    $evidenceReady = $false
    $lateCaptureReason = "fixed-delay"
    $nativeFrameCapture = $null
    $nativeFrameCaptures = @()
    $skipLateWindowCapture = $false

    try {
        if (-not $SkipWindowScreenshots -and -not $process.HasExited) {
            Capture-Window $handle $earlyShot $process.Id
        }

        if ($ObserveSeconds -gt 0) {
            $observeStart = Get-Date
            $snapshotIndex = 0

            while (-not $process.HasExited -and ((Get-Date) - $observeStart).TotalSeconds -lt $ObserveSeconds) {
                Start-Sleep -Seconds ([Math]::Max(1, $SnapshotIntervalSeconds))
                $process.Refresh()

                if ($process.HasExited) {
                    break
                }

                if ($SkipWindowScreenshots) {
                    continue
                }

                $snapshotIndex += 1
                $snapshotPath = Join-Path $targetDir ("screen_observe_{0:D3}.png" -f $snapshotIndex)
                Capture-Window $handle $snapshotPath $process.Id
                $snapshots += $snapshotPath
            }
        }
        else {
            if (-not $Observer) {
                $maxEvidenceWaitSeconds = [Math]::Max(0, $SecondCaptureDelaySeconds)

                if ($AutoExitSeconds -gt 0) {
                    $remainingWindowSeconds = $AutoExitSeconds - $InitialWaitSeconds - 2
                    $maxEvidenceWaitSeconds = [Math]::Max($maxEvidenceWaitSeconds, $remainingWindowSeconds)
                }

                $waitedEvidence = Wait-UiLabEvidenceEvents $target $eventsPath $maxEvidenceWaitSeconds $process
                $evidenceReady = $waitedEvidence.passed

                if ($evidenceReady) {
                    $lateCaptureReason = "required-events-observed"
                    $settleSeconds = if ($stageTargets -contains $target) { $StagePostEvidenceDelaySeconds } else { $PostEvidenceDelaySeconds }

                    if ($settleSeconds -gt 0) {
                        Start-Sleep -Seconds $settleSeconds
                    }
                }
                else {
                    $lateCaptureReason = "required-events-timeout"
                }
            }
            else {
                Start-Sleep -Seconds $SecondCaptureDelaySeconds
            }
        }

        if ($NativeCapture) {
            $nativeWaitSeconds = if ($evidenceReady) { [Math]::Max(1, $PostEvidenceDelaySeconds) } else { [Math]::Max(1, $SecondCaptureDelaySeconds) }
            $nativeFrameCapture = Wait-UiLabNativeFrameCapture $eventsPath $nativeWaitSeconds $process

            if ($null -ne $nativeFrameCapture -and $nativeFrameCapture.exists) {
                $lateCaptureReason = "native-frame-captured"
                $skipLateWindowCapture = $true
            }
        }

        if ($SkipWindowScreenshots -and $lateCaptureReason -eq "fixed-delay") {
            $lateCaptureReason = "window-screenshots-skipped"
        }

        if (-not $SkipWindowScreenshots -and -not $skipLateWindowCapture -and -not $process.HasExited) {
            Capture-Window $handle $lateShot $process.Id
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

    $evidenceChecks = Test-UiLabEvidenceEvents $target $eventsPath
    $nativeFrameCaptures = @(Get-UiLabNativeFrameCaptures $eventsPath)
    if ($null -eq $nativeFrameCapture) {
        if ($nativeFrameCaptures.Count -gt 0) {
            $nativeFrameCapture = $nativeFrameCaptures[$nativeFrameCaptures.Count - 1]
        }
    }
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
        evidenceReady = $evidenceReady
        lateCaptureReason = $lateCaptureReason
        windowScreenshotsSkipped = [bool]$SkipWindowScreenshots
        evidenceChecks = $evidenceChecks
        nativeFrameCapture = $nativeFrameCapture
        nativeFrameCaptures = $nativeFrameCaptures
    }

    if ($evidenceChecks.passed) {
        Write-Host "[$target] evidence PASS"
    }
    else {
        Write-Warning "[$target] evidence missing: $($evidenceChecks.missingEvents -join ', ')"
    }
}

$manifest = Join-Path $sessionDir "capture_manifest.json"
$records | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifest -Encoding UTF8

Write-Host "UI Lab runtime evidence: $sessionDir"
