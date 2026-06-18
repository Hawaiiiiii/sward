// preview_installer_stub.cpp — host-side install backend for the standalone preview.
// The installer WIZARD UI (installer_wizard.cpp) is the recomp's authentic code; the
// install ENGINE (VFS/XEX/hash/copy), the embedded installer music, the controller
// emulation and the Win32 cursor are HOST infrastructure (see sgfx_installer.h /
// sgfx_window.h / game_window.h). For the preview we stub them: no real install runs,
// the wizard just renders its pages so we can prove the UI/UX is the recomp's.
#include "../platform/sgfx_installer.h"
#include "../platform/sgfx_window.h"
#include "../ui/game_window.h"
#include <array>

// ---- install pipeline (no-op demo: nothing is parsed/installed) -------------
bool Installer::checkGameInstall(const std::filesystem::path&, std::filesystem::path&) { return false; }
bool Installer::checkDLCInstall(const std::filesystem::path&, DLC) { return false; }
bool Installer::checkAllDLC(const std::filesystem::path&) { return false; }
bool Installer::parseSources(const Input&, Journal&, Sources&) { return false; }
bool Installer::install(const Sources&, const std::filesystem::path&, bool, Journal&, std::chrono::seconds, const std::function<bool()>&) { return false; }
void Installer::rollback(Journal&) {}
bool Installer::parseGame(const std::filesystem::path&) { return false; }
bool Installer::parseUpdate(const std::filesystem::path&) { return false; }
DLC  Installer::parseDLC(const std::filesystem::path&) { return DLC::Unknown; }
XexPatcher::Result Installer::checkGameUpdateCompatibility(const std::filesystem::path&, const std::filesystem::path&) { return XexPatcher::Result::Success; }

// ---- embedded installer music (host APU; demo no-op) ------------------------
namespace EmbeddedPlayer {
    void Init() {}
    void PlayMusic(const char*) {}
    void FadeOutMusic() {}
    void Shutdown() {}
}

// ---- swapchain (host renderer; the preview drives its own SDL loop) ----------
void Video::WaitForGPU() {}
void Video::WaitOnSwapChain() {}
void Video::Present() {}

// ---- controller emulation (no input in the preview) -------------------------
namespace hid { uint32_t GetState(uint32_t, XAMINPUT_STATE* state) { if (state) *state = {}; return 0; } }

// ---- window host hooks used only by the wizard ------------------------------
void GameWindow::SetFullscreenCursorVisibility(bool) {}
void GameWindow::Update() {}

// ---- installer "thanks" marquee (host-supplied contributor list) ------------
std::array<const char*, 17> g_credits =
{
    "Sonic Team", "Hedgehog Engine", "Project Quality Hero", "SGFX",
    "UnleashedRecomp", "Hedge Development", "Contributors", "Community",
    "Testers", "Translators", "Artists", "Engineers",
    "Designers", "Sound", "Tools", "Documentation", "Thank You",
};
