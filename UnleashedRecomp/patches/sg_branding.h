#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace SGBranding
{
    // Phase 370A: host-side branding overrides for the SGFX shell.
    //
    // Values are read once at boot from (in precedence order):
    //   1. Environment variables:
    //        SGFX_SHELL_WINDOW_TITLE  -> window title text
    //        SGFX_SHELL_BUILD_LABEL   -> build label string
    //   2. <SG_PREFLIGHT_OVERRIDE_DIR>/sgfx_pack.json :
    //        {
    //          "version": 1,
    //          "branding": {
    //            "window_title": "SGFX Shell -- Phase 370A",
    //            "build_label":  "SGFX 0.4 (Phase 370A)",
    //            "icon":         "sgfx_branding/icon.png"
    //          }
    //        }
    //
    // The icon file is resolved as:
    //   - branding.icon (relative to override dir) if pack present
    //   - <override>/sgfx_branding/icon.png (auto-discovered)
    //   - <override>/sgfx_branding/icon.bmp (auto-discovered)
    //
    // PNG decoding goes through stb_image (already linked via
    // stdafx.h); BMP routes through SDL_LoadBMP_RW directly.
    //
    // Phase 370A intentionally does NOT consult sgfx_pack.json for
    // text/asset lane routing -- that is the Phase 370B beat. A
    // pack with only a `branding` section co-exists with the
    // existing flat-file `sg_text_overrides.json` /
    // `sg_asset_overrides.json` loaders without changing their
    // behavior.

    // Idempotent. Reads env vars + sgfx_pack.json (if present),
    // populates internal state, and emits a single
    //   screen_entered: "Branding:Active:<title>|<icon>|<label>"
    // bridge event when ANY branding override is configured.
    // No-op when nothing is configured (keeps unmodified UR boots
    // free of spurious branding markers).
    void EnsureLoaded();

    // Window title accessor. Returns nullptr when no override is
    // configured (caller falls back to the default UR title).
    // Pointer/string is stable for the lifetime of the process.
    const std::string* TryGetWindowTitle();

    // Build label accessor. Returns nullptr when no override is
    // configured. Used by the boot banner / log writer to swap in
    // an SGFX-branded build label without touching the version
    // generator's git-derived value.
    const std::string* TryGetBuildLabel();

    // Icon path accessor. Returns an empty path when no override
    // icon was discovered. Used by GameWindow::SetIcon to load a
    // PNG/BMP from disk via stb_image / SDL_LoadBMP_RW instead of
    // the UR-embedded resource. Returned path is absolute.
    std::filesystem::path TryGetIconPath();
}
