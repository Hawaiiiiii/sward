// Default standalone implementation of the sgfx_platform shim — enough to build the
// preview harness without any host project. A real host (SGFX) replaces this TU with
// its own bindings (its config store, its input, its locale).
#include "sgfx_platform.h"

namespace sgfx {

float g_aspectRatio = WIDE_ASPECT_RATIO;   // host updates each frame from the viewport

namespace window {
    float Width()        { return ImGui::GetIO().DisplaySize.x; }
    float Height()       { return ImGui::GetIO().DisplaySize.y; }
    bool  IsFocused()    { return true; }
    bool  IsFullscreen() { return false; }
}

std::string Localise(const std::string& key) { return key; }   // identity until locale tables are bound

namespace input {
    bool Pressed(Button) { return false; }
    bool Held(Button)    { return false; }
}

namespace app {
    double Time()        { return ImGui::GetTime(); }
    bool   IsInstaller() { return false; }
}

} // namespace sgfx
