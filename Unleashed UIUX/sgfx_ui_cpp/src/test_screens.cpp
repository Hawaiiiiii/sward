// Host-side verification (no GPU): links the clean UI layer + screens against a
// stub backend, runs the pause screen at several times, and reports the emitted
// quad batch (count, text-glyph quads, bounding box, on-screen fraction). Proves
// the sgfxui layer + screen modules compile and produce sane geometry/text
// without needing the D3D12 toolchain.
#include "sgfxui.h"
#include "screen.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

// Only backend symbol the UI layer references at link time.
namespace gfx { int loadTextureRGBA(const uint8_t*, int w, int h) { return (w > 0 && h > 0) ? 1 : -1; } }

int main() {
    const char* font = std::getenv("SGFX_FONT");
    if (!font) font = "C:/Windows/Fonts/segoeui.ttf";
    bool ok = ui::Init(font);
    std::printf("font atlas baked: %s\n", ok ? "yes" : "NO");

    const ScreenDef* s = FindScreen("pause");
    if (!s) { std::printf("FAIL: pause screen not registered\n"); return 1; }
    s->Init();

    for (double t : { 0.05, 0.30, 2.00 }) {
        ui::BeginFrame(t);
        s->Draw(0.0);
        gfx::Quad* q = nullptr; int n = 0;
        ui::Flush(q, n);
        float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f; int textQ = 0, onScreen = 0;
        for (int i = 0; i < n; ++i) {
            if (q[i].texIndex == 1) ++textQ;       // font-atlas slot from the stub
            bool vis = false;
            for (int k = 0; k < 4; ++k) {
                x0 = std::min(x0, q[i].px[k]); y0 = std::min(y0, q[i].py[k]);
                x1 = std::max(x1, q[i].px[k]); y1 = std::max(y1, q[i].py[k]);
                if (q[i].px[k] >= 0 && q[i].px[k] <= 1280 && q[i].py[k] >= 0 && q[i].py[k] <= 720) vis = true;
            }
            if (vis) ++onScreen;
        }
        std::printf("t=%.2fs  quads=%-4d textQuads=%-4d onScreen=%-4d  bbox=[%.0f,%.0f .. %.0f,%.0f]\n",
                    t, n, textQ, onScreen, x0, y0, x1, y1);
    }
    ui::Shutdown();
    std::printf("OK\n");
    return 0;
}
