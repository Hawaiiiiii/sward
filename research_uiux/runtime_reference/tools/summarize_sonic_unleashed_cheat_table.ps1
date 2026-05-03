param(
    [Parameter(Mandatory = $true)]
    [string]$CheatTablePath,

    [string]$RuntimeExePath = "",

    [string]$OutputPath = "",

    [string]$SymbolizerPath = "",

    [int]$MaxSymbolHits = 128,

    [switch]$Json
)

$ErrorActionPreference = "Stop"

# local CT evidence lane: parse a local Cheat Engine table into repo-safe
# gameplay value anchors without committing the CT or publishing proprietary data.

function Format-HexInt {
    param([AllowNull()][object]$Value)

    if ($null -eq $Value) {
        return ""
    }

    return ("0x{0:X}" -f ([int64]$Value))
}

function Get-XmlChildText {
    param(
        [Parameter(Mandatory = $true)]$Node,
        [Parameter(Mandatory = $true)][string]$XPath
    )

    $child = $Node.SelectSingleNode($XPath)
    if ($null -eq $child -or $null -eq $child.InnerText) {
        return ""
    }

    return $child.InnerText.Trim()
}

function Normalize-CheatDescription {
    param([string]$Text)

    $trimmed = $Text.Trim()
    if ([string]::IsNullOrWhiteSpace($trimmed)) {
        return "(unnamed)"
    }

    return $trimmed.Trim([char]'"')
}

function Convert-AobPatternToBytes {
    param([Parameter(Mandatory = $true)][string]$PatternText)

    $tokens = @($PatternText -split '\s+' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    $bytes = New-Object System.Collections.Generic.List[int]

    foreach ($token in $tokens) {
        if ($token -eq "?" -or $token -eq "??") {
            $bytes.Add(-1)
            continue
        }

        if ($token -notmatch '^[0-9A-Fa-f]{2}$') {
            throw "Unsupported AOB token '$token' in pattern '$PatternText'"
        }

        $bytes.Add([Convert]::ToInt32($token, 16))
    }

    return [int[]]$bytes.ToArray()
}

function Convert-AobBytesToText {
    param([int[]]$PatternBytes)

    return (@(
        foreach ($value in $PatternBytes) {
            if ($value -lt 0) {
                "??"
            } else {
                "{0:X2}" -f $value
            }
        }
    ) -join " ")
}

function Get-UniqueRegexCaptures {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$GroupName
    )

    $values = New-Object System.Collections.Generic.List[string]
    foreach ($match in [regex]::Matches($Text, $Pattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)) {
        $value = $match.Groups[$GroupName].Value.Trim()
        if (![string]::IsNullOrWhiteSpace($value) -and -not $values.Contains($value)) {
            $values.Add($value)
        }
    }

    return [string[]]$values.ToArray()
}

function Get-AutoAssemblerConstants {
    param([string]$ScriptText)

    $constants = New-Object System.Collections.Generic.List[string]
    foreach ($match in [regex]::Matches($ScriptText, '\((?:int|float|double)\)\s*(?<value>-?\d+(?:\.\d+)?)', [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)) {
        $value = $match.Groups["value"].Value.Trim()
        if (![string]::IsNullOrWhiteSpace($value) -and -not $constants.Contains($value)) {
            $constants.Add($value)
        }
    }

    return [string[]]$constants.ToArray()
}

function Get-CodeEntryByteValues {
    param(
        [Parameter(Mandatory = $true)]$Node,
        [Parameter(Mandatory = $true)][string]$XPath
    )

    $bytes = New-Object System.Collections.Generic.List[int]
    foreach ($byteNode in @($Node.SelectNodes($XPath))) {
        if ($null -eq $byteNode -or $null -eq $byteNode.InnerText) {
            continue
        }

        $text = $byteNode.InnerText.Trim()
        if ([string]::IsNullOrWhiteSpace($text)) {
            continue
        }

        if ($text -notmatch '^[0-9A-Fa-f]{2}$') {
            throw "Unsupported Cheat Engine CodeEntry byte '$text'"
        }

        $bytes.Add([Convert]::ToInt32($text, 16))
    }

    return [int[]]$bytes.ToArray()
}

function Get-ModuleNameFromAddressString {
    param([string]$AddressString)

    if ($AddressString -match '^(?<module>[^+]+)\+[0-9A-Fa-f]+$') {
        return $Matches["module"].Trim()
    }

    return ""
}

function Get-RvaFromInjectionPoint {
    param([string]$InjectionPoint)

    if ($InjectionPoint -match '\+(?<hex>[0-9A-Fa-f]+)$') {
        return [int64]([Convert]::ToInt64($Matches["hex"], 16))
    }

    return $null
}

function Add-CheatEntryAnchors {
    param(
        [Parameter(Mandatory = $true)]$Nodes,
        [string[]]$ParentPath = @(),
        [Parameter(Mandatory = $true)]$Anchors
    )

    foreach ($node in @($Nodes)) {
        if ($null -eq $node) {
            continue
        }

        $id = Get-XmlChildText -Node $node -XPath "./ID"
        $description = Normalize-CheatDescription (Get-XmlChildText -Node $node -XPath "./Description")
        $pathParts = @($ParentPath + @($description))
        $path = ($pathParts | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }) -join "/"
        $scriptText = Get-XmlChildText -Node $node -XPath "./AutoAssemblerScript"
        if ([string]::IsNullOrWhiteSpace($scriptText)) {
            $scriptText = Get-XmlChildText -Node $node -XPath "./AssemblerScript"
        }

        if (![string]::IsNullOrWhiteSpace($scriptText)) {
            $aobMatches = [regex]::Matches(
                $scriptText,
                'aobscanmodule\(\s*(?<label>[^,\r\n\)]+)\s*,\s*(?<module>[^,\r\n\)]+)\s*,\s*(?<aob>[0-9A-Fa-f\?\s]+?)\s*\)',
                [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
            )

            foreach ($match in $aobMatches) {
                $patternBytes = Convert-AobPatternToBytes -PatternText $match.Groups["aob"].Value
                $referencedInjectionPoints = [string[]]@(Get-UniqueRegexCaptures `
                    -Text $scriptText `
                    -Pattern '(?<point>UnleashedRecomp\.exe\+[0-9A-Fa-f]+)' `
                    -GroupName "point")
                $oldInjectionPoints = [string[]]@(Get-UniqueRegexCaptures `
                    -Text $scriptText `
                    -Pattern 'INJECTION POINT:\s*(?<point>UnleashedRecomp\.exe\+[0-9A-Fa-f]+)' `
                    -GroupName "point")
                if ($oldInjectionPoints.Count -eq 0 -and $referencedInjectionPoints.Count -gt 0) {
                    $oldInjectionPoints = [string[]]@($referencedInjectionPoints[0])
                }
                $scriptConstants = [string[]]@(Get-AutoAssemblerConstants -ScriptText $scriptText)

                $Anchors.Add([ordered]@{
                    entryKind = "auto-assembler-aob"
                    id = $id
                    path = $path
                    description = $description
                    aobLabel = $match.Groups["label"].Value.Trim()
                    module = $match.Groups["module"].Value.Trim()
                    aobPattern = Convert-AobBytesToText -PatternBytes $patternBytes
                    aobByteCount = $patternBytes.Count
                    aobPatternBytes = $patternBytes
                    oldInjectionPoints = $oldInjectionPoints
                    referencedInjectionPoints = $referencedInjectionPoints
                    scriptConstants = $scriptConstants
                    runtimeHitFileOffsetDelta = 0
                    oldInjectionPointRuntimeRvaMatches = @()
                    oldInjectionPointRuntimeRvaMatchCount = 0
                    oldInjectionPointRuntimeRvaStatus = "not-scanned"
                    runtimeHitCount = 0
                    runtimeHits = @()
                }) | Out-Null
            }
        }

        $children = @($node.SelectNodes("./CheatEntries/CheatEntry"))
        if ($children.Count -gt 0) {
            Add-CheatEntryAnchors -Nodes $children -ParentPath $pathParts -Anchors $Anchors
        }
    }
}

function Add-CodeEntryAnchors {
    param(
        [Parameter(Mandatory = $true)]$Nodes,
        [Parameter(Mandatory = $true)]$Anchors
    )

    foreach ($node in @($Nodes)) {
        if ($null -eq $node) {
            continue
        }

        $description = Normalize-CheatDescription (Get-XmlChildText -Node $node -XPath "./Description")
        $addressString = Get-XmlChildText -Node $node -XPath "./AddressString"
        $beforeBytes = [int[]]@(Get-CodeEntryByteValues -Node $node -XPath "./Before/Byte")
        $actualBytes = [int[]]@(Get-CodeEntryByteValues -Node $node -XPath "./Actual/Byte")
        $afterBytes = [int[]]@(Get-CodeEntryByteValues -Node $node -XPath "./After/Byte")
        if ($actualBytes.Count -eq 0) {
            continue
        }

        $patternBytes = [int[]]@($beforeBytes + $actualBytes + $afterBytes)
        $oldInjectionPointList = New-Object System.Collections.Generic.List[string]
        if (![string]::IsNullOrWhiteSpace($addressString)) {
            $oldInjectionPointList.Add($addressString) | Out-Null
        }
        $oldInjectionPoints = [string[]]$oldInjectionPointList.ToArray()

        $Anchors.Add([ordered]@{
            entryKind = "ce-code-entry"
            id = ""
            path = "CheatCodes/$description"
            description = $description
            aobLabel = ""
            module = Get-ModuleNameFromAddressString -AddressString $addressString
            aobPattern = Convert-AobBytesToText -PatternBytes $patternBytes
            aobByteCount = $patternBytes.Count
            aobPatternBytes = $patternBytes
            codeEntryAddressString = $addressString
            codeEntryBeforeByteCount = $beforeBytes.Count
            codeEntryActualByteCount = $actualBytes.Count
            codeEntryAfterByteCount = $afterBytes.Count
            oldInjectionPoints = [string[]]$oldInjectionPoints
            referencedInjectionPoints = [string[]]$oldInjectionPoints
            scriptConstants = @()
            runtimeHitFileOffsetDelta = $beforeBytes.Count
            oldInjectionPointRuntimeRvaMatches = @()
            oldInjectionPointRuntimeRvaMatchCount = 0
            oldInjectionPointRuntimeRvaStatus = "not-scanned"
            runtimeHitCount = 0
            runtimeHits = @()
        }) | Out-Null
    }
}

function Read-UInt16LE {
    param([byte[]]$Bytes, [int]$Offset)
    return [BitConverter]::ToUInt16($Bytes, $Offset)
}

function Read-UInt32LE {
    param([byte[]]$Bytes, [int]$Offset)
    return [BitConverter]::ToUInt32($Bytes, $Offset)
}

function Read-UInt64LE {
    param([byte[]]$Bytes, [int]$Offset)
    return [BitConverter]::ToUInt64($Bytes, $Offset)
}

function Get-PeImageMap {
    param([byte[]]$Bytes)

    if ($Bytes.Count -lt 0x40 -or $Bytes[0] -ne 0x4D -or $Bytes[1] -ne 0x5A) {
        return [ordered]@{
            status = "not-pe-rva-unavailable"
            imageBase = $null
            sections = @()
        }
    }

    $peOffset = [int](Read-UInt32LE -Bytes $Bytes -Offset 0x3C)
    if ($peOffset -lt 0 -or ($peOffset + 0x18) -ge $Bytes.Count) {
        return [ordered]@{
            status = "invalid-pe-rva-unavailable"
            imageBase = $null
            sections = @()
        }
    }

    if ($Bytes[$peOffset] -ne 0x50 -or $Bytes[$peOffset + 1] -ne 0x45 -or $Bytes[$peOffset + 2] -ne 0 -or $Bytes[$peOffset + 3] -ne 0) {
        return [ordered]@{
            status = "invalid-pe-rva-unavailable"
            imageBase = $null
            sections = @()
        }
    }

    $numberOfSections = [int](Read-UInt16LE -Bytes $Bytes -Offset ($peOffset + 6))
    $sizeOfOptionalHeader = [int](Read-UInt16LE -Bytes $Bytes -Offset ($peOffset + 20))
    $optionalHeaderOffset = $peOffset + 24
    $magic = Read-UInt16LE -Bytes $Bytes -Offset $optionalHeaderOffset

    if ($magic -eq 0x20B) {
        $imageBase = [int64](Read-UInt64LE -Bytes $Bytes -Offset ($optionalHeaderOffset + 24))
    } elseif ($magic -eq 0x10B) {
        $imageBase = [int64](Read-UInt32LE -Bytes $Bytes -Offset ($optionalHeaderOffset + 28))
    } else {
        return [ordered]@{
            status = "unsupported-pe-rva-unavailable"
            imageBase = $null
            sections = @()
        }
    }

    $sectionTableOffset = $optionalHeaderOffset + $sizeOfOptionalHeader
    $sections = New-Object System.Collections.Generic.List[object]
    for ($index = 0; $index -lt $numberOfSections; ++$index) {
        $sectionOffset = $sectionTableOffset + ($index * 40)
        if (($sectionOffset + 39) -ge $Bytes.Count) {
            break
        }

        $nameBytes = $Bytes[$sectionOffset..($sectionOffset + 7)]
        $name = [System.Text.Encoding]::ASCII.GetString($nameBytes).TrimEnd([char]0)
        $virtualSize = [int64](Read-UInt32LE -Bytes $Bytes -Offset ($sectionOffset + 8))
        $virtualAddress = [int64](Read-UInt32LE -Bytes $Bytes -Offset ($sectionOffset + 12))
        $sizeOfRawData = [int64](Read-UInt32LE -Bytes $Bytes -Offset ($sectionOffset + 16))
        $pointerToRawData = [int64](Read-UInt32LE -Bytes $Bytes -Offset ($sectionOffset + 20))

        $sections.Add([ordered]@{
            name = $name
            virtualSize = $virtualSize
            virtualAddress = $virtualAddress
            sizeOfRawData = $sizeOfRawData
            pointerToRawData = $pointerToRawData
        }) | Out-Null
    }

    return [ordered]@{
        status = "pe-rva-available"
        imageBase = $imageBase
        sections = [object[]]$sections.ToArray()
    }
}

function Convert-FileOffsetToImageAddress {
    param(
        [Parameter(Mandatory = $true)]$ImageMap,
        [Parameter(Mandatory = $true)][int64]$FileOffset
    )

    if ($ImageMap.status -ne "pe-rva-available") {
        return [ordered]@{
            section = ""
            rva = $null
            virtualAddress = $null
        }
    }

    foreach ($section in @($ImageMap.sections)) {
        $rawStart = [int64]$section.pointerToRawData
        $rawEnd = $rawStart + [Math]::Max([int64]$section.sizeOfRawData, [int64]1)
        if ($FileOffset -ge $rawStart -and $FileOffset -lt $rawEnd) {
            $rva = [int64]$section.virtualAddress + ($FileOffset - $rawStart)
            return [ordered]@{
                section = $section.name
                rva = $rva
                virtualAddress = ([int64]$ImageMap.imageBase + $rva)
            }
        }
    }

    return [ordered]@{
        section = ""
        rva = $null
        virtualAddress = $null
    }
}

function Resolve-OldInjectionPointRuntimeRvaAlignment {
    param(
        [Parameter(Mandatory = $true)]$Entry,
        [Parameter(Mandatory = $true)][string]$PeStatus
    )

    $matches = New-Object System.Collections.Generic.List[string]
    $oldPoints = @($Entry.oldInjectionPoints)
    if ($oldPoints.Count -eq 0) {
        $Entry.oldInjectionPointRuntimeRvaMatches = @()
        $Entry.oldInjectionPointRuntimeRvaMatchCount = 0
        $Entry.oldInjectionPointRuntimeRvaStatus = "no-old-injection-point"
        return
    }

    if ($PeStatus -ne "pe-rva-available") {
        $Entry.oldInjectionPointRuntimeRvaMatches = @()
        $Entry.oldInjectionPointRuntimeRvaMatchCount = 0
        $Entry.oldInjectionPointRuntimeRvaStatus = "pe-rva-unavailable"
        return
    }

    $hitRvas = New-Object System.Collections.Generic.HashSet[int64]
    foreach ($hit in @($Entry.runtimeHits)) {
        if ($null -ne $hit.rva) {
            [void]$hitRvas.Add([int64]$hit.rva)
        }
    }

    foreach ($point in $oldPoints) {
        $oldRva = Get-RvaFromInjectionPoint -InjectionPoint $point
        if ($null -ne $oldRva -and $hitRvas.Contains([int64]$oldRva)) {
            $matches.Add($point) | Out-Null
        }
    }

    $Entry.oldInjectionPointRuntimeRvaMatches = [string[]]$matches.ToArray()
    $Entry.oldInjectionPointRuntimeRvaMatchCount = $Entry.oldInjectionPointRuntimeRvaMatches.Count
    $Entry.oldInjectionPointRuntimeRvaStatus = if ($Entry.oldInjectionPointRuntimeRvaMatchCount -gt 0) {
        "exact-rva-match"
    } else {
        "shifted-or-no-exact-rva-match"
    }
}

function Ensure-AobScannerType {
    $scannerTypeName = [System.Management.Automation.PSTypeName]"Sward.UiLab.AobScanner"
    if ($null -ne $scannerTypeName.Type) {
        return
    }

    Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;

namespace Sward.UiLab
{
    public static class AobScanner
    {
        public static long[] Find(byte[] haystack, int[] pattern)
        {
            if (haystack == null || pattern == null || pattern.Length == 0 || haystack.Length < pattern.Length)
            {
                return Array.Empty<long>();
            }

            var matches = new List<long>();
            int firstConcrete = -1;
            for (int index = 0; index < pattern.Length; ++index)
            {
                if (pattern[index] >= 0)
                {
                    firstConcrete = index;
                    break;
                }
            }

            for (int offset = 0; offset <= haystack.Length - pattern.Length; ++offset)
            {
                if (firstConcrete >= 0 && haystack[offset + firstConcrete] != (byte)pattern[firstConcrete])
                {
                    continue;
                }

                bool matched = true;
                for (int index = 0; index < pattern.Length; ++index)
                {
                    int expected = pattern[index];
                    if (expected >= 0 && haystack[offset + index] != (byte)expected)
                    {
                        matched = false;
                        break;
                    }
                }

                if (matched)
                {
                    matches.Add(offset);
                }
            }

            return matches.ToArray();
        }
    }
}
"@
}

function Resolve-HitSymbols {
    param(
        [string]$ExecutablePath,
        [string]$ResolvedSymbolizerPath,
        [object[]]$Entries,
        [int]$Limit
    )

    if ([string]::IsNullOrWhiteSpace($ResolvedSymbolizerPath) -or !(Test-Path -LiteralPath $ResolvedSymbolizerPath -PathType Leaf)) {
        return "symbolizer-not-available"
    }

    $resolvedCount = 0
    foreach ($entry in @($Entries)) {
        foreach ($hit in @($entry.runtimeHits)) {
            if ($resolvedCount -ge $Limit) {
                return "symbolizer-hit-limit-reached"
            }

            if ($null -eq $hit.virtualAddress) {
                continue
            }

            $addressText = Format-HexInt $hit.virtualAddress
            try {
                $symbolLines = @(& $ResolvedSymbolizerPath "--obj=$ExecutablePath" $addressText 2>$null)
                if ($symbolLines.Count -gt 0) {
                    $hit.symbol = $symbolLines[0].Trim()
                }
                if ($symbolLines.Count -gt 1) {
                    $hit.source = $symbolLines[1].Trim()
                }
                ++$resolvedCount
            } catch {
                $hit.symbol = ""
                $hit.source = ""
            }
        }
    }

    if ($resolvedCount -gt 0) {
        return "symbolizer-resolved:$resolvedCount"
    }

    return "symbolizer-no-addresses"
}

if (!(Test-Path -LiteralPath $CheatTablePath -PathType Leaf)) {
    throw "CheatTablePath does not exist or is not a file: $CheatTablePath"
}

$resolvedCheatTablePath = (Resolve-Path -LiteralPath $CheatTablePath).Path
[xml]$document = Get-Content -Raw -LiteralPath $resolvedCheatTablePath
$cheatTableVersion = $document.CheatTable.CheatEngineTableVersion
if ([string]::IsNullOrWhiteSpace($cheatTableVersion)) {
    $cheatTableVersion = ""
}

$anchors = New-Object System.Collections.Generic.List[object]
$rootEntries = @($document.SelectNodes("/CheatTable/CheatEntries/CheatEntry"))
Add-CheatEntryAnchors -Nodes $rootEntries -Anchors $anchors
$codeEntryAnchors = New-Object System.Collections.Generic.List[object]
$rootCodeEntries = @($document.SelectNodes("/CheatTable/CheatCodes/CodeEntry"))
Add-CodeEntryAnchors -Nodes $rootCodeEntries -Anchors $codeEntryAnchors

$scanEntries = New-Object System.Collections.Generic.List[object]
foreach ($entry in @($anchors.ToArray())) {
    $scanEntries.Add($entry) | Out-Null
}
foreach ($entry in @($codeEntryAnchors.ToArray())) {
    $scanEntries.Add($entry) | Out-Null
}

$resolvedRuntimeExePath = ""
$runtimeScanStatus = "not-requested"
$peImageMap = [ordered]@{
    status = "not-scanned"
    imageBase = $null
    sections = @()
}

if (![string]::IsNullOrWhiteSpace($RuntimeExePath)) {
    if (!(Test-Path -LiteralPath $RuntimeExePath -PathType Leaf)) {
        throw "RuntimeExePath does not exist or is not a file: $RuntimeExePath"
    }

    $resolvedRuntimeExePath = (Resolve-Path -LiteralPath $RuntimeExePath).Path
    $runtimeBytes = [System.IO.File]::ReadAllBytes($resolvedRuntimeExePath)
    $peImageMap = Get-PeImageMap -Bytes $runtimeBytes
    Ensure-AobScannerType

    foreach ($entry in [object[]]$scanEntries.ToArray()) {
        $patternBytes = [int[]]@($entry.aobPatternBytes | ForEach-Object { [int]$_ })
        $offsets = [Sward.UiLab.AobScanner]::Find($runtimeBytes, $patternBytes)
        $hits = New-Object System.Collections.Generic.List[object]
        foreach ($offset in @($offsets)) {
            $hitDelta = if ($null -ne $entry.runtimeHitFileOffsetDelta) {
                [int64]$entry.runtimeHitFileOffsetDelta
            } else {
                [int64]0
            }
            $scanFileOffset = [int64]$offset
            $actualFileOffset = $scanFileOffset + $hitDelta
            $address = Convert-FileOffsetToImageAddress -ImageMap $peImageMap -FileOffset $actualFileOffset
            $hits.Add([ordered]@{
                fileOffset = $actualFileOffset
                fileOffsetHex = Format-HexInt $actualFileOffset
                scanFileOffset = $scanFileOffset
                scanFileOffsetHex = Format-HexInt $scanFileOffset
                section = $address.section
                rva = $address.rva
                rvaHex = Format-HexInt $address.rva
                virtualAddress = $address.virtualAddress
                virtualAddressHex = Format-HexInt $address.virtualAddress
                symbol = ""
                source = ""
            }) | Out-Null
        }

        $entry.runtimeHits = [object[]]$hits.ToArray()
        $entry.runtimeHitCount = $entry.runtimeHits.Count
        Resolve-OldInjectionPointRuntimeRvaAlignment -Entry $entry -PeStatus $peImageMap.status
    }

    if ([string]::IsNullOrWhiteSpace($SymbolizerPath)) {
        $defaultSymbolizerPath = "C:\Program Files\LLVM\bin\llvm-symbolizer.exe"
        if (Test-Path -LiteralPath $defaultSymbolizerPath -PathType Leaf) {
            $SymbolizerPath = $defaultSymbolizerPath
        }
    }

    $symbolizerStatus = Resolve-HitSymbols `
        -ExecutablePath $resolvedRuntimeExePath `
        -ResolvedSymbolizerPath $SymbolizerPath `
        -Entries ([object[]]$scanEntries.ToArray()) `
        -Limit $MaxSymbolHits
    $runtimeScanStatus = "scanned;$($peImageMap.status);$symbolizerStatus"
}

$entries = @($anchors.ToArray())
$codeEntries = @($codeEntryAnchors.ToArray())
$oldInjectionPointRuntimeRvaMatchCount = 0
foreach ($entry in @($scanEntries.ToArray())) {
    if ($null -ne $entry.oldInjectionPointRuntimeRvaMatchCount) {
        $oldInjectionPointRuntimeRvaMatchCount += [int]$entry.oldInjectionPointRuntimeRvaMatchCount
    }
}
$summary = [ordered]@{
    evidenceLane = "local CT evidence lane"
    status = "ct-gameplay-value-anchors-not-ui-source-proof"
    cheatTablePath = $resolvedCheatTablePath
    cheatTableVersion = $cheatTableVersion
    entriesWithAob = $entries.Count
    codeEntriesWithBytes = $codeEntries.Count
    hostNativeSites = $entries.Count + $codeEntries.Count
    oldInjectionPointRuntimeRvaMatchCount = $oldInjectionPointRuntimeRvaMatchCount
    runtimeExePath = $resolvedRuntimeExePath
    runtimeScanStatus = $runtimeScanStatus
    peStatus = $peImageMap.status
    entries = $entries
    codeEntries = $codeEntries
}

if (![string]::IsNullOrWhiteSpace($OutputPath)) {
    $resolvedOutputPath = [System.IO.Path]::GetFullPath($OutputPath)
    $outputDirectory = Split-Path -Parent $resolvedOutputPath
    if (![string]::IsNullOrWhiteSpace($outputDirectory) -and !(Test-Path -LiteralPath $outputDirectory -PathType Container)) {
        New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
    }

    $jsonText = $summary | ConvertTo-Json -Depth 16
    [System.IO.File]::WriteAllText($resolvedOutputPath, $jsonText, (New-Object System.Text.UTF8Encoding($false)))
}

if ($Json) {
    $summary | ConvertTo-Json -Depth 16
    exit 0
}

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("local_ct_evidence_lane=runtime-gameplay-value-anchors") | Out-Null
$lines.Add("cheat_table_path=$resolvedCheatTablePath") | Out-Null
$lines.Add("cheat_table_version=$cheatTableVersion") | Out-Null
$lines.Add("ct_entries_with_aob=$($entries.Count)") | Out-Null
$lines.Add("ct_code_entries=$($codeEntries.Count)") | Out-Null
$lines.Add("ct_host_native_sites=$($entries.Count + $codeEntries.Count)") | Out-Null
$lines.Add("old_ct_injection_point_rva_matches=$oldInjectionPointRuntimeRvaMatchCount") | Out-Null
$lines.Add("runtime_exe_path=$resolvedRuntimeExePath") | Out-Null
$lines.Add("runtime_scan_status=$runtimeScanStatus") | Out-Null
$lines.Add("pe_status=$($peImageMap.status)") | Out-Null

foreach ($entry in $entries) {
    $fileOffsets = if ($entry.runtimeHits.Count -gt 0) {
        (@($entry.runtimeHits) | ForEach-Object { $_.fileOffsetHex }) -join ","
    } else {
        "<none>"
    }
    $rvas = if ($entry.runtimeHits.Count -gt 0) {
        (@($entry.runtimeHits) | Where-Object { -not [string]::IsNullOrWhiteSpace($_.rvaHex) } | ForEach-Object { $_.rvaHex }) -join ","
    } else {
        ""
    }
    $virtualAddresses = if ($entry.runtimeHits.Count -gt 0) {
        (@($entry.runtimeHits) | Where-Object { -not [string]::IsNullOrWhiteSpace($_.virtualAddressHex) } | ForEach-Object { $_.virtualAddressHex }) -join ","
    } else {
        ""
    }
    $symbols = if ($entry.runtimeHits.Count -gt 0) {
        (@($entry.runtimeHits) |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_.symbol) } |
            ForEach-Object {
                if ([string]::IsNullOrWhiteSpace($_.source)) {
                    $_.symbol
                } else {
                    "$($_.symbol)@$($_.source)"
                }
            }) -join ","
    } else {
        ""
    }
    $oldInjectionPoints = if ($entry.oldInjectionPoints.Count -gt 0) {
        $entry.oldInjectionPoints -join ","
    } else {
        "<none>"
    }
    $oldInjectionPointRuntimeRvaMatches = if ($entry.oldInjectionPointRuntimeRvaMatches.Count -gt 0) {
        $entry.oldInjectionPointRuntimeRvaMatches -join ","
    } else {
        "<none>"
    }

    $referencedPointCount = if ($entry.referencedInjectionPoints) { $entry.referencedInjectionPoints.Count } else { 0 }
    $line = "ct_entry id={0} path={1} description={2} aob_label={3} module={4} aob_bytes={5} old_injection_points={6} old_point_rva_status={7} old_point_rva_matches={8} referenced_points={9} runtime_hits={10} file_offsets={11}" -f `
        $entry.id,
        $entry.path,
        $entry.description,
        $entry.aobLabel,
        $entry.module,
        $entry.aobPattern,
        $oldInjectionPoints,
        $entry.oldInjectionPointRuntimeRvaStatus,
        $oldInjectionPointRuntimeRvaMatches,
        $referencedPointCount,
        $entry.runtimeHitCount,
        $fileOffsets

    if (![string]::IsNullOrWhiteSpace($rvas)) {
        $line += " rvas=$rvas"
    }
    if (![string]::IsNullOrWhiteSpace($virtualAddresses)) {
        $line += " vas=$virtualAddresses"
    }
    if (![string]::IsNullOrWhiteSpace($symbols)) {
        $line += " symbols=$symbols"
    }

    $lines.Add($line) | Out-Null
}

foreach ($entry in $codeEntries) {
    $fileOffsets = if ($entry.runtimeHits.Count -gt 0) {
        (@($entry.runtimeHits) | ForEach-Object { $_.fileOffsetHex }) -join ","
    } else {
        "<none>"
    }
    $rvas = if ($entry.runtimeHits.Count -gt 0) {
        (@($entry.runtimeHits) | Where-Object { -not [string]::IsNullOrWhiteSpace($_.rvaHex) } | ForEach-Object { $_.rvaHex }) -join ","
    } else {
        ""
    }
    $virtualAddresses = if ($entry.runtimeHits.Count -gt 0) {
        (@($entry.runtimeHits) | Where-Object { -not [string]::IsNullOrWhiteSpace($_.virtualAddressHex) } | ForEach-Object { $_.virtualAddressHex }) -join ","
    } else {
        ""
    }
    $symbols = if ($entry.runtimeHits.Count -gt 0) {
        (@($entry.runtimeHits) |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_.symbol) } |
            ForEach-Object {
                if ([string]::IsNullOrWhiteSpace($_.source)) {
                    $_.symbol
                } else {
                    "$($_.symbol)@$($_.source)"
                }
            }) -join ","
    } else {
        ""
    }
    $oldInjectionPoints = if ($entry.oldInjectionPoints.Count -gt 0) {
        $entry.oldInjectionPoints -join ","
    } else {
        "<none>"
    }
    $oldInjectionPointRuntimeRvaMatches = if ($entry.oldInjectionPointRuntimeRvaMatches.Count -gt 0) {
        $entry.oldInjectionPointRuntimeRvaMatches -join ","
    } else {
        "<none>"
    }

    $line = "ct_code_entry description={0} address={1} module={2} bytes={3} old_injection_points={4} old_point_rva_status={5} old_point_rva_matches={6} runtime_hits={7} file_offsets={8}" -f `
        $entry.description,
        $entry.codeEntryAddressString,
        $entry.module,
        $entry.aobPattern,
        $oldInjectionPoints,
        $entry.oldInjectionPointRuntimeRvaStatus,
        $oldInjectionPointRuntimeRvaMatches,
        $entry.runtimeHitCount,
        $fileOffsets

    if (![string]::IsNullOrWhiteSpace($rvas)) {
        $line += " rvas=$rvas"
    }
    if (![string]::IsNullOrWhiteSpace($virtualAddresses)) {
        $line += " vas=$virtualAddresses"
    }
    if (![string]::IsNullOrWhiteSpace($symbols)) {
        $line += " symbols=$symbols"
    }

    $lines.Add($line) | Out-Null
}

$lines -join [Environment]::NewLine
