// =============================================================================
// sgfx_installer.h — the first-run installer's BACKEND contract (was
// install/installer.h + apu/embedded_player.h + hid). The actual Xbox game-data
// install pipeline (VFS parse, XEX patch, hash/copy, journal/rollback) is HOST
// infrastructure — the lib only declares it so installer_wizard.cpp (the wizard UI)
// compiles. A host binds the real pipeline; a preview drives the progress bar with a
// fake timer. Symbols keep their recomp names so the wizard body stays byte-identical.
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <span>
#include <memory>
#include <functional>
#include <chrono>
#include <filesystem>

namespace XexPatcher { enum class Result { Success, XexFileUnsupported, XexFileInvalid, PatchFileInvalid, PatchIncompatible, PatchFailed, PatchUnsupported }; }

struct VirtualFileSystem {};   // host's source-archive abstraction (opaque to the UI)
struct FilePair {};

enum class DLC { Unknown, SpagoniaAdventurePack, ChunNanAdventurePack, MazuriAdventurePack, HolokyAdventurePack, ApotosShamarAdventurePack, EmpireCityAdventurePack, Count };

struct Journal
{
    enum class Result { Success, FileMissing, FileReadFailed, FileHashFailed, FileWriteFailed, DirectoryCreationFailed, VirtualFileSystemFailed };
    uint64_t progressCounter = 0;
    uint64_t progressTotal = 0;
    std::list<std::filesystem::path> createdFiles;
    std::set<std::filesystem::path>  createdDirectories;
    Result            lastResult = Result::Success;
    XexPatcher::Result lastPatcherResult = XexPatcher::Result::Success;
    std::string       lastErrorMessage;
};

struct Installer
{
    struct Input { std::filesystem::path gameSource, updateSource; std::list<std::filesystem::path> dlcSources; };
    struct DLCSource { std::unique_ptr<VirtualFileSystem> sourceVfs; std::span<const FilePair> filePairs; const uint64_t* fileHashes = nullptr; std::string targetSubDirectory; };
    struct Sources { std::unique_ptr<VirtualFileSystem> game, update; std::vector<DLCSource> dlc; uint64_t totalSize = 0; };

    static bool checkGameInstall(const std::filesystem::path&, std::filesystem::path&);
    static bool checkDLCInstall(const std::filesystem::path&, DLC);
    static bool checkAllDLC(const std::filesystem::path&);
    static bool parseSources(const Input&, Journal&, Sources&);
    static bool install(const Sources&, const std::filesystem::path&, bool skipHashChecks, Journal&, std::chrono::seconds endWaitTime, const std::function<bool()>& progressCallback);
    static void rollback(Journal&);
    static bool parseGame(const std::filesystem::path&);
    static bool parseUpdate(const std::filesystem::path&);
    static DLC  parseDLC(const std::filesystem::path&);
    static XexPatcher::Result checkGameUpdateCompatibility(const std::filesystem::path&, const std::filesystem::path&);
};

namespace EmbeddedPlayer { void Init(); void PlayMusic(const char* name = nullptr); void FadeOutMusic(); void Shutdown(); }

// installer "thanks" marquee on the success page — host-supplied contributor list
#include <array>
extern std::array<const char*, 17> g_credits;

// Xbox input-emulation state used only by InstallerWizard::Run() to force controller 0 (was hid/hid.h)
struct XAMINPUT_STATE { uint32_t dwPacketNumber = 0; struct { uint16_t wButtons = 0; } Gamepad; };
namespace hid { uint32_t GetState(uint32_t userIndex, XAMINPUT_STATE* state); }
