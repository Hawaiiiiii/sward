# Phase 373 -- sg-preflight runtime bridge export.
#
# Calls the real sg-preflight CLI in the operator's checkout
# (default: C:\Users\...\Downloads\sg-preflight) and writes a
# normalised JSON snapshot the SGFX shell can consume:
#
#   <OutputPath> = <PackDir>/sg_preflight_state.json (or wherever
#                   the caller asks)
#
# Schema (sgfx_preflight_state v1):
#
#   {
#     "schema": "sgfx_preflight_state",
#     "version": 1,
#     "source_root": "C:/.../sg-preflight",
#     "python":      "C:/.../sg-preflight/.venv/Scripts/python.exe",
#     "generated_at_utc": "2026-05-08T12:00:00Z",
#     "selected_profile": "G65",
#     "ticket":   "...",  // forwarded from the BMW QA pack if known
#     "project":  "...",
#     "profiles": [ /* sg-preflight list-profiles --json output */ ],
#     "actions":  [ /* sg-preflight list-actions  --json output,
#                       filtered to selected profile + workspace
#                       scope when -Profile is set */ ],
#     "checkers": [ /* sg-preflight list-checkers --json output */ ],
#     "workflow": [ /* sg-preflight workflow-status --json output */ ],
#     "latest_run":     null,
#     "evidence_paths": [],
#     "warnings": [ "human-readable strings; one per failed CLI step
#                    or skipped probe (e.g. workflow-status not available
#                    on this preflight version)" ]
#   }
#
# Key design properties:
#   - The bridge does NOT duplicate sg-preflight's logic. It only
#     calls sg-preflight, parses the JSON it already returns, and
#     writes a single consolidated file. If sg-preflight changes
#     its schema, this bridge's output changes with it.
#   - Failures are captured as warnings, not as fatal errors. The
#     bridge always writes a file -- even when sg-preflight is
#     entirely unavailable, the consumer (the SGFX shell) gets a
#     valid `sgfx_preflight_state` document with empty arrays and
#     the failure reason in `warnings`. This keeps the SGFX shell
#     boot path resilient: an operator without a working
#     sg-preflight venv still gets the shell, just without
#     preflight context.
#   - Python interpreter discovery: tries .venv\Scripts\python.exe
#     first (the canonical sg-preflight venv), then
#     .venv_bmw_ci\Scripts\python.exe, then the caller's PATH
#     `python`. The selected path is recorded in the output so the
#     consumer can debug "wrong python" issues.

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [string]$SgPreflightRoot,

    [Parameter(Mandatory=$true)]
    [string]$OutputPath,

    [string]$Profile = '',
    [string]$Ticket  = '',
    [string]$Project = ''
)

$ErrorActionPreference = 'Stop'

# --- 0. Sanity check root + interpreter ----------------------------------

if (-not (Test-Path -LiteralPath $SgPreflightRoot)) {
    throw "SgPreflightRoot not found: $SgPreflightRoot"
}
$rootFull = (Resolve-Path -LiteralPath $SgPreflightRoot).ProviderPath

# Discover Python interpreter. Prefer the canonical sg-preflight
# .venv so installed deps (openpyxl, fastapi, etc.) are reachable.
$candidatePyPaths = @(
    (Join-Path $rootFull '.venv\Scripts\python.exe'),
    (Join-Path $rootFull '.venv_bmw_ci\Scripts\python.exe')
)
$pythonExe = ''
foreach ($cand in $candidatePyPaths) {
    if (Test-Path -LiteralPath $cand) { $pythonExe = $cand; break }
}
if (-not $pythonExe) {
    # Fall back to PATH `python`. We do NOT throw if this also
    # fails -- the bridge's contract is to always write SOMETHING.
    $cmd = Get-Command python -ErrorAction SilentlyContinue
    if ($cmd) { $pythonExe = $cmd.Source }
}

# --- 1. Helper: invoke a CLI subcommand; capture stdout/stderr/exit -----

$warnings = New-Object System.Collections.Generic.List[string]

function Invoke-PreflightJson {
    param(
        [string[]]$ArgList,
        [string]$Label
    )
    if (-not $pythonExe) {
        $warnings.Add("[$Label] no python interpreter found; skipped")
        return $null
    }
    $stdoutFile = Join-Path $env:TEMP ("sgfx_preflight_" + [guid]::NewGuid().ToString('N') + ".stdout")
    $stderrFile = Join-Path $env:TEMP ("sgfx_preflight_" + [guid]::NewGuid().ToString('N') + ".stderr")
    try {
        # Quote args defensively so paths with spaces survive the
        # Start-Process argument list (same lesson as Phase 372).
        $quoted = New-Object System.Collections.Generic.List[string]
        $quoted.Add('-m')
        $quoted.Add('sg_preflight')
        foreach ($a in $ArgList) {
            if ($a -match '\s') { $quoted.Add('"' + $a + '"') } else { $quoted.Add($a) }
        }
        $proc = Start-Process -FilePath $pythonExe `
            -ArgumentList $quoted.ToArray() `
            -WorkingDirectory $rootFull `
            -NoNewWindow -Wait -PassThru `
            -RedirectStandardOutput $stdoutFile `
            -RedirectStandardError  $stderrFile
        if ($proc.ExitCode -ne 0) {
            $errText = ''
            if (Test-Path -LiteralPath $stderrFile) {
                $errText = Get-Content -LiteralPath $stderrFile -Raw -ErrorAction SilentlyContinue
            }
            $errSummary = if ($errText) { ($errText -split "`r?`n")[0] } else { "(no stderr)" }
            $warnings.Add("[$Label] sg-preflight exited $($proc.ExitCode); $errSummary")
            return $null
        }
        $stdout = Get-Content -LiteralPath $stdoutFile -Raw -ErrorAction SilentlyContinue
        if (-not $stdout) {
            $warnings.Add("[$Label] sg-preflight produced no stdout")
            return $null
        }
        try {
            return $stdout | ConvertFrom-Json -ErrorAction Stop
        } catch {
            $warnings.Add("[$Label] stdout was not valid JSON: $($_.Exception.Message)")
            return $null
        }
    }
    finally {
        Remove-Item -LiteralPath $stdoutFile -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $stderrFile -Force -ErrorAction SilentlyContinue
    }
}

# --- 2. Probe each CLI surface ------------------------------------------

$profiles = Invoke-PreflightJson -ArgList @('list-profiles','--json') -Label 'list-profiles'
if (-not $profiles) { $profiles = @() }

$actionsAll = Invoke-PreflightJson -ArgList @('list-actions','--json') -Label 'list-actions'
if (-not $actionsAll) { $actionsAll = @() }

$checkers = Invoke-PreflightJson -ArgList @('list-checkers','--json') -Label 'list-checkers'
if (-not $checkers) { $checkers = @() }

$workflow = Invoke-PreflightJson -ArgList @('workflow-status','--json') -Label 'workflow-status'
if (-not $workflow) { $workflow = @() }

# --- 3. Filter actions to selected profile when one was given ----------

# When -Profile is set: keep actions whose profile_id matches AND
# actions with empty profile_id whose scope is `workspace` (those
# apply to the whole workspace, not to a specific car). When
# -Profile is empty: keep everything.
$actions = @()
if ($Profile) {
    foreach ($a in @($actionsAll)) {
        if ($a.profile_id -ieq $Profile) { $actions += $a; continue }
        if ((-not $a.profile_id) -and $a.scope -ieq 'workspace') { $actions += $a }
    }
    if ($actions.Count -eq 0 -and $actionsAll.Count -gt 0) {
        $warnings.Add("[filter] -Profile '$Profile' matched no actions out of $($actionsAll.Count) total")
    }
} else {
    $actions = @($actionsAll)
}

# --- 4. Validate selected profile exists -------------------------------

if ($Profile) {
    $profileMatch = @($profiles | Where-Object { $_.profile_id -ieq $Profile })
    if ($profileMatch.Count -eq 0) {
        $warnings.Add("[selected_profile] '$Profile' not found in list-profiles output")
    }
}

# --- 5. Build output document via templated literal --------------------

# We build the JSON as a templated string rather than ConvertTo-Json
# on a hashtable because PowerShell wraps `[ordered]@{}` arrays as
# `{"value":[...], "Count":N}` (same lesson as the Phase 371A
# exporter). Each sub-array is re-emitted via ConvertTo-Json on the
# leaf array directly, which produces a clean JSON array.

function Json-OrEmpty {
    param($value)
    if ($null -eq $value) { return '[]' }
    if ($value -is [System.Array]) {
        if ($value.Count -eq 0) { return '[]' }
        return ($value | ConvertTo-Json -Depth 12 -Compress)
    }
    # Single object -- wrap in an array literal.
    return ('[' + ($value | ConvertTo-Json -Depth 12 -Compress) + ']')
}

$profilesJson = Json-OrEmpty $profiles
$actionsJson  = Json-OrEmpty $actions
$checkersJson = Json-OrEmpty $checkers
$workflowJson = Json-OrEmpty $workflow

$warningsJson = '[]'
if ($warnings.Count -gt 0) {
    $items = $warnings | ForEach-Object { $_ | ConvertTo-Json -Compress }
    $warningsJson = "[`n    " + (($items) -join ",`n    ") + "`n  ]"
}

$nowUtc = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
$srcRootForward = $rootFull -replace '\\','/'
$pyForward      = if ($pythonExe) { $pythonExe -replace '\\','/' } else { '' }

$doc = @"
{
  "schema": "sgfx_preflight_state",
  "version": 1,
  "source_root": $($srcRootForward | ConvertTo-Json -Compress),
  "python":      $($pyForward      | ConvertTo-Json -Compress),
  "generated_at_utc": $($nowUtc | ConvertTo-Json -Compress),
  "selected_profile": $($Profile | ConvertTo-Json -Compress),
  "ticket":  $($Ticket  | ConvertTo-Json -Compress),
  "project": $($Project | ConvertTo-Json -Compress),
  "profiles": $profilesJson,
  "actions":  $actionsJson,
  "checkers": $checkersJson,
  "workflow": $workflowJson,
  "latest_run":     null,
  "evidence_paths": [],
  "warnings": $warningsJson
}
"@

# --- 6. Atomic write to OutputPath -------------------------------------

$outDir = Split-Path -Parent $OutputPath
if ($outDir -and -not (Test-Path -LiteralPath $outDir)) {
    New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}
$tmpOut = $OutputPath + '.tmp'
Set-Content -LiteralPath $tmpOut -Value $doc -Encoding UTF8
# Atomic rename: a concurrent UR boot reading the file never sees
# a half-written document. Same pattern as the bridge daemon's
# state-file rewrite.
Move-Item -LiteralPath $tmpOut -Destination $OutputPath -Force

Write-Host ""
Write-Host "==[ SGFX Preflight Bridge ]== state written" -ForegroundColor Green
Write-Host "  output:           $OutputPath"
Write-Host "  source root:      $rootFull"
Write-Host "  python:           $pythonExe"
Write-Host "  selected profile: $Profile"
Write-Host "  profiles:         $($profiles.Count)"
Write-Host "  actions (filt):   $($actions.Count) of $($actionsAll.Count)"
Write-Host "  checkers:         $($checkers.Count)"
Write-Host "  workflow areas:   $($workflow.Count)"
Write-Host "  warnings:         $($warnings.Count)"
foreach ($w in $warnings) { Write-Host "    - $w" -ForegroundColor Yellow }

$OutputPath
