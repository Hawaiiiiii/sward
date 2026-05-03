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
    [switch]$HideOverlay
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

Write-Host "[*] SWARD UI Lab manual launch root: $installRootResolved"
Write-Host "[*] SWARD UI Lab sidecar runtime: $sidecarExe"
Write-Host "[*] SWARD UI Lab evidence dir: $targetDir"
Write-Host "[*] SWARD UI Lab live bridge: \\.\pipe\$LiveBridgeName"
Write-Host "[*] SWARD UI Lab args: $($args -join ' ')"

if ($NoConsole) {
    return Start-Process -FilePath $sidecarExe -WorkingDirectory $installRootResolved -ArgumentList $args -PassThru
}

$quotedExe = Quote-UiLabArgument $sidecarExe
$quotedInstall = Quote-UiLabArgument $installRootResolved
$quotedArgs = ($args | ForEach-Object { Quote-UiLabArgument $_ }) -join ", "
$command = "Set-Location -LiteralPath $quotedInstall; Write-Host '[*] SWARD UI Lab manual runtime console'; & $quotedExe @($quotedArgs)"

return Start-Process -FilePath "powershell" -WorkingDirectory $installRootResolved -ArgumentList @("-NoExit", "-ExecutionPolicy", "Bypass", "-Command", $command) -PassThru
