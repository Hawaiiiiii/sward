// Default standalone implementation of the sgfx_platform + sgfx_config shim — enough to
// build a preview harness with no host project. A real host (SGFX) replaces this TU with
// its own bindings (config store + load/save, locale tables, input, audio, display).
#include "sgfx_platform.h"
#include <unordered_map>

// ---- aspect / scale (host updates from the viewport each frame) -------------
float g_aspectRatio        = WIDE_ASPECT_RATIO;
float g_aspectRatioScale   = 1.0f;
float g_aspectRatioNarrowScale = 0.0f;
float g_aspectRatioOffsetX = 0.0f;
float g_aspectRatioOffsetY = 0.0f;

// ---- input ------------------------------------------------------------------
namespace hid {
    EInputDevice g_inputDeviceController = EInputDevice::Xbox;
    EInputDevice g_inputDevice           = EInputDevice::Xbox;
    bool IsInputAllowed()         { return true; }
    bool IsInputDeviceController() { return g_inputDevice == EInputDevice::Xbox || g_inputDevice == EInputDevice::PlayStation; }
    void SetProhibitedInputs(uint32_t) {}
}

// ---- version / locale -------------------------------------------------------
const char* g_versionString = "sgfx_recomp_ui";
std::string g_localeMissing = "<missing string>";

std::string& Localise(const std::string_view& key)
{
    // identity-by-reference: stable storage per key so the returned ref outlives the call
    static std::unordered_map<std::string, std::string> cache;
    auto& s = cache[std::string(key)];
    if (s.empty()) s = std::string(key);
    return s;
}

// ---- config -----------------------------------------------------------------
std::string ConfigLocalise(std::string_view name, std::string_view /*kind*/, ELanguage)
{
    return std::string(name);   // host binds the real localised labels
}
namespace Config { void Save() {} }

// ---- audio ------------------------------------------------------------------
void Game_PlaySound(const char*) {}   // host routes to its SFX system
