// Default standalone implementation of the sgfx_platform + sgfx_config shim — enough to
// build a preview harness with no host project. A real host (SGFX) replaces this TU with
// its own bindings (config store + load/save, locale tables, input, audio, display).
#include "sgfx_platform.h"
#include "sgfx_sdl.h"
#include "../render/sgfx_render.h"   // sgfx::render::Texture (g_xdbfTextureCache)
#include <unordered_map>
#include <cctype>

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
    void SetProhibitedInputs(uint16_t, bool, bool) {}
}
namespace AudioPatches { bool CanAttenuate() { return true; } }

// ---- version / locale -------------------------------------------------------
const char* g_versionString = "sgfx_recomp_ui";
std::string g_localeMissing = "<missing string>";

std::string& Localise(const std::string_view& key)
{
    // demo locale: make the keys readable (a host binds the real localised tables).
    static std::unordered_map<std::string, std::string> cache;
    auto it = cache.find(std::string(key));
    if (it != cache.end()) return it->second;
    std::string k(key), v = k;
    if      (k == "Options_Header_Name")  v = "OPTIONS";
    else if (k.rfind("Options_Category_", 0) == 0) v = k.substr(17);   // -> System / Input / Audio / Video
    else if (k.rfind("Options_Name_", 0) == 0)     v = k.substr(13);
    else { auto p = k.rfind('_'); if (p != std::string::npos) v = k.substr(p + 1); }   // last segment
    return cache.emplace(std::move(k), std::move(v)).first->second;
}

// ---- config -----------------------------------------------------------------
// A real host binds localised tables keyed by (option, value). The demo returns readable
// placeholders so the preview reads like the game: spaced names + realistic default values.
static std::string SpaceCamel(std::string_view s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (i && std::isupper((unsigned char)s[i]) && !std::isupper((unsigned char)s[i - 1])) out += ' ';
        out += s[i];
    }
    return out;
}
std::string ConfigLocalise(std::string_view name, std::string_view kind, ELanguage)
{
    static const std::unordered_map<std::string, std::string> values = {
        {"Language","English"}, {"VoiceLanguage","English"}, {"Subtitles","On"}, {"Hints","On"},
        {"ControlTutorial","On"}, {"AchievementNotifications","On"}, {"TimeOfDayTransition","Xbox"},
        {"HorizontalCamera","Normal"}, {"VerticalCamera","Normal"}, {"Vibration","On"},
        {"AllowBackgroundInput","Off"}, {"ControllerIcons","Auto"}, {"MasterVolume","100%"},
        {"MusicVolume","100%"}, {"EffectsVolume","100%"}, {"ChannelConfiguration","Stereo"},
        {"MusicAttenuation","Off"}, {"BattleTheme","On"}, {"WindowSize","1280 x 720"}, {"Monitor","1"},
        {"AspectRatio","Wide"}, {"ResolutionScale","100%"}, {"Fullscreen","Off"}, {"VSync","On"},
        {"FPS","60"}, {"Brightness","50%"}, {"AntiAliasing","MSAA 4x"}, {"TransparencyAntiAliasing","On"},
        {"ShadowResolution","4096"}, {"GITextureFiltering","Bilinear"}, {"MotionBlur","Original"},
        {"XboxColorCorrection","Off"}, {"CutsceneAspectRatio","Original"}, {"UIAlignmentMode","Edge"},
    };
    std::string n(name);
    if (kind == "value")    { auto it = values.find(n); return it != values.end() ? it->second : "On"; }
    if (kind == "desc")     return "Adjusts the " + SpaceCamel(name) + " setting.";
    if (kind == "valuedesc") return std::string();
    return SpaceCamel(name);   // "name": spaced camelCase, e.g. "Voice Language"
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
