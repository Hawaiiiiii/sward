// =============================================================================
// settings.h — tiny persistent key=value settings (sgfx_settings.ini next to
// the exe). Write-through: SetInt saves immediately. Used for the window
// placement (display / fullscreen / size) and the options-screen values.
// =============================================================================
#pragma once

namespace settings {

void Load();                                  // call once at startup (cwd ini)
int  GetInt(const char* key, int defaultValue);
void SetInt(const char* key, int value);      // persists immediately

} // namespace settings
