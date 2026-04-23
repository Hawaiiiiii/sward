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
            _ => throw new InvalidOperationException("Unknown reference profile."),
        });

    public static ScreenContract Load(ReferenceProfile profile) =>
        ContractLoader.LoadFromFile(BundledPath(profile));
}
