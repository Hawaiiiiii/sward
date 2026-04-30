param(
    [string]$EventsPath = "",
    [string]$EvidenceDir = "",
    [string]$DrawListPath = "",
    [int]$CandidateValueLimit = 32,
    [switch]$AsJson
)

$ErrorActionPreference = "Stop"

function Resolve-HudValueEventsPath([string]$EventsPath, [string]$EvidenceDir) {
    if (-not [string]::IsNullOrWhiteSpace($EventsPath)) {
        $resolved = Resolve-Path -LiteralPath $EventsPath -ErrorAction Stop
        return $resolved.Path
    }

    if ([string]::IsNullOrWhiteSpace($EvidenceDir)) {
        throw "Pass -EventsPath <ui_lab_events.jsonl> or -EvidenceDir <capture directory>."
    }

    $resolvedDir = Resolve-Path -LiteralPath $EvidenceDir -ErrorAction Stop
    $latest = Get-ChildItem -LiteralPath $resolvedDir.Path -Recurse -Filter "ui_lab_events.jsonl" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if ($null -eq $latest) {
        throw "No ui_lab_events.jsonl found under evidence directory: $($resolvedDir.Path)"
    }

    return $latest.FullName
}

function Add-UniqueSorted([System.Collections.Generic.SortedSet[string]]$Set, [string]$Value) {
    if (-not [string]::IsNullOrWhiteSpace($Value)) {
        [void]$Set.Add($Value)
    }
}

function Get-DetailToken([string]$Detail, [string]$Name) {
    $match = [regex]::Match($Detail, "(^| )$([regex]::Escape($Name))=([^ ]+)")
    if ($match.Success) {
        return $match.Groups[2].Value.Trim('"')
    }
    return ""
}

function Get-EventFrame($EventObject) {
    if ($EventObject.PSObject.Properties.Name -contains "frame") {
        try {
            return [int]$EventObject.frame
        }
        catch {
            return $null
        }
    }

    return $null
}

function New-UnresolvedNodeCandidate([string]$Node) {
    return [ordered]@{
        node = $Node
        writes = 0
        kinds = [System.Collections.Generic.SortedSet[string]]::new()
        values = [System.Collections.Generic.SortedSet[string]]::new()
        sources = [System.Collections.Generic.SortedSet[string]]::new()
        semanticPathCandidates = [System.Collections.Generic.SortedSet[string]]::new()
        semanticValueNames = [System.Collections.Generic.SortedSet[string]]::new()
        firstFrame = $null
        lastFrame = $null
        likely = "unknown-unresolved-node-candidate"
    }
}

function New-SemanticPathCandidateGroup([string]$Path, [string]$ValueName) {
    return [ordered]@{
        path = $Path
        semanticValueName = $ValueName
        writes = 0
        nodes = [System.Collections.Generic.SortedSet[string]]::new()
        kinds = [System.Collections.Generic.SortedSet[string]]::new()
        values = [System.Collections.Generic.SortedSet[string]]::new()
        sources = [System.Collections.Generic.SortedSet[string]]::new()
        firstFrame = $null
        lastFrame = $null
    }
}

function Add-SemanticPathCandidateGroup($Groups, [string]$Detail, $EventObject) {
    $path = Get-DetailToken $Detail "semanticPathCandidate"
    if ([string]::IsNullOrWhiteSpace($path)) {
        return
    }

    $valueName = Get-DetailToken $Detail "semanticValueName"
    if ([string]::IsNullOrWhiteSpace($valueName)) {
        $valueName = Get-DetailToken $Detail "valueCandidate"
    }
    if ([string]::IsNullOrWhiteSpace($valueName)) {
        $valueName = "unknown"
    }

    $key = "{0}:{1}" -f $path, $valueName
    if (-not $Groups.ContainsKey($key)) {
        $Groups[$key] = New-SemanticPathCandidateGroup $path $valueName
    }

    $group = $Groups[$key]
    $group.writes++
    Add-UniqueSorted $group.nodes (Get-DetailToken $Detail "node")
    Add-UniqueSorted $group.kinds (Get-DetailToken $Detail "kind")
    Add-UniqueSorted $group.values (Get-DetailToken $Detail "value")
    Add-UniqueSorted $group.sources (Get-DetailToken $Detail "source")

    $frame = Get-EventFrame $EventObject
    if ($null -ne $frame) {
        if ($null -eq $group.firstFrame -or $frame -lt $group.firstFrame) {
            $group.firstFrame = $frame
        }
        if ($null -eq $group.lastFrame -or $frame -gt $group.lastFrame) {
            $group.lastFrame = $frame
        }
    }
}

function Add-SemanticBoundGroup($Groups, [string]$Detail, $EventObject) {
    Add-SemanticPathCandidateGroup $Groups $Detail $EventObject
}

function New-GaugeDrawPathGroup([string]$Path, [string]$ValueName) {
    return [ordered]@{
        path = $Path
        semanticValueName = $ValueName
        draws = 0
        childPaths = [System.Collections.Generic.SortedSet[string]]::new()
        layerAddresses = [System.Collections.Generic.SortedSet[string]]::new()
        castNodeAddresses = [System.Collections.Generic.SortedSet[string]]::new()
    }
}

function Get-GaugeDrawPathInfo([string]$LayerPath) {
    $speedGaugeParent = "ui_playscreen/so_speed_gauge/position/speed_gauge_color"
    $ringEnergyParent = "ui_playscreen/so_ringenagy_gauge/position/ringenagy_gauge_color"

    if ($LayerPath.StartsWith($speedGaugeParent)) {
        return [ordered]@{ parent = $speedGaugeParent; semanticValueName = "boostGauge" }
    }

    if ($LayerPath.StartsWith($ringEnergyParent)) {
        return [ordered]@{ parent = $ringEnergyParent; semanticValueName = "ringEnergyGauge" }
    }

    return $null
}

function Add-GaugeDrawPathGroup($Groups, $DrawCall) {
    $layerPath = [string]$DrawCall.layerPath
    if ([string]::IsNullOrWhiteSpace($layerPath)) {
        return
    }

    $info = Get-GaugeDrawPathInfo $layerPath
    if ($null -eq $info) {
        return
    }

    $key = "{0}:{1}" -f $info.parent, $info.semanticValueName
    if (-not $Groups.ContainsKey($key)) {
        $Groups[$key] = New-GaugeDrawPathGroup $info.parent $info.semanticValueName
    }

    $group = $Groups[$key]
    $group.draws++
    Add-UniqueSorted $group.childPaths $layerPath
    Add-UniqueSorted $group.layerAddresses ([string]$DrawCall.layerAddress)
    Add-UniqueSorted $group.castNodeAddresses ([string]$DrawCall.castNodeAddress)
}

function Get-DrawListCalls($DrawListObject) {
    if ($null -ne $DrawListObject.uiDrawListOracle -and $null -ne $DrawListObject.uiDrawListOracle.drawCalls) {
        return @($DrawListObject.uiDrawListOracle.drawCalls)
    }

    if ($null -ne $DrawListObject.drawCalls) {
        return @($DrawListObject.drawCalls)
    }

    return @()
}

function Get-UnresolvedNodeCandidateLabel($Candidate) {
    $kindList = @($Candidate.kinds)
    if (
        ($kindList -contains "scale") -or
        ($kindList -contains "pattern-index") -or
        ($kindList -contains "hide-flag"))
    {
        return "gauge-or-prompt-candidate"
    }

    if ($kindList -contains "text") {
        $valueList = @($Candidate.values)
        if ($valueList.Count -gt 0) {
            $allNumeric = $true
            foreach ($value in $valueList) {
                if ($value -notmatch '^[0-9]+$') {
                    $allNumeric = $false
                    break
                }
            }

            if ($allNumeric) {
                return "numeric-text-counter-candidate"
            }
        }

        return "text-node-candidate"
    }

    return "unknown-unresolved-node-candidate"
}

function Format-CandidateList($Values, [int]$Limit) {
    $items = @($Values)
    if ($Limit -lt 1 -or $items.Count -le $Limit) {
        return ($items -join ",")
    }

    return (($items | Select-Object -First $Limit) -join ",") + ("...+{0}more" -f ($items.Count - $Limit))
}

$resolvedEventsPath = Resolve-HudValueEventsPath $EventsPath $EvidenceDir
$paths = [System.Collections.Generic.SortedSet[string]]::new()
$values = [System.Collections.Generic.SortedSet[string]]::new()
$sources = [System.Collections.Generic.SortedSet[string]]::new()
$semanticPathCandidates = [System.Collections.Generic.SortedSet[string]]::new()
$semanticBoundPaths = [System.Collections.Generic.SortedSet[string]]::new()
$unresolvedNodeCandidatesByNode = @{}
$semanticPathCandidateGroupsByKey = @{}
$semanticBoundGroupsByKey = @{}
$gaugeDrawPathGroupsByKey = @{}
$manualObserverHudPaths = @(
    "ui_playscreen/so_speed_gauge",
    "ui_playscreen/so_ringenagy_gauge",
    "ui_playscreen/add/u_info"
)

$summary = [ordered]@{
    kind = "manual gameplay observer Sonic HUD value summary"
    eventsPath = $resolvedEventsPath
    totalEvents = 0
    textWrites = 0
    gaugeWrites = 0
    gaugeScaleWrites = 0
    gaugePatternWrites = 0
    gaugeHideWrites = 0
    lateResolvedWrites = 0
    gameplayUpdates = 0
    gameplayValueSnapshots = 0
    callsiteClassifications = 0
    unresolvedNodeWrites = 0
    unresolvedNodeCandidateCount = 0
    semanticPathCandidateWrites = 0
    semanticBoundWrites = 0
    semanticPathCandidates = @()
    semanticPathCandidateGroups = @()
    semanticBoundPaths = @()
    semanticBoundGroups = @()
    drawListPath = ""
    gaugeDrawPathGroups = @()
    gaugeSetterNodeCandidates = @()
    gaugeChildPathStatus = "pending-runtime-ui-draw-list"
    unresolvedNodeCandidates = @()
    paths = @()
    values = @()
    sources = @()
    status = "no-sonic-hud-value-events-found"
    resolver = "manual unresolved node resolver"
}

Get-Content -LiteralPath $resolvedEventsPath | ForEach-Object {
    $line = $_
    if ([string]::IsNullOrWhiteSpace($line)) {
        return
    }

    $eventObject = $null
    try {
        $eventObject = $line | ConvertFrom-Json
    }
    catch {
        return
    }

    $eventName = [string]$eventObject.event
    $detail = [string]$eventObject.detail
    $summary.totalEvents++

    switch ($eventName) {
        "sonic-hud-value-text-write" {
            $summary.textWrites++
        }
        "sonic-hud-gauge-scale-write" {
            $summary.gaugeWrites++
            $summary.gaugeScaleWrites++
        }
        "sonic-hud-gauge-pattern-write" {
            $summary.gaugeWrites++
            $summary.gaugePatternWrites++
        }
        "sonic-hud-gauge-hide-write" {
            $summary.gaugeWrites++
            $summary.gaugeHideWrites++
        }
        "sonic-hud-node-write-late-resolved" {
            $summary.lateResolvedWrites++
        }
        "sonic-hud-value-write-update" {
            $summary.gameplayUpdates++
        }
        "sonic-hud-gameplay-values" {
            $summary.gameplayValueSnapshots++
        }
        "sonic-hud-callsite-value-classified" {
            $summary.callsiteClassifications++
        }
        "sonic-hud-node-write-semantic-path-candidate" {
            $summary.semanticPathCandidateWrites++
            Add-UniqueSorted $semanticPathCandidates (Get-DetailToken $detail "semanticPathCandidate")
            Add-SemanticPathCandidateGroup $semanticPathCandidateGroupsByKey $detail $eventObject
        }
        "sonic-hud-node-write-semantic-bound" {
            $summary.semanticBoundWrites++
            Add-UniqueSorted $semanticBoundPaths (Get-DetailToken $detail "semanticPathCandidate")
            Add-SemanticBoundGroup $semanticBoundGroupsByKey $detail $eventObject
        }
        "sonic-hud-node-write-unresolved" {
            $summary.unresolvedNodeWrites++

            $node = Get-DetailToken $detail "node"
            if (-not [string]::IsNullOrWhiteSpace($node)) {
                if (-not $unresolvedNodeCandidatesByNode.ContainsKey($node)) {
                    $unresolvedNodeCandidatesByNode[$node] = New-UnresolvedNodeCandidate $node
                }

                $candidate = $unresolvedNodeCandidatesByNode[$node]
                $candidate.writes++
                Add-UniqueSorted $candidate.kinds (Get-DetailToken $detail "kind")
                Add-UniqueSorted $candidate.values (Get-DetailToken $detail "value")
                Add-UniqueSorted $candidate.sources (Get-DetailToken $detail "source")
                Add-UniqueSorted $candidate.semanticPathCandidates (Get-DetailToken $detail "semanticPathCandidate")
                Add-UniqueSorted $candidate.semanticValueNames (Get-DetailToken $detail "semanticValueName")

                $frame = Get-EventFrame $eventObject
                if ($null -ne $frame) {
                    if ($null -eq $candidate.firstFrame -or $frame -lt $candidate.firstFrame) {
                        $candidate.firstFrame = $frame
                    }
                    if ($null -eq $candidate.lastFrame -or $frame -gt $candidate.lastFrame) {
                        $candidate.lastFrame = $frame
                    }
                }
            }
        }
    }

    if ($eventName -like "sonic-hud-*") {
        Add-UniqueSorted $paths (Get-DetailToken $detail "path")
        Add-UniqueSorted $values (Get-DetailToken $detail "value")
        Add-UniqueSorted $sources (Get-DetailToken $detail "source")
        if ($eventName -ne "sonic-hud-node-write-semantic-bound") {
            Add-UniqueSorted $semanticPathCandidates (Get-DetailToken $detail "semanticPathCandidate")
        }
    }
}

if (-not [string]::IsNullOrWhiteSpace($DrawListPath)) {
    $resolvedDrawListPath = Resolve-Path -LiteralPath $DrawListPath -ErrorAction Stop
    $summary.drawListPath = $resolvedDrawListPath.Path

    try {
        $drawListObject = Get-Content -Raw -LiteralPath $resolvedDrawListPath.Path | ConvertFrom-Json
        foreach ($drawCall in (Get-DrawListCalls $drawListObject)) {
            Add-GaugeDrawPathGroup $gaugeDrawPathGroupsByKey $drawCall
        }
    }
    catch {
        throw "Could not parse UI draw-list JSON: $($resolvedDrawListPath.Path)"
    }
}

$summary.paths = @($paths)
$summary.values = @($values)
$summary.sources = @($sources)
$summary.semanticPathCandidates = @($semanticPathCandidates)
$summary.semanticBoundPaths = @($semanticBoundPaths)
$summary.semanticPathCandidateGroups = @(
    $semanticPathCandidateGroupsByKey.Keys |
        Sort-Object @{ Expression = { -1 * $semanticPathCandidateGroupsByKey[$_].writes } }, @{ Expression = { $_ } } |
        ForEach-Object {
            $group = $semanticPathCandidateGroupsByKey[$_]
            [ordered]@{
                path = $group.path
                semanticValueName = $group.semanticValueName
                writes = $group.writes
                nodes = @($group.nodes)
                kinds = @($group.kinds)
                values = @($group.values)
                sources = @($group.sources)
                firstFrame = $group.firstFrame
                lastFrame = $group.lastFrame
            }
        }
)
$summary.semanticBoundGroups = @(
    $semanticBoundGroupsByKey.Keys |
        Sort-Object @{ Expression = { -1 * $semanticBoundGroupsByKey[$_].writes } }, @{ Expression = { $_ } } |
        ForEach-Object {
            $group = $semanticBoundGroupsByKey[$_]
            [ordered]@{
                path = $group.path
                semanticValueName = $group.semanticValueName
                writes = $group.writes
                nodes = @($group.nodes)
                kinds = @($group.kinds)
                values = @($group.values)
                sources = @($group.sources)
                firstFrame = $group.firstFrame
                lastFrame = $group.lastFrame
            }
        }
)
$summary.gaugeDrawPathGroups = @(
    $gaugeDrawPathGroupsByKey.Keys |
        Sort-Object @{ Expression = { -1 * $gaugeDrawPathGroupsByKey[$_].draws } }, @{ Expression = { $_ } } |
        ForEach-Object {
            $group = $gaugeDrawPathGroupsByKey[$_]
            [ordered]@{
                path = $group.path
                semanticValueName = $group.semanticValueName
                draws = $group.draws
                childPaths = @($group.childPaths)
                layerAddresses = @($group.layerAddresses)
                castNodeAddresses = @($group.castNodeAddresses)
            }
        }
)
$summary.unresolvedNodeCandidates = @(
    $unresolvedNodeCandidatesByNode.Keys |
        Sort-Object @{ Expression = { -1 * $unresolvedNodeCandidatesByNode[$_].writes } }, @{ Expression = { $_ } } |
        ForEach-Object {
            $candidate = $unresolvedNodeCandidatesByNode[$_]
            $candidate.likely = Get-UnresolvedNodeCandidateLabel $candidate
            [ordered]@{
                node = $candidate.node
                writes = $candidate.writes
                kinds = @($candidate.kinds)
                values = @($candidate.values)
                sources = @($candidate.sources)
                semanticPathCandidates = @($candidate.semanticPathCandidates)
                semanticValueNames = @($candidate.semanticValueNames)
                firstFrame = $candidate.firstFrame
                lastFrame = $candidate.lastFrame
                likely = $candidate.likely
            }
        }
)
$summary.unresolvedNodeCandidateCount = $summary.unresolvedNodeCandidates.Count
$summary.gaugeSetterNodeCandidates = @(
    $summary.unresolvedNodeCandidates |
        Where-Object {
            $_.likely -eq "gauge-or-prompt-candidate" -and
            @($_.semanticPathCandidates).Count -gt 0
        } |
        Sort-Object @{ Expression = { -1 * $_.writes } }, @{ Expression = { $_.node } } |
        ForEach-Object {
            [ordered]@{
                node = $_.node
                writes = $_.writes
                kinds = @($_.kinds)
                semanticValueNames = @($_.semanticValueNames)
                semanticPathCandidates = @($_.semanticPathCandidates)
                firstFrame = $_.firstFrame
                lastFrame = $_.lastFrame
                nodeJoinStatus = "setter-node-address-join-pending"
            }
        }
)
if ($summary.gaugeDrawPathGroups.Count -gt 0) {
    $summary.gaugeChildPathStatus = "runtime-draw-list-exact-child-paths;setter-node-address-join-pending"
}

if (
    $summary.textWrites -gt 0 -or
    $summary.gaugeWrites -gt 0 -or
    $summary.lateResolvedWrites -gt 0 -or
    $summary.gameplayUpdates -gt 0 -or
    $summary.gameplayValueSnapshots -gt 0 -or
    $summary.callsiteClassifications -gt 0 -or
    $summary.semanticPathCandidateWrites -gt 0 -or
    $summary.semanticBoundWrites -gt 0 -or
    $summary.unresolvedNodeWrites -gt 0)
{
    $summary.status = "sonic-hud-value-events-found"
}

if ($AsJson) {
    $summary | ConvertTo-Json -Depth 6
    return
}

Write-Output "sward_ui_lab_hud_value_summary"
Write-Output ("events_path={0}" -f $summary.eventsPath)
Write-Output (
    "text_writes={0}:gauge_writes={1}:gauge_scale={2}:gauge_pattern={3}:gauge_hide={4}:late_resolved={5}:gameplay_updates={6}:gameplay_values={7}:callsite_classifications={8}" -f
    $summary.textWrites,
    $summary.gaugeWrites,
    $summary.gaugeScaleWrites,
    $summary.gaugePatternWrites,
    $summary.gaugeHideWrites,
    $summary.lateResolvedWrites,
    $summary.gameplayUpdates,
    $summary.gameplayValueSnapshots,
    $summary.callsiteClassifications)
Write-Output (
    "unresolved_node_writes={0}:node_candidates={1}" -f
    $summary.unresolvedNodeWrites,
    $summary.unresolvedNodeCandidateCount)
Write-Output ("semantic_path_candidates={0}" -f $summary.semanticPathCandidateWrites)
Write-Output ("semantic_bound={0}" -f $summary.semanticBoundWrites)
Write-Output ("semantic_candidate_paths={0}" -f (Format-CandidateList $summary.semanticPathCandidates $CandidateValueLimit))
Write-Output (
    "semantic_candidate_groups={0}" -f
    (Format-CandidateList (
        $summary.semanticPathCandidateGroups |
            ForEach-Object { "{0}:{1}={2}" -f $_.path, $_.semanticValueName, $_.writes }
    ) $CandidateValueLimit))
Write-Output ("semantic_bound_paths={0}" -f (Format-CandidateList $summary.semanticBoundPaths $CandidateValueLimit))
Write-Output (
    "semantic_bound_groups={0}" -f
    (Format-CandidateList (
        $summary.semanticBoundGroups |
            ForEach-Object { "{0}:{1}={2}" -f $_.path, $_.semanticValueName, $_.writes }
    ) $CandidateValueLimit))
Write-Output (
    "gauge_draw_path_groups={0}" -f
    (Format-CandidateList (
        $summary.gaugeDrawPathGroups |
            ForEach-Object { "{0}:{1}={2}" -f $_.path, $_.semanticValueName, $_.draws }
    ) $CandidateValueLimit))
Write-Output (
    "gauge_setter_node_candidates={0}" -f
    (Format-CandidateList (
        $summary.gaugeSetterNodeCandidates |
            ForEach-Object {
                "{0}:{1}:{2}:{3}={4}" -f
                    $_.node,
                    (Format-CandidateList $_.semanticValueNames $CandidateValueLimit),
                    (Format-CandidateList $_.kinds $CandidateValueLimit),
                    (Format-CandidateList $_.semanticPathCandidates $CandidateValueLimit),
                    $_.writes
            }
    ) $CandidateValueLimit))
Write-Output ("gauge_child_path_status={0}" -f $summary.gaugeChildPathStatus)
foreach ($candidate in $summary.unresolvedNodeCandidates) {
    Write-Output (
        "node_candidate node={0} writes={1} kinds={2} values={3} frames={4}-{5} likely={6} sources={7}" -f
        $candidate.node,
        $candidate.writes,
        (Format-CandidateList $candidate.kinds $CandidateValueLimit),
        (Format-CandidateList $candidate.values $CandidateValueLimit),
        $candidate.firstFrame,
        $candidate.lastFrame,
        $candidate.likely,
        (Format-CandidateList $candidate.sources $CandidateValueLimit))
}
Write-Output ("paths={0}" -f (Format-CandidateList $summary.paths $CandidateValueLimit))
Write-Output ("values={0}" -f (Format-CandidateList $summary.values $CandidateValueLimit))
Write-Output ("sources={0}" -f (Format-CandidateList $summary.sources $CandidateValueLimit))
Write-Output ("status={0}" -f $summary.status)
