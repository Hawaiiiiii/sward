namespace Sward.UiRuntime.Reference;

public enum ReferenceProfile
{
    PauseMenu,
    TitleMenu,
    AutosaveToast,
    LoadingTransition,
    MissionResult,
    SubtitleCutscene,
    WorldMap,
    SonicStageHud,
    WerehogStageHud,
    ExtraStageHud,
    SuperSonicHud,
    BossHud,
    TownUi,
    CameraShell,
    ApplicationWorldShell,
    FrontendSequenceShell,
    AchievementUnlockSupport,
    AudioCueSupport,
    XmlDataLoadingSupport,
}

public static class ReferenceProfiles
{
    public static string BundledPath(ReferenceProfile profile) =>
        Path.Combine(AppContext.BaseDirectory, "contracts", profile switch
        {
            ReferenceProfile.PauseMenu => "pause_menu_reference.json",
            ReferenceProfile.TitleMenu => "title_menu_reference.json",
            ReferenceProfile.AutosaveToast => "autosave_toast_reference.json",
            ReferenceProfile.LoadingTransition => "loading_transition_reference.json",
            ReferenceProfile.MissionResult => "mission_result_reference.json",
            ReferenceProfile.SubtitleCutscene => "subtitle_cutscene_reference.json",
            ReferenceProfile.WorldMap => "world_map_reference.json",
            ReferenceProfile.SonicStageHud => "sonic_stage_hud_reference.json",
            ReferenceProfile.WerehogStageHud => "werehog_stage_hud_reference.json",
            ReferenceProfile.ExtraStageHud => "extra_stage_hud_reference.json",
            ReferenceProfile.SuperSonicHud => "super_sonic_hud_reference.json",
            ReferenceProfile.BossHud => "boss_hud_reference.json",
            ReferenceProfile.TownUi => "town_ui_reference.json",
            ReferenceProfile.CameraShell => "camera_shell_reference.json",
            ReferenceProfile.ApplicationWorldShell => "application_world_shell_reference.json",
            ReferenceProfile.FrontendSequenceShell => "frontend_sequence_shell_reference.json",
            ReferenceProfile.AchievementUnlockSupport => "achievement_unlock_support_reference.json",
            ReferenceProfile.AudioCueSupport => "audio_cue_support_reference.json",
            ReferenceProfile.XmlDataLoadingSupport => "xml_data_loading_support_reference.json",
            _ => throw new InvalidOperationException("Unknown reference profile."),
        });

    public static ScreenContract Load(ReferenceProfile profile) =>
        ContractLoader.LoadFromFile(BundledPath(profile));
}
