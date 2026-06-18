// =============================================================================
// sgfx_config.h — the Config reflection system + option enums (the customization
// surface). Modelled on UnleashedRecomp/user/config.h's IConfigDef + ConfigDef<T>,
// simplified to drop the toml/locale serialization (host owns load/save + localised
// label text). Every option is a ConfigDef<T> object: menus take &Config::Field as
// const IConfigDef*, read ->GetValue()/->Value, and rely on operator T() so
// `Config::Foo == Bar` and `if (Config::SomeBool)` stay byte-identical.
//
// THESE FIELDS ARE THE TEMPLATE KNOBS. A host (SGFX) reads/writes Config::X.Value and
// binds load/save + the localised strings + the change callbacks.
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <set>
#include <map>
#include <functional>

// ---- option enums (were user/config.h / locale.h) ---------------------------
enum class ELanguage : uint32_t { English = 1, Japanese, German, French, Spanish, Italian };
enum class EVoiceLanguage : uint32_t { English = 1, Japanese };
enum class ETimeOfDayTransition : uint32_t { Xbox, PlayStation };
enum class ECameraRotationMode : uint32_t { Normal, Reverse };
enum class EChannelConfiguration : uint32_t { Stereo, Surround };
enum class EAntiAliasing : uint32_t { None = 0, MSAA2x = 2, MSAA4x = 4, MSAA8x = 8 };
enum class EShadowResolution : int32_t { Original = -1, x512 = 512, x1024 = 1024, x2048 = 2048, x4096 = 4096, x8192 = 8192 };
enum class EGITextureFiltering : uint32_t { Bilinear, Bicubic };
enum class EMotionBlur : uint32_t { Off, Original, Enhanced };
enum class ECutsceneAspectRatio : uint32_t { Original, Unlocked };
enum class EUIAlignmentMode : uint32_t { Edge, Centre };
enum class EWindowState : uint32_t { Normal, Maximised };
enum class EAspectRatio : uint32_t { Auto, Wide, Narrow, OriginalWide, OriginalNarrow };
enum class EControllerIcons : uint32_t { Auto, Xbox, PlayStation };

// ---- the config-definition interface (was user/config.h) --------------------
class IConfigDef
{
public:
    virtual ~IConfigDef() = default;
    virtual void MakeDefault() = 0;
    virtual bool IsDefaultValue() const = 0;
    virtual const void* GetValue() const = 0;
    virtual std::string_view GetName() const = 0;
    virtual std::string GetNameLocalised(ELanguage) const = 0;
    virtual std::string GetDescription(ELanguage) const = 0;
    virtual std::string GetValueLocalised(ELanguage) const = 0;
    virtual std::string GetValueDescription(ELanguage) const = 0;
    virtual void SnapToNearestAccessibleValue(bool searchUp) = 0;
};

// host hook: localised label/desc lookup for a config field+value (default = name/blank)
std::string ConfigLocalise(std::string_view name, std::string_view kind, ELanguage);

template<typename T>
class ConfigDef final : public IConfigDef
{
public:
    std::string_view Name{};
    T DefaultValue{};
    T Value{};
    std::set<T> InaccessibleValues{};
    // enum value -> localised label, in display order (host populates for enum options;
    // the menu cycles it with ++/-- to switch the value). Empty for bool/int/float.
    std::map<T, std::string> EnumTemplateReverse{};
    std::function<void(ConfigDef<T>*)> Callback;
    std::function<void(ConfigDef<T>*)> LockCallback;
    std::function<void(ConfigDef<T>*)> ApplyCallback;

    ConfigDef() = default;
    explicit ConfigDef(std::string_view name, T def) : Name(name), DefaultValue(def), Value(def) {}

    // implicit read conversion (by value, matching the recomp) — keeps `Config::Foo == Bar`
    // and `if (Config::Bool)` byte-identical
    operator T() const { return Value; }
    void operator=(const T& v) { Value = v; }

    void MakeDefault() override { Value = DefaultValue; }
    bool IsDefaultValue() const override { return Value == DefaultValue; }
    const void* GetValue() const override { return &Value; }
    std::string_view GetName() const override { return Name; }
    std::string GetNameLocalised(ELanguage l) const override { return ConfigLocalise(Name, "name", l); }
    std::string GetDescription(ELanguage l) const override { return ConfigLocalise(Name, "desc", l); }
    std::string GetValueLocalised(ELanguage l) const override { return ConfigLocalise(Name, "value", l); }
    std::string GetValueDescription(ELanguage l) const override { return ConfigLocalise(Name, "valuedesc", l); }
    void SnapToNearestAccessibleValue(bool) override {}   // host overrides for inaccessible-value skipping
};

// ---- the option set (the customizable Unleashed UI settings) -----------------
// inline objects -> one shared instance across TUs (pointer identity holds, which the
// options_menu <-> thumbnails address-keying relies on).
namespace Config
{
    inline ConfigDef<ELanguage>            Language{ "Language", ELanguage::English };
    inline ConfigDef<EVoiceLanguage>       VoiceLanguage{ "VoiceLanguage", EVoiceLanguage::English };
    inline ConfigDef<bool>                 Subtitles{ "Subtitles", true };
    inline ConfigDef<bool>                 Hints{ "Hints", true };
    inline ConfigDef<bool>                 ControlTutorial{ "ControlTutorial", true };
    inline ConfigDef<bool>                 AchievementNotifications{ "AchievementNotifications", true };
    inline ConfigDef<ETimeOfDayTransition> TimeOfDayTransition{ "TimeOfDayTransition", ETimeOfDayTransition::Xbox };
    inline ConfigDef<bool>                 UseArrowsForTimeOfDayTransition{ "UseArrowsForTimeOfDayTransition", false };
    inline ConfigDef<bool>                 ShowConsole{ "ShowConsole", false };
    inline ConfigDef<ECameraRotationMode>  HorizontalCamera{ "HorizontalCamera", ECameraRotationMode::Normal };
    inline ConfigDef<ECameraRotationMode>  VerticalCamera{ "VerticalCamera", ECameraRotationMode::Normal };
    inline ConfigDef<bool>                 Vibration{ "Vibration", true };
    inline ConfigDef<bool>                 AllowBackgroundInput{ "AllowBackgroundInput", false };
    inline ConfigDef<float>                MasterVolume{ "MasterVolume", 1.0f };
    inline ConfigDef<float>                MusicVolume{ "MusicVolume", 1.0f };
    inline ConfigDef<float>                EffectsVolume{ "EffectsVolume", 1.0f };
    inline ConfigDef<EChannelConfiguration> ChannelConfiguration{ "ChannelConfiguration", EChannelConfiguration::Stereo };
    inline ConfigDef<bool>                 MusicAttenuation{ "MusicAttenuation", false };
    inline ConfigDef<bool>                 BattleTheme{ "BattleTheme", true };
    inline ConfigDef<int>                  WindowSize{ "WindowSize", -1 };
    inline ConfigDef<int>                  Monitor{ "Monitor", 0 };
    inline ConfigDef<float>                ResolutionScale{ "ResolutionScale", 1.0f };
    inline ConfigDef<bool>                 Fullscreen{ "Fullscreen", false };
    inline ConfigDef<bool>                 VSync{ "VSync", true };
    inline ConfigDef<int>                  FPS{ "FPS", 60 };
    inline ConfigDef<float>                Brightness{ "Brightness", 0.5f };
    inline ConfigDef<EAntiAliasing>        AntiAliasing{ "AntiAliasing", EAntiAliasing::MSAA4x };
    inline ConfigDef<bool>                 TransparencyAntiAliasing{ "TransparencyAntiAliasing", true };
    inline ConfigDef<EShadowResolution>    ShadowResolution{ "ShadowResolution", EShadowResolution::x4096 };
    inline ConfigDef<EGITextureFiltering>  GITextureFiltering{ "GITextureFiltering", EGITextureFiltering::Bilinear };
    inline ConfigDef<EMotionBlur>          MotionBlur{ "MotionBlur", EMotionBlur::Original };
    inline ConfigDef<bool>                 XboxColorCorrection{ "XboxColorCorrection", false };
    inline ConfigDef<ECutsceneAspectRatio> CutsceneAspectRatio{ "CutsceneAspectRatio", ECutsceneAspectRatio::Original };
    inline ConfigDef<EUIAlignmentMode>     UIAlignmentMode{ "UIAlignmentMode", EUIAlignmentMode::Edge };
    // window placement
    inline ConfigDef<EWindowState>         WindowState{ "WindowState", EWindowState::Normal };
    inline ConfigDef<int>                  WindowX{ "WindowX", -1 };
    inline ConfigDef<int>                  WindowY{ "WindowY", -1 };
    inline ConfigDef<int>                  WindowWidth{ "WindowWidth", 1280 };
    inline ConfigDef<int>                  WindowHeight{ "WindowHeight", 720 };
    inline ConfigDef<bool>                 UseOfficialTitleOnTitleBar{ "UseOfficialTitleOnTitleBar", false };
    inline ConfigDef<bool>                 UseAlternateTitle{ "UseAlternateTitle", false };
    inline ConfigDef<bool>                 DisableDWMRoundedCorners{ "DisableDWMRoundedCorners", false };
    // UI-render config used by the already-compiling support layer (imgui_utils/button_guide/tv_static)
    inline ConfigDef<EAspectRatio>         AspectRatio{ "AspectRatio", EAspectRatio::Wide };
    inline ConfigDef<EControllerIcons>     ControllerIcons{ "ControllerIcons", EControllerIcons::Auto };
    inline ConfigDef<bool>                 DisableLowResolutionFontOnCustomUI{ "DisableLowResolutionFontOnCustomUI", false };

    void Save();   // host-implemented persistence
}

static constexpr int32_t FPS_MIN = 15;
static constexpr int32_t FPS_MAX = 241;
