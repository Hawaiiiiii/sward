// DECOUPLED from UnleashedRecomp/ui/options_menu_thumbnails.cpp. The game runtime
// (decompressor / hid / the ~50 embedded res/images thumbnails / gpu/video's
// GuestTexture) is replaced by the lib's shim + render contract:
//   - GuestTexture*                  -> sgfx::render::Texture*
//   - LOAD_ZSTD_TEXTURE(g_<name>)    -> sgfx::render::LoadUISprite(UISprite::Thumb<Name>)
// The Config::/enum read sites, IConfigDef::GetValue, hid::g_inputDeviceController and
// the whole map/lookup logic are the recomp's code, byte-for-byte (they resolve to the
// shim under the same names).
#include "options_menu_thumbnails.h"
#include "../platform/sgfx_platform.h"
#include "../render/sgfx_render.h"

#include <memory>
#include <type_traits>
#include <unordered_map>

#define VALUE_THUMBNAIL_MAP(type) std::unordered_map<type, std::unique_ptr<sgfx::render::Texture>>

static std::unique_ptr<sgfx::render::Texture> g_defaultThumbnail;

static std::unique_ptr<sgfx::render::Texture> g_controlTutorialXBThumbnail;
static std::unique_ptr<sgfx::render::Texture> g_controlTutorialPSThumbnail;
static std::unique_ptr<sgfx::render::Texture> g_vibrationXBThumbnail;
static std::unique_ptr<sgfx::render::Texture> g_vibrationPSThumbnail;
static std::unique_ptr<sgfx::render::Texture> g_backgroundInputXBThumbnail;
static std::unique_ptr<sgfx::render::Texture> g_backgroundInputPSThumbnail;

static std::unordered_map<const IConfigDef*, std::unique_ptr<sgfx::render::Texture>> g_configThumbnails;

static VALUE_THUMBNAIL_MAP(ETimeOfDayTransition) g_timeOfDayTransitionThumbnails;
static VALUE_THUMBNAIL_MAP(EChannelConfiguration) g_channelConfigurationThumbnails;
static VALUE_THUMBNAIL_MAP(EAntiAliasing) g_msaaAntiAliasingThumbnails;
static VALUE_THUMBNAIL_MAP(bool) g_vsyncThumbnails;
static VALUE_THUMBNAIL_MAP(bool) g_transparencyAntiAliasingThumbnails;
static VALUE_THUMBNAIL_MAP(EShadowResolution) g_shadowResolutionThumbnails;
static VALUE_THUMBNAIL_MAP(EGITextureFiltering) g_giTextureFilteringThumbnails;
static VALUE_THUMBNAIL_MAP(EMotionBlur) g_motionBlurThumbnails;
static VALUE_THUMBNAIL_MAP(bool) g_xboxColorCorrectionThumbnails;
static VALUE_THUMBNAIL_MAP(ECutsceneAspectRatio) g_cutsceneAspectRatioThumbnails;
static VALUE_THUMBNAIL_MAP(EUIAlignmentMode) g_uiAlignmentThumbnails;

void LoadThumbnails()
{
    g_defaultThumbnail = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbDefault);

    g_controlTutorialXBThumbnail = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbControlTutorialXB);
    g_controlTutorialPSThumbnail = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbControlTutorialPS);
    g_vibrationXBThumbnail = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbVibrationXB);
    g_vibrationPSThumbnail = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbVibrationPS);
    g_backgroundInputXBThumbnail = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbAllowBackgroundInputXB);
    g_backgroundInputPSThumbnail = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbAllowBackgroundInputPS);

    g_configThumbnails[&Config::Language] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbLanguage);
    g_configThumbnails[&Config::VoiceLanguage] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbVoiceLanguage);
    g_configThumbnails[&Config::Hints] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbHints);
    g_configThumbnails[&Config::AchievementNotifications] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbAchievementNotifications);

    g_timeOfDayTransitionThumbnails[ETimeOfDayTransition::Xbox] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbTimeTransitionXB);
    g_timeOfDayTransitionThumbnails[ETimeOfDayTransition::PlayStation] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbTimeTransitionPS);

    g_configThumbnails[&Config::HorizontalCamera] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbHorizontalCamera);
    g_configThumbnails[&Config::VerticalCamera] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbVerticalCamera);
    g_configThumbnails[&Config::ControllerIcons] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbControllerIcons);
    g_configThumbnails[&Config::MasterVolume] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbMasterVolume);
    g_configThumbnails[&Config::MusicVolume] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbMusicVolume);
    g_configThumbnails[&Config::EffectsVolume] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbEffectsVolume);

    g_channelConfigurationThumbnails[EChannelConfiguration::Stereo] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbChannelStereo);
    g_channelConfigurationThumbnails[EChannelConfiguration::Surround] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbChannelSurround);

    g_configThumbnails[&Config::MusicAttenuation] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbMusicAttenuation);
    g_configThumbnails[&Config::BattleTheme] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbBattleTheme);
    g_configThumbnails[&Config::WindowSize] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbWindowSize);
    g_configThumbnails[&Config::Monitor] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbMonitor);
    g_configThumbnails[&Config::AspectRatio] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbAspectRatio);
    g_configThumbnails[&Config::Fullscreen] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbFullscreen);
    g_configThumbnails[&Config::XboxColorCorrection] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbXboxColorCorrection);

    g_vsyncThumbnails[false] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbVSyncOff);
    g_vsyncThumbnails[true] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbVSyncOn);

    g_configThumbnails[&Config::FPS] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbFPS);
    g_configThumbnails[&Config::Brightness] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbBrightness);

    g_msaaAntiAliasingThumbnails[EAntiAliasing::None] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbAntialiasingNone);
    g_msaaAntiAliasingThumbnails[EAntiAliasing::MSAA2x] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbAntialiasing2x);
    g_msaaAntiAliasingThumbnails[EAntiAliasing::MSAA4x] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbAntialiasing4x);
    g_msaaAntiAliasingThumbnails[EAntiAliasing::MSAA8x] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbAntialiasing8x);

    g_transparencyAntiAliasingThumbnails[false] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbTransparencyAntialiasingFalse);
    g_transparencyAntiAliasingThumbnails[true] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbTransparencyAntialiasingTrue);

    g_shadowResolutionThumbnails[EShadowResolution::x512] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbShadowResolutionX512);
    g_shadowResolutionThumbnails[EShadowResolution::x1024] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbShadowResolutionX1024);
    g_shadowResolutionThumbnails[EShadowResolution::x2048] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbShadowResolutionX2048);
    g_shadowResolutionThumbnails[EShadowResolution::x4096] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbShadowResolutionX4096);
    g_shadowResolutionThumbnails[EShadowResolution::x8192] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbShadowResolutionX8192);

    g_giTextureFilteringThumbnails[EGITextureFiltering::Bilinear] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbGITextureFilteringBilinear);
    g_giTextureFilteringThumbnails[EGITextureFiltering::Bicubic] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbGITextureFilteringBicubic);

    g_motionBlurThumbnails[EMotionBlur::Off] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbMotionBlurOff);
    g_motionBlurThumbnails[EMotionBlur::Original] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbMotionBlurOriginal);
    g_motionBlurThumbnails[EMotionBlur::Enhanced] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbMotionBlurEnhanced);

    g_cutsceneAspectRatioThumbnails[ECutsceneAspectRatio::Original] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbMovieScaleFit);
    g_cutsceneAspectRatioThumbnails[ECutsceneAspectRatio::Unlocked] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbMovieScaleFill);

    g_uiAlignmentThumbnails[EUIAlignmentMode::Centre] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbUIAlignmentCentre);
    g_uiAlignmentThumbnails[EUIAlignmentMode::Edge] = sgfx::render::LoadUISprite(sgfx::render::UISprite::ThumbUIAlignmentEdge);
}

template<typename T>
bool TryGetValueThumbnail(const IConfigDef* cfg, VALUE_THUMBNAIL_MAP(T)* thumbnails, sgfx::render::Texture** texture)
{
    if (!texture)
        return false;

    if (!cfg->GetValue())
        return false;

    T value = *(T*)cfg->GetValue();

    if constexpr (std::is_same_v<T, EShadowResolution>)
    {
        if (value == EShadowResolution::Original)
            value = EShadowResolution::x1024;
    }

    auto findResult = thumbnails->find(value);

    if (findResult != thumbnails->end())
    {
        *texture = findResult->second.get();
        return true;
    }

    return false;
}

sgfx::render::Texture* GetThumbnail(const IConfigDef* cfg)
{
    auto findResult = g_configThumbnails.find(cfg);
    if (findResult == g_configThumbnails.end())
    {
        auto texture = g_defaultThumbnail.get();

        bool isPlayStation = Config::ControllerIcons == EControllerIcons::PlayStation;

        if (Config::ControllerIcons == EControllerIcons::Auto)
            isPlayStation = hid::g_inputDeviceController == hid::EInputDevice::PlayStation;

        if (cfg == &Config::ControlTutorial)
        {
            texture = isPlayStation ? g_controlTutorialPSThumbnail.get() : g_controlTutorialXBThumbnail.get();
        }
        else if (cfg == &Config::Vibration)
        {
            texture = isPlayStation ? g_vibrationPSThumbnail.get() : g_vibrationXBThumbnail.get();
        }
        else if (cfg == &Config::AllowBackgroundInput)
        {
            texture = isPlayStation ? g_backgroundInputPSThumbnail.get() : g_backgroundInputXBThumbnail.get();
        }
        else if (cfg == &Config::TimeOfDayTransition)
        {
            TryGetValueThumbnail<ETimeOfDayTransition>(cfg, &g_timeOfDayTransitionThumbnails, &texture);
        }
        else if (cfg == &Config::AntiAliasing)
        {
            TryGetValueThumbnail<EAntiAliasing>(cfg, &g_msaaAntiAliasingThumbnails, &texture);
        }
        else if (cfg == &Config::TransparencyAntiAliasing)
        {
            TryGetValueThumbnail<bool>(cfg, &g_transparencyAntiAliasingThumbnails, &texture);
        }
        else if (cfg == &Config::ShadowResolution)
        {
            TryGetValueThumbnail<EShadowResolution>(cfg, &g_shadowResolutionThumbnails, &texture);
        }
        else if (cfg == &Config::GITextureFiltering)
        {
            TryGetValueThumbnail<EGITextureFiltering>(cfg, &g_giTextureFilteringThumbnails, &texture);
        }
        else if (cfg == &Config::MotionBlur)
        {
            TryGetValueThumbnail<EMotionBlur>(cfg, &g_motionBlurThumbnails, &texture);
        }
        else if (cfg == &Config::XboxColorCorrection)
        {
            TryGetValueThumbnail<bool>(cfg, &g_xboxColorCorrectionThumbnails, &texture);
        }
        else if (cfg == &Config::CutsceneAspectRatio)
        {
            TryGetValueThumbnail<ECutsceneAspectRatio>(cfg, &g_cutsceneAspectRatioThumbnails, &texture);
        }
        else if (cfg == &Config::VSync)
        {
            TryGetValueThumbnail<bool>(cfg, &g_vsyncThumbnails, &texture);
        }
        else if (cfg == &Config::ChannelConfiguration)
        {
            TryGetValueThumbnail<EChannelConfiguration>(cfg, &g_channelConfigurationThumbnails, &texture);
        }
        else if (cfg == &Config::UIAlignmentMode)
        {
            TryGetValueThumbnail<EUIAlignmentMode>(cfg, &g_uiAlignmentThumbnails, &texture);
        }

        return texture;
    }

    return findResult->second.get();
}
