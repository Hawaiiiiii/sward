// =============================================================================
// audio.h — a tiny one-shot SFX mixer for the UI sounds, built directly on SDL2
// audio (no SDL_mixer dependency). It loads the REAL Sonic Unleashed / recomp UI
// sound effects (sys_actstg_pause* etc., 5.1ch 32-bit-float WAVs), downmixes them
// to stereo at load, and plays fire-and-forget voices mixed in the audio callback.
// Wired centrally from main_screens so every screen gets cursor/decide/cancel feedback.
// =============================================================================
#pragma once

namespace audio {

// Distinct per-action cues — the real game's UI sounds, one per action (not one
// sound for everything). Names are the game's own SFX bank entries.
enum Sfx {
    SFX_CURSOR = 0,   // sys_actstg_pausecursor       — cursor move (up/down/left/right)
    SFX_DECIDE,       // sys_actstg_pausedecide        — accept / confirm (A)
    SFX_CANCEL,       // sys_actstg_pausecansel        — cancel / back (B)
    SFX_WINOPEN,      // sys_actstg_pausewinopen       — a screen / window opens
    SFX_WINCLOSE,     // sys_actstg_pausewinclose      — a window closes
    SFX_TAB,          // sys_actstg_statescharachange  — category / form switch (LB/RB bumpers)
    SFX_LEVELUP,      // sys_actstg_stateslevup        — stat level-up
    SFX_ERROR,        // sys_actstg_stateserror        — denied / can't
    SFX_COUNT
};

bool Init();          // open the audio device + load clips from assets/sfx; false if unavailable
void Shutdown();
void Play(Sfx s);     // start a one-shot voice (no-op if audio is unavailable)

// Decode an OGG or WAV and loop it as background music. loopStartFrame > 0 loops
// the body (skipping the intro) — pass the track's CRI loop-start sample.
bool PlayMusic(const char* path, int loopStartFrame = 0);
void StopMusic();

} // namespace audio
