param(
    [Parameter(Mandatory = $true)]
    [string]$PreviewRoot,

    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"

function Convert-ToRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $rootPath = [System.IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if ($fullPath.StartsWith($rootPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $fullPath.Substring($rootPath.Length).TrimStart('\', '/')
        return ($relative -replace '\\', '/')
    }

    return ($fullPath -replace '\\', '/')
}

function Get-ArchiveFamilyName {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalized = $RelativePath -replace '\\', '/'
    if ($normalized.EndsWith(".ar.00", [System.StringComparison]::OrdinalIgnoreCase)) {
        return $normalized.Substring(0, $normalized.Length - ".ar.00".Length)
    }

    if ($normalized.EndsWith(".ar", [System.StringComparison]::OrdinalIgnoreCase)) {
        return $normalized.Substring(0, $normalized.Length - ".ar".Length)
    }

    return $normalized
}

if (!(Test-Path -LiteralPath $PreviewRoot -PathType Container)) {
    throw "PreviewRoot does not exist or is not a directory: $PreviewRoot"
}

$resolvedRoot = (Resolve-Path -LiteralPath $PreviewRoot).Path
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $repoRoot = (Resolve-Path ".").Path
    $OutputPath = Join-Path $repoRoot "out\sonic_unleashed_preview_build_inventory.json"
}

$files = @(Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File)
$xexFiles = @($files | Where-Object { $_.Name -ieq "default.xex" })
$shaderFiles = @($files | Where-Object { $_.Name -ieq "shader.ar" -or $_.Name -ieq "shader.arl" })
$archiveFamilies = @(
    $files |
        Where-Object { $_.Name -match '\.ar\.00$' } |
        ForEach-Object { Get-ArchiveFamilyName -RelativePath (Convert-ToRelativePath -Root $resolvedRoot -Path $_.FullName) } |
        Sort-Object -Unique
)

$knownUiArchiveNames = @(
    "ActionCommon",
    "EvilActionCommon",
    "EvilActionCommonGeneral",
    "Loading",
    "MainMenu",
    "SelectStage",
    "SonicActionCommon",
    "SonicActionCommonGeneral",
    "StaffRoll",
    "SuperSonic",
    "SystemCommon",
    "Title",
    "TitleE3",
    "TitleModel",
    "WorldMap"
)

$uiArchiveFamilies = @(
    $archiveFamilies |
        Where-Object {
            $family = $_
            $leaf = (Split-Path -Leaf $family).TrimStart("#")
            $knownUiArchiveNames -contains $leaf
        } |
        Sort-Object -Unique
)

$localizedArchiveFamilies = @(
    $archiveFamilies |
        Where-Object { $_ -like "Languages/*" -or $_ -like "voices/*" } |
        Sort-Object -Unique
)

$reddogDebugUiAssets = @(
    $files |
        Where-Object { (Convert-ToRelativePath -Root $resolvedRoot -Path $_.FullName) -like "reddog/*" } |
        ForEach-Object { $_.Name } |
        Sort-Object -Unique
)

$soundBanks = @(
    $files |
        Where-Object { $_.Extension -ieq ".csb" -or $_.Extension -ieq ".cpk" } |
        Where-Object {
            $relative = Convert-ToRelativePath -Root $resolvedRoot -Path $_.FullName
            $relative -like "Sound/*" -or $relative -like "voices/*" -or $relative -like "*/Sound/*"
        } |
        ForEach-Object { $_.Name } |
        Sort-Object -Unique
)

$extensionCounts = [ordered]@{}
$files |
    Group-Object { if ([string]::IsNullOrWhiteSpace($_.Extension)) { "<none>" } else { $_.Extension.ToLowerInvariant() } } |
    Sort-Object Count -Descending |
    ForEach-Object { $extensionCounts[$_.Name] = $_.Count }

$xex = if ($xexFiles.Count -gt 0) {
    [ordered]@{
        present = $true
        relativePath = Convert-ToRelativePath -Root $resolvedRoot -Path $xexFiles[0].FullName
        sizeBytes = $xexFiles[0].Length
    }
} else {
    [ordered]@{
        present = $false
        relativePath = ""
        sizeBytes = 0
    }
}

$summary = [ordered]@{
    buildKind = "sonic-unleashed-preview-build-disc1"
    previewRoot = $resolvedRoot
    generatedAtUtc = [DateTime]::UtcNow.ToString("o")
    fileCount = $files.Count
    xex = $xex
    shaderArchivePresent = ($shaderFiles.Count -ge 2)
    archiveFamilyCount = $archiveFamilies.Count
    archiveFamilies = $archiveFamilies
    uiArchiveFamilies = $uiArchiveFamilies
    localizedArchiveFamilies = $localizedArchiveFamilies
    reddogDebugUiAssets = $reddogDebugUiAssets
    soundBanks = $soundBanks
    expectedUiPackageNames = @(
        "ui_title",
        "ui_mainmenu",
        "ui_loading",
        "ui_worldmap",
        "ui_general",
        "ui_help",
        "ui_itemresult",
        "ui_playscreen",
        "ui_playscreen_evil",
        "ui_playscreen_ev_hit",
        "ui_result",
        "ui_gate",
        "ui_end"
    )
    extensionCounts = $extensionCounts
    publishBoundary = "metadata-only; do not commit preview build, extracted assets, XEX, AR, DDS, CSB, CPK, or generated extraction output"
}

$outputDirectory = Split-Path -Parent $OutputPath
if (![string]::IsNullOrWhiteSpace($outputDirectory)) {
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
}

$json = $summary | ConvertTo-Json -Depth 8
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($OutputPath, $json + [Environment]::NewLine, $utf8NoBom)

Write-Output "preview_inventory_status=ok"
Write-Output "preview_inventory_path=$OutputPath"
Write-Output "preview_archive_families=$($archiveFamilies.Count)"
Write-Output "preview_ui_archive_families=$($uiArchiveFamilies -join ',')"
Write-Output "preview_reddog_assets=$($reddogDebugUiAssets.Count)"
