param(
    [Parameter(Mandatory = $true)]
    [string]$SelectXmlPath,

    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"

function Get-XmlText {
    param(
        [Parameter(Mandatory = $true)]$Node,
        [Parameter(Mandatory = $true)][string]$XPath
    )

    $child = $Node.SelectSingleNode($XPath)
    if ($null -eq $child) {
        return ""
    }

    $text = $child.InnerText
    if ($null -eq $text) {
        return ""
    }

    return $text.Trim()
}

function Convert-ToRouteSample {
    param([Parameter(Mandatory = $true)]$Route)

    return [ordered]@{
        categoryPath = $Route.categoryPath
        type = $Route.type
        name = $Route.name
        archive = $Route.archive
        appendArchive = $Route.appendArchive
        isEvil = $Route.isEvil
    }
}

function New-RecoveryTargetMapping {
    param(
        [Parameter(Mandatory = $true)][string]$TargetId,
        [Parameter(Mandatory = $true)][string]$Controller,
        [object[]]$Routes = @()
    )

    $routeSamples = @(
        $Routes |
            Sort-Object @{ Expression = { $_.categoryPath } }, @{ Expression = { $_.type } }, @{ Expression = { $_.name } } |
            Select-Object -First 8 |
            ForEach-Object { Convert-ToRouteSample -Route $_ }
    )

    $status = if ($Routes.Count -gt 0) {
        "prototype-only secondary oracle route-taxonomy-proven"
    } else {
        "not-present-in-select-xml"
    }

    return [ordered]@{
        targetId = $TargetId
        controller = $Controller
        status = $status
        routeCount = $Routes.Count
        sampleRoutes = $routeSamples
    }
}

function Test-CategoryPathContains {
    param(
        [Parameter(Mandatory = $true)]$Route,
        [Parameter(Mandatory = $true)][string]$Needle
    )

    return ($Route.categoryPath -split "/" | Where-Object { $_ -ieq $Needle }).Count -gt 0
}

if (!(Test-Path -LiteralPath $SelectXmlPath -PathType Leaf)) {
    throw "SelectXmlPath does not exist or is not a file: $SelectXmlPath"
}

$resolvedSelectXmlPath = (Resolve-Path -LiteralPath $SelectXmlPath).Path
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $repoRoot = (Resolve-Path ".").Path
    $OutputPath = Join-Path $repoRoot "out\sonic_unleashed_preview_select_stage_taxonomy.json"
}

[xml]$document = Get-Content -Raw -LiteralPath $resolvedSelectXmlPath
$root = $document.DocumentElement
if ($null -eq $root) {
    throw "Select XML has no document element: $SelectXmlPath"
}

$script:routes = New-Object System.Collections.Generic.List[object]

function Add-StageRoute {
    param(
        [Parameter(Mandatory = $true)]$StageNode,
        [string[]]$CategoryPathParts = @()
    )

    $isEvilText = Get-XmlText -Node $StageNode -XPath "./IsEvil"
    $categoryPath = ($CategoryPathParts -join "/")
    $script:routes.Add([pscustomobject]([ordered]@{
        categoryPath = $categoryPath
        type = Get-XmlText -Node $StageNode -XPath "./Type"
        name = Get-XmlText -Node $StageNode -XPath "./Name"
        archive = Get-XmlText -Node $StageNode -XPath "./Archive"
        appendArchive = Get-XmlText -Node $StageNode -XPath "./AppendArchive"
        isEvil = ($isEvilText -match '^(true|1|yes)$')
    }))
}

function Walk-SelectContainer {
    param(
        [Parameter(Mandatory = $true)]$ContainerNode,
        [string[]]$CategoryPathParts = @()
    )

    foreach ($stageNode in @($ContainerNode.SelectNodes("./Stage"))) {
        Add-StageRoute -StageNode $stageNode -CategoryPathParts $CategoryPathParts
    }

    foreach ($categoryNode in @($ContainerNode.SelectNodes("./Category"))) {
        $categoryName = Get-XmlText -Node $categoryNode -XPath "./Name"
        if ([string]::IsNullOrWhiteSpace($categoryName)) {
            $categoryName = "<unnamed-category>"
        }

        Walk-SelectContainer -ContainerNode $categoryNode -CategoryPathParts ($CategoryPathParts + @($categoryName))
    }
}

Walk-SelectContainer -ContainerNode $root -CategoryPathParts @()

$rootCategories = @(
    $root.SelectNodes("./Category") |
        ForEach-Object { Get-XmlText -Node $_ -XPath "./Name" } |
        Where-Object { ![string]::IsNullOrWhiteSpace($_) } |
        Sort-Object -Unique
)

$typeCounts = [ordered]@{}
$routes |
    Group-Object { if ([string]::IsNullOrWhiteSpace($_.type)) { "<missing>" } else { $_.type } } |
    Sort-Object Name |
    ForEach-Object { $typeCounts[$_.Name] = $_.Count }

$titleRoutes = @($routes | Where-Object { $_.type -ieq "Title" -or $_.name -ieq "Title" })
$oldMainMenuRoutes = @($routes | Where-Object { $_.type -ieq "OldMainMenu" -or $_.name -ieq "OldMainMenu" })
$worldMapRoutes = @($routes | Where-Object { $_.type -ieq "WorldMap" -or $_.name -ieq "WorldMap" })
$sequenceRoutes = @($routes | Where-Object { $_.type -ieq "SequenceEntryPoint" -or $_.name -ieq "SequenceEntryPoint" })
$soundRoutes = @($routes | Where-Object {
    $_.type -match "Sound" -or
    $_.name -match "Sound|SE Test" -or
    (Test-CategoryPathContains -Route $_ -Needle "Sound Test")
})
$sonicDayRoutes = @($routes | Where-Object {
    $_.type -ieq "LoadXML" -and
    -not $_.isEvil -and
    (
        (Test-CategoryPathContains -Route $_ -Needle "Rom") -or
        (Test-CategoryPathContains -Route $_ -Needle "STAGE_Sonic") -or
        $_.appendArchive -match "SonicActionCommon" -or
        $_.archive -match "ActD_|Sonic|Mykonos|Africa|EU|Snow|China|Petra|NY|Beach|Eggman"
    )
})
$werehogRoutes = @($routes | Where-Object {
    $_.type -ieq "LoadXML" -and
    (
        $_.isEvil -or
        (Test-CategoryPathContains -Route $_ -Needle "STAGE_Evil") -or
        $_.appendArchive -match "EvilActionCommon" -or
        $_.archive -match "Evil|ActN_"
    )
})
$resultRoutes = @($routes | Where-Object {
    $_.name -match "Result|StaffRoll|Ending" -or
    $_.type -match "Result|Ending"
})

$recoveryTargetMappings = @(
    New-RecoveryTargetMapping -TargetId "title" -Controller "TitleMenuController" -Routes $titleRoutes
    New-RecoveryTargetMapping -TargetId "old-main-menu" -Controller "TitleMenuController" -Routes $oldMainMenuRoutes
    New-RecoveryTargetMapping -TargetId "world-map" -Controller "WorldMapController" -Routes $worldMapRoutes
    New-RecoveryTargetMapping -TargetId "sonic-day-stage-hud" -Controller "SonicDayHudController" -Routes $sonicDayRoutes
    New-RecoveryTargetMapping -TargetId "werehog-stage-hud" -Controller "WerehogHudController" -Routes $werehogRoutes
    New-RecoveryTargetMapping -TargetId "result" -Controller "ResultScreenController" -Routes $resultRoutes
    New-RecoveryTargetMapping -TargetId "sound-test" -Controller "AudioCueCatalog" -Routes $soundRoutes
    New-RecoveryTargetMapping -TargetId "sequence-entry-point" -Controller "SequenceRouteController" -Routes $sequenceRoutes
    New-RecoveryTargetMapping -TargetId "loading" -Controller "LoadingScreenController" -Routes @()
    New-RecoveryTargetMapping -TargetId "pause" -Controller "PauseMenuController" -Routes @()
)

function Get-RecoveryTargetMappingById {
    param([Parameter(Mandatory = $true)][string]$TargetId)

    foreach ($mapping in $recoveryTargetMappings) {
        if ($mapping.targetId -eq $TargetId) {
            return $mapping
        }
    }

    return $null
}

function New-StarterScreenCoverageRow {
    param(
        [Parameter(Mandatory = $true)][string]$ScreenId,
        [Parameter(Mandatory = $true)][string]$Controller,
        [Parameter(Mandatory = $true)][string[]]$MappingIds,
        [Parameter(Mandatory = $true)][string]$FallbackStatus,
        [Parameter(Mandatory = $true)][string]$SourceRecoveryFocus,
        [Parameter(Mandatory = $true)][string]$NextEvidenceBeat
    )

    $routeCount = 0
    $sampleRoutes = @()
    foreach ($mappingId in $MappingIds) {
        $mapping = Get-RecoveryTargetMappingById -TargetId $mappingId
        if ($null -eq $mapping) {
            continue
        }

        $routeCount += [int]$mapping.routeCount
        $sampleRoutes += @($mapping.sampleRoutes)
    }

    $status = if ($routeCount -gt 0) {
        "prototype-route-taxonomy-proven"
    } else {
        $FallbackStatus
    }

    return [ordered]@{
        screenId = $ScreenId
        controller = $Controller
        routeEvidenceStatus = $status
        routeCount = $routeCount
        sampleRoutes = @($sampleRoutes | Select-Object -First 8)
        sourceRecoveryFocus = $SourceRecoveryFocus
        nextEvidenceBeat = $NextEvidenceBeat
        oracleBoundary = "prototype route taxonomy is secondary; retail runtime evidence remains primary"
    }
}

$starterScreenCoverageMatrix = @(
    New-StarterScreenCoverageRow `
        -ScreenId "title-menu" `
        -Controller "TitleMenuController" `
        -MappingIds @("title", "old-main-menu") `
        -FallbackStatus "prototype-route-pending" `
        -SourceRecoveryFocus "title loop, title menu, E3 old-main-menu state-machine comparison" `
        -NextEvidenceBeat "compare prototype Title/OldMainMenu routes against retail title/menu controller transitions"
    New-StarterScreenCoverageRow `
        -ScreenId "loading" `
        -Controller "LoadingScreenController" `
        -MappingIds @("loading") `
        -FallbackStatus "package/runtime-lane-not-select-route" `
        -SourceRecoveryFocus "ui_loading package, loading display type, text/glyph/fade/SFX timing" `
        -NextEvidenceBeat "use preview ui_loading package as secondary layout reference; prove behavior in retail runtime"
    New-StarterScreenCoverageRow `
        -ScreenId "options-settings" `
        -Controller "OptionsMenuController" `
        -MappingIds @("old-main-menu") `
        -FallbackStatus "prototype-route-pending" `
        -SourceRecoveryFocus "title/options submenu ownership, cursor policy, input lock, SFX hooks" `
        -NextEvidenceBeat "map old main menu/options affordances to retail title-options evidence"
    New-StarterScreenCoverageRow `
        -ScreenId "pause" `
        -Controller "PauseMenuController" `
        -MappingIds @("pause") `
        -FallbackStatus "retail-runtime-lane-not-select-route" `
        -SourceRecoveryFocus "ui_pause package, CHudPause owner/action policy, render gate split" `
        -NextEvidenceBeat "continue retail pause owner/runtime capture; prototype route taxonomy does not expose pause directly"
    New-StarterScreenCoverageRow `
        -ScreenId "sonic-day-hud" `
        -Controller "SonicDayHudController" `
        -MappingIds @("sonic-day-stage-hud") `
        -FallbackStatus "prototype-route-pending" `
        -SourceRecoveryFocus "ui_playscreen, CHudSonicStage, rings/score/timer/speed/boost/ring-energy/tutorial" `
        -NextEvidenceBeat "join retail owner-field/gauge evidence to exact gameplay-fed values"
    New-StarterScreenCoverageRow `
        -ScreenId "werehog-hud" `
        -Controller "WerehogHudController" `
        -MappingIds @("werehog-stage-hud") `
        -FallbackStatus "prototype-route-pending" `
        -SourceRecoveryFocus "ui_playscreen_evil and night/Evil Sonic HUD ownership" `
        -NextEvidenceBeat "defer until Sonic Day HUD value model stabilizes, then replicate controller pattern"
    New-StarterScreenCoverageRow `
        -ScreenId "world-map" `
        -Controller "WorldMapController" `
        -MappingIds @("world-map") `
        -FallbackStatus "prototype-route-pending" `
        -SourceRecoveryFocus "world-map icons, route selection, tutorial entry, disc indicators" `
        -NextEvidenceBeat "harvest prototype world-map route/layout facts and compare to retail runtime coverage"
    New-StarterScreenCoverageRow `
        -ScreenId "results" `
        -Controller "ResultScreenController" `
        -MappingIds @("result") `
        -FallbackStatus "prototype-route-pending" `
        -SourceRecoveryFocus "ui_result, item result/status flow, end/credits overlap boundaries" `
        -NextEvidenceBeat "separate result screen controller from ending/staff-roll prototype route evidence"
    New-StarterScreenCoverageRow `
        -ScreenId "audio-sfx" `
        -Controller "AudioCueCatalog" `
        -MappingIds @("sound-test") `
        -FallbackStatus "prototype-sound-route-pending" `
        -SourceRecoveryFocus "SFX/audio bank IDs for menus, loading, HUD, pause, result" `
        -NextEvidenceBeat "use Sound Test route and sound banks as secondary cue-index hints; prove exact IDs in retail runtime"
)

$targetMappingLine = @(
    $recoveryTargetMappings |
        ForEach-Object { "$($_.targetId):$($_.routeCount)" }
) -join ","

$coverageMappingLine = @(
    $starterScreenCoverageMatrix |
        ForEach-Object { "$($_.screenId):$($_.routeEvidenceStatus):$($_.routeCount)" }
) -join ","

$summary = [ordered]@{
    buildKind = "sonic-unleashed-preview-select-stage"
    selectXmlPath = $resolvedSelectXmlPath
    generatedAtUtc = [DateTime]::UtcNow.ToString("o")
    categoryCount = $root.SelectNodes("//Category").Count
    stageCount = $routes.Count
    rootCategories = $rootCategories
    typeCounts = $typeCounts
    recoveryTargetMappings = $recoveryTargetMappings
    starterScreenCoverageMatrix = $starterScreenCoverageMatrix
    f2PanelStyleReference = [ordered]@{
        localAssetFamily = "Reddog debug/profiler assets from the local preview/debug-menu references"
        intendedUse = "style guide for SWARD F2 profiler panel spacing, title bars, compact status strips, and window-list behavior"
        publishBoundary = "not committed; use local-only assets as visual reference, then implement repo-safe ImGui style logic"
    }
    routeTaxonomyUse = "secondary oracle for route/menu taxonomy and source-recovery prioritization; retail runtime evidence remains primary"
    reddogStyleUse = "local-only Reddog debug/profiler assets are style references for the SWARD F2 panel; do not publish extracted DDS payloads"
    publishBoundary = "metadata-only; do not commit prototype Select.xml, extracted archives, XEX, DDS, CSB, CPK, or generated extraction output"
}

$outputDirectory = Split-Path -Parent $OutputPath
if (![string]::IsNullOrWhiteSpace($outputDirectory)) {
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
}

$json = $summary | ConvertTo-Json -Depth 10
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($OutputPath, $json + [Environment]::NewLine, $utf8NoBom)

Write-Output "preview_select_stage_status=ok"
Write-Output "preview_select_stage_path=$OutputPath"
Write-Output "preview_select_stage_categories=$($summary.categoryCount)"
Write-Output "preview_select_stage_stages=$($summary.stageCount)"
Write-Output "preview_select_stage_type_counts=$(($typeCounts.GetEnumerator() | ForEach-Object { "$($_.Key)=$($_.Value)" }) -join ',')"
Write-Output "preview_select_stage_target_mappings=$targetMappingLine"
Write-Output "preview_select_stage_coverage_status=starter-uiux-route-coverage-matrix-ready"
Write-Output "preview_select_stage_coverage_matrix=$coverageMappingLine"
