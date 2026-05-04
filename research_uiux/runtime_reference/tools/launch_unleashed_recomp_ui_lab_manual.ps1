param(
    [string]$BuildExePath = "local_build_env\ur103clean\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe",
    [string]$InstallRoot = "Unleashed Recomp - Windows (Complete Installation) 1.0.3",
    [string]$SidecarName = "sward_ui_lab_runtime_manual",
    [string]$OutputRoot = "out\ui_lab_runtime_evidence",
    [string]$EvidenceLabel = "manual",
    [string]$LiveBridgeName = "sward_ui_lab_live_manual",
    [switch]$NoBuild,
    [switch]$NoCopy,
    [switch]$NoConsole,
    [switch]$HideOverlay,
    [switch]$UseWindowsTerminal,
    [switch]$NoWindowsTerminal
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..\..")

function Resolve-UiLabPath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return (Resolve-Path -LiteralPath $Path).Path
    }

    return (Resolve-Path -LiteralPath (Join-Path $repoRoot.Path $Path)).Path
}

function Quote-UiLabArgument([string]$Value) {
    return "'" + ($Value -replace "'", "''") + "'"
}

function Quote-NativeArgument([string]$Value) {
    if ($Value -notmatch '[\s"]') {
        return $Value
    }

    return '"' + ($Value -replace '"', '\"') + '"'
}

if (-not $NoBuild) {
    & (Join-Path $PSScriptRoot "build_unleashed_recomp_ui_lab.ps1")
}

$buildExe = Resolve-UiLabPath $BuildExePath
$buildRoot = Split-Path -Parent $buildExe
$installRootResolved = Resolve-UiLabPath $InstallRoot
$gameDefaultXex = Join-Path $installRootResolved "game\default.xex"

if (-not (Test-Path -LiteralPath $gameDefaultXex)) {
    throw "InstallRoot must be the Complete Installation game folder root. Missing: $gameDefaultXex"
}

$sidecarRoot = Join-Path $installRootResolved $SidecarName
$sidecarD3D12 = Join-Path $sidecarRoot "D3D12"
$sidecarExe = Join-Path $sidecarRoot "UnleashedRecomp.exe"

if (-not $NoCopy) {
    New-Item -ItemType Directory -Force -Path $sidecarRoot | Out-Null
    New-Item -ItemType Directory -Force -Path $sidecarD3D12 | Out-Null

    $copyPairs = @(
        @{ Source = $buildExe; Target = $sidecarExe },
        @{ Source = (Join-Path $buildRoot "dxcompiler.dll"); Target = (Join-Path $sidecarRoot "dxcompiler.dll") },
        @{ Source = (Join-Path $buildRoot "dxil.dll"); Target = (Join-Path $sidecarRoot "dxil.dll") },
        @{ Source = (Join-Path $buildRoot "D3D12\D3D12Core.dll"); Target = (Join-Path $sidecarD3D12 "D3D12Core.dll") },
        @{ Source = (Join-Path $buildRoot "D3D12\d3d12SDKLayers.dll"); Target = (Join-Path $sidecarD3D12 "d3d12SDKLayers.dll") }
    )

    foreach ($pair in $copyPairs) {
        if (-not (Test-Path -LiteralPath $pair.Source)) {
            throw "Missing runtime dependency: $($pair.Source)"
        }

        Copy-Item -LiteralPath $pair.Source -Destination $pair.Target -Force
    }
}

if (-not (Test-Path -LiteralPath $sidecarExe)) {
    throw "Sidecar runtime is missing. Re-run without -NoCopy: $sidecarExe"
}

$sessionStamp = Get-Date -Format "yyyyMMdd_HHmmss"
$sessionDir = Join-Path (Resolve-UiLabPath $OutputRoot) ("{0}_{1}" -f $EvidenceLabel, $sessionStamp)
$targetDir = Join-Path $sessionDir "manual-observer"
New-Item -ItemType Directory -Force -Path $targetDir | Out-Null

$args = @(
    "--use-cwd",
    "--ui-lab-observer",
    "--ui-lab-evidence-dir", $targetDir,
    "--ui-lab-live-bridge",
    "--ui-lab-live-bridge-name", $LiveBridgeName
)

if ($HideOverlay) {
    $args += @("--ui-lab-overlay", "off")
}

$runtimeArgumentLine = ($args | ForEach-Object { Quote-NativeArgument $_ }) -join " "

Write-Host "[*] SWARD UI Lab manual launch root: $installRootResolved"
Write-Host "[*] SWARD UI Lab sidecar runtime: $sidecarExe"
Write-Host "[*] SWARD UI Lab evidence dir: $targetDir"
Write-Host "[*] SWARD UI Lab live bridge: \\.\pipe\$LiveBridgeName"
Write-Host "[*] SWARD UI Lab args: $($args -join ' ')"

if ($NoConsole) {
    $process = Start-Process -FilePath $sidecarExe -WorkingDirectory $installRootResolved -ArgumentList $runtimeArgumentLine -PassThru
    $processCompanionPath = Join-Path $targetDir "process-companion.json"
    $processRecord = [ordered]@{
        pid = $process.Id
        processName = $process.ProcessName
        exe = $sidecarExe
        arguments = $runtimeArgumentLine
        evidenceDir = $targetDir
        liveBridge = "\\.\pipe\$LiveBridgeName"
        presentation = "game-window-native-overlay"
        companionConsolePid = $null
        companionTitle = $null
        launchedAt = (Get-Date).ToString('o')
    }
    $processRecord | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $processCompanionPath -Encoding UTF8
    Write-Host "[*] Attached UnleashedRecomp PID: $($process.Id)"
    Write-Host "[*] Process companion record: $processCompanionPath"
    Write-Host "[*] Presentation: game-window native SWARD UI Lab overlay; no companion console"
    return $process
}

$quotedExe = Quote-UiLabArgument $sidecarExe
$quotedInstall = Quote-UiLabArgument $installRootResolved
$quotedEvidence = Quote-UiLabArgument $targetDir
$quotedBridge = Quote-UiLabArgument $LiveBridgeName
$quotedRuntimeArgumentLine = Quote-UiLabArgument $runtimeArgumentLine
$processCompanionPath = Join-Path $targetDir "process-companion.json"
$quotedProcessCompanionPath = Quote-UiLabArgument $processCompanionPath
$consoleTitle = "SWARD UI Lab - UnleashedRecomp manual observer"
$escapedConsoleTitle = $consoleTitle -replace "'", "''"
$runnerScript = Join-Path $sessionDir "run_manual_observer.ps1"
$runnerContent = @"
`$Host.UI.RawUI.WindowTitle = '$escapedConsoleTitle'
`$runtimeExe = $quotedExe
`$installRoot = $quotedInstall
`$evidenceDir = $quotedEvidence
`$liveBridgeName = $quotedBridge
`$runtimeArgumentLine = $quotedRuntimeArgumentLine
`$processCompanionPath = $quotedProcessCompanionPath
Set-Location -LiteralPath `$installRoot
Write-Host '[*] SWARD UI Lab manual runtime console'
Write-Host ('[*] Runtime exe: {0}' -f `$runtimeExe)
Write-Host ('[*] Evidence dir: {0}' -f `$evidenceDir)
Write-Host ('[*] Live bridge: \\.\pipe\{0}' -f `$liveBridgeName)
Write-Host '[*] Launch mode: manual observer; no control automation'
`$process = Start-Process -FilePath `$runtimeExe -WorkingDirectory `$installRoot -ArgumentList `$runtimeArgumentLine -NoNewWindow -PassThru
`$pidTitle = 'SWARD UI Lab - UnleashedRecomp PID {0}' -f `$process.Id
`$Host.UI.RawUI.WindowTitle = `$pidTitle
Write-Host ('[*] Attached UnleashedRecomp PID: {0}' -f `$process.Id)
Write-Host ('[*] Companion console PID: {0}' -f `$PID)
`$processRecord = [ordered]@{
    pid = `$process.Id
    processName = `$process.ProcessName
    exe = `$runtimeExe
    arguments = `$runtimeArgumentLine
    evidenceDir = `$evidenceDir
    liveBridge = ('\\.\pipe\{0}' -f `$liveBridgeName)
    companionConsolePid = `$PID
    companionTitle = `$pidTitle
    launchedAt = (Get-Date).ToString('o')
}
`$processRecord | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath `$processCompanionPath -Encoding UTF8
while (-not `$process.HasExited) {
    Start-Sleep -Seconds 2
    try { `$process.Refresh() } catch { break }
}
try { `$exitCode = `$process.ExitCode } catch { `$exitCode = `$null }
Write-Host ''
Write-Host ('[*] UnleashedRecomp PID {0} exited with code: {1}' -f `$process.Id, `$exitCode)
Write-Host ('[*] Process companion record: {0}' -f `$processCompanionPath)
Write-Host '[*] Console kept open for evidence review.'
"@
Set-Content -LiteralPath $runnerScript -Value $runnerContent -Encoding UTF8

$powerShellArgs = @("-NoExit", "-ExecutionPolicy", "Bypass", "-File", $runnerScript)
$powerShellArgumentLine = ($powerShellArgs | ForEach-Object { Quote-NativeArgument $_ }) -join " "
$wt = Get-Command "wt.exe" -ErrorAction SilentlyContinue
if ($UseWindowsTerminal -and $wt -and -not $NoWindowsTerminal) {
    $wtArgs = @(
        "new-window",
        "--title", $consoleTitle,
        "--icon", $sidecarExe,
        "powershell"
    ) + $powerShellArgs
    $wtArgumentLine = ($wtArgs | ForEach-Object { Quote-NativeArgument $_ }) -join " "

    return Start-Process -FilePath $wt.Source -WorkingDirectory $installRootResolved -ArgumentList $wtArgumentLine -PassThru
}

try {
    $shortcutPath = Join-Path $sessionDir "SWARD UI Lab Manual Observer.lnk"
    $powerShellPath = (Get-Command "powershell.exe" -ErrorAction Stop).Source
    $wshShell = New-Object -ComObject WScript.Shell
    $shortcut = $wshShell.CreateShortcut($shortcutPath)
    $shortcut.TargetPath = $powerShellPath
    $shortcut.Arguments = $powerShellArgumentLine
    $shortcut.WorkingDirectory = $installRootResolved
    $shortcut.IconLocation = "$sidecarExe,0"
    $shortcut.Description = "SWARD UI Lab manual observer for UnleashedRecomp"
    $shortcut.WindowStyle = 1
    $shortcut.Save()

    return Start-Process -FilePath $shortcutPath -WorkingDirectory $installRootResolved -PassThru
}
catch {
    Write-Warning "Unable to create icon shortcut for manual console: $($_.Exception.Message)"
}

return Start-Process -FilePath "powershell" -WorkingDirectory $installRootResolved -ArgumentList $powerShellArgumentLine -PassThru
