using System.Text.Json;
using System.Text.Json.Serialization;

namespace Sward.UiRuntime.Reference;

public static class ContractLoader
{
    public static ScreenContract LoadFromFile(string path)
    {
        using var stream = File.OpenRead(path);
        var dto = JsonSerializer.Deserialize<ScreenContractDto>(stream, SerializerOptions)
            ?? throw new InvalidOperationException("Failed to deserialize runtime contract.");

        return new ScreenContract
        {
            ScreenId = dto.ScreenId,
            TimelineBands = dto.TimelineBands.ToDictionary(item => item.Id, item => new TimelineBand(item.Id, item.Seconds)),
            States = dto.States.ToDictionary(
                item => ParseScreenState(item.State),
                item => new StateDefinition(
                    ParseScreenState(item.State),
                    item.DebugName,
                    item.EnterScene,
                    item.TimelineBandId,
                    item.TimeoutTarget is null ? null : ParseScreenState(item.TimeoutTarget),
                    item.InputEnabled)),
            OverlayLayers = dto.OverlayLayers.Select(item => new OverlayLayer(item.Id, item.Role, item.Interactive)).ToList(),
            VisibleOverlayRoles = dto.VisibleOverlayRoles.ToDictionary(
                item => ParseScreenState(item.Key),
                item => (IReadOnlySet<string>)new HashSet<string>(item.Value)),
            PromptSlots = dto.PromptSlots.Select(item =>
                new PromptSlot(
                    item.SlotId,
                    ParsePromptButton(item.Button),
                    item.Label,
                    new HashSet<ScreenState>(item.VisibleStates.Select(ParseScreenState)),
                    item.RequiredPredicates)).ToList(),
        };
    }

    private static ScreenState ParseScreenState(string value) =>
        Enum.Parse<ScreenState>(value, ignoreCase: false);

    private static PromptButton ParsePromptButton(string value) =>
        Enum.Parse<PromptButton>(value, ignoreCase: false);

    private static readonly JsonSerializerOptions SerializerOptions = new()
    {
        PropertyNameCaseInsensitive = false,
        ReadCommentHandling = JsonCommentHandling.Disallow,
    };

    private sealed class ScreenContractDto
    {
        [JsonPropertyName("screen_id")]
        public string ScreenId { get; set; } = string.Empty;

        [JsonPropertyName("timeline_bands")]
        public List<TimelineBandDto> TimelineBands { get; set; } = [];

        [JsonPropertyName("states")]
        public List<StateDefinitionDto> States { get; set; } = [];

        [JsonPropertyName("overlay_layers")]
        public List<OverlayLayerDto> OverlayLayers { get; set; } = [];

        [JsonPropertyName("visible_overlay_roles")]
        public Dictionary<string, List<string>> VisibleOverlayRoles { get; set; } = [];

        [JsonPropertyName("prompt_slots")]
        public List<PromptSlotDto> PromptSlots { get; set; } = [];
    }

    private sealed class TimelineBandDto
    {
        [JsonPropertyName("id")]
        public string Id { get; set; } = string.Empty;

        [JsonPropertyName("seconds")]
        public double Seconds { get; set; }
    }

    private sealed class StateDefinitionDto
    {
        [JsonPropertyName("state")]
        public string State { get; set; } = string.Empty;

        [JsonPropertyName("debug_name")]
        public string DebugName { get; set; } = string.Empty;

        [JsonPropertyName("enter_scene")]
        public string EnterScene { get; set; } = string.Empty;

        [JsonPropertyName("timeline_band_id")]
        public string? TimelineBandId { get; set; }

        [JsonPropertyName("timeout_target")]
        public string? TimeoutTarget { get; set; }

        [JsonPropertyName("input_enabled")]
        public bool InputEnabled { get; set; }
    }

    private sealed class OverlayLayerDto
    {
        [JsonPropertyName("id")]
        public string Id { get; set; } = string.Empty;

        [JsonPropertyName("role")]
        public string Role { get; set; } = string.Empty;

        [JsonPropertyName("interactive")]
        public bool Interactive { get; set; }
    }

    private sealed class PromptSlotDto
    {
        [JsonPropertyName("slot_id")]
        public string SlotId { get; set; } = string.Empty;

        [JsonPropertyName("button")]
        public string Button { get; set; } = string.Empty;

        [JsonPropertyName("label")]
        public string Label { get; set; } = string.Empty;

        [JsonPropertyName("visible_states")]
        public List<string> VisibleStates { get; set; } = [];

        [JsonPropertyName("required_predicates")]
        public List<string> RequiredPredicates { get; set; } = [];
    }
}
