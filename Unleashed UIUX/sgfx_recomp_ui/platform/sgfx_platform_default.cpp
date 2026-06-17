// Default standalone implementation of the sgfx_platform + sgfx_config shim — enough to
// build a preview harness with no host project. A real host (SGFX) replaces this TU with
// its own bindings (config store + load/save, locale tables, input, audio, display).
#include "sgfx_platform.h"
#include "sgfx_sdl.h"
#include "../render/sgfx_render.h"   // sgfx::render::Texture (g_xdbfTextureCache)
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

// ---- app lifecycle ----------------------------------------------------------
void App::Restart(std::vector<std::string>) {}
void App::Exit() {}

// ---- SDL event hub ----------------------------------------------------------
std::vector<ISDLEventListener*>& GetEventListeners()
{
    static std::vector<ISDLEventListener*> listeners;
    return listeners;
}

// ---- audio ------------------------------------------------------------------
void Game_PlaySound(const char*) {}   // host routes to its SFX system

// ---- in-game input facade (neutral by default; host feeds real pad state) ---
namespace SWA {
    bool SPadState::IsDown(eKeyState) const     { return false; }
    bool SPadState::IsTapped(eKeyState) const   { return false; }
    bool SPadState::IsReleased(eKeyState) const { return false; }
    CInputState* CInputState::GetInstance()     { static CInputState s; return &s; }
    SPadState&   CInputState::GetPadState()      { static SPadState s; return s; }
    static bool  s_renderHud = true;
    bool* SGlobals::ms_IsRenderHud = &s_renderHud;
}

// ---- achievements DB (empty demo provider; host binds the real XDBF) --------
XdbfWrapper g_xdbfWrapper;
Achievement              XdbfWrapper::GetAchievement(EXDBFLanguage, uint16_t id) { Achievement a; a.ID = id; a.Name = "Achievement"; return a; }
std::vector<Achievement> XdbfWrapper::GetAchievements(EXDBFLanguage)             { return {}; }
namespace xdbf { std::string FixInvalidSequences(const std::string& s) { return s; } }
namespace AchievementManager {
    bool    IsUnlocked(uint16_t)   { return false; }
    int64_t GetTimestamp(uint16_t) { return 0; }
    int     GetTotalRecords()      { return 0; }
}
std::unordered_map<uint16_t, sgfx::render::Texture*> g_xdbfTextureCache;
