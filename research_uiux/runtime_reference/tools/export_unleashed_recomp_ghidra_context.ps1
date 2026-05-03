param(
    [string]$ExePath = "local_build_env\ur103clean\b\ui_lab_runtime\UnleashedRecomp\UnleashedRecomp.exe",
    [string]$OutputPath = "",
    [string]$Label = "ui_lab_runtime",
    [string[]]$Targets = @(
        "sub_824D6C18",
        "sub_8231C590",
        "sub_8231C5F0",
        "sub_8231C628",
        "sub_82519FE8",
        "sub_82FE41C0",
        "sub_82BDBA20",
        "sub_82BDBA60",
        "sub_830BF640",
        "sub_830BF090",
        "sub_830BF300",
        "sub_830BF080"
    ),
    [string]$ManifestPath = "",
    [string]$ProjectRoot = "out\ui_lab_static_re\ghidra\projects",
    [string]$ProjectName = "sward_unleashed_recomp_context",
    [switch]$NoAnalysis
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..\..")

function Resolve-UiLabPath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return (Resolve-Path -LiteralPath $Path -ErrorAction Stop).Path
    }

    return (Resolve-Path -LiteralPath (Join-Path $repoRoot.Path $Path) -ErrorAction Stop).Path
}

function Resolve-UiLabOutputPath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        $parent = Split-Path -Parent $Path
        if (-not [string]::IsNullOrWhiteSpace($parent)) {
            New-Item -ItemType Directory -Force -Path $parent | Out-Null
        }
        return $Path
    }

    $absolute = Join-Path $repoRoot.Path $Path
    $parent = Split-Path -Parent $absolute
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
    return $absolute
}

function Resolve-SwardReToolManifest([string]$RequestedPath) {
    if (-not [string]::IsNullOrWhiteSpace($RequestedPath)) {
        return (Resolve-Path -LiteralPath $RequestedPath -ErrorAction Stop).Path
    }

    $defaultManifest = Join-Path $env:LOCALAPPDATA "SWARD\re_tools\manifest.json"
    if (Test-Path -LiteralPath $defaultManifest) {
        return (Resolve-Path -LiteralPath $defaultManifest).Path
    }

    return ""
}

function Resolve-AnalyzeHeadless([string]$ManifestFile) {
    if (-not [string]::IsNullOrWhiteSpace($ManifestFile)) {
        $manifest = Get-Content -Raw -LiteralPath $ManifestFile | ConvertFrom-Json
        if ($manifest.PSObject.Properties.Name -contains "analyzeHeadless" -and
            -not [string]::IsNullOrWhiteSpace([string]$manifest.analyzeHeadless)) {
            return (Resolve-Path -LiteralPath ([string]$manifest.analyzeHeadless) -ErrorAction Stop).Path
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($env:SWARD_GHIDRA_HOME)) {
        $fromEnv = Join-Path $env:SWARD_GHIDRA_HOME "support\analyzeHeadless.bat"
        if (Test-Path -LiteralPath $fromEnv) {
            return (Resolve-Path -LiteralPath $fromEnv).Path
        }
    }

    throw "Ghidra analyzeHeadless was not found. Install Ghidra and set SWARD_GHIDRA_HOME or $env:LOCALAPPDATA\SWARD\re_tools\manifest.json."
}

function Initialize-GhidraJavaFromManifest([string]$ManifestFile) {
    if ([string]::IsNullOrWhiteSpace($ManifestFile)) {
        return
    }

    $manifest = Get-Content -Raw -LiteralPath $ManifestFile | ConvertFrom-Json
    if (-not ($manifest.PSObject.Properties.Name -contains "javaHome")) {
        return
    }

    $javaHome = [string]$manifest.javaHome
    if ([string]::IsNullOrWhiteSpace($javaHome) -or -not (Test-Path -LiteralPath $javaHome)) {
        return
    }

    $env:JAVA_HOME = $javaHome
    $javaBin = Join-Path $javaHome "bin"
    if (Test-Path -LiteralPath $javaBin) {
        $pathParts = @($env:PATH -split ";")
        if ($pathParts -notcontains $javaBin) {
            $env:PATH = "$javaBin;$env:PATH"
        }
    }
}

function New-StagedExecutable([string]$ResolvedExePath, [string]$TargetLabel) {
    $safeLabel = [regex]::Replace($TargetLabel, "[^A-Za-z0-9_.-]", "_")
    if ([string]::IsNullOrWhiteSpace($safeLabel)) {
        $safeLabel = "unleashed_recomp"
    }

    $targetDir = Join-Path $repoRoot.Path "out\ghidra_targets"
    New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
    $stagedExe = Join-Path $targetDir ("{0}.exe" -f $safeLabel)

    if (Test-Path -LiteralPath $stagedExe) {
        Remove-Item -LiteralPath $stagedExe -Force
    }

    try {
        New-Item -ItemType HardLink -Path $stagedExe -Target $ResolvedExePath | Out-Null
    }
    catch {
        Copy-Item -LiteralPath $ResolvedExePath -Destination $stagedExe -Force
    }

    return $stagedExe
}

$resolvedExePath = Resolve-UiLabPath $ExePath
$manifestFile = Resolve-SwardReToolManifest $ManifestPath
$analyzeHeadless = Resolve-AnalyzeHeadless $manifestFile
Initialize-GhidraJavaFromManifest $manifestFile
$scriptDir = Resolve-UiLabPath "research_uiux\runtime_reference\ghidra_scripts"
$resolvedProjectRoot = Resolve-UiLabOutputPath $ProjectRoot
New-Item -ItemType Directory -Force -Path $resolvedProjectRoot | Out-Null

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path "out\ui_lab_static_re\ghidra" (Join-Path $Label "function_context.json")
}
$resolvedOutputPath = Resolve-UiLabOutputPath $OutputPath
$stagedExe = New-StagedExecutable $resolvedExePath $Label

$headlessArgs = @(
    $resolvedProjectRoot,
    $ProjectName,
    "-import",
    $stagedExe,
    "-overwrite"
)

if ($NoAnalysis) {
    $headlessArgs += "-noanalysis"
}

$headlessArgs += @(
    "-scriptPath",
    $scriptDir,
    "-postScript",
    "SwardExportFunctionContext.java",
    $resolvedOutputPath
)
$headlessArgs += $Targets

& $analyzeHeadless @headlessArgs
if ($LASTEXITCODE -ne 0) {
    throw "Ghidra analyzeHeadless failed with exit code $LASTEXITCODE."
}

if (-not (Test-Path -LiteralPath $resolvedOutputPath)) {
    throw "Ghidra export completed but JSON was not written: $resolvedOutputPath"
}

Write-Host "SWARD Ghidra context JSON: $resolvedOutputPath"
