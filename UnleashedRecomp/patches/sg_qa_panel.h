#pragma once

namespace SGQAPanel
{
    // Phase 371C: in-game QA status panel.
    //
    // Reads pack metadata via SGPack::TryGet*() accessors and reload
    // counters via SG{Text,Asset,Pack}Overrides/SGPack::GetReloadCount(),
    // then renders a small ImGui overlay in the top-right corner of
    // the host window with:
    //
    //   SGFX QA Shell
    //   ticket:  IDCEVODEV-960073
    //   project: BMW SGFX QA Shell
    //   phase:   371C
    //   route:   title
    //   pack:    loaded
    //   reload:  text=0  asset=0  pack=0
    //
    // Emits a single bridge event the first time the panel actually
    // makes it to a render frame:
    //
    //   QAPanel:Active:<ticket>:<route>
    //
    // Visibility:
    //   - default: enabled when SGPack::TryGetTicket() returns non-null
    //     (i.e. the operator staged a pack_meta.json with a ticket)
    //   - explicit env override: SG_PREFLIGHT_QA_PANEL=0 disables the
    //     panel even when a ticket is present (useful when the
    //     operator wants the screenshot to be branding-clean)
    //   - explicit env enable: SG_PREFLIGHT_QA_PANEL=1 forces the
    //     panel on even when no pack_meta.json is staged (panel
    //     shows "(no pack)" placeholders for the metadata fields)
    //
    // Drawn from the same per-frame ImGui pump as the rest of UR's
    // overlays. Cheap (one ImGui::Begin/Text block; no allocations
    // on the hot path beyond what ImGui itself does).
    void Draw();
}
