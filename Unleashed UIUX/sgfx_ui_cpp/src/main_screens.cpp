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
#define NOMINMAX                 // keep std::max/min usable; no windows.h min/max macros
#define WIN32_LEAN_AND_MEAN
#include <SDL.h>
#include <SDL_syswm.h>
#include <windows.h>
#include <psapi.h>          // GetProcessMemoryInfo (soak-mode working-set sampling)
#pragma comment(lib, "psapi.lib")
#include <algorithm>        // std::max

#include "gfx_d3d12.h"
#include "sgfxui.h"
#include "screen.h"
#include "audio.h"
#include "settings.h"
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

static void OpenScreen(const ScreenDef* scr, bool isBack = false) {
    if (!scr) return;
    scr->Init();
    if (scr->Reset) scr->Reset();
    // window cue: opening a screen plays winopen; backing out of one (a window
    // closing) plays winclose — matches the recomp (achievement_menu.cpp fires
    // pausewinclose on close, pausewinopen on open). No-op if audio is disabled.
    audio::Play(isBack ? audio::SFX_WINCLOSE : audio::SFX_WINOPEN);
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
    bool shot = false, csdMode = false, soak = false;
    std::string id = "boot_logos", out;     // bare launch = the game's boot flow
    double shotSec = 1.0, soakSeconds = 20.0;

    // --csd <id> [<sec> <out.png>]  -> render the REAL game CSD layout/animation (true 1:1).
    // --soak [seconds]              -> headless integration soak: cycle every screen
    //                                  through OpenScreen/Reset/Draw + transitions under
    //                                  synthetic input, reporting frames + memory growth.
    if (argc >= 3 && std::strcmp(argv[1], "--csd") == 0) {
        csdMode = true; id = argv[2];
        if (argc >= 5) { shot = true; shotSec = atof(argv[3]); out = argv[4]; }
    } else if (argc >= 5 && std::strcmp(argv[1], "--shot") == 0) {
        shot = true; id = argv[2]; shotSec = atof(argv[3]); out = argv[4];
    } else if (argc >= 2 && std::strcmp(argv[1], "--soak") == 0) {
        soak = true; shot = true;   // headless + hidden window, no audio
        if (argc >= 3) soakSeconds = atof(argv[2]);
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
    // ---- persistent window placement: display / fullscreen / size ----
    settings::Load();
    int dispCount = SDL_GetNumVideoDisplays();
    int disp = settings::GetInt("display", 0);
    if (disp < 0 || disp >= dispCount) disp = 0;
    int winW = settings::GetInt("win_w", W), winH = settings::GetInt("win_h", H);
    bool fullscreen = settings::GetInt("fullscreen", 0) != 0;
    SDL_Window* win = SDL_CreateWindow("sgfx_screens — clean C++ UI (D3D12)",
        SDL_WINDOWPOS_CENTERED_DISPLAY(disp), SDL_WINDOWPOS_CENTERED_DISPLAY(disp),
        shot ? W : winW, shot ? H : winH,
        SDL_WINDOW_RESIZABLE | (shot ? SDL_WINDOW_HIDDEN : 0));
    if (!shot && fullscreen) SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);

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

    // ---- integration soak: cycle every screen through the full Init/Reset/Draw/
    //      switch/transition path under synthetic input, watching for crashes and
    //      memory growth (the production-readiness gate component verification
    //      can't reach) ----
    if (soak) {
        int nScr = 0; const ScreenDef* screens = AllScreens(nScr);
        auto procMemMB = [] {
            PROCESS_MEMORY_COUNTERS pmc{}; pmc.cb = sizeof(pmc);
            GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
            return (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
        };
        const double startMem = procMemMB();
        double peakMem = startMem, synth = 0.0;
        uint64_t frames = 0; int opens = 0, navFires = 0;
        uint32_t rng = 0x9e3779b9u;
        auto rnd = [&] { rng = rng * 1664525u + 1013904223u; return rng; };
        const double t0 = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
        int si = 0; scr = &screens[si]; OpenScreen(scr);
        double memAtHalf = 0.0;
        for (;;) {
            double wall = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency() - t0;
            if (wall >= soakSeconds) break;
            if (memAtHalf == 0.0 && wall >= soakSeconds * 0.5) memAtHalf = procMemMB();
            // draw the active screen for a burst of frames with random navigation,
            // firing the leading frames as an in-progress transition (exercises the
            // chevron-wipe path too)
            for (int f = 0; f < 24; ++f) {
                synth += 1.0 / 60.0;
                ui::BeginFrame(synth);
                ScreenInput in{};
                switch (rnd() % 10) {
                    case 0: in.up = true; break;      case 1: in.down = true; break;
                    case 2: in.left = true; break;    case 3: in.right = true; break;
                    case 4: in.accept = true; break;  case 5: in.cancel = true; break;
                    case 6: in.tabLeft = true; break; case 7: in.tabRight = true; break;
                    default: break;   // idle frames (exercise loops/animations)
                }
                if (scr->Input) scr->Input(in);
                if (scr->Nav) { if (scr->Nav()) navFires++; }   // drain nav requests
                float fade = (f < 6) ? (1.0f - f / 6.0f) : 0.0f;
                drawFrame(synth, fade, synth);
                ++frames;
            }
            // advance to the next screen — exercises OpenScreen (texture load on
            // first visit, Reset, BGM-select guard) and the bindless heap growth
            si = (si + 1) % nScr; scr = &screens[si]; OpenScreen(scr); ++opens;
            double m = procMemMB(); if (m > peakMem) peakMem = m;
        }
        const double endMem = procMemMB();
        // steady-state growth = change over the SECOND half only (the first half is
        // one-time texture warm-up as each screen's atlas loads into the heap)
        const double warmup = std::max(memAtHalf, startMem) - startMem;
        const double steady = endMem - std::max(memAtHalf, startMem);
        const double steadyPerMin = (soakSeconds > 0 ? steady * 60.0 / (soakSeconds * 0.5) : 0.0);
        printf("\n[soak] %.0fs done: %llu frames, %d screen-opens (%d screens x %d cycles), %d nav-fires\n",
               soakSeconds, (unsigned long long)frames, opens, nScr, opens / (nScr ? nScr : 1), navFires);
        printf("[soak] memory MB: start %.1f | warm-up +%.1f (one-time texture load) | steady-state +%.2f (peak %.1f)\n",
               startMem, warmup, steady, peakMem);
        printf("[soak] steady-state growth %.2f MB/min -> verdict: %s\n", steadyPerMin,
               (steady < 4.0) ? "STABLE (no leak; bounded warm-up then flat)" : "REVIEW (memory still climbing post-warm-up)");
        audio::Shutdown(); ui::Shutdown(); gfx::shutdown(); SDL_DestroyWindow(win); SDL_Quit();
        return 0;
    }

    if (shot) {
        // Optional scripted input (argv[5]): one navigation step per character,
        // applied in the past so the final frame shows the settled result.
        //   u/d/l/r = d-pad, a = accept (A), b = cancel (B), q/e = LB/RB
        // Flow edges apply INSTANTLY here (no wipe), so navigation chains are
        // verifiable headlessly: e.g. `--shot pause 3 out.png da` renders STATUS.
        if (argc >= 6) {
            const ScreenDef* shotStack[16]; int shotDepth = 0;
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
                if (scr->Input) scr->Input(in);
                if (scr->Nav) {
                    if (const char* tgt = scr->Nav()) {
                        const ScreenDef* next = nullptr;
                        if (strcmp(tgt, "@back") == 0) {
                            if (shotDepth > 0) next = shotStack[--shotDepth];
                        } else {
                            const char* dest = tgt;
                            if (const char* p = strchr(tgt, '>')) dest = p + 1;   // skip the loading hop headlessly
                            next = FindScreen(dest);
                            if (next && shotDepth < 16) shotStack[shotDepth++] = scr;
                        }
                        if (next) {
                            scr = next;
                            if (scr->Init) scr->Init();
                            if (scr->Reset) scr->Reset();
                        }
                    }
                }
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
        for (size_t i = 3; i < px.size(); i += 4) px[i] = 255;   // opaque screenshots
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
    bool   transWasBack = false;                    // the pending switch is a back-stack pop (winclose, not winopen)
    bool   replayPending = false;
    // ---- the runtime flow: back-stack + loading chains ("loading>target") ----
    const ScreenDef* navStack[16]; int navDepth = 0;
    const ScreenDef* afterLoading = nullptr;        // destination once the loader has run

    while (run) {
        double absNow = (SDL_GetPerformanceCounter() - tStart) / freq;
        ScreenInput in;          // this frame's edge-triggered presses
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { run = false; }
            else if (e.type == SDL_KEYDOWN) {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_ESCAPE) { run = false; }
                else if (k == SDLK_F11) {                    // borderless fullscreen toggle
                    fullscreen = !fullscreen;
                    SDL_SetWindowFullscreen(win, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                    settings::SetInt("fullscreen", fullscreen ? 1 : 0);
                }
                else if (k == SDLK_F9) {                     // hop to the next monitor
                    disp = (disp + 1) % SDL_GetNumVideoDisplays();
                    bool wasFs = fullscreen;
                    if (wasFs) SDL_SetWindowFullscreen(win, 0);
                    SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED_DISPLAY(disp), SDL_WINDOWPOS_CENTERED_DISPLAY(disp));
                    if (wasFs) SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    settings::SetInt("display", disp);
                }
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
                if (transTarget) { scr = transTarget; OpenScreen(scr, transWasBack); transTarget = nullptr; transWasBack = false; }
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

        // ---- the runtime flow: screens request navigation; the host runs the
        //      measured chevron wipe + the loading hops + the back-stack ----
        if (transPhase == 0 && scr->Nav) {
            if (const char* tgt = scr->Nav()) {
                const ScreenDef* next = nullptr;
                if (strcmp(tgt, "@back") == 0) {
                    if (navDepth > 0) { next = navStack[--navDepth]; transWasBack = true; }  // a window closes
                } else {
                    const char* dest = tgt;
                    if (const char* p = strchr(tgt, '>')) {            // chain: hop via the loader
                        static char hop[32];
                        snprintf(hop, sizeof hop, "%.*s", (int)(p - tgt), tgt);
                        afterLoading = FindScreen(p + 1);
                        dest = hop;
                    }
                    next = FindScreen(dest);
                    if (next && navDepth < 16) navStack[navDepth++] = scr;
                }
                if (next) { transTarget = next; transPhase = 2; transStart = absNow; }
            }
        }
        // the loader runs ~2.8 s, then chains on to its destination (retail pacing)
        if (transPhase == 0 && afterLoading && (scr->id && strstr(scr->id, "load")) && now > 2.8) {
            transTarget = afterLoading; afterLoading = nullptr;
            transPhase = 2; transStart = absNow;
        }
        drawFrame(openSec, fade, absNow);
    }
    // persist the windowed size on the way out
    if (!fullscreen) {
        int cw, chh; SDL_GetWindowSize(win, &cw, &chh);
        settings::SetInt("win_w", cw); settings::SetInt("win_h", chh);
    }
    audio::Shutdown(); ui::Shutdown(); gfx::shutdown(); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}
