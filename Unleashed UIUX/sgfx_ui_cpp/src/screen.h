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
};

const ScreenDef*  FindScreen(const char* id);
const ScreenDef*  AllScreens(int& count);
