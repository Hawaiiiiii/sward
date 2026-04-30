param(
    [string]$EventsPath = "",
    [string]$EvidenceDir = "",
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
        firstFrame = $null
        lastFrame = $null
        likely = "unknown-unresolved-node-candidate"
    }
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
$unresolvedNodeCandidatesByNode = @{}
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
    semanticPathCandidates = @()
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
        Add-UniqueSorted $semanticPathCandidates (Get-DetailToken $detail "semanticPathCandidate")
    }
}

$summary.paths = @($paths)
$summary.values = @($values)
$summary.sources = @($sources)
$summary.semanticPathCandidates = @($semanticPathCandidates)
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
                firstFrame = $candidate.firstFrame
                lastFrame = $candidate.lastFrame
                likely = $candidate.likely
            }
        }
)
$summary.unresolvedNodeCandidateCount = $summary.unresolvedNodeCandidates.Count

if (
    $summary.textWrites -gt 0 -or
    $summary.gaugeWrites -gt 0 -or
    $summary.lateResolvedWrites -gt 0 -or
    $summary.gameplayUpdates -gt 0 -or
    $summary.gameplayValueSnapshots -gt 0 -or
    $summary.callsiteClassifications -gt 0 -or
    $summary.semanticPathCandidateWrites -gt 0 -or
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
Write-Output ("semantic_candidate_paths={0}" -f (Format-CandidateList $summary.semanticPathCandidates $CandidateValueLimit))
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
