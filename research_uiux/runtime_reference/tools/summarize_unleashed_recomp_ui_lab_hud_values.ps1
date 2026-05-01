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

function New-RollingCounterSemanticGroup([string]$Path, [string]$ValueName, [string]$Kind, [string]$Callsite) {
    return [ordered]@{
        path = $Path
        semanticValueName = $ValueName
        kind = $Kind
        callsite = $Callsite
        writes = 0
        nodes = [System.Collections.Generic.SortedSet[string]]::new()
        values = [System.Collections.Generic.SortedSet[string]]::new()
        sources = [System.Collections.Generic.SortedSet[string]]::new()
        firstFrame = $null
        lastFrame = $null
    }
}

function Get-CallsiteFromSource([string]$Source) {
    $match = [regex]::Match($Source, "sub_[0-9A-Fa-f]+")
    if ($match.Success) {
        return $match.Value
    }
    return ""
}

function Add-RollingCounterSemanticGroup($Groups, [string]$Detail, $EventObject) {
    $kind = Get-DetailToken $Detail "kind"
    if ($kind -ne "text") {
        return
    }

    $source = Get-DetailToken $Detail "source"
    $callsite = Get-CallsiteFromSource $source
    if ($callsite -ne "sub_824D6C18") {
        return
    }

    $path = Get-DetailToken $Detail "semanticPathCandidate"
    $valueName = Get-DetailToken $Detail "semanticValueName"
    if (
        [string]::IsNullOrWhiteSpace($path) -or
        [string]::IsNullOrWhiteSpace($valueName) -or
        ($valueName -ne "boostGauge" -and $valueName -ne "ringEnergyGauge"))
    {
        return
    }

    $key = "{0}:{1}:{2}:{3}" -f $path, $valueName, $kind, $callsite
    if (-not $Groups.ContainsKey($key)) {
        $Groups[$key] = New-RollingCounterSemanticGroup $path $valueName $kind $callsite
    }

    $group = $Groups[$key]
    $group.writes++
    Add-UniqueSorted $group.nodes (Get-DetailToken $Detail "node")
    Add-UniqueSorted $group.values (Get-DetailToken $Detail "value")
    Add-UniqueSorted $group.sources $source

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

function New-OwnerFieldGaugeScaleCorrelationGroup([string]$Path, [string]$OwnerAddress, [int]$FieldOffset) {
    return [ordered]@{
        path = $Path
        ownerAddress = $OwnerAddress
        fieldOffset = $FieldOffset
        joins = 0
        observedScales = [System.Collections.Generic.SortedSet[string]]::new()
        observedFieldValues = [System.Collections.Generic.SortedSet[string]]::new()
        sources = [System.Collections.Generic.SortedSet[string]]::new()
        firstFrame = $null
        lastFrame = $null
    }
}

function Add-OwnerFieldGaugeScaleCorrelationGroup($Groups, [string]$Detail, $EventObject) {
    $path = Get-DetailToken $Detail "path"
    $ownerAddress = Get-DetailToken $Detail "ownerAddress"
    $scale = Get-DetailToken $Detail "scale"

    if ([string]::IsNullOrWhiteSpace($path) -or [string]::IsNullOrWhiteSpace($ownerAddress)) {
        return
    }

    if (-not (
        $path.StartsWith("ui_playscreen/so_speed_gauge") -or
        $path.StartsWith("ui_playscreen/so_ringenagy_gauge")))
    {
        return
    }

    $offsetFields = @(
        @{ Token = "ownerField460"; Offset = 460 },
        @{ Token = "ownerField464"; Offset = 464 },
        @{ Token = "ownerField468"; Offset = 468 },
        @{ Token = "ownerField472"; Offset = 472 },
        @{ Token = "ownerField480"; Offset = 480 }
    )

    $source = Get-DetailToken $Detail "source"
    $frame = Get-EventFrame $EventObject

    foreach ($entry in $offsetFields) {
        $rawValue = Get-DetailToken $Detail $entry.Token
        if ([string]::IsNullOrWhiteSpace($rawValue)) {
            continue
        }

        $key = "{0}:{1}:{2}" -f $path, $ownerAddress, $entry.Offset
        if (-not $Groups.ContainsKey($key)) {
            $Groups[$key] = New-OwnerFieldGaugeScaleCorrelationGroup $path $ownerAddress $entry.Offset
        }

        $group = $Groups[$key]
        $group.joins++
        Add-UniqueSorted $group.observedFieldValues $rawValue
        if (-not [string]::IsNullOrWhiteSpace($scale)) {
            Add-UniqueSorted $group.observedScales $scale
        }
        if (-not [string]::IsNullOrWhiteSpace($source)) {
            Add-UniqueSorted $group.sources $source
        }

        if ($null -ne $frame) {
            if ($null -eq $group.firstFrame -or $frame -lt $group.firstFrame) {
                $group.firstFrame = $frame
            }
            if ($null -eq $group.lastFrame -or $frame -gt $group.lastFrame) {
                $group.lastFrame = $frame
            }
        }
    }
}

function New-OwnerFieldRollingCounterGroup([string]$OwnerAddress, [int]$FieldOffset, [string]$ValueName, [string]$Callsite) {
    return [ordered]@{
        ownerAddress = $OwnerAddress
        fieldOffset = $FieldOffset
        semanticValueName = $ValueName
        callsite = $Callsite
        samples = 0
        observedValues = [System.Collections.Generic.SortedSet[string]]::new()
        observedValueHexes = [System.Collections.Generic.SortedSet[string]]::new()
        candidatePaths = [System.Collections.Generic.SortedSet[string]]::new()
        sources = [System.Collections.Generic.SortedSet[string]]::new()
        firstFrame = $null
        lastFrame = $null
    }
}

function Add-OwnerFieldRollingCounterGroup($Groups, [string]$Detail, $EventObject) {
    $callsite = Get-DetailToken $Detail "callsite"
    if ($callsite -ne "sub_824D6C18") {
        return
    }

    $ownerAddress = Get-DetailToken $Detail "ownerAddress"
    if ([string]::IsNullOrWhiteSpace($ownerAddress)) {
        return
    }

    $offsetsRaw = Get-DetailToken $Detail "fieldOffsets"
    $valuesRaw = Get-DetailToken $Detail "fieldValues"
    $hexesRaw = Get-DetailToken $Detail "fieldValueHexes"
    $candidateValueNamesRaw = Get-DetailToken $Detail "candidateValueNames"
    $candidatePathsRaw = Get-DetailToken $Detail "candidatePaths"

    if ([string]::IsNullOrWhiteSpace($offsetsRaw) -or [string]::IsNullOrWhiteSpace($valuesRaw)) {
        return
    }

    $offsets = $offsetsRaw -split ","
    $values = $valuesRaw -split ","
    $hexes = if ([string]::IsNullOrWhiteSpace($hexesRaw)) { @() } else { $hexesRaw -split "," }

    if ($offsets.Count -ne $values.Count) {
        return
    }

    $candidateValueNames = if ([string]::IsNullOrWhiteSpace($candidateValueNamesRaw)) {
        @("boostGauge", "ringEnergyGauge")
    } else {
        @($candidateValueNamesRaw -split "\|" | Where-Object { $_ -eq "boostGauge" -or $_ -eq "ringEnergyGauge" })
    }
    if ($candidateValueNames.Count -eq 0) {
        return
    }

    $candidatePaths = if ([string]::IsNullOrWhiteSpace($candidatePathsRaw)) { @() } else { $candidatePathsRaw -split "\|" }
    $source = Get-DetailToken $Detail "source"
    $frame = Get-EventFrame $EventObject

    for ($i = 0; $i -lt $offsets.Count; $i++) {
        $offsetText = $offsets[$i].Trim()
        $valueText = $values[$i].Trim()
        if ([string]::IsNullOrWhiteSpace($offsetText)) {
            continue
        }

        $offset = 0
        if (-not [int]::TryParse($offsetText, [ref]$offset)) {
            continue
        }

        $hexText = ""
        if ($i -lt $hexes.Count) {
            $hexText = $hexes[$i].Trim()
        }

        foreach ($valueName in $candidateValueNames) {
            $key = "{0}:{1}:{2}:{3}" -f $ownerAddress, $offset, $valueName, $callsite
            if (-not $Groups.ContainsKey($key)) {
                $Groups[$key] = New-OwnerFieldRollingCounterGroup $ownerAddress $offset $valueName $callsite
            }

            $group = $Groups[$key]
            $group.samples++
            Add-UniqueSorted $group.observedValues $valueText
            if (-not [string]::IsNullOrWhiteSpace($hexText)) {
                Add-UniqueSorted $group.observedValueHexes $hexText
            }
            foreach ($candidatePath in $candidatePaths) {
                Add-UniqueSorted $group.candidatePaths $candidatePath
            }
            if (-not [string]::IsNullOrWhiteSpace($source)) {
                Add-UniqueSorted $group.sources $source
            }

            if ($null -ne $frame) {
                if ($null -eq $group.firstFrame -or $frame -lt $group.firstFrame) {
                    $group.firstFrame = $frame
                }
                if ($null -eq $group.lastFrame -or $frame -gt $group.lastFrame) {
                    $group.lastFrame = $frame
                }
            }
        }
    }
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

function New-GaugeDrawCallRecord($DrawCall) {
    $layerPath = [string]$DrawCall.layerPath
    if ([string]::IsNullOrWhiteSpace($layerPath)) {
        return $null
    }

    $info = Get-GaugeDrawPathInfo $layerPath
    if ($null -eq $info) {
        return $null
    }

    return [ordered]@{
        parentPath = $info.parent
        semanticValueName = $info.semanticValueName
        childPath = $layerPath
        layerAddress = [string]$DrawCall.layerAddress
        castNodeAddress = [string]$DrawCall.castNodeAddress
    }
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

function Get-CompositeTokens($Values) {
    $tokens = [System.Collections.Generic.List[string]]::new()
    foreach ($value in @($Values)) {
        foreach ($part in ([string]$value -split "\|")) {
            $trimmed = $part.Trim()
            if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
                [void]$tokens.Add($trimmed)
            }
        }
    }
    return @($tokens)
}

function Test-SemanticPathCandidateMatchesGaugeDraw([string[]]$SemanticPathCandidates, [string]$GaugeParentPath) {
    foreach ($candidatePath in (Get-CompositeTokens $SemanticPathCandidates)) {
        if ($candidatePath -eq $GaugeParentPath) {
            return $true
        }

        if ($GaugeParentPath.StartsWith($candidatePath + "/")) {
            return $true
        }
    }

    return $false
}

function Test-SemanticValueNameMatchesGaugeDraw([string[]]$SemanticValueNames, [string]$DrawValueName) {
    $tokens = @(Get-CompositeTokens $SemanticValueNames)
    if ($tokens.Count -eq 0) {
        return $false
    }

    return $tokens -contains $DrawValueName
}

function New-GaugeSetterChildPathJoin($Candidate, $DrawCall, [string]$AddressMatchKind) {
    return [ordered]@{
        node = $Candidate.node
        writes = $Candidate.writes
        kinds = @($Candidate.kinds)
        semanticValueName = $DrawCall.semanticValueName
        exactParentPath = $DrawCall.parentPath
        exactChildPath = $DrawCall.childPath
        addressMatchKind = $AddressMatchKind
        nodeJoinStatus = "setter-node-address-join-runtime-proven"
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
$semanticBoundPaths = [System.Collections.Generic.SortedSet[string]]::new()
$unresolvedNodeCandidatesByNode = @{}
$semanticPathCandidateGroupsByKey = @{}
$semanticBoundGroupsByKey = @{}
$rollingCounterSemanticGroupsByKey = @{}
$ownerFieldRollingCounterGroupsByKey = @{}
$ownerFieldGaugeScaleCorrelationGroupsByKey = @{}
$gaugeDrawPathGroupsByKey = @{}
$gaugeDrawCalls = @()
$gaugeDrawCastNodeAddresses = [System.Collections.Generic.SortedSet[string]]::new()
$gaugeDrawLayerAddresses = [System.Collections.Generic.SortedSet[string]]::new()
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
    rollingCounterSemanticGroups = @()
    boostRingEnergyStatus = "pending-runtime-rolling-counter-evidence"
    ownerFieldRollingCounterGroups = @()
    ownerFieldRollingCounterStatus = "pending-runtime-owner-field-rolling-counter-evidence"
    ownerFieldGaugeScaleCorrelationGroups = @()
    ownerFieldGaugeScaleCorrelationStatus = "pending-runtime-owner-field-gauge-scale-correlation-evidence"
    drawListPath = ""
    gaugeDrawPathGroups = @()
    gaugeSetterNodeCandidates = @()
    gaugeSetterChildPathJoins = @()
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
            Add-RollingCounterSemanticGroup $rollingCounterSemanticGroupsByKey $detail $eventObject
        }
        "sonic-hud-owner-gauge-snapshot" {
            Add-OwnerFieldRollingCounterGroup $ownerFieldRollingCounterGroupsByKey $detail $eventObject
        }
        "sonic-hud-gauge-scale-owner-correlated" {
            Add-OwnerFieldGaugeScaleCorrelationGroup $ownerFieldGaugeScaleCorrelationGroupsByKey $detail $eventObject
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
            $drawRecord = New-GaugeDrawCallRecord $drawCall
            if ($null -ne $drawRecord) {
                $gaugeDrawCalls += $drawRecord
                Add-UniqueSorted $gaugeDrawLayerAddresses $drawRecord.layerAddress
                Add-UniqueSorted $gaugeDrawCastNodeAddresses $drawRecord.castNodeAddress
            }
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
$summary.rollingCounterSemanticGroups = @(
    $rollingCounterSemanticGroupsByKey.Keys |
        Sort-Object @{ Expression = { -1 * $rollingCounterSemanticGroupsByKey[$_].writes } }, @{ Expression = { $_ } } |
        ForEach-Object {
            $group = $rollingCounterSemanticGroupsByKey[$_]
            [ordered]@{
                path = $group.path
                semanticValueName = $group.semanticValueName
                kind = $group.kind
                callsite = $group.callsite
                writes = $group.writes
                nodes = @($group.nodes)
                values = @($group.values)
                sources = @($group.sources)
                firstFrame = $group.firstFrame
                lastFrame = $group.lastFrame
            }
        }
)
$summary.ownerFieldRollingCounterGroups = @(
    $ownerFieldRollingCounterGroupsByKey.Keys |
        Sort-Object `
            @{ Expression = { -1 * $ownerFieldRollingCounterGroupsByKey[$_].samples } }, `
            @{ Expression = { $ownerFieldRollingCounterGroupsByKey[$_].fieldOffset } }, `
            @{ Expression = { $ownerFieldRollingCounterGroupsByKey[$_].semanticValueName } } |
        ForEach-Object {
            $group = $ownerFieldRollingCounterGroupsByKey[$_]
            [ordered]@{
                ownerAddress = $group.ownerAddress
                fieldOffset = $group.fieldOffset
                semanticValueName = $group.semanticValueName
                callsite = $group.callsite
                samples = $group.samples
                observedValues = @($group.observedValues)
                observedValueHexes = @($group.observedValueHexes)
                candidatePaths = @($group.candidatePaths)
                sources = @($group.sources)
                firstFrame = $group.firstFrame
                lastFrame = $group.lastFrame
            }
        }
)
$summary.ownerFieldGaugeScaleCorrelationGroups = @(
    $ownerFieldGaugeScaleCorrelationGroupsByKey.Keys |
        Sort-Object `
            @{ Expression = { -1 * $ownerFieldGaugeScaleCorrelationGroupsByKey[$_].joins } }, `
            @{ Expression = { $ownerFieldGaugeScaleCorrelationGroupsByKey[$_].path } }, `
            @{ Expression = { $ownerFieldGaugeScaleCorrelationGroupsByKey[$_].fieldOffset } } |
        ForEach-Object {
            $group = $ownerFieldGaugeScaleCorrelationGroupsByKey[$_]
            [ordered]@{
                path = $group.path
                ownerAddress = $group.ownerAddress
                fieldOffset = $group.fieldOffset
                joins = $group.joins
                observedScales = @($group.observedScales)
                observedFieldValues = @($group.observedFieldValues)
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
        ForEach-Object {
            $joinStatus = "setter-node-address-join-pending"
            if ($gaugeDrawCastNodeAddresses.Contains($_.node) -or $gaugeDrawLayerAddresses.Contains($_.node)) {
                $joinStatus = "setter-node-address-join-runtime-proven"
            }

            [ordered]@{
                node = $_.node
                writes = $_.writes
                kinds = @($_.kinds)
                semanticValueNames = @($_.semanticValueNames)
                semanticPathCandidates = @($_.semanticPathCandidates)
                firstFrame = $_.firstFrame
                lastFrame = $_.lastFrame
                nodeJoinStatus = $joinStatus
            }
        } |
        Sort-Object `
            @{ Expression = { if ($_.nodeJoinStatus -eq "setter-node-address-join-runtime-proven") { 0 } else { 1 } } }, `
            @{ Expression = { -1 * $_.writes } }, `
            @{ Expression = { $_.node } }
)
$summary.gaugeSetterChildPathJoins = @(
    foreach ($candidate in $summary.gaugeSetterNodeCandidates) {
        foreach ($drawCall in $gaugeDrawCalls) {
            $addressMatchKind = ""
            if ($candidate.node -eq $drawCall.castNodeAddress) {
                $addressMatchKind = "cast-node"
            }
            elseif ($candidate.node -eq $drawCall.layerAddress) {
                $addressMatchKind = "layer"
            }

            if ([string]::IsNullOrWhiteSpace($addressMatchKind)) {
                continue
            }

            if (-not (Test-SemanticValueNameMatchesGaugeDraw $candidate.semanticValueNames $drawCall.semanticValueName)) {
                continue
            }

            if (-not (Test-SemanticPathCandidateMatchesGaugeDraw $candidate.semanticPathCandidates $drawCall.parentPath)) {
                continue
            }

            New-GaugeSetterChildPathJoin $candidate $drawCall $addressMatchKind
        }
    }
)
if ($summary.gaugeSetterChildPathJoins.Count -gt 0) {
    $summary.gaugeChildPathStatus = "runtime-draw-list-setter-node-joined"
}
elseif ($summary.gaugeDrawPathGroups.Count -gt 0) {
    $summary.gaugeChildPathStatus = "runtime-draw-list-exact-child-paths;setter-node-address-join-pending"
}

if ($summary.rollingCounterSemanticGroups.Count -gt 0 -and $summary.gaugeSetterChildPathJoins.Count -eq 0) {
    $summary.boostRingEnergyStatus = "rolling-counter-text-candidate-pending-gauge-state-normalization"
}
elseif ($summary.gaugeSetterChildPathJoins.Count -gt 0) {
    $summary.boostRingEnergyStatus = "setter-node-address-join-runtime-proven"
}

if ($summary.ownerFieldRollingCounterGroups.Count -gt 0 -and $summary.gaugeSetterChildPathJoins.Count -eq 0) {
    $summary.ownerFieldRollingCounterStatus = "owner-field-rolling-counter-pending-exact-offset-normalization"
}
elseif ($summary.gaugeSetterChildPathJoins.Count -gt 0) {
    $summary.ownerFieldRollingCounterStatus = "owner-field-rolling-counter-correlated-with-setter-node-address-join"
}

if ($summary.ownerFieldGaugeScaleCorrelationGroups.Count -gt 0) {
    $summary.ownerFieldGaugeScaleCorrelationStatus = "owner-field-gauge-scale-correlation-pending-formula-proof"
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
    "rolling_counter_semantic_groups={0}" -f
    (Format-CandidateList (
        $summary.rollingCounterSemanticGroups |
            ForEach-Object { "{0}:{1}:{2}:{3}={4}" -f $_.path, $_.semanticValueName, $_.kind, $_.callsite, $_.writes }
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
Write-Output (
    "gauge_setter_child_path_joins={0}" -f
    (Format-CandidateList (
        $summary.gaugeSetterChildPathJoins |
            ForEach-Object {
                "{0}:{1}:{2}:{3}:{4}={5}" -f
                    $_.node,
                    $_.semanticValueName,
                    (Format-CandidateList $_.kinds $CandidateValueLimit),
                    $_.addressMatchKind,
                    $_.exactChildPath,
                    $_.writes
            }
    ) $CandidateValueLimit))
Write-Output ("gauge_child_path_status={0}" -f $summary.gaugeChildPathStatus)
Write-Output ("boost_ring_energy_status={0}" -f $summary.boostRingEnergyStatus)
Write-Output (
    "owner_field_rolling_counter_groups={0}" -f
    (Format-CandidateList (
        $summary.ownerFieldRollingCounterGroups |
            ForEach-Object {
                "owner={0}:field+{1}={2}:samples={3}" -f
                    $_.ownerAddress,
                    $_.fieldOffset,
                    $_.semanticValueName,
                    $_.samples
            }
    ) $CandidateValueLimit))
Write-Output ("owner_field_rolling_counter_status={0}" -f $summary.ownerFieldRollingCounterStatus)
Write-Output (
    "owner_field_gauge_scale_correlation_groups={0}" -f
    (Format-CandidateList (
        $summary.ownerFieldGaugeScaleCorrelationGroups |
            ForEach-Object {
                "path={0}:owner={1}:field+{2}:joins={3}" -f
                    $_.path,
                    $_.ownerAddress,
                    $_.fieldOffset,
                    $_.joins
            }
    ) $CandidateValueLimit))
Write-Output ("owner_field_gauge_scale_correlation_status={0}" -f $summary.ownerFieldGaugeScaleCorrelationStatus)
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
