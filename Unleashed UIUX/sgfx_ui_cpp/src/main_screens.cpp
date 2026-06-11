// =============================================================================
// main_screens.cpp — entry point for the clean hand-written screen reconstruction.
//
//   sgfx_screens <id>                          interactive: render the screen in a window
//   sgfx_screens --shot <id> <sec> <out.png>   headless: render at time <sec>, read back to PNG
//
// Each screen is a clean C++ module (see screen_pause.cpp) drawn through the
// sgfxui layer onto the real CSD GPU path (gfx_d3d12). Interactive screens also
// expose Input()/Reset() (see screen.h) so they navigate + hold state exactly like
// UnleashedRecomp's ui/options_menu. The --shot mode renders off-screen and reads
// the framebuffer back so output can be verified without a display.
//
// Controls (window): arrows/WASD move, Enter/Z = A (accept), Backspace/X = B
// (cancel), Q/E = LB/RB (tab), Space replays the intro, 1-9 switch screens, Esc quits.
// =============================================================================
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>

#include "gfx_d3d12.h"
#include "sgfxui.h"
#include "screen.h"
#include "audio.h"
#include "csd_player.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"            // gfx::loadTexture() uses stbi_load
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static const int W = 1280, H = 720;

static const char* FontPath() {
    if (const char* env = std::getenv("SGFX_FONT")) return env;
    // DynaFont Hei (heavy gothic) staged next to the exe — close to the recomp's
    // DFSoGeiStd category type and far more game-accurate than a system sans.
    if (FILE* f = fopen("assets/ui_font.otf", "rb")) { fclose(f); return "assets/ui_font.otf"; }
    return "C:/Windows/Fonts/segoeui.ttf";   // fallback
}

// Fold an SDL keydown into the per-frame edge-triggered navigation input.
static void ApplyKey(ScreenInput& in, SDL_Keycode k) {
    switch (k) {
        case SDLK_UP:    case SDLK_w: in.up    = true; break;
        case SDLK_DOWN:  case SDLK_s: in.down  = true; break;
        case SDLK_LEFT:  case SDLK_a: in.left  = true; break;
        case SDLK_RIGHT: case SDLK_d: in.right = true; break;
        case SDLK_RETURN: case SDLK_z: in.accept = true; break;
        case SDLK_BACKSPACE: case SDLK_x: in.cancel = true; break;
        case SDLK_q: in.tabLeft  = true; break;
        case SDLK_e: in.tabRight = true; break;
        default: break;
    }
}

static int g_curBgm = -1;   // which BGM track is playing (-1 none, 0 menu, 1 title)

static void OpenScreen(const ScreenDef* scr) {
    if (!scr) return;
    scr->Init();
    if (scr->Reset) scr->Reset();
    audio::Play(audio::SFX_WINOPEN);   // window-open cue (no-op if audio disabled)
    // per-screen BGM: title screen gets the title theme, all menus share the menu theme.
    // Only (re)start when the track actually changes, so menu music flows across screens.
    int want = (std::strcmp(scr->id, "title") == 0) ? 1 : 0;
    if (want != g_curBgm) {
        bool ok = (want == 1) ? audio::PlayMusic("assets/music/bgm_sys_title.wav", 0)
                              : audio::PlayMusic("assets/music/bgm_sys_menu.wav", 599217);
        if (!ok && g_curBgm < 0) audio::PlayMusic("assets/music/installer.ogg");  // fallback
        g_curBgm = want;
    }
}

// A 512x512 grayscale-noise atlas (a 4x4 grid of cells) used for the TV-static wipe;
// each frame samples a different cell for animated static. Returns -1 on failure.
static int MakeNoiseTexture() {
    const int N = 512;
    std::vector<unsigned char> px((size_t)N * N * 4);
    uint32_t s = 0x1234567u;
    auto rnd = [&]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; };
    for (size_t i = 0; i < (size_t)N * N; ++i) {
        unsigned char v = (unsigned char)(rnd() & 0xFF);
        px[i*4+0] = v; px[i*4+1] = v; px[i*4+2] = v; px[i*4+3] = 255;
    }
    return gfx::loadTextureRGBA(px.data(), N, N);
}
static constexpr int NOISE_CELLS = 4;   // 4x4 grid -> 16 static frames

int main(int argc, char** argv) {
    bool shot = false, csdMode = false;
    std::string id = "pause", out;
    double shotSec = 1.0;

    // --csd <id> [<sec> <out.png>]  -> render the REAL game CSD layout/animation (true 1:1).
    if (argc >= 3 && std::strcmp(argv[1], "--csd") == 0) {
        csdMode = true; id = argv[2];
        if (argc >= 5) { shot = true; shotSec = atof(argv[3]); out = argv[4]; }
    } else if (argc >= 5 && std::strcmp(argv[1], "--shot") == 0) {
        shot = true; id = argv[2]; shotSec = atof(argv[3]); out = argv[4];
    } else if (argc >= 2) {
        id = argv[1];
    }

    const ScreenDef* scr = nullptr;
    if (!csdMode) {
        scr = FindScreen(id.c_str());
        if (!scr) { int n; const ScreenDef* a = AllScreens(n); scr = (n > 0) ? &a[0] : nullptr; }
        if (!scr) { fprintf(stderr, "no screens registered\n"); return 1; }
    }

    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL init: %s\n", SDL_GetError()); return 1; }
    SDL_Window* win = SDL_CreateWindow("sgfx_screens — clean C++ UI (D3D12)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H,
        SDL_WINDOW_RESIZABLE | (shot ? SDL_WINDOW_HIDDEN : 0));

    void* hwnd = nullptr;
    SDL_SysWMinfo wm; SDL_VERSION(&wm.version);
    if (SDL_GetWindowWMInfo(win, &wm)) hwnd = (void*)wm.info.win.window;

    if (!gfx::init(hwnd, W, H, shot, 4)) { fprintf(stderr, "gfx init failed\n"); return 1; }
    if (!ui::Init(FontPath()))
        fprintf(stderr, "[warn] font not loaded; shapes will draw but text will be blank\n");
    if (csdMode) {
        if (!csd::Load(id.c_str())) { fprintf(stderr, "csd load failed for '%s'\n", id.c_str()); return 1; }
    } else {
        if (!shot) audio::Init();   // UI sound feedback (interactive only; off for headless shots)
        OpenScreen(scr);   // loads textures, resets state, and starts the screen's per-screen BGM
    }

    int noiseTex = -1;   // TV-static atlas (assigned before the interactive loop)

    // Draw the active screen for the given clock; assumes ui::BeginFrame already set.
    // During a transition (`fade` 0..1, `absT` = wall clock for the static cycle) it
    // adds a TV-static wipe + a black dip + letterbox bars that pinch in — the
    // recomp's "channel change" feel.
    auto drawFrame = [&](double openSec, float fade, double absT) {
        if (csdMode) csd::Draw(openSec);   // openSec carries the CSD animation time here
        else         scr->Draw(openSec);
        // CRT scanlines are NOT in the real runtime (they read as banding vs the
        // reference frames) — opt-in SGFX stylization via SGFX_SCANLINES=1.
        static const bool wantScan = [] { const char* e = getenv("SGFX_SCANLINES"); return e && e[0] == '1'; }();
        if (wantScan) ui::DrawScanlines({ 0, 0 }, { ui::REF_W, ui::REF_H }, ui::RGBA(0, 0, 0, 16), 3.0f);
        if (fade > 0.001f) {
            // Default transition = the GAME's measured chevron-band wipe (live
            // capture, gate->loading). The old TV-static/letterbox stylization
            // stays available behind SGFX_STATIC=1.
            static const bool wantStatic = [] { const char* e = getenv("SGFX_STATIC"); return e && e[0] == '1'; }();
            if (!wantStatic) {
                ui::DrawChevronWipe(fade);
            } else {
                ui::DrawRect({ 0, 0 }, { ui::REF_W, ui::REF_H }, ui::RGBA(0, 0, 0, int(fade * 255.0f)));   // black dip
                if (noiseTex >= 0) {                                                                       // TV static
                    int idx = int(absT * 30.0) % (NOISE_CELLS * NOISE_CELLS);
                    if (idx < 0) idx += NOISE_CELLS * NOISE_CELLS;
                    int cx = idx % NOISE_CELLS, cy = idx / NOISE_CELLS;
                    float c = 1.0f / NOISE_CELLS;
                    ui::DrawImage(noiseTex, { 0, 0 }, { ui::REF_W, ui::REF_H },
                                  { cx * c, cy * c }, { (cx + 1) * c, (cy + 1) * c },
                                  ui::RGBA(255, 255, 255, int(fade * 150.0f)), /*additive*/ true);
                }
                float bar = fade * 72.0f;                                                                  // letterbox bars
                ui::DrawRect({ 0, 0 }, { ui::REF_W, bar }, ui::RGBA(0, 0, 0, 255));
                ui::DrawRect({ 0, ui::REF_H - bar }, { ui::REF_W, ui::REF_H }, ui::RGBA(0, 0, 0, 255));
            }
        }
        gfx::Quad* q = nullptr; int n = 0; ui::Flush(q, n);
        gfx::beginFrame(0.10f, 0.12f, 0.16f, 1.0f);   // stand-in gameplay backdrop
        gfx::drawQuads(q, n);
        gfx::endFrame();
        return n;
    };

    if (shot) {
        // Optional scripted input (argv[5]): one navigation step per character,
        // applied in the past so the final frame shows the settled result.
        //   u/d/l/r = d-pad, a = accept (A), b = cancel (B), q/e = LB/RB
        if (argc >= 6 && scr->Input) {
            const char* keys = argv[5];
            for (int i = 0; keys[i]; ++i) {
                ui::BeginFrame(0.5 + i * 0.25);
                ScreenInput in;
                switch (keys[i]) {
                    case 'u': in.up = true; break;       case 'd': in.down = true; break;
                    case 'l': in.left = true; break;     case 'r': in.right = true; break;
                    case 'a': in.accept = true; break;   case 'b': in.cancel = true; break;
                    case 'q': in.tabLeft = true; break;  case 'e': in.tabRight = true; break;
                    default: break;
                }
                scr->Input(in);
            }
        }
        // SGFX_FADE forces the transition overlay (static+letterbox+black) for headless verification.
        float shotFade = 0.0f;
        if (const char* fe = std::getenv("SGFX_FADE")) shotFade = (float)atof(fe);
        if (shotFade > 0.0f) noiseTex = MakeNoiseTexture();
        ui::BeginFrame(shotSec);
        int n = drawFrame(csdMode ? shotSec : 0.0, shotFade, shotSec);
        std::vector<unsigned char> px((size_t)W * H * 4);
        gfx::readbackRGBA(px.data(), W, H);
        stbi_write_png(out.c_str(), W, H, 4, px.data(), W * 4);
        printf("wrote %s (%s=%s t=%.2fs, %d quads, MSAA x%d)\n",
               out.c_str(), csdMode ? "csd" : "screen", id.c_str(), shotSec, n, gfx::sampleCount());
        csd::Unload(); ui::Shutdown(); gfx::shutdown(); SDL_DestroyWindow(win); SDL_Quit();
        return 0;
    }

    // ---- CSD interactive: render the real game layout/animation (loops); Esc quits, Space restarts ----
    if (csdMode) {
        bool run = true; double freq = (double)SDL_GetPerformanceFrequency();
        Uint64 t0 = SDL_GetPerformanceCounter();
        while (run) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) run = false;
                else if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_ESCAPE) run = false;
                    else if (e.key.keysym.sym == SDLK_SPACE) t0 = SDL_GetPerformanceCounter();
                }
            }
            double now = (SDL_GetPerformanceCounter() - t0) / freq;
            ui::BeginFrame(now);
            drawFrame(now, 0.0f, now);
        }
        csd::Unload(); ui::Shutdown(); gfx::shutdown(); SDL_DestroyWindow(win); SDL_Quit();
        return 0;
    }

    noiseTex = MakeNoiseTexture();                 // TV-static atlas for transitions
    bool run = true;
    double freq = (double)SDL_GetPerformanceFrequency();
    Uint64 tStart = SDL_GetPerformanceCounter();   // fixed clock for transition fades
    Uint64 t0 = tStart;                            // per-screen intro clock (reset on open)
    double openSec = 0.0;
    const double FADE = 0.32;                       // wipe duration (measured ~0.32 s live)
    int    transPhase = 1;                          // 0 idle, 1 fade-in, 2 fade-out (switch pending)
    double transStart = 0.0;                        // absNow when the current phase began
    const ScreenDef* transTarget = nullptr;         // screen to switch to once black
    bool   replayPending = false;

    while (run) {
        double absNow = (SDL_GetPerformanceCounter() - tStart) / freq;
        ScreenInput in;          // this frame's edge-triggered presses
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { run = false; }
            else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_ESCAPE) { run = false; }
                else if (transPhase != 0) { /* ignore input mid-transition */ }
                else if (k == SDLK_SPACE) {                 // replay current screen (fades)
                    replayPending = true; transPhase = 2; transStart = absNow;
                    audio::Play(audio::SFX_CANCEL);
                }
                else if (k >= SDLK_1 && k <= SDLK_9) {      // switch screen (fades)
                    int idx = k - SDLK_1, n; const ScreenDef* a = AllScreens(n);
                    if (idx < n && &a[idx] != scr) { transTarget = &a[idx]; transPhase = 2; transStart = absNow; }
                }
                else { ApplyKey(in, k); }                   // navigation -> active screen
            }
        }

        // ---- transition state machine -> fade alpha (black overlay) ----
        float fade = 0.0f;
        if (transPhase == 1) {                              // fading IN from black
            float p = (float)((absNow - transStart) / FADE);
            if (p >= 1.0f) transPhase = 0; else fade = 1.0f - p;
        } else if (transPhase == 2) {                       // fading OUT to black
            float p = (float)((absNow - transStart) / FADE);
            if (p >= 1.0f) {                                // at full black: apply the change, then fade in
                if (transTarget) { scr = transTarget; OpenScreen(scr); transTarget = nullptr; }
                else if (replayPending) { if (scr->Reset) scr->Reset(); audio::Play(audio::SFX_WINOPEN); replayPending = false; }
                t0 = SDL_GetPerformanceCounter();           // restart the screen intro
                transPhase = 1; transStart = absNow; fade = 1.0f;
            } else fade = p;
        }

        double now = (SDL_GetPerformanceCounter() - t0) / freq;
        ui::BeginFrame(now);                                // set the clock before Input/Draw
        if (scr->Input) scr->Input(in);                     // navigate / mutate state
        // UI sound feedback — distinct real-game cue per action (bumpers differ from cursor)
        if (in.up || in.down || in.left || in.right) audio::Play(audio::SFX_CURSOR);
        if (in.tabLeft || in.tabRight)               audio::Play(audio::SFX_TAB);     // LB/RB: form/category switch
        if (in.accept)                               audio::Play(audio::SFX_DECIDE);
        if (in.cancel)                               audio::Play(audio::SFX_CANCEL);
        drawFrame(openSec, fade, absNow);
    }
    audio::Shutdown(); ui::Shutdown(); gfx::shutdown(); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}
