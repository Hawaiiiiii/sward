// =============================================================================
// screen_boot_loading.cpp — the first-boot loading screen, brand-neutral. A dark
// field with a muted-green "Loading" line lower-right and a drawn dot-ring
// spinner (the boot_logos idiom): primitives + text only, no atlas/sprite/font
// art. The "Loading" line breathes on a ~1.0s cycle while the spinner walks the
// ring. No header wordmark.
// =============================================================================
#include "sgfxui.h"
#include "screen.h"
#include <cmath>
#include <algorithm>

using namespace ui;
namespace {

int g_fRodin = 0;

void Init() { if (g_fRodin == 0) g_fRodin = LoadMsdfFont("rodin_db"); }
void Reset() {}
void Input(const ScreenInput&) {}

void Draw(double openSec) {
    (void)openSec;
    const double t = Now();   // continuous clock (loops)

    // dark neutral field
    DrawVGradient({ 0, 0 }, { REF_W, REF_H }, RGBA(8, 10, 24, 255), RGBA(4, 5, 14, 255));

    // breathing pulse (boot_logos idiom): bright-biased over a 1.0s period
    float t01 = (float)(t - std::floor(t));
    float tri = 0.5f - 0.5f * std::cos(6.2831853f * t01);
    float pulse = 0.40f + 0.60f * std::sqrt(tri);

    // "Loading" line lower-right, muted green via the rodin MSDF font
    SetFont(g_fRodin);
    const char* MSG = "Loading";
    float w = MeasureText(24.0f, MSG).x;
    DrawText({ 1011.0f - w, 596 }, 24.0f, WithAlpha(RGBA(120, 210, 90, 255), pulse), MSG);
    ResetFont();

    // dot-ring spinner just right of the word: 8 cells, a lit head sweeping
    static const int RING[8][2] = { {0,0},{1,0},{2,0},{2,1},{2,2},{1,2},{0,2},{0,1} };
    int head = (int)(t * 10.0) % 8;
    for (int i = 0; i < 8; ++i) {
        int d = (head - i + 8) % 8;
        float k = (d == 0) ? 1.0f : (d == 1 ? 0.86f : (d == 2 ? 0.76f : 0.55f));
        uint32_t c = ColourLerp(RGBA(28, 92, 8, 255), RGBA(120, 210, 90, 255), k);
        float gx = 1023.0f + RING[i][0] * 11.5f, gy = 593.0f + RING[i][1] * 11.5f;
        DrawRect({ gx, gy }, { gx + 9, gy + 9 }, c, true);
    }
}

} // namespace

void BootLoadingInit() { Init(); }
void BootLoadingDraw(double openSeconds) { Draw(openSeconds); }
void BootLoadingInput(const ScreenInput& in) { Input(in); }
void BootLoadingReset() { Reset(); }
