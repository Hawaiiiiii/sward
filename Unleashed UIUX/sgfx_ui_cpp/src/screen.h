// =============================================================================
// screen.h — the screen registry. Each reconstructed screen is a self-contained
// translation unit exposing Init()/Draw()/Input()/Reset(), registered in
// screen_registry.cpp. This mirrors how UnleashedRecomp structures ui/options_menu
// and ui/installer_wizard as discrete screen modules (each with its own draw +
// per-frame input/state), but as a small table the standalone app can drive
// (pick by id, headless-shot, or step interactively).
// =============================================================================
#pragma once

// Edge-triggered navigation input for a frame (mirrors the recomp's pad/keys ->
// menu navigation). A field is true only on the frame the control is pressed.
//   up/down/left/right : d-pad / left stick / arrow keys / WASD
//   accept / cancel    : A / B   (Enter,Z / Backspace,X)
//   tabLeft / tabRight : LB / RB  (Q / E)  — page/tab switching
struct ScreenInput {
    bool up = false, down = false, left = false, right = false;
    bool accept = false, cancel = false;
    bool tabLeft = false, tabRight = false;
};

struct ScreenDef {
    const char* id;
    void (*Init)();                      // load textures/resources once
    void (*Draw)(double openSeconds);    // openSeconds = ui::Now() at the moment the screen opened
    void (*Input)(const ScreenInput&);   // optional (may be null): per-frame navigation/state
    void (*Reset)();                     // optional (may be null): re-init state when (re)opened
    // optional (may be null): poll-and-clear navigation request — the screen
    // returns a target screen id when the player activates something that leads
    // elsewhere in the runtime game's flow ("@back" = pop to the previous
    // screen). The host drives the measured wipe transition + switch.
    const char* (*Nav)();
    // optional (may be null): the real game CSD layout id (data/<id>.json). When
    // set, the host renders the actual game cast/animation as the BASE layer each
    // frame (csd_player) and the screen's Draw() composites its C++ overlay on
    // top — the maximally-1:1 path (real game files instead of hand-authoring).
    const char* csd = nullptr;
    // optional (may be null): returns the CSD cast STATE tag ("so"=day/Sonic,
    // "ev"=night/Werehog, ""=none) so day/night sub-states pick the right variant.
    const char* (*csdState)() = nullptr;
};

const ScreenDef*  FindScreen(const char* id);
const ScreenDef*  AllScreens(int& count);
