#include <sward/ui_runtime/contract_loader.hpp>
#include <sward/ui_runtime/profiles.hpp>

#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

namespace sward::ui_runtime
{
namespace
{
struct JsonValue;
using JsonObject = std::map<std::string, JsonValue>;
using JsonArray = std::vector<JsonValue>;

struct JsonValue
{
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, JsonObject, JsonArray>;

    Storage storage = nullptr;

    [[nodiscard]] bool isNull() const { return std::holds_alternative<std::nullptr_t>(storage); }
    [[nodiscard]] bool isBool() const { return std::holds_alternative<bool>(storage); }
    [[nodiscard]] bool isNumber() const { return std::holds_alternative<double>(storage); }
    [[nodiscard]] bool isString() const { return std::holds_alternative<std::string>(storage); }
    [[nodiscard]] bool isObject() const { return std::holds_alternative<JsonObject>(storage); }
    [[nodiscard]] bool isArray() const { return std::holds_alternative<JsonArray>(storage); }

    [[nodiscard]] const bool& asBool() const { return std::get<bool>(storage); }
    [[nodiscard]] const double& asNumber() const { return std::get<double>(storage); }
    [[nodiscard]] const std::string& asString() const { return std::get<std::string>(storage); }
    [[nodiscard]] const JsonObject& asObject() const { return std::get<JsonObject>(storage); }
    [[nodiscard]] const JsonArray& asArray() const { return std::get<JsonArray>(storage); }
};

class JsonParser
{
public:
    explicit JsonParser(std::string_view input)
        : m_input(input)
    {
    }

    [[nodiscard]] JsonValue parse()
    {
        skipWhitespace();
        JsonValue value = parseValue();
        skipWhitespace();
        if (m_pos != m_input.size())
            throw std::runtime_error("Unexpected trailing JSON content.");
        return value;
    }

private:
    void skipWhitespace()
    {
        while (m_pos < m_input.size() && std::isspace(static_cast<unsigned char>(m_input[m_pos])))
            ++m_pos;
    }

    [[nodiscard]] char peek() const
    {
        if (m_pos >= m_input.size())
            throw std::runtime_error("Unexpected end of JSON input.");
        return m_input[m_pos];
    }

    [[nodiscard]] char consume()
    {
        char value = peek();
        ++m_pos;
        return value;
    }

    void expect(char expected)
    {
        if (consume() != expected)
            throw std::runtime_error("Unexpected JSON token.");
    }

    [[nodiscard]] JsonValue parseValue()
    {
        skipWhitespace();
        switch (peek())
        {
        case '{':
            return parseObject();
        case '[':
            return parseArray();
        case '"':
            return JsonValue{ parseString() };
        case 't':
            consumeLiteral("true");
            return JsonValue{ true };
        case 'f':
            consumeLiteral("false");
            return JsonValue{ false };
        case 'n':
            consumeLiteral("null");
            return JsonValue{};
        default:
            return JsonValue{ parseNumber() };
        }
    }

    [[nodiscard]] JsonValue parseObject()
    {
        expect('{');
        skipWhitespace();
        JsonObject object;
        if (peek() == '}')
        {
            (void)consume();
            return JsonValue{ object };
        }

        while (true)
        {
            skipWhitespace();
            std::string key = parseString();
            skipWhitespace();
            expect(':');
            skipWhitespace();
            object.emplace(std::move(key), parseValue());
            skipWhitespace();

            char token = consume();
            if (token == '}')
                break;
            if (token != ',')
                throw std::runtime_error("Expected ',' or '}' in JSON object.");
        }

        return JsonValue{ object };
    }

    [[nodiscard]] JsonValue parseArray()
    {
        expect('[');
        skipWhitespace();
        JsonArray array;
        if (peek() == ']')
        {
            (void)consume();
            return JsonValue{ array };
        }

        while (true)
        {
            skipWhitespace();
            array.push_back(parseValue());
            skipWhitespace();
            char token = consume();
            if (token == ']')
                break;
            if (token != ',')
                throw std::runtime_error("Expected ',' or ']' in JSON array.");
        }

        return JsonValue{ array };
    }

    [[nodiscard]] std::string parseString()
    {
        expect('"');
        std::string result;

        while (true)
        {
            char token = consume();
            if (token == '"')
                break;
            if (token == '\\')
            {
                char escaped = consume();
                switch (escaped)
                {
                case '"':
                case '\\':
                case '/':
                    result.push_back(escaped);
                    break;
                case 'b':
                    result.push_back('\b');
                    break;
                case 'f':
                    result.push_back('\f');
                    break;
                case 'n':
                    result.push_back('\n');
                    break;
                case 'r':
                    result.push_back('\r');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                case 'u':
                    result.push_back(parseUnicodeEscape());
                    break;
                default:
                    throw std::runtime_error("Unsupported JSON escape sequence.");
                }
                continue;
            }
            result.push_back(token);
        }

        return result;
    }

    [[nodiscard]] char parseUnicodeEscape()
    {
        std::string digits;
        for (int i = 0; i < 4; ++i)
            digits.push_back(consume());
        unsigned int value = std::stoul(digits, nullptr, 16);
        if (value <= 0x7F)
            return static_cast<char>(value);
        return '?';
    }

    [[nodiscard]] double parseNumber()
    {
        std::size_t start = m_pos;
        if (peek() == '-')
            ++m_pos;
        while (m_pos < m_input.size() && std::isdigit(static_cast<unsigned char>(m_input[m_pos])))
            ++m_pos;
        if (m_pos < m_input.size() && m_input[m_pos] == '.')
        {
            ++m_pos;
            while (m_pos < m_input.size() && std::isdigit(static_cast<unsigned char>(m_input[m_pos])))
                ++m_pos;
        }
        if (m_pos < m_input.size() && (m_input[m_pos] == 'e' || m_input[m_pos] == 'E'))
        {
            ++m_pos;
            if (m_pos < m_input.size() && (m_input[m_pos] == '+' || m_input[m_pos] == '-'))
                ++m_pos;
            while (m_pos < m_input.size() && std::isdigit(static_cast<unsigned char>(m_input[m_pos])))
                ++m_pos;
        }

        return std::stod(std::string(m_input.substr(start, m_pos - start)));
    }

    void consumeLiteral(std::string_view literal)
    {
        for (char expected : literal)
        {
            if (consume() != expected)
                throw std::runtime_error("Unexpected JSON literal.");
        }
    }

    std::string_view m_input;
    std::size_t m_pos = 0;
};

[[nodiscard]] const JsonObject& requireObject(const JsonValue& value, std::string_view field)
{
    if (!value.isObject())
        throw std::runtime_error(std::string(field) + " must be a JSON object.");
    return value.asObject();
}

[[nodiscard]] const JsonArray& requireArray(const JsonValue& value, std::string_view field)
{
    if (!value.isArray())
        throw std::runtime_error(std::string(field) + " must be a JSON array.");
    return value.asArray();
}

[[nodiscard]] const std::string& requireString(const JsonObject& object, const char* field)
{
    auto found = object.find(field);
    if (found == object.end() || !found->second.isString())
        throw std::runtime_error(std::string("Missing or invalid string field: ") + field);
    return found->second.asString();
}

[[nodiscard]] std::optional<std::string> optionalString(const JsonObject& object, const char* field)
{
    auto found = object.find(field);
    if (found == object.end() || found->second.isNull())
        return std::nullopt;
    if (!found->second.isString())
        throw std::runtime_error(std::string("Expected string field: ") + field);
    return found->second.asString();
}

[[nodiscard]] bool requireBool(const JsonObject& object, const char* field)
{
    auto found = object.find(field);
    if (found == object.end() || !found->second.isBool())
        throw std::runtime_error(std::string("Missing or invalid bool field: ") + field);
    return found->second.asBool();
}

[[nodiscard]] double requireNumber(const JsonObject& object, const char* field)
{
    auto found = object.find(field);
    if (found == object.end() || !found->second.isNumber())
        throw std::runtime_error(std::string("Missing or invalid number field: ") + field);
    return found->second.asNumber();
}

[[nodiscard]] std::vector<std::string> requireStringArray(const JsonObject& object, const char* field)
{
    auto found = object.find(field);
    if (found == object.end())
        return {};
    const JsonArray& array = requireArray(found->second, field);
    std::vector<std::string> result;
    result.reserve(array.size());
    for (const auto& item : array)
    {
        if (!item.isString())
            throw std::runtime_error(std::string("Expected string array item in field: ") + field);
        result.push_back(item.asString());
    }
    return result;
}

[[nodiscard]] ScreenState parseScreenState(std::string_view value)
{
    if (value == "Boot")
        return ScreenState::Boot;
    if (value == "Intro")
        return ScreenState::Intro;
    if (value == "Idle")
        return ScreenState::Idle;
    if (value == "Navigate")
        return ScreenState::Navigate;
    if (value == "Confirm")
        return ScreenState::Confirm;
    if (value == "Cancel")
        return ScreenState::Cancel;
    if (value == "Outro")
        return ScreenState::Outro;
    if (value == "Closed")
        return ScreenState::Closed;
    throw std::runtime_error("Unknown ScreenState: " + std::string(value));
}

[[nodiscard]] PromptButton parsePromptButton(std::string_view value)
{
    if (value == "A")
        return PromptButton::A;
    if (value == "B")
        return PromptButton::B;
    if (value == "X")
        return PromptButton::X;
    if (value == "Y")
        return PromptButton::Y;
    if (value == "LB")
        return PromptButton::LB;
    if (value == "RB")
        return PromptButton::RB;
    if (value == "Start")
        return PromptButton::Start;
    if (value == "Unknown")
        return PromptButton::Unknown;
    throw std::runtime_error("Unknown PromptButton: " + std::string(value));
}

[[nodiscard]] std::filesystem::path defaultContractsDir()
{
#ifdef SWARD_UI_RUNTIME_CONTRACT_SOURCE_DIR
    return std::filesystem::path(SWARD_UI_RUNTIME_CONTRACT_SOURCE_DIR);
#else
    return std::filesystem::path("contracts");
#endif
}

[[nodiscard]] std::string contractFileName(ReferenceProfile profile)
{
    switch (profile)
    {
    case ReferenceProfile::PauseMenu:
        return "pause_menu_reference.json";
    case ReferenceProfile::TitleMenu:
        return "title_menu_reference.json";
    case ReferenceProfile::AutosaveToast:
        return "autosave_toast_reference.json";
    case ReferenceProfile::LoadingTransition:
        return "loading_transition_reference.json";
    case ReferenceProfile::MissionResult:
        return "mission_result_reference.json";
    case ReferenceProfile::WorldMap:
        return "world_map_reference.json";
    }

    throw std::runtime_error("Unknown reference profile.");
}
} // namespace

ScreenContract loadContractFromJsonFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Unable to open contract file: " + path.string());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    JsonValue rootValue = JsonParser(buffer.str()).parse();
    const JsonObject& root = requireObject(rootValue, "root");

    ScreenContract contract;
    contract.screenId = requireString(root, "screen_id");

    for (const auto& item : requireArray(root.at("timeline_bands"), "timeline_bands"))
    {
        const JsonObject& band = requireObject(item, "timeline_band");
        TimelineBand value;
        value.id = requireString(band, "id");
        value.seconds = requireNumber(band, "seconds");
        contract.timelineBands.emplace(value.id, value);
    }

    for (const auto& item : requireArray(root.at("states"), "states"))
    {
        const JsonObject& stateObject = requireObject(item, "state");
        StateDefinition definition;
        definition.state = parseScreenState(requireString(stateObject, "state"));
        definition.debugName = requireString(stateObject, "debug_name");
        definition.enterScene = requireString(stateObject, "enter_scene");
        definition.timelineBandId = optionalString(stateObject, "timeline_band_id");
        auto timeoutTarget = optionalString(stateObject, "timeout_target");
        if (timeoutTarget.has_value())
            definition.timeoutTarget = parseScreenState(*timeoutTarget);
        definition.inputEnabled = requireBool(stateObject, "input_enabled");
        contract.states.emplace(definition.state, definition);
    }

    for (const auto& item : requireArray(root.at("overlay_layers"), "overlay_layers"))
    {
        const JsonObject& layerObject = requireObject(item, "overlay_layer");
        OverlayLayer layer;
        layer.id = requireString(layerObject, "id");
        layer.role = requireString(layerObject, "role");
        layer.interactive = requireBool(layerObject, "interactive");
        contract.overlayLayers.push_back(layer);
    }

    const JsonObject& visibleRoles = requireObject(root.at("visible_overlay_roles"), "visible_overlay_roles");
    for (const auto& [stateName, values] : visibleRoles)
    {
        std::set<std::string> roles;
        for (const auto& item : requireArray(values, "visible_overlay_roles.<state>"))
        {
            if (!item.isString())
                throw std::runtime_error("Expected string in visible_overlay_roles array.");
            roles.insert(item.asString());
        }
        contract.visibleOverlayRoles.emplace(parseScreenState(stateName), std::move(roles));
    }

    for (const auto& item : requireArray(root.at("prompt_slots"), "prompt_slots"))
    {
        const JsonObject& promptObject = requireObject(item, "prompt_slot");
        PromptSlot prompt;
        prompt.slotId = requireString(promptObject, "slot_id");
        prompt.button = parsePromptButton(requireString(promptObject, "button"));
        prompt.label = requireString(promptObject, "label");

        for (const auto& stateName : requireStringArray(promptObject, "visible_states"))
            prompt.visibleStates.insert(parseScreenState(stateName));
        prompt.requiredPredicates = requireStringArray(promptObject, "required_predicates");
        contract.promptSlots.push_back(std::move(prompt));
    }

    return contract;
}

ScreenContract loadBundledContract(ReferenceProfile profile)
{
    return loadContractFromJsonFile(bundledContractPath(profile));
}

std::filesystem::path bundledContractPath(ReferenceProfile profile)
{
    return defaultContractsDir() / contractFileName(profile);
}

std::vector<std::filesystem::path> bundledContractPaths()
{
    std::vector<std::filesystem::path> result;
    std::filesystem::path root = defaultContractsDir();
    result.push_back(root / "pause_menu_reference.json");
    result.push_back(root / "title_menu_reference.json");
    result.push_back(root / "autosave_toast_reference.json");
    result.push_back(root / "loading_transition_reference.json");
    result.push_back(root / "mission_result_reference.json");
    result.push_back(root / "world_map_reference.json");
    return result;
}
} // namespace sward::ui_runtime
