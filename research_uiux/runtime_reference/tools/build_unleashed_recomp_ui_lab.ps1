param(
    [string]$GeneratedRoot = "local_build_env\ur103clean",
    [string]$InstallRoot = "Unleashed Recomp - Windows (Complete Installation) 1.0.3",
    [string]$DriveLetter = "W",
    [string]$BuildDir = "b\ui_lab_runtime",
    [string]$Configuration = "RelWithDebInfo",
    [string]$Target = "UnleashedRecomp",
    [switch]$KeepSubstDrive
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot "..\..\..")

function Resolve-RequiredPath([string]$Path, [string]$Label) {
    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction SilentlyContinue
    if (-not $resolved) {
        throw "$Label not found: $Path"
    }

    return $resolved.Path
}

function Patch-SdlPrefetchShim([string]$ShortRoot) {
    $headers = @(
        (Join-Path $ShortRoot "thirdparty\SDL\include\SDL_endian.h"),
        (Join-Path $ShortRoot "thirdparty\SDL\include\SDL_cpuinfo.h")
    )

    foreach ($header in $headers) {
        if (-not (Test-Path -LiteralPath $header)) {
            continue
        }

        $text = Get-Content -LiteralPath $header -Raw
        $patched = $text `
            -replace "#ifdef __clang__", "#if defined(__clang__) && (__clang_major__ < 22)" `
            -replace "#endif /\* __clang__ \*/", "#endif /* defined(__clang__) && (__clang_major__ < 22) */"

        if ($patched -ne $text) {
            Set-Content -LiteralPath $header -Value $patched -NoNewline
        }
    }
}

function Sync-TrackedRuntimeFile([string]$RelativePath, [string]$GeneratedRoot) {
    $source = Join-Path $repoRoot.Path $RelativePath
    $destination = Join-Path $GeneratedRoot $RelativePath
    $destinationDir = Split-Path -Parent $destination

    if (-not (Test-Path -LiteralPath $source)) {
        throw "Tracked runtime file not found: $source"
    }

    New-Item -ItemType Directory -Force -Path $destinationDir | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
}

function Invoke-PythonTool([string]$Script, [string[]]$Arguments, [string]$Label) {
    if (-not (Test-Path -LiteralPath $Script)) {
        throw "$Label script not found: $Script"
    }

    & python $Script @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Label failed with exit code $LASTEXITCODE"
    }
}

function Invoke-DevCmd([string]$Command) {
    $vsDevCmd = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
    $llvmBin = "C:\Program Files\LLVM\bin"

    if (-not (Test-Path -LiteralPath $vsDevCmd)) {
        throw "Visual Studio Build Tools developer command not found: $vsDevCmd"
    }

    if (-not (Test-Path -LiteralPath (Join-Path $llvmBin "clang-cl.exe"))) {
        throw "LLVM clang-cl not found under: $llvmBin"
    }

    cmd /c "`"$vsDevCmd`" -arch=x64 && set `"PATH=$llvmBin;%PATH%`" && $Command"
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $Command"
    }
}

function Remove-TransientSubstDrive([string]$Drive, [bool]$createdSubstDrive, [bool]$KeepSubstDrive) {
    if ($createdSubstDrive -and -not $KeepSubstDrive) {
        cmd /c "subst $drive /D"
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to unmount transient build drive $Drive"
        }
    }
}

$root = Resolve-RequiredPath $GeneratedRoot "Generated UnleashedRecomp root"
$currentInstallRoot = Resolve-RequiredPath (Join-Path $repoRoot.Path $InstallRoot) "Complete Installation root"
$runtimeBridgeAssetIndex = Join-Path $repoRoot.Path "out\runtime_bridge_asset_index_current_install.json"
$runtimeBridgeAssetIndexDir = Split-Path -Parent $runtimeBridgeAssetIndex
New-Item -ItemType Directory -Force -Path $runtimeBridgeAssetIndexDir | Out-Null

$assetRoots = @(
    (Join-Path $currentInstallRoot "game"),
    (Join-Path $currentInstallRoot "update"),
    (Join-Path $currentInstallRoot "dlc"),
    (Join-Path $repoRoot.Path "extracted_assets")
)

$scanArgs = @("--repo-root", $repoRoot.Path)
foreach ($assetRoot in $assetRoots) {
    $scanArgs += @("--asset-root", $assetRoot)
}
$scanArgs += @("--output", $runtimeBridgeAssetIndex)

Invoke-PythonTool `
    (Join-Path $repoRoot.Path "research_uiux\tools\scan_assets.py") `
    $scanArgs `
    "Runtime bridge current install asset scan"

Invoke-PythonTool `
    (Join-Path $repoRoot.Path "research_uiux\tools\build_runtime_bridge_screen_index.py") `
    @("--repo-root", $repoRoot.Path, "--asset-index", $runtimeBridgeAssetIndex) `
    "Runtime bridge screen index generation"

Invoke-PythonTool `
    (Join-Path $repoRoot.Path "research_uiux\tools\build_yncp_native_component_map.py") `
    @(
        "--root", (Join-Path $repoRoot.Path "extracted_assets\full_install_archives"),
        "--output", (Join-Path $repoRoot.Path "research_uiux\data\yncp_native_component_map.json"),
        "--markdown", (Join-Path $repoRoot.Path "research_uiux\YNCP_NATIVE_COMPONENT_MAP.md"),
        "--output-header", (Join-Path $repoRoot.Path "UnleashedRecomp\patches\ui_lab_yncp_native_component_map.generated.h")
    ) `
    "YNCP native component map generation"

@(
    "UnleashedRecomp\CMakeLists.txt",
    "UnleashedRecomp\gpu\video.cpp",
    "UnleashedRecomp\gpu\rhi\plume_d3d12.cpp",
    "UnleashedRecomp\gpu\rhi\plume_vulkan.cpp",
    "UnleashedRecomp\patches\aspect_ratio_patches.cpp",
    "UnleashedRecomp\patches\CGameModeStage_patches.cpp",
    "UnleashedRecomp\patches\CGameModeStageTitle_patches.cpp",
    "UnleashedRecomp\patches\CHudPause_patches.cpp",
    "UnleashedRecomp\patches\CTitleStateIntro_patches.cpp",
    "UnleashedRecomp\patches\CTitleStateMenu_patches.cpp",
    "UnleashedRecomp\patches\CHudSonicStage_patches.cpp",
    "UnleashedRecomp\patches\csd_overlay_patches.cpp",
    "UnleashedRecomp\patches\CsdNodeLookup_patches.cpp",
    "UnleashedRecomp\patches\CsdNodeText_patches.cpp",
    "UnleashedRecomp\patches\CsdNodeValue_patches.cpp",
    "UnleashedRecomp\patches\resident_patches.cpp",
    "UnleashedRecomp\patches\ui_lab_patches.cpp",
    "UnleashedRecomp\patches\ui_lab_runtime_screen_index.generated.h",
    "UnleashedRecomp\patches\ui_lab_yncp_native_component_map.generated.h",
    "UnleashedRecomp\patches\ui_lab_patches.h",
    "research_uiux\runtime_reference\include\sward\ui_runtime\sgfx_hud_csd_project_loader.hpp",
    "research_uiux\runtime_reference\include\sward\ui_runtime\sgfx_hud_csd_cast_extractor.hpp",
    "research_uiux\runtime_reference\include\sward\ui_runtime\sgfx_hud_native_csd_renderer.hpp"
) | ForEach-Object { Sync-TrackedRuntimeFile $_ $root }

$drive = "$DriveLetter`:"
$shortRoot = "$drive\"
$createdSubstDrive = $false

$existingSubst = cmd /c "subst"
$existingMapping = $existingSubst | Where-Object { $_ -like "$drive\:*" } | Select-Object -First 1

if ($existingMapping) {
    $expectedMapping = "$drive\`: => $root"
    if ($existingMapping -ne $expectedMapping) {
        throw "$drive is already mounted elsewhere: $existingMapping"
    }
}
else {
    cmd /c "subst $drive `"$root`""
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to mount $root on $drive"
    }
    $createdSubstDrive = $true
}

try {
    Patch-SdlPrefetchShim $shortRoot

    $buildPath = Join-Path $shortRoot $BuildDir
    $hostBuildPath = Join-Path $root $BuildDir
    $toolchain = Join-Path $shortRoot "thirdparty\vcpkg\scripts\buildsystems\vcpkg.cmake"
    $dxilPath = Join-Path $buildPath "vcpkg_installed\x64-windows-static\bin\dxil.dll"

    $configureBase = "cmake -S $shortRoot -B $buildPath -G Ninja " +
        "-DCMAKE_BUILD_TYPE=$Configuration " +
        "-DCMAKE_C_COMPILER=clang-cl.exe " +
        "-DCMAKE_CXX_COMPILER=clang-cl.exe " +
        "-DCMAKE_LINKER=lld-link.exe " +
        "-DCMAKE_TOOLCHAIN_FILE=$toolchain " +
        "-DVCPKG_TARGET_TRIPLET=x64-windows-static"

    try {
        Invoke-DevCmd $configureBase
    }
    catch {
        if (-not (Test-Path -LiteralPath $dxilPath)) {
            throw
        }

        Invoke-DevCmd "$configureBase -DDIRECTX_DXIL_LIBRARY=$dxilPath"
    }

    if (Test-Path -LiteralPath $dxilPath) {
        Invoke-DevCmd "$configureBase -DDIRECTX_DXIL_LIBRARY=$dxilPath"
    }

    Invoke-DevCmd "cmake --build $buildPath --target $Target -j 4"

    $exe = Join-Path $buildPath "UnleashedRecomp\UnleashedRecomp.exe"
    $hostExe = Join-Path $hostBuildPath "UnleashedRecomp\UnleashedRecomp.exe"
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "Build finished but executable was not found: $exe"
    }

    Write-Host "Built UI Lab runtime: $hostExe"
}
finally {
    Remove-TransientSubstDrive $drive $createdSubstDrive ([bool]$KeepSubstDrive)
}
