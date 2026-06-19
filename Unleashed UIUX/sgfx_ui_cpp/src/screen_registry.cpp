// =============================================================================
// screen_registry.cpp — the table of reconstructed screens. Add a screen by
// implementing <id>Init()/<id>Draw(double) (and optionally <id>Input(const
// ScreenInput&)/<id>Reset()/<id>Nav()) in its own translation unit, then adding
// one row here (and the source to CMakeLists).
//
// FLOW: screens with their own navigation logic export <id>Nav() (poll-and-
// clear). Screens whose only flow edge is "B backs out" get a tiny wrapper
// here instead (forward Input, surface "@back") — the host (main_screens)
// drives the measured chevron-wipe transition, the loading chains
// ("loading>target") and the back-stack.
// =============================================================================
#include "screen.h"
#include <cstring>

// Each screen module exposes these (external linkage); internals stay file-local.
void PauseInit();     void PauseDraw(double);     void PauseInput(const ScreenInput&);    void PauseReset();
const char* PauseNav();
void ResultInit();    void ResultDraw(double);
void TitleInit();     void TitleDraw(double);      void TitleInput(const ScreenInput&);    void TitleReset();
const char* TitleNav();
void WorldMapInit();  void WorldMapDraw(double);   void WorldMapInput(const ScreenInput&); void WorldMapReset();
const char* WorldMapNav();
void StatusInit();    void StatusDraw(double);     void StatusInput(const ScreenInput&);   void StatusReset();
bool StatusOnStatRow();   // cursor on a pack row; cancel backs out
void ShopInit();      void ShopDraw(double);       void ShopInput(const ScreenInput&);     void ShopReset();
void SonicHudInit();  void SonicHudDraw(double);   void SonicHudInput(const ScreenInput&); void SonicHudReset();
void OptionsInit();    void OptionsDraw(double);    void OptionsInput(const ScreenInput&);    void OptionsReset();
void TownInit();       void TownDraw(double);       void TownInput(const ScreenInput&);       void TownReset();
const char* TownNav();
void GateInit();       void GateDraw(double);       void GateInput(const ScreenInput&);       void GateReset();
const char* GateNav();
void BossInit();       void BossDraw(double);       void BossInput(const ScreenInput&);       void BossReset();
void ItemResultInit(); void ItemResultDraw(double); void ItemResultInput(const ScreenInput&); void ItemResultReset();
void ResultExInit();   void ResultExDraw(double);   void ResultExInput(const ScreenInput&);   void ResultExReset();
void MediaRoomInit();     void MediaRoomDraw(double);     void MediaRoomInput(const ScreenInput&);     void MediaRoomReset();
void LoadingInit();       void LoadingDraw(double);       void LoadingInput(const ScreenInput&);       void LoadingReset();
void StartInit();         void StartDraw(double);         void StartInput(const ScreenInput&);         void StartReset();
void MissionInit();       void MissionDraw(double);       void MissionInput(const ScreenInput&);       void MissionReset();
void MissionScreenInit(); void MissionScreenDraw(double); void MissionScreenInput(const ScreenInput&); void MissionScreenReset();
void QteInit();           void QteDraw(double);           void QteInput(const ScreenInput&);           void QteReset();
void BalloonInit();       void BalloonDraw(double);       void BalloonInput(const ScreenInput&);       void BalloonReset();
void ExStageInit();       void ExStageDraw(double);       void ExStageInput(const ScreenInput&);       void ExStageReset();
void WorldMapHelpInit();  void WorldMapHelpDraw(double);  void WorldMapHelpInput(const ScreenInput&);  void WorldMapHelpReset();
void InstallerInit();     void InstallerDraw(double);     void InstallerInput(const ScreenInput&);     void InstallerReset();
void BootLoadingInit();   void BootLoadingDraw(double);   void BootLoadingInput(const ScreenInput&);   void BootLoadingReset();
void BootTitleInit();     void BootTitleDraw(double);     void BootTitleInput(const ScreenInput&);     void BootTitleReset();
const char* BootTitleNav();
void BootLogosInit();     void BootLogosDraw(double);     void BootLogosInput(const ScreenInput&);     void BootLogosReset();
const char* BootLogosNav();
void Viewport3DInit();    void Viewport3DDraw(double);    void Viewport3DInput(const ScreenInput&);    void Viewport3DReset();
void CarViewInit();       void CarViewDraw(double);       void CarViewInput(const ScreenInput&);       void CarViewReset();

// ---- flow wrappers: forward to the screen's own Input, surface the edge -----
namespace {
const char* g_wrapNav = nullptr;
const char* WrapNav() { const char* n = g_wrapNav; g_wrapNav = nullptr; return n; }
void StatusFlowInput(const ScreenInput& in)  { StatusInput(in);       if (in.cancel) g_wrapNav = "@back"; else if (in.accept) g_wrapNav = "result"; }   // B backs out; A -> the verdict
void OptionsFlowInput(const ScreenInput& in) { OptionsInput(in);      if (in.cancel) g_wrapNav = "@back"; }
void MediaFlowInput(const ScreenInput& in)   { MediaRoomInput(in);    if (in.cancel) g_wrapNav = "@back"; }
void WmHelpFlowInput(const ScreenInput& in)  { WorldMapHelpInput(in); if (in.cancel) g_wrapNav = "@back"; }
void BalloonFlowInput(const ScreenInput& in) { BalloonInput(in);      if (in.cancel) g_wrapNav = "@back"; }
void HudFlowInput(const ScreenInput& in)     { SonicHudInput(in);     if (in.cancel) g_wrapNav = "pause"; else if (in.accept) g_wrapNav = "status"; }   // Esc pauses; Enter -> metrics
void ResultFlowInput(const ScreenInput& in)  { if (in.accept) g_wrapNav = "world_map"; else if (in.cancel) g_wrapNav = "@back"; }   // A -> hub, B backs out
void ResultExFlowInput(const ScreenInput& in){ ResultExInput(in);     if (in.cancel) g_wrapNav = "@back"; }
void CarViewFlowInput(const ScreenInput& in) { CarViewInput(in);      if (in.cancel) g_wrapNav = "@back"; }
} // namespace

static const ScreenDef g_screens[] = {
    { "boot_logos", &BootLogosInit, &BootLogosDraw, &BootLogosInput, &BootLogosReset, &BootLogosNav },
    { "boot_title", &BootTitleInit, &BootTitleDraw, &BootTitleInput, &BootTitleReset, &BootTitleNav },
    { "installer", &InstallerInit, &InstallerDraw, &InstallerInput, &InstallerReset, nullptr },
    { "boot_loading", &BootLoadingInit, &BootLoadingDraw, &BootLoadingInput, &BootLoadingReset, nullptr },
    { "title",     &TitleInit,    &TitleDraw,    &TitleInput,    &TitleReset,    &TitleNav },
    { "world_map", &WorldMapInit, &WorldMapDraw, &WorldMapInput, &WorldMapReset, &WorldMapNav },
    { "status",    &StatusInit,   &StatusDraw,   &StatusFlowInput, &StatusReset, &WrapNav },
    { "sonic_hud", &SonicHudInit, &SonicHudDraw, &HudFlowInput,    &SonicHudReset, &WrapNav },
    { "pause",     &PauseInit,    &PauseDraw,    &PauseInput,   &PauseReset,  &PauseNav },
    { "result",    &ResultInit,   &ResultDraw,   &ResultFlowInput, nullptr,    &WrapNav },
    { "options",     &OptionsInit,    &OptionsDraw,    &OptionsFlowInput, &OptionsReset, &WrapNav },
    { "town",        &TownInit,       &TownDraw,       &TownInput,       &TownReset,    &TownNav },
    { "gate",        &GateInit,       &GateDraw,       &GateInput,       &GateReset,    &GateNav },
    { "result_ex",   &ResultExInit,   &ResultExDraw,   &ResultExFlowInput, &ResultExReset, &WrapNav },
    { "mediaroom",      &MediaRoomInit,     &MediaRoomDraw,     &MediaFlowInput,     &MediaRoomReset, &WrapNav },
    { "loading",        &LoadingInit,       &LoadingDraw,       &LoadingInput,       &LoadingReset,   nullptr },
    { "start",          &StartInit,         &StartDraw,         &StartInput,         &StartReset,     nullptr },
    { "balloon",        &BalloonInit,       &BalloonDraw,       &BalloonFlowInput,   &BalloonReset,   &WrapNav },
    { "world_map_help", &WorldMapHelpInit,  &WorldMapHelpDraw,  &WmHelpFlowInput,    &WorldMapHelpReset, &WrapNav },
    { "carview",        &CarViewInit,       &CarViewDraw,       &CarViewFlowInput,   &CarViewReset,      &WrapNav },
};

const ScreenDef* AllScreens(int& count) {
    count = (int)(sizeof(g_screens) / sizeof(g_screens[0]));
    return g_screens;
}

const ScreenDef* FindScreen(const char* id) {
    int n; const ScreenDef* a = AllScreens(n);
    for (int i = 0; i < n; ++i)
        if (std::strcmp(a[i].id, id) == 0) return &a[i];
    return nullptr;
}
