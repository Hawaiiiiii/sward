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
    [bool]$NativeCapture = $false,
    [int]$NativeCaptureCount = 1,
    [int]$NativeCaptureIntervalFrames = 120,
    [switch]$RequireNativeRgbSignal,
    [bool]$LiveBridge = $true,
    [string]$LiveBridgeName = "sward_ui_lab_live",
    [switch]$UseLiveBridgeReadiness,
    [switch]$UseUniqueLiveBridgeName,
    [int]$LiveBridgeReadinessPollMilliseconds = 250,
    [bool]$NormalizeWindow = $true,
    [switch]$Observer,
    [switch]$HideOverlay,
    [switch]$SkipWindowScreenshots,
    [switch]$KeepRunning,
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"

if (-not $PSBoundParameters.ContainsKey("UseUniqueLiveBridgeName")) {
    $UseUniqueLiveBridgeName = [bool]$UseLiveBridgeReadiness
}

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

function Prepare-UiLabWindow([IntPtr]$Handle) {
    if ($Handle -eq [IntPtr]::Zero) {
        throw "Cannot prepare UI Lab window because the process has no window handle."
    }

    [UiLabNative]::ShowWindow($Handle, 9) | Out-Null

    if ($NormalizeWindow) {
        [UiLabNative]::MoveWindow($Handle, 64, 64, 1280, 720, $true) | Out-Null
    }

    [UiLabNative]::SetForegroundWindow($Handle) | Out-Null
    Start-Sleep -Milliseconds 500
}

function Capture-Window([IntPtr]$Handle, [string]$Path, [int]$ProcessId) {
    Prepare-UiLabWindow $Handle

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
            return @("title-loop", "title-menu", "title-options", "loading", "sonic-hud", "pause", "status", "world-map", "tutorial", "result", "extra-stage-hud")
        }
        default {
            return @()
        }
    }
}

function Get-UiLabEffectiveLiveBridgeName(
    [string]$BaseName,
    [string]$SessionName,
    [string]$Target,
    [bool]$Unique
) {
    if (-not $Unique) {
        return $BaseName
    }

    $safeSession = $SessionName -replace "[^A-Za-z0-9_.-]", "_"
    $safeTarget = $Target -replace "[^A-Za-z0-9_.-]", "_"
    $safeBase = $BaseName -replace "[^A-Za-z0-9_.-]", "_"
    # Default unique pipe shape: sward_ui_lab_live_<session>_<target>.
    return "$safeBase`_$safeSession`_$safeTarget"
}

function Get-UiLabRequiredEvents([string]$Target) {
    switch ($Target) {
        "title-loop" {
            return @("target-csd-project-made")
        }
        "title-menu" {
            return @("target-csd-project-made", "title-press-start-accept-injected", "title-menu-reached", "title-menu-post-press-start-held", "title-menu-post-press-start-ready", "title-menu-visible")
        }
        "title-options" {
            return @("target-csd-project-made", "title-options-accept-injected")
        }
        "loading" {
            return @("target-csd-project-made", "loading-requested", "loading-display-active")
        }
        "sonic-hud" {
            return @("stage-context-observed", "target-csd-project-made", "stage-target-csd-bound", "sonic-hud-ready")
        }
        "pause" {
            return @("stage-context-observed", "target-csd-project-made", "stage-target-csd-bound", "pause-owner-observed", "pause-route-start-injected", "pause-target-ready", "pause-ready")
        }
        "tutorial" {
            return @("stage-context-observed", "target-csd-project-made", "stage-target-csd-bound", "tutorial-ready")
        }
        "result" {
            return @("stage-context-observed", "target-csd-project-made", "stage-target-csd-bound", "result-ready")
        }
        "extra-stage-hud" {
            return @("stage-context-observed", "target-csd-project-made", "stage-target-csd-bound", "extra-stage-hud-ready")
        }
        default {
            return @("first-presented-frame")
        }
    }
}

function Get-UiLabNativeCapturePlan([string]$Target, [int]$RequestedCount, [int]$RequestedIntervalFrames) {
    $effectiveNativeCaptureCount = [Math]::Max(1, $RequestedCount)
    $effectiveNativeCaptureIntervalFrames = [Math]::Max(1, $RequestedIntervalFrames)

    switch ($Target) {
        "loading" {
            $effectiveNativeCaptureCount = [Math]::Max($effectiveNativeCaptureCount, 12)
            $effectiveNativeCaptureIntervalFrames = [Math]::Min($effectiveNativeCaptureIntervalFrames, 15)
        }
        "pause" {
            $effectiveNativeCaptureCount = [Math]::Max($effectiveNativeCaptureCount, 4)
            $effectiveNativeCaptureIntervalFrames = [Math]::Min($effectiveNativeCaptureIntervalFrames, 30)
        }
        { $_ -in @("sonic-hud", "tutorial", "result", "extra-stage-hud") } {
            $effectiveNativeCaptureCount = [Math]::Max($effectiveNativeCaptureCount, 6)
            $effectiveNativeCaptureIntervalFrames = [Math]::Min($effectiveNativeCaptureIntervalFrames, 30)
        }
        "title-menu" {
            $effectiveNativeCaptureCount = [Math]::Max($effectiveNativeCaptureCount, 14)
            $effectiveNativeCaptureIntervalFrames = [Math]::Max($effectiveNativeCaptureIntervalFrames, 60)
        }
        "title-options" {
            $effectiveNativeCaptureCount = [Math]::Max($effectiveNativeCaptureCount, 6)
            $effectiveNativeCaptureIntervalFrames = [Math]::Min($effectiveNativeCaptureIntervalFrames, 30)
        }
    }

    return [ordered]@{
        requestedCount = $RequestedCount
        requestedIntervalFrames = $RequestedIntervalFrames
        effectiveNativeCaptureCount = $effectiveNativeCaptureCount
        effectiveNativeCaptureIntervalFrames = $effectiveNativeCaptureIntervalFrames
    }
}

function Get-UiLabEffectiveAutoExitSeconds([string]$Target, [int]$RequestedAutoExitSeconds) {
    if ($RequestedAutoExitSeconds -le 0) {
        return $RequestedAutoExitSeconds
    }

    switch ($Target) {
        "pause" {
            return [Math]::Max($RequestedAutoExitSeconds, 95)
        }
        default {
            return $RequestedAutoExitSeconds
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

function Test-UiLabDurableEvidenceEvent([string]$EventsPath, [string]$EventName) {
    if ([string]::IsNullOrWhiteSpace($EventName)) {
        return $true
    }

    if (-not (Test-Path -LiteralPath $EventsPath)) {
        return $false
    }

    foreach ($line in Get-Content -LiteralPath $EventsPath) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }

        try {
            $event = $line | ConvertFrom-Json
            if ([string]$event.event -eq $EventName) {
                return $true
            }
        }
        catch {
        }
    }

    return $false
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

function Read-UiLabBridgeResponse([System.IO.Pipes.NamedPipeClientStream]$Pipe, [int]$TimeoutMilliseconds) {
    try {
        $Pipe.ReadTimeout = $TimeoutMilliseconds
    }
    catch {
    }

    $buffer = New-Object byte[] 65536
    $builder = [System.Text.StringBuilder]::new()

    do {
        $read = $Pipe.Read($buffer, 0, $buffer.Length)
        if ($read -le 0) {
            break
        }

        [void]$builder.Append([System.Text.Encoding]::UTF8.GetString($buffer, 0, $read))
    } while ($Pipe.IsMessageComplete -eq $false)

    return $builder.ToString()
}

function Invoke-UiLabBridgeCommand([string]$CommandText, [string]$PipeName, [int]$TimeoutMilliseconds = 1000) {
    $pipe = [System.IO.Pipes.NamedPipeClientStream]::new(
        ".",
        $PipeName,
        [System.IO.Pipes.PipeDirection]::InOut,
        [System.IO.Pipes.PipeOptions]::None)

    try {
        $pipe.Connect($TimeoutMilliseconds)
        $pipe.ReadMode = [System.IO.Pipes.PipeTransmissionMode]::Message

        $bytes = [System.Text.Encoding]::UTF8.GetBytes($CommandText)
        $pipe.Write($bytes, 0, $bytes.Length)
        $pipe.Flush()

        return Read-UiLabBridgeResponse $pipe $TimeoutMilliseconds
    }
    finally {
        $pipe.Dispose()
    }
}

function Get-UiLabLiveBridgeState([string]$PipeName, [int]$TimeoutMilliseconds = 1000) {
    try {
        $raw = Invoke-UiLabBridgeCommand "state" $PipeName $TimeoutMilliseconds
        $state = $raw | ConvertFrom-Json

        return [ordered]@{
            ok = $true
            raw = $raw
            state = $state
            error = $null
        }
    }
    catch {
        return [ordered]@{
            ok = $false
            raw = $null
            state = $null
            error = $_.Exception.Message
        }
    }
}

function Test-UiLabLiveBridgeReadiness([string]$Target, [object]$State) {
    $observedEvents = New-Object "System.Collections.Generic.HashSet[string]"

    if ($null -ne $State -and $null -ne $State.recentEvents) {
        foreach ($event in @($State.recentEvents)) {
            if ($event.event) {
                [void]$observedEvents.Add([string]$event.event)
            }
        }
    }

    $readiness = if ($null -ne $State) { $State.readiness } else { $null }
    $titleMenuVisible = $null -ne $readiness -and [bool]$readiness.titleMenuVisible
    $loadingActive = $null -ne $readiness -and [bool]$readiness.loadingActive
    $stageContextObserved = $null -ne $readiness -and [bool]$readiness.stageContextObserved
    $targetCsdObserved = $null -ne $State -and [bool]$State.targetCsdObserved
    $stageTargetReady = $null -ne $readiness -and [bool]$readiness.stageTargetReady
    $stageTargetReadyEvent = if ($null -ne $State) { [string]$State.stageReadyEvent } else { "" }
    $route = if ($null -ne $State) { [string]$State.route } else { "" }
    $passed = $false

    switch ($Target) {
        "title-loop" {
            $passed = $targetCsdObserved -or $observedEvents.Contains("target-csd-project-made")
        }
        "title-menu" {
            $passed = $titleMenuVisible -or ($null -ne $State -and [bool]$State.titleMenuVisualReady)
        }
        "title-options" {
            $passed = $observedEvents.Contains("title-options-accept-injected") -or $route -eq "title options accept injected"
        }
        "loading" {
            $passed = $loadingActive -or ($null -ne $State -and [bool]$State.loadingDisplayActive) -or $observedEvents.Contains("loading-display-active")
        }
        "sonic-hud" {
            $passed = ($stageContextObserved -and $targetCsdObserved -and $stageTargetReady) -or $stageTargetReadyEvent -eq "sonic-hud-ready"
        }
        "pause" {
            $passed = (($stageContextObserved -and $targetCsdObserved -and $stageTargetReady) -or $stageTargetReadyEvent -eq "pause-ready" -or $route -eq "pause target ready") -and $observedEvents.Contains("pause-target-ready")
        }
        "tutorial" {
            $passed = ($stageContextObserved -and $targetCsdObserved -and $stageTargetReady) -or $stageTargetReadyEvent -eq "tutorial-ready"
        }
        "result" {
            $passed = ($stageContextObserved -and $targetCsdObserved -and $stageTargetReady) -or $stageTargetReadyEvent -eq "result-ready"
        }
        "extra-stage-hud" {
            $passed = ($stageContextObserved -and $targetCsdObserved -and $stageTargetReady) -or $stageTargetReadyEvent -eq "extra-stage-hud-ready"
        }
        default {
            $passed = $observedEvents.Contains("first-presented-frame")
        }
    }

    $missingEvents = @()
    if (-not $passed) {
        $missingEvents = @(Get-UiLabRequiredEvents $Target | Where-Object { -not $observedEvents.Contains($_) })
    }

    return [ordered]@{
        passed = $passed
        source = "liveBridge"
        target = $Target
        targetState = if ($null -ne $State) { [string]$State.target } else { $null }
        route = $route
        stateFrame = if ($null -ne $State) { $State.frame } else { $null }
        observedEvents = @($observedEvents | Sort-Object)
        missingEvents = $missingEvents
        "titleMenuVisible" = $titleMenuVisible
        "loadingActive" = $loadingActive
        "stageContextObserved" = $stageContextObserved
        "targetCsdObserved" = $targetCsdObserved
        "stageTargetReady" = $stageTargetReady
        "stageTargetReadyEvent" = $stageTargetReadyEvent
        state = $State
    }
}

function Wait-UiLabLiveBridgeReadiness(
    [string]$Target,
    [string]$PipeName,
    [int]$TimeoutSeconds,
    [System.Diagnostics.Process]$Process = $null,
    [string]$EventsPath = ""
) {
    $timeout = [Math]::Max(0, $TimeoutSeconds)
    $deadline = (Get-Date).AddSeconds($timeout)
    $durableEvidenceEvent = if ($Target -eq "title-menu") { "title-menu-visible" } else { "" }
    $checks = [ordered]@{
        passed = $false
        source = "liveBridge"
        target = $Target
        durableEvidenceEvent = $durableEvidenceEvent
        durableEvidencePassed = [string]::IsNullOrWhiteSpace($durableEvidenceEvent)
        error = "not checked"
    }

    while ((Get-Date) -lt $deadline) {
        if ($null -ne $Process) {
            $Process.Refresh()

            if ($Process.HasExited) {
                break
            }
        }

        $stateResponse = Get-UiLabLiveBridgeState $PipeName 1000
        if ($stateResponse.ok) {
            $checks = Test-UiLabLiveBridgeReadiness $Target $stateResponse.state
            $checks["durableEvidenceEvent"] = $durableEvidenceEvent
            $checks["durableEvidencePassed"] = Test-UiLabDurableEvidenceEvent $EventsPath $durableEvidenceEvent

            if ($checks.passed) {
                if ($checks.durableEvidencePassed) {
                    return $checks
                }

                $checks["passed"] = $false
                $checks["error"] = "waiting for durable JSONL event $durableEvidenceEvent"
            }
        }
        else {
            $checks = [ordered]@{
                passed = $false
                source = "liveBridge"
                target = $Target
                durableEvidenceEvent = $durableEvidenceEvent
                durableEvidencePassed = Test-UiLabDurableEvidenceEvent $EventsPath $durableEvidenceEvent
                error = $stateResponse.error
            }
        }

        Start-Sleep -Milliseconds ([Math]::Max(50, $LiveBridgeReadinessPollMilliseconds))
    }

    return $checks
}

function Get-BmpSignalStats([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 54 -or $bytes[0] -ne [byte][char]'B' -or $bytes[1] -ne [byte][char]'M') {
        return [ordered]@{
            validBmp = $false
            error = "not a BMP file"
        }
    }

    $pixelOffset = [BitConverter]::ToInt32($bytes, 10)
    $bmpWidth = [BitConverter]::ToInt32($bytes, 18)
    $bmpHeight = [Math]::Abs([BitConverter]::ToInt32($bytes, 22))
    $bitsPerPixel = [BitConverter]::ToInt16($bytes, 28)

    if ($bitsPerPixel -ne 32 -or $pixelOffset -lt 0 -or $pixelOffset -ge $bytes.Length) {
        return [ordered]@{
            validBmp = $false
            bytes = $bytes.Length
            bmpWidth = $bmpWidth
            bmpHeight = $bmpHeight
            bitsPerPixel = $bitsPerPixel
            pixelOffset = $pixelOffset
            error = "unsupported BMP layout"
        }
    }

    [Int64]$rgbSum = 0
    [Int64]$alphaSum = 0
    [Int64]$rgbNonZeroBytes = 0
    [Int64]$alphaNonZeroBytes = 0

    for ($i = $pixelOffset; $i + 3 -lt $bytes.Length; $i += 4) {
        $b = [int]$bytes[$i]
        $g = [int]$bytes[$i + 1]
        $r = [int]$bytes[$i + 2]
        $a = [int]$bytes[$i + 3]

        $rgbSum += $b + $g + $r
        $alphaSum += $a

        if ($b -ne 0) { ++$rgbNonZeroBytes }
        if ($g -ne 0) { ++$rgbNonZeroBytes }
        if ($r -ne 0) { ++$rgbNonZeroBytes }
        if ($a -ne 0) { ++$alphaNonZeroBytes }
    }

    return [ordered]@{
        validBmp = $true
        bytes = $bytes.Length
        bmpWidth = $bmpWidth
        bmpHeight = $bmpHeight
        bitsPerPixel = $bitsPerPixel
        pixelOffset = $pixelOffset
        rgbSum = $rgbSum
        alphaSum = $alphaSum
        rgbNonZeroBytes = $rgbNonZeroBytes
        alphaNonZeroBytes = $alphaNonZeroBytes
        rgbNonBlack = $rgbSum -gt 0
    }
}

function Get-UiLabNativeFramePreferenceScore([string]$Target, [string]$Route, [string]$Csd, [string]$Stage) {
    $score = 0

    switch ($Target) {
        "title-loop" {
            if ($Route -eq "loading display ended") { $score += 300 }
            if ($Csd -eq "ui_title") { $score += 80 }
        }
        "title-menu" {
            if ($Route -eq "title menu visual ready") { $score += 500 }
            if ($Route -eq "title menu reached") { $score += 220 }
            if ($Csd -eq "ui_title") { $score += 80 }
        }
        "title-options" {
            if ($Route -eq "title options accept injected") { $score += 300 }
            if ($Csd -eq "ui_title") { $score += 80 }
        }
        "loading" {
            if ($Route -eq "loading display active") { $score += 300 }
            if ($Route -eq "loading request observed") { $score += 180 }
            if ($Route -eq "loading display ended") { $score -= 200 }
            if ($Csd -eq "ui_loading") { $score += 80 }
        }
        { $_ -in @("sonic-hud", "tutorial", "result", "extra-stage-hud", "pause") } {
            if ($Route -eq "pause target ready") { $score += 380 }
            if ($Route -eq "stage target ready") { $score += 340 }
            if ($Route -eq "stage target csd bound") { $score += 300 }
            if ($Route -eq "stage context live") { $score += 220 }
            if ($Route -eq "loading display active") { $score += 120 }
            if ($Route -eq "loading request observed") { $score -= 80 }
            if ($Stage -eq "stage context observed") { $score += 80 }
            if ($Csd -eq "ui_playscreen") { $score += 80 }
            if ($Csd -eq "ui_pause") { $score += 100 }
        }
        default {
            if ($Route -match "target csd|context|menu|options|loading display active") { $score += 120 }
            if ($Csd) { $score += 40 }
        }
    }

    return $score
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

        $exists = if ($path) { Test-Path -LiteralPath $path } else { $false }
        $signal = if ($exists) { Get-BmpSignalStats $path } else { $null }
        $target = [string]$event.target
        $route = [string]$event.route
        $csd = [string]$event.csd
        $stage = [string]$event.stage
        $preferredScore = Get-UiLabNativeFramePreferenceScore $target $route $csd $stage

        $nativeCaptures += [ordered]@{
            path = $path
            width = $width
            height = $height
            index = $index
            max = $max
            eventTime = $event.time
            eventFrame = $event.frame
            target = $target
            route = $route
            csd = $csd
            stage = $stage
            preferredScore = $preferredScore
            exists = $exists
            signal = $signal
            rgbNonBlack = if ($null -ne $signal -and $signal.Contains("rgbNonBlack")) { $signal.rgbNonBlack } else { $false }
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

function Get-UiLabNativeFrameSignalSummary([object[]]$NativeCaptures) {
    $validCaptures = @($NativeCaptures | Where-Object { $_.exists -and $null -ne $_.signal -and $_.signal.validBmp })
    $nonBlackCaptures = @($validCaptures | Where-Object { $_.rgbNonBlack })
    $bestCapture = $null

    if ($validCaptures.Count -gt 0) {
        $bestCapture = $validCaptures |
            Sort-Object `
                @{ Expression = { if ($_.rgbNonBlack) { 1 } else { 0 } }; Descending = $true },
                @{ Expression = { if ($_.preferredScore) { [int]$_.preferredScore } else { 0 } }; Descending = $true },
                @{ Expression = { if ($_.target -eq "title-menu" -and $_.route -eq "title menu visual ready" -and $null -ne $_.index) { [int]$_.index } else { 0 } }; Descending = $true },
                @{ Expression = {
                    if ($_.target -eq "pause" -and $_.route -eq "pause target ready" -and $null -ne $_.index) {
                        $pauseIndex = [int]$_.index
                        if ($pauseIndex -ge 2 -and $pauseIndex -le 4) { 100 + $pauseIndex }
                        elseif ($pauseIndex -eq 1) { 1 }
                        else { 0 }
                    }
                    else {
                        0
                    }
                }; Descending = $true },
                @{ Expression = { if ($_.signal.rgbSum) { [Int64]$_.signal.rgbSum } else { 0 } }; Descending = $true } |
            Select-Object -First 1
    }

    return [ordered]@{
        captures = $NativeCaptures.Count
        validBmp = $validCaptures.Count
        rgbNonBlack = $nonBlackCaptures.Count
        allBlack = $validCaptures.Count -gt 0 -and $nonBlackCaptures.Count -eq 0
        bestIndex = if ($null -ne $bestCapture) { $bestCapture.index } else { $null }
        bestRoute = if ($null -ne $bestCapture) { $bestCapture.route } else { $null }
        bestTarget = if ($null -ne $bestCapture) { $bestCapture.target } else { $null }
        bestPreferenceScore = if ($null -ne $bestCapture) { $bestCapture.preferredScore } else { $null }
        bestRgbSum = if ($null -ne $bestCapture) { $bestCapture.signal.rgbSum } else { $null }
        bestPath = if ($null -ne $bestCapture) { $bestCapture.path } else { $null }
    }
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

$stageTargets = @("sonic-hud", "extra-stage-hud", "tutorial", "result", "pause")
$records = @()
$expandedTargets = @()
$requiredNativeSignalFailed = $false

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
    $liveStatePath = Join-Path $targetDir "ui_lab_live_state.json"
    $nativeCapturePlan = Get-UiLabNativeCapturePlan $target $NativeCaptureCount $NativeCaptureIntervalFrames
    $effectiveAutoExitSeconds = Get-UiLabEffectiveAutoExitSeconds $target $AutoExitSeconds
    $effectiveLiveBridgeName = Get-UiLabEffectiveLiveBridgeName $LiveBridgeName (Split-Path -Leaf $sessionDir) $safeTarget ([bool]$UseUniqueLiveBridgeName)

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

    if ($LiveBridge) {
        $args += @("--ui-lab-live-bridge")
        $args += @("--ui-lab-live-bridge-name", $effectiveLiveBridgeName)
    }
    else {
        $args += @("--ui-lab-live-bridge", "off")
    }

    if ($NativeCapture) {
        $args += @("--ui-lab-native-capture")
        $args += @("--ui-lab-native-capture-dir", $targetDir)
        $args += @("--ui-lab-native-capture-count", "$($nativeCapturePlan.effectiveNativeCaptureCount)")
        $args += @("--ui-lab-native-capture-interval-frames", "$($nativeCapturePlan.effectiveNativeCaptureIntervalFrames)")
    }

    if (-not $KeepRunning -and $effectiveAutoExitSeconds -gt 0) {
        $args += @("--ui-lab-auto-exit", "$effectiveAutoExitSeconds")
    }

    if (-not $Observer -and $stageTargets -contains $target) {
        $args += @("--ui-lab-stage", "auto")
    }

    $process = Start-UiLabProcess $exe $args $install
    $handle = Wait-UiLabWindow $process 12

    if ($SkipWindowScreenshots -and -not $process.HasExited) {
        Prepare-UiLabWindow $handle
    }

    Start-Sleep -Seconds $InitialWaitSeconds

    $earlyShot = Join-Path $targetDir "screen_early.png"
    $lateShot = Join-Path $targetDir "screen_late.png"
    $snapshots = @()
    $captureError = $null
    $evidenceReady = $false
    $readinessSource = "jsonl"
    $liveBridgeReadiness = $null
    $liveBridgeState = $null
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

                if ($effectiveAutoExitSeconds -gt 0) {
                    $remainingWindowSeconds = $effectiveAutoExitSeconds - $InitialWaitSeconds - 2
                    $maxEvidenceWaitSeconds = [Math]::Max($maxEvidenceWaitSeconds, $remainingWindowSeconds)
                }

                if ($UseLiveBridgeReadiness -and $LiveBridge) {
                    $liveBridgeReadiness = Wait-UiLabLiveBridgeReadiness $target $effectiveLiveBridgeName $maxEvidenceWaitSeconds $process $eventsPath
                    $liveBridgeState = if ($null -ne $liveBridgeReadiness -and $liveBridgeReadiness.Contains("state")) { $liveBridgeReadiness.state } else { $null }
                    $evidenceReady = $liveBridgeReadiness.passed

                    if ($evidenceReady) {
                        $readinessSource = "live-bridge"
                        $lateCaptureReason = if (-not [string]::IsNullOrWhiteSpace([string]$liveBridgeReadiness.durableEvidenceEvent)) {
                            "required-events-observed-via-live-bridge-and-jsonl"
                        } else {
                            "required-events-observed-via-live-bridge"
                        }
                    }
                    else {
                        $lateCaptureReason = if ($null -ne $liveBridgeReadiness -and -not [string]::IsNullOrWhiteSpace([string]$liveBridgeReadiness.durableEvidenceEvent)) {
                            "required-events-timeout-via-live-bridge-jsonl"
                        } else {
                            "required-events-timeout-via-live-bridge"
                        }
                    }
                }

                if (-not $evidenceReady) {
                    $waitedEvidence = Wait-UiLabEvidenceEvents $target $eventsPath $maxEvidenceWaitSeconds $process
                    $evidenceReady = $waitedEvidence.passed

                    if ($evidenceReady) {
                        $readinessSource = "jsonl"
                        $lateCaptureReason = "required-events-observed"
                    }
                    elseif (-not $UseLiveBridgeReadiness) {
                        $lateCaptureReason = "required-events-timeout"
                    }
                }

                if ($evidenceReady) {
                    $settleSeconds = if ($stageTargets -contains $target) { $StagePostEvidenceDelaySeconds } else { $PostEvidenceDelaySeconds }

                    if ($settleSeconds -gt 0) {
                        Start-Sleep -Seconds $settleSeconds
                    }
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

    $waitSeconds = if ($effectiveAutoExitSeconds -gt 0) { [Math]::Max(2, $effectiveAutoExitSeconds - $InitialWaitSeconds) } else { 2 }
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
    $nativeFrameSignalSummary = Get-UiLabNativeFrameSignalSummary $nativeFrameCaptures
    $nativeSignalRequired = $NativeCapture -and [bool]$RequireNativeRgbSignal
    $nativeSignalPassed = -not $nativeSignalRequired -or ([int]$nativeFrameSignalSummary.rgbNonBlack -gt 0)
    if (-not $nativeSignalPassed) {
        $requiredNativeSignalFailed = $true
    }

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
        liveStatePath = if (Test-Path -LiteralPath $liveStatePath) { $liveStatePath } else { $null }
        stoppedByHarness = $stopped
        stillRunning = -not $process.HasExited
        processId = $process.Id
        exitCode = if ($process.HasExited) { $process.ExitCode } else { $null }
        captureError = $captureError
        evidenceReady = $evidenceReady
        readinessSource = $readinessSource
        liveBridgeReadiness = $liveBridgeReadiness
        liveBridgeState = $liveBridgeState
        lateCaptureReason = $lateCaptureReason
        windowScreenshotsSkipped = [bool]$SkipWindowScreenshots
        evidenceChecks = $evidenceChecks
        nativeFrameCapture = $nativeFrameCapture
        nativeFrameCaptures = $nativeFrameCaptures
        nativeFrameSignalSummary = $nativeFrameSignalSummary
        nativeCapturePlan = $nativeCapturePlan
        requestedAutoExitSeconds = $AutoExitSeconds
        effectiveAutoExitSeconds = $effectiveAutoExitSeconds
        effectiveLiveBridgeName = if ($LiveBridge) { $effectiveLiveBridgeName } else { $null }
        liveBridgeName = if ($LiveBridge) { $effectiveLiveBridgeName } else { $null }
        nativeSignalRequired = $nativeSignalRequired
        nativeSignalPassed = $nativeSignalPassed
    }

    if ($evidenceChecks.passed -and $nativeSignalPassed) {
        Write-Host "[$target] evidence PASS"
    }
    else {
        if (-not $evidenceChecks.passed) {
            Write-Warning "[$target] evidence missing: $($evidenceChecks.missingEvents -join ', ')"
        }

        if (-not $nativeSignalPassed) {
            Write-Warning "[$target] native BMP RGB signal missing"
        }
    }
}

$manifest = Join-Path $sessionDir "capture_manifest.json"
$records | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifest -Encoding UTF8

Write-Host "UI Lab runtime evidence: $sessionDir"

if ($requiredNativeSignalFailed) {
    throw "One or more UI Lab native captures did not produce RGB-nonblack BMP evidence."
}
