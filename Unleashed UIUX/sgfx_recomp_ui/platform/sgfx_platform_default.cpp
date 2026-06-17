// Default standalone implementation of the sgfx_platform shim — enough to build a
// preview harness with no host project. A real host (SGFX) replaces this TU with its
// own bindings (its config store, its locale, its real aspect/scale each frame).
#include "sgfx_platform.h"

float g_aspectRatio      = WIDE_ASPECT_RATIO;   // host updates from the viewport each frame
float g_aspectRatioScale = 1.0f;                // host derives from resolution

const char* g_versionString = "sgfx_recomp_ui";

std::string Localise(const std::string& key) { return key; }   // identity until locale tables are bound
