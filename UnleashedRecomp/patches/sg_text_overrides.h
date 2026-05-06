#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace SGTextOverrides
{
    // Phase 364: per-string text override layer.
    //
    // Layered on top of the Phase 359 asset override mount so the same
    // SG_PREFLIGHT_OVERRIDE_DIR holds both file-replacement assets and a
    // single sg_text_overrides.json that swaps individual UI strings by
    // their original literal value.
    //
    // File path: <OVERRIDE_DIR>/sg_text_overrides.json
    // Schema:
    //   {
    //     "version": 1,
    //     "strings": {
    //       "Sonic Unleashed": "BMW Drive 35",
    //       "PRESS START": "PRESS A"
    //     }
    //   }
    //
    // Two consumers:
    //
    //   1. Localise() in locale/locale.cpp  -- patches the host-side
    //      g_locale map so any UR-owned UI string (installer wizard,
    //      options menu, message windows) honours the override.
    //
    //   2. CsdNodeText_patches.cpp::sub_830BF640  -- patches the guest
    //      r4 argument to point at a guest-heap-allocated copy of the
    //      override before the retail SetText runs, so retail CSD
    //      strings (Title menu, Pause, World Map labels) get the
    //      override too.
    //
    // Both consumers are no-ops when the override file is absent.

    // Idempotent. First call after the override dir is known (i.e. after
    // the Phase 359 mod loader runs) loads sg_text_overrides.json and
    // applies its strings to the host-side g_locale. Re-calls return
    // immediately.
    void EnsureLoaded();

    // Host-side override lookup. Returns nullptr if no override.
    // Used by Localise() before falling back to g_locale.
    const std::string* TryGetOverride(std::string_view original);

    // Guest-side override lookup. Returns 0 if no override; otherwise
    // a guest virtual address of a permanent UTF-8 copy of the override
    // (allocated on g_userHeap on first hit, cached for subsequent
    // calls). The caller may pass this directly to PPC code expecting
    // a guest char* pointer.
    uint32_t TryGetOverrideGuestPtr(std::string_view original);

    // Phase 369B: scoped text overrides.
    //
    // Schema v2 adds a `scoped_rules` array alongside the v1
    // `strings` map. Both lanes co-exist. v1 entries are global
    // (apply everywhere); v2 rules apply only when the literal AND
    // the scope match.
    //
    //   {
    //     "version": 2,
    //     "strings": {
    //       "Common_Yes": "BMW Yes 35"
    //     },
    //     "scoped_rules": [
    //       { "literal": "99",
    //         "csd_project_substring": "playscreen",
    //         "replacement": "BMW99" }
    //     ]
    //   }
    //
    // Scope is a substring match against the names of the CSD
    // projects active in the current process (every project that has
    // had `MarkCsdProjectActive` called). This is intentionally
    // coarse -- node-path matching is too brittle in practice given
    // how retail SU's CSD trees vary across regions / patches -- but
    // it is enough to fix the Phase 367b "BMW999 everywhere" issue
    // because the HUD project name (e.g. `ui_prov_playscreen`) is
    // distinct from the title project name (`ui_title`).
    void MarkCsdProjectActive(std::string_view projectName);

    // Phase 369B: extended guest-side override lookup. Returns 0 if
    // no override applies. When a scope-matched rule wins,
    // `*outScopeSubstring` is set to the rule's `csd_project_substring`
    // (so the caller can report which scope matched in the bridge
    // event). When the global lane wins, `*outScopeSubstring` is
    // cleared. May return a previously-cached guest pointer when the
    // same (literal, replacement) pair has already been allocated.
    //
    // The caller passes `original` exactly as it was read from guest
    // memory; the loader matches by the JSON key string.
    uint32_t TryGetOverrideGuestPtrScoped(std::string_view original,
                                          std::string* outScopeSubstring);

    // Phase 367b: host-side proof emit. Called by Localise() the first
    // time it returns an override-backed string for a given key. Emits
    // exactly one bridge event per unique key per process boot:
    //
    //   screen_entered: "Text:HostOverrideHit:<key>"
    //
    // The event name is intentionally distinct from the guest CSD
    // SetText path's `Text:CsdOverrideHit:<original>` so that bridge
    // consumers can tell which override consumer fired. There is no
    // synthetic boot-time probe -- this only fires on real Localise()
    // calls from UR's UI code (button guide, message windows,
    // installer wizard, options menu, etc.).
    //
    // Bounded volume = number of override keys actually requested via
    // Localise() during the session. Cheap (one unordered_set lookup
    // on the hot path; the emit only fires on the first hit).
    void NoteHostHitForKey(std::string_view key);
}
