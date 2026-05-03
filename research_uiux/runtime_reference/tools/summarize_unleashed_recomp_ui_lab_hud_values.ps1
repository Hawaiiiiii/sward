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

function Resolve-OwnerFieldOffsetMonotonicTrendLabel([int]$IncCount, [int]$DecCount, [int]$EqCount) {
    # Phase 200: pure-shape trend label over the consecutive-pair transitions
    # of one (owner, offset). Diagnostic only — does not assert any per-frame
    # meaning of the transitions, only the direction mix.
    $total = $IncCount + $DecCount + $EqCount
    if ($total -le 0) {
        return "no-transitions"
    }
    if ($EqCount -eq $total) {
        return "stable"
    }
    if ($IncCount -gt 0 -and $DecCount -eq 0) {
        return "increasing"
    }
    if ($DecCount -gt 0 -and $IncCount -eq 0) {
        return "decreasing"
    }
    return "non-monotonic"
}

function New-OwnerFieldOffsetTransitionTrack([string]$OwnerAddress, [int]$FieldOffset) {
    return [ordered]@{
        ownerAddress = $OwnerAddress
        fieldOffset = $FieldOffset
        # Ordered (frame, value) samples; frame may be $null when the source
        # event lacks a frame field.
        samples = New-Object System.Collections.Generic.List[object]
    }
}

function Add-OwnerFieldOffsetTransitionSample($Tracks, [string]$Detail, $EventObject) {
    # Phase 200: append one (frame, value) sample per offset for every
    # sonic-hud-owner-gauge-snapshot event so the temporal order is
    # preserved (the existing Phase 197 group only kept a SortedSet).
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
    if ([string]::IsNullOrWhiteSpace($offsetsRaw) -or [string]::IsNullOrWhiteSpace($valuesRaw)) {
        return
    }

    $offsets = $offsetsRaw -split ","
    $values = $valuesRaw -split ","
    if ($offsets.Count -ne $values.Count) {
        return
    }

    $frame = Get-EventFrame $EventObject
    for ($i = 0; $i -lt $offsets.Count; $i++) {
        $offset = 0
        if (-not [int]::TryParse($offsets[$i].Trim(), [ref]$offset)) {
            continue
        }
        $value = 0
        if (-not [int]::TryParse($values[$i].Trim(), [ref]$value)) {
            continue
        }
        $key = "{0}:{1}" -f $ownerAddress, $offset
        if (-not $Tracks.ContainsKey($key)) {
            $Tracks[$key] = New-OwnerFieldOffsetTransitionTrack $ownerAddress $offset
        }
        [void]$Tracks[$key].samples.Add([pscustomobject]@{ frame = $frame; value = $value })
    }
}

function Resolve-OwnerFieldOffsetCandidateLabel([int]$Cardinality, [int]$ObservedMin, [int]$ObservedMax) {
    # Phase 199: shape candidate labels, evaluated in narrowest-first order so
    # the smallest concrete shape wins. These are *shapes*, not formulas; the
    # label says nothing definitive about the offset's per-frame meaning, only
    # what the observed integer distribution looks like across Phase 197/198
    # evidence.
    [void]$ObservedMin
    if ($Cardinality -le 2 -and $ObservedMax -le 8) {
        return "low-cardinality-narrow-range-candidate"
    }
    if ($Cardinality -le 8 -and $ObservedMax -le 16) {
        return "moderate-cardinality-narrow-range-candidate"
    }
    if ($Cardinality -ge 4 -and $ObservedMax -ge 32 -and $ObservedMax -le 256) {
        return "high-cardinality-narrow-range-candidate"
    }
    if ($Cardinality -ge 4 -and $ObservedMax -gt 256) {
        return "high-cardinality-wide-range-candidate"
    }
    return "unclassified-pending-more-evidence"
}

function New-OwnerFieldOffsetClassification([string]$OwnerAddress, [int]$FieldOffset) {
    return [ordered]@{
        ownerAddress = $OwnerAddress
        fieldOffset = $FieldOffset
        observedCardinality = 0
        observedMin = 0
        observedMax = 0
        joinCount = 0
        snapshotCount = 0
        observedValueSet = [System.Collections.Generic.SortedSet[int]]::new()
        candidateLabel = "unclassified-pending-more-evidence"
    }
}

function Add-OwnerFieldOffsetClassification(
    $Classifications,
    [string]$OwnerAddress,
    [int]$FieldOffset,
    $ObservedValueStrings,
    [int]$JoinDelta,
    [int]$SnapshotDelta)
{
    if ([string]::IsNullOrWhiteSpace($OwnerAddress) -or $FieldOffset -le 0) {
        return
    }
    $key = "{0}:{1}" -f $OwnerAddress, $FieldOffset
    if (-not $Classifications.ContainsKey($key)) {
        $Classifications[$key] = New-OwnerFieldOffsetClassification $OwnerAddress $FieldOffset
    }
    $entry = $Classifications[$key]
    if ($JoinDelta -gt 0) {
        $entry.joinCount += $JoinDelta
    }
    if ($SnapshotDelta -gt $entry.snapshotCount) {
        $entry.snapshotCount = $SnapshotDelta
    }
    if ($null -ne $ObservedValueStrings) {
        foreach ($valueText in $ObservedValueStrings) {
            $parsed = 0
            if ([int]::TryParse($valueText, [ref]$parsed)) {
                [void]$entry.observedValueSet.Add($parsed)
            }
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

function New-OwnerSetterCandidateCorrelationGroup(
    [string]$Node,
    [string]$SemanticValueName,
    [string]$Kind,
    [string]$SemanticPathCandidate,
    [string]$OwnerAddress)
{
    return [ordered]@{
        node = $Node
        semanticValueName = $SemanticValueName
        kind = $Kind
        semanticPathCandidate = $SemanticPathCandidate
        ownerAddress = $OwnerAddress
        joins = 0
        observedSetterValues = [System.Collections.Generic.SortedSet[string]]::new()
        ownerFieldValuesByOffset = [ordered]@{
            "460" = [System.Collections.Generic.SortedSet[string]]::new()
            "464" = [System.Collections.Generic.SortedSet[string]]::new()
            "468" = [System.Collections.Generic.SortedSet[string]]::new()
            "472" = [System.Collections.Generic.SortedSet[string]]::new()
            "480" = [System.Collections.Generic.SortedSet[string]]::new()
        }
        frameDeltas = [System.Collections.Generic.SortedSet[string]]::new()
        callsites = [System.Collections.Generic.SortedSet[string]]::new()
        sources = [System.Collections.Generic.SortedSet[string]]::new()
        firstFrame = $null
        lastFrame = $null
    }
}

function Add-OwnerSetterCandidateCorrelationGroup($Groups, [string]$Detail, $EventObject) {
    $node = Get-DetailToken $Detail "node"
    $semanticValueName = Get-DetailToken $Detail "semanticValueName"
    $kind = Get-DetailToken $Detail "kind"
    $semanticPathCandidate = Get-DetailToken $Detail "semanticPathCandidate"
    $ownerAddress = Get-DetailToken $Detail "ownerAddress"

    if (
        [string]::IsNullOrWhiteSpace($node) -or
        [string]::IsNullOrWhiteSpace($semanticValueName) -or
        [string]::IsNullOrWhiteSpace($kind) -or
        [string]::IsNullOrWhiteSpace($semanticPathCandidate) -or
        [string]::IsNullOrWhiteSpace($ownerAddress))
    {
        return
    }

    $key = "{0}:{1}:{2}:{3}:{4}" -f $node, $semanticValueName, $kind, $semanticPathCandidate, $ownerAddress
    if (-not $Groups.ContainsKey($key)) {
        $Groups[$key] = New-OwnerSetterCandidateCorrelationGroup `
            $node `
            $semanticValueName `
            $kind `
            $semanticPathCandidate `
            $ownerAddress
    }

    $group = $Groups[$key]
    $group.joins++
    Add-UniqueSorted $group.observedSetterValues (Get-DetailToken $Detail "value")
    Add-UniqueSorted $group.frameDeltas (Get-DetailToken $Detail "frameDelta")

    $callsiteSource = Get-DetailToken $Detail "callsiteSource"
    $callsite = Get-CallsiteFromSource $callsiteSource
    if (-not [string]::IsNullOrWhiteSpace($callsite)) {
        Add-UniqueSorted $group.callsites $callsite
    }
    Add-UniqueSorted $group.sources (Get-DetailToken $Detail "source")

    foreach ($offset in @("460", "464", "468", "472", "480")) {
        $fieldValue = Get-DetailToken $Detail ("ownerField{0}" -f $offset)
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            Add-UniqueSorted $group.ownerFieldValuesByOffset[$offset] $fieldValue
        }
    }

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

function New-OwnerSetterCandidateNumericRelationGroup(
    [string]$Node,
    [string]$SemanticValueName,
    [string]$Kind,
    [string]$SemanticPathCandidate,
    [string]$OwnerAddress,
    [int]$FieldOffset)
{
    return [ordered]@{
        node = $Node
        semanticValueName = $SemanticValueName
        kind = $Kind
        semanticPathCandidate = $SemanticPathCandidate
        ownerAddress = $OwnerAddress
        fieldOffset = $FieldOffset
        pairs = 0
        exactNumericMatches = 0
        minAbsDelta = $null
        maxAbsDelta = $null
        setterMin = $null
        setterMax = $null
        ownerFieldMin = $null
        ownerFieldMax = $null
        firstFrame = $null
        lastFrame = $null
    }
}

function Add-OwnerSetterCandidateNumericRelationGroup($Groups, [string]$Detail, $EventObject) {
    $setterValueText = Get-DetailToken $Detail "value"
    if ($setterValueText -notmatch '^[0-9]+$') {
        return
    }

    $setterValue = 0
    if (-not [int]::TryParse($setterValueText, [ref]$setterValue)) {
        return
    }

    $node = Get-DetailToken $Detail "node"
    $semanticValueName = Get-DetailToken $Detail "semanticValueName"
    $kind = Get-DetailToken $Detail "kind"
    $semanticPathCandidate = Get-DetailToken $Detail "semanticPathCandidate"
    $ownerAddress = Get-DetailToken $Detail "ownerAddress"

    if (
        [string]::IsNullOrWhiteSpace($node) -or
        [string]::IsNullOrWhiteSpace($semanticValueName) -or
        [string]::IsNullOrWhiteSpace($kind) -or
        [string]::IsNullOrWhiteSpace($semanticPathCandidate) -or
        [string]::IsNullOrWhiteSpace($ownerAddress))
    {
        return
    }

    $frame = Get-EventFrame $EventObject
    foreach ($offset in @(460, 464, 468, 472, 480)) {
        $ownerValueText = Get-DetailToken $Detail ("ownerField{0}" -f $offset)
        if ($ownerValueText -notmatch '^-?[0-9]+$') {
            continue
        }

        $ownerValue = 0
        if (-not [int]::TryParse($ownerValueText, [ref]$ownerValue)) {
            continue
        }

        $key = "{0}:{1}:{2}:{3}:{4}:{5}" -f $node, $semanticValueName, $kind, $semanticPathCandidate, $ownerAddress, $offset
        if (-not $Groups.ContainsKey($key)) {
            $Groups[$key] = New-OwnerSetterCandidateNumericRelationGroup `
                $node `
                $semanticValueName `
                $kind `
                $semanticPathCandidate `
                $ownerAddress `
                $offset
        }

        $group = $Groups[$key]
        $group.pairs++
        if ($setterValue -eq $ownerValue) {
            $group.exactNumericMatches++
        }

        $absDelta = [Math]::Abs($setterValue - $ownerValue)
        if ($null -eq $group.minAbsDelta -or $absDelta -lt $group.minAbsDelta) {
            $group.minAbsDelta = $absDelta
        }
        if ($null -eq $group.maxAbsDelta -or $absDelta -gt $group.maxAbsDelta) {
            $group.maxAbsDelta = $absDelta
        }
        if ($null -eq $group.setterMin -or $setterValue -lt $group.setterMin) {
            $group.setterMin = $setterValue
        }
        if ($null -eq $group.setterMax -or $setterValue -gt $group.setterMax) {
            $group.setterMax = $setterValue
        }
        if ($null -eq $group.ownerFieldMin -or $ownerValue -lt $group.ownerFieldMin) {
            $group.ownerFieldMin = $ownerValue
        }
        if ($null -eq $group.ownerFieldMax -or $ownerValue -gt $group.ownerFieldMax) {
            $group.ownerFieldMax = $ownerValue
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

function New-CtGameplayWriterGroup([string]$ValueName, [string]$Callsite) {
    return [ordered]@{
        valueName = $ValueName
        callsite = $Callsite
        writes = 0
        minValue = $null
        maxValue = $null
        minFloat = $null
        maxFloat = $null
        minDelta = $null
        maxDelta = $null
        firstFrame = $null
        lastFrame = $null
    }
}

function Add-CtGameplayWriterGroup($Groups, [string]$Detail, $EventObject) {
    $valueName = Get-DetailToken $Detail "valueName"
    $callsite = Get-DetailToken $Detail "callsite"
    if ([string]::IsNullOrWhiteSpace($valueName) -or [string]::IsNullOrWhiteSpace($callsite)) {
        return
    }

    $key = "{0}:{1}" -f $valueName, $callsite
    if (-not $Groups.ContainsKey($key)) {
        $Groups[$key] = New-CtGameplayWriterGroup $valueName $callsite
    }

    $group = $Groups[$key]
    $group.writes++

    $value = 0L
    if ([Int64]::TryParse((Get-DetailToken $Detail "value"), [ref]$value)) {
        if ($null -eq $group.minValue -or $value -lt $group.minValue) {
            $group.minValue = $value
        }
        if ($null -eq $group.maxValue -or $value -gt $group.maxValue) {
            $group.maxValue = $value
        }
    }

    $delta = 0
    if ([int]::TryParse((Get-DetailToken $Detail "delta"), [ref]$delta)) {
        if ($null -eq $group.minDelta -or $delta -lt $group.minDelta) {
            $group.minDelta = $delta
        }
        if ($null -eq $group.maxDelta -or $delta -gt $group.maxDelta) {
            $group.maxDelta = $delta
        }
    }

    $floatText = Get-DetailToken $Detail "valueFloat"
    $floatValue = 0.0
    if (-not [string]::IsNullOrWhiteSpace($floatText) -and [double]::TryParse($floatText, [ref]$floatValue)) {
        if ($null -eq $group.minFloat -or $floatValue -lt $group.minFloat) {
            $group.minFloat = $floatValue
        }
        if ($null -eq $group.maxFloat -or $floatValue -gt $group.maxFloat) {
            $group.maxFloat = $floatValue
        }
    }

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

function New-CtGameplayWriterRecord([string]$Detail, $EventObject) {
    return [ordered]@{
        valueName = Get-DetailToken $Detail "valueName"
        callsite = Get-DetailToken $Detail "callsite"
        frame = Get-EventFrame $EventObject
    }
}

function New-OwnerSetterCandidateRecord([string]$Detail, $EventObject) {
    return [ordered]@{
        valueName = Get-DetailToken $Detail "semanticValueName"
        node = Get-DetailToken $Detail "node"
        kind = Get-DetailToken $Detail "kind"
        path = Get-DetailToken $Detail "semanticPathCandidate"
        frame = Get-EventFrame $EventObject
    }
}

function New-CtGameplayWriterOwnerSetterCandidateCorrelationGroup(
    [string]$ValueName,
    [string]$WriterCallsite,
    [string]$SetterNode,
    [string]$SetterKind,
    [string]$Path)
{
    return [ordered]@{
        valueName = $ValueName
        writerCallsite = $WriterCallsite
        setterNode = $SetterNode
        setterKind = $SetterKind
        path = $Path
        joins = 0
        minFrameDelta = $null
        maxFrameDelta = $null
    }
}

function Add-CtGameplayWriterOwnerSetterCandidateCorrelationGroups(
    $Groups,
    [object[]]$WriterEvents,
    [object[]]$SetterCandidateEvents)
{
    foreach ($writer in @($WriterEvents)) {
        if ([string]::IsNullOrWhiteSpace($writer.valueName) -or $null -eq $writer.frame) {
            continue
        }

        foreach ($setter in @($SetterCandidateEvents)) {
            if (
                [string]::IsNullOrWhiteSpace($setter.valueName) -or
                $null -eq $setter.frame -or
                $setter.valueName -ne $writer.valueName)
            {
                continue
            }

            $frameDelta = [Math]::Abs([int]$setter.frame - [int]$writer.frame)
            if ($frameDelta -gt 60) {
                continue
            }

            $key = "{0}:{1}:{2}:{3}:{4}" -f $writer.valueName, $writer.callsite, $setter.node, $setter.kind, $setter.path
            if (-not $Groups.ContainsKey($key)) {
                $Groups[$key] = New-CtGameplayWriterOwnerSetterCandidateCorrelationGroup `
                    $writer.valueName `
                    $writer.callsite `
                    $setter.node `
                    $setter.kind `
                    $setter.path
            }

            $group = $Groups[$key]
            $group.joins++
            if ($null -eq $group.minFrameDelta -or $frameDelta -lt $group.minFrameDelta) {
                $group.minFrameDelta = $frameDelta
            }
            if ($null -eq $group.maxFrameDelta -or $frameDelta -gt $group.maxFrameDelta) {
                $group.maxFrameDelta = $frameDelta
            }
        }
    }
}

function Format-RangeValue($Min, $Max, [string]$EmptyValue = "<none>") {
    if ($null -eq $Min -or $null -eq $Max) {
        return $EmptyValue
    }

    return "{0}-{1}" -f $Min, $Max
}

function Format-OwnerSetterCandidateCorrelationFields($Group, [int]$Limit) {
    return (
        @("460", "464", "468", "472", "480") |
            ForEach-Object {
                "{0}={1}" -f $_, (Format-CandidateList $Group.ownerFieldValuesByOffset[$_] $Limit)
            }
    ) -join "|"
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

function New-OwnerFieldDrawPathBridgeGroup(
    [string]$Path,
    [string]$ValueName,
    [string]$OwnerAddress,
    [int]$FieldOffset)
{
    return [ordered]@{
        path = $Path
        semanticValueName = $ValueName
        ownerAddress = $OwnerAddress
        fieldOffset = $FieldOffset
        samples = 0
        draws = 0
        observedValues = [System.Collections.Generic.SortedSet[string]]::new()
        childPaths = [System.Collections.Generic.SortedSet[string]]::new()
        sources = [System.Collections.Generic.SortedSet[string]]::new()
    }
}

function Add-OwnerFieldDrawPathBridgeGroup($Groups, $OwnerFieldGroup, $GaugeDrawGroup) {
    # Phase 201: bridge the Phase 197 owner-field rolling-counter stream to
    # Phase 194 exact visible gauge draw paths. This is still not a formula,
    # not a setter-node join, and not a final gameplay value promotion.
    if ($OwnerFieldGroup.semanticValueName -ne $GaugeDrawGroup.semanticValueName) {
        return
    }

    if (-not (Test-SemanticPathCandidateMatchesGaugeDraw $OwnerFieldGroup.candidatePaths $GaugeDrawGroup.path)) {
        return
    }

    $key = "{0}:{1}:{2}:{3}" -f `
        $GaugeDrawGroup.path,
        $GaugeDrawGroup.semanticValueName,
        $OwnerFieldGroup.ownerAddress,
        $OwnerFieldGroup.fieldOffset
    if (-not $Groups.ContainsKey($key)) {
        $Groups[$key] = New-OwnerFieldDrawPathBridgeGroup `
            $GaugeDrawGroup.path `
            $GaugeDrawGroup.semanticValueName `
            $OwnerFieldGroup.ownerAddress `
            $OwnerFieldGroup.fieldOffset
    }

    $group = $Groups[$key]
    $group.samples += $OwnerFieldGroup.samples
    $group.draws += $GaugeDrawGroup.draws
    foreach ($value in @($OwnerFieldGroup.observedValues)) {
        Add-UniqueSorted $group.observedValues $value
    }
    foreach ($childPath in @($GaugeDrawGroup.childPaths)) {
        Add-UniqueSorted $group.childPaths $childPath
    }
    foreach ($source in @($OwnerFieldGroup.sources)) {
        Add-UniqueSorted $group.sources $source
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

function New-SonicHudRuntimeProofLane(
    [string]$LaneId,
    [int]$EvidenceCount,
    [string]$RuntimeSurface,
    [string]$PendingReason)
{
    $status = "pending"
    if ($EvidenceCount -gt 0) {
        $status = "present"
    }

    return [ordered]@{
        laneId = $LaneId
        status = $status
        evidenceCount = $EvidenceCount
        runtimeSurface = $RuntimeSurface
        pendingReason = if ($status -eq "pending") { $PendingReason } else { "" }
    }
}

function Resolve-SonicHudRuntimeProofMatrixStatus($Matrix) {
    $presentLaneIds = @(
        $Matrix |
            Where-Object { $_.status -eq "present" } |
            ForEach-Object { $_.laneId }
    )

    $requiredGaugeLaneIds = @(
        "gauge-scale-write",
        "gauge-pattern-write",
        "gauge-hide-write",
        "owner-field-snapshot",
        "owner-scale-correlation"
    )

    $hasRequiredGaugeProof = $true
    foreach ($laneId in $requiredGaugeLaneIds) {
        if ($presentLaneIds -notcontains $laneId) {
            $hasRequiredGaugeProof = $false
            break
        }
    }

    $hasAnyGaugeProof = @(
        $presentLaneIds |
            Where-Object {
                $_ -like "gauge-*" -or
                $_ -like "owner-*" -or
                $_ -like "setter-*"
            }
    ).Count -gt 0
    $hasAudioProof = $presentLaneIds -contains "audio-callsite"

    if ($hasRequiredGaugeProof -and $hasAudioProof) {
        return "retail-runtime-gauge-and-audio-proof-ready"
    }
    if ($hasRequiredGaugeProof) {
        return "retail-runtime-gauge-proof-partial-audio-pending"
    }
    if ($hasAudioProof -and -not $hasAnyGaugeProof) {
        return "retail-runtime-audio-proof-partial-gauge-pending"
    }
    if ($hasAnyGaugeProof) {
        return "retail-runtime-gauge-proof-incomplete-audio-pending"
    }
    return "pending-retail-runtime-stage-hud-proof"
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
$ownerSetterCandidateCorrelationGroupsByKey = @{}
$ownerSetterCandidateNumericRelationGroupsByKey = @{}
$ctGameplayWriterGroupsByKey = @{}
$ctGameplayWriterOwnerSetterCandidateCorrelationGroupsByKey = @{}
$ctGameplayWriterRecords = New-Object System.Collections.Generic.List[object]
$ownerSetterCandidateRecords = New-Object System.Collections.Generic.List[object]
$ownerFieldOffsetClassificationsByKey = @{}
$ownerFieldOffsetTransitionTracksByKey = @{}
$gaugeDrawPathGroupsByKey = @{}
$ownerFieldDrawPathBridgeGroupsByKey = @{}
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
    ownerFieldSnapshotEvents = 0
    ownerScaleCorrelationEvents = 0
    ownerSetterCandidateCorrelationEvents = 0
    ownerSetterCandidateCorrelationGroups = @()
    ownerSetterCandidateNumericRelationGroups = @()
    ctGameplayWriterEvents = 0
    ctGameplayWriterGroups = @()
    ctGameplayWriterOwnerSetterCandidateCorrelationGroups = @()
    audioCallsiteEvents = 0
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
    ownerFieldOffsetClassifications = @()
    ownerFieldOffsetClassificationStatus = "pending-runtime-owner-field-offset-classification-evidence"
    ownerFieldOffsetTransitionDiagnostics = @()
    ownerFieldOffsetTransitionDiagnosticsStatus = "pending-runtime-owner-field-offset-transition-evidence"
    ownerFieldDrawPathBridgeGroups = @()
    ownerFieldDrawPathBridgeStatus = "pending-runtime-owner-field-draw-path-bridge-evidence"
    sonicHudRuntimeProofMatrix = @()
    sonicHudRuntimeProofMatrixStatus = "pending-retail-runtime-stage-hud-proof"
    sonicHudAudioCallsiteStatus = "audio-callsite-pending"
    sonicHudOwnerSetterCandidateCorrelationStatus = "pending-runtime-setter-owner-candidate-correlation-evidence"
    sonicHudCtGameplayWriterStatus = "pending-ct-anchored-gameplay-writer-evidence"
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
            $summary.ownerFieldSnapshotEvents++
            Add-OwnerFieldRollingCounterGroup $ownerFieldRollingCounterGroupsByKey $detail $eventObject
            Add-OwnerFieldOffsetTransitionSample $ownerFieldOffsetTransitionTracksByKey $detail $eventObject
        }
        "sonic-hud-gauge-scale-owner-correlated" {
            $summary.ownerScaleCorrelationEvents++
            Add-OwnerFieldGaugeScaleCorrelationGroup $ownerFieldGaugeScaleCorrelationGroupsByKey $detail $eventObject
        }
        "sonic-hud-gauge-setter-owner-candidate-correlated" {
            $summary.ownerSetterCandidateCorrelationEvents++
            Add-OwnerSetterCandidateCorrelationGroup $ownerSetterCandidateCorrelationGroupsByKey $detail $eventObject
            Add-OwnerSetterCandidateNumericRelationGroup $ownerSetterCandidateNumericRelationGroupsByKey $detail $eventObject
            [void]$ownerSetterCandidateRecords.Add((New-OwnerSetterCandidateRecord $detail $eventObject))
        }
        "sonic-hud-ct-gameplay-writer" {
            $summary.ctGameplayWriterEvents++
            Add-CtGameplayWriterGroup $ctGameplayWriterGroupsByKey $detail $eventObject
            [void]$ctGameplayWriterRecords.Add((New-CtGameplayWriterRecord $detail $eventObject))
        }
        "sonic-hud-audio-cue-callsite" {
            $summary.audioCallsiteEvents++
        }
        "sonic-hud-sfx-callsite" {
            $summary.audioCallsiteEvents++
        }
        "sonic-hud-audio-id-resolved" {
            $summary.audioCallsiteEvents++
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
$summary.ownerSetterCandidateCorrelationGroups = @(
    $ownerSetterCandidateCorrelationGroupsByKey.Keys |
        Sort-Object `
            @{ Expression = { -1 * $ownerSetterCandidateCorrelationGroupsByKey[$_].joins } }, `
            @{ Expression = { $ownerSetterCandidateCorrelationGroupsByKey[$_].node } }, `
            @{ Expression = { $ownerSetterCandidateCorrelationGroupsByKey[$_].semanticValueName } }, `
            @{ Expression = { $ownerSetterCandidateCorrelationGroupsByKey[$_].semanticPathCandidate } } |
        ForEach-Object {
            $group = $ownerSetterCandidateCorrelationGroupsByKey[$_]
            [ordered]@{
                node = $group.node
                semanticValueName = $group.semanticValueName
                kind = $group.kind
                semanticPathCandidate = $group.semanticPathCandidate
                ownerAddress = $group.ownerAddress
                joins = $group.joins
                observedSetterValues = @($group.observedSetterValues)
                ownerFieldValuesByOffset = [ordered]@{
                    "460" = @($group.ownerFieldValuesByOffset["460"])
                    "464" = @($group.ownerFieldValuesByOffset["464"])
                    "468" = @($group.ownerFieldValuesByOffset["468"])
                    "472" = @($group.ownerFieldValuesByOffset["472"])
                    "480" = @($group.ownerFieldValuesByOffset["480"])
                }
                frameDeltas = @($group.frameDeltas)
                callsites = @($group.callsites)
                sources = @($group.sources)
                firstFrame = $group.firstFrame
                lastFrame = $group.lastFrame
            }
        }
)
$summary.ownerSetterCandidateNumericRelationGroups = @(
    $ownerSetterCandidateNumericRelationGroupsByKey.Keys |
        Sort-Object `
            @{ Expression = { -1 * $ownerSetterCandidateNumericRelationGroupsByKey[$_].exactNumericMatches } }, `
            @{ Expression = { $ownerSetterCandidateNumericRelationGroupsByKey[$_].minAbsDelta } }, `
            @{ Expression = { -1 * $ownerSetterCandidateNumericRelationGroupsByKey[$_].pairs } }, `
            @{ Expression = { $ownerSetterCandidateNumericRelationGroupsByKey[$_].node } }, `
            @{ Expression = { $ownerSetterCandidateNumericRelationGroupsByKey[$_].fieldOffset } } |
        ForEach-Object {
            $group = $ownerSetterCandidateNumericRelationGroupsByKey[$_]
            [ordered]@{
                node = $group.node
                semanticValueName = $group.semanticValueName
                kind = $group.kind
                semanticPathCandidate = $group.semanticPathCandidate
                ownerAddress = $group.ownerAddress
                fieldOffset = $group.fieldOffset
                pairs = $group.pairs
                exactNumericMatches = $group.exactNumericMatches
                minAbsDelta = $group.minAbsDelta
                maxAbsDelta = $group.maxAbsDelta
                setterMin = $group.setterMin
                setterMax = $group.setterMax
                ownerFieldMin = $group.ownerFieldMin
                ownerFieldMax = $group.ownerFieldMax
                firstFrame = $group.firstFrame
                lastFrame = $group.lastFrame
            }
        }
)
Add-CtGameplayWriterOwnerSetterCandidateCorrelationGroups `
    $ctGameplayWriterOwnerSetterCandidateCorrelationGroupsByKey `
    ([object[]]$ctGameplayWriterRecords.ToArray()) `
    ([object[]]$ownerSetterCandidateRecords.ToArray())
$summary.ctGameplayWriterGroups = @(
    $ctGameplayWriterGroupsByKey.Keys |
        Sort-Object `
            @{ Expression = { $ctGameplayWriterGroupsByKey[$_].valueName } }, `
            @{ Expression = { $ctGameplayWriterGroupsByKey[$_].callsite } } |
        ForEach-Object {
            $group = $ctGameplayWriterGroupsByKey[$_]
            [ordered]@{
                valueName = $group.valueName
                callsite = $group.callsite
                writes = $group.writes
                minValue = $group.minValue
                maxValue = $group.maxValue
                minFloat = $group.minFloat
                maxFloat = $group.maxFloat
                minDelta = $group.minDelta
                maxDelta = $group.maxDelta
                firstFrame = $group.firstFrame
                lastFrame = $group.lastFrame
            }
        }
)
$summary.ctGameplayWriterOwnerSetterCandidateCorrelationGroups = @(
    $ctGameplayWriterOwnerSetterCandidateCorrelationGroupsByKey.Keys |
        Sort-Object `
            @{ Expression = { -1 * $ctGameplayWriterOwnerSetterCandidateCorrelationGroupsByKey[$_].joins } }, `
            @{ Expression = { $ctGameplayWriterOwnerSetterCandidateCorrelationGroupsByKey[$_].valueName } }, `
            @{ Expression = { $ctGameplayWriterOwnerSetterCandidateCorrelationGroupsByKey[$_].writerCallsite } } |
        ForEach-Object {
            $group = $ctGameplayWriterOwnerSetterCandidateCorrelationGroupsByKey[$_]
            [ordered]@{
                valueName = $group.valueName
                writerCallsite = $group.writerCallsite
                setterNode = $group.setterNode
                setterKind = $group.setterKind
                path = $group.path
                joins = $group.joins
                minFrameDelta = $group.minFrameDelta
                maxFrameDelta = $group.maxFrameDelta
            }
        }
)
# Phase 199: aggregate Phase 197 snapshot evidence + Phase 198 SetScale-join
# evidence into a per-(owner,offset) shape classification entry. Pure
# post-processing — no new event source — so the classifier can never
# disagree with what the runtime already emitted.
foreach ($group in $ownerFieldRollingCounterGroupsByKey.Values) {
    Add-OwnerFieldOffsetClassification `
        $ownerFieldOffsetClassificationsByKey `
        $group.ownerAddress `
        $group.fieldOffset `
        $group.observedValues `
        0 `
        $group.samples
}
foreach ($group in $ownerFieldGaugeScaleCorrelationGroupsByKey.Values) {
    Add-OwnerFieldOffsetClassification `
        $ownerFieldOffsetClassificationsByKey `
        $group.ownerAddress `
        $group.fieldOffset `
        $group.observedFieldValues `
        $group.joins `
        0
}
foreach ($entry in $ownerFieldOffsetClassificationsByKey.Values) {
    $entry.observedCardinality = $entry.observedValueSet.Count
    if ($entry.observedCardinality -gt 0) {
        $entry.observedMin = $entry.observedValueSet.Min
        $entry.observedMax = $entry.observedValueSet.Max
    }
    $entry.candidateLabel = Resolve-OwnerFieldOffsetCandidateLabel `
        $entry.observedCardinality `
        $entry.observedMin `
        $entry.observedMax
}
$summary.ownerFieldOffsetClassifications = @(
    $ownerFieldOffsetClassificationsByKey.Keys |
        Sort-Object `
            @{ Expression = { $ownerFieldOffsetClassificationsByKey[$_].ownerAddress } }, `
            @{ Expression = { $ownerFieldOffsetClassificationsByKey[$_].fieldOffset } } |
        ForEach-Object {
            $entry = $ownerFieldOffsetClassificationsByKey[$_]
            [ordered]@{
                ownerAddress = $entry.ownerAddress
                fieldOffset = $entry.fieldOffset
                observedCardinality = $entry.observedCardinality
                observedMin = $entry.observedMin
                observedMax = $entry.observedMax
                joinCount = $entry.joinCount
                snapshotCount = $entry.snapshotCount
                candidateLabel = $entry.candidateLabel
            }
        }
)
# Phase 200: walk each (owner, offset) sample track in chronological order
# and compute consecutive-pair transition diagnostics. Pure post-processing
# of the Phase 197 snapshot stream — no scale evidence, no controller value
# promotion. The trend label is descriptive; it does not assert any per-frame
# meaning of the value sequence.
$summary.ownerFieldOffsetTransitionDiagnostics = @(
    $ownerFieldOffsetTransitionTracksByKey.Keys |
        Sort-Object `
            @{ Expression = { $ownerFieldOffsetTransitionTracksByKey[$_].ownerAddress } }, `
            @{ Expression = { $ownerFieldOffsetTransitionTracksByKey[$_].fieldOffset } } |
        ForEach-Object {
            $track = $ownerFieldOffsetTransitionTracksByKey[$_]
            $orderedSamples = @(
                $track.samples |
                    Sort-Object @{ Expression = {
                        if ($null -ne $_.frame) { [int]$_.frame } else { [int]::MaxValue }
                    }}
            )
            $incCount = 0
            $decCount = 0
            $eqCount = 0
            $minDelta = 0
            $maxDelta = 0
            $deltaInitialized = $false
            for ($i = 1; $i -lt $orderedSamples.Count; $i++) {
                $delta = [int]$orderedSamples[$i].value - [int]$orderedSamples[$i - 1].value
                $abs = [Math]::Abs($delta)
                if (-not $deltaInitialized) {
                    $minDelta = $abs
                    $maxDelta = $abs
                    $deltaInitialized = $true
                }
                else {
                    if ($abs -lt $minDelta) { $minDelta = $abs }
                    if ($abs -gt $maxDelta) { $maxDelta = $abs }
                }
                if ($delta -gt 0) { $incCount++ }
                elseif ($delta -lt 0) { $decCount++ }
                else { $eqCount++ }
            }
            $transitions = $incCount + $decCount + $eqCount
            $trend = Resolve-OwnerFieldOffsetMonotonicTrendLabel $incCount $decCount $eqCount
            [ordered]@{
                ownerAddress = $track.ownerAddress
                fieldOffset = $track.fieldOffset
                snapshotCount = $orderedSamples.Count
                transitionCount = $transitions
                increasingTransitions = $incCount
                decreasingTransitions = $decCount
                equalTransitions = $eqCount
                minTransitionMagnitude = $minDelta
                maxTransitionMagnitude = $maxDelta
                wrapDetected = ($decCount -gt 0)
                monotonicTrend = $trend
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
foreach ($ownerFieldGroup in $summary.ownerFieldRollingCounterGroups) {
    foreach ($gaugeDrawGroup in $summary.gaugeDrawPathGroups) {
        Add-OwnerFieldDrawPathBridgeGroup `
            $ownerFieldDrawPathBridgeGroupsByKey `
            $ownerFieldGroup `
            $gaugeDrawGroup
    }
}
$summary.ownerFieldDrawPathBridgeGroups = @(
    $ownerFieldDrawPathBridgeGroupsByKey.Keys |
        Sort-Object `
            @{ Expression = { $ownerFieldDrawPathBridgeGroupsByKey[$_].semanticValueName } }, `
            @{ Expression = { $ownerFieldDrawPathBridgeGroupsByKey[$_].path } }, `
            @{ Expression = { $ownerFieldDrawPathBridgeGroupsByKey[$_].ownerAddress } }, `
            @{ Expression = { $ownerFieldDrawPathBridgeGroupsByKey[$_].fieldOffset } } |
        ForEach-Object {
            $group = $ownerFieldDrawPathBridgeGroupsByKey[$_]
            [ordered]@{
                path = $group.path
                semanticValueName = $group.semanticValueName
                ownerAddress = $group.ownerAddress
                fieldOffset = $group.fieldOffset
                samples = $group.samples
                draws = $group.draws
                observedValues = @($group.observedValues)
                childPaths = @($group.childPaths)
                sources = @($group.sources)
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

if ($summary.ownerFieldOffsetClassifications.Count -gt 0) {
    $summary.ownerFieldOffsetClassificationStatus = "owner-field-offset-classification-pending-formula-proof"
}

if ($summary.ownerFieldOffsetTransitionDiagnostics.Count -gt 0) {
    $summary.ownerFieldOffsetTransitionDiagnosticsStatus = "owner-field-offset-transition-diagnostics-pending-formula-proof"
}

if ($summary.ownerFieldDrawPathBridgeGroups.Count -gt 0) {
    $summary.ownerFieldDrawPathBridgeStatus = "owner-field-draw-path-bridge-pending-formula-proof"
}

if ($summary.ownerSetterCandidateCorrelationEvents -gt 0) {
    $summary.sonicHudOwnerSetterCandidateCorrelationStatus =
        "retail-runtime-setter-owner-candidate-correlation-pending-exact-child-path"
}

if ($summary.ctGameplayWriterEvents -gt 0) {
    $summary.sonicHudCtGameplayWriterStatus =
        "ct-anchored-gameplay-writer-evidence-present-pending-final-hud-formula"
}

if ($summary.audioCallsiteEvents -gt 0) {
    $summary.sonicHudAudioCallsiteStatus = "retail-runtime-audio-callsite-evidence-found"
}

$summary.sonicHudRuntimeProofMatrix = @(
    New-SonicHudRuntimeProofLane `
        "gauge-scale-write" `
        $summary.gaugeScaleWrites `
        "CSD::CNode::SetScale/sub_830BF090" `
        "waiting for same-frame retail Sonic Day HUD SetScale evidence"
    New-SonicHudRuntimeProofLane `
        "gauge-pattern-write" `
        $summary.gaugePatternWrites `
        "CSD::CNode::SetPatternIndex/sub_830BF300" `
        "waiting for retail Sonic Day HUD SetPatternIndex evidence"
    New-SonicHudRuntimeProofLane `
        "gauge-hide-write" `
        $summary.gaugeHideWrites `
        "CSD::CNode::SetHideFlag/sub_830BF080" `
        "waiting for retail Sonic Day HUD SetHideFlag evidence"
    New-SonicHudRuntimeProofLane `
        "owner-field-snapshot" `
        $summary.ownerFieldSnapshotEvents `
        "CHudSonicStage/sub_824D6C18 owner fields +460/+464/+468/+472/+480" `
        "waiting for retail Sonic Day HUD owner-field snapshots"
    New-SonicHudRuntimeProofLane `
        "owner-scale-correlation" `
        $summary.ownerScaleCorrelationEvents `
        "same-frame owner-field snapshot plus CSD::CNode::SetScale join" `
        "waiting for owner-field to gauge SetScale correlation"
    New-SonicHudRuntimeProofLane `
        "setter-child-path-join" `
        $summary.gaugeSetterChildPathJoins.Count `
        "runtime ui-draw-list cast/layer address join" `
        "waiting for setter node to exact gauge child path join"
    New-SonicHudRuntimeProofLane `
        "owner-setter-candidate-correlation" `
        $summary.ownerSetterCandidateCorrelationEvents `
        "unresolved CSD setter node plus owner-field cache candidate join" `
        "waiting for unresolved setter owner-candidate correlation"
    New-SonicHudRuntimeProofLane `
        "ct-gameplay-writer" `
        $summary.ctGameplayWriterEvents `
        "CT-anchored rings/lives/Day boost gameplay writer hooks" `
        "waiting for CT-anchored gameplay writer evidence"
    New-SonicHudRuntimeProofLane `
        "audio-callsite" `
        $summary.audioCallsiteEvents `
        "retail Sonic HUD SFX/audio callsite hook" `
        "exact Sonic HUD SFX/audio IDs pending"
)
$summary.sonicHudRuntimeProofMatrixStatus = Resolve-SonicHudRuntimeProofMatrixStatus $summary.sonicHudRuntimeProofMatrix

if (
    $summary.textWrites -gt 0 -or
    $summary.gaugeWrites -gt 0 -or
    $summary.lateResolvedWrites -gt 0 -or
    $summary.gameplayUpdates -gt 0 -or
    $summary.gameplayValueSnapshots -gt 0 -or
    $summary.callsiteClassifications -gt 0 -or
    $summary.ownerSetterCandidateCorrelationEvents -gt 0 -or
    $summary.ctGameplayWriterEvents -gt 0 -or
    $summary.audioCallsiteEvents -gt 0 -or
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
Write-Output ("owner_setter_candidate_correlations={0}" -f $summary.ownerSetterCandidateCorrelationEvents)
Write-Output (
    "owner_setter_candidate_correlation_groups={0}" -f
    (Format-CandidateList (
        $summary.ownerSetterCandidateCorrelationGroups |
            ForEach-Object {
                "node={0}:{1}:{2}:{3}:owner={4}:joins={5}:values={6}:fields={7}:frames={8}-{9}:frame_deltas={10}" -f
                    $_.node,
                    $_.semanticValueName,
                    $_.kind,
                    $_.semanticPathCandidate,
                    $_.ownerAddress,
                    $_.joins,
                    (Format-CandidateList $_.observedSetterValues $CandidateValueLimit),
                    (Format-OwnerSetterCandidateCorrelationFields $_ $CandidateValueLimit),
                    $_.firstFrame,
                    $_.lastFrame,
                    (Format-CandidateList $_.frameDeltas $CandidateValueLimit)
            }
    ) $CandidateValueLimit))
Write-Output (
    "owner_setter_candidate_numeric_relation_groups={0}" -f
    (Format-CandidateList (
        $summary.ownerSetterCandidateNumericRelationGroups |
            ForEach-Object {
                "node={0}:{1}:{2}:{3}:owner={4}:field+{5}:pairs={6}:matches={7}:min_delta={8}:max_delta={9}:setter={10}-{11}:owner_field={12}-{13}:frames={14}-{15}" -f
                    $_.node,
                    $_.semanticValueName,
                    $_.kind,
                    $_.semanticPathCandidate,
                    $_.ownerAddress,
                    $_.fieldOffset,
                    $_.pairs,
                    $_.exactNumericMatches,
                    $_.minAbsDelta,
                    $_.maxAbsDelta,
                    $_.setterMin,
                    $_.setterMax,
                    $_.ownerFieldMin,
                    $_.ownerFieldMax,
                    $_.firstFrame,
                    $_.lastFrame
            }
    ) $CandidateValueLimit))
Write-Output ("ct_gameplay_writer_events={0}" -f $summary.ctGameplayWriterEvents)
Write-Output (
    "ct_gameplay_writer_groups={0}" -f
    (Format-CandidateList (
        $summary.ctGameplayWriterGroups |
            ForEach-Object {
                "{0}:{1}:writes={2}:value={3}:float={4}:delta={5}:frames={6}-{7}" -f
                    $_.valueName,
                    $_.callsite,
                    $_.writes,
                    (Format-RangeValue $_.minValue $_.maxValue),
                    (Format-RangeValue $_.minFloat $_.maxFloat),
                    (Format-RangeValue $_.minDelta $_.maxDelta),
                    $_.firstFrame,
                    $_.lastFrame
            }
    ) $CandidateValueLimit))
Write-Output (
    "ct_gameplay_writer_owner_setter_candidate_correlation_groups={0}" -f
    (Format-CandidateList (
        $summary.ctGameplayWriterOwnerSetterCandidateCorrelationGroups |
            ForEach-Object {
                "value={0}:writer={1}:setterNode={2}:setterKind={3}:path={4}:joins={5}:frame_delta={6}-{7}" -f
                    $_.valueName,
                    $_.writerCallsite,
                    $_.setterNode,
                    $_.setterKind,
                    $_.path,
                    $_.joins,
                    $_.minFrameDelta,
                    $_.maxFrameDelta
            }
    ) $CandidateValueLimit))
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
    "sonic_hud_runtime_proof_matrix={0}" -f
    (Format-CandidateList (
        $summary.sonicHudRuntimeProofMatrix |
            ForEach-Object {
                "{0}:{1}:{2}" -f $_.laneId, $_.status, $_.evidenceCount
            }
    ) $CandidateValueLimit))
Write-Output ("sonic_hud_runtime_proof_matrix_status={0}" -f $summary.sonicHudRuntimeProofMatrixStatus)
Write-Output ("sonic_hud_audio_callsite_status={0}" -f $summary.sonicHudAudioCallsiteStatus)
Write-Output ("sonic_hud_owner_setter_candidate_correlation_status={0}" -f $summary.sonicHudOwnerSetterCandidateCorrelationStatus)
Write-Output ("sonic_hud_ct_gameplay_writer_status={0}" -f $summary.sonicHudCtGameplayWriterStatus)
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
    "owner_field_draw_path_bridge_groups={0}" -f
    (Format-CandidateList (
        $summary.ownerFieldDrawPathBridgeGroups |
            ForEach-Object {
                "path={0}:value={1}:owner={2}:field+{3}:samples={4}:draws={5}" -f
                    $_.path,
                    $_.semanticValueName,
                    $_.ownerAddress,
                    $_.fieldOffset,
                    $_.samples,
                    $_.draws
            }
    ) $CandidateValueLimit))
Write-Output ("owner_field_draw_path_bridge_status={0}" -f $summary.ownerFieldDrawPathBridgeStatus)
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
Write-Output (
    "owner_field_offset_classifications={0}" -f
    (Format-CandidateList (
        $summary.ownerFieldOffsetClassifications |
            ForEach-Object {
                "owner={0}:field+{1}:cardinality={2}:min={3}:max={4}:joins={5}:candidate={6}" -f
                    $_.ownerAddress,
                    $_.fieldOffset,
                    $_.observedCardinality,
                    $_.observedMin,
                    $_.observedMax,
                    $_.joinCount,
                    $_.candidateLabel
            }
    ) $CandidateValueLimit))
Write-Output ("owner_field_offset_classification_status={0}" -f $summary.ownerFieldOffsetClassificationStatus)
Write-Output (
    "owner_field_offset_transition_diagnostics={0}" -f
    (Format-CandidateList (
        $summary.ownerFieldOffsetTransitionDiagnostics |
            ForEach-Object {
                "owner={0}:field+{1}:trend={2}:wrap={3}:transitions={4}:inc={5}:dec={6}:eq={7}:min_delta={8}:max_delta={9}" -f
                    $_.ownerAddress,
                    $_.fieldOffset,
                    $_.monotonicTrend,
                    ($_.wrapDetected.ToString().ToLower()),
                    $_.transitionCount,
                    $_.increasingTransitions,
                    $_.decreasingTransitions,
                    $_.equalTransitions,
                    $_.minTransitionMagnitude,
                    $_.maxTransitionMagnitude
            }
    ) $CandidateValueLimit))
Write-Output ("owner_field_offset_transition_diagnostics_status={0}" -f $summary.ownerFieldOffsetTransitionDiagnosticsStatus)
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
