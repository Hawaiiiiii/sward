// =============================================================================
// screen_registry.cpp — the table of reconstructed screens. Add a screen by
// implementing <id>Init()/<id>Draw(double) (and optionally <id>Input(const
// ScreenInput&)/<id>Reset()) in its own translation unit, then adding one row
// here (and the source to CMakeLists). The fan-out of the remaining game screens
// follows exactly this pattern.
// =============================================================================
#include "screen.h"
#include <cstring>

// Each screen module exposes these (external linkage); internals stay file-local.
// Init/Draw are required; Input/Reset are optional (pass nullptr when absent).
void PauseInit();     void PauseDraw(double);     void PauseInput(const ScreenInput&);    void PauseReset();
void ResultInit();    void ResultDraw(double);
void TitleInit();     void TitleDraw(double);      void TitleInput(const ScreenInput&);    void TitleReset();
void WorldMapInit();  void WorldMapDraw(double);   void WorldMapInput(const ScreenInput&); void WorldMapReset();
void StatusInit();    void StatusDraw(double);     void StatusInput(const ScreenInput&);   void StatusReset();
void ShopInit();      void ShopDraw(double);       void ShopInput(const ScreenInput&);     void ShopReset();
void SonicHudInit();  void SonicHudDraw(double);   void SonicHudInput(const ScreenInput&); void SonicHudReset();
void OptionsInit();    void OptionsDraw(double);    void OptionsInput(const ScreenInput&);    void OptionsReset();
void TownInit();       void TownDraw(double);       void TownInput(const ScreenInput&);       void TownReset();
void GateInit();       void GateDraw(double);       void GateInput(const ScreenInput&);       void GateReset();
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
void Viewport3DInit();    void Viewport3DDraw(double);    void Viewport3DInput(const ScreenInput&);    void Viewport3DReset();

static const ScreenDef g_screens[] = {
    { "installer", &InstallerInit, &InstallerDraw, &InstallerInput, &InstallerReset },
    { "boot_loading", &BootLoadingInit, &BootLoadingDraw, &BootLoadingInput, &BootLoadingReset },
    { "boot_title", &BootTitleInit, &BootTitleDraw, &BootTitleInput, &BootTitleReset },
    { "viewport3d", &Viewport3DInit, &Viewport3DDraw, &Viewport3DInput, &Viewport3DReset },
    { "title",     &TitleInit,    &TitleDraw,    &TitleInput,    &TitleReset    },
    { "world_map", &WorldMapInit, &WorldMapDraw, &WorldMapInput, &WorldMapReset },
    { "status",    &StatusInit,   &StatusDraw,   &StatusInput,   &StatusReset   },
    { "shop",      &ShopInit,     &ShopDraw,     &ShopInput,     &ShopReset     },
    { "sonic_hud", &SonicHudInit, &SonicHudDraw, &SonicHudInput, &SonicHudReset },
    { "pause",     &PauseInit,    &PauseDraw,    &PauseInput,   &PauseReset  },
    { "result",    &ResultInit,   &ResultDraw,   nullptr,       nullptr      },
    { "options",     &OptionsInit,    &OptionsDraw,    &OptionsInput,    &OptionsReset    },
    { "town",        &TownInit,       &TownDraw,       &TownInput,       &TownReset       },
    { "gate",        &GateInit,       &GateDraw,       &GateInput,       &GateReset       },
    { "boss",        &BossInit,       &BossDraw,       &BossInput,       &BossReset       },
    { "item_result", &ItemResultInit, &ItemResultDraw, &ItemResultInput, &ItemResultReset },
    { "result_ex",   &ResultExInit,   &ResultExDraw,   &ResultExInput,   &ResultExReset   },
    { "mediaroom",      &MediaRoomInit,     &MediaRoomDraw,     &MediaRoomInput,     &MediaRoomReset     },
    { "loading",        &LoadingInit,       &LoadingDraw,       &LoadingInput,       &LoadingReset       },
    { "start",          &StartInit,         &StartDraw,         &StartInput,         &StartReset         },
    { "mission",        &MissionInit,       &MissionDraw,       &MissionInput,       &MissionReset       },
    { "mission_screen", &MissionScreenInit, &MissionScreenDraw, &MissionScreenInput, &MissionScreenReset },
    { "qte",            &QteInit,           &QteDraw,           &QteInput,           &QteReset           },
    { "balloon",        &BalloonInit,       &BalloonDraw,       &BalloonInput,       &BalloonReset       },
    { "exstage",        &ExStageInit,       &ExStageDraw,       &ExStageInput,       &ExStageReset       },
    { "world_map_help", &WorldMapHelpInit,  &WorldMapHelpDraw,  &WorldMapHelpInput,  &WorldMapHelpReset  },
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
