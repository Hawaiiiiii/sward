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
    else if (k.find("_Uppercase") != std::string::npos)               // e.g. Achievements_Name_Uppercase
    {
        v = k.substr(0, k.find('_'));
        for (auto& c : v) c = (char)std::toupper((unsigned char)c);   // -> ACHIEVEMENTS
    }
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

// ---- achievements DB (demo provider; a host binds the real XDBF) ------------
// A representative list so the Achievements menu renders with content. The first
// DEMO_UNLOCKED are shown unlocked (with a timestamp); the rest are locked.
static constexpr int DEMO_UNLOCKED = 12;
static const std::vector<Achievement>& DemoAchievements()
{
    static const std::vector<Achievement> list = {
        {  1, "First Steps",        "Cleared the first stage.",            "Clear the first stage." },
        {  2, "Night of the Werehog","Transformed into the Werehog.",      "Transform into the Werehog." },
        {  3, "Apotos Adventurer",  "Cleared the Apotos area.",            "Clear the Apotos area." },
        {  4, "Spagonia Sightseer", "Cleared the Spagonia area.",          "Clear the Spagonia area." },
        {  5, "Mazuri Marathoner",  "Cleared the Mazuri area.",            "Clear the Mazuri area." },
        {  6, "Speed Demon",        "Finished a stage in record time.",    "Finish a stage quickly." },
        {  7, "Ring Leader",        "Collected 1000 rings.",               "Collect 1000 rings." },
        {  8, "Medal Collector",    "Found 50 Sun and Moon Medals.",       "Find 50 Sun and Moon Medals." },
        {  9, "Continental Champion","Cleared three continents.",          "Clear three continents." },
        { 10, "Day and Night",      "Played both day and night stages.",   "Play day and night stages." },
        { 11, "Gaia Guardian",      "Restored a Gaia Temple.",             "Restore a Gaia Temple." },
        { 12, "Combo Master",       "Reached a 100-hit combo.",            "Reach a 100-hit combo." },
        { 13, "World Traveler",     "Visited every continent.",            "Visit every continent." },
        { 14, "Perfect Run",        "Cleared a stage without damage.",     "Clear a stage without damage." },
        { 15, "S Rank",             "Earned an S Rank on any stage.",      "Earn an S Rank on any stage." },
        { 16, "Eggmanland",         "Cleared Eggmanland.",                 "Clear Eggmanland." },
        { 17, "Gaia Unleashed",     "Defeated Dark Gaia.",                 "Defeat Dark Gaia." },
        { 18, "Completionist",      "Earned every other achievement.",     "Earn every other achievement." },
    };
    return list;
}
XdbfWrapper g_xdbfWrapper;
Achievement XdbfWrapper::GetAchievement(EXDBFLanguage, uint16_t id)
{
    for (auto& a : DemoAchievements()) if (a.ID == id) return a;
    Achievement a; a.ID = id; a.Name = "Achievement"; return a;
}
std::vector<Achievement> XdbfWrapper::GetAchievements(EXDBFLanguage)
{
    auto list = DemoAchievements();
    for (auto& a : list)   // give each achievement an icon tile so the menu draws it
        if (g_xdbfTextureCache.find(a.ID) == g_xdbfTextureCache.end())
            g_xdbfTextureCache[a.ID] = sgfx::render::LoadUISprite(sgfx::render::UISprite::Trophy).release();
    return list;
}
namespace xdbf { std::string FixInvalidSequences(const std::string& s) { return s; } }
namespace AchievementManager {
    bool    IsUnlocked(uint16_t id)   { return id <= DEMO_UNLOCKED; }
    int64_t GetTimestamp(uint16_t id) { return IsUnlocked(id) ? (int64_t)1700000000 : 0; }
    int     GetTotalRecords()         { return DEMO_UNLOCKED; }
}
std::unordered_map<uint16_t, sgfx::render::Texture*> g_xdbfTextureCache;
