#pragma once

#include <cstddef>
#include <string>

namespace SGPreflightState
{
    // Phase 373: in-process loader for the sg-preflight bridge
    // state JSON.
    //
    // The launcher (`sgfx_shell_launch.ps1 -SgPreflightRoot ...`)
    // calls `sgfx_preflight_bridge_export.ps1`, which invokes the
    // real sg-preflight CLI (list-profiles / list-actions /
    // list-checkers / workflow-status) and writes the consolidated
    // result to `<PackDir>/sg_preflight_state.json`. The launcher
    // then exports `SG_PREFLIGHT_STATE_JSON` pointing at that file.
    //
    // At boot, this loader reads the env var (or the conventional
    // path `<override>/sg_preflight_state.json` if the env is
    // unset), parses the document, and exposes a small set of
    // accessors the SGQAPanel renders. We intentionally do NOT
    // expose the full action/profile arrays: the panel needs only
    // the headline values (selected profile, action count, source
    // root, latest run summary, first warning), and a leaner
    // surface keeps the C++ parse + storage cheap.
    //
    // Schema accepted (`sgfx_preflight_state` v1):
    //   {
    //     "schema": "sgfx_preflight_state",
    //     "version": 1,
    //     "source_root": "...",
    //     "selected_profile": "G65",
    //     "ticket": "...",
    //     "project": "...",
    //     "actions": [ ... ],
    //     "warnings": [ "..." ],
    //     "latest_run": null | { "status": "..." }
    //   }
    //
    // Bridge events (always emit one, exactly once at boot):
    //   `SgPreflightState:Loaded:<profile>:<actionCount>`   -- success
    //   `SgPreflightState:Missing:<reason>`                  -- absent / parse error
    //
    // Failure modes are NEVER fatal: a missing / invalid state
    // file leaves all accessors returning nullptr / 0 and the
    // panel renders "(no preflight state)" placeholders. Vanilla
    // UR boots stay unaffected.

    // Idempotent. First call reads SG_PREFLIGHT_STATE_JSON (or
    // the fallback path), parses it, populates the cached values,
    // and emits the load/missing event exactly once.
    void EnsureLoaded();

    // Accessors. Each returns nullptr when no document was loaded
    // OR the field was empty in the document. Pointer-stable for
    // the process lifetime.
    const std::string* TryGetSelectedProfile();
    const std::string* TryGetSourceRoot();
    const std::string* TryGetLatestRunStatus();
    const std::string* TryGetFirstWarning();

    // Action count from the filtered `actions` array. Returns 0
    // when the document is missing or has no actions array.
    std::size_t GetActionCount();

    // True iff EnsureLoaded() found a document and parsed it. If
    // false, all string accessors return nullptr.
    bool IsLoaded();
}
