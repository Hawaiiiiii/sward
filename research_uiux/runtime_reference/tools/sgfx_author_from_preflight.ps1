# Phase 372 -- BMW Pack Authoring from sg-preflight project data.
#
# Reads a "BMW QA pack-authoring file" sitting alongside the existing
# sg-preflight rule configs (e.g. sg-preflight\config\bmw_qa_g65.json),
# resolves all paths/templates relative to that file, then invokes
# sgfx_pack_exporter.ps1 with the resolved values to produce a
# complete SGFX shell pack.
#
# This is the layer that turns hand-written test JSON into a real
# department workflow: an operator authors one BMW QA file per
# ticket/profile, the authoring tool produces the pack, the launcher
# runs it, and the in-game QA panel surfaces the metadata.
#
# Schema (sgfx_bmw_qa_pack v1):
#
#   {
#     "schema": "sgfx_bmw_qa_pack",
#     "version": 1,
#     "profile_id": "g65",                  // optional; for templating
#     "ticket":  "IDCEVODEV-960073",        // mandatory
#     "project": "BMW G65 IDCevo SGFX QA",  // mandatory
#     "route":   "title",                   // mandatory; one of
#                                           //   title|auto|worldmap|hud|results
#     "branding": {
#       "window_title_template": "${project} (${ticket})",
#       "build_label_template":  "SGFX 0.6 (Phase 372 / ${profile_id})",
#       "icon_relative":         "../assets/g65_icon.png",
#       "logo_relative":         "../assets/g65_logo.png"
#     },
#     "scoped_text_rules": [
#       {
#         "literal":                "99",
#         "csd_project_substring":  "status",      // MANDATORY
#         "replacement":            "G65",
#         "rationale":              "..."          // optional, surfaced in log
#       }
#     ],
#     "picture_overrides": [
#       {
#         "guest_picture":   "logo_sonicteam",
#         "source_relative": "../assets/g65_logo.dds",
#         "rationale":       "..."
#       }
#     ]
#   }
#
# Validation rules enforced by this script (and worth flagging early
# in the operator's editor before they even hit the launcher):
#
#   - ticket, project, route are non-empty strings
#   - route is one of the canonical values
#   - every scoped_text_rules entry has a non-empty
#     `csd_project_substring`. Unscoped rules are REJECTED to prevent
#     a Phase 367b "BMW999 everywhere" regression where a global rule
#     swapped a literal across every CSD project, including ones
#     where the swap was nonsensical.
#   - every picture_overrides entry has both `guest_picture` and an
#     existing source file
#   - every required picture source resolves to an existing file.
#     Relative paths are anchored at the project-file directory and
#     may intentionally point at neighboring SGFX asset folders; the
#     pack exporter later guards pack-internal output paths.

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [string]$ProjectFile,

    [Parameter(Mandatory=$true)]
    [string]$OutputDir,

    # CLI overrides. When set, beat the project file's value.
    [string]$Ticket = '',
    [string]$Project = '',
    [ValidateSet('','title','auto','worldmap','hud','results')]
    [string]$Route = '',

    # Forward-only flags. The authoring tool never launches anything;
    # composition with the launcher is the operator's job (or the
    # phase372 proof's job). -Force is forwarded to the exporter.
    [switch]$Force,

    [string]$RepoRoot = ''
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $ProjectFile)) {
    throw "ProjectFile not found: $ProjectFile"
}
$projectFileFull = (Resolve-Path -LiteralPath $ProjectFile).ProviderPath
$projectDir      = Split-Path -Parent $projectFileFull

if (-not $RepoRoot -or $RepoRoot -eq '') {
    $scriptDir = Split-Path -Parent $PSCommandPath
    $RepoRoot  = (Resolve-Path (Join-Path $scriptDir '..\..\..')).ProviderPath
}
$exporter = Join-Path (Split-Path -Parent $PSCommandPath) 'sgfx_pack_exporter.ps1'
if (-not (Test-Path -LiteralPath $exporter)) {
    throw "Exporter not found alongside this script: $exporter"
}

# --- Load + validate project file --------------------------------------

$raw = Get-Content -LiteralPath $projectFileFull -Raw
try {
    $doc = $raw | ConvertFrom-Json -ErrorAction Stop
} catch {
    throw "ProjectFile is not valid JSON: $($_.Exception.Message)"
}

if (-not $doc.schema -or $doc.schema -ne 'sgfx_bmw_qa_pack') {
    throw "ProjectFile schema must be 'sgfx_bmw_qa_pack' (got '$($doc.schema)')"
}
if (-not $doc.version -or [int]$doc.version -ne 1) {
    throw "ProjectFile version must be 1 (got '$($doc.version)')"
}

$effectiveTicket  = if ($Ticket)  { $Ticket }  else { [string]$doc.ticket }
$effectiveProject = if ($Project) { $Project } else { [string]$doc.project }
$effectiveRoute   = if ($Route)   { $Route }   else { [string]$doc.route }
if ([string]::IsNullOrWhiteSpace($effectiveTicket)) {
    throw "ProjectFile must declare a non-empty `"ticket`" (or pass -Ticket)"
}
if ([string]::IsNullOrWhiteSpace($effectiveProject)) {
    throw "ProjectFile must declare a non-empty `"project`" (or pass -Project)"
}
$validRoutes = @('title','auto','worldmap','hud','results')
if ($validRoutes -notcontains $effectiveRoute) {
    throw "Route '$effectiveRoute' is not one of: $($validRoutes -join ', ')"
}

$profileId = if ($doc.profile_id) { [string]$doc.profile_id } else { '' }

# Template substitution for branding strings: ${ticket}, ${project},
# ${profile_id}.
function Expand-Template {
    param([string]$Template)
    if ([string]::IsNullOrEmpty($Template)) { return '' }
    $out = $Template
    $out = $out.Replace('${ticket}',     $effectiveTicket)
    $out = $out.Replace('${project}',    $effectiveProject)
    $out = $out.Replace('${profile_id}', $profileId)
    return $out
}

# Resolve a path against $projectDir. Relative paths anchor on the
# project file's directory (portable across checkouts); absolute
# paths are accepted unchanged (useful when the operator drags an
# asset in from outside the sg-preflight tree, or when a proof
# runner synthesises a project file in a temp dir).
function Resolve-ProjectRelative {
    param([string]$Relative, [string]$Field)
    if ([string]::IsNullOrWhiteSpace($Relative)) { return '' }
    if ([System.IO.Path]::IsPathRooted($Relative)) {
        return [System.IO.Path]::GetFullPath($Relative)
    }
    $combined = [System.IO.Path]::GetFullPath((Join-Path $projectDir $Relative))
    return $combined
}

# Branding paths.
$brandingNode = $doc.branding
$windowTitle = ''
$buildLabel  = ''
$iconPath    = ''
$logoPath    = ''
if ($brandingNode) {
    $windowTitle = Expand-Template ($brandingNode.window_title_template)
    $buildLabel  = Expand-Template ($brandingNode.build_label_template)
    if ($brandingNode.icon_relative) {
        $iconPath = Resolve-ProjectRelative -Relative $brandingNode.icon_relative -Field 'branding.icon_relative'
        if (-not (Test-Path -LiteralPath $iconPath)) {
            Write-Host "[author] WARN: branding.icon_relative resolves to missing file: $iconPath" -ForegroundColor Yellow
            $iconPath = ''
        }
    }
    if ($brandingNode.logo_relative) {
        $logoPath = Resolve-ProjectRelative -Relative $brandingNode.logo_relative -Field 'branding.logo_relative'
        if (-not (Test-Path -LiteralPath $logoPath)) {
            Write-Host "[author] WARN: branding.logo_relative resolves to missing file: $logoPath" -ForegroundColor Yellow
            $logoPath = ''
        }
    }
}

# Scoped text rules: validate every entry has csd_project_substring.
$scopedRules = @()
if ($doc.scoped_text_rules) {
    $idx = 0
    foreach ($r in @($doc.scoped_text_rules)) {
        $idx++
        $literal = [string]$r.literal
        $scope   = [string]$r.csd_project_substring
        $replace = [string]$r.replacement
        if ([string]::IsNullOrWhiteSpace($literal)) {
            throw "scoped_text_rules[$idx] missing 'literal'"
        }
        if ([string]::IsNullOrWhiteSpace($scope)) {
            throw ("scoped_text_rules[" + $idx + "] missing 'csd_project_substring'. " +
                   "Unscoped rules are rejected to prevent a Phase 367b-style " +
                   "'BMW999 everywhere' regression. Add a substring of the CSD " +
                   "project name where this rule should apply (e.g. 'status', 'title').")
        }
        if ([string]::IsNullOrWhiteSpace($replace)) {
            throw "scoped_text_rules[$idx] missing 'replacement'"
        }
        $scopedRules += [pscustomobject]@{
            literal               = $literal
            csd_project_substring = $scope
            replacement           = $replace
        }
    }
}

# Picture overrides: validate every entry resolves to an existing
# source file.
$pictureEntries = New-Object System.Collections.Generic.List[hashtable]
$pictureRationales = New-Object System.Collections.Generic.List[string]
if ($doc.picture_overrides) {
    $idx = 0
    foreach ($p in @($doc.picture_overrides)) {
        $idx++
        $guest = [string]$p.guest_picture
        $rel   = [string]$p.source_relative
        if ([string]::IsNullOrWhiteSpace($guest)) {
            throw "picture_overrides[$idx] missing 'guest_picture'"
        }
        if ([string]::IsNullOrWhiteSpace($rel)) {
            throw "picture_overrides[$idx] missing 'source_relative'"
        }
        $abs = Resolve-ProjectRelative -Relative $rel -Field "picture_overrides[$idx].source_relative"
        if (-not (Test-Path -LiteralPath $abs)) {
            throw "picture_overrides[$idx] source not found: $abs"
        }
        $pictureEntries.Add(@{ Guest = $guest; Source = $abs })
        if ($p.rationale) {
            $pictureRationales.Add("$guest -> $($p.rationale)")
        }
    }
}

# --- Stage scoped rules into a temp file the exporter can read --------

$tempRulesPath = Join-Path $env:TEMP ("sgfx_author_rules_" + (Get-Date -Format 'yyyyMMdd_HHmmss') + "_" + ([guid]::NewGuid().ToString('N').Substring(0,8)) + ".json")
$scopedRulesJson = '[]'
if ($scopedRules.Count -gt 0) {
    $lines = $scopedRules | ForEach-Object { $_ | ConvertTo-Json -Compress }
    $scopedRulesJson = "[`n  " + (($lines) -join ",`n  ") + "`n]"
}
Set-Content -LiteralPath $tempRulesPath -Value $scopedRulesJson -Encoding UTF8

# --- Print the plan + invoke the exporter ------------------------------

Write-Host ""
Write-Host "==[ SGFX Author ]== plan" -ForegroundColor Cyan
Write-Host "  project file: $projectFileFull"
Write-Host "  ticket:       $effectiveTicket"
Write-Host "  project:      $effectiveProject"
Write-Host "  route:        $effectiveRoute"
Write-Host "  profile_id:   $profileId"
Write-Host "  windowTitle:  $windowTitle"
Write-Host "  buildLabel:   $buildLabel"
Write-Host "  icon:         $iconPath"
Write-Host "  logo:         $logoPath"
Write-Host "  rules:        $($scopedRules.Count)"
Write-Host "  pictures:     $($pictureEntries.Count)"

# Splat through a hashtable rather than a positional array so the
# exporter's param binder does name-based resolution (no risk of
# values-with-dashes being misparsed as parameter names, and
# argument ordering becomes irrelevant).
$exporterArgs = @{
    OutputDir       = $OutputDir
    Ticket          = $effectiveTicket
    Project         = $effectiveProject
    Phase           = '372'
    Route           = $effectiveRoute
    ScopedRulesJson = $tempRulesPath
}
if ($windowTitle) { $exporterArgs['WindowTitle'] = $windowTitle }
if ($buildLabel)  { $exporterArgs['BuildLabel']  = $buildLabel }
if ($iconPath)    { $exporterArgs['IconPath']    = $iconPath }
if ($logoPath)    { $exporterArgs['LogoPath']    = $logoPath }
if ($Force)       { $exporterArgs['Force']       = $true }
if ($pictureEntries.Count -gt 0) {
    # Wrap as @() so a single-element list does not auto-unwrap.
    $exporterArgs['PictureOverrides'] = @($pictureEntries.ToArray())
}

# `$LASTEXITCODE` is only updated by NATIVE processes; calling a
# .ps1 via the call operator does NOT touch it. The exporter has
# `$ErrorActionPreference = 'Stop'` so any failure path throws; if
# control returns here, the exporter completed successfully.
& $exporter @exporterArgs | Out-Null

# Append the rationales the operator wrote into pack_export.log so
# future-them remembers WHY each rule exists. The exporter wrote
# the log already; we open it and append.
$logPath = Join-Path $OutputDir 'pack_export.log'
if (Test-Path -LiteralPath $logPath) {
    $append = New-Object System.Collections.Generic.List[string]
    $append.Add('')
    $append.Add('--- author rationale (Phase 372) ---')
    $append.Add("source project file: $projectFileFull")
    foreach ($r in $scopedRules) {
        $rationale = ''
        $rationaleHits = @($doc.scoped_text_rules | Where-Object {
            $_.literal -eq $r.literal -and $_.csd_project_substring -eq $r.csd_project_substring
        })
        if ($rationaleHits.Count -gt 0 -and $rationaleHits[0].rationale) {
            $rationale = [string]$rationaleHits[0].rationale
        }
        $append.Add("  rule: literal='$($r.literal)' scope='$($r.csd_project_substring)' -> '$($r.replacement)'")
        if ($rationale) {
            $append.Add("    why: $rationale")
        }
    }
    foreach ($pr in $pictureRationales) {
        $append.Add("  pic:  $pr")
    }
    Add-Content -LiteralPath $logPath -Value ($append -join [System.Environment]::NewLine) -Encoding UTF8
}

# Clean up the temp rules file.
Remove-Item -LiteralPath $tempRulesPath -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "==[ SGFX Author ]== done -> $OutputDir" -ForegroundColor Green
$OutputDir
