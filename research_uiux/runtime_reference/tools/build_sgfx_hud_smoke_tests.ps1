#requires -Version 5.1

<#
.SYNOPSIS
    Phase 278: build (and optionally run) the SGFX HUD C++ smoke tests.

.DESCRIPTION
    Compiles `sgfx_hud_csd_project_loader_smoke_test.cpp` against the
    header-only loader using the same Visual Studio Build Tools developer
    command line + LLVM clang-cl driver the main UnleashedRecomp UI Lab
    build wraps. By default also runs the resulting binary against an
    extracted retail asset and reports the loader's findings.

    The script exits non-zero on either compile failure or smoke-run
    failure (loader did not recognize the asset's CPAF / YNCP / XNCP
    magic), so it's safe to call from CI / contract tests.

.PARAMETER RepoRoot
    Path to the repository root. Defaults to the parent of this script's
    grandparent directory (i.e. the project working directory).

.PARAMETER AssetPath
    Path (absolute or repo-relative) to a `.yncp` / `.xncp` asset to
    drive the smoke test against. Defaults to the user's extracted Sonic
    `ui_playscreen.yncp`.

.PARAMETER OutputDir
    Where the compiled .exe and intermediate .obj should land. Defaults
    to `out/sgfx_hud_smoke_tests/` under the repo root.

.PARAMETER SkipRun
    Compile only — do not invoke the produced binary.
#>
[CmdletBinding()]
param(
    [string] $RepoRoot,
    [string] $AssetPath = "extracted_assets/full_install_archives/game/Sonic/ui_playscreen.yncp",
    [string] $OutputDir,
    [switch] $SkipRun
)

$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..\..")).Path
}
if (-not $OutputDir) {
    $OutputDir = Join-Path $RepoRoot "out\sgfx_hud_smoke_tests"
}

if (-not (Test-Path -LiteralPath $RepoRoot -PathType Container)) {
    throw "Repo root not found: $RepoRoot"
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$includeDir = Join-Path $RepoRoot "research_uiux\runtime_reference\include"
$srcDir     = Join-Path $RepoRoot "research_uiux\runtime_reference\src"

# Each entry: source file -> exe name. The CHudSonicStage smoke test
# links the methods .cpp because the smoke driver and the methods live
# in separate translation units. Phase 285 method-body translation
# units for CGeneralWindow / CLoading / CSaveIcon are also included so
# their static_asserts and `#include` chains get built every smoke run.
$targets = @(
    @{
        Sources = @((Join-Path $srcDir "sgfx_hud_csd_project_loader_smoke_test.cpp"))
        Exe     = (Join-Path $OutputDir "sgfx_hud_csd_project_loader_smoke_test.exe")
        Args    = @($null) # set per-target below
    },
    @{
        Sources = @(
            (Join-Path $srcDir "sgfx_hud_chud_sonic_stage_methods_smoke_test.cpp"),
            (Join-Path $srcDir "sgfx_hud_chud_sonic_stage_methods.cpp"),
            (Join-Path $srcDir "sgfx_hud_chud_pause_methods.cpp"),
            (Join-Path $srcDir "sgfx_hud_cgeneral_window_methods.cpp"),
            (Join-Path $srcDir "sgfx_hud_cloading_methods.cpp"),
            (Join-Path $srcDir "sgfx_hud_csave_icon_methods.cpp")
        )
        Exe     = (Join-Path $OutputDir "sgfx_hud_chud_sonic_stage_methods_smoke_test.exe")
        Args    = @($null)
    }
)

foreach ($p in @($includeDir)) {
    if (-not (Test-Path -LiteralPath $p)) {
        throw "Required path missing: $p"
    }
}
foreach ($t in $targets) {
    foreach ($src in $t.Sources) {
        if (-not (Test-Path -LiteralPath $src)) {
            throw "Required source missing: $src"
        }
    }
}

$resolvedAssetPath = $AssetPath
if (-not [System.IO.Path]::IsPathRooted($resolvedAssetPath)) {
    $resolvedAssetPath = Join-Path $RepoRoot $resolvedAssetPath
}
if (-not $SkipRun -and -not (Test-Path -LiteralPath $resolvedAssetPath)) {
    throw "Asset path for smoke run not found: $resolvedAssetPath"
}

# Locate the Visual Studio Build Tools developer command line and the
# LLVM clang-cl driver the same way `build_unleashed_recomp_ui_lab.ps1`
# does. Both are required because clang-cl needs the MSVC headers /
# libraries that VsDevCmd puts on PATH.
$vsDevCmd = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
$llvmBin  = "C:\Program Files\LLVM\bin"
foreach ($p in @($vsDevCmd, (Join-Path $llvmBin "clang-cl.exe"))) {
    if (-not (Test-Path -LiteralPath $p)) {
        throw "Required toolchain piece missing: $p"
    }
}

# Build each smoke target in its own cmd.exe so VsDevCmd's environment
# edits survive into the clang-cl invocation. Use backslash paths inside
# the cmd /c string so they survive PowerShell→cmd quoting.
$includeDirEscaped = $includeDir.Replace("/", "\")

# VsDevCmd internally calls `vswhere.exe`; on this build host vswhere
# lives under the VS Installer dir but is not on PATH globally. Prepend
# its directory so VsDevCmd can find it.
$vswhereDir = "C:\Program Files (x86)\Microsoft Visual Studio\Installer"
if (-not (Test-Path -LiteralPath (Join-Path $vswhereDir "vswhere.exe"))) {
    throw "vswhere.exe not found under: $vswhereDir"
}

$failedTargets = 0
foreach ($t in $targets) {
    $exePath = $t.Exe
    $exePathEscaped = $exePath.Replace("/", "\")
    $sourcesEscaped = ($t.Sources | ForEach-Object { '"' + $_.Replace("/", "\") + '"' }) -join " "
    $objArg = "/Fo`"$($OutputDir.Replace("/", "\"))\\`""

    $compileLogPath = Join-Path $OutputDir "compile.log"
    $compileLogEscaped = $compileLogPath.Replace("/", "\")
    $compileCmd = @(
        "set `"PATH=$vswhereDir;%PATH%`"",
        "`"$vsDevCmd`" -arch=x64 >nul",
        "set `"PATH=$llvmBin;%PATH%`"",
        ("clang-cl /nologo /std:c++17 /EHsc " +
         "/I`"$includeDirEscaped`" $sourcesEscaped " +
         "$objArg /Fe`"$exePathEscaped`" > `"$compileLogEscaped`" 2>&1")
    ) -join " && "

    Write-Host "[sgfx-hud-smoke] compiling -> $exePathEscaped" -ForegroundColor Cyan
    cmd /c $compileCmd | Out-Null
    $compileExit = $LASTEXITCODE
    if (Test-Path -LiteralPath $compileLogPath) {
        Get-Content -LiteralPath $compileLogPath | ForEach-Object { Write-Host "  $_" }
    }
    if ($compileExit -ne 0) {
        Write-Host "[sgfx-hud-smoke] compile failed for $exePathEscaped (exit $compileExit)" -ForegroundColor Red
        $failedTargets++
        continue
    }
    if (-not (Test-Path -LiteralPath $exePath)) {
        Write-Host "[sgfx-hud-smoke] compile reported success but output missing: $exePath" -ForegroundColor Red
        $failedTargets++
        continue
    }

    if ($SkipRun) {
        Write-Host "[sgfx-hud-smoke] -SkipRun set; skipping run for $exePathEscaped"
        continue
    }

    Write-Host "[sgfx-hud-smoke] running $exePathEscaped against $resolvedAssetPath" -ForegroundColor Cyan
    & $exePath $resolvedAssetPath
    $runExit = $LASTEXITCODE
    if ($runExit -ne 0) {
        Write-Host "[sgfx-hud-smoke] run failed for $exePathEscaped (exit $runExit)" -ForegroundColor Red
        $failedTargets++
        continue
    }
    Write-Host "[sgfx-hud-smoke] OK $exePathEscaped" -ForegroundColor Green
}

if ($failedTargets -gt 0) {
    throw "$failedTargets smoke target(s) failed"
}
